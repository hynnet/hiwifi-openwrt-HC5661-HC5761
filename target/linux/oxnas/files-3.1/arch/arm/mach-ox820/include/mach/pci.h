/*
 * arch/arm/mach-0x820/include/mach/pci.h
 *
 * Copyright (C) 2009 Oxford Semiconductor Ltd
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARM_ARCH_PCI_H
#define __ASM_ARM_ARCH_PCI_H

extern unsigned long __ox820_inl(u32 phys);
extern unsigned short __ox820_inw(u32 phys);
extern unsigned char __ox820_inb(u32 phys);
extern void __ox820_outl(unsigned long value, u32 phys);
extern void __ox820_outw(unsigned short value, u32 phys);
extern void __ox820_outb(unsigned char value, u32 phys);
extern void __ox820_outsb(u32 p, unsigned char *from, u32 len);
extern void __ox820_outsw(u32 p, unsigned short *from, u32 len);
extern void __ox820_outsl(u32 p, unsigned long *from, u32 len);
extern void __ox820_insb(u32 p, unsigned char *to, u32 len);
extern void __ox820_insw(u32 p, unsigned short *to, u32 len);
extern void __ox820_insl(u32 p, unsigned long *to, u32 len);

#endif // __ASM_ARM_ARCH_PCI_H
