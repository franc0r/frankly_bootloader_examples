cmake_minimum_required(VERSION 3.13)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Project config
project(franklyboot_pico C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(${PROJECT_NAME}
    Core/Src/main.c
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
)

# Enable compile commands json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Copy complete app from flash to RAM
pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

