/*
 * (C) Copyright 2005
 * Oxford Semiconductor Ltd
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <malloc.h>
#include <net.h>
#include <asm/barrier.h>

//#define DEBUG_GMAC_INIT
//#define DEBUG_GMAC

// The number of bytes wasted at the start of a received packet buffer in order
// to ensure the IP header will be aligned to a 32-bit boundary
static const int ETHER_FRAME_ALIGN_WASTAGE = 2;
static const int EXTRA_RX_SKB_SPACE = 32;   // Otherwise GMAC spans over >1 skb
static const int ETHER_MTU  = 1500;

static const u32 MAC_BASE_OFFSET = 0x0000;
static const u32 DMA_BASE_OFFSET = 0x1000;

static const int NUM_TX_DMA_DESCRIPTORS = 1;
static const int NUM_RX_DMA_DESCRIPTORS = 32;

/* Generic MII registers. */
#define MII_BMCR            0x00	/* Basic mode control register */
#define MII_BMSR            0x01	/* Basic mode status register  */
#define MII_PHYSID1         0x02	/* PHYS ID 1                   */
#define MII_PHYSID2         0x03	/* PHYS ID 2                   */
#define MII_ADVERTISE       0x04	/* Advertisement control reg   */
#define MII_LPA             0x05	/* Link partner ability reg    */
#define MII_EXPANSION       0x06	/* Expansion register          */
#define MII_CTRL1000        0x09	/* 1000BASE-T control          */
#define MII_STAT1000        0x0a	/* 1000BASE-T status           */
#define MII_ESTATUS         0x0f	/* Extended Status */

/* Basic mode control register. */
#define BMCR_RESV          0x003f  /* Unused...                   */
#define BMCR_SPEED1000		0x0040  /* MSB of Speed (1000)         */
#define BMCR_CTST          0x0080  /* Collision test              */
#define BMCR_FULLDPLX      0x0100  /* Full duplex                 */
#define BMCR_ANRESTART     0x0200  /* Auto negotiation restart    */
#define BMCR_ISOLATE       0x0400  /* Disconnect DP83840 from MII */
#define BMCR_PDOWN         0x0800  /* Powerdown the DP83840       */
#define BMCR_ANENABLE      0x1000  /* Enable auto negotiation     */
#define BMCR_SPEED100      0x2000  /* Select 100Mbps              */
#define BMCR_LOOPBACK      0x4000  /* TXD loopback bits           */
#define BMCR_RESET         0x8000  /* Reset the DP83840           */

/* Basic mode status register. */
#define BMSR_ERCAP         0x0001  /* Ext-reg capability          */
#define BMSR_JCD           0x0002  /* Jabber detected             */
#define BMSR_LSTATUS       0x0004  /* Link status                 */
#define BMSR_ANEGCAPABLE   0x0008  /* Able to do auto-negotiation */
#define BMSR_RFAULT        0x0010  /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE  0x0020  /* Auto-negotiation complete   */
#define BMSR_RESV          0x00c0  /* Unused...                   */
#define BMSR_ESTATEN		0x0100	/* Extended Status in R15 */
#define BMSR_100FULL2		0x0200	/* Can do 100BASE-T2 HDX */
#define BMSR_100HALF2		0x0400	/* Can do 100BASE-T2 FDX */
#define BMSR_10HALF        0x0800  /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL        0x1000  /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF       0x2000  /* Can do 100mbps, half-duplex */
#define BMSR_100FULL       0x4000  /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4      0x8000  /* Can do 100mbps, 4k packets  */

/* Link partner ability register (what the other end of the cable con do)*/
#define LPA_100BASE4        (1 << 9)  /* 100M T4 */
#define LPA_100FULL         (1 << 8)  /* 100M TX Full duplex */
#define LPA_100HALF         (1 << 7)  /* 100M TX Half duplex*/
#define LPA_10FULL          (1 << 6)  /* 10M Full duplex */
#define LPA_10HALF          (1 << 5)  /* 10M Half duplex */


/* 1000BASE-T Status register */
#define LPA_1000LOCALRXOK	0x2000  /* Link partner local receiver status */
#define LPA_1000REMRXOK  	0x1000  /* Link partner remote receiver status */
#define LPA_1000FULL     	0x0800  /* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF     	0x0400  /* Link partner 1000BASE-T half duplex */

#define PHY_TYPE_NONE					0
#define PHY_TYPE_MICREL_KS8721BL		0x00221619
#define PHY_TYPE_VITESSE_VSC8201XVZ	0x000fc413
#define PHY_TYPE_REALTEK_RTL8211BGR	0x001cc912
#define PHY_TYPE_REALTEK_RTL8211D       0x001cc914
#define PHY_TYPE_LSI_ET1011C			0x0282f013
#define PHY_TYPE_LSI_ET1011C2			0x0282f014
#define PHY_TYPE_ICPLUS_IP1001LF_0        0x02430d90
#define PHY_TYPE_ICPLUS_IP1001LF_1        0x02430d91
/* Specific PHY values */
#define VSC8201_MII_MCR			    0x00	// Vitesse VCS8201 gigabit PHY mode control register
#define VSC8201_MII_MCRLOOPBACK_BIT 14	    // Phy loopback mode.
#define VSC8201_MII_ACSR			0x1c	// Vitesse VCS8201 gigabit PHY Auxillary Control and Status register
#define VSC8201_MII_ACSR_MDPPS_BIT	2		// Mode/Duplex Pin Priority Select

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


#if (NAS_VERSION == 820)
//#define DESCRIPTOR_VERSION 810 
#define DESCRIPTOR_VERSION 820 
#else
#define DESCRIPTOR_VERSION 810 
#endif

// Some typedefs to cope with std Linux types
typedef void sk_buff_t;

// The in-memory descriptor structures
typedef struct gmac_dma_desc
{
    /** The encoded status field of the GMAC descriptor */
    u32 status;
    /** The encoded length field of GMAC descriptor */
    u32 length;
    /** Buffer 1 pointer field of GMAC descriptor */
    u32 buffer1;
    /** Buffer 2 pointer or next descriptor pointer field of GMAC descriptor */
    u32 buffer2;
    /** Not used for U-Boot */
    u32 skb;
} __attribute ((packed)) gmac_dma_desc_t;

typedef struct gmac_desc_list_info
{
    gmac_dma_desc_t* base_ptr;
    int              num_descriptors;
    int              empty_count;
    int              full_count;
    int              r_index;
    int              w_index;
} gmac_desc_list_info_t;

typedef enum {
    link_is_disconnected,
    link_is_10M,
    link_is_100M,
    link_is_1000M,
} enum_link_speed;

// Private data structure for the GMAC driver
typedef struct gmac_priv
{
    /** Base address of GMAC MAC registers */
    u32                     macBase;
    /** Base address of GMAC DMA registers */
    u32                     dmaBase;

    /** The number of descriptors in the gmac_dma_desc_t array holding both the
     *  TX and RX descriptors. The TX descriptors reside at the start of the
     *  array */
    unsigned                total_num_descriptors;

    /** The address of the start of the descriptor array */
    gmac_dma_desc_t        *desc_base_addr;

    /** Descriptor list management */
    gmac_desc_list_info_t   tx_gmac_desc_list_info;
    gmac_desc_list_info_t   rx_gmac_desc_list_info;

	/** PHY info */
	u32						 phy_type;
	u32						 phy_addr;
	int						 phy_id;
    enum_link_speed          link_speed;
//	int						 link_is_1000M;
} gmac_priv_t;

/**
 * MAC register indices
 */
typedef enum gmac_mac_regs {
    MAC_CONFIG_REG       = 0,
    MAC_FRAME_FILTER_REG = 1,
    MAC_HASH_HIGH_REG    = 2,
    MAC_HASH_LOW_REG     = 3,
    MAC_GMII_ADR_REG     = 4,
    MAC_GMII_DATA_REG    = 5,
    MAC_FLOW_CNTL_REG    = 6,
    MAC_VLAN_TAG_REG     = 7,
    MAC_VERSION_REG      = 8,
    MAC_ADR0_HIGH_REG    = 16,
    MAC_ADR0_LOW_REG     = 17,
    MAC_ADR1_HIGH_REG    = 18,
    MAC_ADR1_LOW_REG     = 19,
    MAC_ADR2_HIGH_REG    = 20,
    MAC_ADR2_LOW_REG     = 21,
    MAC_ADR3_HIGH_REG    = 22,
    MAC_ADR3_LOW_REG     = 23,
    MAC_ADR4_HIGH_REG    = 24,
    MAC_ADR4_LOW_REG     = 25,
    MAC_ADR5_HIGH_REG    = 26,
    MAC_ADR5_LOW_REG     = 27,
    MAC_ADR6_HIGH_REG    = 28,
    MAC_ADR6_LOW_REG     = 29,
    MAC_ADR7_HIGH_REG    = 30,
    MAC_ADR7_LOW_REG     = 31,
    MAC_ADR8_HIGH_REG    = 32,
    MAC_ADR8_LOW_REG     = 33,
    MAC_ADR9_HIGH_REG    = 34,
    MAC_ADR9_LOW_REG     = 35,
    MAC_ADR10_HIGH_REG   = 36,
    MAC_ADR10_LOW_REG    = 37,
    MAC_ADR11_HIGH_REG   = 38,
    MAC_ADR11_LOW_REG    = 39,
    MAC_ADR12_HIGH_REG   = 40,
    MAC_ADR12_LOW_REG    = 41,
    MAC_ADR13_HIGH_REG   = 42,
    MAC_ADR13_LOW_REG    = 43,
    MAC_ADR14_HIGH_REG   = 44,
    MAC_ADR14_LOW_REG    = 45,
    MAC_ADR15_HIGH_REG   = 46,
    MAC_ADR15_LOW_REG    = 47
} gmac_mac_regs_t;


/**
 * MAC register field definitions
 */
typedef enum gmac_config_reg {
    MAC_CONFIG_TC_BIT  = 24,
    MAC_CONFIG_WD_BIT  = 23,
    MAC_CONFIG_JD_BIT  = 22,
    MAC_CONFIG_BE_BIT  = 21,
    MAC_CONFIG_JE_BIT  = 20,
    MAC_CONFIG_IFG_BIT = 17,
    MAC_CONFIG_PS_BIT  = 15,
    MAC_CONFIG_FES_BIT = 14,
    MAC_CONFIG_DO_BIT  = 13,
    MAC_CONFIG_LM_BIT  = 12,
    MAC_CONFIG_DM_BIT  = 11,
    MAC_CONFIG_IPC_BIT = 10,
    MAC_CONFIG_DR_BIT  = 9,
    MAC_CONFIG_ACS_BIT = 7,
    MAC_CONFIG_BL_BIT  = 5,
    MAC_CONFIG_DC_BIT  = 4,
    MAC_CONFIG_TE_BIT  = 3,
    MAC_CONFIG_RE_BIT  = 2
} gmac_config_reg_t;

#define MAC_CONFIG_IFG_NUM_BITS 3
#define MAC_CONFIG_BL_NUM_BITS 2

typedef enum gmac_frame_filter_reg {
    MAC_FRAME_FILTER_RA_BIT   = 31,
    MAC_FRAME_FILTER_SAF_BIT  = 9,
    MAC_FRAME_FILTER_SAIF_BIT = 8,
    MAC_FRAME_FILTER_PCF_BIT  = 6,
    MAC_FRAME_FILTER_DBF_BIT  = 5,
    MAC_FRAME_FILTER_PM_BIT   = 4,
    MAC_FRAME_FILTER_DAIF_BIT = 3,
    MAC_FRAME_FILTER_HMC_BIT  = 2,
    MAC_FRAME_FILTER_HUC_BIT  = 1,
    MAC_FRAME_FILTER_PR_BIT   = 0
} gmac_frame_filter_reg_t;

#define MAC_FRAME_FILTER_PCF_NUM_BITS 2

typedef enum gmac_hash_table_high_reg {
    MAC_HASH_HIGH_HTH_BIT = 0
} gmac_hash_table_high_reg_t;

typedef enum gmac_hash_table_low_reg {
    MAC_HASH_LOW_HTL_BIT = 0
} gmac_hash_table_low_reg_t;

typedef enum gmac_gmii_address_reg {
    MAC_GMII_ADR_PA_BIT = 11,
    MAC_GMII_ADR_GR_BIT = 6,
    MAC_GMII_ADR_CR_BIT = 2,
    MAC_GMII_ADR_GW_BIT = 1,
    MAC_GMII_ADR_GB_BIT = 0
} gmac_gmii_address_reg_t;

#define MAC_GMII_ADR_PA_NUM_BITS 5
#define MAC_GMII_ADR_GR_NUM_BITS 5
#define MAC_GMII_ADR_CR_NUM_BITS 3

typedef enum gmac_gmii_data_reg {
    MAC_GMII_DATA_GD_BIT = 0
} gmac_gmii_data_reg_t;

#define MAC_GMII_DATA_GD_NUM_BITS 16

typedef enum gmac_flow_control_reg {
    MAC_FLOW_CNTL_PT_BIT      = 16,
    MAC_FLOW_CNTL_PLT_BIT     = 4,
    MAC_FLOW_CNTL_UP_BIT      = 3,
    MAC_FLOW_CNTL_RFE_BIT     = 2,
    MAC_FLOW_CNTL_TFE_BIT     = 1,
    MAC_FLOW_CNTL_FCB_BPA_BIT = 0
} gmac_flow_control_reg_t;

#define MAC_FLOW_CNTL_PT_NUM_BITS 16
#define MAC_FLOW_CNTL_PLT_NUM_BITS 2

typedef enum gmac_vlan_tag_reg {
    MAC_VLAN_TAG_LV_BIT = 0
} gmac_vlan_tag_reg_t;

#define MAC_VLAN_TAG_LV_NUM_BITS 16

typedef enum gmac_version_reg {
    MAC_VERSION_UD_BIT = 8,
    MAC_VERSION_SD_BIT = 0
} gmac_version_reg_t;

#define MAC_VERSION_UD_NUM_BITS 8
#define MAC_VERSION_SD_NUM_BITS 8

typedef enum gmac_mac_adr_0_high_reg {
    MAC_ADR0_HIGH_MO_BIT = 31,
    MAC_ADR0_HIGH_A_BIT  = 0
} gmac_mac_adr_0_high_reg_t;

#define MAC_ADR0_HIGH_A_NUM_BITS 16

typedef enum gmac_mac_adr_0_low_reg {
    MAC_ADR0_LOW_A_BIT = 0
} gmac_mac_adr_0_low_reg_t;

typedef enum gmac_mac_adr_1_high_reg {
    MAC_ADR1_HIGH_AE_BIT  = 31,
    MAC_ADR1_HIGH_SA_BIT  = 30,
    MAC_ADR1_HIGH_MBC_BIT = 24,
    MAC_ADR1_HIGH_A_BIT   = 0
} gmac_mac_adr_1_high_reg_t;

#define MAC_ADR1_HIGH_MBC_NUM_BITS 6
#define MAC_ADR1_HIGH_A_NUM_BITS 16

typedef enum gmac_mac_adr_1_low_reg {
    MAC_ADR1_LOW_A_BIT = 0
} gmac_mac_adr_1_low_reg_t;


/**
 * MMC register indices - registers accessed via the MAC accessor functions
 */
typedef enum gmac_mmc_regs {
    MMC_CONTROL_REG = 64,
    MMC_RX_INT_REG  = 65,
    MMC_TX_INT_REG  = 66,
    MMC_RX_MASK_REG = 67,
    MMC_TX_MASK_REG = 68
} gmac_mmc_regs_t;

/**
 * DMA register indices
 */
typedef enum gmac_dma_regs {
    DMA_BUS_MODE_REG        = 0,
    DMA_TX_POLL_REG         = 1,
    DMA_RX_POLL_REG         = 2,
    DMA_RX_DESC_ADR_REG     = 3,
    DMA_TX_DESC_ADR_REG     = 4,
    DMA_STATUS_REG          = 5,
    DMA_OP_MODE_REG         = 6,
    DMA_INT_ENABLE_REG      = 7,
    DMA_MISSED_OVERFLOW_REG = 8,
    DMA_CUR_TX_DESC_REG     = 18,
    DMA_CUR_RX_DESC_REG     = 19,
    DMA_CUR_TX_ADR_REG      = 20,
    DMA_CUR_RX_ADR_REG      = 21
} gmac_dma_regs_t;

/**
 * DMA register field definitions
 */

typedef enum gmac_dma_bus_mode_reg {
    DMA_BUS_MODE_FB_BIT  = 16,
    DMA_BUS_MODE_PR_BIT  = 14,
    DMA_BUS_MODE_PBL_BIT = 8,
    DMA_BUS_MODE_DSL_BIT = 2,
    DMA_BUS_MODE_DA_BIT  = 1,
    DMA_BUS_MODE_SWR_BIT = 0
} gmac_dma_bus_mode_reg_t;

#define DMA_BUS_MODE_PR_NUM_BITS 2
#define DMA_BUS_MODE_PBL_NUM_BITS 6
#define DMA_BUS_MODE_DSL_NUM_BITS 5

typedef enum gmac_dma_tx_poll_demand_reg {
    DMA_TX_POLL_TPD_BIT = 0
} gmac_dma_tx_poll_demand_reg_t;

typedef enum gmac_dma_rx_poll_demand_reg {
    DMA_RX_POLL_RPD_BIT = 0
} gmac_dma_rx_poll_demand_reg_t;

typedef enum gmac_dma_rx_desc_list_adr_reg {
    DMA_RX_DESC_ADR_SRL_BIT = 0
} gmac_dma_rx_desc_list_adr_reg_t;

typedef enum gmac_dma_tx_desc_list_adr_reg {
    DMA_TX_DESC_ADR_STL_BIT = 0
} gmac_dma_tx_desc_list_adr_reg_t;

typedef enum gmac_dma_status_reg {
    DMA_STATUS_GPI_BIT = 28,
    DMA_STATUS_GMI_BIT = 27,
    DMA_STATUS_GLI_BIT = 26,
    DMA_STATUS_EB_BIT  = 23,
    DMA_STATUS_TS_BIT  = 20,
    DMA_STATUS_RS_BIT  = 17,
    DMA_STATUS_NIS_BIT = 16,
    DMA_STATUS_AIS_BIT = 15,
    DMA_STATUS_ERI_BIT = 14,
    DMA_STATUS_FBE_BIT = 13,
    DMA_STATUS_ETI_BIT = 10,
    DMA_STATUS_RWT_BIT = 9,
    DMA_STATUS_RPS_BIT = 8,
    DMA_STATUS_RU_BIT  = 7,
    DMA_STATUS_RI_BIT  = 6,
    DMA_STATUS_UNF_BIT = 5,
    DMA_STATUS_OVF_BIT = 4,
    DMA_STATUS_TJT_BIT = 3,
    DMA_STATUS_TU_BIT  = 2,
    DMA_STATUS_TPS_BIT = 1,
    DMA_STATUS_TI_BIT  = 0
} gmac_dma_status_reg_t;

#define DMA_STATUS_EB_NUM_BITS 3
#define DMA_STATUS_TS_NUM_BITS 3
#define DMA_STATUS_RS_NUM_BITS 3

typedef enum gmac_dma_op_mode_reg {
    DMA_OP_MODE_SF_BIT  = 21,
    DMA_OP_MODE_FTF_BIT = 20,
    DMA_OP_MODE_TTC_BIT = 14,
    DMA_OP_MODE_ST_BIT  = 13,
    DMA_OP_MODE_RFD_BIT = 11,
    DMA_OP_MODE_RFA_BIT = 9,
    DMA_OP_MODE_EFC_BIT = 8,
    DMA_OP_MODE_FEF_BIT = 7,
    DMA_OP_MODE_FUF_BIT = 6,
    DMA_OP_MODE_RTC_BIT = 3,
    DMA_OP_MODE_OSF_BIT = 2,
    DMA_OP_MODE_SR_BIT  = 1
} gmac_dma_op_mode_reg_t;

#define DMA_OP_MODE_TTC_NUM_BITS 3
#define DMA_OP_MODE_RFD_NUM_BITS 2
#define DMA_OP_MODE_RFA_NUM_BITS 2
#define DMA_OP_MODE_RTC_NUM_BITS 2

typedef enum gmac_dma_intr_enable_reg {
    DMA_INT_ENABLE_NI_BIT  = 16,
    DMA_INT_ENABLE_AI_BIT  = 15,
    DMA_INT_ENABLE_ERE_BIT = 14,
    DMA_INT_ENABLE_FBE_BIT = 13,
    DMA_INT_ENABLE_ETE_BIT = 10,
    DMA_INT_ENABLE_RW_BIT  = 9,
    DMA_INT_ENABLE_RS_BIT  = 8,
    DMA_INT_ENABLE_RU_BIT  = 7,
    DMA_INT_ENABLE_RI_BIT  = 6,
    DMA_INT_ENABLE_UN_BIT  = 5,
    DMA_INT_ENABLE_OV_BIT  = 4,
    DMA_INT_ENABLE_TJ_BIT  = 3,
    DMA_INT_ENABLE_TU_BIT  = 2,
    DMA_INT_ENABLE_TS_BIT  = 1,
    DMA_INT_ENABLE_TI_BIT  = 0
} gmac_dma_intr_enable_reg_t;

typedef enum gmac_dma_missed_overflow_reg {
    DMA_MISSED_OVERFLOW_OFOC_BIT  = 28,    // Overflow bit for FIFO Overflow Counter
    DMA_MISSED_OVERFLOW_AMFC_BIT  = 17,    // Application Missed Frames Count
    DMA_MISSED_OVERFLOW_OAMFO_BIT = 16,    // Overflow bit for Application Missed Frames Count
    DMA_MISSED_OVERFLOW_CMFC_BIT  = 0      // Controller Missed Frames Count
} gmac_dma_missed_overflow_reg_t;

#define DMA_MISSED_OVERFLOW_OAMFO_NUM_BITS 11
#define DMA_MISSED_OVERFLOW_CMFC_NUM_BITS 16

typedef enum gmac_dma_current_tx_desc_reg {
    DMA_CUR_TX_DESC_A_BIT = 0
} gmac_dma_current_tx_desc_reg_t;

typedef enum gmac_dma_current_rx_desc_reg {
    DMA_CUR_RX_DESC_A_BIT = 0
} gmac_dma_current_rx_desc_reg_t;

typedef enum gmac_dma_current_tx_adr_reg {
    DMA_CUR_TX_ADR_A_BIT = 0
} gmac_dma_current_tx_adr_reg_t;

typedef enum gmac_dma_current_rx_adr_reg {
    DMA_CUR_RX_ADR_A_BIT = 0
} gmac_dma_current_rx_adr_reg_t;

/**
 * Descriptor support
 */
/** Descriptor status word field definitions */
typedef enum desc_status {
  descOwnByDma          = 0x80000000,   /* descriptor is owned by DMA engine  */

  descFrameLengthMask   = 0x3FFF0000,   /* Receive descriptor frame length */
  descFrameLengthShift  = 16,

  descError             = 0x00008000,   /* Error summary bit  - OR of the following bits:    v  */

  descRxTruncated       = 0x00004000,   /* Rx - no more descriptors for receive frame        E  */

  descRxLengthError     = 0x00001000,   /* Rx - frame size not matching with length field */
  descRxDamaged         = 0x00000800,   /* Rx - frame was damaged due to buffer overflow     E  */
  descRxFirst           = 0x00000200,   /* Rx - first descriptor of the frame                   */
  descRxLast            = 0x00000100,   /* Rx - last descriptor of the frame                    */
  descRxLongFrame       = 0x00000080,   /* Rx - frame is longer than 1518 bytes              E  */
  descRxCollision       = 0x00000040,   /* Rx - late collision occurred during reception     E  */
  descRxFrameEther      = 0x00000020,   /* Rx - Frame type - Ethernet, otherwise 802.3          */
  descRxWatchdog        = 0x00000010,   /* Rx - watchdog timer expired during reception      E  */
  descRxMiiError        = 0x00000008,   /* Rx - error reported by MII interface              E  */
  descRxDribbling       = 0x00000004,   /* Rx - frame contains noninteger multiple of 8 bits    */
  descRxCrc             = 0x00000002,   /* Rx - CRC error                                    E  */

  descTxTimeout         = 0x00004000,   /* Tx - Transmit jabber timeout                      E  */
  descTxLostCarrier     = 0x00000800,   /* Tx - carrier lost during tramsmission             E  */
  descTxNoCarrier       = 0x00000400,   /* Tx - no carrier signal from the tranceiver        E  */
  descTxLateCollision   = 0x00000200,   /* Tx - transmission aborted due to collision        E  */
  descTxExcCollisions   = 0x00000100,   /* Tx - transmission aborted after 16 collisions     E  */
  descTxVLANFrame       = 0x00000080,   /* Tx - VLAN-type frame                                 */
  descTxCollMask        = 0x00000078,   /* Tx - Collision count                                 */
  descTxCollShift       = 3,
  descTxExcDeferral     = 0x00000004,   /* Tx - excessive deferral                           E  */
  descTxUnderflow       = 0x00000002,   /* Tx - late data arrival from the memory            E  */
  descTxDeferred        = 0x00000001,   /* Tx - frame transmision deferred                      */
} desc_status_t;

/** Descriptor length word field definitions */
typedef enum desc_length {
  descTxIntEnable       = 0x80000000,   /* Tx - interrupt on completion                         */
  descTxLast            = 0x40000000,   /* Tx - Last segment of the frame                       */
  descTxFirst           = 0x20000000,   /* Tx - First segment of the frame                      */
  descTxDisableCrc      = 0x04000000,   /* Tx - Add CRC disabled (first segment only)           */

  descEndOfRing         = 0x02000000,   /* End of descriptors ring                              */
  descChain             = 0x01000000,   /* Second buffer address is chain address               */
  descTxDisablePadd     = 0x00800000,   /* disable padding, added by - reyaz */

  descSize2Mask         = 0x003FF800,   /* Buffer 2 size                                        */
  descSize2Shift        = 11,
  descSize1Mask         = 0x000007FF,   /* Buffer 1 size                                        */
  descSize1Shift        = 0,
} desc_length_t;

typedef enum rx_desc_status {
    RX_DESC_STATUS_OWN_BIT  = 31,
    RX_DESC_STATUS_AFM_BIT  = 30,
    RX_DESC_STATUS_FL_BIT   = 16,
    RX_DESC_STATUS_ES_BIT   = 15,
    RX_DESC_STATUS_DE_BIT   = 14,
    RX_DESC_STATUS_SAF_BIT  = 13,
    RX_DESC_STATUS_LE_BIT   = 12,
    RX_DESC_STATUS_OE_BIT   = 11,
    RX_DESC_STATUS_IPC_BIT  = 10,
    RX_DESC_STATUS_FS_BIT   = 9,
    RX_DESC_STATUS_LS_BIT   = 8,
    RX_DESC_STATUS_VLAN_BIT = 7,
    RX_DESC_STATUS_LC_BIT   = 6,
    RX_DESC_STATUS_FT_BIT   = 5,
    RX_DESC_STATUS_RWT_BIT  = 4,
    RX_DESC_STATUS_RE_BIT   = 3,
    RX_DESC_STATUS_DRE_BIT  = 2,
    RX_DESC_STATUS_CE_BIT   = 1,
    RX_DESC_STATUS_MAC_BIT  = 0
} rx_desc_status_t;

#define RX_DESC_STATUS_FL_NUM_BITS 14

#if (DESCRIPTOR_VERSION == 810)
/* regular descriptor format definitions */

typedef enum rx_desc_length {
    RX_DESC_LENGTH_DIC_BIT  = 31,
    RX_DESC_LENGTH_RER_BIT  = 25,
    RX_DESC_LENGTH_RCH_BIT  = 24,
    RX_DESC_LENGTH_RBS2_BIT = 11,
    RX_DESC_LENGTH_RBS1_BIT = 0,
} rx_desc_length_t;

#define RX_DESC_LENGTH_RBS2_NUM_BITS 11
#define RX_DESC_LENGTH_RBS1_NUM_BITS 11

typedef enum tx_desc_status {
    TX_DESC_STATUS_OWN_BIT  = 31,
    TX_DESC_STATUS_ES_BIT   = 15,
    TX_DESC_STATUS_JT_BIT   = 14,
    TX_DESC_STATUS_FF_BIT   = 13,
    TX_DESC_STATUS_LOC_BIT  = 11,
    TX_DESC_STATUS_NC_BIT   = 10,
    TX_DESC_STATUS_LC_BIT   = 9,
    TX_DESC_STATUS_EC_BIT   = 8,
    TX_DESC_STATUS_VF_BIT   = 7,
    TX_DESC_STATUS_CC_BIT   = 3,
    TX_DESC_STATUS_ED_BIT   = 2,
    TX_DESC_STATUS_UF_BIT   = 1,
    TX_DESC_STATUS_DB_BIT   = 0
} tx_desc_status_t;

#define TX_DESC_STATUS_CC_NUM_BITS 4

typedef enum tx_desc_length {
    TX_DESC_LENGTH_IC_BIT   = 31,
    TX_DESC_LENGTH_LS_BIT   = 30,
    TX_DESC_LENGTH_FS_BIT   = 29,
    TX_DESC_LENGTH_DC_BIT   = 26,
    TX_DESC_LENGTH_TER_BIT  = 25,
    TX_DESC_LENGTH_TCH_BIT  = 24,
    TX_DESC_LENGTH_DP_BIT   = 23,
    TX_DESC_LENGTH_TBS2_BIT = 11,
    TX_DESC_LENGTH_TBS1_BIT = 0
} tx_desc_length_t;

#define TX_DESC_LENGTH_TBS2_NUM_BITS 11
#define TX_DESC_LENGTH_TBS1_NUM_BITS 11

#else // DESCRIPTOR_VERSION == 810
/* enhanced descriptor format definitions */

typedef enum rx_desc_length {
    RX_DESC_LENGTH_DIC_BIT  = 31,
    RX_DESC_LENGTH_RBS2_BIT = 16,
    RX_DESC_LENGTH_RER_BIT  = 15,
    RX_DESC_LENGTH_RCH_BIT  = 14,
    RX_DESC_LENGTH_RBS1_BIT = 0,
} rx_desc_length_t;

#define RX_DESC_LENGTH_RBS2_NUM_BITS 13
#define RX_DESC_LENGTH_RBS1_NUM_BITS 13

typedef enum tx_desc_status {
    TX_DESC_STATUS_OWN_BIT = 31,
	TX_DESC_STATUS_IC_BIT  = 30,
	TX_DESC_STATUS_LS_BIT  = 29,
	TX_DESC_STATUS_FS_BIT  = 28,
    TX_DESC_STATUS_DC_BIT  = 27,
    TX_DESC_STATUS_DP_BIT  = 27,
    TX_DESC_STATUS_CIC_BIT = 22,
    TX_DESC_STATUS_TER_BIT = 21,
    TX_DESC_STATUS_TCH_BIT = 20,
    TX_DESC_STATUS_IHE_BIT = 16,
    TX_DESC_STATUS_ES_BIT  = 15,
    TX_DESC_STATUS_JT_BIT  = 14,
    TX_DESC_STATUS_FF_BIT  = 13,
    TX_DESC_STATUS_IPE_BIT = 12,
    TX_DESC_STATUS_LOC_BIT = 11,
    TX_DESC_STATUS_NC_BIT  = 10,
    TX_DESC_STATUS_LC_BIT  = 9,
    TX_DESC_STATUS_EC_BIT  = 8,
    TX_DESC_STATUS_VF_BIT  = 7,
    TX_DESC_STATUS_CC_BIT  = 3,
    TX_DESC_STATUS_ED_BIT  = 2,
    TX_DESC_STATUS_UF_BIT  = 1,
    TX_DESC_STATUS_DB_BIT  = 0
} tx_desc_status_t;

#define TX_DESC_STATUS_CIC_NUM_BITS	2
#define TX_DESC_STATUS_CC_NUM_BITS	4

#define TX_DESC_STATUS_CIC_NONE    0
#define TX_DESC_STATUS_CIC_HDR     1
#define TX_DESC_STATUS_CIC_PAYLOAD 2
#define TX_DESC_STATUS_CIC_FULL    3

typedef enum tx_desc_length {
    TX_DESC_LENGTH_TBS2_BIT = 16,
    TX_DESC_LENGTH_TBS1_BIT = 0
} tx_desc_length_t;

#define TX_DESC_LENGTH_TBS2_NUM_BITS 13
#define TX_DESC_LENGTH_TBS1_NUM_BITS 13

#endif // DESCRIPTOR_VERSION == 810

/** Return the number of descriptors available for the CPU to fill with new
 *  packet info */
static inline int available_for_write(gmac_desc_list_info_t* desc_list)
{
    return desc_list->empty_count;
}

/** Return non-zero if there is a descriptor available with a packet with which
 *  the GMAC DMA has finished */
static inline int tx_available_for_read(gmac_desc_list_info_t* desc_list)
{
    return desc_list->full_count &&
           !((desc_list->base_ptr + desc_list->r_index)->status & (1UL << TX_DESC_STATUS_OWN_BIT));
}

/** Return non-zero if there is a descriptor available with a packet with which
 *  the GMAC DMA has finished */
static inline int rx_available_for_read(gmac_desc_list_info_t* desc_list)
{
    return desc_list->full_count &&
           !((desc_list->base_ptr + desc_list->r_index)->status & (1UL << RX_DESC_STATUS_OWN_BIT));
}

/**
 * @param A u32 containing the status from a received frame's DMA descriptor
 * @return An int which is non-zero if a valid received frame is fully contained
 *  within the descriptor from whence the status came
 */
static inline int is_rx_valid(u32 status)
{
    return !(status & descError)  &&
           (status & descRxFirst) &&
           (status & descRxLast);
}

static inline int is_rx_dribbling(u32 status)
{
    return status & descRxDribbling;
}

static inline u32 get_rx_length(u32 status)
{
    return (status & descFrameLengthMask) >> descFrameLengthShift;
}

static inline int is_rx_collision_error(u32 status)
{
    return status & (descRxDamaged | descRxCollision);
}

static inline int is_rx_crc_error(u32 status)
{
    return status & descRxCrc;
}

static inline int is_rx_frame_error(u32 status)
{
    return status & descRxDribbling;
}

static inline int is_rx_length_error(u32 status)
{
    return status & descRxLengthError;
}

static inline int is_rx_long_frame(u32 status)
{
    return status & descRxLongFrame;
}

static inline int is_tx_valid(u32 status)
{
    return !(status & descError);
}

static inline int is_tx_collision_error(u32 status)
{
    return (status & descTxCollMask) >> descTxCollShift;
}

static inline int is_tx_aborted(u32 status)
{
    return status & (descTxLateCollision | descTxExcCollisions);
}

static inline int is_tx_carrier_error(u32 status)
{
    return status & (descTxLostCarrier | descTxNoCarrier);
}

/**
 * GMAC private metadata
 */
static gmac_priv_t priv_data;
static gmac_priv_t* priv = &priv_data;

/**
 * Descriptor list management
 */

static void init_rx_descriptor(
    gmac_dma_desc_t* desc,
    int              end_of_ring,
    int              disable_ioc)
{
    desc->status = 0;
    desc->length = 0;
    if (disable_ioc) {
        desc->length |= (1UL << RX_DESC_LENGTH_DIC_BIT);
    }
    if (end_of_ring) {
        desc->length |= (1UL << RX_DESC_LENGTH_RER_BIT);
    }
    desc->buffer1 = 0;
    desc->buffer2 = 0;
    desc->skb = 0;
}

static void init_tx_descriptor(
    gmac_dma_desc_t* desc,
    int              end_of_ring,
    int              enable_ioc,
    int              disable_crc,
    int              disable_padding)
{
    desc->status = 0;
    desc->length = 0;
#if (DESCRIPTOR_VERSION == 810)
    if (enable_ioc) {
        desc->length |= (1UL << TX_DESC_LENGTH_IC_BIT);
    }
    if (disable_crc) {
        desc->length |= (1UL << TX_DESC_LENGTH_DC_BIT);
    }
    if (disable_padding) {
        desc->length |= (1UL << TX_DESC_LENGTH_DP_BIT);
    }
    if (end_of_ring) {
        desc->length |= (1UL << TX_DESC_LENGTH_TER_BIT);
    }
#else // DESCRIPTOR_VERSION == 810
    if (enable_ioc) {
        desc->status |= (1UL << TX_DESC_STATUS_IC_BIT);
    }
    if (disable_crc) {
        desc->status |= (1UL << TX_DESC_STATUS_DC_BIT);
    }
    if (disable_padding) {
        desc->status |= (1UL << TX_DESC_STATUS_DP_BIT);
    }
    if (end_of_ring) {
        desc->status |= (1UL << TX_DESC_STATUS_TER_BIT);
    }
#endif // DESCRIPTOR_VERSION == 810
    desc->buffer1 = 0;
    desc->buffer2 = 0;
    desc->skb = 0;
}

static void init_rx_desc_list(
    gmac_desc_list_info_t* desc_list,
    gmac_dma_desc_t*       base_ptr,
    int                    num_descriptors)
{
    int i;
    
    desc_list->base_ptr = base_ptr;
    desc_list->num_descriptors = num_descriptors;
    desc_list->empty_count = num_descriptors;
    desc_list->full_count = 0;
    desc_list->r_index = 0;
    desc_list->w_index = 0;

    for (i=0; i < (num_descriptors - 1); i++) {
        init_rx_descriptor(base_ptr + i, 0, 0);
    }
    init_rx_descriptor(base_ptr + i, 1, 0);
}

static void init_tx_desc_list(
    gmac_desc_list_info_t* desc_list,
    gmac_dma_desc_t*       base_ptr,
    int                    num_descriptors)
{
    int i;
    
    desc_list->base_ptr = base_ptr;
    desc_list->num_descriptors = num_descriptors;
    desc_list->empty_count = num_descriptors;
    desc_list->full_count = 0;
    desc_list->r_index = 0;
    desc_list->w_index = 0;

    for (i=0; i < (num_descriptors - 1); i++) {
        init_tx_descriptor(base_ptr + i, 0, 1, 0, 0);
    }
    init_tx_descriptor(base_ptr + i, 1, 1, 0, 0);
}

static void rx_take_ownership(gmac_desc_list_info_t* desc_list)
{
    int i;
    for (i=0; i < desc_list->num_descriptors; i++) {
        (desc_list->base_ptr + i)->status &= ~(1UL << RX_DESC_STATUS_OWN_BIT);
    }
}

static void tx_take_ownership(gmac_desc_list_info_t* desc_list)
{
    int i;
    for (i=0; i < desc_list->num_descriptors; i++) {
        (desc_list->base_ptr + i)->status &= ~(1UL << TX_DESC_STATUS_OWN_BIT);
    }
}

static int set_tx_descriptor(
    gmac_priv_t* priv,
    dma_addr_t   dma_address,
    u32          length,
    sk_buff_t*   skb)
{
    int index;
    gmac_dma_desc_t* tx;

    // Are sufficicent descriptors available for writing by the CPU?
    if (!available_for_write(&priv->tx_gmac_desc_list_info)) {
        return -1;
    }

    // Get the index of the next TX descriptor available for writing by the CPU
    index = priv->tx_gmac_desc_list_info.w_index;

    // Get a pointer to the next TX descriptor available for writing by the CPU
    tx = priv->tx_gmac_desc_list_info.base_ptr + index;

    // Initialise the TX descriptor length field for the passed single buffer,
    // without destroying any fields we wish to be persistent

#if (DESCRIPTOR_VERSION == 810)
    // No chained second buffer
    tx->length &= ~(1UL << TX_DESC_LENGTH_TCH_BIT);
    // Single descriptor holds entire packet
    tx->length |= ((1UL << TX_DESC_LENGTH_LS_BIT) | (1UL << TX_DESC_LENGTH_FS_BIT));
#else // DESCRIPTOR_VERSION == 810
    // No chained second buffer
    tx->status &= ~(1UL << TX_DESC_STATUS_TCH_BIT);
    // Single descriptor holds entire packet
    tx->status |= ((1UL << TX_DESC_STATUS_LS_BIT) | (1UL << TX_DESC_STATUS_FS_BIT));
#endif // DESCRIPTOR_VERSION == 810
    // Zero the second buffer length field
    tx->length &= ~(((1UL << TX_DESC_LENGTH_TBS2_NUM_BITS) - 1) << TX_DESC_LENGTH_TBS2_BIT);
    // Zero the first buffer length field
    tx->length &= ~(((1UL << TX_DESC_LENGTH_TBS1_NUM_BITS) - 1) << TX_DESC_LENGTH_TBS1_BIT);
    // Fill in the first buffer length
    tx->length |= (length << TX_DESC_LENGTH_TBS1_BIT);

    // Initialise the first buffer pointer to the single passed buffer
    tx->buffer1 = dma_address;

    // Remember the socket buffer associated with the single passed buffer
    tx->skb = (u32)skb;

    // Update the index of the next descriptor available for writing by the CPU
#if (DESCRIPTOR_VERSION == 810)
    priv->tx_gmac_desc_list_info.w_index = (tx->length & (1UL << TX_DESC_LENGTH_TER_BIT)) ? 0 : index + 1;
#else // DESCRIPTOR_VERSION == 810
    priv->tx_gmac_desc_list_info.w_index = (tx->status & (1UL << TX_DESC_STATUS_TER_BIT)) ? 0 : index + 1;
#endif // DESCRIPTOR_VERSION == 810

    // make sure all memory updates are complete before releasing the GMAC on the data.
    wmb();

    // Hand TX descriptor to the GMAC DMA by setting the status bit.
    tx->status |= (1UL << TX_DESC_STATUS_OWN_BIT);

    // Account for the number of descriptors used to hold the new packet
    --priv->tx_gmac_desc_list_info.empty_count;
    ++priv->tx_gmac_desc_list_info.full_count;

    return index;
}

static int get_tx_descriptor(
    gmac_priv_t* priv,
    u32*         status,
    dma_addr_t*  dma_address,
    u32*         length,
    sk_buff_t**  skb)
{
    int index;
    gmac_dma_desc_t *tx;

    // Is there at least one descriptor with which the GMAC DMA has finished?
    if (!tx_available_for_read(&priv->tx_gmac_desc_list_info)) {
        return -1;
    }

    // Get the index of the descriptor released the longest time ago by the
    // GMAC DMA 
    index = priv->tx_gmac_desc_list_info.r_index;

    // Get a pointer to the descriptor released the longest time ago by the
    // GMAC DMA 
    tx = priv->tx_gmac_desc_list_info.base_ptr + index;

    // Extract the status field
    if (status) {
        *status = tx->status;
    }

    // Extract the length field - only cope with the first buffer associated
    // with the descriptor
    if (length) {
        *length = (tx->length >> TX_DESC_LENGTH_TBS1_BIT) &
                  ((1UL << TX_DESC_LENGTH_TBS1_NUM_BITS) - 1);
    }

    // Extract the pointer to the buffer containing the packet - only cope with
    // the first buffer associated with the descriptor
    if (dma_address) {
        *dma_address = tx->buffer1;
    }

    // Extract the pointer to the socket buffer associated with the packet
    if (skb) {
        *skb = (sk_buff_t*)(tx->skb);
    }

    // Update the index of the next descriptor with which the GMAC DMA may have
    // finished
#if (DESCRIPTOR_VERSION == 810)
    priv->tx_gmac_desc_list_info.r_index = (tx->length & (1UL << TX_DESC_LENGTH_TER_BIT)) ? 0 : index + 1;
#else // DESCRIPTOR_VERSION == 810
    priv->tx_gmac_desc_list_info.r_index = (tx->status & (1UL << TX_DESC_STATUS_TER_BIT)) ? 0 : index + 1;
#endif // DESCRIPTOR_VERSION == 810

    // Account for the number of descriptors freed to hold new packets
    ++priv->tx_gmac_desc_list_info.empty_count;
    --priv->tx_gmac_desc_list_info.full_count;

    return index;
}

int set_rx_descriptor(
    gmac_priv_t* priv,
    dma_addr_t   dma_address,
    u32          length,
    sk_buff_t*   skb)
{
    int index;
    gmac_dma_desc_t* rx;
    int num_descriptors_required;

    // Currently only support using a single descriptor to describe each packet
    // queued with the GMAc DMA
    num_descriptors_required = 1;

    // Are sufficicent descriptors available for writing by the CPU?
    if (available_for_write(&priv->rx_gmac_desc_list_info) < num_descriptors_required) {
        index = -1;
    } else {
        // Get the index of the next RX descriptor available for writing by the CPU
        index = priv->rx_gmac_desc_list_info.w_index;

        // Get a pointer to the next RX descriptor available for writing by the CPU
        rx = priv->rx_gmac_desc_list_info.base_ptr + index;

        // Initialise the RX descriptor length field for the passed single buffer,
        // without destroying any fields we wish to be persistent

        // No chained second buffer
        rx->length &= ~(1UL << RX_DESC_LENGTH_RCH_BIT);
        // Zero the second buffer length field
        rx->length &= ~(((1UL << RX_DESC_LENGTH_RBS2_NUM_BITS) - 1) << RX_DESC_LENGTH_RBS2_BIT);
        // Zero the first buffer length field
        rx->length &= ~(((1UL << RX_DESC_LENGTH_RBS1_NUM_BITS) - 1) << RX_DESC_LENGTH_RBS1_BIT);
        // Fill in the first buffer length
        rx->length |= (length << RX_DESC_LENGTH_RBS1_BIT);

        // Initialise the first buffer pointer to the single passed buffer
        rx->buffer1 = dma_address;

        // Remember the socket buffer associated with the single passed buffer
        rx->skb = (u32)skb;
        
        wmb();
        
        // Initialise RX descriptor status to be owned by the GMAC DMA
        rx->status = (1UL << RX_DESC_STATUS_OWN_BIT);

        // Update the index of the next descriptor available for writing by the CPU
        priv->rx_gmac_desc_list_info.w_index = (rx->length & (1UL << RX_DESC_LENGTH_RER_BIT)) ? 0 : index + 1;

        // Account for the number of descriptors used to hold the new packet
        priv->rx_gmac_desc_list_info.empty_count -= num_descriptors_required;
        priv->rx_gmac_desc_list_info.full_count  += num_descriptors_required;
    }

    return index;
}

int get_rx_descriptor(
    gmac_priv_t* priv,
    u32*         status,
    dma_addr_t*  dma_address,
    u32*         length,
    sk_buff_t**  skb)
{
    int index;
    gmac_dma_desc_t *rx;
    int num_descriptors_required;

    // Is there at least one descriptor with which the GMAC DMA has finished?
    if (!rx_available_for_read(&priv->rx_gmac_desc_list_info)) {
        return -1;
    }

#ifdef DEBUG_GMAC
    printf("rx desc avail for read \n");
#endif // DEBUG_GMAC
	

    // Currently can only cope with packets entirely contained within a single
    // descriptor
    num_descriptors_required = 1;

    // Get the index of the descriptor released the longest time ago by the
    // GMAC DMA 
    index = priv->rx_gmac_desc_list_info.r_index;

    // Get a pointer to the descriptor released the longest time ago by the
    // GMAC DMA 
    rx = priv->rx_gmac_desc_list_info.base_ptr + index;

    // Extract the status field
    if (status) {
        *status = rx->status;
    }

    // Extract the length field - only cope with the first buffer associated
    // with the descriptor
    if (length) {
#if (DESCRIPTOR_VERSION==810)
        *length = (rx->length >> RX_DESC_LENGTH_RBS1_BIT) &
                  ((1UL << RX_DESC_LENGTH_RBS1_NUM_BITS) - 1);
#else
        *length = (rx->status >> RX_DESC_STATUS_FL_BIT) &
                  ((1UL << RX_DESC_STATUS_FL_NUM_BITS) - 1);
#endif
    }
#ifdef DEBUG_GMAC
    printf("rx desc length:%d \n", *length);
#endif // DEBUG_GMAC

    // Extract the pointer to the buffer containing the packet - only cope with
    // the first buffer associated with the descriptor
    if (dma_address) {
        *dma_address = rx->buffer1;
    }

    // Extract the pointer to the socket buffer associated with the packet
    if (skb) {
        *skb = (sk_buff_t*)(rx->skb);
    }

    wmb();
    // Update the index of the next descriptor with which the GMAC DMA may have
    // finished
    priv->rx_gmac_desc_list_info.r_index = (rx->length & (1UL << RX_DESC_LENGTH_RER_BIT)) ? 0 : index + 1;

    // Account for the number of descriptors freed to hold new packets
    priv->rx_gmac_desc_list_info.empty_count += num_descriptors_required;
    priv->rx_gmac_desc_list_info.full_count  -= num_descriptors_required;

    return index;
}

/**
 * GMAC register access functions
 */

/**
 * MAC register access functions
 */

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 */
static inline u32 mac_reg_read(gmac_priv_t* priv, int reg_num)
{
    return *(volatile u32*)(priv->macBase + (reg_num << 2));
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 */
static inline void mac_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
    *(volatile u32*)(priv->macBase + (reg_num << 2)) = value;
   
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 * @param bits_to_clear A u32 specifying which bits of the specified register to
 *  clear. A set bit in this parameter will cause the matching bit in the
 *  register to be cleared
 */
static inline void mac_reg_clear_mask(gmac_priv_t* priv, int reg_num, u32 bits_to_clear)
{
    mac_reg_write(priv, reg_num, mac_reg_read(priv, reg_num) & ~bits_to_clear);
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 * @param bits_to_set A u32 specifying which bits of the specified register to
 *  set. A set bit in this parameter will cause the matching bit in the register
 *  to be set
 */
static inline void mac_reg_set_mask(gmac_priv_t* priv, int reg_num, u32 bits_to_set)
{
    mac_reg_write(priv, reg_num, mac_reg_read(priv, reg_num) | bits_to_set);
}


/**
 * DMA register access functions
 */

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the DMA register to access
 */
static inline u32 dma_reg_read(gmac_priv_t* priv, int reg_num)
{
    return *(volatile u32*)(priv->dmaBase + (reg_num << 2));
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the DMA register to access
 */
static inline void dma_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
    *(volatile u32*)(priv->dmaBase + (reg_num << 2)) = value;
    wmb();
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the DMA register to access
 * @param bits_to_clear A u32 specifying which bits of the specified register to
 *  clear. A set bit in this parameter will cause the matching bit in the
 *  register to be cleared
 * @return An u32 containing the new value written to the register
 */
static inline u32 dma_reg_clear_mask(gmac_priv_t* priv, int reg_num, u32 bits_to_clear)
{
    u32 new_value = dma_reg_read(priv, reg_num) & ~bits_to_clear;
    dma_reg_write(priv, reg_num, new_value);
    return new_value;
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the DMA register to access
 * @param bits_to_set A u32 specifying which bits of the specified register to
 *  set. A set bit in this parameter will cause the matching bit in the register
 *  to be set
 * @return An u32 containing the new value written to the register
 */
static inline u32 dma_reg_set_mask(gmac_priv_t* priv, int reg_num, u32 bits_to_set)
{
    u32 new_value = dma_reg_read(priv, reg_num) | bits_to_set;
    dma_reg_write(priv, reg_num, new_value);
    return new_value;
}

static void eth_down(void)
{
    // Stop transmitter, take ownership of all tx descriptors
    dma_reg_clear_mask(priv, DMA_OP_MODE_REG, DMA_OP_MODE_ST_BIT);
    if (priv->desc_base_addr) {
        tx_take_ownership(&priv->tx_gmac_desc_list_info);
    }

    // Stop receiver, take ownership of all rx descriptors
    dma_reg_clear_mask(priv, DMA_OP_MODE_REG, DMA_OP_MODE_SR_BIT);
    if (priv->desc_base_addr) {
        rx_take_ownership(&priv->rx_gmac_desc_list_info);
    }

    // Free descriptor resources. The TX descriptor will not have a packet
    // buffer attached, as this is provided by the stack when transmission is
    // required and ownership is not retained by the descriptor, as the stack
    // waits for transmission to complete via polling
    if (priv->desc_base_addr) {
        // Free receive descriptors, accounting for buffer offset used to
        // ensure IP header alignment
        while (1) {
            dma_addr_t dma_address;
            if (get_rx_descriptor(priv, 0, &dma_address, 0, 0) < 0) {
                break;
            }
            free((void*)(dma_address - ETHER_FRAME_ALIGN_WASTAGE));
        }

        // Free DMA descriptors' storage
        free(priv->desc_base_addr);

        // Remember that we've freed the descriptors memory
        priv->desc_base_addr = 0;
    }
}

/*
 * Reads a register from the MII Management serial interface
 */
int phy_read(int phyaddr, int phyreg)
{
    int data = 0;
    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (5 << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);

    mac_reg_write(priv, MAC_GMII_ADR_REG, addr);

    for (;;) {
        if (!(mac_reg_read(priv, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            // Successfully read from PHY
            data = mac_reg_read(priv, MAC_GMII_DATA_REG) & 0xFFFF;
            break;
        }
    }

#ifdef DEBUG_GMAC_INIT
    printf("phy_read() phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n", phyaddr, phyreg, data);
#endif // DEBUG_GMAC_INIT

    return data;
}

/*
 * Writes a register to the MII Management serial interface
 */
void phy_write(int phyaddr, int phyreg, int phydata)
{
    u32 addr = (phyaddr << MAC_GMII_ADR_PA_BIT) |
               (phyreg << MAC_GMII_ADR_GR_BIT) |
               (5 << MAC_GMII_ADR_CR_BIT) |
               (1UL << MAC_GMII_ADR_GW_BIT) |
               (1UL << MAC_GMII_ADR_GB_BIT);

    mac_reg_write(priv, MAC_GMII_DATA_REG, phydata);
    mac_reg_write(priv, MAC_GMII_ADR_REG, addr);

    for (;;) {
        if (!(mac_reg_read(priv, MAC_GMII_ADR_REG) & (1UL << MAC_GMII_ADR_GB_BIT))) {
            break;
        }
    }

#ifdef DEBUG_GMAC_INIT
    printf("phy_write() phyaddr=0x%x, phyreg=0x%x, phydata=0x%x\n", phyaddr, phyreg, phydata);
#endif // DEBUG_GMAC_INIT
}

/*
 * Finds and reports the PHY address
 */
int phy_detect(void)
{
	int found = 0;
    int phyaddr;

    // Scan all 32 PHY addresses if necessary
    priv->phy_type = 0;
    for (phyaddr = 1; phyaddr < 33; ++phyaddr) {
        unsigned int id1, id2;

        // Read the PHY identifiers
        id1 = phy_read(phyaddr & 31, MII_PHYSID1);
        id2 = phy_read(phyaddr & 31, MII_PHYSID2);

#ifdef DEBUG_GMAC_INIT
        printf("phy_detect() PHY adr = %u -> phy_id1=0x%x, phy_id2=0x%x\n", phyaddr, id1, id2);
#endif // DEBUG_GMAC_INIT

        // Make sure it is a valid identifier
        if (id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000 &&
            id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
#ifdef DEBUG_GMAC_INIT
            printf("phy_detect() Found PHY at address = %u\n", phyaddr);
#endif // DEBUG_GMAC_INIT

            priv->phy_id   = phyaddr & 31;
            priv->phy_type = id1 << 16 | id2;
            priv->phy_addr = phyaddr;

			found = 1;
            break;
        }
    }

	return found;
}

void start_phy_reset(void)
{
    // Ask the PHY to reset
    //phy_write(priv->phy_addr, MII_BMCR, BMCR_RESET);
    
    // Ask the PHY to reset and allow autonegotiation (Realtek PHY requires
    // auto-neg to be explicitly enabled at this point)
    phy_write(priv->phy_addr, MII_BMCR, BMCR_RESET | BMCR_ANRESTART | BMCR_ANENABLE);
}

int is_phy_reset_complete(void)
{
    int complete = 0;
    int bmcr;

    // Read back the status until it indicates reset, or we timeout
    bmcr = phy_read(priv->phy_addr, MII_BMCR);
    if (!(bmcr & BMCR_RESET)) {
        complete = 1;
    }

    return complete;
}

int is_phy_interface_rgmii(void)
{
	u32 reg_contents = *(volatile u32*)SYS_CTRL_GMAC_CTRL;
	return !!(reg_contents | (1UL << SYS_CTRL_GMAC_RGMII));
}

void phy_initialise(void)
{
	switch (priv->phy_type) {
		case PHY_TYPE_VITESSE_VSC8201XVZ:
			{
				// Allow s/w to override mode/duplex pin settings
				u32 acsr = phy_read(priv->phy_id, VSC8201_MII_ACSR);

				printf("PHY is Vitesse VSC8201XVZ\n");
				acsr |= (1UL << VSC8201_MII_ACSR_MDPPS_BIT);
				phy_write(priv->phy_id, VSC8201_MII_ACSR, acsr);
			}
			break;
		case PHY_TYPE_REALTEK_RTL8211BGR:
			printf("PHY is Realtek RTL8211BGR\n");
			break;
		case PHY_TYPE_REALTEK_RTL8211D:
                        printf("PHY is Realtek RTL8211D\n");
                        break;
		case PHY_TYPE_LSI_ET1011C:
		case PHY_TYPE_LSI_ET1011C2:
			{
				u32 phy_reg;

				printf("PHY is LSI ET1011C\n");

				if (is_phy_interface_rgmii()) {
					// Configure clocks for RGMII
					phy_reg = phy_read(priv->phy_id, ET1011C_MII_CONFIG);
					phy_reg &= ~(((1UL << ET1011C_MII_CONFIG_IFMODESEL_NUM_BITS) - 1) << ET1011C_MII_CONFIG_IFMODESEL);
					//phy_reg |= (ET1011C_MII_CONFIG_IFMODESEL_RGMII_TRACE << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (ET1011C_MII_CONFIG_IFMODESEL_RGMII_DLL << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (1UL << ET1011C_MII_CONFIG_SYSCLKEN);
					phy_write(priv->phy_id, ET1011C_MII_CONFIG, phy_reg);

					//phy_reg = phy_read(priv->phy_id, ET1011C_MII_CONTROL);
					//phy_reg |= ET1011C_MII_CONTROL_ALT_RGMII_DELAY;
					//phy_write(priv->phy_id, ET1011C_MII_CONTROL, phy_reg);
				} else {
					// Configure clocks for (G)MII
					phy_reg = phy_read(priv->phy_id, ET1011C_MII_CONFIG);
					phy_reg &= ~(((1UL << ET1011C_MII_CONFIG_IFMODESEL_NUM_BITS) - 1) << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= (ET1011C_MII_CONFIG_IFMODESEL_GMII_MII << ET1011C_MII_CONFIG_IFMODESEL);
					phy_reg |= ((1UL << ET1011C_MII_CONFIG_SYSCLKEN) |
					(1UL << ET1011C_MII_CONFIG_TXCLKEN) |
					(1UL << ET1011C_MII_CONFIG_TBI_RATESEL) |
					(1UL << ET1011C_MII_CONFIG_CRS_TX_EN));
					phy_write(priv->phy_id, ET1011C_MII_CONFIG, phy_reg);
				}

				// Enable Tx/Rx LED
				phy_reg = phy_read(priv->phy_id, ET1011C_MII_LED2);
				phy_reg &= ~(((1UL << ET1011C_MII_LED2_LED_NUM_BITS) - 1) << ET1011C_MII_LED2_LED_TXRX);
				phy_reg |= (ET1011C_MII_LED2_LED_TXRX_ACTIVITY << ET1011C_MII_LED2_LED_TXRX);
				phy_write(priv->phy_id, ET1011C_MII_LED2, phy_reg);
			}
			break;
        case PHY_TYPE_ICPLUS_IP1001LF_0:
        case PHY_TYPE_ICPLUS_IP1001LF_1:
			printf("PHY is IC+ IP1001LF\n");

			if (is_phy_interface_rgmii()) {
				u32 phy_reg;

                /* Delay on tx clock */
				phy_reg = phy_read(priv->phy_id, IP1001LF_PHYSPECIFIC_CSR);
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXPHASE_SEL;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_TXPHASE_SEL;

                /* adjust digtial drive strength */
				phy_reg &= ~IP1001LF_PHYSPECIFIC_CSR_DRIVE_MASK;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXCLKDRIVE_M;
				phy_reg |=  IP1001LF_PHYSPECIFIC_CSR_RXDDRIVE_M;
				phy_write(priv->phy_id, IP1001LF_PHYSPECIFIC_CSR, phy_reg);
            }
            break;
        default:
            printf("Ethernet Phy not recognised!\n");
            break;
	}
}

int detect_link_speed(void)
{
	u32 lpa = phy_read(priv->phy_id, MII_LPA);
	u32 lpa2 = phy_read(priv->phy_id, MII_STAT1000);

	if (((lpa2 & LPA_1000FULL)) ||
		((lpa2 & LPA_1000HALF))) {
		priv->link_speed = link_is_1000M;
        return 0;
    }
	if (((lpa & LPA_100BASE4)) ||
        ((lpa & LPA_100FULL )) ||
		((lpa & LPA_100HALF ))) {
		priv->link_speed = link_is_100M;
        return 0;
	}
	if (((lpa & LPA_10FULL)) ||
		((lpa & LPA_10HALF))) {
		priv->link_speed = link_is_10M;
        return 0;
	}

    priv->link_speed = link_is_disconnected;
	return 1;
}

int is_autoneg_complete(void)
{
    return phy_read(priv->phy_addr, MII_BMSR) & BMSR_ANEGCOMPLETE;
}

int is_link_ok(void)
{
	return phy_read(priv->phy_id, MII_BMSR) & BMSR_LSTATUS;
}

int eth_init(bd_t *bd)
{
    u32  version;
    u32  reg_contents;
    u8  *mac_addr;
    int  desc;

    // Set hardware device base addresses
    priv->macBase = (MAC_BASE_PA + MAC_BASE_OFFSET);
    priv->dmaBase = (MAC_BASE_PA + DMA_BASE_OFFSET);

#ifdef DEBUG_GMAC_INIT
    printf("eth_init(): About to reset MAC core\n");
#endif // DEBUG_GMAC_INIT
    // Ensure the MAC block is properly reset
    *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL = (1UL << SYS_CTRL_RSTEN_MAC_BIT);
    *(volatile u32*)SYS_CTRL_RSTEN_CLR_CTRL = (1UL << SYS_CTRL_RSTEN_MAC_BIT);

    // Enable the clock to the MAC block
    *(volatile u32*)SYS_CTRL_CKEN_SET_CTRL = (1UL << SYS_CTRL_CKEN_MAC_BIT);
    
#if (NAS_VERSION == 820)
    /* set the pin multiplexers to enable talking to Ethernent Phys */
    {
        unsigned int pins = (1 << MACA_MDC_MF_PIN ) |
                            (1 << MACA_MDIO_MF_PIN) ;/* |
                            (1 << MACA_COL_MF_PIN ) |
                            (1 << MACA_CRS_MF_PIN ) |
                            (1 << MACA_TXER_MF_PIN) |
                            (1 << MACA_RXER_MF_PIN) ; */

        *(volatile u32*)SYS_CTRL_SECONDARY_SEL |=  pins ;
        *(volatile u32*)SYS_CTRL_TERTIARY_SEL  &= ~pins ;
        *(volatile u32*)SYS_CTRL_DEBUG_SEL     &= ~pins ;
    }
#endif

    version = mac_reg_read(priv, MAC_VERSION_REG);
#ifdef DEBUG_GMAC_INIT
    printf("eth_init(): GMAC Synopsis version = 0x%x, vendor version = 0x%x\n", version & 0xff, (version >> 8) & 0xff);
#endif // DEBUG_GMAC_INIT

    // Use simple mux for 25/125 Mhz clock switching
    *(volatile u32*)SYS_CTRL_GMAC_CTRL |= (1UL << SYS_CTRL_GMAC_SIMPLE_MAX);

    // Enable GMII_GTXCLK to follow GMII_REFCLK - required for gigabit PHY
    *(volatile u32*)SYS_CTRL_GMAC_CTRL |= (1UL << SYS_CTRL_GMAC_CKEN_GTX);

#if (NAS_VERSION == 820)
    // set auto tx speed
    *(volatile u32*)SYS_CTRL_GMAC_CTRL |= (1UL << SYS_CTRL_GMAC_AUTOSPEED);
#endif

    // Disable all GMAC interrupts
    dma_reg_write(priv, DMA_INT_ENABLE_REG, 0);

    // Reset the entire GMAC
    dma_reg_write(priv, DMA_BUS_MODE_REG, 1UL << DMA_BUS_MODE_SWR_BIT);

    // Wait for the reset operation to complete
	printf("Wait GMAC to reset");
    while (dma_reg_read(priv, DMA_BUS_MODE_REG) & (1UL << DMA_BUS_MODE_SWR_BIT)) {
		udelay(250000);
		printf(".");
    }
	printf("\n");

	// Attempt to discover link speed from the PHY
	if (!phy_detect()) {
		printf("No PHY found\n");
	} else {
		// Ensure the PHY is in a sensible state by resetting it
		start_phy_reset();

		// Read back the status until it indicates reset, or we timeout
		printf("Wait for PHY reset");
		while (!is_phy_reset_complete()) {
			udelay(250000);
			printf(".");
		}
		printf("\n");

		// Setup the PHY based on its type
		phy_initialise();

		printf("Wait for link to come up");
		while (!is_link_ok()) {
			udelay(250000);
			printf(".");
		}
		printf("Link up\n");

		// Wait for PHY to have completed autonegotiation
		printf("Wait for auto-negotiation to complete");
		while (!is_autoneg_complete()) {
			udelay(250000);
			printf(".");
		}
		printf("\n");

		// Interrogate the PHY for the link speed
		if (detect_link_speed()) {
			printf("Failed to detect link speed\n");
		} else {
            switch (priv->link_speed) {
            case link_is_1000M:
                printf("Link is 1000M\n");
                break;
            case link_is_100M:
                printf("Link is 100M\n");
                break;
            case link_is_10M:
                printf("Link is 10M\n");
                break;
            default:
                printf("Unknown link speed!\n");
                break;
            }
		}
	}

    // Form the MAC config register contents
    reg_contents = 0;
    switch (priv->link_speed) {
    case link_is_1000M:
        /* PS and FES clear */
        break;
    case link_is_100M:
		reg_contents |= (1UL << MAC_CONFIG_PS_BIT); // not gigabit
		reg_contents |= (1UL << MAC_CONFIG_FES_BIT); // 100M
        break;
    case link_is_10M:
		reg_contents |=  (1UL << MAC_CONFIG_PS_BIT); // not gigabit
		reg_contents &= ~(1UL << MAC_CONFIG_FES_BIT); // 10M
		reg_contents |=  (1UL << MAC_CONFIG_TC_BIT); // 
        break;
    default:
        break;
    }
    reg_contents |= (1UL << MAC_CONFIG_DM_BIT); // Full duplex
    reg_contents |= ((1UL << MAC_CONFIG_TE_BIT) |
                     (1UL << MAC_CONFIG_RE_BIT));
    mac_reg_write(priv, MAC_CONFIG_REG, reg_contents);
    
    // Form the MAC frame filter register contents
    reg_contents = 0;
    mac_reg_write(priv, MAC_FRAME_FILTER_REG, reg_contents);

    // Form the hash table registers contents
    mac_reg_write(priv, MAC_HASH_HIGH_REG, 0);
    mac_reg_write(priv, MAC_HASH_LOW_REG, 0);

    // Form the MAC flow control register contents
    reg_contents = 0;
    reg_contents |= ((1UL << MAC_FLOW_CNTL_RFE_BIT) |
                     (1UL << MAC_FLOW_CNTL_TFE_BIT));
    mac_reg_write(priv, MAC_FLOW_CNTL_REG, reg_contents);

    // Form the MAC VLAN tag register contents
    reg_contents = 0;
    mac_reg_write(priv, MAC_VLAN_TAG_REG, reg_contents);

    // Form the MAC addr0 high and low registers contents from the character
    // string representation from the environment
    mac_addr = getenv("ethaddr");
#ifdef DEBUG_GMAC_INIT
    printf("eth_init(): Mac addr = %s\n", mac_addr);
#endif // DEBUG_GMAC_INIT

    /* writing to a MAC address _must_ be done in order, with low-bits last! */
    reg_contents  =  simple_strtoul(mac_addr+12, 0, 16);
    reg_contents |= (simple_strtoul(mac_addr+15, 0, 16) << 8);
    mac_reg_write(priv, MAC_ADR0_HIGH_REG, reg_contents);
    reg_contents  =  simple_strtoul(mac_addr+0, 0, 16);
    reg_contents |= (simple_strtoul(mac_addr+3, 0, 16) << 8);
    reg_contents |= (simple_strtoul(mac_addr+6, 0, 16) << 16);
    reg_contents |= (simple_strtoul(mac_addr+9, 0, 16) << 24);
    mac_reg_write(priv, MAC_ADR0_LOW_REG, reg_contents);

    // Disable all MMC interrupt sources
    mac_reg_write(priv, MMC_RX_MASK_REG, ~0UL);
    mac_reg_write(priv, MMC_TX_MASK_REG, ~0UL);

    // Remember how large the unified descriptor array is to be
    priv->total_num_descriptors = NUM_RX_DMA_DESCRIPTORS + NUM_TX_DMA_DESCRIPTORS;

    // Need a consistent DMA mapping covering all the memory occupied by DMA
    // unified descriptor array, as both CPU and DMA engine will be reading and
    // writing descriptor fields.
    priv->desc_base_addr = (gmac_dma_desc_t*)malloc(sizeof(gmac_dma_desc_t) * priv->total_num_descriptors);
    if (!priv->desc_base_addr) {
       printf("eth_init(): Failed to allocate memory for DMA descriptors\n");
        goto err_out;
    }

    // Initialise the structures managing the TX descriptor list
    init_tx_desc_list(&priv->tx_gmac_desc_list_info,
                      priv->desc_base_addr,
                      NUM_TX_DMA_DESCRIPTORS);

    // Initialise the structures managing the RX descriptor list
    init_rx_desc_list(&priv->rx_gmac_desc_list_info,
                      priv->desc_base_addr + NUM_TX_DMA_DESCRIPTORS,
                      priv->total_num_descriptors - NUM_TX_DMA_DESCRIPTORS);

    // Prepare receive descriptors
    desc = 0;
    while (available_for_write(&priv->rx_gmac_desc_list_info)) {
        // Allocate a new buffer for the descriptor which is large enough for
        // any packet received from the link
        dma_addr_t dma_address = (dma_addr_t)malloc(ETHER_MTU + ETHER_FRAME_ALIGN_WASTAGE + EXTRA_RX_SKB_SPACE);
        if (!dma_address) {
            printf("eth_init(): No memory for socket buffer\n");
            break;
        }

        desc = set_rx_descriptor(priv,
                                 dma_address + ETHER_FRAME_ALIGN_WASTAGE,
                                 ETHER_MTU + EXTRA_RX_SKB_SPACE,
                                 0);

        if (desc < 0) {
            // Release the buffer
            free((void*)dma_address);

            printf("eth_init(): Error, no RX descriptor available\n");
            goto err_out;
        }
    }

    // Initialise the GMAC DMA bus mode register
    dma_reg_write(priv, DMA_BUS_MODE_REG, ((0UL << DMA_BUS_MODE_FB_BIT)   |
                                           (0UL << DMA_BUS_MODE_PR_BIT)   |
                                           (32UL << DMA_BUS_MODE_PBL_BIT) | // AHB burst size
                                           (1UL << DMA_BUS_MODE_DSL_BIT)  | 
                                           (0UL << DMA_BUS_MODE_DA_BIT)));

    // Write the address of the start of the tx descriptor array
    dma_reg_write(priv, DMA_TX_DESC_ADR_REG, (u32)priv->desc_base_addr);

    // Write the address of the start of the rx descriptor array
    dma_reg_write(priv, DMA_RX_DESC_ADR_REG,
        (u32)(priv->desc_base_addr + priv->tx_gmac_desc_list_info.num_descriptors));

    // Clear any pending interrupt requests
    dma_reg_write(priv, DMA_STATUS_REG, dma_reg_read(priv, DMA_STATUS_REG));

    // Initialise the GMAC DMA operation mode register, starting both the
    // transmitter and receiver
    dma_reg_write(priv, DMA_OP_MODE_REG, ((1UL << DMA_OP_MODE_SF_BIT)  |    // Store and forward
                                          (0UL << DMA_OP_MODE_TTC_BIT) |    // Tx threshold
                                          (1UL << DMA_OP_MODE_ST_BIT)  |    // Enable transmitter
                                          (0UL << DMA_OP_MODE_RTC_BIT) |    // Rx threshold
                                          (1UL << DMA_OP_MODE_SR_BIT)));    // Enable receiver

    // Success
    return 1;

err_out:
    eth_down();

    return 0;
}

void eth_halt(void)
{
    eth_down();

    // Disable the clock to the MAC block
    *(volatile u32*)(SYS_CTRL_CKEN_CLR_CTRL) = (1UL << SYS_CTRL_CKEN_MAC_BIT);
}

int eth_rx(void)
{
    static const int MAX_LOOPS = 2000;   // 2 seconds

    int length = 0;
    dma_addr_t dma_address;
    u32 desc_status;
    int loops = 0;

    // Look for the first available received packet
    while (loops++ < MAX_LOOPS) {
        if (get_rx_descriptor(priv, &desc_status, &dma_address, 0, 0) >= 0) {
            if (is_rx_valid(desc_status)) {
                // Get the length of the packet within the buffer
                length = get_rx_length(desc_status);

                // Pass packet up the network stack - will block until processing is
                // completed
                NetReceive((uchar*)dma_address, length);
            } else {
                printf("eth_rx() Received packet has bad desc_status = 0x%08x\n", desc_status);
            }

            // Re-initialise the RX descriptor with its buffer - relies on always
            // setting an RX descriptor directly after getting it
            if (set_rx_descriptor(priv, dma_address, ETHER_MTU + EXTRA_RX_SKB_SPACE, 0) < 0) {
                 printf("eth_rx(): Failed to set RX descriptor\n");
            }

            break;
        }

        // Wait a bit before trying again to get a descriptor
        udelay(1000);   // 1mS
    }
#ifdef DEBUG_GMAC
    if (loops >= MAX_LOOPS) printf("fail to detect rx packet\n");
#endif // DEBUG_GMAC
	

    return length;
}

int eth_send(volatile void *packet, int length)
{
    // Transmit the new packet
    while (1) {
        // Get the TX descriptor
        if (set_tx_descriptor(priv, (dma_addr_t)packet, length, 0) >= 0) {
            // Tell the GMAC to poll for the updated descriptor
            dma_reg_write(priv, DMA_TX_POLL_REG, 0);
            break;
        }

        // Wait a bit before trying again to get a descriptor
        udelay(1000);   // 1mS
    }

    // Wait for the packet buffer to be finished with
    while (get_tx_descriptor(priv, 0, 0, 0, 0) < 0) {
        // Wait a bit before examining the descriptor again
        udelay(1000);   // 1mS
    }

    return length;
}

