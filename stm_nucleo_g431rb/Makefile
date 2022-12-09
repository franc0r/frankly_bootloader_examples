#
# Makefile of the Firmware Example Project
#
# 

PROJECT_NAME := franklyboot_nucleo_g4931rb

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := STM32G431xx

INCLUDE_DIRS := Core/Inc
INCLUDE_DIRS += Drivers/CMSIS/Device/ST/STM32G4xx/Include
INCLUDE_DIRS += Drivers/CMSIS/Include
INCLUDE_DIRS += ../../frankly-bootloader/include

SRCS_FILES := Core/Src/main.c
SRCS_FILES += Core/Src/syscalls.c
SRCS_FILES += Core/Src/bootloader_api.cpp
SRCS_FILES += Core/Startup/startup.S
SRCS_FILES += ../../frankly-bootloader/src/francor/franklyboot/msg.cpp

LD_SCRIPT = STM32G431RBTX_FLASH.ld

# Setup C-Version -------------------------------------------------------------

C_VER		:= -std=gnu11
CXX_VER		:= -std=gnu++17

# Configuration ---------------------------------------------------------------

include ../make/toolchain.mk

# Core configuration
FLAGS_CORE := -mcpu=cortex-m4					   # Setup Core to M4
FLAGS_CORE += -mthumb							   # Instruction set THUMB
FLAGS_CORE += -mfpu=fpv4-sp-d16 -mfloat-abi=hard   # FPU Present

# Optimization level
FLAGS_OPTIM := -Og

# Include build area
include ../make/build.mk