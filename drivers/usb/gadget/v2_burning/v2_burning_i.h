/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __V2_BURNING_I_H__
#define __V2_BURNING_I_H__

#include <config.h>
//#include <environment.h>
#include <linux/types.h>
#include <asm/string.h>
#include <linux/errno.h>
#include <asm/byteorder.h>
#include <malloc.h>
#include <u-boot/sha1.h>
#include <console.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/io.h>
#include <asm/amlogic/arch/bl31_apis.h>

#include <amlogic/aml_v2_burning.h>
//#include <asm/arch/reboot.h>
#include <asm/amlogic/arch/romboot.h>
//#include <amlogic/aml_lcd.h>
#include <amlogic/storage.h>
#include "v2_common/sparse_format.h"
#include "v2_common/optimus_download.h"
#include "v2_common/amlImage_if.h"
#include "v2_common/optimus_progress_ui.h"
#include <amlogic/store_wrapper.h>

struct partitions *get_partition_info_by_num(const int num);
extern int cli_simple_parse_line(char *line, char *argv[]);
#ifndef getenv
#define getenv env_get
#define setenv env_set
#endif//#ifndef getenv

#endif//ifndef __V2_BURNING_I_H__

