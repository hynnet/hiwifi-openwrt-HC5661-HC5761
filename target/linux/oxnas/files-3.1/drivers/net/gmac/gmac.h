/*
 * linux/arch/arm/mach-oxnas/gmac.h
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
#if !defined(__GMAC_H__)
#define __GMAC_H__

#include <linux/semaphore.h>
#include <asm/types.h>
#include <linux/mii.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/ethtool.h>
#include <linux/kobject.h>
#include <mach/desc_alloc.h>
#include <mach/dma.h>
#include <linux/kernel.h>

#ifdef GMAC_DEBUG
#define DBG(n, args...)\
    do {\
            printk(args);\
    } while (0)
#else
#define DBG(n, args...)   do { } while(0)
#endif

#define MS_TO_JIFFIES(x) (((x) < (1000/(HZ))) ? 1 : (x) * (HZ) / 1000)

#define USE_RX_CSUM

#define USE_TX_TIMEOUT

#if (CONFIG_OXNAS_MAX_GMAC_UNITS == 1)
static const int mac_base[] = { MAC_BASE };
#elif (CONFIG_OXNAS_MAX_GMAC_UNITS == 2)
static const int mac_base[] = { MAC_BASE, MAC_BASE_2 };
#else
#error "Invalid number of Ethernet ports"
#endif

typedef struct gmac_desc_list_info {
    volatile gmac_dma_desc_t *base_ptr;
    gmac_dma_desc_t          *shadow_ptr;
    int                       num_descriptors;
    int                       empty_count;
    int                       full_count;
    int                       r_index;
    int                       w_index;
} gmac_desc_list_info_t;

typedef struct copro_params {
    u32 cmd_que_head_;
    u32 cmd_que_tail_;
    u32 fwd_intrs_mailbox_;
    u32 tx_que_head_;
    u32 tx_que_tail_;
    u32 free_start_;
    u32 mtu_;
	u32 tx_descs_base_;
	u32 num_tx_descs_;
} __attribute ((aligned(4),packed)) copro_params_t;

typedef struct tx_frag_info {
    dma_addr_t  phys_adr;
    u16         length;
} tx_frag_info_t;

typedef struct gmac_cmd_que_ent {
    u32 opcode_;
    u32 operand_;
} __attribute ((aligned(4),packed)) gmac_cmd_que_ent_t;

typedef struct cmd_que {
    gmac_cmd_que_ent_t          *head_;
    gmac_cmd_que_ent_t          *tail_;
    volatile gmac_cmd_que_ent_t *w_ptr_;
    struct list_head             ack_list_;
} cmd_que_t;

/* Make even number else gmac_tx_que_ent_t struct below alignment will be wrong */
#define COPRO_NUM_TX_FRAGS_DIRECT 18

typedef struct gmac_tx_que_ent {
    u32 skb_;
    u32 len_;
    u32 data_len_;
    u32 ethhdr_;
    u32 iphdr_;
    u32 transport_hdr_;
    u16 iphdr_csum_;
    u16 tso_segs_;
    u16 tso_size_;
    u16 flags_;
    u32 frag_ptr_[COPRO_NUM_TX_FRAGS_DIRECT];
    u16 frag_len_[COPRO_NUM_TX_FRAGS_DIRECT];
    u32 statistics_;
} __attribute ((aligned(4),packed)) gmac_tx_que_ent_t;

typedef struct tx_que {
    gmac_tx_que_ent_t          *head_;
    gmac_tx_que_ent_t          *tail_;
    volatile gmac_tx_que_ent_t *w_ptr_;
    volatile gmac_tx_que_ent_t *r_ptr_;
    int                         full_;
} tx_que_t;

typedef void (*gmac_finish_xmit)(struct net_device* dev);

typedef enum watchdog_state {
	WDS_IDLE,
	WDS_RESETTING,
	WDS_NEGOTIATING
} watchdog_state_t;

// Private data structure for the GMAC driver
typedef struct gmac_priv {
	int	unit;
	int offload_tx;     // 0 => acceleration disabled, 1 => acceleration enabled
#if defined(CONFIG_ARCH_OX820)
	int offload_netoe;  // 0 => copro/software acceleration, 1 => hardware accel
	int leon_x2;
#endif
	int num_tx_descriptors;
	int num_rx_descriptors;
	int desc_since_refill_limit;

    /** Base address of GMAC MAC registers */
    u32                       macBase;
    /** Base address of GMAC DMA registers */
    u32                       dmaBase;

    struct net_device*        netdev;

    struct net_device_stats   stats;

    u32                       msg_level;
    
    /** Whether we own an IRQ */
    int                       have_irq;

    /** Pointer to outstanding tx packet that has not yet been queued due to
     *  lack of descriptors */
    struct sk_buff           *tx_pending_skb;
    tx_frag_info_t            tx_pending_fragments[18];
    int                       tx_pending_fragment_count;

    /** DMA consistent physical address of outstanding tx packet */
    dma_addr_t                tx_pending_dma_addr;
    unsigned long             tx_pending_length;

    /** To synchronise ISR and thread TX activities' access to private data */
    spinlock_t                tx_spinlock_;

    /** To synchronise access to the PHY */
    spinlock_t                phy_lock;

    /** The timer for NAPI polling when out of memory when trying to fill RX
     *  descriptor ring */

    /** PHY related info */
    struct mii_if_info        mii;
    struct ethtool_cmd        ethtool_cmd;
	struct ethtool_pauseparam ethtool_pauseparam;
    u32                       phy_type;
    int                       gmii_csr_clk_range;

    /** Periodic timer to check link status etc */
    struct timer_list         watchdog_timer;
	int                       watchdog_timer_state;
    volatile int              watchdog_timer_shutdown;

    /** The CPU accessible virtual address of the start of the descriptor array */
    gmac_dma_desc_t*          desc_vaddr;
    /** The hardware accessible physical address of the start of the descriptor array */
    dma_addr_t                desc_dma_addr;

#ifdef CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
#ifndef CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
	void       *copro_tx_desc_vaddr;
	dma_addr_t  copro_tx_desc_paddr;
#endif // CONFIG_OXNAS_GMAC_SRAM_TX_DESCRIPTORS
#endif // CONFIG_OXNAS_GMAC_DDR_DESCRIPTORS
	int         copro_num_tx_descs;

    /** Descriptor list management */
    gmac_desc_list_info_t     tx_gmac_desc_list_info;
    gmac_desc_list_info_t     rx_gmac_desc_list_info;

    /** Record of disabling RX overflow interrupts */
    unsigned                  rx_overflow_ints_disabled;

    /** The result of the last H/W DMA generated checksum operation */
    u16                       tx_csum_result_;

    /** Whether we deal in jumbo frames */
    int                       jumbo_;

    volatile int              mii_init_media;
    volatile int              phy_force_negotiation;

    /** DMA coherent memory for CoPro's parameter storage */
    copro_params_t  *shared_copro_params_;
    dma_addr_t       shared_copro_params_pa_;

    /** ARM's local CoPro parameter storage */
    copro_params_t   copro_params_;

    /** Queue for commands/acks to/from CoPro */
    int              copro_irq_offload_;
    int              copro_irq_fwd_;
    int              copro_irq_alloced_;
    cmd_que_t        cmd_queue_;
    tx_que_t         tx_queue_;
    int              copro_cmd_que_num_entries_;
    int              copro_tx_que_num_entries_;

    struct semaphore copro_stop_semaphore_;
    struct semaphore copro_start_semaphore_;
    struct semaphore copro_int_clr_semaphore_;
    struct semaphore copro_update_semaphore_;
    struct semaphore copro_rx_enable_semaphore_;
	struct semaphore copro_stop_complete_semaphore_;

	unsigned long copro_int_clr_return_;

#if defined(CONFIG_ARCH_OX820)
    u32              netoe_job_list;
    int              job_count_;
    /** Base address of GMAC Netork offload engine registers */
    u32              netoeBase;
    /** A pointer to the allocated physical address of the tx job queue */
    u32              queue_pa_;
    /** A pointer to the base of the tx job queue - the physical address aligned on a 4k boundary*/
    u32              queue_base_;
    /** A pointer to the next completed tx job on the queue */
    u32              completed_tx_ptr_;
    /** A pointer to the next free tx job on the queue */
    u32              free_tx_ptr_;
    /** Status summary from the NetOE core */
    u32              cumulative_status_;
#endif
    // Transmit complete function pointer (called from IRQ handler)
    gmac_finish_xmit finish_xmit;

    spinlock_t       cmd_que_lock_;

	u32	rx_buffer_size_;
	int	bytes_consumed_per_rx_buffer_;
	int	rx_buffers_per_page_;

	gmac_dma_desc_t *tx_desc_shadow_;
	gmac_dma_desc_t *rx_desc_shadow_;

	struct napi_struct napi_struct;

	/** sysfs dir tree root for recovery button driver */
	struct kobject     link_state_kobject;
	struct work_struct link_state_change_work;
	int                link_state;

	struct device *device;
	struct platform_device *plat_dev;
	int napi_enabled;
	int copro_started;
	int interface_up;
#ifdef USE_TX_TIMEOUT
	struct work_struct tx_timeout_work;
	int                tx_timeout_count;
#endif // USE_TX_TIMEOUT
} gmac_priv_t;

#define GMAC_CMD_QUE_OWN_BIT 31 // 0-Owned by ARM, 1-Owned by CoPro
#define GMAC_CMD_QUE_ACK_BIT 30
#define GMAC_CMD_QUE_SKP_BIT 29

#define GMAC_CMD_STOP               0
#define GMAC_CMD_START              1
#define GMAC_CMD_INT_EN_SET         2
#define GMAC_CMD_INT_EN_CLR         3
#define GMAC_CMD_HEARTBEAT          4
#define GMAC_CMD_UPDATE_PARAMS      5
#define GMAC_CMD_CONFIG_LINK_SPEED	6
#define GMAC_CMD_CHANGE_RX_ENABLE   7
#define GMAC_CMD_CLEAR_RX_INTS      8

typedef void (*cmd_que_ack_callback)(gmac_priv_t *priv, volatile gmac_cmd_que_ent_t *entry);

typedef struct cmd_que_ack {
    struct list_head			 list_;
    gmac_priv_t					*priv_;
    volatile gmac_cmd_que_ent_t *entry_;
    cmd_que_ack_callback		 callback_;
} cmd_que_ack_t;

extern void cmd_que_init(
    cmd_que_t          *queue,
    gmac_cmd_que_ent_t *start,
    int                 num_entries);

extern int cmd_que_dequeue_ack(cmd_que_t *queue);

extern int cmd_que_queue_cmd(
    gmac_priv_t			 *priv,
    u32                   opcode,
    u32                   operand,
    cmd_que_ack_callback  callback);    // non-zero if ack. required

#define COPRO_SEM_INT_CMD 0
#define COPRO_SEM_INT_TX  1

typedef struct gmac_fwd_intrs {
    u32 status_;
} __attribute ((aligned(4),packed)) gmac_fwd_intrs_t;

#define TX_JOB_FLAGS_OWN_BIT              0
#define TX_JOB_FLAGS_COPRO_RESERVED_1_BIT 1
#define TX_JOB_FLAGS_ACCELERATE_BIT       2
#define TX_JOB_FLAGS_DEBUG_BIT            3

#define TX_JOB_STATS_BYTES_BIT 0
#define TX_JOB_STATS_BYTES_NUM_BITS 16
#define TX_JOB_STATS_BYTES_MASK (((1UL << TX_JOB_STATS_BYTES_NUM_BITS) - 1) << TX_JOB_STATS_BYTES_BIT)
#define TX_JOB_STATS_PACKETS_BIT 16
#define TX_JOB_STATS_PACKETS_NUM_BITS 6
#define TX_JOB_STATS_PACKETS_MASK (((1UL << TX_JOB_STATS_PACKETS_NUM_BITS) - 1) << TX_JOB_STATS_PACKETS_BIT)
#define TX_JOB_STATS_ABORT_BIT 22
#define TX_JOB_STATS_ABORT_NUM_BITS 3
#define TX_JOB_STATS_ABORT_MASK (((1UL << TX_JOB_STATS_ABORT_NUM_BITS) - 1) << TX_JOB_STATS_ABORT_BIT)
#define TX_JOB_STATS_CARRIER_BIT 25
#define TX_JOB_STATS_CARRIER_NUM_BITS 3
#define TX_JOB_STATS_CARRIER_MASK (((1UL << TX_JOB_STATS_CARRIER_NUM_BITS) - 1) << TX_JOB_STATS_CARRIER_BIT)
#define TX_JOB_STATS_COLLISION_BIT 28
#define TX_JOB_STATS_COLLISION_NUM_BITS 4
#define TX_JOB_STATS_COLLISION_MASK (((1UL << TX_JOB_STATS_COLLISION_NUM_BITS) - 1) << TX_JOB_STATS_COLLISION_BIT)

extern void tx_que_init(
    tx_que_t          *queue,
    gmac_tx_que_ent_t *start,
    int                num_entries);

static inline int tx_que_not_empty(tx_que_t *queue)
{
    return (queue->r_ptr_ != queue->w_ptr_) || queue->full_;
}

static inline int tx_que_is_full(tx_que_t *queue)
{
    return queue->full_;
}

static inline void tx_que_inc_r_ptr(tx_que_t *queue)
{
    if (++queue->r_ptr_ == queue->tail_) {
        queue->r_ptr_ = queue->head_;
    }
    if (queue->full_) {
        queue->full_ = 0;
    }
}

extern volatile gmac_tx_que_ent_t* tx_que_get_finished_job(struct net_device *dev);

extern volatile gmac_tx_que_ent_t* tx_que_get_idle_job(struct net_device *dev);

extern void tx_que_new_job(
    struct net_device *dev,
    volatile gmac_tx_que_ent_t *entry);

/* Bits for controlling the delays in the RGMII hardware */
#define SYS_CTRL_DELAY_RX_RST   (1 << (16 + 0))
#define SYS_CTRL_DELAY_RX_CE    (1 << (16 + 1))
#define SYS_CTRL_DELAY_RX_INC   (1 << (16 + 2))
#define SYS_CTRL_DELAY_RX_CLK   (1 << (16 + 3))

/* extern void post_phy_reset_action(struct net_device *dev); */
#endif        //  #if !defined(__GMAC_H__)

