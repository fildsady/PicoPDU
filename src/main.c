#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#define LED_PIN1 0
#define LED_PIN2 1
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 25
#endif

// ---------------------------------------------------------
// Task 1: ควบคุมไฟ LED ดวงที่ 1 (เชื่อมต่อที่ขา LED_PIN1)
// ทำหน้าที่กระพริบ LED โดยสว่าง 500 ms และ ดับ 500 ms
// ---------------------------------------------------------
void task_1(void *pvParameters)
{
    // กำหนดค่าเริ่มต้นให้กับขา GPIO และตั้งค่าให้เป็น Output (ส่งสัญญาณออก)
    gpio_init(LED_PIN1);          
    gpio_set_dir(LED_PIN1, GPIO_OUT);

    // ลูปการทำงานหลักของ Task (ทำงานวนซ้ำไม่รู้จบ)
    while(1)
    {
        // สั่งให้ LED ติด (ส่งค่าสถานะ High หรือ 1)
        gpio_put(LED_PIN1, 1);  
        // หน่วงเวลา 500 มิลลิวินาที (แปลงเวลาเป็นหน่วย Tick ของระบบปฏิบัติการ)
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ (ส่งค่าสถานะ Low หรือ 0)
        gpio_put(LED_PIN1, 0);  
        // หน่วงเวลา 500 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(500 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task 2: ควบคุมไฟ LED ดวงที่ 2 (เชื่อมต่อที่ขา LED_PIN2)
// ทำหน้าที่กระพริบ LED โดยสว่าง 200 ms และ ดับ 250 ms
// ---------------------------------------------------------
void task_2(void *pvParameters)
{
     // กำหนดค่าเริ่มต้นให้กับขา GPIO และตั้งค่าให้เป็น Output
     gpio_init(LED_PIN2);          
     gpio_set_dir(LED_PIN2, GPIO_OUT);

    // ลูปการทำงานหลักของ Task
    while(1)
    {
        // สั่งให้ LED ติด
        gpio_put(LED_PIN2, 1);  
        // หน่วงเวลา 200 มิลลิวินาที
        vTaskDelay(200 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ
        gpio_put(LED_PIN2, 0);  
        // หน่วงเวลา 250 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(250 / portTICK_PERIOD_MS);
    } 
}

// ---------------------------------------------------------
// Task Blink Builtin: ควบคุมไฟ LED ที่ติดมากับบอร์ด (Built-in LED)
// ทำหน้าที่กระพริบ LED โดยสว่าง 100 ms และ ดับ 100 ms
// ---------------------------------------------------------
void task_blink_builtin(void *pvParameters)
{
    // กำหนดค่าเริ่มต้นให้กับขา GPIO ของ Built-in LED และตั้งค่าให้เป็น Output
    gpio_init(PICO_DEFAULT_LED_PIN);          
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // ลูปการทำงานหลักของ Task
    while(1)
    {
        // สั่งให้ LED ติด
        gpio_put(PICO_DEFAULT_LED_PIN, 1);  
        // หน่วงเวลา 100 มิลลิวินาที
        vTaskDelay(100 / portTICK_PERIOD_MS); 
        // สั่งให้ LED ดับ
        gpio_put(PICO_DEFAULT_LED_PIN, 0);  
        // หน่วงเวลา 100 มิลลิวินาที ก่อนวนลูปใหม่
        vTaskDelay(100 / portTICK_PERIOD_MS);
    } 
}
 
// ---------------------------------------------------------
// ฟังก์ชัน main: จุดเริ่มต้นของโปรแกรม
// ทำหน้าที่ตั้งค่าเริ่มต้นของระบบ, สร้าง Task และเริ่มการทำงานของ OS
// ---------------------------------------------------------
int main()
{
    // เริ่มต้นระบบ Standard I/O (เช่น การใช้งาน UART สำหรับคำสั่ง printf)
    stdio_init_all();

    // สร้าง Task จำนวน 3 Task ลงในหน่วยความจำของ FreeRTOS โดยกำหนดพารามิเตอร์ดังนี้:
    // 1. ฟังก์ชันเป้าหมายของ Task
    // 2. ชื่อของ Task (สำหรับใช้ในการ Debug)
    // 3. ขนาดของ Stack ที่จองให้ Task นี้ (หน่วยเป็น words ไม่ใช่ bytes)
    // 4. พารามิเตอร์ที่จะส่งไปให้ Task (ในที่นี้ไม่มี จึงเป็น NULL)
    // 5. ระดับความสำคัญของ Task (Priority) ยิ่งค่าน้อย ความสำคัญยิ่งต่ำ (ตั้งไว้เท่ากันที่ 1)
    // 6. ตัวแปรเก็บ Handle ของ Task (ไม่ได้ใช้งาน จึงเป็น NULL)
    xTaskCreate(task_1, "LED_Task_1", 256, NULL, 1, NULL);
    xTaskCreate(task_2, "LED_Task_2", 256, NULL, 1, NULL);
    xTaskCreate(task_blink_builtin, "Blink_Builtin_Task", 256, NULL, 1, NULL);
    
    // สั่งให้ตัวจัดการการสลับ Task (Scheduler) ของ FreeRTOS เริ่มทำงาน
    // หลังจากบรรทัดนี้ไป CPU จะถูกควบคุมโดย FreeRTOS และเริ่มสลับการทำงานไปมาระหว่าง Task 1, 2 และ Builtin
    vTaskStartScheduler();

    // ลูปทำงานเผื่อไว้ในกรณีที่ Scheduler ทำงานผิดพลาดจนหลุดจากการทำงานปกติ
    // ถ้าโปรแกรมทำงานถูกต้อง บรรทัดนี้จะไม่มีวันถูกเรียกถึง
    while(1){};
}