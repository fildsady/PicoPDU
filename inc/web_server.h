#pragma once
#include "main_shared.h" // lcd_state_t, cmd_type_t, command_t

// WiFi credentials — เปลี่ยนก่อน build
#define WIFI_SSID  "your_ssid"
#define WIFI_PASS  "your_password"

void task_webserver(void *pvParameters);
