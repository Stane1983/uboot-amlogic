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
#include <partition_table.h>

#ifdef CONFIG_AML_LCD
int glcd_cus_ctrl_cnt;

static unsigned short handle_lcd_cus_ctrl_ufr(unsigned char *p, unsigned short *ctrl_attr)
{
	const char *ini_value = NULL;
	unsigned short offset = 0, size;

	//step 1: base vtotal range
	ini_value = IniGetString("lcd_Attr", "ufr_vtotal_min", "none");
	if (strcmp(ini_value, "none") == 0)
		ini_value = IniGetString("lcd_Attr", "ctrl_attr_0_parm0", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_min is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return 0;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = IniGetString("lcd_Attr", "ufr_vtotal_max", "none");
	if (strcmp(ini_value, "none") == 0)
		ini_value = IniGetString("lcd_Attr", "ctrl_attr_0_parm1", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_max is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return 0;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;
	size = offset;

	//step 2: frame_rate range
	ini_value = IniGetString("lcd_Attr", "ufr_frame_rate_min", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_frame_rate_min is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = IniGetString("lcd_Attr", "ufr_frame_rate_max", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_frame_rate_max is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;
	size = offset;

	//step 3: vsync config
	ini_value = IniGetString("lcd_Attr", "ufr_vpw", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vpw is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		return size;
	*(unsigned short *)(p + offset) = (unsigned short)strtoul(ini_value, NULL, 0);
	offset += 2;

	ini_value = IniGetString("lcd_Attr", "ufr_vbp", "none");
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
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		temp[0] = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_fr_%d_frame_rate", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		temp[1] = strtoul(ini_value, NULL, 0);
		*(unsigned short *)(p + offset) = (temp[1] & 0xfff) | ((temp[0] & 0xf) << 12);
		offset += 2;
		fr_cnt++;
	}
	*p_fr_cnt = fr_cnt;

	timing_size = sizeof(struct lcd_dfr_timing_s);
	for (i = 1; i < 15; i++) {
		sprintf(str, "dfr_tmg_%d_htotal", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.htotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vtotal", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vtotal_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vtotal_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vtotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_frame_rate_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			dfr_timing.frame_rate_min = 0;
		else
			dfr_timing.frame_rate_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_frame_rate_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			dfr_timing.frame_rate_max = 0;
		else
			dfr_timing.frame_rate_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_hpw", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.hpw = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_hbp", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.hbp = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vpw", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		dfr_timing.vpw = strtoul(ini_value, NULL, 0);

		sprintf(str, "dfr_tmg_%d_vbp", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
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
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.hactive = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vactive", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vactive = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hpw", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spw = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hbp", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.hbp = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_hs_pol", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spol = strtoul(ini_value, NULL, 0);
		extend_tmg.hpw_pol = (spw & 0xfff) | ((spol & 0xf) << 12);

		sprintf(str, "extend_tmg_%d_vpw", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spw = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vbp", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vbp = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vs_pol", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		spol = strtoul(ini_value, NULL, 0);
		extend_tmg.vpw_pol = (spw & 0xfff) | ((spol & 0xf) << 12);

		sprintf(str, "extend_tmg_%d_fr_adj_type", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.fr_adjust_type = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pixel_clk", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.pixel_clk = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_htotal_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.htotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_vtotal_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.vtotal_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_frame_rate_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			extend_tmg.frame_rate_min = 0;
		else
			extend_tmg.frame_rate_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_frame_rate_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			extend_tmg.frame_rate_max = 0;
		else
			extend_tmg.frame_rate_max = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pclk_min", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			break;
		extend_tmg.pclk_min = strtoul(ini_value, NULL, 0);

		sprintf(str, "extend_tmg_%d_pclk_max", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
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

	ini_value = IniGetString("lcd_Attr", "ss_freq", "0");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_min is (%s)\n", __func__, ini_value);
	*(p + offset) = strtoul(ini_value, NULL, 0);
	offset += 1;

	ini_value = IniGetString("lcd_Attr", "ss_mode", "0");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ufr_vtotal_max is (%s)\n", __func__, ini_value);
	*(p + offset) = strtoul(ini_value, NULL, 0);
	offset += 1;

	return offset;
}

static unsigned short handle_lcd_cus_ctrl_swpdf(unsigned char *p, unsigned short *ctrl_attr)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned short offset = 0, threshold_size, block_size, act_size;
	unsigned short pattern_cnt = 0;
	unsigned int num_cnt, num_buf[128];
	int i, k, n, block_num, act_num;
	unsigned short threshold[2];
	struct lcd_cus_swpdf_block_s block[SWPDF_MAX_BLOCK];
	struct lcd_cus_swpdf_act_s act[SWPDF_MAX_ACT];

	threshold_size = sizeof(threshold);
	block_size = sizeof(struct lcd_cus_swpdf_block_s);
	act_size = sizeof(struct lcd_cus_swpdf_act_s);
/*
 * block_num
 * act_num
 * threshold
 * block[0] block[1] ... block[block_num - 1]
 * act[0] act[1] ... act[act_num - 1]
 */
	for (i = 0; i < 32; i++) {
		sprintf(str, "pattern_%d_threshold", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			continue;
		num_cnt = trans_buffer_data(ini_value, num_buf);
		if (num_cnt != 2)
			continue;
		threshold[0] = (unsigned short)num_buf[0];
		threshold[1] = (unsigned short)num_buf[1];

		sprintf(str, "pattern_%d_block", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			continue;
		num_cnt = trans_buffer_data(ini_value, num_buf);
		block_num = num_cnt / 8;
		if (num_cnt % 8 || block_num > SWPDF_MAX_BLOCK)
			continue;
		for (k = 0, n = 0; k < block_num; k++, n += 8) {
			block[k].x = (unsigned short)num_buf[n + 0];
			block[k].y = (unsigned short)num_buf[n + 1];
			block[k].w = (unsigned char)num_buf[n + 2];
			block[k].h = (unsigned char)num_buf[n + 3];
			block[k].mat_val0 = num_buf[n + 4];
			block[k].mat_val1 = num_buf[n + 5];
			block[k].mat_val2 = num_buf[n + 6];
			block[k].mat_val3 = num_buf[n + 7];
		}

		sprintf(str, "pattern_%d_act", i);
		ini_value = IniGetString("lcd_Attr", str, "none");
		if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		if (strcmp(ini_value, "none") == 0)
			continue;
		num_cnt = trans_buffer_data(ini_value, num_buf);
		act_num = num_cnt / 4;
		if (num_cnt % 4 || act_num > SWPDF_MAX_ACT)
			continue;
		for (k = 0, n = 0; k < act_num; k++, n += 4) {
			act[k].reg = num_buf[n + 0];
			act[k].mask = num_buf[n + 1];
			act[k].val = num_buf[n + 2];
			act[k].bus = num_buf[n + 3];
		}

		*(p + offset + 0) = (unsigned char)block_num;
		*(p + offset + 1) = (unsigned char)act_num;
		offset += 2;

		memcpy((p + offset), threshold, threshold_size);
		offset += threshold_size;

		memcpy((p + offset), block, block_size * block_num);
		offset += block_size * block_num;

		memcpy((p + offset), act, act_size * act_num);
		offset += act_size * act_num;
		pattern_cnt++;
	}

	*ctrl_attr &= (unsigned short)~0xff;
	*ctrl_attr |= pattern_cnt;//bit[7:0]: pattern_cnt

	return offset;
}

int handle_lcd_cus_ctrl(struct lcd_v2_attr_s *p_attr)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned char *p;
	unsigned short offset, param_size, ctrl_attr;
	unsigned int attr_type;
	int i;

	ini_value = IniGetString("lcd_Attr", "ctrl_attr_en", "none");
	if (strcmp(ini_value, "none") == 0) //old version compatible
		ini_value = IniGetString("lcd_Attr", "ctrl_attr_flag", "none");
	if (model_debug_flag & DEBUG_LCD_CUS_CTRL)
		ALOGD("%s, ctrl_attr_en is (%s)\n", __func__, ini_value);
	p_attr->cus_ctrl.ctrl_attr_en = strtoul(ini_value, NULL, 0);

	p = p_attr->cus_ctrl.data;
	offset = 0;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		sprintf(str, "ctrl_attr_%d", i);
		ini_value = IniGetString("lcd_Attr", str, "0xffff");
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
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			param_size = 0;
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			param_size = handle_lcd_cus_ctrl_swpdf((p + offset + 4), &ctrl_attr);
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
