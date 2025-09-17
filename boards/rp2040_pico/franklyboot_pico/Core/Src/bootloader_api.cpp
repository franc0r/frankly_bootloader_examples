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
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/unique_id.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

// Use Pico SDK equivalents for CMSIS intrinsics
#include "hardware/sync.h"

// Simple implementations for missing CMSIS functions
extern "C" {
  void __NOP(void) { asm volatile("nop"); }
  void __disable_irq(void) {
    asm volatile("cpsid i" ::: "memory");
  }
  void __enable_irq(void) {
    asm volatile("cpsie i" ::: "memory");
  }
  void __set_MSP(uint32_t topOfMainStack) {
    asm volatile("msr msp, %0" : : "r" (topOfMainStack) : "sp");
  }
}

using namespace franklyboot;

// RP2040 flash constants
#define FLASH_BASE            0x10000000
#define USB_BUFFER_SIZE       64

// Device identification ----------------------------------------------------------------------------------------------

enum DeviceIdentIdx {
  DEV_IDENT_VENDOR_ID = 0U,
  DEV_IDENT_PRODUCT_ID = 1U,
  DEV_IDENT_PRODUCTION_DATE = 2U,
};

/**
 * @brief Device identification data for bootflash tool.
 *
 * This array contains the device identification data and is placed in the
 * ._dev_ident section in flash memory. The default values are 0xFFFFFFFFU for
 * uninitialized flash. The data is written during the flash process.
 */
#pragma pack(push, 1)
volatile uint32_t __DEVICE_IDENT__[4U]
    __attribute__((section("._dev_ident"))) = {0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};
#pragma pack(pop)

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

// USB CDC communication using Pico SDK stdio
static int buffered_char = -1;

static bool usb_cdc_available(void) {
  // Check if we have a buffered character
  if (buffered_char != -1) {
    return true;
  }

  // Check if USB CDC has data available using Pico SDK
  int ch = getchar_timeout_us(0); // Non-blocking read
  if (ch != PICO_ERROR_TIMEOUT) {
    buffered_char = ch;
    return true;
  }
  return false;
}

static uint8_t usb_cdc_read(void) {
  // Check buffered character first
  if (buffered_char != -1) {
    uint8_t ch = (uint8_t)buffered_char;
    buffered_char = -1;
    return ch;
  }

  // Read byte from USB CDC using Pico SDK
  int ch = getchar_timeout_us(1000); // 1ms timeout
  if (ch != PICO_ERROR_TIMEOUT) {
    return (uint8_t)ch;
  }
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

// USB CDC transmit using Pico SDK stdio
static void usb_cdc_write(uint8_t data) {
  // Write byte to USB CDC using Pico SDK
  putchar_raw(data);
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

// CRC-32/ISO-HDLC implementation - matches frankly-fw-update-cli CRC_32_ISO_HDLC
// Polynomial: 0x04C11DB7, Init: 0xFFFFFFFF, RefIn: true, RefOut: true, XorOut: 0xFFFFFFFF
// This is the bit-reversed (LSB-first) lookup table for polynomial 0xEDB88320
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t software_crc32(const uint8_t* data, uint32_t length) {
  // CRC-32/ISO-HDLC implementation
  // Init: 0xFFFFFFFF, RefIn: true, RefOut: true, XorOut: 0xFFFFFFFF
  uint32_t crc = 0xFFFFFFFF;

  for (uint32_t i = 0; i < length; i++) {
    crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
  }

  // XorOut: 0xFFFFFFFF (invert final result)
  return ~crc;
}

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {}

extern "C" void FRANKLYBOOT_Run(void) {
  Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::BOOTLOADER_FLASH_PAGE_SIZE>
      hBootloader;

  gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn on LED to show bootloader is running

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
  // Use Pico SDK watchdog reset
  watchdog_reboot(0, 0, 0);
  while (true) {
    __NOP();
  }
}

[[nodiscard]] uint32_t hwi::getVendorID() { return __DEVICE_IDENT__[DEV_IDENT_VENDOR_ID]; }

[[nodiscard]] uint32_t hwi::getProductID() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCT_ID]; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCTION_DATE]; }

[[nodiscard]] uint32_t hwi::getUniqueIDWord(const uint32_t idx) {
  // Read from RP2040 unique ID using Pico SDK
  pico_unique_board_id_t unique_id;
  pico_get_unique_board_id(&unique_id);

  if (idx < 2) {
    // Return first 8 bytes as two 32-bit words
    return ((uint32_t*)unique_id.id)[idx];
  }
  return 0;
}

uint32_t hwi::calculateCRC(uint32_t src_address, uint32_t num_bytes) {
  // CRC-32/ISO-HDLC implementation to match frankly-fw-update-cli
  // Parameters: poly=0x04C11DB7, init=0xFFFFFFFF, refin=true, refout=true, xorout=0xFFFFFFFF

  const uint8_t* data_ptr = (const uint8_t*)src_address;
  return software_crc32(data_ptr, num_bytes);
}

bool hwi::eraseFlashPage(uint32_t page_id) {
  // Use Pico SDK flash erase (4KB sectors)
  uint32_t offset = page_id * device::BOOTLOADER_FLASH_PAGE_SIZE;

  // Disable interrupts for flash operation
  uint32_t ints = save_and_disable_interrupts();

  // Erase 4KB sector
  flash_range_erase(offset, FLASH_SECTOR_SIZE);

  // Restore interrupts
  restore_interrupts(ints);

  return true;
}

bool hwi::writeDataBufferToFlash(uint32_t dst_address, uint32_t dst_page_id, uint8_t* src_data_ptr,
                                 uint32_t num_bytes) {
  // Use Pico SDK flash programming
  uint32_t offset = dst_address - FLASH_BASE;

  // RP2040 flash programming requirements:
  // - Program in 256-byte pages (FLASH_PAGE_SIZE)
  // - Address and size must be 256-byte aligned
  if (offset % FLASH_PAGE_SIZE != 0 || num_bytes % FLASH_PAGE_SIZE != 0) {
    return false;
  }

  // Disable interrupts for flash operation
  uint32_t ints = save_and_disable_interrupts();

  // Program flash (256-byte pages)
  flash_range_program(offset, src_data_ptr, num_bytes);

  // Restore interrupts
  restore_interrupts(ints);

  return true;
}

[[nodiscard]] uint8_t hwi::readByteFromFlash(uint32_t flash_src_address) {
  uint8_t* flash_src_ptr = (uint8_t*)(flash_src_address);
  return *(flash_src_ptr);
}

void hwi::startApp(uint32_t app_flash_address) {
  __disable_irq();

  // Get application address from vector table
  void (*App)(void) = (void (*)(void))(*((uint32_t*)(app_flash_address + 4u)));

  // Set Main Stack Pointer
  __set_MSP(*(uint32_t*)app_flash_address);

  // Load Vector Table Offset Register (VTOR) - crucial for interrupt handling
  // The RP2040 (Cortex-M0+) does support VTOR
  *((volatile uint32_t*)0xE000ED08) = app_flash_address;  // SCB->VTOR

  __enable_irq();

  // Jump to application
  App();
}