/**
 * @file device_defines.h
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Frankly Bootloader API Firmware Header Device Definitions
 * @version 1.0
 * @date 2023-01-09
 *
 * @copyright Copyright (c) 2023 - BSD-3-clause - FRANCOR e.V.
 */

#ifndef DEVICE_DEFINES_H_
#define DEVICE_DEFINES_H_

// Includes -----------------------------------------------------------------------------------------------------------
#include <stdint.h>

// Public Functions ---------------------------------------------------------------------------------------------------

#ifdef __cplusplus

namespace device {

constexpr uint32_t SYS_TICK = {16000000U};

constexpr uint32_t VENDOR_ID = {0x45445541U};
constexpr uint32_t PRODUCT_ID = {0x00000001U};
constexpr uint32_t PRODUCTION_DATE = {0x00000000U};

constexpr uint32_t FLASH_START_ADDR = {0x08000000U};
constexpr uint32_t FLASH_APP_FIRST_PAGE = {4U};
constexpr uint32_t FLASH_SIZE = {128 * 1024U};
constexpr uint32_t FLASH_PAGE_SIZE = {2048U};
constexpr uint32_t FLASH_APP_START_ADDR = FLASH_START_ADDR + FLASH_APP_FIRST_PAGE * FLASH_PAGE_SIZE;
};  // namespace device

#endif /* __cplusplus */

// Defines ------------------------------------------------------------------------------------------------------------

#define CAN_BROADCAST_ID (uint16_t)(0x780U)
#define CAN_NODE_ID (uint16_t)(1)

#endif /* DEVICE_DEFINES_H_ */