/**
 * @file main.c
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @brief Main Source File of Example firmware
 * @version 1.0
 * @date 2023-01-09
 *
 * @copyright Copyright (c) 2023 - BSD-3-clause - FRANCOR e.V.
 *
 */

// Includes -----------------------------------------------------------------------------------------------------------
#include "bootloader_api.h"
#include "device_defines.h"
#include "stm32l4xx.h"

// Private Functions --------------------------------------------------------------------------------------------------

/** \brief Init core and sys clocks */
static void initCore(void);

/** \brief Init CRC unit */
static void initCRC(void);

/** \brief Init CAN unit */
static void initCAN(void);

/** \brief Init Systick ISR */
static void initSysTick(void);

// Public Functions ---------------------------------------------------------------------------------------------------

void SystemInit(void) {
  initCore();
  initCRC();
  initCAN();
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
  RCC->APB1ENR1 = RCC_APB1ENR1_CAN1EN | RCC_APB1ENR1_RTCAPBEN;

  // Config GPIOs for CAN Pin PA11 & PA12
  GPIOA->MODER = 0xAABFFFFF;
  GPIOA->OSPEEDR = 0x0FC00000;
  GPIOA->AFR[1] = 0x00099000;
}

static void initCRC(void) {
  // Set data input inversion mode to byte
  MODIFY_REG(CRC->CR, CRC_CR_REV_IN, CRC_CR_REV_IN_0);

  // Set data output inversion
  MODIFY_REG(CRC->CR, CRC_CR_REV_OUT, CRC_CR_REV_OUT);
}

static void initCAN(void) {
  uint32_t max_timeout_ticks = 4000000;

  // Exit sleep mode and request initialization
  CLEAR_BIT(CAN->MCR, CAN_MCR_SLEEP);
  SET_BIT(CAN->MCR, CAN_MCR_INRQ);

  uint32_t ticks = 0U;
  while ((CAN->MSR & CAN_MSR_INAK) == 0) {
    ticks++;
    if (ticks > max_timeout_ticks) {
      __NOP();  // TODO add error reaction
    }
  }

  // Config CAN module
  SET_BIT(CAN->MCR, CAN_MCR_AWUM);  // Enable auto wakeup
  CAN->BTR = 0x001C0001;            // Config CAN speed to 500 kBit/s

  // Enable CAN module
  CLEAR_BIT(CAN->MCR, CAN_MCR_INRQ);

  while ((CAN->MSR & CAN_MSR_INAK) == CAN_MSR_INAK) {
    ticks++;

    if (ticks > max_timeout_ticks) {
      __NOP();  // TODO add error reaction
    }
  }

  // Setup CAN filters for reception
  SET_BIT(CAN->FMR, CAN_FMR_FINIT);  // Enable filter init mode

  // Determine IDs and mask
  const uint32_t msg_broadcast_id = CAN_BROADCAST_ID;
  const uint32_t msg_node_id = (msg_broadcast_id + 1U) + (CAN_NODE_ID << 1U);
  const uint32_t msg_mask = 0x7FF;

  // Setup filters
  CAN->sFilterRegister[0].FR2 = (msg_mask << 21U) | (msg_broadcast_id << 5U);
  CAN->sFilterRegister[1].FR2 = (msg_mask << 21U) | (msg_node_id << 5U);
  CAN->FA1R = 3U;

  CLEAR_BIT(CAN->FMR, CAN_FMR_FINIT);  // Disable filter init mode

  // Setup tx message
  CAN->sTxMailBox[0].TIR = ((msg_node_id + 1U) << CAN_TI0R_STID_Pos);
  CAN->sTxMailBox[0].TDTR = 8U;
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