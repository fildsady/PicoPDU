#include "web_server.h"
#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// forward-declared from main.c — web server writes commands here
extern QueueHandle_t        cmd_queue;
extern volatile lcd_state_t lcd_state;

// ----------------------------------------------------------------
// CGI handlers — called by lwIP httpd when URL matches
// ----------------------------------------------------------------

// /control?cmd=play&track=3
// /control?cmd=stop
// /control?cmd=next  /prev
// /control?cmd=vol&val=20
static const char *cgi_control(int index, int n_params,
                                char *params[], char *values[]) {
    command_t cmd = { .type = CMD_UNKNOWN, .arg = 0 };

    for (int i = 0; i < n_params; i++) {
        if (strcmp(params[i], "cmd") == 0) {
            if      (strcmp(values[i], "play") == 0) cmd.type = CMD_PLAY;
            else if (strcmp(values[i], "stop") == 0) cmd.type = CMD_STOP;
            else if (strcmp(values[i], "next") == 0) cmd.type = CMD_NEXT;
            else if (strcmp(values[i], "prev") == 0) cmd.type = CMD_PREV;
            else if (strcmp(values[i], "vol")  == 0) cmd.type = CMD_VOLUME;
            else if (strcmp(values[i], "repeat_all") == 0) cmd.type = CMD_REPEAT_ALL;
            else if (strcmp(values[i], "repeat_one") == 0) cmd.type = CMD_REPEAT_ONE;
            else if (strcmp(values[i], "repeat_off") == 0) cmd.type = CMD_REPEAT_OFF;
        }
        if (strcmp(params[i], "track") == 0) cmd.arg = atoi(values[i]);
        if (strcmp(params[i], "val")   == 0) cmd.arg = atoi(values[i]);
    }

    if (cmd.type != CMD_UNKNOWN)
        xQueueSend(cmd_queue, &cmd, 0);

    return "/index.shtml"; // redirect กลับหน้าหลัก
}

static const tCGI cgi_handlers[] = {
    { "/control", cgi_control },
};

// ----------------------------------------------------------------
// SSI handler — แทนที่ <!--#tag--> ใน .shtml ด้วยข้อมูลจริง
// ----------------------------------------------------------------
// tags ใน HTML:  <!--#track-->  <!--#vol-->  <!--#status-->

static const char *ssi_tags[] = { "track", "vol", "status" };

static u16_t ssi_handler(int index, char *insert, int insert_len) {
    switch (index) {
        case 0: // track
            snprintf(insert, insert_len, "%d", lcd_state.track);
            break;
        case 1: // vol
            snprintf(insert, insert_len, "%d", lcd_state.volume);
            break;
        case 2: // status
            snprintf(insert, insert_len, "%s", lcd_state.playing ? "Playing" : "Stopped");
            break;
    }
    return strlen(insert);
}

// ----------------------------------------------------------------
// Task: init WiFi, connect, start httpd
// ----------------------------------------------------------------
void task_webserver(void *pvParameters) {
    (void)pvParameters;

    // init CYW43 WiFi chip
    if (cyw43_arch_init()) {
        printf("ERR: WiFi init failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("WiFi: connecting to %s ...\r\n", WIFI_SSID);

    // block until connected (timeout 30s)
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                            CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("ERR: WiFi connect failed\r\n");
        vTaskDelete(NULL);
        return;
    }

    printf("WiFi: connected, IP = %s\r\n",
           ip4addr_ntoa(netif_ip4_addr(netif_default)));

    // start httpd with CGI + SSI
    httpd_init();
    http_set_cgi_handlers(cgi_handlers, LWIP_ARRAYSIZE(cgi_handlers));
    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));

    printf("HTTP: server ready\r\n");

    while (1) {
        cyw43_arch_poll(); // ต้อง poll อยู่เสมอถ้าใช้ threadsafe_background
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
