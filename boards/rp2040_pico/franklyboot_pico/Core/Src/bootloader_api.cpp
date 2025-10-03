/**
 * @file bootloader_api.cpp
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Source file of bootloader API for RP2040
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
#include "pico/multicore.h"
#include "pico/bootrom.h"
#include "pico/unique_id.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "hardware/structs/xip_ctrl.h"
#include "tusb.h"

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
constexpr uint32_t MSG_TIMEOUT_US = {500U};  // 500us timeout for message reception
constexpr uint32_t MSG_SIZE = {8U};

// Communication FIFO between Core0 and Core1
constexpr uint32_t RX_FIFO_SIZE = {256U};
constexpr uint32_t TX_FIFO_SIZE = {256U};

// Private Variables --------------------------------------------------------------------------------------------------
static volatile bool autostart_possible = {false};
static volatile bool req_autostart = {false};

// Circular buffers for inter-core communication
static volatile uint8_t rx_fifo[RX_FIFO_SIZE];
static volatile uint32_t rx_fifo_read_idx = 0;
static volatile uint32_t rx_fifo_write_idx = 0;

static volatile uint8_t tx_fifo[TX_FIFO_SIZE];
static volatile uint32_t tx_fifo_read_idx = 0;
static volatile uint32_t tx_fifo_write_idx = 0;

// Communication activity tracking
static volatile uint32_t last_comm_time_ms = 0;
static volatile uint32_t led_timer_ms = 0;
static volatile bool led_state = false;

// LED pin definition (from main.c)
#define LED_PIN PICO_DEFAULT_LED_PIN

// Communication timeout (consider idle after 500ms of no activity)
#define COMM_IDLE_TIMEOUT_MS 500U

// Private Function Prototypes ----------------------------------------------------------------------------------------

/**
 * @brief Write byte to RX FIFO (called from Core1)
 */
static inline void rxFifoPush(uint8_t byte) {
  uint32_t next_write_idx = (rx_fifo_write_idx + 1) % RX_FIFO_SIZE;
  if (next_write_idx != rx_fifo_read_idx) {
    rx_fifo[rx_fifo_write_idx] = byte;
    rx_fifo_write_idx = next_write_idx;
  }
}

/**
 * @brief Read byte from RX FIFO (called from Core0)
 */
static inline bool rxFifoPop(uint8_t& byte) {
  if (rx_fifo_read_idx != rx_fifo_write_idx) {
    byte = rx_fifo[rx_fifo_read_idx];
    rx_fifo_read_idx = (rx_fifo_read_idx + 1) % RX_FIFO_SIZE;
    return true;
  }
  return false;
}

/**
 * @brief Write byte to TX FIFO (called from Core0)
 */
static inline void txFifoPush(uint8_t byte) {
  uint32_t next_write_idx = (tx_fifo_write_idx + 1) % TX_FIFO_SIZE;
  while (next_write_idx == tx_fifo_read_idx) {
    // Wait for space in FIFO
    tight_loop_contents();
  }
  tx_fifo[tx_fifo_write_idx] = byte;
  tx_fifo_write_idx = next_write_idx;
}

/**
 * @brief Read byte from TX FIFO (called from Core1)
 */
static inline bool txFifoPop(uint8_t& byte) {
  if (tx_fifo_read_idx != tx_fifo_write_idx) {
    byte = tx_fifo[tx_fifo_read_idx];
    tx_fifo_read_idx = (tx_fifo_read_idx + 1) % TX_FIFO_SIZE;
    return true;
  }
  return false;
}

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
 * @brief Block until message is received from USB CDC via Core1
 */
static void waitForMessage(msg::Msg& request) {
  std::array<std::uint8_t, MSG_SIZE> buffer;
  uint32_t buffer_idx = 0U;
  absolute_time_t timeout_time = nil_time;

  for (;;) {
    // Check for autostart override
    if (req_autostart) {
      hwi::startApp(device::FLASH_APP_START_ADDR);
    }

    // Try to read byte from RX FIFO
    uint8_t rx_byte;
    if (rxFifoPop(rx_byte)) {
      buffer[buffer_idx] = rx_byte;
      buffer_idx++;

      // Update communication timestamp
      last_comm_time_ms = to_ms_since_boot(get_absolute_time());

      if (buffer_idx >= buffer.size()) {
        break;
      }

      // Reset timeout
      timeout_time = make_timeout_time_us(MSG_TIMEOUT_US);
    } else {
      // Check if timeout occurred
      if (buffer_idx != 0 && !is_nil_time(timeout_time) && time_reached(timeout_time)) {
        buffer_idx = 0U;
        timeout_time = nil_time;
      }
    }

    tight_loop_contents();
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
 * @brief Transmit response to USB CDC via Core1
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

  /* Transmit message to TX FIFO */
  for (uint32_t buffer_idx = 0U; buffer_idx < buffer.size(); buffer_idx++) {
    txFifoPush(buffer.at(buffer_idx));
  }
}

// Public Functions ---------------------------------------------------------------------------------------------------

extern "C" void FRANKLYBOOT_Init(void) {
  // Initialize FIFOs
  rx_fifo_read_idx = 0;
  rx_fifo_write_idx = 0;
  tx_fifo_read_idx = 0;
  tx_fifo_write_idx = 0;
}

extern "C" void FRANKLYBOOT_Run(void) {
  static Handler<device::FLASH_START_ADDR, device::FLASH_APP_FIRST_PAGE, device::FLASH_SIZE, device::FLASH_PAGE_SIZE_BOOT>
      hBootloader;

  // Check if autostart shall be disabled
  // On RP2040, we can use watchdog scratch registers for persistent storage
  const bool autostart_disable = (watchdog_hw->scratch[0] == AUTOBOOT_DISABLE_OVERRIDE_KEY);
  watchdog_hw->scratch[0] = 0;  // Reset scratch register

  // Autostart is possible if a valid app in flash is available
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

extern "C" uint32_t FRANKLYBOOT_getDevSysTickHz(void) { return device::SYS_TICK; }

extern "C" void FRANKLYBOOT_autoStartISR(void) {
  /* Start app if possible */
  if (autostart_possible) {
    req_autostart = true;
  }
}

extern "C" bool FRANKLYBOOT_isCommunicating(void) {
  uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());
  return (current_time_ms - last_comm_time_ms) < COMM_IDLE_TIMEOUT_MS;
}

extern "C" void FRANKLYBOOT_updateLED(void) {
  uint32_t current_time_ms = to_ms_since_boot(get_absolute_time());

  // Check if we're actively communicating
  bool is_communicating = FRANKLYBOOT_isCommunicating();

  if (is_communicating) {
    // Fast flash during communication (100ms period = 50ms on, 50ms off)
    if ((current_time_ms - led_timer_ms) >= 50) {
      led_timer_ms = current_time_ms;
      led_state = !led_state;
      gpio_put(LED_PIN, led_state);
    }
  } else {
    // Slow blink when idle (2000ms period = 1000ms on, 1000ms off)
    if ((current_time_ms - led_timer_ms) >= 1000) {
      led_timer_ms = current_time_ms;
      led_state = !led_state;
      gpio_put(LED_PIN, led_state);
    }
  }
}

/**
 * @brief Core1 entry point - handles USB CDC communication
 */
extern "C" void FRANKLYBOOT_Core1Entry(void) {
  // Initialize TinyUSB on Core1
  tusb_init();

  for (;;) {
    // Handle USB tasks
    tud_task();

    // Handle RX: USB CDC -> RX FIFO
    if (tud_cdc_connected() && tud_cdc_available()) {
      uint8_t buf[64];
      uint32_t count = tud_cdc_read(buf, sizeof(buf));
      for (uint32_t i = 0; i < count; i++) {
        rxFifoPush(buf[i]);
      }
    }

    // Handle TX: TX FIFO -> USB CDC
    if (tud_cdc_connected()) {
      uint8_t buf[64];
      uint32_t count = 0;
      while (count < sizeof(buf) && txFifoPop(buf[count])) {
        count++;
      }
      if (count > 0) {
        tud_cdc_write(buf, count);
        tud_cdc_write_flush();
      }
    }

    tight_loop_contents();
  }
}

// Hardware Interface -------------------------------------------------------------------------------------------------

void hwi::resetDevice() {
  /* Delay system reset */
  sleep_ms(100);

  // Use watchdog to reset the device
  watchdog_reboot(0, 0, 0);

  // Should not reach here
  while(1) {
    tight_loop_contents();
  }
}

[[nodiscard]] uint32_t hwi::getVendorID() { return __DEVICE_IDENT__[DEV_IDENT_VENDOR_ID]; }

[[nodiscard]] uint32_t hwi::getProductID() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCT_ID]; }

[[nodiscard]] uint32_t hwi::getProductionDate() { return __DEVICE_IDENT__[DEV_IDENT_PRODUCTION_DATE]; }

[[nodiscard]] uint32_t hwi::getUniqueIDWord(const uint32_t idx) {
  // RP2040 has 8-byte unique ID
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);

  uint32_t uid_value = 0;
  if (idx < 2U) {
    // Return as 32-bit words
    uid_value = ((uint32_t)board_id.id[idx * 4 + 0] << 0) |
                ((uint32_t)board_id.id[idx * 4 + 1] << 8) |
                ((uint32_t)board_id.id[idx * 4 + 2] << 16) |
                ((uint32_t)board_id.id[idx * 4 + 3] << 24);
  }

  return uid_value;
}

// Software CRC-32 implementation (RP2040 doesn't have hardware CRC)
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

static void init_crc32_table() {
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (uint32_t j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
    crc32_table[i] = crc;
  }
  crc32_table_initialized = true;
}

uint32_t hwi::calculateCRC(uint32_t src_address, uint32_t num_bytes) {
  if (!crc32_table_initialized) {
    init_crc32_table();
  }

  uint32_t crc = 0xFFFFFFFF;
  uint8_t* data_ptr = (uint8_t*)src_address;

  for (uint32_t i = 0; i < num_bytes; i++) {
    uint8_t byte = data_ptr[i];
    crc = (crc >> 8) ^ crc32_table[(crc ^ byte) & 0xFF];
  }

  return ~crc;
}

bool hwi::eraseFlashPage(uint32_t page_id) {
  // RP2040 flash sector size is 4KB
  const uint32_t sector_size = 4096U;

  // Calculate flash offset from page ID
  uint32_t flash_offset = page_id * sector_size;

  // Disable interrupts during flash operation
  uint32_t ints = save_and_disable_interrupts();

  // Erase flash sector (4KB on RP2040)
  flash_range_erase(flash_offset, sector_size);

  // Restore interrupts
  restore_interrupts(ints);

  return true;
}

bool hwi::writeDataBufferToFlash(uint32_t dst_address, uint32_t dst_page_id, uint8_t* src_data_ptr,
                                 uint32_t num_bytes) {
  // Check if data size is valid (must be multiple of 256 bytes for RP2040)
  if ((num_bytes % FLASH_PAGE_SIZE) != 0) {
    return false;
  }

  // Calculate flash offset (remove XIP base address)
  uint32_t flash_offset = dst_address - device::FLASH_START_ADDR;

  // Disable interrupts during flash operation
  uint32_t ints = save_and_disable_interrupts();

  // Program flash
  flash_range_program(flash_offset, src_data_ptr, num_bytes);

  // Restore interrupts
  restore_interrupts(ints);

  return true;
}

[[nodiscard]] uint8_t franklyboot::hwi::readByteFromFlash(uint32_t flash_src_address) {
  uint8_t* flash_src_ptr = (uint8_t*)(flash_src_address);
  return *(flash_src_ptr);
}

void franklyboot::hwi::startApp(uint32_t app_flash_address) {
  // Sleep before starting app
  //sleep_ms(100);
  
  // Disable interrupts
  __asm volatile("cpsid i");

  // Deinitialize USB
  tud_disconnect();
  tud_deinit(BOARD_TUD_RHPORT);

  // Reset Core1
  multicore_reset_core1();

  // Disable SysTick
  systick_hw->csr = 0;
  systick_hw->rvr = 0;

  // Clear pending interrupts
  scb_hw->icsr = M0PLUS_ICSR_PENDSVCLR_BITS | M0PLUS_ICSR_PENDSTCLR_BITS;

  // Disable all peripheral interrupts
  *((volatile uint32_t*)0xe000e180) = 0xFFFFFFFF;  // NVIC_ICER0

  // CRITICAL: Flush XIP cache!
  // After writing to flash, the XIP cache contains stale data
  // Must flush it before jumping to the application
  xip_ctrl_hw->flush = 1;
  while (!(xip_ctrl_hw->stat & XIP_STAT_FLUSH_READY_BITS)) {
    tight_loop_contents();
  }

  // Get application stack pointer and reset handler
  uint32_t* app_vector_table = (uint32_t*)app_flash_address;
  uint32_t app_stack_pointer = app_vector_table[0];
  void (*app_reset_handler)(void) = (void (*)(void))app_vector_table[1];

  // CRITICAL: Clear RAM before jumping!
  // The bootloader runs from RAM and leaves artifacts that corrupt the app's .data section
  // Clear from start of RAM to just before the stack
  volatile uint32_t* ram_clear = (volatile uint32_t*)0x20000000;
  volatile uint32_t* ram_clear_end = (volatile uint32_t*)(app_stack_pointer - 0x1000);  // Leave 4KB safety margin before stack
  while (ram_clear < ram_clear_end) {
    *ram_clear++ = 0;
  }

  // Set vector table offset
  scb_hw->vtor = app_flash_address;

  // Set stack pointer
  __asm volatile("MSR msp, %0" : : "r" (app_stack_pointer) : );

  // Jump to application
  __asm volatile("cpsie i");
  app_reset_handler();

  // Should never reach here
  while(1) {
    tight_loop_contents();
  }
}
