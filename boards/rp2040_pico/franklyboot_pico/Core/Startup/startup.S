/**
 * @file startup.S
 * @brief RP2040 Startup file for bootloader
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @date 2025-09-14
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 */

.syntax unified
.cpu cortex-m0plus
.thumb

/* Memory layout definitions from linker script */
.extern _sidata    /* Start of initialized data in flash */
.extern _sdata     /* Start of data section in RAM */
.extern _edata     /* End of data section in RAM */
.extern _sbss      /* Start of BSS section */
.extern _ebss      /* End of BSS section */
.extern _stack_top /* Top of stack */

/* External functions */
.extern SystemInit
.extern main

/* Vector table */
.section .vectors, "a", %progbits
.global g_pfnVectors
.global _start

g_pfnVectors:
_start:
    .word _stack_top                    /* 0x00: Initial stack pointer */
    .word Reset_Handler + 1             /* 0x04: Reset handler */
    .word NMI_Handler + 1               /* 0x08: NMI handler */
    .word HardFault_Handler + 1         /* 0x0C: Hard fault handler */
    .word 0                             /* 0x10: Reserved */
    .word 0                             /* 0x14: Reserved */
    .word 0                             /* 0x18: Reserved */
    .word 0                             /* 0x1C: Reserved */
    .word 0                             /* 0x20: Reserved */
    .word 0                             /* 0x24: Reserved */
    .word 0                             /* 0x28: Reserved */
    .word SVC_Handler + 1               /* 0x2C: SVCall handler */
    .word 0                             /* 0x30: Reserved */
    .word 0                             /* 0x34: Reserved */
    .word PendSV_Handler + 1            /* 0x38: PendSV handler */
    .word SysTick_Handler + 1           /* 0x3C: SysTick handler */

    /* External interrupts for RP2040 */
    .word TIMER_IRQ_0_Handler + 1       /* 0x40: Timer 0 */
    .word TIMER_IRQ_1_Handler + 1       /* 0x44: Timer 1 */
    .word TIMER_IRQ_2_Handler + 1       /* 0x48: Timer 2 */
    .word TIMER_IRQ_3_Handler + 1       /* 0x4C: Timer 3 */
    .word PWM_IRQ_WRAP_Handler + 1      /* 0x50: PWM wrap */
    .word USBCTRL_IRQ_Handler + 1       /* 0x54: USB controller */
    .word XIP_IRQ_Handler + 1           /* 0x58: XIP cache */
    .word PIO0_IRQ_0_Handler + 1        /* 0x5C: PIO0 interrupt 0 */
    .word PIO0_IRQ_1_Handler + 1        /* 0x60: PIO0 interrupt 1 */
    .word PIO1_IRQ_0_Handler + 1        /* 0x64: PIO1 interrupt 0 */
    .word PIO1_IRQ_1_Handler + 1        /* 0x68: PIO1 interrupt 1 */
    .word DMA_IRQ_0_Handler + 1         /* 0x6C: DMA interrupt 0 */
    .word DMA_IRQ_1_Handler + 1         /* 0x70: DMA interrupt 1 */
    .word IO_IRQ_BANK0_Handler + 1      /* 0x74: GPIO bank 0 */
    .word IO_IRQ_QSPI_Handler + 1       /* 0x78: QSPI GPIO */
    .word SIO_IRQ_PROC0_Handler + 1     /* 0x7C: SIO processor 0 */
    .word SIO_IRQ_PROC1_Handler + 1     /* 0x80: SIO processor 1 */
    .word CLOCKS_IRQ_Handler + 1        /* 0x84: Clock control */
    .word SPI0_IRQ_Handler + 1          /* 0x88: SPI0 */
    .word SPI1_IRQ_Handler + 1          /* 0x8C: SPI1 */
    .word UART0_IRQ_Handler + 1         /* 0x90: UART0 */
    .word UART1_IRQ_Handler + 1         /* 0x94: UART1 */
    .word ADC_IRQ_FIFO_Handler + 1      /* 0x98: ADC FIFO */
    .word I2C0_IRQ_Handler + 1          /* 0x9C: I2C0 */
    .word I2C1_IRQ_Handler + 1          /* 0xA0: I2C1 */
    .word RTC_IRQ_Handler + 1           /* 0xA4: RTC */

/* Reset handler */
.section .text.Reset_Handler, "ax", %progbits
.global Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    /* Set stack pointer */
    ldr r0, =_stack_top
    mov sp, r0

    /* Copy initialized data from flash to RAM */
    ldr r0, =_sdata        /* Destination start */
    ldr r1, =_edata        /* Destination end */
    ldr r2, =_sidata       /* Source start */
    movs r3, #0
    b copy_data_check

copy_data_loop:
    ldr r4, [r2, r3]       /* Load from flash */
    str r4, [r0, r3]       /* Store to RAM */
    adds r3, r3, #4        /* Increment offset */

copy_data_check:
    adds r4, r0, r3        /* Calculate current destination */
    cmp r4, r1             /* Compare with end */
    bcc copy_data_loop     /* Continue if not done */

    /* Zero-initialize BSS section */
    ldr r0, =_sbss         /* BSS start */
    ldr r1, =_ebss         /* BSS end */
    movs r2, #0            /* Zero value */
    b zero_bss_check

zero_bss_loop:
    str r2, [r0]           /* Store zero */
    adds r0, r0, #4        /* Next word */

zero_bss_check:
    cmp r0, r1             /* Compare with end */
    bcc zero_bss_loop      /* Continue if not done */

    /* Call SystemInit */
    bl SystemInit

    /* Call main */
    bl main

    /* Infinite loop if main returns */
infinite_loop:
    b infinite_loop

.size Reset_Handler, .-Reset_Handler

/* Default exception handlers */
.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function

Default_Handler:
    b Default_Handler

.size Default_Handler, .-Default_Handler

/* Exception handler aliases */
.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler

.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler

.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler

.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler

.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler

/* RP2040 interrupt handler aliases */
.weak TIMER_IRQ_0_Handler
.thumb_set TIMER_IRQ_0_Handler, Default_Handler

.weak TIMER_IRQ_1_Handler
.thumb_set TIMER_IRQ_1_Handler, Default_Handler

.weak TIMER_IRQ_2_Handler
.thumb_set TIMER_IRQ_2_Handler, Default_Handler

.weak TIMER_IRQ_3_Handler
.thumb_set TIMER_IRQ_3_Handler, Default_Handler

.weak PWM_IRQ_WRAP_Handler
.thumb_set PWM_IRQ_WRAP_Handler, Default_Handler

.weak USBCTRL_IRQ_Handler
.thumb_set USBCTRL_IRQ_Handler, Default_Handler

.weak XIP_IRQ_Handler
.thumb_set XIP_IRQ_Handler, Default_Handler

.weak PIO0_IRQ_0_Handler
.thumb_set PIO0_IRQ_0_Handler, Default_Handler

.weak PIO0_IRQ_1_Handler
.thumb_set PIO0_IRQ_1_Handler, Default_Handler

.weak PIO1_IRQ_0_Handler
.thumb_set PIO1_IRQ_0_Handler, Default_Handler

.weak PIO1_IRQ_1_Handler
.thumb_set PIO1_IRQ_1_Handler, Default_Handler

.weak DMA_IRQ_0_Handler
.thumb_set DMA_IRQ_0_Handler, Default_Handler

.weak DMA_IRQ_1_Handler
.thumb_set DMA_IRQ_1_Handler, Default_Handler

.weak IO_IRQ_BANK0_Handler
.thumb_set IO_IRQ_BANK0_Handler, Default_Handler

.weak IO_IRQ_QSPI_Handler
.thumb_set IO_IRQ_QSPI_Handler, Default_Handler

.weak SIO_IRQ_PROC0_Handler
.thumb_set SIO_IRQ_PROC0_Handler, Default_Handler

.weak SIO_IRQ_PROC1_Handler
.thumb_set SIO_IRQ_PROC1_Handler, Default_Handler

.weak CLOCKS_IRQ_Handler
.thumb_set CLOCKS_IRQ_Handler, Default_Handler

.weak SPI0_IRQ_Handler
.thumb_set SPI0_IRQ_Handler, Default_Handler

.weak SPI1_IRQ_Handler
.thumb_set SPI1_IRQ_Handler, Default_Handler

.weak UART0_IRQ_Handler
.thumb_set UART0_IRQ_Handler, Default_Handler

.weak UART1_IRQ_Handler
.thumb_set UART1_IRQ_Handler, Default_Handler

.weak ADC_IRQ_FIFO_Handler
.thumb_set ADC_IRQ_FIFO_Handler, Default_Handler

.weak I2C0_IRQ_Handler
.thumb_set I2C0_IRQ_Handler, Default_Handler

.weak I2C1_IRQ_Handler
.thumb_set I2C1_IRQ_Handler, Default_Handler

.weak RTC_IRQ_Handler
.thumb_set RTC_IRQ_Handler, Default_Handler

/* CMSIS intrinsic functions */
.section .text.cmsis_intrinsics, "ax", %progbits

.global __disable_irq
.type __disable_irq, %function
__disable_irq:
    cpsid i
    bx lr
.size __disable_irq, .-__disable_irq

.global __enable_irq
.type __enable_irq, %function
__enable_irq:
    cpsie i
    bx lr
.size __enable_irq, .-__enable_irq

.global __NOP
.type __NOP, %function
__NOP:
    nop
    bx lr
.size __NOP, .-__NOP

.global __set_MSP
.type __set_MSP, %function
__set_MSP:
    msr msp, r0
    bx lr
.size __set_MSP, .-__set_MSP

/* Minimal syscalls */
.section .text.syscalls, "ax", %progbits

.global _exit
.type _exit, %function
_exit:
    b _exit
.size _exit, .-_exit

.global _kill
.type _kill, %function
_kill:
    movs r0, #0
    subs r0, #1
    bx lr
.size _kill, .-_kill

.global _getpid
.type _getpid, %function
_getpid:
    movs r0, #1
    bx lr
.size _getpid, .-_getpid

.end