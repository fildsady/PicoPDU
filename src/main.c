#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "dfplayer.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

#define CMD_QUEUE_LEN   8   // max pending commands waiting to execute
#define LINE_BUF_SIZE   64  // max length of one command line
#define DEFAULT_VOLUME  25  // startup volume (0-30)

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
    command_t cmd;

    while (1) {
        if (xQueueReceive(cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) continue;

        switch (cmd.type) {
            case CMD_PLAY:
                current_track = cmd.arg;
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
                dfplayer_volume((uint8_t)cmd.arg);
                printf("OK: volume set to %d\r\n", cmd.arg);
                break;
            case CMD_REPEAT_ALL:
                dfplayer_repeat_all();
                printf("OK: repeat all\r\n");
                break;
            case CMD_REPEAT_ONE:
                dfplayer_repeat_one((uint16_t)current_track);
                printf("OK: repeat track %d\r\n", current_track);
                break;
            case CMD_REPEAT_OFF:
                dfplayer_repeat_off();
                printf("OK: repeat off\r\n");
                break;
            default:
                break;
        }
    }
}

int main(void) {
    stdio_init_all();

    cmd_queue = xQueueCreate(CMD_QUEUE_LEN, sizeof(command_t));

    // create 3 tasks — scheduler manages priority
    xTaskCreate(task_status_led, "StatusLED", 256, NULL, 1, NULL);
    xTaskCreate(task_uart_rx,    "UART_RX",   512, NULL, 2, NULL);
    xTaskCreate(task_dfplayer,   "DFPlayer",  512, NULL, 1, NULL);

    vTaskStartScheduler(); // start FreeRTOS — should never return
    while (1) {}
}
