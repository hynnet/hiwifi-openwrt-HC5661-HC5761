/*******************************************************************
*
* File:            sata.c
*
* Description:     SATA control.
*
* Author:          Richard Crewe
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*
*
*/

#include "types.h"
#include "oxnas.h"
#include "debug.h"
#include "sata.h"
#include "dma.h"

static void CrazyDumpDebug(void);

#define MAKE_FIELD(value, num_bits, bit_num) \
(((value) & ((1 << (num_bits)) - 1)) << (bit_num))

#define CFG_ATA_DATA_OFFSET 0
#define CFG_ATA_REG_OFFSET  0
#define CFG_ATA_ALT_OFFSET  0

/* The internal SATA drive on which we should attempt to find partitions */
static volatile u32 sata_regs_base;

/* multiples of 10us */
static const int MAX_SRC_READ_LOOPS = 50;
static const int MAX_SRC_WRITE_LOOPS = 50;

/* multiples of ? */
static const int MAX_DMA_XFER_LOOPS = 500;
static const int MAX_DMA_ABORT_LOOPS = 10000;
static const int MAX_NO_ERROR_LOOPS = 500;
static const int MAX_NOT_BUSY_LOOPS = 500;

static const int IDE_SPIN_UP_TIME_OUT = 10000; 
static const int IDE_TIME_OUT = 5000; /* 500 x 1ms = 500ms*/

/* static data controlling the operation of the DMA controller in this 
* application.
*/
static oxnas_dma_device_settings_t oxnas_sata_dma_settings;

static const oxnas_dma_device_settings_t oxnas_sata_dma_rom_settings = {
    .address_ = SATA_DATA_BASE_PA,
    .fifo_size_ = 16,
    .dreq_ = OXNAS_DMA_DREQ_SATA,
    .read_eot_ = 0,
    .read_final_eot_ = 1,
    .write_eot_ = 0,
    .write_final_eot_ = 1,
    .bus_ = OXNAS_DMA_SIDE_B,
    .width_ = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_ = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_ = OXNAS_DMA_MODE_FIXED,
    .address_really_fixed_ = 0
};

const oxnas_dma_device_settings_t oxnas_ram_dma_rom_settings = {
    .address_ = 0,
    .fifo_size_ = 0,
    .dreq_ = OXNAS_DMA_DREQ_MEMORY,
    .read_eot_ = 1,
    .read_final_eot_ = 1,
    .write_eot_ = 1,
    .write_final_eot_ = 1,
    .bus_ = OXNAS_DMA_SIDE_A,
    .width_ = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_ = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_ = OXNAS_DMA_MODE_INC,
    .address_really_fixed_ = 0
};

#define PHY_RESET_FAIL               1
#define PHY_2ND_RESET_FAIL           2
#define PROG_READ_FAILURE            4
#define SECTOR_0_READ_FAIL           8
#define FAULT_LEDS                0x3FF
/* fault signal output controls */
#define FAULT_GPIO_OUT_SET (GPIO_A + 0x14)
#define FAULT_GPIO_DIR_OUT (GPIO_A + 0x1C)

enum fault_codes {
    NO_FAULT, 
    NOT_READY_1,
    NOT_READY_2,
    NOT_READY_3,
    NOT_READY_4,
} ;


/* memory copy - don't use standard library so need our own implimentation */
static void* memcpy(void *dest, const void *src, u32 n)
{
    void* old_dest = dest;
    for (; n > 0; n--)
        *((char *) dest++) = *((char *) src++);
    return old_dest;
}

/**
 * initialise functions and macros for ASIC implementation 
 */
#define PH_GAIN         2
#define FR_GAIN         3
#define PH_GAIN_OFFSET  6
#define FR_GAIN_OFFSET  8
#define PH_GAIN_MASK  (0x3 << PH_GAIN_OFFSET)
#define FR_GAIN_MASK  (0x3 << FR_GAIN_OFFSET)
#define USE_INT_SETTING  (1<<5)

#define LOS_AND_TX_LVL   0x2988
#define TX_ATTEN         0x55629


#define CR_READ_ENABLE  (1<<16)
#define CR_WRITE_ENABLE (1<<17)
#define CR_CAP_DATA     (1<<18)
#define SATA_PHY_ASIC_STAT  (0x44900000)
#define SATA_PHY_ASIC_DATA  (0x44900004)

static void wait_cr_ack(void){
	while ((readl(SATA_PHY_ASIC_STAT) >> 16) & 0x1f)
		/* wait for an ack bit to be set */ ;
}

static u16 read_cr(u16 address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel(CR_READ_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return readl(SATA_PHY_ASIC_STAT);
}

static void write_cr(u16 data, u16 address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel((data | CR_CAP_DATA), SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	writel(CR_WRITE_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return ;
}

static void workaround5458(void){
	unsigned i;
	
	for (i=0; i<2;i++){
		u16 rx_control = read_cr( 0x201d + (i<<8));
		rx_control &= ~(PH_GAIN_MASK | FR_GAIN_MASK);
		rx_control |= PH_GAIN << PH_GAIN_OFFSET;
		rx_control |= (FR_GAIN << FR_GAIN_OFFSET) | USE_INT_SETTING ;
		write_cr( rx_control, 0x201d+(i<<8));
	}
}


/*
 * Wait until Busy bit is off, or timeout (in ms)
 * Return last status
 */
static u8 ide_wait(int dev, u32 t)
{
    u32 delay = 10 * t;     /* poll every 1ms */
    
    /* check SATA core busy not ATA registers. */
    while ( !(readl(SATA_DATAPLANE_STAT) & SATA_CORE_DPS_IDLE0) ) {
        if (delay-- == 0) {
            return 1;
        }
        udelay(100);
    }
    /* get ATA status but mask busy bit. */
    return  0 ;
}


/* read a SATA control register */
static u32 scr_read(unsigned int sc_reg)
{
    /* Setup adr of required register. std regs start eight into async region */
    writel(sc_reg, sata_regs_base + SATA_LINK_RD_ADDR);
    barrier();
    /* Wait for data to be available */
    int loops = MAX_SRC_READ_LOOPS;
    do {
        if (readl(sata_regs_base + SATA_LINK_CONTROL) & 1UL) {
            break;
        }
        udelay(10);
    } while (--loops);
    
    if (!loops) {
        putstr(debug_uart,"scr_read() Timed out of wait for read completion\r\n");
    }
    
    /* Read the data from the async register */
    return readl(sata_regs_base + SATA_LINK_DATA);
}

/* write data to the SATA control registers. */
static void scr_write(unsigned int sc_reg, u32 val)
{
    /* Setup the data for the write */
    writel(val, sata_regs_base + SATA_LINK_DATA);
    barrier();
    
    /* Setup adr of required register. std regs start eight into async region */
    writel(sc_reg, sata_regs_base + SATA_LINK_WR_ADDR);
    
    /* Wait for data to be written */
    int loops = MAX_SRC_WRITE_LOOPS;
    do {
        if (readl(sata_regs_base + SATA_LINK_CONTROL) & 1UL) {
            break;
        }
        udelay(10);
    } while (--loops);
    
    if (!loops) {
        putstr(debug_uart,"scr_write() Timed out of wait for write completion\r\n");
    }
}

/* 5 sec wait 25 x 200ms for start up to complete */
#define PHY_LOOP_COUNT  100

/* reset the SATA PHY device and hence reset the attached disk drive */
int phy_startup(int device)
{
    /* tune for sata compatability */
    scr_write(0x60, LOS_AND_TX_LVL );

    /* each port in turn */
    scr_write(0x70, TX_ATTEN );
    
    /* Issue phy wake & core reset - force 1.5G data rate  */
    scr_write(SATA_SCR_CONTROL, 0x311);
    udelay(2000);
    scr_read(SATA_SCR_STATUS);      /* Dummy read; flush */
    udelay(2000);
    /* Issue phy wake & clear core reset keep at 1.5G */
    scr_write(SATA_SCR_CONTROL, 0x310);
    udelay(2000);
    
    /* Wait for upto 5 seconds for PHY to become ready */
    int phy_status = 0;
    int loops = 0;
    do {
        phy_status = scr_read(SATA_SCR_STATUS);
        if ((phy_status & 0xf) == 3) {
            /*clear errors*/
            scr_write( SATA_SCR_ERROR, ~0);
            return 1;
        }
        udelay(200000);
    } while (++loops < PHY_LOOP_COUNT);
    
    return 0;
}

#define FIS_LOOP_COUNT 50
static int wait_fis(void)
{
    /* Wait for upto 10 seconds for FIS to be received */
    int fis_status = 0;
    int loops = 0;
    do {
        udelay(2000);
        if ((readl(sata_regs_base + SATA_ORB2_OFF) & 0xff) > 0) {
            fis_status = 1;
            break;
        }
    } while (++loops < FIS_LOOP_COUNT);
    
    if (loops >= FIS_LOOP_COUNT) {
        putstr(debug_uart, "No FIS received\r\n");
    }
    
    return fis_status;
}

/**
* Possible that ATA status will not become no-error, so must have timeout
* @returns An int which is zero on error
*/
static inline int wait_no_error(int device)
{
    int status = 0;
    int loops = MAX_NO_ERROR_LOOPS;
    do {
        if (!(readl(sata_regs_base + SATA_ORB2_OFF) & (1 << ATA_STATUS_ERR_BIT))) {
            status = 1;
            break;
        }
        udelay(10);
    } while (--loops);
    
    if (!loops) {
        putstr(debug_uart, "wait_no_error() Timed out of wait for SATA no-error condition\r\n");
    }
    
    return status;
}

/**
* Expect SATA command to always finish, perhaps with error
* @returns An int which is zero on error
*/
static inline int wait_sata_command_not_busy(int device)
{
    /* Wait for data to be available */
    int status = 0;
    int loops = MAX_NOT_BUSY_LOOPS;
    do {
        if (!(readl(sata_regs_base + SATA_COMMAND_OFF) & (1<<SATA_CMD_BUSY_BIT))) 
        {
            status = 1;
            break;
        }
        udelay(100);
    } while (--loops);
    
    if (!loops) {
        putstr(debug_uart, "wait_sata_command_not_busy() Timed out of wait"
            " for SATA command to finish\r\n");
    }
    
    return status;
}

/* true if the DMA channel is still busy */
static inline int dma_sata_busy(void)
{
    return (*DMA_CALC_REG_ADR(SATA_DMA_CHANNEL, DMA_CTRL_STATUS)) & DMA_CTRL_STATUS_IN_PROGRESS;
}

/* start the DMA transfer */
static void dma_sata_start(void)
{
    *DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_CTRL_STATUS) =
        encode_start(* (DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_CTRL_STATUS)));
}

/* configure the DMA controller to transfer data from the IDE controller data 
 * buffers into memory.
 */
static void dma_start_sata_read(u32 * buffer, int num_bytes)
{
    /* Assemble complete memory settings */
    /* initialise image with default settings for dma */
    oxnas_dma_device_settings_t mem_settings = oxnas_ram_dma_rom_settings;
    
    /* put address of where to put data */
    mem_settings.address_ = (unsigned long) buffer;
    
    /* initialise image with default sata control settings */
    memcpy(&oxnas_sata_dma_settings, &oxnas_sata_dma_rom_settings,
        sizeof(oxnas_dma_device_settings_t));
    
    /* configure the DMA controller to transfer the incoming data */
    *DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_CTRL_STATUS) = encode_control_status
        (&oxnas_sata_dma_settings, &mem_settings);
    *DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_BASE_SRC_ADR) = mem_settings.address_;
    *DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_BASE_DST_ADR) = mem_settings.address_;
    *DMA_CALC_REG_ADR(SATA_DMA_CHANNEL,DMA_BYTE_CNT) =
        encode_final_eot(&oxnas_sata_dma_settings, &mem_settings,num_bytes);
    
    dma_sata_start();
}

/**
 * Ref bug-6320
 *
 * This code is a work around for a DMA hardware bug that will repeat the 
 * penultimate 8-bytes on some reads. This code will check that the amount 
 * of data transferred is a multiple of 512 bytes, if not it will fetch the
 * correct data from a buffer in the SATA core and copy it into memory.
 */
static void sata_dma_corruption_workaround(
	int  port,
	u32 *buffer,
	int  length)
{
	u32 quads_transferred;
	u32 remainder;
	u32 sector_quads_remaining;

    /* Check for an incomplete transfer, i.e. not a multiple of 512 bytes
       transferred (datacount_port register counts quads transferred) */
    quads_transferred = *((u32*)(port ? SATA_DATACOUNT_PORT1 : SATA_DATACOUNT_PORT0));

    remainder = quads_transferred & 0x7f;
    sector_quads_remaining = remainder ? (0x80 - remainder): 0;

    if (sector_quads_remaining == 2) {
        void *sata_data_port_base;
        int sata_offset;
        u32 *sata_data_ptr;
        volatile u32 *memory_data_ptr;

		putstr(debug_uart, "\r\nSATA read fixup port ");
		puthex32(debug_uart, port);
		putstr(debug_uart, " only transfered ");
		puthex32(debug_uart, quads_transferred);
		putstr(debug_uart, " quads, sector_quads_remaining ");
		puthex32(debug_uart, sector_quads_remaining);
		putstr(debug_uart, "\r\n");

        /* Determine the addresses in the SATA and memory buffers of the
           last two quads of the transfer */
        sata_data_port_base = (void*)(port ? SATA_DATA_MUX_RAM1 : SATA_DATA_MUX_RAM0);

        sata_offset = ((length - 1) % 2048);
        sata_data_ptr = (u32*)(sata_data_port_base + sata_offset - 7);

        memory_data_ptr = (u32*)(buffer + length - 8);

		putstr(debug_uart, " sata_data_ptr ");
		puthex32(debug_uart, (u32)sata_data_ptr);
		putstr(debug_uart, ", memory_data_ptr ");
		puthex32(debug_uart, (u32)memory_data_ptr);
		putstr(debug_uart, "\r\n");

        /* Copy the correct last two quads of the transfer from the SATA
           buffer to memory. The total transfer length will always be at
           least a multiple of 512 bytes so no need to worry about wrapping
           around the end of the SATA data mux ram FIFO */
        *memory_data_ptr       = *sata_data_ptr;
        *(memory_data_ptr + 1) = *(sata_data_ptr + 1);
    } else if (sector_quads_remaining) {
		putstr(debug_uart, "\r\nSATA read fixup port ");
		puthex32(debug_uart, port);
		putstr(debug_uart, " cannot deal with ");
		puthex32(debug_uart, sector_quads_remaining);
		putstr(debug_uart, " quads remaining\r\n");
    }
}

/* read data from an IDE (SATA) disk drive */
u32 ide_read(int device,
             u32 blknr,
             u32 blkcnt,
             u32* buffer)
{
    u32 n = 0;
    
    switch (device) {
    case 0:
        sata_regs_base = SATA0_REGS_BASE;
        break;
    case 1:
        sata_regs_base = SATA1_REGS_BASE;
        break;
    default:
        return 0;
        break;
    }
    
    if (!phy_startup(device) ||
        !wait_fis()) {
        return 0;
    }

    /* clear interupt register of any pending status bits */
    writel(0xFFFFFFFF, sata_regs_base + SATA_INT_STATUS_OFF);
    writel( readl(SATA_DATAPLANE_CTRL) | SATA_CORE_DPC_EMSK, SATA_DATAPLANE_CTRL); 
    
    while (blkcnt > 0) {
        unsigned int blocks;
        unsigned int reg;
        
        /* limit transfers to 128 sectors */
        if (blkcnt > 128) {
            blocks = 128;
        } else {
            blocks = blkcnt;
        }
        
        /* Start the DMA channel receiving data from the SATA 
        * core into the passed buffer */
        dma_start_sata_read(buffer, blocks * ATA_SECTOR_BYTES);
        
        /* device/head */
        reg = 0xE0 << 24; 
        writel( reg , sata_regs_base + SATA_ORB1_OFF );
        
        /* command, features, sector count (limited to 1 - 256)*/
        reg  = ATA_CMD_READ_DMA << 24;
        reg |= blocks & 0xff;
        writel( reg , sata_regs_base + SATA_ORB2_OFF );
        
        /* lba 0-31 */
        reg = blknr;
        writel( reg , sata_regs_base + SATA_ORB3_OFF );
        
        /* control, features-ext, lba 32-47 */
        reg = 0;
        writel( reg , sata_regs_base + SATA_ORB4_OFF );

        barrier();
        wait_sata_command_not_busy(device);
        reg = readl(sata_regs_base + SATA_COMMAND_OFF);
        reg &= ~SATA_OPCODE_MASK;
        reg |= SATA_CMD_WRITE_TO_ORB_REGS;
        writel(reg, sata_regs_base + SATA_COMMAND_OFF);
        barrier();
        
        if(ide_wait(device, IDE_TIME_OUT)) {
            putstr(debug_uart, "IDE read: device not ready\r\n");
            CrazyDumpDebug();
            while(1)
                ;
            break;
        }
        //CrazyDumpDebug();
        /* clear interrupts */
        writel( ~0, sata_regs_base + SATA_INT_STATUS_OFF);

		sata_dma_corruption_workaround(device, buffer, blocks * ATA_SECTOR_BYTES);
        
        /* update pointers and counters */ 
        blkcnt -= blocks;
        n += blocks;
        blknr += blocks;
        buffer += blocks * (ATA_SECTOR_BYTES / sizeof(*buffer));
    }

    return (n);
}

/*******************************************************************
 *
 * Function:             init_sata_hw
 *
 * Description:          initialise secondary facilities for sata use
 *
 ******************************************************************/
void init_sata_hw(void)
{
    /* Put cores into reset */
    writel((1 << SYS_CTRL_RSTEN_SATA_BIT) |
           (1 << SYS_CTRL_RSTEN_SATA_LK_BIT) |
           (1 << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_SET_CTRL);
    
    /* Enable clocks */
    writel((1 << SYS_CTRL_CKEN_SATA_BIT), SYS_CTRL_CKEN_SET_CTRL);
    
    /* Take cores out of reset */
    udelay(100);
    writel((1 << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
    
    udelay(100);
    writel((1 << SYS_CTRL_RSTEN_SATA_BIT) |
           (1 << SYS_CTRL_RSTEN_SATA_LK_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
    udelay(100);
    /* correct bandwidth in Synopsis phy after reset */
    workaround5458();
}


static void CrazyDumpDebug(void) {
    unsigned int part = 0;
    unsigned int reg;
    unsigned int port_addresss[] = {
        SATA0_REGS_BASE,
        SATA1_REGS_BASE,
        DMA_BASE,
        SATACORE_REGS_BASE,
        0
    };
    
    while (port_addresss[part]) {
        for ( reg = port_addresss[part]; reg < (port_addresss[part] + (0x100));reg+=4) {
            putc_NS16550(debug_uart, '[');
            putstr(debug_uart, ultohex(reg) );
            putc_NS16550(debug_uart, ']');
            putc_NS16550(debug_uart, '=');
            putstr(debug_uart, ultohex(readl(reg))   );
            putc_NS16550(debug_uart, '\r');
            putc_NS16550(debug_uart, '\n');
        }
        ++part;
    }
}
