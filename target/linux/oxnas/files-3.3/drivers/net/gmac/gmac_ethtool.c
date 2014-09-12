/*
 * linux/arch/arm/mach-oxnas/gmac_ethtool.c
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
 
#include <asm/types.h>
#include <linux/errno.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/io.h>
//#include <mach/leon.h>

//#define GMAC_DEBUG
#undef GMAC_DEBUG

#include "gmac.h"
#include "gmac_desc.h"

static int get_settings(struct net_device* dev, struct ethtool_cmd* cmd)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long irq_flags;
    int status;

    spin_lock_irqsave(&priv->phy_lock, irq_flags);
    status = mii_ethtool_gset(&priv->mii, cmd);
    spin_unlock_irqrestore(&priv->phy_lock, irq_flags);

    return status;
}

static int set_settings(struct net_device* dev, struct ethtool_cmd* cmd)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long irq_flags;
    int status;

    spin_lock_irqsave(&priv->phy_lock, irq_flags);
    status = mii_ethtool_sset(&priv->mii, cmd);
    spin_unlock_irqrestore(&priv->phy_lock, irq_flags);

	// Copy significant setting to our local store
	priv->ethtool_cmd.speed  = cmd->speed;
	priv->ethtool_cmd.duplex = cmd->duplex;
	priv->ethtool_cmd.autoneg = cmd->autoneg;

	// Force an autonegotiation next time the watchdog fires
	priv->phy_force_negotiation = 1;
    return status;
}

static void get_drvinfo(struct net_device* dev, struct ethtool_drvinfo* drvinfo)
{
    strncpy(drvinfo->driver,     "GMAC", 32);
    strncpy(drvinfo->version,    "1.0", 32);
    strncpy(drvinfo->fw_version, "1.0", 32);    // Version of CoPro s/w
    strncpy(drvinfo->bus_info,   "AMBA", 32);
}

static int nway_reset(struct net_device* dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long irq_flags;
    int status;

    spin_lock_irqsave(&priv->phy_lock, irq_flags);
    status = mii_nway_restart(&priv->mii);
    spin_unlock_irqrestore(&priv->phy_lock, irq_flags);

    return status;
}

static u32 get_msglevel(struct net_device* dev)
{
    return ((gmac_priv_t*)netdev_priv(dev))->msg_level;
}

static void set_msglevel(struct net_device* dev, u32 data)
{
    ((gmac_priv_t*)netdev_priv(dev))->msg_level = data;
}

static u32 get_rx_csum(struct net_device* dev)
{
#ifdef USE_RX_CSUM
	return 1;
#else
	return 0;
#endif
}

static int set_rx_csum(struct net_device* dev, u32 data)
{
    return 0;
}

static int get_regs_len(struct net_device* dev)
{
    return 0;
}

static void get_regs(struct net_device* dev, struct ethtool_regs* regs, void *p)
{
    gmac_priv_t   *priv = (gmac_priv_t*)netdev_priv(dev);
    unsigned long  irq_state;
	u32            status;

    printk("RX ring info:\n");
    printk("  num_descriptors     = %d\n", priv->rx_gmac_desc_list_info.num_descriptors);
    printk("  empty_count         = %d\n", priv->rx_gmac_desc_list_info.empty_count);
    printk("  full_count          = %d\n", priv->rx_gmac_desc_list_info.full_count);
    printk("  r_index             = %d\n", priv->rx_gmac_desc_list_info.r_index);
    printk("  w_index             = %d\n", priv->rx_gmac_desc_list_info.w_index);
    printk("  available_for_write = %d\n", available_for_write(&priv->rx_gmac_desc_list_info));
    printk("  available_for_read    %s\n", rx_available_for_read(&priv->rx_gmac_desc_list_info, 0) ? "yes" :"no");

    spin_lock_irqsave(&priv->tx_spinlock_, irq_state);
    printk("TX ring info:\n");
    printk("  num_descriptors     = %d\n", priv->tx_gmac_desc_list_info.num_descriptors);
    printk("  empty_count         = %d\n", priv->tx_gmac_desc_list_info.empty_count);
    printk("  full_count          = %d\n", priv->tx_gmac_desc_list_info.full_count);
    printk("  r_index             = %d\n", priv->tx_gmac_desc_list_info.r_index);
    printk("  w_index             = %d\n", priv->tx_gmac_desc_list_info.w_index);
    printk("  available_for_write = %d\n", available_for_write(&priv->tx_gmac_desc_list_info));
    printk("  available_for_read    %s\n", tx_available_for_read(&priv->tx_gmac_desc_list_info, &status) ? "yes" : "no");
    spin_unlock_irqrestore(&priv->tx_spinlock_, irq_state);
}

static void get_wol(struct net_device* dev, struct ethtool_wolinfo* wol_info)
{
}

static int set_wol(struct net_device* dev, struct ethtool_wolinfo* wol_info)
{
    return -EINVAL;
}

static void get_ringparam(struct net_device* dev, struct ethtool_ringparam *ethtool_ringparam)
{
}

static int set_ringparam(struct net_device* dev, struct ethtool_ringparam *ethtool_ringparam)
{
    return -EINVAL;
}

static void get_pauseparam(struct net_device* dev, struct ethtool_pauseparam* ethtool_pauseparam)
{
}

static int set_pauseparam(struct net_device* dev, struct ethtool_pauseparam* ethtool_pauseparam)
{
    return -EINVAL;
}

static int self_test_count(struct net_device* dev)
{
    return -EINVAL;
}

static void self_test(struct net_device* dev, struct ethtool_test* ethtool_test, u64 *data)
{
}

static void get_strings(struct net_device* dev, u32 stringset, u8 *data)
{
}

static int phys_id(struct net_device* dev, u32 data)
{
    return -EINVAL;
}

static int get_stats_count(struct net_device* dev)
{
    return -EINVAL;
}

static void get_ethtool_stats(struct net_device* dev, struct ethtool_stats* ethtool_stats, u64 *data)
{
}

static struct ethtool_ops ethtool_ops = {
    .get_settings      = get_settings,
    .set_settings      = set_settings,
    .get_drvinfo       = get_drvinfo,
    .get_regs_len      = get_regs_len,
    .get_regs          = get_regs,
    .get_wol           = get_wol,
    .set_wol           = set_wol,
    .get_msglevel      = get_msglevel,
    .set_msglevel      = set_msglevel,
    .nway_reset        = nway_reset,
    .get_link          = ethtool_op_get_link,
    .get_ringparam     = get_ringparam,
    .set_ringparam     = set_ringparam,
    .get_pauseparam    = get_pauseparam,
    .set_pauseparam    = set_pauseparam,
//    .get_rx_csum       = get_rx_csum,
//    .set_rx_csum       = set_rx_csum,
//    .get_tx_csum       = ethtool_op_get_tx_csum,
//    .set_tx_csum       = ethtool_op_set_tx_csum,
//    .get_sg            = ethtool_op_get_sg,
//    .set_sg            = ethtool_op_set_sg,
//    .get_tso           = ethtool_op_get_tso,
//    .set_tso           = ethtool_op_set_tso,
    //.self_test_count   = self_test_count,
    .self_test         = self_test,
    .get_strings       = get_strings,
    //.phys_id           = phys_id,
    //.get_stats_count   = get_stats_count,
    .get_ethtool_stats = get_ethtool_stats
};

void set_ethtool_ops(struct net_device *netdev)
{
    SET_ETHTOOL_OPS(netdev, &ethtool_ops);
}
