/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_IR_H
#define __AML_IR_H

int update_for_dtb_wakeup_key(void);

int meson_ir_suspend(void *dev_info);
int meson_ir_resume(void *dev_info);
int meson_ir_poweroff(void *dev_info);

#endif
