/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __CMD_PDVFS_H
#define __CMD_PDVFS_H

int get_phandle(const char *path);
int get_board_id(void);
unsigned int get_cpufreq_table_index(u64 function_id,
					    u64 arg0, u64 arg1, u64 arg2);

#endif
