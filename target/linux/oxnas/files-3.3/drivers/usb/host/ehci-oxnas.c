/*
 * drivers/usb/host/ehci-oxnas.c
 *
 * Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>
//#include <plat/ehci-oxnas.h>

#define rdl(off)	__raw_readl(hcd->regs + (off))
#define wrl(off, val)	__raw_writel((val), hcd->regs + (off))

static int start_oxnas_usb_ehci(struct platform_device *dev)
{
	unsigned long input_polarity = 0;
	unsigned long output_polarity = 0;
	unsigned long power_polarity_default=readl(SYS_CTRL_USBHSMPH_CTRL);
	unsigned usb_hs_ifg;

	if (usb_disabled()) {
		printk(KERN_INFO "usb is disabled\n");
		return -ENODEV;
	}

	/*printk(KERN_INFO "starting usb for 820\n");
	printk("%s: block sizes: qh %Zd qtd %Zd itd %Zd sitd %Zd\n",
		hcd_name,
		sizeof (struct ehci_qh), sizeof (struct ehci_qtd),
		sizeof (struct ehci_itd), sizeof (struct ehci_sitd));

	printk(KERN_INFO "initialise for OX820 series USB\n");*/
#ifdef CONFIG_OXNAS_USB_OVERCURRENT_POLARITY_NEGATIVE
	input_polarity = ((1UL << SYS_CTRL_USBHSMPH_IP_POL_A_BIT) |
					  (1UL << SYS_CTRL_USBHSMPH_IP_POL_B_BIT));
#endif // CONFIG_OXNAS_USB_OVERCURRENT_POLARITY_NEGATIVE

#ifdef CONFIG_OXNAS_USB_POWER_SWITCH_POLARITY_NEGATIVE
	output_polarity = ((1UL << SYS_CTRL_USBHSMPH_OP_POL_A_BIT) |
					   (1UL << SYS_CTRL_USBHSMPH_OP_POL_B_BIT));
#endif // CONFIG_OXNAS_USB_POWER_SWITCH_POLARITY_NEGATIVE

	power_polarity_default &= ~0xf;
	usb_hs_ifg = (power_polarity_default >> 25) & 0x3f;
	usb_hs_ifg += 12;
	power_polarity_default &= ~(0x3f << 25);
	power_polarity_default |= (usb_hs_ifg << 25);
	power_polarity_default |= (input_polarity & 0x3);
	power_polarity_default |= (output_polarity & ( 0x3 <<2));

	writel(power_polarity_default, SYS_CTRL_USBHSMPH_CTRL);
	/*printk(KERN_INFO "usb hsmph ctrl set to:%#lx\n", power_polarity_default);*/

#ifdef CONFIG_OXNAS_USB_PORTA_POWER_CONTROL

#ifdef CONFIG_USB_PORTA_POWO_SECONDARY
	// Select USBA power output from secondary MFP function
	mask = 1UL << USBA_POWO_SEC_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   |  mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable output onto USBA power output secondary function
	writel(mask, GPIO_A_OUTPUT_ENABLE_SET);
#endif // CONFIG_USB_PORTA_POWO_SECONDARY

#ifdef CONFIG_USB_PORTA_POWO_TERTIARY
	// Select USBA power output from tertiary MFP function
	mask = 1UL << (USBA_POWO_TER_MFP - SYS_CTRL_NUM_PINS);
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SEC_CTRL_SECONDARY_SEL)   & ~mask, SEC_CTRL_SECONDARY_SEL);
	writel(readl(SEC_CTRL_TERTIARY_SEL)    |  mask, SEC_CTRL_TERTIARY_SEL);
	writel(readl(SEC_CTRL_QUATERNARY_SEL)  & ~mask, SEC_CTRL_QUATERNARY_SEL);
	writel(readl(SEC_CTRL_DEBUG_SEL)       & ~mask, SEC_CTRL_DEBUG_SEL);
	writel(readl(SEC_CTRL_ALTERNATIVE_SEL) & ~mask, SEC_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable output onto USBA power output tertiary function
	writel(mask, GPIO_B_OUTPUT_ENABLE_SET);
#endif // CONFIG_USB_PORTA_POWO_TERTIARY

#ifdef CONFIG_USB_PORTA_OVERI_SECONDARY
	// Select USBA overcurrent from secondary MFP function
	mask = 1UL << USBA_OVERI_SEC_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   |  mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable input from USBA secondary overcurrent function
	writel(mask, GPIO_A_OUTPUT_ENABLE_CLEAR);
#endif // CONFIG_USB_PORTA_OVERI_SECONDARY

#ifdef CONFIG_USB_PORTA_OVERI_TERTIARY
	// Select USBA overcurrent from tertiary MFP function
	mask = 1UL << (USBA_OVERI_TER_MFP - SYS_CTRL_NUM_PINS);
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SEC_CTRL_SECONDARY_SEL)   & ~mask, SEC_CTRL_SECONDARY_SEL);
	writel(readl(SEC_CTRL_TERTIARY_SEL)    |  mask, SEC_CTRL_TERTIARY_SEL);
	writel(readl(SEC_CTRL_QUATERNARY_SEL)  & ~mask, SEC_CTRL_QUATERNARY_SEL);
	writel(readl(SEC_CTRL_DEBUG_SEL)       & ~mask, SEC_CTRL_DEBUG_SEL);
	writel(readl(SEC_CTRL_ALTERNATIVE_SEL) & ~mask, SEC_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable input from USBA tertiary overcurrent function
	writel(mask, GPIO_B_OUTPUT_ENABLE_CLEAR);
#endif // CONFIG_USB_PORTA_OVERI_TERTIARY

#endif // CONFIG_OXNAS_USB_PORTA_POWER_CONTROL

#ifdef CONFIG_OXNAS_USB_PORTB_POWER_CONTROL

#ifdef CONFIG_USB_PORTB_POWO_SECONDARY
	// Select USBB power output from secondary MFP function
	mask = 1UL << USBB_POWO_SEC_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   |  mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable output onto USBB power output secondary function
	writel(mask, GPIO_A_OUTPUT_ENABLE_SET);
#endif // CONFIG_USB_PORTB_POWO_SECONDARY

#ifdef CONFIG_USB_PORTB_POWO_TERTIARY
	// Select USBB power output from tertiary MFP function
	mask = 1UL << USBB_POWO_TER_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   & ~mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    |  mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable output onto USBB power output tertiary function
	writel(mask, GPIO_A_OUTPUT_ENABLE_SET);
#endif // CONFIG_USB_PORTB_POWO_TERTIARY

#ifdef CONFIG_USB_PORTB_OVERI_SECONDARY
	// Select USBB overcurrent from secondary MFP function
	mask = 1UL << USBB_OVERI_SEC_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   |  mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable input from USBB secondary overcurrent function
	writel(mask, GPIO_A_OUTPUT_ENABLE_CLEAR);
#endif // CONFIG_USB_PORTB_OVERI_SECONDARY

#ifdef CONFIG_USB_PORTB_OVERI_TERTIARY
	// Select USBB overcurrent from tertiary MFP function
	mask = 1UL << USBB_OVERI_TER_MFP;
	spin_lock_irqsave(&oxnas_gpio_spinlock, flags);
	writel(readl(SYS_CTRL_SECONDARY_SEL)   & ~mask, SYS_CTRL_SECONDARY_SEL);
	writel(readl(SYS_CTRL_TERTIARY_SEL)    |  mask, SYS_CTRL_TERTIARY_SEL);
	writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~mask, SYS_CTRL_QUATERNARY_SEL);
	writel(readl(SYS_CTRL_DEBUG_SEL)       & ~mask, SYS_CTRL_DEBUG_SEL);
	writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~mask, SYS_CTRL_ALTERNATIVE_SEL);
	spin_unlock_irqrestore(&oxnas_gpio_spinlock, flags);

	// Enable input from USBB tertiary overcurrent function
	writel(mask, GPIO_A_OUTPUT_ENABLE_CLEAR);
#endif // CONFIG_USB_PORTB_OVERI_TERTIARY

#endif // CONFIG_OXNAS_USB_PORTB_POWER_CONTROL

	// turn on internal 12MHz clock for OX820 architecture USB

	writel(1<<SYS_CTRL_CKEN_REF600_BIT, SYS_CTRL_CKEN_SET_CTRL);
	writel(1<<SYS_CTRL_RSTEN_PLLB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);

#ifdef CONFIG_USB_OX820_FROM_PLLB
	
	writel((readl(SYS_CTRL_SECONDARY_SEL) | (1UL<<9)), SYS_CTRL_SECONDARY_SEL); /* enable monitor output  mf_a9 */
	writel( (1 << PLLB_ENSAT) | (1 << PLLB_OUTDIV) | (2<<PLLB_REFDIV), SEC_CTRL_PLLB_CTRL0); /*  */
	writel( (50 << USB_REF_600_DIVIDER), SEC_CTRL_PLLB_DIV_CTRL);  // 600MHz pllb divider for 12MHz
#endif	   
	
	writel((25 << USB_REF_300_DIVIDER), SYS_CTRL_REF300_DIV); // ref 300 divider for 12MHz
 
	
	// Ensure the USB block is properly reset
	writel(1UL << SYS_CTRL_RSTEN_USBHS_BIT, SYS_CTRL_RSTEN_SET_CTRL);
	wmb();
	writel(1UL << SYS_CTRL_RSTEN_USBHS_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
	writel(1UL << SYS_CTRL_RSTEN_USBPHYA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
	wmb();
	writel(1UL << SYS_CTRL_RSTEN_USBPHYA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
	writel(1UL << SYS_CTRL_RSTEN_USBPHYB_BIT, SYS_CTRL_RSTEN_SET_CTRL);
	wmb();
	writel(1UL << SYS_CTRL_RSTEN_USBPHYB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);

	// Force the high speed clock to be generated all the time, via serial
	// programming of the USB HS PHY
	writel((2UL << SYS_CTRL_USBHSPHY_TEST_ADD) |
		   (0xe0UL << SYS_CTRL_USBHSPHY_TEST_DIN), SYS_CTRL_USBHSPHY_CTRL);

	writel((1UL << SYS_CTRL_USBHSPHY_TEST_CLK) |
		   (2UL << SYS_CTRL_USBHSPHY_TEST_ADD) |
		   (0xe0UL << SYS_CTRL_USBHSPHY_TEST_DIN), SYS_CTRL_USBHSPHY_CTRL);

	writel((0xfUL << SYS_CTRL_USBHSPHY_TEST_ADD) | 
		   (0xaaUL << SYS_CTRL_USBHSPHY_TEST_DIN), SYS_CTRL_USBHSPHY_CTRL);

	writel((1UL << SYS_CTRL_USBHSPHY_TEST_CLK) |
		   (0xfUL << SYS_CTRL_USBHSPHY_TEST_ADD) |
		   (0xaaUL << SYS_CTRL_USBHSPHY_TEST_DIN), SYS_CTRL_USBHSPHY_CTRL);

	/* select the correct clock now out of reset */
#ifdef CONFIG_USB_OX820_FROM_PLLB
	writel( (USB_CLK_INTERNAL<< USB_CLK_SEL) | USB_INT_CLK_PLLB, SYS_CTRL_USB_CTRL); // use pllb clock
#else
	writel( (USB_CLK_INTERNAL<< USB_CLK_SEL) | USB_INT_CLK_REF300, SYS_CTRL_USB_CTRL); // use ref300 derived clock
#endif

#ifdef CONFIG_USB_OX820_PHYA_IS_HOST
	// Configure USB PHYA as a host
	{
		unsigned long reg;
		reg = readl(SYS_CTRL_USB_CTRL);
		reg &= ~(1UL << SYS_CTRL_USB_CTRL_USBAMUX_DEVICE_BIT);
		writel(reg, SYS_CTRL_USB_CTRL); 
	}
#endif

	// Enable the clock to the USB block
	writel(1UL << SYS_CTRL_CKEN_USBHS_BIT, SYS_CTRL_CKEN_SET_CTRL);

	// Ensure reset and clock operations are complete
	wmb();
	
	return 0;
}


static int ehci_oxnas_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd	*ehci;
	int temp;
	int retval;
	
	hcd->has_tt = 1;
	ehci = hcd_to_ehci(hcd);

	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci, readl(&ehci->caps->hc_capbase));
	dbg_hcs_params(ehci, "reset\n");
	dbg_hcc_params(ehci, "reset\n");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = readl(&ehci->caps->hcs_params);

	retval = ehci_halt(ehci);
	if (retval)
			return retval;

	/* data structure init */
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	ehci->has_ppcd = 0;

	if (ehci_is_TDI(ehci))
		ehci_reset(ehci);

	/* at least the Genesys GL880S needs fixup here */
	temp = HCS_N_CC(ehci->hcs_params) * HCS_N_PCC(ehci->hcs_params);
	temp &= 0x0f;
	if (temp && HCS_N_PORTS(ehci->hcs_params) > temp) {
		ehci_dbg(ehci, "bogus port configuration: "
			"cc=%d x pcc=%d < ports=%d\n",
			HCS_N_CC(ehci->hcs_params),
			HCS_N_PCC(ehci->hcs_params),
			HCS_N_PORTS(ehci->hcs_params));
	}


	ehci_port_power(ehci, 0);

	return retval;
}

static const struct hc_driver ehci_oxnas_hc_driver = {
	.description = hcd_name,
	.product_desc = "OXNAS EHCI",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = HCD_MEMORY | HCD_USB2,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_oxnas_setup,
	.start = ehci_run,
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,
	.endpoint_reset = ehci_endpoint_reset,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	.bus_suspend = ehci_bus_suspend,
	.bus_resume = ehci_bus_resume,
	.relinquish_port = ehci_relinquish_port,
	.port_handed_over = ehci_port_handed_over,

	.clear_tt_buffer_complete = ehci_clear_tt_buffer_complete,
};

static int __devinit ehci_oxnas_drv_probe(struct platform_device *pdev)
{
	struct oxnas_ehci_data *pd = pdev->dev.platform_data;
	struct resource *res;
	struct usb_hcd *hcd;
	struct ehci_hcd *ehci;
	void __iomem *regs;
	int irq, err;

	if (usb_disabled())
		return -ENODEV;

	printk("Initializing Oxnas-SoC USB Host Controller\n");

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev,
			"Found HC with no IRQ. Check %s setup!\n",
			dev_name(&pdev->dev));
		err = -ENODEV;
		goto err1;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev,
			"Found HC with no register addr. Check %s setup!\n",
			dev_name(&pdev->dev));
		err = -ENODEV;
		goto err1;
	}

	if (!request_mem_region(res->start, resource_size(res),
				ehci_oxnas_hc_driver.description)) {
		dev_dbg(&pdev->dev, "controller already in use\n");
		err = -EBUSY;
		goto err1;
	}

	regs = ioremap(res->start, resource_size(res));
	if (regs == NULL) {
		dev_dbg(&pdev->dev, "error mapping memory\n");
		err = -EFAULT;
		goto err2;
	}
	start_oxnas_usb_ehci(pdev);

	hcd = usb_create_hcd(&ehci_oxnas_hc_driver,
			&pdev->dev, dev_name(&pdev->dev));
	if (!hcd) {
		err = -ENOMEM;
		goto err3;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = regs;//(void *)(USBHOST_BASE  + 0x100); //regs;

	/*printk(KERN_INFO "@%p Device ID register %lx\n", (void *)USBHOST_BASE, *(unsigned long *)USBHOST_BASE);*/

	hcd->has_tt = 1;
	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs +
		HC_LENGTH(ehci, ehci_readl(ehci, &ehci->caps->hc_capbase));
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);
	ehci->sbrn = 0x20;



	err = usb_add_hcd(hcd, irq, IRQF_SHARED | IRQF_DISABLED);
	if (err)
		goto err4;

	return 0;

err4:
	usb_put_hcd(hcd);
err3:
	iounmap(regs);
err2:
	release_mem_region(res->start, resource_size(res));
err1:
	dev_err(&pdev->dev, "init %s fail, %d\n",
		dev_name(&pdev->dev), err);

	return err;
}

static int __exit ehci_oxnas_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);

	return 0;
}

MODULE_ALIAS("platform:oxnas-ehci");

static struct platform_driver ehci_oxnas_driver = {
	.probe		= ehci_oxnas_drv_probe,
	.remove		= __exit_p(ehci_oxnas_drv_remove),
	.shutdown	= usb_hcd_platform_shutdown,
	.driver.name	= "oxnas-ehci",
};
