#ifndef __ASM_MACH_BRCM_GPIO_H
#define __ASM_MACH_BRCM_GPIO_H

#ifndef ARCH_NR_GPIOS
#define ARCH_NR_GPIOS	17
#endif

#define __ARM_GPIOLIB_COMPLEX

#include <asm-generic/gpio.h>

int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int value);

static inline int irq_to_gpio(unsigned gpio)
{
	return -EINVAL;
}

static inline int gpio_to_irq(unsigned gpio)
{
	return -EINVAL;
}
 
#define gpio_to_irq	gpio_to_irq
#define gpio_cansleep __gpio_cansleep

#endif	/* __ASM_MACH_BRCM_GPIO_H */

