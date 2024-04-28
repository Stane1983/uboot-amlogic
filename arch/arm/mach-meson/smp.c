// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/system.h>
#include <asm/global_data.h>
#include <asm/arch-meson/smp.h>

DECLARE_GLOBAL_DATA_PTR;

unsigned int __cpu_online_mask;
unsigned int __num_online_cpus;
struct smp_boot_param smp_boot[NR_CPUS - 1];

void set_cpu_online(unsigned int cpu, bool online)
{
	/*
	 * atomic_inc/dec() is required to handle the horrid abuse of this
	 * function by the reboot and kexec code which invoke it from
	 * IPI/NMI broadcasts when shutting down CPUs. Invocation from
	 * regular CPU hotplug is properly serialized.
	 *
	 * Note, that the fact that __num_online_cpus is of type atomic_t
	 * does not protect readers which are not serialized against
	 * concurrent hotplug operations.
	 */
	if (online) {
		__cpu_online_mask |= (1 << cpu);
		__num_online_cpus++;

	} else {
		__cpu_online_mask &= ~(1 << cpu);
		__num_online_cpus--;
	}
}

unsigned int cpu_online(unsigned int cpu)
{
	return (__cpu_online_mask >> cpu) & 0x1;
}

unsigned int cpu_online_status(void)
{
	return __cpu_online_mask;
}

/*
 * Activate the first processor.
 */
void  boot_cpu_init(void)
{
	/* Mark the boot cpu "present", "online" etc for SMP and UP case */
	set_cpu_online(0, true);
	flush_dcache_all();
}

int cpu_up(unsigned int cpu, unsigned int entrypoint)
{
	return psci_cpu_on(cpu, entrypoint);
}

void cpu_off(void)
{
	psci_cpu_off();
}

static int secondary_boot(unsigned int cpu, unsigned int entrypoint)
{
	int ret = -1;

		/* Skip online CPUs and CPUs on offline nodes */
	if (cpu_online(cpu))
		return 0;

	boot_cpu_init();

	ret = cpu_up(cpu, entrypoint);
	if (ret == 0) {
		udelay(1000);
		__asm__ volatile("sev");
		udelay(100);
		if (cpu_online(cpu)) {
			printf("CPU%u: come online\n", cpu);
		} else {
			printf("CPU%u: failed to come online\n", cpu);
			ret = -1;
		}
	} else {
		printf("CPU%u: failed to boot\n", cpu);
	}
	return ret;
}

unsigned int get_core_id(void)
{
	unsigned long mpidr = read_mpidr();
	unsigned int cpuid;

	if (mpidr & (1 << 24))
		mpidr = mpidr >> 8;
	cpuid = mpidr & 0xff;
	mpidr = (mpidr >> 8) & 0xff;
	if (mpidr)              /* cluster 1 */
		cpuid = cpuid + 4;  /* meson soc cluster 0 has 4 cores */

	return cpuid;
}

void secondary_off(void)
{
	unsigned long mpidr = read_mpidr();
	unsigned int cpu;

	if (mpidr & (1 << 24))
		mpidr = mpidr >> 8;
	cpu = mpidr & 0xff;
	mpidr = (mpidr >> 8) & 0xff;
	if (mpidr)              /* cluster 1 */
		cpu = cpu + 4;     /* meson soc cluster 0 has 4 cores*/

	dcache_disable();
	set_cpu_online(cpu, 0);
	cpu_off();
}

void secondary_start(void)
{
	unsigned long mpidr = read_mpidr();
	unsigned int cpu;

	enable_caches();
	if (mpidr & (1 << 24))
		mpidr = mpidr >> 8;
	cpu = mpidr & 0xff;
	mpidr = (mpidr >> 8) & 0xff;
	if (mpidr)              /* cluster 1 */
		cpu = cpu + 4;     /* meson soc cluster 0 has 4 cores*/
	set_cpu_online(cpu, true);

	if (smp_boot[cpu].bootup)
		smp_boot[cpu].bootup(smp_boot[cpu].param);
}

int run_smp_function(unsigned int cpu, void (*func)(unsigned long arg),  unsigned long arg)
{
	int ret;

	if (cpu > (NR_CPUS - 1))
		printf("CPU%u larger than max cpuidx %d\n", cpu, NR_CPUS - 1);
	smp_boot[cpu].bootup = func;
	smp_boot[cpu].param = arg;
	ret = secondary_boot(cpu, (unsigned int)gd->relocaddr);

	return ret;
}
