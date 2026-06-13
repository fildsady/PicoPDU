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

// ---- shared state (defined in main.c) ----
extern volatile bool     ch_state[NUM_CH];
extern volatile time_t   s_epoch;
extern volatile uint32_t s_sync_tick;

// ---- HTML template ----
// <!--#t-->     = เวลา HH:MM:SS
// <!--#c1-->    = <td class="on/off">ON/OFF</td><td>Toggle link</td>
static const char s_html[] =
"<!DOCTYPE html>\r\n"
"<html><head>"
"<meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<meta http-equiv=\"refresh\" content=\"10\">"
"<title>PicoPDU</title>"
"<style>"
"body{font-family:monospace;background:#1a1a2e;color:#eee;"
     "max-width:440px;margin:0 auto;padding:16px}"
"h2{color:#0af;margin-bottom:4px}"
"#clk{font-size:18px;color:#8af;margin:0 0 12px}"
"table{width:100%;border-collapse:collapse}"
"th,td{border:1px solid #333;padding:7px;text-align:center}"
"th{background:#222;color:#aaa}"
".on{background:#0d5c1e;color:#4f8;font-weight:bold}"
".off{background:#5c1010;color:#f88}"
"a{display:inline-block;padding:2px 10px;background:#0af;"
  "color:#000;text-decoration:none;border-radius:3px}"
"</style></head><body>"
"<h2>&#x26A1; PicoPDU</h2>"
"<p id=\"clk\"><!--#t--></p>"
"<table>"
"<tr><th>CH</th><th>State</th><th>Action</th></tr>"
"<tr><th>1</th><!--#c1--></tr>"
"<tr><th>2</th><!--#c2--></tr>"
"<tr><th>3</th><!--#c3--></tr>"
"<tr><th>4</th><!--#c4--></tr>"
"<tr><th>5</th><!--#c5--></tr>"
"<tr><th>6</th><!--#c6--></tr>"
"<tr><th>7</th><!--#c7--></tr>"
"<tr><th>8</th><!--#c8--></tr>"
"</table>"
"<p style=\"font-size:11px;color:#555\">"
"Pico 2W &bull; UTC+7 &bull; <a href=\"/\">Refresh</a>"
"</p>"
"</body></html>\r\n";

// ---- SSI ----
// tags ต้องเรียงตามตัวอักษร (httpd ใช้ binary search)
static const char *ssi_tags[] = {
    "c1","c2","c3","c4","c5","c6","c7","c8","t"
};

static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
    if (iIndex < 8) {
        int ch = iIndex;
        return (u16_t)snprintf(pcInsert, (size_t)iInsertLen,
            "<td class=\"%s\">%s</td>"
            "<td><a href=\"/cgi/toggle?ch=%d\">Toggle</a></td>",
            ch_state[ch] ? "on" : "off",
            ch_state[ch] ? "ON" : "OFF",
            ch + 1);
    }
    if (iIndex == 8) { // "t"
        if (!s_epoch) return (u16_t)snprintf(pcInsert, (size_t)iInsertLen, "--:--:--");
        time_t now = s_epoch + (time_t)((xTaskGetTickCount() - s_sync_tick) / 1000);
        struct tm t;
        gmtime_r(&now, &t);
        return (u16_t)snprintf(pcInsert, (size_t)iInsertLen,
            "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    }
    return 0;
}

// ---- CGI ----
static const char *cgi_toggle(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
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
    return "/index.shtml";
}

static const tCGI cgi_handlers[] = {{ "/cgi/toggle", cgi_toggle }};

// ---- init ----
void http_server_init(void) {
    http_set_ssi_handler(ssi_handler, ssi_tags,
                         sizeof(ssi_tags) / sizeof(ssi_tags[0]));
    http_set_cgi_handlers(cgi_handlers, 1);
    httpd_init();
}
