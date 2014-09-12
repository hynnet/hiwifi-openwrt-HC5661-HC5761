/*******************************************************************
*
* File:            sata.h
*
* Description:     definitions to support the sata code
*
* Author:          John Larkworthy
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*/

/* SATA-core port register base-addresses */
#define SATA0_REGS_BASE         (SATA_BASE + 0x00000)
#define SATA1_REGS_BASE         (SATA_BASE + 0x10000)
#define DMA_BASE                (SATA_BASE + 0xa0000)
#define SATACORE_REGS_BASE      (SATA_BASE + 0xe0000)

#define ATA_SECTOR_BYTES  512

/* LBA address mode bit in the ORB 1 register */
#define ATA_LBA  (6 + 24)

/* 28-bit LBA read dma */
#define ATA_CMD_READ_DMA 0xc8

/* SATA host port registers */
#define SATA_ORB1_OFF           0x00
#define SATA_ORB2_OFF           0x04
#define SATA_ORB3_OFF           0x08
#define SATA_ORB4_OFF           0x0c
#define SATA_ORB5_OFF           0x10

#define SATA_INT_STATUS_OFF     0x30	/* Read only */
#define SATA_INT_CLR_OFF        0x30	/* Write only */
#define SATA_INT_ENABLE_OFF     0x34	/* Read only */
#define SATA_INT_ENABLE_SET_OFF 0x34	/* Write only */
#define SATA_INT_ENABLE_CLR_OFF 0x38	/* Write only */
#define SATA_VERSION_OFF        0x3c

#define SATA_CONTROL_OFF        0x5c
#define SATA_COMMAND_OFF        0x60

/* The offsets to the link-layer registers that are accessed asynchronously */
#define SATA_LINK_DATA          0x70
#define SATA_LINK_RD_ADDR       0x74
#define SATA_LINK_WR_ADDR       0x78
#define SATA_LINK_CONTROL       0x7c

/* host port interrupt status register fields */
#define SATA_INT_STATUS_EOC_RAW_BIT      (1 + 16)
#define SATA_INT_STATUS_ERROR_RAW_BIT    (2 + 16)

/* ATA status register field definitions */
#define ATA_STATUS_BSY_BIT     7
#define ATA_STATUS_DRDY_BIT    6
#define ATA_STATUS_DF_BIT      5
#define ATA_STATUS_DRQ_BIT     3
#define ATA_STATUS_ERR_BIT     0

/* ATA device (6) register field definitions */
#define ATA_DEVICE_FIXED_MASK 0xA0
#define ATA_DEVICE_DRV_BIT 4
#define ATA_DEVICE_DRV_NUM_BITS 1
#define ATA_DEVICE_LBA_BIT 6

/* ATA control (0) register field definitions */
#define ATA_CTL_SRST_BIT   2

/* ATA Command register initiated commands */
#define ATA_CMD_INIT    0x91
#define ATA_CMD_IDENT   0xEC

/* SATA core command register commands */
#define SATA_CMD_WRITE_TO_ORB_REGS_NO_COMMAND   1
#define SATA_CMD_WRITE_TO_ORB_REGS              2
#define SATA_CMD_READ_ALL_REGS                  3
#define SATA_CMD_READ_STATUS_REG                4

#define SATA_CMD_BUSY_BIT 7

#define SATA_OPCODE_MASK 0x3

#define SATA_LBAL_BIT    0
#define SATA_LBAM_BIT    8
#define SATA_LBAH_BIT    16
#define SATA_DEVICE_BIT  24

#define SATA_NSECT_BIT   0
#define SATA_FEATURE_BIT 16
#define SATA_COMMAND_BIT 24
#define SATA_CTL_BIT     24

#define SATA_SCR_STATUS      0x20
#define SATA_SCR_ERROR       0x24
#define SATA_SCR_CONTROL     0x28

/* SATA core port registers */
#define SATA_DM_DBG1		(SATACORE_REGS_BASE + 0x0)
#define SATA_RAID_SET		(SATACORE_REGS_BASE + 0x4)
#define SATA_DM_DBG2		(SATACORE_REGS_BASE + 0x8)
#define SATA_DATACOUNT_PORT0	(SATACORE_REGS_BASE + 0x10)
#define SATA_DATACOUNT_PORT1	(SATACORE_REGS_BASE + 0x14)
#define SATA_DATAPLANE_CTRL	(SATACORE_REGS_BASE + 0xac)
#define SATA_DATAPLANE_STAT	(SATACORE_REGS_BASE + 0xb8)
#define SATA_DATA_MUX_RAM0	(SATACORE_REGS_BASE + 0x8000)
#define SATA_DATA_MUX_RAM1	(SATACORE_REGS_BASE + 0xA000)

#define SATA_CORE_DPS_IDLE0         (1 << 0) 
#define SATA_CORE_DPS_IDLE1         (1 << 1)

#define SATA_CORE_DPC_EMSK0         (1 << 8) 
#define SATA_CORE_DPC_EMSK1         (1 << 9)
#define SATA_CORE_DPC_EMSK          (SATA_CORE_DPC_EMSK0 | SATA_CORE_DPC_EMSK1) 
/* execute the sata disk loader to obtain an executable image from
 * the first sata disk drive.
 * report the number of sectors read.
 * This should be greater than zero for a valid image. 
 */
u32 ide_read(int device, u32 blknr, u32 blkcnt, u32* buffer);

void init_sata_hw(void);
