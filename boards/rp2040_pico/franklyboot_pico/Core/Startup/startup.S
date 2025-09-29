  .syntax unified
	.cpu cortex-m0plus
	.fpu softvfp
	.thumb

.word _sitext
.word _stext
.word _etext

.word _sidata
.word _sdata
.word _edata

.word _estack

.word _siram_app
.word _sram_app
.word _eram_app

.set MAIN, 0x10000219

    .section	.load_func.Reset_Handler,"ax",%progbits
	.weak	Reset_Handler
	.type	Reset_Handler, %function
Reset_Handler:
	/* Set up stack pointer */
	ldr r0, =_estack
	mov sp, r0

	/* Copy complete app to RAM */
	ldr r0, =_sram_app      /* destination start in RAM */
	ldr r1, =_eram_app       /* destination end in RAM */
	ldr r2, =_siram_app      /* source start */
	bl copy_section

	/* Copy .data section from flash to RAM */
	ldr r0, =_sdata          /* destination start */
	ldr r1, =_edata          /* destination end */
	ldr r2, =_sidata         /* source start */
	bl copy_section

	/* Zero .bss section */
	ldr r0, =_sbss           /* start */
	ldr r1, =_ebss           /* end */
	bl zero_section

	/* Jump to main application via RAM vector table */
	bl main

infinite_loop:
	b infinite_loop

/* Copy section helper function */
copy_section:
	cmp r0, r1               /* check if dest_start < dest_end */
	bge copy_done
copy_loop:
	ldr r3, [r2]             /* load from source */
	str r3, [r0]             /* store to destination */
	adds r0, r0, #4          /* increment destination */
	adds r2, r2, #4          /* increment source */
	cmp r0, r1               /* check if done */
	blt copy_loop
copy_done:
	bx lr

/* Zero section helper function */
zero_section:
	movs r2, #0              /* zero value */
	cmp r0, r1               /* check if start < end */
	bge zero_done
zero_loop:
	str r2, [r0]             /* store zero */
	adds r0, r0, #4          /* increment address */
	cmp r0, r1               /* check if done */
	blt zero_loop
zero_done:
	bx lr

.size	Reset_Handler, .-Reset_Handler


 	.section	.isr_vector,"a",%progbits
	.type	g_pfnFlashVector, %object
	.size	g_pfnFlashVector, .-g_pfnFlashVector
g_pfnFlashVector:
	.word	_estack
	.word	Reset_Handler
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
    .word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
