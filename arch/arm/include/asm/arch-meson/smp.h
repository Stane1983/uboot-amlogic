/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SMP_H__
#define __SMP_H__

#define secondary_sp_size 0x100000
struct smp_boot_param {
	void (*bootup)(unsigned long args);
	unsigned long param;
};

void cpu_smp_init_r(void);
void secondary_bootup_r(void);
void secondary_off(void);
unsigned int cpu_online_status(void);
int run_smp_function(unsigned int cpu, void (*func)(unsigned long arg),  unsigned long arg);
unsigned int get_core_id(void);
#endif
