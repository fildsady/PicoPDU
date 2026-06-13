#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "arm_math.h"

// ---------------------------------------------------------------------------
// DSP Example — RP2350 Cortex-M33 + CMSIS-DSP (hardware DSP instructions)
//
// Demo: เปรียบเทียบ FIR Low-pass filter 2 แบบ
//   [A] float  — ใช้ FPU (VMLA)
//   [B] Q15    — ใช้ DSP SIMD instruction จริง (SMLAD: dual 16-bit MAC in 1 cycle)
//
// Signal: sine 1kHz (passband) + sine 5kHz (stopband)
// Fs = 10kHz, cutoff = 2kHz, FIR 31-tap
// ---------------------------------------------------------------------------

#define SAMPLE_RATE  10000
#define BLOCK_SIZE   256
#define FIR_TAPS     32   // multiple of 4 — required for Q15 DSP SIMD path

// FIR coefficients (float) — firwin(31, 2000/5000), zero-padded to 32
static const float32_t fir_f32[FIR_TAPS] = {
    -0.00126f, -0.00198f, -0.00178f,  0.00000f,  0.00446f,
     0.00981f,  0.01475f,  0.01699f,  0.01462f,  0.00654f,
    -0.00629f, -0.02220f, -0.03858f, -0.05141f, -0.05668f,
     0.94000f, -0.05668f, -0.05141f, -0.03858f, -0.02220f,
    -0.00629f,  0.00654f,  0.01462f,  0.01699f,  0.01475f,
     0.00981f,  0.00446f,  0.00000f, -0.00178f, -0.00198f,
    -0.00126f,  0.00000f,  // zero-pad
};

// Q15 coefficients — แปลงจาก float (range -1.0 to +0.9999)
// Q15 = float * 32768, clamp to [-32768, 32767]
static q15_t fir_q15[FIR_TAPS];

// buffers
static float32_t in_f32[BLOCK_SIZE];
static float32_t out_f32[BLOCK_SIZE];
static q15_t     in_q15[BLOCK_SIZE];
static q15_t     out_q15[BLOCK_SIZE];

// CMSIS-DSP state buffers — ต้อง 4-byte aligned สำหรับ SIMD
static float32_t state_f32[FIR_TAPS + BLOCK_SIZE - 1] __attribute__((aligned(4)));
static q15_t     state_q15[FIR_TAPS + BLOCK_SIZE - 1] __attribute__((aligned(4)));

// CMSIS-DSP instance structs
static arm_fir_instance_f32 fir_inst_f32;
static arm_fir_instance_q15 fir_inst_q15;

// ---------------------------------------------------------------------------
// สร้าง signal: sine 1kHz + 0.5 * sine 5kHz
// ---------------------------------------------------------------------------
static void gen_signal_f32(float32_t *buf, int len) {
    static float phase1 = 0.0f, phase2 = 0.0f;
    const float dt = 1.0f / SAMPLE_RATE;
    for (int i = 0; i < len; i++) {
        buf[i] = sinf(2.0f * (float)M_PI * 1000.0f * phase1)
               + 0.5f * sinf(2.0f * (float)M_PI * 5000.0f * phase2);
        phase1 += dt; if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += dt; if (phase2 >= 1.0f) phase2 -= 1.0f;
    }
}

// ---------------------------------------------------------------------------
// RMS ของ signal
// ---------------------------------------------------------------------------
static float rms_f32(const float32_t *buf, int len) {
    float sum = 0.0f;
    for (int i = 0; i < len; i++) sum += buf[i] * buf[i];
    return sqrtf(sum / len);
}

static float rms_q15(const q15_t *buf, int len) {
    float sum = 0.0f;
    for (int i = 0; i < len; i++) {
        float v = buf[i] / 32768.0f;
        sum += v * v;
    }
    return sqrtf(sum / len);
}

// HardFault handler — print PC, LR, CFSR จาก exception frame
void HardFault_Handler(void) __attribute__((naked));
void HardFault_Handler(void) {
    __asm volatile(
        "tst lr, #4        \n"
        "ite eq            \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "b HardFault_dump  \n"
    );
}

void HardFault_dump(uint32_t *frame) {
    uint32_t pc  = frame[6];
    uint32_t lr  = frame[5];
    uint32_t cfsr = *(volatile uint32_t *)0xE000ED28;
    printf("\n*** HARD FAULT ***\n");
    printf("PC=0x%08lX  LR=0x%08lX\n", pc, lr);
    printf("CFSR=0x%08lX\n", cfsr);
    printf("  IBUSERR=%lu PRECISERR=%lu IMPRECISERR=%lu\n",
           (cfsr>>8)&1, (cfsr>>9)&1, (cfsr>>10)&1);
    while(1);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\n=== RP2350 CMSIS-DSP Example ===\n");
    printf("FIR LPF 31-tap, Fs=10kHz, cutoff=2kHz\n");
    printf("Signal: 1kHz (passband) + 5kHz (stopband)\n\n");

    // แปลง float coeffs → Q15
    for (int i = 0; i < FIR_TAPS; i++) {
        float v = fir_f32[i] * 32768.0f;
        if (v >  32767.0f) v =  32767.0f;
        if (v < -32768.0f) v = -32768.0f;
        fir_q15[i] = (q15_t)v;
    }

#define DBG(msg) do { printf(msg "\n"); sleep_ms(50); } while(0)

    // test simple CMSIS-DSP function ก่อน
    DBG("test arm_scale_f32...");
    arm_scale_f32(in_f32, 1.0f, out_f32, BLOCK_SIZE);
    DBG("arm_scale_f32 OK");

    float32_t maxval; uint32_t maxidx;
    arm_max_f32(in_f32, BLOCK_SIZE, &maxval, &maxidx);
    printf("arm_max_f32 OK: max=%.4f\n", (double)maxval);
    sleep_ms(50);

    // init CMSIS-DSP instances
    DBG("init f32...");
    arm_fir_init_f32(&fir_inst_f32, FIR_TAPS, fir_f32, state_f32, BLOCK_SIZE);
    DBG("init q15...");
    arm_fir_init_q15(&fir_inst_q15, FIR_TAPS, fir_q15, state_q15, BLOCK_SIZE);

    // warm up
    DBG("gen signal...");
    gen_signal_f32(in_f32, BLOCK_SIZE);
    DBG("float_to_q15...");
    arm_float_to_q15(in_f32, in_q15, BLOCK_SIZE);
    DBG("fir_f32...");
    arm_fir_f32(&fir_inst_f32, in_f32, out_f32, BLOCK_SIZE);
    DBG("fir_q15...");
    arm_fir_q15(&fir_inst_q15, in_q15, out_q15, BLOCK_SIZE);
    DBG("warm up done");

    const int ITER = 1000;

    // pre-generate ก่อน benchmark เพื่อไม่ให้ sinf distort ผลวัด
    gen_signal_f32(in_f32, BLOCK_SIZE);
    arm_float_to_q15(in_f32, in_q15, BLOCK_SIZE);

    // ---- Benchmark A: float (FPU / VMLA) ----
    uint64_t t0 = time_us_64();
    for (int i = 0; i < ITER; i++) {
        arm_fir_f32(&fir_inst_f32, in_f32, out_f32, BLOCK_SIZE);
    }
    uint64_t t1 = time_us_64();
    float us_f32 = (float)(t1 - t0) / ITER;

    // ---- Benchmark B: Q15 (DSP SIMD / SMLAD) ----
    uint64_t t2 = time_us_64();
    for (int i = 0; i < ITER; i++) {
        arm_fir_q15(&fir_inst_q15, in_q15, out_q15, BLOCK_SIZE);
    }
    uint64_t t3 = time_us_64();
    float us_q15 = (float)(t3 - t2) / ITER;

    printf("--- Benchmark (%d iter x %d samples) ---\n", ITER, BLOCK_SIZE);
    printf("[A] arm_fir_f32 (FPU/VMLA)  : %7.2f us/block  %5.3f us/sample\n",
           (double)us_f32, (double)(us_f32 / BLOCK_SIZE));
    printf("[B] arm_fir_q15 (DSP/SMLAD) : %7.2f us/block  %5.3f us/sample\n",
           (double)us_q15, (double)(us_q15 / BLOCK_SIZE));
    printf("Speedup Q15 vs f32: %.2fx\n\n",
           (double)(us_f32 / us_q15));

    // ---- Signal quality ----
    gen_signal_f32(in_f32, BLOCK_SIZE);
    arm_float_to_q15(in_f32, in_q15, BLOCK_SIZE);
    float rms_in = rms_f32(in_f32, BLOCK_SIZE);

    arm_fir_f32(&fir_inst_f32, in_f32, out_f32, BLOCK_SIZE);
    arm_fir_q15(&fir_inst_q15, in_q15, out_q15, BLOCK_SIZE);

    float rms_out_f32 = rms_f32(out_f32, BLOCK_SIZE);
    float rms_out_q15 = rms_q15(out_q15, BLOCK_SIZE);

    printf("--- Signal Attenuation ---\n");
    printf("RMS input        : %.4f\n", (double)rms_in);
    printf("[A] f32 output   : %.4f  (%+.1f dB)\n",
           (double)rms_out_f32,
           (double)(20.0f * log10f(rms_out_f32 / rms_in)));
    printf("[B] Q15 output   : %.4f  (%+.1f dB)\n",
           (double)rms_out_q15,
           (double)(20.0f * log10f(rms_out_q15 / rms_in)));

    printf("\nDone.\n");

    // blink
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    while (true) {
        gpio_put(25, 1); sleep_ms(200);
        gpio_put(25, 0); sleep_ms(200);
    }
}
