/*
 *	Oxford Semiconductor Ltd 2009
 *
 *	Title  : start.S
 *
 *	Description:
 *	Entry point for stage-1 loader to be copied from disk into SRAM by the ROM
 *	loader boot-from-SATA code
 *
 *	Modifications:
 *		BHC: 24/05/06	Reworked to add stage2 loader environment load from disk
 *      JJL: 21/07/06   Restructured to provide space for boot loader validation
 *      RC : 24/02/09   Updated for ox82x
 */	
 
.section .init
.org 0
	ldr pc, [pc, #pc_start-.-8]

    
/* space for the CRC and length information needed by the boot ROM */
.org 0x30 
	.ascii "BOOT"
	.word   0x50

.org 0x3C
.ifdef COPRO_COUNT
	.word   0x50007000  /* boot report location */
.else
	.word   0x5000f000  /* boot report location */
.endif

pc_start: 
    .word   start
image_length:   .word code_length
image_crc:      .word 0
header_crc:     .word 0
check_start:
.global check_start

.ifdef COPRO_COUNT
.org 0x80
/* leave room for coprocessor code and configuration */
	.space (16 + 12 * COPRO_COUNT), 0
.endif

.section .text	
/* end of header */

start:
/* setup the stack and jump into the C language main() */
.ifdef  COPRO_COUNT
/* use a busy wait to allow the co-processor to report activity */
	mov r0, #0x1
	mov r0, r0, LSL #24
wait_copro:
	subs r0, r0, #1
	bne wait_copro
.endif

	ldr	r13,	initial_stack
	ldr	pc,		= main

initial_stack:	.word	0x50009ff0
