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

#include "leds-gpio-ox820-common.h"

#define DRIVER_NAME "ox820-led-gpio-stg212"

static struct ox820_gpio_led ox820_leds[] = {
	{
		.led = {
			.name = "sysled.blue",
			.brightness_set = ox820_gpioleds_set
		},
		.gpio = 37,
		.active_low = 0,
		.delayed_switch_to_output = 1
	},
	{
		.led = {
			.name = "sysled.red",
			.brightness_set = ox820_gpioleds_set
		},
		.gpio = 38,
		.active_low = 1,
		.delayed_switch_to_output = 1
	},
	{
		.led = {
			.name = "copyled",
			.brightness_set = ox820_gpioleds_set
		},
		.gpio = 40,
		.active_low = 1,
		.delayed_switch_to_output = 1
	},
#ifdef CONFIG_LEDS_OX820_STG212_BUZZER
	{
		.led = {
			.name = "buzzer",
			.brightness_set = ox820_gpioleds_set
		},
		.gpio = 47,
		.active_low = 1,
		.delayed_switch_to_output = 1,
        .no_pwm = 1
	}
#endif
};

#include "leds-gpio-ox820-common.c"

MODULE_DESCRIPTION("OX820 GPIO-LED STG212 driver");
MODULE_AUTHOR("Sven Bormann");
MODULE_LICENSE("GPL");
