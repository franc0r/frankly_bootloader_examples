cmake_minimum_required(VERSION 3.13)

# Set board type before including pico_sdk_import
set(PICO_BOARD pico)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Project configuration
project(franklyboot_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(${PROJECT_NAME}
    Core/Src/main.c
    Core/Src/bootloader_api.cpp
    ../../../../frankly-bootloader/src/francor/franklyboot/msg.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc
    ../../../../frankly-bootloader/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_gpio
    hardware_flash
    hardware_sync
    hardware_timer
    pico_unique_id
)

# Create UF2 output
pico_add_extra_outputs(${PROJECT_NAME})

# Use default linker script for now (can customize later for bootloader layout)
# pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/memmap_pico.ld)

# Use default Boot2 for the Pico board (will automatically choose the right one)
# pico_set_boot_stage2(${PROJECT_NAME} ${PICO_SDK_PATH}/src/rp2_common/boot_stage2/boot2_w25q080.S)

# Enable/disable stdio
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Set optimization for size
target_compile_options(${PROJECT_NAME} PRIVATE -Os)