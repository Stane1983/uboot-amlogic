/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_INI_H__
#define __AML_INI_H__

int handle_model_list_panel_key(void);
int handle_model_list(void);
int handle_model_sum(void);
int handle_model_get(const char *model, char buf[]);
int handle_model_set(const char *model, const char *val);

#endif /* __AML_INI_H__ */
