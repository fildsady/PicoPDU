#include "dfplayer.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#define DFPLAYER_UART    uart0
#define DFPLAYER_BAUD    9600
#define DFPLAYER_TX_PIN  0
#define DFPLAYER_RX_PIN  1

static void send_cmd(uint8_t cmd, uint16_t param) {
    uint8_t ph = (param >> 8) & 0xFF;
    uint8_t pl = param & 0xFF;

    int16_t checksum = -(int16_t)(0xFF + 0x06 + (uint16_t)cmd + ph + pl);

    uint8_t frame[10] = {
        0x7E, 0xFF, 0x06, cmd, 0x00,
        ph, pl,
        (uint8_t)((checksum >> 8) & 0xFF),
        (uint8_t)(checksum & 0xFF),
        0xEF
    };

    uart_write_blocking(DFPLAYER_UART, frame, sizeof(frame));
    vTaskDelay(pdMS_TO_TICKS(30));
}

void dfplayer_init(void) {
    uart_init(DFPLAYER_UART, DFPLAYER_BAUD);
    gpio_set_function(DFPLAYER_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(DFPLAYER_RX_PIN, GPIO_FUNC_UART);
    vTaskDelay(pdMS_TO_TICKS(1000)); // wait for DFPlayer to boot
}

void dfplayer_play(uint16_t track)       { send_cmd(0x03, track); }
void dfplayer_stop(void)                 { send_cmd(0x16, 0); }
void dfplayer_next(void)                 { send_cmd(0x01, 0); }
void dfplayer_prev(void)                 { send_cmd(0x02, 0); }
void dfplayer_volume(uint8_t vol)        { send_cmd(0x06, vol); }
void dfplayer_repeat_all(void)           { send_cmd(0x11, 0x0001); }
void dfplayer_repeat_one(uint16_t track) { send_cmd(0x08, track); }
void dfplayer_repeat_off(void)           { send_cmd(0x11, 0x0000); }
