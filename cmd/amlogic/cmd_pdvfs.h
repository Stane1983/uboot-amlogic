/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __CMD_PDVFS_H
#define __CMD_PDVFS_H

#if (IS_ENABLED(CONFIG_AML_UPDATE_PDVFS))
int update_pdvfs_flag(void);
#else
int update_pdvfs_flag(void)
{
	return -1;
}
#endif

#endif
