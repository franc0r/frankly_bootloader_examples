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

// Public Functions ---------------------------------------------------------------------------------------------------

void SystemInit(void) {

}

int main(void) {
    for(;;) {
      __NOP();
    }

    return 0;
}

