#
# Makefile of the Firmware Example Project
#
# 

PROJECT_NAME := franklyboot_pico

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := RP2040

INCLUDE_DIRS := Core/Inc

SRCS_FILES := Core/Startup/boot2.S
SRCS_FILES += Core/Startup/startup.S
SRCS_FILES += Core/Src/main.c

LD_SCRIPT = rp2040.ld

# Setup C-Version -------------------------------------------------------------

C_VER		:= -std=gnu11
CXX_VER		:= -std=gnu++17

# Configuration ---------------------------------------------------------------

include ../../../make/toolchain.mk

# Core configuration
FLAGS_CORE := -mcpu=cortex-m0plus                  # Setup Core to M0
FLAGS_CORE += -mthumb							   # Instruction set THUMB

# Optimization level
FLAGS_OPTIM := -Os -g

# Include build area
include ../../../make/build.mk