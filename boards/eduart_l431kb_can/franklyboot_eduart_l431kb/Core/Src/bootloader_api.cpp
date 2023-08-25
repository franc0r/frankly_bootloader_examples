/**
 * @file bootloader_api.cpp
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Source file of bootloader API
 * @version 1.0
 * @date 2023-01-09
 *
 * @copyright Copyright (c) 2023 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"

#include <francor/franklyboot/handler.h>

#include "device_defines.h"
#include "stm32l4xx.h"

using namespace franklyboot;
// Defines ------------------------------------------------------------------------------------------------------------

constexpr uint32_t AUTOBOOT_DISABLE_OVERRIDE_KEY = {0xDEADBEEFU};
constexpr uint32_t MSG_TIMEOUT_CNT = {device::SYS_TICK / 2000U};
constexpr uint32_t MSG_SIZE = {8U};

// Private Variables --------------------------------------------------------------------------------------------------
static volatile bool autostart_possible = {false};
static volatile bool req_autostart = {false};

// Private Function Prototypes ----------------------------------------------------------------------------------------

/**
 * @brief Checks if autostart shall be aborted by ping message request
 */
static void checkAutoStartAbort(msg::Msg& request) {
  if (autostart_possible) {
    /* Abort autostart if a ping for the bootloader was received */
    if (request.request == msg::REQ_PING || request.request == msg::REQ_DEV_INFO_BOOTLOADER_VERSION) {
      autostart_possible = false;
    }
  }
}

/**
 * @brief Block until message is received via CAN
 */
static void waitForMessage(msg::Msg& request) {
  std::array<std::uint8_t, MSG_SIZE> buffer;

  for (;;) {
    // Check for autostart override
    if (req_autostart) {
      hwi::startApp(device::FLASH_APP_START_ADDR);
    } else {
      // Otherwise wait for data
      const uint8_t rx_msg_pending = ((CAN1->RF0R & CAN_RF0R_FMP0_Msk) != 0);
      if (rx_msg_pending) {
        // Copy data to buffer
        buffer[0U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDLR);
        buffer[1U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDLR >> 8U);
        buffer[2U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDLR >> 16U);
        buffer[3U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDLR >> 24U);
        buffer[4U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDHR);
        buffer[5U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDHR >> 8U);
        buffer[6U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDHR >> 16U);
        buffer[7U] = static_cast<uint8_t>(CAN1->sFIFOMailBox[0].RDHR >> 24U);

        // Release data from RX FIFO
        CAN1->RF0R = CAN1->RF0R | CAN_RF0R_RFOM0;

        break;
      }
    }
  }

  /* Decode message */
  const uint16_t rx_request_raw = static_cast<uint16_t>(buffer[0U]) | static_cast<uint16_t>(buffer[1U] << 8U);
  request.request = static_cast<msg::RequestType>(rx_request_raw);
  request.result = static_cast<msg::ResultType>(buffer[2U]);
  request.packet_id = static_cast<uint8_t>(buffer[3U]);
  request.data[0U] = static_cast<uint8_t>(buffer[4U]);
  request.data[1U] = static_cast<uint8_t>(buffer[5U]);
  request.data[2U] = static_cast<uint8_t>(buffer[6U]);
  request.data[3U] = static_cast<uint8_t>(buffer[7U]);
}

/**
 * @brief Transmit response over CAN
 */
static void transmitResponse(const msg::Msg& response) {
  /* Encode message */
  uint32_t tx_data_l = static_cast<uint32_t>(response.request);
  tx_data_l |= (static_cast<uint32_t>(response.result) << 16U);
  tx_data_l |= (static_cast<uint32_t>(response.packet_id) << 24U);

  uint32_t tx_data_h = static_cast<uint32_t>(response.data.at(0));
  tx_data_h |= (static_cast<uint32_t>(response.data.at(1)) << 8U);
  tx_data_h |= (static_cast<uint32_t>(response.data.at(2)) << 16U);
  tx_data_h |= (static_cast<uint32_t>(response.data.at(3)) << 24U);

  CAN1->sTxMailBox[0].TDLR = tx_data_l;
  CAN1->sTxMailBox[0].TDHR = tx_data_h;

  /* Transmit message */
  CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;
}

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {}

extern "C" void FRANKLYBOOT_Run(void) {
  Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::FLASH_PAGE_SIZE>
      hBootloader;

  // Check if autostart shall be disabled by app firmware via backup register
  const bool autostart_disable = (RTC->BKP0R == AUTOBOOT_DISABLE_OVERRIDE_KEY);
  RTC->BKP0R = 0;  // Reset backup register

  // Autostart is possible if a valid app in flash is available
  autostart_possible = hBootloader.isAppValid() && !autostart_disable;

  // TODO init sys tick in main.c!

  for (;;) {
    msg::Msg request;
    hBootloader.processBufferedCmds();
    waitForMessage(request);
    checkAutoStartAbort(request);
    hBootloader.processRequest(request);
    const auto response = hBootloader.getResponse();
    transmitResponse(response);
  }
}

extern "C" uint32_t FRANKLYBOOT_getDevSysTickHz(void) { return device::SYS_TICK; }

extern "C" void FRANKLYBOOT_autoStartISR(void) {
  /* Start app if possible */
  if (autostart_possible) {
    req_autostart = true;
  }
}

// Hardware Interface -------------------------------------------------------------------------------------------------

void hwi::resetDevice() { NVIC_SystemReset(); }

[[nodiscard]] uint32_t hwi::getVendorID() { return device::VENDOR_ID; }

[[nodiscard]] uint32_t hwi::getProductID() { return device::PRODUCT_ID; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return device::PRODUCTION_DATE; }

[[nodiscard]] uint32_t hwi::getUniqueIDWord(const uint32_t idx) {
  uint32_t* device_uid_ptr = (uint32_t*)(UID_BASE);

  uint32_t uid_value;
  if (idx < 3U) {
    uid_value = device_uid_ptr[idx];
  } else {
    uid_value = 0U;
  }

  return uid_value;
}

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
  // Unlock flash
  FLASH->KEYR = 0x45670123U;
  FLASH->KEYR = 0xCDEF89ABU;

  uint32_t tmp_reg_value = FLASH->CR;
  tmp_reg_value |= FLASH_CR_PER;                   // Enable page erase mode
  tmp_reg_value &= ~(FLASH_CR_PNB_Msk);            // Clear old page idx
  tmp_reg_value |= (page_id << FLASH_CR_PNB_Pos);  // Setup page idx
  FLASH->CR = tmp_reg_value;

  // Start erase page
  FLASH->CR = FLASH->CR | FLASH_CR_STRT;

  // Wait for erase to finish
  bool in_progress = true;
  while (in_progress) {
    in_progress = ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY);
  }

  // Clear page erase mode
  FLASH->CR &= ~FLASH_CR_PER;

  // Lock flash
  FLASH->CR |= FLASH_CR_LOCK;

  return true;
}

bool hwi::writeDataBufferToFlash(uint32_t dst_address, uint32_t dst_page_id, uint8_t* src_data_ptr,
                                 uint32_t num_bytes) {
  // Check if data size is correct
  bool data_size_valid = ((num_bytes % 8) == 0);

  if (data_size_valid) {
    // Unlock flash
    FLASH->KEYR = 0x45670123U;
    FLASH->KEYR = 0xCDEF89ABU;

    // Enable programming
    FLASH->CR |= FLASH_CR_PG;

    // Write data
    uint32_t* dst_word_ptr = (uint32_t*)(dst_address);
    const uint32_t* dst_word_max_ptr = (uint32_t*)(dst_address + num_bytes);
    uint32_t* src_data_word_ptr = (uint32_t*)(src_data_ptr);

    while (dst_word_ptr < dst_word_max_ptr) {
      // Write word to flash
      *(dst_word_ptr) = *(src_data_word_ptr);

      // Wait until finished
      bool in_progress = true;
      while (in_progress) {
        in_progress = ((FLASH->SR & FLASH_SR_BSY) == FLASH_SR_BSY);
      }

      // Increase pointer
      dst_word_ptr++;
      src_data_word_ptr++;
    }

    // LOCK FLASH
    uint32_t tmp_reg_value = FLASH->CR;
    tmp_reg_value &= ~FLASH_CR_PG;
    tmp_reg_value |= FLASH_CR_LOCK;
    FLASH->CR = tmp_reg_value;

    return true;
  }

  return false;
}

[[nodiscard]] uint8_t franklyboot::hwi::readByteFromFlash(uint32_t flash_src_address) {
  uint8_t* flash_src_ptr = (uint8_t*)(flash_src_address);
  return *(flash_src_ptr);
}

void franklyboot::hwi::startApp(uint32_t app_flash_address) {  // Disable interrupts
  __disable_irq();

  // Clear pending interrupts
  NVIC->ICPR[0] = 0xFFFFFFFFu;

  // Get application address
  void (*App)(void) = (void (*)(void))(*((uint32_t*)(app_flash_address + 4u)));

  // Disable SysTick
  SysTick->CTRL = 0;
  SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

  // Set Main Stack Pointer
  __set_MSP(*(uint32_t*)app_flash_address);

  // Load Vector Table Offset
  SCB->VTOR = app_flash_address;

  // Boot into application
  __enable_irq();

  // Jump to app
  App();
}
