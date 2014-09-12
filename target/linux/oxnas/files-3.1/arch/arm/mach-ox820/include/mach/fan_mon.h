/*
 * arch/arm/plat-oxnas/include/mach/taco_820.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARM_ARCH_TACHO_H
#define __ASM_ARM_ARCH_TACHO_H

#include "mach/hardware.h"

/* Routines ----------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#define TACHO_TARGET_CORE_FREQ_HZ       128000//3840000//512000//128000
//#define TACHO_CORE_TACHO_DIVIDER_VALUE    (((NOMINAL_SYSCLK / TACHO_TARGET_CORE_FREQ_HZ) - 1))
#define TACHO_CORE_TACHO_DIVIDER_VALUE    (((6250000 / TACHO_TARGET_CORE_FREQ_HZ) - 1))

#define SECONDARY_FUNCTION_ENABLE_FAN_PWM 	2
#define SECONDARY_FUNCTION_ENABLE_FAN_TEMP 	0
#define SECONDARY_FUNCTION_ENABLE_FAN_TACHO 	1

#define TEMP_TACHO_PULLUP_CTRL_VALUE 		0x00000001

// 256kHz with 50MHz pclk (reset value)
#define PWM_CORE_CLK_DIVIDER_VALUE      TACHO_CORE_TACHO_DIVIDER_VALUE//(28)

/* Registers ---------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* FAN Speed Counter ----------------------------------- */

#define TACHO_FAN_SPEED_COUNTER                    (FAN_MON_BASE + 0x00)
    // 31:17 - RO - Unused (0x00)
	// 16 	 - R0 - Fan Count Valid - used in one shot mode
	// 15:10 - R0 - Unused
    //  9:0  - RO - Fan counter value. (See DD for conversion to rpm)
#define TACHO_FAN_SPEED_COUNTER_FAN_COUNT       0
#define TACHO_FAN_SPEED_COUNTER_COUNT_VALID 	16

#define TACHO_FAN_SPEED_COUNTER_MASK 	1023

/* Thermistor RC Counter ------------------------------- */
#define TACHO_THERMISTOR_RC_COUNTER                (FAN_MON_BASE + 0x04)
    // 31:10 - RO - Unused (0x00)
    //  9:0  - RO - Thermistor counter value (See DD for conversion to temperature)
#define TACHO_THERMISTOR_RC_COUNTER_THERM_COUNT 0

#define TACHO_THERMISTOR_RC_COUNTER_MASK 	1023


/* Thermistor Control ---------------------------------- */
#define TACHO_THERMISTOR_CONTROL                   (FAN_MON_BASE + 0x08)
    // 31:2  - RO - Unused (0x00)
    //  1:1  - R0 - THERM_COUNT value is valid
    //  0:0  - RW - Set to 1 to enable thermistor PWM output
#define TACHO_THERMISTOR_CONTROL_THERM_VALID    1
#define TACHO_THERMISTOR_CONTROL_THERM_ENABLE   0


/* Clock divider ---------------------------- */
#define TACHO_CLOCK_DIVIDER                        (FAN_MON_BASE + 0x0C)
    // 31:10 - RO - Unused (0x00)
    //  0:9  - RW - set PWM effective clock frequency to a division of pclk (0x030C )
    //         0000 - pclk divided by 1 (=pclk)
    //         0001 - pclk divided by 2
    //         0780 - ~128kHz with 100MHz pclk (reset value)
    //         1023 - pclk divided by 1024
#define TACHO_CLOCK_DIVIDER_PWM_DIVIDER	0
#define TACHO_CLOCK_DIVIDER_MASK 	65535//1023


/* New hardware registers added for 810 */
/* Fan Speed Control ..........................*/
#define TACHO_FAN_SPEED_CONTROL 					(FAN_MON_BASE + 0x10)
	// 31:N+16 - R0 - Unused (0x0000)
	// N+15:16 - RW - Select PWM which controls FAN speed
	// 15:1 - 		  Unused 0
	// 0 	-	 RW - Fan Count mode 0 - Continuous running mode 1 - One shot mode
#define TACHO_FAN_SPEED_CONTROL_PWM_ENABLE_BASE 	16
#define TACHO_FAN_SPEED_CONTROL_PWM_USED 		8
#define TACHO_FAN_SPEED_CONTROL_FAN_COUNT_MODE 	0


/* Fan One Shot Control .........................*/
#define TACHO_FAN_ONE_SHOT_CONTROL 			(FAN_MON_BASE + 0x14)
	// 31:1 - R - Unusedn
	// 0 - W - Start One-shot - Tacho - Self Clearing bit
#define TACHO_FAN_ONE_SHOT_CONTROL_START 		0

/* PWM SECTION ------------------------------ */

// 15:0 R/W 0x00C2
// 31:16 Unused R 0x0000 Unused
#define PWM_CLOCK_DIVIDER (FAN_MON_BASE+0x18)

#endif // __ASM_ARM_ARCH_TACHO_H

/* End oF File */

