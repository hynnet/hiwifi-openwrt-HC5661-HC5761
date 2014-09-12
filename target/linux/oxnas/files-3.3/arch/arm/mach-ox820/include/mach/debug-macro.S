/* linux/include/asm-arm/arch-oxnas/debug-macro.S
 *
 * Debugging macro include header
 *
 *  Copyright (C) 2005 B.H.Clarke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/
#include <mach/hardware.h>

		.macro  addruart,rx, rp
/*		mrc		p15, 0, \rx, c1, c0
		tst		\rx, #1					@ MMU enabled?*/
#ifdef CONFIG_ARCH_OXNAS_UART1
		ldr	\rx, =UART_1_BASE_PA	@ physical base address
		ldr	\rp, =UART_1_BASE		@ virtual address
#elif CONFIG_ARCH_OXNAS_UART2
		ldreq	\rx, =UART_2_BASE_PA	@ physical base address
		ldrne	\rx, =UART_2_BASE		@ virtual address
#elif CONFIG_ARCH_OXNAS_UART3
		ldreq	\rx, =UART_3_BASE_PA	@ physical base address
		ldrne	\rx, =UART_3_BASE		@ virtual address
#else
		ldreq	\rx, =UART_4_BASE_PA	@ physical base address
		ldrne	\rx, =UART_4_BASE		@ virtual address
#endif
		.endm

		.macro	senduart,rd,rx			@ Load byte into Tx holding register
		strb	\rd, [\rx, #0]			@ THR
		.endm

		.macro	waituart,rd,rx			@ Wait until there is space in the TX FIFO
1001:	ldrb	\rd, [\rx, #5]			@ LSR
		tst		\rd, #1 << 5			@ THR empty
		beq		1001b
		.endm

		.macro	busyuart,rd,rx			@ Wait until the TX is idle
#1001:	ldrb	\rd, [\rx, #5]			@ LSR
#		tst		\rd, #1 << 6			@ THR and Tx FIFO empty
#		beq		1001b
		.endm

