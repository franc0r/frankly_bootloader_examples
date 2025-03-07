#
# Makefile of the Firmware Example Project
#
# 

PROJECT_NAME := example_app_f303k8

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := STM32F303x8
DEFINES += USE_HAL_DRIVER
DEFINES += DEBUG

INCLUDE_DIRS := Core/Inc
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32F3xx/Include
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32F3xx/Include/Legacy
INCLUDE_DIRS += Drivers/CMSIS/Include
INCLUDE_DIRS += Drivers/STM32F3xx_HAL_Driver/Inc
INCLUDE_DIRS += Drivers/STM32F3xx_HAL_Driver/Inc/Legacy


# Core Files
SRCS_FILES := Core/Src/main.c
SRCS_FILES += Core/Src/stm32f3xx_hal_msp.c
SRCS_FILES += Core/Src/stm32f3xx_it.c
SRCS_FILES += Core/Src/syscalls.c
SRCS_FILES += Core/Src/sysmem.c
SRCS_FILES += Core/Src/system_stm32f3xx.c
SRCS_FILES += Core/Startup/startup_stm32f303k8tx.S

# Drivers
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_cortex.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_exti.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash_ex.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_gpio.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr_ex.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc_ex.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim_ex.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart.c
SRCS_FILES += Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart_ex.c

LD_SCRIPT = STM32F303K8TX_FLASH.ld

# Setup C-Version -------------------------------------------------------------

C_VER		:= -std=gnu11
CXX_VER		:= -std=gnu++17

# Configuration ---------------------------------------------------------------

include ../../../make/toolchain.mk

# Core configuration
FLAGS_CORE := -mcpu=cortex-m4					   # Setup Core to M4
FLAGS_CORE += -mthumb							   # Instruction set THUMB
FLAGS_CORE += -mfpu=fpv4-sp-d16 -mfloat-abi=hard   # FPU Present

# Optimization level
FLAGS_OPTIM := -Og -ggdb

# Include build area
include ../../../make/build.mk