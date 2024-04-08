/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PDID_DOS_HCODEC		0
#define PDID_DOS_HEVC		1
#define PDID_DOS_VDEC  		2
#define PDID_VPU_HDMI   	3
#define PDID_USB_U2DRD     	4
#define PDID_USB_U2H     	5
#define PDID_SD_EMMC_C          6
#define PDID_GE2D          	7
#define PDID_AMFC		8
#define PDID_SD_EMMC_A    	9
#define PDID_SD_EMMC_B          10
#define PDID_ETH     		11
#define PDID_AUCPU         	12
#define PDID_AUDIO     		13
#define PDID_SRAMA		14
#define PDID_DMC0     		15
#define PDID_DMC1         	16
#define PDID_DDR     		17

#define PM_ETH			PDID_ETH

#define PM_MAX			18

unsigned long viu_init_psci_smc(void);
unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
unsigned long pwr_ctrl_status_psci_smc(unsigned int power_domain);
#endif
