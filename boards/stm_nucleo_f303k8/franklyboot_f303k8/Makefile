#
# Makefile of the Firmware Example Project
#
# 

PROJECT_NAME := franklyboot_nucleo_f303k8

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := STM32F303x8

INCLUDE_DIRS := Core/Inc
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32F3xx/Include
INCLUDE_DIRS += Drivers/CMSIS/Include
INCLUDE_DIRS += ../../../../frankly-bootloader/include

SRCS_FILES := Core/Src/main.c
SRCS_FILES += Core/Src/syscalls.c
SRCS_FILES += Core/Src/bootloader_api.cpp
SRCS_FILES += Core/Startup/startup.S
SRCS_FILES += ../../../../frankly-bootloader/src/francor/franklyboot/msg.cpp

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
FLAGS_OPTIM := -Os -g

# Include build area
include ../../../make/build.mk