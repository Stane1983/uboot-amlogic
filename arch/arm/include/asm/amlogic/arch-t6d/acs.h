/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __ACS_H
#define __ACS_H

#include <asm/amlogic/arch/types.h>
#include <asm/amlogic/arch/ddr_define.h>

#define CHIP_PARAM_MAGIC		0x50696863  //"chiP"
#define DEV_PARAM_MAGIC			0x50766564  //"devP"

#define CHIP_PARAM_VERSION		0x1
#define DEV_PARAM_VERSION		0x1

#define MAX_REG_OPS_ENTRIES		(32)

#ifndef __ASSEMBLY__
#include <asm/amlogic/arch/acs_struct.h>
#endif
#endif
