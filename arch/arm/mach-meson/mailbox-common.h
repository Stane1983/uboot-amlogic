/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MAILBOX_COMMON_H__
#define __MAILBOX_COMMON_H__

#ifdef CONFIG_ARMV8_MULTIENTRY
#include <asm/arch-meson/smp.h>
#undef spin_lock_init
#undef spin_lock
#undef spin_unlock
#include <spinlock.h>

struct mb_spinlock_t {
	spin_lock_t lock;
	bool init;
};

int mb_spinlock_try_init(struct mb_spinlock_t *mb_lock);
#endif

#endif /* __MAILBOX_COMMON_H__ */
