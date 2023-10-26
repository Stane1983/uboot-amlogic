/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Common header file for U-Boot
 *
 * This file still includes quite a few headers that should be included
 * individually as needed. Patches to remove things are welcome.
 *
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

#ifndef __COMMON_H_
#define __COMMON_H_	1

#ifndef __ASSEMBLY__		/* put C only stuff in this section */
#include <config.h>
#include <errno.h>
#include <time.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <stdarg.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <asm/u-boot.h> /* boot information for Linux kernel */
#include <vsprintf.h>
#endif	/* __ASSEMBLY__ */

#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/delay.h>
#include <linux/bug.h>

/* lib/rand.c */
#define RAND_MAX -1U
void srand(unsigned int seed);
unsigned int rand(void);
unsigned int rand_r(unsigned int *seedp);
#endif

/* Pull in stuff for the build system */
#ifdef DO_DEPS_ONLY
# include <env_internal.h>
#endif

#ifdef CONFIG_AML_UASAN
#include <amlogic/uasan.h>
#endif

#ifdef CONFIG_AMLOGIC_MODIFY
#ifdef CONFIG_KALLSYMS
extern const char *symbol_lookup(unsigned long addr, unsigned long *caddr, unsigned long *naddr);
#else
static inline const char *symbol_lookup(unsigned long addr,
					unsigned long *caddr,
					unsigned long *naddr)
{
	return NULL;
}
#endif
#endif

#endif	/* __COMMON_H_ */
