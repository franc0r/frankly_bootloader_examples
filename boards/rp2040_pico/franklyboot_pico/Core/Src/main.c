/**
 * @file main.c
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Main Source File of RP2040 Pico bootloader firmware
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "bootloader_api.h"

// Pico onboard LED pin
#define LED_PIN               PICO_DEFAULT_LED_PIN

// Autostart timeout in milliseconds
#define AUTOSTART_TIMEOUT_MS  2000U

// Private Variables --------------------------------------------------------------------------------------------------
static volatile uint32_t tick_counter = 0;
static volatile bool autostart_triggered = false;

// Private Functions --------------------------------------------------------------------------------------------------

/** \brief Init core hardware */
static void initCore(void);

/** \brief Timer callback for autostart */
static bool autostartTimerCallback(struct repeating_timer *t);

// Public Functions ---------------------------------------------------------------------------------------------------

int main(void) {
    // Initialize core hardware
    initCore();

    // Initialize bootloader API
    FRANKLYBOOT_Init();

    // Set up autostart timer (check every 100ms)
    struct repeating_timer timer;
    add_repeating_timer_ms(100, autostartTimerCallback, NULL, &timer);

    // Launch Core1 for USB CDC communication
    multicore_launch_core1(FRANKLYBOOT_Core1Entry);

    // Run bootloader (this function does not return)
    FRANKLYBOOT_Run();

    // Should never reach here
    return 0;
}

// Private Functions --------------------------------------------------------------------------------------------------

static void initCore(void) {
    // Initialize Pico SDK
    // Note: We don't call stdio_init_all() here because TinyUSB will be
    // initialized on Core1 by FRANKLYBOOT_Core1Entry()
    // The system clock is already set to 125 MHz by the SDK

    // Disable timer debug pause
    timer_hw->dbgpause = 0;

    // Small delay for system stabilization
    sleep_ms(100);

    // Initialize LED GPIO for status indication
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // Start with LED off
}

static bool autostartTimerCallback(struct repeating_timer *t) {
    tick_counter += 100;

    // Toggle LED while waiting for autostart timeout
    if (tick_counter < AUTOSTART_TIMEOUT_MS) {
        // Fast blink during autostart wait (250ms period)
        gpio_put(LED_PIN, (tick_counter / 125) % 2);
    } else if (!autostart_triggered) {
        autostart_triggered = true;
        FRANKLYBOOT_autoStartISR();
    } else {
        // After autostart check, use bootloader LED control
        // This handles fast flash during communication and slow blink when idle
        FRANKLYBOOT_updateLED();
    }

    return true; // Keep timer running
}
