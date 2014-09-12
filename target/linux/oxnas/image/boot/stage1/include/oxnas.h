/*******************************************************************
 *
 * File:            oxnas.h
 *
 * Description:     oxnas configuration header file 
 *
 * Date:            27 July 2005
 *
 * Author:          J J Larkworthy
 *
 * Copyright:       Oxford Semiconductor Ltd, 2009
 *
 *
 *******************************************************************/
#ifndef _OXNAS_H_
#define _OXNAS_H_

#define STAGE_1_LOADER

#include "types.h"

/* RPS clocks at 6.25MHz */
#define RPS_CLK         (6250000)

/* Core base addresses */
#define ROM_BASE                0x40000000

#define DRAM_CTRL_BASE          0x44700000

#define UART_1_BASE             0x44200000
#define UART_2_BASE             0x44300000

#define GPIO_A_BASE             0x44000000
#define GPIO_B_BASE             0x44100000
#define SATA_PHY_ASIC_STAT  (0x44900000)
#define SATA_PHY_ASIC_DATA  (0x44900004)

#define SYS_CONTROL_BASE        0x44e00000
#define SEC_CONTROL_BASE        0x44f00000
#define CLOCK_CONTROL_BASE      (APB_BRIDGE_B_BASE + 0x100000)
#define RPSA_BASE               0x44400000
#define RPSB_BASE               0x44500000
#define SD_BASE                (0x45400000)
#define SATA_BASE               0x45900000

/* this isn't actually used per-say, the DMA controller just needs an address */
#define SATA_DATA_BASE_PA       0x42000000
#define DMA_SG_BASE             0x459b0000
#define COPRO_REGS_BASE         0x45b00000

/* System Control registers @todo check these for 820 compliance */
#define SYS_CTRL_USB11_CTRL             (SYS_CONTROL_BASE + 0x00)
#define SYS_CTRL_PCI_CTRL0              (SYS_CONTROL_BASE + 0x04)
#define SYS_CTRL_PCI_CTRL1              (SYS_CONTROL_BASE + 0x08)

#define SYS_CTRL_USB11_STAT             (SYS_CONTROL_BASE + 0x1c)
#define SYS_CTRL_PCI_STAT               (SYS_CONTROL_BASE + 0x20)
#define SYS_CTRL_CKEN_CTRL              (SYS_CONTROL_BASE + 0x24)
#define SYS_CTRL_RSTEN_CTRL             (SYS_CONTROL_BASE + 0x28)
#define SYS_CTRL_CKEN_SET_CTRL          (SYS_CONTROL_BASE + 0x2C)
#define SYS_CTRL_CKEN_CLR_CTRL          (SYS_CONTROL_BASE + 0x30)
#define SYS_CTRL_RSTEN_SET_CTRL         (SYS_CONTROL_BASE + 0x34)
#define SYS_CTRL_RSTEN_CLR_CTRL         (SYS_CONTROL_BASE + 0x38)
#define SYS_CTRL_USBHSMPH_CTRL          (SYS_CONTROL_BASE + 0x40)
#define SYS_CTRL_USBHSMPH_STAT          (SYS_CONTROL_BASE + 0x44)
#define SYS_CTRL_PLLSYS_CTRL            (SYS_CONTROL_BASE + 0x48)
#define SYS_CTRL_SEMA_STAT              (SYS_CONTROL_BASE + 0x4C)
#define SYS_CTRL_SEMA_SET_CTRL          (SYS_CONTROL_BASE + 0x50)
#define SYS_CTRL_SEMA_CLR_CTRL          (SYS_CONTROL_BASE + 0x54)
#define SYS_CTRL_SEMA_MASKA_CTRL        (SYS_CONTROL_BASE + 0x58)
#define SYS_CTRL_SEMA_MASKB_CTRL        (SYS_CONTROL_BASE + 0x5C)
#define SYS_CTRL_SEMA_MASKC_CTRL        (SYS_CONTROL_BASE + 0x60)
#define SYS_CTRL_CKCTRL_CTRL            (SYS_CONTROL_BASE + 0x64)
#define SYS_CTRL_COPRO_CTRL             (SYS_CONTROL_BASE + 0x68)
#define SYS_CTRL_PLLSYS_KEY_CTRL        (SYS_CONTROL_BASE + 0x6C)

/* System control multi-function pin function selection */
#define SYS_CTRL_SECONDARY_SEL        (SYS_CONTROL_BASE + 0x14)
#define SYS_CTRL_TERTIARY_SEL         (SYS_CONTROL_BASE + 0x8c)
#define SYS_CTRL_DEBUG_SEL            (SYS_CONTROL_BASE + 0x9c)
#define SYS_CTRL_ALTERNATIVE_SEL      (SYS_CONTROL_BASE + 0xa4)
//Michael add C_SYSCTRL_DDRAMIO_CTRL_ADDR
#define SYS_CTRL_DDRAMIO_CTRL_ADDR    (SYS_CONTROL_BASE + 0x134)

/* Secure control multi-function pin function selection */
#define SEC_CTRL_SECONDARY_SEL        (SEC_CONTROL_BASE + 0x18)
#define SEC_CTRL_TERTIARY_SEL         (SEC_CONTROL_BASE + 0x90)
#define SEC_CTRL_DEBUG_SEL            (SEC_CONTROL_BASE + 0xa0)

/* clock control bits */
#define SYS_CTRL_CKEN_DMA_BIT       1

#define SYS_CTRL_CKEN_SD_BIT        3
#define SYS_CTRL_CKEN_SATA_BIT      4

#define SYS_CTRL_CKEN_MAC_BIT       7
#define SYS_CTRL_CKEN_PCI_BIT       8
#define SYS_CTRL_CKEN_STATIC_BIT    9

#define SYS_CTRL_CKEN_DDR_PHY_BIT   14
#define SYS_CTRL_CKEN_DDR_BIT       15
#define SYS_CTRL_CKEN_DDR_CK        16

/* reset control bits */
#define SYS_CTRL_RSTEN_ARM_BIT      0
#define SYS_CTRL_RSTEN_MAC_BIT      6
#define SYS_CTRL_RSTEN_DDR_BIT      10
#define SYS_CTRL_RSTEN_SATA_BIT     11
#define SYS_CTRL_RSTEN_SATA_LK_BIT  12
#define SYS_CTRL_RSTEN_SATA_PHY_BIT 13
#define SYS_CTRL_RSTEN_STATIC_BIT   15
#define SYS_CTRL_RSTEN_UARTA_BIT    17
#define SYS_CTRL_RSTEN_UARTB_BIT    18
#define SYS_CTRL_RSTEN_SD_BIT       21
#define SYS_CTRL_RSTEN_DDRPHY_BIT   25
#define SYS_CTRL_RSTEN_PLLA_BIT     30


/* UART_A multi function pins */
#define UART_A_SIN_MF_PIN              30
#define UART_A_SOUT_MF_PIN             31

/* UART_B multi function pins */
#define UART_B_SIN_MF_PIN              7
#define UART_B_SOUT_MF_PIN             8

/* GPIO */
#define GPIO_A_DATA                   ((GPIO_1_BASE) + 0x0)
#define GPIO_A_OE_VAL                 ((GPIO_1_BASE) + 0x4)
#define GPIO_A_SET_OE                 ((GPIO_1_BASE) + 0x1C)
#define GPIO_A_CLR_OE                 ((GPIO_1_BASE) + 0x20)
#define GPIO_A_INPUT_DEBOUNCE_ENABLE  ((GPIO_1_BASE) + 0x20)

#define GPIO_B_DATA                   ((GPIO_2_BASE) + 0x0)
#define GPIO_B_OE_VAL                 ((GPIO_2_BASE) + 0x4)
#define GPIO_B_SET_OE                 ((GPIO_2_BASE) + 0x1C)
#define GPIO_B_CLR_OE                 ((GPIO_2_BASE) + 0x20)
#define GPIO_B_INPUT_DEBOUNCE_ENABLE  ((GPIO_2_BASE) + 0x20)

#define START_OF_SRAM     0x4C000000
/* define the maximum usable static RAM space */
#define MAX_BUFFER       (0x8000 - 1024)

#define SLOW_TIMER_TICK                (0X100)

/* clock control registers and data */
#define RPS_BASE                      RPSA_BASE
#define RPS_CLK_BASE                  (RPS_BASE +0x200)
#define RPS_CLK_CTRL_DATA             (0x40)	/* periodic timer, no prescale) */
#define CLK_ENABLE                    (1<<7) /* clock run enable bit */
#define RPSA_CLK_CTRL                 (RPS_CLK_BASE+0x08)
#define RPSA_CLK_COUNT                (RPS_CLK_BASE+0x04)
#define RPSA_CLK_LOAD                 (RPS_CLK_BASE)
#define RPSA_CLK_CLEAR                (RPS_CLK_BASE + 0x0C)
#define RPSA_INT_STATUS               (RPS_BASE+0x04)
#define RPSA_CLK_INT                  (1<<4)
#define RPSA_TIMER_WIDTH              24
#define RPSA_TIMER_MODULUS            (1<<RPSA_TIMER_WIDTH)

/* RPS clock is 6.25 MHz */
#if (RPS_CLK != 6250000)
#error The macro USECS2TICKS needs re-calculating for the change in RPS clock speed
#endif

/* This is the formula USECS2TICKS(a) = a * RPS_CLK / 1000000  carefully
* worked out to avoid introducing divide code. */
#define USECS2TICKS(A)             ((A * 25)/ 4 )

#ifdef SDK_BUILD_SPI_BOOT
/* Image locations (sector number) */
#define NOR_BOOT_STAGE2      0x08000

#define NOR_RECOVERY_STAGE2  0x48000
#else
/* Image locations (sector number) */
#define SECTOR_BOOT_STAGE2      154
#define SECTOR_RECOVERY_STAGE2  57208
#endif
/* macros to make reading and writing hardware easier */
#define readb(p)     (*((volatile u8  *)(p)))
#define readl(p)     (*((volatile u32 *)(p)))
#define writeb(v, p) (*((volatile u8  *)(p))= (v))
#define writel(v, p) (*((volatile u32 *)(p))=(v))

#define barrier()   __asm__ __volatile__("": : :"memory")

void udelay(u32 time);

#endif				/* _OXNAS_H_ */
