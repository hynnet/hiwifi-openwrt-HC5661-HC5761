/*
 * arch/arm/mach-0x820/include/mach/rps-timer.h
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

/* Timer registers */
#define RPS_TIMER_BASE  (RPSA_BASE + 0x200)
#define RPS_TIMER1_BASE (RPS_TIMER_BASE)
#define RPS_TIMER2_BASE (RPS_TIMER_BASE + 0x20)

#define TIMER1_LOAD    (RPS_TIMER1_BASE + 0x0)
#define TIMER1_VALUE   (RPS_TIMER1_BASE + 0x4)
#define TIMER1_CONTROL (RPS_TIMER1_BASE + 0x8)
#define TIMER1_CLEAR   (RPS_TIMER1_BASE + 0xc)

#define TIMER2_LOAD    (RPS_TIMER2_BASE + 0x0)
#define TIMER2_VALUE   (RPS_TIMER2_BASE + 0x4)
#define TIMER2_CONTROL (RPS_TIMER2_BASE + 0x8)
#define TIMER2_CLEAR   (RPS_TIMER2_BASE + 0xc)

#define TIMER_MODE_BIT          6
#define TIMER_MODE_FREE_RUNNING 0
#define TIMER_MODE_PERIODIC     1
#define TIMER_ENABLE_BIT        7
#define TIMER_ENABLE_DISABLE    0
#define TIMER_ENABLE_ENABLE     1

struct rps_hires_time {
    unsigned int time;
};

extern struct rps_hires_time oxnas_rps_get_hires_time(void);
extern unsigned int oxnas_rps_hires_elapsed_time(struct rps_hires_time start);
