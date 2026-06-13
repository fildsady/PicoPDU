#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"

// ---------------------------------------------------------------------------
// DSP Example — RP2350 Cortex-M33 FPU + DSP instructions
//
// Demo: FIR Low-pass filter
//   - สร้าง signal = sine 1kHz + sine 5kHz (noise)
//   - กรองด้วย FIR LPF 31-tap ตัด > 2kHz
//   - วัด execution time เปรียบเทียบ float vs เร็วแค่ไหน
//   - print ผลออก UART ดูด้วย serial monitor
// ---------------------------------------------------------------------------

#define SAMPLE_RATE     10000   // 10kHz
#define BLOCK_SIZE      256     // samples per block
#define FIR_TAPS        31      // จำนวน tap ของ FIR filter

// FIR Low-pass filter coefficients (cutoff 2kHz @ 10kHz Fs)
// generated with: scipy.signal.firwin(31, 2000/5000)
static const float fir_coeffs[FIR_TAPS] = {
    -0.00126f, -0.00198f, -0.00178f,  0.00000f,  0.00446f,
     0.00981f,  0.01475f,  0.01699f,  0.01462f,  0.00654f,
    -0.00629f, -0.02220f, -0.03858f, -0.05141f, -0.05668f,
     0.94000f, -0.05668f, -0.05141f, -0.03858f, -0.02220f,
    -0.00629f,  0.00654f,  0.01462f,  0.01699f,  0.01475f,
     0.00981f,  0.00446f,  0.00000f, -0.00178f, -0.00198f,
    -0.00126f,
};

// FIR state buffer (delay line)
static float fir_state[FIR_TAPS - 1];

// input/output buffers
static float input_buf[BLOCK_SIZE];
static float output_buf[BLOCK_SIZE];

// ---------------------------------------------------------------------------
// FIR filter — direct form, float
// ใช้ FPU ของ M33 โดยอัตโนมัติเมื่อ compile -mfpu=fpv5-sp-d16 -mfloat-abi=hard
// ---------------------------------------------------------------------------
static void fir_filter(const float *in, float *out, int len) {
    static float delay[FIR_TAPS] = {0};

    for (int n = 0; n < len; n++) {
        // shift delay line
        memmove(&delay[1], &delay[0], (FIR_TAPS - 1) * sizeof(float));
        delay[0] = in[n];

        // MAC accumulate
        float acc = 0.0f;
        for (int k = 0; k < FIR_TAPS; k++) {
            acc += fir_coeffs[k] * delay[k];
        }
        out[n] = acc;
    }
}

// ---------------------------------------------------------------------------
// สร้าง test signal: sine 1kHz + sine 5kHz (เหนือ cutoff)
// ---------------------------------------------------------------------------
static void gen_signal(float *buf, int len) {
    static float phase1 = 0.0f;
    static float phase2 = 0.0f;
    const float f1 = 1000.0f;  // 1kHz — อยู่ใน passband
    const float f2 = 5000.0f;  // 5kHz — อยู่เหนือ cutoff ควรถูกกรอง
    const float dt = 1.0f / SAMPLE_RATE;

    for (int i = 0; i < len; i++) {
        buf[i] = sinf(2.0f * (float)M_PI * f1 * phase1)
               + 0.5f * sinf(2.0f * (float)M_PI * f2 * phase2);
        phase1 += dt;
        phase2 += dt;
        if (phase1 >= 1.0f) phase1 -= 1.0f;
        if (phase2 >= 1.0f) phase2 -= 1.0f;
    }
}

// ---------------------------------------------------------------------------
// วัด amplitude ของ signal (RMS)
// ---------------------------------------------------------------------------
static float rms(const float *buf, int len) {
    float sum = 0.0f;
    for (int i = 0; i < len; i++)
        sum += buf[i] * buf[i];
    return sqrtf(sum / len);
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);  // รอ serial terminal เปิด

    printf("\n=== RP2350 DSP Example ===\n");
    printf("FIR Low-Pass Filter Demo\n");
    printf("Sample rate : %d Hz\n", SAMPLE_RATE);
    printf("FIR taps    : %d\n", FIR_TAPS);
    printf("Cutoff      : 2000 Hz\n");
    printf("Signal      : 1kHz (passband) + 5kHz (stopband)\n\n");

    // warm up (cache, branch predictor)
    gen_signal(input_buf, BLOCK_SIZE);
    fir_filter(input_buf, output_buf, BLOCK_SIZE);

    // ---- benchmark ----
    const int ITER = 1000;
    uint64_t t0 = time_us_64();
    for (int i = 0; i < ITER; i++) {
        gen_signal(input_buf, BLOCK_SIZE);
        fir_filter(input_buf, output_buf, BLOCK_SIZE);
    }
    uint64_t t1 = time_us_64();

    float us_per_block = (float)(t1 - t0) / ITER;
    float us_per_sample = us_per_block / BLOCK_SIZE;
    float cpu_load = (us_per_sample / (1e6f / SAMPLE_RATE)) * 100.0f;

    printf("--- Benchmark (%d iterations x %d samples) ---\n", ITER, BLOCK_SIZE);
    printf("Time per block  : %.2f us\n", (double)us_per_block);
    printf("Time per sample : %.3f us\n", (double)us_per_sample);
    printf("CPU load        : %.1f %%\n\n", (double)cpu_load);

    // ---- signal analysis ----
    gen_signal(input_buf, BLOCK_SIZE);
    float rms_in = rms(input_buf, BLOCK_SIZE);

    fir_filter(input_buf, output_buf, BLOCK_SIZE);
    float rms_out = rms(output_buf, BLOCK_SIZE);

    printf("--- Signal Level ---\n");
    printf("RMS input  : %.4f (1kHz + 5kHz)\n", (double)rms_in);
    printf("RMS output : %.4f (after LPF)\n", (double)rms_out);
    printf("Attenuation: %.1f dB\n\n",
           (double)(20.0f * log10f(rms_out / rms_in)));

    // ---- print sample ----
    printf("--- First 10 samples (in → out) ---\n");
    gen_signal(input_buf, BLOCK_SIZE);
    fir_filter(input_buf, output_buf, BLOCK_SIZE);
    for (int i = 20; i < 30; i++) {
        printf("  [%3d] in=%7.4f  out=%7.4f\n", i,
               (double)input_buf[i], (double)output_buf[i]);
    }

    printf("\nDone.\n");

    // blink ยืนยัน
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    while (true) {
        gpio_put(25, 1); sleep_ms(500);
        gpio_put(25, 0); sleep_ms(500);
    }
}
