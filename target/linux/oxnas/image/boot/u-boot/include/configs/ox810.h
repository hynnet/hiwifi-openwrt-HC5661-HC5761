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

#ifndef __CONFIG810_H
#define __CONFIG810_H

#define readb(p)  (*(volatile u8 *)(p))
#define readl(p)  (*(volatile u32 *)(p))
#define writeb(v, p) (*(volatile u8 *)(p)= (v))
#define writel(v, p) (*(volatile u32*)(p)=(v))

#define CFG_FLASH_EMPTY_INFO

/**
 * Architecture
 */
#define CONFIG_ARM926EJS    1
#define CONFIG_OXNAS        1
#define CONFIG_OXNAS_ENABLE_PCI         /* Enables PCI clock and takes out of reset - needed if require access to static bus */
#define CONFIG_OXNAS_FEEDBACK_PCI_CLKS  /* Feedback PCI clock out 3 to drive PCI core clock - needed if require access to static bus */
#define CONFIG_OXNAS_MANUAL_STATIC_ARBITRATION
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
#define CFG_PROMPT		"$ "		/* Monitor Command Prompt */
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
#define PHYS_SDRAM_1_PA         0x48000000  /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_MAX_SIZE	 (256 * 1024 * 1024)
#define CFG_SRAM_BASE           ((PHYS_SDRAM_1_PA) + (PHYS_SDRAM_1_MAX_SIZE))
#define CFG_SRAM_SIZE			 (128 * 1024)
#define INITIALISE_SDRAM

/*
 * PLLSYS = ((PLLSYS_S << 0) | (PLLSYS_P << 4) | (PLLSYS_M << 16))
 *
 * Default S = 0
 *		   P = 6
 *         M = 176 -> 733.33 MHz
 */
#define OVERCLOCK 12583008	/* S=0, P=6, M=192 -> 800MHz */
//#define OVERCLOCK 12845152	/* S=0, P=6, M=196 -> 816.66MHz */
//#define OVERCLOCK 13107296	/* S=0, P=6, M=200 -> 833.33MHz */
//#define OVERCLOCK 13631584	/* S=0, P=6, M=208 -> 866.66MHz */
//#define OVERCLOCK 14155872	/* S=0, P=6, M=216 -> 900MHz */
//#define OVERCLOCK 14680160	/* S=0, P=6, M=224 -> 933.33MHz */
//#define OVERCLOCK 18350176	/* S=0, P=6, M=280 -> 1000MHz */

/* Default location from which bootm etc will load */
#define CFG_LOAD_ADDR   (PHYS_SDRAM_1_PA)

/**
 * Core addresses
 */
#define MAC_BASE_PA             0x40400000
#define STATIC_CS0_BASE_PA      0x41000000
#define STATIC_CS1_BASE_PA      0x41400000
#define STATIC_CS2_BASE_PA      0x41800000
#define STATIC_CONTROL_BASE_PA  0x41C00000
#define SATA_DATA_BASE_PA       0x42000000

#define APB_BRIDGE_A_BASE_PA    0x44000000
#define APB_BRIDGE_B_BASE_PA    0x45000000

#define GPIO_1_PA               ((APB_BRIDGE_A_BASE_PA) + 0x0)
#define GPIO_2_PA               ((APB_BRIDGE_A_BASE_PA) + 0x100000)

#define SYS_CONTROL_BASE_PA     ((APB_BRIDGE_B_BASE_PA) + 0x0)
#define DMA_BASE_PA             ((APB_BRIDGE_B_BASE_PA) + 0x600000)
#define RPS_BASE                ((APB_BRIDGE_B_BASE_PA) + 0x300000)

/* Static bus registers */
#define STATIC_CONTROL_VERSION  ((STATIC_CONTROL_BASE_PA) + 0x0)
#define STATIC_CONTROL_BANK0    ((STATIC_CONTROL_BASE_PA) + 0x4)
#define STATIC_CONTROL_BANK1    ((STATIC_CONTROL_BASE_PA) + 0x8)
#define STATIC_CONTROL_BANK2    ((STATIC_CONTROL_BASE_PA) + 0xC)

/* Clock to the ARM/DDR */
#if (FPGA == 0)
#define NOMINAL_ARMCLK  ((PLL400) / 2)
#define NOMINAL_SYSCLK  ((PLL400) / 4)
#else // !FPGA
#define NOMINAL_ARMCLK  (FPGA_ARM_CLK)
#define NOMINAL_SYSCLK  ((PLL400) / 4)
#endif // !FPGA

/**
 * Timer
 */
#define CFG_TIMERBASE            ((RPS_BASE) + 0x200)
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

#define TIMER_PRESCALE_ENUM      (TIMER_PRESCALE_256_ENUM)
#define CFG_HZ                   ((RPSCLK) / 256)

/**
 * GPIO
 */
#define GPIO_1_OE       ((GPIO_1_PA) + 0x4)
#define GPIO_1_SET_OE   ((GPIO_1_PA) + 0x1C)
#define GPIO_1_CLR_OE   ((GPIO_1_PA) + 0x20)

#define GPIO_2_OE       ((GPIO_2_PA) + 0x4)
#define GPIO_2_SET_OE   ((GPIO_2_PA) + 0x1C)
#define GPIO_2_CLR_OE   ((GPIO_2_PA) + 0x20)

/**
 * Serial Configuration
 */
#define EXT_UART_BASE       0x28000000

#define UART_1_BASE (APB_BRIDGE_A_BASE_PA + 0x200000)
#define UART_2_BASE (APB_BRIDGE_A_BASE_PA + 0x300000)
#define UART_3_BASE (APB_BRIDGE_A_BASE_PA + 0x900000) 
#define UART_4_BASE (APB_BRIDGE_A_BASE_PA + 0xA00000) 

#define CFG_NS16550          1
#define CFG_NS16550_SERIAL   1
#define CFG_NS16550_REG_SIZE 1

#if (USE_EXTERNAL_UART != 0)
#define CFG_NS16550_CLK      16000000
#define CFG_NS16550_COM1     (EXT_UART_BASE)
#else // USE_EXTERNAL_UART
#define CFG_NS16550_CLK      (NOMINAL_SYSCLK)
#define USE_UART_FRACTIONAL_DIVIDER
#if (INTERNAL_UART == 1)
#define CONFIG_OXNAS_UART1
#define CFG_NS16550_COM1     (UART_1_BASE)
#elif (INTERNAL_UART == 2)
#define CONFIG_OXNAS_UART2
#define CFG_NS16550_COM1     (UART_2_BASE)
#elif (INTERNAL_UART == 3)
#define CONFIG_OXNAS_UART3
#define CFG_NS16550_COM1     (UART_3_BASE)
#else
#define CONFIG_OXNAS_UART4
#define CFG_NS16550_COM1     (UART_4_BASE)
#endif // CONFIG_OXNAS_UART
#endif // USE_EXTERNAL_UART

#define CONFIG_CONS_INDEX    1
#define CONFIG_BAUDRATE      115200
#define CFG_BAUDRATE_TABLE   { 9600, 19200, 38400, 57600, 115200 }

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

#ifdef CFG_NO_FLASH
#define FLASH_COMMANDS (BASE_COMMANDS)
#else
#define FLASH_COMMANDS (BASE_COMMANDS | CFG_CMD_FLASH)
#endif // CFG_NO_FLASH

#ifdef CONFIG_OXNAS_USE_SATA
#define SATA_COMMANDS (FLASH_COMMANDS | CFG_CMD_IDE | CFG_CMD_EXT2 | CFG_CMD_LEDFAIL)
#else
#define SATA_COMMANDS (FLASH_COMMANDS)
#endif // CONFIG_OXNAS_USE_SATA

#define CONFIG_COMMANDS SATA_COMMANDS

/* This must be included AFTER the definition of CONFIG_COMMANDS */
#include <cmd_confdefs.h>

/**
 * Booting
 */
#if (LINUX_ROOT_RAIDED == 1)
#define LINUX_ROOT_DEVICE "root=/dev/md1"
#else
#define LINUX_ROOT_DEVICE "root=/dev/sda1"
#endif
#define CONFIG_BOOTARGS LINUX_ROOT_DEVICE " console=ttyS0,115200 elevator=cfq mac_adr=0x00,0x30,0xe0,0x00,0x00,0x01"

#ifdef CONFIG_OXNAS_USE_SATA
#define CONFIG_BOOTDELAY	2
#define CONFIG_BOOTCOMMAND	"run select0 load boot || run select0 load2 boot || run lightled select1 load extinguishled boot || run lightled select1 load2 extinguishled boot || lightled"
#define CONFIG_EXTRA_ENV_SETTINGS \
    "select0=ide dev 0\0" \
    "select1=ide dev 1\0" \
    "load=ide read 0x48500000 122 1644\0" \
    "load2=ide read 0x48500000 e000 1644\0" \
	"lightled=ledfail 1\0" \
	"extinguishled=ledfail 0\0" \
    "boot=bootm 48500000\0"

/* converts 'x' to a string _without_ evaluating x */
#define STRINGIFY(x)    #x
    
/**
 * Disk stored flags that control entry into upgrade mode, recovery mode 
 * and controlled power down. 
 */
#define MODE_FLAG_LOCATION  0x48700000
    
#define UPGRADE_FS_LOCATION     0x48700000
#define UPGRADE_KERNEL_LOCATION 0x48800000

#else // CONFIG_OXNAS_USE_SATA
#define CONFIG_BOOTDELAY	15
#define CONFIG_BOOTCOMMAND	"bootm 0x41020000"
#endif // CONFIG_OXNAS_USE_SATA

//#define CONFIG_SHOW_BOOT_PROGRESS   1

/**
 * Networking
 */
#define CONFIG_ETHADDR      00:30:e0:00:00:01
#define CONFIG_NETMASK      255.255.0.0
#define CONFIG_IPADDR       172.31.0.128
#define CONFIG_SERVERIP     172.31.0.100
#define CONFIG_BOOTFILE     "uImage"
#define CFG_AUTOLOAD        "n"
#define CONFIG_NET_RETRY_COUNT 30

/**
 * Flash support
 */
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

#define STATIC_BUS_FLASH_CONFIG 0x4f1f3f3f  /* Slow ASIC settings */

#endif // !CFG_NO_FLASH

/**
 * Environment organization
 */
#ifdef ENV_ON_SATA

/* Environment on SATA disk */
#define SIZE_TO_SECTORS(x) ((x) / 512)
#define CFG_ENV_IS_IN_DISK
#define CFG_ENV_SIZE			(8*1024)
#define ENVIRONMENT_OFFSET		((CFG_SRAM_SIZE) - (CFG_ENV_SIZE) - 1024)
#define CFG_ENV_ADDR			((CFG_SRAM_BASE) + (ENVIRONMENT_OFFSET))
#define ROM_LOADER_LOAD_START_SECTOR 34
#define CFG_ENV_DISK_SECTOR 	((ROM_LOADER_LOAD_START_SECTOR) + SIZE_TO_SECTORS(ENVIRONMENT_OFFSET))
#define ROM_LOADER_LOAD_REDUNDANT_START_SECTOR 57088
#define CFG_ENV_DISK_REDUNDANT_SECTOR ((ROM_LOADER_LOAD_REDUNDANT_START_SECTOR) + SIZE_TO_SECTORS(ENVIRONMENT_OFFSET))

#else // ENV_ON_SATA

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
#define SYS_CTRL_USB11_CTRL           (SYS_CONTROL_BASE_PA + 0x00)
#define SYS_CTRL_PCI_CTRL0            (SYS_CONTROL_BASE_PA + 0x04)
#define SYS_CTRL_PCI_CTRL1            (SYS_CONTROL_BASE_PA + 0x08)
#define SYS_CTRL_GPIO_PRIMSEL_CTRL_0  (SYS_CONTROL_BASE_PA + 0x0C)
#define SYS_CTRL_GPIO_PRIMSEL_CTRL_1  (SYS_CONTROL_BASE_PA + 0x10)
#define SYS_CTRL_GPIO_SECSEL_CTRL_0   (SYS_CONTROL_BASE_PA + 0x14)
#define SYS_CTRL_GPIO_SECSEL_CTRL_1   (SYS_CONTROL_BASE_PA + 0x18)
#define SYS_CTRL_GPIO_TERTSEL_CTRL_0  (SYS_CONTROL_BASE_PA + 0x8C)
#define SYS_CTRL_GPIO_TERTSEL_CTRL_1  (SYS_CONTROL_BASE_PA + 0x90)
#define SYS_CTRL_USB11_STAT           (SYS_CONTROL_BASE_PA + 0x1c)
#define SYS_CTRL_PCI_STAT             (SYS_CONTROL_BASE_PA + 0x20)
#define SYS_CTRL_CKEN_SET_CTRL        (SYS_CONTROL_BASE_PA + 0x2C)
#define SYS_CTRL_CKEN_CLR_CTRL        (SYS_CONTROL_BASE_PA + 0x30)
#define SYS_CTRL_RSTEN_SET_CTRL       (SYS_CONTROL_BASE_PA + 0x34)
#define SYS_CTRL_RSTEN_CLR_CTRL       (SYS_CONTROL_BASE_PA + 0x38)
#define SYS_CTRL_PLLSYS_CTRL          (SYS_CONTROL_BASE_PA + 0x48)
#define SYS_CTRL_PLLSYS_KEY_CTRL      (SYS_CONTROL_BASE_PA + 0x6C)
#define SYS_CTRL_GMAC_CTRL            (SYS_CONTROL_BASE_PA + 0x78)
#define SYS_CTRL_UART_CTRL            (SYS_CONTROL_BASE_PA + 0x94)

#define SYS_CTRL_CKEN_COPRO_BIT  0
#define SYS_CTRL_CKEN_DMA_BIT    1
#define SYS_CTRL_CKEN_DPE_BIT    2
#define SYS_CTRL_CKEN_DDR_BIT    3
#define SYS_CTRL_CKEN_SATA_BIT   4
#define SYS_CTRL_CKEN_I2S_BIT    5
#define SYS_CTRL_CKEN_USBHS_BIT  6
#define SYS_CTRL_CKEN_MAC_BIT    7
#define SYS_CTRL_CKEN_PCI_BIT    8
#define SYS_CTRL_CKEN_STATIC_BIT 9
#define SYS_CTRL_CKEN_DDR_PHY_BIT 10

#define SYS_CTRL_RSTEN_ARM_BIT          0
#define SYS_CTRL_RSTEN_COPRO_BIT        1
#define SYS_CTRL_RSTEN_USBHS_BIT        4
#define SYS_CTRL_RSTEN_USBHSPHY_BIT     5
#define SYS_CTRL_RSTEN_MAC_BIT          6
#define SYS_CTRL_RSTEN_PCI_BIT          7
#define SYS_CTRL_RSTEN_DMA_BIT          8
#define SYS_CTRL_RSTEN_DPE_BIT          9
#define SYS_CTRL_RSTEN_DDR_BIT          10
#define SYS_CTRL_RSTEN_SATA_BIT         11
#define SYS_CTRL_RSTEN_SATA_LINK_BIT    12
#define SYS_CTRL_RSTEN_SATA_PHY_BIT     13
#define SYS_CTRL_RSTEN_STATIC_BIT       15
#define SYS_CTRL_RSTEN_GPIO_BIT         16
#define SYS_CTRL_RSTEN_UART1_BIT        17
#define SYS_CTRL_RSTEN_UART2_BIT        18
#define SYS_CTRL_RSTEN_MISC_BIT         19
#define SYS_CTRL_RSTEN_I2S_BIT          20
#define SYS_CTRL_RSTEN_AHB_MON_BIT      21
#define SYS_CTRL_RSTEN_UART3_BIT        22
#define SYS_CTRL_RSTEN_UART4_BIT        23
#define SYS_CTRL_RSTEN_SGDMA_BIT        24
#define SYS_CTRL_RSTEN_DDR_PHY_BIT      25
#define SYS_CTRL_RSTEN_BUS_BIT          31

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

#define SATA_0_REGS_BASE    (APB_BRIDGE_B_BASE_PA + 0x900000)
#define SATA_1_REGS_BASE    (APB_BRIDGE_B_BASE_PA + 0x910000)
#define SATA_HOST_REGS_BASE (APB_BRIDGE_B_BASE_PA + 0x9e0000)

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

#endif // CONFIG810_H
