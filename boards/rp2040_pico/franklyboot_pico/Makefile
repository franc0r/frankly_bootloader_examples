#
# Makefile of the RP2040 Pico Bootloader Firmware
#
#

PROJECT_NAME := franklyboot_pico

# Paths -----------------------------------------------------------------------

BUILD_DIR = ./build

# Header / Source Files -------------------------------------------------------

DEFINES := RP2040
DEFINES += PICO_BOARD=pico
DEFINES += PICO_NO_HARDWARE=0
DEFINES += PICO_COPY_TO_RAM=0
DEFINES += PICO_CXX_ENABLE_EXCEPTIONS=0
DEFINES += PICO_NO_FLASH=0
DEFINES += PICO_NO_FPGA_CHECK=0
DEFINES += PICO_USE_BLOCKED_RAM=0

# Core includes
INCLUDE_DIRS := Core/Inc

# No Pico SDK includes needed - using minimal register-level approach

# Frankly Bootloader library includes
INCLUDE_DIRS += ../../../../frankly-bootloader/include

# Core source files
SRCS_FILES := Core/Src/main.c
SRCS_FILES += Core/Src/bootloader_api.cpp
SRCS_FILES += Core/Startup/startup.S
SRCS_FILES += Core/Startup/boot2_generic.S

# Frankly Bootloader library sources
SRCS_FILES += ../../../../frankly-bootloader/src/francor/franklyboot/msg.cpp

# No Pico SDK sources needed - using minimal register-level approach

# Linker script
LD_SCRIPT = memmap.ld

# Setup C-Version -------------------------------------------------------------

C_VER		:= -std=gnu11
CXX_VER		:= -std=gnu++17

# Configuration ---------------------------------------------------------------

include ../../../make/toolchain.mk

# Core configuration for RP2040 (Cortex-M0+)
FLAGS_CORE := -mcpu=cortex-m0plus    # RP2040 uses Cortex-M0+
FLAGS_CORE += -mthumb                 # Thumb instruction set
# Note: No FPU flags for Cortex-M0+

# Optimization level (size optimized for minimal bootloader)
FLAGS_OPTIM := -Os -g

# Additional linker flags for RP2040
FLAGS_LD += -nostartfiles

# Include build area
include ../../../make/build.mk

# Additional rules for UF2 generation
.PHONY: uf2
uf2: $(BUILD_DIR)/$(PROJECT_NAME).elf
	@echo ""
	@echo "--------------------------------------------------------------------"
	@echo "Converting to UF2 format for RP2040"
	@echo "--------------------------------------------------------------------"
	@if command -v elf2uf2 >/dev/null 2>&1; then \
		elf2uf2 $(BUILD_DIR)/$(PROJECT_NAME).elf $(BUILD_DIR)/$(PROJECT_NAME).uf2; \
		echo "Generated $(PROJECT_NAME).uf2"; \
	else \
		echo "Warning: elf2uf2 not found, UF2 generation skipped"; \
		echo "Install pico-sdk tools or copy .bin manually to Pico in bootloader mode"; \
	fi
	@echo "--------------------------------------------------------------------"

# Override default all target to include UF2 generation
all: pre-info $(BUILD_DIR)/$(PROJECT_NAME) size objcopy uf2
	@echo ""
	@echo "==> Done"
	@echo "To flash: Hold BOOTSEL button, connect Pico, copy $(PROJECT_NAME).uf2 to RPI-RP2 drive"