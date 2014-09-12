/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG820_H
#define __CONFIG820_H

#define readb(p)  (*(volatile u8 *)(p))
#define readl(p)  (*(volatile u32 *)(p))
#define writeb(v, p) (*(volatile u8 *)(p)= (v))
#define writel(v, p) (*(volatile u32*)(p)=(v))

#define CFG_FLASH_EMPTY_INFO

/**
 * Architecture
 */
#define CONFIG_ARM11        1
#define CONFIG_OXNAS        1
#define CONFIG_OXNAS_MANUAL_STATIC_ARBITRATION

#if (USE_NAND == 1)
#define CONFIG_CMD_NAND
#if (USE_NAND_ENV == 1)
#define ENV_ON_NAND                     /* Define to have the U-Boot env. stored on NAND flash */
#endif // USE_NAND_ENV
#endif // USE_NAND

#if (USE_OTP == 1)
#define CONFIG_CMD_OTP
#endif // USE_OTP

#if (USE_SPI == 1)
#define CONFIG_SPI      /* enable serial flash through spi */
#define CONFIG_SOFT_SPI /* alternatively access through bit bashed i2c interface */
#define CONFIG_SPI_FLASH_SST
#define CONFIG_SPI_FLASH_ATMEL
#define CONFIG_SPI_FLASH_EON
#define CONFIG_SPI_FLASH_STMICRO
//#define DEBUG_SPI
#define CONFIG_CMD_SF
#if (USE_SPI_ENV == 1)
#define ENV_ON_SPI                     /* Define to have the U-Boot env. stored on SPI flash */
#endif
#define CONFIG_SYS_HZ	25000000
#endif // USE_SPI

/*****
 * nand parameters 
 */

// number of pages to duplicate to allow for errors in NAND flash, 2 for SLC more for MLC
#define CONFIG_PAGE_REPLICATION 2

// number of blocks to duplicate to allow for errors in NAND flash, 2 for SLC more for MLC
#define CONFIG_BLOCK_REPLICATION 2

#define CONFIG_FLASH_KERNEL_SPACE       3 * 1024 * 1024 // 3MB
/*****
 *  spi flash parameters 
 */
#define CFG_I2C_EEPROM_ADDR_LEN 2


#if (USE_SATA == 1)
#define CONFIG_OXNAS_USE_SATA           /* Define to include support for SATA disks */
#if (USE_SATA_ENV == 1)
#define ENV_ON_SATA                     /* Define to have the U-Boot env. stored on SATA disk */
#endif // USE_SATA_ENV
#endif // USE_SATA
#if (USE_FLASH == 0)
#define CFG_NO_FLASH                    /* Define to NOT include flash support on static bus*/
#endif //USE_FLASH



/* Won't be using any interrupts */
#undef CONFIG_USE_IRQ

/* Everything, incl board info, in Hz */
#undef CFG_CLKS_IN_HZ

#define CFG_HUSH_PARSER		1
#define CFG_PROMPT_HUSH_PS2	"> "

/* Miscellaneous configurable options */
#define CFG_LONGHELP				/* undef to save memory		*/
#ifdef CFG_HUSH_PARSER
#define CFG_PROMPT		"[root@PogoPlug-Pro]#"		/* Monitor Command Prompt */
#else
#define CFG_PROMPT		"# "		/* Monitor Command Prompt */
#endif
#define CFG_CBSIZE      256             /* Console I/O Buffer Size  */

/* Print Buffer Size */
#define CFG_PBSIZE      ((CFG_CBSIZE)+sizeof(CFG_PROMPT)+16)
#define CFG_MAXARGS     16              /* max number of command args   */
#define CFG_BARGSIZE    (CFG_CBSIZE)    /* Boot Argument Buffer Size    */

#define CONFIG_CMDLINE_TAG          1   /* enable passing of ATAGs  */
#define CONFIG_SETUP_MEMORY_TAGS    1
#define CONFIG_MISC_INIT_R          1   /* call misc_init_r during start up */
#define CONFIG_INITRD_TAG           1   /* allow initrd tag to be generated */

/* May want to do some setup prior to relocation */
#define CONFIG_INIT_CRITICAL

/* ARM specific late initialisation */
#define BOARD_LATE_INIT

/**
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE        (128*1024)  /* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ    (4*1024)    /* IRQ stack */
#define CONFIG_STACKSIZE_FIQ    (4*1024)    /* FIQ stack */
#endif

/**
 * RAM
 */
#define CONFIG_NR_DRAM_BANKS    1           /* We have 1 bank of SDRAM */
#define PHYS_SDRAM_1_PA         0x60000000  /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_MAX_SIZE	(512 * 1024 * 1024)
#define CFG_SRAM_BASE           0x50000000
#define CFG_SRAM_SIZE			(64 * 1024)

#undef INITIALISE_SDRAM

/* Default location from which bootm etc will load */
#define CFG_LOAD_ADDR   (PHYS_SDRAM_1_PA)

/* Core addresses */
#define MACA_BASE_PA            0x40400000
#define MACB_BASE_PA            0x40800000
#define MAC_BASE_PA             MACA_BASE_PA
#define STATIC_CS0_BASE_PA      0x41000000
#define STATIC_CS1_BASE_PA      0x41400000
#define STATIC_CONTROL_BASE_PA  0x41C00000
#define SATA_DATA_BASE_PA       0x42000000 /* non-functional, DMA just needs an address */

#define GPIO_1_PA               0x44000000
#define GPIO_2_PA               0x44100000

#define SYS_CONTROL_BASE_PA     0x44e00000
#define SEC_CONTROL_BASE_PA     0x44f00000
#define RPSA_BASE_PA            0x44400000
#define RPSC_BASE_PA            0x44500000

/* Static bus registers */
#define STATIC_CONTROL_VERSION  ((STATIC_CONTROL_BASE_PA) + 0x0)
#define STATIC_CONTROL_BANK0    ((STATIC_CONTROL_BASE_PA) + 0x4)
#define STATIC_CONTROL_BANK1    ((STATIC_CONTROL_BASE_PA) + 0x8)

/* OTP control registers */
#define OTP_ADDR_PROG		(SEC_CONTROL_BASE_PA + 0x1E0)
#define OTP_READ_DATA 		(SEC_CONTROL_BASE_PA + 0x1E4)

/* OTP bit control */
#define OTP_ADDR_MASK    	(0x03FF)
#define OTP_PS			(1<<10)
#define OTP_PGENB		(1<<11)
#define OTP_LOAD		(1<<12)
#define OTP_STROBE		(1<<13)
#define OTP_CSB			(1<<14)
#define FUSE_BLOWN  		1
#define OTP_MAC                 (0x79)
/**
 * Timer
 */
#define CFG_TIMERBASE            ((RPSA_BASE_PA) + 0x200)
#define TIMER_PRESCALE_BIT       2
#define TIMER_PRESCALE_1_ENUM    0
#define TIMER_PRESCALE_16_ENUM   1
#define TIMER_PRESCALE_256_ENUM  2
#define TIMER_MODE_BIT           6
#define TIMER_MODE_FREE_RUNNING  0
#define TIMER_MODE_PERIODIC      1
#define TIMER_ENABLE_BIT         7
#define TIMER_ENABLE_DISABLE     0
#define TIMER_ENABLE_ENABLE      1

#define TIMER_PRESCALE_ENUM      (TIMER_PRESCALE_16_ENUM)
/* The divisor should match TIMER_PRESCALE_ENUM */
#define CFG_HZ                   ((RPSCLK) / 16) 

/**
 * GPIO
 */
#define GPIO_1_IN       ((GPIO_1_PA) + 0x0)
#define GPIO_1_OE       ((GPIO_1_PA) + 0x4)
#define GPIO_1_SET_OE   ((GPIO_1_PA) + 0x1C)
#define GPIO_1_CLR_OE   ((GPIO_1_PA) + 0x20)

#define GPIO_1_OUT      ((GPIO_1_PA) + 0x10)
#define GPIO_1_SET      ((GPIO_1_PA) + 0x14)
#define GPIO_1_CLR      ((GPIO_1_PA) + 0x18)

#define GPIO_1_DBC      ((GPIO_1_PA) + 0x24)

#define GPIO_1_PULL     ((GPIO_1_PA) + 0x50)
#define GPIO_1_DRN      ((GPIO_1_PA) + 0x54)

#define GPIO_2_IN       ((GPIO_2_PA) + 0x0)
#define GPIO_2_OE       ((GPIO_2_PA) + 0x4)
#define GPIO_2_SET_OE   ((GPIO_2_PA) + 0x1C)
#define GPIO_2_CLR_OE   ((GPIO_2_PA) + 0x20)

#define GPIO_2_OUT      ((GPIO_2_PA) + 0x10)
#define GPIO_2_SET      ((GPIO_2_PA) + 0x14)
#define GPIO_2_CLR      ((GPIO_2_PA) + 0x18)

/**
 * Serial Configuration
 */
#define UART_1_BASE (0x44200000)
#define UART_2_BASE (0x44300000)

#define CFG_NS16550          1
#define CFG_NS16550_SERIAL   1
#define CFG_NS16550_REG_SIZE 1

#define CFG_NS16550_CLK      (RPSCLK)
#define USE_UART_FRACTIONAL_DIVIDER

#if (INTERNAL_UART == 1)
#define CONFIG_OXNAS_UART1
#define CFG_NS16550_COM1     (UART_1_BASE)
//Michael
#define CFG_NS16550_COM2     (UART_2_BASE)
#elif (INTERNAL_UART == 2)
#define CONFIG_OXNAS_UART2
#define CFG_NS16550_COM1     (UART_2_BASE)
#elif (INTERNAL_UART == 3)
    #error Only UARTs 1 and 2
#else
#define CFG_NS16550_COM1     (UART_1_BASE)
#endif // CONFIG_OXNAS_UART

#define CONFIG_CONS_INDEX    1
#define CONFIG_BAUDRATE      115200
#define CFG_BAUDRATE_TABLE   { 9600, 19200, 38400, 57600, 115200 }

/* UART_A multi function pins (in sys ctrl core)*/
#define UART_1_SIN_MF_PIN              30
#define UART_1_SOUT_MF_PIN             31

/* UART_B multi function pins (in sec ctrl core) */
#ifdef CONFIG_7715
#define UART_2_SIN_MF_PIN              7
#define UART_2_SOUT_MF_PIN             8
#else
#define UART_2_SIN_MF_PIN              14
#define UART_2_SOUT_MF_PIN             15
#endif

/* Eth A multi function pins */
#define MACA_MDC_MF_PIN                3
#define MACA_MDIO_MF_PIN               4
//#define MACA_COL_MF_PIN                5
//#define MACA_CRS_MF_PIN                6
//#define MACA_TXER_MF_PIN               7
//#define MACA_RXER_MF_PIN               8

/**
 * Monitor commands
 */

#define BASE_COMMANDS (CFG_CMD_IMI    | \
                       CFG_CMD_BDI    | \
                       CFG_CMD_NET    | \
                       CFG_CMD_PING   | \
                       CFG_CMD_ENV    | \
                       CFG_CMD_RUN    | \
                       CFG_CMD_MEMORY)

#define SERIAL_COMMANDS (BASE_COMMANDS)

#ifdef CFG_NO_FLASH
#define FLASH_COMMANDS (SERIAL_COMMANDS)
#else
#define FLASH_COMMANDS (SERIAL_COMMANDS | CFG_CMD_FLASH)
#endif // CFG_NO_FLASH


#ifdef CONFIG_OXNAS_USE_SATA
#define SATA_COMMANDS (FLASH_COMMANDS | CFG_CMD_IDE | CFG_CMD_EXT2 | CFG_CMD_FAT | CFG_CMD_LEDFAIL)
#else
#define SATA_COMMANDS  FLASH_COMMANDS 
#endif // CONFIG_OXNAS_USE_SATA

#ifdef CONFIG_CMD_NAND
#define NAND_COMMANDS (SATA_COMMANDS | CFG_CMD_NAND | CFG_CMD_LEDFAIL)
#else
#define NAND_COMMANDS (SATA_COMMANDS)
#endif // CONFIG_CMD_NAND

#ifdef CONFIG_CMD_OTP
#define OTP_COMMANDS (NAND_COMMANDS | CFG_CMD_OTP)
#else
#define OTP_COMMANDS (NAND_COMMANDS)
#endif // CFG_OTP_MEMORY

#ifdef CONFIG_CMD_SPI
#define SPI_COMMANDS (OTP_COMMANDS | CFG_CMD_SPI)
#else
#define SPI_COMMANDS (OTP_COMMANDS)
#endif

#ifdef CONFIG_CMD_SF
#define SF_COMMANDS (SPI_COMMANDS | CFG_CMD_LEDFAIL)
#else
#define SF_COMMANDS (SPI_COMMANDS)
#endif

#ifdef CONFIG_PROG_BTN_GPIO
#define PROG_COMMANDS (SF_COMMANDS | CFG_CMD_PROG) 
#else
#define PROG_COMMANDS (SF_COMMANDS)
#endif

#define CONFIG_COMMANDS PROG_COMMANDS

/* This must be included AFTER the definition of CONFIG_COMMANDS */
#include <cmd_confdefs.h>

/**
 * Booting
 */
#if (LINUX_ROOT_RAIDED == 1) && !defined(USE_NAND)
#define LINUX_ROOT_DEVICE "root=/dev/md1"
#else
#define LINUX_ROOT_DEVICE "root=/dev/sda1"
#endif
#define COMMON_BOOTARGS " console=ttyS0,115200 elevator=cfq mac_adr=0x00,0x30,0xe0,0x00,0x00,0x01"
#if (USE_SATA_ENV==1)
#define CONFIG_BOOTARGS LINUX_ROOT_DEVICE COMMON_BOOTARGS
#endif
#if defined(CONFIG_OXNAS_USE_SATA) && (USE_SATA_ENV==1)
#define CONFIG_BOOTDELAY	2
#define CONFIG_BOOTCOMMAND	"run select0 load boot || run select0 load2 boot || run lightled select1 load extinguishled boot || run lightled select1 load2 extinguishled boot || lightled"
#define CONFIG_EXTRA_ENV_SETTINGS \
    "select0=ide dev 0\0" \
    "select1=ide dev 1\0" \
    "load=ide read 0x60500000 50a 1644\0" \
    "load2=ide read 0x60500000 e3e8 1644\0" \
	"lightled=ledfail 1\0" \
	"extinguishled=ledfail 0\0" \
    "boot=bootm 60500000\0"
   
/* converts 'x' to a string _without_ evaluating x */
#define STRINGIFY(x)    #x
    
/**
 * Disk stored flags that control entry into upgrade mode, recovery mode 
 * and controlled power down. 
 */
#define MODE_FLAG_LOCATION  0x60700000

#define UPGRADE_FS_LOCATION     0x60700000
#define UPGRADE_KERNEL_LOCATION 0x60800000

#elif defined(SPI_DEFENV)
#define MODE_FLAG_LOCATION  0x60500000
#define UPGRADE_KERNEL_LOCATION 0x60500000
#define CONFIG_BOOTARGS "root=/dev/ram0" COMMON_BOOTARGS
#define CONFIG_BOOTDELAY    2
#define CONFIG_BOOTCOMMAND  "run extinguishled boot_spi"
#define CONFIG_EXTRA_ENV_SETTINGS \
    "probe_spi=sf probe 0 \0" \
    "load_spi=sf read 60500000 88000 270000\0" \
    "load_spi2=sf read 61000000 388000 1C0000\0" \
    "lightled=ledfail 1\0" \
    "extinguishled=ledfail 0\0" \
    "boot=bootm 60500000 61000000\0" \
    "boot_spi=run probe_spi load_spi load_spi2 boot || run lightled\0"
#elif defined(NAND_DEFENV)
#define XSTRINGIFY(x)   STRINGIFY(x)
#define STRINGIFY(x)    #x
#define MODE_FLAG_LOCATION  0x60500000
#define UPGRADE_KERNEL_LOCATION 0x60500000
#define CONFIG_BOOTARGS "root=ubi0:rootfs ubi.mtd=6,512 rootfstype=ubifs rootwait " COMMON_BOOTARGS
#define CONFIG_BOOTDELAY    1
#define CONFIG_BOOTCOMMAND  "run lightled  root_sata boot_sata || run root_nand boot_nand || run extinguishled"
#define CONFIG_EXTRA_ENV_SETTINGS \
    "load_nand=nboot 0x60500000 0 0x200000\0" \
    "load_nand2=nboot 0x60500000 0 0x440000\0" \
    "lightled=ledfail 1\0" \
    "extinguishled=ledfail 0\0" \
    "boot=bootm 60500000\0" \
    "root_dev=ubi0:rootfs \0" \
    "root_nand=setenv bootargs root=ubi0:rootfs ubi.mtd=6,512 rootfstype=ubifs rootwait console=ttyS0,115200  mac_adr=0x00,0x30,0xe0,0x00,0x00,0x01 mem=128M poweroutage=yes\0" \
    "root_sata=setenv bootargs root=/dev/sda2 ubi.mtd=6,512 rootwait console=ttyS0,115200 mac_adr=0x00,0x30,0xe0,0x00,0x00,0x01 mem=128M poweroutage=yes\0" \
    "fload1=fatload ide 0:1 0x60500000 /boot/uImage \0" \
    "fload2=ext2load ide 0:1 0x60500000 /boot/uImage \0" \
    "load_hdd=diskboot 0x60500000 0:1\0" \
    "load_hdd2=diskboot 0x60500000 0:2\0" \
    "load_hdd3=diskboot 0x60500000 0:3\0"  \
    "reset_sata=ide reset\0" \
    "select0=ide dev 0\0" \
    "select1=ide dev 1\0" \
    "boot_nand=run load_nand boot || run load_nand2 boot\0" \
    "boot_sata=run reset_sata select0 fload1 boot || run reset_sata select0 fload2 boot || run reset_sata select0  load_hdd boot || run reset_sata select0 load_hdd2 boot\0" 
    
#else // CONFIG_OXNAS_USE_SATA

/* command for booting from flash */
#define CONFIG_BOOTDELAY	15
#define CONFIG_BOOTCOMMAND	"bootm 0x41020000"
#endif // CONFIG_OXNAS_USE_SATA

//#define CONFIG_SHOW_BOOT_PROGRESS   1

/**
 * Networking
 */
#define CONFIG_ETHADDR      00:30:e0:00:00:01
#define CONFIG_NETMASK      255.255.0.0
#define CONFIG_IPADDR       192.168.1.200
#define CONFIG_SERVERIP     192.168.1.100
#define CONFIG_BOOTFILE     "uImage"
#define CFG_AUTOLOAD        "n"
#define CONFIG_NET_RETRY_COUNT 30

/**
 * Flash support
 */
#define STATIC_BUS_FLASH_CONFIG 0x4f1f3f3f  /* Slow ASIC settings */

#ifndef CFG_NO_FLASH

#define FORCE_TOP_BOOT_FLASH	1

#define CFG_FLASH_CFI			1
#define CFG_FLASH_CFI_DRIVER	1

#define NUM_FLASH_MAIN_BLOCKS   63          /* For Intel 28F320B3T */
#define NUM_FLASH_PARAM_BLOCKS  8           /* For Intel 28F320B3T */
#define FLASH_MAIN_BLOCK_SIZE   (64*1024)   /* For Intel 28F320B3T family */
#define FLASH_PARAM_BLOCK_SIZE  (8*1024)    /* For Intel 28F320B3T family */

/* Assuming counts main blocks and parameter blocks, as the Intel/AMD detection */
/* I'm intending to copy would seem to indicate */
#define CFG_MAX_FLASH_SECT      (NUM_FLASH_MAIN_BLOCKS + NUM_FLASH_PARAM_BLOCKS)

#define CFG_MAX_FLASH_BANKS     1           /* Assume counts flash devices */
#define FLASH_BASE_OFF          0
#define CFG_FLASH_BASE          ((STATIC_CS0_BASE_PA) + (FLASH_BASE_OFF))
#define PHYS_FLASH_1            (CFG_FLASH_BASE)

#define CFG_FLASH_ERASE_TOUT    (20*CFG_HZ)	/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT    (20*CFG_HZ)	/* Timeout for Flash Write */
#define CFG_FLASH_WRITE_ATTEMPTS 5


#endif // !CFG_NO_FLASH

/**
 * Environment organization
 */
#ifdef ENV_ON_SATA

/* Environment on SATA disk */
#define SIZE_TO_SECTORS(x) ((x) / 512)
#define UBOOT_DISK_ALLOTMENT (256 * 1024)
#define CFG_ENV_IS_IN_DISK
#define CFG_ENV_SIZE			(8*1024)
#define ENVIRONMENT_OFFSET		((UBOOT_DISK_ALLOTMENT) - (CFG_ENV_SIZE) - 1024)
#define CFG_ENV_ADDR			((CFG_SRAM_BASE) + (ENVIRONMENT_OFFSET))
#define ROM_LOADER_LOAD_START_SECTOR 64
#define CFG_ENV_DISK_SECTOR 	((ROM_LOADER_LOAD_START_SECTOR) + SIZE_TO_SECTORS(ENVIRONMENT_OFFSET))
#define ROM_LOADER_LOAD_REDUNDANT_START_SECTOR 57088
#define CFG_ENV_DISK_REDUNDANT_SECTOR ((ROM_LOADER_LOAD_REDUNDANT_START_SECTOR) + SIZE_TO_SECTORS(ENVIRONMENT_OFFSET))

#elif defined(ENV_ON_SPI)
#define CFG_ENV_IS_IN_SPI
/* Environment in flash device parameter blocks */
#define CFG_ENV_SECT_SIZE   (8*1024)
/* First parameter block for environment */
#define CFG_ENV_SIZE    CFG_ENV_SECT_SIZE
#define CFG_ENV_OFFSET 0x8000 // 0x10000//STMICRO M25P32 SPI, the erase size is 64KB.//the ENV addrese = Flash size - CFG_ENV_OFFSET

#elif defined(ENV_ON_NAND)
#define CFG_ENV_IS_IN_NAND
/* Environment in flash device parameter blocks */
#define CFG_ENV_SECT_SIZE   (8*1024)
/* First parameter block for environment */
#define CFG_ENV_OFFSET			0x1a0000
#define CFG_ENV_SIZE			CFG_ENV_SECT_SIZE
#define CFG_NAND_ENV_USE_RS_ECC
#define CFG_NAND_ENV_NSPARE_BLOCKS	2
#define CFG_ENV_BAD_BLOCKS 		2
#define CFG_ENV_NAND_REDUNDANT_OFFSET  0x1c0000// 236 //0x1180000//the 140th block
#define CFG_ENV_NAND_REDUNDANT_REGION   2

#else // ENV_ON_NAND

#if (USE_FLASH == 1)
/** Flash based environment
 *
 *  It appears that all flash env start/size info. has to be pre-defined. How
 *  this is supposed to work when the flash detection code could cope with all
 *  sorts of different flash is hard to see.
 *  It appears from the README that with bottom/top boot flashes with smaller
 *  parameter blocks available, the environment code will only use a single
 *  one of these smaller sectors for the environment, i.e. CFG_ENV_SECT_SIZE
 *  is the size of the environment. I hope this isn't really true. The defines
 *  below may well not work if this is the truth
 */
#define CFG_ENV_IS_IN_FLASH
#endif // (USE_FLASH == 1)

/* Environment in flash device parameter blocks */
#define CFG_ENV_SECT_SIZE   (8*1024)
/* First parameter block for environment */
#define CFG_ENV_SIZE        CFG_ENV_SECT_SIZE
/* Second parameter block for backup environment */
#define CFG_ENV_SIZE_REDUND (CFG_ENV_SIZE)
/* Main environment occupies first parameter block */
#define CFG_ENV_ADDR        ((CFG_FLASH_BASE)+((NUM_FLASH_MAIN_BLOCKS)*(FLASH_MAIN_BLOCK_SIZE)))
/* Backup environment occupies second parameter block */
#define CFG_ENV_ADDR_REDUND ((CFG_ENV_ADDR)+(CFG_ENV_SIZE))
#endif 


#define CONFIG_ENV_OVERWRITE

/* Magic number that indicates rebooting into upgrade mode */
#define UPGRADE_MAGIC 0x31	/* ASCII '1' */

/* Magic number that indicates user recovery on reboot */
/* Also defined in oxnas_user_recovery.agent */
#define RECOVERY_MAGIC 0x31    /* ASCII '1' */

/* Magic number that indicates controlled power down on reboot */
/* Also defined in controlled_power_down.sh in init.d */
#define CONTROLLED_POWER_DOWN_MAGIC 0x31  /* ASCII '1' */

/* This flag is set in SRAM location by Co Proc */
#define CONTROLLED_POWER_UP_MAGIC  0x31 /* ASCII '1' */
/* 9k + a quad from top */
/* Be carefule on changing the location of this flag
 * u-boot has other things to write in SRAM too
 */
#define POWER_ON_FLAG_SRAM_OFFSET 	9220
#if (USE_LEON_TIME_COUNT == 1)
#define MS_TIME_COUNT_SRAM_OFFSET	(POWER_ON_FLAG_SRAM_OFFSET + 4) 
#endif

/* Size of malloc() pool */
#define CFG_MALLOC_LEN      (CFG_ENV_SIZE + 128*1024)
#define CFG_GBL_DATA_SIZE   128 /* size in bytes reserved for initial data */

/**
 * ASM startup control
 */
/* Start of address within SRAM of loader's exception table. */
/* ROM-based exception table will redirect to here */
#define EXCEPTION_BASE  (CFG_SRAM_BASE)

/*
 * NAND FLash support
 */

/*
#define	NAND_ChipID_UNKNOWN	0
#define	CFG_MAX_NAND_DEVICE	1
#define	NAND_MAX_FLOORS	1
#define	NAND_MAX_CHIPS	1
#define	SECTORSIZE	(2 * 1024)
#define	ADDR_COLUMN	1
#define	ADDR_PAGE	2
#define	ADDR_COLUMN_PAGE	3
#define	CFG_NAND_BASE	STATIC_CS1_BASE_PA
#define	NAND_WAIT_READY(nand)	udelay(25);
#define	NAND_CTL_CLRALE(n)
#define	NAND_CTL_SETALE(n)
#define	NAND_CTL_CLRCLE(n)
#define	NAND_CTL_SETCLE(n)
#define	NAND_ENABLE_CE(n)
#define	NAND_DISABLE_CE(n)
#define	WRITE_NAND_COMMAND(d,adr)	*(volatile __u8 *)((unsigned long)adr + 0x4000) = (__u8)(d)
#define	WRITE_NAND_ADDRESS(d,adr)	*(volatile __u8 *)((unsigned long)adr + 0x8000) = (__u8)(d)
#define WRITE_NAND(d,adr)	*(volatile __u8 *)((unsigned long)adr) = (__u8)d
#define READ_NAND(adr)	((volatile unsigned char)(*(volatile __u8 *)(unsigned long)adr))
*/
#define	CFG_NAND_BASE	STATIC_CS0_BASE_PA
#define	CFG_NAND_ADDRESS_LATCH	CFG_NAND_BASE + (1<<18)
#define	CFG_NAND_COMMAND_LATCH	CFG_NAND_BASE + (1<<19)

#define	CFG_MAX_NAND_DEVICE	1


#define	writew(v,a)	*(volatile __u16 *)((unsigned long)a) = (__u16)v
#define	readw(a)	((volatile __u16)(*(volatile __u8 *)(unsigned long)a))
#define	show_boot_progress(n)

/**
 * Disk related stuff
 */
//#define CONFIG_LBA48
#define CONFIG_DOS_PARTITION
#define CFG_IDE_MAXDEVICE   2
#define CFG_IDE_MAXBUS      1
#define CONFIG_IDE_PREINIT
#undef CONFIG_IDE_RESET
#undef CONFIG_IDE_LED
#define CFG_ATA_DATA_OFFSET 0
#define CFG_ATA_REG_OFFSET  0
#define CFG_ATA_ALT_OFFSET  0

/**
 * System block reset and clock control
 */
#define SYS_CTRL_PCI_STAT             (SYS_CONTROL_BASE_PA + 0x20)
#define SYS_CTRL_CKEN_SET_CTRL        (SYS_CONTROL_BASE_PA + 0x2C)
#define SYS_CTRL_CKEN_CLR_CTRL        (SYS_CONTROL_BASE_PA + 0x30)
#define SYS_CTRL_RSTEN_SET_CTRL       (SYS_CONTROL_BASE_PA + 0x34)
#define SYS_CTRL_RSTEN_CLR_CTRL       (SYS_CONTROL_BASE_PA + 0x38)
#define SYS_CTRL_PLLSYS_CTRL          (SYS_CONTROL_BASE_PA + 0x48)
#define SYS_CTRL_PLLSYS_KEY_CTRL      (SYS_CONTROL_BASE_PA + 0x6C)
#define SYS_CTRL_GMAC_CTRL            (SYS_CONTROL_BASE_PA + 0x78)

/* System control multi-function pin function selection */
#define SYS_CTRL_SECONDARY_SEL        (SYS_CONTROL_BASE_PA + 0x14)
#define SYS_CTRL_TERTIARY_SEL         (SYS_CONTROL_BASE_PA + 0x8c)
#define SYS_CTRL_QUATERNARY_SEL       (SYS_CONTROL_BASE_PA + 0x94)
#define SYS_CTRL_DEBUG_SEL            (SYS_CONTROL_BASE_PA + 0x9c)
#define SYS_CTRL_ALTERNATIVE_SEL      (SYS_CONTROL_BASE_PA + 0xa4)
#define SYS_CTRL_PULLUP_SEL           (SYS_CONTROL_BASE_PA + 0xac)

/* Scratch registers */
#define SYS_CTRL_SCRATCHWORD0         (SYS_CONTROL_BASE_PA + 0xc4)
#define SYS_CTRL_SCRATCHWORD1         (SYS_CONTROL_BASE_PA + 0xc8)
#define SYS_CTRL_SCRATCHWORD2         (SYS_CONTROL_BASE_PA + 0xcc)
#define SYS_CTRL_SCRATCHWORD3         (SYS_CONTROL_BASE_PA + 0xd0)

#define SEC_CTRL_SECONDARY_SEL        (SEC_CONTROL_BASE_PA + 0x14)
#define SEC_CTRL_TERTIARY_SEL         (SEC_CONTROL_BASE_PA + 0x8c)
#define SEC_CTRL_QUATERNARY_SEL       (SEC_CONTROL_BASE_PA + 0x94)
#define SEC_CTRL_DEBUG_SEL            (SEC_CONTROL_BASE_PA + 0x9c)
#define SEC_CTRL_ALTERNATIVE_SEL      (SEC_CONTROL_BASE_PA + 0xa4)
#define SEC_CTRL_PULLUP_SEL           (SEC_CONTROL_BASE_PA + 0xac)

/* bit numbers of clock control register */
#define SYS_CTRL_CKEN_COPRO_BIT  0
#define SYS_CTRL_CKEN_DMA_BIT    1
#define SYS_CTRL_CKEN_CIPHER_BIT 2
#define SYS_CTRL_CKEN_SD_BIT     3
#define SYS_CTRL_CKEN_SATA_BIT   4
#define SYS_CTRL_CKEN_I2S_BIT    5
#define SYS_CTRL_CKEN_USBHS_BIT  6
#define SYS_CTRL_CKEN_MACA_BIT   7
#define SYS_CTRL_CKEN_MAC_BIT   SYS_CTRL_CKEN_MACA_BIT
#define SYS_CTRL_CKEN_PCIEA_BIT  8
#define SYS_CTRL_CKEN_STATIC_BIT 9
#define SYS_CTRL_CKEN_MACB_BIT   10
#define SYS_CTRL_CKEN_PCIEB_BIT  11
#define SYS_CTRL_CKEN_REF600_BIT 12
#define SYS_CTRL_CKEN_USBDEV_BIT 13
#define SYS_CTRL_CKEN_DDR_BIT    14
#define SYS_CTRL_CKEN_DDRPHY_BIT 15
#define SYS_CTRL_CKEN_DDRCK_BIT  16

/* bit numbers of reset control register */
#define SYS_CTRL_RSTEN_SCU_BIT          0
#define SYS_CTRL_RSTEN_COPRO_BIT        1
#define SYS_CTRL_RSTEN_ARM0_BIT         2
#define SYS_CTRL_RSTEN_ARM1_BIT         3
#define SYS_CTRL_RSTEN_USBHS_BIT        4
#define SYS_CTRL_RSTEN_USBHSPHYA_BIT    5
#define SYS_CTRL_RSTEN_MACA_BIT         6
#define SYS_CTRL_RSTEN_MAC_BIT	SYS_CTRL_RSTEN_MACA_BIT
#define SYS_CTRL_RSTEN_PCIEA_BIT        7
#define SYS_CTRL_RSTEN_SGDMA_BIT        8
#define SYS_CTRL_RSTEN_CIPHER_BIT       9
#define SYS_CTRL_RSTEN_DDR_BIT          10
#define SYS_CTRL_RSTEN_SATA_BIT         11
#define SYS_CTRL_RSTEN_SATA_LINK_BIT    12
#define SYS_CTRL_RSTEN_SATA_PHY_BIT     13
#define SYS_CTRL_RSTEN_PCIEPHY_BIT      14
#define SYS_CTRL_RSTEN_STATIC_BIT       15
#define SYS_CTRL_RSTEN_GPIO_BIT         16
#define SYS_CTRL_RSTEN_UART1_BIT        17
#define SYS_CTRL_RSTEN_UART2_BIT        18
#define SYS_CTRL_RSTEN_MISC_BIT         19
#define SYS_CTRL_RSTEN_I2S_BIT          20
#define SYS_CTRL_RSTEN_SD_BIT           21
#define SYS_CTRL_RSTEN_MACB_BIT         22
#define SYS_CTRL_RSTEN_PCIEB_BIT        23
#define SYS_CTRL_RSTEN_VIDEO_BIT        24
#define SYS_CTRL_RSTEN_DDR_PHY_BIT      25
#define SYS_CTRL_RSTEN_USBHSPHYB_BIT    26
#define SYS_CTRL_RSTEN_USBDEV_BIT       27
#define SYS_CTRL_RSTEN_ARMDBG_BIT       29
#define SYS_CTRL_RSTEN_PLLA_BIT         30
#define SYS_CTRL_RSTEN_PLLB_BIT         31

#define SYS_CTRL_GMAC_AUTOSPEED     3
#define SYS_CTRL_GMAC_RGMII         2
#define SYS_CTRL_GMAC_SIMPLE_MAX    1
#define SYS_CTRL_GMAC_CKEN_GTX      0

#define SYS_CTRL_CKCTRL_CTRL_ADDR     (SYS_CONTROL_BASE_PA + 0x64)

#define SYS_CTRL_CKCTRL_PCI_DIV_BIT 0
#define SYS_CTRL_CKCTRL_SLOW_BIT    8

#define SYS_CTRL_UART2_DEQ_EN       0
#define SYS_CTRL_UART3_DEQ_EN       1
#define SYS_CTRL_UART3_IQ_EN        2
#define SYS_CTRL_UART4_IQ_EN        3
#define SYS_CTRL_UART4_NOT_PCI_MODE 4

#define SYS_CTRL_PCI_CTRL1_PCI_STATIC_RQ_BIT 11

/**
 * SATA related definitions
 */
#define ATA_PORT_CTL        0
#define ATA_PORT_FEATURE    1
#define ATA_PORT_NSECT      2
#define ATA_PORT_LBAL       3
#define ATA_PORT_LBAM       4
#define ATA_PORT_LBAH       5
#define ATA_PORT_DEVICE     6
#define ATA_PORT_COMMAND    7

#define SATA_0_REGS_BASE        (0x45900000)
#define SATA_1_REGS_BASE        (0x45910000)
#define SATA_DMA_REGS_BASE      (0x459a0000)
#define SATA_SGDMA_REGS_BASE    (0x459b0000)
#define SATA_HOST_REGS_BASE     (0x459e0000)

/* The offsets to the SATA registers */
#define SATA_ORB1_OFF           0
#define SATA_ORB2_OFF           1
#define SATA_ORB3_OFF           2
#define SATA_ORB4_OFF           3
#define SATA_ORB5_OFF           4

#define SATA_FIS_ACCESS         11
#define SATA_INT_STATUS_OFF     12  /* Read only */
#define SATA_INT_CLR_OFF        12  /* Write only */
#define SATA_INT_ENABLE_OFF     13  /* Read only */
#define SATA_INT_ENABLE_SET_OFF 13  /* Write only */
#define SATA_INT_ENABLE_CLR_OFF 14  /* Write only */
#define SATA_VERSION_OFF        15
#define SATA_CONTROL_OFF        23
#define SATA_COMMAND_OFF        24
#define SATA_PORT_CONTROL_OFF   25
#define SATA_DRIVE_CONTROL_OFF  26

#define SATACORE_DM_DBG1         0
#define SATACORE_RAID_SET        1
#define SATACORE_DM_DBG2         2
#define SATACORE_DATACOUNT_PORT0 4
#define SATACORE_DATACOUNT_PORT1 5
#define SATACORE_DATA_MUX_RAM0	 0x2000
#define SATACORE_DATA_MUX_RAM1	 0x2800

/* The offsets to the link registers that are access in an asynchronous manner */
#define SATA_LINK_DATA     28
#define SATA_LINK_RD_ADDR  29
#define SATA_LINK_WR_ADDR  30
#define SATA_LINK_CONTROL  31

/* SATA interrupt status register fields */
#define SATA_INT_STATUS_EOC_RAW_BIT     ( 0 + 16) 
#define SATA_INT_STATUS_ERROR_BIT       ( 2 + 16)
#define SATA_INT_STATUS_EOADT_RAW_BIT   ( 1 + 16)

/* SATA core command register commands */
#define SATA_CMD_WRITE_TO_ORB_REGS              2
#define SATA_CMD_WRITE_TO_ORB_REGS_NO_COMMAND   4

#define SATA_CMD_BUSY_BIT 7

#define SATA_SCTL_CLR_ERR 0x00000316UL

#define SATA_OPCODE_MASK 0x3

#define SATA_LBAL_BIT    0
#define SATA_LBAM_BIT    8
#define SATA_LBAH_BIT    16
#define SATA_HOB_LBAH_BIT 24
#define SATA_DEVICE_BIT  24
#define SATA_NSECT_BIT   0
#define SATA_FEATURE_BIT 16
#define SATA_COMMAND_BIT 24
#define SATA_CTL_BIT     24

/* ATA status (7) register field definitions */
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

#define SATA_STD_ASYNC_REGS_OFF 0x20
#define SATA_SCR_STATUS      0
#define SATA_SCR_ERROR       1
#define SATA_SCR_CONTROL     2
#define SATA_SCR_ACTIVE      3
#define SATA_SCR_NOTIFICAION 4

#define SATA_BURST_BUF_FORCE_EOT_BIT        0
#define SATA_BURST_BUF_DATA_INJ_ENABLE_BIT  1
#define SATA_BURST_BUF_DIR_BIT              2
#define SATA_BURST_BUF_DATA_INJ_END_BIT     3
#define SATA_BURST_BUF_FIFO_DIS_BIT         4
#define SATA_BURST_BUF_DIS_DREQ_BIT         5
#define SATA_BURST_BUF_DREQ_BIT             6

/* Button on GPIO 32 */
#define RECOVERY_BUTTON         (0x00000001 << 0)
#define RECOVERY_PRISEL_REG     SYS_CTRL_GPIO_PRIMSEL_CTRL_1
#define RECOVERY_SECSEL_REG     SYS_CTRL_GPIO_SECSEL_CTRL_1
#define RECOVERY_TERSEL_REG     SYS_CTRL_GPIO_TERTSEL_CTRL_1
#define RECOVERY_CLR_OE_REG     GPIO_2_CLR_OE
#define RECOVERY_DEBOUNCE_REG   GPIO_2_INPUT_DEBOUNCE_ENABLE
#define RECOVERY_DATA           GPIO_2_PA

#if defined (CONFIG_SPI)

/****
 * SD combo connected SPI flash memory.
 */
#define SD_BASE         (0x45400000)
#define SD_CONFIG 		(SD_BASE + 0x00)
#define SD_COMMAND      (SD_BASE + 0x04)  
#define SD_ARGUMENT     (SD_BASE + 0x08)
#define SD_DATA_SIZE    (SD_BASE + 0x0C)
#define SD_STATUS       (SD_BASE + 0x1C)
#define SD_RESPONSE1    (SD_BASE + 0x24)
#define SD_RESPONSE2    (SD_BASE + 0x28)

#define SD_FIFO_PORT    (SD_BASE + 0x40)
#define SD_FIFO_STATUS  (SD_BASE + 0x48)
#define SD_CLOCK_CTRL   (SD_BASE + 0x50)
#define SD_BANK0_CONFIG (SD_BASE + 0x60)

#define SD_NOR_ENABLE0 0x000001e0	/* only enable clk, dat in out and cs. */

#define SD_SPI_CONFIG  0x00018300 /* bank 0 in spi mode */
#define SD_SPI_PORT    0x00001408 /* bank 0 gap length 7 cycles */
#define SD_CLOCK_STOP  0x80000000
#define SD_CLOCK_RUN   0x0000000e /* 12.5MHz at 200MHz clock. */

/* command control data */
#define ID_DATA_SIZE   0x00010004
#define GET_ID1_CMD    0x904A0401   /* command 90 request 2 bytes returned */
#define GET_ID2_CMD    0x9F4A0401   /* command 9F request 3 bytes returned */
#define GET_ID3_CMD    0xAB4A0401   /* command AB request 2 bytes returned */
#define SD_CMD0        0x40010601
#define SD_CMD8		   0x48010601
#define SD_CMD55       0XB7010601
#define SD_ACMD41      0xA9010601
#define SD_CMD58       0xBA030601
#define SD_FLASH_READ  0x03400401
#define SD_READ_BLOCK  0x51410601

#define SD_IDLE_BIT    (1 <<  4)

#endif
/****
 * communicating with the eeprom using bit bang spi driver.
 */
#if defined(CONFIG_SOFT_SPI)

//#undef DEBUG_SPI

/*
 * Software (bit-bang) SPI driver configuration
 */
#define SPI_CS          0x00000100      /*  spi device Chip select */
#define I2C_SCLK        0x00000020      /* PD 18: Shift clock */
#define I2C_MOSI        0x00000040      /* PD 17: Master Out, Slave In */
#define I2C_MISO        0x00000080      /* PD 16: Master In, Slave Out */

/* not required debounce input? \ */
                 /* port initialization needed */
#define  SPI_INIT       do { writel(SPI_CS, GPIO_1_SET);\
							 (*(u32 *)GPIO_1_OE) |= (SPI_CS | I2C_SCLK | I2C_MOSI);\
							 (*(u32 *)GPIO_1_PULL) |= (SPI_CS | I2C_SCLK | I2C_MOSI);\
							 (*(u32 *)GPIO_1_DRN) |= (SPI_CS | I2C_SCLK | I2C_MOSI);\
							 (*(u32 *)GPIO_1_DBC) &= ~(I2C_MISO);\
						} while (0)
						
#define SPI_READ        (((*(volatile unsigned int *)GPIO_1_PA) & I2C_MISO) == I2C_MISO ? 1 : 0)

#define SPI_SDA(bit)    do {      \
                        if(bit){ (*(volatile u32 *)GPIO_1_SET) = I2C_MOSI; }\
                        else   { (*(volatile u32 *)GPIO_1_CLR) = I2C_MOSI; }\
                        asm volatile ("" : : : "memory");\
                        } while (0)
#define SPI_SCL(bit)    do {                                            \
                        if(bit) (*(volatile u32 *)GPIO_1_SET) = I2C_SCLK; \
                        else    (*(volatile u32 *)GPIO_1_CLR) = I2C_SCLK; \
                        asm volatile ("" : : : "memory");\
                        } while (0)

#define SPI_DELAY       do { volatile int i; for ( i=1 ; i < 0; i--)  ; asm volatile ("" : : : "memory");\
 				} while (0)                 /* No delay is needed */

#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_1

#endif /* CONFIG_SOFT_SPI */


#endif // CONFIG820_H
