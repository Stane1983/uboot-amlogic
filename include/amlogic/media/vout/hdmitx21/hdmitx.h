/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_H_
#define __HDMITX_H_
#include "hdmi_common.h"
#include "hdmitx_module.h"
#include "hdmitx_reg.h"
#include "mach_reg.h"
#include <command.h>

int hdmitx_likely_frac_rate_mode(char *m);
unsigned int hdmi_outputmode_check(char *mode, unsigned int frac);
int do_hpd_detect(cmd_tbl_t *cmdtp, int flag, int argc,
		  char *const argv[]);
/*
 * sync with uboot 2019 follow SWPL-166617
 * When updating outputmodeX env, check whether the connectorX is HDMI
 * to avoid affecting the MIPI screen
 */
int is_valid_hdmi(const char *input);

#endif
