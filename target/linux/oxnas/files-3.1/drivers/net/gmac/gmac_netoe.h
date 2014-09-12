/*
 * linux/drivers/net/gmac/gmac_netoe.h
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
#if !defined(__GMAC_NETOE_H__)
#define __GMAC_NETOE_H__

#include <linux/skbuff.h>
#include <linux/kernel.h>
#include "gmac.h"

#define NETOE_MAX_JOBS 15
#define NETOE_MAX_FRAGS 31
#define NETOE_JOB_QUEUE_SIZE  4096
#define NETOE_ALIGNMENT  ~(NETOE_JOB_QUEUE_SIZE - 1)

typedef struct netoe_src_seg0
{
    u32               src_;
    u16               len_;
    u16               path_mtu_;    
}__attribute__ ((packed)) netoe_src_seg0_t;
    
typedef struct netoe_src_seg
{
    u32               src_;
    u16               len_;
    u16               reserved;    
}__attribute__ ((packed)) netoe_src_seg_t;
    

typedef struct netoe_job
{
    unsigned int   tot_len_:16;
    unsigned int    n_src_segs_:5;
    unsigned int    reserved:3;
    unsigned int    ip_ver_hdr_len_:5;
    unsigned int    udp_flag_:1;
    unsigned int    irq_flag_:1;
    unsigned int    tso_flag_:1;
    struct sk_buff  *skb_;
    netoe_src_seg0_t src_seg0_;
    netoe_src_seg_t  src_segs_[NETOE_MAX_FRAGS - 1];
    
}__attribute__ ((packed,aligned)) netoe_job_t;

/**
 *  Get the netoe on its feet and allocate a job queue
 */ 
extern void netoe_start(gmac_priv_t *priv);

/**
 *  Halt the netoe and deallocate the job queue
 */ 
extern void netoe_stop(gmac_priv_t *priv);

/**
 *  Set the MTU size in the NetOE
 */ 
extern void netoe_set_mtu(gmac_priv_t *priv, unsigned mtu);

/**
 *  Get a new job, pending addition to the queue.
 *  Returns either a valid job or NULL
 */ 
extern volatile netoe_job_t* netoe_get_free_job(gmac_priv_t *priv);

/**
 *  Add a new job to the queue
 */ 
extern void netoe_send_job(gmac_priv_t *priv, volatile netoe_job_t* job, struct sk_buff *skb);

/**
 *  Get a completed job, following completion from the queue.
 *  Returns either a valid job or NULL
 */ 
extern volatile netoe_job_t* netoe_get_completed_job(gmac_priv_t *priv);

/**
 *  Free the resources from a completed job
 */ 
extern void netoe_clear_job(gmac_priv_t *priv, volatile netoe_job_t* job);

/**
 *  Get the fill level of the job queue
 */ 
extern unsigned netoe_get_fill_level(gmac_priv_t *priv);

/**
 *  return non-zero if the network offload engine queue is full
 */ 
extern unsigned netoe_is_full(gmac_priv_t *priv);

/**
 *  get the number of bytes transmitted since the last reset 
 */ 
extern unsigned netoe_get_bytes(gmac_priv_t *priv);

/**
 *  get the number of packets transmitted since the last reset 
 */ 
extern unsigned netoe_get_packets(gmac_priv_t *priv);

/**
 *  get the number of transmit aborts since the last reset 
 */ 
extern unsigned netoe_get_aborts(gmac_priv_t *priv);

/**
 *  get the number of transmit collisions since the last reset 
 */ 
extern unsigned netoe_get_collisions(gmac_priv_t *priv);

/**
 *  get the number of transmit carrier errors since the last reset 
 */ 
extern unsigned netoe_get_carrier_errors(gmac_priv_t *priv);

/**
 *  get the netoe status
 */ 
extern unsigned long netoe_get_status(gmac_priv_t *priv);

#endif // __GMAC_NETOE_H

