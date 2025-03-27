/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PDID_AMFC	0
#define PDID_DOS_HEVC	1
#define PDID_GE2D	2
#define PDID_VPU_HDMI	3
#define PDID_DEMOD	4

#define PM_MAX		5

#define PM_ETH		PM_MAX

unsigned long viu_init_psci_smc(unsigned long flag);
unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
unsigned long pwr_ctrl_status_psci_smc(unsigned int power_domain);
#endif
