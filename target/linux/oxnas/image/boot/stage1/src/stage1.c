/*******************************************************************
*
* File:            stage1.c
*
* Description:     Top(ish) level of a bootloader, loaded and run
*                  by the boot ROM, it sets up memory, runs a simple mem
*                  test, then attempts to load some software, usually u-boot
*                  from disk. it expects two CRC protected copies of the 
*                  software per disk for redundent backup.
*
* Author:          Richard Crewe
*
* Copyright:       Oxford Semiconductor Ltd, 2009
*/
#include "oxnas.h"
#if defined(SDK_BUILD_SPI_BOOT)
#include "nor.h"
#elif defined(SDK_BUILD_NAND_BOOT)
#include "nand.h"
#else
#include "sata.h"
#endif
#include "debug.h"
#include "crc32.h"
#include "ddr_oxsemi.h"
#include "pll.h"

/* Forward declaration of static functions */
static void init_ddr(int mhz);
static int test_memory(u32 memory);
static void start_timer(void);

#if defined(SDK_BUILD_SPI_BOOT)
static const u32 stage2_disk_sector[] = {
    NOR_BOOT_STAGE2,
    NOR_RECOVERY_STAGE2
};
static const u32 numStage2Images = sizeof(stage2_disk_sector) / sizeof(u32);
static u32 stage2_blocks;
#elif defined(SDK_BUILD_NAND_BOOT)
static const u32 stage2_nand_block[] = {
    SDK_BUILD_NAND_STAGE2_BLOCK,
    SDK_BUILD_NAND_STAGE2_BLOCK2
};
static const u32 numStage2Images = sizeof(stage2_nand_block) / sizeof(u32);
#else
static const u32 stage2_disk_sector[] = {
    SECTOR_BOOT_STAGE2,
    SECTOR_RECOVERY_STAGE2
};
static const u32 numStage2Images = sizeof(stage2_disk_sector) / sizeof(u32);
static u32 stage2_blocks;
#endif

static u32 stage2_ram_addr;
static u32 *header_length;
static u32 *header_crc;
extern const char *build_string;

#define RPSA_GPIO     ((void *) (RPSA_BASE + 0X3C0))

inline int secure_boot(void) {
	return (readl(RPSA_GPIO) >>24) & 1 ? 0 : 1 ;
}

#if (PLL_FREQUENCY_PROMPT == 1)
int set_pll_ask()
{
    int idx,i; 
    int num_configs = plla_num_configs();
//  int mhz;
//  char key;

    for (i=0;i<num_configs;i++)
    {
        char str[5] = { 'A'+i,' ',':',' ',0 };
        
        putstr(debug_uart, str);
        putstr(debug_uart, ultodec((unsigned)plla_config_mhz(i)));
        putstr(debug_uart, "\r\n");
    }
    
    putstr(debug_uart, "\r\nSelect a PLL frequency : ");

    /*
    do 
    {
	    putstr(debug_uart, "\r\nEnter PLL frequency in MHz : ");
	    key = mhz = 0;
	    
	    while (key /= '\r')
	    {
	        key = getc_NS16550(debug_uart);
	        
	        if ((key >= '0') && (key <= '9'))
	        {
	            putc_NS16550(debug_uart, key); // Echo
	            mhz = (mhz * 10) + (key - '0');
	        }
	    }
	    
	} while (mhz > 1200);
    
    plla_set_mhz(mhz);
    */
    
    idx = (getc_NS16550(debug_uart) & ~0x20) - 'A';
    
    putstr(debug_uart, "\r\n");
    
    if ((idx >= num_configs) || (idx < 0))
    {
        putstr(debug_uart, "Invalid selection\r\n");
        idx = 0;
    }
        
    return plla_set_config(idx);
}
#endif // PLL_FREQUENCY_PROMPT

int main(void)
{
    int stage2ImageNumber;
    unsigned int keeplooping;
    int baud_divisor_x16;
#ifdef SDK_BUILD_NAND_BOOT
    struct nand_t nand;
    u32 bytes_read;
#else
    int disk;
#endif

#ifndef SDK_BUILD_NAND_BOOT    
    /* software is assumed to be a maximum of 256kib */
    stage2_blocks = 256*1024/512 ;
#endif
    stage2ImageNumber = 0;
    
    /* The location in memory where the software will be loaded */
    stage2_ram_addr = 0x60d00000;
    
    /* CRC parameters */
    header_length = (u32 *) (stage2_ram_addr - 8);
    header_crc    = (u32 *) (stage2_ram_addr - 4);

    // UART divider is based on the RPS clock
    baud_divisor_x16 = RPS_CLK / 115200;
    start_timer();

#ifdef CONFIG_UARTB
	debug_uart = (NS16550_t) UART_2_BASE;
	init_NS16550(debug_uart, baud_divisor_x16);
#else
	debug_uart = (NS16550_t) UART_1_BASE;
	if (secure_boot()) { 
		// let CoPro intialise and use UART.
		udelay(1000);
	}
	else {
		// Initialise the UART to be used for debug messages
		init_NS16550(debug_uart, baud_divisor_x16);
	}
#endif
    
    /* wait for UART to settle */
    udelay(100);
    putstr(debug_uart, "\r\n");
    putstr(debug_uart, "NAS7820 BootStrap v0.1 \r\n[");
    putstr(debug_uart, build_string);
    putstr(debug_uart, "]\r\n");
    putstr(debug_uart, "PLX Technology, Inc. 2010. All Rights Reserved");
    putstr(debug_uart, "\r\n");
    putstr(debug_uart, "Copyright, lintel 2013.09");
    putstr(debug_uart, "\r\n");
    
#if (PLL_FREQUENCY_PROMPT == 1)
    init_ddr(set_pll_ask());
#else
    init_ddr(plla_set_config(PLL_FIXED_INDEX));
#endif
    putstr(debug_uart, "\r\n");
    putstr(debug_uart, "Testing Stage2 Memory...");
    if (test_memory(stage2_ram_addr) < 0) {
        putstr(debug_uart, "Memory test failed, reverting to default PLL config");
        init_ddr(plla_set_config(0));
    }
    else
    putstr(debug_uart, "OK!\r\n");

#ifndef SDK_BUILD_NAND_BOOT
    disk = 0;
#endif

#ifdef SDK_BUILD_SPI_BOOT
    init_NOR();

    do {
    	struct load_data nor_data;
    	u32 block_count = stage2_blocks;

        keeplooping = 1;
        putstr(debug_uart, "\r\nReading NOR ");
        putc_NS16550(debug_uart, (char) ('0' + (char)disk));
        putstr(debug_uart, ", Image ");
        putc_NS16550(debug_uart, (char) ('0' + (char)stage2ImageNumber));

        *header_length = 0xA1A2A3A4;
        *header_crc    = 0xB1B2B3B4;
        
        /* fetch stage-2 (u-Boot) from SATA disk */
        
        putstr(debug_uart, "\r\n  Address : ");
        puthex32(debug_uart, stage2_disk_sector[stage2ImageNumber]);
        nor_data.buffer= header_length;
	nor_data.start = stage2_disk_sector[stage2ImageNumber];
	
	
	do {
		nor_data.length= NOR_BLOCK;
		read_NOR(&nor_data);
	} while (--block_count);
        
        putstr(debug_uart, "\r\n  Hdr len: ");
        puthex32(debug_uart, *header_length);
        putstr(debug_uart, "\r\n  Hdr CRC: ");
        puthex32(debug_uart, *header_crc);        
        putstr(debug_uart, "\r\n");
                        
        /* try the backup stage2 on this disk first (first time round, at least we know
           it is working to some extent, go to next disk if this wraps round */

	if (++stage2ImageNumber >= numStage2Images) {
		stage2ImageNumber=0;
	}

        if (*header_length == 0) {
            putstr(debug_uart, " length 0");
        } else if (*header_length > (stage2_blocks * 512) ) {
            putstr(debug_uart, " too big ");
        } else if (*header_crc != crc32(0, (unsigned char*)stage2_ram_addr,*header_length)) {
            putstr(debug_uart, " CRC fail");
        } else {
            putstr(debug_uart, " OK");
            keeplooping = 0;
        }

    } while (keeplooping);

    putc_NS16550(debug_uart, '\r');
    putc_NS16550(debug_uart, '\n');
#else

#ifndef SDK_BUILD_NAND_BOOT
    /* reset sata core */
    init_sata_hw();
#else
    nand_init(&nand);
#endif

    do {
        keeplooping = 1;
#ifndef SDK_BUILD_NAND_BOOT
	int blocks_read;
        putstr(debug_uart, "\r\nReading disk ");
        putc_NS16550(debug_uart, (char) ('0' + (char)disk));
#else
        putstr(debug_uart, "\r\nReading Nand Flash");
        putstr(debug_uart, ", Image ");
#endif
        putc_NS16550(debug_uart, (char) ('0' + (char)stage2ImageNumber));

        *header_length = 0xA1A2A3A4;
        *header_crc    = 0xB1B2B3B4;
        
#ifndef SDK_BUILD_NAND_BOOT
        /* fetch stage-2 (u-Boot) from SATA disk */
        
        putstr(debug_uart, "\r\n  Sector : ");
        puthex32(debug_uart, stage2_disk_sector[stage2ImageNumber]);
        
        blocks_read = ide_read(disk,
                               stage2_disk_sector[stage2ImageNumber],
                               stage2_blocks,
                               header_length );
#else
		/* fetch stage-2 (u-Boot) header from NAND flash */
		bytes_read = nand_read(&nand,
				stage2_nand_block[stage2ImageNumber] * nand.block_size,
				8, (u8 *)header_length);
		/* fetch stage-2 (u-Boot) from NAND flash */
		bytes_read = nand_read(&nand,
				stage2_nand_block[stage2ImageNumber] * nand.block_size,
				(*header_length) + 8, (u8 *)header_length);
#endif

        putstr(debug_uart, "\r\n  Hdr len: ");
        puthex32(debug_uart, *header_length);
        putstr(debug_uart, "\r\n  Hdr CRC: ");
        puthex32(debug_uart, *header_crc);        
        putstr(debug_uart, "\r\n");
                        
        /* try the backup stage2 on this disk first (first time round, at least we know
           it is working to some extent, go to next disk if this wraps round */
        if (++stage2ImageNumber >= numStage2Images) {
            stage2ImageNumber = 0;
#ifndef SDK_BUILD_NAND_BOOT
            if (++disk > 1) {
                disk = 0;
            }
#endif
        }

#ifndef SDK_BUILD_NAND_BOOT 
        if (blocks_read != stage2_blocks ) {
#else
	if (bytes_read != (*header_length) + 8) {
#ifdef SDK_BUILD_DEBUG
		unsigned int i, j = 0;
		for (i = 0; i < bytes_read; i++) {
			if (j++ % 16 == 0) {
				putstr(debug_uart, "\r\n");
			} else {
				putstr(debug_uart, " ");
			}
			puthex8(debug_uart, *(((u8 *)stage2_ram_addr)+i));
		}
#endif
#endif
            putstr(debug_uart, " read failed");
        } else if (*header_length == 0) {
            putstr(debug_uart, " length 0");
#ifndef SDK_BUILD_NAND_BOOT
        } else if (*header_length > (stage2_blocks * 512) ) {
            putstr(debug_uart, " too big ");
#endif
        } else if (*header_crc != crc32(0, (unsigned char*)stage2_ram_addr,*header_length)) {
            putstr(debug_uart, " CRC fail");
        } else {
            putstr(debug_uart, " OK");
            keeplooping = 0;
        } 

    } while (keeplooping);

    putc_NS16550(debug_uart, '\r');
    putc_NS16550(debug_uart, '\n');

#endif
    /* execute the loaded software */
    ((void (*)(void)) stage2_ram_addr)();

    return 0;
}

//Function used to Setup SDRAM in DDR/SDR mode
static void init_ddr(int mhz) {
    /* start clocks */
    writel((1 << SYS_CTRL_CKEN_DDR_PHY_BIT), SYS_CTRL_CKEN_SET_CTRL);
    writel((1 << SYS_CTRL_CKEN_DDR_BIT), SYS_CTRL_CKEN_SET_CTRL);
    writel((1 << SYS_CTRL_CKEN_DDR_CK), SYS_CTRL_CKEN_SET_CTRL);

    /* bring phy and core out of reset */
    writel((1 << SYS_CTRL_RSTEN_DDRPHY_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
    writel((1 << SYS_CTRL_RSTEN_DDR_BIT), SYS_CTRL_RSTEN_CLR_CTRL);

	/* DDR runs at half the speed of the CPU */
    ddr_oxsemi_setup(mhz >> 1);
    return;
}
  
/*******************************************************************
 *
 * Function:             test_memory
 *
 * Description:          Check that memory is accessible and retains data.
 *
 ******************************************************************/
#define DILIGENCE (1048576/4)

static int test_memory(u32 memory)
{
    volatile u32 *read;
    volatile u32 *write;
    const u32 INIT_PATTERN = 0xAA55AA55;
    const u32 INC_PATTERN  = 0x01030507;
    u32 pattern;
    int check;
    int i;

//    do {
        check = 0;
        read = write = (volatile u32 *) memory;
        pattern = INIT_PATTERN;
        for (i = 0; i < DILIGENCE; i++) {
            *write++ = pattern;
//           pattern = ~pattern;
            pattern += INC_PATTERN;
        }
//         putstr(debug_uart, ", testing");
        pattern = INIT_PATTERN;
        for (i = 0; i < DILIGENCE; i++) {
            check += (pattern == *read++) ? 1 : 0;
//          pattern = ~pattern;
            pattern += INC_PATTERN;
        }
//    } while (check < DILIGENCE);
    return (check == DILIGENCE) ? 0 : -1;
}

/*******************************************************************
 *
 * Function:             start_timer
 *
 * Description:          initialise the timer for use
 *
 ******************************************************************/
static void start_timer(void)
{
    /* enable clock in sys_ctrl */
    writel(SLOW_TIMER_TICK, SYS_CTRL_CKCTRL_CTRL);

}
/*******************************************************************
 *
 * Function:             udelay
 *
 * Description:          function to pause operation in a busy wait 
 *                       for a number of microseconds.
 *
 ******************************************************************/
void udelay(u32 time)
{
    /* calculate time in ticks with limits*/
    u32 limit = USECS2TICKS(time);
    if (limit < 2)
        limit = 2;
    else if (limit > 0xFFFFFF)
        limit=0xFFFFFF;
    
    /* set the clock mode and prescale */
    writel(RPS_CLK_CTRL_DATA, RPSA_CLK_CTRL);
    writel(~0, RPSA_CLK_CLEAR);     
    barrier();
    
    /* set time */
    writel(limit, RPSA_CLK_LOAD);
    barrier();
    
    /* start */
    writel((RPS_CLK_CTRL_DATA | CLK_ENABLE), RPSA_CLK_CTRL);
    
    /* wait */
    while(!(readl(RPSA_INT_STATUS) & RPSA_CLK_INT))
        ;
}
