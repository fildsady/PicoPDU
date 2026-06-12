// Minimal WiFi + FreeRTOS test for Pico 2W
// Goal: verify FreeRTOS scheduler starts and CYW43 WiFi init works
// Single task — no LCD, no DFPlayer, no buttons
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#define WIFI_SSID  "IOT_503"
#define WIFI_PASS  "0811399455"

static void task_wifi_test(void *pvParameters) {
    (void)pvParameters;

    printf("[TASK] started\r\n");

    // init CYW43
    printf("[WIFI] cyw43_arch_init...\r\n");
    if (cyw43_arch_init()) {
        printf("[WIFI] init FAILED\r\n");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("[WIFI] init OK\r\n");

    // blink LED to confirm CYW43 is alive (LED is on CYW43 GPIO 0)
    for (int i = 0; i < 6; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, i & 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // try WiFi connect
    cyw43_arch_enable_sta_mode();
    printf("[WIFI] connecting to '%s'...\r\n", WIFI_SSID);

    int err = cyw43_arch_wifi_connect_timeout_ms(
        WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 15000);

    if (err) {
        printf("[WIFI] connect FAILED (err=%d)\r\n", err);
    } else {
        printf("[WIFI] connected!\r\n");
    }

    // blink forever to show we reached this point
    while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(err ? 100 : 500)); // fast=fail, slow=ok
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(err ? 100 : 500));
    }
}

int main(void) {
    stdio_init_all();
    printf("[BOOT] main started\r\n");

    xTaskCreate(task_wifi_test, "WiFiTest", 4096, NULL, 1, NULL);

    printf("[BOOT] starting scheduler\r\n");
    vTaskStartScheduler();

    printf("[BOOT] scheduler returned — out of memory?\r\n");
    while (1) {}
}
