/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLOGIC_WATCHKEY_
#define __AMLOGIC_WATCHKEY_

#include <common.h>

/* Key type definition */
#define WATCHKEY_ID_ADCKEY		0
#define WATCHKEY_ID_GPIOKEY		1

#if CONFIG_IS_ENABLED(AMLOGIC_WATCHKEY)
char * const *amlogic_watchkey_list(void);
int amlogic_watchkey_state(u16 id);
int amlogic_watchkey_enable(u16 id);
int amlogic_watchkey_disable(u16 id);
int amlogic_watchkey_dump(u16 id, u32 *d0, u32 *d1, u32 *d2);
int amlogic_watchkey_test(u16 id);
#else
static inline char * const *amlogic_watchkey_list(void)
{
	return NULL;
}

static inline int amlogic_watchkey_state(u16 id)
{
	return -EOPNOTSUPP;
}

static inline int amlogic_watchkey_enable(u16 id)
{
	return -EOPNOTSUPP;
}

static inline int amlogic_watchkey_disable(u16 id)
{
	return -EOPNOTSUPP;
}

static inline int amlogic_watchkey_dump(u16 id, u32 *d0, u32 *d1, u32 *d2)
{
	return -EOPNOTSUPP;
}

static inline int amlogic_watchkey_test(u16 id)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_AMLOGIC_WATCHKEY */

#endif /* __AMLOGIC_WATCHKEY_ */
