/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMLOGIC_DisplayPort_TX_H
#define _AMLOGIC_DisplayPort_TX_H

#include <common.h>
#include <linux/list.h>
#include <dm.h>
#include <asm/gpio.h>
#include <linux/types.h>
#include <amlogic/media/vout/DisplayPort/DPTX_timing.h>

//#define LCD_DEBUG_INFO
extern u8 dptx_print_level;
//bit[15:0]
#define LOG_E      0 // log level ERROR & Key
#define LOG_I      1 // log level Informative
#define LOG_V      2 // log level Verbose
#define LOG_A      3 // log level All

#define DPTXPR(idx, level, fmt, args...) \
	do { \
		if (!level) \
			pr_err("[DPTX-%u]E: " fmt "\n", idx, ## args); \
		else if (dptx_print_level >= level) \
			pr_info("[DPTX-%u]: " fmt "\n", idx, ## args); \
	} while (0)

#define PR_BUF_MAX              (4 * 1024)

#define DPTX_MAX_DRV              2
#define DPTX_MAX_VOUT             3

#define RGB_DELAY                   13
#define PRE_DE_DELAY                8

enum dptx_chip_e {
	DPTX_CHIP_T7,
	DPTX_CHIP_MAX,
};

struct dptx_chip_data_s {
	enum dptx_chip_e chip_type;
	const char *chip_name;
	uint8_t  drv_max;
	uint32_t offset_venc[DPTX_MAX_DRV];     // 0x600
	uint32_t offset_venc_if[DPTX_MAX_DRV];  // 0x500
	uint32_t offset_venc_data[DPTX_MAX_DRV];// 0x100

	uint32_t link_rate;
	uint8_t  TPS_support;
	uint8_t  DACP_support;
	uint8_t venc_clk_msr_id[DPTX_MAX_DRV];
};

#define MOD_LEN_MAX        30

struct dptx_venc_cfg_s {
	uint8_t pll_flag;
	uint8_t clk_mode;
	uint8_t ppc;
	uint8_t clk_change; /* internal used */
	uint8_t ss_level;
	uint8_t ss_freq;
	uint8_t ss_mode;

	unsigned int enc_clk;
	unsigned long long bit_rate; /* Hz */

	unsigned int hstart;
	unsigned int hend;
	unsigned int vstart;
	unsigned int vend;
	uint8_t pre_de_h;
	uint8_t pre_de_v;

	unsigned short de_hs_addr;
	unsigned short de_he_addr;
	unsigned short de_vs_addr;
	unsigned short de_ve_addr;

	unsigned short hs_hs_addr;
	unsigned short hs_he_addr;
	unsigned short hs_vs_addr;
	unsigned short hs_ve_addr;

	unsigned short vs_hs_addr;
	unsigned short vs_he_addr;
	unsigned short vs_vs_addr;
	unsigned short vs_ve_addr;

	unsigned short pre_h_de_start;
	unsigned short pre_h_de_end;
	unsigned short pre_v_de_start;
	unsigned short pre_v_de_end;
	unsigned short pre_hso_start;
	unsigned short pre_hso_end;
	unsigned short pre_vso_hstart;
	unsigned short pre_vso_hend;
	unsigned short pre_vso_start;
	unsigned short pre_vso_end;
};

struct dptx_phy_cfg_s {
	u32 flag;
	u32 vswing;
	struct dptx_phy_lane_s {
		uint8_t status;
		uint8_t amp;
		uint8_t preem;
		uint8_t post_cur;
	} lane[4];
	u16 level_to_phy_lut[10][3];
};

#define DPTX_TYPE_DP          0
#define DPTX_TYPE_eDP         1

struct dptx_link_cfg_s {
	uint8_t max_lane_count;
	uint8_t max_link_rate;
	uint8_t TPS_support;   // bit[0]=TPS3;bit[1]=TPS4;
	uint8_t DACP_support;  // bit[0]=M1(HDCP);bit[1]=M2(Panel Limit);bit[1]=M3a(eDP ASSR);
	uint8_t down_ss;
	uint8_t train_aux_rd_interval;
	uint8_t dev_type;      // 0=DP;1=eDP
	uint8_t DPCD_reg_func; // bit[0]=extend DPCD;bit[1]=eDP_DPCD;bit[2]=eDP_DPCD_BL;

	uint8_t enhanced_framing_en;
	uint8_t sync_clk_mode;
	// u8 EDID_en;

	/* internal used */
	// unsigned char coding_support;
	uint8_t training_mode;
	uint8_t preset_ds[4];

	uint8_t HPD_level;
	uint8_t irq_sta;

	uint8_t link_rate;
	uint8_t lane_count;
	uint8_t link_rate_update;

	uint8_t phy_update;
	uint8_t curr_ds[4];
	uint8_t adj_req_ds[4];
};

#define LCD_CLK_SS_BIT_FREQ             0
#define LCD_CLK_SS_BIT_MODE             4

#define DPTX_GPIO_MAX                    0xff
#define DPTX_GPIO_OUTPUT_LOW             0
#define DPTX_GPIO_OUTPUT_HIGH            1
#define DPTX_GPIO_INPUT                  2

#define DPTX_VENC_1PPC          1
#define DPTX_VENC_2PPC          2
#define DPTX_VENC_4PPC          4

struct dptx_clk_cfg_s {
	u64 fin;
	u64 fout;
	uint8_t clk_src; // 0=PLL, 1=FIX_PLL, 2=LINK_CLK
	void *pll_data;

	/* pll parameters */
	unsigned int pll_id;
	unsigned int pll_offset;
	unsigned int pll_od_fb;
	uint16_t pll_m;
	uint16_t pll_n;
	u64 pll_fvco;
	uint8_t pll_od_sel[3];
	uint8_t pll_tcon_div_sel;

	unsigned int pll_frac;
	unsigned int pll_frac_half_shift;
	unsigned long long pll_fout;
	unsigned int pll_div_fout;

	unsigned int ss_level;
	unsigned int ss_dep_sel;
	unsigned int ss_str_m;
	unsigned int ss_ppm;
	unsigned int ss_freq;
	unsigned int ss_mode;
	unsigned int ss_en;

	uint8_t pll_clk_div_sel;
	uint8_t div0;
	uint8_t div1;
	uint8_t xd;
	uint8_t vclk_sel;

	// unsigned int err_fmin;
	unsigned int done;
};

#define DPTX_DRV_TIMING_MAX  8
struct dptx_edid_info_s {
	uint8_t manufacturer_id[4];  //[8:9]2byte
	uint16_t product_id;         //[10:11]2byte
	uint32_t product_sn;         //[12:15]4byte
	uint8_t week;                //[16]1byte
	uint8_t year;                //[17]1byte + 1990
	uint16_t version;            //[18:19]2byte
	uint32_t established_timing; //[35:37]3byte
	// uint32_t standard_timing1;   //[38:45]4byte
	// uint32_t standard_timing2;   //[46:53]4byte
	uint16_t cfmt_support;       // each bit to CFMT

	char name[14];
	char serial_num[14];
	char asc_string[14];
	// unsigned int VIC_val[8];
	struct dptx_edid_range_limit_s {
		uint16_t vfreq[2];
		uint16_t hfreq[2];
		uint32_t pclk[2];
		uint16_t h_blank_min, v_blank_min;
	} range;

	uint8_t dtd_cnt;

	struct dptx_detail_timing_s dtd_timing[DPTX_DRV_TIMING_MAX];

	uint8_t ext_flag;  //[126]1byte
	uint8_t did_version;

	uint8_t edid_crc;
};

#define DPTX_DRV_VMODE_MAX   64
#define VMODE_FLAG_VALID     BIT(0)
#define VMODE_FLAG_FR_RANGE  BIT(1)
#define VMODE_FLAG_PREFERRED BIT(2)
// #define VMODE_FLAG_BW_ENOUGH BIT(3)
struct dptx_vmode_mgr_s {
	struct dptx_vmode_s {
		uint32_t fr100_int;
		uint8_t fr_adv; // 0=fr_int; 1=fr_frac; 0xff=raw fr
		uint8_t base_dtd_idx;
		uint8_t flag;
		uint16_t cfmt_support;
	} vmodes[DPTX_DRV_VMODE_MAX];
	uint8_t vmode_sel_idx;
	uint8_t vmode_cfmt_sel;
};

#define DPTX_STA_PROBE_DONE   BIT(0)
#define DPTX_STA_DRV_READY    BIT(1)
#define DPTX_STA_HPD_HIGH     BIT(2)
#define DPTX_STA_LINK_ON      BIT(3)
#define DPTX_STA_DISP_ON      BIT(4)

#define DPTX_GPIO_NAME_MAX    12

struct dptx_drv_s {
	uint8_t idx;
	uint8_t status;
	uint8_t mode; // 0=DP; 1=eDP
	uint8_t viu_sel; // 1=vout; 2=vou2; 3=vout3

	char dev_name[32];
	uint8_t user_link_rate;  //dft: 4
	uint8_t user_lane_count; //dft: 0xfff
	uint8_t user_color_format;
	uint16_t hpd_ignore;

	char PWR_gpio_name[DPTX_GPIO_NAME_MAX];
	char CFG1_gpio_name[DPTX_GPIO_NAME_MAX];
	char CFG2_gpio_name[DPTX_GPIO_NAME_MAX];

	struct dptx_chip_data_s *data;

	struct dptx_phy_cfg_s phy_cfg;
	struct dptx_clk_cfg_s vid_clk;
	struct dptx_clk_cfg_s link_clk;
	struct dptx_venc_cfg_s venc_cfg;

	struct dptx_edid_info_s edid_info;

	struct dptx_vmode_mgr_s vmode_mgr;

	struct dptx_detail_timing_s act_timing;

	struct dptx_link_cfg_s link_cfg;

	struct dev_pm_ops *dev_pm_ops;

	void *if_ctrls;
};

#endif
