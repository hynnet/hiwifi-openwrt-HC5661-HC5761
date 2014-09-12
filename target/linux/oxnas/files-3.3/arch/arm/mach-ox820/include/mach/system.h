/*
 * arch/arm/mach-0x820/include/mach/system.h
 *
 * Copyright (C) 2009 Oxford Semiconductor Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/hardware.h>
#include <linux/io.h>

static inline void arch_idle(void)
{
    /*
     * This should do all the clock switching
     * and wait for interrupt tricks
     */
    cpu_do_idle();
}

static inline void arch_reset(char mode, const char* command)
{
    // Assert reset to cores as per power on defaults
	// Don't touch the DDR interface as things will come to an impromptu stop
	// NB Possibly should be asserting reset for PLLB, but there are timing
	//    concerns here according to the docs
    writel((1UL << SYS_CTRL_RSTEN_COPRO_BIT    )|
           (1UL << SYS_CTRL_RSTEN_USBHS_BIT    )|
           (1UL << SYS_CTRL_RSTEN_USBPHYA_BIT  )|
           (1UL << SYS_CTRL_RSTEN_MAC_BIT      )|
           (1UL << SYS_CTRL_RSTEN_PCIEA_BIT    )|
           (1UL << SYS_CTRL_RSTEN_SGDMA_BIT    )|
           (1UL << SYS_CTRL_RSTEN_CIPHER_BIT   )|
           (1UL << SYS_CTRL_RSTEN_SATA_BIT     )|
           (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT)|
           (1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT )|
           (1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT  )|
           (1UL << SYS_CTRL_RSTEN_STATIC_BIT   )|
           (1UL << SYS_CTRL_RSTEN_UART1_BIT    )|
           (1UL << SYS_CTRL_RSTEN_UART2_BIT    )|
           (1UL << SYS_CTRL_RSTEN_MISC_BIT     )|
           (1UL << SYS_CTRL_RSTEN_I2S_BIT      )|
           (1UL << SYS_CTRL_RSTEN_SD_BIT       )|
           (1UL << SYS_CTRL_RSTEN_MAC_2_BIT    )|
           (1UL << SYS_CTRL_RSTEN_PCIEB_BIT    )|
           (1UL << SYS_CTRL_RSTEN_VIDEO_BIT    )|
           (1UL << SYS_CTRL_RSTEN_USBPHYB_BIT  )|
           (1UL << SYS_CTRL_RSTEN_USBDEV_BIT), SYS_CTRL_RSTEN_SET_CTRL);

    // Release reset to cores as per power on defaults
    writel((1UL << SYS_CTRL_RSTEN_GPIO_BIT), SYS_CTRL_RSTEN_CLR_CTRL);

    // Disable clocks to cores as per power-on defaults - must leave DDR
	// related clocks enabled otherwise we'll stop rather abruptly.
    writel((1UL << SYS_CTRL_CKEN_COPRO_BIT  )|
           (1UL << SYS_CTRL_CKEN_DMA_BIT    )|
           (1UL << SYS_CTRL_CKEN_CIPHER_BIT )|
           (1UL << SYS_CTRL_CKEN_SD_BIT     )|
           (1UL << SYS_CTRL_CKEN_SATA_BIT   )|
           (1UL << SYS_CTRL_CKEN_I2S_BIT    )|
           (1UL << SYS_CTRL_CKEN_USBHS_BIT  )|
           (1UL << SYS_CTRL_CKEN_MAC_BIT    )|
           (1UL << SYS_CTRL_CKEN_PCIEA_BIT  )|
           (1UL << SYS_CTRL_CKEN_STATIC_BIT )|
           (1UL << SYS_CTRL_CKEN_MAC_2_BIT  )|
           (1UL << SYS_CTRL_CKEN_PCIEB_BIT  )|
           (1UL << SYS_CTRL_CKEN_REF600_BIT )|
           (1UL << SYS_CTRL_CKEN_USBDEV_BIT), SYS_CTRL_CKEN_CLR_CTRL);

    // Enable clocks to cores as per power-on defaults

    // Set sys-control pin mux'ing as per power-on defaults
    writel(0UL, SYS_CTRL_SECONDARY_SEL);
    writel(0UL, SYS_CTRL_TERTIARY_SEL);
    writel(0UL, SYS_CTRL_QUATERNARY_SEL);
    writel(0UL, SYS_CTRL_DEBUG_SEL);
    writel(0UL, SYS_CTRL_ALTERNATIVE_SEL);
    writel(0UL, SYS_CTRL_PULLUP_SEL);

    writel(0UL, SEC_CTRL_SECONDARY_SEL);
    writel(0UL, SEC_CTRL_TERTIARY_SEL);
    writel(0UL, SEC_CTRL_QUATERNARY_SEL);
    writel(0UL, SEC_CTRL_DEBUG_SEL);
    writel(0UL, SEC_CTRL_ALTERNATIVE_SEL);
    writel(0UL, SEC_CTRL_PULLUP_SEL);

    // No need to save any state, as the ROM loader can determine whether reset
    // is due to power cycling or programatic action, just hit the (self-
    // clearing) CPU reset bit of the block reset register
    writel((1UL << SYS_CTRL_RSTEN_SCU_BIT)  |
		   (1UL << SYS_CTRL_RSTEN_ARM0_BIT) |
		   (1UL << SYS_CTRL_RSTEN_ARM1_BIT), SYS_CTRL_RSTEN_SET_CTRL);
}

#endif // __ASM_ARCH_SYSTEM_H

