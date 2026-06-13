#include <string.h>
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/mqtt.h"
#include "lwip/apps/sntp.h"
#include "i2c_lcd.h"
#include "FreeRTOS.h"
#include "task.h"

#define WIFI_SSID        "IOT_503"
#define WIFI_PASS        "0811399455"
#define MQTT_BROKER_IP   "192.168.0.2"
#define MQTT_BROKER_PORT 1883
#define MQTT_USER        "anon"
#define MQTT_PASS        "095033221"
#define MQTT_CLIENT_ID   "pico_pdu"

#define NTP_SERVER   "pool.ntp.org"
#define TZ_OFFSET    (7 * 3600)     // UTC+7

// ---- SNTP ----
static volatile time_t   s_epoch     = 0;
static volatile uint32_t s_sync_tick = 0;

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

#define NUM_CH      8
#define CH_GPIO_BASE 4  // GP4-GP11

// ---- Sensor placeholders — swap with real driver later ----
static volatile float s_temp = 25.3f;   // °C
static volatile float s_hum  = 62.0f;   // %RH

// ---- task handle for notification from IRQ ----
static TaskHandle_t s_wifi_task_handle = NULL;

// ---- channel state ----
static volatile bool ch_state[NUM_CH]       = {false};
static volatile bool ch_pending_pub[NUM_CH] = {false};

static const char * const ch_set_topic[NUM_CH] = {
    "pdu/ch1/set","pdu/ch2/set","pdu/ch3/set","pdu/ch4/set",
    "pdu/ch5/set","pdu/ch6/set","pdu/ch7/set","pdu/ch8/set",
};
static const char * const ch_state_topic[NUM_CH] = {
    "pdu/ch1/state","pdu/ch2/state","pdu/ch3/state","pdu/ch4/state",
    "pdu/ch5/state","pdu/ch6/state","pdu/ch7/state","pdu/ch8/state",
};

// ---- MQTT incoming ----
static char     s_in_topic[64];
static uint8_t  s_in_data[16];
static uint16_t s_in_len;

static void mqtt_incoming_pub_cb(void *arg, const char *topic, u32_t tot_len) {
    (void)arg; (void)tot_len;
    strncpy(s_in_topic, topic, sizeof(s_in_topic) - 1);
    s_in_topic[sizeof(s_in_topic) - 1] = '\0';
    s_in_len = 0;
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    (void)arg;
    if (s_in_len + len < sizeof(s_in_data)) {
        memcpy(s_in_data + s_in_len, data, len);
        s_in_len += len;
    }
    if (flags & MQTT_DATA_FLAG_LAST) {
        bool on = (s_in_len >= 2 && strncmp((char *)s_in_data, "ON", 2) == 0);
        for (int i = 0; i < NUM_CH; i++) {
            if (strcmp(s_in_topic, ch_set_topic[i]) == 0) {
                ch_state[i]       = on;
                gpio_put(CH_GPIO_BASE + i, on ? 1 : 0);
                ch_pending_pub[i] = true;
                break;
            }
        }
        // ปลุก wifi_task ทันที ไม่ต้องรอ poll
        if (s_wifi_task_handle) {
            BaseType_t woken = pdFALSE;
            vTaskNotifyGiveFromISR(s_wifi_task_handle, &woken);
            portYIELD_FROM_ISR(woken);
        }
    }
}

// ---- MQTT connect callback ----
static volatile bool s_mqtt_connected = false;
static volatile bool s_mqtt_conn_done = false;

static void mqtt_conn_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    (void)client; (void)arg;
    s_mqtt_connected = (status == MQTT_CONNECT_ACCEPTED);
    s_mqtt_conn_done = true;
}

// ---- helpers ----
static void mqtt_pub(mqtt_client_t *mqtt, const char *topic, const char *payload, uint8_t retain) {
    cyw43_arch_lwip_begin();
    mqtt_publish(mqtt, topic, payload, strlen(payload), 1, retain, NULL, NULL);
    cyw43_arch_lwip_end();
}

static void update_lcd(void) {
    char timebuf[9];
    get_time_str(timebuf);
    char ch[9] = {0};
    for (int i = 0; i < NUM_CH; i++)
        ch[i] = ch_state[i] ? '1' : '0';
    lcd_clear_buff_all();
    // "17:02:09  25.3C"  (16 chars)
    lcd_buff_printf(0, 0, "%s %5.1fC", timebuf, (double)s_temp);
    // "CH:00000000 62% "  (16 chars)
    lcd_buff_printf(1, 0, "CH:%s %3.0f%%", ch, (double)s_hum);
    put_buff_to_lcd();
}

// ---- tasks ----
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
    const char *ip_str = ip4addr_ntoa(netif_ip4_addr(netif_default));
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "WiFi OK!");
    lcd_buff_printf(1, 0, "%-16s", ip_str);
    put_buff_to_lcd();
    // SNTP — sync เวลาจาก router
    cyw43_arch_lwip_begin();
    sntp_setservername(0, NTP_SERVER);
    sntp_init();
    cyw43_arch_lwip_end();

    vTaskDelay(pdMS_TO_TICKS(1000));

    // MQTT connect
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "MQTT conn...");
    put_buff_to_lcd();

    cyw43_arch_lwip_begin();
    mqtt_client_t *mqtt = mqtt_client_new();
    cyw43_arch_lwip_end();
    if (!mqtt) {
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "MQTT alloc err");
        put_buff_to_lcd();
        vTaskDelete(NULL); return;
    }

    ip_addr_t broker;
    ip4addr_aton(MQTT_BROKER_IP, &broker);

    struct mqtt_connect_client_info_t ci = {
        .client_id    = MQTT_CLIENT_ID,
        .client_user  = MQTT_USER,
        .client_pass  = MQTT_PASS,
        .keep_alive   = 60,
        .will_topic   = "pdu/status",
        .will_msg     = "offline",
        .will_qos     = 1,
        .will_retain  = 1,
    };

    s_mqtt_conn_done = false;
    s_mqtt_connected = false;
    cyw43_arch_lwip_begin();
    err_t err = mqtt_client_connect(mqtt, &broker, MQTT_BROKER_PORT, mqtt_conn_cb, NULL, &ci);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "MQTT err:%d", (int)err);
        put_buff_to_lcd();
        vTaskDelete(NULL); return;
    }

    for (int i = 0; i < 100 && !s_mqtt_conn_done; i++)
        vTaskDelay(pdMS_TO_TICKS(100));

    if (!s_mqtt_connected) {
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "MQTT FAILED");
        put_buff_to_lcd();
        vTaskDelete(NULL); return;
    }

    // online LWT
    mqtt_pub(mqtt, "pdu/status", "online", 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    // set incoming callbacks
    cyw43_arch_lwip_begin();
    mqtt_set_inpub_callback(mqtt, mqtt_incoming_pub_cb, mqtt_incoming_data_cb, NULL);
    cyw43_arch_lwip_end();

    // subscribe all set topics
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "Subscribing...");
    put_buff_to_lcd();
    for (int i = 0; i < NUM_CH; i++) {
        cyw43_arch_lwip_begin();
        mqtt_subscribe(mqtt, ch_set_topic[i], 1, NULL, NULL);
        cyw43_arch_lwip_end();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // publish initial states (all OFF, retain)
    for (int i = 0; i < NUM_CH; i++) {
        mqtt_pub(mqtt, ch_state_topic[i], "OFF", 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // HA Discovery — switches
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "HA Discovery...");
    put_buff_to_lcd();
    for (int i = 0; i < NUM_CH; i++) {
        char config_topic[72];
        char payload[256];
        snprintf(config_topic, sizeof(config_topic),
                 "homeassistant/switch/picopdu_ch%d/config", i + 1);
        snprintf(payload, sizeof(payload),
                 "{\"name\":\"PDU Ch%d\","
                 "\"cmd_t\":\"pdu/ch%d/set\","
                 "\"stat_t\":\"pdu/ch%d/state\","
                 "\"uniq_id\":\"pdu2_ch%d\","
                 "\"avty_t\":\"pdu/status\","
                 "\"pl_avail\":\"online\","
                 "\"pl_not_avail\":\"offline\","
                 "\"device\":{\"ids\":[\"pdu2\"],"
                 "\"name\":\"PicoPDU\",\"model\":\"Pico 2W\",\"manufacturer\":\"RPi\"}}",
                 i+1, i+1, i+1, i+1);
        mqtt_pub(mqtt, config_topic, payload, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // HA Discovery — temperature sensor
    {
        char pl[320];
        snprintf(pl, sizeof(pl),
            "{\"name\":\"PDU Temperature\","
            "\"stat_t\":\"pdu/sensor/temp\","
            "\"unit_of_meas\":\"\xc2\xb0""C\","
            "\"dev_cla\":\"temperature\","
            "\"stat_cla\":\"measurement\","
            "\"uniq_id\":\"pdu2_temp\","
            "\"avty_t\":\"pdu/status\","
            "\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
            "\"device\":{\"ids\":[\"pdu2\"],"
            "\"name\":\"PicoPDU\",\"model\":\"Pico 2W\",\"manufacturer\":\"RPi\"}}");
        mqtt_pub(mqtt, "homeassistant/sensor/picopdu_temp/config", pl, 1);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // HA Discovery — humidity sensor
    {
        char pl[320];
        snprintf(pl, sizeof(pl),
            "{\"name\":\"PDU Humidity\","
            "\"stat_t\":\"pdu/sensor/hum\","
            "\"unit_of_meas\":\"%%\","
            "\"dev_cla\":\"humidity\","
            "\"stat_cla\":\"measurement\","
            "\"uniq_id\":\"pdu2_hum\","
            "\"avty_t\":\"pdu/status\","
            "\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
            "\"device\":{\"ids\":[\"pdu2\"],"
            "\"name\":\"PicoPDU\",\"model\":\"Pico 2W\",\"manufacturer\":\"RPi\"}}");
        mqtt_pub(mqtt, "homeassistant/sensor/picopdu_hum/config", pl, 1);
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // publish ค่าเริ่มต้นของ sensor
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", (double)s_temp);
        mqtt_pub(mqtt, "pdu/sensor/temp", buf, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
        snprintf(buf, sizeof(buf), "%.1f", (double)s_hum);
        mqtt_pub(mqtt, "pdu/sensor/hum", buf, 1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    update_lcd();
    s_wifi_task_handle = xTaskGetCurrentTaskHandle();

    // main loop
    TickType_t last_blink  = xTaskGetTickCount();
    TickType_t last_sensor = xTaskGetTickCount();
    int last_sec = -1;
    while (true) {
        // poll ทุก 200ms หรือปลุกทันทีเมื่อมี MQTT command
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));

        // publish pending state
        bool ch_changed = false;
        for (int i = 0; i < NUM_CH; i++) {
            if (ch_pending_pub[i]) {
                ch_pending_pub[i] = false;
                mqtt_pub(mqtt, ch_state_topic[i], ch_state[i] ? "ON" : "OFF", 1);
                ch_changed = true;
            }
        }

        // อัป LCD เฉพาะเมื่อวินาทีเปลี่ยน หรือ channel เปลี่ยน
        int cur_sec = -1;
        if (s_epoch) {
            time_t now_t = s_epoch + (time_t)((xTaskGetTickCount() - s_sync_tick) / 1000);
            struct tm t;
            gmtime_r(&now_t, &t);
            cur_sec = t.tm_sec;
        }
        if (cur_sec != last_sec || ch_changed) {
            last_sec = cur_sec;
            update_lcd();
        }

        // publish sensor ทุก 60 วิ
        TickType_t now_tick = xTaskGetTickCount();
        if (now_tick - last_sensor >= pdMS_TO_TICKS(60000)) {
            last_sensor = now_tick;
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f", (double)s_temp);
            mqtt_pub(mqtt, "pdu/sensor/temp", buf, 1);
            snprintf(buf, sizeof(buf), "%.1f", (double)s_hum);
            mqtt_pub(mqtt, "pdu/sensor/hum",  buf, 1);
        }

        // heartbeat LED ทุก ~1s
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
