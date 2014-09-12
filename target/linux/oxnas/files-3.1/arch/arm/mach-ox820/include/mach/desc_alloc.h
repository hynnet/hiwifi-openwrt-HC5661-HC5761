/*
 * linux/include/asm-arm/arch-oxnas/dma.h
 *
 * Copyright (C) 2005 Oxford Semiconductor Ltd
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
 *
 *
 * Here we partition the available internal SRAM between the GMAC and generic
 * DMA descriptors. This requires defining the gmac_dma_desc_t structure here,
 * rather than in its more natural position within gmac.h
 */
#if !defined(__DESC_ALLOC_H__)
#define __DESC_ALLOC_H__

#include <mach/hardware.h>
#define CONFIG_ARCH_OXNAS_GMAC1_NUM_RX_DESCRIPTORS 512
#define CONFIG_ARCH_OXNAS_GMAC2_NUM_RX_DESCRIPTORS 0
#define CONFIG_ARCH_OXNAS_GMAC1_NUM_TX_DESCRIPTORS 128
#define CONFIG_ARCH_OXNAS_GMAC2_NUM_TX_DESCRIPTORS 0
/*
 * GMAC DMA in-memory descriptor structures
 */
typedef struct gmac_dma_desc {
    /** The encoded status field of the GMAC descriptor */
    u32 status;
    /** The encoded length field of GMAC descriptor */
    u32 length;
    /** Buffer 1 pointer field of GMAC descriptor */
    u32 buffer1;
    /** Buffer 2 pointer or next descriptor pointer field of GMAC descriptor */
    u32 buffer2;
} __attribute ((aligned(4),packed)) gmac_dma_desc_t;
/** Must be modified to track gmac_dma_desc_t definition above */
#define SIZE_OF_GMAC_DESC (4 * 4)

/*
 * Common SG control structure for generic and SATA/PRD SGDMA controllers
 */
typedef struct oxnas_dma_simple_sg_info {
    unsigned long qualifier;
    unsigned long control;
    dma_addr_t    src_entries;
    dma_addr_t    dst_entries;
} __attribute ((aligned(4),packed)) oxnas_dma_simple_sg_info_t;
/** Must be modified to track oxnas_dma_simple_sg_info_t definition above */
#define SIZE_OF_SIMPLE_SG_INFO (4 * 4)

#ifndef CONFIG_ARCH_OXNAS
/*
 * SATA/PRD SGDMA controller PRD table entry
 */
#define PRD_MAX_LEN (64*1024)
#define PRD_EOF_MASK (1UL << 31)
typedef struct prd_table_entry {
	dma_addr_t adr;
	u32        flags_len;
}__attribute ((aligned(4),packed)) prd_table_entry_t;
/** Must be modified to track prd_table_entry_t definition above */
#define SIZE_OF_PRD_ENTRY (2 * 4)
#endif // !CONFIG_ARCH_OXNAS

/*
 * GMAC descriptors are stored first in SRAM
 */
#define GMAC1_DESC_ALLOC_START       DESCRIPTORS_BASE
#define GMAC1_DESC_ALLOC_START_PA    DESCRIPTORS_BASE_PA
#define GMAC1_DESC_ALLOC_SIZE	((CONFIG_ARCH_OXNAS_GMAC1_NUM_TX_DESCRIPTORS + \
	CONFIG_ARCH_OXNAS_GMAC1_NUM_RX_DESCRIPTORS) * SIZE_OF_GMAC_DESC)

#define GMAC2_DESC_ALLOC_START       (GMAC1_DESC_ALLOC_START + GMAC1_DESC_ALLOC_SIZE)
#define GMAC2_DESC_ALLOC_START_PA    (GMAC1_DESC_ALLOC_START_PA + GMAC1_DESC_ALLOC_SIZE)
#define GMAC2_DESC_ALLOC_SIZE	((CONFIG_ARCH_OXNAS_GMAC2_NUM_TX_DESCRIPTORS + \
	CONFIG_ARCH_OXNAS_GMAC2_NUM_RX_DESCRIPTORS) * SIZE_OF_GMAC_DESC)

#define GMAC_DESC_ALLOC_SIZE	(GMAC1_DESC_ALLOC_SIZE + GMAC2_DESC_ALLOC_SIZE)

#if (GMAC_DESC_ALLOC_SIZE > DESCRIPTORS_SIZE)
#error "Too many GMAC descriptors - descriptor SRAM allocation exceeded"
#endif

#ifdef CONFIG_SATA_OX820
/*
 * SATA PRD SGDMA descriptors are stored after the GMAC descriptors
 */
#define OX820SATA_PRD        	(GMAC1_DESC_ALLOC_START + GMAC_DESC_ALLOC_SIZE)
#define OX820SATA_PRD_PA        (GMAC1_DESC_ALLOC_START_PA + GMAC_DESC_ALLOC_SIZE)
#define OX820SATA_PRD_SIZE      (CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * CONFIG_ODRB_SATA_PRD_ARRAY_SIZE * SIZE_OF_PRD_ENTRY)
#define OX820SATA_SGDMA_REQ		(OX820SATA_PRD    + OX820SATA_PRD_SIZE)
#define OX820SATA_SGDMA_REQ_PA  (OX820SATA_PRD_PA + OX820SATA_PRD_SIZE)
#define OX820SATA_SGDMA_SIZE    (CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * SIZE_OF_SIMPLE_SG_INFO)
#endif // !CONFIG_SATA_OX820

/* Size of all fixed-size descriptor allocations */
#ifdef CONFIG_SATA_OX820
#define TOTAL_FIXED_SIZE_DESC_ALLOC (GMAC_DESC_ALLOC_SIZE + \
									 OX820SATA_PRD_SIZE + \
									 OX820SATA_SGDMA_SIZE)
#else // CONFIG_ARCH_OXNAS
#define TOTAL_FIXED_SIZE_DESC_ALLOC	GMAC_DESC_ALLOC_SIZE
#endif // CONFIG_ARCH_OXNAS

#if (TOTAL_FIXED_SIZE_DESC_ALLOC > DESCRIPTORS_SIZE)
#error "GMAC + SATA PRD descriptors exceed SRAM space allocated to descriptors"
#endif

/*
 * Generic SGDMA descriptors use up whatever space is left of the portion of
 * SRAM allocated to descriptors
 */
#define DMA_DESC_ALLOC_START	(DESCRIPTORS_BASE    + TOTAL_FIXED_SIZE_DESC_ALLOC)
#define DMA_DESC_ALLOC_START_PA	(DESCRIPTORS_BASE_PA + TOTAL_FIXED_SIZE_DESC_ALLOC)
#define DMA_DESC_ALLOC_SIZE		(DESCRIPTORS_SIZE    - TOTAL_FIXED_SIZE_DESC_ALLOC)

#define descriptors_virt_to_phys(x) ((x) - DESCRIPTORS_BASE + DESCRIPTORS_BASE_PA)
#define descriptors_phys_to_virt(x) ((x) - DESCRIPTORS_BASE_PA + DESCRIPTORS_BASE)

#endif        //  #if !defined(__DESC_ALLOC_H__)

