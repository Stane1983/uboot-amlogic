/*
 * plat/s7d/include/timing.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 */
#ifndef __AML_TIMING_H_
#define __AML_TIMING_H_
#include "ddr_define.h"
#include <stdint.h>
//#include <dev_parameter.h>

#define BL2_INIT_STAGE_0 0
#define BL2_INIT_STAGE_1 1
#define BL2_INIT_STAGE_2 2
#define BL2_INIT_STAGE_3 3
#define BL2_INIT_STAGE_4 4
#define BL2_INIT_STAGE_5 5
#define BL2_INIT_STAGE_6 6
#define BL2_INIT_STAGE_7 7
#define BL2_INIT_STAGE_8 8
#define BL2_INIT_STAGE_9 9

#define BL2_INIT_STAGE_PWM_PRE_INIT 0x81
#define BL2_INIT_STAGE_PWM_CHK_HW 0x82
#define BL2_INIT_STAGE_PWM_CFG_GROUP 0x83
#define BL2_INIT_STAGE_PWM_INIT 0xC0
#define reserve_variable_enable 1
#define reserve_variable_disable 0

#define DDR_AMLOGIC_9029_PHY_AC_LANE_NUM  16
#define DDR_AMLOGIC_9029_PHY_AC_LANE_RESERVE_NUM  0
#define DWC_AC_PINMUX_TOTAL						30
#define DWC_DFI_PINMUX_TOTAL					10
#define DWC_DQ_PINMUX_TOTAL						32
/*bl2 efuse val flag*/
#define BL2_INIT_STAGE_VDDCORE_CONFIG_1		0x84
#define BL2_INIT_STAGE_VDDCORE_CONFIG_2		0x85
#define BL2_INIT_STAGE_VDDCORE_CONFIG_3		0x86
/* board vmin_flag */
#define BL2_INIT_STAGE_VMIN_FLAG_1     0x87
#define BL2_INIT_STAGE_VMIN_FLAG_2     0x88
#define BL2_INIT_STAGE_VMIN_FLAG_3     0x89

#define DDR_FW_TOTAL_OFFSET 0
#define DDR_FW_TOTAL_SIZE 2
#define DDR_FW_TOTAL_VERSION 3
#define DDR_FW_BIN_OFFSET 4
#define DDR_FW_BIN_SIZE 6
#define DDR_FW_VERSION 7
#define DDR_ACS_BIN_OFFSET 8
#define DDR_ACS_BIN_SIZE 10
#define DDR_ACS_VERSION 11
#define DDR_FAST_BOOT_DATA_OFFSET 12
#define DDR_FAST_BOOT_DATA_SIZE 14
#define DDR_FAST_BOOT_DATA_VERSION 15
#define DDR_STICKY_REG_ADD_OFFSET 16
#define DDR_STICKY_REG_SIZE_OFFSET 18
#define DDR_STICKY_REG_VERSION_OFFSET 19

#define __packed __attribute__ ((packed))

typedef struct ddr_reg {
	unsigned int	reg;
	unsigned int	value;
	unsigned int	mask;
	unsigned short	udelay;
	unsigned char	flag;
	unsigned char	rsv_0;
} __packed ddr_reg_t;

typedef struct ddr_offset {
	unsigned short	read_dq_delay_t[72];
	unsigned short	read_dq_delay_c[72];
	unsigned char	soc_bit_vref0[36];
	unsigned char	soc_bit_vref1[36];
	unsigned char	soc_bit_vref2[36];
	unsigned char	soc_bit_vref3[36];
}  __packed ddr_offset_t;

typedef struct ddr_mrs_reg {
	unsigned short ddr_mr[52];
}  __packed ddr_mrs_reg_t;

typedef struct ddr_timing {
	unsigned int	identifier;
	unsigned int	ddr_mrd;
	unsigned int	cfg_ddr_rfcab;
	unsigned int	cfg_ddr_rfcpb;
	unsigned int	cfg_ddr_pbr2act;
	unsigned int	cfg_ddr_pbr2pbr;
	unsigned int	cfg_ddr_ppd;
	unsigned int	cfg_ddr_rpab;
	unsigned int	cfg_ddr_rppb;
	unsigned int	cfg_ddr_rtw;
	unsigned int	cfg_ddr_rl;
	unsigned int	cfg_ddr_wl;
	//unsigned	int		cfg_ddr_nrbtp;
	unsigned int	cfg_ddr_ras;
	unsigned int	cfg_ddr_rc;
	unsigned int	cfg_ddr_rcd;
	unsigned int	cfg_ddr_rrds;
	unsigned int	cfg_ddr_rrdl;
	unsigned int	cfg_ddr_faw;
	unsigned int	cfg_ddr_rtp;
	unsigned int	cfg_ddr_wr;
	unsigned int	cfg_ddr_wtrs;
	unsigned int	cfg_ddr_wtrl;
	unsigned int	cfg_ddr_ccds;
	unsigned int	cfg_ddr_ccdl;
	unsigned int	cfg_ddr_exsr;
	unsigned int	cfg_ddr_xs;
	unsigned int	cfg_ddr_xp;
	unsigned int	cfg_ddr_xpdll;
	unsigned int	cfg_ddr_zqcs;
	unsigned int	cfg_ddr_cksre;
	unsigned int	cfg_ddr_cksrx;
	unsigned int	cfg_ddr_cke;
	unsigned int	cfg_ddr_mod;
	unsigned int	cfg_ddr_dqs;
	unsigned int	cfg_ddr_rstl;
	unsigned int	cfg_ddr_zqlat;
	unsigned int	ddr_mrr;
	unsigned int	cfg_ddr_ckesr;
	unsigned int	cfg_ddr_dpd;
	unsigned int	cfg_ddr_ckeck;
	unsigned int	cfg_ddr_refi;
	unsigned int	cfg_ddr_sr;
	unsigned int	cfg_ddr_ccdmw;
	unsigned int	cfg_ddr_escke;
	unsigned int	cfg_ddr_refi_ddr3;
	unsigned int	cfg_ddr_dfictrldelay;
	unsigned int	cfg_ddr_dfiphywrdata;
	unsigned int	cfg_ddr_dfiphywrlat;
	unsigned int	cfg_ddr_dfiphyrddataen;
	unsigned int	cfg_ddr_dfiphyrdlat;
	unsigned int	cfg_ddr_dfiphywrlatcsn;
	unsigned int	cfg_ddr_dfiphyrddatacsn;
	unsigned int	cfg_ddr_dfictrlupdmin;
	unsigned int	cfg_ddr_dfictrlupdmax;
	unsigned int	cfg_ddr_dfimstrresp;
	unsigned int	cfg_ddr_dfirefmski;
	unsigned int	cfg_ddr_dfictrlupdi;
	unsigned int	cfg_ddr_dfidramclk;
	unsigned int	cfg_ddr_dfilpresp;
	unsigned int	cfg_ddr_dfiphymstr;
	unsigned int	cfg_ddr_rtodt;
	unsigned int	cfg_ddr_wlmrd;
	unsigned int	cfg_ddr_wlo;
	unsigned int	cfg_ddr_al;
	unsigned int	cfg_ddr_zqcl;
	unsigned int	cfg_ddr_zqcsi;
	unsigned int	cfg_ddr_zqreset;
	unsigned int	cfg_ddr_tdqsck_min;
	unsigned int	cfg_ddr_tdqsck_max;
	//uint32_t addition_cl;
	uint32_t	dfi_wrlat;
	uint32_t	dfi_rdlat;
	//training_delay_set_ps_t	cfg_ddr_training_delay_ps;
	ddr_mrs_reg_t	ddr_mrs_reg_ps[2];
	unsigned int	dfi_odt1_config_ps[2];
	//ddr_mrs_reg_t		ddr_mrs_reg_ps1;
#if PSEUDO_DISABLE
	unsigned int	ddr_mr[8];
	unsigned int	ddr_mr11;
	unsigned int	ddr_mr12;
	unsigned int	ddr_mr13;
	unsigned int	ddr_mr14;
	unsigned int	ddr_mr16;
	unsigned int	ddr_mr17;
	unsigned int	ddr_mr22;
	unsigned int	ddr_mr24;
	unsigned int	cfg_ddr_reserve[5];
#endif
}__attribute__ ((packed)) ddr_timing_t;

typedef struct ddr_org_delay_set {
	unsigned short	read_dq_delay_t[72];
	unsigned short	read_dq_delay_c[72];
	unsigned short	read_dqs_delay[8];
	unsigned short	write_dqs_delay[8];
	unsigned short	write_wck_delay[8];
	unsigned short	wdq_delay[72];
	unsigned short	read_dqs_gate_delay[8];
}  __packed ddr_org_delay_set_t;

//#define DDR_FW_HEAD_SIZE		(32<<2)//>96
//#define DDR_BL2_DDR_FW_HEAD_SIZE					256

typedef struct ddr_fw_head_struct {
	uint32_t	ddr_all_fw_add;
	uint32_t	ddr_all_fw_size;
	uint32_t	ddr_all_fw_version;

	uint32_t	ddr_fw_add;
	uint32_t	ddr_fw_size;
	uint32_t	ddr_fw_version;

	uint32_t	ddr_acs_bin_add;
	uint32_t	ddr_acs_bin_size;
	uint32_t	ddr_acs_bin_version;

	uint32_t	ddr_fast_boot_data_add;
	uint32_t	ddr_fast_boot_data_size;
	uint32_t	ddr_fast_boot_data_version;

	uint32_t	ddr_sticky_add;
	uint32_t	ddr_sticky_size;
	uint32_t	ddr_sticky_version;

	// void (*log_info)(log_chl chl,const char *fmt, ...);
	int (*serial_puts)(const char *s);
	void (*serial_put_hex)(unsigned long data, int bitlen);
	void (*ddr_init_return)(void);

	uint32_t ddr_bl2_ddr_fw_mail_message[12];
} ddr_fw_head_struct_t;

typedef struct pll_ctrl {
	/*Enable flag: 0xa5:need set pll,  other: no need*/
	unsigned char	flag;
	/* Delay time for timing sequence in 10us, range [0 .. 255]*/
	unsigned char	delay_10u;
	/* PLL clock
	 * syspll	[93 .. 6000]
	 * fixpll	[93 .. 6000]
	 * gp0pll	[93 .. 6000]
	 * gp1pll	[93 .. 6000]
	 * hifipll	[375 .. 6000]
	 */
	unsigned short	clkset;
	unsigned int	pll_para[8];
	unsigned int	reserve;
} __attribute__ ((packed)) pll_ctrl_t;

typedef struct pll_set {
	pll_ctrl_t	sys_pll_ctrl;
	pll_ctrl_t	sys1_pll_ctrl;
	pll_ctrl_t	fix_pll_ctrl;
	pll_ctrl_t	gp0_pll_ctrl;
	pll_ctrl_t	gp1_pll_ctrl;
	pll_ctrl_t	hifi_pll_ctrl;
}__attribute__ ((packed)) pll_set_t;

typedef struct dwc_apb {
	unsigned int	addr;
	unsigned short	val;
} dwc_apb_t;

typedef struct bl2_sec_parameter {
	uint32_t	version;
	uint32_t	bl31_region_start;
	uint32_t	bl31_region_size;
	uint32_t	bl32_region_start;
	uint32_t	bl32_region_size;

	uint32_t	RFU[27];
}__attribute__ ((packed)) sec_parameter_t;

#endif //__AML_TIMING_H_
