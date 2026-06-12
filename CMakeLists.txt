cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

project(WiFi_RTOS_Test C CXX ASM)
pico_sdk_init()

add_executable(WiFi_RTOS_Test
    src/main.c
)

pico_set_program_name(WiFi_RTOS_Test "WiFi_RTOS_Test")
pico_set_program_version(WiFi_RTOS_Test "0.1")

pico_enable_stdio_uart(WiFi_RTOS_Test 0)
pico_enable_stdio_usb(WiFi_RTOS_Test 1)

target_link_libraries(WiFi_RTOS_Test
    pico_stdlib
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_lwip_sys_freertos
)

target_include_directories(WiFi_RTOS_Test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_options(${PROJECT_NAME} PRIVATE -Wl,--print-memory-usage)
pico_add_extra_outputs(WiFi_RTOS_Test)
