/*
 * linux/arch/arm/mach-oxnas/gmac_phy.h
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
#if !defined(__GMAC_PHY_H__)
#define __GMAC_PHY_H__

#include <asm/types.h>
#include <linux/netdevice.h>
#include <linux/mii.h>
#include "gmac.h"

#define PHY_TYPE_NONE					0
#define PHY_TYPE_MICREL_KS8721BL		0x00221619
#define PHY_TYPE_VITESSE_VSC8201XVZ	0x000fc413
#define PHY_TYPE_REALTEK_RTL8211BGR	0x001cc912
#define PHY_TYPE_REALTEK_RTL8211D	0x001cc914
#define PHY_TYPE_LSI_ET1011C			0x0282f013
#define PHY_TYPE_LSI_ET1011C2			0x0282f014
#define PHY_TYPE_ICPLUS_IP1001_0		0x02430d90
#define PHY_TYPE_ICPLUS_IP1001_1		0x02430d91

#define VSC8201_MII_ACSR			0x1c	// Vitesse VCS8201 gigabit PHY Auxillary Control and Status register
#define VSC8201_MII_ACSR_MDPPS_BIT	2		// Mode/Duplex Pin Priority Select

#define ET1011C_MII_LOOPBACK_CNTL	0x13
#define ET1011C_MII_LOOPBACK_DIGITAL_LOOPBACK	12
#define ET1011C_MII_LOOPBACK_MII_LOOPBACK		15

#define ET1011C_MII_CONFIG	0x16
#define ET1011C_MII_CONFIG_IFMODESEL	0
#define ET1011C_MII_CONFIG_IFMODESEL_NUM_BITS	3
#define ET1011C_MII_CONFIG_SYSCLKEN	4
#define ET1011C_MII_CONFIG_TXCLKEN		5
#define ET1011C_MII_CONFIG_TBI_RATESEL	8
#define ET1011C_MII_CONFIG_CRS_TX_EN	15

#define ET1011C_MII_CONFIG_IFMODESEL_GMII_MII		0
#define ET1011C_MII_CONFIG_IFMODESEL_TBI			1
#define ET1011C_MII_CONFIG_IFMODESEL_GMII_MII_GTX	2
#define ET1011C_MII_CONFIG_IFMODESEL_RGMII_TRACE    4
#define ET1011C_MII_CONFIG_IFMODESEL_RGMII_DLL      6

#define ET1011C_MII_CONTROL	0x17
#define ET1011C_MII_CONTROL_ALT_RGMII_DELAY	    (1 << 6)

#define ET1011C_MII_LED2 0x1c
#define ET1011C_MII_LED2_LED_TXRX		12
#define ET1011C_MII_LED2_LED_NUM_BITS	4

#define ET1011C_MII_LED2_LED_TXRX_ON		0xe
#define ET1011C_MII_LED2_LED_TXRX_ACTIVITY	0x7

/* IC+ Phy */
#define IP1001LF_PHYSPECIFIC_CSR        16
#define IP1001LF_PHYSPECIFIC_CSR_RXPHASE_SEL    (1  << 0)
#define IP1001LF_PHYSPECIFIC_CSR_TXPHASE_SEL    (1  << 1)
#define IP1001LF_PHYSPECIFIC_CSR_DRIVE_MASK     (15 << 5)
#define IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_HI  (2  << 5)
#define IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_HI    (2  << 7)
#define IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_M   (1  << 5)
#define IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_M     (1  << 7)
#define IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_L   (0  << 5)
#define IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_L     (0  << 7)
#define IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_VL  (3  << 5)
#define IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_VL    (3  << 7)

extern spinlock_t phy_access_spinlock[CONFIG_OXNAS_MAX_GMAC_UNITS];

#define lock_mdio_bus(priv, flags) \
	do { \
		spin_lock_irqsave(&phy_access_spinlock[(priv)->unit], (flags)); \
	} while (0)

#define unlock_mdio_bus(priv, flags) \
	do { \
		spin_unlock_irqrestore(&phy_access_spinlock[(priv)->unit], (flags)); \
	} while (0)

extern int phy_read(struct net_device *dev, int phyaddr, int phyreg);

extern int phy_read_via_mac0(struct net_device *dev, int phyaddr, int phyreg);

extern void phy_write(struct net_device *dev, int phyaddr, int phyreg, int phydata);

extern void phy_write_via_mac0(struct net_device *dev, int phyaddr, int phyreg, int phydata);

extern void phy_detect(struct net_device *dev, int id);

extern int phy_reset(struct net_device *dev);

extern void phy_powerdown(struct net_device *dev);

extern void start_phy_reset(gmac_priv_t* priv);

extern int is_phy_reset_complete(gmac_priv_t* priv);

extern void set_phy_negotiate_mode(struct net_device *dev);

extern u32 get_phy_capabilies(gmac_priv_t* priv);

extern u32 get_phy_capabilies(gmac_priv_t* priv);
#endif        //  #if !defined(__GMAC_PHY_H__)

