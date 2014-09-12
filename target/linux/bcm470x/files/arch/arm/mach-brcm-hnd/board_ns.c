
#include <linux/types.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/platform_device.h>
#include <linux/ctype.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/clkdev.h>
#include <asm/hardware/gic.h>

#include <mach/clkdev.h>
#include <mach/memory.h>
#include <mach/io_map.h>

#include <plat/bsp.h>
#include <plat/mpcore.h>
#include <plat/plat-bcm5301x.h>

#ifdef CONFIG_MTD
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include "../../fs/squashfs/squashfs_fs.h"
#endif

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <bcmendian.h>
#include <hndsoc.h>
#include <siutils.h>
#include <hndcpu.h>
#include <hndpci.h>
#include <pcicfg.h>
#include <bcmdevs.h>
#include <trxhdr.h>
#ifdef HNDCTF
#include <ctf/hndctf.h>
#endif /* HNDCTF */
#include <hndsflash.h>
#ifdef CONFIG_MTD_NFLASH
#include <hndnand.h>
#endif
#include "bdinfo.h"

#define HC_BDINFO_SIZE	0x10000		/* 64k */
#define HC_BACKUP_SIZE	0x10000		/* 64k */

extern char __initdata saved_root_name[];

/* Global SB handle */
si_t *bcm947xx_sih = NULL;
DEFINE_SPINLOCK(bcm947xx_sih_lock);
EXPORT_SYMBOL(bcm947xx_sih);
EXPORT_SYMBOL(bcm947xx_sih_lock);

/* Convenience */
#define sih bcm947xx_sih
#define sih_lock bcm947xx_sih_lock

#define WATCHDOG_MIN    3000    /* milliseconds */
extern int panic_timeout;
extern int panic_on_oops;
static int watchdog = 0;

#ifdef HNDCTF
ctf_t *kcih = NULL;
EXPORT_SYMBOL(kcih);
ctf_attach_t ctf_attach_fn = NULL;
EXPORT_SYMBOL(ctf_attach_fn);
#endif /* HNDCTF */


struct dummy_super_block {
	u32	s_magic ;
};

/* This is the main reference clock 25MHz from external crystal */
static struct clk clk_ref = {
	.name = "Refclk",
	.rate = 25 * 1000000,	/* run-time override */
	.fixed = 1,
	.type	= CLK_XTAL,
};


static struct clk_lookup board_clk_lookups[] = {
	{
	.con_id		= "refclk",
	.clk		= &clk_ref,
	}
};

extern int _memsize;

void __init board_map_io(void)
{
	/* Install clock sources into the lookup table */
	clkdev_add_table(board_clk_lookups, 
			ARRAY_SIZE(board_clk_lookups));

	/* Map SoC specific I/O */
	soc_map_io( &clk_ref );
}


void __init board_init_irq(void)
{
	early_printk("board_init_irq\n");
	soc_init_irq();
}

void board_init_timer(void)
{
	soc_init_timer();
}

static int __init rootfs_mtdblock(void)
{
	int bootdev;
	int knldev;
	int block = 0;
#ifdef CONFIG_FAILSAFE_UPGRADE
	char *img_boot = nvram_get(BOOTPARTITION);
#endif

	bootdev = soc_boot_dev((void *)sih);
	knldev = soc_knl_dev((void *)sih);

	/* NANDBOOT */
	if (bootdev == SOC_BOOTDEV_NANDFLASH &&
	    knldev == SOC_KNLDEV_NANDFLASH) {
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (img_boot && simple_strtol(img_boot, NULL, 10))
			return 5;
		else
			return 3;
#else
		return 3;
#endif
	}

	/* SFLASH/PFLASH only */
	if (bootdev != SOC_BOOTDEV_NANDFLASH &&
	    knldev != SOC_KNLDEV_NANDFLASH) {
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (img_boot && simple_strtol(img_boot, NULL, 10))
			return 4;
		else
			return 2;
#else
		return 2;
#endif
	}

#ifdef BCMCONFMTD
	block++;
#endif
#ifdef CONFIG_FAILSAFE_UPGRADE
	if (img_boot && simple_strtol(img_boot, NULL, 10))
		block += 2;
#endif
	/* Boot from norflash and kernel in nandflash */
	return block+3;
}

static void __init brcm_setup(void)
{
	char *ptr;
	/* Get global SB handle */
	sih = si_kattach(SI_OSH);

	ptr = strnstr(boot_command_line, "root/dev/mtdblock", strlen("root=/dev/mtdblock"));
	
	if (ptr)
	{
		ptr += strlen("root=/dev/mtdblock");
		if (!isdigit(ptr))
		{
			sprintf(saved_root_name, "/dev/mtdblock%d", rootfs_mtdblock());
		}
	}
	else
	{
		sprintf(saved_root_name, "/dev/mtdblock%d", rootfs_mtdblock());
	}
	/* Set watchdog interval in ms */
    watchdog = simple_strtoul(nvram_safe_get("watchdog"), NULL, 0);

	/* Ensure at least WATCHDOG_MIN */
	if ((watchdog > 0) && (watchdog < WATCHDOG_MIN))
		watchdog = WATCHDOG_MIN;

	/* Set panic timeout in seconds */
	panic_timeout = watchdog / 1000;
	panic_on_oops = watchdog / 1000;
}

void soc_watchdog(void)
{
	if (watchdog > 0)
		si_watchdog_ms(sih, watchdog);
}

void __init board_init(void)
{
early_printk("board_init\n");
	brcm_setup();
	/*
	 * Add common platform devices that do not have board dependent HW
	 * configurations
	 */
	soc_add_devices();

	return;
}

void __init board_fixup(struct tag *t, char **cmdline,struct meminfo *mi)
{        
	u32 mem_size, lo_size ;

	/* Fuxup reference clock rate */
//	if (desc->nr == MACH_TYPE_BRCM_NS_QT )
//		clk_ref.rate = 17594;	/* Emulator ref clock rate */


	mem_size = _memsize;

	early_printk("board_fixup: mem=%uMiB\n", mem_size >> 20);

	lo_size = min(mem_size, DRAM_MEMORY_REGION_SIZE);

	mi->bank[0].start = PHYS_OFFSET;
	mi->bank[0].size = lo_size;
	mi->nr_banks++;

	if (lo_size == mem_size)
		return;

	mi->bank[1].start = DRAM_LARGE_REGION_BASE + lo_size;
	mi->bank[1].size = mem_size - lo_size;
	mi->nr_banks++;
}

#ifdef CONFIG_ZONE_DMA
/*
 * Adjust the zones if there are restrictions for DMA access.
 */
void __init bcm47xx_adjust_zones(unsigned long *size, unsigned long *hole)
{
	unsigned long dma_size = SZ_128M >> PAGE_SHIFT;

	if (size[0] <= dma_size)
		return;

	size[ZONE_NORMAL] = size[0] - dma_size;
	size[ZONE_DMA] = dma_size;
	hole[ZONE_NORMAL] = hole[0];
	hole[ZONE_DMA] = 0;
}
#endif

static struct sys_timer board_timer = {
   .init = board_init_timer,
};

void brcm_reset(char mode, const char *cmd)
{
#ifdef CONFIG_OUTER_CACHE_SYNC
	outer_cache.sync = NULL;
#endif
	hnd_cpu_reset(sih);
}

#if 0
#if (( (IO_BASE_VA >>18) & 0xfffc) != 0x3c40)
#error IO_BASE_VA 
#endif
#endif

MACHINE_START(BRCM_NS, "Northstar Prototype")
   //.phys_io = 					/* UART I/O mapping */
	//IO_BASE_PA,
   //.io_pg_offst = 				/* for early debug */
	//(IO_BASE_VA >>18) & 0xfffc,
   .fixup = board_fixup,			/* Opt. early setup_arch() */
   .map_io = board_map_io,			/* Opt. from setup_arch() */
   .init_irq = board_init_irq,			/* main.c after setup_arch() */
   .timer  = &board_timer,			/* main.c after IRQs */
   .init_machine = board_init,			/* Late archinitcall */
   .atag_offset = CONFIG_BOARD_PARAMS_PHYS,
    .handle_irq	= gic_handle_irq,
    .restart	= brcm_reset,

MACHINE_END

#ifdef	CONFIG_MACH_BRCM_NS_QT
MACHINE_START(BRCM_NS_QT, "Northstar Emulation Model")
   //.phys_io = 					/* UART I/O mapping */
	//IO_BASE_PA,
   //.io_pg_offst = 				/* for early debug */
	//(IO_BASE_VA >>18) & 0xfffc,
   //.fixup = board_fixup,			/* Opt. early setup_arch() */
   .map_io = board_map_io,			/* Opt. from setup_arch() */
   .init_irq = board_init_irq,			/* main.c after setup_arch() */
   .timer  = &board_timer,			/* main.c after IRQs */
   .init_machine = board_init,			/* Late archinitcall */
   .atag_offset = CONFIG_BOARD_PARAMS_PHYS,
    .handle_irq	= gic_handle_irq,
    .restart	= brcm_reset,
MACHINE_END
#endif


#ifdef CONFIG_MTD

static spinlock_t *bcm_mtd_lock = NULL;

spinlock_t *partitions_lock_init(void)
{
	if (!bcm_mtd_lock) {
		bcm_mtd_lock = (spinlock_t *)kzalloc(sizeof(spinlock_t), GFP_KERNEL);
		if (!bcm_mtd_lock)
			return NULL;

		spin_lock_init( bcm_mtd_lock );
	}
	return bcm_mtd_lock;
}
EXPORT_SYMBOL(partitions_lock_init);

static struct nand_hw_control *nand_hwcontrol = NULL;
struct nand_hw_control *nand_hwcontrol_lock_init(void)
{
	if (!nand_hwcontrol) {
		nand_hwcontrol = (struct nand_hw_control *)kzalloc(sizeof(struct nand_hw_control), GFP_KERNEL);
		if (!nand_hwcontrol)
			return NULL;

		spin_lock_init(&nand_hwcontrol->lock);
		init_waitqueue_head(&nand_hwcontrol->wq);
	}
	return nand_hwcontrol;
}
EXPORT_SYMBOL(nand_hwcontrol_lock_init);

/* Find out prom size */
static uint32 boot_partition_size(uint32 flash_phys) {
	uint32 bootsz, *bisz;

	/* Default is 256K boot partition */
	bootsz = 256 * 1024;

	/* Do we have a self-describing binary image? */
	bisz = (uint32 *)(flash_phys + BISZ_OFFSET);
	if (bisz[BISZ_MAGIC_IDX] == BISZ_MAGIC) {
		int isz = bisz[BISZ_DATAEND_IDX] - bisz[BISZ_TXTST_IDX];

		if (isz > (1024 * 1024))
			bootsz = 2048 * 1024;
		else if (isz > (512 * 1024))
			bootsz = 1024 * 1024;
		else if (isz > (256 * 1024))
			bootsz = 512 * 1024;
		else if (isz <= (128 * 1024))
			bootsz = 128 * 1024;
	}
	return bootsz;
}

#if defined(BCMCONFMTD)
#define MTD_PARTS 1
#else
#define MTD_PARTS 0
#endif
#if defined(PLC)
#define PLC_PARTS 1
#else
#define PLC_PARTS 0
#endif
#if defined(CONFIG_FAILSAFE_UPGRADE)
#define FAILSAFE_PARTS 2
#else
#define FAILSAFE_PARTS 0
#endif

#define HC_BDINFO_PARTS	1
#define HC_BACKUP_PARTS	1

/* boot;nvram;kernel;rootfs;empty */
#define FLASH_PARTS_NUM	(5+MTD_PARTS+PLC_PARTS+FAILSAFE_PARTS+HC_BDINFO_PARTS+HC_BACKUP_PARTS)

static struct mtd_partition bcm947xx_flash_parts[FLASH_PARTS_NUM] = {{0}};

static uint lookup_flash_rootfs_offset(struct mtd_info *mtd, int *trx_off, size_t size)
{
	struct romfs_super_block *romfsb;
	struct cramfs_super *cramfsb;
	struct squashfs_super_block *squashfsb;
	struct trx_header *trx;
	unsigned char buf[512];
	int off;
	size_t len;

	romfsb = (struct romfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	squashfsb = (void *) buf;
	trx = (struct trx_header *) buf;

	/* Look at every 64 KB boundary */
	for (off = *trx_off; off < size; off += (64 * 1024)) {
		memset(buf, 0xe5, sizeof(buf));

		/*
		 * Read block 0 to test for romfs and cramfs superblock
		 */
		if (mtd_read(mtd, off, sizeof(buf), &len, buf) ||
		    len != sizeof(buf))
			continue;

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			*trx_off = off;
			if (trx->offsets[1] == 0)
				continue;
			/*
			 * Read to test for romfs and cramfs superblock
			 */
			off += le32_to_cpu(trx->offsets[1]);
			memset(buf, 0xe5, sizeof(buf));
			if (mtd_read(mtd, off, sizeof(buf), &len, buf) || len != sizeof(buf))
				continue;
		}

		/* romfs is at block zero too */
		if (romfsb->word0 == ROMSB_WORD0 &&
		    romfsb->word1 == ROMSB_WORD1) {
			printk(KERN_NOTICE
			       "%s: romfs filesystem found at block %d\n",
			       mtd->name, off / mtd->erasesize);
			break;
		}

		/* so is cramfs */
		if (cramfsb->magic == CRAMFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: cramfs filesystem found at block %d\n",
			       mtd->name, off / mtd->erasesize);
			break;
		}

		if (squashfsb->s_magic == SQUASHFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: squash filesystem found at block %d\n",
			       mtd->name, off / mtd->erasesize);
			break;
		}
	}

	return off;
}

static void board_parse_bdinfo(struct mtd_info *mtd, int offset, size_t bdinfo_size)
{
	unsigned char *bdinfo;
	size_t len;
	bdinfo = (unsigned char *)kmalloc(bdinfo_size, GFP_KERNEL);

	if (NULL == bdinfo)
	{
		printk(KERN_ERR "kmalloc 0x%x buf for bdinfo failed\n", bdinfo_size);
		return;
	}

	if (mtd_read(mtd, offset, bdinfo_size, &len, bdinfo) || len != bdinfo_size)
	{
		printk(KERN_ERR "read bdinfo failed\n");
		goto err;
	}

	if (parse_bdinfo(bdinfo + 0x180, 0xfe80) != 0)
	{
		printk(KERN_ERR "unable to parse bdinfo from flash\n");
	}

err:
	kfree(bdinfo);
}

struct mtd_partition *
init_mtd_partitions(hndsflash_t *sfl_info, struct mtd_info *mtd, size_t size)
{
	int bootdev;
	int knldev;
	int nparts = 0;
	uint32 offset = 0;
	uint rfs_off = 0;
	uint vmlz_off, knl_size;
	uint32 top = 0;
	uint32 bootsz;
#ifdef CONFIG_FAILSAFE_UPGRADE
	char *img_boot = nvram_get(BOOTPARTITION);
	char *imag_1st_offset = nvram_get(IMAGE_FIRST_OFFSET);
	char *imag_2nd_offset = nvram_get(IMAGE_SECOND_OFFSET);
	unsigned int image_first_offset = 0;
	unsigned int image_second_offset = 0;
	char dual_image_on = 0;

	/* The image_1st_size and image_2nd_size are necessary if the Flash does not have any
	 * image
	 */
	dual_image_on = (img_boot != NULL && imag_1st_offset != NULL && imag_2nd_offset != NULL);

	if (dual_image_on) {
		image_first_offset = simple_strtol(imag_1st_offset, NULL, 10);
		image_second_offset = simple_strtol(imag_2nd_offset, NULL, 10);
		printk("The first offset=%x, 2nd offset=%x\n", image_first_offset,
			image_second_offset);

	}
#endif	/* CONFIG_FAILSAFE_UPGRADE */

	bootdev = soc_boot_dev((void *)sih);
	knldev = soc_knl_dev((void *)sih);

	if (bootdev == SOC_BOOTDEV_NANDFLASH) {
		/* Do not init MTD partitions on NOR flash when NAND boot */
		return NULL;	
	}

	if (knldev != SOC_KNLDEV_NANDFLASH) {
		vmlz_off = 0;
		rfs_off = lookup_flash_rootfs_offset(mtd, &vmlz_off, size);

		/* Size pmon */
		bcm947xx_flash_parts[nparts].name = "boot";
		bcm947xx_flash_parts[nparts].size = vmlz_off;
		bcm947xx_flash_parts[nparts].offset = top;
		/* clear this flags to make sure can update bootloader */
#if 0
		bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE; /* forces on read only */
#endif
		nparts++;

		/* Setup kernel MTD partition */
		bcm947xx_flash_parts[nparts].name = "linux";
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (dual_image_on) {
			bcm947xx_flash_parts[nparts].size = image_second_offset-image_first_offset;
		} else {
			bcm947xx_flash_parts[nparts].size = mtd->size - vmlz_off;

			/* Reserve for NVRAM */
			bcm947xx_flash_parts[nparts].size -= ROUNDUP(nvram_space, mtd->erasesize);
#ifdef PLC
			/* Reserve for PLC */
			bcm947xx_flash_parts[nparts].size -= ROUNDUP(0x1000, mtd->erasesize);
#endif
#ifdef BCMCONFMTD
			bcm947xx_flash_parts[nparts].size -= (mtd->erasesize *4);
#endif
		}
#else

		bcm947xx_flash_parts[nparts].size = mtd->size - vmlz_off;
		
#ifdef PLC
		/* Reserve for PLC */
		bcm947xx_flash_parts[nparts].size -= ROUNDUP(0x1000, mtd->erasesize);
#endif
		/* Reserve for bdinfo */
		bcm947xx_flash_parts[nparts].size -= ROUNDUP(HC_BDINFO_SIZE, mtd->erasesize);

		/* Reserve for backup */
		bcm947xx_flash_parts[nparts].size -= ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize);		

		/* Reserve for NVRAM */
		bcm947xx_flash_parts[nparts].size -= ROUNDUP(nvram_space, mtd->erasesize);

#ifdef BCMCONFMTD
		bcm947xx_flash_parts[nparts].size -= (mtd->erasesize *4);
#endif
#endif	/* CONFIG_FAILSAFE_UPGRADE */
		bcm947xx_flash_parts[nparts].offset = vmlz_off;
		knl_size = bcm947xx_flash_parts[nparts].size;
		offset = bcm947xx_flash_parts[nparts].offset + knl_size;
		nparts++; 
		
		/* Setup rootfs MTD partition */
		bcm947xx_flash_parts[nparts].name = "rootfs";
		bcm947xx_flash_parts[nparts].size = knl_size - (rfs_off - vmlz_off);
		bcm947xx_flash_parts[nparts].offset = rfs_off;
		/* make sure can update */
#if 0
		bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE; /* forces on read only */
#endif
		nparts++;
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (dual_image_on) {
			offset = image_second_offset;
			rfs_off = lookup_flash_rootfs_offset(mtd, &offset, size);
			vmlz_off = offset;
			/* Setup kernel2 MTD partition */
			bcm947xx_flash_parts[nparts].name = "linux2";
			bcm947xx_flash_parts[nparts].size = mtd->size - image_second_offset;
			/* Reserve for NVRAM */
			bcm947xx_flash_parts[nparts].size -= ROUNDUP(nvram_space, mtd->erasesize);

#ifdef BCMCONFMTD
			bcm947xx_flash_parts[nparts].size -= (mtd->erasesize *4);
#endif
#ifdef PLC
			/* Reserve for PLC */
			bcm947xx_flash_parts[nparts].size -= ROUNDUP(0x1000, mtd->erasesize);
#endif
			bcm947xx_flash_parts[nparts].offset = image_second_offset;
			knl_size = bcm947xx_flash_parts[nparts].size;
			offset = bcm947xx_flash_parts[nparts].offset + knl_size;
			nparts++;

			/* Setup rootfs MTD partition */
			bcm947xx_flash_parts[nparts].name = "rootfs2";
			bcm947xx_flash_parts[nparts].size =
				knl_size - (rfs_off - image_second_offset);
			bcm947xx_flash_parts[nparts].offset = rfs_off;
			/* forces on read only */
			bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE;
			nparts++;
		}
#endif	/* CONFIG_FAILSAFE_UPGRADE */

	} else {
		bootsz = boot_partition_size(sfl_info->base);
		printk("Boot partition size = %d(0x%x)\n", bootsz, bootsz);
		/* Size pmon */
		bcm947xx_flash_parts[nparts].name = "boot";
		bcm947xx_flash_parts[nparts].size = bootsz;
		bcm947xx_flash_parts[nparts].offset = top;
#if 0
		bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE; /* forces on read only */
#endif
		offset = bcm947xx_flash_parts[nparts].size;
		nparts++;
	}

#ifdef BCMCONFMTD
	/* Setup CONF MTD partition */
	bcm947xx_flash_parts[nparts].name = "confmtd";
	bcm947xx_flash_parts[nparts].size = mtd->erasesize * 4;
	bcm947xx_flash_parts[nparts].offset = offset;
	offset = bcm947xx_flash_parts[nparts].offset + bcm947xx_flash_parts[nparts].size;
	nparts++;
#endif	/* BCMCONFMTD */

#ifdef PLC
	/* Setup plc MTD partition */
	bcm947xx_flash_parts[nparts].name = "plc";
	bcm947xx_flash_parts[nparts].size = ROUNDUP(0x1000, mtd->erasesize);
	bcm947xx_flash_parts[nparts].offset =
		size - (ROUNDUP(nvram_space, mtd->erasesize) + ROUNDUP(0x1000, mtd->erasesize));
	nparts++;
#endif

	/* Setup bdinfo MTD partition */
	bcm947xx_flash_parts[nparts].name = "bdinfo";
	bcm947xx_flash_parts[nparts].size = ROUNDUP(HC_BDINFO_SIZE, mtd->erasesize);
	bcm947xx_flash_parts[nparts].offset =
		size - (ROUNDUP(nvram_space, mtd->erasesize) + ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize) + ROUNDUP(HC_BDINFO_SIZE, mtd->erasesize));
	bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE;
	board_parse_bdinfo(mtd, bcm947xx_flash_parts[nparts].offset, bcm947xx_flash_parts[nparts].size);
	nparts++;

	/* Setup bckup MTD partition */
	bcm947xx_flash_parts[nparts].name = "backup";
	bcm947xx_flash_parts[nparts].size = ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize);
	bcm947xx_flash_parts[nparts].offset =
		size - (ROUNDUP(nvram_space, mtd->erasesize) + ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize));
	bcm947xx_flash_parts[nparts].mask_flags = MTD_WRITEABLE; /* forces on read only */
	nparts++;

	/* Setup nvram MTD partition */
	bcm947xx_flash_parts[nparts].name = "nvram";
	bcm947xx_flash_parts[nparts].size = ROUNDUP(nvram_space, mtd->erasesize);
	bcm947xx_flash_parts[nparts].offset = size - bcm947xx_flash_parts[nparts].size;
	nparts++;

	return bcm947xx_flash_parts;
}

EXPORT_SYMBOL(init_mtd_partitions);

#endif /* CONFIG_MTD */


#ifdef	CONFIG_MTD_NFLASH

static void board_nparse_bdinfo(hndnand_t *nfl, struct mtd_info *mtd, int offset, size_t bdinfo_size)
{
	uint blocksize, mask, blk_offset, off;
	unsigned char *bdinfo;
	int ret;

	bdinfo = (unsigned char *)kmalloc(HC_BDINFO_SIZE, GFP_KERNEL);
	blocksize = mtd->erasesize;

	if (NULL == bdinfo)
	{
		printk(KERN_ERR "kmalloc 0x%x buf for bdinfo failed\n", HC_BDINFO_SIZE);
		return;
	}

	for (off = offset; off < offset + bdinfo_size; off += blocksize)
	{	
		mask = blocksize - 1;
		blk_offset = off & ~mask;
		if (hndnand_checkbadb(nfl, blk_offset) != 0)
			continue;

		memset(bdinfo, 0x0, HC_BDINFO_SIZE);

		if ((ret = hndnand_read(nfl, off, HC_BDINFO_SIZE, bdinfo)) != HC_BDINFO_SIZE) {
			printk(KERN_NOTICE "%s: nflash_read return %d\n", mtd->name, ret);
			continue;
		}

		if (parse_bdinfo(bdinfo + 0x180, 0xfe80) != 0)
		{
			printk(KERN_ERR "unable to parse bdinfo from flash\n");
		}
		else
		{
			break;
		}		
	}

	if (bdinfo)
		kfree(bdinfo);	
}

#define NFLASH_PARTS_NUM	9
static struct mtd_partition bcm947xx_nflash_parts[NFLASH_PARTS_NUM] = {{0}};

static uint
lookup_nflash_rootfs_offset(hndnand_t *nfl, struct mtd_info *mtd, int offset, size_t size)
{
	struct romfs_super_block *romfsb;
	struct cramfs_super *cramfsb;
	struct squashfs_super_block *squashfsb;
	struct trx_header *trx;
	unsigned char *buf;
	uint blocksize, pagesize, mask, blk_offset, off, shift = 0;
	int ret;

	pagesize = nfl->pagesize;
	buf = (unsigned char *)kmalloc(pagesize, GFP_KERNEL);
	if (!buf) {
		printk("lookup_nflash_rootfs_offset: kmalloc fail\n");
		return 0;
	}

	romfsb = (struct romfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	squashfsb = (void *) buf;
	trx = (struct trx_header *) buf;

	/* Look at every block boundary till 16MB; higher space is reserved for application data. */
	blocksize = mtd->erasesize;
	printk("lookup_nflash_rootfs_offset: offset = 0x%x\n", offset);
	for (off = offset; off < offset + size; off += blocksize) {
		mask = blocksize - 1;
		blk_offset = off & ~mask;
		if (hndnand_checkbadb(nfl, blk_offset) != 0)
			continue;
		memset(buf, 0xe5, pagesize);
		if ((ret = hndnand_read(nfl, off, pagesize, buf)) != pagesize) {
			printk(KERN_NOTICE
			       "%s: nflash_read return %d\n", mtd->name, ret);
			continue;
		}

		/* Try looking at TRX header for rootfs offset */
		if (le32_to_cpu(trx->magic) == TRX_MAGIC) {
			mask = pagesize - 1;
			off = offset + (le32_to_cpu(trx->offsets[1]) & ~mask) - blocksize;
			shift = (le32_to_cpu(trx->offsets[1]) & mask);
			romfsb = (struct romfs_super_block *)((unsigned char *)romfsb + shift);
			cramfsb = (struct cramfs_super *)((unsigned char *)cramfsb + shift);
			squashfsb = (struct squashfs_super_block *)
				((unsigned char *)squashfsb + shift);
			continue;
		}

		/* romfs is at block zero too */
		if (romfsb->word0 == ROMSB_WORD0 &&
		    romfsb->word1 == ROMSB_WORD1) {
			printk(KERN_NOTICE
			       "%s: romfs filesystem found at block %d\n",
			       mtd->name, off / blocksize);
			break;
		}

		/* so is cramfs */
		if (cramfsb->magic == CRAMFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: cramfs filesystem found at block %d\n",
			       mtd->name, off / blocksize);
			break;
		}

		if (squashfsb->s_magic == SQUASHFS_MAGIC) {
			printk(KERN_NOTICE
			       "%s: squash filesystem with lzma found at block %d\n",
			       mtd->name, off / blocksize);
			break;
		}

	}

	if (buf)
		kfree(buf);

	return shift + off;
}

struct mtd_partition *
init_nflash_mtd_partitions(hndnand_t *nfl, struct mtd_info *mtd, size_t size)
{
	int bootdev;
	int knldev;
	int nparts = 0;
	uint32 offset = 0;
	uint shift = 0;
	uint32 top = 0;
	uint32 bootsz;
#ifdef CONFIG_FAILSAFE_UPGRADE
	char *img_boot = nvram_get(BOOTPARTITION);
	char *imag_1st_offset = nvram_get(IMAGE_FIRST_OFFSET);
	char *imag_2nd_offset = nvram_get(IMAGE_SECOND_OFFSET);
	unsigned int image_first_offset = 0;
	unsigned int image_second_offset = 0;
	char dual_image_on = 0;

	/* The image_1st_size and image_2nd_size are necessary if the Flash does not have any
	 * image
	 */
	dual_image_on = (img_boot != NULL && imag_1st_offset != NULL && imag_2nd_offset != NULL);

	if (dual_image_on) {
		image_first_offset = simple_strtol(imag_1st_offset, NULL, 10);
		image_second_offset = simple_strtol(imag_2nd_offset, NULL, 10);
		printk("The first offset=%x, 2nd offset=%x\n", image_first_offset,
			image_second_offset);

	}
#endif	/* CONFIG_FAILSAFE_UPGRADE */

	bootdev = soc_boot_dev((void *)sih);
	knldev = soc_knl_dev((void *)sih);

	if (bootdev == SOC_BOOTDEV_NANDFLASH) {
		bootsz = boot_partition_size(nfl->base);
		if (bootsz > mtd->erasesize) {
			/* Prepare double space in case of bad blocks */
			bootsz = (bootsz << 1);
		} else {
			/* CFE occupies at least one block */
			bootsz = mtd->erasesize;
		}
		printk("Boot partition size = %d(0x%x)\n", bootsz, bootsz);

		/* Size pmon */
		bcm947xx_nflash_parts[nparts].name = "boot";
		bcm947xx_nflash_parts[nparts].size = bootsz;
		bcm947xx_nflash_parts[nparts].offset = top;
		offset = bcm947xx_nflash_parts[nparts].size;
		nparts++;

		/* Setup NVRAM MTD partition */
		bcm947xx_nflash_parts[nparts].name = "nvram";
		bcm947xx_nflash_parts[nparts].size = NFL_BOOT_SIZE - offset;
		bcm947xx_nflash_parts[nparts].offset = offset;

		offset = NFL_BOOT_SIZE;
		nparts++;
	}

	if (knldev == SOC_KNLDEV_NANDFLASH) {
		/* Setup kernel MTD partition */
		bcm947xx_nflash_parts[nparts].name = "linux";
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (dual_image_on) {
			bcm947xx_nflash_parts[nparts].size =
				image_second_offset - image_first_offset;
		} else
#endif
		{
			bcm947xx_nflash_parts[nparts].size =
				nparts ? (NFL_BOOT_OS_SIZE - NFL_BOOT_SIZE) : NFL_BOOT_OS_SIZE;
		}
		bcm947xx_nflash_parts[nparts].offset = offset;

		shift = lookup_nflash_rootfs_offset(nfl, mtd, offset,
			bcm947xx_nflash_parts[nparts].size);

#ifdef CONFIG_FAILSAFE_UPGRADE
		if (dual_image_on)
			offset = image_second_offset;
		else
#endif
		offset = NFL_BOOT_OS_SIZE;
		nparts++;

		/* Setup rootfs MTD partition */
		bcm947xx_nflash_parts[nparts].name = "rootfs";
#ifdef CONFIG_FAILSAFE_UPGRADE
		if (dual_image_on)
			bcm947xx_nflash_parts[nparts].size = image_second_offset - shift;
		else
#endif
		bcm947xx_nflash_parts[nparts].size = NFL_BOOT_OS_SIZE - shift;
		bcm947xx_nflash_parts[nparts].offset = shift;
		offset = NFL_BOOT_OS_SIZE;

		nparts++;

#ifdef CONFIG_FAILSAFE_UPGRADE
		/* Setup 2nd kernel MTD partition */
		if (dual_image_on) {
			bcm947xx_nflash_parts[nparts].name = "linux2";
			bcm947xx_nflash_parts[nparts].size = NFL_BOOT_OS_SIZE - image_second_offset;
			bcm947xx_nflash_parts[nparts].offset = image_second_offset;
			shift = lookup_nflash_rootfs_offset(nfl, mtd, image_second_offset,
			                                    bcm947xx_nflash_parts[nparts].size);
			nparts++;
			/* Setup rootfs MTD partition */
			bcm947xx_nflash_parts[nparts].name = "rootfs2";
			bcm947xx_nflash_parts[nparts].size = NFL_BOOT_OS_SIZE - shift;
			bcm947xx_nflash_parts[nparts].offset = shift;
			bcm947xx_nflash_parts[nparts].mask_flags = MTD_WRITEABLE;
			nparts++;
		}
#endif	/* CONFIG_FAILSAFE_UPGRADE */

	}

#ifdef PLAT_NAND_JFFS2
	/* Setup the remainder of NAND Flash as FFS partition */
	if( size > offset ) {
		bcm947xx_nflash_parts[nparts].name = "ffs";
		bcm947xx_nflash_parts[nparts].size = size - offset ;
		bcm947xx_nflash_parts[nparts].offset = offset;
		bcm947xx_nflash_parts[nparts].mask_flags = 0;
		bcm947xx_nflash_parts[nparts].ecclayout = mtd->ecclayout;
		nparts++;
	}
#endif

	/* Setup bdinfo MTD partition */
	bcm947xx_nflash_parts[nparts].name = "bdinfo";
	bcm947xx_nflash_parts[nparts].size = ROUNDUP(HC_BDINFO_SIZE, mtd->erasesize);
	bcm947xx_nflash_parts[nparts].offset = size - ROUNDUP(HC_BDINFO_SIZE, mtd->erasesize) - ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize);
	bcm947xx_nflash_parts[nparts].mask_flags = MTD_WRITEABLE;
	board_nparse_bdinfo(nfl, mtd, bcm947xx_nflash_parts[nparts].offset, bcm947xx_nflash_parts[nparts].size);
	nparts++;

	/* Setup bckup MTD partition */
	bcm947xx_nflash_parts[nparts].name = "backup";
	bcm947xx_nflash_parts[nparts].size = ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize);
	bcm947xx_nflash_parts[nparts].offset = size - ROUNDUP(HC_BACKUP_SIZE, mtd->erasesize);
	bcm947xx_nflash_parts[nparts].mask_flags = MTD_WRITEABLE; /* forces on read only */
	nparts++;

	return bcm947xx_nflash_parts;
}

/* LR: Calling this function directly violates Linux API conventions */
EXPORT_SYMBOL(init_nflash_mtd_partitions);
#else

/* Note: NFL_BOOT_OS_SIZE is now set to 32M for FAILSAFE issue */
#ifndef	NFL_BOOT_OS_SIZE
#define	NFL_BOOT_OS_SIZE	SZ_32M
#endif

/* LR: Here is how partition managers are intended to be */
static int parse_cfenand_partitions(struct mtd_info *mtd,
                             struct mtd_partition **pparts,
                             unsigned long origin)
{
	unsigned size, offset, nparts ;
	static struct mtd_partition cfenand_parts[2] = {{0}};

	if ( pparts == NULL )
		return -EINVAL;

	/*
	NOTE:
	Can not call init_nflash_mtd_partitions)_ because it depends
	on the "other" conflicting driver for its operation,
	so instead this will just create two partitions
	so that jffs2 does not overwrite bootable areas.
	*/

	size = mtd->size ;
	nparts = 0;
	offset = origin ;

	printk("%s: slicing up %uMiB provisionally\n", __func__, size >> 20 );

	cfenand_parts[nparts].name = "cfenand";
	cfenand_parts[nparts].size = NFL_BOOT_OS_SIZE ;
	cfenand_parts[nparts].offset = offset ;
	cfenand_parts[nparts].mask_flags = MTD_WRITEABLE;
	cfenand_parts[nparts].ecclayout = mtd->ecclayout;
	nparts++;
	offset += NFL_BOOT_OS_SIZE ;

	cfenand_parts[nparts].name = "jffs2";
	cfenand_parts[nparts].size = size - offset ;
	cfenand_parts[nparts].offset = offset;
	cfenand_parts[nparts].mask_flags = 0;
	cfenand_parts[nparts].ecclayout = mtd->ecclayout;
	nparts++;

	*pparts = cfenand_parts;
	return nparts;
}

static struct mtd_part_parser cfenand_parser = {
        .owner = THIS_MODULE,
        .parse_fn = parse_cfenand_partitions,
        .name = "cfenandpart",
};

static int __init cfenenad_parser_init(void)
{
        return register_mtd_parser(&cfenand_parser);
}

module_init(cfenenad_parser_init);

#endif /* CONFIG_MTD_NFLASH */
