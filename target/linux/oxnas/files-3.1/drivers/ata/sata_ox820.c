/**************************************************************************
 * based on ox820 sata code by:
 *  Copyright (c) 2007 Oxford Semiconductor Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  Module Name:
 *      ox820sata.c
 *
 *  Abstract:
 *      A driver to interface the 934 based sata core present in the ox820
 *      with libata and scsi
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/ata.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/raid/md_p.h>
#include <linux/raid/md_u.h>
#include <linux/leds.h>

#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <asm/io.h>


#include <mach/hardware.h>
#include <mach/desc_alloc.h>
#include <mach/memory.h>
#include <mach/ox820sata.h>

#include <linux/libata.h>
#include "libata.h"


/***************************************************************************
* CONSTANTS
***************************************************************************/

#define DRIVER_AUTHOR   "Oxford Semiconductor Ltd."
#define DRIVER_DESC     "934 SATA core controler"
#define DRIVER_NAME     "oxnassata"
/**************************************************************************/
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

/***************************************************************************
* Module Parameters
***************************************************************************/
static int fpga = 0;
static int ports = 2;
static int no_microcode = 0;
static int disable_hotplug = 0;

module_param(fpga, bool, 0);
MODULE_PARM_DESC(fpga, "true to switch to FPGA based variant");
module_param(ports, int, 0);
MODULE_PARM_DESC(single_sata, "Number of SATA ports 1-2 (defaults to 2)");
module_param(no_microcode, bool, 0);
MODULE_PARM_DESC(no_microcode, "do not use microcode");
module_param(disable_hotplug, bool, 0);
MODULE_PARM_DESC(disable_hotplug, "disable hotplug");

/***************************************************************************
* Structures
***************************************************************************/
/* check this matches the space reserved in hardware.h */
typedef struct {
	volatile u32 qualifier;
	volatile u32 control;
	dma_addr_t src_pa;
	dma_addr_t dst_pa;
} __attribute ((aligned(4),packed)) sgdma_request_t;

typedef struct {
	struct kobject kobj;
	struct platform_driver driver;
	int num_ports;
	struct ata_port* ap[2];
	
	/* bitfield of frozen ports */
	unsigned long port_frozen;
	unsigned long port_in_eh;
} ox820sata_driver_t;

/**
 * Struct to hold per-port private (specific to this driver) data
 */
typedef struct
{
	u32* sgdma_controller;
	u32* dma_controller;
	sgdma_request_t* sgdma_request_va;
	dma_addr_t sgdma_request_pa;
	u32* reg_base;
} ox820sata_private_data;

/***************************************************************************
* Microcode References
***************************************************************************/
extern const unsigned int ox820_jbod_microcode[];
extern const unsigned int ox820_raid_microcode[];

/***************************************************************************
* Function Prototypes
***************************************************************************/
/* ASIC access */
static void wait_cr_ack(void);
static u16 read_cr(u16 address);
static void write_cr(u16 data, u16 address);
static void workaround5458(void);

/* Core Reset */
static void ox820sata_reset_core(void);
cleanup_recovery_t ox820sata_reset_cleanup(void);
cleanup_recovery_t ox820sata_progressive_cleanup(void);
static int ox820sata_softreset(struct ata_link *link, unsigned int *class, unsigned long deadline);
static void ox820sata_postreset(struct ata_link *link, unsigned int *classes);

/* Driver Init */
static int ox820sata_driver_probe(struct platform_device* pdev);
static int ox820sata_driver_remove(struct platform_device* pdev);

/* Driver Code */
static void ox820sata_dev_config(struct ata_device* pdev);

/* Task File */
static void ox820sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf);
static void ox820sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf);

/* QC */
static bool ox820sata_qc_fill_rtf(struct ata_queued_cmd *qc);
#define ox820sata_qc_defer ata_std_qc_defer
static void ox820sata_freeze(struct ata_port* ap);
static void ox820sata_thaw(struct ata_port* ap);
static void ox820sata_qc_prep(struct ata_queued_cmd* qc);
static unsigned int ox820sata_qc_issue(struct ata_queued_cmd *qc);
void ox820sata_freeze_host(int port_no);
void ox820sata_thaw_host(int port_no);

/* IRQ */
static void ox820sata_port_irq(struct ata_port* ap, int force_error);
static int ox820sata_bug_6320_workaround(int port);
static irqreturn_t ox820sata_irq_handler(int irq, void *dev_instance);
static void ox820sata_irq_clear(struct ata_port* ap);
static void ox820sata_irq_on(struct ata_port *ap);

/* Ready and Link Check */
static int ox820sata_check_ready(struct ata_link *link);
int ox820sata_check_link(int port_no);

/* Link Layer */
static int ox820sata_scr_read_port(struct ata_port *ap, unsigned int sc_reg, u32 *val);
static int ox820sata_scr_write_port(struct ata_port *ap, unsigned int sc_reg, u32 val);
static int ox820sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
static int ox820sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);
u32 ox820sata_link_read(u32* core_addr, unsigned int link_reg);
void ox820sata_link_write(u32* core_addr, unsigned int link_reg, u32 val);
static int ox820sata_port_start(struct ata_port *ap);
static void ox820sata_port_stop(struct ata_port *ap);
static void configure_ucode_engine(int mode, int reset, int program, int set_params);
static void ox820sata_post_reset_init(struct ata_port* ap);
static void ox820sata_host_stop(struct ata_host *host_set);

/* Error Handler and Cleanup */
static void ox820sata_error_handler(struct ata_port *ap);
static void ox820sata_post_internal_cmd(struct ata_queued_cmd *qc);

/***************************************************************************
* Global variables
***************************************************************************/
static struct ata_port_operations ox820sata_port_ops =
{
	.inherits			= &sata_port_ops,
						
	.qc_defer			= ox820sata_qc_defer,
	.qc_prep			= ox820sata_qc_prep,
	.qc_issue			= ox820sata_qc_issue,
	.qc_fill_rtf		= ox820sata_qc_fill_rtf,
						
	.freeze             = ox820sata_freeze,
	.thaw               = ox820sata_thaw,
	.softreset			= ox820sata_softreset,
	.dev_config         = ox820sata_dev_config,
						
	.scr_read           = ox820sata_scr_read,
	.scr_write          = ox820sata_scr_write,
						
	.port_start         = ox820sata_port_start,
	.port_stop          = ox820sata_port_stop,
	.host_stop          = ox820sata_host_stop,
						
	.postreset		    = ox820sata_postreset,
	.post_internal_cmd  = ox820sata_post_internal_cmd,
	.error_handler      = ox820sata_error_handler,
};

/** the scsi_host_template structure describes the basic capabilities of libata
and our 921 core to the SCSI framework, it contains the addresses of functions 
in the libata library that handle top level comands from the SCSI library */
static struct scsi_host_template ox820sata_sht = 
{
	ATA_BASE_SHT(DRIVER_NAME),
	.sg_tablesize       = CONFIG_ARCH_OXNAS_MAX_SATA_SG_ENTRIES,
	.dma_boundary       = ~0UL, // NAS has no DMA boundary restrictions
	.unchecked_isa_dma  = 0,
};

/**
 * port capabilities for the ox820 sata ports.
 */
static const struct ata_port_info ox820sata_port_info = {
	.flags = ATA_FLAG_SATA |
			 //ATA_FLAG_SATA_RESET |
			 //ATA_FLAG_NO_LEGACY |
			 ATA_FLAG_NO_ATAPI |
			 ATA_FLAG_PIO_DMA |
			 ATA_FLAG_PMP ,
	.pio_mask   = 0x1f, /* pio modes 0..4*/
	.mwdma_mask = 0x07, /* mwdma0-2 */
	.udma_mask  = 0x7f, /* udma0-5 */
	.port_ops   = &ox820sata_port_ops,
};

/**
 * Describes the identity of the SATA core and the resources it requires
 */ 
static struct resource ox820sata_resources[] = {
	{
		.name       = "sata_registers",
		.start		= SATA0_REGS_BASE,
		.end		= SATA0_REGS_BASE + 0xfffff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.name       = "sata_irq",
		.start      = SATA_INTERRUPT,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device ox820sata_dev = 
{
	.name = DRIVER_NAME,
	.id = 0,
	.num_resources = 2,
	.resource  = ox820sata_resources,
	.dev.coherent_dma_mask = 0xffffffff,
}; 

ox820sata_driver_t ox820sata_driver = 
{
	.driver = {
		.driver.name = DRIVER_NAME,
		.driver.bus = &platform_bus_type,
		.probe = ox820sata_driver_probe, 
		.remove = ox820sata_driver_remove,
	},
	.ap = {0,0},
};

int current_ucode_mode = OXNASSATA_UCODE_NONE;
int ox820sata_hotplug_events = 0;
DEFINE_SPINLOCK(async_register_lock);

/***************************************************************************
* Definitions of hardware registers
***************************************************************************/
#define PH_GAIN         2
#define FR_GAIN         3
#define PH_GAIN_OFFSET  6
#define FR_GAIN_OFFSET  8
#define PH_GAIN_MASK  (0x3 << PH_GAIN_OFFSET)
#define FR_GAIN_MASK  (0x3 << FR_GAIN_OFFSET)
#define USE_INT_SETTING  (1<<5)

#define CR_READ_ENABLE  (1<<16)
#define CR_WRITE_ENABLE (1<<17)
#define CR_CAP_DATA     (1<<18)

#define SATA_PHY_ASIC_STAT (SATA_PHY_BASE + 0x00)
#define SATA_PHY_ASIC_DATA (SATA_PHY_BASE + 0x04)

/***************************************************************************
* LED code
***************************************************************************/
#ifdef CONFIG_LEDS_TRIGGERS
static struct led_classdev* ox820_disklight_led = NULL;
static struct timer_list disklight_timer;
static unsigned long disklight_delay;
/* directly from drivers/leds/leds.h */
static inline void led_set_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	if (value > led_cdev->max_brightness) {
		value = led_cdev->max_brightness;
	}
	led_cdev->brightness = value;
	if (!(led_cdev->flags & LED_SUSPENDED)) {
		led_cdev->brightness_set(led_cdev, value);
	}
}

#endif

static inline void ox820_disklight_led_turn_on(void)
{
#ifdef CONFIG_LEDS_TRIGGERS
	struct led_classdev* ox820_disklight = ox820_disklight_led;
	
	if(NULL != ox820_disklight) {
		mod_timer(&disklight_timer, jiffies + disklight_delay);
		led_set_brightness(ox820_disklight, ox820_disklight->max_brightness);
	}
#endif
}

static inline void ox820_disklight_led_turn_off(void)
{
}

#ifdef CONFIG_LEDS_TRIGGERS
static void disklight_function(unsigned long data)
{
	struct led_classdev* ox820_disklight = ox820_disklight_led;

	if(NULL != ox820_disklight) {
		led_set_brightness(ox820_disklight, LED_OFF);
	}
}

static void ox820_disklight_trig_activate(struct led_classdev* led_cdev)
{
	ox820_disklight_led = led_cdev;
}

static void ox820_disklight_trig_deactivate(struct led_classdev* led_cdev)
{
	ox820_disklight_led = NULL;
}

static struct led_trigger ox820_disklight_led_trigger = {
	.name     = "ox820_disklight",
	.activate = ox820_disklight_trig_activate,
	.deactivate = ox820_disklight_trig_deactivate,
};
#endif

static int ox820_disklight_led_register(void)
{
#ifdef CONFIG_LEDS_TRIGGERS
	int ret;
	disklight_delay = msecs_to_jiffies(50);
	ret = led_trigger_register(&ox820_disklight_led_trigger);
	if(0 == ret) {
		setup_timer(&disklight_timer, disklight_function, 0);
	}
	return ret;
#else
	return 0;
#endif
}

static void ox820_disklight_led_unregister(void)
{
#ifdef CONFIG_LEDS_TRIGGERS
	del_timer_sync(&disklight_timer);
	led_trigger_unregister(&ox820_disklight_led_trigger);
#endif
}

/***************************************************************************
* ASIC access
***************************************************************************/

static void wait_cr_ack(void) 
{
	while ((readl(SATA_PHY_ASIC_STAT) >> 16) & 0x1f) {
		/* wait for an ack bit to be set */
	}
}

static u16 read_cr(u16 address) 
{
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel(CR_READ_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return readl(SATA_PHY_ASIC_STAT);
}

static void write_cr(u16 data, u16 address) 
{
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel((data | CR_CAP_DATA), SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	writel(CR_WRITE_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return ;
}

static void workaround5458(void) 
{
	unsigned i;
	
	for (i=0; i<2;i++) {
		u16 rx_control = read_cr( 0x201d + (i << 8));
		rx_control &= ~(PH_GAIN_MASK | FR_GAIN_MASK);
		rx_control |= PH_GAIN << PH_GAIN_OFFSET;
		rx_control |= (FR_GAIN << FR_GAIN_OFFSET) | USE_INT_SETTING ;
		write_cr( rx_control, 0x201d + (i << 8));
	}
}

/***************************************************************************
* Helper Functions
***************************************************************************/

/**
 * Gets the base address of the ata core from the ata_port structure. The value
 * returned will remain the same when hardware raid is active.
 *
 * @param ap pointer to the appropriate ata_port structure
 * @return the base address of the SATA core
 */
static inline u32* ox820sata_get_io_base(struct ata_port* ap)
{
	return ((ox820sata_private_data* )(ap->private_data))->reg_base;
}

/** 
 * Returns 0 if all the host ports are idle 
 * @param ap not used, but kept for commonality with the ox820sata.c driver
 */
static inline u32 ox820sata_hostportbusy(struct ata_port* ap) 
{
	/* port busy */
	u32 reg;
	reg = readl((u32* )SATA0_REGS_BASE + OX820SATA_SATA_COMMAND);
	if (unlikely(reg & CMD_CORE_BUSY)) {
	   return 1;
	}
	reg = readl((u32* )SATA1_REGS_BASE + OX820SATA_SATA_COMMAND);
	if (unlikely(reg & CMD_CORE_BUSY)) {
	   return 1;
	}
	
	/* idle */
	return 0;
}

/** 
 * Returns 0 if the scatter gather DMA channel used by ap is idle 
 */
static inline u32 ox820sata_hostdmabusy(struct ata_port* ap) 
{
	ox820sata_private_data* pd = (ox820sata_private_data*) ap->private_data;
	
	/* dma busy */
	if (unlikely(readl(pd->sgdma_controller + OX820SATA_SGDMA_STATUS) & OX820SATA_SGDMA_BUSY)) {
	   return 1;
	}
	
	/* idle */
	return 0;
}

/***************************************************************************
* Error and Sync Escape Control
***************************************************************************/
/**
 * sends a sync-escape if there is a link present 
 */
static inline void ox820sata_send_sync_escape(u32* base)
{
	u32 reg;
	/* read the SSTATUS register and only send a sync escape if there is a
	* link active */
	if ((ox820sata_link_read(base, 0x20) & 3) == 3) {
		reg = readl(base + OX820SATA_SATA_COMMAND);
		reg &= ~SATA_OPCODE_MASK;
		reg |= CMD_SYNC_ESCAPE;
		writel(reg, base + OX820SATA_SATA_COMMAND);
	}
}

/* clears errors */
static inline void ox820sata_clear_CS_error(u32* base)
{
	u32 reg;
	reg = readl(base + OX820SATA_SATA_CONTROL);
	reg &= OX820SATA_SATA_CTL_ERR_MASK;
	writel(reg, base + OX820SATA_SATA_CONTROL);
}

/**
 * Clears the error caused by the core's registers being accessed when the
 * core is busy. 
 */
static inline void ox820sata_clear_reg_access_error(u32* base)
{
	u32 reg;
	reg = readl(base + OX820SATA_INT_STATUS);
	if (reg & OX820SATA_INT_REG_ACCESS_ERR) {
		printk("clearing register access error\n");
		writel(OX820SATA_INT_REG_ACCESS_ERR, base + OX820SATA_INT_STATUS);
	}
	if (reg & OX820SATA_INT_REG_ACCESS_ERR) { 
		printk("register access error didn't clear\n");
	}    
}

/***************************************************************************
* Core Reset
***************************************************************************/

/**
 * Turns on the cores clock and resets it
 */
static void ox820sata_reset_core(void) 
{
	/* Enable the clock to the SATA block */
	writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_SET_CTRL);
	wmb();

	/* reset Controller, Link and PHY */
	writel( (1UL << SYS_CTRL_RSTEN_SATA_BIT)      |
			(1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
			(1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_SET_CTRL);
	wmb();
	udelay(50);
	
	/* un-reset the PHY, then Link and Controller */
	writel(1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
	udelay(50);
	writel( (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
			(1UL << SYS_CTRL_RSTEN_SATA_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
	udelay(50);

	workaround5458();
	
	/* tune for sata compatability */
	ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x60, 0x2988 );

	/* each port in turn */
	ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x70, 0x55629 );
	ox820sata_link_write((u32* )SATA1_REGS_BASE , 0x70, 0x55629 );
	udelay(50);
}

static void ox820sata_post_reset_init(struct ata_port* ap)
{
	uint dev;
	u32* ioaddr = ox820sata_get_io_base(ap);
	
	if(no_microcode)
	{
		u32 reg;

		ox820sata_set_mode(OXNASSATA_UCODE_NONE, 1);
		reg = readl(OX820SATA_DEVICE_CONTROL);
		reg |= OX820SATA_DEVICE_CONTROL_ATA_ERR_OVERRIDE;
		writel(reg, OX820SATA_DEVICE_CONTROL);
		
	} else {
		/* JBOD uCode */
		ox820sata_set_mode(OXNASSATA_UCODE_JBOD, 1);

		/* Turn the work around off as it may have been left on by any HW-RAID
		code that we've been working with */
		writel(0x0, OX820SATA_PORT_ERROR_MASK);
	}

	/* turn on phy error detection by removing the masks */ 
	ox820sata_link_write(ioaddr, 0x0C, 0x30003);

	/* enable hotplug event detection */    
	ox820sata_scr_write_port(ap, SCR_ERROR, ~0);
	ox820sata_scr_write_port(ap, OX820SATA_SERROR_IRQ_MASK, 0x03feffff);
	ox820sata_scr_write_port(ap, SCR_ACTIVE, ~0 & ~(1 << 26) & ~(1 << 16));
	
	/* enable interrupts for ports */
	ox820sata_irq_on(ap);
	
	/* go through all the devices and configure them */
	for (dev = 0; dev < ATA_MAX_DEVICES; ++dev) {
		if (ap->link.device[dev].class == ATA_DEV_ATA) {
			sata_std_hardreset(&ap->link, NULL, jiffies + HZ);
			ox820sata_dev_config(&(ap->link.device[dev]));
		}
	}

	/* clean up any remaining errors */
	ox820sata_scr_write_port(ap, SCR_ERROR, ~0);
}

/** 
 * host_stop() is called when the rmmod or hot unplug process begins. The
 * hook must stop all hardware interrupts, DMA engines, etc.
 *
 * @param ap hardware with the registers in
 */
static void ox820sata_host_stop(struct ata_host *host_set)
{
}

/**
 * Clean up all the state machines in the sata core.
 * @return post cleanup action required
 */
cleanup_recovery_t ox820sata_reset_cleanup(void) {
	int actions_required = 0;

	/* core not recovering, reset it */
	mdelay(5);
	ox820sata_reset_core();
	mdelay(5);
	actions_required |= re_init;
	/* Perform any SATA core re-initialisation after reset */
	/* post reset init needs to be called for both ports as there's one reset
	for both ports*/
	if (ox820sata_driver.ap[0]) {
		ox820sata_post_reset_init(ox820sata_driver.ap[0]);
	}
	if (ox820sata_driver.ap[1]) {
		ox820sata_post_reset_init(ox820sata_driver.ap[1]);
	}

	return actions_required;
}

/**
 * Clean up all the state machines in the sata core.
 * @return post cleanup action required
 */
cleanup_recovery_t ox820sata_progressive_cleanup(void) {
	int actions_required = 0;
	if(ports > 1) {
		u32 reg;
		u32 count;

		/* The maximum time waited at each stage of the cleanup before moving on to 
		a more severe cleanup action (10000 * 50us = 0.5s) */
		const u32 delay_loops = 10000;
		count = delay_loops;
		
		//CrazyDumpDebug();
		ox820sata_clear_reg_access_error((u32*)SATA0_REGS_BASE);
		ox820sata_clear_reg_access_error((u32*)SATA1_REGS_BASE);

		DPRINTK("ox820sata resetting some things.\n");
		/* reset the SGDMA channels */
		writel( OX820SATA_SGDMA_RESETS_CTRL, OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_RESETS);
		writel( OX820SATA_SGDMA_RESETS_CTRL, OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_RESETS);
		
		/* reset the DMA channels */
		reg = readl(OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
		reg |= OX820SATA_DMA_CONTROL_RESET;
		writel( reg, OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
		reg = readl(OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
		reg |= OX820SATA_DMA_CONTROL_RESET;
		writel( reg, OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);

		/* set dm port abort for both ports */
		reg = readl(OX820SATA_DEVICE_CONTROL);
		reg |= OX820SATA_DEVICE_CONTROL_DMABT << 0 ;
		reg |= OX820SATA_DEVICE_CONTROL_DMABT << 1 ;
		reg |= OX820SATA_DEVICE_CONTROL_ABORT ;
		writel( reg, OX820SATA_DEVICE_CONTROL);
		
		/* Wait a maximum af 500ms for both ports in the SATA core to go idle */
		count = 0;
		while ((count < delay_loops) && (
			   (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
			   (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
			   (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
			   (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) )) {
			count++;
			udelay(50);
		}

		/* still not idle, send a sync escape on each port */
		if (count >= delay_loops ) {
			DPRINTK("ox820sata sending sync escape\n");
			ox820sata_send_sync_escape((u32*)SATA0_REGS_BASE);
			ox820sata_send_sync_escape((u32*)SATA1_REGS_BASE);
			actions_required |= softreset;
		}

		/* Wait a maximum af 500ms for both ports in the SATA core to go idle */
		count = 0;
		while ((count < delay_loops) && (
			   (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
			   (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
			   (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
			   (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) ))
		{
			count++;
			udelay(50);
		}

		/* if the SATA core went idle before the timeout, clear resets */
		if (count < delay_loops ) {
			DPRINTK("Core idle, clear resets.\n");
			
			ox820sata_clear_CS_error((u32*)SATA0_REGS_BASE);
			ox820sata_clear_CS_error((u32*)SATA1_REGS_BASE);
			
			/* Clear link error */
			ox820sata_scr_write_port(ox820sata_driver.ap[0], SCR_ERROR, ~0);
			ox820sata_scr_write_port(ox820sata_driver.ap[1], SCR_ERROR, ~0);
		
			/* Clear errors in both ports*/
			reg = readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_CONTROL);
			reg |= OX820SATA_SCTL_CLR_ERR ;
			writel(reg, (u32*)SATA0_REGS_BASE + OX820SATA_SATA_CONTROL);
			reg = readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_CONTROL);
			reg |= OX820SATA_SCTL_CLR_ERR ;
			writel(reg, (u32*)SATA1_REGS_BASE + OX820SATA_SATA_CONTROL);
			reg = readl((u32*)SATARAID_REGS_BASE + OX820SATA_SATA_CONTROL);
			reg |= OX820SATA_RAID_CLR_ERR ;
			writel(reg, (u32*)SATARAID_REGS_BASE + OX820SATA_SATA_CONTROL);
			
			/* clear reset for the DMA channel */
			reg = readl(OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
			reg &= ~OX820SATA_DMA_CONTROL_RESET;
			writel( reg, OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
			reg = readl(OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
			reg &= ~OX820SATA_DMA_CONTROL_RESET;
			writel( reg, OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
		
			/* clear port and dma abort */    
			reg = readl(OX820SATA_DEVICE_CONTROL);
			reg &= ~OX820SATA_DEVICE_CONTROL_DMABT << 0 ;
			reg &= ~OX820SATA_DEVICE_CONTROL_DMABT << 1 ;
			reg &= ~OX820SATA_DEVICE_CONTROL_ABORT ;
			writel(reg, OX820SATA_DEVICE_CONTROL);

			/* set dm_mux_ram reset and port reset */
			reg = readl(OX820SATA_DEVICE_CONTROL);
			reg |= (OX820SATA_DEVICE_CONTROL_PRTRST |
				OX820SATA_DEVICE_CONTROL_RAMRST) << 0 ;
			reg |= (OX820SATA_DEVICE_CONTROL_PRTRST |
				OX820SATA_DEVICE_CONTROL_RAMRST) << 1 ;
			writel( reg, OX820SATA_DEVICE_CONTROL);
			wmb();
			
			/* resume micro code (shouldn't affect JBOD micro code) */
			writel(OX820SATA_CONFIG_IN_RESUME, OX820SATA_CONFIG_IN);

			/* Wait a maximum af 500ms for both ports in the SATA core to go idle */
			count = 0;
			while ((count < delay_loops) && (
				   (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
				   (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
				   (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
				   (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) )) {
				count++;
				udelay(50);
			}
		}

		/* if didn't go idle reset the core */
		if  (count >= delay_loops) {
			//CrazyDumpDebug();
			printk(KERN_ERR"ox820sata unable to carefully recover "
				"from SATA error, reseting core\n");
			actions_required |= ox820sata_reset_cleanup();
		}
	}
	else
	{
		actions_required |= ox820sata_reset_cleanup();
	}

	return actions_required;
}

static int ox820sata_softreset(struct ata_link *link, unsigned int *class, unsigned long deadline)
{
	int rc;
	struct ata_port *ap;
	u32 *ioaddr;
	struct ata_taskfile tf;
	u32 Command_Reg;

	DPRINTK("ENTER\n");

	ap = link->ap;
	ioaddr = ox820sata_get_io_base(ap);

	if (ata_link_offline(link)) {
		DPRINTK("PHY reports no device\n");
		*class = ATA_DEV_NONE;
		goto out;
	}

	/* write value to register */
	writel(0, ioaddr + OX820SATA_ORB1);
	writel(0, ioaddr + OX820SATA_ORB2);
	writel(0, ioaddr + OX820SATA_ORB3);
	writel((ap->ctl) << 24, ioaddr + OX820SATA_ORB4);

	/* command the core to send a control FIS */
	Command_Reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
	Command_Reg &= ~SATA_OPCODE_MASK;
	Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
	writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);
	udelay(20);	/* FIXME: flush */

	/* write value to register */
	writel((ap->ctl | ATA_SRST) << 24, ioaddr + OX820SATA_ORB4);

	/* command the core to send a control FIS */
	Command_Reg &= ~SATA_OPCODE_MASK;
	Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
	writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);
	udelay(20);	/* FIXME: flush */
	
	/* write value to register */
	writel((ap->ctl) << 24, ioaddr + OX820SATA_ORB4);

	/* command the core to send a control FIS */
	Command_Reg &= ~SATA_OPCODE_MASK;
	Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
	writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);

	msleep(150);

	rc = ata_wait_ready(link, deadline, ox820sata_check_ready);
	
	/* if link is occupied, -ENODEV too is an error */
	if (rc && (rc != -ENODEV || sata_scr_valid(link))) {
		ata_link_printk(link, KERN_ERR, "SRST failed (errno=%d)\n", rc);
		return rc;
	}

	/* determine by signature whether we have ATA or ATAPI devices */
	ox820sata_tf_read(ap, &tf);
	*class = ata_dev_classify(&tf);

	if (*class == ATA_DEV_UNKNOWN) {
		*class = ATA_DEV_NONE;
	}
 out:
	DPRINTK("EXIT, class=%u\n", *class );
	return 0;
}
	

/**
 *	ox820sata_postreset - ox820 postreset callback
 *	@link: the target ata_link
 *	@classes: classes of attached devices
 *
 *	This function is invoked after a successful reset.  Note that
 *	the device might have been reset more than once using
 *	different reset methods before postreset is invoked.
 *
 *	LOCKING:
 *	Kernel thread context (may sleep)
 */
static void ox820sata_postreset(struct ata_link *link, unsigned int *classes)
{
	struct ata_port *ap = link->ap;
	unsigned int dev;

	DPRINTK("ENTER\n");

	ata_std_postreset(link, classes);
	
	/* turn on phy error detection by removing the masks */ 
	ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x0c, 0x30003 );
	ox820sata_link_write((u32* )SATA1_REGS_BASE , 0x0c, 0x30003 );

	/* bail out if no device is present */
	if (classes[0] == ATA_DEV_NONE && classes[1] == ATA_DEV_NONE) {
		DPRINTK("EXIT, no device\n");
		return;
	}

	/* go through all the devices and configure them */
	for (dev = 0; dev < ATA_MAX_DEVICES; ++dev) {
		if (ap->link.device[dev].class == ATA_DEV_ATA) {
			ox820sata_dev_config(&(ap->link.device[dev]));
		}
	}

	DPRINTK("EXIT\n");
}

/***************************************************************************
* Driver Init
***************************************************************************/
/** 
 * The driver probe function.
 * Registered with the amba bus driver as a parameter of ox820sata_driver.bus
 * it will register the ata device with kernel first performing any 
 * initialisation required (if the correct device is present).
 * @param pdev Pointer to the 921 device structure 
 * @return 0 if no errors
 */
static int ox820sata_driver_probe(struct platform_device* pdev)
{
	u32 version;
	struct ata_host *host;
	void __iomem* iomem;
	struct ata_port_info* port_info[] = {
		(struct ata_port_info*) &ox820sata_port_info,
		NULL,
		NULL
	};
	struct resource* memres = platform_get_resource(pdev, IORESOURCE_MEM, 0 );
	int irq = platform_get_irq(pdev, 0);
	
	if(ports > 1) {
		port_info[1] = (struct ata_port_info*) &ox820sata_port_info;
	}
	
	/* check resourses for sanity */
	if ((memres == NULL) || (irq < 0)) {
		return 0;
	}
	iomem = (void __iomem* ) memres->start;
	
	/* check we support this version of the core */
	version = readl(((u32* )iomem) + OX820SATA_VERSION);
	switch (version) {
		case OX820SATA_CORE_VERSION:
			printk(KERN_INFO"ox820sata: OX820 sata core.\n");   
			break;
		default:
			printk(KERN_ERR"ox820sata: unknown sata core (version register = 0x%08x)\n",version);     
			return 0;
			break;
	}

	/* allocate memory and check */
	host = ata_host_alloc_pinfo(&(pdev->dev), (const struct ata_port_info**) port_info, ports);
	if (!host) {
		printk(KERN_ERR DRIVER_NAME " Couldn't create an ata host.\n");
	}

	/* set to base of ata core */
	host->iomap  = iomem;

	/* call ata_device_add and begin probing for drives*/
	ata_host_activate(host, irq, ox820sata_irq_handler, IRQF_SHARED, &ox820sata_sht);

	return 0;
}

/** 
 * Called when the amba bus tells this device to remove itself.
 * @param pdev pointer to the device that needs to be shutdown
 */
static int ox820sata_driver_remove(struct platform_device* pdev)
{
	struct ata_host *host_set = dev_get_drvdata( &(pdev->dev) );
	struct ata_port *ap;
	unsigned int i;
	
	for (i = 0; i < host_set->n_ports; i++) 
	{
		ap = host_set->ports[i];
		scsi_remove_host( ap->scsi_host );
	}

	// reset Controller, Link and PHY
	writel( (1UL << SYS_CTRL_RSTEN_SATA_BIT)      |
			(1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
			(1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_SET_CTRL);
	
	// Disable the clock to the SATA block
	writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_CLR_CTRL);
	
	return 0;
}

/***************************************************************************
* Module Init
***************************************************************************/

/** 
 * module initialisation
 * @return success
 */
static int __init ox820sata_init_driver( void )
{
	int ret, aborted_at = 0;
	/* check ports parameter */
	if(ports < 1 || ports > 2) {
		return -EINVAL;
	}
	
	/* check there is enough space for PRD entries in SRAM */
	if (ATA_PRD_TBL_SZ > OX820SATA_PRD_SIZE) {
		printk(KERN_ERR"PRD table size is bigger than the space allocated for it in hardware.h");
		return -EINVAL;
	}

	/* check this matches the space reserved in hardware.h */
	if (sizeof(sgdma_request_t) > OX820SATA_SGDMA_SIZE) {
		printk(KERN_ERR"sgdma_request_t has grown beyond the space allocated for it in hardware.h");
		return -EINVAL;
	}

	if(ports > 1)
	{

		/* check there is enough space for PRD entries in SRAM */
		if ((2 * ATA_PRD_TBL_SZ) > OX820SATA_PRD_SIZE) {
			printk(KERN_ERR"PRD table size is bigger than the space allocated for it in hardware.h");
			return -EINVAL;
		}

		/* check this matches the space reserved in hardware.h */
		if ((2 * sizeof(sgdma_request_t)) > OX820SATA_SGDMA_SIZE) {
			printk(KERN_ERR"sgdma_request_t has grown beyond the space allocated for it in hardware.h");
			return -EINVAL;
		}
	}
	
	ret = ox820_disklight_led_register();
	
	if(0 == ret) {
		ret = platform_driver_register( &ox820sata_driver.driver );
		if(0 != ret) {
			ox820_disklight_led_unregister();
		}
	}
	
	if(0 == ret) {
	++aborted_at;
		/* reset the core */
		ox820sata_reset_core();
	}

	if(0 == ret) {
	++aborted_at;
		/* register the ata device for the driver to find */
		ret = platform_device_register( &ox820sata_dev );
		if(ret != 0) {
			platform_driver_unregister( &ox820sata_driver.driver );
			ox820_disklight_led_unregister();
		}
	}
	
	if(0 == ret) {
		printk(KERN_INFO"sata_ox820: Initialized\n");
	} else {
		printk(KERN_ERR"sata_ox820: Initialization result %u at %u\n", ret, aborted_at);
	}

	return ret; 
}

/** 
 * module cleanup
 */
static void __exit ox820sata_exit_driver( void )
{
	platform_device_unregister( &ox820sata_dev );
	platform_driver_unregister( &ox820sata_driver.driver );
	ox820_disklight_led_unregister();
}

module_init(ox820sata_init_driver);
module_exit(ox820sata_exit_driver);

/***************************************************************************
* Microcodes
***************************************************************************/
/* these micro-code programs _should_ include the version word */

/* JBOD microcode */
const unsigned int ox820_jbod_microcode[] = {
	0x07B400AC, 0x0228A280, 0x00200001, 0x00204002, 0x00224001,
	0x00EE0009, 0x00724901, 0x01A24903, 0x00E40009, 0x00224001,
	0x00621120, 0x0183C908, 0x00E20005, 0x00718908, 0x0198A206,
	0x00621124, 0x0183C908, 0x00E20046, 0x00621104, 0x0183C908,
	0x00E20015, 0x00EE009D, 0x01A3E301, 0x00E2001B, 0x0183C900,
	0x00E2001B, 0x00210001, 0x00EE0020, 0x01A3E302, 0x00E2009D,
	0x0183C901, 0x00E2009D, 0x00210002, 0x0235D700, 0x0208A204,
	0x0071C908, 0x000F8207, 0x000FC207, 0x0071C920, 0x000F8507,
	0x000FC507, 0x0228A240, 0x02269A40, 0x00094004, 0x00621104,
	0x0180C908, 0x00E40031, 0x00621112, 0x01A3C801, 0x00E2002B,
	0x00294000, 0x0228A220, 0x01A69ABF, 0x002F8000, 0x002FC000,
	0x0198A204, 0x0001C022, 0x01B1A220, 0x0001C106, 0x00088007,
	0x0183C903, 0x00E2009D, 0x0228A220, 0x0071890C, 0x0208A206,
	0x0198A206, 0x0001C022, 0x01B1A220, 0x0001C106, 0x00088007,
	0x00EE009D, 0x00621104, 0x0183C908, 0x00E2004A, 0x00EE009D,
	0x01A3C901, 0x00E20050, 0x0021E7FF, 0x0183E007, 0x00E2009D,
	0x00EE0054, 0x0061600B, 0x0021E7FF, 0x0183C507, 0x00E2009D,
	0x01A3E301, 0x00E2005A, 0x0183C900, 0x00E2005A, 0x00210001,
	0x00EE005F, 0x01A3E302, 0x00E20005, 0x0183C901, 0x00E20005,
	0x00210002, 0x0235D700, 0x0208A204, 0x000F8109, 0x000FC109,
	0x0071C918, 0x000F8407, 0x000FC407, 0x0001C022, 0x01A1A2BF,
	0x0001C106, 0x00088007, 0x02269A40, 0x00094004, 0x00621112,
	0x01A3C801, 0x00E4007F, 0x00621104, 0x0180C908, 0x00E4008D,
	0x00621128, 0x0183C908, 0x00E2006C, 0x01A3C901, 0x00E2007B,
	0x0021E7FF, 0x0183E007, 0x00E2007F, 0x00EE006C, 0x0061600B,
	0x0021E7FF, 0x0183C507, 0x00E4006C, 0x00621111, 0x01A3C801,
	0x00E2007F, 0x00621110, 0x01A3C801, 0x00E20082, 0x0228A220,
	0x00621119, 0x01A3C801, 0x00E20086, 0x0001C022, 0x01B1A220,
	0x0001C106, 0x00088007, 0x0198A204, 0x00294000, 0x01A69ABF,
	0x002F8000, 0x002FC000, 0x0183C903, 0x00E20005, 0x0228A220,
	0x0071890C, 0x0208A206, 0x0198A206, 0x0001C022, 0x01B1A220,
	0x0001C106, 0x00088007, 0x00EE009D, 0x00621128, 0x0183C908,
	0x00E20005, 0x00621104, 0x0183C908, 0x00E200A6, 0x0062111C,
	0x0183C908, 0x00E20005, 0x0071890C, 0x0208A206, 0x0198A206,
	0x00718908, 0x0208A206, 0x00EE0005, ~0
};

/* Bi-Modal RAID-0/1 Microcode */
const unsigned int ox820_raid_microcode[] = {
	0x00F20145, 0x00EE20FA, 0x00EE20A7, 0x0001C009, 0x00EE0004,
	0x00220000, 0x0001000B, 0x037003FF, 0x00700018, 0x037003FE,
	0x037043FD, 0x00704118, 0x037043FC, 0x01A3D240, 0x00E20017,
	0x00B3C235, 0x00E40018, 0x0093C104, 0x00E80014, 0x0093C004,
	0x00E80017, 0x01020000, 0x00274020, 0x00EE0083, 0x0080C904,
	0x0093C104, 0x00EA0020, 0x0093C103, 0x00EC001F, 0x00220002,
	0x00924104, 0x0005C009, 0x00EE0058, 0x0093CF04, 0x00E80026,
	0x00900F01, 0x00600001, 0x00910400, 0x00EE0058, 0x00601604,
	0x01A00003, 0x00E2002C, 0x01018000, 0x00274040, 0x00EE0083,
	0x0093CF03, 0x00EC0031, 0x00220003, 0x00924F04, 0x0005C009,
	0x00810104, 0x00B3C235, 0x00E20037, 0x0022C000, 0x00218210,
	0x00EE0039, 0x0022C001, 0x00218200, 0x00600401, 0x00A04901,
	0x00604101, 0x01A0C401, 0x00E20040, 0x00216202, 0x00EE0041,
	0x00216101, 0x02018506, 0x00EE2141, 0x00904901, 0x00E20049,
	0x00A00401, 0x00600001, 0x02E0C301, 0x00EE2141, 0x00216303,
	0x037003EE, 0x01A3C001, 0x00E40105, 0x00250080, 0x00204000,
	0x002042F1, 0x0004C001, 0x00230001, 0x00100006, 0x02C18605,
	0x00100006, 0x01A3D502, 0x00E20055, 0x00EE0053, 0x00004009,
	0x00000004, 0x00B3C235, 0x00E40062, 0x0022C001, 0x0020C000,
	0x00EE2141, 0x0020C001, 0x00EE2141, 0x00EE006B, 0x0022C000,
	0x0060D207, 0x00EE2141, 0x00B3C242, 0x00E20069, 0x01A3D601,
	0x00E2006E, 0x02E0C301, 0x00EE2141, 0x00230001, 0x00301303,
	0x00EE007B, 0x00218210, 0x01A3C301, 0x00E20073, 0x00216202,
	0x00EE0074, 0x00216101, 0x02018506, 0x00214000, 0x037003EE,
	0x01A3C001, 0x00E40108, 0x00230001, 0x00100006, 0x00250080,
	0x00204000, 0x002042F1, 0x0004C001, 0x00EE007F, 0x0024C000,
	0x01A3D1F0, 0x00E20088, 0x00230001, 0x00300000, 0x01A3D202,
	0x00E20085, 0x00EE00A5, 0x00B3C800, 0x00E20096, 0x00218000,
	0x00924709, 0x0005C009, 0x00B20802, 0x00E40093, 0x037103FD,
	0x00710418, 0x037103FC, 0x00EE0006, 0x00220000, 0x0001000F,
	0x00EE0006, 0x00800B0C, 0x00B00001, 0x00204000, 0x00208550,
	0x00208440, 0x002083E0, 0x00208200, 0x00208100, 0x01008000,
	0x037083EE, 0x02008212, 0x02008216, 0x01A3C201, 0x00E400A5,
	0x0100C000, 0x00EE20FA, 0x02800000, 0x00208000, 0x00B24C00,
	0x00E400AD, 0x00224001, 0x00724910, 0x0005C009, 0x00B3CDC4,
	0x00E200D5, 0x00B3CD29, 0x00E200D5, 0x00B3CD20, 0x00E200D5,
	0x00B3CD24, 0x00E200D5, 0x00B3CDC5, 0x00E200D2, 0x00B3CD39,
	0x00E200D2, 0x00B3CD30, 0x00E200D2, 0x00B3CD34, 0x00E200D2,
	0x00B3CDCA, 0x00E200CF, 0x00B3CD35, 0x00E200CF, 0x00B3CDC8,
	0x00E200CC, 0x00B3CD25, 0x00E200CC, 0x00B3CD40, 0x00E200CB,
	0x00B3CD42, 0x00E200CB, 0x01018000, 0x00EE0083, 0x0025C000,
	0x036083EE, 0x0000800D, 0x00EE00D8, 0x036083EE, 0x00208035,
	0x00EE00DA, 0x036083EE, 0x00208035, 0x00EE00DA, 0x00208007,
	0x036083EE, 0x00208025, 0x036083EF, 0x02400000, 0x01A3D208,
	0x00E200D8, 0x0067120A, 0x0021C000, 0x0021C224, 0x00220000,
	0x00404B1C, 0x00600105, 0x00800007, 0x0020C00E, 0x00214000,
	0x01004000, 0x01A0411F, 0x00404E01, 0x01A3C101, 0x00E200F1,
	0x00B20800, 0x00E400D8, 0x00220001, 0x0080490B, 0x00B04101,
	0x0040411C, 0x00EE00E1, 0x02269A01, 0x01020000, 0x02275D80,
	0x01A3D202, 0x00E200F4, 0x01B75D80, 0x01030000, 0x01B69A01,
	0x00EE00D8, 0x01A3D204, 0x00E40104, 0x00224000, 0x0020C00E,
	0x0020001E, 0x00214000, 0x01004000, 0x0212490E, 0x00214001,
	0x01004000, 0x02400000, 0x00B3D702, 0x00E80112, 0x00EE010E,
	0x00B3D702, 0x00E80112, 0x00B3D702, 0x00E4010E, 0x00230001,
	0x00EE0140, 0x00200005, 0x036003EE, 0x00204001, 0x00EE0116,
	0x00230001, 0x00100006, 0x02C18605, 0x00100006, 0x01A3D1F0,
	0x00E40083, 0x037003EE, 0x01A3C002, 0x00E20121, 0x0020A300,
	0x0183D102, 0x00E20124, 0x037003EE, 0x01A00005, 0x036003EE,
	0x01A0910F, 0x00B3C20F, 0x00E2012F, 0x01A3D502, 0x00E20116,
	0x01A3C002, 0x00E20116, 0x00B3D702, 0x00E4012C, 0x00300000,
	0x00EE011F, 0x02C18605, 0x00100006, 0x00EE0116, 0x01A3D1F0,
	0x00E40083, 0x037003EE, 0x01A3C004, 0x00E20088, 0x00200003,
	0x036003EE, 0x01A3D502, 0x00E20136, 0x00230001, 0x00B3C101,
	0x00E4012C, 0x00100006, 0x02C18605, 0x00100006, 0x00204000,
	0x00EE0116, 0x00100006, 0x01A3D1F0, 0x00E40083, 0x01000000,
	0x02400000, ~0
};

/***************************************************************************
* ATA status
***************************************************************************/

/** 
 * Reads the Status ATA shadow register from hardware.
 *
 * @return The status register
 */
static u8 ox820sata_check_status(struct ata_port *ap)
{
	u32 Reg;
	u8 status;
	u32 *ioaddr = ox820sata_get_io_base(ap);

	/* read byte 3 of Orb2 register */
	status = readl(ioaddr + OX820SATA_ORB2) >> 24;

	/* check for the drive going missing indicated by SCR status bits 0-3 = 0 */
	ox820sata_scr_read_port(ap, SCR_STATUS, &Reg );

	if (!(Reg & 0x1)) { 
		status |= ATA_DF;
		status |= ATA_ERR;
	}

	return status;
}

/***************************************************************************
* Task File
***************************************************************************/
/** 
 * called to write a taskfile into the ORB registers
 * @param ap hardware with the registers in
 * @param tf taskfile to write to the registers
 */
static void ox820sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf)
{
	u32 count = 0;
	u32 Orb1 = 0; 
	u32 Orb2 = 0; 
	u32 Orb3 = 0;
	u32 Orb4 = 0;
	u32 Command_Reg;
	u32 *ioaddr = ox820sata_get_io_base(ap);
	unsigned int is_addr = tf->flags & ATA_TFLAG_ISADDR;

	/* wait a maximum of 10ms for the core to be idle */
	do {
		Command_Reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
		if (!(Command_Reg & CMD_CORE_BUSY)) {
			break;
		}
		count++;
		udelay(50);
	} while (count < 200);

	/* check if the ctl register has interrupts disabled or enabled and
	modify the interrupt enable registers on the ata core as required */
	if (tf->ctl & ATA_NIEN) {
		/* interrupts disabled */
		u32 mask = (OX820SATA_COREINT_END << ap->port_no );
		writel(mask, OX820SATA_CORE_INT_DISABLE);
		ox820sata_irq_clear(ap);
	} else {
		ox820sata_irq_on(ap);
	}

	Orb2 |= (tf->command)    << 24;
	
	/* write 48 or 28 bit tf parameters */
	if (is_addr) {
		/* set LBA bit as it's an address */
		Orb1 |= (tf->device & ATA_LBA) << 24;

		if (tf->flags & ATA_TFLAG_LBA48) {
			Orb1 |= ATA_LBA << 24;

			Orb2 |= (tf->hob_nsect)  << 8 ;

			Orb3 |= (tf->hob_lbal)   << 24;

			Orb4 |= (tf->hob_lbam)   << 0 ;
			Orb4 |= (tf->hob_lbah)   << 8 ;
			Orb4 |= (tf->hob_feature)<< 16;
		} else {
			Orb3 |= (tf->device & 0xf)<< 24;
		}

		/* write 28-bit lba */
		Orb2 |= (tf->nsect)      << 0 ;
		Orb2 |= (tf->feature)    << 16;

		Orb3 |= (tf->lbal)       << 0 ;
		Orb3 |= (tf->lbam)       << 8 ;
		Orb3 |= (tf->lbah)       << 16;

		Orb4 |= (tf->ctl)        << 24;

	}

	if (tf->flags & ATA_TFLAG_DEVICE) {
		Orb1 |= (tf->device) << 24;
	}
	ap->last_ctl = tf->ctl;

	/* write values to registers */
	writel(Orb1, ioaddr + OX820SATA_ORB1 );
	writel(Orb2, ioaddr + OX820SATA_ORB2 );
	writel(Orb3, ioaddr + OX820SATA_ORB3 );
	writel(Orb4, ioaddr + OX820SATA_ORB4 );
}

/** 
 * Called to read the hardware registers / DMA buffers, to
 * obtain the current set of taskfile register values.
 * @param ap hardware with the registers in
 * @param tf taskfile to read the registers into
 */
static void ox820sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf)
{
	u32 *ioaddr = ox820sata_get_io_base(ap);

	/* read the orb registers */
	u32 Orb1 = readl(ioaddr + OX820SATA_ORB1); 
	u32 Orb2 = readl(ioaddr + OX820SATA_ORB2); 
	u32 Orb3 = readl(ioaddr + OX820SATA_ORB3);
	u32 Orb4 = readl(ioaddr + OX820SATA_ORB4);

	/* read common 28/48 bit tf parameters */
	tf->device  = (Orb1 >> 24);
	tf->nsect   = (Orb2 >> 0);
	tf->feature = (Orb2 >> 16);
	tf->command = ox820sata_check_status(ap);

	/* read 48 or 28 bit tf parameters */
	if (tf->flags & ATA_TFLAG_LBA48) {
		tf->hob_nsect = (Orb2 >> 8) ;
		
		tf->lbal      = (Orb3 >> 0) ;
		tf->lbam      = (Orb3 >> 8) ;
		tf->lbah      = (Orb3 >> 16) ;
		tf->hob_lbal  = (Orb3 >> 24) ;
		
		tf->hob_lbam  = (Orb4 >> 0) ;
		tf->hob_lbah  = (Orb4 >> 8) ;
		/* feature ext and control are write only */
	} else {
		/* read 28-bit lba */
		tf->lbal      = (Orb3 >> 0) ;
		tf->lbam      = (Orb3 >> 8) ;
		tf->lbah      = (Orb3 >> 16) ;
	}
}

/***************************************************************************
* QC
***************************************************************************/

/**
 * Read a result task-file from the sata core registers.
 */
static bool ox820sata_qc_fill_rtf(struct ata_queued_cmd *qc)
{
	/* Read the most recently received FIS from the SATA core ORB registers
	   and convert to an ATA taskfile */
	ox820sata_tf_read(qc->ap, &qc->result_tf);
	return true;
}

static void ox820sata_freeze(struct ata_port* ap)
{
	set_bit(ap->port_no, &ox820sata_driver.port_frozen);
	smp_wmb();
}

static void ox820sata_thaw(struct ata_port* ap)
{
	clear_bit(ap->port_no, &ox820sata_driver.port_frozen);
	smp_wmb();
}

/**
 * Prepare as much as possible for a command without involving anything that is
 * shared between ports. 
 */
static void ox820sata_qc_prep(struct ata_queued_cmd* qc) 
{
	ox820sata_private_data* pd;
	int dma_channel;

	DPRINTK("Port %d\n", qc->ap->port_no);
	if(no_microcode) {
		u32 reg;

		ox820sata_set_mode(OXNASSATA_UCODE_NONE, 0);
		reg = readl(OX820SATA_DEVICE_CONTROL);
		reg |= OX820SATA_DEVICE_CONTROL_ATA_ERR_OVERRIDE;
		writel(reg, OX820SATA_DEVICE_CONTROL);
	} else {
		/* JBOD uCode */
		ox820sata_set_mode(OXNASSATA_UCODE_JBOD, 0);

		/* Turn the work around off as it may have been left on by any HW-RAID
		code that we've been working with */
		writel(0x0, OX820SATA_PORT_ERROR_MASK);
	}

	dma_channel = qc->ap->port_no;

	/* if the port's not connected, complete now with an error */
	if (!ox820sata_check_link(qc->ap->port_no)) {
		printk(KERN_ERR"port %d not connected completing with error\n",qc->ap->port_no);
		qc->err_mask |= AC_ERR_ATA_BUS;
		ata_qc_complete(qc);
	}
	
	/* both pio and dma commands use dma */
	if (ata_is_dma(qc->tf.protocol) || ata_is_pio(qc->tf.protocol) ) {
		/* program the scatterlist into the prd table */
		ata_bmdma_qc_prep(qc);
		
		/* point the sgdma controller at the dma request structure */
		pd = (ox820sata_private_data*)qc->ap->private_data;
	
		writel(pd->sgdma_request_pa,
			pd->sgdma_controller + OX820SATA_SGDMA_REQUESTPTR );
		
		/* setup the request table */
		if (dma_channel == 0) {
			pd->sgdma_request_va->control = (qc->dma_dir == DMA_FROM_DEVICE) ? 
					OX820SATA_SGDMA_REQCTL0IN : OX820SATA_SGDMA_REQCTL0OUT ;
		} else {
			pd->sgdma_request_va->control = (qc->dma_dir == DMA_FROM_DEVICE) ? 
					OX820SATA_SGDMA_REQCTL1IN : OX820SATA_SGDMA_REQCTL1OUT ;
		}
		pd->sgdma_request_va->qualifier = OX820SATA_SGDMA_REQQUAL;
		pd->sgdma_request_va->src_pa = qc->ap->bmdma_prd_dma;
		pd->sgdma_request_va->dst_pa = qc->ap->bmdma_prd_dma;
		smp_wmb();

		/* Telling DMA SG controller to wait */
		writel(OX820SATA_SGDMA_CONTROL_NOGO,
			pd->sgdma_controller + OX820SATA_SGDMA_CONTROL);
		
	}
}

/** 
 * qc_issue is used to make a command active, once the hardware and S/G tables
 * have been prepared. IDE BMDMA drivers use the helper function
 * ata_qc_issue_prot() for taskfile protocol-based dispatch. More advanced drivers
 * roll their own ->qc_issue implementation, using this as the "issue new ATA
 * command to hardware" hook.
 * @param qc the queued command to issue
 */
static unsigned int ox820sata_qc_issue(struct ata_queued_cmd *qc)
{
	ox820sata_private_data* pd;
	u32 reg;
	u32* ioaddr;
	int dma_channel;

	dma_channel = qc->ap->port_no;
	
	pd = (ox820sata_private_data*)qc->ap->private_data;
	ioaddr = ox820sata_get_io_base(qc->ap);

	ox820_disklight_led_turn_on();

	/* check the core is idle */
	if (readl(ioaddr + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) {
		printk(KERN_ERR"ox820sata_qc_issue: Core busy, returning an error.\n");
		/* it'll never work, report an error and give up */
		return AC_ERR_OTHER;
	}

	/* enable passing of error signals to DMA sub-core by clearing the 
	appropriate bit */
	reg = readl(OX820SATA_DATA_PLANE_CTRL);
	if(no_microcode) {
		reg |= (OX820SATA_DPC_ERROR_MASK_BIT | (OX820SATA_DPC_ERROR_MASK_BIT << 1));
	}
	reg &= ~(OX820SATA_DPC_ERROR_MASK_BIT << dma_channel);
	writel(reg, OX820SATA_DATA_PLANE_CTRL);

	/* Load the command settings into the orb registers */
	ox820sata_tf_load(qc->ap, &qc->tf);

	/* both pio and dma commands use dma */
	if (ata_is_dma(qc->tf.protocol) || ata_is_pio(qc->tf.protocol) )
	{
		/* Starting DMA SG controller */
		writel(OX820SATA_SGDMA_CONTROL_GO,
			pd->sgdma_controller + OX820SATA_SGDMA_CONTROL);
		wmb();
	}
	
	/* Start the command */
	reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
	reg &= ~SATA_OPCODE_MASK;
	reg |= CMD_WRITE_TO_ORB_REGS;
	writel(reg , ioaddr + OX820SATA_SATA_COMMAND);
	wmb();
	
	return 0;
}

/***************************************************************************
* Interrupt Code
***************************************************************************/

/* This port done an interrupt */
static void ox820sata_port_irq(struct ata_port* ap, int force_error)
{    
	struct ata_queued_cmd* qc;
	ox820sata_private_data* pd;
	unsigned long flags = 0;

	qc = ata_qc_from_tag(ap, ap->link.active_tag);    
	pd = (ox820sata_private_data*)ap->private_data;

	/* If there's no command associated with this IRQ, ignore it. We may get
	spurious interrupts when cleaning-up after a failed command, ignore these 
	too. */
	if (likely(qc)) {
		/* get the status before any error cleanup */
		qc->err_mask = ac_err_mask(ox820sata_check_status(ap));

		if (force_error) {
			// Pretend there has been a link error
			qc->err_mask |= AC_ERR_ATA_BUS;
		}

		/* tell libata we're done */
		DPRINTK(" returning err_mask=0x%x\n", qc->err_mask);
		local_irq_save(flags);
		ox820sata_irq_clear(ap);
		local_irq_restore(flags);
		ata_qc_complete(qc);
	} else {
		VPRINTK("Ignoring interrupt, can't find the command tag=  %d %08x\n", ap->link.active_tag, ap->qc_active );
	}

	if(!disable_hotplug)
	{
		u32* ioaddr = ox820sata_get_io_base(ap);

		/* maybe a hotplug event */
		if (unlikely(readl(ioaddr + OX820SATA_INT_STATUS) & OX820SATA_INT_LINK_SERROR)) {
			u32 serror;
			
			ox820sata_scr_read_port(ap, SCR_ERROR, &serror);
			if(serror & (SERR_DEV_XCHG | SERR_PHYRDY_CHG)) {
				ata_ehi_hotplugged(&ap->link.eh_info);
				ata_port_freeze(ap);
			}
		}
	}
}

/**
 * Ref bug-6320
 *
 * This code is a work around for a DMA hardware bug that will repeat the 
 * penultimate 8-bytes on some reads. This code will check that the amount 
 * of data transferred is a multiple of 512 bytes, if not the in it will 
 * fetch the correct data from a buffer in the SATA core and copy it into
 * memory.
 *
 * @param port SATA port to check and if necessary, correct.
 */
static int ox820sata_bug_6320_workaround(int port)
{
	int is_read;
	int quads_transferred;
	int remainder;
	int sector_quads_remaining;
	int bug_present = 0;

	/* Only want to apply fix to reads */
	is_read = !(readl(OX820SATA_DM_DBG1) &
		(1UL << (port ? OX820SATA_CORE_PORT1_DATA_DIR_BIT :
						OX820SATA_CORE_PORT0_DATA_DIR_BIT)));

	/* Check for an incomplete transfer, i.e. not a multiple of 512 bytes
	   transferred (datacount_port register counts quads transferred) */
	quads_transferred =
		readl(port ? OX820SATA_DATACOUNT_PORT1 : OX820SATA_DATACOUNT_PORT0);

	remainder = quads_transferred & 0x7f;
	sector_quads_remaining = remainder ? (0x80 - remainder): 0;

	if (is_read && (sector_quads_remaining == 2)) {
		bug_present = 1;
	} else if (sector_quads_remaining) {
		if (is_read) {
			printk(KERN_WARNING "SATA read fixup cannot deal with %d quads remaining\n",
				sector_quads_remaining);
		} else {
			printk(KERN_WARNING "SATA write fixup of %d quads remaining not supported\n",
				sector_quads_remaining);
		}
	}

	return bug_present;
}

/** 
 * irq_handler is the interrupt handling routine registered with the system,
 * by libata.
 */
static irqreturn_t ox820sata_irq_handler(int irq, void *dev_instance)
{
	u32 int_status;
	irqreturn_t ret = IRQ_NONE;
	u32 all_ports = OX820SATA_COREINT_END | (OX820SATA_COREINT_END << 1);

	/* loop until there are no more interrupts */
	while ((int_status = readl(OX820SATA_CORE_INT_STATUS)) & all_ports) {
		int check_for_6320_present = 0;
		int bug_6320_present = 0;

		DPRINTK("outer loop irq %d, int_status 0x%p\n", irq, (void*)int_status);
		check_for_6320_present = no_microcode;
		/*
		 * Needed for all commands that do not actively use micro-code for
		 * dual disk configurations, ie. raid-1 reads. Relies on the 
		 * workaround code to do nothing on a write. Without uCode the interrupt
		 * always appears to come from port 0, but the SATA/AHB detection
		 * register resides with the port and we don't know which port to check
		 */
		if (current_ucode_mode == OXNASSATA_UCODE_RAID1) {
			check_for_6320_present = 1;
		}

		if (check_for_6320_present) {
			if (int_status & OX820SATA_COREINT_END) {
				bug_6320_present = ox820sata_bug_6320_workaround(0);
				if (!bug_6320_present) {
					bug_6320_present = ox820sata_bug_6320_workaround(1);
				}
			}
			if ((int_status & (OX820SATA_COREINT_END << 1))) {
				printk(KERN_WARNING "ox820sata_irq_handler() Interrupt from SATA port 1 when operating with no uCode\n");
			}
		}

		/* Clear interrupts for either port */
		writel(int_status, OX820SATA_CORE_INT_CLEAR);

		if (ports < 2 && (int_status & (OX820SATA_COREINT_END << 1))) {
			printk(KERN_WARNING "ox820sata_irq_handler() Interrupt from SATA "
				"port 1 when configured for single SATA\n");
		} else {
			u32 port_no;
			smp_rmb();
			for (port_no = 0; port_no < ports; ++port_no) {
				u32 mask = (OX820SATA_COREINT_END << port_no );

				if (int_status & mask) {
					ox820sata_port_irq(((struct ata_host* )dev_instance)->ports[port_no], bug_6320_present);
					ret = IRQ_HANDLED;
				}
			}
		}

		ox820_disklight_led_turn_off();
	}
	return ret;
}

/** 
 * ox820sata_irq_clear is called during probe just before the interrupt handler is
 * registered, to be sure hardware is quiet. It clears and masks interrupt bits
 * in the SATA core.
 *
 * @param ap hardware with the registers in
 */
static void ox820sata_irq_clear(struct ata_port* ap)
{
	u32 *ioaddr = ox820sata_get_io_base(ap);
	//DPRINTK(KERN_INFO"ox820sata_irq_clear\n");

	/* clear pending interrupts */
	writel(~0, ioaddr + OX820SATA_INT_CLEAR);
	writel(OX820SATA_COREINT_END, OX820SATA_CORE_INT_CLEAR);
}

/** 
 * turn on the interrupts
 *
 * @param ap Hardware with the registers in
 */
static void ox820sata_irq_on(struct ata_port *ap)
{
	u32* ioaddr = ox820sata_get_io_base(ap);
	int port_no = ap->port_no;
	u32 mask = OX820SATA_COREINT_END << port_no;

	/* Clear pending interrupts */
	writel(~0, ioaddr + OX820SATA_INT_CLEAR);
	writel(mask, OX820SATA_CORE_INT_STATUS);
	wmb();
	
	/* enable End of command interrupt */
	writel(OX820SATA_INT_WANT, ioaddr + OX820SATA_INT_ENABLE);
	writel(mask, OX820SATA_CORE_INT_ENABLE);
}

/***************************************************************************
* Link Layer
***************************************************************************/
/**
 * allows access to the link layer registers
 * @param link_reg the link layer register to access (oxsemi indexing ie 
 *        00 = static config, 04 = phy ctrl) 
 */
u32 ox820sata_link_read(u32* core_addr, unsigned int link_reg) 
{
	u32 result;
	u32 patience;
	unsigned long flags;
	
	spin_lock_irqsave(&async_register_lock, flags);

	/* accessed twice as a work around for a bug in the SATA abp bridge 
	 * hardware (bug 6828) */
	writel(link_reg, core_addr + OX820SATA_LINK_RD_ADDR );
	readl(core_addr + OX820SATA_LINK_RD_ADDR );

	for (patience = 0x100000; patience > 0; --patience) {
		if (readl(core_addr + OX820SATA_LINK_CONTROL) & 0x00000001) {
			break;
		}
	}

	result = readl(core_addr + OX820SATA_LINK_DATA);
	spin_unlock_irqrestore(&async_register_lock, flags);

	if(fpga) {
		/* report a speed limit of 1.5Gb */
		if ( link_reg == 0x28 ) {
			VPRINTK("Reporting a 1.5Gb speed limit\n");
			result |= 0x00000010 ;
		}
	}
	return result;
}
EXPORT_SYMBOL(ox820sata_link_read);
/** 
 *  Read standard SATA phy registers. Currently only used if 
 * ->phy_reset hook called the sata_phy_reset() helper function.
 *
 * These registers are in another clock domain to the processor, access is via
 * some bridging registers
 *
 * @param ap hardware with the registers in
 * @param sc_reg the SATA PHY register
 * @return the value in the register
 */
static int ox820sata_scr_read_port(struct ata_port *ap, unsigned int sc_reg, u32 *val)
{
	u32* ioaddr = ox820sata_get_io_base(ap);
	*val = ox820sata_link_read(ioaddr, 0x20 + (sc_reg*4));
	return 0;
}

static int ox820sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val)
{
	return ox820sata_scr_read_port(link->ap, sc_reg, val);
}

/**
 * allows access to the link layer registers
 * @param link_reg the link layer register to access (oxsemi indexing ie 
 *        00 = static config, 04 = phy ctrl) 
 */
void ox820sata_link_write(u32* core_addr, unsigned int link_reg, u32 val)
{
	u32 patience;
	unsigned long flags;

	spin_lock_irqsave(&async_register_lock, flags);
	writel(val, core_addr + OX820SATA_LINK_DATA );
	
	/* accessed twice as a work around for a bug in the SATA abp bridge 
	 * hardware (bug 6828) */
	writel(link_reg , core_addr + OX820SATA_LINK_WR_ADDR );
	readl(core_addr + OX820SATA_LINK_WR_ADDR );

	for (patience = 0x100000; patience > 0; --patience) {
		if (readl(core_addr + OX820SATA_LINK_CONTROL) & 0x00000001) {
			break;
		}
	}
	spin_unlock_irqrestore(&async_register_lock, flags);
}

/** 
 *  Write standard SATA phy registers. Currently only used if 
 * phy_reset hook called the sata_phy_reset() helper function.
 *
 * These registers are in another clock domain to the processor, access is via
 * some bridging registers
 *
 * @param ap hardware with the registers in
 * @param sc_reg the SATA PHY register
 * @param val the value to write into the register
 */
static int ox820sata_scr_write_port(struct ata_port *ap, unsigned int sc_reg, u32 val)
{
	u32 *ioaddr = ox820sata_get_io_base(ap);
	ox820sata_link_write(ioaddr, 0x20 + (sc_reg * 4), val);
	return 0;
}

static int ox820sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val)
{
	return ox820sata_scr_write_port(link->ap, sc_reg, val);
}

/** 
 * port_start() is called just after the data structures for each port are
 * initialized. Typically this is used to alloc per-port DMA buffers, tables
 * rings, enable DMA engines and similar tasks.
 *
 * @return 0 = success
 * @param ap hardware with the registers in
 */
static int ox820sata_port_start(struct ata_port *ap)
{
	ox820sata_private_data* pd;
	int dma_channel;

	/* allocate port private data memory and attach to port */    
	pd = (ox820sata_private_data* )kmalloc(sizeof(ox820sata_private_data), GFP_KERNEL);
	if (!pd) {
		return -ENOMEM;
	}

	/* store the ata_port pointer in the driver structure */
	ox820sata_driver.ap[ap->port_no] = ap;

	ap->private_data = pd;
	DPRINTK("ap[%d] = %p, pd = %p\n", ap->port_no, ap, ap->private_data );

	/* initialise */
	if(ports < 2) {
		BUG_ON(ap->port_no);
	}

	dma_channel = ap->port_no;
	
	pd->reg_base = (u32*)(ap->host->iomap + (ap->port_no * OX820SATA_PORT_SIZE));
	pd->dma_controller = 
		(u32* )(SATADMA_REGS_BASE + (dma_channel * OX820SATA_DMA_CORESIZE));  
	pd->sgdma_controller = 
		(u32* )(SATASGDMA_REGS_BASE + (dma_channel * OX820SATA_SGDMA_CORESIZE));  

	/* set-up the sgdma controller addresses */
	pd->sgdma_request_va = (sgdma_request_t* )(OX820SATA_SGDMA_REQ + 
		(dma_channel * sizeof(sgdma_request_t)));
	pd->sgdma_request_pa = (dma_addr_t)(OX820SATA_SGDMA_REQ_PA + 
		(dma_channel * sizeof(sgdma_request_t)));
		
	/* set the PRD table pointers to the space for the PRD tables in SRAM */    
	ap->bmdma_prd = (struct ata_bmdma_prd* )(OX820SATA_PRD +
		(dma_channel * CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * sizeof(struct ata_bmdma_prd)));
	ap->bmdma_prd_dma = (dma_addr_t) (OX820SATA_PRD_PA +
		(dma_channel * CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * sizeof(struct ata_bmdma_prd)));

	/* perform post resetnis initialisation */
	ox820sata_post_reset_init(ap);

	return 0;
}

/** 
 * port_stop() is called after ->host_stop(). It's sole function is to 
 * release DMA/memory resources, now that they are no longer actively being
 * used.
 */
static void ox820sata_port_stop(struct ata_port *ap)
{
	kfree(ap->private_data);
}

static int ox820sata_check_ready(struct ata_link *link)
{
	u8 status = ox820sata_check_status(link->ap);

	return ata_check_ready(status);
}

/** @return true if the port has a cable connected */
int ox820sata_check_link(int port_no) 
{
	int reg;
	struct ata_port* ap = ox820sata_driver.ap[port_no];
	int result = 0;
	if (ap) {
		ox820sata_scr_read_port(ap, SCR_STATUS, &reg );
	
		/* Check for the cable present indicated by SCR status bit-0 set */
		if (reg & 0x1) { 
			result = 1;
		}
	}
	
	return result;
}
EXPORT_SYMBOL( ox820sata_check_link );

/***************************************************************************
* Mode Control
***************************************************************************/
static void configure_ucode_engine(int mode, int reset, int program, int set_params)
{
	if (reset) {
		writel(1, OX820SATA_PROC_RESET);
		wmb();
	}

	if (program) {
		unsigned int *src;
		unsigned int  dst;

		switch (mode) {
		case OXNASSATA_UCODE_RAID0:
		case OXNASSATA_UCODE_RAID1:
			DPRINTK("Loading RAID ucode\n");
			src = (unsigned int*)&ox820_raid_microcode[1];
			break;
		
		case OXNASSATA_UCODE_JBOD:
			DPRINTK("Loading JBOD ucode\n");
			src = (unsigned int*)&ox820_jbod_microcode[1];
			break;
		
		default:
			BUG();
			break;
		}

		dst = OX820SATA_UCODE_STORE;
		while (*src != ~0) {
			writel(*src,dst);
			src++;
			dst += sizeof(*src);
		}
		wmb();
	}

	if (set_params) {
		u32 reg;

		switch (mode) {
		case OXNASSATA_UCODE_RAID0:
		case OXNASSATA_UCODE_RAID1:
			DPRINTK("Enabling H/W supermux for RAID ucode\n");
			
			reg = readl(OX820SATA_DATA_PLANE_CTRL);
			reg |= OX820SATA_DPC_HW_SUPERMUX_AUTO;
			reg &= ~OX820SATA_DPC_FIS_SWCH;
			writel(reg, OX820SATA_DATA_PLANE_CTRL);
			break;
		
		case OXNASSATA_UCODE_JBOD:
			DPRINTK("Disabling H/W supermux for non-RAID ucode\n");
			
			reg = readl(OX820SATA_DATA_PLANE_CTRL);
			reg &= ~OX820SATA_DPC_HW_SUPERMUX_AUTO;
			reg &= ~OX820SATA_DPC_FIS_SWCH;
			writel(reg, OX820SATA_DATA_PLANE_CTRL);
			break;
		
		case OXNASSATA_UCODE_NONE:
			DPRINTK("Enabling H/W supermux for no ucode\n");
			
			reg = readl(OX820SATA_DATA_PLANE_CTRL);
			reg |= OX820SATA_DPC_HW_SUPERMUX_AUTO;
			reg &= ~OX820SATA_DPC_FIS_SWCH;
			writel(reg, OX820SATA_DATA_PLANE_CTRL);
			break;
		
		default:
			BUG();
			break;
		}
		wmb();

		switch (mode) {
		case OXNASSATA_UCODE_RAID0:
			DPRINTK("Configuring ucode engine for RAID0\n");
		
			writel( 0, OX820SATA_RAID_WP_BOT_LOW );
			writel( 0, OX820SATA_RAID_WP_BOT_HIGH);
			writel( 0xffffffff, OX820SATA_RAID_WP_TOP_LOW );
			writel( 0x7fffffff, OX820SATA_RAID_WP_TOP_HIGH);
			writel( 0xffffffff, OX820SATA_RAID_SIZE_LOW   );
			writel( 0x7fffffff, OX820SATA_RAID_SIZE_HIGH  );
			break;
		
		case OXNASSATA_UCODE_RAID1:
			DPRINTK("Configuring ucode engine for RAID1\n");
		
			writel( 0, OX820SATA_RAID_WP_BOT_LOW );
			writel( 0, OX820SATA_RAID_WP_BOT_HIGH);
			writel( 0xffffffff, OX820SATA_RAID_WP_TOP_LOW );
			writel( 0x7fffffff, OX820SATA_RAID_WP_TOP_HIGH);
			writel( 0, OX820SATA_RAID_SIZE_LOW   );
			writel( 0, OX820SATA_RAID_SIZE_HIGH  );
			break;
		
		case OXNASSATA_UCODE_JBOD:
			DPRINTK("Starting JBOD ucode\n");
		
			writel(1, OX820SATA_PROC_START);
			break;
		}
		wmb();
	}
}

void ox820sata_set_mode(u32 mode, u32 force)
{
	if (!force && (mode == current_ucode_mode)) {
		return;
	}

	switch (mode) {
	case OXNASSATA_UCODE_NONE:
		configure_ucode_engine(OXNASSATA_UCODE_NONE, 1, 0, 1);
		break;
	
	case OXNASSATA_UCODE_JBOD:
		configure_ucode_engine(OXNASSATA_UCODE_JBOD, 1, 1, 1);
		break;
	
	case OXNASSATA_UCODE_RAID0:
		switch (current_ucode_mode) {
		case OXNASSATA_UCODE_RAID1:
			configure_ucode_engine(OXNASSATA_UCODE_RAID0, 0, 0, 1);
			break;
		
		default:
			configure_ucode_engine(OXNASSATA_UCODE_RAID0, 1, 1, 1);
			break;
		}
		break;
		
	case OXNASSATA_UCODE_RAID1:
		switch (current_ucode_mode) {
		case OXNASSATA_UCODE_RAID0:
			configure_ucode_engine(OXNASSATA_UCODE_RAID1, 0, 0, 1);
			break;
		
		default:
			configure_ucode_engine(OXNASSATA_UCODE_RAID1, 1, 1, 1);
			break;
		}
		break;
	default:
		BUG();
		break;
	}

	current_ucode_mode = mode;
	return;
}

/***************************************************************************
* Main Driver Code
***************************************************************************/
/**
 *
 * Called after an identify device command has worked out what kind of device
 * is on the port
 *
 * @param port The port to configure
 * @param pdev The hardware associated with controlling the port
 */
static void ox820sata_dev_config(struct ata_device* pdev)
{
	u32 reg;
	u32 *ioaddr = ox820sata_get_io_base(pdev->link->ap);

	/* Set the bits to put the port into 28 or 48-bit node */
	reg = readl(ioaddr + OX820SATA_DRIVE_CONTROL);
	reg &= ~3;
	reg |= (pdev->flags & ATA_DFLAG_LBA48) ? OX820SATA_DR_CON_48 : OX820SATA_DR_CON_28;
	writel(reg, ioaddr + OX820SATA_DRIVE_CONTROL);

	/* if this is an ATA-6 disk, put the port into ATA-5 auto translate mode */
	if (pdev->flags & ATA_DFLAG_LBA48) {
		reg = readl(ioaddr + OX820SATA_PORT_CONTROL);
		reg |= 2;
		writel(reg, ioaddr + OX820SATA_PORT_CONTROL);
	}
}

/**
 *
 */
void ox820sata_freeze_host(int port_no)
{
	set_bit(port_no, &ox820sata_driver.port_in_eh);
	smp_wmb();
}

void ox820sata_thaw_host(int port_no)
{
	clear_bit(port_no, &ox820sata_driver.port_in_eh);
	smp_wmb();
}


/***************************************************************************
* Error Handling
***************************************************************************/

static void ox820sata_error_handler(struct ata_port *ap)
{
	ox820sata_freeze_host(ap->port_no);

	ox820sata_progressive_cleanup();

	ata_std_error_handler(ap);
	/*
	 * The error handling will have gained the SATA core lock either normally,
	 * or by breaking the lock obtained via qc_issue() presumably because the
	 * command has failed and timed-out. In either case we don't expect someone
	 * else to have release the SATA core lock from under us
	 */
	ox820sata_thaw_host(ap->port_no);
}

static void ox820sata_post_internal_cmd(struct ata_queued_cmd *qc) 
{
	if (qc->flags & ATA_QCFLAG_FAILED) {
		ox820sata_progressive_cleanup();
	}
}
