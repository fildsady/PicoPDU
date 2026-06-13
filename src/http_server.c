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

// ---- Static HTML + JavaScript ----
static const char s_html[] =
"<!DOCTYPE html>\n"
"<html><head>"
"<meta charset=\"utf-8\">"
"<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>PicoPDU</title>"
"<style>"
"*{box-sizing:border-box}"
"body{font-family:monospace;background:#1a1a2e;color:#eee;"
     "max-width:460px;margin:0 auto;padding:16px}"
"h2{color:#0af;margin:0 0 4px}"
"#clk{font-size:20px;color:#8af;margin:0 0 12px}"
"table{width:100%;border-collapse:collapse}"
"th,td{border:1px solid #333;padding:8px;text-align:center}"
"th{background:#222;color:#aaa}"
".on{background:#0d5c1e;color:#4f8;font-weight:bold}"
".off{background:#5c1010;color:#f88}"
"button{padding:4px 16px;background:#0af;color:#000;"
        "border:none;border-radius:3px;cursor:pointer;"
        "font-family:monospace;font-size:13px}"
"button:active{opacity:.7}"
"#dot{display:inline-block;width:8px;height:8px;"
      "border-radius:50%;background:#0f0;margin-left:6px}"
"</style></head><body>"
"<h2>&#x26A1; PicoPDU <span id=\"dot\"></span></h2>"
"<p id=\"clk\">--:--:--</p>"
"<table id=\"tbl\">"
"<tr><th>CH</th><th>State</th><th>Action</th></tr>"
"</table>"
"<script>"
"let ok=false;"
"async function load(){"
"  try{"
"    const d=await(await fetch('/api/state')).json();"
"    document.getElementById('clk').textContent=d.t;"
"    let h='<tr><th>CH</th><th>State</th><th>Action</th></tr>';"
"    for(let i=0;i<8;i++){"
"      const on=d.ch[i];"
"      h+='<tr><th>'+(i+1)+'</th>'"
"        +'<td class=\"'+(on?'on':'off')+'\">'+(on?'ON':'OFF')+'</td>'"
"        +'<td><button onclick=\"tog('+(i+1)+')\">Toggle</button></td></tr>';"
"    }"
"    document.getElementById('tbl').innerHTML=h;"
"    document.getElementById('dot').style.background='#0f0';"
"  }catch(e){"
"    document.getElementById('dot').style.background='#f40';"
"  }"
"}"
"async function tog(n){"
"  document.getElementById('dot').style.background='#fa0';"
"  await fetch('/cgi/toggle?ch='+n);"
"  load();"
"}"
"load();"
"setInterval(load,1000);"
"</script>"
"</body></html>\n";

// ---- JSON state buffer ----
static char s_json[96];

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
        "{\"t\":\"%s\",\"ch\":[%d,%d,%d,%d,%d,%d,%d,%d]}",
        tbuf,
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
