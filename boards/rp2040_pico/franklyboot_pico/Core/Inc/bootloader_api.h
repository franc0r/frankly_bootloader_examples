/**
 * @file bootloader_api.h
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Frankly Bootloader API Firmware Header for RP2040 Pico
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 */

#ifndef BOOTLOADER_API_H_
#define BOOTLOADER_API_H_

// Includes -----------------------------------------------------------------------------------------------------------
#include <stdint.h>

// Public Functions ---------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the bootloader API
 */
void FRANKLYBOOT_Init(void);

/**
 * @brief Runs the bootloader API (function is not exiting)
 */
void FRANKLYBOOT_Run(void);

/**
 * @brief Returns the system tick frequency in Hz
 */
uint32_t FRANKLYBOOT_getDevSysTickHz(void);

/**
 * @brief Called by sys tick timeout to autostart the app if possible
 */
void FRANKLYBOOT_autoStartISR(void);

#ifdef __cplusplus
};
#endif

#endif /* BOOTLOADER_API_H_ */