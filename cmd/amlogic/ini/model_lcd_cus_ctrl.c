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

static unsigned short handle_lcd_cus_ctrl_ufr(unsigned char *p, unsigned char param_flag)
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

static unsigned short handle_lcd_cus_ctrl_dfr(unsigned char *p, unsigned char param_flag)
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

static unsigned short handle_lcd_cus_ctrl_extend_tmg(unsigned char *p, unsigned char param_flag)
{
	struct lcd_cus_ctrl_extend_tmg_s extend_tmg;
	const char *ini_value = NULL;
	char str[30];
	unsigned short offset = 0, tmg_size;
	unsigned char tmg_group_cnt = param_flag;
	int spw, spol, i;

	tmg_size = sizeof(struct lcd_cus_ctrl_extend_tmg_s);
	for (i = 0; i < tmg_group_cnt; i++) {
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
	}

	return offset;
}

static unsigned short handle_lcd_cus_ctrl_clk_adv(unsigned char *p, unsigned char param_flag)
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

int handle_lcd_cus_ctrl(struct lcd_v2_attr_s *p_attr)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned char *p;
	unsigned short offset, param_size, ctrl_attr;
	unsigned short *p_param_size;
	unsigned char attr_type, param_flag;
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
		*(unsigned short *)(p + offset) = ctrl_attr;
		offset += 2;

		p_param_size = (unsigned short *)(p + offset);
		offset += 2;

		attr_type = (ctrl_attr >> 8) & 0xff;
		param_flag = (ctrl_attr >> 4) & 0xf;
		switch (attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			param_size = handle_lcd_cus_ctrl_ufr((p + offset), param_flag);
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			param_size = handle_lcd_cus_ctrl_dfr((p + offset), param_flag);
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			param_size = handle_lcd_cus_ctrl_extend_tmg((p + offset), param_flag);
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			param_size = handle_lcd_cus_ctrl_clk_adv((p + offset), param_flag);
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
		offset += param_size;
		*p_param_size = param_size;
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
