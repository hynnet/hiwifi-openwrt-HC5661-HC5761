/*
 * OX820 chip GPIO driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Supports:
 * OX820
 */

static void ox820_program_pwm(struct ox820_gpio_led* led_dat, 
						unsigned brightness)
{
	if(led_dat->active_low) {
		brightness = 255 - brightness;
		if(led_dat->gpio < SYS_CTRL_NUM_PINS) {
			writel(brightness, GPIO_A_PWM_VALUE_0 + led_dat->gpio * 8);
			writel(0, GPIO_A_RR_0 + led_dat->gpio * 8);
		} else {
			writel(brightness, GPIO_B_PWM_VALUE_0 + (led_dat->gpio - SYS_CTRL_NUM_PINS) * 8);
			writel(0, GPIO_B_RR_0 + (led_dat->gpio - SYS_CTRL_NUM_PINS) * 8);
		}
	} else {
		if(led_dat->gpio < SYS_CTRL_NUM_PINS) {
			writel(brightness, GPIO_A_PWM_VALUE_0 + led_dat->gpio * 8);
			writel(0, GPIO_A_RR_0 + led_dat->gpio * 8);
		} else {
			writel(brightness, GPIO_B_PWM_VALUE_0 + (led_dat->gpio - SYS_CTRL_NUM_PINS) * 8);
			writel(0, GPIO_B_RR_0 + (led_dat->gpio - SYS_CTRL_NUM_PINS) * 8);
		}
	}
}

static void ox820_gpioleds_set(struct led_classdev* led_cdev,
						enum led_brightness value)
{
	struct ox820_gpio_led* led_dat = 
		container_of(led_cdev, struct ox820_gpio_led, led);
	int level;
	if(value == LED_OFF) {
		level = 0;
	} else {
		level = 1;
	}
	if(value > 255) {
		value = 255;
	}
	if(led_dat->active_low) {
		level = !level;
	}
	
	if(led_dat->delayed_switch_to_output && value != LED_OFF) {
		gpio_direction_output(led_dat->gpio, led_dat->active_low);
		led_dat->delayed_switch_to_output = 0;
	}
	
	if(led_dat->no_pwm) {
		gpio_set_value(led_dat->gpio, level);
	} else {
		ox820_program_pwm(led_dat, value);
	}
}

static int __devinit ox820_gpioleds_probe(struct platform_device* pdev)
{
	int ret = 0;
	
	return ret;
}

static int __devexit ox820_gpioleds_remove(struct platform_device* pdev)
{
	int ret = 0;
	return ret;
}

static struct platform_driver ox820_gpioleds_driver = {
	.driver = {
		.name = DRIVER_NAME,
	},
	.probe = ox820_gpioleds_probe,
	.remove = ox820_gpioleds_remove
};

static void ox820_gpioleds_release(struct device* dev)
{
}

static struct platform_device ox820_gpioleds_dev = 
{
	.name = DRIVER_NAME,
	.id = 0,
	.num_resources = 0,
	.resource  = NULL,
	.dev.coherent_dma_mask = 0,
	.dev.release = ox820_gpioleds_release
}; 

static int __init ox820_gpioleds_platform_init(void)
{
	int ret, idx;

	ret = platform_driver_register(&ox820_gpioleds_driver);
	if(0 == ret) {
		ret = platform_device_register(&ox820_gpioleds_dev);
		if(0 != ret) {
			platform_driver_unregister(&ox820_gpioleds_driver);
		}
	}
	
	if(0 == ret) {
		ox820_printk(KERN_INFO"ox820_led_gpio: requesting GPIOs\n");
		for(idx = 0; idx < sizeof(ox820_leds) / sizeof(ox820_leds[0]); ++idx) {
			ret = gpio_request(ox820_leds[idx].gpio, ox820_leds[idx].led.name);
			if(0 == ret) {
				if(!ox820_leds[idx].delayed_switch_to_output) {
					ret = gpio_direction_output(ox820_leds[idx].gpio, ox820_leds[idx].active_low);
					if(0 != ret) {
						gpio_free(ox820_leds[idx].gpio);
					} else if(!ox820_leds[idx].no_pwm) {
						ox820_program_pwm(&ox820_leds[idx], 0);
					}
				}
			}
			if (0 != ret) {
				break;
			}
		}
		
		if(0 != ret) {
			while(idx-- != 0) {
				gpio_free(ox820_leds[idx].gpio);
			}
			platform_device_unregister(&ox820_gpioleds_dev);
			platform_driver_unregister(&ox820_gpioleds_driver);
		}
	}
	
	if(0 == ret) {
		ox820_printk(KERN_INFO"ox820_led_gpio: registering LEDs\n");
		for(idx = 0; idx < sizeof(ox820_leds) / sizeof(ox820_leds[0]); ++idx) {
			ret = led_classdev_register(&ox820_gpioleds_dev.dev, &ox820_leds[idx].led);
			if(0 != ret) {
				break;
			}
		}
		
		if(0 != ret) {
			while(idx-- != 0) {
				led_classdev_unregister(&ox820_leds[idx].led);
			}
			for(idx = 0; idx < sizeof(ox820_leds) / sizeof(ox820_leds[0]); ++idx) {
				gpio_free(ox820_leds[idx].gpio);
			}
			platform_device_unregister(&ox820_gpioleds_dev);
			platform_driver_unregister(&ox820_gpioleds_driver);
		}
	}
	
	if(0 != ret) {
		printk(KERN_ERR"ox820_led_gpio: initialization result %d\n", ret);
	} else {	
		printk(KERN_INFO"ox820_led_gpio: initialized\n");
	}
	
	return ret;
}

static void __exit ox820_gpioleds_platform_exit(void)
{
	int idx;
	for(idx = 0; idx < sizeof(ox820_leds) / sizeof(ox820_leds[0]); ++idx) {
		gpio_direction_input(ox820_leds[idx].gpio);
		led_classdev_unregister(&ox820_leds[idx].led);
		gpio_free(ox820_leds[idx].gpio);
	}
	platform_device_unregister(&ox820_gpioleds_dev);
	platform_driver_unregister(&ox820_gpioleds_driver);
}

module_init(ox820_gpioleds_platform_init);
module_exit(ox820_gpioleds_platform_exit);
