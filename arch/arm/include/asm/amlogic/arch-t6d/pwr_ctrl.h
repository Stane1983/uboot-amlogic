/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PDID_DOS_HEVC	0
#define PDID_DOS_VDEC	1
#define PDID_VPU_HDMI	2
#define PDID_USB_COMB	3
#define PDID_SD_EMMC_C	4
#define PDID_GE2D	5
#define PDID_SD_EMMC_A	6
#define PDID_SD_EMMC_B	7
#define PDID_ETH	8
#define PDID_AUCPU	9
#define PDID_AUDIO	10

#define PM_ETH			PDID_ETH

#define PM_MAX		11

unsigned long viu_init_psci_smc(void);
unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
unsigned long pwr_ctrl_status_psci_smc(unsigned int power_domain);
#endif
