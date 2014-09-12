/*
 * linux/include/asm-arm/arch-oxnas/i2s.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_I2S_H
#define __ASM_ARM_ARCH_I2S_H

#include "hardware.h"

/* Routines ----------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

extern void DumpI2SRegisters(void);



/* Registers ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


#define    TX_CONTROL                      (0x0000 + I2S_BASE)
#define    TX_SETUP                        (0x0004 + I2S_BASE)
#define    TX_SETUP1                       (0x0008 + I2S_BASE)
#define    TX_STATUS                       (0x000C + I2S_BASE)
#define    RX_CONTROL                      (0x0010 + I2S_BASE)
#define    RX_SETUP                        (0x0014 + I2S_BASE)
#define    RX_SETUP1                       (0x0018 + I2S_BASE)
#define    RX_STATUS                       (0x001C + I2S_BASE)
#define    TX_DEBUG                        (0x0020 + I2S_BASE)
#define    TX_DEBUG2                       (0x0024 + I2S_BASE)
#define    TX_DEBUG3                       (0x0028 + I2S_BASE)
#define    RX_DEBUG_                       (0x0030 + I2S_BASE)
#define    RX_DEBUG2                       (0x0034 + I2S_BASE)
#define    RX_DEBUG3                       (0x0038 + I2S_BASE)
#define    TX_BUFFER_LEVEL                 (0x0040 + I2S_BASE)
#define    TX_BUFFER_INTERRUPT_LEVEL       (0x0048 + I2S_BASE)
#define    RX_BUFFER_LEVEL                 (0x0050 + I2S_BASE)
#define    RX_BUFFER_INTERRUPT_LEVEL       (0x0058 + I2S_BASE)
#define    RX_SPDIF_DEBUG                  (0x0070 + I2S_BASE)
#define    RX_SPDIF_DEBUG2                 (0x0074 + I2S_BASE)
#define    INTERRUPT_CONTROL_STATUS        (0x0080 + I2S_BASE)
#define    INTERRUPT_MASK                  (0x0084 + I2S_BASE)
#define    VERSION                         (0x008C + I2S_BASE)
#define    TX_DATA_IN_FORMAT               (0x0100 + I2S_BASE)
#define    TX_CHANNELS_ENABLE              (0x0104 + I2S_BASE)
#define    TX_WRITES_TO                    (0x0108 + I2S_BASE)
#define    RX_DATA_OUT_FORMAT              (0x0200 + I2S_BASE)
#define    RX_CHANNELS_ENABLE              (0x0204 + I2S_BASE)
#define    RX_READS_FROM                   (0x0208 + I2S_BASE)
#define    TX_CPU_DATA_WRITES_ALT          (0x04FF + I2S_BASE)
#define    RX_CPU_DATA_READS_ALT           (0x08FF + I2S_BASE)
#define    TX_CPU_DATA_WRITES              (0x1FFF + I2S_BASE)
#define    RX_CPU_DATA_READS               (0x2FFF + I2S_BASE)



/* TX_CONTROL ------------ */
#define    TX_CONTROL_ENABLE                0
    /**
     * 0 - Audio transmit is disabled 
     * 1 - Audio transmit is enabled. 
     */
#define    TX_CONTROL_FLUSH                 1
    /**
     * 0 - Normal Operation 
     * 1 - Flush TX buffer 
     */
#define    TX_CONTROL_MUTE                  2
    /**
     * 0 - Normal Operation 
     * 1 - Muted operation 
     */
#define    TX_CONTROL_TRICK                 3
    /**
     *     (UNTESTED FEATURE)   
     * 0 - Trickplay features disabled 
     * 1 - Trickplay features enabled 
     */
#define    TX_CONTROL_SPEED                 4
    /**
     *  (UNTESTED FEATURE)  
     * 00 - Quarter speed playback    - DOESN’T WORK 
     * 01 - Half speed playback       - DOESN’T WORK 
     * 10 - Double speed playback 
     * 11 - Quadruple speed playback. 
     */
    #define QUARTER_SPEED_PLAYBACK          0x00 
    #define HALF_SPEED_PLAYBACK             0x01 
    #define DOUBLE_SPEED_PLAYBACK           0x02 
    #define QUADRUPLE_SPEED_PLAYBACK        0x03 
#define    TX_CONTROL_ABORT_DMA             6    
    /**
     * Write a 1 to abort any outstanding DMAs (self-clearing)     
     */
#define    TX_CONTROL_AHB_ENABLE            8
    /**
     * 0 - AHB disabled (APB only) 
     * 1 - AHB enabled (APB disabled for data plane) 
     */
#define    TX_CONTROL_QUAD_BURSTS           9
    /**
     * 0 - DMA transfers up to Single Quads 
     * 1 - DMA transfers up to Bursts of 4 Quads 
     */
    
    
/* TX_SETUP ------------ */ 
/**
 * In order for the changes made to the fields in bold to propagate to the Tx  
 * audio clock domain, a write must be performed to TX_SETUP1  And the TX_CLK   
 * must be running. 
 */

#define    TX_SETUP_FORMAT                  0  
    /**
     * 00 - True/Early-I2S (standard format) 
     * 01 - Late-I2S 
     * 10 - MP3 Valid 
     * 11 - MP3 Start 
     */
    #define TRUE_I2S                        0x00
    #define LATE_I2S                        0x01
    #define MP3_VALID                       0x02
    #define MP3_START                       0x03
#define    TX_SETUP_MODE                    2
    /**
     * 0 - Slave 
     * 1 - Master 
     */
    #define I2S_SLAVE                       0x00
    #define I2S_MASTER                      0x01
#define    TX_SETUP_FLOW_INVERT             3
    /**
     * 0 - Flow Control is active high  
     *     (ie. Audio core is held off when TX_FLOW_CONTROL=’1’) 
     * 1 - Flow Control is active low 
     *     (ie. Audio core is held off when TX_FLOW_CONTROL=’0’) 
     */
    #define FLOW_CONTROL_ACTIVE_HIGH        0
    #define FLOW_CONTROL_ACTIVE_LOW         1
#define    TX_SETUP_POS_EDGE                4
    /**
     * 0 - Data is output on the negative edge of the TX Audio clock 
     * 1 - Data is output on the positive edge of the TX Audio clock 
     */
#define    TX_SETUP_CLOCK_STOP              5
    /**
     * 0 - Audio TX clock should not be stopped during flow control hold off 
     * 1 - Audio TX clock should be stopped during flow control hold off 
     */
#define    TX_SETUP_SPLIT_QUAD              6
    /**
     * 0 - No splitting 
     * 1 - Split quadlet into two 16 bit samples - NOT VALID FOR S/PDIF 
     */
#define    TX_SETUP_INSPECT_WORD_CLOCK      7
    /**
     * Used when recovering from underrun. This bit is set to look for either  
     * a high or low word clock before the hardware carries on as normal. 
     */
#define    TX_SETUP_SPDIF_EN                8
    /**
     * 0 - No SPDIF on Channel 0 (disables biphase coding on output) 
     * 1 - SPDIF on Channel 0 (enables the biphase coding even when Tx disabled) 
     */

 
/* TX_SETUP1 ------------ */
#define    TX_SETUP1_INPUT                  0    
    /**
     * 00 - Twos Compliment (pass-thru) 
     * 01 - Offset Binary 
     * 10 - Sign Magnitude 
     * 11 - Reserved 
     */
    #define TWOS_COMPLIMENT                 0x00
    #define OFFSET_BINARY                   0x01
    #define SIGN_MAGNITUDE                  0x02
    #define RESERVED                        0x03
#define    TX_SETUP1_REVERSE                2
    /**
     * 0 - Normal operation 
     * 1 - Reverse Stereo        - DOES NOT WORK FOR NON- SPLIT_QUAD/MP3 
     */
#define    TX_SETUP1_INVERT                 3
    /**
     * 0 - Normal operation 
     * 1 - Inverted word clock 
     */
#define    TX_SETUP1_BIG_ENDIAN             4
    /**
     * 0 - System data is in little endian format 
     * 1 - System data is in big endian format 
     */
#define    TX_SETUP1_QUAD_ENDIAN            5
    /**
     * 0 - System data is 16 bit. 
     * 1 - System data is 32 bit 
     */
#define    TX_SETUP1_QUAD_SAMPLES           6
    /**
     * 0 - I2S Master word clock uses 16 bit samples    - NOT VALID FOR S/PDIF 
     * 1 - I2S Master word clock uses 32 bit samples 
     */
#define    TX_SETUP1_FLOW_CONTROL           7
    /**
     * 0 - No flow control (MP3 only) 
     * 1 - Flow control (MP3 only) 
     */

/* TX_STATUS ------------ */
#define    TX_STATUS_UNDERRUN               0
    /**
     * 0 - Underrun has not occurred. 
     * 1 - Underrun has occurred. (Note that this bit must be written to with  
     *     a ‘1’ to clear) 
     */
#define    TX_STATUS_OVERRUN                1
    /**
     * 0 - Overrun has not occurred. 
     * 1 - Overrun has occurred. (Note that this bit must be written to with  
     *     a ‘1’ to clear) 
     */
#define    TX_STATUS_FIFO_UNDERRUN          2
    
    /**
     * 0 - FIFO Underrun has not occurred. 
     * 1 - FIFO Underrun has occurred. (Note that this bit must be written to  
     *     with a ‘1’ to clear) 
     */
#define    TX_STATUS_FIFO_OVERRUN           3    
    /**
     * 0 - FIFO Overrun has not occurred. 
     * 1 - FIFO Overrun has occurred. (Note that this bit must be written to  
     *     with a ‘1’ to clear) 
     */
#define    TX_STATUS_HW_READ                8
    /**
     * 0 - H/W Read of FIFO has not occurred. 
     * 1 - H/W Read of FIFO has occurred. (Note that this bit must be written  
     *     to with a ‘1’ to clear) 
     */
#define    TX_STATUS_BUFFER_LEVEL_MET       9
    /**
     * 0 - Fill level of FIFO does not match BUFFER_INTERRUPT_LEVEL. 
     * 1 - Fill level of FIFO matches BUFFER_INTERRUPT_LEVEL 
     */

 
/* RX_CONTROL ------------ */
#define    RX_CONTROL_ENABLE                0
    /**
     * 0 - Audio receive is disabled 
     * 1 - Audio receive is enabled. 
     */
#define    RX_CONTROL_FLUSH                 1
    /**
     * 0 - Normal Operation 
     * 1 - Flush RX buffer 
     */
#define    RX_CONTROL_MUTE                  2
    /**
     * 0 - Normal Operation 
     * 1 - Muted operation 
     */
#define    RX_CONTROL_TRICK                 3
    /**
     * (UNTESTED FEATURE) 
     * 0 - Trickplay features disabled 
     * 1 - Trickplay features enabled 
     */
#define    RX_CONTROL_SPEED                 4   
    /**
     * (UNTESTED FEATURE) 
     * 00 - Quarter speed playback     - DOESN’T WORK 
     * 01 - Half speed playback        - DOESN’T WORK 
     * 10 - Double speed playback 
     * 11 - Quadruple speed playback. 
     */
#define    RX_CONTROL_ABORT_DMA             6
    /**
     * Write a 1 to abort any outstanding DMAs (self-clearing)  
     */
#define    RX_CONTROL_AHB_ENABLE            8    
    /**
     * 0 - AHB disabled (APB only) 
     * 1 - AHB enabled (APB disabled for data plane) 
     */
#define    RX_CONTROL_QUAD_BURSTS           9
    /**
     * 0 - DMA transfers up to Single Quads 
     * 1 - DMA transfers up to Bursts of 4 Quads 
     */

    
/* RX_SETUP ------------ */ 
/**
 * In order for the changes made to the fields in bold to propagate to the Rx 
 * audio clock domain, a write must be performed to RX_SETUP1  And the RX_CLK 
 * must be running. 
 */

#define    RX_SETUP_FORMAT                  0   
    /**
     * 00 - True/Early-I2S (standard format) 
     * 01 - Late-I2S 
     * 10 - MP3 Valid 
     * 11 - MP3 Start 
     */
#define    RX_SETUP_MODE                    2
    /**
     * 0 - Slave 
     * 1 - Master - NOT VALID FOR S/PDIF 
     */
#define    RX_SETUP_POS_EDGE                4
    /**
     * 0 - Data is output on the negative edge of the RX Audio clock 
     * 1 - Data is output on the positive edge of the RX Audio clock 
     */
#define    RX_SETUP_COMBINE_QUAD            6
    /**
     * 0 - No combining 
     * 1 - Combine two 16 bit samples into single quadlet - NOT VALID FOR S/PDIF 
     */
#define    RX_SETUP_INSPECT_WORD_CLOCK      7
    /**
     * Used when recovering from overrun. This bit is set to look for either  
     * a high or low word clock before the hardware carries on as normal.   
     */
#define    RX_SETUP_SPDIF_EN                8
    /**
     * 0 - No SPDIF on Channel 0 (disables biphase decoding and clk recovery on input) 
     * 1 - SPDIF on Channel 0 (enables the biphase decoding and clk recovery even when Rx disabled) 
     */
#define    RX_SETUP_SPDIF_DEBUG_EN          9
    /**
     * Provides direct control over RX_SPDIF_OE  Output signal 
     */

 
/* RX_SETUP1 ------------ */
#define    RX_SETUP1_INPUT                  0       
    /**
     * 00 - Twos Compliment (pass-thru) 
     * 01 - Offset Binary 
     * 10 - Sign Magnitude 
     * 11 - Reserved 
     */
#define    RX_SETUP1_REVERSE                2
    /**
     * 0 - Normal operation 
     * 1 - Reverse Stereo - DOES NOT WORK FOR NON- SPLIT_QUAD/MP3 
     */
#define    RX_SETUP1_INVERT                 3
    /**
     * 0 - Normal operation 
     * 1 - Inverted word clock 
     */
#define    RX_SETUP1_BIG_ENDIAN             4
    /**
     * 0 - System data is in little endian format 
     * 1 - System data is in big endian format 
     */
#define    RX_SETUP1_QUAD_ENDIAN            5
    /**
     * 0 - System data is 16 bit. 
     * 1 - System data is 32 bit 
     */
#define    RX_SETUP1_QUAD_SAMPLES           6
    /**
     * 0 - I2S Master word clock uses 16 bit samples    - NOT VALID FOR S/PDIF 
     * 1 - I2S Master word clock uses 32 bit samples 
     */

    
/* RX_STATUS ------------ */
#define    RX_STATUS_UNDERRUN               0
    /**
     * 0 - Underrun has not occurred. 
     * 1 - Underrun has occurred. (Note that this bit must be written to with
     *     a ‘1’ to clear) 
     */
#define    RX_STATUS_OVERRUN                1
    /**
     * 0 - Overrun has not occurred. 
     * 1 - Overrun has occurred. (Note that this bit must be written to with
     *     a ‘1’ to clear) 
     */
#define    RX_STATUS_FIFO_UNDERRUN          2
    /**
     * 0 - FIFO Underrun has not occurred. 
     * 1 - FIFO Underrun has occurred. (Note that this bit must be written to 
     *     with a ‘1’ to clear) 
     */
#define    RX_STATUS_FIFO_OVERRUN           3
    /**
     * 0 - FIFO Overrun has not occurred. 
     * 1 - FIFO Overrun has occurred. (Note that this bit must be written to 
     *     with a ‘1’ to clear) 
     */
#define    RX_STATUS_SPDIF_PARITY_ERROR     4
    /**
     * 0 - Rx S/PDIF Parity Error has not occurred. 
     * 1 - Rx S/PDIF Parity Error has occurred. (Note that this bit must be 
     *     written to with a ‘1’ to clear) 
     */
#define    RX_STATUS_SPDIF_PREAMBLE_ERROR   5
    /**
     * 0 - Rx S/PDIF Preamble Error has not occurred. 
     * 1 - Rx S/PDIF Preamble Error has occurred. (Note that this bit must be 
     *     written to with a ‘1’ to clear) 
     */
#define    RX_STATUS_SPDIF_FRAMING_ERROR    6
    /**
     * 0 - Rx S/PDIF Framing Error has not occurred. 
     * 1 - Rx S/PDIF Framing Error has occurred. (Note that this bit must be 
     *     written to with a ‘1’ to clear) 
     */
#define    RX_STATUS_SPDIF_LOCK_EVENT       7
    /**
     * 0 - Rx S/PDIF Lock status has not changed. 
     * 1 - Rx S/PDIF Lock status has changed. (Note that this bit must be 
     *     written to with a ‘1’ to clear) 
     */
#define    RX_STATUS_HW_WRITE               8
    /**
     * 0 - H/W Write of FIFO has not occurred. 
     * 1 - H/W Write of FIFO has occurred. (Note that this bit must be written 
     *     to with a ‘1’ to clear) 
     */
#define    RX_STATUS_BUFFER_LEVEL_MET       9
    /**
     * 0 - Fill level of FIFO does not match BUFFER_INTERRUPT_LEVEL. 
     * 1 - Fill level of FIFO matches BUFFER_INTERRUPT_LEVEL 
     */
#define    RX_STATUS_SPDIF_LOCK             10
    /**
     * 0 - Rx S/PDIF timing is not locked. 
     * 1 - Rx S/PDIF timing is locked. 
     */

 
/* TX_DEBUG ------------ */
#define TX_DEBUG_TX_DMA_STATE               0
    /**
     * 00 - START 
     * 01 - IDLE 
     * 10 - BURST 
     * 11 - NON_ALIGNED 
     */
#define TX_DEBUG_WRITE_OFFSET               4
    /**
     * Write Offset Determines the alignment of the writes  
     * (0=quad aligned; 1/3=bytes aligned; 2 doublet aligned) 
     */
#define    TX_DEBUG_TX_DREQ                 31
    /**
     * Set if DMA request for Tx is set 
     */
    

/* TX_DEBUG2 ------------ */
#define TX_DEBUG2_TX_CTRL_STATE             0
    /**
     * 000 - WAITING 
     * 001 - WAITING_HALF 
     * 010 - Invalid 
     * 011 - WAIT_LEFT 
     * 100 - READ_FIFO 
     * 101 - WAIT_TILL_DATA_TAKEN 
     * 11x - Invalid 
     */
#define    TX_DEBUG2_FIFO_READER_2ND_STAGE  3
    /**
     * 0 - First stage (L) of FIFO read 
     * 1 - Second stage (R) of FIFO read 
     */
#define    TX_DEBUG2_FIFO_READER_CHANNEL    4
    /**
     * Current channel being read from FIFO 
     */
#define    TX_DEBUG2_NUM_WRITES             8
    /**
     * Number of writes to be performed every interrupt
     */
#define    TX_DEBUG2_SAFE_TO_UPDATE_REGS    12
    /**
     * 0 - Changes to registers will be deferred 
     * 1 - Changes to registers will be immediate 
     */
#define    TX_DEBUG2_WR_DMARQ               13
    /**
     * 0 - No writes being requested 
     * 1 - Writes being requested (=DMA DREQ when using APB) 
     */
#define    TX_DEBUG2_FIFO_READ_ADDRESS      16
    /**
     * 19 - 16    Current read pointer of FIFO 
     */
#define    TX_DEBUG2_FIFO_WRITE_ADDRESS     24
    /**
     * 27 - 24     Current write pointer of FIFO 
     */
#define    TX_DEBUG2_WORD_CLK               31
    /** 
     * State of internal safe TX_WORD_CLK 
     */

 
/* TX_DEBUG3 ------------ */ 
/**
 * This register is a read back of status directly from the TX_CLK  domain.   
 * For this reason it may be unstable during operation.  Several back-to-back  
 * reads might need to be made to get an accurate reflection of the status  
 * during operation. 
 */

#define    TX_DEBUG3_TX_STATE               0
    /**
     * 000 - DISABLED 
     * 001 - WAITING 
     * 010 - WAIT_FOR_8 
     * 011 - SAMPLE_8 
     * 100 - WAIT_FOR_LEFT 
     * 101 - SAMPLE_LEFT 
     * 110 - WAIT_FOR_RIGHT 
     * 111 - SAMPLE_RIGHT 
     */
#define    TX_DEBUG3_TX_IN_RESET            31
    /**
     * 0 - Tx domain is out of reset 
     * 1 - Tx domain is in reset 
     */


 
/* RX_DEBUG ------------ */
#define    RX_DEBUG_RX_DMA_STATE            0
    /**
     * 00 - START 
     * 01 - IDLE 
     * 10 - BURST 
     * 11 - NON_ALIGNED 
     */
#define    RX_DEBUG_READ_OFFSET             4
    /**
     * 5 - 4  Determines the alignment of the reads  
     *        (0=quad aligned; 1/3=bytes aligned; 2 doublet aligned) 
     */
#define    RX_DEBUG_RX_DREQ                 31
    /**
     * Set if DMA request for Rx is set 
     */

    
/* RX_DEBUG2 ------------ */
#define    RX_DEBUG2_RX_CTRL_STATE          0
    /**
     * 000 - WAITING 
     * 001 - WAITING_HALF 
     * 01x - Invalid 
     * 100 - WRITE_FIFO 
     * 101 - WAIT_TILL_DATA_PUT 
     * 11x - Invalid 
     */
#define    RX_DEBUG2_FIFO_WRITER_2ND_STAGE  3
    /**
     * 0 - First stage (L) of FIFO write 
     * 1 - Second stage (R) of FIFO write 
     */
#define    RX_DEBUG2_FIFO_WRITER_CHANNEL    4
    /**
     * 7 - 4  Current channel being written to FIFO 
     */
#define    RX_DEBUG2_NUM_READS              8
    /**
     * 11 - 8  Number of reads to be performed every interrupt 
     */
#define    RX_DEBUG2_SAFE_TO_UPDATE_REGS    12
    /**
     * 0 - Changes to registers will be deferred 
     * 1 - Changes to registers will be immediate 
     */
#define    RX_DEBUG2_RD_DMARQ               13
    /**
     * 0 - No reads being requested 
     * 1 - Reads being requested (=DMA DREQ when using APB) 
     */
#define    RX_DEBUG2_FIFO_READ_ADDRESS      16
    /**
     * 19 - 16   Current read pointer of FIFO 
     */
#define    RX_DEBUG2_FIFO_WRITE_ADDRESS     24
    /**
     * 27 - 24   Current write pointer of FIFO 
     */
#define    RX_DEBUG2_WORD_CLK               31
    /**
     * State of internal safe RX_WORD_CLK     
     */

 
/* RX_DEBUG3 ------------ */ 
/**
 * This register is a read back of status directly from the RX_CLK  domain.   
 * For this reason it may be unstable during operation.  Several back-to-back  
 * reads might need to be made to get an accurate reflection of the status 
 * during operation. 
 */

#define    RX_DEBUG3_RX_STATE               0
    /**
     * 000 - DISABLED 
     * 001 - WAITING 
     * 010 - WAIT_FOR_8 
     * 011 - SAMPLE_8 
     * 100 - WAIT_FOR_LEFT 
     * 101 - SAMPLE_LEFT 
     * 110 - WAIT_FOR_RIGHT 
     * 111 - SAMPLE_RIGHT 
     */
#define    RX_DEBUG3_RX_IN_RESET            31
    /**
     * 0 - Rx domain is out of reset 
     * 1 - Rx domain is in reset 
     */


/**
 * VERSION* ------------ 
 * Refer to VERSION. ?? 
 */

 
/* TX_BUFFER_LEVEL ------------ */ 
/**
 * When this register is read it returns the current fill level of the buffer  
 * within the audio core, when a ‘1’ is written to the lower bit, the buffer  
 * fill level, read and write pointers are all set to zero. 
 * FT - 0    Buffer Level    Fill level of Tx buffer 
 */

/**
 * INTERRUPT_CONTROL* ------------ 
 * Refer to INTERRUPT_CONTROL_STATUS. ?? 
 */

/* TX_BUFFER_INTERRUPT_LEVEL ------------ */ 
/**
 * FT - 0    Interrupt Level    Programmable Buffer Level to initiate Interrupt 
 */


/* RX_BUFFER_LEVEL ------------ */ 
/**
 * When this register is read it returns the current fill level of the buffer 
 * within the audio core, when a ‘1’ is written to the lower bit, the buffer  
 * fill level, read and write pointers are all set to zero. 
 * FR - 0    Buffer Level    Fill level of Rx buffer 
 */


/* RX_BUFFER_INTERRUPT_LEVEL ------------ */ 
/**
 * FR - 0    Interrupt Level    Programmable Buffer Level to initiate Interrupt 
 */

 
/* RX_SPDIF_DEBUG ------------ */
#define    RX_SPDIF_DEBUG_MAX_PULSE         0
    /**
     * 7 - 0    Number of cycles of RX_SPDIF_OSAMP_CLK  in maximum pulse width  
     *          detected (=1.5x BIT_CLK) 
     */
#define    RX_SPDIF_DEBUG_MIN_PULSE         8
    /**
     * 15 - 8   Number of cycles of RX_SPDIF_OSAMP_CLK  in minimum pulse width  
     *          detected (=0.5x BIT_CLK) 
     */
#define    RX_SPDIF_DEBUG_VALID             16
    /**
     * 0 - Min Pulse is not valid (<2) for doing recovery 
     * 1 - Min Pulse is valid (?2) and recovery is possible 
     */
#define    RX_SPDIF_DEBUG_LOCK              17
    /**
     * 0 - Rx S/PDIF timing is not locked. 
     * 1 - Rx S/PDIF timing is locked. 
     */
#define    RX_SPDIF_DEBUG_NO_PULSE          18
    /**
     * 0 - Pulse detected 
     * 1 - Pulse detection has timed out 
     */
#define    RX_SPDIF_DEBUG_BLOCK_START       20
    /**
     * 0 - Current frame is not at start of block 
     * 1 - Current frame is at start of block 
     */
#define    RX_SPDIF_DEBUG_CHAN_A            21
    /**
     * 0 - Current subframe is B 
     * 1 - Current subframe is A 
     */

    
/* RX_SPDIF_DEBUG2 ------------ */
#define    RX_SPDIF_DEBUG2_FRAME            0
    /**
     * 7 - 0    Current frame counter (0 to 191) 
     */
#define    RX_SPDIF_DEBUG2_BLOCK_SYNC       8
    /**
     * 0 - Not yet achieved block sync 
     * 1 - Block sync has been achieved (i.e. received a start of block) 
     */

 
/* INTERRUPT_CONTROL_STATUS ------------ */
#define    INTERRUPT_CONTROL_STATUS_AUDIO_IRQ 0
    /**
     * ** BACKWARDS COMPATIBLE, DO NOT USE 
     * Set if the Audio Core Interrupt is set. Write ‘0’ to clear                  
     */
#define    INTERRUPT_CONTROL_STATUS_AUTO_CLEAR 2
    /**
     * 0 - Auto Clear disabled 
     * 1 - Auto clear of H/W read/write interrupt on next data write/read (Tx/Rx) 
     */
#define    INTERRUPT_CONTROL_STATUS_TX_IRQ  8
    /**
     * Set if the Audio Core Tx Interrupt is set  
     * Set to ‘1’ to trigger a Normal Interrupt (TX_READ/RX_WRITE  IRQ Enable 
     * must be set) 
     */
#define    INTERRUPT_CONTROL_STATUS_TX_ERR_IRQ 9
    /**
     * Set if the Audio Core Tx Error Interrupt is set 
     * Set to ‘1’ to trigger an Error Interrupt (TX_URUN/RX_ORUN  IRQ Enable  
     * must be set) 
     */
#define    INTERRUPT_CONTROL_STATUS_RX_IRQ  16
    /**
     * Set if the Audio Core Rx Interrupt is set 
     * Set to ‘1’ to trigger a Normal Interrupt (RX_WRITE/TX_READ  IRQ Enable 
     * must be set) 
     */
#define    INTERRUPT_CONTROL_STATUS_RX_ERR_IRQ 17
   /**
    * Set if the Audio Core Rx Error Interrupt is set 
    * Set to ‘1’ to trigger an Error Interrupt (RX_ORUN/TX_URUN  IRQ Enable must 
    * be set) 
    */
   

/* INTERRUPT_MASK ------------ */
#define    INTERRUPT_MASK_TX_READ_IRQ_ENABLE 0    
    /**
     * 0 - No Normal Interrupt generated by TX_READ 
     * 1 - Normal Interrupt generated by TX_READ 
     */
#define    INTERRUPT_MASK_TX_LEVEL_IRQ_ENABLE 1
    /**
     * 0 - No Normal Interrupt generated by TX_BUFFER_LEVEL 
     * 1 - Normal Interrupt generated by TX_BUFFER_LEVEL 
     */
#define    INTERRUPT_MASK_TX_ERROR_IRQ_ENABLE 2
    /**
     * 0 - No Normal Interrupt generated by TX_ERROR 
     * 1 - Normal Interrupt generated by TX_ERROR 
     */
#define    INTERRUPT_MASK_RX_WRITE_IRQ_ENABLE 8
    /**
     * 0 - No Normal Interrupt generated by RX_WRITE 
     * 1 - Normal Interrupt generated by RX_WRITE 
     */
#define    INTERRUPT_MASK_RX_LEVEL_IRQ_ENABLE 9
    /**
     * 0 - No Normal Interrupt generated by RX_BUFFER_LEVEL 
     * 1 - Normal Interrupt generated by RX_BUFFER_LEVEL 
     */
#define    INTERRUPT_MASK_RX_ERROR_IRQ_ENABLE 10
    /**
     * 0 - No Normal Interrupt generated by RX_ERROR 
     * 1 - Normal Interrupt generated by RX_ERROR 
     */
#define    INTERRUPT_MASK_TX_URUN_IRQ_ENABLE 16
    /**
     * 0 - No Error Interrupt generated by Tx Underrun 
     * 1 - Error Interrupt generated by Tx Underrun 
     */
#define    INTERRUPT_MASK_TX_ORUN_IRQ_ENABLE 17
    /**
     * 0 - No Error Interrupt generated by Tx Overrun 
     * 1 - Error Interrupt generated by Tx Overrun 
     */
#define    INTERRUPT_MASK_TX_FIFO_URUN_ERR_IRQ_ENABLE 18
    /**
     * 0 - No Error Interrupt generated by Tx FIFO Underrun 
     * 1 - Error Interrupt generated by Tx FIFO Underrun 
     */
#define    INTERRUPT_MASK_TX_FIFO_ORUN_ERR_IRQ_ENABLE 19
    /**
     * 0 - No Error Interrupt generated by Tx FIFO Overrun 
     * 1 - Error Interrupt generated by Tx FIFO Overrun 
     */
#define    INTERRUPT_MASK_RX_URUN_ERR_IRQ_ENABLE 24
    /**
     * 0 - No Error Interrupt generated by Rx Underrun 
     * 1 - Error Interrupt generated by Rx Underrun 
     */
#define    INTERRUPT_MASK_RX_ORUN_ERR_IRQ_ENABLE 25    
    /**
     * 0 - No Error Interrupt generated by Rx Overrun 
     * 1 - Error Interrupt generated by Rx Overrun 
     */
#define    INTERRUPT_MASK_RX_FIFO_URUN_ERR_IRQ_ENABLE 26    
    /**
     * 0 - No Error Interrupt generated by Rx FIFO Underrun 
     * 1 - Error Interrupt generated by Rx FIFO Underrun 
     */
#define    INTERRUPT_MASK_RX_FIFO_ORUN_ERR_IRQ_ENABLE 27    
    /**
     * 0 - No Error Interrupt generated by Rx FIFO Overrun 
     * 1 - Error Interrupt generated by Rx FIFO Overrun 
     */
#define    INTERRUPT_MASK_SPDIF_RX_ERROR    28
    /**
     * 0 - No Error Interrupt generated by Rx S/PDIF Error (Parity/Preamble/Framing) 
     * 1 - Error Interrupt generated by Rx S/PDIF Error (Parity/Preamble/ Framing) 
     */
#define    INTERRUPT_MASK_SPDIF_RX_LOCK     29
    /**
     * 0 - No Error Interrupt generated by Rx S/PDIF Lock event 
     * 1 - Error Interrupt generated by Rx S/PDIF Lock event 
     */

    
/* VERSION ------------ */ 
/**
 * Returns a 32 bit value that indicates the version and build options of the  
 * audio core being used. See the version list at the end of the document to  
 * find out how they translate. 
 */

#define    VERSION_NUMBER                   0
    /**
     * 0 - 7        Version of the core 
     */
#define    VERSION_AUX_APB                  12         
    /**
     * AUX_APB  build option    
     */
#define    VERSION_RX_SPDIF                 13       
    /**
     * RX_SPDIF  build option  
     */
#define    VERSION_TX_SPDIF                 14      
    /**
     * TX_SPDIF  build option  
     */
#define    VERSION_AHB_DMA                  15        
    /**
     * DMA_AHB  build option 
     */
#define    VERSION_RX_FIFO_ADDR_BITS        19   
    /**
     * 16 - 19    G_RX_FIFO_A_BITS  generic
     */
#define    VERSION_RX_CHANS                 23   
    /**
     * 20 - 23    G_RX_CHANNELS  generic 
     */
#define    VERSION_TX_FIFO_ADDR_BITS        27   
    /**
     * 24 - 27    G_TX_FIFO_A_BITS  generic 
     */
#define    VERSION_TX_CHANS                 31   
    /**
     * 28 - 31    G_TX_CHANNELS  generic 
     */


 
/* TX_DATA_IN_FORMAT ------------ */
#define    TX_DATA_IN_FORMAT_SAMPLE_ORDER   0
    /**
     * 0 - Samples written in as left/right pairs 
     * 1 - All left channels written then all right 
     */
#define    TX_DATA_IN_FORMAT_24_BIT_SAMPLE  1
    /**
     * Set to ‘1’ to indicate write contains 24 bit data (used in conjunction  
     * with following setting) 
     * ** NOT VALID FOR S/PDIF 
     */
#define    TX_DATA_IN_FORMAT_PAD_TOP_BYTE   2
    /**
     * 0 - Output data becomes WRITE_DATA(23 downto 0) & “00000000” 
     * 1 - Output data becomes “0000000” & WRITE_DATA(23 downto 0) 
     */
#define    TX_DATA_IN_FORMAT_WAIT_FOR_HALF  4
    /**
     * 0 - Hardware waits for full number of writes before progressing 
     * 1 - Hardware waits for half the number of writes before progressing 
     */

    
/* TX_CHANNELS_ENABLE ------------ */ 
/**
 * If a channel is disabled, then no new data is presented to the data line of  
 * that I2S channel 
 *   0 - Tx Channel N disabled 
 *   1 - Tx Channel N enabled 
 */


/* TX_WRITES_TO ------------ */ 
/**
 * Can be used in conjunction with the Tx Channel Enable, whereby if an output 
 * channel is disabled then software can select whether or not it still wishes  
 * to write to this channel or not, effectively reduces its number of writes  
 * per interrupt. If it chooses to still write to this location then the write  
 * data is effectively ignored. 
 *   0 - Tx Channel N not included in data 
 *   1 - Tx Channel N included in data 
 */

 
/* RX_DATA_OUT_FORMAT ------------ */
#define    RX_DATA_OUT_FORMAT_SAMPLE_ORDER  0
    /**
     * 0 - Samples read in as left/right pairs 
     * 1 - All left channels read then all right 
     */
#define    RX_DATA_OUT_FORMAT_24_BIT_SAMPLE 1   
    /**
     * Set to ‘1’ to indicate read contains 24 bit data (used in conjunction  
     * with following setting)          
     * ** NOT VALID FOR S/PDIF    
     */
#define    RX_DATA_OUT_FORMAT_PAD_TOP_BYTE  2
    /**
     * 0 - Read data becomes RX_DATA(23 downto 0) & “00000000” 
     * 1 - Read data becomes “0000000” & RX_DATA(23 downto 0) 
     */
#define    RX_DATA_OUT_FORMAT_WAIT_FOR_HALF 4
    /**
     * 0 - Hardware waits for full number of reads before progressing 
     * 1 - Hardware waits for half the number of reads before progressing 
     */

    
/* RX_CHANNELS_ENABLE ------------ */ 
/**
 * If a channel is disabled, then no new data is presented to the data line of 
 * that I2S channel 
 *   0 - Rx Channel N disabled 
 *   1 - Rx Channel N enabled 
 */


/* RX_READS_FROM ------------ */ 
/**
 * Can be used in conjunction with the Rx Channel Enable, whereby if an input 
 * channel is disabled then software can select whether or not it still wishes  
 * to read from this channel or not, effectively reduces its number of reads  
 * per interrupt. If it chooses to still read from this location then the read 
 * data is invalid. 
 *    0 - Rx Channel N not included in data 
 *    1 - Rx Channel N included in data 
 */


 
/* TX_CPU_DATA_WRITES_ALT ------------ */ 
/**
 * See TX_CPU_DATA_WRITES.   
 * This alternative address has been provided as it may be referenced within a  
 * 13 bit immediate address (which may have benefits when being accessed by a  
 * CPU with only a 13 bit immediate offset such as the LEON2 IU). 
 * 0x0400-0x04FF    TX_CPU_DATA_WRITES_ALT 
 */


/* RX_CPU_DATA_READS_ALT ------------ */ 
/**
 * See RX_CPU_DATA_READS.  This alternative address has been provided as it may 
 * be referenced within a 13 bit immediate address (which may have benefits  
 * when being accessed by a CPU with only a 13 bit immediate offset such as the 
 * LEON2 IU). 
 * 0x0800-0x08FF    RX_CPU_DATA_READS_ALT 
 */


/* TX_CPU_DATA_WRITES ------------ */ 
/**
 * Any writes to this location writes a valid sample into the internal buffer  
 * of the audio core.  All writes should be the full 32 bits. 
 * This address is WRITE ONLY - any reads will return the previous read value  
 * on the bus. 
 * 0x1000-0x1FFF    TX_CPU_DATA_WRITES 
 */


/* RX_CPU_DATA_READS ------------ */ 
/**
 * Any reads from this location read a valid sample from the internal buffer of  
 * the audio core.  All reads should be the full 32 bits. 
 * This address is READ ONLY - any writes will be ignored. 
 * 0x2000-0x2FFF    RX_CPU_DATA_READS 
 */

#endif /* __ASM_ARM_ARCH_I2S_H */

