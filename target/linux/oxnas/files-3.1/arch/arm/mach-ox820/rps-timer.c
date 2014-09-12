/*
 * arch/arm/mach-ox820/rps-time.c
 *
 * Copyright (C) 2009 Oxford Semiconductor Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/time.h>
#include <asm/smp_twd.h>
#include <asm/mach/time.h>
#include <mach/hardware.h>
#include <mach/rps-timer.h>
#include <mach/rps-irq.h>

static struct clock_event_device ckevt_rps_timer1;

static int oxnas_rps_set_next_event(unsigned long delta, struct clock_event_device* unused)
{
	if (delta == 0)
		return -ETIME;

	/* Stop timers before programming */
    *((volatile unsigned long*)TIMER1_CONTROL) = 0;

    /* Setup timer 1 load value */
    *((volatile unsigned long*)TIMER1_LOAD) = delta;

    /* Setup timer 1 prescaler, periodic operation and start it */
    *((volatile unsigned long*)TIMER1_CONTROL) =
        (TIMER_1_PRESCALE_ENUM << TIMER_PRESCALE_BIT) |
        (TIMER_1_MODE          << TIMER_MODE_BIT)     |
        (TIMER_ENABLE_ENABLE   << TIMER_ENABLE_BIT);

	return 0;
}

static void oxnas_rps_set_mode(enum clock_event_mode mode, struct clock_event_device *dev)
{
    switch(mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		/* Stop timers before programming */
		*((volatile unsigned long*)TIMER1_CONTROL) = 0;

		/* Set period to match HZ */
		*((volatile unsigned long*)TIMER1_LOAD) = TIMER_1_LOAD_VALUE;

		/* Setup prescaler, periodic operation and start it */
        *((volatile unsigned long*)TIMER1_CONTROL) =
            (TIMER_1_PRESCALE_ENUM << TIMER_PRESCALE_BIT) |
            (TIMER_1_MODE          << TIMER_MODE_BIT)     |
            (TIMER_ENABLE_ENABLE   << TIMER_ENABLE_BIT);
		break;
	case CLOCK_EVT_MODE_ONESHOT:
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
        /* Stop timer */
        *((volatile unsigned long*)TIMER1_CONTROL) &=
            ~(TIMER_ENABLE_ENABLE   << TIMER_ENABLE_BIT);
        break;
	}
}

static irqreturn_t OXNAS_RPS_timer_interrupt(int irq, void *dev_id)
{
    /* Clear the timer interrupt - any write will do */
    *((volatile unsigned long*)TIMER1_CLEAR) = 0;

    /* Quick, to the high level handler... */
    if(ckevt_rps_timer1.event_handler) {
        ckevt_rps_timer1.event_handler(&ckevt_rps_timer1);
    }

    return IRQ_HANDLED;
}

static struct irqaction oxnas_timer_irq = {
    .name    = "RPSA timer1",
	.flags	 = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
    .handler = OXNAS_RPS_timer_interrupt
};

static cycle_t ox820_get_cycles(struct clocksource *cs)
{
	cycle_t time = *((volatile unsigned long*)TIMER2_VALUE);
	return ~time;
}


static struct clocksource clocksource_ox820 = {
	.name	= "rps-timer2",
	.rating	= 200,
	.read	= ox820_get_cycles,
	.mask	= CLOCKSOURCE_MASK(24),
	.shift	= 10,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static void __init ox820_clocksource_init(void)
{
	*((volatile unsigned long*)TIMER2_LOAD) = (0xffffff);
	
	/* setup timer 2 as free-running clocksource */
	*((volatile unsigned long*)TIMER2_CONTROL) = 0;
	*((volatile unsigned long*)TIMER2_LOAD) = (0xffffff);

	*((volatile unsigned long*)TIMER2_CONTROL) =
	        (TIMER_PRESCALE_16 << TIMER_PRESCALE_BIT) |
		(TIMER_2_MODE << TIMER_MODE_BIT) |
		(TIMER_ENABLE_ENABLE << TIMER_ENABLE_BIT );

	clocksource_ox820.mult = 
		 clocksource_hz2mult((TIMER_INPUT_CLOCK / (1 << (4*TIMER_PRESCALE_16))), 
			 clocksource_ox820.shift);
/* 	printk("820:start timer 2 clock %dkHz\n", 
		(TIMER_INPUT_CLOCK / (1 << (4*TIMER_PRESCALE_16))));
 */
 	clocksource_register(&clocksource_ox820);
}

void oxnas_init_time(void) {
#ifdef CONFIG_LOCAL_TIMERS
	twd_base = __io_address(OX820_TWD_BASE);
#endif

	ckevt_rps_timer1.name = "RPSA-timer1";
    ckevt_rps_timer1.features = CLOCK_EVT_FEAT_PERIODIC;
    ckevt_rps_timer1.rating = 306;
    ckevt_rps_timer1.shift = 24;
    ckevt_rps_timer1.mult = 
        div_sc(CLOCK_TICK_RATE , NSEC_PER_SEC, ckevt_rps_timer1.shift);
    ckevt_rps_timer1.max_delta_ns = clockevent_delta2ns(0x7fff, &ckevt_rps_timer1);
    ckevt_rps_timer1.min_delta_ns = clockevent_delta2ns(0xf, &ckevt_rps_timer1);
    ckevt_rps_timer1.set_next_event	= oxnas_rps_set_next_event;
    ckevt_rps_timer1.set_mode = oxnas_rps_set_mode;
	ckevt_rps_timer1.cpumask = cpumask_of(0);

    // Connect the timer interrupt handler
    oxnas_timer_irq.handler = OXNAS_RPS_timer_interrupt;
    setup_irq(RPS_TIMER_1_INTERRUPT, &oxnas_timer_irq);

    ox820_clocksource_init();
  	clockevents_register_device(&ckevt_rps_timer1);
}

#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
/*
 * Returns number of microseconds since last clock tick interrupt.
 * Note that interrupts will be disabled when this is called
 * Should take account of any pending timer tick interrupt
 */
static unsigned long oxnas_gettimeoffset(void)
{
	// How long since last timer interrupt?
    unsigned long ticks_since_last_intr =
		(unsigned long)TIMER_1_LOAD_VALUE - *((volatile unsigned long*)TIMER1_VALUE);

    // Is there a timer interrupt pending
    int timer_int_pending = *((volatile unsigned long*)RPSA_IRQ_RAW_STATUS) &
		(1UL << DIRECT_RPS_TIMER_1_INTERRUPT);

    if (timer_int_pending) {
		// Sample time since last timer interrupt again. Theoretical race between
		// interrupt occuring and ARM reading value before reload has taken
		// effect, but in practice it's not going to happen because it takes
		// multiple clock cycles for the ARM to read the timer value register
		unsigned long ticks2 = (unsigned long)TIMER_1_LOAD_VALUE - *((volatile unsigned long*)TIMER1_VALUE);

		// If the timer interrupt which hasn't yet been serviced, and thus has
		// not yet contributed to the tick count, occured before our initial
		// read of the current timer value then we need to account for a whole
		// timer interrupt period
		if (ticks_since_last_intr <= ticks2) {
			// Add on a whole timer interrupt period, as the tick count will have
			// wrapped around since the previously seen timer interrupt (?)
			ticks_since_last_intr += TIMER_1_LOAD_VALUE;
		}
    }

    return TICKS_TO_US(ticks_since_last_intr);
}
#endif // CONFIG_ARCH_USES_GETTIMEOFFSET

struct sys_timer oxnas_timer = {
    .init   = oxnas_init_time,
#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
    //.offset = oxnas_gettimeoffset,
#endif // CONFIG_ARCH_USES_GETTIMEOFFSET
};
