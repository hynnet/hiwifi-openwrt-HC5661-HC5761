/*
 *  arch/arm/mach-ox820/platsmp.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <asm/hardware/gic.h>
#include <asm/cacheflush.h>
#include <asm/localtimer.h>
#include <asm/smp_scu.h>
#include <mach/rps-irq.h>
#include <mach/hardware.h>
#include <asm/tlbflush.h>
#include <asm/cputype.h>

extern void ox820_secondary_startup(void);

static void __iomem *scu_base = __io_address(OX820_ARM11MP_SCU_BASE);

void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	int i;
	printk(KERN_ERR "%s %d\n", __FUNCTION__, __LINE__);
	/*
	 * Initialise the present map, which describes the set of CPUs
	 * actually populated at the present time.
	 */
	
	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	scu_enable(scu_base);
	/*
	 * Enable gic interrupts on the secondary CPU so the interrupt that wakes
	 * it from WFI can be received
	 */
	writel(1, __io_address(OX820_GIC_CPUN_BASE_ADDR(1)));

	/* Write the address that we want the cpu to start at. */
	writel(virt_to_phys(ox820_secondary_startup), HOLDINGPEN_LOCATION);
	smp_wmb();
	writel(1, HOLDINGPEN_CPU);
	smp_wmb();

}

int platform_cpu_disable(unsigned int cpu)
{
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}

static inline unsigned int get_core_count(void)
{
	return scu_get_core_count(scu_base);
}

static DEFINE_SPINLOCK(boot_lock);

volatile int __cpuinitdata pen_release = -1;
static void __cpuinit write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}



void __cpuinit platform_secondary_init(unsigned int cpu)
{
	/*
	 * If any interrupts are already enabled for the primary
	 * core (e.g. timer irq), then they will not have been enabled
	 * for us: do so
	 */
	gic_secondary_init(0);
	
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * Don't know why we need this when realview and omap2 appear not to, but
	 * the secondary CPU doesn't start without it.
	 */
	write_pen_release(cpu);

	gic_raise_softirq(cpumask_of(cpu), 1);

    /* Wake the CPU from WFI */
	/* Give the secondary CPU time to get going */
	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;
	}
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
void __init smp_init_cpus(void)
{
	unsigned int i, ncores = get_core_count();

	/* sanity check */
	if (ncores == 0) {
		printk(KERN_ERR
		       "OX820: strange CM count of 0? Default to 1\n");
		ncores = 1;
	}

	if (ncores > NR_CPUS) {
		printk(KERN_WARNING
		       "OX820: no. of cores (%d) greater than configured "
		       "maximum of %d - clipping\n",
		       ncores, NR_CPUS);
		ncores = NR_CPUS;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}
