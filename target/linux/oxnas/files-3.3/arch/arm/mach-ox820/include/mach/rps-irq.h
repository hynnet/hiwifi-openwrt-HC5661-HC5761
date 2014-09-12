/*
 * arch/arm/mach-0x820/include/mach/rps_irq.h
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

/* Interrupt Controller registers */
#define RPS_IRQ_STATUS     (0x000)
#define RPS_IRQ_RAW_STATUS (0x004)
#define RPS_IRQ_ENABLE     (0x008)
#define RPS_IRQ_DISABLE    (0x00C)
#define RPS_IRQ_SOFT       (0x010)

/* FIQ registers */
#define RPS_FIQ_STATUS     (0x100)
#define RPS_FIQ_RAW_STATUS (0x104)
#define RPS_FIQ_ENABLE     (0x108)
#define RPS_FIQ_DISABLE    (0x10C)
#define RPS_FIQ_IRQ_TO_FIQ (0x1fc)

#define RPSA_IRQ_STATUS     (RPSA_BASE + RPS_IRQ_STATUS    )
#define RPSA_IRQ_RAW_STATUS (RPSA_BASE + RPS_IRQ_RAW_STATUS)
#define RPSA_IRQ_ENABLE     (RPSA_BASE + RPS_IRQ_ENABLE    )
#define RPSA_IRQ_DISABLE    (RPSA_BASE + RPS_IRQ_DISABLE   )
#define RPSA_IRQ_SOFT       (RPSA_BASE + RPS_IRQ_SOFT      )

struct OX820_RPS_chip_data {
	unsigned int irq_offset;
};

extern void OX820_RPS_init_irq(unsigned int start, unsigned int end);
extern void OX820_RPS_cascade_irq(unsigned int irq);
extern void OX820_RPS_trigger_fiq(unsigned int cpu);
extern void OX820_RPS_clear_fiq(unsigned int cpu);
