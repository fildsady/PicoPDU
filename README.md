# MP3 Control Project

ระบบควบคุม DFPlayer Mini ผ่าน Serial command บน Raspberry Pi Pico 2 (RP2350) + FreeRTOS

---

## Hardware

| อุปกรณ์ | รายละเอียด |
|---------|------------|
| MCU | Raspberry Pi Pico 2 (RP2350) |
| MP3 Module | DFPlayer Mini |
| การเชื่อมต่อ | UART0 — GP0 (TX) → DFPlayer RX, GP1 (RX) → DFPlayer TX |
| Interface | USB CDC (Serial Monitor) สำหรับรับคำสั่งจาก PC |

```
Pico 2
 GP0 (TX) ──────────────→ RX   DFPlayer Mini
 GP1 (RX) ←────────────── TX
 GND      ─────────────── GND
 3.3V/5V  ─────────────── VCC
                           SPK+ → ลำโพง
                           SPK- → ลำโพง
```

---

## คำสั่งที่ใช้ได้ (Serial Commands)

พิมพ์คำสั่งแล้วกด Enter ผ่าน Serial Monitor (115200 baud, USB)

| คำสั่ง | ผลลัพธ์ |
|--------|---------|
| `play <n>` | เล่น track ที่ n (1–99) |
| `stop` | หยุดเล่น |
| `next` | track ถัดไป |
| `prev` | track ก่อนหน้า |
| `vol <n>` | ตั้ง volume (0–30) |
| `repeat all` | วนเล่นทุก track |
| `repeat one` | วนเล่น track ปัจจุบัน |
| `repeat off` | ปิด repeat |

ตัวอย่าง:
```
> play 1
OK: playing track 1
> vol 20
OK: volume set to 20
> repeat all
OK: repeat all
> stop
OK: stopped
> xyz
ERR: unknown command
```

---

## FreeRTOS Task Structure

```
┌─────────────────┐     Queue (command_t)     ┌──────────────────┐
│  task_uart_rx   │ ─────────────────────────→ │  task_dfplayer   │
│  (priority 2)   │                            │  (priority 1)    │
│                 │                            │                  │
│ รับ char จาก    │                            │ ส่ง byte frame   │
│ USB CDC         │                            │ ไป DFPlayer Mini │
│ parse command   │                            │ via UART0        │
└─────────────────┘                            └──────────────────┘

┌──────────────────┐
│  task_status_led │
│  (priority 1)    │
│                  │
│ blink GP25 LED   │
│ ทุก 250ms        │
│ (บอร์ดยังทำงาน) │
└──────────────────┘
```

| Task | Priority | Stack | หน้าที่ |
|------|----------|-------|---------|
| `task_uart_rx` | 2 | 512 words | รับ command จาก USB, parse, ส่ง Queue |
| `task_dfplayer` | 1 | 512 words | รับจาก Queue, ขับ DFPlayer ผ่าน UART0 |
| `task_status_led` | 1 | 256 words | blink LED GP25 ทุก 250ms |

---

## โครงสร้างไฟล์

```
MP3 Control Project/
├── inc/
│   ├── FreeRTOSConfig.h     # ตั้งค่า FreeRTOS kernel
│   └── dfplayer.h           # DFPlayer Mini API
├── lib/
│   └── FreeRTOS-Kernel/     # FreeRTOS submodule
├── src/
│   ├── main.c               # Tasks, Queue, command parser
│   └── dfplayer.c           # UART0 driver สำหรับ DFPlayer Mini
├── CMakeLists.txt
└── pico_sdk_import.cmake
```

---

## Build

**ต้องการ:** Pico SDK 2.2.0, ARM GCC 15.2, CMake, Ninja

```powershell
$cmake   = "C:\Users\<user>\.pico-sdk\cmake\v3.31.5\bin\cmake.exe"
$ninja   = "C:\Users\<user>\.pico-sdk\ninja\v1.12.1\ninja.exe"
$sdk     = "C:\Users\<user>\.pico-sdk\sdk\2.2.0"
$project = "<path>\MP3 Control Project"

& $cmake -S "$project" -B "$project\build" `
  -DPICO_SDK_PATH="$sdk" -DPICO_BOARD=pico2 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MAKE_PROGRAM="$ninja" -G Ninja

& $ninja -C "$project\build"
```

ไฟล์ `.uf2` จะอยู่ที่ `build/MP3_Control_Project.uf2` — กด BOOTSEL แล้วลาก drop ลงบอร์ดได้เลย

---

## Branches

| Branch | คำอธิบาย |
|--------|----------|
| `main` | พัฒนาหลัก |
| `feature/dfplayer-uart-control-demo` | Demo DFPlayer Mini ผ่าน Serial command |
| `Blink_Test` | LED blink ทดสอบบอร์ด |
