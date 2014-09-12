/*
 * linux/arch/arm/mach-oxnas/gmac_phy.c
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
#include "gmac_phy.h"
#include "gmac_reg.h"
#include <asm/io.h>

static const int PHY_TRANSFER_TIMEOUT_MS = 100;

spinlock_t phy_access_spinlock[CONFIG_OXNAS_MAX_GMAC_UNITS];

int __init gmac_phy_init(void)
{
	int i;

	for (i=0; i < CONFIG_OXNAS_MAX_GMAC_UNITS; ++i) {
		spin_lock_init(&phy_access_spinlock[i]);
	}

	return 0;
}

/**
 *   Local PHY register accessors via a particular ethernet port (0/1)
 */
static inline void unit_mac_reg_write(int ethport, int reg_num, u32 value)
{
    writel(value, mac_base[ethport] + (reg_num << 2));
}

static inline u32 unit_mac_reg_read(int ethport, int reg_num)
{
    return readl(mac_base[ethport] + (reg_num << 2));
}

/*
 * Reads a register from the MII Management serial interface via the GMAC registers
 */
static inline int phy_read_via_mac_unit(
	int          unit,
	gmac_priv_t *priv,
	int          phyaddr,
	int          phyreg)
{
	unsigned long mask;
    int data = 0;
    unsigned long end;
    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (priv->gmii_csr_clk_range << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);


	spin_lock_irqsave(&phy_access_spinlock[priv->unit], mask);
    unit_mac_reg_write(unit, MAC_GMII_ADR_REG, addr);

    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);
    while (time_before(jiffies, end)) {
        if (!(unit_mac_reg_read(unit, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            // Successfully read from PHY
            data = unit_mac_reg_read(unit, MAC_GMII_DATA_REG) & 0xFFFF;
            break;
        }
    }
	spin_unlock_irqrestore(&phy_access_spinlock[priv->unit], mask);

    return data;
}

/*
 * Reads a register from the MII Management serial interface via the GMAC registers
 * This implementation uses whatever MAC is associated with the net_device object.
 */
int phy_read(struct net_device *dev, int phyaddr, int phyreg)
{
    int data = 0;
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    data = phy_read_via_mac_unit(priv->unit, priv, phyaddr, phyreg);
    DBG(1, KERN_INFO "phy_read() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n",
        dev->name, phyaddr, phyreg, data);
    return data;
}

/*
 * Reads a register from the MII Management serial interface via the GMAC registers.
 * This implementation always uses MAC 0, irrespective of the net_device object.
 */
int phy_read_via_mac0(struct net_device *dev, int phyaddr, int phyreg)
{
    int data = 0;
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    data = phy_read_via_mac_unit(0, priv, phyaddr, phyreg);
    DBG(1, KERN_INFO "phy_read_via_mac0() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n",
        dev->name, phyaddr, phyreg, data);
    return data;
}

/*
 * Writes a register to the MII Management serial interface
 */
static inline void phy_write_via_mac_unit(
	int          unit,
	gmac_priv_t *priv,
	int          phyaddr,
	int          phyreg,
	int          phydata)
{
	unsigned long mask;
    unsigned long end;
    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (priv->gmii_csr_clk_range << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GW_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);

	spin_lock_irqsave(&phy_access_spinlock[priv->unit], mask);
    unit_mac_reg_write(unit, MAC_GMII_DATA_REG, phydata);
    unit_mac_reg_write(unit, MAC_GMII_ADR_REG, addr);

    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);
    while (time_before(jiffies, end)) {
        if (!(unit_mac_reg_read(unit, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            // Successfully wrote to PHY
            break;
        }
    }
	spin_unlock_irqrestore(&phy_access_spinlock[priv->unit], mask);

}

/*
 * Writes a register to the MII Management serial interface via the GMAC's registers.
 * This implementation uses whatever MAC is associated with the net_device object.
 */
void phy_write(struct net_device *dev, int phyaddr, int phyreg, int phydata)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    phy_write_via_mac_unit(priv->unit, priv, phyaddr, phyreg, phydata);    
    DBG(1, KERN_INFO "phy_write() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n",
        dev->name, phyaddr, phyreg, phydata);
}

/*
 * Writes a register to the MII Management serial interface via the GMAC's registers.
 * This implementation always uses MAC 0, irrespective of the net_device object.
 */
void phy_write_via_mac0(struct net_device *dev, int phyaddr, int phyreg, int phydata)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    phy_write_via_mac_unit(0, priv, phyaddr, phyreg, phydata);    
    DBG(1, KERN_INFO "phy_write_via_mac0() %s: phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n",
        dev->name, phyaddr, phyreg, phydata);
}

static int find_phy(
	struct net_device *dev,
	int				   id,
	u32				  *type)
{
	int found = 0;
	unsigned int id1, id2;
    gmac_priv_t        *priv = (gmac_priv_t*)netdev_priv(dev);

	// Read the PHY identifiers
	id1 = priv->mii.mdio_read(dev, id, MII_PHYSID1);
	id2 = priv->mii.mdio_read(dev, id, MII_PHYSID2);

	DBG(2, KERN_INFO "phy_detect() %s: PHY adr = %u -> phy_id1=0x%x, phy_id2=0x%x\n", dev->name, id, id1, id2);

	// Make sure it is a valid identifier
	if (id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000 &&
		id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
		*type = id1 << 16 | id2;
		DBG(2, KERN_NOTICE "find_detect() %s: Found PHY at address = %u, type = %u\n", dev->name, id, *type);
		found = 1;
	}

	return found;
}

/*
 * Finds and reports the PHY address
 */
void phy_detect(
	struct net_device *dev,
	int				   id)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    DBG(2, KERN_INFO "phy_detect() %s: Entered\n", priv->netdev->name);

    priv->phy_type = PHY_TYPE_NONE;
	if (id != -1) {
		u32 phy_type;
		if (find_phy(priv->netdev, id, &phy_type)) {
			priv->mii.phy_id = id;
			priv->phy_type = phy_type;
		}
	} else {
		int phyaddr;
		// Scan all 31 PHY addresses if a PHY id wasn't specified
		for (phyaddr = 1; phyaddr < 32; ++phyaddr) {
			u32 phy_type;
			if (find_phy(priv->netdev, phyaddr, &phy_type)) {
				priv->mii.phy_id = phyaddr;
				priv->phy_type = phy_type;
				break;
			}
		}
	}
}

void start_phy_reset(gmac_priv_t* priv)
{
    // Ask the PHY to reset and allow autonegotiation (Realtek PHY requires
    // auto-neg to be explicitly enabled at this point)
    priv->mii.mdio_write(priv->netdev, priv->mii.phy_id,
    	MII_BMCR, BMCR_RESET | BMCR_ANRESTART | BMCR_ANENABLE);
}

int is_phy_reset_complete(gmac_priv_t* priv)
{
    int complete = 0;
    int bmcr;

    // Read back the status until it indicates reset, or we timeout
    bmcr = priv->mii.mdio_read(priv->netdev, priv->mii.phy_id, MII_BMCR);
    if (!(bmcr & BMCR_RESET)) {
        complete = 1;
    }

    return complete;
}

int phy_reset(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);
    int complete = 0;
    unsigned long end;

    // Start the reset operation
    start_phy_reset(priv);

    // Total time to wait for reset to complete
    end = jiffies + MS_TO_JIFFIES(PHY_TRANSFER_TIMEOUT_MS);

    // Read back the status until it indicates reset, or we timeout
    while (!(complete = is_phy_reset_complete(priv)) && time_before(jiffies, end)) {
        msleep(1);
    }

    /* if (complete) { */
    /*     	post_phy_reset_action(dev); */
    /* } */

    return !complete;
}

void phy_powerdown(struct net_device *dev)
{
    gmac_priv_t* priv = (gmac_priv_t*)netdev_priv(dev);

    unsigned int bmcr = priv->mii.mdio_read(dev, priv->mii.phy_id, MII_BMCR);
    priv->mii.mdio_write(dev, priv->mii.phy_id, MII_BMCR, bmcr | BMCR_PDOWN);

 	if ((priv->phy_type == PHY_TYPE_ICPLUS_IP1001_0) ||
		(priv->phy_type == PHY_TYPE_ICPLUS_IP1001_1)) {
		// Cope with weird ICPlus PHY behaviour
		priv->mii.mdio_read(dev, priv->mii.phy_id, MII_BMCR);
	}
}

void set_phy_negotiate_mode(struct net_device *dev)
{
    gmac_priv_t        *priv = (gmac_priv_t*)netdev_priv(dev);
    struct mii_if_info *mii = &priv->mii;
    struct ethtool_cmd *ecmd = &priv->ethtool_cmd;
    u32                 bmcr;

	bmcr = mii->mdio_read(dev, mii->phy_id, MII_BMCR);

    if (ecmd->autoneg == AUTONEG_ENABLE) {
        u32 advert, tmp;
        u32 advert2 = 0, tmp2 = 0;

//printk("set_phy_negotiate_mode() Auto negotiating link mode\n");
        // Advertise only what has been requested
        advert = mii->mdio_read(dev, mii->phy_id, MII_ADVERTISE);
        tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4 |
		                 ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);

        if (ecmd->supported & (SUPPORTED_1000baseT_Full | ADVERTISE_1000HALF)) {
            advert2 = mii->mdio_read(dev, mii->phy_id, MII_CTRL1000);
            tmp2 = advert2 & ~(ADVERTISE_1000HALF | ADVERTISE_1000FULL);
        }

		switch (ecmd->speed) {
			case SPEED_1000:
//				printk("set_phy_negotiate_mode() Allowing 1000M\n");
				if (mii->supports_gmii) {
					if (ecmd->advertising & ADVERTISED_1000baseT_Half)
						tmp2 |= ADVERTISE_1000HALF;
					if (ecmd->advertising & ADVERTISED_1000baseT_Full)
						tmp2 |= ADVERTISE_1000FULL;
				}
			case SPEED_100:
//				printk("set_phy_negotiate_mode() Allowing 100M\n");
				if (ecmd->advertising & ADVERTISED_100baseT_Half)
					tmp |= ADVERTISE_100HALF;
				if (ecmd->advertising & ADVERTISED_100baseT_Full)
					tmp |= ADVERTISE_100FULL;
			case SPEED_10:
//				printk("set_phy_negotiate_mode() Allowing 10M\n");
				if (ecmd->advertising & ADVERTISED_10baseT_Half)
					tmp |= ADVERTISE_10HALF;
				if (ecmd->advertising & ADVERTISED_10baseT_Full)
					tmp |= ADVERTISE_10FULL;
		}

        if (ecmd->advertising & ADVERTISED_Pause) {
            tmp |= ADVERTISE_PAUSE_CAP;
        }
        if (ecmd->advertising & ADVERTISED_Asym_Pause) {
            tmp |= ADVERTISE_PAUSE_ASYM;
        }

        if (advert != tmp) {
//printk("set_phy_negotiate_mode() Setting MII_ADVERTISE to 0x%08x\n", tmp);
            mii->mdio_write(dev, mii->phy_id, MII_ADVERTISE, tmp);
            mii->advertising = tmp;
        }
        if (advert2 != tmp2) {
//printk("set_phy_negotiate_mode() Setting MII_CTRL1000 to 0x%08x\n", tmp2);
            mii->mdio_write(dev, mii->phy_id, MII_CTRL1000, tmp2);
        }

        // Auto-negotiate the link state
        bmcr |= (BMCR_ANRESTART | BMCR_ANENABLE);
        mii->mdio_write(dev, mii->phy_id, MII_BMCR, bmcr);
    } else {
        u32 tmp;
//printk("set_phy_negotiate_mode() Unilaterally setting link mode\n");

        // Turn off auto negotiation, set speed and duplicitly unilaterally
        tmp = bmcr & ~(BMCR_ANENABLE | BMCR_SPEED100 | BMCR_SPEED1000 | BMCR_FULLDPLX);
        if (ecmd->speed == SPEED_1000) {
            tmp |= BMCR_SPEED1000;
        } else if (ecmd->speed == SPEED_100) {
            tmp |= BMCR_SPEED100;
        }

        if (ecmd->duplex == DUPLEX_FULL) {
            tmp |= BMCR_FULLDPLX;
            mii->full_duplex = 1;
        } else {
            mii->full_duplex = 0;
        }

        if (bmcr != tmp) {
            mii->mdio_write(dev, mii->phy_id, MII_BMCR, tmp);
        }
    }
}

u32 get_phy_capabilies(gmac_priv_t* priv)
{
    struct mii_if_info *mii = &priv->mii;

	// Ask the PHY for it's capabilities
	u32 reg = mii->mdio_read(priv->netdev, mii->phy_id, MII_BMSR);

	// Assume PHY has MII interface
	u32 features = SUPPORTED_MII;

	if (reg & BMSR_ANEGCAPABLE) {
		features |= SUPPORTED_Autoneg;
	}
	if (reg & BMSR_100FULL) {
		features |= SUPPORTED_100baseT_Full;
	}
	if (reg & BMSR_100HALF) {
		features |= SUPPORTED_100baseT_Half;
	}
	if (reg & BMSR_10FULL) {
		features |= SUPPORTED_10baseT_Full;
	}
	if (reg & BMSR_10HALF) {
		features |= SUPPORTED_10baseT_Half;
	}

	// Does the PHY have the extended status register?
	if (reg & BMSR_ESTATEN) {
		reg = mii->mdio_read(priv->netdev, mii->phy_id, MII_ESTATUS);

		if (reg & ESTATUS_1000_TFULL)
			features |= SUPPORTED_1000baseT_Full;
		if (reg & ESTATUS_1000_THALF)
			features |= SUPPORTED_1000baseT_Half;
	}

	return features;
}
