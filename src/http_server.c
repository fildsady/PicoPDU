#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hardware/gpio.h"
#include "lwip/apps/httpd.h"
#include "lwip/apps/fs.h"
#include "FreeRTOS.h"
#include "task.h"
#include "http_server.h"

#define NUM_CH       8
#define CH_GPIO_BASE 4

extern volatile bool     ch_state[NUM_CH];
extern volatile time_t   s_epoch;
extern volatile uint32_t s_sync_tick;
extern volatile float    s_temp;
extern volatile float    s_hum;

// generated from src/web/index.html at build time
#include "index_html.h"

// ---- JSON state buffer ----
static char s_json[128];

static int build_json(void) {
    char tbuf[9];
    if (!s_epoch) {
        strcpy(tbuf, "--:--:--");
    } else {
        time_t now = s_epoch + (time_t)((xTaskGetTickCount() - s_sync_tick) / 1000);
        struct tm t;
        gmtime_r(&now, &t);
        snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    }
    return snprintf(s_json, sizeof(s_json),
        "{\"t\":\"%s\",\"temp\":%.1f,\"hum\":%.1f,\"ch\":[%d,%d,%d,%d,%d,%d,%d,%d]}",
        tbuf, (double)s_temp, (double)s_hum,
        ch_state[0]?1:0, ch_state[1]?1:0,
        ch_state[2]?1:0, ch_state[3]?1:0,
        ch_state[4]?1:0, ch_state[5]?1:0,
        ch_state[6]?1:0, ch_state[7]?1:0);
}

// ---- Custom filesystem ----
int fs_open_custom(struct fs_file *file, const char *name) {
    memset(file, 0, sizeof(*file));
    if (strcmp(name, "/api/state") == 0) {
        int len = build_json();
        file->data  = s_json;
        file->len   = len;
        file->index = len;
        return 1;
    }
    // ทุก URL อื่น → HTML
    file->data  = s_html;
    file->len   = sizeof(s_html) - 1;
    file->index = sizeof(s_html) - 1;
    return 1;
}
void fs_close_custom(struct fs_file *file) { (void)file; }

// ---- CGI toggle ----
static const char *cgi_toggle(int iIndex, int iNumParams,
                               char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "ch") == 0) {
            int ch = atoi(pcValue[i]) - 1;
            if (ch >= 0 && ch < NUM_CH) {
                ch_state[ch] = !ch_state[ch];
                gpio_put(CH_GPIO_BASE + ch, ch_state[ch] ? 1 : 0);
            }
            break;
        }
    }
    return "/api/state";   // JS fetch ตามมาดึง JSON ใหม่ทันที
}

static const tCGI cgi_handlers[] = {{ "/cgi/toggle", cgi_toggle }};

// ---- init ----
void http_server_init(void) {
    http_set_cgi_handlers(cgi_handlers, 1);
    httpd_init();
}
