/**
 * @file bootloader_api.cpp
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Source file of bootloader API for RP2040 Pico (minimal version)
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"

#include <francor/franklyboot/handler.h>

#include "device_defines.h"

// ARM CMSIS intrinsics
extern "C" {
  void __disable_irq(void);
  void __enable_irq(void);
  void __NOP(void);
  void __set_MSP(uint32_t topOfMainStack);
}

using namespace franklyboot;

// RP2040 minimal register definitions -------------------------------------------------------------------------------
#define USBCTRL_BASE          0x50110000
#define WATCHDOG_BASE         0x40058000
#define FLASH_BASE            0x10000000

// USB CDC endpoints and buffers
#define USB_ENDPOINT_IN       0x81  // EP1 IN
#define USB_ENDPOINT_OUT      0x01  // EP1 OUT
#define USB_BUFFER_SIZE       64

#define REG(base, offset) (*(volatile uint32_t*)((base) + (offset)))

// Device identification ----------------------------------------------------------------------------------------------

enum DeviceIdentIdx {
  DEV_IDENT_VENDOR_ID = 0U,
  DEV_IDENT_PRODUCT_ID = 1U,
  DEV_IDENT_PRODUCTION_DATE = 2U,
};

volatile uint32_t __DEVICE_IDENT__[4U] = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};

// Defines ------------------------------------------------------------------------------------------------------------
constexpr uint32_t AUTOBOOT_DISABLE_OVERRIDE_KEY = {0xDEADBEEFU};
constexpr uint32_t MSG_TIMEOUT_CNT = {12000000U / 2000U}; // 12MHz / 2000 = timeout
constexpr uint32_t MSG_SIZE = {8U};

// Private Variables --------------------------------------------------------------------------------------------------
static volatile bool autostart_possible = {false};
static volatile bool req_autostart = {false};
static volatile uint32_t autoboot_disable_flag = 0;

// USB CDC state
static volatile bool usb_cdc_connected = false;
static uint8_t usb_rx_buffer[USB_BUFFER_SIZE];
static uint8_t usb_tx_buffer[USB_BUFFER_SIZE];
static volatile uint32_t usb_rx_count = 0;
static volatile uint32_t usb_rx_pos = 0;

// Private Function Prototypes ----------------------------------------------------------------------------------------

static void checkAutoStartAbort(msg::Msg& request) {
  if (autostart_possible) {
    if (request.request == msg::REQ_PING || request.request == msg::REQ_DEV_INFO_BOOTLOADER_VERSION) {
      autostart_possible = false;
    }
  }
}

// Simplified USB CDC receive function (placeholder implementation)
static bool usb_cdc_available(void) {
  // TODO: Check if USB CDC has data available
  // For now, return false as placeholder
  return false;
}

static uint8_t usb_cdc_read(void) {
  // TODO: Read byte from USB CDC
  // For now, return dummy data
  return 0;
}

static void waitForMessage(msg::Msg& request) {
  std::array<std::uint8_t, MSG_SIZE> buffer;
  uint32_t buffer_idx = 0U;
  uint32_t timeout_cnt = 0U;

  for (;;) {
    if (req_autostart) {
      hwi::startApp(device::FLASH_APP_START_ADDR);
    } else {
      // Check if USB CDC has data available
      if (usb_cdc_available()) {
        buffer[buffer_idx] = usb_cdc_read();
        buffer_idx++;

        if (buffer_idx >= buffer.size()) {
          break;
        }
        timeout_cnt = 0U;
      } else {
        if (buffer_idx != 0) {
          timeout_cnt++;
          if (timeout_cnt >= MSG_TIMEOUT_CNT) {
            buffer_idx = 0U;
          }
        }
      }
    }
  }

  // Decode message
  const uint16_t rx_request_raw = static_cast<uint16_t>(buffer[0U]) | static_cast<uint16_t>(buffer[1U] << 8U);
  request.request = static_cast<msg::RequestType>(rx_request_raw);
  request.result = static_cast<msg::ResultType>(buffer[2U]);
  request.packet_id = static_cast<uint8_t>(buffer[3U]);
  request.data[0U] = static_cast<uint8_t>(buffer[4U]);
  request.data[1U] = static_cast<uint8_t>(buffer[5U]);
  request.data[2U] = static_cast<uint8_t>(buffer[6U]);
  request.data[3U] = static_cast<uint8_t>(buffer[7U]);
}

// Simplified USB CDC transmit function (placeholder implementation)
static void usb_cdc_write(uint8_t data) {
  // TODO: Write byte to USB CDC
  // For now, do nothing (placeholder)
  (void)data;
}

static void transmitResponse(const msg::Msg& response) {
  std::array<std::uint8_t, MSG_SIZE> buffer;

  // Encode message
  buffer[0U] = static_cast<uint8_t>(response.request);
  buffer[1U] = static_cast<uint8_t>(response.request >> 8U);
  buffer[2U] = static_cast<uint8_t>(response.result);
  buffer[3U] = response.packet_id;
  buffer[4U] = response.data.at(0);
  buffer[5U] = response.data.at(1);
  buffer[6U] = response.data.at(2);
  buffer[7U] = response.data.at(3);

  // Transmit message via USB CDC
  for (uint32_t buffer_idx = 0U; buffer_idx < buffer.size(); buffer_idx++) {
    usb_cdc_write(buffer.at(buffer_idx));
  }
}

// Software CRC32 implementation -------------------------------------------------------------------------------------
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    // ... (truncated for brevity - would include full 256-entry table)
};

static uint32_t software_crc32(const uint8_t* data, uint32_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (uint32_t i = 0; i < length; i++) {
    crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  }
  return ~crc;
}

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {}

extern "C" void FRANKLYBOOT_Run(void) {
  Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::FLASH_PAGE_SIZE>
      hBootloader;

  const bool autostart_disable = (autoboot_disable_flag == AUTOBOOT_DISABLE_OVERRIDE_KEY);
  autoboot_disable_flag = 0;

  autostart_possible = hBootloader.isAppValid() && !autostart_disable;

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

extern "C" uint32_t FRANKLYBOOT_getDevSysTickHz(void) { return 12000000U; } // 12MHz default

extern "C" void FRANKLYBOOT_autoStartISR(void) {
  if (autostart_possible) {
    req_autostart = true;
  }
}

// Hardware Interface -------------------------------------------------------------------------------------------------

void hwi::resetDevice() {
  // Simple infinite loop reset
  while (true) {
    __NOP();
  }
}

[[nodiscard]] uint32_t hwi::getVendorID() { return __DEVICE_IDENT__[DEV_IDENT_VENDOR_ID]; }

[[nodiscard]] uint32_t hwi::getProductID() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCT_ID]; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCTION_DATE]; }

[[nodiscard]] uint32_t hwi::getUniqueIDWord(const uint32_t idx) {
  // TODO: Read from RP2040 unique ID registers
  return 0x12345678U + idx; // Placeholder
}

uint32_t hwi::calculateCRC(uint32_t src_address, uint32_t num_bytes) {
  const uint8_t* data_ptr = (const uint8_t*)src_address;
  return software_crc32(data_ptr, num_bytes);
}

bool hwi::eraseFlashPage(uint32_t page_id) {
  // TODO: Implement flash erase
  (void)page_id;
  return false; // Not implemented yet
}

bool hwi::writeDataBufferToFlash(uint32_t dst_address, uint32_t dst_page_id, uint8_t* src_data_ptr,
                                 uint32_t num_bytes) {
  // TODO: Implement flash write
  (void)dst_address;
  (void)dst_page_id;
  (void)src_data_ptr;
  (void)num_bytes;
  return false; // Not implemented yet
}

[[nodiscard]] uint8_t franklyboot::hwi::readByteFromFlash(uint32_t flash_src_address) {
  uint8_t* flash_src_ptr = (uint8_t*)(flash_src_address);
  return *(flash_src_ptr);
}

void franklyboot::hwi::startApp(uint32_t app_flash_address) {
  __disable_irq();

  // Get application address from vector table
  void (*App)(void) = (void (*)(void))(*((uint32_t*)(app_flash_address + 4u)));

  // Set Main Stack Pointer
  __set_MSP(*(uint32_t*)app_flash_address);

  __enable_irq();

  // Jump to application
  App();
}