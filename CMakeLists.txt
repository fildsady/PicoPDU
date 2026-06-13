cmake_minimum_required(VERSION 3.13)
# กำหนดเวอร์ชันขั้นต่ำของ CMake ที่จำเป็นต้องใช้ (ในที่นี้คือ 3.13)

set(CMAKE_C_STANDARD 11)
# กำหนดให้ใช้มาตรฐานภาษา C เวอร์ชัน C11 สำหรับไฟล์ C
set(CMAKE_CXX_STANDARD 17)
# กำหนดให้ใช้มาตรฐานภาษา C++ เวอร์ชัน C++17 สำหรับไฟล์ C++
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# เปิดใช้งานการสร้างไฟล์ compile_commands.json ซึ่งมีประโยชน์สำหรับเครื่องมือพัฒนา (เช่น VS Code)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
# ส่วนนี้ถูกสร้างโดยอัตโนมัติ (Scaffolding) เพื่อรองรับส่วนขยาย (Extension) ของ VS Code
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico2_w CACHE STRING "Board type")
# กำหนดบอร์ดที่ใช้เป็น 'pico2' (หากคุณใช้ Pico W หรือบอร์ดอื่น ควรปรับแก้ที่นี่)
# CACHE STRING คือการบันทึกค่าไว้ใน CMake cache เพื่อใช้งานซ้ำ

# นำเข้า Pico SDK
include(pico_sdk_import.cmake)

project(PicoPDU C CXX ASM)
# กำหนดชื่อโปรเจกต์เป็น PicoPDU
# และระบุภาษาที่ใช้ในโปรเจกต์ (C, CXX, ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
# เรียกใช้ฟังก์ชันจาก Pico SDK เพื่อเริ่มต้นการตั้งค่าสภาพแวดล้อมที่จำเป็นทั้งหมด

# Add executable. Default name is the project name, version 0.1
add_executable(PicoPDU
    src/main.c
    src/dfplayer.c
    src/i2c_lcd.c
    src/web_server.c
)
# สร้าง Target ที่เป็นไฟล์ปฏิบัติการ (Executable) ชื่อเดียวกับชื่อโปรเจกต์
# โดยใช้ไฟล์ต้นฉบับคือ main.c ในการคอมไพล์

pico_set_program_name(PicoPDU "PicoPDU")
# กำหนดชื่อสุดท้ายของโปรแกรมที่คอมไพล์เสร็จ (เช่น ชื่อไฟล์ .elf หรือ .uf2)
pico_set_program_version(PicoPDU "0.1")
# กำหนดเวอร์ชันของโปรแกรม

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(PicoPDU 0)
# ปิดใช้งาน Standard I/O (stdio) ผ่าน UART (Serial Port) (ค่า 0)
pico_enable_stdio_usb(PicoPDU 1)
# เปิดใช้งาน Standard I/O (stdio) ผ่าน USB (ค่า 1) ซึ่งจำเป็นสำหรับฟังก์ชัน printf ใน Pico

# Add the standard library to the build
target_link_libraries(PicoPDU
        pico_stdlib
        hardware_uart
        hardware_i2c
        pico_multicore
        # Pico W WiFi + lwIP (bare-metal poll mode — Core 1 owns CYW43)
        pico_cyw43_arch_lwip_poll
        pico_lwip_http
)
# เชื่อมโยง (Link) Libraries ที่ Target ต้องใช้ในการคอมไพล์และรัน:
# - pico_stdlib: Libraries พื้นฐานของ Pico
# - pico_multicore: Libraries สำหรับใช้งาน Multi-core
# - hardware_adc: Libraries สำหรับใช้งาน Analog-to-Digital Converter
# - FreeRTOS-Kernel-Heap4: Libraries หลักของ FreeRTOS Kernel พร้อมวิธีการจัดสรรหน่วยความจำแบบ Heap4

# Add the standard include files to the build
target_include_directories(PicoPDU PRIVATE
 ${CMAKE_CURRENT_LIST_DIR}
 ${CMAKE_CURRENT_SOURCE_DIR}/inc
)
# กำหนดพาธของ Header File (Include Directories) ให้กับ Target
# - ${CMAKE_CURRENT_LIST_DIR}: พาธของไฟล์ CMakeLists.txt ปัจจุบัน
# - ${CMAKE_CURRENT_SOURCE_DIR}/inc: พาธไปยังโฟลเดอร์ 'inc' (หากมี Header File ที่กำหนดเอง)

# แสดง memory usage หลัง build
target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)
pico_add_extra_outputs(PicoPDU)
# สั่งให้ Pico SDK สร้างไฟล์เอาต์พุตเพิ่มเติม (เช่น .uf2 สำหรับอัปโหลด)