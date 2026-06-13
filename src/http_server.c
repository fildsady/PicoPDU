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
#include "pdu_config.h"

#define NUM_CH       8
#define CH_GPIO_BASE 4

extern volatile bool     ch_state[NUM_CH];
extern volatile time_t   s_epoch;
extern volatile uint32_t s_sync_tick;
extern volatile float    s_temp;
extern volatile float    s_hum;

// generated from src/web/index.html at build time
#include "index_html.h"

// ---- Session token (single active session) ----
static char s_token[32] = "";   // empty = no active session

static void gen_token(void) {
    // ใช้ tick count XOR ค่าสุ่มง่ายๆ — ไม่ต้องการ crypto จริง
    uint32_t t = (uint32_t)xTaskGetTickCount() ^ 0xA3F1B2C4u;
    snprintf(s_token, sizeof(s_token), "%08lx%08lx",
             (unsigned long)t,
             (unsigned long)(t * 1664525u + 1013904223u));
}

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

// ---- auth/ok and auth/fail buffers ----
static char s_auth_resp[64];

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

    if (strcmp(name, "/auth/ok") == 0) {
        int len = snprintf(s_auth_resp, sizeof(s_auth_resp),
                           "{\"ok\":true,\"tok\":\"%s\"}", s_token);
        file->data  = s_auth_resp;
        file->len   = len;
        file->index = len;
        return 1;
    }

    if (strcmp(name, "/auth/fail") == 0) {
        static const char fail[] = "{\"ok\":false}";
        file->data  = fail;
        file->len   = sizeof(fail) - 1;
        file->index = sizeof(fail) - 1;
        return 1;
    }

    // ทุก URL อื่น → HTML
    file->data  = s_html;
    file->len   = sizeof(s_html) - 1;
    file->index = sizeof(s_html) - 1;
    return 1;
}
void fs_close_custom(struct fs_file *file) { (void)file; }

// ---- CGI login (rate-limited: 5 fails → 30s lockout) ----
static uint8_t  s_fail_count  = 0;
static uint32_t s_lockout_end = 0;   // tick ที่ lockout หมด

static const char *cgi_login(int iIndex, int iNumParams,
                              char *pcParam[], char *pcValue[]) {
    (void)iIndex;

    /* lockout check */
    if (s_fail_count >= 5) {
        uint32_t now = (uint32_t)xTaskGetTickCount();
        if ((int32_t)(now - s_lockout_end) < 0) /* ยังไม่ครบ 30 วิ */
            return "/auth/fail";
        s_fail_count = 0; /* lockout หมดแล้ว reset */
    }

    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "pass") == 0) {
            if (strcmp(pcValue[i], PDU_WEB_PASS) == 0) {
                s_fail_count = 0;
                gen_token();
                return "/auth/ok";
            }
            /* fail → เพิ่ม counter, ถ้าครบ 5 ตั้ง lockout 30 วิ */
            s_fail_count++;
            if (s_fail_count >= 5)
                s_lockout_end = (uint32_t)xTaskGetTickCount() + pdMS_TO_TICKS(30000);
            return "/auth/fail";
        }
    }
    return "/auth/fail";
}

// ---- CGI toggle (ต้องมี token ถูกต้อง) ----
static const char *cgi_toggle(int iIndex, int iNumParams,
                               char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    bool auth = false;
    int  ch   = -1;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "tok") == 0 && s_token[0] &&
            strcmp(pcValue[i], s_token) == 0)
            auth = true;
        if (strcmp(pcParam[i], "ch") == 0)
            ch = atoi(pcValue[i]) - 1;
    }
    if (auth && ch >= 0 && ch < NUM_CH) {
        ch_state[ch] = !ch_state[ch];
        gpio_put(CH_GPIO_BASE + ch, ch_state[ch] ? 1 : 0);
    }
    return "/api/state";
}

// ---- CGI verify (ตรวจ token ยังใช้ได้มั้ย) ----
static const char *cgi_verify(int iIndex, int iNumParams,
                               char *pcParam[], char *pcValue[]) {
    (void)iIndex;
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "tok") == 0 && s_token[0] &&
            strcmp(pcValue[i], s_token) == 0)
            return "/auth/ok";
    }
    return "/auth/fail";
}

// ---- CGI logout (ล้าง token) ----
static const char *cgi_logout(int iIndex, int iNumParams,
                               char *pcParam[], char *pcValue[]) {
    (void)iIndex; (void)iNumParams; (void)pcParam; (void)pcValue;
    memset(s_token, 0, sizeof(s_token));
    return "/auth/fail";
}

static const tCGI cgi_handlers[] = {
    { "/cgi/login",  cgi_login  },
    { "/cgi/verify", cgi_verify },
    { "/cgi/logout", cgi_logout },
    { "/cgi/toggle", cgi_toggle },
};

// ---- init ----
void http_server_init(void) {
    http_set_cgi_handlers(cgi_handlers, 4);
    httpd_init();
}
