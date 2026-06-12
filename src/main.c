#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

void task_blink(void *pvParameters)
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    while (1)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

int main()
{
    stdio_init_all();

    xTaskCreate(task_blink, "Blink_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1){};
}
