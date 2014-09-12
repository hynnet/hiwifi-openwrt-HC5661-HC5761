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
 
#define DELAY_200US   ((NOMINAL_ARMCLK  / (5 * 1000000)) * 200)  
#define DELAY_300US   ((NOMINAL_ARMCLK  / (5 * 1000000)) * 300)  
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
    adrl r0, platformsetup      /* Relative location of function start.*/
    ldr  r1, _platformsetup
    cmp  r0, r1
    moveq pc, lr
#else
    mov pc, lr
#endif

#if (FPGA == 1)
    /* Establish a working setup for the SDRAM */
    mov r6, lr

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

    /* Copy first 6 entries */
    ldr r4, =6
loop0:
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne loop0

    /* Delay for 200uS while DRAM controller stabilises. */
    ldr r0, =DELAY_200US
    bl delay

    /* Copy next 4 entries */
    ldr r4, =4
loop1:  
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne loop1

    /* Wait at least 200 clock cycles. */
    ldr r0, =DELAY_200
    bl delay

    /* Copy next 2 entries */
    ldr r4, =2
loop2:  
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne loop2

    /* Wait at least 8 clock  cycles. */
    ldr r0, =DELAY_8
    bl delay

    /* Copy next 9 entries */
    ldr r4, =9
loop3:  
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne loop3

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
#else // ASIC, (DDR-2)
/*
 * Check that not in SDRAM execution. Suicide if re-initialise DRAM.
 * Controller function is linked to execute in SDRAM must be in ROM if not
 * there. Check for wrong place.
 */
    /* Establish a working setup for the SDRAM */
    mov r6, lr

#ifdef OVERCLOCK
    /* 
      change clock speed on chip 
    */

    /* read SYS_CTRL_PLLSYS_CTRL into r3*/
    mov r5, #0x45000000
    ldr r3, [r5, #72]

    /* load the value at dllkey (0xbeadface) into r7 */
    adrl r7, dllkey
    ldr r7, [r7]

    /* pll_sys |= 0x20000; */
    orr r3, r3, #131072 /* 0x20000 */

    /* write 0xbeadface into SYS_CTRL_PLLSYS_KEY_CTRL */
    str r7, [r5, #108]

    /* write pll_sys (bypass pll)*/
    str r3, [r5, #72]

    /* pll_sys mask out lower 26 bits */
    mov r4, r3, lsr #26
    mov r4, r4, lsl #26

    /* pll_sys |= (26 bits of (m,p and s)) */
    ldr r3, =OVERCLOCK
    orr r4, r4, r3

    /* write 0xbeadface into SYS_CTRL_PLLSYS_KEY_CTRL */
    str r7, [r5, #108]

    /* write pll_sys (with new pll speeds) */    
    str r4, [r5, #72]

    /* delay 300us */
    ldr r0, =DELAY_300US
    bl delay

    /* clear bypass pll bit */
    bic r4, r4, #131072 /* 0x20000 */

    /* write 0xbeadface into SYS_CTRL_PLLSYS_KEY_CTRL */
    str r7, [r5, #108]

    /* write pll_sys (with new pll speeds and pll un-bypassed) */    
    str r4, [r5, #72]
#endif /* OVERCLOCK */
  
    /* Turn on the DDR core and phy clocks */
    ldr r0, =SYS_CTRL_CKEN_SET_CTRL
    ldr r1, =1
    ldr r2, =SYS_CTRL_CKEN_DDR_BIT
    mov r1, r1, LSL r2
    str r1, [r0]
    ldr r1, =1
    ldr r2, =SYS_CTRL_CKEN_DDR_PHY_BIT
    mov r1, r1, LSL r2
    str r1, [r0]

    /* Assert reset to the DDR core and phy */
    ldr r0, =SYS_CTRL_RSTEN_SET_CTRL
    ldr r1, =1
    ldr r2, =SYS_CTRL_RSTEN_DDR_PHY_BIT
    mov r1, r1, LSL r2
    str r1, [r0]
    ldr r1, =1
    ldr r2, =SYS_CTRL_RSTEN_DDR_BIT
    mov r1, r1, LSL r2
    str r1, [r0]

    /* Deassert reset to the DDR core and phy*/
    ldr r0, =SYS_CTRL_RSTEN_CLR_CTRL
    ldr r1, =1
    ldr r2, =SYS_CTRL_RSTEN_DDR_PHY_BIT
    mov r1, r1, LSL r2
    str r1, [r0]
    ldr r1, =1
    ldr r2, =SYS_CTRL_RSTEN_DDR_BIT
    mov r1, r1, LSL r2
    str r1, [r0]

    /* Start using the initialisation value list */
    adrl r3, init_table

    /* Copy first 14 entries of DDR core setup (section A)*/
    ldr r4, =14
loop0:
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne loop0

    /* Delay for 200uS while DDR controller stabilises. */
    ldr r0, =DELAY_200US
    bl delay

    /* Copy next 13 entries of DDR device commands (section B)*/
    ldr r4, =13
loop1:
    ldmia r3!, {r1, r2}
    str r2, [r1]

    /* Wait at least 200 clock cycles between ram chip command writes */
    ldr r0, =DELAY_200
    bl delay

    subs r4, r4, #1
    bne loop1

    /* Copy final DDR controller setup to set memory size/banks (section C)*/
    ldmia r3!, {r1, r2}
    str r2, [r1]

#if (PROBE_MEM_SIZE == 1)
    /* Load the probe values into SDRAM */
    adrl r3, probe_table
    mov r4, #4
.globl pl1
pl1:
    ldmia r3!, {r1, r2}
    str r2, [r1]
    subs r4, r4, #1
    bne pl1

    /* Get the current contents of the DDR controller core's config register */
    adrl r1, ddr_config_reg
    ldr r1, [r1]
    ldr r1, [r1]

    /* Zero the number of banks field - bit 23*/
    mov r2, #1
    bic r1, r1, r2, lsl #23

    /* Zero the size field - bits 17-20 inclusive */
    mov r2, #15
    bic r1, r1, r2, lsl #17

    /* First probe location tells us the SDRAM size */
    adrl r3, probe_table
    ldr r0, [r3]
    ldr r0, [r0]

    /* Is size 64MB? */
    ldr r2, [r3, #28]	/* Get probe value 4 */
    cmp r0, r2
    moveq r4, #6
    orreq r1, r1, r4, lsl #17
    beq pl2

    /* Is 128M or 256M so set banks to 8 */
    mov r4, #1
    orr r1, r1, r4, lsl #23

    /* Is size 128MB? */
    ldr r2, [r3, #20]	/* Get probe value 3 */
    cmp r0, r2
//    moveq r4, #7
    moveq r4, #8	/* DDR controller does not work at 128M, use 256M instead
    orreq r1, r1, r4, lsl #17
    beq pl2

    /* Must be 256MB, or something is very wrong */
    mov r4, #8
    orr r1, r1, r4, lsl #17

pl2:
    /* Write the revised contents to the DDR controller core's config register */
    adrl r2, ddr_config_reg
    ldr r2, [r2]
    str r1, [r2]
#endif

    /* SDRAM setup complete */
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
#endif


init_table:
#if (FPGA == 1)
    /* Table of address, data for loading into the DRAM controller on FPGA */
    .word 0x45800000, 0x000d0000    // Enable the DDR in SDR mode and width 32 bits
    .word 0x45800034, 0x04442032    // SDR mode timings - #0
    .word 0x45800038, 0x570A0907    // SDR mode timings - #1
    .word 0x4580003C, 0x00000002    // SDR mode timings - #2
    .word 0x45800004, 0x80000000    // Enable DDR core, but not clients yet
    .word 0x45800014, 0x80000001    // Enable CK and set DLL mode to manual
/* 200uS delay */
    .word 0x4580000c, 0x80200000    // Assert CKE for all further commands
    .word 0x4580000c, 0x80280400    // Issue precharge to all banks
    .word 0x4580000c, 0x80200000    // NOP, as only DDR has real command here
    .word 0x4580000c, 0x80200022    // Set burst length 4, sequential CAS 2
/* 200uS delay */
    .word 0x4580000c, 0x80280400    // Issue precharge to all banks
    .word 0x4580000c, 0x80240000    // Issue auto-refresh command, CKE not asserted
/* 200uS delay */
    .word 0x4580000c, 0x80240000    // Issue auto-refresh command, CKE not asserted
    .word 0x4580000c, 0x80200000    // Assert CKE for all further commands
    .word 0x4580000c, 0x80200022        // Set burst length 4, sequential CAS 2
    .word 0x45800000, 0x000d0186    // SDR, size and width and refresh rate, assuming
                        // 25Mhz clk to SDR, divide down to get 15.625uS
                    // refresh rate
    .word 0x45800024, 0x00000124    // Set I/O drive strengths
    .word 0x45800028, 0x0000001f    // Enable all arbiter features
    .word 0x45800018, 0x00000000    // Disable all monitoring
    .word 0x45800010, 0xFFFFFFFF    // Disable all read buffering
    .word 0x45800004, 0x800000ff    // Enable all client interfaces
#else   // ASIC DDR-2
    // SECTION A - DDR controller core configuration
    .word 0x45800000, 0x802d0591    // enable in ddr-2 mode 16 bit wide
    .word 0x45800034, 0x04442032    // ddr-2 mode timings 
    .word 0x45800038, 0x870f0b25    // ddr-2 mode timings
    .word 0x4580003c, 0x00000a23    // ddr-2 mode timings
    .word 0x45800054, 0x00072000    // phy-3 settings
    .word 0x45800050, 0x00022828    // phy-2 settings, start
    .word 0x45800050, 0x00032828    // phy-2 settings, on
    .word 0x45800028, 0x0000001f    // Enable all arbiter features
    .word 0x45800018, 0x00000000    // Disable all monitoring
    .word 0x45800010, 0xffff0000    // Enable all read buffering
    .word 0x4580002c, 0x00ff00fd    // no burst accl, no hprot on arm data
    .word 0x45800040, 0x00000000    // enable burst and read cache
    .word 0x45800044, 0xffff0000    // enable write behind prot, disable timeout     
    .word 0x45800004, 0x8000ffff    // Enable all client interfaces
/* 200uS delay after configuring DDR controller core */

    // SECTION B - Memory device configuration
    .word 0x4580000c, 0x807c0000    // exit something or other
    .word 0x4580000c, 0x803c0000    // nop - wake up
    .word 0x4580000c, 0x80280400    // precharge all
    .word 0x4580000c, 0x80220000    // emr2
    .word 0x4580000c, 0x80230000    // emr3

#if (MEM_ODT == 150)
    .word 0x4580000c, 0x80210042    // enable dll, odt to 150
#elif (MEM_ODT == 75)
    .word 0x4580000c, 0x80210006    // enable dll, odt to 75
#elif (MEM_ODT == 50)
    .word 0x4580000c, 0x80210046    // enable dll, odt to 50
#else
#error Unsupported memory on-die termination, set MEM_ODT to 50, 75, or 150
#endif

    .word 0x4580000c, 0x80200733    // set WR CL BL and reset dll
    .word 0x4580000c, 0x80280400    // precharge all
    .word 0x4580000c, 0x80240000    // auto refresh
    .word 0x4580000c, 0x80240000    // auto refresh
    .word 0x4580000c, 0x80200733    // set WR CL BL and reset dll

#if (MEM_ODT == 150)
    .word 0x4580000c, 0x802103c2    // enable OCD
    .word 0x4580000c, 0x80210042    // disable OCD
#elif (MEM_ODT == 75)
    .word 0x4580000c, 0x80210386    // enable OCD
    .word 0x4580000c, 0x80210006    // disable OCD
#elif (MEM_ODT == 50)
    .word 0x4580000c, 0x802103c6    // enable OCD
    .word 0x4580000c, 0x80210046    // disable OCD
#else
#error Unsupported memory on-die termination, set MEM_ODT to 50, 75, or 150
#endif

    // SECTION C - Final memory size/bank configuration
#if (PROBE_MEM_SIZE == 1)
    .word 0x45800000, 0x80b10591    // 256M, 8 banks, 1425 clocks for 7.8us refresh.
#elif (MEM_SIZE == 64)
    .word 0x45800000, 0x802d0591    // 64M,  4 banks, 1425 clocks for 7.8us refresh.
#elif (MEM_SIZE == 128)
    .word 0x45800000, 0x80af0591    // 128M, 8 banks, 1425 clocks for 7.8us refresh.
#elif (MEM_SIZE == 256)
    .word 0x45800000, 0x80b10591    // 256M, 8 banks, 1425 clocks for 7.8us refresh.
#else
#error Unsupported memory size, set MEM_SIZE to 64, 128 or 256
#endif

#endif  // FPGA or ASIC
dllkey:
    .word 0xbeadface

ddr_config_reg:
    .word 0x45800000

probe_table:
    .word 0x48000000, 0x12345678
    .word 0x48000040, 0xdeadbeef
    .word 0x50000000, 0xfafafafa
    .word 0x50000040, 0xabcdef01

.ltorg

