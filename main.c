#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN1 0
#define LED_PIN2 1

void task_1(void *pvParameters)
{

    gpio_init(LED_PIN1);          
    gpio_set_dir(LED_PIN1, GPIO_OUT);

    while(1)
    {
        gpio_put(LED_PIN1, 1);  
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        gpio_put(LED_PIN1, 0);  
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}

void task_2(void *pvParameters)
{
     gpio_init(LED_PIN2);          
     gpio_set_dir(LED_PIN2, GPIO_OUT);

    while(1)
    {
        gpio_put(LED_PIN2, 1);  
        vTaskDelay(200 / portTICK_PERIOD_MS); 
        gpio_put(LED_PIN2, 0);  
        vTaskDelay(250 / portTICK_PERIOD_MS);
    } 
}
 
int main()
{
    stdio_init_all();

    xTaskCreate(task_1, "LED_Task_1", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "LED_Task_2", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while(1){};
}