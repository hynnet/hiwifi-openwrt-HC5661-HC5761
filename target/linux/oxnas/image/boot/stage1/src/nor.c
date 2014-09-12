/*J J Larkworthy 
 * Oxford Semiconductors Ltd
 * 
 * 12 December 2008
 * 
 * NOR flash access functions.
 * 
 */

#include "oxnas.h"
#include "nor.h"
#include "debug.h"

/* SD COMBO registers */
#define SD_CONFIG 		(SD_BASE + 0x00)
#define SD_COMMAND      (SD_BASE + 0x04)  
#define SD_ARGUMENT     (SD_BASE + 0x08)
#define SD_DATA_SIZE    (SD_BASE + 0x0C)
#define SD_STATUS       (SD_BASE + 0x1C)

#define SD_CMD_FINISHED   (1<<0)
#define SD_PENDING_CMD    (1<<2)
#define SD_TIMEOUT_B      (1<<7)
#define SD_CMD_IDLE       (1<<8)
#define SD_CMD_RES_BSY     (1<<9)
#define SD_ERROR_BIT       (1<<10)
#define SD_CMD_RES_SILENT (1<<11)
#define SD_DATA_IDLE      (1<<12)
#define SD_DATA_BUSY      (1<<13)
#define SD_DATA_ERR       (1<<14)

#define SD_RESPONSE1    (SD_BASE + 0x24)
#define SD_RESPONSE2    (SD_BASE + 0x28)

#define SD_FIFO_PORT    (SD_BASE + 0x40)
#define SD_FIFO_CTRL    (SD_BASE + 0x44)
#define SD_FIFO_STATUS  (SD_BASE + 0x48)
#define SD_CLOCK_CTRL   (SD_BASE + 0x50)
#define SD_GAP_CTRL     (SD_BASE + 0x54)
#define SD_TIME_CTRL    (SD_BASE + 0x58)
#define SD_SM_CTRL      (SD_BASE + 0x5C)

#define SM_ERROR_ACK_B  (1<<1)

#define SD_BANK0_CONFIG (SD_BASE + 0x60)
#define SD_BANK1_CONFIG (SD_BASE + 0x64)


/* sys control register bits needing changing MFA functions selection */
#define SD_NOR_ENABLE0 0x000001e0	/* only enable clk, dat in out and cs. */

#define SD_SPI_CONFIG  0x00010000 /* bank 0 in spi mode */

#define SD_SPI_PORT    0x00001428 /* bank gap length 1 cycles */

#define SD_CLOCK_STOP  0x800000FE
#define SD_CLOCK_RUN   0x000000FE /* 400kHz at 200MHz clock. */


#define SD_FLASH_READ  0x03400401
#define SD_READ_BLOCK  0x51410601



#define SD_FIFO_FLUSH  (0x3)
#define SD_FIFO_EMPTY  (1<<17)


/* testing definition */
#ifdef TEST
#define WRITE_SD_COMMAND(a) do{ barrier(); writel(a, SD_COMMAND) ; } while (0)
#else
#define WRITE_SD_COMMAND(a) do{ barrier(); sd_combo->command.data =a; } while (0)
#endif
static  struct SD_combo  * const sd_combo = (struct SD_combo *) SD_BASE;

#define MAX_STAT_LOG 32


static void empty_fifo(void) {
	/* keep reading until the empty bit is set */
	while ((readl(SD_FIFO_STATUS) & SD_FIFO_EMPTY) != SD_FIFO_EMPTY) {
		readl(SD_FIFO_PORT);
	}

	/* Flush and reset SD combo FIFO so it really is empty.
	 */
	writel(SD_FIFO_FLUSH, SD_FIFO_CTRL);
}

static int wait_fifo(void)
{
	u32 fifo_status = readl(SD_FIFO_STATUS);
	/* wait here until data available in fifo
	 * detected by either a count value or
	 * fifo full flag being set.
	 */
	
	while (((fifo_status & 0xffff) == 0) &&  /* count empty */
		((fifo_status & 0x100000) == 0)) {   /* fifo not full */
		
		fifo_status = readl(SD_FIFO_STATUS);
		
		//if (limit_reached())
		//	return 0;
	};
	return 1;
}


void init_NOR(void)
{


	/* enable clock and release sd block reset */
	writel((1<<SYS_CTRL_RSTEN_SD_BIT), SYS_CTRL_RSTEN_SET_CTRL);/* make sure it is in reset */
	writel((1<<SYS_CTRL_CKEN_SD_BIT), SYS_CTRL_CKEN_SET_CTRL);
	writel((1<<SYS_CTRL_RSTEN_SD_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
	
	/* standard serial NOR flash */
	/* enable access to spi bus devices.
	 * clock, reset and GPIO functions 
	 * normal sda interface selection.
	 */
	writel(readl(SYS_CTRL_SECONDARY_SEL) | SD_NOR_ENABLE0, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL) & ~SD_NOR_ENABLE0, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL) & ~SD_NOR_ENABLE0, SYS_CTRL_DEBUG_SEL);

	
	writel(SD_CLOCK_STOP, SD_CLOCK_CTRL);
	writel(SD_CLOCK_RUN, SD_CLOCK_CTRL);


	/* put sd core into spi mode for port 0 */
	writel(SD_SPI_PORT,   SD_BANK0_CONFIG);
	writel(SD_SPI_CONFIG, SD_CONFIG);
	
}



void read_NOR(struct load_data *load)
{

	u32 *address = load->buffer;
	u32 length = load->length;
	
	/* read any data remaining in the FIFO until the status is empty */
	empty_fifo();
	writel(0x3, SD_STATUS); /* clear completed sticky bits */

	/* read block from spi device 
	 */
	/* issue commands dependent on connected device */

	//sift left 8bits for EN25F04 flash only has 24 bits address. (4bytes only when reading data)
	writel((load->start << 8) , SD_ARGUMENT);
	/* transfer a single block of length */
	writel(((0x3ff & length) | 0x10000), SD_DATA_SIZE);

	WRITE_SD_COMMAND(SD_FLASH_READ);

	/* save data in memory
	 */
	while (length) {
		wait_fifo();
		*address = readl(SD_FIFO_PORT);
		(u32 *) address++;
		length -= 4;
		load->start += 4;
	}
	load->buffer = address;

}


void close_NOR(void)
{
	/* close spi bus access devices.
	 */
	writel((1<<SYS_CTRL_RSTEN_SD_BIT), SYS_CTRL_RSTEN_SET_CTRL);
	writel((1<<SYS_CTRL_CKEN_SD_BIT), SYS_CTRL_PCI_STAT);
}
