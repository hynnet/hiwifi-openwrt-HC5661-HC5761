/*
 * arch/arm/mach-0x820/include/mach/io.h
 *
 * Copyright (C) 2009 Oxford Semiconductor Ltd
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include <mach/hardware.h>

#define IO_SPACE_LIMIT 0xffffffff

#define __mem_pci(a)	(a)

#ifndef CONFIG_PCI

#define	__io(v)		__typesafe_io(v)

#else // CONFIG_PCI
#include <mach/pci.h>

#define	outb(p, v)	__ox820_outb(p, v)
#define	outw(p, v)	__ox820_outw(p, v)
#define	outl(p, v)	__ox820_outl(p, v)
	
#define	outsb(p, v, l)	__ox820_outsb(p, v, l)
#define	outsw(p, v, l)	__ox820_outsw(p, v, l)
#define	outsl(p, v, l)	__ox820_outsl(p, v, l)

#define	inb(p)	__ox820_inb(p)
#define	inw(p)	__ox820_inw(p)
#define	inl(p)	__ox820_inl(p)

#define	insb(p, v, l)	__ox820_insb(p, v, l)
#define	insw(p, v, l)	__ox820_insw(p, v, l)
#define	insl(p, v, l)	__ox820_insl(p, v, l)

static inline void __iomem *ioport_map(unsigned long port, unsigned int nr) { return __typesafe_io(port); }
static inline void ioport_unmap(void __iomem *addr) {}

#endif // CONFIG_PCI

#define PCI_CLIENT_REGION_SIZE	(64*1024*1024)

static inline int is_pci_phys_addr(unsigned long addr)
{
	return (addr >= PCIEA_CLIENT_BASE_PA) &&
		   (addr <= (PCIEB_CLIENT_BASE_PA + PCI_CLIENT_REGION_SIZE));
}

static inline int is_pci_virt_addr(unsigned long addr)
{
	return (addr >= PCIEA_CLIENT_BASE) &&
		   (addr <= (PCIEB_CLIENT_BASE + PCI_CLIENT_REGION_SIZE));
}

/*
 * We have direct access to PCI I/O space with the 820 core
 */
static inline u32 pci_phys_to_virt(u32 phys)
{
	return (phys >= PCIEB_CLIENT_BASE_PA) ?
		(PCIEB_CLIENT_BASE + (phys - PCIEB_CLIENT_BASE_PA)) :
		(PCIEA_CLIENT_BASE + (phys - PCIEA_CLIENT_BASE_PA));
}

/*
 * We have direct access to PCI I/O space with the 820 core
 */
static inline u32 pci_virt_to_phys(u32 virt)
{
	return (virt >= PCIEB_CLIENT_BASE) ?
		(PCIEB_CLIENT_BASE_PA + (virt - PCIEB_CLIENT_BASE)) :
		(PCIEA_CLIENT_BASE_PA + (virt - PCIEA_CLIENT_BASE));
}

extern void __iomem * ox820_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype);
extern void ox820_iounmap(void __iomem *virt_addr);

#define __arch_ioremap(a, s, f)	ox820_ioremap(a, s, f)
#define	__arch_iounmap(a)		ox820_iounmap(a)

#endif //__ASM_ARM_ARCH_IO_H
