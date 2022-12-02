cmake_minimum_required (VERSION 3.7.2)

# Project Setup -------------------------------------------------------------------------------------------------------

project(franklyboot-nucleo-g431rb-bootloader)

# Compiler Setup ------------------------------------------------------------------------------------------------------

# Change compiler path if necessary
set(COMPILER_PATH "/opt/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi/bin/")

# Compiler Config -----------------------------------------------------------------------------------------------------

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_SYSTEM_NAME Generic)  # Set system to generic
set(CMAKE_C_COMPILER   arm-none-eabi-gcc) # Setup C compiler to arm none eabi
set(CMAKE_CXX_COMPILER arm-none-eabi-g++) # Setup C++ compiler to arm none eabi

# Compiler Options ----------------------------------------------------------------------------------------------------

set(CMAKE_C_STANDARD 11)   # Enable C11 standard
set(CMAKE_CXX_STANDARD 14) # Enable C++14 standard

set(GENERAL_COMPILER_OPTIONS
    -mcpu=cortex-m4 
    --specs=nano.specs
    -mfpu=fpv4-sp-d16 
    -mfloat-abi=hard 
    -mthumb
    -ffunction-sections 
    -fdata-sections 
    -Wall 
    -fstack-usage
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GENERAL_COMPILER_OPTIONS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GENERAL_COMPILER_OPTIONS} -fno-exceptions -fno-rtti -fno-use-cxa-atexit")


# Header / Src files --------------------------------------------------------------------------------------------------

set(SRC_CORE
    Core/Src/bootloader_api.cpp
    Core/Src/main.c
    Core/Startup/startup.S
)

set(EXECUTABLE ${PROJECT_NAME}.out)

add_executable(${EXECUTABLE} ${SRC_CORE})

target_compile_definitions(${EXECUTABLE} PRIVATE
        -DSTM32G431xx
)

target_include_directories(${EXECUTABLE} PRIVATE
        Core/Inc
        Drivers/CMSIS/Device/ST/STM32G4xx/Include
        Drivers/CMSIS/Include
        ../../frankly-bootloader/include/
)

target_link_options(${EXECUTABLE} PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/STM32G431RBTX_FLASH.ld
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -specs=nano.specs
    -lc
    -lm
    -lnosys
    -Wl,-Map=${PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
)

add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND arm-none-eabi-size ${EXECUTABLE}
)

add_custom_command(TARGET ${EXECUTABLE}
    POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex ${EXECUTABLE} ${PROJECT_NAME}.hex
    COMMAND arm-none-eabi-objcopy -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
)