# PicoPDU — Claude Context

## Project
8-channel network PDU (power/device on-off) via MQTT + Home Assistant, on Raspberry Pi Pico 2W (RP2350).

- **Active branch:** `feature/mqtt-pdu` (LED simulation demo)
- **GPIO:** GP4–GP11 (8 channels), GP2/3 reserved for LCD I2C
- **MQTT broker:** Home Assistant Mosquitto at 192.168.0.2:1883, user=anon
- **Architecture:** bare-metal poll mode (`pico_cyw43_arch_lwip_poll`), no RTOS

## Build
```
cd build
cmake .. -G Ninja -DPICO_BOARD=pico2_w -DPython3_EXECUTABLE=C:/Users/Anon/.pico-sdk/python/3.13.7/python.exe
ninja
```
> Windows Store Python stub ใช้ไม่ได้กับ CMake ต้องชี้ไปที่ pico-sdk bundle เสมอ

## MQTT Topic Structure
- `pdu/ch{1-8}/set` ← รับ ON/OFF
- `pdu/ch{1-8}/state` ← publish state กลับ (retain=true)
- `pdu/status` ← LWT: online/offline (retain=true)
- `homeassistant/switch/picopdu_ch{1-8}/config` ← HA MQTT Discovery

## lwIP MQTT Pitfalls (แก้แล้วทั้งหมด — อย่าลบออก)

### 1. PANIC: sys_timeout pool empty
```c
// lwipopts.h
#define MEMP_NUM_SYS_TIMEOUT 16
```

### 2. MQTT CONNECT ไม่ถูกส่ง — broker timeout หลัง 30s
```c
// ต้องตั้ง keep_alive เสมอ ถ้า 0 lwIP ไม่ส่ง CONNECT packet
struct mqtt_connect_client_info_t ci = {
    .keep_alive = 60,
    ...
};
```

### 3. TCP ต่อไม่ได้ — MEM_SIZE น้อยเกินไป
```c
// lwipopts.h — default 4000 ไม่พอสำหรับ TCP send buffer
#define MEM_SIZE     16000
#define TCP_MSS      536
#define TCP_SND_BUF  (2 * TCP_MSS)
```

### 4. mqtt_publish return ERR_MEM — MQTT ringbuf เล็กเกินไป
lwipopts.h **ไม่ถูก pick up โดย mqtt.c** ต้อง define ผ่าน CMakeLists.txt เท่านั้น:
```cmake
target_compile_definitions(PicoPDU PRIVATE
    MQTT_OUTPUT_RINGBUF_SIZE=8192
    MQTT_VAR_HEADER_BUFFER_LEN=512
    MQTT_REQ_MAX_IN_FLIGHT=20
)
```

### 5. Discovery ERR_MEM — MQTT_REQ_MAX_IN_FLIGHT เต็ม
default=4 ไม่พอสำหรับ 8 subscribe + 8 publish รัวๆ → ใช้ 20 (ดูข้อ 4)
