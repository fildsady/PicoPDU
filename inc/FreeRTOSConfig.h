/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com
 *
 * ============================================================
 * 🧩 ไฟล์นี้คือ FreeRTOSConfig.h
 * ------------------------------------------------------------
 * ไฟล์นี้ใช้สำหรับ "กำหนดค่าการทำงานของ FreeRTOS" ให้เหมาะกับฮาร์ดแวร์
 * และความต้องการของแอปพลิเคชัน เช่น:
 *  - จำนวน Task สูงสุด
 *  - การใช้หน่วยความจำ
 *  - การเปิด/ปิดฟีเจอร์ต่าง ๆ (Mutex, Timer, Hook Function ฯลฯ)
 *
 * โปรแกรมหลัก (main.c) จะต้อง include ไฟล์นี้ผ่าน header ของ FreeRTOS
 * เพื่อให้ kernel รู้ว่าควรตั้งค่าการทำงานอย่างไร
 *
 * 🔹 เหมาะสำหรับบอร์ด Raspberry Pi Pico / RP2040 / RP2350
 * ============================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * 🧠 การตั้งค่าพื้นฐานของระบบ (System Behavior)
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                    1   // ใช้ระบบ Preemptive Multitasking (Task ที่มี priority สูงจะขัดจังหวะได้)
#define configUSE_TICKLESS_IDLE                 0   // ปิดโหมดประหยัดพลังงานแบบ Tickless Idle
#define configUSE_IDLE_HOOK                     0   // ปิดการใช้ฟังก์ชัน Idle Hook
#define configUSE_TICK_HOOK                     0   // ปิดการใช้ Tick Hook
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // ตั้งค่า tick rate = 1000Hz (1ms ต่อ tick)
#define configMAX_PRIORITIES                    32  // จำนวนระดับความสำคัญของ task สูงสุด = 32
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 256 // ขนาด stack เริ่มต้นของ task = 256 words
#define configUSE_16_BIT_TICKS                  0   // ใช้ตัวนับ tick 32-bit (ตั้งเป็น 0 ถ้า CPU เป็น 32-bit)
#define configIDLE_SHOULD_YIELD                 1   // ให้ task Idle ยอมสละ CPU ให้ task อื่นได้

/*-----------------------------------------------------------
 * 🔄 การซิงโครไนซ์และการสื่อสาร (Synchronization & IPC)
 *----------------------------------------------------------*/
#define configUSE_MUTEXES                       1   // เปิดใช้งาน Mutex
#define configUSE_RECURSIVE_MUTEXES             1   // เปิดใช้งาน Recursive Mutex (Mutex ซ้อนกัน)
#define configUSE_APPLICATION_TASK_TAG          0   // ปิดการใช้ Application Task Tag (ไม่ใช้ tag พิเศษใน task)
#define configUSE_COUNTING_SEMAPHORES           1   // เปิดใช้ Counting Semaphore
#define configQUEUE_REGISTRY_SIZE               8   // ขนาดของ Queue Registry (สำหรับ debug)
#define configUSE_QUEUE_SETS                    1   // เปิดใช้ Queue Set (รวมหลาย queue เข้าด้วยกัน)
#define configUSE_TIME_SLICING                  1   // เปิดใช้ time slicing (แบ่งเวลาระหว่าง task priority เดียวกัน)
#define configUSE_NEWLIB_REENTRANT              0   // ปิดการใช้ Newlib thread-safe (ไม่จำเป็นใน Pico)
#define configENABLE_BACKWARD_COMPATIBILITY     1   // เปิดโหมดเข้ากันได้กับเวอร์ชันเก่า
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5   // จำนวน pointer ต่อ task สำหรับเก็บข้อมูลเฉพาะ thread

/*-----------------------------------------------------------
 * 🧮 การจัดการหน่วยความจำ (Memory Management)
 *----------------------------------------------------------*/
#define configSTACK_DEPTH_TYPE                  uint32_t  // กำหนดชนิดข้อมูลของความลึก stack
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t    // ชนิดข้อมูลของ message buffer length
#define configSUPPORT_STATIC_ALLOCATION         0         // ไม่รองรับการจัดสรรหน่วยความจำแบบ static
#define configSUPPORT_DYNAMIC_ALLOCATION        1         // รองรับการจัดสรรหน่วยความจำแบบ dynamic
#define configTOTAL_HEAP_SIZE                   (200*1024) // 200KB — เพิ่มจาก 128KB เพื่อรองรับ lwIP + WiFi driver
#define configAPPLICATION_ALLOCATED_HEAP        0         // ไม่ใช้ heap ที่จัดสรรโดยแอปพลิเคชันเอง

/*-----------------------------------------------------------
 * ⚠️ Hook Function (ฟังก์ชัน callback พิเศษของระบบ)
 *----------------------------------------------------------*/
#define configCHECK_FOR_STACK_OVERFLOW          0   // ปิดการตรวจ stack overflow (แนะนำให้เปิดในงานจริง)
#define configUSE_MALLOC_FAILED_HOOK            0   // ปิด malloc failed hook (ใช้ถ้าต้องการจัดการ error)
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0   // ปิด daemon startup hook

/*-----------------------------------------------------------
 * 📊 สถิติและการ Debug (Run-time Statistics & Trace)
 *----------------------------------------------------------*/
#define configGENERATE_RUN_TIME_STATS           0   // ไม่เก็บสถิติการทำงาน
#define configUSE_TRACE_FACILITY                1   // เปิดการใช้ trace facility (ช่วย debug task)
#define configUSE_STATS_FORMATTING_FUNCTIONS    0   // ปิดฟังก์ชันแปลงสถิติเป็น string

/*-----------------------------------------------------------
 * 🧩 Co-routine (Task แบบเบา)
 *----------------------------------------------------------*/
#define configUSE_CO_ROUTINES                   0   // ปิด co-routines
#define configMAX_CO_ROUTINE_PRIORITIES         1   // ถ้าเปิด co-routine จะมี priority แค่ 1 ระดับ

/*-----------------------------------------------------------
 * ⏰ Software Timer Configuration
 *----------------------------------------------------------*/
#define configUSE_TIMERS                        1   // เปิดใช้งาน Software Timer
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 ) // Timer task มี priority สูงสุด - 1
#define configTIMER_QUEUE_LENGTH                10  // Queue ของ timer มีได้ 10 รายการ
#define configTIMER_TASK_STACK_DEPTH            1024 // ขนาด stack ของ timer task = 1024 words

/*-----------------------------------------------------------
 * ⚙️ การตั้งค่า Interrupt Priority (สำคัญกับ Cortex-M33)
 *----------------------------------------------------------*/
/*
 * บางค่า (เช่น configKERNEL_INTERRUPT_PRIORITY)
 * จะขึ้นอยู่กับโปรเซสเซอร์และต้องดูจากเอกสารของ MCU
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    16  // ระดับ interrupt สูงสุดที่สามารถใช้ FreeRTOS API ได้

/*-----------------------------------------------------------
 * 🧠 การตั้งค่าสำหรับระบบ SMP (Multi-core)
 *----------------------------------------------------------*/
#define configNUMBER_OF_CORES                   1   // single-core — CYW43 WiFi driver ไม่รองรับ FreeRTOS SMP
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_PASSIVE_IDLE_HOOK             0

/*-----------------------------------------------------------
 * 🪶 การตั้งค่าพิเศษของ RP2040/RP2350
 *----------------------------------------------------------*/
#define configSUPPORT_PICO_SYNC_INTEROP         1   // รองรับ sync API ของ Pico SDK
#define configSUPPORT_PICO_TIME_INTEROP         1   // รองรับ time API ของ Pico SDK

#define configENABLE_FPU                        1   // เปิดการใช้ FPU (Floating Point Unit)
#define configENABLE_MPU                        0   // ปิด MPU (Memory Protection Unit)
#define configENABLE_TRUSTZONE                  0   // ปิด TrustZone (ไม่มีใน Pico)
#define configRUN_FREERTOS_SECURE_ONLY          1   // รันเฉพาะ secure state (ไม่ใช้ TrustZone)

#include <assert.h>  // ใช้สำหรับตรวจสอบเงื่อนไขตอนพัฒนา

/*-----------------------------------------------------------
 * 🧩 การตรวจสอบความถูกต้อง (Assertion)
 *----------------------------------------------------------*/
#define configASSERT(x)                         assert(x)  // ถ้า x เป็น false → หยุดโปรแกรมตอน debug

/*-----------------------------------------------------------
 * 🧰 กำหนดว่าฟังก์ชัน API ใดของ FreeRTOS จะถูก include เข้ามา
 * (ตั้ง 1 = เปิดใช้งาน, 0 = ปิด)
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet                1   // ฟังก์ชันเปลี่ยน priority ของ task
#define INCLUDE_uxTaskPriorityGet               1   // ฟังก์ชันอ่าน priority ของ task
#define INCLUDE_vTaskDelete                     1   // ฟังก์ชันลบ task
#define INCLUDE_vTaskSuspend                    1   // ฟังก์ชันหยุดชั่วคราว task
#define INCLUDE_vTaskDelayUntil                 1   // ฟังก์ชันหน่วงเวลาแบบ absolute
#define INCLUDE_vTaskDelay                      1   // ฟังก์ชันหน่วงเวลาแบบ relative
#define INCLUDE_xTaskGetSchedulerState          1   // ฟังก์ชันดูสถานะ scheduler
#define INCLUDE_xTaskGetCurrentTaskHandle       1   // ฟังก์ชันดึง handle ของ task ปัจจุบัน
#define INCLUDE_uxTaskGetStackHighWaterMark     1   // ฟังก์ชันตรวจ stack usage
#define INCLUDE_xTaskGetIdleTaskHandle          1   // ฟังก์ชันดึง handle ของ idle task
#define INCLUDE_eTaskGetState                   1   // ฟังก์ชันดูสถานะของ task
#define INCLUDE_xTimerPendFunctionCall          1   // ฟังก์ชันรัน callback ผ่าน timer service
#define INCLUDE_xTaskAbortDelay                 1   // ฟังก์ชันยกเลิก delay ของ task
#define INCLUDE_xTaskGetHandle                  1   // ฟังก์ชันดึง handle ของ task จากชื่อ
#define INCLUDE_xTaskResumeFromISR              1   // ฟังก์ชัน resume task จาก ISR
#define INCLUDE_xQueueGetMutexHolder            1   // ฟังก์ชันตรวจสอบว่าใครถือ mutex อยู่

/*-----------------------------------------------------------
 * (Option) รวม header สำหรับ trace macro เพิ่มเติมได้ที่นี่
 *----------------------------------------------------------*/

#endif /* FREERTOS_CONFIG_H */
