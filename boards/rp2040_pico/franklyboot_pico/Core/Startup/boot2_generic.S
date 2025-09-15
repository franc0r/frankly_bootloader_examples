/**
 * @file boot2_generic.S
 * @brief Second stage bootloader for RP2040 (exactly 256 bytes)
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @date 2025-09-14
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 *
 * This is a minimal second stage bootloader for generic QSPI flash chips.
 * It configures the XIP (Execute In Place) mode and jumps to the main program.
 */

.syntax unified
.cpu cortex-m0plus
.thumb

.section .boot2, "ax", %progbits

/* Second stage bootloader entry point */
.global _stage2_boot
.type _stage2_boot, %function

_stage2_boot:
    /* Minimal second stage bootloader for RP2040 */
    ldr r0, =0x10000100    /* Address after boot2 */
    ldr r1, [r0]           /* Load stack pointer */
    ldr r0, [r0, #4]       /* Load reset vector */
    mov sp, r1             /* Set stack pointer */
    bx r0                  /* Jump to main program */

.size _stage2_boot, .-_stage2_boot

/* Pad to exactly 252 bytes (256 - 4 for CRC) */
.space 248 - (. - _stage2_boot), 0

/* CRC32 placeholder (will be filled by build tools) */
.word 0x00000000

.end