/*
 * linux/arch/arm/mach-oxnas/gmac_reg.h
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
#if !defined(__GMAC_REG_H__)
#define __GMAC_REG_H__

#include <asm/io.h>
#include "gmac.h"

/**
 * MAC register access functions
 */

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 */
static inline u32 mac_reg_read(gmac_priv_t* priv, int reg_num)
{
//printk("Reading MAC register %u at byte adr 0x%08x\n", reg_num, priv->macBase + (reg_num << 2));
    return readl(priv->macBase + (reg_num << 2));
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the MAC register to access
 */
static inline void mac_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
//printk("Writing MAC register %u at byte adr 0x%08x with 0x%08x\n", reg_num, priv->macBase + (reg_num << 2), value);
    writel(value, priv->macBase + (reg_num << 2));
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
//printk("Reading DMA register %u at byte adr 0x%08x\n", reg_num, priv->dmaBase + (reg_num << 2));
    return readl(priv->dmaBase + (reg_num << 2));
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the DMA register to access
 */
static inline void dma_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
//printk("Writing DMA register %u at byte adr 0x%08x with 0x%08x\n", reg_num, priv->dmaBase + (reg_num << 2), value);
    writel(value, priv->dmaBase + (reg_num << 2));
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

#define NUM_PERFECT_MATCH_REGISTERS 15

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
	MAC_INT_STATUS_REG	 = 14,
	MAC_INT_MASK_REG	 = 15,
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
    MAC_ADR15_LOW_REG    = 47,
    MAC_RGMII_STATUS_REG = 54
} gmac_mac_regs_t;

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the perfect matching low register
 * @param value A u32 specifying the value to write to the register
 */
static inline void mac_adrlo_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
    mac_reg_write(priv, MAC_ADR1_LOW_REG + (2*reg_num), value);
}

/**
 * @param priv A gmac_priv_t* pointing to private device data
 * @param reg_num An int specifying the index of the perfect matching high register
 * @param value A u32 specifying the value to write to the register
 */
static inline void mac_adrhi_reg_write(gmac_priv_t* priv, int reg_num, u32 value)
{
    mac_reg_write(priv, MAC_ADR1_HIGH_REG + (2*reg_num), value);
}

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
    MAC_FRAME_FILTER_HPF_BIT  = 10,
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
    DMA_STATUS_TTI_BIT = 29,
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

typedef enum gmac_dma_status_ts_val {
    DMA_STATUS_TS_CLOSING   = 7,
    DMA_STATUS_TS_SUSPENDED = 6,
    DMA_STATUS_TS_RESERVED  = 5,
    DMA_STATUS_TS_FLUSHING  = 4,
    DMA_STATUS_TS_READING   = 3,
    DMA_STATUS_TS_WAITING   = 2,
    DMA_STATUS_TS_FETCHING  = 1,
    DMA_STATUS_TS_STOPPED   = 0
} gmac_dma_status_ts_val_t;

typedef enum gmac_dma_status_rs_val {
    DMA_STATUS_RS_TRANSFERRING = 7,
    DMA_STATUS_RS_RESERVED_6   = 6,
    DMA_STATUS_RS_CLOSING      = 5,
    DMA_STATUS_RS_SUSPENDED    = 4,
    DMA_STATUS_RS_WAITING      = 3,
    DMA_STATUS_RS_RESERVED_2   = 2,
    DMA_STATUS_RS_FETCHING     = 1,
    DMA_STATUS_RS_STOPPED      = 0
} gmac_dma_status_rs_val_t;

typedef enum gmac_dma_op_mode_reg {
    DMA_OP_MODE_DT_BIT   = 26,
    DMA_OP_MODE_RSF_BIT  = 25,
    DMA_OP_MODE_DFF_BIT  = 24,
    DMA_OP_MODE_RFA2_BIT = 23,
    DMA_OP_MODE_RFD2_BIT = 22,
    DMA_OP_MODE_SF_BIT   = 21,
    DMA_OP_MODE_FTF_BIT  = 20,
    DMA_OP_MODE_TTC_BIT  = 14,
    DMA_OP_MODE_ST_BIT   = 13,
    DMA_OP_MODE_RFD_BIT  = 11,
    DMA_OP_MODE_RFA_BIT  = 9,
    DMA_OP_MODE_EFC_BIT  = 8,
    DMA_OP_MODE_FEF_BIT  = 7,
    DMA_OP_MODE_FUF_BIT  = 6,
    DMA_OP_MODE_RTC_BIT  = 3,
    DMA_OP_MODE_OSF_BIT  = 2,
    DMA_OP_MODE_SR_BIT   = 1
} gmac_dma_op_mode_reg_t;

#define DMA_OP_MODE_TTC_NUM_BITS 3

typedef enum gmac_dma_op_mode_ttc_val {
    DMA_OP_MODE_TTC_16  = 7,
    DMA_OP_MODE_TTC_24  = 6,
    DMA_OP_MODE_TTC_32  = 5,
    DMA_OP_MODE_TTC_40  = 4,
    DMA_OP_MODE_TTC_256 = 3,
    DMA_OP_MODE_TTC_192 = 2,
    DMA_OP_MODE_TTC_128 = 1,
    DMA_OP_MODE_TTC_64  = 0 
} gmac_dma_op_mode_ttc_val_t;

#define DMA_OP_MODE_RFD_NUM_BITS 2
#define DMA_OP_MODE_RFA_NUM_BITS 2
#define DMA_OP_MODE_RTC_NUM_BITS 2

typedef enum gmac_dma_op_mode_rtc_val {
    DMA_OP_MODE_RTC_128 = 3,
    DMA_OP_MODE_RTC_96  = 2,
    DMA_OP_MODE_RTC_32  = 1,
    DMA_OP_MODE_RTC_64  = 0 
} gmac_dma_op_mode_rtc_val_t;

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

#endif        //  #if !defined(__GMAC_REG_H__)

