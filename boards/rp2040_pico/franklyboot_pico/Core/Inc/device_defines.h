/**
 * @file device_defines.h
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Frankly Bootloader API Firmware Header Device Definitions for RP2040
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 */

#ifndef DEVICE_DEFINES_H_
#define DEVICE_DEFINES_H_

// Includes -----------------------------------------------------------------------------------------------------------
#include <stdint.h>

// Public Functions ---------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace device {

constexpr uint32_t SYS_TICK = {125000000U};  // 125 MHz default system clock

// RP2040 Flash Memory Layout (2MB external QSPI flash)
constexpr uint32_t FLASH_START_ADDR = {0x10000000U};      // XIP flash base address
constexpr uint32_t FLASH_APP_FIRST_PAGE = {16U};          // App starts after bootloader (64KB)
constexpr uint32_t FLASH_SIZE = {2 * 1024 * 1024U};       // 2MB total flash
constexpr uint32_t FLASH_PAGE_SIZE = {4096U};             // 4KB sectors
constexpr uint32_t FLASH_APP_START_ADDR = FLASH_START_ADDR + FLASH_APP_FIRST_PAGE * FLASH_PAGE_SIZE;

};  // namespace device

#endif /* __cplusplus */

#endif /* DEVICE_DEFINES_H_ */