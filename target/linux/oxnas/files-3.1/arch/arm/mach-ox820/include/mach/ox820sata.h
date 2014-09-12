/*
 * arch/arm/mach-0x820/include/mach/sata.h
 *
 * Copyright (C) 2009 Oxford Semiconductor Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Definitions for using the SATA core in the ox800
 */

#ifndef __ASM_ARCH_SATA_H__
#define __ASM_ARCH_SATA_H__

/* number of ports per interface */
#define OX820SATA_MAX_PORTS 2

#define SATA0_REGS_BASE     (SATA_REG_BASE + 0x00000)
#define SATA1_REGS_BASE     (SATA_REG_BASE + 0x10000)
#define SATADMA_REGS_BASE   (SATA_REG_BASE + 0xa0000)
#define SATASGDMA_REGS_BASE (SATA_REG_BASE + 0xb0000)
#define SATADPE_REGS_BASE   (SATA_REG_BASE + 0xc0000)
#define SATACORE_REGS_BASE  (SATA_REG_BASE + 0xe0000)
#define SATARAID_REGS_BASE  (SATA_REG_BASE + 0xf0000)

/** sata host port register offsets */
#define OX820SATA_ORB1          (0x00 / sizeof(u32))
#define OX820SATA_ORB2          (0x04 / sizeof(u32))
#define OX820SATA_ORB3          (0x08 / sizeof(u32))
#define OX820SATA_ORB4          (0x0C / sizeof(u32))
#define OX820SATA_ORB5          (0x10 / sizeof(u32))

#define OX820SATA_MASTER_STATUS  (0x10 / sizeof(u32))
#define OX820SATA_FIS_CTRL       (0x18 / sizeof(u32))
#define OX820SATA_FIS_DATA       (0x1C / sizeof(u32))

#define OX820SATA_INT_STATUS     (0x30 / sizeof(u32))
#define OX820SATA_INT_CLEAR      (0x30 / sizeof(u32))
#define OX820SATA_INT_ENABLE     (0x34 / sizeof(u32))
#define OX820SATA_INT_DISABLE    (0x38 / sizeof(u32))
#define OX820SATA_VERSION        (0x3C / sizeof(u32))
#define OX820SATA_SATA_CONTROL   (0x5C / sizeof(u32))
#define OX820SATA_SATA_COMMAND   (0x60 / sizeof(u32))
#define OX820SATA_HID_FEATURES   (0x64 / sizeof(u32))
#define OX820SATA_PORT_CONTROL   (0x68 / sizeof(u32))
#define OX820SATA_DRIVE_CONTROL  (0x6C / sizeof(u32))

/** These registers allow access to the link layer registers
that reside in a different clock domain to the processor bus */
#define OX820SATA_LINK_DATA      (0x70 / sizeof(u32))
#define OX820SATA_LINK_RD_ADDR   (0x74 / sizeof(u32))
#define OX820SATA_LINK_WR_ADDR   (0x78 / sizeof(u32))
#define OX820SATA_LINK_CONTROL   (0x7C / sizeof(u32))

/* window control */
#define OX820SATA_WIN1LO         (0x80 / sizeof(u32))
#define OX820SATA_WIN1HI         (0x84 / sizeof(u32))
#define OX820SATA_WIN2LO         (0x88 / sizeof(u32))
#define OX820SATA_WIN2HI         (0x8C / sizeof(u32))
#define OX820SATA_WIN0_CONTROL   (0x90 / sizeof(u32))

/* Link layer registers */
#define OX820SATA_SERROR_IRQ_MASK (5)


#define OX820SATA_WIN0_OFF_MASK  (0xffff0000)
#define OX820SATA_WIN0_OFF_OFF   (16)
#define OX820SATA_WIN0_CONT_ERR  (1 << 15)
#define OX820SATA_WIN0_CONT_NERR (1 << 14)
#define OX820SATA_WIN0_SW_CPLT   (1 << 13)
#define OX820SATA_WIN0_CMD_CPLT  (1 <<  5)
#define OX820SATA_WIN0_INT_NERR  (1 <<  4)
#define OX820SATA_WIN0_ENCRYPT   (1 <<  3)
#define OX820SATA_WIN0_CLIP_EN   (1 <<  2)
#define OX820SATA_WIN0_RGN_EN    (1 <<  1)
#define OX820SATA_WIN0_GLB_EN    (1 <<  0)

#define OX820SATA_WIN1_CONTROL   (0x94 / sizeof(u32))

#define OX820SATA_WIN1_OFF_MASK  (0xffff0000)
#define OX820SATA_WIN1_OFF_OFF   (16)
#define OX820SATA_WIN1_TWK_MASK  (0x0000ff00)
#define OX820SATA_WIN1_TWK_OFF   (8)
#define OX820SATA_WIN1_INT_NERR  (1 <<  4)
#define OX820SATA_WIN1_ENCRYPT   (1 <<  3)
#define OX820SATA_WIN1_CLIP_EN   (1 <<  2)
#define OX820SATA_WIN1_RGN_EN    (1 <<  1)
#define OX820SATA_WIN1_GLB_EN    (1 <<  0)

#define OX820SATA_WIN2_CONTROL   (0x98 / sizeof(u32))

/** Backup registers contain a copy of the command sent to the disk */
#define OX820SATA_WIN2_OFF_MASK  (0xffff0000)
#define OX820SATA_WIN2_OFF_OFF   (16)
#define OX820SATA_WIN2_GLB_FIX   (1 << 15)
#define OX820SATA_WIN2_GLB_JUMP  (1 << 14)
#define OX820SATA_WIN2_GLB_STATE (1 <<  8)
#define OX820SATA_WIN2_INT_NERR  (1 <<  4)
#define OX820SATA_WIN2_ENCRYPT   (1 <<  3)
#define OX820SATA_WIN2_CLIP_EN   (1 <<  2)
#define OX820SATA_WIN2_RGN_EN    (1 <<  1)
#define OX820SATA_WIN2_GLB_EN    (1 <<  0)

#define OX820SATA_BACKUP1        (0xB0 / sizeof(u32))
#define OX820SATA_BACKUP2        (0xB4 / sizeof(u32))
#define OX820SATA_BACKUP3        (0xB8 / sizeof(u32))
#define OX820SATA_BACKUP4        (0xBC / sizeof(u32))

#define OX820SATA_PORT_SIZE      (0x10000/ sizeof(u32))

/**
 * commands to issue in the master status to tell it to move shadow 
 * registers to the actual device 
 */
#define SATA_OPCODE_MASK                    0x00000007
#define CMD_WRITE_TO_ORB_REGS_NO_COMMAND    0x4
#define CMD_WRITE_TO_ORB_REGS               0x2
#define CMD_SYNC_ESCAPE                     0x7
#define CMD_CORE_BUSY                       (1 << 7)
#define CMD_DRIVE_SELECT_SHIFT              12
#define CMD_DRIVE_SELECT_MASK               (0xf << CMD_DRIVE_SELECT_SHIFT)

#define OX820SATA_SATA_CTL_ERR_MASK   0x00000016

/** interrupt bits */
#define OX820SATA_INT_END_OF_CMD      (1 << 0)
#define OX820SATA_INT_LINK_SERROR     (1 << 1)
#define OX820SATA_INT_ERROR           (1 << 2)
#define OX820SATA_INT_LINK_IRQ        (1 << 3)
#define OX820SATA_INT_REG_ACCESS_ERR  (1 << 7)
#define OX820SATA_INT_BIST_FIS        (1 << 11)
#define OX820SATA_INT_MASKABLE        (OX820SATA_INT_END_OF_CMD     |\
                                       OX820SATA_INT_LINK_SERROR    |\
                                       OX820SATA_INT_ERROR          |\
                                       OX820SATA_INT_LINK_IRQ       |\
                                       OX820SATA_INT_REG_ACCESS_ERR |\
                                       OX820SATA_INT_BIST_FIS       )

#define OX820SATA_INT_WANT            (OX820SATA_INT_END_OF_CMD  |\
                                       OX820SATA_INT_LINK_SERROR |\
                                       OX820SATA_INT_REG_ACCESS_ERR |\
                                       OX820SATA_INT_ERROR       )                                        
#define OX820SATA_INT_ERRORS          (OX820SATA_INT_LINK_SERROR |\
                                       OX820SATA_INT_REG_ACCESS_ERR |\
                                       OX820SATA_INT_ERROR       )                                        
                                       
/** raw interrupt bits, unmaskable, but do not generate interrupts */
#define OX820SATA_RAW_END_OF_CMD      (OX820SATA_INT_END_OF_CMD     << 16)
#define OX820SATA_RAW_LINK_SERROR     (OX820SATA_INT_LINK_SERROR    << 16)
#define OX820SATA_RAW_ERROR           (OX820SATA_INT_ERROR          << 16)
#define OX820SATA_RAW_LINK_IRQ        (OX820SATA_INT_LINK_IRQ       << 16)
#define OX820SATA_RAW_REG_ACCESS_ERR  (OX820SATA_INT_REG_ACCESS_ERR << 16)
#define OX820SATA_RAW_BIST_FIS        (OX820SATA_INT_BIST_FIS       << 16)
#define OX820SATA_RAW_WANT            (OX820SATA_INT_WANT           << 16)
#define OX820SATA_RAW_ERRORS          (OX820SATA_INT_ERRORS         << 16)



/** SATA core register offsets */
#define OX820SATA_DM_DBG1             ( SATACORE_REGS_BASE + 0x000 )
#define OX820SATA_RAID_SET            ( SATACORE_REGS_BASE + 0x004 )
#define OX820SATA_DM_DBG2             ( SATACORE_REGS_BASE + 0x008 )
#define OX820SATA_DATACOUNT_PORT0     ( SATACORE_REGS_BASE + 0x010 )
#define OX820SATA_DATACOUNT_PORT1     ( SATACORE_REGS_BASE + 0x014 )
#define OX820SATA_CORE_INT_STATUS     ( SATACORE_REGS_BASE + 0x030 )
#define OX820SATA_CORE_INT_CLEAR      ( SATACORE_REGS_BASE + 0x030 )
#define OX820SATA_CORE_INT_ENABLE     ( SATACORE_REGS_BASE + 0x034 )
#define OX820SATA_CORE_INT_DISABLE    ( SATACORE_REGS_BASE + 0x038 )
#define OX820SATA_CORE_REBUILD_ENABLE ( SATACORE_REGS_BASE + 0x050 )
#define OX820SATA_CORE_FAILED_PORT_R  ( SATACORE_REGS_BASE + 0x054 )
#define OX820SATA_DEVICE_CONTROL      ( SATACORE_REGS_BASE + 0x068 )
#define OX820SATA_EXCESS              ( SATACORE_REGS_BASE + 0x06C )
#define OX820SATA_RAID_SIZE_LOW       ( SATACORE_REGS_BASE + 0x070 )
#define OX820SATA_RAID_SIZE_HIGH      ( SATACORE_REGS_BASE + 0x074 )
#define OX820SATA_PORT_ERROR_MASK     ( SATACORE_REGS_BASE + 0x078 )
#define OX820SATA_IDLE_STATUS         ( SATACORE_REGS_BASE + 0x07C )
#define OX820SATA_RAID_CONTROL        ( SATACORE_REGS_BASE + 0x090 )
#define OX820SATA_DATA_PLANE_CTRL     ( SATACORE_REGS_BASE + 0x0AC )
#define OX820SATA_CORE_DATAPLANE_STAT ( SATACORE_REGS_BASE + 0x0b8 )
#define OX820SATA_PROC_PC             ( SATACORE_REGS_BASE + 0x100 )
#define OX820SATA_CONFIG_IN           ( SATACORE_REGS_BASE + 0x3d8 )
#define OX820SATA_PROC_START          ( SATACORE_REGS_BASE + 0x3f0 )
#define OX820SATA_PROC_RESET          ( SATACORE_REGS_BASE + 0x3f4 )
#define OX820SATA_UCODE_STORE         ( SATACORE_REGS_BASE + 0x1000)
#define OX820SATA_RAID_WP_BOT_LOW     ( SATACORE_REGS_BASE + 0x1FF0)
#define OX820SATA_RAID_WP_BOT_HIGH    ( SATACORE_REGS_BASE + 0x1FF4)
#define OX820SATA_RAID_WP_TOP_LOW     ( SATACORE_REGS_BASE + 0x1FF8)
#define OX820SATA_RAID_WP_TOP_HIGH    ( SATACORE_REGS_BASE + 0x1FFC)
#define OX820SATA_DATA_MUX_RAM0       ( SATACORE_REGS_BASE + 0x8000)
#define OX820SATA_DATA_MUX_RAM1       ( SATACORE_REGS_BASE + 0xA000)

/* Sata core debug1 register bits */
#define OX820SATA_CORE_PORT0_DATA_DIR_BIT	20
#define OX820SATA_CORE_PORT1_DATA_DIR_BIT	21
#define OX820SATA_CORE_PORT0_DATA_DIR	(1 << OX820SATA_CORE_PORT0_DATA_DIR_BIT)
#define OX820SATA_CORE_PORT1_DATA_DIR	(1 << OX820SATA_CORE_PORT1_DATA_DIR_BIT)

/** sata core control register bits */
#define OX820SATA_SCTL_CLR_ERR        (0x00003016)
#define OX820SATA_RAID_CLR_ERR        (0x0000011e)

/* Interrupts direct from the ports */
#define OX820SATA_NORMAL_INTS_WANTED  (0x00000303)

/* shift these left by port number */
#define OX820SATA_COREINT_HOST        (0x00000001)
#define OX820SATA_COREINT_END         (0x00000100)
#define OX820SATA_CORERAW_HOST        (OX820SATA_COREINT_HOST << 16)
#define OX820SATA_CORERAW_END         (OX820SATA_COREINT_END  << 16)

/* Interrupts from the RAID controller only */
#define OX820SATA_RAID_INTS_WANTED    (0x00008300)

/* The bits in the OX820SATA_IDLE_STATUS that, when set indicate an idle core */
#define OX820SATA_IDLE_CORES          ((1 << 18) | (1 << 19))

/** Device Control register bits */
#define OX820SATA_DEVICE_CONTROL_DMABT (1 << 4)
#define OX820SATA_DEVICE_CONTROL_ABORT (1 << 2)
#define OX820SATA_DEVICE_CONTROL_PAD   (1 << 3)
#define OX820SATA_DEVICE_CONTROL_PADPAT (1 << 16)
#define OX820SATA_DEVICE_CONTROL_PRTRST (1 << 8)
#define OX820SATA_DEVICE_CONTROL_RAMRST (1 << 12)
#define OX820SATA_DEVICE_CONTROL_ATA_ERR_OVERRIDE (1 << 28)

/** ORB4 register bits */
#define OX820SATA_ORB4_SRST	       (1 << 26)

/** oxsemi HW raid modes */
#define OXNASSATA_NOTRAID 0
#define OXNASSATA_RAID0   1
#define OXNASSATA_RAID1   2

#define OXNASSATA_UCODE_JBOD	0
#define OXNASSATA_UCODE_RAID0	1
#define OXNASSATA_UCODE_RAID1	2
#define OXNASSATA_UCODE_NONE	3


/** OX820 specific HW-RAID register values */
#define OX820SATA_RAID_TWODISKS 3
#define OX820SATA_UNKNOWN_MODE ~0

/**
 * variables to write to the device control register to set the current device
 * ie, master or slave
 */
#define OX820SATA_DR_CON_48 2
#define OX820SATA_DR_CON_28 0

/** The different Oxsemi SATA core version numbers */
#define OX820SATA_CORE_VERSION 0x1f3

/* after errors, some micro-code will wait until told to continue, this bit
signals that it should continue */
#define OX820SATA_CONFIG_IN_RESUME  (1 << 1)

/* Data plane control error-mask mask and bit, these bit in the data plane 
control mask out errors from the ports that prevent the SGDMA care from sending 
an interrupt */
#define OX820SATA_DPC_ERROR_MASK      ( 0x00000300 )
#define OX820SATA_DPC_ERROR_MASK_BIT  ( 0x00000100 )

/* enable jbod micro-code */
#define OX820SATA_DPC_JBOD_UCODE      ( 1 << 0 )
#define OX820SATA_DPC_HW_SUPERMUX_AUTO      ( 1 << 0 )
#define OX820SATA_DPC_FIS_SWCH        ( 1 << 1 )

/* Final EOTs */
#define OX820SATA_SGDMA_REQQUAL     (0x00220001)


#define OX820SATA_DMA_CORESIZE      (0x20)
#define OX820SATA_DMA_BASE0         ((u32* )(SATADMA_REGS_BASE + (0 * OX820SATA_DMA_CORESIZE)))
#define OX820SATA_DMA_BASE1         ((u32* )(SATADMA_REGS_BASE + (1 * OX820SATA_DMA_CORESIZE)))
#define OX820SATA_DMA_CONTROL       (0x0 / sizeof(u32))
#define OX820SATA_DMA_CONTROL_RESET (1 << 12)

/* ATA SGDMA request structure offsets */
#define OX820SATA_SGDMAREQ_QUALIFIER (0x0 / sizeof(u32))
#define OX820SATA_SGDMAREQ_CONTROL   (0x4 / sizeof(u32))
#define OX820SATA_SGDMAREQ_SRCPRD    (0x8 / sizeof(u32))
#define OX820SATA_SGDMAREQ_DSTPRD    (0xc / sizeof(u32))

/* ATA SGDMA register offsets */
#define OX820SATA_SGDMA_CORESIZE    (0x10)
#define OX820SATA_SGDMA_BASE0       ((u32* )(SATASGDMA_REGS_BASE + (0 * OX820SATA_SGDMA_CORESIZE)))
#define OX820SATA_SGDMA_BASE1       ((u32* )(SATASGDMA_REGS_BASE + (1 * OX820SATA_SGDMA_CORESIZE)))
#define OX820SATA_SGDMA_CONTROL     (0x0 / sizeof(u32))
#define OX820SATA_SGDMA_STATUS      (0x4 / sizeof(u32))
#define OX820SATA_SGDMA_REQUESTPTR  (0x8 / sizeof(u32))
#define OX820SATA_SGDMA_RESETS      (0xC / sizeof(u32))

/* see DMA core docs for the values. Out means from memory (bus B) out to
disk (bus A) */
//#define OX820SATA_SGDMA_REQCTLOUT   (0x0497a03d)
//#define OX820SATA_SGDMA_REQCTLIN    (0x0497c3c1)

/* see DMA core docs for the values. Out means from memory (bus A) out to 
disk (bus B) */
#define OX820SATA_SGDMA_REQCTL0OUT      (0x0497c03d)
#ifdef CONFIG_SATA_OXNAS_SINGLE_SATA
/* burst mode disabled when no micro code used */
#define OX820SATA_SGDMA_REQCTL0IN       (0x0493a3c1)
#else
#define OX820SATA_SGDMA_REQCTL0IN       (0x0497a3c1)
#endif
#define OX820SATA_SGDMA_REQCTL1OUT      (0x0497c07d)
#define OX820SATA_SGDMA_REQCTL1IN       (0x0497a3c5)
#define OX820SATA_SGDMA_CONTROL_NOGO    (0x3e)
#define OX820SATA_SGDMA_CONTROL_GO      (OX820SATA_SGDMA_CONTROL_NOGO | 1 )
#define OX820SATA_SGDMA_ERRORMASK       (0x3f)
#define OX820SATA_SGDMA_BUSY            (0x80)

#define OX820SATA_SGDMA_RESETS_CTRL   (1 << 0)
#define OX820SATA_SGDMA_RESETS_ARBT   (1 << 1)
#define OX820SATA_SGDMA_RESETS_AHB    (1 << 2)
#define OX820SATA_SGDMA_RESETS_ALL    (OX820SATA_SGDMA_RESETS_CTRL|\
                                       OX820SATA_SGDMA_RESETS_ARBT|\
                                       OX820SATA_SGDMA_RESETS_AHB)

/* Switches between 256 and 128 bit encryption */
#define OX820SATA_256BIT_ENCRYPTION 1

/* Data path encryption registers and bit definitions */
#define OX820SATA_DPE_CONTROL         ( SATADPE_REGS_BASE + 0x00 )
#define OX820SATA_DPE_CTL_ENABLE      (1 <<  0)
#define OX820SATA_DPE_CTL_DIRECTION   (1 <<  1)
#define OX820SATA_DPE_CTL_OTHERKEY    (1 <<  2)
#define OX820SATA_DPE_CTL_SW_ACCESS   (1 <<  3)
#define OX820SATA_DPE_CTL_ENC_KEY     (1 <<  4)
#define OX820SATA_DPE_CTL_SW_DIR      (1 <<  5)
#define OX820SATA_DPE_CTL_ABORT       (1 <<  6)
#define OX820SATA_DPE_CTL_XTS         (1 <<  7)
#define OX820SATA_DPE_CTL_CBC         (2 <<  7)
#define OX820SATA_DPE_CTL_ECB         (3 <<  7)
#define OX820SATA_DPE_CTL_SW_IV       (1 <<  9)
#define OX820SATA_DPE_CTL_SW_ENABLE   (1 << 10)
#define OX820SATA_DPE_CTL_KEY_LEN     (1 << 11)
#define OX820SATA_DPE_CTL_BSWAP       (1 << 12)
#define OX820SATA_DPE_CTL_ESWAP       (1 << 13)
#define OX820SATA_DPE_CTL_SW_PORT     (1 << 14)

/* de/encryption off*/
#define OX820SATA_DPE_PASSTHRU \
    (OX820SATA_DPE_CTL_SW_DIR      |\
     OX820SATA_DPE_CTL_BSWAP       |\
     OX820SATA_DPE_CTL_SW_IV       |\
     OX820SATA_DPE_CTL_SW_ENABLE   |\
     OX820SATA_DPE_CTL_SW_PORT)

/* de/encryption on*/
#define OX820SATA_DPE_CIPHER \
    (OX820SATA_DPE_CTL_ENABLE | OX820SATA_DPE_PASSTHRU)
    

#define OX820SATA_DPE_STATUS          ( SATADPE_REGS_BASE + 0x04 )
#define OX820SATA_DPE_STS_IDLE        (1 <<  0)
#define OX820SATA_DPE_STS_RX_SPACE    (1 <<  2)
#define OX820SATA_DPE_STS_TX_SPACE    (1 <<  3)
#define OX820SATA_DPE_STS_STATE       (1 <<  4)

#define OX820SATA_DPE_PORT            ( SATADPE_REGS_BASE + 0x08 )

#define OX820SATA_DPE_KEYSIZE128 16
#define OX820SATA_DPE_KEYSIZE256 32

#define OX820SATA_DPE_KEY00           ( SATADPE_REGS_BASE + 0x10 )
#define OX820SATA_DPE_KEY01           ( SATADPE_REGS_BASE + 0x14 )
#define OX820SATA_DPE_KEY02           ( SATADPE_REGS_BASE + 0x18 )
#define OX820SATA_DPE_KEY03           ( SATADPE_REGS_BASE + 0x1c )
#define OX820SATA_DPE_KEY04           ( SATADPE_REGS_BASE + 0x20 )
#define OX820SATA_DPE_KEY05           ( SATADPE_REGS_BASE + 0x24 )
#define OX820SATA_DPE_KEY06           ( SATADPE_REGS_BASE + 0x28 )
#define OX820SATA_DPE_KEY07           ( SATADPE_REGS_BASE + 0x2c )

#define OX820SATA_DPE_KEY10           ( SATADPE_REGS_BASE + 0x30 )
#define OX820SATA_DPE_KEY11           ( SATADPE_REGS_BASE + 0x34 )
#define OX820SATA_DPE_KEY12           ( SATADPE_REGS_BASE + 0x38 )
#define OX820SATA_DPE_KEY13           ( SATADPE_REGS_BASE + 0x3c )
#define OX820SATA_DPE_KEY14           ( SATADPE_REGS_BASE + 0x40 )
#define OX820SATA_DPE_KEY15           ( SATADPE_REGS_BASE + 0x44 )
#define OX820SATA_DPE_KEY16           ( SATADPE_REGS_BASE + 0x48 )
#define OX820SATA_DPE_KEY17           ( SATADPE_REGS_BASE + 0x4c )

#define OX820SATA_DPE_KEY20           ( SATADPE_REGS_BASE + 0x50 )
#define OX820SATA_DPE_KEY21           ( SATADPE_REGS_BASE + 0x54 )
#define OX820SATA_DPE_KEY22           ( SATADPE_REGS_BASE + 0x58 )
#define OX820SATA_DPE_KEY23           ( SATADPE_REGS_BASE + 0x5c )
#define OX820SATA_DPE_KEY24           ( SATADPE_REGS_BASE + 0x60 )
#define OX820SATA_DPE_KEY25           ( SATADPE_REGS_BASE + 0x64 )
#define OX820SATA_DPE_KEY26           ( SATADPE_REGS_BASE + 0x68 )
#define OX820SATA_DPE_KEY27           ( SATADPE_REGS_BASE + 0x6c )

/* Initialisation vector */
#define OX820SATA_DPE_IVLBALO         ( SATADPE_REGS_BASE + 0x90 )
#define OX820SATA_DPE_IVLBAHI         ( SATADPE_REGS_BASE + 0x94 )
#define OX820SATA_DPE_IV1             ( SATADPE_REGS_BASE + 0x98 )
#define OX820SATA_DPE_IV0             ( SATADPE_REGS_BASE + 0x9c )

struct request_queue;

extern u32 ox820sata_link_read(u32* core_addr, unsigned int sc_reg);
extern void ox820sata_link_write(u32* core_addr, unsigned int sc_reg, u32 val);
extern int ox820sata_check_link(int port_no);
extern int oxnassata_RAID_faults( void );
extern int oxnassata_get_port_no(struct request_queue* );
extern int oxnassata_LBA_schemes_compatible( void );
extern void ox820sata_set_mode(u32 mode, u32 force);
extern struct ata_port* ox820sata_get_ap(int port_no);
extern void ox820sata_checkforhotplug(int port_no);

extern void ox820sata_freeze_host(int);
extern void ox820sata_thaw_host(int);

typedef enum {
    softreset = 1,
    re_init = 2,
} cleanup_recovery_t;
extern cleanup_recovery_t ox820sata_cleanup(void);

typedef irqreturn_t (*ox820sata_isr_callback_t)(int, unsigned long);

typedef enum {
	SATA_UNLOCKED,
	SATA_WRITER,
	SATA_READER,
	SATA_REBUILD,
	SATA_HWRAID,
	SATA_SCSI_STACK
} sata_locker_t;

extern int acquire_sata_core_hwraid(
    ox820sata_isr_callback_t callback,
    unsigned long            arg,
	void                    *uid);

extern int acquire_sata_core_direct(
	ox820sata_isr_callback_t callback,
	unsigned long            arg,
	int                      timeout_jiffies,
	void                    *uid,
	sata_locker_t            locker_type);

extern void release_sata_core(sata_locker_t locker_type);
extern void release_sata_core_without_restart(sata_locker_t locker_type);
extern int sata_core_has_waiters(void);

#endif  /*  #if !defined(__ASM_ARCH_SATA_H__) */
