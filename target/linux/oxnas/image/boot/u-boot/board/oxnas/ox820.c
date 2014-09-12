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

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

static inline void delay(unsigned long loops)
{
    __asm__ volatile ("1:\n"
        "subs %0, %1, #1\n"
        "bne 1b":"=r" (loops):"0" (loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

#define FLASH_WORD_SIZE unsigned short
#define STATIC_NAND_ENABLE0  0x01fff000

int board_init(void)
{
    DECLARE_GLOBAL_DATA_PTR;

    gd->bd->bi_arch_number = MACH_TYPE_OXNAS;
    gd->bd->bi_boot_params = PHYS_SDRAM_1_PA + 0x100;
    gd->flags = 0;

    icache_enable();

    /* Block reset Static core */
    *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL = (1UL << SYS_CTRL_RSTEN_STATIC_BIT);
    *(volatile u32*)SYS_CTRL_RSTEN_CLR_CTRL = (1UL << SYS_CTRL_RSTEN_STATIC_BIT);

    /* Enable clock to Static core */
    *(volatile u32*)SYS_CTRL_CKEN_SET_CTRL = (1UL << SYS_CTRL_CKEN_STATIC_BIT);

	/* enable flash support on static bus.
     * Enable static bus onto GPIOs, only CS0 */
    *(volatile u32*)SYS_CTRL_SECONDARY_SEL |= 0x01FFF000;

    /* Setup the static bus CS0 to access FLASH */
    *(volatile u32*)STATIC_CONTROL_BANK0 = STATIC_BUS_FLASH_CONFIG;

#ifdef CONFIG_OXNAS_UART1
    {
        unsigned int pins;
        /* Reset UART-1 */
        pins = 1 << SYS_CTRL_RSTEN_UART1_BIT;
       *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL = pins;
       udelay(100);
       *(volatile u32*)SYS_CTRL_RSTEN_CLR_CTRL = pins;
       udelay(100);

        /* Setup pin mux'ing for UART2 */
        pins = (1 << UART_1_SIN_MF_PIN) | (1 << UART_1_SOUT_MF_PIN);
        *(volatile u32*)SYS_CTRL_SECONDARY_SEL &= ~pins;
        *(volatile u32*)SYS_CTRL_TERTIARY_SEL  &= ~pins;
        *(volatile u32*)SYS_CTRL_DEBUG_SEL  &=  ~pins;
        *(volatile u32*)SYS_CTRL_ALTERNATIVE_SEL  |= pins; // Route UART1 SOUT and SIN onto external pins
    }
#endif // CONFIG_OXNAS_UART1

#ifdef CONFIG_OXNAS_UART2
    {
        unsigned int pins;
        /* Block reset UART2 */
        pins = 1 << SYS_CTRL_RSTEN_UART2_BIT;
       *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL = pins;
       *(volatile u32*)SYS_CTRL_RSTEN_CLR_CTRL = pins;
        /* Setup pin mux'ing for UART2 */
        pins = (1 << UART_2_SIN_MF_PIN) | (1 << UART_2_SOUT_MF_PIN);
        *(volatile u32*)SYS_CTRL_SECONDARY_SEL &= ~pins;
        *(volatile u32*)SYS_CTRL_TERTIARY_SEL  &= ~pins;
#ifdef CONFIG_7715
        *(volatile u32*)SYS_CTRL_ALTERNATIVE_SEL  &= ~pins; 
        *(volatile u32*)SYS_CTRL_DEBUG_SEL  |=  pins; // Route UART2 SOUT and SIN onto external pins
#else
        *(volatile u32*)SYS_CTRL_DEBUG_SEL  &=  ~pins;
        *(volatile u32*)SYS_CTRL_ALTERNATIVE_SEL  |= pins; // Route UART2 SOUT and SIN onto external pins
#endif
    }
#endif // CONFIG_OXNAS_UART2

    return 0;
}

int board_late_init()
{
    return 0;
}

int misc_init_r(void)
{
    return 0;
}

int dram_init(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    
#if (PROBE_MEM_SIZE == 1)
#error Automatic memory size detection not yet implemented on the OX82x platform
#else // PROBE_MEM_SIZE
    gd->bd->bi_dram[0].size = MEM_SIZE << 20; /* convert to MBytes */
#endif // PROBE_MEM_SIZE

    gd->bd->bi_dram[0].start = PHYS_SDRAM_1_PA;

	gd->bd->bi_sramstart = CFG_SRAM_BASE;
	gd->bd->bi_sramsize = CFG_SRAM_SIZE;

    return 0;
}

/* OX820 code */
int reset_cpu(void)
{
	printf("Resetting...\n");

    // Assert reset to cores as per power on defaults
	// Don't touch the DDR interface as things will come to an impromptu stop
	// NB Possibly should be asserting reset for PLLB, but there are timing
	//    concerns here according to the docs
    *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL =
        (1UL << SYS_CTRL_RSTEN_COPRO_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_USBHS_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_USBHSPHYA_BIT ) |
        (1UL << SYS_CTRL_RSTEN_MACA_BIT      ) |
        (1UL << SYS_CTRL_RSTEN_PCIEA_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_SGDMA_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_CIPHER_BIT    ) |
        (1UL << SYS_CTRL_RSTEN_SATA_BIT      ) |
        (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT ) |
        (1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT  ) |
        (1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT   ) |
        (1UL << SYS_CTRL_RSTEN_STATIC_BIT    ) |
        (1UL << SYS_CTRL_RSTEN_UART1_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_UART2_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_MISC_BIT      ) |
        (1UL << SYS_CTRL_RSTEN_I2S_BIT       ) |
        (1UL << SYS_CTRL_RSTEN_SD_BIT        ) |
        (1UL << SYS_CTRL_RSTEN_MACB_BIT      ) |
        (1UL << SYS_CTRL_RSTEN_PCIEB_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_VIDEO_BIT     ) |
        (1UL << SYS_CTRL_RSTEN_USBHSPHYB_BIT ) |
        (1UL << SYS_CTRL_RSTEN_USBDEV_BIT    );

    // Release reset to cores as per power on defaults
    *(volatile u32*)SYS_CTRL_RSTEN_CLR_CTRL = (1UL << SYS_CTRL_RSTEN_GPIO_BIT);

    // Disable clocks to cores as per power-on defaults - must leave DDR
	// related clocks enabled otherwise we'll stop rather abruptly.
    *(volatile u32*)SYS_CTRL_CKEN_CLR_CTRL =
        (1UL << SYS_CTRL_CKEN_COPRO_BIT) 	|
        (1UL << SYS_CTRL_CKEN_DMA_BIT)   	|
        (1UL << SYS_CTRL_CKEN_CIPHER_BIT)	|
        (1UL << SYS_CTRL_CKEN_SD_BIT)  		|
        (1UL << SYS_CTRL_CKEN_SATA_BIT)  	|
        (1UL << SYS_CTRL_CKEN_I2S_BIT)   	|
        (1UL << SYS_CTRL_CKEN_USBHS_BIT) 	|
        (1UL << SYS_CTRL_CKEN_MAC_BIT)   	|
        (1UL << SYS_CTRL_CKEN_PCIEA_BIT)   	|
        (1UL << SYS_CTRL_CKEN_STATIC_BIT)	|
        (1UL << SYS_CTRL_CKEN_MACB_BIT)		|
        (1UL << SYS_CTRL_CKEN_PCIEB_BIT)	|
        (1UL << SYS_CTRL_CKEN_REF600_BIT)	|
        (1UL << SYS_CTRL_CKEN_USBDEV_BIT);

    // Enable clocks to cores as per power-on defaults

    // Set sys-control pin mux'ing as per power-on defaults
    *(volatile u32*)SYS_CTRL_SECONDARY_SEL 	 = 0x0UL;
    *(volatile u32*)SYS_CTRL_TERTIARY_SEL 	 = 0x0UL;
    *(volatile u32*)SYS_CTRL_QUATERNARY_SEL  = 0x0UL;
    *(volatile u32*)SYS_CTRL_DEBUG_SEL  	 = 0x0UL;
    *(volatile u32*)SYS_CTRL_ALTERNATIVE_SEL = 0x0UL;
    *(volatile u32*)SYS_CTRL_PULLUP_SEL 	 = 0x0UL;

    *(volatile u32*)SEC_CTRL_SECONDARY_SEL 	 = 0x0UL;
    *(volatile u32*)SEC_CTRL_TERTIARY_SEL 	 = 0x0UL;
    *(volatile u32*)SEC_CTRL_QUATERNARY_SEL  = 0x0UL;
    *(volatile u32*)SEC_CTRL_DEBUG_SEL  	 = 0x0UL;
    *(volatile u32*)SEC_CTRL_ALTERNATIVE_SEL = 0x0UL;
    *(volatile u32*)SEC_CTRL_PULLUP_SEL 	 = 0x0UL;

    // No need to save any state, as the ROM loader can determine whether reset
    // is due to power cycling or programatic action, just hit the (self-
    // clearing) CPU reset bit of the block reset register
    *(volatile u32*)SYS_CTRL_RSTEN_SET_CTRL = 
		(1UL << SYS_CTRL_RSTEN_SCU_BIT) |
		(1UL << SYS_CTRL_RSTEN_ARM0_BIT) |
		(1UL << SYS_CTRL_RSTEN_ARM1_BIT);

	return 0;
}

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#include <linux/mtd/nand.h>


static void nand_hwcontrol(struct mtd_info *mtdinfo, int cmd)
{
	struct nand_chip *this = mtdinfo->priv;
	u32 nand_baseaddr = (u32) CFG_NAND_BASE;

	switch (cmd)
	{
		case NAND_CTL_SETNCE:
		case NAND_CTL_CLRNCE:
		case NAND_CTL_CLRCLE:
		case NAND_CTL_CLRALE:
			break;

		case NAND_CTL_SETCLE:
			nand_baseaddr = CFG_NAND_COMMAND_LATCH;
			break;

		case NAND_CTL_SETALE:
			nand_baseaddr = CFG_NAND_ADDRESS_LATCH;
			break;
	}
	this->IO_ADDR_W = (void __iomem *)(nand_baseaddr);
}

static void nand_write_byte(struct mtd_info *mtdinfo, u_char byte)
{
	struct nand_chip *this = mtdinfo->priv;
	*((volatile u8 *)(this->IO_ADDR_W)) = byte;
}

static u8 nand_read_byte(struct mtd_info *mtdinfo)
{
	u8 ret;

	struct nand_chip *this = mtdinfo->priv;
	ret =  (u8) (*((volatile u8 *)this->IO_ADDR_R));
	return ret;
}

static int nand_dev_ready(struct mtd_info *mtdinfo)
{
	return 1;
}

/*
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];
*/

int board_nand_init(struct nand_chip *nand)
{
 	///*enable static bus to device_select */
	///* enable secondary functions for: gpioa12 .. gpioa 19 (data bus), 
	 //* gpioa20(we) gpioa21(oe), gpioa 22(cs 0), gpiob 0 ..4 (address a0-a4)
	 //*/
	writel(STATIC_NAND_ENABLE0, SYS_CTRL_SECONDARY_SEL);

	///* enable clock and release static block reset */
	writel((1<<SYS_CTRL_CKEN_STATIC_BIT), SYS_CTRL_CKEN_SET_CTRL);
	writel((1<<SYS_CTRL_RSTEN_STATIC_BIT), SYS_CTRL_RSTEN_CLR_CTRL);

	//// enable CS_0
	//*(volatile u32*)SYS_CTRL_SECONDARY_SEL |=  0x00100000;
	//*(volatile u32*)SYS_CTRL_TERTIARY_SEL &= ~0x00100000;
	//*(volatile u32*)SYS_CTRL_DEBUG_SEL  &= ~0x00100000;
	

	// reset NAND unit
	*((volatile u8 *)(CFG_NAND_COMMAND_LATCH)) = 0xff;	// reset command
	udelay(500);

	nand->chip_delay = 100;
	nand->eccmode = NAND_ECC_SOFT;
	nand->hwcontrol = nand_hwcontrol;
	nand->read_byte = nand_read_byte;
	nand->write_byte = nand_write_byte;
	//nand->dev_ready = nand_dev_ready;

	return 0;
	/*
	if (nand_dev_desc[0].ChipID != NAND_ChipID_UNKNOWN) {
		print_size(nand_dev_desc[0].totlen, "\n");
	}
	*/
}

#endif

#ifdef CONFIG_SPI
void spi_init_f (void){
	/***** 
	 * initialise the SD card controller 
	 * No need to get the chip details as this is assumed to be 
	 * SST25vf040b - the only chip I have. JJL
	 */

#ifdef CONFIG_SOFT_SPI
	writel(readl(SYS_CTRL_SECONDARY_SEL)   & ~(SPI_CS | I2C_SCLK | I2C_MOSI | I2C_MISO), SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~(SPI_CS | I2C_SCLK | I2C_MOSI | I2C_MISO), SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~(SPI_CS | I2C_SCLK | I2C_MOSI | I2C_MISO), SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~(SPI_CS | I2C_SCLK | I2C_MOSI | I2C_MISO), SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~(SPI_CS | I2C_SCLK | I2C_MOSI | I2C_MISO), SYS_CTRL_ALTERNATIVE_SEL);
#else
		/* enable clock and release static block reset */
	writel((1<<SYS_CTRL_CKEN_SD_BIT), SYS_CTRL_CKEN_SET_CTRL);
	writel((1<<SYS_CTRL_RSTEN_SD_BIT), SYS_CTRL_RSTEN_CLR_CTRL);


	/* enable access to spi bus devices.
	 * clock, reset and GPIO functions 
	 */
	writel(readl(SYS_CTRL_SECONDARY_SEL) | SD_NOR_ENABLE0, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL) & ~SD_NOR_ENABLE0, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL) & ~SD_NOR_ENABLE0, SYS_CTRL_DEBUG_SEL);

	writel(SD_CLOCK_STOP, SD_CLOCK_CTRL);
	writel(SD_CLOCK_RUN, SD_CLOCK_CTRL);


	/* put sd core into spi mode for port 0 */
	writel(SD_SPI_CONFIG, SD_CONFIG);
	writel(SD_SPI_PORT,   SD_BANK0_CONFIG);
#endif
}

void spi_init_r (void){
}

ssize_t spi_read (uchar *addr, int alen, uchar * buffer, int len){
	int offset;
	int i;

	offset = 0;
	for (i=0;i<alen;i++) {
		offset <<= 8;
		offset |= addr[i];
	}

	
	return 0;
}

ssize_t spi_write (uchar *addr, int alen, uchar *buffer , int len){
	int offset;
	int i;

	offset = 0;
	for (i=0;i<alen;i++) {
		offset <<= 8;
		offset |= addr[i];
	}

	return 0;
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
    return bus== 0 && cs == 0;
}
#endif

#ifdef CONFIG_SOFT_SPI
void spi_cs_activate(struct spi_slave *slave)
{
#if 0
	printf("CS:0\n");
#endif

	writel(SPI_CS, GPIO_1_CLR);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
#if 0
		printf("CS:1\n");
#endif
	writel(SPI_CS, GPIO_1_SET);
}


#endif


