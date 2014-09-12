/*
 *  linux/drivers/mtd/nand/ox820_nand.c
 *
 *  Copyright (C) 2008 Oxford Semiconductor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   OX820 demo board.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include "../mtdcore.h"

#define	OX820_NAND_NAME	"OX820_NAND"
#define	OX820_NAND_BASE	STATIC_CS0_BASE	// base address of NAND chip on static bus
#define	OX820_NAND_DATA				OX820_NAND_BASE + 0x0000
#define	OX820_NAND_ADDRESS_LATCH	OX820_NAND_BASE + (1<<18)
#define	OX820_NAND_COMMAND_LATCH	OX820_NAND_BASE + (1<<19)

// commands
#define	OX820_NAND_COMMAND_READ_CYCLE1	0x00
#define	OX820_NAND_COMMAND_WRITE_CYCLE2	0x10
#define	OX820_NAND_COMMAND_READ_CYCLE2	0x30
#define	OX820_NAND_COMMAND_CACHE_READ	0x31
#define	OX820_NAND_COMMAND_BLOCK_ERASE	0x60
#define	OX820_NAND_COMMAND_READ_STATUS	0x70
#define	OX820_NAND_COMMAND_READ_ID		0x90
#define	OX820_NAND_COMMAND_STATUS		0x70
#define	OX820_NAND_COMMAND_WRITE_CYCLE1	0x80
#define	OX820_NAND_COMMAND_ERASE_CONFIRM	0xd0
#define	OX820_NAND_COMMAND_PARAMETER_PAGE	0xec
#define	OX820_NAND_COMMAND_RESET		0xff

// status register bits
#define	OX820_NAND_STATUS_FAIL			(1 << 0)
#define	OX820_NAND_STATUS_READY			(1 << 6)

extern spinlock_t oxnas_gpio_spinlock;

DECLARE_WAIT_QUEUE_HEAD(wq);

#define	NUM_PARTITIONS	7
static struct mtd_partition partition_info[] =
{
	{
		.name = "stage1",
		.offset = 0,
		.size = 1024 * 128 * 2		/* 2 blocks; 2 stage1 */
	},
	{
		.name = "uboot",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 1024 * 1024 * 3.5	/* 3.5 MB */
	},
	{
		.name = "uboot_env",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 1024 * 128 * 4		/* 4 blocks */
	},
	{
		.name = "kernel",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 1024 * 1024 * 10	/* 10 MB */
	},
	{
		.name = "etc",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 1024 * 1024 * 10	/* 10 MB */
	},
	{
		.name = "info",
		.offset = MTDPART_OFS_NXTBLK,
		.size = 1024 * 1024 * 10	/* 10 MB */
	},
	{
		.name = "sysdisk",
		.offset = MTDPART_OFS_NXTBLK,
		.size = MTDPART_SIZ_FULL	/* Should be 93.75 MB */
	}
};

static struct priv {
	struct mtd_info *mtd;
} priv;

static void ox820_nand_write_command(u_int8_t command)
{
	writeb(command, OX820_NAND_COMMAND_LATCH);
}

static u_int8_t ox820_nand_read_data(void)
{
	return readb(OX820_NAND_DATA);
}

static uint8_t ox820_nand_wait_for_ready(void)
{
	int timeout = 100;
	uint8_t status;

	ox820_nand_write_command(OX820_NAND_COMMAND_STATUS);
	status = ox820_nand_read_data();
	if (status & OX820_NAND_STATUS_READY)
		return status;

	udelay(100);

	while (timeout--) {
		status = ox820_nand_read_data();
		if (status & OX820_NAND_STATUS_READY)
			return status;
		msleep(1);
	}
	printk(KERN_ERR "OX820 NAND Timeout waiting for ready\n");

	return OX820_NAND_STATUS_FAIL;
}

static void ox820_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *this = (struct nand_chip *)priv.mtd->priv;
	unsigned long bits = 0;
	char *addr = this->IO_ADDR_W;

	if (ctrl & NAND_CLE)
		bits |= (OX820_NAND_COMMAND_LATCH - OX820_NAND_BASE);
	if (ctrl & NAND_ALE)
		bits |= (OX820_NAND_ADDRESS_LATCH - OX820_NAND_BASE);

	if (likely(cmd != NAND_CMD_NONE))
		writeb(cmd, addr + bits);
}

#define SYSCTRL_MFA_SECSEL_CTRL        ((void *)(SYS_CONTROL_BASE+ 0x14))
#define STATIC_NAND_ENABLE0  0x01fff000

static int ox820_nand_init(void)
{
	int err,i ;
	struct nand_chip *this;

	priv.mtd = kzalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!priv.mtd)
		return -ENOMEM;

	this = (struct nand_chip *)((char *)(priv.mtd) + sizeof(struct mtd_info));
	priv.mtd->priv = this;
	priv.mtd->owner = THIS_MODULE;
	this->IO_ADDR_R = (void __iomem *)OX820_NAND_DATA;
	this->IO_ADDR_W = (void __iomem *)OX820_NAND_DATA;
	this->cmd_ctrl = ox820_nand_hwcontrol;
	this->dev_ready = NULL;
	this->ecc.mode = NAND_ECC_SOFT;

	spin_lock(&oxnas_gpio_spinlock);
        /*enable static bus to device_select */
	/* enable secondary functions for: gpioa12 .. gpioa 19 (data bus),
	 *          * gpioa20(we) gpioa21(oe), gpioa 22(cs 0), gpiob 0 ..4 (address a0-a4)
	 *
	 * */
	writel(readl(SYSCTRL_MFA_SECSEL_CTRL) | STATIC_NAND_ENABLE0, SYSCTRL_MFA_SECSEL_CTRL);

	/* enable clock and release static block reset */
	writel(1 << SYS_CTRL_CKEN_STATIC_BIT, SYS_CTRL_CKEN_SET_CTRL);
	writel(1 << SYS_CTRL_RSTEN_STATIC_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
	spin_unlock(&oxnas_gpio_spinlock);

	// reset
	ox820_nand_write_command(OX820_NAND_COMMAND_RESET);
	ox820_nand_wait_for_ready();
	ox820_nand_write_command(OX820_NAND_COMMAND_PARAMETER_PAGE);
	ox820_nand_wait_for_ready();
	ox820_nand_write_command(OX820_NAND_COMMAND_READ_CYCLE1);
	for (i = 0; i < 137; i++) { // skip to max page read time parameter
		ox820_nand_read_data();
	}
	this->chip_delay = (ox820_nand_read_data() + 256 * ox820_nand_read_data()) / 1000;
#ifdef	CONFIG_MTD_DEBUG
	printk("Page read time %dms\n", this->chip_delay);
#endif
	if (nand_scan(priv.mtd, 1)) {
		err = -ENXIO;
		goto error;
	}

	err = add_mtd_device(priv.mtd);
	if (err) {
		err = -ENFILE;
		goto error;
	}

	add_mtd_partitions(priv.mtd, partition_info, NUM_PARTITIONS);

	return 0;
error:
	kfree(priv.mtd);
	return err;
}

static void ox820_nand_exit(void)
{
	if (priv.mtd) {
		del_mtd_device(priv.mtd);
		nand_release(priv.mtd);
		kfree(priv.mtd);
	}
}

module_init(ox820_nand_init);
module_exit(ox820_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oxford Semiconductor");
MODULE_DESCRIPTION("NAND flash driver");
