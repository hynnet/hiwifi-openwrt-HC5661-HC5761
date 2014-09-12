/*******************************************************************
 *
 * File:            dma.h
 *
 * Description:     DMA declaration file.
 *
 * Date:            31 October 2005
 *
 * Author:          J J Larkworthy
 *
 * Copyright:       Oxford Semiconductor Ltd, 2005
 *
 *
 *******************************************************************/
#ifndef _DMA_H_
#define _DMA_H_
/* DMA controller macros definitions */
#define SATA_DMA_CHANNEL 0

#define DMA_CTRL_STATUS      (0x0)
#define DMA_BASE_SRC_ADR     (0x4)
#define DMA_BASE_DST_ADR     (0x8)
#define DMA_BYTE_CNT         (0xC)
#define DMA_CURRENT_SRC_ADR  (0x10)
#define DMA_CURRENT_DST_ADR  (0x14)
#define DMA_CURRENT_BYTE_CNT (0x18)
#define DMA_INTR_ID          (0x1C)
#define DMA_INTR_CLEAR_REG   (DMA_CURRENT_SRC_ADR)

#define DMA_CALC_REG_ADR(channel, register) ((volatile u32*)(DMA_BASE + ((channel) << 5) + (register)))

#define DMA_CTRL_STATUS_FAIR_SHARE_ARB            (1 << 0)
#define DMA_CTRL_STATUS_IN_PROGRESS               (1 << 1)
#define DMA_CTRL_STATUS_SRC_DREQ_MASK             (0x0000003C)
#define DMA_CTRL_STATUS_SRC_DREQ_SHIFT            (2)
#define DMA_CTRL_STATUS_DEST_DREQ_MASK            (0x000003C0)
#define DMA_CTRL_STATUS_DEST_DREQ_SHIFT           (6)
#define DMA_CTRL_STATUS_INTR                      (1 << 10)
#define DMA_CTRL_STATUS_NXT_FREE                  (1 << 11)
#define DMA_CTRL_STATUS_RESET                     (1 << 12)
#define DMA_CTRL_STATUS_DIR_MASK                  (0x00006000)
#define DMA_CTRL_STATUS_DIR_SHIFT                 (13)
#define DMA_CTRL_STATUS_SRC_ADR_MODE              (1 << 15)
#define DMA_CTRL_STATUS_DEST_ADR_MODE             (1 << 16)
#define DMA_CTRL_STATUS_TRANSFER_MODE_A           (1 << 17)
#define DMA_CTRL_STATUS_TRANSFER_MODE_B           (1 << 18)
#define DMA_CTRL_STATUS_SRC_WIDTH_MASK            (0x00380000)
#define DMA_CTRL_STATUS_SRC_WIDTH_SHIFT           (19)
#define DMA_CTRL_STATUS_DEST_WIDTH_MASK           (0x01C00000)
#define DMA_CTRL_STATUS_DEST_WIDTH_SHIFT          (22)
#define DMA_CTRL_STATUS_PAUSE                     (1 << 25)
#define DMA_CTRL_STATUS_INTERRUPT_ENABLE          (1 << 26)
#define DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED      (1 << 27)
#define DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED (1 << 28)
#define DMA_CTRL_STATUS_STARVE_LOW_PRIORITY       (1 << 29)
#define DMA_CTRL_STATUS_INTR_CLEAR_ENABLE         (1 << 30)

#define DMA_BYTE_CNT_MASK        ((1 << 21) - 1)
#define DMA_BYTE_CNT_WR_EOT_MASK (1 << 30)
#define DMA_BYTE_CNT_RD_EOT_MASK (1 << 31)
#define DMA_BYTE_CNT_HPROT_MASK (1 << 29)
#define DMA_BYTE_CNT_BURST_MASK (1 << 28)


/* DMA controller data structures */

typedef u32 lbaint_t;

typedef enum oxnas_dma_mode {
    OXNAS_DMA_MODE_FIXED,
    OXNAS_DMA_MODE_INC
} oxnas_dma_mode_t;

typedef enum oxnas_dma_direction {
    OXNAS_DMA_TO_DEVICE,
    OXNAS_DMA_FROM_DEVICE
} oxnas_dma_direction_t;

/* The available buses to which the DMA controller is attached */
typedef enum oxnas_dma_transfer_bus {
    OXNAS_DMA_SIDE_A,
    OXNAS_DMA_SIDE_B
} oxnas_dma_transfer_bus_t;

/* Direction of data flow between the DMA controller's pair of interfaces */
typedef enum oxnas_dma_transfer_direction {
    OXNAS_DMA_A_TO_A,
    OXNAS_DMA_B_TO_A,
    OXNAS_DMA_A_TO_B,
    OXNAS_DMA_B_TO_B
} oxnas_dma_transfer_direction_t;

/* The available data widths */
typedef enum oxnas_dma_transfer_width {
    OXNAS_DMA_TRANSFER_WIDTH_8BITS,
    OXNAS_DMA_TRANSFER_WIDTH_16BITS,
    OXNAS_DMA_TRANSFER_WIDTH_32BITS
} oxnas_dma_transfer_width_t;

/* The mode of the DMA transfer */
typedef enum oxnas_dma_transfer_mode {
    OXNAS_DMA_TRANSFER_MODE_SINGLE,
    OXNAS_DMA_TRANSFER_MODE_BURST
} oxnas_dma_transfer_mode_t;

/* The available transfer targets */
typedef enum oxnas_dma_dreq {
    OXNAS_DMA_DREQ_SATA = 0,
    OXNAS_DMA_DREQ_MEMORY = 15
} oxnas_dma_dreq_t;


typedef struct oxnas_dma_device_settings {
    unsigned long address_;
    unsigned fifo_size_;	// Chained transfers must take account of FIFO offset at end of previous transfer
    unsigned char dreq_;
    unsigned read_eot_:1;
    unsigned read_final_eot_:1;
    unsigned write_eot_:1;
    unsigned write_final_eot_:1;
    unsigned bus_:1;
    unsigned width_:2;
    unsigned transfer_mode_:1;
    unsigned address_mode_:1;
    unsigned address_really_fixed_:1;
} oxnas_dma_device_settings_t;

extern u32 encode_start(u32 ctrl_status);

extern unsigned long encode_control_status(oxnas_dma_device_settings_t *
					   src_settings,
					   oxnas_dma_device_settings_t *
					   dst_settings);
/* check if the DMA controller is still busy */

extern u32 encode_final_eot(oxnas_dma_device_settings_t * src_settings,
			    oxnas_dma_device_settings_t * dst_settings,
			    unsigned long length);

#endif				//_DMA_H_
