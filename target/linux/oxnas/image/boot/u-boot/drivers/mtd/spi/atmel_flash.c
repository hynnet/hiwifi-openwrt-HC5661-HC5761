/*
 * (C) Copyright 2010
 * John Larkworthy, PLX Technology Inc jlarkworthy@plxtech.com.
 *
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Copyright 2008, Network Appliance Inc.
 * Jason McMullan <mcmullan@netapp.com>
 *
 * Copyright (C) 2004-2007 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#define DEBUG
#include <common.h>
#ifdef CONFIG_SPI_FLASH_ATMEL

#include <malloc.h>
#include <spi_flash.h>

#include "spi_flash_internal.h"

/* M25Pxx-specific commands */
#define CMD_AT25FSXX_WREN		0x06	/* Write Enable */
#define CMD_AT25FSXX_WRDI		0x04	/* Write Disable */
#define CMD_AT25FSXX_RDSR		0x05	/* Read Status Register */
#define CMD_AT25FSXX_WRSR		0x01	/* Write Status Register */
#define CMD_AT25FSXX_READ		0x03	/* Read Data Bytes */
#define CMD_AT25FSXX_FAST_READ	0x0b	/* Read Data Bytes at Higher Speed */
#define CMD_AT25FSXX_BP		0x02	/* Byte Program */
#define CMD_AT25FSXX_SE		0x20	/* Sector Erase */
#define CMD_AT25FSXX_BE		0x52	/* Block Erase */
#define CMD_AT25FSXX_CE		0x60	/* Chip Erase */
//#define CMD_AT25FSXX_DP		0xb9	/* Deep Power-down */
//#define CMD_AT25FSXX_RES		0xab	/* Release from DP, and Read Signature */

//#define SST_ID_SST25VF16		0x15
//#define SST_ID_SST25VF20		0x12
//#define SST_ID_SST25VF32		0x16
#define ATMEL_ID_AT25FS010		0x66
//#define SST_ID_SST25VF64		0x17
//#define SST_ID_SST25VF80		0x14
//#define SST_ID_SST25VF128		0x18

#define STMICRO_SR_WIP		(1 << 0)	/* Write-in-Progress */

struct atm_spi_flash_params {
	u8 idcode1;
	u16 page_size;
	u16 pages_per_sector;
	u16 sectors_per_block;
	u16 nr_blocks;
	const char *name;
};

/* spi_flash needs to be first so upper layers can free() it */
struct atm_spi_flash {
	struct spi_flash flash;
	const struct atm_spi_flash_params *params;
};

static inline struct atm_spi_flash *to_atm_spi_flash(struct spi_flash
							     *flash)
{
	return container_of(flash, struct atm_spi_flash, flash);
}

static const struct atm_spi_flash_params atm_spi_flash_table[] = {
	{
		.idcode1 = ATMEL_ID_AT25FS010,
		.page_size = 256,
		.pages_per_sector = 256,
  		.sectors_per_block = 4096,
		.nr_blocks = 4,
		.name = "AT25FS010",
	},
};

static int atm_wait_ready(struct spi_flash *flash, unsigned long timeout)
{
	struct spi_slave *spi = flash->spi;
	unsigned long timebase;
	int ret;
	u8 status;
	u8 cmd[4] = { CMD_AT25FSXX_RDSR, 0xff, 0xff, 0xff };

	ret = spi_xfer(spi, 32, &cmd[0], NULL, SPI_XFER_BEGIN);
	if (ret) {
		debug("SF: Failed to send command %02x: %d\n", cmd[0], ret);
		return ret;
	}

	timebase = get_timer(0);
	do {
		ret = spi_xfer(spi, 8, NULL, &status, 0);
		if (ret)
			return -1;

		if ((status & STMICRO_SR_WIP) == 0)
			break;

	} while (get_timer(timebase) < timeout);

	spi_xfer(spi, 0, NULL, NULL, SPI_XFER_END);

	if ((status & STMICRO_SR_WIP) == 0)
		return 0;

	/* Timed out */
	return -1;
}

static int atm_read_fast(struct spi_flash *flash,
			     u32 offset, size_t len, void *buf)
{
	struct atm_spi_flash *atm = to_atm_spi_flash(flash);
	unsigned long page_addr;
	unsigned long page_size;
	u8 cmd[5];

	page_size = atm->params->page_size;
	page_addr = offset / page_size;

	cmd[0] = CMD_READ_ARRAY_FAST;
	cmd[1] = page_addr >> 8;
	cmd[2] = page_addr;
	cmd[3] = offset % page_size;
	cmd[4] = 0x00;

	return spi_flash_read_common(flash, cmd, sizeof(cmd), buf, len);
}

static int atm_write(struct spi_flash *flash,
			 u32 offset, size_t len, const void *buf)
{
	struct atm_spi_flash *atm = to_atm_spi_flash(flash);
	u32 actual;
	
/* st device only programs a single byte at a time */

	int ret;
	u8 cmd[4];

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	ret = 0;
	for (actual = 0; actual < len; actual++) {
		u32 current = offset + actual;

		cmd[0] = CMD_AT25FSXX_BP;
		cmd[1] = current >> 16;
		cmd[2] = current >> 8;
		cmd[3] = current;
		

		debug
		    ("PP: 0x%p => cmd = { 0x%02x 0x%02x%02x%02x }\n",
		     buf + actual, cmd[0], cmd[1], cmd[2], cmd[3]);

		ret = spi_flash_cmd(flash->spi, CMD_AT25FSXX_WREN, NULL, 0);
		if (ret < 0) {
			debug("SF: Enabling Write failed\n");
			break;
		}

		ret = spi_flash_cmd_write(flash->spi, cmd, 4,
					  buf + actual, 1);
		if (ret < 0) {
			debug("SF: Atmel Page Program failed\n");
			break;
		}

		ret = atm_wait_ready(flash, SPI_FLASH_PROG_TIMEOUT);
		if (ret < 0) {
			debug("SF: Atmel page programming timed out\n");
			break;
		}
	}

	debug("SF: Atmel: Successfully programmed %u bytes @ 0x%x\n",
	      len, offset);

	spi_release_bus(flash->spi);
	return ret;
}

int atm_erase(struct spi_flash *flash, u32 offset, size_t len)
{
	struct atm_spi_flash *atm = to_atm_spi_flash(flash);
	unsigned long sector_size;
	size_t actual;
	int ret;
	u8 cmd[4];

	/*
	 * This function currently uses sector erase only.
	 * probably speed things up by using bulk erase
	 * when possible.
	 */

	sector_size = atm->params->page_size * atm->params->pages_per_sector;

	if (offset % sector_size || len % sector_size) {
		printf("SF: Erase offset/length not multiple of sector size\n");
		return -1;
	}

	len /= sector_size;
	cmd[0] = CMD_AT25FSXX_SE;

	ret = spi_claim_bus(flash->spi);
	if (ret) {
		debug("SF: Unable to claim SPI bus\n");
		return ret;
	}

	ret = 0;
	for (actual = 0; actual < len; actual++) {
		u32 current = offset + (actual * sector_size);
		cmd[1] = current >> 16;
		cmd[2] = current >> 8;
		cmd[3] = current;

		ret = spi_flash_cmd(flash->spi, CMD_AT25FSXX_WREN, NULL, 0);
		if (ret < 0) {
			debug("SF: Enabling Write failed\n");
			break;
		}

		ret = spi_flash_cmd_write(flash->spi, cmd, 4, NULL, 0);
		if (ret < 0) {
			debug("SF: Atmel page erase failed\n");
			break;
		}

		/* Up to 2 seconds */
		ret = atm_wait_ready(flash, 2 * CONFIG_SYS_HZ);
		if (ret < 0) {
			debug("SF: Atmel page erase timed out\n");
			break;
		}
	}

	debug("SF: Atmel: Successfully erased %u bytes @ 0x%x\n",
	      len * sector_size, offset);

	spi_release_bus(flash->spi);
	return ret;
}

static void unlock_atm(struct atm_spi_flash *atm) {
	u8 cmd[2];
	int ret;


	cmd[0] = CMD_AT25FSXX_WRSR;
	cmd[1] = 0; /* clear all block protection */

	ret = spi_flash_cmd(atm->flash.spi, CMD_AT25FSXX_WREN, NULL, 0);
	if (ret < 0) {
		debug("SF: Enabling Write failed\n");
		return;
	}
	ret = spi_flash_cmd_write(atm->flash.spi, cmd, 2, NULL, 0);
	if (ret < 0) {
		debug("SF: Atmel Unlock failed\n");
		return;
	}
}

struct spi_flash *spi_flash_probe_atmel(struct spi_slave *spi, u8 * idcode)
{
	const struct atm_spi_flash_params *params;
	struct atm_spi_flash *atm;
	unsigned int i;
	int ret;
	u8 id[3];

	ret = spi_flash_cmd(spi, CMD_READ_ID, id, sizeof(id));
	if (ret)
		return NULL;

	debug("Atmel SF: Got idcode %02x %02x %02x\n", id[0],
			id[1], id[2]);

	for (i = 0; i < ARRAY_SIZE(atm_spi_flash_table); i++) {
		params = &atm_spi_flash_table[i];
		if (params->idcode1 == idcode[1]) {
			break;
		}
	}

	if (i == ARRAY_SIZE(atm_spi_flash_table)) {
		debug("SF: Unsupported Atmel ID %02x\n", id[1]);
		return NULL;
	}

	atm = malloc(sizeof(struct atm_spi_flash));
	if (!atm) {
		debug("SF: Failed to allocate memory\n");
		return NULL;
	}

	atm->params = params;
	atm->flash.spi = spi;
	atm->flash.name = params->name;

	atm->flash.write = atm_write;
	atm->flash.erase = atm_erase;
	atm->flash.read = atm_read_fast;
	atm->flash.size = params->page_size * params->pages_per_sector 
			* params->sectors_per_block * params->nr_blocks;

	debug("SF: Detected %s with page size %u, total %u bytes\n",
	      params->name, params->page_size, atm->flash.size);

    unlock_atm(atm);
	
	return &atm->flash;
}

#endif // CONFIG_SPI_FLASH_ATMEL
