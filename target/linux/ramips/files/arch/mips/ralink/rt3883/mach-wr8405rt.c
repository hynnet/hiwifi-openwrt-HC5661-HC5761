/*
 *     ZBT WR8405RT board support
 *
 *  Copyright (C) 2011-2013 lintel<lintel.huang@gmail.com>
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/rtl8367.h>
#include <linux/ethtool.h>
#include <linux/pci.h>
#include <linux/rt2x00_platform.h>

#include <asm/mach-ralink/machine.h>
#include <asm/mach-ralink/dev-gpio-buttons.h>
#include <asm/mach-ralink/dev-gpio-leds.h>
#include <asm/mach-ralink/rt3883.h>
#include <asm/mach-ralink/rt3883_regs.h>
#include <asm/mach-ralink/ramips_eth_platform.h>

#include "devices.h"

#define WR8405RT_GPIO_LED_POWER		0
#define WR8405RT_GPIO_LED_LAN		19
#define WR8405RT_GPIO_LED_USB		24
#define WR8405RT_GPIO_LED_WAN		27
#define WR8405RT_GPIO_BUTTON_RESET	13
#define WR8405RT_GPIO_BUTTON_WPS		26

#define WR8405RT_GPIO_RTL8367_SCK	2
#define WR8405RT_GPIO_RTL8367_SDA	1

#define WR8405RT_KEYS_POLL_INTERVAL	20
#define WR8405RT_KEYS_DEBOUNCE_INTERVAL	(3 * WR8405RT_KEYS_POLL_INTERVAL)

static struct gpio_led wr8405rt_leds_gpio[] __initdata = {
	{
		.name		= "zbt:blue:power",
		.gpio		= WR8405RT_GPIO_LED_POWER,
		.active_low	= 1,
	},
	{
		.name		= "zbt:blue:lan",
		.gpio		= WR8405RT_GPIO_LED_LAN,
		.active_low	= 1,
	},
	{
		.name		= "zbt:blue:wan",
		.gpio		= WR8405RT_GPIO_LED_WAN,
		.active_low	= 1,
	},
	{
		.name		= "zbt:blue:usb",
		.gpio		= WR8405RT_GPIO_LED_USB,
		.active_low	= 1,
	},
};

static struct gpio_keys_button wr8405rt_gpio_buttons[] __initdata = {
	{
		.desc		= "reset",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = WR8405RT_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= WR8405RT_GPIO_BUTTON_RESET,
		.active_low	= 1,
	},
	{
		.desc		= "wps",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = WR8405RT_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= WR8405RT_GPIO_BUTTON_WPS,
		.active_low	= 1,
	}
};

static struct rtl8367_extif_config wr8405rt_rtl8367_extif1_cfg = {
	.txdelay = 1,
	.rxdelay = 0,
	.mode = RTL8367_EXTIF_MODE_RGMII,
	.ability = {
		.force_mode = 1,
		.txpause = 1,
		.rxpause = 1,
		.link = 1,
		.duplex = 1,
		.speed = RTL8367_PORT_SPEED_1000,
	}
};

static struct rtl8367_platform_data wr8405rt_rtl8367_data = {
	.gpio_sda	= WR8405RT_GPIO_RTL8367_SDA,
	.gpio_sck	= WR8405RT_GPIO_RTL8367_SCK,
	.extif1_cfg	= &wr8405rt_rtl8367_extif1_cfg,
};

static struct platform_device wr8405rt_rtl8367_device = {
	.name		= RTL8367_DRIVER_NAME,
	.id		= -1,
	.dev = {
		.platform_data	= &wr8405rt_rtl8367_data,
	}
};

static struct rt2x00_platform_data wr8405rt_pci_wlan_data = {
	.eeprom_file_name	= "rt2x00pci_1_0.eeprom",
};

static int wr8405rt_pci_plat_dev_init(struct pci_dev *dev)
{
	if (dev->bus->number == 1 && PCI_SLOT(dev->devfn) == 0)
		dev->dev.platform_data = &wr8405rt_pci_wlan_data;

	return 0;
}

static void __init wr8405rt_init(void)
{
	rt3883_gpio_init(RT3883_GPIO_MODE_I2C |
			 RT3883_GPIO_MODE_UART0(RT3883_GPIO_MODE_GPIO) |
			 RT3883_GPIO_MODE_JTAG |
			 RT3883_GPIO_MODE_PCI(RT3883_GPIO_MODE_PCI_FNC));

	rt3883_register_pflash(0);

	ramips_register_gpio_leds(-1, ARRAY_SIZE(wr8405rt_leds_gpio),
				  wr8405rt_leds_gpio);

	ramips_register_gpio_buttons(-1, WR8405RT_KEYS_POLL_INTERVAL,
				     ARRAY_SIZE(wr8405rt_gpio_buttons),
				     wr8405rt_gpio_buttons);

	platform_device_register(&wr8405rt_rtl8367_device);

	rt3883_wlan_data.disable_2ghz = 1;
	rt3883_register_wlan();

	rt3883_eth_data.speed = SPEED_1000;
	rt3883_eth_data.duplex = DUPLEX_FULL;
	rt3883_eth_data.tx_fc = 1;
	rt3883_eth_data.rx_fc = 1;
	rt3883_register_ethernet();

	rt3883_register_wdt(false);
	rt3883_register_usbhost();
	rt3883_pci_set_plat_dev_init(wr8405rt_pci_plat_dev_init);
	rt3883_pci_init(RT3883_PCI_MODE_PCIE);
}

MIPS_MACHINE(RAMIPS_MACH_WR8405RT, "WR8405RT", "ZBT WR8405RT", wr8405rt_init);
