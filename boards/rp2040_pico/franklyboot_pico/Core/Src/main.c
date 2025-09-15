/**
 * @file main.c
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Main Source File of RP2040 Pico bootloader firmware
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"

// RP2040 base addresses
#define RESETS_BASE           0x4000c000
#define USBCTRL_BASE          0x50110000
#define SYS_TICK_BASE         0xe000e010

// Register offsets
#define RESETS_RESET_OFFSET   0x0
#define RESETS_RESET_DONE_OFFSET 0x8
#define USB_CTRL_OFFSET       0x40
#define USB_ENDPOINT_CTRL_OFFSET 0x48

// Helper macros
#define REG(base, offset) (*(volatile uint32_t*)((base) + (offset)))

// Private Functions --------------------------------------------------------------------------------------------------

/** \brief Init core and release peripheral resets */
static void initCore(void);

/** \brief Init USB CDC for communication */
static void initUSB(void);

/** \brief Init Systick ISR */
static void initSysTick(void);

// Public Functions ---------------------------------------------------------------------------------------------------

void SystemInit(void) {
  initCore();
  initUSB();
  initSysTick();
  FRANKLYBOOT_Init();
}

int main(void) {
  FRANKLYBOOT_Run();
  return 0;
}

void SysTick_Handler(void) { FRANKLYBOOT_autoStartISR(); }

// Private Functions --------------------------------------------------------------------------------------------------

static void initCore(void) {
  // Release reset on required peripherals
  REG(RESETS_BASE, RESETS_RESET_OFFSET) &= ~((1 << 24)); // USBCTRL

  // Wait for reset release
  while (!(REG(RESETS_BASE, RESETS_RESET_DONE_OFFSET) & (1 << 24)));
}

static void initUSB(void) {
  // Basic USB controller initialization
  // For a minimal bootloader, we'll implement a simple CDC device
  // This is a placeholder - full USB CDC implementation would be complex

  // Enable USB controller
  REG(USBCTRL_BASE, USB_CTRL_OFFSET) |= (1 << 0); // Enable USB

  // Note: Full USB CDC implementation would require:
  // - USB descriptor setup
  // - Endpoint configuration
  // - USB enumeration handling
  // - CDC class-specific requests
  // For now, this is a minimal placeholder
}

static void initSysTick(void) {
  // Set reload value for 1 second timeout (12MHz default clock)
  const uint32_t tick_value = 12000000U - 1;
  REG(SYS_TICK_BASE, 0x4) = tick_value;  // SYST_RVR
  REG(SYS_TICK_BASE, 0x8) = tick_value;  // SYST_CVR

  // Enable SysTick with interrupt
  REG(SYS_TICK_BASE, 0x0) = (1 << 2) | (1 << 1) | (1 << 0);  // SYST_CSR
}