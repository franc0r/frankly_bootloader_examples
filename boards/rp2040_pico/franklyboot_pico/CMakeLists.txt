cmake_minimum_required(VERSION 3.13)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Project config
project(franklyboot_pico C CXX ASM)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Initialize the Pico SDK
pico_sdk_init()

# Path to Frankly Bootloader library (sibling directory)
set(FRANKLYBOOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../../frankly-bootloader)

# Create the executable
add_executable(${PROJECT_NAME}
    Core/Src/main.c
    Core/Src/bootloader_api.cpp
    Core/Src/usb_descriptors.c
    ${FRANKLYBOOT_PATH}/src/francor/franklyboot/msg.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${FRANKLYBOOT_PATH}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_flash
    hardware_sync
    hardware_watchdog
    tinyusb_device
    tinyusb_board
)

# Enable USB CDC
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Enable compile commands json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy complete app from flash to RAM
pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

# Use custom linker script
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/rp2040.ld)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

