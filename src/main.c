#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "arm_math.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN1 0
#define LED_PIN2 1
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// ---------------------------------------------------------
// Task 1: Control LED 1 (Connected to LED_PIN1)
// Blinks the LED: ON for 500 ms, OFF for 500 ms
// ---------------------------------------------------------
void task_1(void *pvParameters)
{
    // Initialize GPIO pin and set as Output
    gpio_init(LED_PIN1);          
    gpio_set_dir(LED_PIN1, GPIO_OUT);

    // Main task loop (runs indefinitely)
    while(1)
    {
        // Turn LED ON (Set High or 1)
        gpio_put(LED_PIN1, 1);  
        // Delay for 500 ms (Convert time to OS Ticks)
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        // Turn LED OFF (Set Low or 0)
        gpio_put(LED_PIN1, 0);  
        // Delay for 500 ms before next iteration
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task 2: Control LED 2 (Connected to LED_PIN2)
// Blinks the LED: ON for 200 ms, OFF for 250 ms
// ---------------------------------------------------------
void task_2(void *pvParameters)
{
     // Initialize GPIO pin and set as Output
     gpio_init(LED_PIN2);          
     gpio_set_dir(LED_PIN2, GPIO_OUT);

    // Main task loop
    while(1)
    {
        // Turn LED ON
        gpio_put(LED_PIN2, 1);  
        // Delay for 200 ms
        vTaskDelay(200 / portTICK_PERIOD_MS); 
        // Turn LED OFF
        gpio_put(LED_PIN2, 0);  
        // Delay for 250 ms before next iteration
        vTaskDelay(250 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task Blink Builtin: Control the built-in LED
// Blinks the LED: ON for 100 ms, OFF for 100 ms
// ---------------------------------------------------------
void task_blink_builtin(void *pvParameters)
{
    // Initialize Built-in LED GPIO pin and set as Output
    gpio_init(PICO_DEFAULT_LED_PIN);          
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Main task loop
    while(1)
    {
        // Turn LED ON
        gpio_put(PICO_DEFAULT_LED_PIN, 1);  
        // Delay for 100 ms
        vTaskDelay(100 / portTICK_PERIOD_MS); 
        // Turn LED OFF
        gpio_put(PICO_DEFAULT_LED_PIN, 0);  
        // Delay for 100 ms before next iteration
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task DSP: Digital Signal Processing
// Generates a simulated Sine Wave, adds Noise, and applies a Low-Pass Filter
// ---------------------------------------------------------
#define FILTER_TAPS 5
float filter_buffer[FILTER_TAPS] = {0};
int filter_index = 0;

void dsp_task(void *pvParameters)
{
    float t = 0.0f;
    
    while(1)
    {
        // 1. Simulate reading sensor data (Sine wave 1Hz + random noise)
        float true_signal = sinf(t) * 10.0f;
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 4.0f; // Noise swings between -2.0 and +2.0
        float raw_value = true_signal + noise;

        // 2. DSP Process: Apply Moving Average Filter (Basic FIR Filter)
        filter_buffer[filter_index] = raw_value;
        filter_index = (filter_index + 1) % FILTER_TAPS;

        float filtered_value = 0.0f;
        // Use a built-in function from the CMSIS-DSP library
        arm_mean_f32(filter_buffer, FILTER_TAPS, &filtered_value);

        // 3. Output results via UART/USB (can be viewed using a Serial Plotter)
        printf("Raw: %6.2f | Filtered: %6.2f\n", raw_value, filtered_value);

        t += 0.1f; // Increment simulation time

        // Delay to sample data at 10Hz (100 ms)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// main function: Entry point of the program
// Initializes the system, creates tasks, and starts the OS scheduler
// ---------------------------------------------------------
int main()
{

    stdio_init_all();

    xTaskCreate(task_1, "LED_Task_1", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "LED_Task_2", 256, NULL, 1, NULL);
    xTaskCreate(task_blink_builtin, "Blink_Builtin_Task", 256, NULL, 1, NULL);

    xTaskCreate(dsp_task, "DSP_Task", 1024, NULL, 2, NULL);

    vTaskStartScheduler(); 

    while(1){};
}