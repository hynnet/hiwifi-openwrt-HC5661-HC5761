/*******************************************************************
 *
 * File:            dma.c
 *
 * Description:     DMA controller access functions.
 *
 * Date:            31 October 2005
 *
 * Author:          J J Larkworthy
 *
 * Copyright:       Oxford Semiconductor Ltd, 2005
 *
 *
 * 
 *******************************************************************/

#include "oxnas.h"

#include "dma.h"

/*******************************************************************
 *
 * Function:             encode_control_status
 *
 * Description:          DMA controller control status creating function 
 *                       probably over complex for this application - 
 *                       we need simple device to memory transfer for SATA to memory.
 *                       memory to device and device to memory for encryption/decryption of memory.
 *  
 *
 * 
 ******************************************************************/
unsigned long encode_control_status(oxnas_dma_device_settings_t *
				    src_settings,
				    oxnas_dma_device_settings_t *
				    dst_settings)
{
    unsigned long ctrl_status;
    oxnas_dma_transfer_direction_t direction;

    ctrl_status = DMA_CTRL_STATUS_PAUSE;	// Paused
    ctrl_status |= DMA_CTRL_STATUS_FAIR_SHARE_ARB;	// High priority
    ctrl_status |= (src_settings->dreq_ << DMA_CTRL_STATUS_SRC_DREQ_SHIFT);	// Dreq
    ctrl_status |= (dst_settings->dreq_ << DMA_CTRL_STATUS_DEST_DREQ_SHIFT);	// Dreq
    ctrl_status &= ~DMA_CTRL_STATUS_RESET;	// !RESET

    // Use new interrupt clearing register
    ctrl_status |= DMA_CTRL_STATUS_INTR_CLEAR_ENABLE;

    // Setup the transfer direction and burst/single mode for the two DMA busses
    if (src_settings->bus_ == OXNAS_DMA_SIDE_A) {
	// Set the burst/single mode for bus A based on src device's settings
	if (src_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
	    ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_A;
	} else {
	    ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_A;
	}

	if (dst_settings->bus_ == OXNAS_DMA_SIDE_A) {
	    direction = OXNAS_DMA_A_TO_A;
	} else {
	    direction = OXNAS_DMA_A_TO_B;

	    // Set the burst/single mode for bus B based on dst device's settings
	    if (dst_settings->transfer_mode_ ==
		OXNAS_DMA_TRANSFER_MODE_BURST) {
		ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_B;
	    } else {
		ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_B;
	    }
	}
    } else {
	// Set the burst/single mode for bus B based on src device's settings
	if (src_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
	    ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_B;
	} else {
	    ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_B;
	}

	if (dst_settings->bus_ == OXNAS_DMA_SIDE_A) {
	    direction = OXNAS_DMA_B_TO_A;

	    // Set the burst/single mode for bus A based on dst device's settings
	    if (dst_settings->transfer_mode_ ==
		OXNAS_DMA_TRANSFER_MODE_BURST) {
		ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_A;
	    } else {
		ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_A;
	    }
	} else {
	    direction = OXNAS_DMA_B_TO_B;
	}
    }
    ctrl_status |= (direction << DMA_CTRL_STATUS_DIR_SHIFT);

    // Setup source address mode fixed or increment
    if (src_settings->address_mode_ == OXNAS_DMA_MODE_FIXED) {
	// Fixed address
	ctrl_status &= ~(DMA_CTRL_STATUS_SRC_ADR_MODE);

	// Set up whether fixed address is _really_ fixed
	if (src_settings->address_really_fixed_) {
	    ctrl_status |= DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
	} else {
	    ctrl_status &= ~DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
	}
    } else {
	// Incrementing address
	ctrl_status |= DMA_CTRL_STATUS_SRC_ADR_MODE;
	ctrl_status &= ~DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
    }

    // Setup destination address mode fixed or increment
    if (dst_settings->address_mode_ == OXNAS_DMA_MODE_FIXED) {
	// Fixed address
	ctrl_status &= ~(DMA_CTRL_STATUS_DEST_ADR_MODE);

	// Set up whether fixed address is _really_ fixed
	if (dst_settings->address_really_fixed_) {
	    ctrl_status |= DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
	} else {
	    ctrl_status &= ~DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
	}
    } else {
	// Incrementing address
	ctrl_status |= DMA_CTRL_STATUS_DEST_ADR_MODE;
	ctrl_status &= ~DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
    }

    // Set up the width of the transfers on the DMA buses
    ctrl_status |=
	(src_settings->width_ << DMA_CTRL_STATUS_SRC_WIDTH_SHIFT);
    ctrl_status |=
	(dst_settings->width_ << DMA_CTRL_STATUS_DEST_WIDTH_SHIFT);

    // Setup the priority arbitration scheme
    ctrl_status &= ~DMA_CTRL_STATUS_STARVE_LOW_PRIORITY;	// !Starve low priority

    return ctrl_status;
}

/*******************************************************************
 *
 * Function:             encode_final_eot
 *
 * Description:          encode what the DMA controller should do at the end of the transfer
 * 
 ******************************************************************/
u32 encode_final_eot(oxnas_dma_device_settings_t * src_settings,
		     oxnas_dma_device_settings_t * dst_settings,
		     unsigned long length)
{
    // Write the length, with EOT configuration for a final transfer
    unsigned long encoded = length;
    if (dst_settings->write_final_eot_) {
	encoded |= DMA_BYTE_CNT_WR_EOT_MASK;
    } else {
	encoded &= ~DMA_BYTE_CNT_WR_EOT_MASK;
    }
    if (src_settings->read_final_eot_) {
	encoded |= DMA_BYTE_CNT_RD_EOT_MASK;
    } else {
	encoded &= ~DMA_BYTE_CNT_RD_EOT_MASK;
    }
    
    encoded |= DMA_BYTE_CNT_BURST_MASK;
    return encoded;
}


/*******************************************************************
 *
 * Function:             encode_start
 *
 * Description:          encode the start command
 * 
 ******************************************************************/
u32 encode_start(u32 ctrl_status)
{
    return ctrl_status & ~DMA_CTRL_STATUS_PAUSE;
}
