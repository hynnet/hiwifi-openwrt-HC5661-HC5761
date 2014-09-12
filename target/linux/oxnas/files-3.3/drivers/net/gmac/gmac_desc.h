/*
 * linux/arch/arm/mach-oxnas/gmac_desc.h
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
 */
#if !defined(__GMAC_DESC_H__)
#define __GMAC_DESC_H__

#include <asm/types.h>
#include "gmac.h"

typedef enum rdes0 {
    RDES0_OWN_BIT  = 31,
    RDES0_AFM_BIT  = 30,
    RDES0_FL_BIT   = 16,
    RDES0_ES_BIT   = 15,
    RDES0_DE_BIT   = 14,
    RDES0_SAF_BIT  = 13,
    RDES0_LE_BIT   = 12,
    RDES0_OE_BIT   = 11,
    RDES0_VLAN_BIT = 10,
    RDES0_FS_BIT   = 9,
    RDES0_LS_BIT   = 8,
    RDES0_IPC_BIT  = 7,
    RDES0_LC_BIT   = 6,
    RDES0_FT_BIT   = 5,
    RDES0_RWT_BIT  = 4,
    RDES0_RE_BIT   = 3,
    RDES0_DRE_BIT  = 2,
    RDES0_CE_BIT   = 1,
    RDES0_PCE_BIT  = 0
} rdes0_t;

#define RX_DESC_STATUS_FL_NUM_BITS 14

#if defined(CONFIG_ARCH_OXNAS)
typedef enum rdes1 {
    RDES1_DIC_BIT  = 31,
    RDES1_RER_BIT  = 25,
    RDES1_RCH_BIT  = 24,
    RDES1_RBS2_BIT = 11,
    RDES1_RBS1_BIT = 0,
} rdes1_t;

#define RX_DESC_LENGTH_RBS2_NUM_BITS 11
#define RX_DESC_LENGTH_RBS1_NUM_BITS 11

typedef enum tdes0 {
    TDES0_OWN_BIT = 31,
    TDES0_IHE_BIT = 16,
    TDES0_ES_BIT  = 15,
    TDES0_JT_BIT  = 14,
    TDES0_FF_BIT  = 13,
    TDES0_IPE_BIT = 12,
    TDES0_LOC_BIT = 11,
    TDES0_NC_BIT  = 10,
    TDES0_LC_BIT  = 9,
    TDES0_EC_BIT  = 8,
    TDES0_VF_BIT  = 7,
    TDES0_CC_BIT  = 3,
    TDES0_ED_BIT  = 2,
    TDES0_UF_BIT  = 1,
    TDES0_DB_BIT  = 0
} tdes0_t;

#define TDES0_CC_NUM_BITS 4

typedef enum tdes1 {
    TDES1_IC_BIT   = 31,
    TDES1_LS_BIT   = 30,
    TDES1_FS_BIT   = 29,
    TDES1_CIC_BIT  = 27,
    TDES1_DC_BIT   = 26,
    TDES1_TER_BIT  = 25,
    TDES1_TCH_BIT  = 24,
    TDES1_DP_BIT   = 23,
    TDES1_TBS2_BIT = 11,
    TDES1_TBS1_BIT = 0
} tdes1_t;

#define TDES1_CIC_NUM_BITS  2
#define TDES1_TBS2_NUM_BITS 11
#define TDES1_TBS1_NUM_BITS 11

#define TDES1_CIC_NONE    0
#define TDES1_CIC_HDR     1
#define TDES1_CIC_PAYLOAD 2
#define TDES1_CIC_FULL    3

#else // CONFIG_ARCH_OXNAS

typedef enum rdes1 {
    RDES1_DIC_BIT  = 31,
    RDES1_RBS2_BIT = 16,
    RDES1_RER_BIT  = 15,
    RDES1_RCH_BIT  = 14,
    RDES1_RBS1_BIT = 0,
} rdes1_t;

#define RX_DESC_LENGTH_RBS2_NUM_BITS 13
#define RX_DESC_LENGTH_RBS1_NUM_BITS 13

typedef enum tdes0 {
    TDES0_OWN_BIT = 31,
	TDES0_IC_BIT  = 30,
	TDES0_LS_BIT  = 29,
	TDES0_FS_BIT  = 28,
    TDES0_DC_BIT  = 27,
    TDES0_DP_BIT  = 27,
    TDES0_CIC_BIT = 22,
    TDES0_TER_BIT = 21,
    TDES0_TCH_BIT = 20,
    TDES0_IHE_BIT = 16,
    TDES0_ES_BIT  = 15,
    TDES0_JT_BIT  = 14,
    TDES0_FF_BIT  = 13,
    TDES0_IPE_BIT = 12,
    TDES0_LOC_BIT = 11,
    TDES0_NC_BIT  = 10,
    TDES0_LC_BIT  = 9,
    TDES0_EC_BIT  = 8,
    TDES0_VF_BIT  = 7,
    TDES0_CC_BIT  = 3,
    TDES0_ED_BIT  = 2,
    TDES0_UF_BIT  = 1,
    TDES0_DB_BIT  = 0
} tdes0_t;

#define TDES0_CIC_NUM_BITS	2
#define TDES0_CC_NUM_BITS	4

#define TDES0_CIC_NONE    0
#define TDES0_CIC_HDR     1
#define TDES0_CIC_PAYLOAD 2
#define TDES0_CIC_FULL    3

typedef enum tdes1 {
    TDES1_TBS2_BIT = 16,
    TDES1_TBS1_BIT = 0
} tdes1_t;

#define TDES1_TBS2_NUM_BITS 13
#define TDES1_TBS1_NUM_BITS 13
#endif // CONFIG_ARCH_OXNAS

// Rx and Tx descriptors have the same max size for their associated buffers
#define MAX_DESCRIPTOR_SHIFT	(RX_DESC_LENGTH_RBS1_NUM_BITS)
#define MAX_DESCRIPTOR_LENGTH	((1 << MAX_DESCRIPTOR_SHIFT) - 1)

// Round down to nearest quad length
#define MAX_PACKET_FRAGMENT_LENGTH ((MAX_DESCRIPTOR_LENGTH >> 2) << 2)

extern void init_rx_desc_list(
    gmac_desc_list_info_t    *desc_list,
    volatile gmac_dma_desc_t *base_ptr,
	gmac_dma_desc_t          *shadow_ptr,
    int                       num_descriptors,
	u16                       rx_buffer_length);

extern void init_tx_desc_list(
    gmac_desc_list_info_t    *desc_list,
    volatile gmac_dma_desc_t *base_ptr,
	gmac_dma_desc_t          *shadow_ptr,
    int                       num_descriptors);

/** Force ownership of all descriptors in the specified list to being owned by
 *  the CPU
 */
extern void rx_take_ownership(gmac_desc_list_info_t* desc_list);

/** Force ownership of all descriptors in the specified list to being owned by
 *  the CPU
 */
extern void tx_take_ownership(gmac_desc_list_info_t* desc_list);

/** Return the number of descriptors available for the CPU to fill with new
 *  packet info */
static inline int available_for_write(gmac_desc_list_info_t* desc_list)
{
    return desc_list->empty_count;
}

/** Return non-zero if there is a descriptor available with a packet with which
 *  the GMAC DMA has finished */
static inline int tx_available_for_read(
	volatile gmac_desc_list_info_t *desc_list,
	u32                            *status)
{
	if (!desc_list->full_count) {
		return 0;
	}

	*status = (desc_list->base_ptr + desc_list->r_index)->status;

	if (*status & (1UL << TDES0_OWN_BIT)) {
		return 0;
	}

	return 1;
}

/**
 * Return non-zero if there is a descriptor available with a packet with which
 * the GMAC DMA has finished.
 */
static inline int rx_available_for_read(
	volatile gmac_desc_list_info_t *desc_list,
	u32                            *status)
{
	u32 local_status;

	if (!desc_list->full_count) {
		return 0;
	}

	local_status = (desc_list->base_ptr + desc_list->r_index)->status;

    if (local_status & (1UL << RDES0_OWN_BIT)) {
		return 0;
	}

	if (status) {
		*status = local_status;
	}

    return 1;
}

typedef struct rx_frag_info {
    struct page	*page;
    dma_addr_t	 phys_adr;
    u32		 arg;
    u16		 length;
} rx_frag_info_t;

/**
 * Fill a RX descriptor and pass ownership to DMA engine
 */
extern int set_rx_descriptor(
    gmac_priv_t    *priv,
    rx_frag_info_t *frag_info);

/**
 * Fill in descriptors describing all fragments in a single Tx packet and pass
 * ownership to the GMAC. The 'frag_info' argument points to an array describing
 * each buffer that is to contribute to the transmitted packet. The 'frag_count'
 * argument gives the number of elements in that array
 */
extern int set_tx_descriptor(
    gmac_priv_t    *priv,
    struct sk_buff *skb,
    tx_frag_info_t *frag_info,
    int             frag_count,
    int             use_hw_csum);

/**
 * Extract information about the TX packet transmitted the longest time ago.
 * If the 'status' argument is non-null it will have the status from the
 * descriptor or'ed into it.
 */
extern int get_tx_descriptor(
    gmac_priv_t     *priv,
    struct sk_buff **skb,
    u32             *status,
    tx_frag_info_t  *frag_info,
    int             *buffer_owned); 

/**
 * @param A u32 containing the status from a received frame's DMA descriptor
 * @return An int which is non-zero if a valid received frame has no error
 *         condititions flagged
 */
static inline int is_rx_valid(u32 status)
{
    return !(status & (1UL << RDES0_ES_BIT)) &&
           !(status & (1UL << RDES0_IPC_BIT));
}

static inline int is_rx_dribbling(u32 status)
{
    return status & (1UL << RDES0_DRE_BIT);
}

static inline u32 get_rx_length(u32 status)
{
    return (status >> RDES0_FL_BIT) & ((1UL << RX_DESC_STATUS_FL_NUM_BITS) - 1);
}

static inline int is_rx_collision_error(u32 status)
{
    return status & ((1UL << RDES0_OE_BIT) | (1UL << RDES0_LC_BIT));
}

static inline int is_rx_crc_error(u32 status)
{
    return status & (1UL << RDES0_CE_BIT);
}

static inline int is_rx_frame_error(u32 status)
{
    return status & (1UL << RDES0_DE_BIT);
}

static inline int is_rx_length_error(u32 status)
{
    return status & (1UL << RDES0_LE_BIT);
}

static inline int force_rx_length_error(u32 status)
{
    return status |= (1UL << RDES0_LE_BIT);
}

static inline int is_rx_csum_error(u32 status)
{
    return (status & (1UL << RDES0_IPC_BIT)) ||
		   (status & (1UL << RDES0_PCE_BIT));
}

static inline int is_rx_long_frame(u32 status)
{
    return status & (1UL << RDES0_VLAN_BIT);
}

static inline int is_tx_valid(u32 status)
{
    return !(status & (1UL << TDES0_ES_BIT));
}

static inline int is_tx_collision_error(u32 status)
{
    return (status & (((1UL << TDES0_CC_NUM_BITS) - 1) << TDES0_CC_BIT)) >> TDES0_CC_BIT;
}

static inline int is_tx_aborted(u32 status)
{
    return status & ((1UL << TDES0_LC_BIT) | (1UL << TDES0_EC_BIT));
}

static inline int is_tx_carrier_error(u32 status)
{
    return status & ((1UL << TDES0_LOC_BIT) | (1UL << TDES0_NC_BIT));
}

static inline u32 peek_rx_descriptor_arg(gmac_priv_t *priv)
{
	gmac_desc_list_info_t *list_info = &priv->rx_gmac_desc_list_info;
	int                    index = list_info->w_index;

	gmac_dma_desc_t *shadow = list_info->shadow_ptr + index;
	return shadow->status;
}

static inline u32 getandclear_rx_descriptor_arg(gmac_priv_t *priv)
{
	gmac_desc_list_info_t *list_info = &priv->rx_gmac_desc_list_info;
    int                    index = list_info->r_index;
	u32                    arg;

	gmac_dma_desc_t *shadow = list_info->shadow_ptr + index;
	arg = shadow->status;
	shadow->status = 0;
	return arg;
}

/**
 * Extract data from the next available descriptor with which the GMAC DMA
 * controller has finished.
 * If the 'status' argument is non-null it will have the status from the
 * descriptor or'ed into it, thus enabling the compound status for all
 * descriptors contributing to a packet to be built up
 */
static inline int get_rx_descriptor(
    gmac_priv_t    *priv,
    int            *last,
    u32            *status,
    rx_frag_info_t *frag_info)
{
	int                       index;
	volatile gmac_dma_desc_t *descriptor;
	gmac_dma_desc_t          *shadow;
	u32                       desc_status;
	gmac_desc_list_info_t    *list_info = &priv->rx_gmac_desc_list_info;

	if (!list_info->full_count) {
		return -2;
	}

	// Get the index of the descriptor released the longest time ago by the GMAC DMA 
    index      = list_info->r_index;
	descriptor = list_info->base_ptr   + index;
	shadow     = list_info->shadow_ptr + index;

	if (status && *status) {
		desc_status = *status;
	} else {
		desc_status = descriptor->status;
	}

    if (desc_status & (1UL << RDES0_OWN_BIT)) {
		return -1;
	}

	// Update the index of the next descriptor with which the GMAC DMA may have finished
	list_info->r_index = (shadow->length & (1UL << RDES1_RER_BIT)) ? 0 : index + 1;

	// Account for the descriptor which is now no longer waiting to be processed by the CPU
	++list_info->empty_count;
	--list_info->full_count;

	// Get packet details from the descriptor
	frag_info->page = (struct page*)(shadow->buffer2);
	frag_info->phys_adr = shadow->buffer1;
	frag_info->length = get_rx_length(desc_status);

	// Recover any custom argument stored in the shadow status field that is
	// never used for any descriptor data
	frag_info->arg = shadow->status;

	// Is this descriptor the last contributing to a packet
	*last = desc_status & (1UL << RDES0_LS_BIT);

	// Accumulate the status
	if (status && !*status) {
		*status = desc_status;
	}

    return index;
}
#endif  //  #if !defined(__GMAC_DESC_H__)
