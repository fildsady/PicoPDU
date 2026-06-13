# PicoPDU

ระบบควบคุมเปิด/ปิดอุปกรณ์ผ่านเครือข่าย บน **Raspberry Pi Pico 2W (RP2350)**
รองรับทั้ง **WiFi** และ **Ethernet** — ควบคุมผ่าน Web UI, REST API หรือ MQTT

---

## สถาปัตยกรรม

- **Core 0** — application loop: relay control, GPIO, serial command
- **Core 1** — CYW43 WiFi poll + lwIP HTTP server (bare-metal, ไม่ใช้ RTOS)
- **Ethernet** — รองรับ W5500 / ENC28J60 ผ่าน SPI (planned)
- **Web UI** — dark-theme responsive, AJAX real-time, ใช้งานได้บนมือถือ / PC

---

## Board Support

| Board | LED | Network |
|-------|-----|---------|
| Pico 2W (`pico2_w`) | CYW43 via `cyw43_arch_gpio_put` | WiFi (CYW43439) |
| Pico 2 (`pico2`) | GPIO 25 | Ethernet module (SPI) |

---

## Build

**ต้องการ:** Pico SDK 2.2.0, ARM GCC 15.2, CMake, Ninja

```bash
cd build
cmake .. -G Ninja -DPICO_BOARD=pico2_w \
  -DPython3_EXECUTABLE=$HOME/.pico-sdk/python/3.13.7/python.exe
ninja
```

> **Windows:** Python3 ต้องชี้ไปที่ `%USERPROFILE%\.pico-sdk\python\3.13.7\python.exe`
> (Windows Store stub ใช้ไม่ได้กับ CMake)

ไฟล์ `build/PicoPDU.uf2` — กด BOOTSEL แล้วลาก drop ลงบอร์ดได้เลย

---

## โครงสร้างไฟล์

```
PicoPDU/
├── src/
│   └── main.c               # LED blink template (starting point)
├── lib/
│   └── FreeRTOS-Kernel/     # submodule — พร้อมใช้เมื่อต้องการ RTOS
├── CMakeLists.txt
├── CMakePresets.json        # preset: pico2w-release
└── pico_sdk_import.cmake
```

---

## Branches

| Branch | คำอธิบาย |
|--------|----------|
| `main` | **branch นี้** — LED blink template, starting point สำหรับพัฒนา |
| `feature/lwip-web-control` | bare-metal multicore + lwIP Web UI (ตัวอย่าง WiFi) |
| `feature/dfplayer-uart-control-demo` | ตัวอย่าง UART + LCD (FreeRTOS) |
| `Blink_Test` | LED blink ทดสอบบอร์ด |

---

## FreeRTOS

FreeRTOS-Kernel submodule พร้อมอยู่แล้วใน `lib/` เปิดใช้โดย uncomment ใน `CMakeLists.txt`:

```cmake
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)
```

และเปลี่ยน link library จาก `pico_cyw43_arch_none` เป็น `pico_cyw43_arch_lwip_sys_freertos`

---

## Roadmap

- [ ] Relay output control (GPIO)
- [ ] Web UI — outlet on/off panel
- [ ] REST API — `GET /relay/1/on`
- [ ] Ethernet support (W5500 SPI)
- [ ] MQTT client
- [ ] Schedule / timer
- [ ] Current sensing (ADC)
