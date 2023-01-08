/**
 * @file main.c
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Main Source File of Example firmware
 * @version 1.0
 * @date 2022-12-02
 *
 * @copyright Copyright (c) 2022 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"
#include "stm32g4xx.h"

// Private Functions --------------------------------------------------------------------------------------------------

/** \brief Init core and sys clocks */
static void initCore(void);

/** \brief Init CRC unit */
static void initCRC(void);

/** \brief Init Systick ISR */
static void initSysTick(void);

// Public Functions ---------------------------------------------------------------------------------------------------

void SystemInit(void) {
  initCore();
  initCRC();
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
  // Enable HSI16 clock
  RCC->CR = RCC->CR | RCC_CR_HSION;

  // Wait for HSI
  uint8_t HSI_NOT_RDY = 1;
  while (HSI_NOT_RDY) {
    HSI_NOT_RDY = ((RCC->CR & RCC_CR_HSIRDY) != RCC_CR_HSIRDY);
  }

  // Switch to HSI16
  RCC->CFGR = RCC->CFGR | RCC_CFGR_SW_HSI;

  // Enable Clocks
  RCC->AHB1ENR = RCC_AHB1ENR_FLASHEN | RCC_AHB1ENR_CRCEN;
  RCC->AHB2ENR = RCC_AHB2ENR_GPIOAEN;
  RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

  // Config GPIOs for UART Pin PA2 & PA3

  // Setup alternate function mode
  uint32_t regValue = GPIOA->MODER;
  CLEAR_BIT(regValue, (GPIO_MODER_MODE2_Msk | GPIO_MODER_MODE3_Msk));
  SET_BIT(regValue, (2 << GPIO_MODER_MODE2_Pos) | (2U << GPIO_MODER_MODE3_Pos));
  GPIOA->MODER = regValue;

  // Setup AF12 mode for UART
  SET_BIT(GPIOA->AFR[0], (12 << GPIO_AFRL_AFSEL2_Pos));
  SET_BIT(GPIOA->AFR[0], (12 << GPIO_AFRL_AFSEL3_Pos));

  // Setup UART
  WRITE_REG(LPUART1->BRR, 0x8AE4);
  WRITE_REG(LPUART1->CR1, 0xD);
}

static void initCRC(void) {
  // Set data input inversion mode to byte
  MODIFY_REG(CRC->CR, CRC_CR_REV_IN, CRC_CR_REV_IN_0);

  // Set data output inversion
  MODIFY_REG(CRC->CR, CRC_CR_REV_OUT, CRC_CR_REV_OUT);
}

static void initSysTick(void) {
  // Sys tick is configured to 1 sec.
  // After 1 sec and ISR is called and if a valid app is found
  // the application is started. If the bootloader gets an ping the
  // autostart is canceled.
  // The sys tick is not used as normal timer in this case!

  // Set reload value
  const uint32_t tick_value = FRANKLYBOOT_getDevSysTickHz() - 1;
  SysTick->LOAD = tick_value;
  SysTick->VAL = tick_value;

  // Set priority
  NVIC_SetPriority(SysTick_IRQn, 0);

  // Enable SysTick
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}