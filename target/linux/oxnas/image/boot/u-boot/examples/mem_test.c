/*
 * (C) Copyright 2006
 * Oxford Semiconductor Ltd, www.oxsemi.com
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

/********************* OPTIONS ********************************************************/

#define ARM
/*
#define QUIET
#define SHORT
*/

/********************* TEST DEFINITIONS ********************************************************/


#define NUM_PATTYPES 5
#define PATTYPE_A5     0
#define PATTYPE_5A     1
#define PATTYPE_NO_FF  2
#define PATTYPE_INCR   3
#define PATTYPE_DECR   4

/* Number of words in a block (to ensure that neither data not ~data is 0xFFxx) */

#ifdef SHORT
    #define BLOCKWORDS (254*4)
    #define BLOCKSIZE  (256*4)
#else
    #define BLOCKWORDS (254*256*4)
    #define BLOCKSIZE  (256*256*4)
#endif

#ifdef ARM
    #include <common.h>
    #include <exports.h>

    #define SDRAM_BASE  0x48000000
    #define SDRAM_TOP   0x49000000
    #define SDRAM_BLOCK 0x10000
//    #define SDRAM_WRITE(ADDR, DATA) printf("Write 0x%08x to 0x%08x\n", (DATA), (ADDR));
//    #define SDRAM_READ(ADDR, VAR) printf("Read from 0x%08x\n", (ADDR));
//    #define SDRAM_WRITE(ADDR, DATA) printf("Write 0x%08x to 0x%08x\n", (DATA), (ADDR)); (*((volatile unsigned int *)(ADDR)) = (DATA))
//    #define SDRAM_READ(ADDR, VAR)  printf("Read from 0x%08x\n", (ADDR)); (*(VAR) = *((volatile unsigned int *)(ADDR)))
    #define SDRAM_WRITE(ADDR, DATA) (*((volatile unsigned int *)(ADDR)) = (DATA))
    #define SDRAM_READ(ADDR, VAR)  (*(VAR) = *((volatile unsigned int *)(ADDR)))
#else
    #include <stdio.h>
    #include <stdlib.h>
    /* Not so much space - just 2 blocks from addr 0... */
    #define SDRAM_BASE  0
    #define SDRAM_TOP   (2 * BLOCKSIZE)
    #define SDRAM_BLOCK 0x10000
    #ifdef QUIET
        #define SDRAM_WRITE(ADDR, DATA)      array[ADDR] = (DATA)
        #define SDRAM_READ(ADDR, VAR)        *((volatile unsigned int *)(VAR)) = array[ADDR]
    #else
        #define SDRAM_WRITE(ADDR, DATA)      printf("WRITE(%08x)=%08x\n", ADDR, DATA); array[ADDR] = (DATA)
        #define SDRAM_READ(ADDR, VAR)        printf("READ (%08x)=%08x\n", ADDR, array[ADDR]); *((volatile unsigned int *)(VAR)) = array[ADDR]
    #endif
    unsigned volatile int array[SDRAM_TOP];
#endif


#define SYSCTRL_PRIMSEL_LO      0x4500000C
#define SYSCTRL_SECSEL_LO       0x45000014
#define SYSCTRL_TERSEL_LO       0x4500008C
#define SYSCTRL_PRIMSEL_HI      0x45000010
#define SYSCTRL_SECSEL_HI       0x45000018
#define SYSCTRL_TERSEL_HI       0x45000090

/* GPIO */
#define GPIOB_IO_VAL       0x44100000
#define GPIOB_OE_VAL       0x44100004
#define GPIOB_SET_OE       0x4410001C
#define GPIOB_CLEAR_OE     0x44100020
#define GPIOB_OUTPUT_VAL   0x44100010
#define GPIOB_SET_OUTPUT   0x44100014
#define GPIOB_CLEAR_OUTPUT 0x44100018
#define GPIOB_BIT_34       0x00000004

void configure_caches(void);
void report_err(unsigned int address, unsigned int bad_data, unsigned int correct_data, unsigned int iteration);

/********************* TYPES.H ********************************************************/

typedef unsigned int    UINT,  *PUINT;
/*
#ifndef __MY_BASIC_TYPES_H
#define __MY_BASIC_TYPES_H

typedef signed char     CHAR,  *PCHAR;
typedef unsigned char   BYTE,   UCHAR,  *PBYTE, *PUCHAR;
typedef signed short    SHORT, *PSHORT;
typedef unsigned short  WORD,   USHORT, *PWORD, *PUSHORT;
typedef signed long     LONG,  *PLONG;
typedef unsigned long   DWORD, *PDWORD;
typedef int             BOOL,  *PBOOL;
typedef unsigned int    UINT,  *PUINT;
typedef void            VOID,  *PVOID;

typedef float           SINGLE,*PSINGLE;
typedef double          DOUBLE,*PDOUBLE;


#define FALSE 0
#define TRUE  1

#endif
*/

/********************* CHIP.H ********************************************************/

// Address Map
#define BOOT_ROM_BASE       0x00000000
#define USBHS_BASE          0x00200000
#define GMAC_BASE           0x00400000
#define PCI_BASE            0x00600000
#define PCI_DATA_BASE       0x00800000
#define STATIC0_BASE        0x01000000
#define STATIC1_BASE        0x01400000
#define STATIC2_BASE        0x01800000
#define STATIC_BASE         0x01C00000
#define SATA_DATA_BASE      0x02000000
#define DPE_DATA_BASE       0x03000000
#define GPIOA_BASE          0x04000000
#define GPIOB_BASE          0x04100000
#define UARTA_BASE          0x04200000
#define UARTB_BASE          0x04300000
#define I2C_MASTER_BASE     0x04400000
#define AUDIO_BASE          0x04500000
#define FAN_BASE            0x04600000
#define PWM_BASE            0x04700000
#define IR_RX_BASE          0x04800000
#define UARTC_BASE          0x04900000
#define UARTD_BASE          0x04A00000
#define SYS_CTRL_BASE       0x05000000
#define RPSA_BASE           0x05300000
#define  ARM_RPS_BASE  RPSA_BASE 
#define RPSC_BASE           0x05400000
#define AHB_MON_BASE        0x05500000
#define DMA_BASE            0x05600000
#define DPE_BASE            0x05700000
#define IBIW_BASE           0x05780000
#define DDR_BASE            0x05800000
#define SATA0_BASE          0x05900000
#define SATA1_BASE          0x05980000
#define DMA_CHKSUM_BASE     0x05A00000
#define COPRO_BASE          0x05B00000
#define SGDMA_BASE          0x05C00000
#define DDR_DATA_BASE       0x08000000
#define SRAM_BASE           0x0C000000
#define SRAM0_BASE          0x0C000000
#define SRAM1_BASE          0x0C002000
#define SRAM2_BASE          0x0C004000
#define SRAM3_BASE          0x0C006000

// Virtual peripheral for TB sync
#define TB_SYNC_BASE        0x05F00100


/********************* DMA.H ********************************************************/


// DMA Control register settings

#define DMA_FAIR_SHARE                  (1<<0)
#define DMA_IN_PROGRESS                 (1<<1)

#define DMA_SDREQ_SATA                  (0<<2)
#define DMA_SDREQ_DPE_OUT               (2<<2)
#define DMA_SDREQ_UARTA_RX              (4<<2)
#define DMA_SDREQ_AUDIO_RX              (6<<2)
#define DMA_SDREQ_MEM                   (0xF<<2)

#define DMA_DDREQ_SATA                  (0<<6)
#define DMA_DDREQ_DPE_IN                (1<<6)
#define DMA_DDREQ_UARTA_TX              (3<<6)
#define DMA_DDREQ_AUDIO_TX              (5<<6)
#define DMA_DDREQ_MEM                   (0xF<<6)

#define DMA_INTERRUPT                   (1 << 10)
#define DMA_NEXT_FREE                   (1 << 11)
#define DMA_CH_RESET                    (1 << 12)

#define DMA_DIR_ATOA                    (0 << 13)
#define DMA_DIR_BTOA                    (1 << 13)
#define DMA_DIR_ATOB                    (2 << 13)
#define DMA_DIR_BTOB                    (3 << 13)

#define DMA_BURST_A                     (1 << 17)
#define DMA_BURST_B                     (1 << 18)

#define DMA_SWIDTH_8                    (0 << 19)
#define DMA_SWIDTH_16                   (1 << 19)
#define DMA_SWIDTH_32                   (2 << 19)

#define DMA_DWIDTH_8                    (0 << 22)
#define DMA_DWIDTH_16                   (1 << 22)
#define DMA_DWIDTH_32                   (2 << 22)

#define DMA_PAUSE                       (1 << 25)
#define DMA_INT_ENABLE                  (1 << 26)
#define DMA_STARVE_LO_PRIORITY          (1 << 29)
#define DMA_NEW_INT_CLEAR               (1 << 30)

#define DMA_FIXED_SADDR                 ((0 << 15) | (1 << 27))
#define DMA_INCR_SADDR                  ((1 << 15) | (0 << 27))
#define DMA_SEMI_FIXED_SADDR            ((0 << 15) | (0 << 27))

#define DMA_FIXED_DADDR                 ((0 << 16) | (1 << 28))
#define DMA_INCR_DADDR                  ((1 << 16) | (0 << 28))
#define DMA_SEMI_FIXED_DADDR            ((0 << 16) | (0 << 28))

#define DMA_BASE_CTRL                   (DMA_BURST_A | DMA_BURST_B | DMA_INT_ENABLE | DMA_NEW_INT_CLEAR)

// Common base setups

#define DMA_CTRL_A32TOA32               ( DMA_BASE_CTRL | DMA_DIR_ATOA | DMA_SWIDTH_32 | DMA_DWIDTH_32 )
#define DMA_CTRL_B32TOA32               ( DMA_BASE_CTRL | DMA_DIR_BTOA | DMA_SWIDTH_32 | DMA_DWIDTH_32 )
#define DMA_CTRL_A32TOB32               ( DMA_BASE_CTRL | DMA_DIR_ATOB | DMA_SWIDTH_32 | DMA_DWIDTH_32 )
#define DMA_CTRL_B32TOB32               ( DMA_BASE_CTRL | DMA_DIR_BTOB | DMA_SWIDTH_32 | DMA_DWIDTH_32 )

#define DMA_CTRL_A8TOB32                ( DMA_BASE_CTRL | DMA_DIR_ATOB | DMA_SWIDTH_8  | DMA_DWIDTH_32 )
#define DMA_CTRL_B32TOA8                ( DMA_BASE_CTRL | DMA_DIR_BTOA | DMA_SWIDTH_32 | DMA_DWIDTH_8  )
#define DMA_CTRL_A32TOB8                ( DMA_BASE_CTRL | DMA_DIR_ATOB | DMA_SWIDTH_32 | DMA_DWIDTH_8  )

// Most likely transactions

#define DMA_CTRL_MEM_TO_MEM_AA          ( DMA_CTRL_A32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_MEM_AB          ( DMA_CTRL_A32TOB32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_MEM_BB          ( DMA_CTRL_B32TOB32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_MEM_BA          ( DMA_CTRL_B32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_MEM             ( DMA_CTRL_MEM_TO_MEM_AB ) 

//DMA A-A
#define DMA_CTRL_SATA_TO_MEM_AA         ( DMA_CTRL_A32TOA32 | DMA_SDREQ_SATA     | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_SATA_AA         ( DMA_CTRL_A32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_SATA     | DMA_INCR_SADDR       | DMA_INCR_DADDR )

#define DMA_CTRL_SATA_TO_MEM            ( DMA_CTRL_A32TOB32 | DMA_SDREQ_SATA     | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_SATA            ( DMA_CTRL_B32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_SATA     | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_SATA_TO_DPE            ( DMA_CTRL_A32TOA32 | DMA_SDREQ_SATA     | DMA_DDREQ_DPE_IN   | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_DPE_TO_SATA            ( DMA_CTRL_A32TOA32 | DMA_SDREQ_DPE_OUT  | DMA_DDREQ_SATA     | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_DPE             ( DMA_CTRL_B32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_DPE_IN   | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_DPE_TO_MEM             ( DMA_CTRL_A32TOB32 | DMA_SDREQ_DPE_OUT  | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_PCI_TO_MEM             ( DMA_CTRL_A32TOB32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )

#define DMA_CTRL_MEM_TO_PCI             ( DMA_CTRL_B32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_MEM      | DMA_INCR_SADDR       | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_AUDIO           ( DMA_CTRL_B32TOA32 | DMA_SDREQ_MEM      | DMA_DDREQ_AUDIO_TX | DMA_INCR_SADDR       | DMA_FIXED_DADDR )
#define DMA_CTRL_AUDIO_TO_MEM           ( DMA_CTRL_A32TOB32 | DMA_SDREQ_AUDIO_RX | DMA_DDREQ_MEM      | DMA_FIXED_SADDR      | DMA_INCR_DADDR )
#define DMA_CTRL_MEM_TO_UART            ( DMA_CTRL_B32TOA8  | DMA_SDREQ_MEM      | DMA_DDREQ_UARTA_TX | DMA_INCR_SADDR       | DMA_FIXED_DADDR )
#define DMA_CTRL_UART_TO_MEM            ( DMA_CTRL_A8TOB32  | DMA_SDREQ_UARTA_RX | DMA_DDREQ_MEM      | DMA_FIXED_SADDR      | DMA_INCR_DADDR )

// Byte count register flags

#define DMA_HBURST_EN                   (1<<28)
#define DMA_WR_BUFFERABLE               (1<<29)
#define DMA_WR_EOT                      (1<<30)
#define DMA_RD_EOT                      (1<<31)


// Pause the DMA channel specified
void PauseDMA( UINT channel );

// UnPause the DMA channel specified
void UnPauseDMA( UINT channel );

// Configure a DMA
void SetupDMA( UINT channel,
               UINT src_addr,
               UINT dest_addr,
               UINT byte_count,
               UINT control,
               UINT flags );

// Wait while the given DMA channel is busy
void WaitWhileDMABusy( UINT channel );

// Perform a memory to memory copy
void DMAMemCopy ( UINT channel,
                  UINT src_addr,
                  UINT dest_addr,
                  UINT byte_count );


/****************************** MAIN ***********************************************/
#ifdef ARM
int mem_test(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    unsigned int i;
    unsigned int iteration;
    unsigned int block_base;
    unsigned int datapattern;
    unsigned int correct_data;
    unsigned volatile int read_data;
    unsigned int pattype, starting_pattype;
    unsigned int end_addr;
    unsigned int row, col, bank;

#ifdef ARM
    /* Print the ABI version */
    app_startup(argv);
    printf ("Example expects ABI version %d\n", XF_VERSION);
    printf ("Actual U-Boot ABI version %d\n", (int)get_version());

    printf("GPIO34 is output, low\n");
    * (volatile unsigned int *) GPIOB_CLEAR_OUTPUT = GPIOB_BIT_34;
    * (volatile unsigned int *) GPIOB_SET_OE       = GPIOB_BIT_34;
#endif

//    configure_caches();
//printf("Caches enabled\n");

    /* ******************************************************************* */
    printf("DMA TEST.\n" );
    /* ******************************************************************* */


    #define DMA0_CTRL_STAT    0x45A00000
    #define DMA0_SRC_BASE     0x45A00004
    #define DMA0_DEST_BASE    0x45A00008
    #define DMA0_BYTE_COUNT   0x45A0000C
    #define DMA0_CURRENT_BYTE 0x45A00018

    printf("Test to top of 1st SDRAM" );
    #define BLOCK_BYTES 0x20000
    #define SDRAM_STOP SDRAM_TOP

    for (iteration=0; 1; iteration++) {

        if ((iteration % 5)==0)
            printf("Iteration %d\n", iteration );

//        printf("Write pattern into first block.\n" );
        end_addr = SDRAM_BASE + BLOCK_BYTES;
        for (i=SDRAM_BASE; i < end_addr; i=i+4) {
            SDRAM_WRITE( i, i);
        }

//        printf("Clear last block and a few blocks more - easy to see on LA.\n" );
        end_addr = SDRAM_BASE + (BLOCK_BYTES << 3);
        for (i=SDRAM_STOP - BLOCK_BYTES; i < end_addr; i=i+4) {
            SDRAM_WRITE( i, 0);
        }

        end_addr = SDRAM_STOP - BLOCK_BYTES;
        for (i=SDRAM_BASE; i < end_addr; i=i+BLOCK_BYTES) {

//            printf("DMA transfer from %08x to %08x.\n", i, i + BLOCK_BYTES );
#ifdef ARM
            DMAMemCopy ( 0, i, i + BLOCK_BYTES, BLOCK_BYTES );
#endif
//            printf("...pending.\n" );
#ifdef ARM
            WaitWhileDMABusy( 0 );
#endif
//            printf("...complete.\n" );
        }

//        printf("Verify pattern in last block.\n" );
        end_addr = SDRAM_STOP;
        correct_data = SDRAM_BASE;
        for (i=SDRAM_STOP - BLOCK_BYTES; i < end_addr; i=i+4) {
            SDRAM_READ( i, &read_data);
            if (read_data != correct_data)
            {
            /* Expand out the report_err function to avoid the stack operations. */
            #ifdef ARM
                /* ASSERT GPIO */
                * (volatile unsigned int *) GPIOB_SET_OUTPUT = GPIOB_BIT_34;
            #endif

                /* REPORT ERROR */
                printf("Wrong on [%08x]= %08x should be %08x on iteration %d\n", i, read_data, correct_data, iteration );

                /* WRITE TO ANOTHER LOCATION */
                SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

                /* READ AGAIN */
                SDRAM_READ(i, &read_data);
                if (read_data != correct_data)
                    printf("Again 1  [%08x]= %08x should be %08x\n", i, read_data, correct_data );

                /* WRITE TO ANOTHER LOCATION */
                SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

                /* READ AGAIN */
                SDRAM_READ(i, &read_data);
                if (read_data != correct_data)
                    printf("Again 2  [%08x]= %08x should be %08x\n", i, read_data, correct_data );

                /* WRITE TO ANOTHER LOCATION */
                SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

                /* READ AGAIN */
                SDRAM_READ(i, &read_data);
                if (read_data != correct_data)
                    printf("Again 3  [%08x]= %08x should be %08x\n", i, read_data, correct_data );

                row = (((i >> 26) & 0x1) << 13) | (((i >> 23) & 0x3) << 11) | ((i >> 10) & 0x7FF); /* [26], [24:23], [20:10]*/
                col = (((i >> 27) & 0x1) << 10) | (((i >> 25) & 0x1) << 9) | (((i >> 22) & 0x1) << 8); /* [27], [25], [22]... */
                col |= (((i >> 6) & 0xF) << 4) | (((i >> 21) & 0x1) << 3) | (((i >> 1) & 0x3) << 1); /* ...[9:8], [21], [3:2], '0' */
                col |= 0x800; /* bit 11 set for auto-precharge */
                bank = (i >> 4) & 0x3; /* [5:4] */
                printf("Bank   %08x\n", bank );
                printf("Row    %08x\n", row );
                printf("Column %08x\n", col );
            #ifdef ARM
                /* DEASSERT GPIO */
                * (volatile unsigned int *) GPIOB_CLEAR_OUTPUT = GPIOB_BIT_34;
            #endif
            }


            correct_data += 4;
        }
    }


    /* ******************************************************************* */
    printf("MEM_TEST2\n");
    /* ******************************************************************* */


    pattype=0;
    iteration=0;

    for (;;) { 
        /* FOR EACH 64Kword==256KB BLOCK IN 16Mword=64MB (2 OFF 16M16) MEMORY... */

#ifdef SHORT
        if ((iteration % 5)==0)
            printf("Iteration %d\n", iteration );
#else
        if ((iteration % 1000)==0)
            printf("Iteration %d\n", iteration );
#endif

        /* WRITE DATA BLOCKS */
        starting_pattype = pattype; /* Record for later */

        for (block_base=SDRAM_BASE; block_base < SDRAM_TOP; block_base=block_base + BLOCKSIZE) {
	    switch (pattype) {
	    case PATTYPE_A5 :
                /* Write alternating 1s and 0s... */
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_WRITE( i, 0xaa55aa55);
                }
                break;
	    case PATTYPE_5A :
                /* Write alternating 1s and 0s (inverse of above)... */
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_WRITE( i, 0x55aa55aa);
                }
                break;
	    case PATTYPE_NO_FF : 
                /* Write data=address with bit[n+16]=~bit[n]... */
                datapattern = 0x0100FEFF;
                /* In range 0x0100...0xFEFF so that
                    a. temp[15:8] is never 0xFF
                    b. Inverse of temp[15:8] is never 0xFF
                */
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_WRITE( i, datapattern);
                    datapattern = datapattern + 0xFFFF;
                }
                break;
	    case PATTYPE_INCR : 
                /* Write data=address... */
                end_addr = block_base + BLOCKSIZE;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_WRITE( i, i);
                }
                break;
	    case PATTYPE_DECR : 
                /* Write data=~address... */
                end_addr = block_base + BLOCKSIZE;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_WRITE( i, ~i);
                }
                break;
            }
        }

        /* VERIFY DATA BLOCKS */
        pattype = starting_pattype; /* Reset to same as for writes */

        for (block_base=SDRAM_BASE; block_base < SDRAM_TOP; block_base=block_base + BLOCKSIZE) {
	    switch (pattype) {
	    case PATTYPE_A5 :
                correct_data = 0xaa55aa55;
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_READ( i, &read_data);
                    if (read_data != correct_data)
                        report_err(i, read_data, correct_data, iteration);
                }
                break;
	    case PATTYPE_5A :
                correct_data = 0x55aa55aa;
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_READ( i, &read_data);
                    if (read_data != correct_data)
                        report_err(i, read_data, correct_data, iteration);
                }
                break;
	    case PATTYPE_NO_FF :
                correct_data = 0x0100FEFF;
                end_addr = block_base + BLOCKWORDS;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_READ( i, &read_data);
                    if (read_data != correct_data)
                        report_err(i, read_data, correct_data, iteration);
                    correct_data = correct_data + 0xFFFF;
                }
                break;
	    case PATTYPE_INCR :
                end_addr = block_base + BLOCKSIZE;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_READ( i, &read_data);
                    if (read_data != i)
                        report_err(i, read_data, i, iteration);
                }
                break;
	    case PATTYPE_DECR :
                end_addr = block_base + BLOCKSIZE;
                for (i=block_base; i < end_addr; i=i+4) {
                    SDRAM_READ( i, &read_data);
                    if (read_data != ~i)
                        report_err(i, read_data, ~i, iteration);
                }
                break;
            }
        }

	pattype = pattype + 1;
	if (pattype >= NUM_PATTYPES) { pattype = 0; }
        ++iteration;
    }

    return 0;
}

/********************* REPORT ERROR FUNC ********************************************************/

void report_err(unsigned int address, unsigned int bad_data, unsigned int correct_data, unsigned int iteration)
{
    volatile unsigned int readvalue;

#ifdef ARM
    /* ASSERT GPIO */
    * (volatile unsigned int *) GPIOB_SET_OUTPUT = GPIOB_BIT_34;
#endif

    /* REPORT ERROR */
    printf("Wrong on [%08x]= %08x should be %08x on iteration %d\n", address, bad_data, correct_data, iteration );

    /* WRITE TO ANOTHER LOCATION */
    SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

    /* READ AGAIN */
    SDRAM_READ(address, &readvalue);
    if (readvalue != correct_data)
        printf("Again 1  [%08x]= %08x should be %08x\n", address, readvalue, correct_data );

    /* WRITE TO ANOTHER LOCATION */
    SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

    /* READ AGAIN */
    SDRAM_READ(address, &readvalue);
    if (readvalue != correct_data)
        printf("Again 2  [%08x]= %08x should be %08x\n", address, readvalue, correct_data );

    /* WRITE TO ANOTHER LOCATION */
    SDRAM_WRITE(SDRAM_BASE, 0xFFFFFFFF);

    /* READ AGAIN */
    SDRAM_READ(address, &readvalue);
    if (readvalue != correct_data)
        printf("Again 3  [%08x]= %08x should be %08x\n", address, readvalue, correct_data );

#ifdef ARM
    /* DEASSERT GPIO */
    * (volatile unsigned int *) GPIOB_CLEAR_OUTPUT = GPIOB_BIT_34;
#endif

} /* end of report_err */




/********************* DMA.C FUNCTIONS ********************************************************/

void ResetDMA( UINT channel ) {
    
    // Clear and abort the dma channel
    
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    dma[0] =  (1 << 12);
    dma[0] &=  ~(1 << 12);
}




void PauseDMA( UINT channel ) {
    
    // Pause the DMA channel specified
    
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    UINT  rd;
    
    rd = dma[0];
    
    rd |= DMA_PAUSE;
    
    dma[0] = rd;
}




void UnPauseDMA( UINT channel ) {
    
    // UnPause the DMA channel specified
    
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    UINT  rd;
    
    rd = dma[0];
    
    rd &= ~DMA_PAUSE;
    
    dma[0] = rd;
}




void SetupDMA( UINT channel,
               UINT src_addr,
               UINT dest_addr,
               UINT byte_count,
               UINT control,
               UINT flags ) {

    // Configure a DMA
    
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    
    dma[0] = control;
    dma[1] = src_addr;
    dma[2] = dest_addr;
    dma[3] = byte_count | (flags & 0xF0000000);
}

// EXAMPLE:
//
// DMA 2kB from SRAM to SATA core with a write EOT set, and HBURST enabled, using DMA channel 2
// Then wait for the DMA to complete
//
//   SetupDMA ( 2 , 0x4C001100, BASE_SATA, 2048, DMA_CTRL_MEM_TO_SATA, WR_EOT | DMA_HBURST_EN );
//   WaitWhileDMABusy( 2 );

int DMABusy(UINT channel)
{
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    return (dma[0] & DMA_IN_PROGRESS ? 1 : 0);
}


void WaitWhileDMABusy( UINT channel ) // Wait while the given DMA channel is busy
{
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    while (dma[0] & DMA_IN_PROGRESS) ; // Do Nothing
}

void DMAClearIRQ(UINT channel) 
{
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    dma[4] = 1; // write any value to offset 0x10 (16 / 4 => 4) 

}

void DMAMemCopy ( UINT channel,
                  UINT src_addr,
                  UINT dest_addr,
                  UINT byte_count ) {
    
    // Perform a memory to memory copy
    
    volatile PUINT dma = (PUINT) (DMA_BASE + (channel << 5));
    
    // Choose fastest configuration possible for required transfer
    
    if (src_addr < SRAM_BASE) {
        if (dest_addr < SRAM_BASE) {
            dma[0] = DMA_CTRL_MEM_TO_MEM_AA; // Src and Dest must use A
        } else {
            dma[0] = DMA_CTRL_MEM_TO_MEM_AB; // Src must use A
        }
    } else {
        if (dest_addr < SRAM_BASE) {
            dma[0] = DMA_CTRL_MEM_TO_MEM_BA; // Dest must use A
        } else {
            dma[0] = DMA_CTRL_MEM_TO_MEM_AB; // No restriction
        }
    }
    
    dma[1] = src_addr;
    dma[2] = dest_addr;
    dma[3] = byte_count | DMA_WR_BUFFERABLE | DMA_HBURST_EN;
    
    WaitWhileDMABusy( channel );
}
 






#define CP15R1_M_ENABLE 0x0001 // MMU Enable
#define CP15R1_A_ENABLE 0x0002 // Address alignment fault enable
#define CP15R1_C_ENABLE 0x0004 // (data) cache enable
#define CP15R1_W_ENABLE 0x0008 // write buffer enable
#define CP15R1_PROG32   0x0010 // PROG32
#define CP15R1_DATA32   0x0020 // DATA32
#define CP15R1_L_ENABLE 0x0040 // Late abort on earlier CPUs
#define CP15R1_BIGEND   0x0080 // Big-endian (=1), little-endian (=0)
#define CP15R1_SYSTEM   0x0100 // System bit, modifies MMU protections
#define CP15R1_ROM      0x0200 // ROM bit, modifies MMU protections
#define CP15R1_F        0x0400 // Should Be Zero
#define CP15R1_Z_ENABLE 0x0800 // Branch prediction enable on 810
#define CP15R1_I_ENABLE 0x1000 // Instruction cache enable
#define CP15R1_RESERVED 0x00000078
#define CP15R2_RESERVED 0xFFFFC000

#define NUM_DOMAINS 16

#define DAV_NO_ACCESS 0
#define DAV_CLIENT    1
#define DAV_RESERVED  2
#define DAV_MANAGER   3
#define NUM_DOMAIN_ACCESS_VALUES 4

#define AP_LEVEL_0 0
#define AP_LEVEL_1 0
#define AP_LEVEL_2 0
#define AP_LEVEL_3 0
#define NUM_ACCESS_PERMISSIONS 4

#define FAULT_ID 0

#define FLD_COURSE_ID  1
#define FLD_SECTION_ID 2
#define FLD_FINE_ID    3

#define FD_USER_DATA_BIT 2

#define FD_USER_DATA_NUM_BITS 30

#define SD_BUFFERABLE_BIT 2
#define SD_CACHEABLE_BIT  3
#define SD_IMP_BIT        4
#define SD_DOMAIN_BIT     5
#define SD_AP_BIT         10
#define SD_ADDRESS_BIT    20

#define SD_DOMAIN_NUM_BITS  4
#define SD_AP_NUM_BITS      2

void CoPro15Regs_SetCP15Reg1(const unsigned long mask) {
    asm volatile(
        "MOV  r0, %0;"
        "MRC  p15, 0, r1, c1, c0, 0;"
        "ORR  r1,r1,r0;"
        "MCR  p15, 0, r1, c1, c0, 0;"
        :
        : "r" (mask | CP15R1_RESERVED)
        : "r0","r1");
}

void CoPro15Regs_ClearCP15Reg1(const unsigned long mask) {
    asm volatile(
        "MOV  r0, %0;"
        "MRC  p15, 0, r1, c1, c0, 0;"
        "BIC  r1,r1,r0;"
        "MCR  p15, 0, r1, c1, c0, 0;"
        :
        : "r" (mask)
        : "r0","r1");
}

unsigned long CoPro15Regs_GetCP15Reg1(const unsigned long mask) {
    unsigned long value;
    asm volatile(
        "MRC  p15, 0, r1, c1, c0, 0;"
        "MOV  r0, %1;"
        "BIC  %0,r1,r0; "
        : "=r" (value)
        : "r" (mask)
        : "r0","r1");
    return value;
}

unsigned long CoPro15Regs_GetCP15Reg2(void) {
    unsigned long value;
    asm volatile(
        "MRC  p15, 0, r0, c2, c0, 0;"
        "MOV  %0, r0;"
        : "=r" (value)
        :
        : "r0");
    return value & CP15R2_RESERVED;
}

unsigned long CoPro15Regs_GetCP15Reg3(void) {
    unsigned long value;
    asm volatile(
        "MRC  p15, 0, r0, c3, c0, 0;"
        "MOV  %0, r0;"
        : "=r" (value)
        :
        : "r0");
    return value;
}

void CoPro15Regs_SetCP15Reg3(unsigned long value) {
    asm volatile(
        "MOV  r0, %0;"
        "MCR  p15, 0, r0, c3, c0, 0;"
        :
        : "r" (value)
        : "r0");
}

void CoPro15Regs_SetCP15Reg2(unsigned long value) {
    asm volatile(
        "MOV  r0, %0;"
        "MCR  p15, 0, r0, c2, c0, 0;"
        :
        : "r" (value & CP15R2_RESERVED)
        : "r0");
}

void Cache_CleanDataCache(void)
{
    // Clean the data cache - usually precedes a data cache invalidation.
    // Forces the data cache content to be written to main memory - only
    // required if using write-back data cache
    asm volatile(
        "MOV  r3,pc;"
        "LDR  r1, =0;"
        "  MOV  r4,pc;"
        "  LDR  r0, =0;"
        "    ORR  r2, r1, r0;"
        "    MCR  p15, 0, r2, c7, c10, 2 ;"     // I (BHC) think that this should be c10, 2 not c14, 1 -- See ARM ARM
        "    ADD  r0, r0, #0x10;"
        "    CMP  r0,#0x40;"
        "  BXNE  r4;"
        "  ADD  r1, r1, #0x04000000;"
        "  CMP  r1, #0x0;"
        "BXNE  r3;"
        :
        :
        : "r0","r1","r2","r3","r4");
}

void Cache_DrainWriteBuffer(void)
{
    // Forces the write buffer to update to main memory
    asm volatile(
        "LDR  r1, =0;"
        "MCR  p15, 0, r1, c7, c10, 4 ;"
        :
        :
        : "r1");
}

void Cache_FlushPrefetchBuffer(void)
{
    // Forces the CPU to flush the instruction prefetch buffer
    asm volatile(
        "LDR  r1, =0;"
        "MCR  p15, 0, r1, c7, c5, 4 ;"
        :
        :
        : "r1");
}

void Cache_InvalidateDataCache(void)
{
    asm volatile(
        "LDR  r1, =0;"
        "MCR  p15, 0, r1, c7, c6, 0;"
        :
        :
        : "r1");
}

void Cache_InvalidateInstructionCache(void)
{
    asm volatile(
        "LDR  r1, =0;"
        "MCR  p15, 0, r1, c7, c5, 0;"
        :
        :
        : "r1");
}

void Cache_InstOn(void)
{
    // Invalidate the instruction cache, in case there's anything
    // left from when it was last enabled
    Cache_InvalidateInstructionCache();

    // Enable the instruction cache
    CoPro15Regs_SetCP15Reg1(CP15R1_I_ENABLE);
}

void Cache_InstOff(void)
{
    // Disable the instruction cache
    CoPro15Regs_ClearCP15Reg1(CP15R1_I_ENABLE);
}

void Cache_DataOn(void)
{
    // Invalidate the data cache, in case there's anything left from when
    // it was last enabled
    Cache_InvalidateDataCache();

    // Enable the data cache
    CoPro15Regs_SetCP15Reg1(CP15R1_C_ENABLE);
}

void Cache_DataOff(void)
{
    // Ensure all data in data cache or write buffer is written to memory
    Cache_CleanDataCache();
    Cache_DrainWriteBuffer();

    // Disable the data cache
    CoPro15Regs_ClearCP15Reg1(CP15R1_C_ENABLE);
}

void Cache_WriteBufferOn(void)
{
    // Enable the write buffer
    CoPro15Regs_SetCP15Reg1(CP15R1_W_ENABLE);
}

void Cache_WriteBufferOff(void)
{
    // Ensure all data in the write buffer is written to memory
    Cache_DrainWriteBuffer();

    // Disable the write buffer
    CoPro15Regs_ClearCP15Reg1(CP15R1_W_ENABLE);
}

int MMU_SetDomainAccessValue(
    int domainNumber,
    int value) {
    int status = 0;
    if ((value < NUM_DOMAIN_ACCESS_VALUES) && (domainNumber < NUM_DOMAINS))
    {
        // Insert the 2-bit domain field into the slot for the specified domain
        unsigned long registerContents = CoPro15Regs_GetCP15Reg3();
        registerContents &= ~(3UL << (2*domainNumber));
        registerContents |= ((unsigned long)value << (2*domainNumber));
        CoPro15Regs_SetCP15Reg3(registerContents);
        status = 1;
    }
    return status;
}

void MMU_SetAlignmentChecked(int alignmentChecked) {
    alignmentChecked ? CoPro15Regs_SetCP15Reg1(CP15R1_A_ENABLE) : CoPro15Regs_ClearCP15Reg1(CP15R1_A_ENABLE);
}

void MMU_SetEnabled(int enabled) {
    enabled ? CoPro15Regs_SetCP15Reg1(CP15R1_M_ENABLE) : CoPro15Regs_ClearCP15Reg1(CP15R1_M_ENABLE);
}
void MMU_InvalidateDataTLB(void)
{
    asm volatile(
        "MCR  p15, 0, r0, c8, c6, 0;"
        :
        :
        : "r0");
}

void MMU_InvalidateInstructionTLB(void)
{
    asm volatile(
        "MCR  p15, 0, r0, c8, c5, 0;"
        :
        :
        : "r0");
}

void MMU_SetROMPermission(int rOM_Permitted) {
    rOM_Permitted ? CoPro15Regs_SetCP15Reg1(CP15R1_ROM) : CoPro15Regs_ClearCP15Reg1(CP15R1_ROM);
}

void MMU_SetSystemPermission(int systemPermitted) {
    systemPermitted ? CoPro15Regs_SetCP15Reg1(CP15R1_SYSTEM) : CoPro15Regs_ClearCP15Reg1(CP15R1_SYSTEM);
}

void MMU_SetTranslationTableBaseAddress(unsigned long *baseAddress) {
    CoPro15Regs_SetCP15Reg2((unsigned long)baseAddress);
}

unsigned long SetBit(
    unsigned long source,
    int           state,
    int           offset)
{
    source = state ? (source | (1UL << offset)) :
                     (source & ~(1UL << offset));
    return source;
}

unsigned long SetField(
    unsigned long source,
    unsigned long newFieldContents,
    int           offset,
    int           length)
{
    unsigned long mask = (1UL << length) - 1;
    source &= ~(mask << offset);
    source |= ((newFieldContents & mask) << offset);
    return source;
}

unsigned long FD_SetUserData(
    unsigned long userData,
    unsigned long descriptor)
{
    return SetField(descriptor, userData, FD_USER_DATA_BIT, FD_USER_DATA_NUM_BITS);
}

unsigned long FLPT_CreateFaultDescriptor(unsigned long userData)
{
    unsigned long descriptor = FAULT_ID;
    descriptor = FD_SetUserData(userData, descriptor);
    return descriptor;
}

void FLPT_InsertFaultDescriptor(
    unsigned long *tableBaseAdr,
    int            index,
    unsigned long  descriptor)
{
    *(tableBaseAdr + index) = descriptor;
}

unsigned long SD_SetAccessPermission(
    int           ap,
    unsigned long descriptor)
{
    return SetField(descriptor, ap, SD_AP_BIT, SD_AP_NUM_BITS);
}

unsigned long SD_SetBaseAddress(
    unsigned long baseAddress,
    unsigned long descriptor)
{
    unsigned long mask = ~0UL << SD_ADDRESS_BIT;
    baseAddress &= mask;
    descriptor &= ~mask;
    descriptor |= baseAddress;
    return descriptor;
}

unsigned long SD_SetBufferable(
    int           bufferable,
    unsigned long descriptor)
{
    return SetBit(descriptor, bufferable, SD_BUFFERABLE_BIT);
}

unsigned long SD_SetCacheable(
    int           cacheable,
    unsigned long descriptor)
{
    return SetBit(descriptor, cacheable, SD_CACHEABLE_BIT);
}

unsigned long SD_SetDomain(
    int           domain,
    unsigned long descriptor)
{
    return SetField(descriptor, domain, SD_DOMAIN_BIT, SD_DOMAIN_NUM_BITS);
}

unsigned long SD_SetImplementationDefined(
    unsigned long implementationDefined,
    unsigned long descriptor)
{
    return SetBit(descriptor, implementationDefined, SD_IMP_BIT);
}

unsigned long FLPT_CreateSectionDescriptor(
    unsigned long baseAddress,
    unsigned char domain,
    int           implementationDefined,
    int           ap,
    int           bufferable,
    int           cacheable)
{
    unsigned long descriptor = FLD_SECTION_ID;
    descriptor = SD_SetAccessPermission(ap, descriptor);
    descriptor = SD_SetBaseAddress(baseAddress, descriptor);
    descriptor = SD_SetBufferable(bufferable, descriptor);
    descriptor = SD_SetCacheable(cacheable, descriptor);
    descriptor = SD_SetDomain(domain, descriptor);
    descriptor = SD_SetImplementationDefined(implementationDefined, descriptor);
    return descriptor;
}

void FLPT_InsertSectionDescriptor(
    unsigned long *tableBaseAdr,
    int            index,
    unsigned long  descriptor)
{
    *(tableBaseAdr + index) = descriptor;
}

void FLPT_Zeroise(
    unsigned long *base_adr,
    int            numberOfdescriptors) {
    unsigned long faultDescriptor = FLPT_CreateFaultDescriptor(0);
    int i;
    for (i=0; i < numberOfdescriptors; i++) {
        FLPT_InsertFaultDescriptor(base_adr, i, faultDescriptor);
    }
}

void configure_caches(void)
{
    // Disable caches
//    Cache_DataOff();
printf("1");
    Cache_InstOff();
//    Cache_WriteBufferOff();

    // Disable MMU
printf("2");
    MMU_SetEnabled(0);
printf("3");
    MMU_InvalidateDataTLB();
printf("4");
    MMU_InvalidateInstructionTLB();

    // Setup the MMU
printf("5");
    MMU_SetAlignmentChecked(1);
printf("6");
    MMU_SetROMPermission(0);
printf("7");
    MMU_SetSystemPermission(1);

    // Allow client access to all protection domains
    int i;
    for (i=0; i < NUM_DOMAINS; i++) {
        MMU_SetDomainAccessValue(i, DAV_CLIENT);
    }
printf("8");

    // Allocate first level page table, which we'll populate only with section
    // descriptors, which cover 1MB each. Table must be aligned to a 16KB
    // boundary.
    // We'll put it 4KB into the SRAM and it will occupy:
    //   64 entries for SDRAM
    //   1  entry for SRAM
    //   16 entries for APB bridge A
    //   16 entries for APB bridge B
    // The largest memory address we need to map is that of the SRAM at
    // 0x4c000000 -> (4c000000/2^20)*4 = offset 1300h from table start ->
    // require at least 1300h/4 +1 entries in table = 1217
    unsigned long *firstLevelPageTableBaseAdr = (unsigned long*)SRAM_BASE;
    FLPT_Zeroise(firstLevelPageTableBaseAdr, 4096);
printf("9");

    // Map entire adr space uncached, unbuffered, read/write, virtual == physical
    unsigned megabytesPresent = 4096;
    unsigned index = 0;
    for (i=0; i < megabytesPresent; i++) {
        FLPT_InsertSectionDescriptor(
            firstLevelPageTableBaseAdr,
            index,
            FLPT_CreateSectionDescriptor(
                index * 1024 * 1024,    // Base address
                0,                      // Domain number
                0,                      // Implementation defined
                AP_LEVEL_1,             // Access permissions
                0,                      // Bufferable
                0));                    // Cacheable

        ++index;
    }
printf("10");

    // Map SDRAM as cached and buffered, read/write, virtual == physical
    megabytesPresent = 64;
    index = PHYS_SDRAM_1_PA / (1024 * 1024);
    for (i=0; i < megabytesPresent; i++) {
        FLPT_InsertSectionDescriptor(
            firstLevelPageTableBaseAdr,
            index,
            FLPT_CreateSectionDescriptor(
                index * 1024 * 1024,    // Base address
                0,                      // Domain number
                0,                      // Implementation defined
                AP_LEVEL_1,             // Access permissions
                1,                      // Bufferable
                1));                    // Cacheable

        ++index;
    }
printf("11");

    // Map SRAM as cached and buffered, read/write, virtual == physical
    megabytesPresent = 1;   // Actually only 32KB
    index = SRAM_BASE / (1024 * 1024);
    for (i=0; i < megabytesPresent; i++) {
        FLPT_InsertSectionDescriptor(
            firstLevelPageTableBaseAdr,
            index,
            FLPT_CreateSectionDescriptor(
                index * 1024 * 1024,    // Base address
                0,                      // Domain number
                0,                      // Implementation defined
                AP_LEVEL_1,             // Access permissions
                1,                      // Bufferable
                1));                    // Cacheable

        ++index;
    }
printf("12");

    // Map APB bridge A address space as uncached, unbuffered, read/write,
    // virtual == physical
    megabytesPresent = 16;
    index = APB_BRIDGE_A_BASE_PA / (1024 * 1024);
    for (i=0; i < megabytesPresent; i++) {
        FLPT_InsertSectionDescriptor(
            firstLevelPageTableBaseAdr,
            index,
            FLPT_CreateSectionDescriptor(
                index * 1024 * 1024,    // Base address
                0,                      // Domain number
                0,                      // Implementation defined
                AP_LEVEL_1,             // Access permissions
                0,                      // Bufferable
                0));                    // Cacheable

        ++index;
    }
printf("13");

    // Map APB bridge B address space as uncached, unbuffered, read/write,
    // virtual == physical
    megabytesPresent = 16;
    index = APB_BRIDGE_B_BASE_PA / (1024 * 1024);
    for (i=0; i < megabytesPresent; i++) {
        FLPT_InsertSectionDescriptor(
            firstLevelPageTableBaseAdr,
            index,
            FLPT_CreateSectionDescriptor(
                index * 1024 * 1024,    // Base address
                0,                      // Domain number
                0,                      // Implementation defined
                AP_LEVEL_1,             // Access permissions
                0,                      // Bufferable
                0));                    // Cacheable

        ++index;
    }
printf("14");

    // Load base address of first level page table
    MMU_SetTranslationTableBaseAddress(firstLevelPageTableBaseAdr);
printf("15");

    // Enable MMU
    MMU_SetEnabled(1);
printf("16");

    // Enable caches
    Cache_DataOn();
printf("17");
    Cache_InstOn();
printf("18");
    Cache_WriteBufferOn();
printf("19");
}

