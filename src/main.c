#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "dfplayer.h"
#include "i2c_lcd.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#define CMD_QUEUE_LEN   8   // max pending commands waiting to execute
#define LINE_BUF_SIZE   64  // max length of one command line
#define DEFAULT_VOLUME  20  // startup volume (0-30)

#define BTN_PREV_PIN    20  // GP20 — select prev track
#define BTN_NEXT_PIN    21  // GP21 — select next track
#define BTN_PLAY_PIN    22  // GP22 — toggle play/stop
#define MAX_TRACK       99  // highest track number on SD card

// command types received from Serial
typedef enum {
    CMD_PLAY,
    CMD_STOP,
    CMD_NEXT,
    CMD_PREV,
    CMD_VOLUME,
    CMD_REPEAT_ALL,
    CMD_REPEAT_ONE,
    CMD_REPEAT_OFF,
    CMD_UNKNOWN,
} cmd_type_t;

// passed through Queue from task_uart_rx → task_dfplayer
typedef struct {
    cmd_type_t type;
    int        arg;  // track number or volume depending on type
} command_t;

static QueueHandle_t cmd_queue;

typedef enum { UI_TRACK, UI_VOLUME } ui_mode_t;

// shared LCD state — written by task_dfplayer / task_buttons, read by task_lcd
typedef struct {
    int       track;
    int       volume;
    bool      playing;
    ui_mode_t mode;
} lcd_state_t;

static volatile lcd_state_t lcd_state = { .track = 0, .volume = DEFAULT_VOLUME, .playing = false, .mode = UI_TRACK };

// parse text command into command_t and push to Queue
static void parse_and_enqueue(const char *line) {
    command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };
    int n;

    if (sscanf(line, "play %d", &n) == 1) {
        if (n < 1 || n > 99) {
            printf("ERR: track out of range (usage: play <1-99>)\r\n");
            return;
        }
        cmd.type = CMD_PLAY;
        cmd.arg  = n;
    } else if (strcmp(line, "stop") == 0) {
        cmd.type = CMD_STOP;
    } else if (strcmp(line, "next") == 0) {
        cmd.type = CMD_NEXT;
    } else if (strcmp(line, "prev") == 0) {
        cmd.type = CMD_PREV;
    } else if (sscanf(line, "vol %d", &n) == 1) {
        if (n < 0 || n > 30) {
            printf("ERR: volume out of range (usage: vol <0-30>)\r\n");
            return;
        }
        cmd.type = CMD_VOLUME;
        cmd.arg  = n;
    } else if (strcmp(line, "repeat all") == 0) {
        cmd.type = CMD_REPEAT_ALL;
    } else if (strcmp(line, "repeat one") == 0) {
        cmd.type = CMD_REPEAT_ONE;
    } else if (strcmp(line, "repeat off") == 0) {
        cmd.type = CMD_REPEAT_OFF;
    } else {
        printf("ERR: unknown command\r\n");
        return;
    }

    xQueueSend(cmd_queue, &cmd, 0);
}

// Task: update LCD every 500ms with current track, volume, and play state
static void task_lcd(void *pvParameters) {
    (void)pvParameters;
    lcd_init();
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "MP3 Controller");
    lcd_buff_printf(1, 0, "Ready");
    put_buff_to_lcd();

    while (1) {
        lcd_clear_buff_all();
        if (lcd_state.mode == UI_VOLUME) {
            lcd_buff_printf(0, 0, "[ Vol Mode ]");
            lcd_buff_printf(1, 0, "< Vol: %d >", lcd_state.volume);
        } else {
            if (lcd_state.track > 0)
                lcd_buff_printf(0, 0, "Track: %d", lcd_state.track);
            else
                lcd_buff_printf(0, 0, "MP3 Controller");

            if (lcd_state.playing)
                lcd_buff_printf(1, 0, "Vol:%d Playing", lcd_state.volume);
            else
                lcd_buff_printf(1, 0, "Vol:%d Stopped", lcd_state.volume);
        }
        put_buff_to_lcd();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task: blink GP25 LED every 250ms — indicates board is alive
static void task_status_led(void *pvParameters) {
    (void)pvParameters;
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (1) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// Task: read chars from USB Serial, buffer until newline, then parse
static void task_uart_rx(void *pvParameters) {
    (void)pvParameters;
    char buf[LINE_BUF_SIZE];
    int  idx = 0;

    while (1) {
        int c = getchar_timeout_us(10000); // poll every 10ms — avoids blocking scheduler
        if (c == PICO_ERROR_TIMEOUT) continue;
        if (c == '\r') continue;

        if (c == '\n') {
            buf[idx] = '\0';
            if (idx > 0) {
                printf("> %s\r\n", buf);
                parse_and_enqueue(buf);
            }
            idx = 0;
            continue;
        }

        if (idx < LINE_BUF_SIZE - 1)
            buf[idx++] = (char)c;
    }
}

// Task: receive commands from Queue and drive DFPlayer Mini via UART0
static void task_dfplayer(void *pvParameters) {
    (void)pvParameters;
    dfplayer_init();
    dfplayer_volume(DEFAULT_VOLUME);
    printf("OK: DFPlayer ready, volume %d\r\n", DEFAULT_VOLUME);

    int current_track = 1; // track number used by repeat one
    bool was_busy = false; // tracks previous BUSY state to detect transitions
    bool repeat_active = false; // suppress BUSY noise while DFPlayer loops internally
    command_t cmd;

    while (1) {
        // check BUSY pin every 200ms — skip reporting when repeat is active
        // (DFPlayer briefly pulses BUSY HIGH between loops, which would spam Serial)
        bool is_busy = dfplayer_is_busy();
        if (!repeat_active) {
            if (is_busy && !was_busy) {
                lcd_state.playing = true;
                printf("INFO: playing\r\n");
            } else if (!is_busy && was_busy) {
                lcd_state.playing = false;
                printf("INFO: track finished\r\n");
            }
        }
        was_busy = is_busy;

        if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(200)) != pdTRUE) continue;

        switch (cmd.type) {
            case CMD_PLAY:
                current_track = cmd.arg;
                lcd_state.track = current_track;
                dfplayer_play((uint16_t)current_track);
                printf("OK: playing track %d\r\n", current_track);
                break;
            case CMD_STOP:
                dfplayer_stop();
                printf("OK: stopped\r\n");
                break;
            case CMD_NEXT:
                dfplayer_next();
                printf("OK: next track\r\n");
                break;
            case CMD_PREV:
                dfplayer_prev();
                printf("OK: previous track\r\n");
                break;
            case CMD_VOLUME:
                lcd_state.volume = cmd.arg;
                dfplayer_volume((uint8_t)cmd.arg);
                printf("OK: volume set to %d\r\n", cmd.arg);
                break;
            case CMD_REPEAT_ALL:
                repeat_active = true;
                dfplayer_repeat_all();
                printf("OK: repeat all\r\n");
                break;
            case CMD_REPEAT_ONE:
                repeat_active = true;
                dfplayer_repeat_one((uint16_t)current_track);
                printf("OK: repeat track %d\r\n", current_track);
                break;
            case CMD_REPEAT_OFF:
                repeat_active = false;
                dfplayer_repeat_off();
                printf("OK: repeat off\r\n");
                break;
            default:
                break;
        }
    }
}

// Task: poll physical buttons every 50ms
//
// TRACK mode (default):
//   GP20 short → select prev track (wrap)   GP20 hold 2s → enter VOL mode
//   GP21 short → select next track (wrap)   GP21 hold 2s → enter VOL mode
//   GP22 → play selected track / stop
//
// VOL mode:
//   GP20 → vol down (min 0)
//   GP21 → vol up   (max 30)
//   GP22 → exit VOL mode back to TRACK mode
#define HOLD_TICKS  40  // 40 × 50ms = 2000ms

static void task_buttons(void *pvParameters) {
    (void)pvParameters;

    const uint btn_pins[] = { BTN_PREV_PIN, BTN_NEXT_PIN, BTN_PLAY_PIN };
    for (int i = 0; i < 3; i++) {
        gpio_init(btn_pins[i]);
        gpio_set_dir(btn_pins[i], GPIO_IN);
        gpio_pull_up(btn_pins[i]);
    }

    bool last[3]      = { true, true, true };
    bool cur[3];
    int  hold[2]      = { 0, 0 }; // hold counters for GP20 and GP21
    bool hold_fired[2]= { false, false }; // prevent repeat trigger while held
    int  selected_track = 1;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50));

        for (int i = 0; i < 3; i++)
            cur[i] = gpio_get(btn_pins[i]);

        command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };

        if (lcd_state.mode == UI_TRACK) {
            // --- TRACK mode ---

            // update hold counters for GP20/GP21
            for (int i = 0; i < 2; i++) {
                if (!cur[i]) {
                    hold[i]++;
                    if (hold[i] >= HOLD_TICKS && !hold_fired[i]) {
                        hold_fired[i] = true;
                        lcd_state.mode = UI_VOLUME;
                        printf("BTN: enter vol mode\r\n");
                    }
                } else {
                    hold[i] = 0;
                    hold_fired[i] = false;
                }
            }

            // short press only if not entering hold mode
            if (!cur[0] && last[0] && hold[0] < HOLD_TICKS) { // GP20 — prev
                selected_track = (selected_track <= 1) ? MAX_TRACK : selected_track - 1;
                lcd_state.track = selected_track;
                printf("BTN: select track %d\r\n", selected_track);
            }
            if (!cur[1] && last[1] && hold[1] < HOLD_TICKS) { // GP21 — next
                selected_track = (selected_track >= MAX_TRACK) ? 1 : selected_track + 1;
                lcd_state.track = selected_track;
                printf("BTN: select track %d\r\n", selected_track);
            }
            if (!cur[2] && last[2]) {                          // GP22 — play/stop
                if (lcd_state.playing) {
                    cmd.type = CMD_STOP;
                    xQueueSend(cmd_queue, &cmd, 0);
                    printf("BTN: stop\r\n");
                } else {
                    cmd.type = CMD_PLAY;
                    cmd.arg  = selected_track;
                    xQueueSend(cmd_queue, &cmd, 0);
                    printf("BTN: play track %d\r\n", selected_track);
                }
            }

        } else {
            // --- VOL mode ---
            hold[0] = 0; hold[1] = 0;
            hold_fired[0] = false; hold_fired[1] = false;

            if (!cur[0] && last[0]) {   // GP20 — vol down
                int v = lcd_state.volume > 0 ? lcd_state.volume - 1 : 0;
                lcd_state.volume = v;
                cmd.type = CMD_VOLUME;
                cmd.arg  = v;
                xQueueSend(cmd_queue, &cmd, 0);
                printf("BTN: vol %d\r\n", v);
            }
            if (!cur[1] && last[1]) {   // GP21 — vol up
                int v = lcd_state.volume < 30 ? lcd_state.volume + 1 : 30;
                lcd_state.volume = v;
                cmd.type = CMD_VOLUME;
                cmd.arg  = v;
                xQueueSend(cmd_queue, &cmd, 0);
                printf("BTN: vol %d\r\n", v);
            }
            if (!cur[2] && last[2]) {   // GP22 — exit vol mode
                lcd_state.mode = UI_TRACK;
                printf("BTN: exit vol mode\r\n");
            }
        }

        for (int i = 0; i < 3; i++)
            last[i] = cur[i];
    }
}

int main(void) {
    stdio_init_all();

    cmd_queue = xQueueCreate(CMD_QUEUE_LEN, sizeof(command_t));

    // create 5 tasks — scheduler manages priority
    xTaskCreate(task_status_led, "StatusLED", 256,  NULL, 1, NULL);
    xTaskCreate(task_uart_rx,    "UART_RX",   512,  NULL, 2, NULL);
    xTaskCreate(task_dfplayer,   "DFPlayer",  512,  NULL, 1, NULL);
    xTaskCreate(task_lcd,        "LCD",       512,  NULL, 1, NULL);
    xTaskCreate(task_buttons,    "Buttons",   256,  NULL, 1, NULL);

    vTaskStartScheduler(); // start FreeRTOS — should never return
    while (1) {}
}
