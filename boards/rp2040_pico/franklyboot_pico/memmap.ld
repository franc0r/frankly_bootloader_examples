/**
 * @file memmap.ld
 * @brief Linker script for RP2040 Pico bootloader
 * @author Martin Bauernschmitt (martin.bauernschmitt@francor.de)
 * @date 2025-09-14
 * @copyright Copyright (c) 2025 - BSD-3-clause - FRANCOR e.V.
 */

MEMORY
{
    /*
     * RP2040 Memory Layout:
     * - External QSPI Flash: 2MB starting at 0x10000000 (XIP)
     * - SRAM: 264KB starting at 0x20000000
     * - Boot2: First 256 bytes of flash
     * - Bootloader: Remaining 64KB minus 256 bytes
     * - Application: Remaining flash space
     */
    BOOT2 (rx) : ORIGIN = 0x10000000, LENGTH = 256
    FLASH (rx) : ORIGIN = 0x10000100, LENGTH = 64k - 256
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 264k
}

/* Stack size definitions */
STACK_SIZE = 0x800;   /* 2KB stack */
HEAP_SIZE  = 0x400;   /* 1KB heap */

/* Entry point */
ENTRY(_stage2_boot)

SECTIONS
{
    /* Second stage bootloader (exactly 256 bytes) */
    .boot2 : {
        KEEP(*(.boot2))
    } > BOOT2

    /* Program code starts after boot2 */
    .text : {
        KEEP(*(.vectors))   /* Vector table must be first */
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Device identification section */
    ._dev_ident : {
        KEEP(*(.dev_ident))
        . = ALIGN(4);
    } > FLASH

    /* ARM sections */
    .ARM.exidx : {
        *(.ARM.exidx*)
    } > FLASH

    /* Data section (initialized data) */
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    /* BSS section (zero-initialized data) */
    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Heap */
    .heap : {
        . = ALIGN(4);
        _heap_start = .;
        . += HEAP_SIZE;
        _heap_end = .;
    } > RAM

    /* Stack */
    .stack : {
        . = ALIGN(8);
        . += STACK_SIZE;
        _stack_top = .;
    } > RAM

    /* Memory usage check */
    _end_of_ram = ORIGIN(RAM) + LENGTH(RAM);
    ASSERT(_stack_top <= _end_of_ram, "Stack overflows RAM")
}