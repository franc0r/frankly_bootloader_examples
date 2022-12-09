/**
 * @file bootloader_api.cpp
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Source file of bootloader API
 * @version 1.0
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"

#include <francor/franklyboot/handler.h>

#include "device_defines.h"
#include "stm32g4xx.h"

using namespace franklyboot;

// Private Variables --------------------------------------------------------------------------------------------------

// Private Function Prototypes ----------------------------------------------------------------------------------------

static void waitForMessage(msg::Msg& request) { (void)request; }
static void transmitResponse(const msg::Msg& response) { (void)response; }

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {}

extern "C" void FRANKLYBOOT_Run(void) {
  Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::FLASH_PAGE_SIZE>
      hBootloader;

  for (;;) {
    msg::Msg request;
    hBootloader.processBufferedCmds();
    waitForMessage(request);
    hBootloader.processRequest(request);
    const auto response = hBootloader.getResponse();
    transmitResponse(response);
  }
}

// Hardware Interface -------------------------------------------------------------------------------------------------

void hwi::resetDevice() { NVIC_SystemReset(); }

[[nodiscard]] uint32_t hwi::getVendorID() { return device::VENDOR_ID; }

[[nodiscard]] uint32_t hwi::getProductID() { return device::PRODUCT_ID; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return device::PRODUCTION_DATE; }

[[nodiscard]] uint32_t hwi::getUniqueID() { return 0U; }

uint32_t hwi::calculateCRC(uint32_t src_address, uint32_t num_bytes) {
  // Reset CRC calculation
  SET_BIT(CRC->CR, CRC_CR_RESET);

  const uint32_t num_words = num_bytes >> 2u;

  // Pointer to data
  uint32_t* data_ptr = (uint32_t*)src_address;

  for (uint32_t idx = 0u; idx < num_words; idx++) {
    const uint32_t value = *(data_ptr);
    CRC->DR = __REV(value);
    data_ptr++;
  }

  return ~CRC->DR;
}

bool hwi::eraseFlashPage(uint32_t page_id) {
  (void)page_id;
  return false;
}

bool hwi::writeDataBufferToFlash(uint32_t dst_address, uint32_t dst_page_id, uint8_t* src_data_ptr,
                                 uint32_t num_bytes) {
  (void)dst_address;
  (void)dst_page_id;
  (void)src_data_ptr;
  (void)num_bytes;
  return false;
}

[[nodiscard]] uint8_t franklyboot::hwi::readByteFromFlash(uint32_t flash_src_address) {
  (void)flash_src_address;
  return 0U;
}

void franklyboot::hwi::startApp(uint32_t app_flash_address) { (void)app_flash_address; }
