# PicoPDU — Claude Context

## Project
8-channel network PDU (power/device on-off) via MQTT + Home Assistant, on Raspberry Pi Pico 2W (RP2350).

- **Active branch:** `feature/freertos`
- **GPIO:** GP4–GP11 (8 channels), GP2/3 = LCD I2C1, GP0 = external blink LED (test)
- **MQTT broker:** Home Assistant Mosquitto at 192.168.0.2:1883, user=anon, pass=095033221
- **Architecture:** FreeRTOS + `pico_cyw43_arch_lwip_threadsafe_background` (NO_SYS=1)
- **FreeRTOS Kernel:** `C:/Users/Anon/FreeRTOS-Kernel` (cloned with --recurse-submodules)
- **Port:** `RP2350_ARM_NTZ` (Community-Supported-Ports/GCC/RP2350_ARM_NTZ)

## Build
```
cd build
cmake .. -G Ninja -DPICO_BOARD=pico2_w -DPython3_EXECUTABLE=C:/Users/Anon/.pico-sdk/python/3.13.7/python.exe
ninja
```
> Windows Store Python stub ใช้ไม่ได้กับ CMake ต้องชี้ไปที่ pico-sdk bundle เสมอ

## Task Structure (FreeRTOS)
- `wifi_task` (stack 4096) — cyw43 init → WiFi → MQTT → HA discovery → subscribe → main loop
- `blink_task` (stack 256) — GP0 blink 500ms ยืนยัน scheduler ทำงาน

## MQTT Topic Structure
- `pdu/ch{1-8}/set` ← รับ ON/OFF จาก HA
- `pdu/ch{1-8}/state` ← publish state กลับ (retain=true)
- `pdu/status` ← LWT: online/offline (retain=true)
- `homeassistant/switch/picopdu_ch{1-8}/config` ← HA MQTT Discovery

## FreeRTOS Pitfalls (RP2350 — อย่าลบ)

### 1. vTaskStartScheduler() ค้าง — ปัญหาที่ร้ายแรงที่สุด
RP2350 boot ใน **Secure mode** แต่ port พยายาม switch → Non-Secure → hang
```c
// FreeRTOSConfig.h — บังคับต้องมี
#define configRUN_FREERTOS_SECURE_ONLY  1
```

### 2. RP2350_ARM_NTZ port ต้องการ define ครบ 3 ตัว
```c
#define configENABLE_FPU        1
#define configENABLE_MPU        0
#define configENABLE_TRUSTZONE  0
```

### 3. SMP (2 cores) ทำให้ cyw43 spinlock deadlock
```c
#define configNUMBER_OF_CORES  1  // ห้ามใช้ 2 กับ cyw43
```

### 4. pico-sdk ใช้ชื่อ deprecated
```c
// FreeRTOSConfig.h
#define portTICK_RATE_MS  portTICK_PERIOD_MS
```

### 5. portCHECK_IF_IN_ISR — อย่า define ใน FreeRTOSConfig.h
portmacro.h ของ RP2350_ARM_NTZ define ไว้แล้ว — define ซ้ำจะได้ warning

### 6. CMakeLists.txt — ลำดับ include สำคัญ
```cmake
set(PICO_PLATFORM rp2350-arm-s)   # ต้องก่อน
include(pico_sdk_import.cmake)     # แล้วค่อย sdk
include(FreeRTOS_Kernel_import)    # สุดท้าย
```

### 7. Circular include — ห้าม #include pico/stdlib.h ใน FreeRTOSConfig.h
PICO_CONFIG_RTOS_ADAPTER_HEADER inject ไฟล์นี้ทุก translation unit → crash

## lwIP + FreeRTOS Pitfalls (อย่าลบ)

### 1. ใช้ `threadsafe_background` เท่านั้น — ไม่ใช่ `sys_freertos`
`pico_cyw43_arch_lwip_sys_freertos` → cyw43_arch_init() deadlock (async_context task ordering)
`pico_cyw43_arch_lwip_threadsafe_background` → ทำงานได้ (IRQ-based, NO_SYS=1)

### 2. ทุก lwIP call จาก FreeRTOS task ต้องครอบด้วย
```c
cyw43_arch_lwip_begin();
// ... lwIP / MQTT calls ...
cyw43_arch_lwip_end();
```
callback ที่ fire จาก IRQ (mqtt_conn_cb, incoming_data_cb) ไม่ต้องครอบ

### 3. MQTT ringbuf — ต้อง define ผ่าน CMakeLists.txt เท่านั้น
lwipopts.h ไม่ถูก pick up โดย mqtt.c:
```cmake
target_compile_definitions(PicoPDU PRIVATE
    MQTT_OUTPUT_RINGBUF_SIZE=8192
    MQTT_VAR_HEADER_BUFFER_LEN=512
    MQTT_REQ_MAX_IN_FLIGHT=20
)
```

### 4. MQTT keep_alive ต้องไม่เป็น 0
```c
.keep_alive = 60,  // ถ้า 0 lwIP ไม่ส่ง CONNECT packet
```

### 5. MEM_SIZE ต้องพอสำหรับ TCP
```c
// lwipopts.h
#define MEM_SIZE  16000  // default 4000 ไม่พอ
```

## LCD Response Lag — แก้ด้วย Task Notification
callback (IRQ context) → `vTaskNotifyGiveFromISR()` → ปลุก wifi_task ทันที
wifi_task รอด้วย `ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000))` แทน vTaskDelay
ผล: LCD อัปเดตภายใน <10ms หลัง HA toggle (เดิม ~1000ms)
