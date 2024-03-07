// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 */

#include <common.h>
#include <asm/global_data.h>
#include <amlogic/cpu_id.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_ARMV8_MULTIENTRY
#define secondary_sp_size		 (0x100000)
unsigned long gd_ptr_init;

/*
 * secondary_boot_func
 * this function should be write with asm, here, only for compiling pass
 */
void secondary_bootup_f(unsigned int x)
{
	__asm__ volatile("mov	x18, %0\n"
		:: "r" (gd_ptr_init) : "memory");
}

void cpu_smp_init_r(void)
{
	gd->flags |= GD_FLG_SMP;
	gd_ptr_init = (unsigned long)gd;
	flush_dcache_all();
}

unsigned int get_smp_sp(unsigned int cpuidx)
{
	if (cpuidx >= 1)
		return gd->start_addr_sp - 0x200000 - secondary_sp_size * (cpuidx - 1);
	else
		return gd->start_addr_sp;
}
#endif

