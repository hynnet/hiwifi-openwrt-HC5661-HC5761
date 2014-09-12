/*
 * Linux Broadcom BCM47xx GPIO char driver
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * $Id: linux_gpio.c 342502 2012-07-03 03:08:12Z $
 *
 */
#include <linux/kernel.h> 
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/platform_device.h> 
#include <linux/leds.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>

#include <typedefs.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmdevs.h>

#include <linux_gpio.h>

#define BCM_GPIO_LED_ON		1
#define BCM_GPIO_LED_OFF	0

#define BCM_GPIO_WLAN2G_LED_PIN	4		/* 2.4G led */
#define BCM_GPIO_WLAN5G_LED_PIN	5		/* 5G led */
#define BCM_GPIO_INTERNET_LED_PIN	6	/* internet led */
#define BCM_GPIO_SYS_LED_PIN		7	/* system led */
#define BCM_GPIO_RESET_BTN_PIN		11	/* reset button */

#define BCM_KEYS_POLL_INTERVAL		20 /* msecs */
#define BCM_KEYS_DEBOUNCE_INTERVAL	(3 * BCM_KEYS_POLL_INTERVAL)

static unsigned long bcm_gpio_count = ARCH_NR_GPIOS - 1;
static DEFINE_SPINLOCK(bcm_gpio_lock);

static struct gpio_led bcm_leds_gpio[] __initdata = {
	{
		.name = "bcm:green:system",
		.gpio = BCM_GPIO_SYS_LED_PIN,
		.active_low = 0,
	}, {
		.name = "bcm:green:internet",
		.gpio = BCM_GPIO_INTERNET_LED_PIN,
		.active_low	= 0,
	},
	{
		.name = "bcm:green:wlan2g",
		.gpio = BCM_GPIO_WLAN2G_LED_PIN,
		.active_low = 0,
	}, {
		.name = "bcm:green:wlan5g",
		.gpio = BCM_GPIO_WLAN5G_LED_PIN,
		.active_low	= 0,
	}
};

static struct gpio_keys_button bcm_gpio_keys[] __initdata = {
	{
		.desc = "reset",
		.type = EV_KEY,
		.code = KEY_RESTART,
		.debounce_interval = BCM_KEYS_DEBOUNCE_INTERVAL,
		.gpio = BCM_GPIO_RESET_BTN_PIN,
		.active_low = 1,
	}
};

/* handle to the sb */
static si_t *gpio_sih;

static int __bcm_gpio_get_value(unsigned gpio)
{
	return (si_gpioin(gpio_sih) >> gpio & 1);
}

static int bcm_gpio_get_value(struct gpio_chip *chip, unsigned offset)
{
	return __bcm_gpio_get_value(offset);
}

static void __bcm_gpio_set_value(unsigned gpio, int value)
{
	unsigned gpio_mask = 1 << gpio;
	unsigned value_mask;

	if (value)
	{
		value_mask = BCM_GPIO_LED_ON << gpio;
	}
	else
	{
		value_mask = BCM_GPIO_LED_OFF << gpio;
	}

	si_gpioout(gpio_sih, gpio_mask, value_mask, GPIO_HI_PRIORITY);	
}

static void bcm_gpio_set_value(struct gpio_chip *chip, unsigned offset, int value)
{
	__bcm_gpio_set_value(offset, value);
}

static void bcm_gpio_output_enable(unsigned gpio)
{
	unsigned mask = 1 << gpio;

	si_gpioreserve(gpio_sih, mask, GPIO_HI_PRIORITY);
	si_gpioouten(gpio_sih, mask, mask, GPIO_HI_PRIORITY);
}

static void bcm_gpio_input_enable(unsigned gpio)
{
	unsigned mask = 1 << gpio;

	si_gpioreserve(gpio_sih, mask, GPIO_HI_PRIORITY);
	si_gpioouten(gpio_sih, mask, 0, GPIO_HI_PRIORITY);
}

static int bcm_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	unsigned long flags;

	spin_lock_irqsave(&bcm_gpio_lock, flags);
	bcm_gpio_input_enable(offset);
	spin_unlock_irqrestore(&bcm_gpio_lock, flags);

	return 0;
}

static int bcm_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
					int value)
{
	unsigned long flags;

	spin_lock_irqsave(&bcm_gpio_lock, flags);
	bcm_gpio_output_enable(offset);
	spin_unlock_irqrestore(&bcm_gpio_lock, flags);

	return 0;
}

static struct gpio_chip bcm_gpio_chip = {
	.label			= "bcm470x",
	.get			= bcm_gpio_get_value,
	.set			= bcm_gpio_set_value,
	.direction_input	= bcm_gpio_direction_input,
	.direction_output	= bcm_gpio_direction_output,
	.base			= 0,
};

static void __init bcm_register_leds_gpio(int id,
								unsigned num_leds,
								struct gpio_led *leds)
{
	struct platform_device *pdev;
	struct gpio_led_platform_data pdata;
	struct gpio_led *p;
	int err;

	p = kmalloc(num_leds * sizeof(*p), GFP_KERNEL);
	if (!p)
		return;

	memcpy(p, leds, num_leds * sizeof(*p));

	pdev = platform_device_alloc("leds-gpio", id);
	if (!pdev)
		goto err_free_leds;

	memset(&pdata, 0, sizeof(pdata));
	pdata.num_leds = num_leds;
	pdata.leds = p;

	err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
	if (err)
		goto err_put_pdev;

	err = platform_device_add(pdev);
	if (err)
		goto err_put_pdev;

	return;

err_put_pdev:
	platform_device_put(pdev);

err_free_leds:
	kfree(p);
}

static void __init bcm_register_gpio_keys_polled(int id,
										unsigned poll_interval,
										unsigned nbuttons,
										struct gpio_keys_button *buttons)
{
	struct platform_device *pdev;
	struct gpio_keys_platform_data pdata;
	struct gpio_keys_button *p;
	int err;

	p = kmalloc(nbuttons * sizeof(*p), GFP_KERNEL);
	if (!p)
		return;

	memcpy(p, buttons, nbuttons * sizeof(*p));

	pdev = platform_device_alloc("gpio-keys-polled", id);
	if (!pdev)
		goto err_free_buttons;

	memset(&pdata, 0, sizeof(pdata));
	pdata.poll_interval = poll_interval;
	pdata.nbuttons = nbuttons;
	pdata.buttons = p;

	err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
	if (err)
		goto err_put_pdev;

	err = platform_device_add(pdev);
	if (err)
		goto err_put_pdev;

	return;

err_put_pdev:
	platform_device_put(pdev);

err_free_buttons:
	kfree(p);
}

/*
 * for gpio lib to control gpio
 */
int gpio_get_value(unsigned gpio)
{
	if (gpio < bcm_gpio_count)
		return __bcm_gpio_get_value(gpio);

	return __gpio_get_value(gpio);
}
EXPORT_SYMBOL(gpio_get_value);

void gpio_set_value(unsigned gpio, int value)
{
	if (gpio < bcm_gpio_count)
		return __bcm_gpio_set_value(gpio, value);

	return __gpio_set_value(gpio, value);
}
EXPORT_SYMBOL(gpio_set_value);

static int __init gpio_init(void)
{
	int err;

	if (!(gpio_sih = si_kattach(SI_OSH)))
		return -ENODEV;

	bcm_gpio_chip.ngpio = bcm_gpio_count;
	err = gpiochip_add(&bcm_gpio_chip);

	if (err)
	{
		printk(KERN_ERR "bcm4708 add gpio chip failed\n");
		return err;
	}
	
	bcm_register_leds_gpio(-1, ARRAY_SIZE(bcm_leds_gpio), bcm_leds_gpio);
	bcm_register_gpio_keys_polled(-1, BCM_KEYS_POLL_INTERVAL,
								ARRAY_SIZE(bcm_gpio_keys),
								bcm_gpio_keys);

	return 0;
}

static void __exit gpio_exit(void)
{
	if (gpio_sih)
	{
		si_detach(gpio_sih);
	}
}

module_init(gpio_init);
module_exit(gpio_exit);
