cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
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

# เปลี่ยนตามบอร์ดที่ใช้: pico2 หรือ pico2_w
set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(PicoPDU C CXX ASM)

pico_sdk_init()

# CMSIS-DSP — CMSIS Core headers อยู่ใน pico-sdk btstack
set(CMSISCORE "$ENV{USERPROFILE}/.pico-sdk/sdk/2.2.0/lib/btstack/port/stm32-f4discovery-usb/Drivers/CMSIS/Core")
set(CMSISDSP_INSTALL OFF CACHE BOOL "" FORCE)
set(CMSISDSP ${CMAKE_CURRENT_SOURCE_DIR}/lib/CMSIS-DSP)
set(ARM_CPU "cortex-m33")
set(ARM_ARCH_PROFILE "M")
set(DISABLEFLOAT16 ON)
add_subdirectory(${CMSISDSP} CMSIS-DSP)

add_executable(PicoPDU src/main.c)

pico_set_program_name(PicoPDU "PicoPDU")
pico_set_program_version(PicoPDU "0.1")

pico_enable_stdio_uart(PicoPDU 0)
pico_enable_stdio_usb(PicoPDU 1)

target_link_libraries(PicoPDU pico_stdlib m CMSISDSP)

target_compile_options(PicoPDU PRIVATE -O2)

target_include_directories(PicoPDU PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)

pico_add_extra_outputs(PicoPDU)
