# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")

# ระบุพาธ FreeRTOS Kernel
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

# นำเข้า Pico SDK
include(pico_sdk_import.cmake)

# นำเข้า FreeRTOS CMake scripts (ของ SMP branch)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

project(RP2350_RTOS_Project_Assambly C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(RP2350_RTOS_Project_Assambly RP2350_RTOS_Project_Assambly.c )

pico_set_program_name(RP2350_RTOS_Project_Assambly "RP2350_RTOS_Project_Assambly")
pico_set_program_version(RP2350_RTOS_Project_Assambly "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(RP2350_RTOS_Project_Assambly 0)
pico_enable_stdio_usb(RP2350_RTOS_Project_Assambly 1)

# Add the standard library to the build
target_link_libraries(RP2350_RTOS_Project_Assambly
        pico_stdlib
        pico_multicore
        hardware_adc
        FreeRTOS-Kernel-Heap4
        )

# Add the standard include files to the build
target_include_directories(RP2350_RTOS_Project_Assambly PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(RP2350_RTOS_Project_Assambly)

