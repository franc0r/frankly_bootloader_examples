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
#include "stm32f3xx.h"

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
  // Enable clocks
  RCC->AHBENR = RCC_AHBENR_CRCEN | RCC_AHBENR_GPIOAEN;
  RCC->APB1ENR = RCC_APB1ENR_USART2EN | RCC_APB1ENR_PWREN;

  // Config GPIOs for UART PA2 and PA15

  // Setup alternate function mode
  uint32_t regValue = GPIOA->MODER;
  CLEAR_BIT(regValue, (GPIO_MODER_MODER2_Msk | GPIO_MODER_MODER15_Msk));
  SET_BIT(regValue, (2 << GPIO_MODER_MODER2_Pos) | (2 << GPIO_MODER_MODER15_Pos));
  GPIOA->MODER = regValue;

  // Setup AF7 for UART
  SET_BIT(GPIOA->AFR[0], (7 << GPIO_AFRL_AFRL2_Pos));
  SET_BIT(GPIOA->AFR[1], (7 << GPIO_AFRH_AFRH7_Pos));

  // Setup UART
  WRITE_REG(USART2->BRR, 0x45); // 115200 baud
  SET_BIT(USART2->CR1, USART_CR1_TE | USART_CR1_RE | USART_CR1_UE); // Enable UART
  
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