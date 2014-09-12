/*
 * arch/arm/mach-ox820/pci.c
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
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/mach/pci.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/system.h>

#define VERSION_ID_MAGIC 0x082510b5

static int link_up[2];
static DEFINE_SPINLOCK(pciea_lock);
static DEFINE_SPINLOCK(pcieb_lock);

void __iomem * ox820_ioremap(
	unsigned long phys_addr,
	size_t        size,
	unsigned int  mtype)
{
	/* For non-PCI accesses use std ARM ioremap */
	if (!is_pci_phys_addr(phys_addr)) {
//printk("__ox820_ioremap() Non-PCI phys addr %p\n", (void*)phys_addr);
		return __arm_ioremap(phys_addr, size, mtype);
	}

	/* PCI accesses use the unmodified physical address */
//printk("__ox820_ioremap() PCI phys addr %p\n", (void*)phys_addr);
	return (void*)pci_phys_to_virt(phys_addr);
}
EXPORT_SYMBOL(ox820_ioremap);

void ox820_iounmap(void __iomem *virt_addr)
{
	/* PCI addresses are not ioremapped */
	if (!is_pci_virt_addr((u32)virt_addr)) {
//printk("__ox820_iounmap() Non-PCI virt addr %p\n", virt_addr);
		__iounmap(virt_addr);
	}
//else
//printk("__ox820_iounmap() PCI virt addr %p - not unmapping\n", virt_addr);
}
EXPORT_SYMBOL(ox820_iounmap);

static inline int controller_index_from_phys(u32 phys)
{
	return phys >= PCIEB_CLIENT_BASE_PA;
}

static inline int controller_index_from_virt(u32 virt)
{
	return virt >= PCIEB_CLIENT_BASE;
}

unsigned long __ox820_inl(u32 phys)
{
	unsigned long           value;

	if (!link_up[controller_index_from_phys(phys)]) {
//printk("inl() phys %p -> link down", (void*)phys);
		value = ~0UL;
	} else {
		u32 virt = pci_phys_to_virt(phys);

//printk("inl() phys %p -> virt %p", (void*)phys, (void*)virt);
		value = __raw_readl(virt);
//printk(" value %p\n", (void*)value);
	}

	return value;
}

unsigned short __ox820_inw(u32 phys)
{
	/* Only 32-bit accesses are working on the PCIe controller */
	unsigned short value;

//printk("inw() Adr %p, calling inl() to avoid non-quad access\n", (void*)phys);
	value = (unsigned short)(inl(phys & ~3) >> (phys | 3));
//printk("inw() value after shifting %p\n", (void*)(unsigned long)value);
	return value;
}

unsigned char __ox820_inb(u32 phys)
{
	/* Only 32-bit accesses are working on the PCIe controller */
	unsigned char value;

//printk("inb() Adr %p, calling inl() to avoid non-quad access\n", (void*)phys);
	value = (unsigned char)(inl(phys & ~3) >> (phys | 3));
//printk("inb() value after shifting %p\n", (void*)(unsigned long)value);
	return value;
}

/**
 * Todo: will need locking around slave control register modifications
 *  if it turns out we really do have to do this mucking around with
 *  byte enables
 */
static void inline set_out_lanes(
	u32           virt,
	unsigned char lanes)
{
	/* Each PCIe core has a separate slave control register */
	unsigned long slave_reg_adr =
		controller_index_from_virt(virt) ? SYS_CTRL_PCIEB_AHB_SLAVE_CTRL :
										   SYS_CTRL_PCIEA_AHB_SLAVE_CTRL;

	/* Enable only the requested byte lanes for output */
	unsigned long slave_reg_val = readl(slave_reg_adr);
	slave_reg_val &= ~(0xf << SYS_CTRL_PCIE_SLAVE_BE_BIT);
	slave_reg_val |= (lanes << SYS_CTRL_PCIE_SLAVE_BE_BIT);
	writel(slave_reg_val, slave_reg_adr);
}

static void out_lanes(
	u32           value,
	u32           phys,
	unsigned char lanes)
{
//printk("out_lanes() value %p, phys %p, lanes %p\n", (void*)(unsigned long)value, (void*)phys, (void*)(unsigned long)lanes);

	if (!link_up[controller_index_from_phys(phys)]) {
//printk("out_lanes() %p to phys %p -> link down", (void*)value, (void*)phys);
	} else {
		unsigned long flags;
		u32           virt = pci_phys_to_virt(phys);

		if (lanes != 0xf) {
			/* Lock use of PCIe out lane control register */
			spin_lock_irqsave(
				controller_index_from_phys(phys) ? &pcieb_lock : &pciea_lock, flags);

			/* Enable only the requested byte lanes for output */
			set_out_lanes(virt, lanes);
		}

//printk("out_lanes() %p to phys %p -> virt %p, lanes 0x%p", (void*)value, (void*)phys, (void*)virt, (void*)(unsigned long)lanes);
		/* Do the quad-aligned write */
		__raw_writel(value, virt);
//printk(" OK\n");

		if (lanes != 0xf) {
			/**
			 * Re-enable all byte lanes for output
			 * NB: are we sure the previous PCIe access has completed at this point
			 *     and thus it's safe to change the byte enables?
			 */
			set_out_lanes(virt, 0xf);

			/* Unlock use of PCIe out lane control register */
			spin_unlock_irqrestore(
				controller_index_from_phys(phys) ? &pcieb_lock : &pciea_lock, flags);
		}
	}
}

void __ox820_outl(
	unsigned long value,
	u32           phys)
{
//printk("outl() value %p, addr %p\n", (void*)(unsigned long)value, (void*)phys);
	out_lanes(value, phys, 0xf);
}

void __ox820_outw(
	unsigned short value,
	u32            phys)
{
	u32 quad_val = (u32)value << (8 * (phys & 3));
//printk("outw() value %p, addr %p, quad_val %p\n", (void*)(unsigned long)value, (void*)phys, (void*)quad_val);
	out_lanes(quad_val, phys & ~3, 3 << (phys & 3));
}

void __ox820_outb(
	unsigned char value,
	u32           phys)
{
	u32 quad_val = (unsigned long)value << (8 * (phys & 3));
//printk("outb() value %p, addr %p, quad_val %p\n", (void*)(u32)value, (void*)phys, (void*)quad_val);
	out_lanes(quad_val, phys & ~3, 1 << (phys & 3));
}

void __ox820_outsb(u32 p, unsigned char  * from, u32 len)	{ while (len--) { __ox820_outb((*from++),(p) ); } }
void __ox820_outsw(u32 p, unsigned short * from, u32 len)	{ while (len--) { __ox820_outw((*from++),(p) ); } }
void __ox820_outsl(u32 p, unsigned long  * from, u32 len)	{ while (len--) { __ox820_outl((*from++),(p) ); } }
                                  
void __ox820_insb(u32 p, unsigned char  * to, u32 len)	{ while (len--) { *to++ = __ox820_inb(p); } }
void __ox820_insw(u32 p, unsigned short * to, u32 len)	{ while (len--) { *to++ = __ox820_inw(p); } }
void __ox820_insl(u32 p, unsigned long  * to, u32 len)	{ while (len--) { *to++ = __ox820_inl(p); } }

EXPORT_SYMBOL(__ox820_inb);
EXPORT_SYMBOL(__ox820_inw);
EXPORT_SYMBOL(__ox820_inl);

EXPORT_SYMBOL(__ox820_outb);
EXPORT_SYMBOL(__ox820_outw);
EXPORT_SYMBOL(__ox820_outl);

EXPORT_SYMBOL(__ox820_insb);
EXPORT_SYMBOL(__ox820_insw);
EXPORT_SYMBOL(__ox820_insl);

EXPORT_SYMBOL(__ox820_outsb);
EXPORT_SYMBOL(__ox820_outsw);
EXPORT_SYMBOL(__ox820_outsl);

#ifdef CONFIG_PCI
#ifdef CONFIG_ARCH_OXNAS_PCIE_DISABLE
#else /* !CONFIG_ARCH_OXNAS_PCIE_DISABLE */

#define LINK_UP_TIMEOUT_SECONDS 3

#define TOTAL_WINDOW_SIZE	(64*1024*1024)

#define NON_PREFETCHABLE_WINDOW_SIZE	(32*1024*1024)
#define PREFETCHABLE_WINDOW_SIZE		(30*1024*1024)
#define IO_WINDOW_SIZE					(1*1024*1024)

#if ((NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE + IO_WINDOW_SIZE) >= TOTAL_WINDOW_SIZE)
#error "PCIe windows sizes incorrect"
#endif

#define NON_PREFETCHABLE_WINDOW_OFFSET	0 
#define PREFETCHABLE_WINDOW_OFFSET		(NON_PREFETCHABLE_WINDOW_SIZE)
#define IO_WINDOW_OFFSET				(NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE)
#define CONFIG_WINDOW_OFFSET			(NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE + IO_WINDOW_SIZE)

static int __init ox820_map_irq(
	const struct pci_dev *dev,
	u8              slot,
	u8              pin)
{
//printk(KERN_INFO "ox820_map_irq(): dev->bus->number = %d, returning IRQ %d\n", dev->bus->number, dev->bus->number ? PCIEB_INTERRUPT : PCIEA_INTERRUPT);
	return dev->bus->number ? PCIEB_INTERRUPT : PCIEA_INTERRUPT;
}

static struct resource pciea_non_mem = {
	.name	= "PCIeA non-prefetchable",
	.start	= PCIEA_CLIENT_BASE_PA + NON_PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM,
};

static struct resource pciea_pre_mem = {
	.name	= "PCIeA prefetchable",
	.start	= PCIEA_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + IO_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM | IORESOURCE_PREFETCH,
};

static struct resource pciea_io_mem = {
	.name	= "PCIeA I/O space",
	.start	= PCIEA_CLIENT_BASE_PA + IO_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + CONFIG_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_IO,
};

static int __init ox820_pciea_setup_resources(struct resource **resource)
{
	int ret = 0;

        /* Enable PCIe Pre-Emphasis: */
        printk(KERN_INFO
               "ox820_pcie_setup_resources() "
               "Enabling PCIe Pre-Emphasis\n");
        writel(0x14, PCIE_PHY);
        writel(0x4ce10, PCIE_PHY + 4);
        writel(0x2ce10, PCIE_PHY + 4);
        writel(0x2004, PCIE_PHY);
        writel(0x482c7, PCIE_PHY + 4);
        writel(0x282c7, PCIE_PHY + 4);

        printk(KERN_INFO "ox820_pciea_setup_resources() resource %p\n", resource);
        printk(KERN_INFO "ox820_pciea_setup_resources()    io:      0x%08x - 0x%08x\n",
               pciea_io_mem.start, pciea_io_mem.end);
        printk(KERN_INFO "ox820_pciea_setup_resources()    non-pre: 0x%08x - 0x%08x\n",
               pciea_non_mem.start, pciea_non_mem.end);
        printk(KERN_INFO "ox820_pciea_setup_resources()    pre:     0x%08x - 0x%08x\n",
               pciea_pre_mem.start, pciea_pre_mem.end);

	ret = request_resource(&iomem_resource, &pciea_io_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate I/O memory region (%d)\n", ret);
		goto out;
	}
	ret = request_resource(&iomem_resource, &pciea_non_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate non-prefetchable memory region (%d)\n", ret);
		goto release_io_mem;
	}
	ret = request_resource(&iomem_resource, &pciea_pre_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate prefetchable memory region (%d)\n", ret);
		goto release_non_mem;
	}

	/*
	 * bus->resource[0] is the IO resource for this bus
	 * bus->resource[1] is the mem resource for this bus
	 * bus->resource[2] is the prefetch mem resource for this bus
	 */
	resource[0] = &pciea_io_mem;
	resource[1] = &pciea_non_mem;
	resource[2] = &pciea_pre_mem;

	goto out;

release_non_mem:
	release_resource(&pciea_non_mem);
release_io_mem:
	release_resource(&pciea_io_mem);
out:
	return ret;
}

static struct resource pcieb_non_mem = {
	.name	= "PCIeB non-prefetchable",
	.start	= PCIEB_CLIENT_BASE_PA + NON_PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM,
};

static struct resource pcieb_pre_mem = {
	.name	= "PCIeB prefetchable",
	.start	= PCIEB_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + IO_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM | IORESOURCE_PREFETCH,
};

static struct resource pcieb_io_mem = {
	.name	= "PCIeB I/O space",
	.start	= PCIEB_CLIENT_BASE_PA + IO_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + CONFIG_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_IO,
};

static int __init ox820_pcieb_setup_resources(struct resource **resource)
{
	int ret = 0;

//printk(KERN_INFO "ox820_pcieb_setup_resources() resource %p\n", resource);
	ret = request_resource(&iomem_resource, &pcieb_io_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate I/O memory region (%d)\n", ret);
		goto out;
	}
	ret = request_resource(&iomem_resource, &pcieb_non_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate non-prefetchable memory region (%d)\n", ret);
		goto release_io_mem;
	}
	ret = request_resource(&iomem_resource, &pcieb_pre_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate prefetchable memory region (%d)\n", ret);
		goto release_non_mem;
	}

	/*
	 * bus->resource[0] is the IO resource for this bus
	 * bus->resource[1] is the mem resource for this bus
	 * bus->resource[2] is the prefetch mem resource for this bus
	 */
	resource[0] = &pcieb_io_mem;
	resource[1] = &pcieb_non_mem;
	resource[2] = &pcieb_pre_mem;

	goto out;

release_non_mem:
	release_resource(&pcieb_non_mem);
release_io_mem:
	release_resource(&pcieb_io_mem);
out:
	return ret;
}

int __init ox820_pci_setup(
	int                  nr,
	struct pci_sys_data *sys)
{
	int ret = 0;
//printk(KERN_INFO "ox820_pci_setup() nr %d, sys %p, sys->busnr %d, sys->bus %p\n", nr, sys, sys->busnr, sys->bus);

	if (nr == 0) {
		sys->mem_offset = 0;
		ret = ox820_pciea_setup_resources(sys->resource);
		if (ret < 0) {
			printk(KERN_ERR "ox820_pci_setup: Failed to setup PCIeA resources\n");
			goto out;
		}
	} else if (nr == 1) {
		sys->mem_offset = 0;
		ret = ox820_pcieb_setup_resources(sys->resource);
		if (ret < 0) {
			printk(KERN_ERR "ox820_pci_setup: Failed to setup PCIeA resources\n");
			goto out;
		}
	}
	return 1;

out:
	return ret;
}

static int controller_index_from_bus(int primary_bus_number)
{
	return !!primary_bus_number;
}

/**
 * Returns the virtual address matching the supplied PCIe info
 */
static unsigned long pci_addr(
	int           primary_bus_number,
	unsigned char bus_number,
	unsigned int  devfn,
	int           where)
{
	unsigned int  slot = PCI_SLOT(devfn);
	unsigned int  function = PCI_FUNC(devfn);
	int           controller;
	unsigned char modified_bus_number;
	unsigned long addr;

	/* Only a single device per bus for PCIe point-to-point links */
	BUG_ON(slot != 0);
	
	/*
	 * The first controller will have been assigned bus number 0, whereas the
	 * second controller will have a bus number after the last possible bus
	 * number on the first controller I believe
	 */
	controller = controller_index_from_bus(primary_bus_number);

	/*
	 * Calculate the controller-local bus number to use to offset into the
	 * controller's configuration space
	 */
	modified_bus_number = bus_number - primary_bus_number;

//printk(KERN_INFO "pci_addr() Controller %d, modified_bus_number %d\n", controller, modified_bus_number);

	/* Get the start address of the config region for the controller */
	addr = controller ? (PCIEB_CLIENT_BASE + CONFIG_WINDOW_OFFSET) :
					    (PCIEA_CLIENT_BASE + CONFIG_WINDOW_OFFSET);

	/*
	 * We'll assume for now that the offset, function, slot, bus encoding
	 * should map onto linear, contiguous addresses in PCIe config space, albeit
	 * that the majority will be unused as only slot 0 is valid for any PCIe
	 * bus and most devices have only function 0
	 *
	 * Could be that PCIe in fact works by not encoding the slot number into the
	 * config space address as it's known that only slot 0 is valid. We'll have
	 * to experiment if/when we get a PCIe switch connected to the PCIe host
	 */
	addr += ((modified_bus_number << 20) | (slot << 15) | (function << 12) | (where & ~3));

	return addr;
}

static int ox820_read_config(
	struct pci_bus *bus,
	unsigned int    devfn,
	int             where,
	int             size,
	u32            *val)
{
	struct pci_sys_data *sys = bus->sysdata;
	unsigned int         slot = PCI_SLOT(devfn);
	u32                  v;

//printk(KERN_INFO "ox820_read_config() sys->busnr %d, bus->number %d, devfn %p, "
//	"where %p, size %d\n", sys->busnr, bus->number, (void*)devfn, (void*)where, size);

//printk(KERN_INFO "ox820_read_config() bus->self %p, bus->parent %p, number %d, "
//	"primary %d,  secondary %d, subordinate %d\n", bus->self, bus->parent,
//	bus->number, bus->primary, bus->secondary, bus->subordinate);

	/* Only a single device per bus for PCIe point-to-point links */
	if (!link_up[controller_index_from_bus(sys->busnr)] || slot > 0) {
//printk(KERN_INFO "ox820_read_config() Unsupported slot number %d or controller link down\n", slot);
		/* Fake the no-device response for the unsupported slot numbers */
		*val = ~0UL;
	} else {
		/* Calculate the config space address for the given bus/device/slot */
		unsigned long addr = pci_addr(sys->busnr, bus->number, devfn, where);

//printk(KERN_INFO "ox820_read_config() sys->busnr %d, bus->number %d, devfn %p, "
//	"where %p, size %d -> slot %d, func %d -> addr %p\n", sys->busnr, bus->number,
//	(void*)devfn, (void*)where, size, slot, PCI_FUNC(devfn), (void*)addr);

		v = __raw_readl(addr);
//printk(KERN_INFO "ox820_read_config() v %p\n", (void*)v);
		switch (size) {
			case 1:
				if (where & 2) v >>= 16;
				if (where & 1) v >>= 8;
				v &= 0xff;
				break;
			case 2:
				if (where & 2) v >>= 16;
				v &= 0xffff;
				break;
			case 4:
				break;
			default:
				BUG();
		}

		*val = v;
//printk(KERN_INFO "ox820_read_config() Slot number %d read value %p\n", slot, (void*)*val);
	}

	return PCIBIOS_SUCCESSFUL;
}

static int ox820_write_config(
	struct pci_bus *bus,
	unsigned int    devfn,
	int             where,
	int             size,
	u32             val)
{
	struct pci_sys_data *sys = bus->sysdata;
	unsigned int         slot = PCI_SLOT(devfn);

//printk(KERN_INFO "ox820_write_config() sys->busnr %d, bus->number %d, devfn %p, "
//	"where %p, size %d\n", sys->busnr, bus->number, (void*)devfn, (void*)where, size);

//printk(KERN_INFO "ox820_write_config() bus->self %p, bus->parent %p, number %d, "
//	"primary %d,  secondary %d, subordinate %d\n", bus->self, bus->parent,
//	bus->number, bus->primary, bus->secondary, bus->subordinate);

	/* Only a single device per bus for PCIe point-to-point links */
	if (!link_up[controller_index_from_bus(sys->busnr)] || slot > 0) {
//printk(KERN_INFO "ox820_write_config() Unsupported slot number %d\n", slot);
	} else {
		/* Calculate the config space address for the given bus/device/slot */
		u32 virt = pci_addr(sys->busnr, bus->number, devfn, where);

//printk(KERN_INFO "ox820_write_config() sys->busnr %d, bus->number %d, devfn %p, "
//	"where %p, size %d -> slot %d, func %d -> virt %p write %p\n", sys->busnr, bus->number,
//	(void*)devfn, (void*)where, size, slot, PCI_FUNC(devfn), (void*)virt, (void*)val);

		/* Only 32-bit accesses are working, so do RMW for 8/16 bit accesses */
		/* Todo: Will need some locking if this solution has to go in permanently */
		if (size == 4) {
//printk(KERN_INFO "ox820_write_config() Writing %p to %p\n", (void*)val, (void*)virt);
			__raw_writel(val, virt);
		} else {
			/* Use byte lane enables for byte and word writes */
			unsigned long flags;
			u32           quad_val = val << (8 * (virt & 3));

//printk(KERN_INFO "ox820_write_config() Writing %p to %p, lanes %p\n",
//	(void*)quad_val, (void*)(virt & ~3), (void*)((3 >> (2-size)) << (virt & 3)));

			/* Lock use of PCIe out lane control register */
			spin_lock_irqsave(
				controller_index_from_virt(virt) ? &pcieb_lock : &pciea_lock, flags);

			set_out_lanes(virt, (3 >> (2-size)) << (virt & 3));
			__raw_writel(quad_val, virt & ~3);
			set_out_lanes(virt, 0xf);

			/* Unlock use of PCIe out lane control register */
			spin_unlock_irqrestore(
				controller_index_from_virt(virt) ? &pcieb_lock : &pciea_lock, flags);
		}
	}

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops ox820_pci_ops = {
	.read	= ox820_read_config,
	.write	= ox820_write_config,
};

struct pci_bus *ox820_pci_scan_bus(
	int                  nr,
	struct pci_sys_data *sys)
{
//printk(KERN_INFO "ox820_pci_scan_bus() nr %d sys %p, sys->bus %p\n", nr, sys, sys->bus);
	return pci_scan_bus(sys->busnr, &ox820_pci_ops, sys);
//printk(KERN_INFO "ox820_pci_scan_bus() Leaving: sys->bus %p\n", sys->bus);
}
#define CONFIG_OXNAS_PCIE_RESET_GPIO 44 
#if (CONFIG_OXNAS_PCIE_RESET_GPIO < SYS_CTRL_NUM_PINS)
#define	PCIE_OUTPUT_SET	GPIO_A_OUTPUT_SET
#define	PCIE_OUTPUT_CLEAR	GPIO_A_OUTPUT_CLEAR
#define	PCIE_OUTPUT_ENABLE_SET	GPIO_A_OUTPUT_ENABLE_SET
#define	PCIE_OUTPUT_ENABLE_CLEAR	GPIO_A_OUTPUT_ENABLE_CLEAR
#else
#define PCIE_RESET_PIN          (CONFIG_OXNAS_PCIE_RESET_GPIO - SYS_CTRL_NUM_PINS)
#define	PCIE_OUTPUT_SET	GPIO_B_OUTPUT_SET
#define	PCIE_OUTPUT_CLEAR	GPIO_B_OUTPUT_CLEAR
#define	PCIE_OUTPUT_ENABLE_SET	GPIO_B_OUTPUT_ENABLE_SET
#define	PCIE_OUTPUT_ENABLE_CLEAR	GPIO_B_OUTPUT_ENABLE_CLEAR
#endif

void __init ox820_pci_preinit(void)
{
	unsigned long end;
	unsigned long version_id;
	unsigned long pin = ( 1 << PCIE_RESET_PIN);

    writel(1<<SYS_CTRL_RSTEN_PLLB_BIT, SYS_CTRL_RSTEN_CLR_CTRL); // take PLL B out of reset
    // reset PCIe cards
    writel(pin, PCIE_OUTPUT_ENABLE_SET);
    writel(pin, PCIE_OUTPUT_CLEAR);
    wmb();
    mdelay(500);	// wait for PCIe cards to reset and for PLL B to lock
    writel(pin, PCIE_OUTPUT_ENABLE_CLEAR);	// must tri-state the pin to pull it up, otherwise hardware reset may not work
    wmb();

    writel(0x218, SEC_CTRL_PLLB_CTRL0); // set PLL B control information
	
    writel(0x0F, SYS_CTRL_HCSL_CTRL); // generate clocks from HCSL buffers

    /* Ensure PCIe PHY is properly reset */
    writel(1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    writel(1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();

	/* Ensure both PCIe cores are properly reset */
    writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();

	/* Start both PCIe core clocks */
    writel(1UL << SYS_CTRL_CKEN_PCIEA_BIT, SYS_CTRL_CKEN_SET_CTRL);
    writel(1UL << SYS_CTRL_CKEN_PCIEB_BIT, SYS_CTRL_CKEN_SET_CTRL);
    // allow entry to L23 state
    writel(1UL << SYS_CTRL_PCIE_READY_ENTR_L23_BIT, SYS_CTRL_PCIEA_CTRL);
    writel(1UL << SYS_CTRL_PCIE_READY_ENTR_L23_BIT, SYS_CTRL_PCIEB_CTRL);
    wmb();

	/* Use the version register as an early test for core presence */
	link_up[0] = link_up[1] = 1;

	version_id = readl(PCIEA_DBI_BASE + PCI_CONFIG_VERSION_DEVICEID_REG_OFFSET);
	printk(KERN_INFO "PCIeA version/deviceID %p\n", (void*)version_id);
	if (version_id != VERSION_ID_MAGIC) {
		printk(KERN_INFO "PCIeA controller not found (version_id %p vs "
			"expected %p)\n", (void*)version_id, (void*)VERSION_ID_MAGIC);
		link_up[0] = 0;
	}

	version_id = readl(PCIEB_DBI_BASE + PCI_CONFIG_VERSION_DEVICEID_REG_OFFSET);
	printk(KERN_INFO "PCIeB version/deviceID %p\n", (void*)version_id);
	if (version_id != VERSION_ID_MAGIC) {
		printk(KERN_INFO "PCIeB controller not found (version_id %p vs "
			"expected %p)\n", (void*)version_id, (void*)VERSION_ID_MAGIC);
		link_up[1] = 0;
	}

	/*
	 * Initialise the first PCIe controller if present
	 */
	if (link_up[0]) {
		/* Set PCIe core into RootCore mode */
		writel(SYS_CTRL_PCIE_DEVICE_TYPE_ROOT << SYS_CTRL_PCIE_DEVICE_TYPE_BIT, SYS_CTRL_PCIEA_CTRL);
		wmb();

		/* Pulse PCIe core reset, so take new RootCore mode I assume */
		writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
		wmb();
		writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
		wmb();

		/*
		 * We won't have any inbound address translation. This allows PCI devices
		 * to access anywhere in the AHB address map. Might be regarded as a bit
		 * dangerous, but let's get things working before we worry about that
		 */
		writel(0 << ENABLE_IN_ADDR_TRANS_BIT, IB_ADDR_XLATE_ENABLE);
		wmb();

		/*
		 * Program outbound translation windows
		 *
		 * Outbound window is what is referred to as "PCI client" region in HRM
		 *
		 * Could use the larger alternative address space to get >>64M regions for
		 * graphics cards etc., but will not bother at this point.
		 *
		 * IP bug means that AMBA window size must be a power of 2
		 *
		 * Set mem0 window for first 16MB of outbound window non-prefetchable 
		 * Set mem1 window for second 16MB of outbound window prefetchable 
		 * Set io window for next 16MB of outbound window
		 * Set cfg0 for final 1MB of outbound window
		 *
		 * Ignore mem1, cfg1 and msg windows for now as no obvious use cases for
		 * 820 that would need them
		 *
		 * Probably ideally want no offset between mem0 window start as seen by ARM
		 * and as seen on PCI bus and get Linux to assign memory regions to PCI
		 * devices using the same "PCI client" region start address as seen by ARM
		 */

		/* Set PCIeA mem0 region to be 1st 16MB of the 64MB PCIeA window */
		writel(pciea_non_mem.start,	SYS_CTRL_PCIEA_IN0_MEM_ADDR);
		writel(pciea_non_mem.end,	SYS_CTRL_PCIEA_IN0_MEM_LIMIT);
		writel(pciea_non_mem.start,	SYS_CTRL_PCIEA_POM0_MEM_ADDR);

		/* Set PCIeA mem1 region to be 2nd 16MB of the 64MB PCIeA window */
		writel(pciea_pre_mem.start,	SYS_CTRL_PCIEA_IN1_MEM_ADDR);
		writel(pciea_pre_mem.end,	SYS_CTRL_PCIEA_IN1_MEM_LIMIT);
		writel(pciea_pre_mem.start,	SYS_CTRL_PCIEA_POM1_MEM_ADDR);

		/* Set PCIeA io to be third 16M region of the 64MB PCIeA window*/
		writel(pciea_io_mem.start,	SYS_CTRL_PCIEA_IN_IO_ADDR);
		writel(pciea_io_mem.end,	SYS_CTRL_PCIEA_IN_IO_LIMIT);

		/* Set PCIeA cgf0 to be last 16M region of the 64MB PCIeA window*/
		writel(pciea_io_mem.end + 1,   SYS_CTRL_PCIEA_IN_CFG0_ADDR);
		writel(PCIEA_CLIENT_BASE_PA + TOTAL_WINDOW_SIZE - 1, SYS_CTRL_PCIEA_IN_CFG0_LIMIT);
		wmb();

		/* Enable outbound address translation */
		writel(readl(SYS_CTRL_PCIEA_CTRL) | (1UL << SYS_CTRL_PCIE_OBTRANS_BIT), SYS_CTRL_PCIEA_CTRL);
		wmb();

		/*
		 * Program PCIe command register for core to:
		 *  enable memory space
		 *  enable bus master
		 *  enable io
		 */
		writel(7, PCIEA_DBI_BASE + PCI_CONFIG_COMMAND_STATUS_REG_OFFSET);
		wmb();

		/* Program PHY registers - shouldn't be anything to do here as Synopsis
		   should deliver properly configured PHYs */

		/* Bring up the PCI core */
		writel(readl(SYS_CTRL_PCIEA_CTRL) | (1UL << SYS_CTRL_PCIE_LTSSM_BIT), SYS_CTRL_PCIEA_CTRL);
		wmb();

		/* Poll for PCIEA link up */
		end = jiffies + (LINK_UP_TIMEOUT_SECONDS * HZ);
		while (!(readl(SYS_CTRL_PCIEA_CTRL) & (1UL << SYS_CTRL_PCIE_LINK_UP_BIT))) {
			if (time_after(jiffies, end)) {
				link_up[0] = 0;
				printk(KERN_WARNING "ox820_pci_preinit() PCIEA link up timeout (%p)\n",
					(void*)readl(SYS_CTRL_PCIEA_CTRL));
				break;
			}
		}
	}

	/*
	 * Initialise the second PCIe controller if present
	 */
	if (link_up[1]) {
		/* Set PCIe core into RootCore mode */
		writel(SYS_CTRL_PCIE_DEVICE_TYPE_ROOT << SYS_CTRL_PCIE_DEVICE_TYPE_BIT, SYS_CTRL_PCIEB_CTRL);
		wmb();

		/* Pulse PCIe core reset, so take new RootCore mode I assume */
		writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_SET_CTRL);
		wmb();
		writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
		wmb();

		/*
		 * We won't have any inbound address translation. This allows PCI devices
		 * to access anywhere in the AHB address map. Might be regarded as a bit
		 * dangerous, but let's get things working before we worry about that
		 */
		writel(0 << ENABLE_IN_ADDR_TRANS_BIT, IB_ADDR_XLATE_ENABLE);
		wmb();

		/*
		 * Program outbound translation windows
		 *
		 * Outbound window is what is referred to as "PCI client" region in HRM
		 *
		 * Could use the larger alternative address space to get >>64M regions for
		 * graphics cards etc., but will not bother at this point.
		 *
		 * IP bug means that AMBA window size must be a power of 2
		 *
		 * Set mem0 window for first 16MB of outbound window non-prefetchable 
		 * Set mem1 window for second 16MB of outbound window prefetchable 
		 * Set io window for next 16MB of outbound window
		 * Set cfg0 for final 1MB of outbound window
		 *
		 * Ignore mem1, cfg1 and msg windows for now as no obvious use cases for
		 * 820 that would need them
		 *
		 * Probably ideally want no offset between mem0 window start as seen by ARM
		 * and as seen on PCI bus and get Linux to assign memory regions to PCI
		 * devices using the same "PCI client" region start address as seen by ARM
		 */

		/* Set PCIeB mem0 region to be 1st 16MB of the 64MB PCIeB window */
		writel(pcieb_non_mem.start,	SYS_CTRL_PCIEB_IN0_MEM_ADDR);
		writel(pcieb_non_mem.end,	SYS_CTRL_PCIEB_IN0_MEM_LIMIT);
		writel(pcieb_non_mem.start,	SYS_CTRL_PCIEB_POM0_MEM_ADDR);

		/* Set PCIeB mem1 region to be 2nd 16MB of the 64MB PCIeB window */
		writel(pcieb_pre_mem.start,	SYS_CTRL_PCIEB_IN1_MEM_ADDR);
		writel(pcieb_pre_mem.end,	SYS_CTRL_PCIEB_IN1_MEM_LIMIT);
		writel(pcieb_pre_mem.start,	SYS_CTRL_PCIEB_POM1_MEM_ADDR);

		/* Set PCIeB io to be third 16M region of the 64MB PCIeA window*/
		writel(pcieb_io_mem.start,	SYS_CTRL_PCIEB_IN_IO_ADDR);
		writel(pcieb_io_mem.end,	SYS_CTRL_PCIEB_IN_IO_LIMIT);

		/* Set PCIeB cgf0 to be last 16M region of the 64MB PCIeB window*/
		writel(pcieb_io_mem.end + 1,   SYS_CTRL_PCIEB_IN_CFG0_ADDR);
		writel(PCIEB_CLIENT_BASE_PA + TOTAL_WINDOW_SIZE - 1, SYS_CTRL_PCIEB_IN_CFG0_LIMIT);
		wmb();

		/* Enable outbound address translation */
		writel(readl(SYS_CTRL_PCIEB_CTRL) | (1UL << SYS_CTRL_PCIE_OBTRANS_BIT), SYS_CTRL_PCIEB_CTRL);
		wmb();

		/*
		 * Program PCIe command register for core to:
		 *  enable memory space
		 *  enable bus master
		 *  enable io
		 */
		writel(7, PCIEB_DBI_BASE + PCI_CONFIG_COMMAND_STATUS_REG_OFFSET);
		wmb();

		/* Program PHY registers - shouldn't be anything to do here as Synopsis
		   should deliver properly configured PHYs */

		/* Bring up the PCI core */
		writel(readl(SYS_CTRL_PCIEB_CTRL) | (1UL << SYS_CTRL_PCIE_LTSSM_BIT), SYS_CTRL_PCIEB_CTRL);

		/* Poll for PCIEB link up */
		end = jiffies + (LINK_UP_TIMEOUT_SECONDS * HZ);
		while (!(readl(SYS_CTRL_PCIEB_CTRL) & (1UL << SYS_CTRL_PCIE_LINK_UP_BIT))) {
			if (time_after(jiffies, end)) {
				link_up[1] = 0;
				printk(KERN_WARNING "ox820_pci_preinit() PCIEB link up timeout (%p)\n",
					(void*)readl(SYS_CTRL_PCIEA_CTRL));
				break;
			}
		}
	}
}

static struct hw_pci ox820_pci __initdata = {
	.swizzle        = NULL,
	.map_irq        = ox820_map_irq,
	.setup          = ox820_pci_setup,
	.nr_controllers = 2,
	.scan           = ox820_pci_scan_bus,
	.preinit        = ox820_pci_preinit,
};

static int __init ox820_pci_init(void)
{
//printk(KERN_INFO "ox820_pci_init()\n");
    pci_common_init(&ox820_pci);
	return 0;
}

static void __exit ox820_pci_exit(void)
{
//printk(KERN_INFO "ox820_pci_exit()\n");
}

subsys_initcall(ox820_pci_init);
module_exit(ox820_pci_exit);

#endif /* !CONFIG_ARCH_OXNAS_PCIE_DISABLE */
#endif
