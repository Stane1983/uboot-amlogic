// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <mailbox-common.h>

#ifdef CONFIG_ARMV8_MULTIENTRY
int mb_spinlock_try_init(struct mb_spinlock_t *mb_lock)
{
	int ret = 1;

	if (!mb_lock->init) {
		mb_lock->init = true;
		spin_lock_init(&mb_lock->lock);
		ret = 0;
	}

	return ret;
}
#endif

