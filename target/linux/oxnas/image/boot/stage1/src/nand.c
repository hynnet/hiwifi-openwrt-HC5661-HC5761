
#include "types.h"
#include "oxnas.h"
#include "debug.h"
#include "nand.h"

static void wait_nand_busy(void)
{
	u8 status;
	
	writeb(NAND_CMD_STATUS, CFG_NAND_COMMAND_LATCH);
	do {
		status = (0x7f & readb(CFG_NAND_BASE));
	} while (!(status  == NAND_BUSY_BITS));
}

static void get_sig(void *data)
{
	struct nand_t *nand = data;
 	u8 chip, org, tech;
//	u8 org;
	writeb(NAND_CMD_RESET, CFG_NAND_COMMAND_LATCH);
	wait_nand_busy();
	writeb(NAND_CMD_READID, CFG_NAND_COMMAND_LATCH);
	writeb(0x00, CFG_NAND_ADDRESS_LATCH);
	putstr(debug_uart, "NAND Flash Info: ");
 	chip = readb(CFG_NAND_BASE);	/* maker ID */
	putstr(debug_uart, "\n\r\tVendor ID:");
	puthex32(debug_uart,chip);
 	chip = readb(CFG_NAND_BASE);	/* devide ID */
	putstr(debug_uart, "\n\r\tProduct ID:");
	puthex32(debug_uart,chip);
 	tech = readb(CFG_NAND_BASE);	/* technology (cell type) */
	putstr(debug_uart, "\n\r\tType ID:");
	puthex32(debug_uart,tech);
	putstr(debug_uart, "\n\r");	
	org = readb(CFG_NAND_BASE);	/* organisation (page & block size) */
	nand->page = org & 0x3;
	nand->block = (org >> 4) & 0x3;
	switch (nand->page) {
	case 0:		/* 1k pages */
		nand->page_shift = 10;
		nand->page_mask = 0x3FF;
		break;
	case 1:		/* 2k pages */
		nand->page_shift = 11;
		nand->page_mask = 0x7ff;
		break;
	case 2:		/* 4k pages */
		nand->page_shift = 12;
		nand->page_mask = 0xfff;
		break;
	case 3:		/* 8k pages */
		nand->page_shift = 13;
		nand->page_mask = 0x1fff;
		break;
	}
	switch (nand->block) {
	case 0:		/* 64 k bytes */
		nand->block_shift = 16;
		nand->block_mask = 0xFFFF;
		break;
	case 1:		/* 128 k bytes */
		nand->block_shift = 17;
		nand->block_mask = 0x1ffff;
		break;
	case 2:		/* 256 k bytes */
		nand->block_shift = 18;
		nand->block_mask = 0x3ffff;
		break;
	case 3:		/* 512 k bytes */
		nand->block_shift = 19;
		nand->block_mask = 0x7ffff;
		break;

	}
	nand->block_size = (1 << nand->block_shift);
	nand->spare = 0x1 & (org >> 2);
	nand->badblockpos = ((1 << nand->page_shift) > 512) ?
	   NAND_LARGE_BADBLOCK_POS :
	   NAND_SMALL_BADBLOCK_POS;
}

static void set_nand_start_address(u32 page, u16 column)
{

	writeb(NAND_CMD_READ0, CFG_NAND_COMMAND_LATCH);
	writeb(column & 0xff, CFG_NAND_ADDRESS_LATCH);
	writeb(column >> 8, CFG_NAND_ADDRESS_LATCH);
	writeb((page & 0xff), CFG_NAND_ADDRESS_LATCH);
	writeb(((page >> 8) & 0xff), CFG_NAND_ADDRESS_LATCH);
	/* One more address cycle for devices > 128MiB */
	writeb(((page >> 16) & 0xff), CFG_NAND_ADDRESS_LATCH);
	
	writeb(NAND_CMD_READSTART, CFG_NAND_COMMAND_LATCH);

	udelay(100);
}

void nand_init(void *data)
{
	/*enable static bus to device_select */
	/* enable secondary functions for: gpioa12 .. gpioa 19 (data bus), 
	 * gpioa20(we) gpioa21(oe), gpioa 22(cs 0)
	 */
	writel(STATIC_NAND_ENABLE0, SYS_CTRL_SECONDARY_SEL);

	/* enable clock and release static block reset */
	writel((1<<SYS_CTRL_CKEN_STATIC_BIT), SYS_CTRL_CKEN_SET_CTRL);
	writel((1<<SYS_CTRL_RSTEN_STATIC_BIT), SYS_CTRL_RSTEN_CLR_CTRL);

	/* configure sys_ctrl static bus parameters (no operation use 
	 * hardware defaults) 
	 * get NAND device configuration 
	 */
	get_sig(data);
}

static int nand_bad_block(u32 offs, struct nand_t * nand)
{
	/* Read bad block marker from the chip */
	u32 page = (u32)(offs >> nand->page_shift);
	u32 column = nand->badblockpos + (1 << nand->page_shift);

	writeb(NAND_CMD_READ0, CFG_NAND_COMMAND_LATCH);
	writeb(column & 0xff, CFG_NAND_ADDRESS_LATCH);
	writeb(column >> 8, CFG_NAND_ADDRESS_LATCH);
	writeb((page & 0xff), CFG_NAND_ADDRESS_LATCH);
	writeb(((page >> 8) & 0xff), CFG_NAND_ADDRESS_LATCH);
	/* One more address cycle for devices > 128MiB */
	writeb(((page >> 16) & 0xff), CFG_NAND_ADDRESS_LATCH);
	
	writeb(NAND_CMD_READSTART, CFG_NAND_COMMAND_LATCH);

	udelay(100);

	if (readb(CFG_NAND_BASE) != 0xff)
		return 1;

	return 0;
}

static int error_correct(u8 val)
{
	u8 errors = 0;
	u8 mask = 1;
	u8 i;

	val = val ^ 0xAA;

	for (i = 0; i < 8; i++) {
		errors += (val & mask) ? 1 : 0;
		mask <<= 1;
	}

	if (errors < 3) {
		return 1;
	}
	if (errors > 5) {
		return 0;
	}

	return 2;
}

u32 nand_read(struct nand_t *nand, u32 address, u32 length, u8 *to)
{
	u8 val;
	u8 out = 0;
	u8 mask = 1;

	u32 block, current_block = 0xffffffff;
	u32 page;
	u32 offset;
	u32 count;

	u32 bytes_read = 0;

	/* convert BYTE address to nand device format for device */
	count = length * NAND_ENCODE_SCALING;
	while (count > 0) {
		offset = address & nand->page_mask;
		block = address >> nand->block_shift;
		page = address >> nand->page_shift;
		/* Check bad block only if the block is changed */
		if (current_block != block) {
			if (1 == nand_bad_block(address, nand)) {
				address += (1 << nand->block_shift);
				putstr(debug_uart, "\r\nbad block found at ");
				puthex32(debug_uart, (address & (~nand->block_mask)));
				continue;
			}
			current_block = block;
		}
		if (!offset) {
			set_nand_start_address(page, offset);
		}

		val = readb(CFG_NAND_BASE);
		if ((val ^ 0x55) == 0) {
			out |= mask;
		} else if ((val ^ 0xAA) != 0) {
			switch (error_correct(val)) {
				case 0:
					out |= mask;
				case 1:
					break;
				default:
					putstr(debug_uart, "\r\nFailed to correct error");
					return bytes_read;
			}
		}
		mask <<= 1;
		if ((0xFF & mask) == 0) {
			*to = out;
			to++;
			bytes_read++;
			out = 0;
			mask = 1;
		}
		count--;
		address++;
	}

	return bytes_read;
}

