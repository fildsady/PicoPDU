#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "i2c_lcd.h"

// ---------------------------------------------------------------------------
// Config — แก้ตรงนี้ก่อน flash
// ---------------------------------------------------------------------------
#define WIFI_SSID      "IOT_503"
#define WIFI_PASS      "0811399455"
#define MQTT_SERVER_IP "192.168.0.2"
#define MQTT_BROKER_PORT 1883
#define CLIENT_ID      "picopdu"

// ---------------------------------------------------------------------------
// Channels: GP4–GP11 (8 ช่อง)
// ---------------------------------------------------------------------------
#define CH_COUNT    8
#define CH_PIN_BASE 4

static mqtt_client_t *s_client;
static bool           s_state[CH_COUNT];
static int            s_inpub_ch = -1;
static bool           s_mqtt_connected = false;

// ── LCD ──────────────────────────────────────────────────────────────────────
// Line 0: status  (WiFi/MQTT)
// Line 1: ch1-8   (O=ON, .=OFF)
static void lcd_update(void) {
    lcd_clear_buff_all();
    if (!s_mqtt_connected) {
        lcd_buff_printf(0, 0, "MQTT connecting.");
        lcd_buff_printf(1, 0, "please wait...");
    } else {
        const char *ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
        lcd_buff_printf(0, 0, "%-16s", ip);
        // แสดง 8 ช่อง: O=ON, .=OFF  เช่น "12345678"
        char row[17] = "CH:";
        for (int i = 0; i < CH_COUNT; i++)
            row[3 + i] = s_state[i] ? 'O' : '.';
        row[11] = '\0';
        lcd_buff_printf(1, 0, "%s", row);
    }
    put_buff_to_lcd();
}

// ── GPIO ─────────────────────────────────────────────────────────────────────

static void channels_init(void) {
    for (int i = 0; i < CH_COUNT; i++) {
        gpio_init(CH_PIN_BASE + i);
        gpio_set_dir(CH_PIN_BASE + i, GPIO_OUT);
        gpio_put(CH_PIN_BASE + i, 0);
    }
}

static void channel_set(int ch, bool on) {
    s_state[ch] = on;
    gpio_put(CH_PIN_BASE + ch, on);

    char topic[32];
    snprintf(topic, sizeof(topic), "pdu/ch%d/state", ch + 1);
    const char *payload = on ? "ON" : "OFF";
    mqtt_publish(s_client, topic, payload, strlen(payload), 0, 1, NULL, NULL);
    printf("[PDU] ch%d -> %s\n", ch + 1, payload);
    lcd_update();
}

// ── MQTT incoming publish ─────────────────────────────────────────────────────

static void on_incoming_publish(void *arg, const char *topic, u32_t tot_len) {
    (void)arg; (void)tot_len;
    s_inpub_ch = -1;
    int ch;
    if (sscanf(topic, "pdu/ch%d/set", &ch) == 1 && ch >= 1 && ch <= CH_COUNT)
        s_inpub_ch = ch - 1;
}

static void on_incoming_data(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    (void)arg;
    if (s_inpub_ch < 0 || !(flags & MQTT_DATA_FLAG_LAST)) return;
    char payload[8];
    int n = len < (int)sizeof(payload) - 1 ? len : (int)sizeof(payload) - 1;
    memcpy(payload, data, n);
    payload[n] = '\0';
    if      (strcmp(payload, "ON")  == 0) channel_set(s_inpub_ch, true);
    else if (strcmp(payload, "OFF") == 0) channel_set(s_inpub_ch, false);
}

// ── Subscribe + Discovery ─────────────────────────────────────────────────────

static void on_subscribe(void *arg, err_t err) {
    (void)arg;
    if (err != ERR_OK) printf("[MQTT] subscribe err=%d\n", err);
}

static void subscribe_all(void) {
    for (int i = 1; i <= CH_COUNT; i++) {
        char topic[32];
        snprintf(topic, sizeof(topic), "pdu/ch%d/set", i);
        mqtt_subscribe(s_client, topic, 0, on_subscribe, NULL);
    }
}

static void publish_discovery(void) {
    for (int i = 1; i <= CH_COUNT; i++) {
        char topic[64], payload[512];
        snprintf(topic, sizeof(topic), "homeassistant/switch/picopdu_ch%d/config", i);
        snprintf(payload, sizeof(payload),
            "{\"name\":\"PDU Channel %d\","
            "\"unique_id\":\"picopdu_ch%d\","
            "\"command_topic\":\"pdu/ch%d/set\","
            "\"state_topic\":\"pdu/ch%d/state\","
            "\"availability_topic\":\"pdu/status\","
            "\"payload_available\":\"online\","
            "\"payload_not_available\":\"offline\","
            "\"device\":{\"identifiers\":[\"picopdu\"],"
            "\"name\":\"PicoPDU\",\"model\":\"Pico 2W\","
            "\"manufacturer\":\"Custom\"}}",
            i, i, i, i);
        err_t ret = mqtt_publish(s_client, topic, payload, strlen(payload), 0, 1, NULL, NULL);
        printf("[MQTT] discovery ch%d ret=%d len=%d\n", i, ret, (int)strlen(payload));
    }
    printf("[MQTT] HA discovery done\n");
}

// ── MQTT connect callback ─────────────────────────────────────────────────────

static void on_connect(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {
    (void)arg;
    if (status != MQTT_CONNECT_ACCEPTED) {
        printf("[MQTT] connect failed: %d\n", status);
        return;
    }
    printf("[MQTT] connected\n");
    s_mqtt_connected = true;
    mqtt_publish(client, "pdu/status", "online", 6, 0, 1, NULL, NULL);
    subscribe_all();
    publish_discovery();
    lcd_update();
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(void) {
    stdio_init_all();
    channels_init();
    lcd_init();
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "PicoPDU booting");
    lcd_buff_printf(1, 0, "please wait...");
    put_buff_to_lcd();
    printf("[BOOT] PicoPDU 8ch starting\n");

    if (cyw43_arch_init()) {
        printf("[ERR] cyw43 init failed\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("[WIFI] connecting to '%s'...\n", WIFI_SSID);
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "WiFi connecting");
    lcd_buff_printf(1, 0, "%-16s", WIFI_SSID);
    put_buff_to_lcd();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("[ERR] wifi connect failed\n");
        lcd_clear_buff_all();
        lcd_buff_printf(0, 0, "WiFi FAILED");
        put_buff_to_lcd();
        return 1;
    }
    const char *ip_str = ip4addr_ntoa(netif_ip4_addr(netif_default));
    printf("[WIFI] connected, IP: %s\n", ip_str);
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "WiFi OK");
    lcd_buff_printf(1, 0, "%-16s", ip_str);
    put_buff_to_lcd();

    s_client = mqtt_client_new();
    if (!s_client) {
        printf("[ERR] mqtt_client_new failed\n");
        return 1;
    }
    mqtt_set_inpub_callback(s_client, on_incoming_publish, on_incoming_data, NULL);

    struct mqtt_connect_client_info_t ci = {
        .client_id   = CLIENT_ID,
        .client_user = "anon",
        .client_pass = "095033221",
        .keep_alive  = 60,
        .will_topic  = "pdu/status",
        .will_msg    = "offline",
        .will_qos    = 0,
        .will_retain = 1,
    };

    ip_addr_t broker_ip;
    ipaddr_aton(MQTT_SERVER_IP, &broker_ip);
    printf("[MQTT] connecting to %s:%d...\n", MQTT_SERVER_IP, MQTT_BROKER_PORT);
    lcd_clear_buff_all();
    lcd_buff_printf(0, 0, "MQTT connecting");
    lcd_buff_printf(1, 0, "%-16s", MQTT_SERVER_IP);
    put_buff_to_lcd();
    err_t err = mqtt_client_connect(s_client, &broker_ip, MQTT_BROKER_PORT, on_connect, NULL, &ci);
    if (err != ERR_OK) {
        printf("[ERR] mqtt_client_connect err=%d\n", err);
        return 1;
    }

    while (true) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
}
