# Frankly Bootloader for RP2040 Pico

This is the RP2040 port of the Frankly Bootloader, migrated from the STM32G431 implementation.

## Features

- **Dual-Core Architecture**: 
  - Core0: Main bootloader logic and flash operations
  - Core1: USB CDC communication handling
- **USB CDC Communication**: Uses TinyUSB for device-to-host communication
- **Copy-to-RAM Execution**: Bootloader runs entirely from RAM to allow safe flash operations
- **128KB Bootloader Size**: Leaves 1.87MB for application firmware
- **Hardware Acceleration**: Software CRC-32 implementation
- **Autostart Support**: 2-second timeout with LED indication

## Memory Layout

```
0x10000000 - 0x1001FF80: Bootloader code/data (128KB)
0x1001FF80 - 0x10020000: Device identification section (128 bytes)
0x10020000 - 0x10200000: Application flash region (1.87MB)
```

## Building

### Prerequisites

1. ARM GCC Toolchain
2. Pico SDK (set `PICO_SDK_PATH` environment variable)
3. Frankly Bootloader library in sibling directory

### Build Commands

```bash
cd boards/rp2040_pico/franklyboot_pico
mkdir -p build && cd build
cmake ..
make -j4
```

### Output Files

- `franklyboot_pico.uf2` - Drag-and-drop bootloader image for Pico
- `franklyboot_pico.elf` - ELF executable for debugging
- `franklyboot_pico.bin` - Raw binary image
- `franklyboot_pico.hex` - Intel HEX format

## Flashing

### Method 1: UF2 (Recommended for initial flash)

1. Hold BOOTSEL button on Pico
2. Connect USB cable
3. Drag `franklyboot_pico.uf2` to RPI-RP2 drive
4. Bootloader will start automatically

### Method 2: SWD (For debugging)

Use OpenOCD or picoprobe:
```bash
openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg \
  -c "adapter speed 5000" \
  -c "program franklyboot_pico.elf verify reset exit"
```

## Usage

1. **Auto-boot Mode**: If valid application is present, boots after 2s
2. **Bootloader Mode**: Connect via USB CDC to receive firmware updates
3. **LED Indication**:
   - Fast blink (~250ms period): Waiting for auto-boot
   - Fast flash (100ms period, 50ms on/off): Active communication
   - Slow blink (2s period, 1s on/off): Idle, waiting for commands
   - Off: Application running

## Communication Protocol

The bootloader uses the Frankly Bootloader protocol over USB CDC:

- **Baud Rate**: N/A (USB full-speed)
- **Message Size**: 8 bytes
- **Format**: See Frankly Bootloader documentation

## Memory Usage

```
Code (text):   ~104 KB
Data:          ~16 bytes
BSS:           ~12 KB
Total Flash:   ~104 KB (out of 128 KB allocated)
Total RAM:     ~12 KB (out of 256 KB available)
```

### LED Status Behavior

The onboard LED provides visual feedback about bootloader state:

| LED Pattern | State | Description |
|-------------|-------|-------------|
| Fast blink (250ms) | Auto-boot wait | Bootloader checking for valid app, boots in 2s |
| Fast flash (100ms, 50/50) | Active communication | Receiving/sending data via USB CDC |
| Slow blink (2s, 1s on/off) | Idle | Bootloader ready, waiting for commands |
| Off | Application running | Control transferred to application |

**Communication Detection**: The bootloader considers itself "communicating" when it has received data within the last 500ms. This ensures the LED accurately reflects real-time activity.

## Technical Details

### Dual-Core Communication

- Uses circular FIFO buffers (256 bytes each) for RX/TX
- Lock-free communication between cores
- Core1 handles all USB interrupts and transfers
- Core0 processes bootloader commands

### Flash Operations

- Erase granularity: 4KB sectors
- Write granularity: 256 bytes (FLASH_PAGE_SIZE)
- Interrupts disabled during flash operations
- Uses Pico SDK flash API

### CRC Calculation

Software CRC-32 implementation (RP2040 has no hardware CRC):
- Polynomial: 0xEDB88320
- Initial value: 0xFFFFFFFF
- Final XOR: 0xFFFFFFFF

## Troubleshooting

### Build Errors

**Error: `FLASH_PAGE_SIZE` conflicts**
- This is expected - SDK defines it as 256, we use FLASH_SECTOR_SIZE (4096)

**Error: Region FLASH overflowed**
- Increase FLASH region size in `rp2040.ld`
- Update FLASH_APP_FIRST_PAGE in `device_defines.h`

### Runtime Issues

**USB not enumerated**
- Check TinyUSB configuration in `tusb_config.h`
- Verify Core1 is launched properly
- Check USB cable (must support data, not just power)

**Application not starting**
- Verify application vector table is at 0x10020000
- Check application was flashed with correct offset
- Use serial debug to see bootloader status

## File Structure

```
Core/
├── Inc/
│   ├── bootloader_api.h       - Bootloader API declarations
│   ├── device_defines.h        - RP2040-specific constants
│   └── tusb_config.h           - TinyUSB configuration
└── Src/
    ├── main.c                  - Main entry point, core init
    ├── bootloader_api.cpp      - Bootloader implementation
    └── usb_descriptors.c       - USB device descriptors

CMakeLists.txt                  - Build configuration
rp2040.ld                       - Linker script
README.md                       - This file
MIGRATION_NOTES.md              - Migration details from STM32
```

## License

BSD-3-Clause - FRANCOR e.V.

## Author

Martin Bauernschmitt (martin.bauernschmitt@francor.de)
