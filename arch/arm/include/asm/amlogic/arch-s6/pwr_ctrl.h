/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PDID_DSPA		0
#define PDID_DOS_HEVC	1
#define PDID_DOS_VDEC  	2
#define PDID_VPU_HDMI   3
#define PDID_U2DRD     	4
#define PDID_U3DRD		5
#define PDID_SD_EMMC_C	6
#define PDID_GE2D		7
#define PDID_AMFC		8
#define PDID_VC9000E	9
#define PDID_DEWARP		10
#define PDID_VICP		11
#define PDID_SD_EMMC_A	12
#define PDID_SD_EMMC_B	13
#define PDID_ETH		14
#define PDID_PCIE		15
#define PDID_NNA_4T		16
#define PDID_AUDIO		17
#define PDID_AUCPU		18
#define PDID_ADAPT		19

#define PM_ETH			PDID_ETH

#define PM_MAX		20

unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
unsigned long pwr_ctrl_status_psci_smc(unsigned int power_domain);
#endif
