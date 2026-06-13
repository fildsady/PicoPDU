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

set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(PICO_PLATFORM rp2350-arm-s)

include(pico_sdk_import.cmake)

set(FREERTOS_KERNEL_PATH "$ENV{USERPROFILE}/FreeRTOS-Kernel" CACHE PATH "FreeRTOS Kernel path")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(PicoPDU C CXX ASM)

pico_sdk_init()

# Convert index.html -> index_html.h at build time
set(HTML_SRC  ${CMAKE_CURRENT_SOURCE_DIR}/src/web/index.html)
set(HTML_HDR  ${CMAKE_CURRENT_BINARY_DIR}/index_html.h)
add_custom_command(
    OUTPUT  ${HTML_HDR}
    COMMAND "$ENV{USERPROFILE}/.pico-sdk/python/3.13.7/python.exe"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/html2c.py ${HTML_SRC} ${HTML_HDR}
    DEPENDS ${HTML_SRC}
    COMMENT "Generating index_html.h from index.html"
)
add_custom_target(gen_html DEPENDS ${HTML_HDR})

add_executable(PicoPDU src/main.c src/i2c_lcd.c src/http_server.c)
add_dependencies(PicoPDU gen_html)

pico_set_program_name(PicoPDU "PicoPDU")
pico_set_program_version(PicoPDU "0.1")

pico_enable_stdio_uart(PicoPDU 1)
pico_enable_stdio_usb(PicoPDU 0)

target_link_libraries(PicoPDU
    pico_stdlib
    hardware_i2c
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_lwip_sntp
    FreeRTOS-Kernel-Heap4
)


target_include_directories(PicoPDU PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_options(${PROJECT_NAME} PRIVATE
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
)

pico_add_extra_outputs(PicoPDU)
