#pragma once
#include <stdbool.h>

typedef enum {
    CMD_PLAY, CMD_STOP, CMD_NEXT, CMD_PREV,
    CMD_VOLUME, CMD_REPEAT_ALL, CMD_REPEAT_ONE, CMD_REPEAT_OFF, CMD_UNKNOWN,
} cmd_type_t;

typedef struct { cmd_type_t type; int arg; } command_t;

typedef enum { UI_TRACK, UI_VOLUME } ui_mode_t;

typedef struct {
    int       track;
    int       volume;
    bool      playing;
    ui_mode_t mode;
    bool      wifi_connected;
    char      ip[16];  // "xxx.xxx.xxx.xxx\0"
} lcd_state_t;

// set true by task_webserver after cyw43_arch_init() succeeds
// read by task_status_led before using CYW43 LED API
extern volatile bool cyw43_ready;
