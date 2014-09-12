/*
 * arch/arm/mach-0x820/include/mach/gpio.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ox820_mach_gpio_h
#define _ox820_mach_gpio_h

#include <asm-generic/gpio.h>

static inline void gpio_set_value(unsigned gpio, int value)
{
	__gpio_set_value(gpio, value);
}

/* Returns zero or nonzero; works for gpios configured as inputs OR
 * as outputs, at least for built-in GPIOs.
 */
static inline int gpio_get_value(unsigned gpio)
{
	return __gpio_get_value(gpio);
}

static inline int gpio_cansleep(unsigned gpio)
{
	return __gpio_cansleep(gpio);
}

static inline int gpio_to_irq(unsigned gpio)
{
	return __gpio_to_irq(gpio);
}

static inline int irq_to_gpio(unsigned irq)
{
	/* don't support the reverse mapping */
	return -ENOSYS;
}

#endif
