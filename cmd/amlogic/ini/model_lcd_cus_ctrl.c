// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "ini_config.h"

#define LOG_TAG "model"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_proxy.h"
#include "ini_handler.h"
#include "ini_platform.h"
#include "ini_io.h"
#include "model.h"
#include <amlogic/partition_table.h>

#ifdef CONFIG_AML_LCD
int glcd_cus_ctrl_cnt;

static unsigned short handle_lcd_cus_ctrl_ufr(unsigned char *p, unsigned short *ctrl_attr)
{
	const char *ini_value = NULL;
	unsigned short offset = 0, size;

	//step 1: base vtotal range
	ini_value = ini_get_string("lcd_Attr", "ufr_vtotal_min", "none");
	if (strcmp(ini_value, "none") == 0)
		ini_value = ini_get_string("lcd_Attr", "ctrl_attr_0_parm0", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_min is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return 0;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = ini_get_string("lcd_Attr", "ufr_vtotal_max", "none");
	if (strcmp(ini_value, "none") == 0)
		ini_value = ini_get_string("lcd_Attr", "ctrl_attr_0_parm1", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_max is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return 0;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;
	size = offset;

	//step 2: frame_rate range
	ini_value = ini_get_string("lcd_Attr", "ufr_frame_rate_min", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_frame_rate_min is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = ini_get_string("lcd_Attr", "ufr_frame_rate_max", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_frame_rate_max is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;
	size = offset;

	//step 3: vsync config
	ini_value = ini_get_string("lcd_Attr", "ufr_vpw", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vpw is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = ini_get_string("lcd_Attr", "ufr_vbp", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vbp is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;
	size = offset;

	return size;
}

static unsigned short handle_lcd_cus_ctrl_dfr(unsigned char *p, unsigned short *ctrl_attr)
{
	struct lcd_dfr_timing_s dfr_timing;
	const char *ini_value = NULL;
	char str[30];
	unsigned short offset = 0, temp[2], timing_size;
	unsigned char fr_cnt = 0, tmg_group_cnt = 0;
	unsigned char *p_fr_cnt, *p_tmg_group_cnt;
	int i;

	p_fr_cnt = p + offset;
	offset += 1;
	p_tmg_group_cnt = p + offset;
	offset += 1;

	for (i = 0; i < 15; i++) {
		sprintf(str, "dfr_fr_%d_tmg_index", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		temp[0] = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_fr_%d_frame_rate", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		temp[1] = strtoul(ini_value, NULL, 0);
		*(unsigned short *)(p + offset) = (temp[1] & 0xfff) | ((temp[0] & 0xf) << 12);
		offset += 2;

		sprintf(str, "dfr_fr_%d_frame_rate_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "0");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		*(unsigned short *)(p + offset) = strtoul(ini_value, NULL, 0);
		offset += 2;

		sprintf(str, "dfr_fr_%d_frame_rate_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "0");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		*(unsigned short *)(p + offset) = strtoul(ini_value, NULL, 0);
		offset += 2;

		fr_cnt++;
	}
	*p_fr_cnt = fr_cnt;

	timing_size = sizeof(struct lcd_dfr_timing_s);
	for (i = 1; i < 15; i++) {
		sprintf(str, "dfr_tmg_%d_vtotal", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vtotal_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vtotal_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vpw", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vpw = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vbp", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vbp = strtoul(ini_value, NULL, 0);

		memcpy((p + offset), &dfr_timing, timing_size);
		offset += timing_size;
		tmg_group_cnt++;
	}
	*p_tmg_group_cnt = tmg_group_cnt;

	return offset;
}

static unsigned short handle_lcd_cus_ctrl_extend_tmg(unsigned char *p, unsigned short *ctrl_attr)
{
	struct lcd_cus_ctrl_extend_tmg_s extend_tmg;
	const char *ini_value = NULL;
	char str[30];
	unsigned short offset = 0, tmg_size;
	unsigned int tmg_group_cnt = 0;
	int spw, spol, i;

	tmg_size = sizeof(struct lcd_cus_ctrl_extend_tmg_s);
	for (i = 0; i < 15; i++) {
		sprintf(str, "extend_tmg_%d_hactive", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.hactive = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vactive", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vactive = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hpw", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spw = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hbp", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.hbp = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hs_pol", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spol = strtoul(ini_value, NULL, 0);
		extend_tmg.hpw_pol = (spw & 0xfff) | ((spol & 0xf) << 12);

		sprintf(str, "extend_tmg_%d_vpw", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spw = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vbp", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vbp = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vs_pol", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spol = strtoul(ini_value, NULL, 0);
		extend_tmg.vpw_pol = (spw & 0xfff) | ((spol & 0xf) << 12);

		sprintf(str, "extend_tmg_%d_fr_adj_type", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.fr_adjust_type = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pixel_clk", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.pixel_clk = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_frame_rate_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			extend_tmg.frame_rate_min = 0;
		else
			extend_tmg.frame_rate_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_frame_rate_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			extend_tmg.frame_rate_max = 0;
		else
			extend_tmg.frame_rate_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pclk_min", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.pclk_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pclk_max", i);
		ini_value = ini_get_string("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.pclk_max = strtoul(ini_value, NULL, 0);

		memcpy((p + offset), &extend_tmg, tmg_size);
		offset += tmg_size;
		tmg_group_cnt++;
	}
	*ctrl_attr &= (unsigned short)~0xf0;
	*ctrl_attr |= ((unsigned short)tmg_group_cnt << 4);//bit[7:4]: tmg_group_cnt

	return offset;
}

static unsigned short handle_lcd_cus_ctrl_clk_adv(unsigned char *p, unsigned short *ctrl_attr)
{
	const char *ini_value = NULL;
	unsigned short offset = 0;

	ini_value = ini_get_string("lcd_Attr", "ss_freq", "0");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ss_freq is (%s)\n", __func__, ini_value);
	*(p + offset) = strtoul(ini_value, NULL, 0);
	offset += 1;

	ini_value = ini_get_string("lcd_Attr", "ss_mode", "0");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ss_mode is (%s)\n", __func__, ini_value);
	*(p + offset) = strtoul(ini_value, NULL, 0);
	offset += 1;

	return offset;
}

static unsigned short handle_lcd_cus_ctrl_tuning_attr(unsigned char *p, unsigned short *ctrl_attr)
{
	struct lcd_tuning_ch_sel_s ch_sel;
	struct lcd_tuning_s lcd_tuning;
	struct lcd_tuning_phy_ch_s lcd_phy_ch;
	char sec_str[16], ch_sel_str[16], ch_amp_str[16], ch_preem_str[16];
	char ch_phase_str[16], phase_sel;
	const char *ini_value = NULL;
	unsigned short lane_cnt, offset = 0, ch_sel_size, tuning_size, phy_ch_size;
	unsigned int group_cnt = 0;
	int i, n;

	ch_sel_size = sizeof(struct lcd_tuning_ch_sel_s);
	tuning_size = sizeof(struct lcd_tuning_s);
	phy_ch_size = sizeof(struct lcd_tuning_phy_ch_s);

	*ctrl_attr &= (unsigned short)~0xf0;

	//detect exist
	ini_value = ini_get_string("lane_sel_Attr", "lcd_if", "null");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, lcd_if is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		*ctrl_attr &= (unsigned short)~0xf0;
		return 0;
	}

	ini_value = ini_get_string("lane_sel_Attr", "lane_count", "null");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, lane_count is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		*ctrl_attr &= (unsigned short)~0xf0;
		return 0;
	}
	lane_cnt = strtoul(ini_value, NULL, 0);
	*(p + offset) = lane_cnt;
	offset += 2;

	for (i = 0; i < lane_cnt; i++) {
		sprintf(ch_sel_str, "ch%u_sel", i);
		sprintf(ch_phase_str, "ch%u_phase", i);

		ch_sel.pn_phase = 0; //reserved for pn swap

		ini_value = ini_get_string("lane_sel_Attr", ch_sel_str, "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, ch_sel_str, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		ch_sel.sel = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string("lane_sel_Attr", ch_phase_str, "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, ch_phase_str, ini_value);
		if (strcmp(ini_value, "null") == 0)
			phase_sel = 0xf;
		else
			phase_sel = strtoul(ini_value, NULL, 0);
		ch_sel.pn_phase &= ~(0xf << 1);
		ch_sel.pn_phase |= (phase_sel & 0xf) << 1;

		//save to attr_buf
		memcpy((p + offset), &ch_sel, ch_sel_size);
		offset += ch_sel_size;
	}

	for (n = 0; n < 15; n++) {
		if (n == 0)
			sprintf(sec_str, "tuning_Attr");
		else
			sprintf(sec_str, "tuning_Attr%d", n);

		//phy_clk match
		ini_value = ini_get_string(sec_str, "phy_clk", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, phy_clk is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			break;
		lcd_tuning.phy_clk = strtoul(ini_value, NULL, 0);

		lcd_tuning.phy_clk_min = 0;
		lcd_tuning.phy_clk_max = 0;

		//ssc
		ini_value = ini_get_string(sec_str, "ss_level", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, ss_level is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.ss_level = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "ss_frequency", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, ss_frequency is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.ss_freq = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "ss_mode", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, ss_mode is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.ss_mode = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "mlvds_clk_phase", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, mlvds_clk_phase is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			lcd_tuning.mlvds_clk_phase = 0;
		else
			lcd_tuning.mlvds_clk_phase = strtoul(ini_value, NULL, 0);

		//phy
		ini_value = ini_get_string(sec_str, "vswing", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, vswing is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.phy_vswing = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "vcm", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, vcm is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.phy_vcm = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "ref_bias", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, ref_bias is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.phy_ref_bias = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "odt", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, odt is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.phy_odt = strtoul(ini_value, NULL, 0);

		ini_value = ini_get_string(sec_str, "cv_mode", "null");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, cv_mode is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0)
			goto handle_lcd_cus_ctrl_tuning_attr_err;
		lcd_tuning.phy_cv_mode = strtoul(ini_value, NULL, 0);

		lcd_tuning.phy_attr_5 = 0;
		lcd_tuning.phy_attr_6 = 0;
		lcd_tuning.phy_attr_7 = 0;
		lcd_tuning.phy_attr_8 = 0;
		lcd_tuning.phy_attr_9 = 0;
		lcd_tuning.phy_attr_10 = 0;
		lcd_tuning.phy_attr_11 = 0;

		//save to attr_buf
		memcpy((p + offset), &lcd_tuning, tuning_size);
		offset += tuning_size;

		for (i = 0; i < lane_cnt; i++) {
			sprintf(ch_amp_str, "ch%u_amp", i);
			sprintf(ch_preem_str, "ch%u_preem", i);

			ini_value = ini_get_string(sec_str, ch_preem_str, "null");
			if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
				ALOGD("%s, %s is (%s)\n", __func__, ch_preem_str, ini_value);
			if (strcmp(ini_value, "null") == 0)
				goto handle_lcd_cus_ctrl_tuning_attr_err;
			lcd_phy_ch.preem = strtoul(ini_value, NULL, 0);

			ini_value = ini_get_string(sec_str, ch_amp_str, "null");
			if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
				ALOGD("%s, %s is (%s)\n", __func__, ch_amp_str, ini_value);
			if (strcmp(ini_value, "null") == 0)
				goto handle_lcd_cus_ctrl_tuning_attr_err;
			lcd_phy_ch.amp = strtoul(ini_value, NULL, 0);

			//save to attr_buf
			memcpy((p + offset), &lcd_phy_ch, phy_ch_size);
			offset += phy_ch_size;
		}

		group_cnt++;
	}

	*ctrl_attr |= ((unsigned short)group_cnt << 4);//bit[7:4]: group_cnt
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, lane_cnt:%d, group_cnt:%d\n", __func__, lane_cnt, group_cnt);

	return offset;

handle_lcd_cus_ctrl_tuning_attr_err:
	ALOGE("%s, miss parameter, exit!\n", __func__);
	return 0;
}

int handle_lcd_cus_ctrl(unsigned char *p_attr, unsigned char version)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned char *p;
	struct lcd_cus_ctrl_s *cus_ctrl = NULL;
	unsigned short offset, param_size, ctrl_attr;
	unsigned int attr_type;
	int i;

	if (!p_attr) {
		ALOGE("%s, p_attr is NULL\n", __func__);
		return -1;
	}
	switch (version) {
	case 2:
		offset = sizeof(struct lcd_header_s) + sizeof(struct lcd_phy_s);
		cus_ctrl = (struct lcd_cus_ctrl_s *)(p_attr + offset);
		break;
	case 3:
		offset = sizeof(struct lcd_header_s);
		cus_ctrl = (struct lcd_cus_ctrl_s *)(p_attr + offset);
		break;
	default:
		break;
	}
	if (!cus_ctrl) {
		ALOGE("%s, cus_ctrl is NULL\n", __func__);
		return -1;
	}

	ini_value = ini_get_string("lcd_Attr", "ctrl_attr_en", "none");
	if (strcmp(ini_value, "none") == 0) //old version compatible
		ini_value = ini_get_string("lcd_Attr", "ctrl_attr_flag", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ctrl_attr_en is (%s)\n", __func__, ini_value);
	cus_ctrl->ctrl_attr_en = strtoul(ini_value, NULL, 0);

	p = cus_ctrl->data;
	offset = 0;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		sprintf(str, "ctrl_attr_%d", i);
		ini_value = ini_get_string("lcd_Attr", str, "0xffff");
		if (strcmp(ini_value, "0xffff")) {
			if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
				ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		}
		ctrl_attr = strtoul(ini_value, NULL, 0);
		attr_type = (ctrl_attr >> 8) & 0xff;
		switch (attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			param_size = handle_lcd_cus_ctrl_ufr((p + offset + 4), &ctrl_attr);
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			param_size = handle_lcd_cus_ctrl_dfr((p + offset + 4), &ctrl_attr);
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			param_size = handle_lcd_cus_ctrl_extend_tmg((p + offset + 4), &ctrl_attr);
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			param_size = handle_lcd_cus_ctrl_clk_adv((p + offset + 4), &ctrl_attr);
			break;
		case LCD_CUS_CTRL_TYPE_TUNING_ATTR:
			if (version < 3) {
				param_size = 0;
				ALOGE("%s, don't support tuning_attr with ukey version %d!\n",
				      __func__, version);
				break;
			}
			param_size = handle_lcd_cus_ctrl_tuning_attr((p + offset + 4), &ctrl_attr);
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			param_size = 0;
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			param_size = 0;
			break;
		default:
			param_size = 0;
			break;
		}

		*(unsigned short *)(p + offset) = ctrl_attr;
		offset += 2;

		*(unsigned short *)(p + offset) = param_size;
		offset += 2;

		offset += param_size;
	}
	glcd_cus_ctrl_cnt = 4 + offset;
	if (glcd_cus_ctrl_cnt > LCD_CUS_CTRL_MAX) {
		ALOGE("%s, glcd_cus_ctrl_cnt %d error, max %d!!!\n",
			__func__, glcd_cus_ctrl_cnt, LCD_CUS_CTRL_MAX);
		return -1;
	}

	return 0;
}

#endif
