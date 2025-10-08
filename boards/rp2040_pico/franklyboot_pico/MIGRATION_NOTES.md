# RP2040 Bootloader Migration Notes

This document describes the migration of the STM32G431 Frankly Bootloader to the RP2040 platform.

## Architecture Overview

### Dual-Core Design
- **Core0**: Runs the main bootloader logic and flash operations
- **Core1**: Handles USB CDC communication exclusively

### Communication Flow
```
USB CDC (Core1) <---> RX/TX FIFOs <---> Bootloader Logic (Core0)
```

## Key Differences from STM32

### 1. Flash Memory
- **STM32**: Flash at 0x08000000, 2KB pages
- **RP2040**: Flash at 0x10000000 (XIP), 4KB sectors
- Bootloader occupies first 16KB (4 pages)
- Application starts at 0x10004000

### 2. Communication
- **STM32**: LPUART1 hardware UART
- **RP2040**: USB CDC via TinyUSB library
- Uses circular FIFO buffers for inter-core communication

### 3. CRC Calculation
- **STM32**: Hardware CRC peripheral
- **RP2040**: Software CRC-32 implementation (no hardware CRC)

### 4. Device Reset
- **STM32**: NVIC_SystemReset()
- **RP2040**: watchdog_reboot()

### 5. Unique ID
- **STM32**: 96-bit UID at UID_BASE
- **RP2040**: 64-bit unique board ID via pico_unique_id

### 6. Autostart Storage
- **STM32**: TAMP backup register
- **RP2040**: Watchdog scratch register

## Memory Layout

```
0x10000000 - 0x10003F80: Bootloader code/data
0x10003F80 - 0x10004000: Device identification section (._dev_ident)
0x10004000 - 0x10200000: Application flash region
```

## Build System

### Prerequisites
- Pico SDK environment variable: `PICO_SDK_PATH`
- Frankly Bootloader library in sibling directory

### Build Commands
```bash
cd boards/rp2040_pico/franklyboot_pico
mkdir -p build && cd build
cmake ..
make
```

### Output Files
- `franklyboot_pico.elf` - ELF executable
- `franklyboot_pico.bin` - Binary image
- `franklyboot_pico.hex` - Intel HEX format
- `franklyboot_pico.uf2` - UF2 bootloader format (for drag-and-drop)

## USB Configuration

### VID/PID
- Vendor ID: 0xCAFE (placeholder - should be updated)
- Product ID: 0x4001

### USB Descriptors
- Device: CDC class
- Configuration: Single CDC interface
- Strings: FRANCOR e.V., Frankly Bootloader

## Important Implementation Notes

1. **Copy to RAM**: The bootloader runs entirely from RAM to allow flash operations
   - Set via `pico_set_binary_type(copy_to_ram)`

2. **Flash Operations**: Must disable interrupts during erase/program
   - Uses `save_and_disable_interrupts()` / `restore_interrupts()`

3. **Application Jump**: 
   - Deinitializes USB
   - Resets Core1
   - Sets VTOR to application address
   - Jumps to application reset handler

4. **Autostart Timer**: 2-second timeout with LED blinking
   - LED blinks every 250ms during wait
   - Turns off when jumping to application

## Files Modified/Created

### New Files
- `Core/Src/bootloader_api.cpp` - Main bootloader implementation
- `Core/Src/usb_descriptors.c` - USB device descriptors
- `Core/Inc/tusb_config.h` - TinyUSB configuration

### Updated Files
- `Core/Src/main.c` - Core initialization and multicore launch
- `Core/Inc/bootloader_api.h` - API declarations
- `Core/Inc/device_defines.h` - RP2040-specific defines
- `CMakeLists.txt` - Build configuration
- `rp2040.ld` - Linker script (already configured for RAM execution)

## Testing Checklist

- [ ] Bootloader builds without errors
- [ ] USB CDC device enumerates correctly
- [ ] Can receive bootloader commands via USB CDC
- [ ] Flash erase operations work
- [ ] Flash write operations work
- [ ] CRC calculation is correct
- [ ] Application validation works
- [ ] Autostart functionality (2s timeout)
- [ ] Manual application jump works
- [ ] Device reset works
- [ ] Unique ID retrieval works

## Known Limitations

1. Flash write granularity: RP2040 requires 256-byte aligned writes
2. No hardware CRC: Software implementation may be slower
3. USB CDC only: No UART fallback communication
4. Fixed 16KB bootloader size: Cannot be easily changed due to linker script

## Future Improvements

- [ ] Add UART communication fallback
- [ ] Implement proper vendor ID registration
- [ ] Add LED status codes for different bootloader states
- [ ] Optimize CRC calculation (use DMA or hardware accelerators if available)
- [ ] Add watchdog monitoring during flash operations
