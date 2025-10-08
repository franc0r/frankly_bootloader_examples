# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains firmware examples for FRANCOR's Frankly Bootloader system. It demonstrates bootloader and application firmware implementations for various STM32 microcontroller boards.

## Architecture

The project is organized into board-specific implementations under `boards/`:

- `stm_nucleo_g431rb/` - STM32G431RB Nucleo board implementations
- `stm_nucleo_f303k8/` - STM32F303K8 Nucleo board implementations
- `eduart_l431kb_can/` - EDUART L431KB CAN board implementation

Each board directory contains:
- `franklyboot_*` - Bootloader firmware implementation
- `example_app_*` - Example application firmware that works with the bootloader

The bootloader implementations integrate with the main Frankly Bootloader library (located at `../frankly-bootloader`) using C++ APIs in the `francor::franklyboot` namespace.

## Build System

### Prerequisites
- ARM GCC toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-g++`, etc.)
- The Frankly Bootloader library repository must be checked out as a sibling directory

### Build Commands

Build any specific firmware by navigating to its directory and running `make`:

```bash
# Build STM32G431RB bootloader
cd boards/stm_nucleo_g431rb/franklyboot_g431rb
make

# Build STM32G431RB example application
cd boards/stm_nucleo_g431rb/example_app_g431rb
make

# Build STM32F303K8 bootloader
cd boards/stm_nucleo_f303k8/franklyboot_f303k8
make

# Build STM32F303K8 example application
cd boards/stm_nucleo_f303k8/example_app_f303k8
make

# Build EDUART L431KB CAN bootloader
cd boards/eduart_l431kb_can/franklyboot_eduart_l431kb
make
```

### Build Targets

Each Makefile supports these targets:
- `all` (default) - Build the firmware and generate .elf, .hex, and .bin files
- `clean` - Remove build directory
- `size` - Display memory usage information
- `objcopy` - Convert .elf to .hex and .bin formats

### Build Configuration

The build system uses shared Makefiles in `make/`:
- `toolchain.mk` - ARM GCC toolchain configuration
- `build.mk` - Common build rules and targets

Key build characteristics:
- Cortex-M4 target with FPU support (`-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`)
- C11 and C++17 standards
- Optimized builds with size optimization (`-Os`) for bootloaders, debug optimization (`-Og`) for applications
- Nano.specs for reduced memory footprint

## Development Setup

1. Clone this repository
2. Clone the Frankly Bootloader library as a sibling directory:
   ```bash
   git clone https://github.com/franc0r/frankly-bootloader.git ../frankly-bootloader
   cd ../frankly-bootloader && git checkout devel
   ```
3. Ensure ARM GCC toolchain is installed and in PATH
4. Navigate to any board-specific directory and run `make`

## CI/CD

The project uses GitHub Actions for continuous integration. The pipeline builds all board examples using the `bausma/mbed-dev-env:latest` Docker image. See `.github/workflows/build.yaml` for the complete build matrix.