#ifndef _NAND_H_
#define _NAND_H_

#define NAND_BUSY_BITS   ((1<<6)|(1<<5))

#define NAND_SMALL_BADBLOCK_POS     5
#define NAND_LARGE_BADBLOCK_POS     0

#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_READID		0x90
#define NAND_CMD_RESET		0xff
#define NAND_CMD_READOOB    0x50

#define NAND_CMD_READSTART  0x30

#define STATIC_NAND_ENABLE0  0x01fff000

#define STATIC_CS0_BASE_PA      0x41000000
#define STATIC_CS1_BASE_PA      0x41400000

#define CFG_NAND_BASE   STATIC_CS0_BASE_PA
#define CFG_NAND_ADDRESS_LATCH  CFG_NAND_BASE + (1<<18)
#define CFG_NAND_COMMAND_LATCH  CFG_NAND_BASE + (1<<19)

#define NAND_ENCODE_SCALING	8

struct nand_t {
	u8 page_shift;
	u8 block_shift;
	u8 page;
	u8 block;
	u32 block_mask;
	u32 block_size;
	u32 page_mask;
	u16 badblockpos;
	u16 spare;
};

extern void nand_init(void *);
extern u32 nand_read(struct nand_t *, u32, u32, u8 *);
extern void nand_print_bad(u32, struct nand_t *);

#ifndef SDK_BUILD_NAND_STAGE2_BLOCK
#define SDK_BUILD_NAND_STAGE2_BLOCK	2
#endif

#ifndef SDK_BUILD_NAND_STAGE2_BLOCK2
#define SDK_BUILD_NAND_STAGE2_BLOCK2	8
#endif

#endif

