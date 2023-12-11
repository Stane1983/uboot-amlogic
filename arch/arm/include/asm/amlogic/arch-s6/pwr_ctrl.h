/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PDID_DOS_HCODEC	0
#define PDID_DOS_HEVC	1
#define PDID_DOS_VDEC  2
#define PDID_VPU_HDMI   3
#define PDID_USB_COMB     4
#define PDID_SD_EMMC_C          5
#define PDID_GE2D          6
#define PDID_SD_EMMC_A    7
#define PDID_SD_EMMC_B          8
#define PDID_ETH     9
#define PDID_AUCPU         10
#define PDID_AUDIO     11
#define PDID_AMFC     12

#define PM_MAX		13

unsigned long viu_init_psci_smc(void);
unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
unsigned long pwr_ctrl_status_psci_smc(unsigned int power_domain);
#endif
