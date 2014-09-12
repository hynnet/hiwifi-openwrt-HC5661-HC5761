/* linux/include/asm-arm/arch-oxnas/vmalloc.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_VMALLOC_H
#define __ASM_ARCH_VMALLOC_H

// With 512MB of DDR should allow 288MB for hardware static mappings before
// colliding with the DMA mapping regions and give 184MB of vmalloc space
#define VMALLOC_END	0xED000000

#endif // __ASM_ARCH_VMALLOC_H
