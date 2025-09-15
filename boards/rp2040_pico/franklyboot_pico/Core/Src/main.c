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
#include "hardware/gpio.h"
#include "bootloader_api.h"

// Pico onboard LED pin
#define LED_PIN               PICO_DEFAULT_LED_PIN

// Private Functions --------------------------------------------------------------------------------------------------

/** \brief Init core hardware */
static void initCore(void);

// Public Functions ---------------------------------------------------------------------------------------------------

int main(void) {
    // Initialize core hardware
    initCore();

    // Initialize Frankly Bootloader
    FRANKLYBOOT_Init();

    // Run bootloader main loop
    FRANKLYBOOT_Run();

    return 0;
}

void SysTick_Handler(void) {
    FRANKLYBOOT_autoStartISR();
}

// Private Functions --------------------------------------------------------------------------------------------------

static void initCore(void) {
    // Initialize Pico SDK stdio for USB CDC communication
    stdio_init_all();

    // Initialize LED GPIO for status indication
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // Start with LED off
}