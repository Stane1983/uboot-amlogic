// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPCD_REG.h>
#include "dptx_regs.h"
#include "dptx_common.h"
#include "DPTX_IP/dptx_IP_ops.h"

uint16_t dptx_training_rd_interval[5] = {400, 4000, 8000, 12000, 16000};

static char *eDP_ver_str[6] = {"1.1", "1.2", "1.3", "1.4a", "1.4b", "1.5"};

u16 std_level_to_phy_dft_lut[10][3] = {
/*Vswing|Pre-emphasis: 0,               1,               2,               3 */
/* 0 */  {0x3, 0x3, 0x0}, {0x3, 0x7, 0x0}, {0x3, 0xb, 0x0}, {0x3, 0xf, 0x0},
/* 1 */  {0x7, 0x3, 0x0}, {0x7, 0x8, 0x0}, {0x8, 0xc, 0x0},
/* 2 */  {0xa, 0x4, 0x0}, {0xa, 0x8, 0x0},
/* 3 */  {0xf, 0x5, 0x0}
};

//DPCD value (2bit) to ANACTRL phy value(0~0xf)
uint8_t dptx_vswing_ds_to_phy(struct dptx_drv_s *dptx, uint8_t ds_level)
{
	if (ds_level > 9) {
		DPTXPR(dptx->idx, LOG_E, "%s level %u out of protocal limit", __func__, ds_level);
		return 0;
	}

	if (dptx->phy_cfg.level_to_phy_lut[0][0])
		return dptx->phy_cfg.level_to_phy_lut[ds_level][0];
	else
		return std_level_to_phy_dft_lut[ds_level][0];
}

uint8_t dptx_preem_ds_to_phy(struct dptx_drv_s *dptx, uint8_t ds_level)
{
	if (ds_level > 9) {
		DPTXPR(dptx->idx, LOG_E, "%s level %u out of protocal limit", __func__, ds_level);
		return 0;
	}

	if (dptx->phy_cfg.level_to_phy_lut[0][0])
		return dptx->phy_cfg.level_to_phy_lut[ds_level][1];
	else
		return std_level_to_phy_dft_lut[ds_level][1];
}

uint8_t ds_to_DPCD_LANESET(uint8_t ds_level)
{
	// DPCD TRAINING_LANEx_SET
	// Bits 1:0 = VOLTAGE SWING SET
	// Bit 2 = MAX_SWING_REACHED
	// Bit 4:3 = PRE-EMPHASIS_SET
	// Bit 5 = MAX_PRE-EMPHASIS_REACHED

	switch (ds_level) {
	case DPTX_PHY_STA_VSW_0_PREEM_0:
		return (0x0 << 5 | 0x0 << 3 | 0x0 << 2 | 0x0);
	case DPTX_PHY_STA_VSW_0_PREEM_1:
		return (0x0 << 5 | 0x1 << 3 | 0x0 << 2 | 0x0);
	case DPTX_PHY_STA_VSW_0_PREEM_2:
		return (0x0 << 5 | 0x2 << 3 | 0x0 << 2 | 0x0);
	case DPTX_PHY_STA_VSW_0_PREEM_3:
		return (0x1 << 5 | 0x3 << 3 | 0x0 << 2 | 0x0);
	case DPTX_PHY_STA_VSW_1_PREEM_0:
		return (0x0 << 5 | 0x0 << 3 | 0x0 << 2 | 0x1);
	case DPTX_PHY_STA_VSW_1_PREEM_1:
		return (0x0 << 5 | 0x1 << 3 | 0x0 << 2 | 0x1);
	case DPTX_PHY_STA_VSW_1_PREEM_2:
		return (0x0 << 5 | 0x2 << 3 | 0x0 << 2 | 0x1);
	case DPTX_PHY_STA_VSW_2_PREEM_0:
		return (0x0 << 5 | 0x0 << 3 | 0x0 << 2 | 0x2);
	case DPTX_PHY_STA_VSW_2_PREEM_1:
		return (0x0 << 5 | 0x1 << 3 | 0x0 << 2 | 0x2);
	case DPTX_PHY_STA_VSW_3_PREEM_0:
		return (0x0 << 5 | 0x0 << 3 | 0x1 << 2 | 0x3);
	default:
		DPTXPR(0, LOG_E, "%s level %u out of protocal limit", __func__, ds_level);
		return 0;
	}
}

uint8_t dptx_v_p_to_ds(uint8_t vsw, uint8_t preem)
{
	if (vsw == 0 && preem < 4)
		return 0 + preem;
	else if (vsw == 1 && preem < 3)
		return 4 + preem;
	else if (vsw == 2 && preem < 2)
		return 7 + preem;
	else if (vsw == 3 && preem < 1)
		return 9 + preem;

	DPTXPR(0, LOG_E, "%s (vsw=%u preem=%u) out of protocal limit", __func__, vsw, preem);
	return 0;
}

uint8_t dptx_ds_to_vswing(uint8_t ds)
{
	if (ds < 4)
		return 0;
	if (ds < 7)
		return 1;
	if (ds < 9)
		return 2;
	return 3;
}

uint8_t dptx_ds_to_preem(uint8_t ds)
{
	if (ds == 3)
		return 3;
	if (ds == 2 || ds == 6)
		return 2;
	if (ds == 1 || ds == 5 || ds == 8)
		return 1;
	return 0;
}

uint8_t dptx_DPCD_capability_to_link_cfg(struct dptx_drv_s *dptx)
{
	uint8_t auxdata[16];
	uint8_t sink_DPCD_ver, sink_max_lkr, sink_lane_cnt, sink_enhanced_frame, sink_TPS_support,
		sink_down_spread, sink_coding_support, sink_msa_timing_par_ignored,
		sink_train_aux_rd_interval, sink_extended_receiver_cap, sink_DACP_support = 0,
		sink_eDP_ver = 0, sink_eDP_DPCD_reg = 0;

	// dptx_reg_print(dptx);

	if (__dptx_aux_read(dptx, DPCD_DPCD_REV, 16, auxdata)) {
		DPTXPR(dptx->idx, LOG_I, "fail to get DPCD capability");
		return 1;
	}
	//DPCD_REVISION: 0x0000
	sink_DPCD_ver       = auxdata[0];
	//DPCD_MAX_LINK_RATE: 0x0001
	sink_max_lkr        = auxdata[1];
	//DPCD_MAX_LANE_COUNT: 0x0002
	sink_lane_cnt       = auxdata[2] & 0xf;
	sink_TPS_support    = (auxdata[2] >> 6) & 0x1;
	sink_enhanced_frame = (auxdata[2] >> 7) & 0x1;
	//DPCD_MAX_DOWNSPREAD: 0x0003
	sink_down_spread    = auxdata[3] & 0x1;
	sink_TPS_support   |= ((auxdata[3] >> 7) & 0x1) << 1;
	//MAIN_LINK_CHANNEL_CODING_CAP: 0x0006
	sink_coding_support = auxdata[6] & 0x3;
	//DOWN_STREAM_PORT_COUNT: 0x0007
	sink_msa_timing_par_ignored = (auxdata[7] >> 6) & 0x1;

	if (auxdata[0xd]) {
		//eDP_CONFIGURATION_CAP: 0x000d
		sink_DACP_support  = (auxdata[0] & 0x1) << 2; //eDP ASSR
		sink_eDP_DPCD_reg  = (auxdata[0] >> 3) & 0x1;

		if (sink_eDP_DPCD_reg) {
			if (!__dptx_aux_read(dptx, DPCD_EDP_DPCD_REV, 1, auxdata))
				sink_eDP_ver = auxdata[0] > 5 ? 0 : auxdata[0];
		}
	}
	//8b/10b_TRAINING_AUX_RD_INTERVAL: 0x000e
	sink_train_aux_rd_interval = auxdata[0xe] & 0x7f;
	sink_extended_receiver_cap = (auxdata[1] >> 7) & 0x1;

	//limit to prevent out of bound
	// dptx->link_cfg.train_aux_rd_interval = CAP_COMP(sink_sp.train_aux_rd_interval, 4);
	// DP_cfg->max_link_rate = CAP_COMP(source_sp->link_rate, sink_sp.link_rate);

	dptx->link_cfg.max_lane_count        =
		dptx->user_lane_count == 0xff ? sink_lane_cnt : dptx->user_lane_count;
	dptx->link_cfg.max_link_rate         = dptx->user_link_rate ?
		dptx->user_link_rate : CAP_COMP(sink_max_lkr, dptx->data->link_rate);
	dptx->link_cfg.TPS_support           = sink_TPS_support & dptx->data->TPS_support;
	dptx->link_cfg.DACP_support          = sink_DACP_support & dptx->data->DACP_support;
	dptx->link_cfg.down_ss               = sink_down_spread;
	dptx->link_cfg.train_aux_rd_interval = sink_train_aux_rd_interval;
	dptx->link_cfg.dev_type              = auxdata[0xd] ? 1 : 0;
	dptx->link_cfg.DPCD_reg_func         = sink_extended_receiver_cap ? BIT(0) : 0 |
					       sink_eDP_DPCD_reg          ? BIT(1) : 0;
	dptx->link_cfg.enhanced_framing_en   = sink_enhanced_frame;

	DPTXPR(dptx->idx, LOG_I, "sink DPCD Capability:\n"
		" - DPCD reversion:  %01x.%01x\n"
		" - sink capability: %u lane, %u.%u GHz\n"
		" - enhanced frame:  %u\n"
		" - TPS support:     [TPS3:%u, TPS4:%u]\n"
		" - clk down spread: %u\n"
		" - coding support:  [8b/10b:%u, 128b/132b:%u]\n"
		" - MSA ignore:      %u\n"
		" - train aux rd:    %uus\n"
		" - HDCP:            %u\n"
		" - ext DPCD cap:    %u",
		sink_DPCD_ver >> 4, sink_DPCD_ver & 0xf,
		sink_lane_cnt, (sink_max_lkr * 27) / 100, (sink_max_lkr * 27) % 100,
		sink_enhanced_frame, sink_TPS_support & 0x1, (sink_TPS_support >> 1) & 0x1,
		sink_down_spread,
		sink_coding_support & 0x1, (sink_coding_support >> 1) & 0x1,
		sink_msa_timing_par_ignored,
		dptx_training_rd_interval[sink_train_aux_rd_interval],
		sink_DACP_support & 0x1, sink_extended_receiver_cap);
	if (sink_eDP_DPCD_reg) {
		pr_info(" - eDP version:     eDP %s\n"
			" - eDP ASSR:        %u\n"
			" - eDP DPCD:        %u\n",
			eDP_ver_str[sink_eDP_ver],
			(sink_DACP_support & 0x4) >> 2,
			sink_eDP_DPCD_reg);
	}
	return 0;
}

#define DPTX_BW_CHECK_LIMIT_PERCENT       95 //95%
uint8_t dptx_vid_band_width_check(uint8_t link_rate, uint8_t lane_cnt, uint32_t pclk, uint8_t bpp)
{
	uint32_t bw_req, bw_cal, bw_load;

	bw_cal = 270 * link_rate * lane_cnt * 8 / 10;
	bw_req = (pclk / 1000000 + 1) * bpp;

	bw_load = bw_req * 100 / bw_cal;

	if (bw_load > DPTX_BW_CHECK_LIMIT_PERCENT) {
		DPTXPR(0, LOG_V, "%s: vid(%uMHz) / link(%uMHz) = bw_load(%u%%), over limit",
			__func__, bw_req, bw_cal, bw_load);
		return 0;
	}
	return 1;
}

struct dptx_color_format_s dptx_cfmt_table[DPTX_COLOR_TYPE_MAX] = {
	{CFMT_RGB_6bit,        18, "RGB-6b"},
	{CFMT_RGB_8bit,        24, "RGB-8b"},
	{CFMT_RGB_10bit,       30, "RGB-10b"},
	{CFMT_RGB_12bit,       36, "RGB-12b"},
	{CFMT_YCbCr422_8bit,   16, "YUV422-8b"},
	{CFMT_YCbCr422_10bit,  20, "YUV422-10b"},
	{CFMT_YCbCr422_12bit,  24, "YUV422-12b"},
	{CFMT_YCbCr444_8bit,   24, "YUV444-8b"},
	{CFMT_YCbCr444_10bit,  30, "YUV444-10b"},
	{CFMT_YCbCr444_12bit,  36, "YUV444-12b"},
	{CFMT_YCbCr420_8bit,   12, "YUV420-8b"},
	{CFMT_YCbCr420_10bit,  15, "YUV420-10b"},
	{CFMT_YCbCr420_12bit,  18, "YUV420-12b"},
	{CFMT_Y_only_8bit,      8, "Y-8b"},
	{CFMT_Y_only_10bit,    10, "Y-10b"},
	{CFMT_Y_only_12bit,    12, "Y-12b"},
	{CFMT_invalid,          1, "invalid"},
};

struct dptx_detail_timing_s DPTX_SafeMode_640x480_timing = {
	.name = "SafeMode_480P",

	.h_size = 400,
	.v_size = 300,

	.h_period = 800,
	.h_act    = 640,
	.h_blank  = 160,
	.h_bp     = 48,
	.h_pw     = 86,
	.h_fp     = 16,

	.v_period = 525,
	.v_act    = 480,
	.v_blank  = 45,
	.v_bp     = 33,
	.v_pw     = 2,
	.v_fp     = 10,

	// .h_border = 0,
	// .v_border = 0,

	.ctrl = 0,

	.pclk = 25000000,
	.fr1000 = 60000,

	.cfmt = CFMT_RGB_6bit,

	.v_period_range = {0, 0},
	.pclk_range = {0, 0},
	.frame_rate_range = {0, 0},
	// .vfreq_vrr_range = {0, 0},
	.sync_duration_num = 60,
	.sync_duration_den = 1,
};

struct dptx_vmode_s DPTX_SafeMode_640x480_vmode = {
	.fr100_int = 6000,
	.fr_adv = 0xff,
	.base_dtd_idx = 0xff,
	.flag = VMODE_FLAG_VALID,
	.cfmt_support = 1 << CFMT_RGB_6bit,
};

uint8_t __str_add_vmode(struct dptx_drv_s *dptx, char *buf, uint8_t vmd_idx, uint8_t fr)
{
	uint8_t i, use_short = 0;
	uint16_t vmd_h, vmd_v, vmd_fr;
	struct dptx_vmode_s *vmd;
	struct dptx_detail_timing_s *vmd_timing;
	struct V_name_s { uint16_t h, v; uint8_t fr[5]; } V_name_table[] = {
		{3840, 2160, {0xff,}},
		{1920, 1080, {0xff,}},
		{1366,  768, { 60, 59, 50, 48, 47}},
		{1280,  720, { 60, 50,  0,  0,  0}},
	};

	if (vmd_idx == U8_MAX) {
		vmd = &DPTX_SafeMode_640x480_vmode;
		vmd_timing = &DPTX_SafeMode_640x480_timing;
	} else {
		vmd = &dptx->vmode_mgr.vmodes[vmd_idx];
		vmd_timing = &dptx->edid_info.dtd_timing[vmd->base_dtd_idx];
	}

	vmd_h = vmd_timing->h_act;
	vmd_v = vmd_timing->v_act;
	vmd_fr = vmd->fr_adv == 1 ? (vmd->fr100_int * 10 / 1001) : ((vmd->fr100_int + 50) / 100);

	for (i = 0; i < 5 * ARRAY_SIZE(V_name_table); i++) {
		if (V_name_table[i / 5].h == vmd_h && V_name_table[i / 5].v == vmd_v) {
			if (V_name_table[i / 5].fr[i % 5] == 0xff ||
			    V_name_table[i / 5].fr[i % 5] == vmd_fr) {
				use_short = 1;
				break;
			}
		}
	}

	i = use_short ? sprintf(buf,    "%up",        vmd_v) :
			sprintf(buf, "%ux%up", vmd_h, vmd_v);
	if (fr)
		i += sprintf(buf + i, "%huhz", vmd_fr);

	return i;
}
