#include <string.h>
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/sntp.h"
#include "i2c_lcd.h"
#include "http_server.h"
#include "FreeRTOS.h"
#include "task.h"

#define WIFI_SSID   "IOT_503"
#define WIFI_PASS   "0811399455"
#define NTP_SERVER  "pool.ntp.org"
#define TZ_OFFSET   (7 * 3600)     // UTC+7

// ---- SNTP (accessed also by http_server.c) ----
volatile time_t   s_epoch     = 0;
volatile uint32_t s_sync_tick = 0;

void sntp_time_received(u32_t sec) {
    s_epoch     = (time_t)sec + TZ_OFFSET;
    s_sync_tick = xTaskGetTickCount();
}

static void get_time_str(char *buf9) {
    if (!s_epoch) { strcpy(buf9, "--:--:--"); return; }
    time_t now = s_epoch + (time_t)((xTaskGetTickCount() - s_sync_tick) / 1000);
    struct tm t;
    gmtime_r(&now, &t);
    snprintf(buf9, 9, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
}

// ---- Channel state (accessed also by http_server.c) ----
#define NUM_CH       8
#define CH_GPIO_BASE 4

volatile bool  ch_state[NUM_CH] = {false};

// ---- Sensor placeholders — replace with real sensor driver later ----
volatile float s_temp = 25.3f;   // °C
volatile float s_hum  = 62.0f;   // %RH

// ---- LCD ----
static void update_lcd(void) {
    char timebuf[9];
    get_time_str(timebuf);
    char ch[9] = {0};
    for (int i = 0; i < NUM_CH; i++)
        ch[i] = ch_state[i] ? '1' : '0';
    lcd_clear_buff_all();
    // line 0: "17:02:09  25.3C"  (16 chars)
    lcd_buff_printf(0, 0, "%s %5.1fC", timebuf, (double)s_temp);
    // line 1: "CH:00000000 62% "  (16 chars)
    lcd_buff_printf(1, 0, "CH:%s %3.0f%%", ch, (double)s_hum);
    put_buff_to_lcd();
}

// ---- Tasks ----
static void blink_task(void *params) {
    (void)params;
    gpio_init(0);
    gpio_set_dir(0, GPIO_OUT);
    while (true) {
        gpio_put(0, 1); vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(0, 0); vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void wifi_task(void *params) {
    (void)params;

    // init GPIOs
    for (int i = 0; i < NUM_CH; i++) {
        gpio_init(CH_GPIO_BASE + i);
        gpio_set_dir(CH_GPIO_BASE + i, GPIO_OUT);
        gpio_put(CH_GPIO_BASE + i, 0);
    }

    // cyw43 init
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "cyw43 init...");
    put_buff_to_lcd();
    if (cyw43_arch_init()) {
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "cyw43 FAILED");
        put_buff_to_lcd();
        vTaskDelete(NULL); return;
    }
    cyw43_arch_enable_sta_mode();

    // WiFi connect
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "WiFi conn...");
    lcd_buff_printf(1, 0, WIFI_SSID);
    put_buff_to_lcd();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "WiFi FAILED");
        put_buff_to_lcd();
        vTaskDelete(NULL); return;
    }

    const char *ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "%-16s", ip);
    lcd_buff_printf(1, 0, "HTTP port 80");
    put_buff_to_lcd();

    // SNTP
    cyw43_arch_lwip_begin();
    sntp_setservername(0, NTP_SERVER);
    sntp_init();
    cyw43_arch_lwip_end();

    // HTTP server
    cyw43_arch_lwip_begin();
    http_server_init();
    cyw43_arch_lwip_end();

    vTaskDelay(pdMS_TO_TICKS(500));
    update_lcd();

    // main loop — อัป LCD ทุกวินาที + heartbeat LED
    TickType_t last_blink = xTaskGetTickCount();
    int last_sec = -1;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(200));

        int cur_sec = -1;
        if (s_epoch) {
            time_t now_t = s_epoch + (time_t)((xTaskGetTickCount() - s_sync_tick) / 1000);
            struct tm t;
            gmtime_r(&now_t, &t);
            cur_sec = t.tm_sec;
        }
        if (cur_sec != last_sec) {
            last_sec = cur_sec;
            update_lcd();
        }

        TickType_t now_tick = xTaskGetTickCount();
        if (now_tick - last_blink >= pdMS_TO_TICKS(1000)) {
            last_blink = now_tick;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
    }
}

int main(void) {
    stdio_init_all();
    lcd_init();
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "Booting...");
    put_buff_to_lcd();

    xTaskCreate(wifi_task,  "WIFI",  4096, NULL, 1, NULL);
    xTaskCreate(blink_task, "BLINK", 256,  NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) tight_loop_contents();
}
