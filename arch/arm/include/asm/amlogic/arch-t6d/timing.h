/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_TIMING_H_
#define __AML_TIMING_H_

#include <asm/amlogic/arch/ddr_define.h>
#include <asm/amlogic/arch/types.h>
//#include <asm/arch/mnPmuSramMsgBlock_ddr3.h>
//#include <asm/arch/mnPmuSramMsgBlock_ddr4.h>
//#include <asm/arch/mnPmuSramMsgBlock_ddr4_2d.h>
//#include <asm/arch/mnPmuSramMsgBlock_lpddr3.h>
//#include <asm/arch/mnPmuSramMsgBlock_lpddr4.h>
//#include <asm/arch/mnPmuSramMsgBlock_lpddr4_2d.h>
//#include <asm/amlogic/arch/dev_parameter.h>

#define BL2_INIT_STAGE_0			0
#define BL2_INIT_STAGE_1			1
#define BL2_INIT_STAGE_2			2
#define BL2_INIT_STAGE_3			3
#define BL2_INIT_STAGE_4			4
#define BL2_INIT_STAGE_5			5
#define BL2_INIT_STAGE_6			6
#define BL2_INIT_STAGE_7			7
#define BL2_INIT_STAGE_8			8
#define BL2_INIT_STAGE_9			9
/*bl2 efuse val flag*/
#define BL2_INIT_STAGE_VDDCORE_CONFIG_1		0x84
#define BL2_INIT_STAGE_VDDCORE_CONFIG_2		0x85
#define BL2_INIT_STAGE_VDDCORE_CONFIG_3		0x86
/* board vmin_flag */
#define BL2_INIT_STAGE_VMIN_FLAG_1     0x87
#define BL2_INIT_STAGE_VMIN_FLAG_2     0x88
#define BL2_INIT_STAGE_VMIN_FLAG_3     0x89

typedef struct ddr_mrs_reg {
	unsigned int cfg_ddr_mr[8];
	unsigned int cfg_ddr_mr11;
	unsigned int cfg_ddr_mr12;
	unsigned int cfg_ddr_mr13;
	unsigned int cfg_ddr_mr14;
	unsigned int cfg_ddr_mr16;
	unsigned int cfg_ddr_mr17;
	unsigned int cfg_ddr_mr22;
	unsigned int cfg_ddr_mr24;
} __attribute__((packed)) ddr_mrs_reg_t;

typedef struct ddr_timing {
	unsigned int identifier;
	unsigned int cfg_ddr_mrd;
	unsigned int cfg_ddr_rfcab;
	unsigned int cfg_ddr_rfcpb;
	unsigned int cfg_ddr_rpab;
	unsigned int cfg_ddr_rppb;
	unsigned int cfg_ddr_rtw;
	unsigned int cfg_ddr_rl;
	unsigned int cfg_ddr_wl;
	unsigned int cfg_ddr_ras;
	unsigned int cfg_ddr_rc;
	unsigned int cfg_ddr_rcd;
	unsigned int cfg_ddr_rrds;
	unsigned int cfg_ddr_rrdl;
	unsigned int cfg_ddr_faw;
	unsigned int cfg_ddr_rtp;
	unsigned int cfg_ddr_wr;
	unsigned int cfg_ddr_wtrs;
	unsigned int cfg_ddr_wtrl;
	unsigned int cfg_ddr_ccds;
	unsigned int cfg_ddr_ccdl;
	unsigned int cfg_ddr_exsr;
	unsigned int cfg_ddr_xs;
	unsigned int cfg_ddr_xp;
	unsigned int cfg_ddr_xpdll;
	unsigned int cfg_ddr_zqcs;
	unsigned int cfg_ddr_cksre;
	unsigned int cfg_ddr_cksrx;
	unsigned int cfg_ddr_cke;
	unsigned int cfg_ddr_mod;
	unsigned int cfg_ddr_dqs;
	unsigned int cfg_ddr_rstl;
	unsigned int cfg_ddr_zqlat;
	unsigned int cfg_ddr_mrr;
	unsigned int cfg_ddr_ckesr;
	unsigned int cfg_ddr_dpd;
	unsigned int cfg_ddr_ckeck;
	unsigned int cfg_ddr_refi;
	unsigned int cfg_ddr_sr;
	unsigned int cfg_ddr_ccdmw;
	unsigned int cfg_ddr_escke;
	unsigned int cfg_ddr_refi_ddr3;
	unsigned int cfg_ddr_dfictrldelay;
	unsigned int cfg_ddr_dfiphywrdata;
	unsigned int cfg_ddr_dfiphywrlat;
	unsigned int cfg_ddr_dfiphyrddataen;
	unsigned int cfg_ddr_dfiphyrdlat;
	unsigned int cfg_ddr_dfictrlupdmin;
	unsigned int cfg_ddr_dfictrlupdmax;
	unsigned int cfg_ddr_dfimstrresp;
	unsigned int cfg_ddr_dfirefmski;
	unsigned int cfg_ddr_dfictrlupdi;
	unsigned int cfg_ddr_dfidramclk;
	unsigned int cfg_ddr_dfilpresp;
	unsigned int cfg_ddr_dfiphymstr;
	unsigned int cfg_ddr_rtodt;
	unsigned int cfg_ddr_wlmrd;
	unsigned int cfg_ddr_wlo;
	unsigned int cfg_ddr_al;
	unsigned int cfg_ddr_zqcl;
	unsigned int cfg_ddr_zqcsi;
	unsigned int cfg_ddr_zqreset;
	unsigned int cfg_ddr_tdqsck_min;
	unsigned int cfg_ddr_tdqsck_max;
	//training_delay_set_ps_t       cfg_ddr_training_delay_ps;
	ddr_mrs_reg_t cfg_ddr_mrs_reg_ps[2];
	unsigned int dfi_odt1_config_ps[2];
	//ddr_mrs_reg_t         cfg_ddr_mrs_reg_ps1;
#if 0
	unsigned int cfg_ddr_mr[8];
	unsigned int cfg_ddr_mr11;
	unsigned int cfg_ddr_mr12;
	unsigned int cfg_ddr_mr13;
	unsigned int cfg_ddr_mr14;
	unsigned int cfg_ddr_mr16;
	unsigned int cfg_ddr_mr17;
	unsigned int cfg_ddr_mr22;
	unsigned int cfg_ddr_mr24;
	unsigned int cfg_ddr_reserve[5];
#endif
} __attribute__((packed)) ddr_timing_t;

typedef struct ddr_phy_common_extra_set {
	unsigned short csr_pllctrl3;
	unsigned short csr_pptctlstatic[4];
	unsigned short csr_trainingincdecdtsmen[4];
	unsigned short csr_tsmbyte0[4];
	unsigned short csr_hwtcamode;
	unsigned short csr_hwtlpcsena;
	unsigned short csr_hwtlpcsenb;
	unsigned short csr_acsmctrl13;
	unsigned short csr_acsmctrl23;
	unsigned char csr_soc_vref_dac1_dfe[36];
} __attribute__((packed)) ddr_phy_common_extra_set_t;

typedef struct pll_ctrl {
	/*Enable flag: 0xa1:need set pll in bl2
	 *  0xa2:need set pll in bl2x
	 *  other: no need
	 */
	unsigned char flag;
	/* Delay time for timing sequence in 10us, range [0 .. 255] */
	unsigned char delay_10u;
	/* PLL clock
	 * syspll       [93 .. 6000]
	 * fixpll       [93 .. 6000]
	 * gp0pll       [93 .. 6000]
	 * gp1pll       [93 .. 6000]
	 * hifipll      [375 .. 6000]
	 */
	unsigned short clkset;
	unsigned int pll_para[8];
	unsigned int reserve;
} __attribute__((packed)) pll_ctrl_t;

typedef struct pll_set {
	/*new struct for sc2 */
	pll_ctrl_t sys_pll_ctrl;
	pll_ctrl_t fix_pll_ctrl;
	pll_ctrl_t gp0_pll_ctrl;
	pll_ctrl_t gp1_pll_ctrl;
	pll_ctrl_t hifi_pll_ctrl;
} __attribute__((packed)) pll_set_t;

typedef struct bl2_sec_parameter {
	/*new struct for sc2 */
	uint32_t version;
	uint32_t bl31_region_start;
	uint32_t bl31_region_size;
	uint32_t bl32_region_start;
	uint32_t bl32_region_size;

	uint32_t RFU[27];
} __attribute__((packed)) sec_parameter_t;

#endif //__AML_TIMING_H_
