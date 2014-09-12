/* linux/arch/arm/mach-oxnas/gmac.c
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

#include <linux/crc32.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/in.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/udp.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <net/ip.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <mach/irqs.h>

#ifdef CONFIG_SUPPORT_LEON
#include <mach/leon.h>
#endif // CONFIG_SUPPORT_LEON

#if defined(CONFIG_ARCH_OX820)
#include "gmac_netoe.h"
#endif
//#define GMAC_DEBUG
#undef GMAC_DEBUG

#include "gmac.h"
#include "gmac_ethtool.h"
#include "gmac_phy.h"
#include "gmac_desc.h"
#include "gmac_reg.h"

#define CONFIG_OXNAS_GMAC_HLEN 54
#define DUMP_REGS_SUPPORT
#define DUMP_REGS_ON_RESET
//#define DUMP_REGS_ON_GMAC_UP

#define ALLOW_AUTONEG

#define COPRO_CMD_QUEUE_NUM_ENTRIES 6
#define CONFIG_ARCH_OXNAS_GMAC1_COPRO_NUM_TX_DESCRIPTORS 120
#define CONFIG_ARCH_OXNAS_GMAC2_COPRO_NUM_TX_DESCRIPTORS 0
#define CONFIG_OXNAS_RX_BUFFER_SIZE 2044
static const u32 MAC_BASE_OFFSET = 0x0000;
static const u32 DMA_BASE_OFFSET = 0x1000;
static const u32 NETOE_BASE_OFFSET = 0x10000;

// We set the GMAC's initial descriptor to point to a munged version
// of the network offload's descriptor base. It has bit 31 set.
static const u32 NETOE_DESC_OFFSET = 0x80011000;

static const int MIN_PACKET_SIZE = 68;
static const int NORMAL_PACKET_SIZE = 1500;

#if defined(CONFIG_ARCH_OXNAS)
static const int MAX_JUMBO = 9000;
#elif defined(CONFIG_ARCH_OX820)
static const int MAX_JUMBO = (8*1024);
#else
#error "Unsupported SoC architecture"
#endif

static const int EXTRA_RX_SKB_SPACE = 22;	// Ethernet header 14, VLAN 4, CRC 4

static const u32 AUTO_NEGOTIATION_WAIT_TIMEOUT_MS = 5000;

static const u32 NAPI_OOM_POLL_INTERVAL_MS = 50;

static const int WATCHDOG_TIMER_INTERVAL = HZ/2;

#define AUTO_NEG_MS_WAIT 500
static const int AUTO_NEG_INTERVAL = (AUTO_NEG_MS_WAIT)*HZ/1000;
static const int START_RESET_INTERVAL = 50*HZ/1000;
static const int RESET_INTERVAL = 10*HZ/1000;

static const int GMAC_TX_TIMEOUT = 5*HZ;

static const int GMAC_RESET_TIMEOUT_MS = 10000;

static int debug = 0;

#if (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
static const int mac_interrupt[] = { MAC_INTERRUPT };
static const int mac_reset_bit[] = { SYS_CTRL_RSTEN_MAC_BIT };
static const int mac_clken_bit[] = { SYS_CTRL_CKEN_MAC_BIT };
#elif (CONFIG_OXNAS_MAX_GMAC_UNITS == 2)
static const int mac_interrupt[] = { MAC_INTERRUPT, MAC_2_INTERRUPT };
static const int mac_reset_bit[] = { SYS_CTRL_RSTEN_MAC_BIT, SYS_CTRL_RSTEN_MAC_2_BIT };
static const int mac_clken_bit[] = { SYS_CTRL_CKEN_MAC_BIT, SYS_CTRL_CKEN_MAC_2_BIT };
#else
#error "Invalid number of Ethernet ports"
#endif

static int gmac_found_count = 0;
static struct net_device* gmac_netdev[CONFIG_OXNAS_MAX_GMAC_UNITS];

MODULE_AUTHOR("Brian Clarke (Oxford Semiconductor Ltd)");
MODULE_DESCRIPTION("GMAC Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v2.0");


// Make a module parameter to set the Test mode of RTL8211D phy for Uhihan
static int test_mode = 0;
module_param(test_mode, int, S_IRUGO|S_IWUSR);

/* Ethernet MAC adr to assign to interface */
static int mac_adrs[2][6] = { { 0x00, 0x30, 0xe0, 0x00, 0x00, 0x00 },
							  { 0x00, 0x30, 0xe0, 0x00, 0x00, 0x01 } };

module_param_array_named(mac_adr,   mac_adrs[0], int, NULL, S_IRUGO);
module_param_array_named(mac_2_adr, mac_adrs[1], int, NULL, S_IRUGO);

/* PHY id kernel cmdline options */                                  
static int phy_ids[2] = { -1, -1 };                                  
                                                                     
module_param_named(phy_id,   phy_ids[0], int, S_IRUGO);              
module_param_named(phy_2_id, phy_ids[1], int, S_IRUGO);              

/* Whether to use Leon for network offload; default to 'on' for first GMAC */
#ifdef CONFIG_SUPPORT_LEON
static int offload_tx[2] = { 1, 0 };
#else
static int offload_tx[2] = { 0, 0 };
#endif

/* Only supporting Leon offload on first GMAC, so don't allow user to select
 * offload for second GMAC */
module_param_named(gmac_offload_tx,   offload_tx[0], int, S_IRUGO);
module_param_named(gmac_2_offload_tx, offload_tx[1], int, S_IRUGO);

#if defined(CONFIG_ARCH_OX820)
/* Whether to use offload engine for network offload; default to 'on' for first GMAC */
static int offload_netoe[2] = { 0, 0 };

module_param_named(gmac_offload_netoe,   offload_netoe[0], int, S_IRUGO);
module_param_named(gmac_2_offload_netoe, offload_netoe[1], int, S_IRUGO);
#endif

static struct platform_driver plat_driver = {
	.driver	= {
		.name	= "gmac",
	},
};

#ifdef DUMP_REGS_SUPPORT
static void dump_mac_regs(int unit, u32 macBase, u32 dmaBase)
{
    int n = 0;

	printk(KERN_INFO "GMAC unit %d registers:\n", unit);

    for (n=0; n<0xdc; n+=4) {
        printk(KERN_INFO "  MAC Register %08x (%08x) = %08x\n", n, macBase+n, readl(macBase+n));
    }

    for (n=0; n<0x60; n+=4) {
        printk(KERN_INFO "  DMA Register %08x (%08x) = %08x\n", n, dmaBase+n, readl(dmaBase+n));
    }
}

#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
#ifndef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
static void dump_leon_tx_descs(gmac_priv_t *priv)
{
	int i;
	gmac_dma_desc_t *desc = (gmac_dma_desc_t*)(priv->copro_tx_desc_vaddr);

	printk(KERN_INFO "Leon Tx descriptors for unit %d:\n", priv->unit);
	for (i=0; i < priv->copro_num_tx_descs; ++i, ++desc) {
		printk(KERN_INFO " [%d] stat=0x%p, len=0x%p, buf1=0x%p, buf2=0x%p\n", i,
			(void*)desc->status, (void*)desc->length, (void*)desc->buffer1,
			(void*)desc->buffer2);
	}
}
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS

static void dump_leon_queues(gmac_priv_t *priv)
{
	int i;
	cmd_que_t          *cmd_queue = &priv->cmd_queue_;
	gmac_cmd_que_ent_t *cmd;
	tx_que_t           *job_queue = &priv->tx_queue_;
	gmac_tx_que_ent_t  *job;

	// Dump command queue contents
	printk(KERN_INFO "%d command queue entries for unit %d\n",
		priv->copro_cmd_que_num_entries_, priv->unit);
	printk(KERN_INFO "head=0x%p, tail=0x%p, w_ptr=0x%p\n", cmd_queue->head_,
		cmd_queue->tail_, cmd_queue->w_ptr_);
	cmd = cmd_queue->head_;
	i=0;
	while (cmd != cmd_queue->tail_) {
		printk(KERN_INFO " [%d] opcode=0x%p, operand=0x%p\n", i++,
			(void*)cmd->opcode_, (void*)cmd->operand_);
		++cmd;
	}

	// Dump job queue contents
	printk(KERN_INFO "%d job queue entries for unit %d\n",
		priv->copro_tx_que_num_entries_, priv->unit);
	printk(KERN_INFO "head=0x%p, tail=0x%p, w_ptr=0x%p, r_ptr=0x%p, full=%d\n",
		job_queue->head_, job_queue->tail_, job_queue->w_ptr_,
		job_queue->r_ptr_, job_queue->full_);
	job = job_queue->head_;
	i=0;
	while (job != job_queue->tail_) {
		printk(KERN_INFO " [%d] flags=0x%hx, skb=0x%p, len=%d, data_len=%d, tso_segs=%d, tso_size=%d\n",
			i++, job->flags_, (void*)job->skb_, job->len_, job->data_len_,
			job->tso_segs_, job->tso_size_);
		++job;
	}
}
#endif // DUMP_REGS_SUPPORT

static void copro_update_callback(
	gmac_priv_t *priv,
	volatile gmac_cmd_que_ent_t *entry)
{
    up(&priv->copro_update_semaphore_);
}

static void copro_start_callback(
	gmac_priv_t *priv,
	volatile gmac_cmd_que_ent_t *entry)
{
    up(&priv->copro_start_semaphore_);
}

static void copro_rx_enable_callback(
	gmac_priv_t *priv,
	volatile gmac_cmd_que_ent_t *entry)
{
    up(&priv->copro_rx_enable_semaphore_);
}

static inline int offload_active(gmac_priv_t* priv)
{
    return (likely(priv->offload_tx));
}

static inline int copro_active(gmac_priv_t* priv)
{
    return (likely(priv->offload_tx)
#if defined(CONFIG_ARCH_OX820)
            && (unlikely(!priv->offload_netoe))
#endif
        );
}
#if defined(CONFIG_ARCH_OX820)
static inline int netoe_active(gmac_priv_t* priv)
{
    return (likely(priv->offload_tx) && (likely(priv->offload_netoe)));
}
#endif
static void gmac_int_en_set(
    gmac_priv_t *priv,
    u32          mask)
{
    unsigned long irq_flags = 0;

	if (copro_active(priv)) {
        int cmd_queue_result = -1;
        while (cmd_queue_result) {
            spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
            cmd_queue_result = cmd_que_queue_cmd(priv, GMAC_CMD_INT_EN_SET, mask, 0);
            spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
        }
        // Interrupt the CoPro so it sees the new command
        writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);
    } else {
		spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
		dma_reg_set_mask(priv, DMA_INT_ENABLE_REG, mask);
		spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
	}
}

static void copro_int_clr_callback(
	gmac_priv_t *priv,
	volatile gmac_cmd_que_ent_t *entry)
{
    priv->copro_int_clr_return_ = entry->operand_;
    up(&priv->copro_int_clr_semaphore_);
}

static void gmac_copro_int_en_clr(
    gmac_priv_t *priv,
    u32          mask,
    u32         *new_value)
{
    unsigned long irq_flags = 0;

    int cmd_queue_result = -1;
    while (cmd_queue_result) {
        spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
        cmd_queue_result = cmd_que_queue_cmd(priv, GMAC_CMD_INT_EN_CLR, mask, copro_int_clr_callback);
        spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
    }

    // Interrupt the CoPro so it sees the new command
    writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);

    if (new_value) {
        while(down_interruptible(&priv->copro_int_clr_semaphore_));
        *new_value = priv->copro_int_clr_return_;
    }
}

static void gmac_int_en_clr(
    gmac_priv_t *priv,
    u32          mask,
    u32         *new_value,
    int          in_irq)
{
    unsigned long temp;
    unsigned long irq_flags = 0;

    if (in_irq)
        spin_lock(&priv->cmd_que_lock_);
    else
        spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);

    temp = dma_reg_clear_mask(priv, DMA_INT_ENABLE_REG, mask);

    if (in_irq)
        spin_unlock(&priv->cmd_que_lock_);
    else
        spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);

    if (new_value) {
        *new_value = temp;
    }
}

/**
 * May be invoked from either ISR or process context
 */
static void change_rx_enable(
    gmac_priv_t *priv,
    u32          start,
    int          waitForAck,
    int          in_irq)
{
	if (copro_active(priv)) {        
		unsigned long irq_flags = 0;
		int cmd_queue_result = -1;

		while (cmd_queue_result) {
			if (in_irq)
				spin_lock(&priv->cmd_que_lock_);
			else
				spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);

			// Request the CoPro to start/stop GMAC receiver
			cmd_queue_result =
				cmd_que_queue_cmd(priv,
								  GMAC_CMD_CHANGE_RX_ENABLE,
								  start,
								  waitForAck ? copro_rx_enable_callback : 0);

			if (in_irq)
				spin_unlock(&priv->cmd_que_lock_);
			else
				spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
		}

		// Interrupt the CoPro so it sees the new command
		writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);

		if (waitForAck) {
			// Wait until the CoPro acknowledges that the receiver has been stopped
			if (in_irq) {
				while (down_trylock(&priv->copro_rx_enable_semaphore_)) {
					udelay(100);
				}
			} else {
				while(down_interruptible(&priv->copro_rx_enable_semaphore_));
			}
		}
	} else {
		start ? dma_reg_set_mask(priv, DMA_OP_MODE_REG, 1UL << DMA_OP_MODE_SR_BIT) :
				dma_reg_clear_mask(priv, DMA_OP_MODE_REG, 1UL << DMA_OP_MODE_SR_BIT);
	}
}

/*
 * Until we understand the point of the TC, FES and LUD bits I'm going to always
 * have TC clear (U-Boot works for LSI at 100M with TC clear), so the following
 * code should currently have the same impact as before we added RGMII support,
 * i.e. only changing the PS bit.
 */
static void configure_for_link_speed(gmac_priv_t *priv)
{
	
	u32 link_speed = ethtool_cmd_speed(&(priv->ethtool_cmd));// priv->mii.using_1000 ? 1000 : priv->mii.using_100 ? 100 : 10;

	if (priv->copro_started) {
		unsigned long irq_flags = 0;
		int cmd_queue_result = -1;

		while (cmd_queue_result) {
			// Request the CoPro to set the link speed
			spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
			cmd_queue_result = cmd_que_queue_cmd(priv,
				GMAC_CMD_CONFIG_LINK_SPEED, link_speed, 0);
			spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
		}

		// Interrupt the CoPro so it sees the new command
		writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);
	} else {
		// Possibly need to stop Tx and Rx here, but not convinced it helps

		switch (link_speed) {
			case 1000:
				mac_reg_clear_mask(priv, MAC_CONFIG_REG,
					(1UL << MAC_CONFIG_PS_BIT) |
					(1UL << MAC_CONFIG_TC_BIT) |
					(1UL << MAC_CONFIG_FES_BIT));
				break;
			case 100:
				mac_reg_set_mask(priv, MAC_CONFIG_REG,
					(1UL << MAC_CONFIG_PS_BIT)|
					(1UL << MAC_CONFIG_FES_BIT));
				mac_reg_clear_mask(priv, MAC_CONFIG_REG,
					(1UL << MAC_CONFIG_TC_BIT) );
				break;
			case 10:
				mac_reg_set_mask(priv, MAC_CONFIG_REG,
					(1UL << MAC_CONFIG_PS_BIT) |
					(1UL << MAC_CONFIG_TC_BIT) );
				mac_reg_clear_mask(priv, MAC_CONFIG_REG,
					(1UL << MAC_CONFIG_FES_BIT));
		}

		// If stopped Tx and Rx above should now restart them
	}
}

/**
 * Invoked from the watchdog timer action routine which runs as softirq, so
 * must disable interrupts when obtaining locks
 */
static void change_pause_mode(gmac_priv_t *priv)
{
//	if (priv->mii.using_pause) {
//		mac_reg_set_mask(priv, MAC_FLOW_CNTL_REG, (1UL << MAC_FLOW_CNTL_TFE_BIT));
//	} else {
		mac_reg_clear_mask(priv, MAC_FLOW_CNTL_REG, (1UL << MAC_FLOW_CNTL_TFE_BIT));
//	}
}

static void refill_rx_ring(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
	int filled = 0;

	if (likely(priv->rx_buffers_per_page_)) {
		// Receive into pages
		struct page *page = 0;
		int offset = 0;
		dma_addr_t phys_adr = 0;

		// While there are empty RX descriptor ring slots
		while (1) {
			int available;
			int desc;
			rx_frag_info_t frag_info;
			struct sk_buff *skb;

			// Have we run out of space in the current page?
			if (offset + NET_IP_ALIGN + priv->rx_buffer_size_ > PAGE_SIZE) {
				page = 0;
				offset = 0;
			}

			if (!page) {
				// Start a new page
				available = available_for_write(&priv->rx_gmac_desc_list_info);
				if (available < priv->rx_buffers_per_page_) {
					break;
				}

				// Allocate a page to hold some received packets
				page = alloc_page(GFP_ATOMIC);
				if (unlikely(page == NULL)) {
					printk(KERN_WARNING "refill_rx_ring() Could not alloc page\n");
					break;
				}

				// Get a consistent DMA mapping for the entire page that will be
				// DMAed to - causing an invalidation of any entries in the CPU's
				// cache covering the memory region
				phys_adr = dma_map_page(0, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);
				BUG_ON(dma_mapping_error(0, phys_adr));
			} else if (available_for_write(&priv->rx_gmac_desc_list_info)) {
				// Use the current page again
				get_page(page);
			} else {
				// No Rx descriptors available, so stop refilling descriptor ring
				break;
			}

			if (peek_rx_descriptor_arg(priv)) {
				// Still an unused skb available with the descriptor
				skb = 0;
			} else {
				// Attempt to allocate a new skb to associate with the descriptor
				skb = dev_alloc_skb(CONFIG_OXNAS_GMAC_HLEN + NET_IP_ALIGN);
				if (unlikely(!skb)) {
					// Clean up pages
					if (!offset) {
						// Just allocated new page and mapped for DMA
						dma_unmap_page(0, phys_adr, PAGE_SIZE, DMA_FROM_DEVICE);
					}
					put_page(page);
					break;
				}

				// Align IP header start in header storage
				skb_reserve(skb, NET_IP_ALIGN);
			}

			// Ensure IP header is quad aligned
			offset += NET_IP_ALIGN;
			frag_info.page = page;
			frag_info.length = priv->rx_buffer_size_;
			frag_info.phys_adr = phys_adr + offset;

			// Store pointer to the newly allocated skb, or zero if re-using
			// the existing one
			frag_info.arg = (u32)skb;

			// Try to associate a descriptor with the fragment info
			desc = set_rx_descriptor(priv, &frag_info);

			// We tested for descriptor availability before reaching this
			// point, so something has gone horribly wrong
			BUG_ON(desc < 0);

			// Record that we've refilled at least one entry in the ring
			filled = 1;

			// Account for the space used in the current page
			offset += frag_info.length;

			// Start next packet on a cacheline boundary
			offset = SKB_DATA_ALIGN(offset);
		}
	} else {
		// Preallocate MTU-sized SKBs
		while (1) {
			struct sk_buff *skb;
			rx_frag_info_t frag_info;
			int desc;

			if (!available_for_write(&priv->rx_gmac_desc_list_info)) {
				break;
			}

			// Allocate a new skb for the descriptor ring which is large enough
			// for any packet received from the link
			skb = dev_alloc_skb(priv->rx_buffer_size_ + NET_IP_ALIGN);
			if (unlikely(!skb)) {
				// Can't refill any more RX descriptor ring entries
				break;
			} else {
				// Despite what the comments in the original code from Synopsys
				// claimed, the GMAC DMA can cope with non-quad aligned buffers
				// - it will always perform quad transfers but zero/ignore the
				// unwanted bytes.
				skb_reserve(skb, NET_IP_ALIGN);
			}

			// Get a consistent DMA mapping for the memory to be DMAed to
			// causing invalidation of any entries in the CPU's cache covering
			// the memory region
			frag_info.page = (struct page*)skb;
			frag_info.length = priv->rx_buffer_size_;
			frag_info.phys_adr = dma_map_single(0, skb->tail, frag_info.length, DMA_FROM_DEVICE);
			BUG_ON(dma_mapping_error(0, frag_info.phys_adr));

			// Associate the skb with the descriptor
			desc = set_rx_descriptor(priv, &frag_info);
			if (desc >= 0) {
				filled = 1;
			} else {
				// No, so release the DMA mapping for the socket buffer
				dma_unmap_single(0, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);

				// Free the socket buffer
				dev_kfree_skb(skb);

				// No more RX descriptor ring entries to refill
				break;
			}
		}
	}
}

static inline bool is_phy_interface_rgmii(int unit)
{
#if defined(CONFIG_ARCH_OX820)
	u32 reg_contents = unit ? readl(SYS_CTRL_GMACB_CTRL) : readl(SYS_CTRL_GMACA_CTRL);
	return !!(reg_contents | (1UL << SYS_CTRL_GMAC_RGMII));
#else // CONFIG_ARCH_OX820
	return 0;
#endif // CONFIG_ARCH_OX820
}

static void initialise_phy(gmac_priv_t* priv)
{
	switch (priv->phy_type) {
		case PHY_TYPE_VITESSE_VSC8201XVZ:
			{
				// Allow s/w to override mode/duplex pin settings
				u32 acsr = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, VSC8201_MII_ACSR);

				printk(KERN_INFO "%s: PHY is Vitesse VSC8201XVZ, type 0x%08x\n", priv->netdev->name, priv->phy_type);
				acsr |= (1UL << VSC8201_MII_ACSR_MDPPS_BIT);
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, VSC8201_MII_ACSR, acsr);
			}
			break;
		case PHY_TYPE_REALTEK_RTL8211BGR:
			printk(KERN_INFO "%s: PHY is Realtek RTL8211BGR, type 0x%08x\n", priv->netdev->name, priv->phy_type);
			break;
		case PHY_TYPE_REALTEK_RTL8211D:
			{
			u32 phy_reg;
			printk(KERN_INFO "%s: PHY is Realtek RTL8211D, type 0x%08x\n", priv->netdev->name, priv->phy_type);
			phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, 0x09);
			phy_reg &= ~(7 << 13);
			phy_reg |=  ((test_mode & 0x07) << 13);
			priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, 0x09, phy_reg);
			}
			break;
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
			{
				u32 phy_reg;

				printk(KERN_INFO "%s: PHY is LSI ET1011C, type 0x%08x\n", priv->netdev->name, priv->phy_type);

				if (is_phy_interface_rgmii(priv->unit)) {
					// Configure clocks for RGMII
					phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONFIG);
					phy_reg &= ~(((1UL << ET1011C_MII_CONFIG_IFMODESEL_NUM_BITS) - 1) << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (ET1011C_MII_CONFIG_IFMODESEL_RGMII_TRACE << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (1UL << ET1011C_MII_CONFIG_SYSCLKEN);
					priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONFIG, phy_reg);

					phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONTROL);
					phy_reg |= ET1011C_MII_CONTROL_ALT_RGMII_DELAY;
					priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONTROL, phy_reg);
				} else {
					// Configure clocks for (G)MII
					phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONFIG);
					phy_reg &= ~(((1UL << ET1011C_MII_CONFIG_IFMODESEL_NUM_BITS) - 1) << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (ET1011C_MII_CONFIG_IFMODESEL_GMII_MII << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= ((1UL << ET1011C_MII_CONFIG_SYSCLKEN) |
								  (1UL << ET1011C_MII_CONFIG_TXCLKEN) |
								   (1UL << ET1011C_MII_CONFIG_TBI_RATESEL) |
								   (1UL << ET1011C_MII_CONFIG_CRS_TX_EN));
					priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_CONFIG, phy_reg);
				}

				// Enable Tx/Rx LED
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_LED2);
				phy_reg &= ~(((1UL << ET1011C_MII_LED2_LED_NUM_BITS) - 1) << ET1011C_MII_LED2_LED_TXRX);
				phy_reg |= (ET1011C_MII_LED2_LED_TXRX_ACTIVITY << ET1011C_MII_LED2_LED_TXRX);
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_LED2, phy_reg);
			}
			break;
		case PHY_TYPE_ICPLUS_IP1001_0:
		case PHY_TYPE_ICPLUS_IP1001_1:
			printk(KERN_INFO "%s: PHY is ICPlus 1001, type 0x%08x\n", priv->netdev->name, priv->phy_type);

			if (is_phy_interface_rgmii(priv->unit)) {
				u32 phy_reg;

				printk(KERN_INFO "%s: Tuning ICPlus 1001 PHY for RGMII\n", priv->netdev->name);

                /* Delay on tx clock */
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, IP1001LF_PHYSPECIFIC_CSR);
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXPHASE_SEL;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_TXPHASE_SEL;

                /* Adjust digital drive strength */
				phy_reg &= ~IP1001LF_PHYSPECIFIC_CSR_DRIVE_MASK;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_HI;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_HI;
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, IP1001LF_PHYSPECIFIC_CSR, phy_reg);
            }
			break;
		default:
			// Not a type of PHY we recognise
			printk(KERN_INFO "%s: Unknown PHY, type 0x%08x\n", priv->netdev->name, priv->phy_type);
	}
}

static void do_pre_reset_actions(gmac_priv_t* priv)
{
	switch (priv->phy_type) {
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
			{
				u32 phy_reg;

				printk(KERN_INFO "%s: LSI ET1011C PHY no Rx clk workaround start\n", priv->netdev->name);

				// Enable all digital loopback
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, ET1011C_MII_LOOPBACK_CNTL);
				phy_reg &= ~(1UL << ET1011C_MII_LOOPBACK_MII_LOOPBACK);
				phy_reg |=  (1UL << ET1011C_MII_LOOPBACK_DIGITAL_LOOPBACK);
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, ET1011C_MII_LOOPBACK_CNTL, phy_reg);

				// Disable auto-negotiation and enable loopback
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMCR);
				phy_reg &= ~BMCR_ANENABLE;
				phy_reg |=  BMCR_LOOPBACK;
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_BMCR, phy_reg);
			}
			break;
	}
}

static void do_post_reset_actions(gmac_priv_t* priv)
{
	switch (priv->phy_type) {
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
			{
				u32 phy_reg;

				printk(KERN_INFO "%s: LSI ET1011C PHY no Rx clk workaround end\n", priv->netdev->name);

				// Disable loopback and enable auto-negotiation
				phy_reg = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMCR);
				phy_reg |=  BMCR_ANENABLE;
				phy_reg &= ~BMCR_LOOPBACK;
				priv->mii.mdio_write(priv->netdev, priv->mii.phy_id, MII_BMCR, phy_reg);
			}
			break;
	}
}

static struct kobj_type ktype_gmac_link_state = {
	.release = 0,
	.sysfs_ops = 0,
	.default_attrs = 0,
};

static int gmac_link_state_hotplug_filter(struct kset* kset, struct kobject* kobj) {
	return get_ktype(kobj) == &ktype_gmac_link_state;
}

static struct kset_uevent_ops gmac_link_state_uevent_ops = {
	.filter = gmac_link_state_hotplug_filter,
	.name   = NULL,
	.uevent = NULL,
};

static void work_handler(struct work_struct *ws) {
	gmac_priv_t *priv = container_of(ws, gmac_priv_t, link_state_change_work);

	kobject_uevent(&priv->link_state_kobject, priv->link_state ? KOBJ_ONLINE : KOBJ_OFFLINE);
}

static void link_state_change_callback(
	int   link_state,
	void *arg)
{
	gmac_priv_t* priv = (gmac_priv_t*)arg;

	priv->link_state = link_state;
	schedule_work(&priv->link_state_change_work);
}

static void start_watchdog_timer(gmac_priv_t* priv)
{
    priv->watchdog_timer.expires = jiffies + WATCHDOG_TIMER_INTERVAL;
    priv->watchdog_timer_shutdown = 0;
	priv->watchdog_timer_state = WDS_IDLE;
    mod_timer(&priv->watchdog_timer, priv->watchdog_timer.expires);
}

static void delete_watchdog_timer(gmac_priv_t* priv)
{
    // Ensure link/PHY state watchdog timer won't be invoked again
    priv->watchdog_timer_shutdown = 1;
    del_timer_sync(&priv->watchdog_timer);
}

static inline int is_auto_negotiation_in_progress(gmac_priv_t* priv)
{
    return !(priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMSR) & BMSR_ANEGCOMPLETE);
}

/* static void wait_autoneg_complete(gmac_priv_t* priv) */
/* { */
/* 	unsigned long end = jiffies + 10*HZ; */
/* 	unsigned long tick_end = jiffies; */

/* 	printk("Waiting for auto-negotiation to complete"); */
/* 	while (is_auto_negotiation_in_progress(priv) && time_before(jiffies, end)) { */
/* 		if (time_after(jiffies, tick_end)) { */
/* 			printk("."); */
/* 			tick_end = jiffies + HZ/2; */
/* 		} */
/* 	} */
/* 	if (!time_before(jiffies, end)) { */
/* 		printk("\nTimed-out of wait"); */
/* 	} */
/* 	printk("\n"); */
/* } */

static void watchdog_timer_action(unsigned long arg)
{
    gmac_priv_t* priv = (gmac_priv_t*)arg;
    unsigned long new_timeout = jiffies + WATCHDOG_TIMER_INTERVAL;
    int ready;
    int duplex_changed;
    int speed_changed;
    int pause_changed;
    struct ethtool_cmd ecmd = { .cmd = ETHTOOL_GSET };

	// Interpret the PHY/link state.
	if (priv->phy_force_negotiation || (priv->watchdog_timer_state == WDS_RESETTING)) {
		mii_check_link(&priv->mii);
		ready = 0;
	} else {
		/*duplex_changed = mii_check_media_ex(&priv->mii, 1,
			priv->mii_init_media, &speed_changed, &pause_changed,
			link_state_change_callback, priv);*/
		duplex_changed = mii_check_media(&priv->mii, 0, 1);
		mii_ethtool_gset(&priv->mii, &ecmd);
		if (ethtool_cmd_speed(&(priv->ethtool_cmd)) != ecmd.speed) {
			ethtool_cmd_speed_set(&(priv->ethtool_cmd), ecmd.speed);
			speed_changed = 1;
		}
		if ((duplex_changed || speed_changed) || (duplex_changed && speed_changed)) {
			link_state_change_callback(1, priv);
		} else {
			link_state_change_callback(0, priv);
		}
		priv->mii_init_media = 0;
		ready = netif_carrier_ok(priv->netdev);
	}

    if (!ready) {
        if (priv->phy_force_negotiation) {
            if (netif_carrier_ok(priv->netdev)) {
                priv->watchdog_timer_state = WDS_RESETTING;
            } else {
                priv->watchdog_timer_state = WDS_IDLE;
            }

            priv->phy_force_negotiation = 0;
        }

        // May be a good idea to restart everything here, in an attempt to clear
        // out any fault conditions
        if ((priv->watchdog_timer_state == WDS_NEGOTIATING) &&
			 is_auto_negotiation_in_progress(priv)) {
            new_timeout = jiffies + AUTO_NEG_INTERVAL;
        } else {
            switch (priv->watchdog_timer_state) {
                case WDS_IDLE:
                    // Reset the PHY to get it into a known state
                    start_phy_reset(priv);
                    new_timeout = jiffies + START_RESET_INTERVAL;
                    priv->watchdog_timer_state = WDS_RESETTING;
                    break;
                case WDS_RESETTING:
                    if (!is_phy_reset_complete(priv)) {
                        new_timeout = jiffies + RESET_INTERVAL;
                    } else {
                        /* post_phy_reset_action(priv->netdev); */

                        // Set PHY specfic features
                        initialise_phy(priv);

                        // Force or auto-negotiate PHY mode
                        set_phy_negotiate_mode(priv->netdev);

                        priv->watchdog_timer_state = WDS_NEGOTIATING;
                        new_timeout = jiffies + AUTO_NEG_INTERVAL;
                    }
                    break;
                default:
                    DBG(1, KERN_ERR "watchdog_timer_action() %s: Unexpected state\n", priv->netdev->name);
                    priv->watchdog_timer_state = WDS_IDLE;
                    break;
            }
        }
    } else {
#ifdef USE_TX_TIMEOUT
		struct net_device* dev = priv->netdev;
#endif // USE_TX_TIMEOUT

        priv->watchdog_timer_state = WDS_IDLE;
        if (duplex_changed) {
            priv->mii.full_duplex ? mac_reg_set_mask(priv,   MAC_CONFIG_REG, (1UL << MAC_CONFIG_DM_BIT)) :
                                    mac_reg_clear_mask(priv, MAC_CONFIG_REG, (1UL << MAC_CONFIG_DM_BIT));
        }

        if (speed_changed) {
            configure_for_link_speed(priv);
        }

		if (pause_changed) {
			change_pause_mode(priv);
		}

#ifdef USE_TX_TIMEOUT
		if (netif_queue_stopped(dev) &&
			time_after(jiffies, (dev->trans_start + GMAC_TX_TIMEOUT))) {

			printk(KERN_INFO "watchdog_timer_action() Invoke timeout processing, tx_timeout_count = %d\n",
				priv->tx_timeout_count);

			// Do the reset outside of interrupt context
			priv->tx_timeout_count++;
			schedule_work(&priv->tx_timeout_work);
		}
#endif // USE_TX_TIMEOUT
    }

    // Re-trigger the timer, unless some other thread has requested it be stopped
    if (!priv->watchdog_timer_shutdown) {
        // Restart the timer
        mod_timer(&priv->watchdog_timer, new_timeout);
    }
}

static int inline is_ip_packet(unsigned short eth_protocol)
{
    return (eth_protocol == ETH_P_IP)
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
		|| (eth_protocol == ETH_P_IPV6)
#endif // CONFIG_OXNAS_IPV6_OFFLOAD
		;
}

static int inline is_ipv4_packet(unsigned short eth_protocol)
{
    return eth_protocol == ETH_P_IP;
}

#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
static int inline is_ipv6_packet(unsigned short eth_protocol)
{
    return eth_protocol == ETH_P_IPV6;
}
#endif // CONFIG_OXNAS_IPV6_OFFLOAD

static int inline is_hw_checksummable(unsigned short protocol)
{
    return (protocol == IPPROTO_TCP) ||
		   (protocol == IPPROTO_UDP) ||
		   (protocol == IPPROTO_ICMP) ;
}

static inline u32 unmap_rx_page(
	gmac_priv_t *priv,
	dma_addr_t   phys_adr)
{
	// Offset within page of start of Rx data
	u32 offset = phys_adr & ~PAGE_MASK;

#if !defined(CONFIG_ARCH_OXNAS) && !defined(CONFIG_ARCH_OX820)
	// If this is the last packet in a page
	if (((offset - NET_IP_ALIGN) +
		 (priv->bytes_consumed_per_rx_buffer_ << 1)) > PAGE_SIZE) {
		// Release the DMA mapping for the page
		dma_unmap_page(0, phys_adr & PAGE_MASK, PAGE_SIZE, DMA_FROM_DEVICE);
	}
#endif // !CONFIG_ARCH_OXNAS && !CONFIG_ARCH_OX820

	return offset;
}

#define FCS_LEN 4		// Ethernet CRC length

static inline int get_desc_len(
	u32 desc_status,
	int last)
{
	int length = get_rx_length(desc_status);

	if (last && (length > FCS_LEN)) {
		length -= FCS_LEN;
	}

	return length;
}

static int process_rx_packet_skb(gmac_priv_t *priv)
{
	int             desc;
	int             last;
	u32             desc_status = 0;
	rx_frag_info_t  frag_info;
	int             packet_len;
	struct sk_buff *skb;
	int             valid;
	int             ip_summed;

	desc = get_rx_descriptor(priv, &last, &desc_status, &frag_info);
	if (desc < 0) {
		return 0;
	}

	// Release the DMA mapping for the received data
    dma_unmap_single(0, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);

	// Get pointer to the SKB
	skb = (struct sk_buff*)frag_info.page;

	if (!last) {
		DBG(20, KERN_INFO "Received packet too long, possible MTU mismatch\n");

		// Force a length error into the descriptor status
		desc_status = force_rx_length_error(desc_status);
		goto not_valid_skb;
	}

	// Is the packet entirely contained within the descriptors and without errors?
	valid = !(desc_status & (1UL << RDES0_ES_BIT));
	if (unlikely(!valid)) {
		goto not_valid_skb;
	}

	// Get the packet data length
	packet_len = get_desc_len(desc_status, last);
	if (packet_len > priv->rx_buffer_size_) {
		DBG(20, KERN_INFO "Received length %d greater than Rx buffer size %d\n", packet_len, priv->rx_buffer_size_);

		// Force a length error into the descriptor status
		desc_status = force_rx_length_error(desc_status);
		goto not_valid_skb;
	}

	ip_summed = CHECKSUM_NONE;

#ifdef USE_RX_CSUM
	// Has the h/w flagged an IP header checksum failure?
	valid = !(desc_status & (1UL << RDES0_IPC_BIT));

	if (likely(valid)) {
		// Determine whether Ethernet frame contains an IP packet -
		// only bother with Ethernet II frames, but do cope with
		// 802.1Q VLAN tag presence
		int vlan_offset = 0;
		unsigned short eth_protocol = ntohs(((struct ethhdr*)skb->data)->h_proto);
		int is_ip = is_ip_packet(eth_protocol);

		if (!is_ip) {
			// Check for VLAN tag
			if (eth_protocol == ETH_P_8021Q) {
				// Extract the contained protocol type from after
				// the VLAN tag
				eth_protocol = ntohs(*(unsigned short*)(skb->data + ETH_HLEN));
				is_ip = is_ip_packet(eth_protocol);

				// Adjustment required to skip the VLAN stuff and
				// get to the IP header
				vlan_offset = 4;
			}
		}

		// Only offload checksum calculation for IP packets
		if (is_ip) {
			struct iphdr* ipv4_header = 0;

			if (unlikely(desc_status & (1UL << RDES0_PCE_BIT))) {
				valid = 0;
			} else
			if (is_ipv4_packet(eth_protocol)) {
				ipv4_header = (struct iphdr*)(skb->data + ETH_HLEN + vlan_offset);

				// H/W can only checksum non-fragmented IP packets
				if (!(ipv4_header->frag_off & htons(IP_MF | IP_OFFSET))) {
				if (is_hw_checksummable(ipv4_header->protocol)) {
					ip_summed = CHECKSUM_UNNECESSARY;
				}
				}
			}
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
            else if (is_ipv6_packet(eth_protocol)) {
				struct ipv6hdr* ipv6_header = (struct ipv6hdr*)(skb->data + ETH_HLEN + vlan_offset);

				if (is_hw_checksummable(ipv6_header->nexthdr)) {
					ip_summed = CHECKSUM_UNNECESSARY;
				}
			}
#endif // CONFIG_OXNAS_IPV6_OFFLOAD
		}
	}

	if (unlikely(!valid)) {
		goto not_valid_skb;
	}
#endif // USE_RX_CSUM

	// Increase the skb's data pointer to account for the RX packet that has
	// been DMAed into it
	skb_put(skb, packet_len);

	// Set the device for the skb
	skb->dev = priv->netdev;

	// Set packet protocol
	skb->protocol = eth_type_trans(skb, priv->netdev);

	// Record whether h/w checksumed the packet
	skb->ip_summed = ip_summed;

	// Send the packet up the network stack
	netif_receive_skb(skb);

	// Update receive statistics
	priv->netdev->last_rx = jiffies;
	++priv->stats.rx_packets;
	priv->stats.rx_bytes += packet_len;

	return 1;

not_valid_skb:
	dev_kfree_skb(skb);

	DBG(2, KERN_WARNING "process_rx_packet_skb() %s: Received packet has bad desc_status = 0x%08x\n", priv->netdev->name, desc_status);

	// Update receive statistics from the descriptor status
	if (is_rx_collision_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet_skb() %s: Collision (status: 0x%08x)\n", priv->netdev->name, desc_status);
		++priv->stats.collisions;
	}
	if (is_rx_crc_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet_skb() %s: CRC error (status: 0x%08x)\n", priv->netdev->name, desc_status);
		++priv->stats.rx_crc_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_frame_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet_skb() %s: frame error (status: 0x%08x)\n", priv->netdev->name, desc_status);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_length_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet_skb() %s: Length error (status: 0x%08x)\n", priv->netdev->name, desc_status);
		++priv->stats.rx_length_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_csum_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet_skb() %s: Checksum error (status: 0x%08x)\n", priv->netdev->name, desc_status);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}

	return 1;
}

static int process_rx_packet(gmac_priv_t *priv)
{
	struct sk_buff         *skb;
	struct skb_shared_info *shinfo;
	int                     last;
	u32                     desc_status;
	rx_frag_info_t          frag_info;
	int                     desc;
	u32                     offset;
	int                     desc_len;
	unsigned char          *packet;
	int                     valid;
	int                     desc_used = 0;
	int                     hlen = 0;
	int                     partial_len = 0;
	int                     first = 1;
    int                     crc_remaining = 0;
    
	// Check that there is at least one Rx descriptor available. Cache the
	// descriptor information so we don't have to touch the uncached/unbuffered
	// descriptor memory more than necessary when we come to use that descriptor
	if (!rx_available_for_read(&priv->rx_gmac_desc_list_info, &desc_status)) {
		return 0;
	}

	// Recover the skb associated with this descriptor
	skb = (struct sk_buff*)getandclear_rx_descriptor_arg(priv);
	BUG_ON(!skb);
	shinfo = skb_shinfo(skb);

	// Process all descriptors associated with the packet
	while (1) {
		skb_frag_t *frag;
		int prev_len;

		// First call to get_rx_descriptor() will use the status read from the
		// first descriptor by the call to rx_available_for_read() above
		while ((desc = get_rx_descriptor(priv, &last, &desc_status, &frag_info)) < 0) {
			// We are part way through processing a multi-descriptor packet
			// and the GMAC hasn't finished with the next descriptor for the
			// packet yet, so have to poll until it becomes available
			desc_status = 0;
		}
		BUG_ON(!frag_info.page);

		// We've consumed a descriptor
		++desc_used;

		// If this is the last packet in the page, release the DMA mapping
		offset = unmap_rx_page(priv, frag_info.phys_adr);
		if (!first) {
			// The buffer adr of descriptors associated with middle or last
			// parts of a packet have ls 2 bits of buffer adr ignored by GMAC DMA
			offset &= ~0x3;
            //printk(KERN_INFO "process_rx_packet() Non-first descriptor\n");
		}

		// Get the length of the packet including CRC, h/w csum etc.
		prev_len = partial_len;
		partial_len = get_rx_length(desc_status);
		desc_len = partial_len - prev_len;

        // Check for last packet and deduct CRC len
        if (last) {
            partial_len -= FCS_LEN;
            // Check that the CRC didn't span two buffers, otherwise we'll have to clear up later
            if (likely(desc_len > FCS_LEN)) {
                desc_len -= FCS_LEN;
            } else {
                crc_remaining = FCS_LEN;
            }
        }
        
		if (unlikely(!desc_len)) {
			printk(KERN_WARNING "process_rx_packet() %s: skb %p desc_len is zero, partial_len %d\n", priv->netdev->name, skb, partial_len);
		}

        // Sanity check the descriptor length
//        if (unlikely(desc_len > priv->rx_buffer_size_)) {
//            printk(KERN_WARNING "process_rx_packet() %s: skb %p desc_len is %d (rx_buffer_size %d), partial_len %d\n",
//                   priv->netdev->name, skb, desc_len, priv->rx_buffer_size_, partial_len);
//        }

		// Get a pointer to the start of the packet data received into page
		packet = page_address(frag_info.page) + offset;

		// Is the packet entirely contained within the desciptors and without errors?
		valid = !(desc_status & (1UL << RDES0_ES_BIT));

		if (unlikely(!valid)) {
			goto not_valid;
		}

		if (first) {
			// How much to copy to the skb buffer to allow protocol processing
			// by the network stack?
			hlen = min(CONFIG_OXNAS_GMAC_HLEN, desc_len);

			if (!hlen && !last) {
				// GMAC has given us a zero-length descriptor that doesn't fully
				// describe a packet
				printk(KERN_WARNING "process_rx_packet() %s: skb %p hlen is zero, first %d, last %d - dumping descriptor\n", priv->netdev->name, skb, first, last);

				// There should be a sensible descriptor following this one
				// so pretend we never saw this zero length descriptor
				put_page(frag_info.page);
			} else {
				// This appears to be a sensible first descriptor for the packet
				first = 0;

				// Copy header into skb's buffer and update skb metadata to record
				// that it now contains this header data
				memcpy(skb->data, packet, hlen);
				skb->tail += hlen;
                //printk(KERN_INFO "process_rx_packet() skb %p hlen = %d, offset %d, desc_len %d\n", skb, hlen, offset, desc_len);

#ifndef CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
				if (desc_len > hlen) {
#endif // !CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
					// Size of zero resulting when all of packet has been copied
					// into skb's buffer should stop network Rx stack from using any
					// data from this page, but the ZeroCopyRx code can still infer
					// the length from skb->len
					frag = &shinfo->frags[0];
					shinfo->nr_frags = 1;
					frag->page.p		  = frag_info.page;
					frag->page_offset = offset + hlen;
					frag->size		  = desc_len - hlen;
                    //printk(KERN_INFO "process_rx_packet() skb %p frag[0]: page %p, off %d, size %d\n", skb, frag->page, frag->page_offset, frag->size);
#ifndef CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
				} else {
					// Entire packet now in skb buffer so don't require page anymore
					// for the normal, i.e. not zerocopy, case
					put_page(frag_info.page);
				}
#endif // !CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
			}
		} else {
			// Store intermediate descriptor data into packet
			frag = &shinfo->frags[shinfo->nr_frags];
			frag->page.p		  = frag_info.page;
			frag->page_offset = offset;
			frag->size		  = desc_len;
            //printk(KERN_INFO "process_rx_packet() skb %p frag[%d]: page %p, off %d, size %d\n", skb, shinfo->nr_frags, frag->page, frag->page_offset, frag->size);
			++shinfo->nr_frags;
		}

		if (last) {
			int ip_summed = CHECKSUM_NONE;

            // Deduct the CRC length from the packet. Note the CRC may span two buffers.
            if (unlikely(crc_remaining)) {
                if (shinfo->nr_frags > 1) {
                    // Deduct as much of the CRC as we can from the last frag
                    unsigned crc_len_last_frag = min((__u32)FCS_LEN, frag->size);                
                    frag->size -= crc_len_last_frag;
                    crc_remaining -= crc_len_last_frag;
                    printk(KERN_INFO "process_rx_packet: detected CRC in separate buffer\n");
                    if (unlikely(crc_remaining)) {
                        frag = &shinfo->frags[shinfo->nr_frags - 2];
                        frag->size -= crc_remaining;
                        printk(KERN_INFO "process_rx_packet: detected CRC across 2 buffers\n");                        
                    }                        
                } else {
                    printk(KERN_ERR "process_rx_packet: received CRC across multiple buffers but only had %d frags\n",shinfo->nr_frags);                    
                }
            }
            
            
			// Update total packet length skb metadata
			skb->len = partial_len;
			skb->data_len = skb->len - hlen;
			skb->truesize = skb->len + sizeof(struct sk_buff);
#ifdef USE_RX_CSUM
			// Has the h/w flagged an IP header checksum failure?
			valid = !(desc_status & (1UL << RDES0_IPC_BIT));

			if (unlikely(!valid)) {
				goto not_valid;
			} else if (unlikely(skb->len < ETH_HLEN)) {
				// Packet is too short to contain an Ethernet header - clearly
				// this should not happen. The network stack is not too clever
				// in dealing with such packets and tries to pull from the
				// fragments into the header in order to process the protocol
				// information even of there's no data in the fragments, so
				// throw the packet away here
				printk(KERN_WARNING "process_rx_packet() %s: skb %p too short to contain Ethernet header, skb->len %d\n", priv->netdev->name, skb, skb->len);
				goto not_valid;
			} else if (likely(skb->len >= (ETH_HLEN + 20))) {
				// Determine whether Ethernet frame contains an IP packet -
				// only bother with Ethernet II frames, but do cope with
				// 802.1Q VLAN tag presence
				int vlan_offset = 0;
				unsigned short eth_protocol = ntohs(((struct ethhdr*)skb->data)->h_proto);
				int is_ip = is_ip_packet(eth_protocol);

				if (!is_ip) {
					// Check for VLAN tag
					if (eth_protocol == ETH_P_8021Q) {
						// Extract the contained protocol type from after
						// the VLAN tag
						eth_protocol = ntohs(*(unsigned short*)(skb->data + ETH_HLEN));
						is_ip = is_ip_packet(eth_protocol);

						// Adjustment required to skip the VLAN stuff and
						// get to the IP header
						vlan_offset = 4;
					}
				}

				// Only offload checksum calculation for IP packets
				if (is_ip) {
					struct iphdr* ipv4_header = 0;

					if (unlikely(desc_status & (1UL << RDES0_PCE_BIT))) {
						valid = 0;
					} else if (is_ipv4_packet(eth_protocol)) {
						ipv4_header = (struct iphdr*)(skb->data + ETH_HLEN + vlan_offset);

						// H/W can only checksum non-fragmented IP packets
						if (!(ipv4_header->frag_off & htons(IP_MF | IP_OFFSET))) {
							if (is_hw_checksummable(ipv4_header->protocol)) {
								ip_summed = CHECKSUM_UNNECESSARY;
							}
						}
					}
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
                    else if (is_ipv6_packet(eth_protocol)) {
						struct ipv6hdr* ipv6_header =
							(struct ipv6hdr*)(skb->data + ETH_HLEN + vlan_offset);
						if (is_hw_checksummable(ipv6_header->nexthdr)) {
							ip_summed = CHECKSUM_UNNECESSARY;
						}
					}
#endif // CONFIG_OXNAS_IPV6_OFFLOAD
				}
			}
#endif // USE_RX_CSUM

			// Initialise other required skb header fields
			skb->dev = priv->netdev;
			skb->protocol = eth_type_trans(skb, priv->netdev);

			// Record whether h/w checksumed the packet
			skb->ip_summed = ip_summed;
            
            // Sanity check the skb length
//            if (unlikely(skb->len > (priv->netdev->mtu + EXTRA_RX_SKB_SPACE))) {
//                printk(KERN_INFO "process_rx_packet() Before netif_receive_skb(): skb %p, len=%d, data_len=%d, nr_frags=%d \n",
//                       skb, skb->len, skb->data_len, skb_shinfo(skb)->nr_frags);
//            }

			// Mark the skb as coming from a zero copy capable interface
			//skb->zcc = 1;

			// Send the skb up the network stack
			netif_receive_skb(skb);

			// Update receive statistics
			priv->netdev->last_rx = jiffies;
			++priv->stats.rx_packets;
			priv->stats.rx_bytes += partial_len;

			break;
		}

		// Want next call to get_rx_descriptor() to read status from descriptor
		desc_status = 0;
	}
    return desc_used;

not_valid:
	if (!shinfo->nr_frags) {
		// Free the page as it wasn't attached to the skb
		put_page(frag_info.page);
	}

	dev_kfree_skb(skb);

	DBG(2, KERN_WARNING "process_rx_packet() %s: Received packet has bad desc_status = 0x%08x\n", priv->netdev->name, desc_status);

	// Update receive statistics from the descriptor status
	if (is_rx_collision_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet() %s: Collision (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.collisions;
	}
	if (is_rx_crc_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet() %s: CRC error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_crc_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_frame_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet() %s: frame error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_length_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet() %s: Length error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_length_errors;
		++priv->stats.rx_errors;
	}
	if (is_rx_csum_error(desc_status)) {
		DBG(20, KERN_INFO "process_rx_packet() %s: Checksum error (0x%08x:%u bytes)\n", priv->netdev->name, desc_status, desc_len);
		++priv->stats.rx_frame_errors;
		++priv->stats.rx_errors;
	}

	return desc_used;
}

/*
 * NAPI receive polling method
 */
static int poll(
	struct napi_struct *napi,
	int                 budget)
{
	gmac_priv_t *priv = container_of(napi, gmac_priv_t, napi_struct);
	struct net_device *dev = priv->netdev;
    int continue_polling;
    int rx_work_limit = budget;
    int work_done = 0;
    int finished;
	int total_desc_used = 0;

    finished = 0;
    do {
        // While there are receive polling jobs to be done
        u32 status;
        while (rx_work_limit) {
			int desc_used;
			int desc_since_refill = 0;

			if (likely(priv->rx_buffers_per_page_)) {
				desc_used = process_rx_packet(priv);
			} else {
				desc_used = process_rx_packet_skb(priv);
			}

			if (unlikely(!desc_used)) {
				break;
			}

			total_desc_used += desc_used;

            // Increment count of processed packets
            ++work_done;

            // Decrement our remaining budget
            if (likely(rx_work_limit > 0)) {
                --rx_work_limit;
            }

			desc_since_refill += desc_used;
            if (unlikely(desc_since_refill >= priv->desc_since_refill_limit)) {
                desc_since_refill = 0;
                refill_rx_ring(dev);
			}
        }

        if (likely(rx_work_limit)) {
            // We have unused budget, but apparently no Rx packets to process
            int available = 0;

			// If we processed more than one descriptor there may be RI status
			// hanging around that will re-interrupt us when RI interrupts are
			// re-enabled
			if (total_desc_used > 1) {
				// Clear any RI status so we don't immediately get reinterrupted
				// when we leave polling, due to either a new RI event, or a
				// left over interrupt from one of the RX descriptors we've
				// already processed
				status = dma_reg_read(priv, DMA_STATUS_REG);
				if (status & (1UL << DMA_STATUS_RI_BIT)) {
					// Ack the RI, including the normal summary sticky bit
					dma_reg_write(priv, DMA_STATUS_REG, ((1UL << DMA_STATUS_RI_BIT)  |
														 (1UL << DMA_STATUS_NIS_BIT)));
	
					// Must check again for available RX descriptors, in case
					// the RI status came from a new RX descriptor
					available = rx_available_for_read(&priv->rx_gmac_desc_list_info, 0);
				}
			}

            if (likely(!available)) {
                // We have budget left but no Rx packets to process so stop
                // polling
                continue_polling = 0;
                finished = 1;
            } else {
				// We are starting again from seeing RI asserted
//printk(KERN_INFO "Going again\n");
				total_desc_used = 0;
			}
        } else {
            // If we have consumed all our budget, don't cancel the
            // poll, the NAPI infrastructure assumes we won't
            continue_polling = 1;

            // Must leave poll() routine as no budget left
            finished = 1;
        }
    } while (!finished);

    // Attempt to fill any empty slots in the RX ring
	refill_rx_ring(dev);

    // Decrement the budget even if we didn't process any packets
    if (!work_done) {
//		printk(KERN_INFO "No work done\n");
        work_done = 1;
    }

    if (!continue_polling) {
        // No more received packets to process so return to interrupt mode
        napi_complete(napi);

        // Enable interrupts caused by received packets that may have been
		// disabled in the ISR before entering polled mode. Can be done directly
		// rather than via Leon due to the way the s/w and h/w interact - the
		// Leon clears these bits if it's found them set but we have just filled
		// rx-ring -> RU won't be asserted and if we're here then RI is currently
		// disabled; OVR should not ever occur and if it does we'll run the risk
		// of getting two rather than one interrupts due to it - fine.
		dma_reg_set_mask(priv, DMA_INT_ENABLE_REG, (1UL << DMA_INT_ENABLE_RI_BIT) |
												   (1UL << DMA_INT_ENABLE_RU_BIT) |
												   (1UL << DMA_INT_ENABLE_OV_BIT));

		// Issue a RX poll demand to restart RX descriptor processing as DFF
		// mode does not automatically restart descriptor processing after a
		// descriptor unavailable event
		dma_reg_write(priv, DMA_RX_POLL_REG, 0);

//        gmac_int_en_set(priv, (1UL << DMA_INT_ENABLE_RI_BIT) |
//                              (1UL << DMA_INT_ENABLE_RU_BIT) |
//							  (1UL << DMA_INT_ENABLE_OV_BIT));
    }

//if (!total_desc_used) printk(KERN_INFO "Total desc used is zero\n");
    
    return work_done;
}


#if defined(CONFIG_ARCH_OX820)
static void netoe_finish_xmit(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    volatile netoe_job_t *job;
    int jobs_freed = 0;
    static int dbg_flag = 0;
    //printk(KERN_INFO "netoe_finish_xmit: entry\n");
    
	// Make SMP safe - we could do with making this locking more fine grained
	spin_lock(&priv->tx_spinlock_);

    // Process all available completed jobs
    while ((job = netoe_get_completed_job(priv))) {
        if (dbg_flag) {
            //printk(KERN_INFO "netoe finish xmit - completed pending job %p, skb %p, sk %p",job, job->skb_, (struct skbuff*)(job->skb_)->sk);            
        }
        
        netoe_clear_job(priv, job);
        
        if (dbg_flag) {
            //printk(KERN_INFO "..done\n");            
        }
        // Accumulate TX statistics returned by CoPro in the job structure
        priv->stats.tx_bytes          = netoe_get_bytes(priv);        
        priv->stats.tx_packets        = netoe_get_packets(priv);
        priv->stats.tx_aborted_errors = netoe_get_aborts(priv);
        priv->stats.tx_carrier_errors = netoe_get_carrier_errors(priv);
        priv->stats.collisions        = netoe_get_collisions(priv);
        priv->stats.tx_errors         = (priv->stats.tx_aborted_errors +
                                         priv->stats.tx_carrier_errors);
        jobs_freed++;
        
    }

    dbg_flag = 0;
    // If the network stack's Tx queue was stopped and we now have resources
    // to process more Tx offload jobs
    if (netif_queue_stopped(dev) &&
        !netoe_is_full(priv)) {

        if (priv->tx_pending_skb) {
 
            job = netoe_get_free_job(priv);
            //printk(KERN_INFO "netoe finish xmit - submitting pending job %p, skb %p, sk %p\n",job,priv->tx_pending_skb,priv->tx_pending_skb->sk );
            dbg_flag = 1;
            
            if (!job) {
                // Panic - there should be a job available at this point
                panic("No NetOE job still unavailable after completion!\n");
            }
            // Record start of transmission, so timeouts will work once they're
            // implemented
            dev->trans_start = jiffies;

            // Add the job to the network offload engine
            netoe_send_job(priv, job, priv->tx_pending_skb);

            priv->tx_pending_skb = 0;

            // Poke the transmitter to look for available TX descriptors, as we have
            // just added one, in case it had previously found there were no more
            // pending transmission
            dma_reg_write(priv, DMA_TX_POLL_REG, 0);
            
            jobs_freed--;            
        }

        if (jobs_freed) {            
            //printk(KERN_INFO "netoe finish xmit - waking queue after jobs finished\n");
            // Restart the network stack's TX queue
            netif_wake_queue(dev);
        }
    }

	// Make SMP safe - we could do with making this locking more fine grained
	spin_unlock(&priv->tx_spinlock_);

}
#endif

static void copro_fill_tx_job(
	gmac_priv_t                *priv,
    volatile gmac_tx_que_ent_t *job,
    struct sk_buff             *skb)
{
    int i;
    int nr_frags = skb_shinfo(skb)->nr_frags;
    unsigned short flags = 0;
    dma_addr_t hdr_dma_address;

    // if too many fragments call sbk_linearize()
    // and take the CPU memory copies hit 
    if (nr_frags > COPRO_NUM_TX_FRAGS_DIRECT) {
        int err;
        printk(KERN_WARNING "Fill: linearizing socket buffer as required %d frags and have only %d\n", nr_frags, COPRO_NUM_TX_FRAGS_DIRECT);
        err = skb_linearize(skb);
        if (err) {
            panic("Fill: No free memory");
        }

        // update nr_frags
        nr_frags = skb_shinfo(skb)->nr_frags;
    }

    // Get a DMA mapping of the packet's data
    hdr_dma_address = dma_map_single(0, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
    BUG_ON(dma_mapping_error(0, hdr_dma_address));

    // Allocate storage for remainder of fragments and create DMA mappings
    // Get a DMA mapping for as many fragments as will fit into the first level
    // fragment info. storage within the job structure
    for (i=0; i < nr_frags; ++i) {
        struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[i];

#ifdef CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
        if (PageIncoherentSendfile(frag->page.p) ||
#else // CONFIG_OXNAS_FAST_READS_AND_WRITES
		if (
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
			(PageMappedToDisk(frag->page.p) && !PageDirty(frag->page.p))) {
            job->frag_ptr_[i] = virt_to_dma(0, page_address(frag->page.p) + frag->page_offset);
        } else {
#endif // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
            job->frag_ptr_[i] = dma_map_page(0, frag->page.p, frag->page_offset, frag->size, DMA_TO_DEVICE);
#ifdef CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
        }
#endif // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
        job->frag_len_[i] = frag->size;
    }

    // Is h/w checksumming and possibly TSO required
    if (skb->ip_summed == CHECKSUM_PARTIAL) {
		// Should only be passed offload jobs for IPv4/v6 as configured
		BUG_ON((ntohs(skb->protocol) != ETH_P_IP)
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
			&& (ntohs(skb->protocol) != ETH_P_IPV6)
#endif // CONFIG_OXNAS_IPV6_OFFLOAD
			&& 1);

        flags |= (1UL << TX_JOB_FLAGS_ACCELERATE_BIT);
	}

	if (unlikely(priv->msg_level)) {
        flags |= (1UL << TX_JOB_FLAGS_DEBUG_BIT);
	}

    // Fill the job description with information about the packet
    job->skb_         = (u32)skb;
    job->len_         = skb->len;
    job->data_len_    = skb->data_len;
    job->ethhdr_      = hdr_dma_address;
    job->iphdr_       = hdr_dma_address + (skb_network_header(skb) - skb->data);
    job->iphdr_csum_  = ((struct iphdr*)skb_network_header(skb))->check;
    job->transport_hdr_ = hdr_dma_address + (skb_transport_header(skb) - skb->data);
    job->tso_segs_    = skb_shinfo(skb)->gso_segs;
    job->tso_size_    = skb_shinfo(skb)->gso_size;
    job->flags_       = flags;
    job->statistics_  = 0;
}

static void copro_free_tx_resources(volatile gmac_tx_que_ent_t* job)
{
    int i;
    struct sk_buff* skb = (struct sk_buff*)job->skb_;
    int nr_frags = skb_shinfo(skb)->nr_frags;

    // This should never happen, since we check space when we filled
    // the job in copro_fill_tx_job
    if (nr_frags > COPRO_NUM_TX_FRAGS_DIRECT) {
        panic("Free: Insufficient fragment storage, required %d, have only %d", nr_frags, COPRO_NUM_TX_FRAGS_DIRECT);
    }

    // Release the DMA mapping for the data directly referenced by the SKB
    dma_unmap_single(0, job->ethhdr_, skb_headlen(skb), DMA_TO_DEVICE);

    // Release the DMA mapping for any fragments in the first level fragment
    // info. storage within the job structure
    for (i=0; (i < nr_frags) && (i < COPRO_NUM_TX_FRAGS_DIRECT); ++i) {
        dma_unmap_page(0, job->frag_ptr_[i], job->frag_len_[i], DMA_TO_DEVICE);
    }

    // Inform the network stack that we've finished with the packet
    dev_kfree_skb_irq(skb);
}

static void copro_finish_xmit(struct net_device *dev)
{
    gmac_priv_t                *priv = (gmac_priv_t*)netdev_priv(dev);
    volatile gmac_tx_que_ent_t *job;

	// Make SMP safe - we could do with making this locking more fine grained
	spin_lock(&priv->tx_spinlock_);

//if (netif_queue_stopped(dev)) {
//	printk(KERN_INFO "copro_finish_xmit() when Tx queue is stopped\n");
//}

    // Process all available completed jobs
    while ((job = tx_que_get_finished_job(dev))) {
        int aborted;
        int carrier;
        int collisions;
        u32 statistics = job->statistics_;

        copro_free_tx_resources(job);

        // Accumulate TX statistics returned by CoPro in the job structure
        priv->stats.tx_bytes   += (statistics & TX_JOB_STATS_BYTES_MASK)   >> TX_JOB_STATS_BYTES_BIT;
        priv->stats.tx_packets += (statistics & TX_JOB_STATS_PACKETS_MASK) >> TX_JOB_STATS_PACKETS_BIT;
        aborted    = (statistics & TX_JOB_STATS_ABORT_MASK)     >> TX_JOB_STATS_ABORT_BIT;
        carrier    = (statistics & TX_JOB_STATS_CARRIER_MASK)   >> TX_JOB_STATS_CARRIER_BIT;
        collisions = (statistics & TX_JOB_STATS_COLLISION_MASK) >> TX_JOB_STATS_COLLISION_BIT;
        priv->stats.tx_aborted_errors += aborted;
        priv->stats.tx_carrier_errors += carrier;
        priv->stats.collisions        += collisions;
        priv->stats.tx_errors += (aborted + carrier);
    }

    // If the network stack's Tx queue was stopped and we now have resources
    // to process more Tx offload jobs
    if (netif_queue_stopped(dev) &&
        !tx_que_is_full(&priv->tx_queue_)) {
        // Restart the network stack's TX queue
//printk(KERN_INFO "copro_finish_xmit() calling netif_wake_queue()\n");
        netif_wake_queue(dev);
    }

	// Make SMP safe - we could do with making this locking more fine grained
	spin_unlock(&priv->tx_spinlock_);
}

static void finish_xmit(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    u32          desc_status = 0;

    // Handle transmit descriptors for the completed packet transmission
    while (1) {
        struct sk_buff *skb;
        tx_frag_info_t  fragment;
        int             buffer_owned;
		int				 desc_index;

        // Get tx descriptor content, accumulating status for all buffers
        // contributing to each packet
		spin_lock(&priv->tx_spinlock_);
		desc_index = get_tx_descriptor(priv, &skb, &desc_status, &fragment, &buffer_owned);
		spin_unlock(&priv->tx_spinlock_);

		if (desc_index < 0) {
			// No more completed Tx packets
			break;
		}

        // Only unmap DMA buffer if descriptor owned the buffer
        if (buffer_owned) {
            // Release the DMA mapping for the buffer
            dma_unmap_single(0, fragment.phys_adr, fragment.length, DMA_TO_DEVICE);
        }

        // When all buffers contributing to a packet have been processed
        if (skb) {
            // Check the status of the transmission
            if (likely(is_tx_valid(desc_status))) {
                priv->stats.tx_bytes += skb->len;
                priv->stats.tx_packets++;
            } else {
                priv->stats.tx_errors++;
                if (is_tx_aborted(desc_status)) {
                    ++priv->stats.tx_aborted_errors;
                }
                if (is_tx_carrier_error(desc_status)) {
                    ++priv->stats.tx_carrier_errors;
                }
            }

            if (unlikely(is_tx_collision_error(desc_status))) {
                ++priv->stats.collisions;
            }

            //printk("finish_xmit %p\n",skb);
            // Inform the network stack that packet transmission has finished
            dev_kfree_skb_irq(skb);

            // Start accumulating status for the next packet
            desc_status = 0;
        }
    }

    // If the TX queue is stopped, there may be a pending TX packet waiting to
    // be transmitted
    if (unlikely(netif_queue_stopped(dev))) {
		// No locking with hard_start_xmit() required, as queue is already
		// stopped so hard_start_xmit() won't touch the h/w

        // If any TX descriptors have been freed and there is an outstanding TX
        // packet waiting to be queued due to there not having been a TX
        // descriptor available when hard_start_xmit() was presented with an skb
        // by the network stack
        if (priv->tx_pending_skb) {
            // Construct the GMAC specific DMA descriptor
            if (set_tx_descriptor(priv,
                                  priv->tx_pending_skb,
                                  priv->tx_pending_fragments,
                                  priv->tx_pending_fragment_count,
                                  priv->tx_pending_skb->ip_summed == CHECKSUM_PARTIAL) >= 0) {
                // No TX packets now outstanding
                //printk(KERN_INFO "finish_xmit() queuing tx_pending_skb %p\n", priv->tx_pending_skb);
                priv->tx_pending_skb = 0;
                priv->tx_pending_fragment_count = 0;

                // Issue a TX poll demand to restart TX descriptor processing, as we
                // have just added one, in case it had found there were no more
                // pending transmission
                dma_reg_write(priv, DMA_TX_POLL_REG, 0);
		    }
        }

        // If there are TX descriptors available we should restart the TX queue
        if (!priv->tx_pending_skb &&
			(available_for_write(&priv->tx_gmac_desc_list_info) >
				(priv->num_tx_descriptors >> 4))) {
            // The TX queue had been stopped by hard_start_xmit() due to lack of
            // TX descriptors, so restart it now that we've freed at least one
            //printk(KERN_INFO "finish_xmit() calling netif_wake_queue()\n");
            netif_wake_queue(dev);
        }
    }
}

static void process_non_dma_ints(gmac_priv_t* priv, u32 raw_status)
{
    if (raw_status & (1UL << DMA_STATUS_TTI_BIT) ) {
        printk(KERN_WARNING "%s: Found TTI interrupt\n", priv->netdev->name);
	}

    if (raw_status & (1UL << DMA_STATUS_GPI_BIT) ) {
        printk(KERN_WARNING "%s: Found GPI interrupt\n", priv->netdev->name);
	}

    if (raw_status & (1UL << DMA_STATUS_GMI_BIT) ) {
        printk(KERN_WARNING "%s: Found GMI interrupt\n", priv->netdev->name);
    }

    if (raw_status & (1UL << DMA_STATUS_GLI_BIT) ) {
#if defined(CONFIG_ARCH_OX820)
		// Read and clear RGMII interrupt status
        u32 reg = mac_reg_read(priv, MAC_RGMII_STATUS_REG);
        printk(KERN_WARNING "%s: Found GLI interrupt, RGMII status = 0x%p\n", priv->netdev->name, (void*)reg);
#else // CONFIG_ARCH_OX820
        printk(KERN_WARNING "%s: Found GLI interrupt\n", priv->netdev->name, (void*)reg);
#endif // CONFIG_ARCH_OX820
    }
}

#define GMAC_FWD_ERROR_STATUS_MASK ((1UL << DMA_STATUS_RU_BIT) |\
									(1UL << DMA_STATUS_OVF_BIT) |\
									(1UL << DMA_STATUS_RWT_BIT) |\
									(1UL << DMA_STATUS_UNF_BIT))

static void copro_fwd_intrs_handler(
    void *dev_id,
    u32   status)
{
    struct net_device *dev = (struct net_device *)dev_id;
    gmac_priv_t       *priv = (gmac_priv_t*)netdev_priv(dev);

    // Test for normal receive interrupt
    if (status & (1UL << DMA_STATUS_RI_BIT)) {
        if (likely(napi_schedule_prep(&priv->napi_struct))) {
            // Tell system we have work to be done
            __napi_schedule(&priv->napi_struct);
        } else {
            printk(KERN_ERR "copro_fwd_intrs_handler() %s: RX interrupt while in poll\n", dev->name);
        }
    }

	// Are any error conditions for which we want to record statistics asserted?
	if (unlikely(status & GMAC_FWD_ERROR_STATUS_MASK)) {
		// Test for unavailable RX buffers - CoPro should have disabled
		if (status & (1UL << DMA_STATUS_RU_BIT)) {
			// Accumulate receive statistics
			DBG(20, KERN_INFO "copro_fwd_intrs_handler() RX buffer unavailable\n");
			++priv->stats.rx_over_errors;
			++priv->stats.rx_errors;
		}

		// Test for Rx overflow - CoPro should have disabled
		if (status & (1UL << DMA_STATUS_OVF_BIT)) {
			// Accumulate receive statistics
			DBG(20, KERN_INFO "copro_fwd_intrs_handler() RX overflow\n");
			++priv->stats.rx_fifo_errors;
			++priv->stats.rx_errors;
		}

		if (status & (1UL << DMA_STATUS_RWT_BIT)) {
			// Accumulate receive statistics
			DBG(20, KERN_INFO "copro_fwd_intrs_handler() RWT seen\n");
			++priv->stats.rx_frame_errors;
			++priv->stats.rx_errors;
		}

		// Test for transmitter FIFO underflow
		if (status & (1UL << DMA_STATUS_UNF_BIT)) {
			++priv->stats.tx_fifo_errors;
			++priv->stats.tx_errors;
		}
	}
}

static irqreturn_t int_handler(int int_num, void* dev_id)
{
    struct net_device *dev = (struct net_device *)dev_id;
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    u32 int_enable;
    int rx_polling;
    u32 raw_status;
    u32 status;

    /** Read the interrupt enable register to determine if we're in rx poll mode
     *  Id like to get rid of this read, if a more efficient way of determining
     *  whether we are polling is available */
    spin_lock(&priv->cmd_que_lock_);
    int_enable = dma_reg_read(priv, DMA_INT_ENABLE_REG);
    spin_unlock(&priv->cmd_que_lock_);

    rx_polling = !(int_enable & (1UL << DMA_INT_ENABLE_RI_BIT));

    // Get interrupt status
    raw_status = dma_reg_read(priv, DMA_STATUS_REG);

    // MMC, PMT and GLI interrupts are not masked by the interrupt enable
    // register, so must deal with them on the raw status
    if (unlikely(raw_status & ((1UL << DMA_STATUS_GPI_BIT) |
							   (1UL << DMA_STATUS_GMI_BIT) |
						       (1UL << DMA_STATUS_GLI_BIT)))) {
        process_non_dma_ints(priv, raw_status);
    }

    // Get status of enabled interrupt sources
    status = raw_status & int_enable;

    while (status) {
        // Whether the link/PHY watchdog timer should be restarted
        int restart_watchdog = 0;
        int restart_tx       = 0;
        int poll_tx          = 0;
        u32 int_disable_mask = 0;

        // Test for RX interrupt resulting from sucessful reception of a packet-
        // must do this before ack'ing, else otherwise can get into trouble with
        // the sticky summary bits when we try to disable further RI interrupts
        if (status & (1UL << DMA_STATUS_RI_BIT)) {
//printk(KERN_INFO "RI ");
            // Disable interrupts caused by received packets as henceforth
            // we shall poll for packet reception
            int_disable_mask |= (1UL << DMA_INT_ENABLE_RI_BIT);

            // Do NAPI compatible receive processing for RI interrupts
            if (likely(napi_schedule_prep(&priv->napi_struct))) {
                // Remember that we are polling, so we ignore RX events for the
                // remainder of the ISR
                rx_polling = 1;

                // Tell system we have work to be done
                __napi_schedule(&priv->napi_struct);
            } else {
                printk(KERN_ERR "int_handler() %s: RX interrupt while in poll\n", dev->name);
            }
        }

        // Test for unavailable RX buffers - must do this before ack'ing, else
        // otherwise can get into trouble with the sticky summary bits
        if (unlikely(status & (1UL << DMA_STATUS_RU_BIT))) {
            DBG(30, KERN_INFO "int_handler() %s: RX buffer unavailable\n", dev->name);
            // Accumulate receive statistics
            ++priv->stats.rx_over_errors;
            ++priv->stats.rx_errors;

            // Disable RX buffer unavailable reporting, so we don't get swamped
            int_disable_mask |= (1UL << DMA_INT_ENABLE_RU_BIT);
        }

		if (unlikely(status & (1UL << DMA_STATUS_OVF_BIT))) {
			DBG(30, KERN_INFO "int_handler() %s: RX overflow\n", dev->name);
			// Accumulate receive statistics
			++priv->stats.rx_fifo_errors;
			++priv->stats.rx_errors;

            // Disable RX overflow reporting, so we don't get swamped
            int_disable_mask |= (1UL << DMA_INT_ENABLE_OV_BIT);
		}

        // Do any interrupt disabling with a single register write
        if (int_disable_mask) {
            gmac_int_en_clr(priv, int_disable_mask, 0, 1);

            // Update our record of the current interrupt enable status
            int_enable &= ~int_disable_mask;
        }

        // The broken GMAC interrupt mechanism with its sticky summary bits
        // means that we have to ack all asserted interrupts here; we can't not
        // ack the RI interrupt source as we might like to (in order that the
        // poll() routine could examine the status) because if it was asserted
        // prior to being masked above, then the summary bit(s) would remain
        // asserted and cause an immediate re-interrupt.
        dma_reg_write(priv, DMA_STATUS_REG, status | ((1UL << DMA_STATUS_NIS_BIT) |
                                                      (1UL << DMA_STATUS_AIS_BIT)));

        // Test for normal TX interrupt
        if (status & ((1UL << DMA_STATUS_TI_BIT) |
                      (1UL << DMA_STATUS_ETI_BIT))) {
            //printk(KERN_INFO "Normal complete IRQ\n");

            // Finish packet transmision started by start_xmit
            priv->finish_xmit(dev);
        }

        // Test for abnormal transmitter interrupt where there may be completed
        // packets waiting to be processed
        if (unlikely(status & ((1UL << DMA_STATUS_TJT_BIT) |
                               (1UL << DMA_STATUS_UNF_BIT)))) {
            //printk(KERN_INFO "Abnormal complete IRQ\n");
            // Complete processing of any TX packets closed by the DMA
            priv->finish_xmit(dev);

            if (status & (1UL << DMA_STATUS_TJT_BIT)) {
                // A transmit jabber timeout causes the transmitter to enter the
                // stopped state
                DBG(50, KERN_INFO "int_handler() %s: TX jabber timeout\n", dev->name);
                restart_tx = 1;
            } else {
                DBG(51, KERN_INFO "int_handler() %s: TX underflow\n", dev->name);
            }

            // Issue a TX poll demand in an attempt to restart TX descriptor
            // processing
            poll_tx = 1;
        }

        // Test for any of the error states which we deal with directly within
        // this interrupt service routine.
        if (unlikely(status & ((1UL << DMA_STATUS_ERI_BIT) |
                               (1UL << DMA_STATUS_RWT_BIT) |
                               (1UL << DMA_STATUS_RPS_BIT) |
                               (1UL << DMA_STATUS_TPS_BIT) |
                               (1UL << DMA_STATUS_FBE_BIT)))) {
            // Test for early RX interrupt
            if (status & (1UL << DMA_STATUS_ERI_BIT)) {
                // Don't expect to see this, as never enable it
                DBG(30, KERN_WARNING "int_handler() %s: Early RX \n", dev->name);
            }

            if (status & (1UL << DMA_STATUS_RWT_BIT)) {
                DBG(30, KERN_INFO "int_handler() %s: RX watchdog timeout\n", dev->name);
                // Accumulate receive statistics
				DBG(20, KERN_INFO "int_handler() RWT seen\n");
                ++priv->stats.rx_frame_errors;
                ++priv->stats.rx_errors;
                restart_watchdog = 1;
            }

            if (status & (1UL << DMA_STATUS_RPS_BIT)) {
				// Mask to extract the receive status field from the status register
//				u32 rs_mask = ((1UL << DMA_STATUS_RS_NUM_BITS) - 1) << DMA_STATUS_RS_BIT;
//				u32 rs = (status & rs_mask) >> DMA_STATUS_RS_BIT;
//				DBG(30, KERN_INFO "int_handler() %s: RX process stopped 0x%x\n", dev->name, rs);
				++priv->stats.rx_errors;
				restart_watchdog = 1;

                // Restart the receiver
                DBG(35, KERN_INFO "int_handler() %s: Restarting receiver\n", dev->name);
                dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_SR_BIT));
            }

            if (status & (1UL << DMA_STATUS_TPS_BIT)) {
				// Mask to extract the transmit status field from the status register
//				u32 ts_mask = ((1UL << DMA_STATUS_TS_NUM_BITS) - 1) << DMA_STATUS_TS_BIT;
//				u32 ts = (status & ts_mask) >> DMA_STATUS_TS_BIT;
//				DBG(30, KERN_INFO "int_handler() %s: TX process stopped 0x%x\n", dev->name, ts);
                ++priv->stats.tx_errors;
                restart_watchdog = 1;
                restart_tx = 1;
            }

            // Test for pure error interrupts
            if (status & (1UL << DMA_STATUS_FBE_BIT)) {
				// Mask to extract the bus error status field from the status register
//				u32 eb_mask = ((1UL << DMA_STATUS_EB_NUM_BITS) - 1) << DMA_STATUS_EB_BIT;
//				u32 eb = (status & eb_mask) >> DMA_STATUS_EB_BIT;
//				DBG(30, KERN_INFO "int_handler() %s: Bus error 0x%x\n", dev->name, eb);
                restart_watchdog = 1;
            }

            if (restart_watchdog) {
                // Restart the link/PHY state watchdog immediately, which will
                // attempt to restart the system
                mod_timer(&priv->watchdog_timer, jiffies);
                restart_watchdog = 0;
            }
        }

        if (unlikely(restart_tx)) {
            // Restart the transmitter
            DBG(35, KERN_INFO "int_handler() %s: Restarting transmitter\n", dev->name);
            dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));
        }

        if (unlikely(poll_tx)) {
            // Issue a TX poll demand in an attempt to restart TX descriptor
            // processing
            DBG(33, KERN_INFO "int_handler() %s: Issuing Tx poll demand\n", dev->name);
            dma_reg_write(priv, DMA_TX_POLL_REG, 0);
        }

        // Read the record of current interrupt requests again, in case some
        // more arrived while we were processing
        raw_status = dma_reg_read(priv, DMA_STATUS_REG);

        // MMC, PMT and GLI interrupts are not masked by the interrupt enable
        // register, so must deal with them on the raw status
        if (unlikely(raw_status & ((1UL << DMA_STATUS_GPI_BIT) |
                                   (1UL << DMA_STATUS_GMI_BIT) |
                                   (1UL << DMA_STATUS_GLI_BIT))))  {
            process_non_dma_ints(priv, raw_status);
        }

        // Get status of enabled interrupt sources.
        status = raw_status & int_enable;
    }

    return IRQ_HANDLED;
}

static void copro_stop_callback(
	gmac_priv_t					*priv,
	volatile gmac_cmd_que_ent_t	*entry)
{
    up(&priv->copro_stop_semaphore_);
}

static void gmac_down(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int desc;
    u32 int_enable;
    tx_que_t *tx_queue = &priv->tx_queue_;
    int cmd_queue_result;
    unsigned long irq_flags = 0;

	if (priv->napi_enabled) {
		// Stop NAPI
		napi_disable(&priv->napi_struct);
		priv->napi_enabled = 0;
	}

    // Stop further TX packets being delivered to hard_start_xmit();
//printk(KERN_INFO "gmac_down() calling netif_stop_queue()\n");
    netif_stop_queue(dev);
    netif_carrier_off(dev);

	if (priv->copro_started) {
		// Disable all GMAC interrupts and wait for change to be acknowledged
		gmac_copro_int_en_clr(priv, ~0UL, &int_enable);

		// Tell the CoPro to stop network offload operations
		cmd_queue_result = -1;
		while (cmd_queue_result) {
			spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
			cmd_queue_result =
				cmd_que_queue_cmd(priv, GMAC_CMD_STOP, 0, copro_stop_callback);
			spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
		}

		// Interrupt the CoPro so it sees the new command
		writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);

		// Wait until the CoPro acknowledges the STOP command
		while(down_interruptible(&priv->copro_stop_semaphore_));

		// Wait until the CoPro acknowledges that it has completed stopping
		while(down_interruptible(&priv->copro_stop_complete_semaphore_));
		priv->copro_started = 0;
	}

	if (copro_active(priv)) {
		// Clear out the Tx offload job queue, deallocating associated resources
		while (tx_que_not_empty(tx_queue)) {
			tx_que_inc_r_ptr(tx_queue);
		}

		// Reinitialise the Tx offload queue metadata
		tx_que_init(
			tx_queue,
			(gmac_tx_que_ent_t*)descriptors_phys_to_virt(priv->copro_params_.tx_que_head_),
			priv->copro_tx_que_num_entries_);
	} else {
		// Disable all GMAC interrupts
		gmac_int_en_clr(priv, ~0UL, 0, 0);

		// Stop transmitter, take ownership of all tx descriptors
		dma_reg_clear_mask(priv, DMA_OP_MODE_REG, 1UL << DMA_OP_MODE_ST_BIT);
		if (priv->desc_vaddr) {
			tx_take_ownership(&priv->tx_gmac_desc_list_info);
		}
	}

    // Stop receiver, waiting until it's really stopped and then take ownership
    // of all rx descriptors
    change_rx_enable(priv, 0, 1, 0);

    if (priv->desc_vaddr) {
        rx_take_ownership(&priv->rx_gmac_desc_list_info);
    }

    // Stop all timers
    delete_watchdog_timer(priv);

    if (priv->desc_vaddr) {
        // Free receive descriptors
        do {
            int first_last = 0;
            rx_frag_info_t frag_info;

            desc = get_rx_descriptor(priv, &first_last, 0, &frag_info);
            if (desc >= 0) {
				if (likely(priv->rx_buffers_per_page_)) {
					// If this is the last packet in the page, release the DMA
					// mapping and the page itself
					unmap_rx_page(priv, frag_info.phys_adr);
					put_page(frag_info.page);

					// If there was a preallocated skb associated with this
					// descriptor then free it
					if (frag_info.arg) {
						dev_kfree_skb((struct sk_buff *)frag_info.arg);
					}
				} else {
                    // Release the DMA mapping for the packet buffer
                    dma_unmap_single(0, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);

                    // Free the skb
                    dev_kfree_skb((struct sk_buff *)frag_info.page);
				}
            }
        } while (desc >= 0);

		if (!copro_active(priv)) {
			// Free transmit descriptors
			do {
				struct sk_buff *skb;
				tx_frag_info_t  frag_info;
				int             buffer_owned;

				desc = get_tx_descriptor(priv, &skb, 0, &frag_info, &buffer_owned);
				if (desc >= 0) {
					if (buffer_owned) {
						// Release the DMA mapping for the packet buffer
						dma_unmap_single(0, frag_info.phys_adr, frag_info.length, DMA_FROM_DEVICE);
					}

					if (skb) {
						// Free the skb
						dev_kfree_skb(skb);
					}
				}
			} while (desc >= 0);

			// Free any resources associated with the buffers of a pending packet
			if (priv->tx_pending_fragment_count) {
				tx_frag_info_t *frag_info = priv->tx_pending_fragments;

				while (priv->tx_pending_fragment_count--) {
					dma_unmap_single(0, frag_info->phys_adr, frag_info->length, DMA_FROM_DEVICE);
					++frag_info;
				}
			}

			// Free the socket buffer of a pending packet
			if (priv->tx_pending_skb) {
				dev_kfree_skb(priv->tx_pending_skb);
				priv->tx_pending_skb = 0;
			}
		}
    }

#if (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
    // Power down the PHY, but only if a dual GMAC SoC doesn't depend on clks
	// from one PHY to allow the other MAC to function, e.g. as does the 82x
    phy_powerdown(dev);
#endif // (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
}

static int stop(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    gmac_down(dev);

    if (copro_active(priv)) {
        // Ensure the Leon is held in reset so it can't cause any unexpected interrupts
#ifdef CONFIG_SUPPORT_LEON
        shutdown_copro();
#endif // CONFIG_SUPPORT_LEON

        // Disable all semaphore register bits so ARM cannot be interrupted
        *((volatile unsigned long*)SYS_CTRL_SEMA_MASKA_CTRL) = 0;
        *((volatile unsigned long*)SYS_CTRL_SEMA_MASKB_CTRL) = 0;

        if (priv->shared_copro_params_) {
            // Free the DMA coherent parameter space
printk(KERN_INFO "Freeing CoPro parameters %u bytes\n", sizeof(copro_params_t));
            dma_free_coherent(0, sizeof(copro_params_t), priv->shared_copro_params_, priv->shared_copro_params_pa_);
            priv->shared_copro_params_ = 0;
        }

        // Release interrupts lines used by semaphore register interrupts
        if (priv->copro_irq_alloced_) {
            free_irq(priv->copro_irq_offload_, dev);
            free_irq(priv->copro_irq_fwd_, dev);
            priv->copro_irq_alloced_ = 0;
        }
#if defined(CONFIG_ARCH_OX820)
    } else if (netoe_active(priv)) {
        // Halt the network offload engine and release the resources that it holds..
        netoe_stop(priv);    
#endif
	} else {
		// Free the shadow Tx descriptor memory
		kfree(priv->tx_desc_shadow_);
		priv->tx_desc_shadow_ = 0;
	}

	// Free the shadow Rx descriptor memory
	kfree(priv->rx_desc_shadow_);
	priv->rx_desc_shadow_ = 0;

#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
	// Free coherent DDR memory allocated for ARM descriptors
	if (priv->desc_vaddr) {
printk(KERN_INFO "Freeing ARM descs %u bytes\n", (priv->num_tx_descriptors + priv->num_rx_descriptors) * sizeof(gmac_dma_desc_t));
		dma_free_coherent(0,
			(priv->num_tx_descriptors + priv->num_rx_descriptors) * sizeof(gmac_dma_desc_t),
			priv->desc_vaddr, priv->desc_dma_addr);
		priv->desc_vaddr = 0;
	}

#ifndef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
    if (copro_active(priv)) {
		if (priv->copro_tx_desc_vaddr) {
printk(KERN_INFO "Freeing CoPro descs %u bytes\n", priv->copro_num_tx_descs * sizeof(gmac_dma_desc_t));
			dma_free_coherent(0,
				priv->copro_num_tx_descs * sizeof(gmac_dma_desc_t),
				priv->copro_tx_desc_vaddr, priv->copro_tx_desc_paddr);
			priv->copro_tx_desc_vaddr = 0;
		}
	}
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS

    // Release the IRQ
    if (priv->have_irq) {
        free_irq(dev->irq, dev);
        priv->have_irq = 0;
    }

#if (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
    // Disable the clock to the MAC block and put it into reset
    writel(1UL << mac_clken_bit[priv->unit], SYS_CTRL_CKEN_CLR_CTRL);
    writel(1UL << mac_reset_bit[priv->unit], SYS_CTRL_RSTEN_SET_CTRL);
#endif // (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)

	priv->interface_up = 0;
    return 0;
}

static void hw_set_mac_address(struct net_device *dev, unsigned char* addr)
{
    u32 mac_lo;
    u32 mac_hi;

    mac_lo  =  (u32)addr[0];
    mac_lo |= ((u32)addr[1] << 8);
    mac_lo |= ((u32)addr[2] << 16);
    mac_lo |= ((u32)addr[3] << 24);

    mac_hi  =  (u32)addr[4];
    mac_hi |= ((u32)addr[5] << 8);

    mac_reg_write(netdev_priv(dev), MAC_ADR0_HIGH_REG, mac_hi);
    mac_reg_write(netdev_priv(dev), MAC_ADR0_LOW_REG, mac_lo);
}

static int set_mac_address(struct net_device *dev, void *p)
{
    struct sockaddr *addr = p;

    if (!is_valid_ether_addr(addr->sa_data)) {
        return -EADDRNOTAVAIL;
    }

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    hw_set_mac_address(dev, addr->sa_data);

    return 0;
}
/*
static void multicast_hash(struct dev_mc_list *dmi, u32 *hash_lo, u32 *hash_hi)
{
    u32 crc = ether_crc_le(dmi->dmi_addrlen, dmi->dmi_addr);
    u32 mask = 1 << ((crc >> 26) & 0x1F);

    if (crc >> 31) {
        *hash_hi |= mask;
    } else {
        *hash_lo |= mask;
    }
}*/

static void set_multicast_list(struct net_device *dev)
{
    gmac_priv_t* priv = netdev_priv(dev);
    u32 hash_lo=0;
    u32 hash_hi=0;
    u32 mode = 0;
    int i = 0;
    struct netdev_hw_addr *ha;
    
    // Disable promiscuous mode and uni/multi-cast matching
    mac_reg_write(priv, MAC_FRAME_FILTER_REG, mode);

    // Disable all perfect match registers
    for (i=0; i < NUM_PERFECT_MATCH_REGISTERS; ++i) {
        mac_adrhi_reg_write(priv, i, 0);
    }

    // Promiscuous mode overrides all-multi which overrides other filtering
    if (dev->flags & IFF_PROMISC) {
        mode |= (1 << MAC_FRAME_FILTER_PR_BIT);
    } else if (dev->flags & IFF_ALLMULTI) {
        mode |= (1 << MAC_FRAME_FILTER_PM_BIT);
    } else {

        if (netdev_mc_count(dev) <= NUM_PERFECT_MATCH_REGISTERS) {
            // Use perfect matching registers
            netdev_for_each_mc_addr(ha, dev) {
                u32 addr;

                // Note: write to hi register first. The MAC address registers are
                // double-synchronised to the GMII clock domain, and sync is only
                // triggered on the write to the low address register.
                addr  =      ha->addr[4];
                addr |= (u32)ha->addr[5] << 8;
                addr |= (1 << MAC_ADR1_HIGH_AE_BIT);
                mac_adrhi_reg_write(priv, i, addr);

                addr  =      ha->addr[0];
                addr |= (u32)ha->addr[1] << 8;
                addr |= (u32)ha->addr[2] << 16;
                addr |= (u32)ha->addr[3] << 24;
                mac_adrlo_reg_write(priv, i, addr);
		i++;
            }
        } /*else {
            // Use hashing
            mode |= (1 << MAC_FRAME_FILTER_HUC_BIT);
            mode |= (1 << MAC_FRAME_FILTER_HMC_BIT);

            for (dmi = dev->mc_list; dmi; dmi = dmi->next) {
                //multicast_hash(dmi, &hash_lo, &hash_hi);
            }
        }*/
    }

    // Update the filtering rules
    mac_reg_write(priv, MAC_FRAME_FILTER_REG, mode);

    // Update the filtering hash table
    mac_reg_write(priv, MAC_HASH_HIGH_REG, hash_hi);
    mac_reg_write(priv, MAC_HASH_LOW_REG,  hash_lo);

}

static int gmac_reset(struct net_device *dev)
{
	unsigned long flags;
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    int status = 0;

	// Perform any actions required before GMAC reset
	do_pre_reset_actions(priv);

	// Prevent use of the MDIO bus attached to this GMAC while we are busy
	// resetting the GMAC
	lock_mdio_bus(priv, flags);

    // Reset the entire GMAC
    dma_reg_write(priv, DMA_BUS_MODE_REG, 1UL << DMA_BUS_MODE_SWR_BIT);

    // Ensure reset is performed before testing for completion
    wmb();

    // Wait for the reset operation to complete
    status = -EIO;
	printk(KERN_INFO "%s: Resetting GMAC\n", dev->name);
    for (;;) {
        if (!(dma_reg_read(priv, DMA_BUS_MODE_REG) & (1UL << DMA_BUS_MODE_SWR_BIT))) {
            status = 0;
            break;
        }
    }

#if defined(CONFIG_ARCH_OX820)
	// Disable all GMAC (as opposed to GMAC DMA) interrupts as unbelievably they
	// are enabled by default from reset!
    mac_reg_write(priv, MAC_INT_MASK_REG, ~0UL);
#endif // CONFIG_ARCH_OX820

	// Now safe to allow MDIO accesses via this GMAC
	unlock_mdio_bus(priv, flags);

	// Perform any actions required after GMAC reset
	do_post_reset_actions(priv);

    // Did the GMAC reset operation fail?
    if (status) {
        printk(KERN_ERR "%s: GMAC reset failed\n", dev->name);
    }
	printk(KERN_INFO "%s: GMAC reset complete\n", dev->name);

    return status;    
}

static int gmac_up(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    u32 reg_contents;
    int cmd_queue_result;
    unsigned long irq_flags = 0;
    int status = 0;

    if ((status = gmac_reset(dev))) {
        goto gmac_up_err_out;
    }

	/* Initialise MAC config register contents
	 */
    reg_contents = 0;
    if (priv->mii.full_duplex) {
        reg_contents |= (1UL << MAC_CONFIG_DM_BIT);
    }

#ifdef USE_RX_CSUM
	reg_contents |= (1UL << MAC_CONFIG_IPC_BIT);
#endif // USE_RX_CSUM

    if (priv->jumbo_) {
		// Allow passage of jumbo frames through both transmitter and receiver
		reg_contents |=	((1UL << MAC_CONFIG_JE_BIT) |
                         (1UL << MAC_CONFIG_JD_BIT) |
						 (1UL << MAC_CONFIG_WD_BIT));
	}

	// Select the minimum IFG - I found that 80 bit times caused very poor
	// IOZone performance, so stick with the 96 bit times default
	reg_contents |= (0UL << MAC_CONFIG_IFG_BIT);

    // Write MAC config setup to the GMAC
    mac_reg_write(priv, MAC_CONFIG_REG, reg_contents);

	// Update MAC config setup with the link speed
	configure_for_link_speed(priv);

	// Enable transmitter and receiver
	mac_reg_set_mask(priv, MAC_CONFIG_REG, (1UL << MAC_CONFIG_TE_BIT) | (1UL << MAC_CONFIG_RE_BIT));

	/* Initialise MAC VLAN register contents
	 */
    reg_contents = 0;
    mac_reg_write(priv, MAC_VLAN_TAG_REG, reg_contents);

    // Initialise the hardware's record of our primary MAC address
    hw_set_mac_address(dev, dev->dev_addr);

    // Initialise multicast and promiscuous modes
    set_multicast_list(dev);

    // Disable all MMC interrupt sources
    mac_reg_write(priv, MMC_RX_MASK_REG, ~0UL);
    mac_reg_write(priv, MMC_TX_MASK_REG, ~0UL);

	if (!offload_active(priv)) {
		// Initialise the structures managing the TX descriptor list
		init_tx_desc_list(&priv->tx_gmac_desc_list_info,
						  priv->desc_vaddr,
						  priv->tx_desc_shadow_,
						  priv->num_tx_descriptors);
	}

    // Initialise the structures managing the RX descriptor list
    init_rx_desc_list(&priv->rx_gmac_desc_list_info,
                      priv->desc_vaddr + priv->num_tx_descriptors,
                      priv->rx_desc_shadow_,
                      priv->num_rx_descriptors,
                      priv->rx_buffer_size_);

    // Reset record of pending Tx packet
    priv->tx_pending_skb = 0;
    priv->tx_pending_fragment_count = 0;

	if (!offload_active(priv)) {
		// Write the physical DMA consistent address of the start of the tx descriptor array
		dma_reg_write(priv, DMA_TX_DESC_ADR_REG, priv->desc_dma_addr);
    }
#if defined(CONFIG_ARCH_OX820)
	if (netoe_active(priv)){
        dma_reg_write(priv, DMA_TX_DESC_ADR_REG, NETOE_DESC_OFFSET);
    }
#endif

    // Write the physical DMA consistent address of the start of the rx descriptor array
    dma_reg_write(priv, DMA_RX_DESC_ADR_REG, priv->desc_dma_addr +
		(priv->num_tx_descriptors * sizeof(gmac_dma_desc_t)));

    // Initialise the GMAC DMA bus mode register
    dma_reg_write(priv, DMA_BUS_MODE_REG, ((1UL << DMA_BUS_MODE_FB_BIT)   |	// Force bursts
#if defined(CONFIG_ARCH_OX820)
                                           (16UL << DMA_BUS_MODE_PBL_BIT) |	// AHB burst size
#else // CONFIG_ARCH_OX820
                                           (8UL << DMA_BUS_MODE_PBL_BIT)  |	// AHB burst size
#endif // CONFIG_ARCH_OX820
                                           (1UL << DMA_BUS_MODE_DA_BIT)));	// Round robin Rx/Tx

    // Prepare receive descriptors
    refill_rx_ring(dev);

    // Clear any pending interrupt requests
    dma_reg_write(priv, DMA_STATUS_REG, dma_reg_read(priv, DMA_STATUS_REG));

	/* Initialise flow control register contents
	 */
	// Enable Rx flow control
    reg_contents = (1UL << MAC_FLOW_CNTL_RFE_BIT);

	/*if (priv->mii.using_pause) {*/
		// Enable Tx flow control
	//	reg_contents |= (1UL << MAC_FLOW_CNTL_TFE_BIT);
	/*}*/

    // Set the duration of the pause frames generated by the transmitter when
	// the Rx fifo fill threshold is exceeded
	reg_contents |= ((0x100UL << MAC_FLOW_CNTL_PT_BIT) |	// Pause for 256 slots
					 (0x1UL << MAC_FLOW_CNTL_PLT_BIT));

	// Write flow control setup to the GMAC
    mac_reg_write(priv, MAC_FLOW_CNTL_REG, reg_contents);

	/* Initialise operation mode register contents
	 */
    // Initialise the GMAC DMA operation mode register. Set Tx/Rx FIFO thresholds
    // to make best use of our limited SDRAM bandwidth when operating in gigabit
	reg_contents = ((DMA_OP_MODE_TTC_256 << DMA_OP_MODE_TTC_BIT) |	// Tx threshold
                    (1UL << DMA_OP_MODE_FUF_BIT) |    				// Forward Undersized good Frames
                    (DMA_OP_MODE_RTC_128 << DMA_OP_MODE_RTC_BIT) |	// Rx threshold 128 bytes
                    (1UL << DMA_OP_MODE_OSF_BIT));					// Operate on 2nd frame

	// Enable hardware flow control
	reg_contents |= (1UL << DMA_OP_MODE_EFC_BIT);

	switch (priv->unit) {
		/* 810 has 32KB Rx FIFO, 820 port 0 has 16KB while 820 port 1 has 8KB
		 */
		case 0:
			printk(KERN_INFO "%s: Setting Rx flow control thresholds for LAN port\n", dev->name);

			// Set threshold for enabling hardware flow control at (full-4KB) to
			// give space for upto two in-flight std MTU packets to arrive after
			// pause frame has been sent.
			reg_contents |= ((0UL << DMA_OP_MODE_RFA2_BIT) |
							 (3UL << DMA_OP_MODE_RFA_BIT));

			// Set threshold for disabling hardware flow control (-7KB)
			reg_contents |= ((1UL << DMA_OP_MODE_RFD2_BIT) |
							 (2UL << DMA_OP_MODE_RFD_BIT));
			break;
		case 1:
			printk(KERN_INFO "%s: Setting Rx flow control thresholds for WAN port\n", dev->name);

			// Allow for a bit more than one in-flight 1500 byte MTU packet to
			// arrive after flow control is enabled (full-2KB)
			reg_contents |= ((0UL << DMA_OP_MODE_RFA2_BIT) |
							 (1UL << DMA_OP_MODE_RFA_BIT));

			// Disable flow control when there's nearly enough space for two
			// 1500 byte MTU packets before flow control would be enabled
			// (full-5KB)
			reg_contents |= ((1UL << DMA_OP_MODE_RFD2_BIT) |
							 (0UL << DMA_OP_MODE_RFD_BIT));
			break;
		default:
			BUG();
	}

    // Don't flush Rx frames from FIFO just because there's no descriptor available
	reg_contents |= (1UL << DMA_OP_MODE_DFF_BIT);

	// Write settings to operation mode register
    dma_reg_write(priv, DMA_OP_MODE_REG, reg_contents);

    // GMAC requires store&forward in order to compute Tx checksums
    dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_SF_BIT));

    // Ensure setup is complete, before enabling TX and RX
    wmb();
    
	if (copro_active(priv)) {
		// Update the CoPro's parameters with the current MTU
		priv->copro_params_.mtu_ = dev->mtu;

		// Only attempt to write to uncached/unbuffered shared parameter storage if
		// CoPro is started and thus storage has been allocated
		if (priv->shared_copro_params_) {
			// Fill the CoPro parameter block
			memcpy(priv->shared_copro_params_, &priv->copro_params_, sizeof(copro_params_t));
		}

		// Make sure the CoPro parameter block updates have made it to memory (which
		// is uncached/unbuffered, so just compiler issues to overcome)
		wmb();

		// Tell the CoPro to re-read parameters
		cmd_queue_result = -1;

		while (cmd_queue_result) {
			spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
			cmd_queue_result = cmd_que_queue_cmd(priv, GMAC_CMD_UPDATE_PARAMS, 0,
												 copro_update_callback);
			spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
		}

		// Interrupt the CoPro so it sees the new command
		writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);

		// Wait until the CoPro acknowledges that the update of parameters is complete
		while(down_interruptible(&priv->copro_update_semaphore_));

		// Tell the CoPro to begin network offload operations
		cmd_queue_result = -1;
		while (cmd_queue_result) {
			spin_lock_irqsave(&priv->cmd_que_lock_, irq_flags);
			cmd_queue_result = cmd_que_queue_cmd(priv, GMAC_CMD_START, 0,
												 copro_start_callback);
			spin_unlock_irqrestore(&priv->cmd_que_lock_, irq_flags);
		}

		// Interrupt the CoPro so it sees the new command
		writel(1UL << COPRO_SEM_INT_CMD, SYS_CTRL_SEMA_SET_CTRL);

		// Wait until the CoPro acknowledges that it has started
		while(down_interruptible(&priv->copro_start_semaphore_));

		priv->copro_started = 1;
	}

#if defined(CONFIG_ARCH_OX820)
    if (netoe_active(priv)) {
        // Update the MTU in the offload engine
        netoe_set_mtu(priv, dev->mtu);            
    }
#endif

	// Start NAPI
	BUG_ON(priv->napi_enabled);
	napi_enable(&priv->napi_struct);
	priv->napi_enabled = 1;

	if (!copro_active(priv)) {
		// Start the transmitter
		dma_reg_set_mask(priv, DMA_OP_MODE_REG, (1UL << DMA_OP_MODE_ST_BIT));
	}

	// Start the receiver
    change_rx_enable(priv, 1, 0, 0);

    // Enable interesting GMAC interrupts
    gmac_int_en_set(priv, ((1UL << DMA_INT_ENABLE_NI_BIT)  |
                           (1UL << DMA_INT_ENABLE_AI_BIT)  |
                           (1UL << DMA_INT_ENABLE_FBE_BIT) |
                           (1UL << DMA_INT_ENABLE_RI_BIT)  |
                           (1UL << DMA_INT_ENABLE_RU_BIT)  |
                           (1UL << DMA_INT_ENABLE_OV_BIT)  |
                           (1UL << DMA_INT_ENABLE_RW_BIT)  |
                           (1UL << DMA_INT_ENABLE_RS_BIT)  |
                           (1UL << DMA_INT_ENABLE_TI_BIT)  |
                           (1UL << DMA_INT_ENABLE_UN_BIT)  |
                           (1UL << DMA_INT_ENABLE_TJ_BIT)  |
                           (1UL << DMA_INT_ENABLE_TS_BIT)));

    // (Re)start the link/PHY state monitoring timer
    start_watchdog_timer(priv);

    // Allow the network stack to call hard_start_xmit()
    netif_start_queue(dev);

#ifdef DUMP_REGS_ON_GMAC_UP
    dump_mac_regs(priv->unit, priv->macBase, priv->dmaBase);
#endif // DUMP_REGS_ON_GMAC_UP

    return status;

gmac_up_err_out:
    stop(dev);

    return status;
}

static void set_rx_packet_info(struct net_device *dev)
{
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
#ifndef CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
	int max_packet_buffer_size = dev->mtu + EXTRA_RX_SKB_SPACE;

	if (max_packet_buffer_size <= MAX_DESCRIPTOR_LENGTH) {
		priv->rx_buffer_size_ = max_packet_buffer_size;
		priv->rx_buffers_per_page_ = 0;
	} else {
#endif // CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
		BUG_ON(CONFIG_OXNAS_RX_BUFFER_SIZE > MAX_PACKET_FRAGMENT_LENGTH);
		priv->rx_buffer_size_ = CONFIG_OXNAS_RX_BUFFER_SIZE;
		priv->bytes_consumed_per_rx_buffer_ = SKB_DATA_ALIGN(NET_IP_ALIGN + priv->rx_buffer_size_);
		priv->rx_buffers_per_page_ = PAGE_SIZE / priv->bytes_consumed_per_rx_buffer_;
#ifndef CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
	}
#endif // CONFIG_OXNAS_ZERO_COPY_RX_SUPPORT
}

#define SEM_INT_FWD      8
#define SEM_INT_ACK      16
#define SEM_INT_TX       17
#define SEM_INT_STOP_ACK 18

#define SEM_OFFLOAD_MASK ((1UL << SEM_INT_ACK) |\
						  (1UL << SEM_INT_TX) |\
						  (1UL << SEM_INT_STOP_ACK))

#define SEM_FWD_MASK (1UL << SEM_INT_FWD)

static irqreturn_t copro_offload_intr(int irq, void *dev_id)
{
    struct net_device *dev = (struct net_device *)dev_id;
    gmac_priv_t       *priv = (gmac_priv_t*)netdev_priv(dev);
    u32                asserted;

    // See what offload interrupts are asserted
    asserted = *((volatile unsigned long*)SYS_CTRL_SEMA_STAT) & SEM_OFFLOAD_MASK;

	// Continue processing interrupts until none asserted
    while (asserted) {
        // Clear any offload interrupts directed at the ARM
        *((volatile unsigned long*)SYS_CTRL_SEMA_CLR_CTRL) = asserted;

        // Process any outstanding command acknowledgements
        if (asserted & (1UL << SEM_INT_ACK)) {
            while (!cmd_que_dequeue_ack(&priv->cmd_queue_));
        }

        // Process STOP completion signal
        if (asserted & (1UL << SEM_INT_STOP_ACK)) {
            up(&priv->copro_stop_complete_semaphore_);
        }

        // Process any completed TX offload jobs
        if (asserted & (1UL << SEM_INT_TX)) {
            copro_finish_xmit(dev);
        }

        // Stay in interrupt routine if interrupts have been re-asserted
        asserted = *((volatile unsigned long*)SYS_CTRL_SEMA_STAT) & SEM_OFFLOAD_MASK;
    }

    return IRQ_HANDLED;
}

static irqreturn_t copro_fwd_intr(int irq, void *dev_id)
{
    struct net_device *dev = (struct net_device *)dev_id;
    gmac_priv_t       *priv = (gmac_priv_t*)netdev_priv(dev);

	// Read the currently asserted semaphore interrupts
    u32 asserted = *((volatile unsigned long*)SYS_CTRL_SEMA_STAT) & SEM_FWD_MASK;

	// Continue processing interrupts until none asserted
	while (asserted) {
		// Read the forwarded interrupt status from the mailbox
		u32 fwd_intrs_status = ((volatile gmac_fwd_intrs_t*)descriptors_phys_to_virt(priv->copro_params_.fwd_intrs_mailbox_))->status_;

		// Ack the forwarded interrupt status interrupt
		*((volatile unsigned long*)SYS_CTRL_SEMA_CLR_CTRL) = asserted;

		// Process forwarded interrupts
		copro_fwd_intrs_handler(dev_id, fwd_intrs_status);

		// Check whether we are being re-interrupted
		asserted = *((volatile unsigned long*)SYS_CTRL_SEMA_STAT) & SEM_FWD_MASK;
	}

    return IRQ_HANDLED;
}

#ifdef USE_TX_TIMEOUT
static void reset(struct work_struct *work)
{
	gmac_priv_t *priv = container_of(work, gmac_priv_t, tx_timeout_work);
	struct net_device* dev = priv->netdev;
	int queued_stopped = netif_queue_stopped(dev);

	printk(KERN_WARNING "reset() %s: Queue state '%s'\n", dev->name,
		queued_stopped ? "stopped" : "awake");

	WARN_ON(!queued_stopped);

#ifdef DUMP_REGS_ON_RESET
	dump_mac_regs(priv->unit, priv->macBase, priv->dmaBase);

	if (copro_active(priv)) {
#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
#ifndef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
		dump_leon_tx_descs(priv);
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
		dump_leon_queues(priv);
	}
#endif // DUMP_REGS_ON_RESET

	if (priv->interface_up
#if defined(CONFIG_ARCH_OX820)
		/**
		 * NB 820 NETOE hardware fails if we issue GMAC/PHY reset, so need a
		 * different solution to that for 810
		 */
		&& !netoe_active(priv)
#endif
		) {
		printk(KERN_WARNING "reset() %s: Resetting GMAC/PHY\n", dev->name);

		gmac_down(dev);

		// Reset the PHY to get it into a known state and ensure we have TX/RX
		// clocks to allow the GMAC reset to complete
		if (phy_reset(priv->netdev)) {
			printk(KERN_ERR "reset() %s: Failed to reset PHY\n", dev->name);
		} else {
			// Set PHY specfic features
			initialise_phy(priv);

			// Force or auto-negotiate PHY mode
			priv->phy_force_negotiation = 1;

			gmac_up(dev);
		}
	}
}
#endif // USE_TX_TIMEOUT

static struct kset *link_state_kset;

static int open(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int status;
    const struct firmware* firmware = NULL;
	unsigned long flags;

	// Prevent use of the MDIO bus attached to this GMAC while we are busy
	// resetting the GMAC hardware block
	lock_mdio_bus(priv, flags);

    // Ensure the MAC block is properly reset
    writel(1UL << mac_reset_bit[priv->unit], SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << mac_reset_bit[priv->unit], SYS_CTRL_RSTEN_CLR_CTRL);

    // Enable the clock to the MAC block
    writel(1UL << mac_clken_bit[priv->unit], SYS_CTRL_CKEN_SET_CTRL);

    // Ensure reset and clock operations are complete
    wmb();

	// Now safe to allow MDIO accesses via this GMAC
	unlock_mdio_bus(priv, flags);

#if defined(CONFIG_ARCH_OX820)
	// Disable all GMAC (as opposed to GMAC DMA) interrupts as unbelievably they
	// are enabled by default from reset!
    mac_reg_write(priv, MAC_INT_MASK_REG, ~0UL);
#endif // CONFIG_ARCH_OX820

    // Reset the PHY to get it into a known state and ensure we have TX/RX clocks
    // to allow the GMAC reset to complete
    if (phy_reset(priv->netdev)) {
        DBG(1, KERN_ERR "open() %s: Failed to reset PHY\n", dev->name);
        status = -EIO;
        goto open_err_out;
    }

	// Set PHY specfic features
	initialise_phy(priv);

    // Check that the MAC address is valid.  If it's not, refuse to bring the
    // device up
    if (!is_valid_ether_addr(dev->dev_addr)) {
        DBG(1, KERN_ERR "open() %s: MAC address invalid\n", dev->name);
        status = -EINVAL;
        goto open_err_out;
    }

    if (copro_active(priv)) {
        printk(KERN_INFO "CoPro offload is active on %s\n",dev->name);
        // Allocate the CoPro offload semaphore IRQ
        if (request_irq(priv->copro_irq_offload_, &copro_offload_intr, 0, "GMAC SEMAPHORE OFFLOAD", dev)) {
            panic("open: Failed to allocate GMAC %d CoPro semaphore interrupt %u\n", priv->unit, priv->copro_irq_offload_);
            status = -ENODEV;
            goto open_err_out;
        }
        if (request_irq(priv->copro_irq_fwd_, &copro_fwd_intr, 0, "GMAC SEMAPHORE FWD", dev)) {
            panic("open: Failed to allocate GMAC %d CoPro semaphore interrupt %u\n", priv->unit, priv->copro_irq_fwd_);
            free_irq(priv->copro_irq_offload_, dev);
            status = -ENODEV;
            goto open_err_out;
        }
        priv->copro_irq_alloced_ = 1;
#if defined(CONFIG_ARCH_OX820)
    } else if (netoe_active(priv)) {
        printk(KERN_INFO "NetOE offload is active on %s\n",dev->name);
        // Allocate the IRQ
        if (request_irq(dev->irq, &int_handler, 0, dev->name, dev)) {
            DBG(1, KERN_ERR "open() %s: Failed to allocate GMAC %d irq %d\n", dev->name, priv->unit, dev->irq);
            status = -ENODEV;
            goto open_err_out;
        }
        priv->have_irq = 1;
        priv->finish_xmit = netoe_finish_xmit;        
#endif
	} else {
        printk(KERN_INFO "Offload is not active on %s\n",dev->name);
		// Allocate the IRQ
		if (request_irq(dev->irq, &int_handler, 0, dev->name, dev)) {
			DBG(1, KERN_ERR "open() %s: Failed to allocate GMAC %d irq %d\n", dev->name, priv->unit, dev->irq);
			status = -ENODEV;
			goto open_err_out;
		}
		priv->have_irq = 1;
        priv->finish_xmit = finish_xmit;
	}

    // Need a consistent DMA mapping covering all the memory occupied by DMA
    // unified descriptor array, as both CPU and DMA engine will be reading and
    // writing descriptor fields.
#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
printk(KERN_INFO "Alloc'ing ARM descs %u bytes\n", (priv->num_tx_descriptors + priv->num_rx_descriptors) * sizeof(gmac_dma_desc_t));
	priv->desc_vaddr = dma_alloc_coherent(0,
		(priv->num_tx_descriptors + priv->num_rx_descriptors) * sizeof(gmac_dma_desc_t),
		&priv->desc_dma_addr, GFP_KERNEL);

	if (!priv->desc_vaddr) {
		printk(KERN_ERR "open() %s: Failed to allocate DMA coherent space for ARM DDR descriptors\n", dev->name);
		status = -ENOMEM;
		goto open_err_out;
	}

#ifndef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
    if (copro_active(priv)) {
printk(KERN_INFO "Alloc'ing CoPro descs %u bytes\n", priv->copro_num_tx_descs * sizeof(gmac_dma_desc_t));
		priv->copro_tx_desc_vaddr = dma_alloc_coherent(0,
			priv->copro_num_tx_descs * sizeof(gmac_dma_desc_t),
			&priv->copro_tx_desc_paddr, GFP_KERNEL);

		if (!priv->copro_tx_desc_vaddr) {
			printk(KERN_ERR "open() %s: Failed to allocate DMA coherent space for CoPro DDR descriptors\n", dev->name);
			status = -ENOMEM;
			goto open_err_out;
		}
	}
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#else // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
    priv->desc_vaddr =
		(gmac_dma_desc_t*)(priv->unit ? GMAC2_DESC_ALLOC_START : GMAC1_DESC_ALLOC_START);

    priv->desc_dma_addr =
		priv->unit ? GMAC2_DESC_ALLOC_START_PA : GMAC1_DESC_ALLOC_START_PA;
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS

    if (!priv->desc_vaddr) {
        DBG(1, KERN_ERR "open() %s: Failed to allocate consistent memory for DMA descriptors\n", dev->name);
        status = -ENOMEM;
        goto open_err_out;
    }

	if (!offload_active(priv)) {
		// Allocate memory to hold shadow Tx descriptors
		if (!(priv->tx_desc_shadow_ = kmalloc(priv->num_tx_descriptors * sizeof(gmac_dma_desc_t), GFP_KERNEL))) {
			DBG(1, KERN_ERR "open() %s: Failed to allocate memory for Tx descriptor shadows\n", dev->name);
			status = -ENOMEM;
			goto open_err_out;
		}
	}

	// Allocate memory to hold shadow Rx descriptors
	if (!(priv->rx_desc_shadow_ = kmalloc(priv->num_rx_descriptors * sizeof(gmac_dma_desc_t), GFP_KERNEL))) {
        DBG(1, KERN_ERR "open() %s: Failed to allocate memory for Rx descriptor shadows\n", dev->name);
        status = -ENOMEM;
        goto open_err_out;
	}

	// Record whether jumbo frames should be enabled
    priv->jumbo_ = (dev->mtu > NORMAL_PACKET_SIZE);

	set_rx_packet_info(dev);

	if (copro_active(priv)) {
		// Allocate SRAM for the command queue entries
		priv->copro_params_.cmd_que_head_ = AVAIL_SRAM_START_PA;

		priv->copro_params_.cmd_que_tail_ =
			(u32)((gmac_cmd_que_ent_t*)(priv->copro_params_.cmd_que_head_) + priv->copro_cmd_que_num_entries_);
		priv->copro_params_.fwd_intrs_mailbox_ = priv->copro_params_.cmd_que_tail_;
		priv->copro_params_.tx_que_head_ = priv->copro_params_.fwd_intrs_mailbox_ + sizeof(gmac_fwd_intrs_t);
		priv->copro_params_.tx_que_tail_ =
			(u32)((gmac_tx_que_ent_t*)(priv->copro_params_.tx_que_head_) + priv->copro_tx_que_num_entries_);
		priv->copro_params_.free_start_ = priv->copro_params_.tx_que_tail_;

		// Initialise command queue metadata
		cmd_que_init(
			&priv->cmd_queue_,
			(gmac_cmd_que_ent_t*)descriptors_phys_to_virt(priv->copro_params_.cmd_que_head_),
			priv->copro_cmd_que_num_entries_);

		// Initialise tx offload queue metadata
		tx_que_init(
			&priv->tx_queue_,
			(gmac_tx_que_ent_t*)descriptors_phys_to_virt(priv->copro_params_.tx_que_head_),
			priv->copro_tx_que_num_entries_);

		// Allocate DMA coherent space for the parameter block shared with the CoPro
		priv->shared_copro_params_ = dma_alloc_coherent(0, sizeof(copro_params_t), &priv->shared_copro_params_pa_, GFP_KERNEL);
printk(KERN_INFO "Alloc'ing CoPro parameters %u bytes\n", sizeof(copro_params_t));
		if (!priv->shared_copro_params_) {
			DBG(1, KERN_ERR "open() %s: Failed to allocate DMA coherent space for parameters\n", dev->name);
			status = -ENOMEM;
			goto open_err_out;
		}

		// Update the CoPro's parameters with the current MTU
		priv->copro_params_.mtu_ = dev->mtu;

#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
#ifdef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
		priv->copro_params_.tx_descs_base_ = 0;
#else // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
		priv->copro_params_.tx_descs_base_ = priv->copro_tx_desc_paddr;
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
		priv->copro_params_.num_tx_descs_ = priv->copro_num_tx_descs;

		// Fill the shared CoPro parameter block from the ARM's local copy
		memcpy(priv->shared_copro_params_, &priv->copro_params_, sizeof(copro_params_t));

		// Request CoPro firmware from userspace
		if (request_firmware(&firmware, "gmac_copro_firmware", priv->device)) {
			printk(KERN_ERR "open() %s: Failed to load CoPro firmware\n", dev->name);
			status = -EIO;
			goto open_err_out;
		}

		// Load CoPro program and start it running
#ifdef CONFIG_SUPPORT_LEON
		init_copro(firmware->data, priv->shared_copro_params_pa_);
#endif // CONFIG_SUPPORT_LEON

		// Finished with the firmware so release it
		release_firmware(firmware);

		// Enable selected semaphore register bits to cause ARM interrupts
		*((volatile unsigned long*)SYS_CTRL_SEMA_MASKA_CTRL) = (1UL << SEM_INT_FWD);
		*((volatile unsigned long*)SYS_CTRL_SEMA_MASKB_CTRL) = SEM_OFFLOAD_MASK;
	}

#if defined(CONFIG_ARCH_OX820)
    if (netoe_active(priv)) {
        // Get the network offload engine on its feet.
        netoe_start(priv);
        // Set the MTU in the offload engine
        netoe_set_mtu(priv, dev->mtu);            
    }
#endif

    // Initialise the work queue entry to be used to issue hotplug events to userspace
    INIT_WORK(&priv->link_state_change_work, work_handler);

#ifdef USE_TX_TIMEOUT
    // Initialise the work queue entry to be used when Tx times out
    INIT_WORK(&priv->tx_timeout_work, reset);
#endif // USE_TX_TIMEOUT

    // Do startup operations that are in common with gmac_down()/_up() processing
    priv->mii_init_media = 1;
    priv->phy_force_negotiation = 1;
    status = gmac_up(dev);
    if (status) {
        goto open_err_out;
    }

    // Allow some time for auto-negotiation to work
    msleep(3000);

	priv->interface_up = 1;
    return 0;

open_err_out:
    stop(dev);

    return status;
}

static int change_mtu(struct net_device *dev, int new_mtu)
{
    int status = 0;
    gmac_priv_t *priv = (gmac_priv_t*)netdev_priv(dev);
    int original_mtu = dev->mtu;

    printk(KERN_INFO "Attempting to set new mtu %d\n", new_mtu);

    // Check that new MTU is within supported range
    if ((new_mtu < MIN_PACKET_SIZE) || (new_mtu > MAX_JUMBO)) {
        DBG(1, KERN_WARNING "change_mtu() %s: Invalid MTU %d\n", dev->name, new_mtu);
        status = -EINVAL;
    } else if ((priv->interface_up)
#if defined(CONFIG_ARCH_OX820)
               // Don't issue a PHY/GMAC reset if the netoe is running. The subsequent
               // netoe has no software reset, and we will lose synchronisation between
               // the netoe's and gmac's versions of next tx descriptor.
               // See below for the netoe's change mtu implementation
               && (!netoe_active(priv))
#endif
        ) {
        // Put MAC/PHY into quiesent state, causing all current buffers to be
        // deallocated and the PHY to powerdown
        gmac_down(dev);

        // Record the new MTU, so bringing the MAC back up will allocate
        // resources to suit the new MTU
        dev->mtu = new_mtu;

		// Set length etc. of rx packets
		set_rx_packet_info(dev);

        // Reset the PHY to get it into a known state and ensure we have TX/RX
        // clocks to allow the GMAC reset to complete
        if (phy_reset(priv->netdev)) {
            DBG(1, KERN_ERR "change_mtu() %s: Failed to reset PHY\n", dev->name);
            status = -EIO;
        } else {
			// Set PHY specfic features
			initialise_phy(priv);

            // Record whether jumbo frames should be enabled
            priv->jumbo_ = (dev->mtu > NORMAL_PACKET_SIZE);

            // Force or auto-negotiate PHY mode
            priv->phy_force_negotiation = 1;

            // Reallocate buffers with new MTU
            gmac_up(dev);
        }
	} else {
        // Record the new MTU, so bringing the interface up will allocate
        // resources to suit the new MTU
        dev->mtu = new_mtu;
    }

#if defined(CONFIG_ARCH_OX820)
    if (netoe_active(priv)) {
        // The stop/set_mtu/open will ensure synchronisation of the tx
        // descriptor by issuing a core-level reset to the GMAC, which in turn
        // resets the netoe.
        // We need some form of reset cycle to handle the buffer close-down and
        // recreation. This is important for cases of one buffer per packet (i.e.
        // mtu less than 2025) when the receive skb is set to match the maximum
        // expected packet size. It is also important when crossing the threshold
        // from one buffer per packet to multiple buffers per packet. 
        stop(dev);
        // Set the MTU in the offload engine
        netoe_set_mtu(priv, dev->mtu);            
        open(dev);
    }
#endif    
    // If there was a failure
    if (status) {
        // Return the MTU to its original value
        DBG(1, KERN_INFO "change_mtu() Failed, returning MTU to original value\n");
        dev->mtu = original_mtu;
    }

    return status;
}

#if defined(CONFIG_ARCH_OX820)
static int netoe_hard_start_xmit(
    struct sk_buff *skb,
    struct net_device *dev)
{
    gmac_priv_t                *priv = (gmac_priv_t*)netdev_priv(dev);
    volatile netoe_job_t       *job;
    unsigned long               irq_flags;

    //printk("hard_start_xmit %p\n",skb);

    if (skb_shinfo(skb)->frag_list) {
        panic("Frag list - can't handle this!\n");
    }

    // Protection against concurrent operations in ISR and hard_start_xmit()
    if (!spin_trylock_irqsave(&priv->tx_spinlock_, irq_flags)) {
//        printk(KERN_WARNING "hard_start_xmit: returning locked\n");
        return NETDEV_TX_LOCKED;
    }

    // NETIF_F_LLTX apparently introduces a potential for hard_start_xmit() to
    // be called when the queue has been stopped (although I think only in SMP)
    // so do a check here to make sure we should proceed
    if (netif_queue_stopped(dev)) {
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
//        printk(KERN_WARNING "hard_start_xmit: returning tx busy\n");
        return NETDEV_TX_BUSY;
    }

    job = netoe_get_free_job(priv);

    if (!job) {
        // Shouldn't see a full ring without the queue having already been
        // stopped, and the queue should already have been stopped if we have
        // already queued a single pending packet
        if (priv->tx_pending_skb) {
            printk(KERN_WARNING "hard_start_xmit() Ring full and pending packet already queued\n");
            spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
            return NETDEV_TX_BUSY;
        }

        // Should keep a record of the skb that we haven't been able to queue
        // for transmission and queue it as soon as a job becomes free
        priv->tx_pending_skb = skb;
        netif_stop_queue(dev);
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
        return NETDEV_TX_BUSY;
    } else {
        // Record start of transmission, so timeouts will work once they're
        // implemented
        dev->trans_start = jiffies;

        // Add the job to the network offload engine
        netoe_send_job(priv, job, skb);
        
        // Poke the transmitter to look for available TX descriptors, as we have
        // just added one, in case it had previously found there were no more
        // pending transmission
        dma_reg_write(priv, DMA_TX_POLL_REG, 0);

    }

    spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);

    return NETDEV_TX_OK;
}
#endif

static int copro_hard_start_xmit(
    struct sk_buff *skb,
    struct net_device *dev)
{
    gmac_priv_t                *priv = (gmac_priv_t*)netdev_priv(dev);
    volatile gmac_tx_que_ent_t *job;
    unsigned long               irq_flags;

    if (skb_shinfo(skb)->frag_list) {
        int err;

		printk(KERN_WARNING "Linearizing skb with frag_list\n");
		err = skb_linearize(skb);
        if (err) {
            panic("copro_hard_start_xmit() Failed to linearize skb with frag_list\n");
        }
    }

    // Protection against concurrent operations in ISR and hard_start_xmit()
    if (!spin_trylock_irqsave(&priv->tx_spinlock_, irq_flags)) {
//printk(KERN_INFO "copro_hard_start_xmit() TX_LOCKED\n");
        return NETDEV_TX_LOCKED;
    }

    // NETIF_F_LLTX apparently introduces a potential for hard_start_xmit() to
    // be called when the queue has been stopped (although I think only in SMP)
    // so do a check here to make sure we should proceed
    if (unlikely(netif_queue_stopped(dev))) {
printk(KERN_INFO "copro_hard_start_xmit() Queue is stopped\n");
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
        return NETDEV_TX_BUSY;
    }

    job = tx_que_get_idle_job(dev);
    if (unlikely(!job)) {
//printk(KERN_INFO "copro_hard_start_xmit() No job, calling netif_stop_queue()\n");
		netif_stop_queue(dev);
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
        return NETDEV_TX_BUSY;
    }

	// Fill the Tx offload job with the network packet's details
	copro_fill_tx_job(priv, job, skb);

	// Enqueue the new Tx offload job with the CoPro
	tx_que_new_job(dev, job);

	// Record start of transmission, so timeouts will work once they're
	// implemented
	dev->trans_start = jiffies;

	// Interrupt the CoPro to cause it to examine the Tx offload queue
	wmb();
	writel(1UL << COPRO_SEM_INT_TX, SYS_CTRL_SEMA_SET_CTRL);

    spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);

    return NETDEV_TX_OK;
}

static inline void unmap_fragments(
    tx_frag_info_t *frags,
    int             count)
{
    while (count--) {
        dma_unmap_single(0, frags->phys_adr, frags->length, DMA_TO_DEVICE);
        ++frags;
    }
}

static int hard_start_xmit(
    struct sk_buff    *skb,
    struct net_device *dev)
{
    gmac_priv_t            *priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long           irq_flags;
    struct skb_shared_info *shinfo = skb_shinfo(skb);
    int                     fragment_count = shinfo->nr_frags + 1;
    tx_frag_info_t          fragments[fragment_count];
    int                     frag_index;

//    printk("hard_start_xmit %p\n",skb);
    if (shinfo->frag_list) {
        int err;

		printk(KERN_WARNING "Linearizing skb with frag_list\n");
		err = skb_linearize(skb);
        if (err) {
            panic("hard_start_xmit() Failed to linearize skb with frag_list\n");
        }
    }

    // Protection against concurrent operations in ISR and hard_start_xmit()
    if (!spin_trylock_irqsave(&priv->tx_spinlock_, irq_flags)) {
//        printk(KERN_WARNING "hard_start_xmit: returning locked\n");
        return NETDEV_TX_LOCKED;
    }

    // NETIF_F_LLTX apparently introduces a potential for hard_start_xmit() to
    // be called when the queue has been stopped (although I think only in SMP)
    // so do a check here to make sure we should proceed
    if (unlikely(netif_queue_stopped(dev))) {
//        printk(KERN_WARNING "hard_start_xmit: returning busy\n");
        spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
        return NETDEV_TX_BUSY;
    }

    // Map the main buffer
    fragments[0].length = skb_headlen(skb);
    fragments[0].phys_adr = dma_map_single(0, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
    BUG_ON(dma_mapping_error(0, fragments[0].phys_adr));

    // Map any SG fragments
    for (frag_index = 0; frag_index < shinfo->nr_frags; ++frag_index) {
        skb_frag_t *frag = &shinfo->frags[frag_index];

#ifdef CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
        if (PageIncoherentSendfile(frag->page.p) ||
#else // CONFIG_OXNAS_FAST_READS_AND_WRITES
		if (
#endif // CONFIG_OXNAS_FAST_READS_AND_WRITES
			(PageMappedToDisk(frag->page.p) && !PageDirty(frag->page.p))) {
            fragments[frag_index + 1].phys_adr = virt_to_dma(0, page_address(frag->page.p) + frag->page_offset);
        } else {
#endif // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
			fragments[frag_index + 1].phys_adr = dma_map_page(0, frag->page.p, frag->page_offset, frag->size, DMA_TO_DEVICE);
			BUG_ON(dma_mapping_error(0, fragments[frag_index + 1].phys_adr));
#ifdef CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
        }
#endif // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN

        fragments[frag_index + 1].length = frag->size;
    }

    // Construct the GMAC DMA descriptor
    if (unlikely(set_tx_descriptor(priv,
                          skb,
                          fragments,
                          fragment_count,
                          skb->ip_summed == CHECKSUM_PARTIAL) < 0)) {
        // Shouldn't see a full ring without the queue having already been
        // stopped, and the queue should already have been stopped if we have
        // already queued a single pending packet
        if (priv->tx_pending_skb) {
            printk(KERN_WARNING "hard_start_xmit() Ring full and pending packet already queued\n");
            unmap_fragments(fragments, fragment_count);
            spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);
            return NETDEV_TX_BUSY;
        }

        // Should keep a record of the skb that we haven't been able to queue
        // for transmission and queue it as soon as a descriptor becomes free
        priv->tx_pending_skb = skb;
        priv->tx_pending_fragment_count = fragment_count;

        // Copy the fragment info to the allocated storage
        memcpy(priv->tx_pending_fragments, fragments, sizeof(tx_frag_info_t) * fragment_count);

        // Stop further calls to hard_start_xmit() until some descriptors are
        // freed up by already queued TX packets being completed
        //printk(KERN_INFO "hard_start_xmit() calling netif_stop_queue(), tx_pending_skb %p\n", priv->tx_pending_skb);
		netif_stop_queue(dev);
    } else {
        // Record start of transmission, so timeouts will work once they're
        // implemented
        dev->trans_start = jiffies;

        // Poke the transmitter to look for available TX descriptors, as we have
        // just added one, in case it had previously found there were no more
        // pending transmission
        dma_reg_write(priv, DMA_TX_POLL_REG, 0);
    }

    spin_unlock_irqrestore(&priv->tx_spinlock_, irq_flags);

    return NETDEV_TX_OK;
}

static struct net_device_stats *get_stats(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    return &priv->stats;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/**
 * Polling 'interrupt' - used by things like netconsole to send skbs without
 * having to re-enable interrupts. It's not called while the interrupt routine
 * is executing.
 */
static void netpoll(struct net_device *netdev)
{
    disable_irq(netdev->irq);
    int_handler(netdev->irq, netdev); // only 2 arguments.
    enable_irq(netdev->irq);
}
#endif // CONFIG_NET_POLL_CONTROLLER

static const struct net_device_ops normal_netdev_ops = {
	.ndo_open				= open,
	.ndo_stop				= stop,
	.ndo_start_xmit			= hard_start_xmit,
	.ndo_get_stats			= get_stats,
	.ndo_set_rx_mode	= set_multicast_list,
	.ndo_set_mac_address	= set_mac_address,
	.ndo_change_mtu			= change_mtu,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= netpoll,
#endif
};

static const struct net_device_ops copro_netdev_ops = {
	.ndo_open				= open,
	.ndo_stop				= stop,
	.ndo_start_xmit			= copro_hard_start_xmit,
	.ndo_get_stats			= get_stats,
	.ndo_set_rx_mode	= set_multicast_list,
	.ndo_set_mac_address	= set_mac_address,
	.ndo_change_mtu			= change_mtu,
};

#if defined(CONFIG_ARCH_OX820)
static const struct net_device_ops netoe_netdev_ops = {
	.ndo_open				= open,
	.ndo_stop				= stop,
	.ndo_start_xmit			= netoe_hard_start_xmit,
	.ndo_get_stats			= get_stats,
	.ndo_set_rx_mode	= set_multicast_list,
	.ndo_set_mac_address	= set_mac_address,
	.ndo_change_mtu			= change_mtu,
};
#endif // CONFIG_ARCH_OX820

static int probe(
    struct net_device      *netdev,
    struct platform_device *plat_device,
	int						unit)
{
    u32 version;
    int i;
    unsigned synopsis_version;
    unsigned vendor_version;
    gmac_priv_t* priv;
    u32 reg_contents;
	u32 vaddr = mac_base[unit];
    int err = 0;

    // Ensure all of the device private data are zero, so we can clean up in
    // the event of a later failure to initialise all fields
    priv = (gmac_priv_t*)netdev_priv(netdev);
    memset(priv, 0, sizeof(gmac_priv_t));

    // No debug messages allowed
    priv->msg_level = 0UL;

	// Query module parameters as to whether to use the Leon for Tx offload
	priv->offload_tx = offload_tx[unit];

#if defined(CONFIG_ARCH_OX820)
	// Query module parameters as to whether to use the Leon for Tx offload
	priv->offload_netoe = offload_netoe[unit];
#endif
	if (copro_active(priv)) {
		// Ensure the Leon is held in reset so it can't cause any unexpected interrupts
#ifdef CONFIG_SUPPORT_LEON
		shutdown_copro();
#endif // CONFIG_SUPPORT_LEON

		// Disable all semaphore register bits so ARM cannot be interrupted
		*((volatile unsigned long*)SYS_CTRL_SEMA_MASKA_CTRL) = 0;
		*((volatile unsigned long*)SYS_CTRL_SEMA_MASKB_CTRL) = 0;
	}

	// How many Rx descriptors we are to manage
	priv->num_rx_descriptors =
		unit ? CONFIG_ARCH_OXNAS_GMAC2_NUM_RX_DESCRIPTORS :
			   CONFIG_ARCH_OXNAS_GMAC1_NUM_RX_DESCRIPTORS;

	if (offload_active(priv)) {
		// With offload the ARM doesn't manage any Tx descriptors itself
		priv->num_tx_descriptors = 0;

		if (copro_active(priv)) {
			priv->copro_num_tx_descs =
				unit ? CONFIG_ARCH_OXNAS_GMAC2_COPRO_NUM_TX_DESCRIPTORS :
					   CONFIG_ARCH_OXNAS_GMAC1_COPRO_NUM_TX_DESCRIPTORS;
		}
	} else {
		// How many Tx descriptors we are to manage
		priv->num_tx_descriptors =
			unit ? CONFIG_ARCH_OXNAS_GMAC2_NUM_TX_DESCRIPTORS :
				   CONFIG_ARCH_OXNAS_GMAC1_NUM_TX_DESCRIPTORS;
	}

	// How many Rx desciptors to process during a single call to poll before we
	// refill the Rx descriptor ring. Rx overflows seem to upset the GMAC, so
	// try to ensure we never see them by refilling way before the GMAC is
	// likely to have a chance of exhausting all available Rx descriptors
	priv->desc_since_refill_limit = priv->num_rx_descriptors >> 3;

	// Record the number of this unit
	priv->unit = unit;

    // Ensure the MAC block is properly reset
    writel(1UL << mac_reset_bit[priv->unit], SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << mac_reset_bit[priv->unit], SYS_CTRL_RSTEN_CLR_CTRL);

    // Enable the clock to the MAC block
    writel(1UL << mac_clken_bit[priv->unit], SYS_CTRL_CKEN_SET_CTRL);

    // Ensure reset and clock operations are complete
    wmb();

    // Initialise the ISR/hard_start_xmit() lock
    spin_lock_init(&priv->tx_spinlock_);

    // Initialise the PHY access lock
    spin_lock_init(&priv->phy_lock);

    // Set hardware device base addresses
    priv->macBase = vaddr + MAC_BASE_OFFSET;
    priv->dmaBase = vaddr + DMA_BASE_OFFSET;

#if defined(CONFIG_ARCH_OX820)
    priv->netoeBase = vaddr + NETOE_BASE_OFFSET;
	// Disable all GMAC (as opposed to GMAC DMA) interrupts as unbelievably they
	// are enabled by default from reset!
    mac_reg_write(priv, MAC_INT_MASK_REG, ~0UL);
#endif // CONFIG_ARCH_OX820

    // Initialise IRQ ownership to not owned
    priv->have_irq = 0;

    // Lock protecting access to CoPro command queue functions or direct access
    // to the GMAC interrupt enable register if CoPro is not in use
    spin_lock_init(&priv->cmd_que_lock_);

	if (copro_active(priv)) {
		sema_init(&priv->copro_stop_semaphore_, 0);
		sema_init(&priv->copro_start_semaphore_, 0);
		sema_init(&priv->copro_int_clr_semaphore_, 0);
		sema_init(&priv->copro_update_semaphore_, 0);
		sema_init(&priv->copro_rx_enable_semaphore_, 0);
		sema_init(&priv->copro_stop_complete_semaphore_, 0);
		priv->copro_irq_alloced_ = 0;
	}

    init_timer(&priv->watchdog_timer);
    priv->watchdog_timer.function = &watchdog_timer_action;
    priv->watchdog_timer.data = (unsigned long)priv;

    // Set pointer to device in private data
    priv->netdev = netdev;
    priv->plat_dev = plat_device;
    priv->device = &plat_device->dev;

    /** Do something here to detect the present or otherwise of the MAC
     *  Read the version register as a first test */
    version = mac_reg_read(priv, MAC_VERSION_REG);
    synopsis_version = version & 0xff;
    vendor_version   = (version >> 8) & 0xff;

    /** Assume device is at the adr and irq specified until have probing working */
    netdev->base_addr  = vaddr;
    netdev->irq        = mac_interrupt[unit];

	if (copro_active(priv)) {
		priv->copro_irq_fwd_ 	 = SEM_A_INTERRUPT;
		priv->copro_irq_offload_ = SEM_B_INTERRUPT;
	}

	if (copro_active(priv)) {
		// Allocate the CoPro semaphore IRQs
		err = request_irq(priv->copro_irq_offload_, &copro_offload_intr, 0, "GMAC SEMAPHORE OFFLOAD", netdev);
		if (err) {
			printk(KERN_ERR "probe() %s: Failed to allocate GMAC %d CoPro semaphore interrupt (%d)\n", netdev->name, unit, priv->copro_irq_offload_);
			goto probe_err_out;
		}
		err = request_irq(priv->copro_irq_fwd_, &copro_fwd_intr, 0, "GMAC SEMAPHORE FWD", netdev);
		if (err) {
			printk(KERN_ERR "probe() %s: Failed to allocate GMAC %d CoPro semaphore interrupt (%d)\n", netdev->name, unit, priv->copro_irq_fwd_);
			free_irq(priv->copro_irq_offload_, netdev);
			goto probe_err_out;
		}

		// Release the CoPro semaphore IRQ again, as open()/stop() should manage IRQ ownership
		free_irq(priv->copro_irq_offload_, netdev);
		free_irq(priv->copro_irq_fwd_, netdev);
	} else {
		// Allocate the IRQ
		err = request_irq(netdev->irq, &int_handler, 0, netdev->name, netdev);
		if (err) {
			printk(KERN_ERR "probe() %s: Failed to allocate irq %d\n", netdev->name, netdev->irq);
			goto probe_err_out;
		}

		// Release the IRQ again, as open()/stop() should manage IRQ ownership
		free_irq(netdev->irq, netdev);
	}

    // Initialise the ethernet device with std. contents
    ether_setup(netdev);

    // Tell the kernel of our MAC address
	for (i = 0; i < netdev->addr_len; i++) {
		netdev->dev_addr[i] = (unsigned char)mac_adrs[unit][i];
	}

    // Setup operations pointers
	netdev->netdev_ops = &normal_netdev_ops;

	if (copro_active(priv)) {
		netdev->netdev_ops = &copro_netdev_ops;
#if defined(CONFIG_ARCH_OX820)
    } else if (netoe_active(priv)) {    
		netdev->netdev_ops = &netoe_netdev_ops;
#endif
	} else {
		netdev->netdev_ops = &normal_netdev_ops;
	}

	// Initialise NAPI support
	netif_napi_add(netdev, &priv->napi_struct, &poll, CONFIG_OXNAS_NAPI_POLL_WEIGHT);

    set_ethtool_ops(netdev);

    if (debug) {
      netdev->flags |= IFF_DEBUG;
    }

    // Can process SG lists - we don't currently support processing frag lists
	// for Tx offload
	netdev->features |= NETIF_F_SG;

	// Can checksum TCP/UDP over IPv4
    netdev->features |= NETIF_F_IP_CSUM;
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
	// Can checksum TCP/UDP over IPv6
    netdev->features |= NETIF_F_IPV6_CSUM;
#endif // CONFIG_OXNAS_IPV6_OFFLOAD

	if (offload_active(priv)) {
		// Can do segmentation offload for TCP/UDP over IPv4
		netdev->features |= NETIF_F_TSO;
#ifdef CONFIG_OXNAS_IPV6_OFFLOAD
		// Can do segmentation offload for TCP/UDP over IPv6
        netdev->features |= NETIF_F_TSO6;
#endif // CONFIG_OXNAS_IPV6_OFFLOAD
	}

    // We take care of our own TX locking
    netdev->features |= NETIF_F_LLTX;

    // Initialise PHY support
    priv->mii.phy_id_mask   = 0x1f;
    priv->mii.reg_num_mask  = 0x1f;
    priv->mii.force_media   = 0;
    priv->mii.full_duplex   = 1;
    priv->mii.advertising = ( ADVERTISED_TP 
				| ADVERTISED_MII
				| ADVERTISED_100baseT_Half
				| ADVERTISED_10baseT_Full
				| ADVERTISED_10baseT_Half);
/*priv->mii.using_100     = 0;
    priv->mii.using_1000    = 1;
	priv->mii.using_pause   = 1;*/
    priv->mii.dev           = netdev;
    priv->mii.mdio_read     = phy_read_via_mac0;
    priv->mii.mdio_write    = phy_write_via_mac0;

    priv->gmii_csr_clk_range = 5;   // Slowest for now

	// Setup GMAC clocking
#if defined(CONFIG_ARCH_OX820)
	reg_contents = unit ? readl(SYS_CTRL_GMACB_CTRL) : readl(SYS_CTRL_GMACA_CTRL);
	reg_contents |= ((1UL << SYS_CTRL_GMAC_CKEN_GTX) |
					 (1UL << SYS_CTRL_GMAC_SIMPLE_MUX) |
					 (1UL << SYS_CTRL_GMAC_AUTO_TX_SOURCE) |
					 (1UL << SYS_CTRL_GMAC_CKEN_TX_OUT) |
					 (1UL << SYS_CTRL_GMAC_CKEN_TXN_OUT) |
					 (1UL << SYS_CTRL_GMAC_CKEN_TX_IN) |
					 (1UL << SYS_CTRL_GMAC_CKEN_RX_OUT) |
					 (1UL << SYS_CTRL_GMAC_CKEN_RXN_OUT) |
					 (1UL << SYS_CTRL_GMAC_CKEN_RX_IN));
    writel(reg_contents, unit ? SYS_CTRL_GMACB_CTRL : SYS_CTRL_GMACA_CTRL);

	printk(KERN_INFO "%s: Tuning GMAC %d RGMII timings\n", priv->netdev->name, unit);
	reg_contents = ((0x4 << SYS_CTRL_GMAC_TX_VARDELAY) |
					(0x2 << SYS_CTRL_GMAC_TXN_VARDELAY) |
					(0xa << SYS_CTRL_GMAC_RX_VARDELAY) |
					(0x8 << SYS_CTRL_GMAC_RXN_VARDELAY));
    writel(reg_contents, unit ? SYS_CTRL_GMACB_DELAY_CTRL : SYS_CTRL_GMACA_DELAY_CTRL);
#else // CONFIG_ARCH_OX820
    // Use simple mux for 25/125 Mhz clock switching and
    // enable GMII_GTXCLK to follow GMII_REFCLK - required for gigabit PHY
	reg_contents = readl(SYS_CTRL_GMAC_CTRL);
	reg_contents |= ((1UL << SYS_CTRL_GMAC_SIMPLE_MUX) |
					 (1UL << SYS_CTRL_GMAC_CKEN_GTX));
    writel(reg_contents, SYS_CTRL_GMAC_CTRL);
#endif // CONFIG_ARCH_OX820

    // Remember whether auto-negotiation is allowed
#ifdef ALLOW_AUTONEG
    priv->ethtool_cmd.autoneg = 1;
	priv->ethtool_pauseparam.autoneg = 1;
#else // ALLOW_AUTONEG
    priv->ethtool_cmd.autoneg = 0;
	priv->ethtool_pauseparam.autoneg = 0;
#endif // ALLOW_AUTONEG

    // Set up PHY mode for when auto-negotiation is not allowed
    priv->ethtool_cmd.speed = SPEED_1000;
    priv->ethtool_cmd.duplex = DUPLEX_FULL;
    priv->ethtool_cmd.port = PORT_MII;
    priv->ethtool_cmd.transceiver = XCVR_INTERNAL;

	// We can support both reception and generation of pause frames
	priv->ethtool_pauseparam.rx_pause = 1;
	priv->ethtool_pauseparam.tx_pause = 1;

    // Initialise the set of features we would like to advertise as being
	// available for negotiation
    priv->ethtool_cmd.advertising = (ADVERTISED_10baseT_Half |
                                     ADVERTISED_10baseT_Full |
                                     ADVERTISED_100baseT_Half |
                                     ADVERTISED_100baseT_Full |
                                     ADVERTISED_1000baseT_Half |
                                     ADVERTISED_1000baseT_Full |
									 ADVERTISED_Pause |
									 ADVERTISED_Asym_Pause |
                                     ADVERTISED_Autoneg |
                                     ADVERTISED_MII);

    // Attempt to locate the PHY
    phy_detect(netdev, phy_ids[priv->unit]);
    priv->ethtool_cmd.phy_address = priv->mii.phy_id;

    // Did we find a PHY?
	if (priv->phy_type == PHY_TYPE_NONE) {
		printk(KERN_WARNING "%s: No PHY found\n", netdev->name);
		err = ENXIO;
		goto probe_err_out;
    }

	// Setup the PHY
	initialise_phy(priv);

	// Find out what modes the PHY supports
	priv->ethtool_cmd.supported = get_phy_capabilies(priv);

	// Record whether the PHY is gigabit capable
	if ((priv->ethtool_cmd.supported & SUPPORTED_1000baseT_Full) ||
		(priv->ethtool_cmd.supported & SUPPORTED_1000baseT_Half)) {
		priv->mii.supports_gmii = 1;
	}

    // Register the device with the network intrastructure
    err = register_netdev(netdev);
    if (err) {
        printk(KERN_ERR "probe() %s: Failed to register device\n", netdev->name);
        goto probe_err_out;
    }

    // Record details about the hardware we found
    printk(KERN_NOTICE "%s: GMAC ver = %u, vendor ver = %u at 0x%lx, IRQ %d\n", netdev->name, synopsis_version, vendor_version, netdev->base_addr, netdev->irq);
    printk(KERN_NOTICE "%s: Found PHY at address %u, type 0x%08x -> %s\n", priv->netdev->name, priv->mii.phy_id, priv->phy_type, (priv->ethtool_cmd.supported & SUPPORTED_1000baseT_Full) ? "10/100/1000" : "10/100");
    printk(KERN_NOTICE "%s: Ethernet addr: ", priv->netdev->name);
    for (i = 0; i < 5; i++) {
        printk("%02x:", netdev->dev_addr[i]);
    }
    printk("%02x\n", netdev->dev_addr[5]);

	if (copro_active(priv)) {
		// Define sizes of queues for communicating with the CoPro
		priv->copro_cmd_que_num_entries_ = COPRO_CMD_QUEUE_NUM_ENTRIES;
		priv->copro_tx_que_num_entries_ = CONFIG_OXNAS_COPRO_JOB_QUEUE_NUM_ENTRIES;

		// Zeroise the queues, so checks for empty etc will work before storage
		// for queue entries has been allocated
		tx_que_init(&priv->tx_queue_, 0, 0);
		cmd_que_init(&priv->cmd_queue_, 0, 0);

		// Set the Leon x2 clock mode as requested via the module parameters
#if defined(CONFIG_OXNAS_LEON_X2)
		printk(KERN_INFO "probe() %s: Leon x2 clock\n", priv->netdev->name);
		reg_contents = readl(SEC_CTRL_COPRO_CTRL);
		reg_contents |= (1UL << SEC_CTRL_COPRO_DOUBLE_CLK);
		writel(reg_contents, SEC_CTRL_COPRO_CTRL);
#endif // CONFIG_OXNAS_LEON_X2
	}

	// Initialise sysfs for link state reporting
	priv->link_state_kobject.kset = link_state_kset;
	err = kobject_init_and_add(&priv->link_state_kobject,
		&ktype_gmac_link_state, NULL, "%d", priv->unit);
	if (err) {
		printk(KERN_ERR "probe() %s: Failed to add kobject\n", priv->netdev->name);
		kobject_put(&priv->link_state_kobject);
		goto probe_err_out;
	}

	priv->interface_up = 0;
    return 0;

probe_err_out:
#if (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
    // Disable the clock to the MAC block
    writel(1UL << mac_clken_bit[priv->unit], SYS_CTRL_CKEN_CLR_CTRL);
#endif // (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)

    return err;
}

/**
 * External entry point to the driver, called from Space.c to detect a card
 */
struct net_device* __init synopsys_gmac_probe(int unit)
{
    int err = 0;
    struct net_device *netdev = alloc_etherdev(sizeof(gmac_priv_t));

    printk(KERN_NOTICE "Probing for Synopsis GMAC, unit %d\n", unit);

    // Will allocate private data later, as may want descriptors etc in special memory
    if (!netdev) {
        printk(KERN_WARNING "synopsys_gmac_probe() failed to alloc device\n");
        err = -ENODEV;
    } else {
		struct platform_device *pdev;

		sprintf(netdev->name, "eth%d", unit);
		netdev_boot_setup_check(netdev);

		pdev = platform_device_register_simple("gmac", unit, NULL, 0);
		if (IS_ERR(pdev)) {
			err = PTR_ERR(pdev);
			printk(KERN_WARNING "synopsys_gmac_probe() Failed to register platform device %s, err = %d\n", netdev->name, err);
		} else {
			err = probe(netdev, pdev, unit);
			if (err) {
				printk(KERN_WARNING "synopsys_gmac_probe() Probing failed for %s, err = %d\n", netdev->name, err);
				platform_device_unregister(pdev);
			}
		}

        if (err) {
            netdev->reg_state = NETREG_UNINITIALIZED;
            free_netdev(netdev);
        } else {
			gmac_netdev[gmac_found_count++] = netdev;
		}
    }

    return ERR_PTR(err);
}

void cmd_que_init(
    cmd_que_t          *queue,
    gmac_cmd_que_ent_t *start,
    int                 num_entries)
{
    // Initialise queue management metadata
    INIT_LIST_HEAD(&queue->ack_list_);
    queue->head_ = start;
    queue->tail_ = start + num_entries;
    queue->w_ptr_ = queue->head_;

    // Zeroise all entries in the queue
    memset(start, 0, num_entries * sizeof(gmac_cmd_que_ent_t));
}

int cmd_que_dequeue_ack(cmd_que_t *queue)
{
    struct list_head *list_entry;
    cmd_que_ack_t    *ack;

    if (list_empty(&queue->ack_list_)) {
        return -1;
    }

    // Remove the first entry on the acknowledgement list
    list_entry = queue->ack_list_.next;
    BUG_ON(!list_entry);

    // Get pointer to ack entry from it's list_head member        
    ack = list_entry(list_entry, cmd_que_ack_t, list_);
    BUG_ON(!ack);
    BUG_ON(!ack->priv_);
    BUG_ON(!ack->entry_);
    BUG_ON(!ack->callback_);

    // Has the CoPro acknowledged the command yet?
    if (!(ack->entry_->opcode_ & (1UL << GMAC_CMD_QUE_SKP_BIT))) {
        // No, so no further acknowledgements can be pending as CoPro executes
        // commands/acks in order
        return -1;
    }

    // Going to process the acknowledgement, so remove it from the pending list
    list_del(list_entry);

//printk(KERN_INFO "ak=0x%08x:0x%08x\n", ack->entry_->opcode_, ack->entry_->operand_);
    // Invoke the acknowledgement handler routine
    ack->callback_(ack->priv_, ack->entry_);

    // Reset ACK flag in command queue entry
    ack->entry_->opcode_ &= ~(1UL << GMAC_CMD_QUE_ACK_BIT);

    kfree(ack);
    return 0;
}

#define OPCODE_FLAGS_MASK ((1UL << (GMAC_CMD_QUE_OWN_BIT)) |\
                           (1UL << (GMAC_CMD_QUE_ACK_BIT)) |\
                           (1UL << (GMAC_CMD_QUE_SKP_BIT)))

int cmd_que_queue_cmd(
    gmac_priv_t          *priv,
    u32                   opcode,
    u32                   operand,
    cmd_que_ack_callback  callback)
{
	cmd_que_t *queue = &priv->cmd_queue_;
    int result = -1;

    do {
        volatile gmac_cmd_que_ent_t* entry = queue->w_ptr_;
        u32 old_opcode = entry->opcode_;

        if (old_opcode & (1UL << GMAC_CMD_QUE_OWN_BIT)) {
            // Queue is full as we've run into an entry still owned by the CoPro
            break;
        }

        if (!(old_opcode & (1UL << GMAC_CMD_QUE_ACK_BIT))) {
            // We've found an entry we own that isn't waiting for the contained
            // ack to be processed, so we can use it for the new command
            opcode &= ~(OPCODE_FLAGS_MASK);
            opcode |= (1UL << GMAC_CMD_QUE_OWN_BIT);

            if (callback) {
                // Register ack. handler before releasing entry to CoPro
                cmd_que_ack_t *ack = kmalloc(sizeof(cmd_que_ack_t), GFP_ATOMIC);
                BUG_ON(!ack);

                ack->priv_ = priv;
                ack->entry_ = queue->w_ptr_;
                ack->callback_ = callback;
                INIT_LIST_HEAD(&ack->list_);
                list_add_tail(&ack->list_, &queue->ack_list_);

                // Mark the entry as requiring an ack.
                opcode |= (1UL << GMAC_CMD_QUE_ACK_BIT);
            }

            // Copy the command into the queue entry and pass ownership to the
            // CoPro, being sure to set the OWN flag last
//printk(KERN_INFO "op=0x%08x:0x%08x\n", opcode, operand);
            queue->w_ptr_->operand_ = operand;
            wmb();
            queue->w_ptr_->opcode_  = opcode;
            // Ensure the OWN flag gets to memory before any following interrupt
            // to the CoPro is issued
            wmb();

            result = 0;
        }

        // Make the write pointer point to the next potentially available entry
        if (++queue->w_ptr_ == queue->tail_) {
            queue->w_ptr_ = queue->head_;
        }
    } while (result);

    return result;
}

void tx_que_init(
    tx_que_t          *queue,
    gmac_tx_que_ent_t *start,
    int                num_entries)
{
    // Initialise queue management metadata
    queue->head_ = start;
    queue->tail_ = start + num_entries;
    queue->w_ptr_ = queue->head_;
    queue->r_ptr_ = queue->head_;
    queue->full_ = 0;

    // Zeroise all entries in the queue
    memset(start, 0, num_entries * sizeof(gmac_tx_que_ent_t));
}

static inline void tx_que_inc_w_ptr(tx_que_t *queue)
{
    if (++queue->w_ptr_ == queue->tail_) {
        queue->w_ptr_ = queue->head_;
    }
    queue->full_ = (queue->w_ptr_ == queue->r_ptr_);
}

volatile gmac_tx_que_ent_t* tx_que_get_finished_job(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    tx_que_t *queue = &priv->tx_queue_;
    volatile gmac_tx_que_ent_t *entry = 0;

    if (tx_que_not_empty(queue)) {
        entry = queue->r_ptr_;
        if (entry->flags_ & (1UL << TX_JOB_FLAGS_OWN_BIT)) {
            entry = (volatile gmac_tx_que_ent_t*)0;
        } else {
            tx_que_inc_r_ptr(queue);
        }
    }

    return entry;
}

/**
 * A call to tx_que_get_idle_job() must be followed by a call to tx_que_new_job()
 * before any subsequent call to tx_que_get_idle_job()
 */
volatile gmac_tx_que_ent_t* tx_que_get_idle_job(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    tx_que_t *queue = &priv->tx_queue_;
    volatile gmac_tx_que_ent_t *entry = 0;

    // Must not reuse completed Tx packets returned by CoPro until the queue
    // reader has had a chance to process them
    if (!tx_que_is_full(queue)) {
        entry = queue->w_ptr_;
    }

    return entry;
}

/**
 * A call to tx_que_get_idle_job() must be followed by a call to tx_que_new_job()
 * before any subsequent call to tx_que_get_idle_job()
 */
void tx_que_new_job(
    struct net_device *dev,
    volatile gmac_tx_que_ent_t *entry)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    // Make sure any modifications to Tx job structures make it to memory before
    // setting the OWN flag to pass ownership to the CoPro
    wmb();

    entry->flags_ |= (1UL << TX_JOB_FLAGS_OWN_BIT);

    tx_que_inc_w_ptr(&priv->tx_queue_);
}

extern int __init gmac_phy_init(void);

static int __init gmac_module_init(void) {
	int i;
	int err = platform_driver_register(&plat_driver);
	if (err) {
		printk(KERN_WARNING "gmac_module_init() Failed to register platform driver\n");
		return err;
	}

	/* Prepare the sysfs interface for use */
	link_state_kset = kset_create_and_add("gmac_link_state", &gmac_link_state_uevent_ops, kernel_kobj);
	if (!link_state_kset) {
		printk(KERN_ERR "gmac_module_init() Failed to create kset\n");
		platform_driver_unregister(&plat_driver);
		return -ENOMEM;
	}

	gmac_phy_init();

	/* Find all GMACs */
	for (i=0; i < CONFIG_OXNAS_MAX_GMAC_UNITS; i++) {
		err = (int)synopsys_gmac_probe(i);
		if (err) {
			printk(KERN_WARNING "gmac_module_init() Failed while detecting GMAC instance %d\n", i);
		}
	}

	if (!gmac_found_count) {
		kset_unregister(link_state_kset);
		platform_driver_unregister(&plat_driver);
	}

	return 0;
}
module_init(gmac_module_init);

static void __exit gmac_module_cleanup(void)
{
	int i;
	for (i=0; i < gmac_found_count; i++) {
		struct net_device* netdev = gmac_netdev[i];
		gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(netdev);

#ifdef USE_TX_TIMEOUT
		cancel_work_sync(&priv->tx_timeout_work);
#endif // USE_TX_TIMEOUT
		cancel_work_sync(&priv->link_state_change_work);

		platform_device_unregister(priv->plat_dev);
		unregister_netdev(netdev);
		netdev->reg_state = NETREG_UNREGISTERED;

		// Free the sysfs resources
		kobject_put(&priv->link_state_kobject);

		free_netdev(netdev);
	}

	kset_unregister(link_state_kset);

	if (gmac_found_count) {
		platform_driver_unregister(&plat_driver);
	}
}
module_exit(gmac_module_cleanup);

/* void post_phy_reset_action(struct net_device *dev) */
/* { */
/*     gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev); */

/* 	switch (priv->phy_type) { */
/* 		case PHY_TYPE_REALTEK_RTL8211D: */
/* 			// If we don't have this the Realtek RTL8211D can fail */
/* 			wait_autoneg_complete(priv); */
/* 			break; */
/* 	} */
/* } */

