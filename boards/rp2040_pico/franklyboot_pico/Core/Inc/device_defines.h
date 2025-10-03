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

// RP2040 clock frequency (default 125 MHz)
constexpr uint32_t SYS_TICK = {125000000U};

// RP2040 Flash memory layout
// Flash starts at 0x10000000 (XIP memory region)
constexpr uint32_t FLASH_START_ADDR = {0x10000000U};

// First application page (bootloader occupies first 128KB = 32 pages of 4KB each)
constexpr uint32_t FLASH_APP_FIRST_PAGE = {32U};

// RP2040 has 2MB flash (on Pico board)
constexpr uint32_t FLASH_SIZE = {2 * 1024 * 1024U};

// RP2040 flash sector size is 4KB (for bootloader page management)
// Note: SDK FLASH_PAGE_SIZE is 256 bytes (programming page size)
constexpr uint32_t FLASH_SECTOR_SIZE = {4096U};

// For compatibility with bootloader Handler template
constexpr uint32_t FLASH_PAGE_SIZE_BOOT = FLASH_SECTOR_SIZE;

// Application start address
constexpr uint32_t FLASH_APP_START_ADDR = FLASH_START_ADDR + FLASH_APP_FIRST_PAGE * FLASH_SECTOR_SIZE;

};  // namespace device

#endif /* __cplusplus */

#endif /* DEVICE_DEFINES_H_ */