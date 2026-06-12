# MP3 Control Project

ระบบควบคุม DFPlayer Mini ผ่าน Serial command บน Raspberry Pi Pico 2 (RP2350) + FreeRTOS  
แสดงสถานะผ่าน LCD 16x2 I2C และ LED blink

---

## Hardware

| อุปกรณ์ | รายละเอียด |
|---------|------------|
| MCU | Raspberry Pi Pico 2 (RP2350) |
| MP3 Module | DFPlayer Mini |
| Display | LCD 16x2 I2C (PCF8574, address 0x27) |
| Interface | USB CDC (Serial Monitor) รับคำสั่งจาก PC |

### Wiring

```
Pico 2                          DFPlayer Mini
 GP0  (TX)  ─────────────────→  RX
 GP1  (RX)  ←─────────────────  TX
 GP15 (IN)  ←─────────────────  BUSY  (LOW = playing, HIGH = idle)
 GND        ─────────────────── GND
 3.3V/5V    ─────────────────── VCC
                                 SPK+ → ลำโพง
                                 SPK- → ลำโพง

Pico 2                          LCD 16x2 I2C
 GP2  (SDA) ─────────────────→  SDA
 GP3  (SCL) ─────────────────→  SCL
 GND        ─────────────────── GND
 3.3V/5V    ─────────────────── VCC

Pico 2                          Buttons (active LOW)
 GP20 (IN)  ────── BTN ── GND   Prev track
 GP21 (IN)  ────── BTN ── GND   Next track
 GP22 (IN)  ────── BTN ── GND   Toggle Play/Stop
```

---

## Features

- รับคำสั่ง text ผ่าน USB Serial — ควบคุมได้จาก PC
- ปุ่มกดจริง GP20/GP21/GP22 — prev / next / toggle play/stop
- ควบคุม play / stop / next / prev / volume / repeat
- แสดงสถานะ track และ volume บน LCD 16x2 แบบ real-time
- BUSY pin monitor — แจ้งทาง Serial เมื่อเริ่มเล่นและจบ
- Status LED blink GP25 บอกว่าบอร์ดยังทำงานอยู่
- Repeat mode filter — ไม่ spam Serial ระหว่าง loop

---

## Serial Commands

พิมพ์คำสั่งแล้วกด Enter ผ่าน Serial Monitor (baud ใดก็ได้, USB CDC)

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
INFO: playing
INFO: track finished
> vol 20
OK: volume set to 20
> repeat all
OK: repeat all
> stop
OK: stopped
> play 99
ERR: track out of range (usage: play <1-99>)
> xyz
ERR: unknown command
```

---

## LCD Display

```
┌────────────────┐
│ Track: 1       │
│ Vol:20 Playing │
└────────────────┘
```

อัปเดตทุก 500ms แสดง track ปัจจุบัน, volume, และสถานะ Playing/Stopped

---

## FreeRTOS Task Structure

```mermaid
flowchart TD
    BOOT([Boot / main]) --> Q[xQueueCreate]
    Q --> T1 & T2 & T3 & T4 & T5

    subgraph T1["task_uart_rx  •  priority 2"]
        direction TB
        A1[getchar_timeout_us\n10ms poll] --> A2{newline?}
        A2 -- yes --> A3[parse_and_enqueue]
        A2 -- no --> A4[buffer char]
        A4 --> A1
        A3 --> A1
    end

    subgraph T2["task_dfplayer  •  priority 1"]
        direction TB
        B0[dfplayer_init\nvol 20] --> B1[xQueueReceive\n200ms timeout]
        B1 --> B2{cmd received?}
        B2 -- yes --> B3[switch cmd\ndrive DFPlayer]
        B3 --> B4[update lcd_state]
        B4 --> B5[check BUSY pin]
        B2 -- no --> B5
        B5 --> B1
    end

    subgraph T3["task_lcd  •  priority 1"]
        direction TB
        C0[lcd_init] --> C1[read lcd_state]
        C1 --> C2[lcd_buff_printf]
        C2 --> C3[put_buff_to_lcd]
        C3 --> C4[vTaskDelay 500ms]
        C4 --> C1
    end

    subgraph T4["task_status_led  •  priority 1"]
        direction TB
        D0[gpio_init GP25] --> D1[LED ON]
        D1 --> D2[vTaskDelay 250ms]
        D2 --> D3[LED OFF]
        D3 --> D4[vTaskDelay 250ms]
        D4 --> D1
    end

    subgraph T5["task_buttons  •  priority 1"]
        direction TB
        E0[gpio_init GP20/21/22\npull-up] --> E1[vTaskDelay 50ms]
        E1 --> E2{falling edge?}
        E2 -- prev --> E3[CMD_PREV]
        E2 -- next --> E4[CMD_NEXT]
        E2 -- play btn --> E5{playing?}
        E5 -- yes --> E6[CMD_STOP]
        E5 -- no --> E7[CMD_PLAY]
        E3 & E4 & E6 & E7 --> E1
        E2 -- none --> E1
    end

    A3 -- "cmd_queue\ncommand_t" --> B1
    E3 & E4 & E6 & E7 -- "cmd_queue\ncommand_t" --> B1
    B4 -- "lcd_state\nvolatile struct" --> C1
```

| Task | Priority | Stack | หน้าที่ |
|------|----------|-------|---------|
| `task_uart_rx` | 2 | 512 words | รับ command จาก USB, parse, ส่ง Queue |
| `task_dfplayer` | 1 | 512 words | ขับ DFPlayer, monitor BUSY pin, อัปเดต lcd_state |
| `task_lcd` | 1 | 512 words | อ่าน lcd_state แล้วแสดงบน LCD ทุก 500ms |
| `task_status_led` | 1 | 256 words | blink LED GP25 ทุก 250ms |
| `task_buttons` | 1 | 256 words | poll GP20/21/22 ทุก 50ms, debounce, ส่ง Queue |

---

## โครงสร้างไฟล์

```
MP3 Control Project/
├── inc/
│   ├── FreeRTOSConfig.h     # ตั้งค่า FreeRTOS kernel
│   ├── dfplayer.h           # DFPlayer Mini API
│   └── i2c_lcd.h            # LCD 16x2 I2C API
├── lib/
│   └── FreeRTOS-Kernel/     # FreeRTOS submodule
├── src/
│   ├── main.c               # Tasks, Queue, command parser, lcd_state
│   ├── dfplayer.c           # UART0 driver สำหรับ DFPlayer Mini
│   └── i2c_lcd.c            # I2C driver สำหรับ LCD 16x2 (PCF8574)
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
| `feature/dfplayer-uart-control-demo` | Demo DFPlayer Mini + LCD |
| `Blink_Test` | LED blink ทดสอบบอร์ด |
