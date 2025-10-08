# Example Application for RP2040 Pico with Frankly Bootloader

This is a simple example application that demonstrates how to create firmware compatible with the Frankly Bootloader for RP2040.

## Features

- **LED Blinking**: Onboard LED blinks at 1Hz to show the app is running
- **Bootloader Re-entry**: Mechanism to return to bootloader via watchdog scratch register
- **CRC Placeholder**: Section for bootloader validation
- **USB CDC Support**: Stdio via USB for debugging

## Memory Layout

The application is configured to run from flash address `0x10020000` (after the 128KB bootloader):

```
0x10000000 - 0x10020000: Bootloader (128KB)
0x10020000 - 0x101FFFFC: Application code (1.87MB)
0x101FFFFC - 0x10200000: Application CRC (4 bytes)
```

## Building

### Prerequisites

1. ARM GCC Toolchain
2. Pico SDK (set `PICO_SDK_PATH` environment variable)

### Build Commands

```bash
cd boards/rp2040_pico/example_app_pico
mkdir -p build && cd build
cmake ..
make -j4
```

### Output Files

- `example_app_pico.uf2` - For flashing via bootloader or drag-and-drop
- `example_app_pico.bin` - Raw binary
- `example_app_pico.elf` - For debugging
- `example_app_pico.hex` - Intel HEX format

## Flashing

### Method 1: Via Frankly Bootloader (Recommended)

1. Flash the bootloader first (if not already done)
2. Connect USB to Pico
3. Use Frankly Flashbot tool to upload `example_app_pico.bin`:
   ```bash
   frankly-flashbot flash example_app_pico.bin
   ```

### Method 2: Direct Flash (Development)

For development, you can flash directly via UF2:

1. Hold BOOTSEL button and connect USB
2. Drag `example_app_pico.uf2` to RPI-RP2 drive
3. **Note**: This will overwrite the bootloader!

## Application Behavior

### Normal Operation

1. **Startup**: LED blinks quickly 3 times
2. **Running**: LED blinks steadily at 1Hz (on for 1s, off for 1s)

### Re-entering Bootloader

To return to the bootloader from your application:

```c
// Set autoboot disable flag
watchdog_hw->scratch[0] = 0xDEADBEEF;

// Reset device
watchdog_reboot(0, 0, 0);
```

The bootloader will detect the flag and stay in bootloader mode instead of auto-starting.

## Code Structure

```
Core/
├── Inc/               (Empty - headers would go here)
└── Src/
    └── main.c         Main application code

CMakeLists.txt         Build configuration
rp2040_app.ld          Linker script for 0x10020000 start address
README.md              This file
```

## Memory Usage

```
Flash: ~27KB (out of 1.87MB available)
RAM:   ~4KB  (out of 256KB available)
```

Plenty of room for your application!

## Customization

### Changing LED Blink Rate

Edit `main.c`:

```c
// Change from 1Hz to 2Hz
gpio_put(LED_PIN, 1);
sleep_ms(500);  // Changed from 1000
gpio_put(LED_PIN, 0);
sleep_ms(500);  // Changed from 1000
```

### Adding Button Support

The RP2040 Pico has a special BOOTSEL button, but you can add external buttons:

```c
#define BUTTON_PIN 14  // GPIO 14 for example

// In initHardware():
gpio_init(BUTTON_PIN);
gpio_set_dir(BUTTON_PIN, GPIO_IN);
gpio_pull_up(BUTTON_PIN);

// In main loop:
if (!gpio_get(BUTTON_PIN)) {
    // Button pressed (active low)
    reenterBootloader();
}
```

### Adding USB CDC Commands

The example enables USB CDC. You can add command processing:

```c
#include <stdio.h>

// In main loop:
int c = getchar_timeout_us(0);  // Non-blocking
if (c != PICO_ERROR_TIMEOUT) {
    if (c == 'b' || c == 'B') {
        printf("Rebooting to bootloader...\n");
        sleep_ms(100);
        reenterBootloader();
    }
}
```

## CRC Placeholder

The application includes a CRC placeholder at the end of flash:

```c
const uint32_t __APP_CRC__ __attribute__((section("._app_crc"))) = 0xFFFFFFFF;
```

**Important**: The actual CRC should be calculated and written during the flashing process by the Frankly Flashbot tool. The bootloader uses this to validate the application.

## Linker Script Details

The custom linker script (`rp2040_app.ld`) ensures:

1. Application starts at `0x10020000`
2. Vector table is properly aligned
3. CRC is placed at the end of application space
4. Boot2 stage is included (required by RP2040)
5. All necessary SDK sections are present

## Troubleshooting

### Application Won't Start

**Symptom**: Bootloader fast-flashes indefinitely

**Solution**: 
- Check that CRC was calculated correctly
- Verify application was flashed to correct address (0x10020000)
- Check application size doesn't exceed 1.87MB

### LED Doesn't Blink

**Symptom**: No LED activity after bootloader period

**Possible causes**:
1. Application crashed - check vector table
2. Wrong start address - verify linker script
3. Stack overflow - increase stack size if needed

### Can't Re-enter Bootloader

**Symptom**: Reset doesn't return to bootloader

**Solution**:
- Ensure watchdog scratch register is being set
- Verify `AUTOBOOT_DISABLE_KEY` matches bootloader value (0xDEADBEEF)
- Check watchdog is functioning

## Example Extensions

### Multi-Function App

```c
while (1) {
    // Blink LED
    gpio_put(LED_PIN, !gpio_get(LED_PIN));
    
    // Process USB commands
    handleUSBCommands();
    
    // Your application logic here
    doApplicationWork();
    
    sleep_ms(500);
}
```

### Persistent Settings

Use the RP2040's flash to store settings (be careful not to overwrite code!):

```c
#define SETTINGS_FLASH_OFFSET (0x1FF000)  // Near end of flash
// Read/write settings using flash_range_program
```

## Integration with Frankly Bootloader

This example is designed to work seamlessly with the Frankly Bootloader:

1. **Auto-start**: Bootloader validates CRC and starts app after 2s
2. **Re-entry**: App can request bootloader mode via watchdog scratch
3. **Updates**: New firmware can be flashed via bootloader USB CDC
4. **Validation**: CRC ensures firmware integrity

## License

BSD-3-Clause - FRANCOR e.V.

## Author

Martin Bauernschmitt (martin.bauernschmitt@francor.de)
