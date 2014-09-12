/*
 * linux/drivers/net/gmac/gmac_netoe.c
 *
 * Copyright (C) 2008 Oxford Semiconductor Ltd
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

#if defined(CONFIG_ARCH_OX820)
#include "gmac_netoe.h"
#include <asm/io.h>
#include "gmac.h"
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
//#define NETOE_DEBUG
//#define NETOE_DEBUG_JOB
//#define NETOE_DEBUG_SKB
//#define NETOE_DEBUG_DESC_LIST
//#define NETOE_DEBUG_QUEUE    


typedef enum gmac_mac_regs {
    NETOE_JOB_QUEUE_BASE    =  0,
    NETOE_JOB_QUEUE_FILL    =  1,
    NETOE_JOB_RPTR          =  2,
    NETOE_JOB_WPTR          =  3,
    NETOE_MTU               =  4,
    NETOE_CONTROL           =  5,
    NETOE_STATUS            =  6,
    NETOE_COMPLETE_COUNT    =  7,
    NETOE_TX_BYTES          =  8,
    NETOE_TX_PACKETS        =  9,
    NETOE_TX_ABORTS         = 10,
    NETOE_TX_COLLISIONS     = 11,
    NETOE_TX_CARRIER_ERRORS = 12,
    NETOE_TX_DESC_LIST_BASE = 1024
} netoe_regs_t;

static unsigned fill_level_;

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the NetOE register to access
 */
static inline u32 netoe_reg_read(gmac_priv_t* priv, int reg_num)
{
    return readl(priv->netoeBase + (reg_num << 2));
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the NetOE register to access
 */
static inline void netoe_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
    writel(value, priv->netoeBase + (reg_num << 2));
}


/**
 *  Notify the h/w queue that a new job is ready
 */
static inline void netoe_notify_new_job(gmac_priv_t *priv)
{
    netoe_reg_write(priv, NETOE_JOB_QUEUE_FILL, 1);    
}


/**
 *  get the number of transmit aborts since the last reset 
 */ 
static inline unsigned netoe_get_count(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_COMPLETE_COUNT);
}


/**
 *  Get the layer-4 (transport) protocol from an SKB. Return -1 if unsuccessful
 */ 
static inline int netoe_get_l4_protocol(struct sk_buff* skb)
{
    int protocol = -1;
    
    if (ntohs(skb->protocol) == ETH_P_IP) {
        protocol = ((struct iphdr*)skb_network_header(skb))->protocol;
            
    } else if  (ntohs(skb->protocol) == ETH_P_IPV6) {
        if (skb->sk) {                
            protocol = skb->sk->sk_protocol;            
        } else {
            DBG(1, KERN_ERR "Unexpected owner socket in IPv6 SKB, can't extract protocol information.\n");
            // TODO - If this message is ever observed, some new strategy may be required.
            // It currently looks like the ipv6 output code sets the sk_protocol field, 
            // but if it doesn't then we may need to chew through the IPv6 header,
            // or else modify the socket/skb creation to explicitly mark the protocol.
        }
    }

    return protocol;
    
}

/**
 *  dump the NETOE's desc list to stdio
 */ 
static inline void netoe_desc_dump(gmac_priv_t *priv)
{
    unsigned i;
    printk("Tx Desc List\n");
    for (i=0; i < 32; i+=4) {
        printk("%#8.8x,", netoe_reg_read(priv, NETOE_TX_DESC_LIST_BASE+i));
        printk("%#8.8x,", netoe_reg_read(priv, NETOE_TX_DESC_LIST_BASE+i+1));
        printk("%#8.8x,", netoe_reg_read(priv, NETOE_TX_DESC_LIST_BASE+i+2));
        printk("%#8.8x", netoe_reg_read(priv, NETOE_TX_DESC_LIST_BASE+i+3));
        printk("\n");
    }    
}

/**
 *  dump the NETOE's mac header to stdio
 */ 
static inline void netoe_mac_dump(gmac_priv_t *priv)
{
    unsigned i;
    printk("Mac hdr\n");
    for (i=0; i < 16; i+=4) {
        printk("%#8.8x,", netoe_reg_read(priv, NETOE_TX_DESC_LIST_BASE+ 1024 + i));
        printk("\n");
    }    
}

    
/**
 *  decode the skb contents and report to stdio
 */ 
static inline void netoe_skb_dump(struct sk_buff* skb)
{
    int nr_frags = skb_shinfo(skb)->nr_frags;
    int protocol = -1;
    
    if ((ntohs(skb->protocol) == ETH_P_IP) || (ntohs(skb->protocol) == ETH_P_IPV6)) {
        protocol = netoe_get_l4_protocol(skb);
        switch(protocol) {
            case 1:
                printk("ICMP job ");
                break;
            case 2:
                printk("IGMP job ");
                break;
            case 6:
                printk("TCP job ");
                break;
            case 17:
                printk("UDP job ");
                break;
            case 58:
                printk("IPv6-ICMP job ");
                break;
            default:
                printk("Unknown IP job, ip hdr protocol = %#2.2x ",protocol);
                break;        
        }; // switch
        
    } else if (ntohs(skb->protocol) == ETH_P_ARP) {
        printk("ARP job ");
    } else {
        printk("Unknown eth job, protocol = %#x ",ntohs(skb->protocol));
    }
    printk("len = %d bytes, frags = %d\n",skb->len, nr_frags);
}

/**
 *  dump the skb header contents to stdio
 */ 
static inline void netoe_skb_header_dump(struct sk_buff* skb)
{
    unsigned i;
    for (i = 0; i < ((skb_headlen(skb) & (~3))); i+=4) {
        u8* dat = &skb->data[i];        
        printk("0x%2.2x",dat[0]);
        printk("%2.2x",dat[1]);
        printk("%2.2x",dat[2]);
        printk("%2.2x",dat[3]);
        if ((i % 32) == 28){
            printk("\n");  
        } else {
            printk(",");  
        }        
    }

    if (i != skb_headlen(skb)) {        
        printk("0x");
        for (; i < (skb_headlen(skb)); i++) {
            u8* dat = &skb->data[i];
            printk("%2.2x",*dat);
        }
    }

    printk("\n");
}


/**
 *  dump the job contents to stdio
 */ 
static inline void netoe_job_dump(gmac_priv_t *priv, volatile netoe_job_t* job)
{
    u32* tmp = (u32*) job;
    unsigned i;
    printk("Job dump @ %lx:\n", (unsigned long)job);    
    printk("%#x\n", *tmp++);
    printk("%#x\n", *tmp++);
    for (i=0; i < NETOE_MAX_FRAGS; i++) {
        printk("%#x\n", *tmp++);
        printk("%#x\n", *tmp++);
    }
   
}

/**
 *  dump the job contents to stdio
 */ 
static inline void netoe_reg_dump(gmac_priv_t *priv)
{
    printk("MTU: %#x\n",netoe_reg_read(priv, NETOE_MTU) & 0xFFFF);
    printk("read pointer:  %#x\n",netoe_reg_read(priv, NETOE_JOB_RPTR));
    printk("write pointer: %#x\n",netoe_reg_read(priv, NETOE_JOB_WPTR));
    printk("State: %#x\n",netoe_reg_read(priv, NETOE_STATUS));
    printk("Bytes: %d\n",netoe_reg_read(priv, NETOE_TX_BYTES));
    printk("Packets: %d\n",netoe_reg_read(priv, NETOE_TX_PACKETS));
    printk("Aborts: %d\n",netoe_reg_read(priv, NETOE_TX_ABORTS));
    printk("Collisions: %d\n",netoe_reg_read(priv, NETOE_TX_COLLISIONS));
    printk("Carrier Errors: %d\n",netoe_reg_read(priv, NETOE_TX_CARRIER_ERRORS));
    printk("State: %#x\n",netoe_reg_read(priv, NETOE_STATUS));
    printk("netoeBase @: %#x\n",priv->netoeBase);
}

/**
 *  Align queue base address to correct (next 4K) boundary
 */
static inline unsigned long netoe_align_queue_base(unsigned long address)
{
    return ( (address + NETOE_JOB_QUEUE_SIZE - 1) & NETOE_ALIGNMENT);
}

/**
 *  Get the netoe on its feet and allocate a job queue
 */ 
void netoe_start(gmac_priv_t *priv)
{
    printk("netoe_start\n");
	// Make SMP safe - we could do with making this locking more fine grained
	spin_lock(&priv->tx_spinlock_);

    /* Allocate memory for the job list*/
    if (!(priv->netoe_job_list = (unsigned) dma_alloc_coherent(0,NETOE_JOB_QUEUE_SIZE*2, &priv->queue_pa_,GFP_KERNEL))) {
        netoe_stop(priv);
    
        // Make SMP safe - we could do with making this locking more fine grained
        spin_unlock(&priv->tx_spinlock_);
        return;
        
    }

    /* set up the job queue base register - must be aligned on a 4096 byte boundary */
    priv->queue_base_ = netoe_align_queue_base(priv->queue_pa_);

#ifdef NETOE_DEBUG    
    printk("Queue base @ %#x, pa @  %#x,  malloc @%#x\n",(unsigned)priv->queue_base_, (unsigned)priv->queue_pa_, (unsigned)priv->netoe_job_list);
#endif    

    netoe_reg_write(priv, NETOE_JOB_QUEUE_BASE, (priv->queue_base_ /*& ~(0x40000000)*/));
    
    // Enable transmit job processing
    netoe_reg_write(priv, NETOE_CONTROL, 1);

    // Set the completed pointer to the start of the queue 
    priv->completed_tx_ptr_ = netoe_align_queue_base(priv->netoe_job_list); 
    
    // Set the free pointer to the start of the queue 
    priv->free_tx_ptr_ = netoe_align_queue_base(priv->netoe_job_list);

    // Clear out the status summary
    priv->cumulative_status_ = 0;
    
	// Make SMP safe - we could do with making this locking more fine grained
	spin_unlock(&priv->tx_spinlock_);
 }


/**
 *  Halt the netoe and deallocate the job queue
 */ 
void netoe_stop(gmac_priv_t *priv)
{
    printk("netoe_stop\n");
    
	// Make SMP safe - we could do with making this locking more fine grained
	spin_lock(&priv->tx_spinlock_);

    // Disable transmit job processing
    netoe_reg_write(priv, NETOE_CONTROL, 0);
    
    /* Release memory for the job list*/
    dma_free_coherent(0, NETOE_JOB_QUEUE_SIZE*2, (void*)(priv->netoe_job_list), priv->queue_pa_);
    
	// Make SMP safe - we could do with making this locking more fine grained
	spin_unlock(&priv->tx_spinlock_);
}

/**
 *  Set the MTU size in the NetOE
 */ 
void netoe_set_mtu(gmac_priv_t *priv, unsigned mtu)
{
    netoe_reg_write(priv, NETOE_MTU, mtu);
}

/**
 *  Get a new job, pending addition to the queue.
 *  Returns either a valid job or NULL
 */ 
volatile netoe_job_t* netoe_get_free_job(gmac_priv_t *priv)
{
    volatile netoe_job_t* job;
        
    // If the queue is too full, return NULL
#if 0
    if (!(netoe_get_fill_level(priv) < NETOE_MAX_JOBS)){
        return NULL;        
    }
#endif
    if (fill_level_ == NETOE_MAX_JOBS){
        return NULL;        
    }
    
    job = (volatile netoe_job_t*)(priv->free_tx_ptr_);

    // Increment the completed job pointer
    priv->free_tx_ptr_ += sizeof(netoe_job_t);
    if (priv->free_tx_ptr_ == (netoe_align_queue_base(priv->netoe_job_list) + NETOE_JOB_QUEUE_SIZE)) {
        priv->free_tx_ptr_ = netoe_align_queue_base(priv->netoe_job_list);
    }

    // Clear out the header of the job struct.
    memset((void*)job, 0, 4);

    fill_level_++;
    
    return job;
    
}

#define SKB_RING_LEN 256
static struct sk_buff* skb_ring[SKB_RING_LEN];
static struct sk_buff** skb_push_p = &skb_ring[0];
static struct sk_buff** skb_pop_p = &skb_ring[0];

static inline void skb_push_ring(struct sk_buff *skb)
{
    if (*skb_push_p) {
        printk(KERN_WARNING "skb_push_ring: entry not empty @ %p, skb %p\n",skb_push_p,*skb_push_p);
    } else {
        *skb_push_p = skb;
    }
    //Increment and wrap the push pointer
    skb_push_p++;
    if (skb_push_p == &skb_ring[SKB_RING_LEN]) {
        skb_push_p =  &skb_ring[0];
    }
    
}

static inline void skb_pop_ring(struct sk_buff *skb)
{
    if (skb != *skb_pop_p) {
        printk(KERN_WARNING "skb_pop_ring: entry did not match @ %p, got %p, expected %p\n",skb_pop_p,*skb_pop_p,skb);
    } else {
        *skb_pop_p = NULL;        
    }
    //Increment and wrap the pop pointer
    skb_pop_p++;
    if (skb_pop_p == &skb_ring[SKB_RING_LEN]) {
        skb_pop_p =  &skb_ring[0];
    }

}



/**
 *  Add a new job to the queue
 */ 
void netoe_send_job(gmac_priv_t *priv, volatile netoe_job_t* job, struct sk_buff *skb)
{
    int i;
    int nr_frags = skb_shinfo(skb)->nr_frags;
    dma_addr_t hdr_dma_address;
    int protocol;
#ifdef NETOE_DEBUG_QUEUE    
    skb_push_ring(skb);
#endif    
    // if too many fragments call sbk_linearize()
    // and take the CPU memory copies hit 
    if (nr_frags > (NETOE_MAX_FRAGS - 1)) {
        int err;
        printk(KERN_WARNING "Fill: linearizing socket buffer as required %d frags and have only %d\n", nr_frags, NETOE_MAX_FRAGS-1);
        err = skb_linearize(skb);
        if (err) {
            panic("Fill: No free memory");
        }

        // update nr_frags
        nr_frags = skb_shinfo(skb)->nr_frags;
    }
    
    //Fill in the job details
    job->n_src_segs_ = nr_frags + 1;
    job->tot_len_ = skb->len;
    job->skb_ = skb;
    job->irq_flag_ = 1;
    job->udp_flag_ = 0;
    job->ip_ver_hdr_len_ = 0;
    job->tso_flag_ = 0;
    
    
#ifdef NETOE_DEBUG_SKB
    printk("New       job %p ",job);
    netoe_skb_dump(skb);    
    netoe_skb_header_dump(skb);
#endif
    // Get a DMA mapping of the packet's header data
    hdr_dma_address = dma_map_single(0, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
    BUG_ON(dma_mapping_error(0, hdr_dma_address));

    job->src_seg0_.src_ = hdr_dma_address;
    job->src_seg0_.len_ = skb_headlen(skb);
    job->src_seg0_.path_mtu_ = 0;

    protocol = netoe_get_l4_protocol(skb);
    
    if (skb->ip_summed == CHECKSUM_PARTIAL) {
        if (ntohs(skb->protocol) == ETH_P_IP) {
            if (protocol == IPPROTO_UDP) {
                job->udp_flag_ = 1;
                job->tso_flag_ = 1;
            } else if (protocol == IPPROTO_TCP){
                unsigned hdr_length =  tcp_hdrlen(skb) + skb_network_header_len(skb) ;
                
                job->udp_flag_ = 0;
                job->tso_flag_ = 1;
                // gso size (if set) reflects the mss over the path, but the netoe
                // uses mtu over the path, so correct for the difference.
                if (skb_shinfo(skb)->gso_size) {                    
                    job->src_seg0_.path_mtu_ = (skb_shinfo(skb)->gso_size) + hdr_length;
                }
                 
            } else {
                job->udp_flag_ = 0;            
            }        
        }
     }
    
    if  (ntohs(skb->protocol) == ETH_P_IPV6) {
        job->ip_ver_hdr_len_ = ((unsigned)(skb_transport_header(skb))
                                - ((unsigned)skb_network_header(skb)))/4;
        if (protocol  == IPPROTO_UDP) {
            job->udp_flag_ = 1;
            job->tso_flag_ = 0; // Hardware workaround. UDP segmentation isn't
                                // curretly supported.
        } else if (protocol == IPPROTO_TCP){
            unsigned hdr_length =  tcp_hdrlen(skb) + skb_network_header_len(skb) ;
            job->udp_flag_ = 0;
            job->tso_flag_ = 1; 
            // gso size (if set) reflects the mss over the path, but the netoe
            // uses mtu over the path, so correct for the difference.
            if (skb_shinfo(skb)->gso_size) {                    
                job->src_seg0_.path_mtu_ = (skb_shinfo(skb)->gso_size) + hdr_length;
            }

        }        
    }

       
    
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
            job->src_segs_[i].src_ = virt_to_dma(0, page_address(frag->page.p) + frag->page_offset);
        } else {
#endif  // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN           
            job->src_segs_[i].src_ = dma_map_page(0, frag->page.p, frag->page_offset, frag->size, DMA_TO_DEVICE);
#ifdef CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
        }
#endif  // CONFIG_OXNAS_GMAC_AVOID_CACHE_CLEAN
        job->src_segs_[i].len_ = frag->size;
    }

    // Increment the job reference counter. We use this to figure out how many jobs
    // may have completed when the IRQ lands.
    //priv->job_count_++;

    // Ensure all prior writes to the network offload engine have
    // completed before notifying the hardware
    wmb();

    // Notify the hardware that a new job is ready.
    netoe_notify_new_job(priv);

#ifdef NETOE_DEBUG_JOB
    printk("src[0] = %#x\n",hdr_dma_address);
    netoe_job_dump(priv, job);
#endif
    
#ifdef NETOE_DEBUG_DESC_LIST
    netoe_reg_dump(priv);
    netoe_desc_dump(priv);
#endif

    // Read and update the status summary. The bits are sticky, i.e. reading
    // resets the bit fields, so we need to keep track of this by
    // orring into a status summary field. 
    priv->cumulative_status_ |= (netoe_reg_read(priv, NETOE_STATUS) & 0x300);
    if (priv->cumulative_status_){
        printk("Bad netoe status in alloc_job(): %#3.3x\n",priv->cumulative_status_);
        netoe_reg_dump(priv);
        netoe_mac_dump(priv);
        netoe_job_dump(priv, job);
        netoe_skb_dump(skb);    
        netoe_skb_header_dump(skb);
        
    }
}

/**
 *  Free the resources from a completed job
 */ 
void netoe_clear_job(gmac_priv_t *priv, volatile netoe_job_t* job)
{
    int i;
    struct sk_buff* skb = (struct sk_buff*)job->skb_;
    
    //printk("Completed job %p\n",skb);
    int nr_frags = skb_shinfo(skb)->nr_frags;

#ifdef NETOE_DEBUG_QUEUE    
    skb_pop_ring(skb);
#endif
    
#ifdef NETOE_DEBUG_SKB
    netoe_skb_dump(skb);
#endif

    // This should never happen, since we check space when we filled
    // the job in netoe_alloc_job
    if (nr_frags > (NETOE_MAX_FRAGS - 1) ) {
        panic("Free: Insufficient fragment storage, required %d, have only %d",
              nr_frags, NETOE_MAX_FRAGS - 1);
    }

    // Release the DMA mapping for the data directly referenced by the SKB
    dma_unmap_single(0, job->src_seg0_.src_, skb_headlen(skb), DMA_TO_DEVICE);

    // Release the DMA mapping for any fragments in the first level fragment
    // info. storage within the job structure
    for (i=0; i < nr_frags; ++i) {
        dma_unmap_page(0, job->src_segs_[i].src_, job->src_segs_[i].len_, DMA_TO_DEVICE);
    }

    // Inform the network stack that we've finished with the packet
    dev_kfree_skb_irq(skb);

}

/**
 *  Get a completed job, following completion from the queue.
 *  Returns either a valid job or NULL
 */
volatile netoe_job_t* netoe_get_completed_job(gmac_priv_t *priv)
{
    volatile netoe_job_t* job;

    unsigned count;
    
    // Get the job count.
    count = netoe_get_count(priv);

    if (count == 16) {
        printk(KERN_WARNING "NetOE count was saturated. We missed some jobs...\n");        
    }
    
    priv->job_count_ += count;
    
    if (!priv->job_count_){
        return NULL;   
    }

    if (!fill_level_){
        return NULL;        
    }
    
    // Get the job object
    job = (volatile netoe_job_t*)(priv->completed_tx_ptr_);

    // Decrement the job reference count
    priv->job_count_--;
    //Descrement the fill level
    fill_level_--;
    
    // Increment the completed job pointer
    priv->completed_tx_ptr_ += sizeof(netoe_job_t);
    if (priv->completed_tx_ptr_ == (netoe_align_queue_base(priv->netoe_job_list) + NETOE_JOB_QUEUE_SIZE)) {
        priv->completed_tx_ptr_ = netoe_align_queue_base(priv->netoe_job_list);
    }
    
    return job;
    
}

/**
 *  return non-zero if the network offload engine queue is full
 */ 
unsigned netoe_is_full(gmac_priv_t *priv)
{
    return (fill_level_ == NETOE_MAX_JOBS);    
}

/**
 *  get the number of bytes transmitted since the last reset 
 */ 
unsigned netoe_get_bytes(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_TX_BYTES);
}

/**
 *  get the number of packets transmitted since the last reset 
 */ 
unsigned netoe_get_packets(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_TX_PACKETS);
}

/**
 *  get the number of transmit aborts since the last reset 
 */ 
unsigned netoe_get_aborts(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_TX_ABORTS);
}

/**
 *  get the number of transmit collisions since the last reset 
 */ 
unsigned netoe_get_collisions(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_TX_COLLISIONS);
}

/**
 *  get the number of transmit carrier errors since the last reset 
 */ 
unsigned netoe_get_carrier_errors(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_TX_CARRIER_ERRORS);
}

/**
 *  get the netoe status
 */ 
unsigned long netoe_get_status(gmac_priv_t *priv)
{
    return netoe_reg_read(priv, NETOE_STATUS);
}
#endif
