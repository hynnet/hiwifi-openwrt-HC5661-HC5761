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
 * OX820 - Pogoplug Series 3
 */

#include "leds-gpio-ox820-common.h"

#define DRIVER_NAME "ox820-led-gpio-pogoplugpro"

static struct ox820_gpio_led ox820_leds[] = {
        {
                .led = {
                        .name = "status:misc:blue",
                        .brightness_set = ox820_gpioleds_set
                },
                .gpio = 2,
                .active_low = 0,
                .delayed_switch_to_output = 0
        },
        {
                .led = {
                        .name = "status:health:green",
                        .brightness_set = ox820_gpioleds_set
                },
                .gpio = 49,
                .active_low = 255,
                .delayed_switch_to_output = 0
        },
        {
                .led = {
                        .name = "status:fault:orange",
                        .brightness_set = ox820_gpioleds_set
                },
                .gpio = 48,
                .active_low = 255,
                .delayed_switch_to_output = 0
        },
};

#include "leds-gpio-ox820-common.c"

MODULE_DESCRIPTION("OX820 GPIO-LED PogoPlug Series 3 driver");
MODULE_AUTHOR("Jason Plum");
MODULE_LICENSE("GPL");
