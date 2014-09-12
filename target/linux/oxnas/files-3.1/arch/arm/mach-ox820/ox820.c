/*
 * arch/arm/mach-ox820/ox820.c
 *
 * Copyright (C) 2006,2009 Oxford Semiconductor Ltd
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
 */
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <linux/io.h>
#include <asm/sizes.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/gic.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/rps-irq.h>

#ifdef CONFIG_LEON_START_EARLY
#include <mach/leon.h>
#include <mach/leon-early-prog.h>
#endif // CONFIG_LEON_START_EARLY

extern struct sys_timer oxnas_timer;

// The spinlock exported to allow atomic use of GPIO register set
spinlock_t oxnas_gpio_spinlock;
EXPORT_SYMBOL(oxnas_gpio_spinlock);

// To hold LED inversion state
int oxnas_global_invert_leds = 0;
#include <linux/module.h>
EXPORT_SYMBOL(oxnas_global_invert_leds);

static struct map_desc oxnas_io_desc[] __initdata = {
    { USBHOST_BASE,         __phys_to_pfn(USBHOST_BASE_PA),         SZ_2M,  MT_DEVICE },
    { ETHA_BASE,            __phys_to_pfn(ETHA_BASE_PA),            SZ_2M,  MT_DEVICE },
    { ETHB_BASE,            __phys_to_pfn(ETHB_BASE_PA),            SZ_2M,  MT_DEVICE },
    { USBDEV_BASE,          __phys_to_pfn(USBDEV_BASE_PA),          SZ_2M,  MT_DEVICE },
    { STATIC_CS0_BASE,      __phys_to_pfn(STATIC_CS0_BASE_PA),      SZ_4M,  MT_DEVICE },
    { STATIC_CS1_BASE,      __phys_to_pfn(STATIC_CS1_BASE_PA),      SZ_4M,  MT_DEVICE },
    { STATIC_CONTROL_BASE,  __phys_to_pfn(STATIC_CONTROL_BASE_PA),  SZ_4K,  MT_DEVICE },
    { CIPHER_BASE,          __phys_to_pfn(CIPHER_BASE_PA),          SZ_2M,  MT_DEVICE },
    { GPIO_A_BASE,          __phys_to_pfn(GPIO_A_BASE_PA),          SZ_4K,  MT_DEVICE },
    { GPIO_B_BASE,          __phys_to_pfn(GPIO_B_BASE_PA),          SZ_4K,  MT_DEVICE },
    { UART_1_BASE,          __phys_to_pfn(UART_1_BASE_PA),          SZ_16,  MT_DEVICE },
    { UART_2_BASE,          __phys_to_pfn(UART_2_BASE_PA),          SZ_16,  MT_DEVICE },
    { RPSA_BASE,            __phys_to_pfn(RPSA_BASE_PA),            SZ_1K,  MT_DEVICE },
    { RPSC_BASE,            __phys_to_pfn(RPSC_BASE_PA),            SZ_1K,  MT_DEVICE },
    { FAN_MON_BASE,         __phys_to_pfn(FAN_MON_BASE_PA),         SZ_1M,  MT_DEVICE },
    { DDR_REGS_BASE,        __phys_to_pfn(DDR_REGS_BASE_PA),        SZ_1M,  MT_DEVICE },
    { IRRX_BASE,            __phys_to_pfn(IRRX_BASE_PA),            SZ_1M,  MT_DEVICE },
    { SATA_PHY_BASE,        __phys_to_pfn(SATA_PHY_BASE_PA),        SZ_1M,  MT_DEVICE },
    { PCIE_PHY,             __phys_to_pfn(PCIE_PHY_PA),             SZ_1M,  MT_DEVICE },
    { AHB_MON_BASE,         __phys_to_pfn(AHB_MON_BASE_PA),         SZ_1M,  MT_DEVICE },
    { SYS_CONTROL_BASE,     __phys_to_pfn(SYS_CONTROL_BASE_PA),     SZ_1M,  MT_DEVICE },
    { SEC_CONTROL_BASE,     __phys_to_pfn(SEC_CONTROL_BASE_PA),     SZ_1M,  MT_DEVICE },
    { SD_REG_BASE,          __phys_to_pfn(SD_REG_BASE_PA),          SZ_1M,  MT_DEVICE },
    { AUDIO_BASE,           __phys_to_pfn(AUDIO_BASE_PA),           SZ_1M,  MT_DEVICE },
    { DMA_BASE,             __phys_to_pfn(DMA_BASE_PA),             SZ_1M,  MT_DEVICE },
    { CIPHER_REG_BASE,      __phys_to_pfn(CIPHER_REG_BASE_PA),      SZ_1M,  MT_DEVICE },
    { SATA_REG_BASE,        __phys_to_pfn(SATA_REG_BASE_PA),        SZ_1M,  MT_DEVICE },
    { COPRO_REGS_BASE,      __phys_to_pfn(COPRO_REGS_BASE_PA),      SZ_1M,  MT_DEVICE },
    { PERIPH_BASE,          __phys_to_pfn(PERIPH_BASE_PA),          SZ_8K,  MT_DEVICE },
    { PCIEA_DBI_BASE,       __phys_to_pfn(PCIEA_DBI_BASE_PA),       SZ_1M,  MT_DEVICE },
    { PCIEA_ELBI_BASE,      __phys_to_pfn(PCIEA_ELBI_BASE_PA),      SZ_1M,  MT_DEVICE },
    { PCIEB_DBI_BASE,       __phys_to_pfn(PCIEB_DBI_BASE_PA),       SZ_1M,  MT_DEVICE },
    { PCIEB_ELBI_BASE,      __phys_to_pfn(PCIEB_ELBI_BASE_PA),      SZ_1M,  MT_DEVICE },
    { PCIEA_CLIENT_BASE,	__phys_to_pfn(PCIEA_CLIENT_BASE_PA),	SZ_64M,	MT_DEVICE },
    { PCIEB_CLIENT_BASE,	__phys_to_pfn(PCIEB_CLIENT_BASE_PA),	SZ_64M,	MT_DEVICE }

#ifdef CONFIG_SUPPORT_LEON
	/*
	 * Upto 6 pages for Leon program/data/stack
	 */
#if (CONFIG_LEON_PAGES == 1)
   ,{ LEON_IMAGE_BASE,			__phys_to_pfn(LEON_IMAGE_BASE_PA),			SZ_4K, MT_DEVICE }
#elif (CONFIG_LEON_PAGES == 2)
   ,{ LEON_IMAGE_BASE,			__phys_to_pfn(LEON_IMAGE_BASE_PA),			SZ_8K, MT_DEVICE }
#elif (CONFIG_LEON_PAGES == 3)
   ,{ LEON_IMAGE_BASE,		    __phys_to_pfn(LEON_IMAGE_BASE_PA),			SZ_8K, MT_DEVICE }
   ,{ LEON_IMAGE_BASE+0x2000,	__phys_to_pfn(LEON_IMAGE_BASE_PA+0x2000),	SZ_4K, MT_DEVICE }
#elif (CONFIG_LEON_PAGES == 4)
   ,{ LEON_IMAGE_BASE,		    __phys_to_pfn(LEON_IMAGE_BASE_PA),	  		SZ_8K, MT_DEVICE }
   ,{ LEON_IMAGE_BASE+0x2000,	__phys_to_pfn(LEON_IMAGE_BASE_PA+0x2000),	SZ_8K, MT_DEVICE }
#elif (CONFIG_LEON_PAGES == 5)
   ,{ LEON_IMAGE_BASE,		    __phys_to_pfn(LEON_IMAGE_BASE_PA),	  		SZ_16K, MT_DEVICE }
   ,{ LEON_IMAGE_BASE+0x4000,	__phys_to_pfn(LEON_IMAGE_BASE_PA+0x4000),	SZ_4K,  MT_DEVICE }
#elif (CONFIG_LEON_PAGES == 6)
   ,{ LEON_IMAGE_BASE,		    __phys_to_pfn(LEON_IMAGE_BASE_PA),	  		SZ_16K, MT_DEVICE }
   ,{ LEON_IMAGE_BASE+0x4000,	__phys_to_pfn(LEON_IMAGE_BASE_PA+0x4000),	SZ_8K,  MT_DEVICE }
#else
#error "Unsupported number of Leon code pages"
#endif // CONFIG_LEON_PAGES
#endif // CONFIG_SUPPORT_LEON
	/*
	 * Upto 10 pages for GMAC/DMA descriptors plus ARM/Leon workspace if
	 * offloading in use
	 */
   ,{ SRAM_BASE,		__phys_to_pfn(SRAM_PA),			SZ_16K,	MT_DEVICE }
   ,{ SRAM_BASE+0x4000,	__phys_to_pfn(SRAM_PA+0x4000),	SZ_16K,	MT_DEVICE }
   ,{ SRAM_BASE+0x8000,	__phys_to_pfn(SRAM_PA+0x8000),	SZ_8K,	MT_DEVICE }
};

static struct resource usb_resources[] = {
	[0] = {
		.start		= USBHOST_BASE_PA + 0x100,
		.end		= USBHOST_BASE_PA + 0x10100 - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= USBHOST_INTERRUPT,
		.end		= USBHOST_INTERRUPT,
		.flags		= IORESOURCE_IRQ,
	},
};

static u64 usb_dmamask = ~(u32)0;

static struct platform_device usb_host = {
	.name		= "oxnas-ehci",
	.id		= 0,
	.dev = {
		.dma_mask		= &usb_dmamask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(usb_resources),
	.resource	= usb_resources,
};

static struct platform_device *platform_devices[] __initdata = {
	&usb_host,
};

/* used by entry-macro.S */
//void __iomem *gic_cpu_base_addr;

#define STD_COM_FLAGS (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST )

#define INT_UART_BASE_BAUD (CONFIG_NOMINAL_RPSCLK_FREQ)

#ifdef CONFIG_ARCH_OXNAS_UART1
static struct uart_port internal_serial_port_1 = {
	.membase	= (char *)(UART_1_BASE),
	.mapbase	= UART_1_BASE_PA,
	.irq		= UART_1_INTERRUPT,
	.flags		= STD_COM_FLAGS,
	.iotype		= UPIO_MEM,
	.regshift	= 0,
	.uartclk	= INT_UART_BASE_BAUD,
	.line		= 0,
	.type		= PORT_16550A,
	.fifosize	= 16
};
#endif // CONFIG_ARCH_OXNAS_UART1

#ifdef CONFIG_ARCH_OXNAS_UART2
static struct uart_port internal_serial_port_2 = {
	.membase	= (char *)(UART_2_BASE),
	.mapbase	= UART_2_BASE_PA,
	.irq		= UART_2_INTERRUPT,
	.flags		= STD_COM_FLAGS,
	.iotype		= UPIO_MEM,
	.regshift	= 0,
	.uartclk	= INT_UART_BASE_BAUD,
	.line		= 0,
	.type		= PORT_16550A,
	.fifosize	= 16
};
#endif // CONFIG_ARCH_OXNAS_UART2

static void __init oxnas_mapio(void)
{
    unsigned int uart_line=0;

    // Setup kernel mappings for hardware cores
    iotable_init(oxnas_io_desc, ARRAY_SIZE(oxnas_io_desc));

#ifdef CONFIG_ARCH_OXNAS_UART1
#if (CONFIG_ARCH_OXNAS_CONSOLE_UART != 1)
    {
		// Route UART1 SOUT and SIN onto external pins
		unsigned long pins = (1UL << UART_A_SIN_GPIOA_PIN) |
						 	 (1UL << UART_A_SOUT_GPIOA_PIN);

        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_QUATERNARY_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL       &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL |=  pins;

		// Setup GPIO line direction for UART1 SOUT
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_SET   |= (1UL << UART_A_SOUT_GPIOA_PIN);
	
		// Setup GPIO line direction for UART1 SIN
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_CLEAR |= (1UL << UART_A_SIN_GPIOA_PIN);
    }
#endif // (CONFIG_ARCH_OXNAS_CONSOLE_UART != 1)

#ifdef CONFIG_ARCH_OXNAS_UART1_MODEM
    {
		// Route UART1 modem control line onto external pins
		unsigned long pins = (1UL << UART_A_CTS_GPIOA_PIN) |
							 (1UL << UART_A_RTS_GPIOA_PIN);

        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_QUATERNARY_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL       &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL |=  pins;

		pins = (1UL << UART_A_RI_GPIOA_PIN) |
			   (1UL << UART_A_CD_GPIOA_PIN) |
			   (1UL << UART_A_DSR_GPIOA_PIN) |
			   (1UL << UART_A_DTR_GPIOA_PIN);

        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_QUATERNARY_SEL  &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL       |=  pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL &= ~pins;

		// Setup RI, CD, DSR, CTS as outputs
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_SET |= ((1UL << UART_A_RI_GPIOA_PIN) |
													 (1UL << UART_A_CD_GPIOA_PIN) |
													 (1UL << UART_A_DSR_GPIOA_PIN) |
													 (1UL << UART_A_CTS_GPIOA_PIN));

		// Setup DTR, RTS as inputs
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_CLEAR |= ((1UL << UART_A_DTR_GPIOA_PIN) |
													   (1UL << UART_A_RTS_GPIOA_PIN));
    }
#endif // CONFIG_ARCH_OXNAS_UART1_MODEM

    // Give Linux a contiguous numbering scheme for available UARTs
    internal_serial_port_1.line = uart_line++;
    early_serial_setup(&internal_serial_port_1);
#endif // CONFIG_ARCH_OXNAS_UART1

#ifdef CONFIG_ARCH_OXNAS_UART2
#if (CONFIG_ARCH_OXNAS_CONSOLE_UART != 2)
    {
		// Route UART2 SOUT and SIN onto external pins
		unsigned long pins = (1UL << UART_B_SIN_GPIOA_PIN) |
							 (1UL << UART_B_SOUT_GPIOA_PIN);

        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_QUATERNARY_SEL  &= ~pins ;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL       |=  pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL &= ~pins;

		// Setup GPIO line direction for UART2 SOUT
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_SET   |= (1UL << UART_B_SOUT_GPIOA_PIN);

		// Setup GPIO line direction for UART2 SIN
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_CLEAR |= (1UL << UART_B_SIN_GPIOA_PIN);
    }
#endif // (CONFIG_ARCH_OXNAS_CONSOLE_UART != 2)

#ifdef CONFIG_ARCH_OXNAS_UART2_MODEM
    {
		// Route UART2 modem control line onto external pins
		unsigned long pins = (1UL << UART_B_CTS_GPIOA_PIN) |
							 (1UL << UART_B_RTS_GPIOA_PIN);

        *(volatile unsigned long*)SYS_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SYS_CTRL_QUATERNARY_SEL  &= ~pins ;
        *(volatile unsigned long*)SYS_CTRL_DEBUG_SEL       |=  pins;
        *(volatile unsigned long*)SYS_CTRL_ALTERNATIVE_SEL &= ~pins;

		pins = (1UL << UART_B_RI_GPIOB_PIN) |
			   (1UL << UART_B_CD_GPIOB_PIN) |
			   (1UL << UART_B_DSR_GPIOB_PIN) |
			   (1UL << UART_B_DTR_GPIOB_PIN);

        *(volatile unsigned long*)SEC_CTRL_SECONDARY_SEL   &= ~pins;
        *(volatile unsigned long*)SEC_CTRL_TERTIARY_SEL    &= ~pins;
        *(volatile unsigned long*)SEC_CTRL_QUATERNARY_SEL  &= ~pins ;
        *(volatile unsigned long*)SEC_CTRL_DEBUG_SEL       |=  pins;
        *(volatile unsigned long*)SEC_CTRL_ALTERNATIVE_SEL &= ~pins;

		// Setup RI, CD, DSR, CTS as outputs
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_SET |= (1UL << UART_B_CTS_GPIOA_PIN);

		*(volatile u32*)GPIO_B_OUTPUT_ENABLE_SET |= ((1UL << UART_B_RI_GPIOB_PIN) |
													 (1UL << UART_B_CD_GPIOB_PIN) |
													 (1UL << UART_B_DSR_GPIOB_PIN));

		// Setup DTR, RTS as inputs
		*(volatile u32*)GPIO_A_OUTPUT_ENABLE_CLEAR |= (1UL << UART_B_RTS_GPIOA_PIN);

		*(volatile u32*)GPIO_B_OUTPUT_ENABLE_CLEAR |= (1UL << UART_B_DTR_GPIOB_PIN);
	}
#endif // CONFIG_ARCH_OXNAS_UART2_MODEM

    // Give Linux a contiguous numbering scheme for available UARTs
    internal_serial_port_2.line = uart_line++;
    early_serial_setup(&internal_serial_port_2);
#endif // CONFIG_ARCH_OXNAS_UART2

    /* Both Ethernet cores will use Ethernet 0 MDIO interface */
    {
        unsigned long pins = (1 << MACA_MDC_MF_PIN ) |
                             (1 << MACA_MDIO_MF_PIN) ;
        *(volatile u32*)SYS_CTRL_SECONDARY_SEL   |=  pins ;
        *(volatile u32*)SYS_CTRL_TERTIARY_SEL    &= ~pins ;
        *(volatile u32*)SYS_CTRL_QUATERNARY_SEL  &= ~pins ;
        *(volatile u32*)SYS_CTRL_DEBUG_SEL       &= ~pins ;
        *(volatile u32*)SYS_CTRL_ALTERNATIVE_SEL &= ~pins ;
	}
#define CONFIG_OXNAS_PCIE_RESET_GPIO 44
#ifdef	CONFIG_PCI
#if (CONFIG_OXNAS_PCIE_RESET_GPIO < SYS_CTRL_NUM_PINS)
#define PCIE_RESET_PIN		CONFIG_OXNAS_PCIE_RESET_GPIO
#define	PCIE_RESET_SECONDARY_SEL	SYS_CTRL_SECONDARY_SEL
#define	PCIE_RESET_TERTIARY_SEL	SYS_CTRL_TERTIARY_SEL
#define	PCIE_RESET_QUATERNARY_SEL	SYS_CTRL_QUATERNARY_SEL
#define	PCIE_RESET_DEBUG_SEL	SYS_CTRL_DEBUG_SEL
#define	PCIE_RESET_ALTERNATIVE_SEL	SYS_CTRL_ALTERNATIVE_SEL
#else
#define PCIE_RESET_PIN          (CONFIG_OXNAS_PCIE_RESET_GPIO - SYS_CTRL_NUM_PINS)
#define	PCIE_RESET_SECONDARY_SEL	SEC_CTRL_SECONDARY_SEL
#define	PCIE_RESET_TERTIARY_SEL	SEC_CTRL_TERTIARY_SEL
#define	PCIE_RESET_QUATERNARY_SEL	SEC_CTRL_QUATERNARY_SEL
#define	PCIE_RESET_DEBUG_SEL	SEC_CTRL_DEBUG_SEL
#define	PCIE_RESET_ALTERNATIVE_SEL	SEC_CTRL_ALTERNATIVE_SEL
#endif
    {
	// PCIe card reset line
	unsigned long pin = ( 1 << PCIE_RESET_PIN);
        *(volatile u32*)PCIE_RESET_SECONDARY_SEL   &= ~pin ;
        *(volatile u32*)PCIE_RESET_TERTIARY_SEL    &= ~pin ;
        *(volatile u32*)PCIE_RESET_QUATERNARY_SEL  &= ~pin ;
        *(volatile u32*)PCIE_RESET_DEBUG_SEL       &= ~pin ;
        *(volatile u32*)PCIE_RESET_ALTERNATIVE_SEL &= ~pin ;
    }
#endif // CONFIG_PCI
}

static void __init oxnas_fixup(
    struct machine_desc *desc,
    struct tag *tags,
    char **cmdline,
    struct meminfo *mi)
{
    mi->nr_banks = 0;
    mi->bank[mi->nr_banks].start = SDRAM_PA;
    mi->bank[mi->nr_banks].size  = SDRAM_SIZE;
//    mi->bank[mi->nr_banks].node = mi->nr_banks;
    ++mi->nr_banks;
#ifdef CONFIG_DISCONTIGMEM
    mi->bank[mi->nr_banks].start = SRAM_PA;
    mi->bank[mi->nr_banks].size  = SRAM_SIZE;
#ifdef LEON_IMAGE_IN_SRAM
    mi->bank[mi->nr_banks].size -= LEON_IMAGE_SIZE;
#endif
  //  mi->bank[mi->nr_banks].node = mi->nr_banks;
    ++mi->nr_banks;
#endif

printk(KERN_NOTICE "%d memory %s\n", mi->nr_banks, (mi->nr_banks > 1) ? "regions" : "region");
}

#if defined(CONFIG_LEON_POWER_BUTTON_MONITOR) || defined(CONFIG_LEON_POWER_BUTTON_MONITOR_MODULE)
#include <mach/leon.h>
#include <mach/leon-power-button-prog.h>
#endif // CONFIG_LEON_POWER_BUTTON_MONITOR

static void arch_poweroff(void)
{
#if 0
#if defined(CONFIG_LEON_POWER_BUTTON_MONITOR) || defined(CONFIG_LEON_POWER_BUTTON_MONITOR_MODULE)
    // Load CoPro program and start it running
    init_copro(leon_srec, oxnas_global_invert_leds);
#endif // CONFIG_LEON_POWER_BUTTON_MONITOR

#endif
}

static void __init oxnas_init_machine(void)
{
    /* Initialise the spinlock used to make GPIO register set access atomic */
    spin_lock_init(&oxnas_gpio_spinlock);

    /*
     * Initialise the support for our multi-channel memory-to-memory DMAC
     * The interrupt subsystem needs to be available before we can initialise
     * the DMAC support
     */
//    oxnas_dma_init();

#ifdef CONFIG_LEON_START_EARLY
    init_copro(leon_early_srec, 0);
#endif // CONFIG_LEON_START_EARLY

	// Add any platform bus devices
	platform_add_devices(platform_devices, ARRAY_SIZE(platform_devices));

	pm_power_off = arch_poweroff;

}

/*
 * Code to setup the interrupts
 */
static void __init oxnas_init_irq(void)
{
    /* initialise the RPS interrupt controller */
    OX820_RPS_init_irq(OX820_RPS_IRQ_START, OX820_RPS_IRQ_START + NR_RPS_IRQS);

    /* initialise the GIC */
    gic_init(0, 29, __io_address(OX820_GIC_DIST_BASE_ADDR),
              __io_address(OX820_GIC_CPU_BASE_ADDR));
    
    OX820_RPS_cascade_irq( RPSA_IRQ_INTERRUPT);
}

MACHINE_START(OX820, "PogoPlug Pro")
    /* Maintainer: Oxford Semiconductor Ltd */
#if 0
#ifdef CONFIG_ARCH_OXNAS_UART1
    .phys_io = UART_1_BASE_PA,
    .io_pg_offst = (((u32)UART_1_BASE) >> 18) & 0xfffc,
#elif defined(CONFIG_ARCH_OXNAS_UART2)
    .phys_io = UART_2_BASE_PA,
    .io_pg_offst = (((u32)UART_2_BASE) >> 18) & 0xfffc,
#endif
#endif    
    .boot_params = SDRAM_PA + 0x100,
    .fixup = oxnas_fixup,
    .map_io = oxnas_mapio,
    .init_irq = oxnas_init_irq,
    .timer = &oxnas_timer,
    .init_machine = oxnas_init_machine,
MACHINE_END
