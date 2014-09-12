
#include <common.h>
#include <command.h>

#if (CONFIG_COMMANDS & CFG_CMD_LEDFAIL)

#define GREEN_LED (1 << (49-32)) //use GPIO 49 as and led indicator
#define ORANGE_LED (1 << (48-32)) //use GPIO 48 as and led indicator

#define GPIO_A	0x44000000
#define GPIO_B	0x44100000
#define WARN_GPIO_OUT_REG			(GPIO_B + 0x10)
#define WARN_GPIO_OUT_ENABLE_SET	(GPIO_B + 0x1C)
#define WARN_GPIO_OUT_ENABLE_CLR	(GPIO_B + 0x20)

void ledfail_light(void)
{
	printf("Turn on Green LED.\n");
	u_int32_t led_state = *((volatile u_int32_t *)WARN_GPIO_OUT_REG);
	/* off orange  */
	/* Extinguish the failure LED - assumes active low drive */
	led_state = led_state | ORANGE_LED;
	*((volatile u_int32_t *)WARN_GPIO_OUT_REG) = led_state;

    /* Clear the failure bit output enable in GPIO's */
	*((volatile u_int32_t *)WARN_GPIO_OUT_ENABLE_CLR) = ORANGE_LED;
	
	/* on geeen */
	led_state = *((volatile u_int32_t *)WARN_GPIO_OUT_REG);
	/* Light the failure LED - assumes active low drive */
	led_state = led_state & ~GREEN_LED;
	*((volatile u_int32_t *)WARN_GPIO_OUT_REG) = led_state;

	/* Enable GPIO for output */
	*((volatile u_int32_t *)WARN_GPIO_OUT_ENABLE_SET) = GREEN_LED;
}

void ledfail_extinguish(void)
{
  
	printf("Turn on Orange LED.\n");
	u_int32_t led_state = *((volatile u_int32_t *)WARN_GPIO_OUT_REG);
	/* off green */
	/* Extinguish the failure LED - assumes active low drive */

	led_state = led_state | GREEN_LED;
	*((volatile u_int32_t *)WARN_GPIO_OUT_REG) = led_state;

    /* Clear the failure bit output enable in GPIO's */
	*((volatile u_int32_t *)WARN_GPIO_OUT_ENABLE_CLR) = GREEN_LED;
	
	/* on orange */
	/* Light the failure LED - assumes active low drive */
	led_state = *((volatile u_int32_t *)WARN_GPIO_OUT_REG);

	led_state = led_state & ~ORANGE_LED;
	*((volatile u_int32_t *)WARN_GPIO_OUT_REG) = led_state;

	/* Enable GPIO for output */
	*((volatile u_int32_t *)WARN_GPIO_OUT_ENABLE_SET) = ORANGE_LED;
}

int do_ledfail(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (argc != 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	ulong arg = simple_strtoul(argv[1], NULL, 10);
	switch (arg) {
		case 0:
			ledfail_extinguish();
			break;
		case 1:
			ledfail_light();
			break;
	}

	return 0;
}

U_BOOT_CMD(ledfail, 2, 2, do_ledfail, "ledfail - Extinguish (0) or light (1) failure LED\n", NULL);
#endif	/* CFG_CMD_LEDFAIL */
