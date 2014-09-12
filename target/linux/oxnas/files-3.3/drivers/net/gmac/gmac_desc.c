/*
 * linux/arch/arm/mach-oxnas/gmac_desc.c
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
#include <linux/delay.h>

//#define GMAC_DEBUG
#undef GMAC_DEBUG

#include "gmac.h"
#include "gmac_desc.h"

void init_rx_desc_list(
    gmac_desc_list_info_t    *desc_list,
    volatile gmac_dma_desc_t *base_ptr,
	gmac_dma_desc_t          *shadow_ptr,
    int                       num_descriptors,
	u16                       rx_buffer_length)
{
    int i;

    desc_list->base_ptr = base_ptr;
    desc_list->shadow_ptr = shadow_ptr;
    desc_list->num_descriptors = num_descriptors;
    desc_list->empty_count = num_descriptors;
    desc_list->full_count = 0;
    desc_list->r_index = 0;
    desc_list->w_index = 0;

	for (i=0; i < num_descriptors; ++i) {
		gmac_dma_desc_t          *shadow = shadow_ptr + i;
		volatile gmac_dma_desc_t *desc   = base_ptr + i;

		// Initialise the shadow descriptor
		shadow->length = (rx_buffer_length << RDES1_RBS1_BIT);
		if (i == (num_descriptors - 1)) {
			shadow->length |= (1UL << RDES1_RER_BIT);
		}
		shadow->buffer1 = 0;
		shadow->buffer2 = 0;

		// Copy the shadow descriptor into the real descriptor
		desc->status  = shadow->status = 0;
		desc->length  = shadow->length;
		desc->buffer1 = shadow->buffer1;
		desc->buffer2 = shadow->buffer2;
	}
}

void init_tx_desc_list(
    gmac_desc_list_info_t    *desc_list,
    volatile gmac_dma_desc_t *base_ptr,
	gmac_dma_desc_t          *shadow_ptr,
    int                       num_descriptors)
{
    int i;

    desc_list->base_ptr = base_ptr;
    desc_list->shadow_ptr = shadow_ptr;
    desc_list->num_descriptors = num_descriptors;
    desc_list->empty_count = num_descriptors;
    desc_list->full_count = 0;
    desc_list->r_index = 0;
    desc_list->w_index = 0;

	for (i=0; i < num_descriptors; ++i) {
		gmac_dma_desc_t          *shadow = shadow_ptr + i;
		volatile gmac_dma_desc_t *desc   = base_ptr + i;

		// Initialise the shadow descriptor
#if defined(CONFIG_ARCH_OXNAS)
		shadow->length = (1UL << TDES1_IC_BIT);
		if (i == (num_descriptors - 1)) {
			shadow->length |= (1UL << TDES1_TER_BIT);
		}
		shadow->status = 0;
#else // CONFIG_ARCH_OXNAS
		shadow->status = (1UL << TDES0_IC_BIT);
		if (i == (num_descriptors - 1)) {
			shadow->status |= (1UL << TDES0_TER_BIT);
		}
		shadow->length = 0;
#endif // CONFIG_ARCH_OXNAS
		shadow->buffer1 = 0;
		shadow->buffer2 = 0;

		// Copy the shadow descriptor into the real descriptor
		desc->status  = shadow->status;
		desc->length  = shadow->length;
		desc->buffer1 = shadow->buffer1;
		desc->buffer2 = shadow->buffer2;
	}
}

void rx_take_ownership(gmac_desc_list_info_t* desc_list)
{
    int i;
    for (i=0; i < desc_list->num_descriptors; ++i) {
        (desc_list->base_ptr + i)->status &= ~(1UL << RDES0_OWN_BIT);
    }

    // Ensure all write to the descriptor shared with MAC have completed
    wmb();
}

void tx_take_ownership(gmac_desc_list_info_t* desc_list)
{
    int i;
    for (i=0; i < desc_list->num_descriptors; ++i) {
        (desc_list->base_ptr + i)->status &= ~(1UL << TDES0_OWN_BIT);
    }

    // Ensure all write to the descriptor shared with MAC have completed
    wmb();
}

int set_rx_descriptor(
    gmac_priv_t    *priv,
    rx_frag_info_t *frag_info)
{
    int index = -1;

    // Is there a Rx descriptor available for writing by the CPU?
    if (available_for_write(&priv->rx_gmac_desc_list_info)) {
        // Setup the descriptor required to describe the RX packet
        volatile gmac_dma_desc_t *descriptor;
        gmac_dma_desc_t          *shadow;

        // Get the index of the next RX descriptor available for writing by the CPU
        index = priv->rx_gmac_desc_list_info.w_index;

        // Get a pointer to the next RX descriptor available for writing by the CPU
        descriptor = priv->rx_gmac_desc_list_info.base_ptr   + index;
        shadow     = priv->rx_gmac_desc_list_info.shadow_ptr + index;

        // Set first buffer pointer to buffer from skb
        descriptor->buffer1 = shadow->buffer1 = frag_info->phys_adr;

        // Remember the skb associated with the buffer
        shadow->buffer2 = (u32)frag_info->page;

		if (frag_info->arg) {
			// Remember any custom argument provided in the shadow status field
			// that is never used for any descriptor data
			shadow->status = frag_info->arg;
		}

        // Ensure all prior writes to the descriptor shared with MAC have
        // completed before setting the descriptor ownership flag to transfer
        // ownership to the GMAC
        wmb();

        // Set RX descriptor status to transfer ownership to the GMAC
        descriptor->status = (1UL << RDES0_OWN_BIT);

        // Update the index of the next descriptor available for writing by the CPU
        priv->rx_gmac_desc_list_info.w_index = (shadow->length & (1UL << RDES1_RER_BIT)) ? 0 : index + 1;

        // Account for the descriptor used to hold the new packet
        --priv->rx_gmac_desc_list_info.empty_count;
        ++priv->rx_gmac_desc_list_info.full_count;
    }

    return index;
}

static inline int num_descriptors_needed(u16 length)
{
	int count = length >> MAX_DESCRIPTOR_SHIFT;
	if (length & MAX_DESCRIPTOR_LENGTH) {
		++count;
	}
	return count;
}

int set_tx_descriptor(
    gmac_priv_t    *priv,
    struct sk_buff *skb,
    tx_frag_info_t *frag_info,
    int             frag_count,
    int             use_hw_csum)
{
	int first_descriptor_index = -1;
	int num_descriptors = frag_count;
    int frag_index = 0;
	int check_oversized_frags = priv->netdev->mtu >= (MAX_DESCRIPTOR_LENGTH - ETH_HLEN);

	if (unlikely(check_oversized_frags)) {
		// Calculate the number of extra descriptors required due to fragments
		// being longer than the maximum buffer size that can be described by a
		// single descriptor
		num_descriptors = 0;
		do {
			// How many descriptors are required to describe the fragment?
			num_descriptors += num_descriptors_needed(frag_info[frag_index].length);
		} while (++frag_index < frag_count);
	}

    // Are sufficicent descriptors available for writing by the CPU?
    if (available_for_write(&priv->tx_gmac_desc_list_info) < num_descriptors) {
        return -1;
    }

	{
		volatile gmac_dma_desc_t *previous_descriptor = 0;
		gmac_dma_desc_t          *previous_shadow = 0;
		volatile gmac_dma_desc_t *descriptors[num_descriptors];
		int desc_index = 0;

		frag_index = 0;
		do {
			int        last_frag   = (frag_index == (frag_count - 1));
			u16        part_length = frag_info[frag_index].length;
			dma_addr_t phys_adr    = frag_info[frag_index].phys_adr;
			int        part        = 0;
			int        parts       = 1;

			if (unlikely(check_oversized_frags)) {
				// How many descriptors are required to describe the fragment?
				parts = num_descriptors_needed(part_length);
			}

			// Setup a descriptor for each part of the fragment that can be
			// described by a single descriptor
			do {
				int                       last_part  = (part == (parts - 1));
				int                       index      = priv->tx_gmac_desc_list_info.w_index;
				volatile gmac_dma_desc_t *descriptor = priv->tx_gmac_desc_list_info.base_ptr + index;
				gmac_dma_desc_t          *shadow     = priv->tx_gmac_desc_list_info.shadow_ptr + index;
				u32                       length     = shadow->length;
				u32                       status     = shadow->status;
				u32                       buffer2    = 0;

				// Remember descriptor pointer for final passing of ownership to GMAC
				descriptors[desc_index++] = descriptor;

#if defined(CONFIG_ARCH_OXNAS)
				// May have a second chained descriptor, but never a second buffer,
				// so clear the flag indicating whether there is a chained descriptor
				length &= ~(1UL << TDES1_TCH_BIT);

				// Clear the first/last descriptor flags
				length &= ~((1UL << TDES1_LS_BIT) | (1UL << TDES1_FS_BIT));

				// Set the Tx checksum mode
				length &= ~(((1UL << TDES1_CIC_NUM_BITS) - 1) << TDES1_CIC_BIT);
				if (use_hw_csum) {
					// Don't want full mode as network stack will have already
					// computed the TCP/UCP pseudo header and placed in into the
					// TCP/UCP checksum field
					length |= (TDES1_CIC_PAYLOAD << TDES1_CIC_BIT);
				}
#else // CONFIG_ARCH_OXNAS
				// May have a second chained descriptor, but never a second buffer,
				// so clear the flag indicating whether there is a chained descriptor
				status &= ~(1UL << TDES0_TCH_BIT);

				// Clear the first/last descriptor flags
				status &= ~((1UL << TDES0_LS_BIT) | (1UL << TDES0_FS_BIT));

				// Set the Tx checksum mode
				status &= ~(((1UL << TDES0_CIC_NUM_BITS) - 1) << TDES0_CIC_BIT);
				if (use_hw_csum) {
					// Don't want full mode as network stack will have already
					// computed the TCP/UCP pseudo header and placed in into the
					// TCP/UCP checksum field
					status |= (TDES0_CIC_PAYLOAD << TDES0_CIC_BIT);
				}
#endif // CONFIG_ARCH_OXNAS

				// Set fragment buffer length
				length &= ~(((1UL << TDES1_TBS1_NUM_BITS) - 1) << TDES1_TBS1_BIT);
				length |= ((part_length > MAX_DESCRIPTOR_LENGTH ? MAX_DESCRIPTOR_LENGTH : part_length) << TDES1_TBS1_BIT);

				// Set fragment buffer address
				descriptor->buffer1 = shadow->buffer1 = phys_adr;

				if (previous_shadow) {
					// Make the previous descriptor chain to the current one
#if defined(CONFIG_ARCH_OXNAS)
					previous_shadow->length |= (1UL << TDES1_TCH_BIT);
#else // CONFIG_ARCH_OXNAS
					previous_shadow->status |= (1UL << TDES0_TCH_BIT);
#endif // CONFIG_ARCH_OXNAS
					previous_descriptor->status = previous_shadow->status;
					previous_descriptor->length = previous_shadow->length;

#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
					previous_shadow->buffer2 |= (priv->desc_dma_addr + ((void*)descriptor - (void*)priv->desc_vaddr));
#else // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
					previous_shadow->buffer2 |= descriptors_virt_to_phys((u32)descriptor);
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
					previous_descriptor->buffer2 = previous_shadow->buffer2;
				}

				// Is this the first desciptor for the packet?
				if (!frag_index && !part) {
					// Need to return index of first descriptor used for packet
					first_descriptor_index = index;

					// Set flag indicating is first descriptor for packet
#if defined(CONFIG_ARCH_OXNAS)
					length |= (1UL << TDES1_FS_BIT);
#else // CONFIG_ARCH_OXNAS
					status |= (1UL << TDES0_FS_BIT);
#endif // CONFIG_ARCH_OXNAS
				}

				// Is this the last descriptor for the packet?
				if (last_frag && last_part) {
					// Store the skb pointer with the last descriptor for packet, in
					// which the second buffer address will be unused as we do not use
					// second buffers and only intermedate buffers may use the chained
					// descriptor address
					buffer2 = (u32)skb;

					// Set flag indicating is last descriptor for packet
#if defined(CONFIG_ARCH_OXNAS)
					length |= (1UL << TDES1_LS_BIT);
#else // CONFIG_ARCH_OXNAS
					status |= (1UL << TDES0_LS_BIT);
#endif // CONFIG_ARCH_OXNAS
				} else {
					// For descriptor chaining need to remember previous descriptor
					previous_descriptor = descriptor;
					previous_shadow = shadow;

					// Is this descriptor not the last describing a single fragment
					// buffer?
					if (!last_part) {
						// This descriptor does not own the fragment buffer, so use
						// the (h/w ignored) lsb of buffer2 to encode this info.
						buffer2 = 1;

						// Update the fragment buffer part details
						part_length -= MAX_DESCRIPTOR_LENGTH;
						phys_adr    += MAX_DESCRIPTOR_LENGTH;
					}
				}

				// Write the assembled status descriptor entry to the descriptor
				descriptor->status = shadow->status = status;

				// Write the assembled length descriptor entry to the descriptor
				descriptor->length = shadow->length = length;

				// Write the assembled buffer2 descriptor entry to the descriptor
				shadow->buffer2 = buffer2;

				// Update the index of the next descriptor available for writing by the CPU
#if defined(CONFIG_ARCH_OXNAS)
				priv->tx_gmac_desc_list_info.w_index = (length & (1UL << TDES1_TER_BIT)) ? 0 : index + 1;
#else // CONFIG_ARCH_OXNAS
				priv->tx_gmac_desc_list_info.w_index = (status & (1UL << TDES0_TER_BIT)) ? 0 : index + 1;
#endif // CONFIG_ARCH_OXNAS
			} while (++part < parts);
		} while (++frag_index < frag_count);

		// Ensure all prior writes to the descriptors shared with MAC have
		// completed before setting the descriptor ownership flags to transfer
		// ownership to the GMAC
		wmb();

		// Transfer descriptors to GMAC's ownership in reverse order, so when
		// GMAC begins processing the first descriptor all others are already
		// owned by the GMAC
		for (desc_index = (num_descriptors - 1); desc_index >= 0; --desc_index) {
			descriptors[desc_index]->status |= (1UL << TDES0_OWN_BIT);
			wmb();
		}
	}

    // Account for the number of descriptors used to hold the new packet
    priv->tx_gmac_desc_list_info.empty_count -= (num_descriptors);
    priv->tx_gmac_desc_list_info.full_count  += (num_descriptors);

    return first_descriptor_index;
}

int get_tx_descriptor(
    gmac_priv_t     *priv,
    struct sk_buff **skb,
    u32             *status,
    tx_frag_info_t  *frag_info,
    int             *buffer_owned)
{
    int index = -1;
	u32 local_status;

    // Find the first available Tx descriptor
    if (tx_available_for_read(&priv->tx_gmac_desc_list_info, &local_status)) {
        gmac_dma_desc_t *shadow;
        u32 length;
        u32 buffer2;

        // Get the descriptor released the longest time ago by the GMAC DMA
        index = priv->tx_gmac_desc_list_info.r_index;
        shadow = priv->tx_gmac_desc_list_info.shadow_ptr + index;

        // Get the length of the buffer
        length = shadow->length;
        frag_info->length = ((length >> TDES1_TBS1_BIT) & ((1UL << TDES1_TBS1_NUM_BITS) - 1));

        // Get a pointer to the buffer
        frag_info->phys_adr = shadow->buffer1;

		// Get buffer ownership or skb info
        buffer2 = shadow->buffer2;

        // Check that chained buffer not is use before setting skb from buffer2
#if defined(CONFIG_ARCH_OXNAS)
        if (!(length & (1UL << TDES1_TCH_BIT))) {
#else // CONFIG_ARCH_OXNAS
        if (!(local_status & (1UL << TDES0_TCH_BIT))) {
#endif // CONFIG_ARCH_OXNAS
            *skb = (struct sk_buff*)buffer2;
			*buffer_owned = 1;
        } else {
            // The lsb (h/w ignored) is used to encode buffer ownership
            *buffer_owned = !(buffer2 & 1);
			*skb = 0;
        }

        // Accumulate status
        if (status) {
            *status |= local_status;
        }

        // Update the index of the next descriptor with which the GMAC DMA may have finished
#if defined(CONFIG_ARCH_OXNAS)
        priv->tx_gmac_desc_list_info.r_index = (length & (1UL << TDES1_TER_BIT)) ? 0 : index + 1;
#else // CONFIG_ARCH_OXNAS
        priv->tx_gmac_desc_list_info.r_index = (local_status & (1UL << TDES0_TER_BIT)) ? 0 : index + 1;
#endif // CONFIG_ARCH_OXNAS

        // Account for the descriptor which is now no longer waiting to be processed by the CPU
        ++priv->tx_gmac_desc_list_info.empty_count;
        --priv->tx_gmac_desc_list_info.full_count;
    }

    return index;
}

