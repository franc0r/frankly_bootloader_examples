cmake_minimum_required(VERSION 3.13)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Project config
project(example_app_pico C CXX ASM)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Initialize the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(${PROJECT_NAME}
    Core/Src/main.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_watchdog
    hardware_gpio
)

# Enable USB CDC for stdio
#pico_enable_stdio_usb(${PROJECT_NAME} 1)
#pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Enable compile commands json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use custom linker script
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/rp2040_app.ld)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# Add custom command to display memory usage
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Memory usage:"
)
