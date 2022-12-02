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

#ifdef __cplusplus
};
#endif

#endif /* BOOTLOADER_API_H_ */