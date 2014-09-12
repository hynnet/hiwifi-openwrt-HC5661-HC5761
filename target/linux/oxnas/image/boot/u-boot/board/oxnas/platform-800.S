/*
 * Board specific setup info
 *
 * (C) Copyright 2005
 * Oxford Semiconductor Ltd
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <version.h>

/* use estimate of processor speed to calculate number of cycles delay */
/* delay count is nominal (PLL200 frequency x delay time) / loop count 
 * expressing 200us as 200/1000000 and re-arranging gives the expression below
 */
 
#define DELAY_200US   ((NOMINAL_ARMCLK / (5 * 1000000)) * 200) 
/* this is 8 cycles of ? so choose 8 resulting in 40 cycles */
#define DELAY_1S   ((DELAY_200US) * 5000)
#define DELAY_8       8
#define DELAY_200     200

.globl platformsetup
platformsetup:
/* register allocations
 * r0 - delay counter and scratch
 * r1 - address register
 * r2 - data register
 * r3 - index to table pointer
 * r4 - iteration counter.
 * 
 * r5 - hold return address.
 * lr - (R14) link register
 * pc - (R15) program counter.
 */

#ifdef INITIALISE_SDRAM
/*
 * Check that not in SDRAM execution. Suicide if re-initialise DRAM.
 * Controller function is linked to execute in SDRAM must be in ROM if not
 * there. Check for wrong place.
 */
	adrl r0, platformsetup		/* Relative location of function start.*/
	ldr  r1, _platformsetup
	cmp  r0, r1
	moveq pc, lr
#else
	mov pc, lr
#endif

	/* Establish a working setup for the SDRAM */
	mov r6, lr

#ifdef OXNAS_OVERCLOCK
	/* Delay so the broken JTAG can get control */
	ldr r0, =DELAY_1S
	bl delay

	/* Configure the PLL to run faster */
	ldr r1, =SYS_CTRL_PLLSYS_CTRL
	ldr r2, =SYS_CTRL_PLLSYS_KEY_CTRL

	/* 0xBEADFACE -> PLL_KEY */
	/* Bypass PLL */
	ldr r3, [r1]
	ldr r5, =0x20000
	orr r3, r3, r5
	ldr r4, =0xbeadface
	str r4, [r2]
	str r3, [r1]

	/* 0xBEADFACE -> PLL_KEY */
	/* Set m,p and s for PLL at 400MHz */
	ldr r5, =0xffff0000
	and r3, r3, r5
	ldr r5, =OXNAS_OVERCLOCK
	orr r3, r3, r5
	str r4, [r2]
	str r3, [r1]

	/* Wait at least 300uS */
	ldr r0, =DELAY_200US
	bl delay
	ldr r0, =DELAY_200US
	bl delay

	/* 0xBEADFACE -> PLL_KEY */
	/* Disable PLL bypass */
	ldr r5, =0xfffdffff
	and r3, r3, r5
	str r4, [r2]
	str r3, [r1]
#endif // OXNAS_OVERCLOCK

	/* Assert reset to the DDR core */
	ldr r0, =SYS_CTRL_RSTEN_SET_CTRL
	ldr r1, =1
	ldr r2, =SYS_CTRL_RSTEN_DDR_BIT
	mov r1, r1, LSL r2
	str r1, [r0]

	/* Deassert reset to the DDR core */
	ldr r0, =SYS_CTRL_RSTEN_CLR_CTRL
	str r1, [r0]

	/* Turn on the DDR core clock */
	ldr r0, =SYS_CTRL_CKEN_SET_CTRL
	ldr r1, =1
	ldr r2, =SYS_CTRL_CKEN_DDR_BIT
	mov r1, r1, LSL r2
	str r1, [r0]

	/* Start using the initialisation value list */
	adrl r3, init_table

	/* Copy next 6 entries from DDR init table*/
	ldr r4, =6
loop0:
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loop0

	/* Delay for 200uS while DRAM controller stabilises. */
	ldr r0, =DELAY_200US
	bl delay

#if !TEST_BRD
	/* Copy next entry */
	ldr r4, =1
loopx:	
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loopx

	/* Delay for 200uS while DRAM controller stabilises. */
	ldr r0, =DELAY_200US
	bl delay
#endif // TEST_BRD

	/* Copy next entry */
	ldr r4, =1
loop1:	
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loop1

	/* Delay for 200uS while DRAM controller stabilises. */
	ldr r0, =DELAY_200US
	bl delay

	/* Copy next entry */
	ldr r4, =1
loop2:	
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loop2

	/* Delay for 200uS while DRAM controller stabilises. */
	ldr r0, =DELAY_200US
	bl delay

	/* Copy next entry */
	ldr r4, =1
loop3:	
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loop3

	/* Delay for 200uS while DRAM controller stabilises. */
	ldr r0, =DELAY_200US
	bl delay

	/* Copy next 5 entries */
	ldr r4, =5
loop4:	
	ldmia r3!, {r1, r2}
	str r2, [r1]
	subs r4, r4, #1
	bne loop4

	/* SDRAM initialised so now exit. */
	mov lr, r6
	mov pc, lr

/*
 *  delay()
 *
 *  uses 1 + r0 * 5 cycles
 */
delay:
	nop
	nop
	nop
	subs r0, r0, #1
	bne delay
	mov pc, lr

_platformsetup:
	.word platformsetup

init_table:
	/* Table of address, data for loading into the DRAM controller */
    	/* Configure for a single DDR device */
	.word 0x4500002C, 0x08
	.word 0x45000038, 0x400
	.word 0x45800000, 0x80100000
	.word 0x45800004, 0x8000ffff	// Enable DDR core and all clients
	.word 0x45800024, 0x1e4
	.word 0x45800014, 0xe0000001	// DLL to automatic with starting value=1
/* 200uS delay */
#if !TEST_BRD
	.word 0x45800014, 0xa0000003	// DLL to automatic with offset value=3
/* 200uS delay */
#endif // TEST_BRD
#if (MEM_SIZE == 32)
	.word 0x45800000, 0x801B030C
#else
	.word 0x45800000, 0x801D030C
#endif // MEM_SIZE
/* 200uS delay */
	.word 0x4580000c, 0x80280400
/* 200uS delay */
	.word 0x4580000c, 0x80210000
/* 200uS delay */
	.word 0x4580000c, 0x80200063
	.word 0x45800028, 0x0000001f	// Enable all arbiter features
	.word 0x45800018, 0x00000000	// Disable all monitoring
	.word 0x45800010, 0xffffffff	// Disable all read buffering, due to h/w bug
	.word 0x4580002C, 0x00000000	// Do NOT disable HPROT, ie want write coherency

.ltorg

