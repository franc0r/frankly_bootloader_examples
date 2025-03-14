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

// Device identification ----------------------------------------------------------------------------------------------

/**
 * Device identification index
 */
enum DeviceIdentIdx {
  DEV_IDENT_VENDOR_ID = 0U,
  DEV_IDENT_PRODUCT_ID = 1U,
  DEV_IDENT_PRODUCTION_DATE = 2U,
};

/*
 * This array contains the device identification information, which is stored in a extra section in the flash
 * memory. The default value is 0xFFFFFFFFU for uninitialized flash.
 * The data is written during the flash process.
 */
#pragma pack(push, 1)
volatile uint32_t __DEVICE_IDENT__[4U]
    __attribute__((section("._dev_ident"))) = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};
#pragma pack(pop)

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
 * @brief Block until message is received from serial line
 */
static void waitForMessage(msg::Msg& request) {
  std::array<std::uint8_t, MSG_SIZE> buffer;
  uint32_t buffer_idx = 0U;
  uint32_t timeout_cnt = 0U;

  for (;;) {
    // Check for autostart override
    if (req_autostart) {
      hwi::startApp(device::FLASH_APP_START_ADDR);
    } else {
      // Otherwise wait for data
      const uint8_t rx_new_byte = ((LPUART1->ISR & USART_ISR_RXNE) == USART_ISR_RXNE);
      if (rx_new_byte) {
        buffer[buffer_idx] = LPUART1->RDR;
        buffer_idx++;

        if (buffer_idx >= buffer.size()) {
          break;
        }

        timeout_cnt = 0U;
      } else {
        // Check if new message has started
        // If new byte is not received within a timeout cnt
        // the message is ignored
        if (buffer_idx != 0) {
          timeout_cnt++;

          if (timeout_cnt >= MSG_TIMEOUT_CNT) {
            buffer_idx = 0U;
          }
        }
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
 * @brief Transmit response over serial line
 */
static void transmitResponse(const msg::Msg& response) {
  std::array<std::uint8_t, MSG_SIZE> buffer;

  /* Encode message */
  buffer[0U] = static_cast<uint8_t>(response.request);
  buffer[1U] = static_cast<uint8_t>(response.request >> 8U);
  buffer[2U] = static_cast<uint8_t>(response.result);
  buffer[3U] = response.packet_id;

  buffer[4U] = response.data.at(0);
  buffer[5U] = response.data.at(1);
  buffer[6U] = response.data.at(2);
  buffer[7U] = response.data.at(3);

  /* Transmit message */
  for (uint32_t buffer_idx = 0U; buffer_idx < buffer.size(); buffer_idx++) {
    uint8_t tx_not_ready = ((LPUART1->ISR & USART_ISR_TXE) == USART_ISR_TXE);

    LPUART1->TDR = buffer.at(buffer_idx);

    do {
      tx_not_ready = !((LPUART1->ISR & USART_ISR_TXE) == USART_ISR_TXE);
    } while (tx_not_ready);
  }
}

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {}

extern "C" void FRANKLYBOOT_Run(void) {
  Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::FLASH_PAGE_SIZE>
      hBootloader;

  // Check if autostart shall be disabled by app firmware via backup register
  const bool autostart_disable = (TAMP->BKP0R == AUTOBOOT_DISABLE_OVERRIDE_KEY);
  TAMP->BKP0R = 0;  // Reset backup register

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

void hwi::resetDevice() { 
  /* Delay system reset */
  for(uint32_t idx = 0U; idx < 1000000U; idx++) {
    __NOP();
  }
  
  NVIC_SystemReset(); 
}

[[nodiscard]] uint32_t hwi::getVendorID() { return __DEVICE_IDENT__[DEV_IDENT_VENDOR_ID]; }

[[nodiscard]] uint32_t hwi::getProductID() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCT_ID]; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCTION_DATE]; }

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
