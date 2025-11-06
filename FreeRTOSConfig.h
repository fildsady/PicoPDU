/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * การอนุญาตนี้มอบให้ฟรี โดยไม่มีค่าใช้จ่าย ให้กับบุคคลใด ๆ ที่ได้รับสำเนาของ
 * ซอฟต์แวร์นี้และเอกสารประกอบ (ที่เรียกว่า "ซอฟต์แวร์") เพื่อดำเนินการใน
 * ซอฟต์แวร์โดยไม่มีข้อจำกัด รวมถึงสิทธิ์ในการใช้, คัดลอก, แก้ไข, รวม, เผยแพร่,
 * แจกจ่าย, อนุญาตให้ใช้ และ/หรือขายสำเนาของซอฟต์แวร์ และอนุญาตให้บุคคลที่
 * ซอฟต์แวร์ถูกมอบให้ทำเช่นนั้น ภายใต้เงื่อนไขต่อไปนี้:
 *
 * คำประกาศลิขสิทธิ์ข้างต้นและคำอนุญาตนี้จะต้องรวมอยู่ในสำเนาทั้งหมดหรือส่วนที่
 * สำคัญของซอฟต์แวร์
 *
 * ซอฟต์แวร์นี้จัดเตรียม "ตามที่เป็น" โดยไม่มีการรับประกันใด ๆ ไม่ว่าจะเป็น
 * การรับประกันโดยชัดแจ้งหรือโดยนัย รวมถึงแต่ไม่จำกัดเพียงการรับประกันความ
 * สามารถในการขาย, ความเหมาะสมสำหรับวัตถุประสงค์เฉพาะ และการไม่ละเมิด
 * ในกรณีใด ๆ ผู้เขียนหรือผู้ถือสิทธิลิขสิทธิ์จะไม่รับผิดชอบต่อการเรียกร้อง,
 * ความเสียหาย หรือความรับผิดอื่น ๆ ไม่ว่าจะเป็นในกรอบของสัญญา, ความเสียหาย
 * หรืออื่น ๆ ที่เกิดจาก, จาก หรือในความเชื่อมโยงกับซอฟต์แวร์หรือการใช้หรือ
 * การจัดการอื่น ๆ ในซอฟต์แวร์
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 แท็บ == 4 ช่องว่าง!
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * การกำหนดค่าที่เฉพาะเจาะจงกับแอปพลิเคชัน
 *
 * การกำหนดค่าต่าง ๆ เหล่านี้ควรได้รับการปรับให้เหมาะกับฮาร์ดแวร์และข้อกำหนด
 * ของแอปพลิเคชันของคุณ
 *
 * พารามิเตอร์เหล่านี้จะถูกอธิบายในส่วน 'CONFIGURATION' ของเอกสาร API ของ FreeRTOS
 * ที่มีอยู่บนเว็บไซต์ FreeRTOS.org
 *
 * ดูที่ http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* การจัดการกับการเปลี่ยนแปลง */
#define configUSE_PREEMPTION                    1  // ใช้การเปลี่ยนแปลง (preemption)
#define configUSE_TICKLESS_IDLE                 0  // ไม่ใช้โหมด Idle ที่ไม่ใช้ Tick
#define configUSE_IDLE_HOOK                     0  // ไม่ใช้ Idle Hook
#define configUSE_TICK_HOOK                     0  // ไม่ใช้ Tick Hook
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 ) // ความถี่ของ Tick = 1000 Hz
#define configMAX_PRIORITIES                    32 // จำนวนความสำคัญสูงสุดของ Task = 32
#define configMINIMAL_STACK_SIZE                ( configSTACK_DEPTH_TYPE ) 256 // ขนาดของ Stack ที่ขั้นต่ำ = 256
#define configUSE_16_BIT_TICKS                  0  // ไม่ใช้ Tick ขนาด 16 บิต

#define configIDLE_SHOULD_YIELD                 1  // Task Idle ควรยอมให้ Task อื่น

/* การซิงโครไนเซชัน */
#define configUSE_MUTEXES                       1  // ใช้ Mutexes
#define configUSE_RECURSIVE_MUTEXES             1  // ใช้ Recursive Mutexes
#define configUSE_APPLICATION_TASK_TAG          0  // ไม่ใช้ Application Task Tag
#define configUSE_COUNTING_SEMAPHORES           1  // ใช้ Counting Semaphores
#define configQUEUE_REGISTRY_SIZE               8  // ขนาดของ Queue Registry = 8
#define configUSE_QUEUE_SETS                    1  // ใช้ Queue Sets
#define configUSE_TIME_SLICING                  1  // ใช้ Time Slicing
#define configUSE_NEWLIB_REENTRANT              0  // ไม่ใช้ Newlib Reentrant
// todo: ต้องการสิ่งนี้สำหรับ lwip FreeRTOS sys_arch เพื่อการคอมไพล์
#define configENABLE_BACKWARD_COMPATIBILITY     1  // เปิดใช้งานความเข้ากันได้ย้อนหลัง
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5  // จำนวน Thread Local Storage Pointers = 5

/* ระบบ */
#define configSTACK_DEPTH_TYPE                  uint32_t  // ประเภทข้อมูลสำหรับความลึกของ Stack
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t    // ประเภทข้อมูลสำหรับความยาวของ Message Buffer

/* การจัดการหน่วยความจำ */
#define configSUPPORT_STATIC_ALLOCATION         0  // ไม่สนับสนุนการจัดสรรหน่วยความจำแบบ Static
#define configSUPPORT_DYNAMIC_ALLOCATION        1  // สนับสนุนการจัดสรรหน่วยความจำแบบ Dynamic
#define configTOTAL_HEAP_SIZE                   (128*1024) // ขนาดของ Heap = 128 KB
#define configAPPLICATION_ALLOCATED_HEAP        0  // ไม่ใช้ Heap ที่จัดสรรโดยแอปพลิเคชัน

/* การจัดการ Hook */
#define configCHECK_FOR_STACK_OVERFLOW          0  // ไม่ตรวจสอบ Stack Overflow
#define configUSE_MALLOC_FAILED_HOOK            0  // ไม่ใช้ Malloc Failed Hook
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0  // ไม่ใช้ Daemon Task Startup Hook

/* การรวบรวมสถิติการทำงานและ Task */
#define configGENERATE_RUN_TIME_STATS           0  // ไม่สร้างสถิติการทำงาน
#define configUSE_TRACE_FACILITY                1  // ใช้ Trace Facility
#define configUSE_STATS_FORMATTING_FUNCTIONS    0  // ไม่ใช้ฟังก์ชันการจัดรูปแบบสถิติ

/* การใช้ Co-routine */
#define configUSE_CO_ROUTINES                   0  // ไม่ใช้ Co-routines
#define configMAX_CO_ROUTINE_PRIORITIES         1  // จำนวนความสำคัญสูงสุดของ Co-routines = 1

/* การใช้ Software Timer */
#define configUSE_TIMERS                        1  // ใช้ Software Timers
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 ) // ความสำคัญของ Timer Task
#define configTIMER_QUEUE_LENGTH                10 // ขนาดของ Timer Queue = 10
#define configTIMER_TASK_STACK_DEPTH            1024 // ขนาดของ Stack สำหรับ Timer Task = 1024

/* การกำหนดพฤติกรรมการซ้อนการตอบสนองของ Interrupt */
/*
#define configKERNEL_INTERRUPT_PRIORITY         [ขึ้นอยู่กับโปรเซสเซอร์]
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    [ขึ้นอยู่กับโปรเซสเซอร์และแอปพลิเคชัน]
#define configMAX_API_CALL_INTERRUPT_PRIORITY   [ขึ้นอยู่กับโปรเซสเซอร์และแอปพลิเคชัน]
*/

/* SMP port only */
#define configNUMBER_OF_CORES                   2
#define configRUN_MULTIPLE_PRIORITIES           1
#if configNUMBER_OF_CORES > 1
#define configUSE_CORE_AFFINITY                 1
#endif
#define configUSE_PASSIVE_IDLE_HOOK             0

/* RP2040 specific */
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

/* RP2350 grows some features */
#define configENABLE_FPU                        1
#define configENABLE_MPU                        0
#define configENABLE_TRUSTZONE                  0
#define configRUN_FREERTOS_SECURE_ONLY          1
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    16

#include <assert.h>
/* กำหนดเพื่อจับข้อผิดพลาดระหว่างการพัฒนา */
#define configASSERT(x)                         assert(x)

/* กำหนดให้ฟังก์ชัน API ที่ต้องการรวมเข้าไปในโปรแกรม, หรือกำหนดเป็น 0 เพื่อไม่รวมฟังก์ชัน API */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1

/* รวมไฟล์หัวข้อที่กำหนด trace macro ได้ที่นี่ */

#endif /* FREERTOS_CONFIG_H */