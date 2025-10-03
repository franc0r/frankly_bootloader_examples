/**
 * @file main.c
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Example Application for RP2040 Pico with Frankly Bootloader
 * @version 1.0
 * @date 2025-10-01
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 *
 * This example application demonstrates:
 * - LED blinking to show the app is running
 * - Button press detection to re-enter bootloader
 * - CRC placeholder for bootloader validation
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "hardware/gpio.h"

// Pico onboard LED and BOOTSEL button
#define LED_PIN               PICO_DEFAULT_LED_PIN
#define BOOTSEL_PIN           0  // Note: BOOTSEL is special, we'll detect long press via timing

// Autostart disable key (same as bootloader)
#define AUTOBOOT_DISABLE_KEY  0xDEADBEEF

// CRC placeholder - will be calculated and written by flashing tool
const uint32_t __APP_CRC__ __attribute__((section("._app_crc"))) = 0xFFFFFFFF;

// Private Functions --------------------------------------------------------------------------------------------------

/**
 * @brief Initialize hardware
 */
static void initHardware(void) {
    // Initialize LED GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Disable timer debug pause
    timer_hw->dbgpause = 0;
}

// Public Functions ---------------------------------------------------------------------------------------------------

int main(void) {
    // Initialize hardware
    initHardware();
    
    // Small startup delay
    sleep_ms(100);
    
    // Indicate app has started with a quick blink
    for (uint32_t i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    // Main application loop
    while (1) {        
        // Normal operation: blink LED at 1Hz
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }
    
    return 0;
}
