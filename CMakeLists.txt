cmake_minimum_required(VERSION 3.12)
set(PROJECT webServer)

# Pull in SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/external/pico_extras_import_optional.cmake)

project(${PROJECT} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_executable(${PROJECT} main.c)

pico_add_extra_outputs(${PROJECT})

add_subdirectory(lib)
target_link_libraries(${PROJECT} PRIVATE pico_stdlib hardware_spi lfs)
target_include_directories(lfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/littlefs)

pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)
