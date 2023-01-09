/**
 * @file bootloader_api.h
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Frankly Bootloader API Firmware Header for STM32G431RB
 * @version 1.0
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022 - BSD-3-clause - FRANCOR e.V.
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
 * @brief Initiazes the bootloader API
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