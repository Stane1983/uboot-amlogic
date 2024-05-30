// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/aml_vmode.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_tv.h"

/* ************************************************** *
   lcd mode function
 * ************************************************** */
static struct lcd_duration_s lcd_std_fr[] = {
	{288, 288,    1,    0},
	{240, 240,    1,    0},
	{239, 240000, 1001, 1},
	{200, 200,    1,    0},
	{192, 192,    1,    0},
	{191, 192000, 1001, 1},
	{144, 144,    1,    0},
	{120, 120,    1,    0},
	{119, 120000, 1001, 1},
	{100, 100,    1,    0},
	{96,  96,     1,    0},
	{95,  96000,  1001, 1},
	{60,  60,     1,    0},
	{59,  60000,  1001, 1},
	{50,  50,     1,    0},
	{48,  48,     1,    0},
	{47,  48000,  1001, 1},
	{0,   0,      0,    0}
};

//ref default mode compatible
static struct lcd_vmode_info_s lcd_vmode_ref[] = {
	{//0
		.name     = "600p",
		.width    = 1024,
		.height   = 600,
		.base_fr  = 60,
		.duration_index = 0,
		.duration = {
			{60,  60,     1,    0},
			{59,  60000,  1001, 1},
			{50,  50,     1,    0},
			{48,  48,     1,    0},
			{47,  48000,  1001, 1},
			{0,   0,      0,    0}
		},
		.dft_timing = NULL,
	},
	{//1
		.name     = "768p",
		.width    = 1366,
		.height   = 768,
		.base_fr  = 60,
		.duration_index = 0,
		.duration = {
			{60,  60,     1,    0},
			{59,  60000,  1001, 1},
			{50,  50,     1,    0},
			{48,  48,     1,    0},
			{47,  48000,  1001, 1},
			{0,   0,      0,    0}
		},
		.dft_timing = NULL,
	},
	{//2
		.name     = "1080p",
		.width    = 1920,
		.height   = 1080,
		.base_fr  = 60,
		.duration_index = 0,
		.duration = {
			{60,  60,     1,    0},
			{59,  60000,  1001, 1},
			{50,  50,     1,    0},
			{48,  48,     1,    0},
			{47,  48000,  1001, 1},
			{0,   0,      0,    0}
		},
		.dft_timing = NULL,
	},
	{//3
		.name     = "2160p",
		.width    = 3840,
		.height   = 2160,
		.base_fr  = 60,
		.duration_index = 0,
		.duration = {
			{60,  60,     1,    0},
			{59,  60000,  1001, 1},
			{50,  50,     1,    0},
			{48,  48,     1,    0},
			{47,  48000,  1001, 1},
			{0,   0,      0,    0}
		},
		.dft_timing = NULL,
	},
	{//4
		.name     = "invalid",
		.width    = 1920,
		.height   = 1080,
		.base_fr  = 60,
		.duration_index = 0,
		.duration = {
			{60,  60,     1,    0},
			{0,   0,      0,    0}
		},
		.dft_timing = NULL,
	},
};

static int lcd_vmode_add_list(struct aml_lcd_drv_s *pdrv, struct lcd_vmode_info_s *vmode_info)
{
	struct lcd_vmode_list_s *temp_list;
	struct lcd_vmode_list_s *cur_list;

	if (!vmode_info)
		return -1;

	/* creat list */
	cur_list = malloc(sizeof(*cur_list));
	if (!cur_list)
		return -1;
	memset(cur_list, 0, sizeof(*cur_list));
	cur_list->info = vmode_info;

	if (!pdrv->vmode_mgr.vmode_list_header) {
		pdrv->vmode_mgr.vmode_list_header = cur_list;
	} else {
		temp_list = pdrv->vmode_mgr.vmode_list_header;
		while (temp_list->next)
			temp_list = temp_list->next;
		temp_list->next = cur_list;
	}

	LCDPR("%s: name:%s, base_fr:%dhz\n",
		__func__, cur_list->info->name, cur_list->info->base_fr);

	return 0;
}

static int lcd_vmode_remove_list(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_list_s *cur_list;
	struct lcd_vmode_list_s *next_list;

	cur_list = pdrv->vmode_mgr.vmode_list_header;
	while (cur_list) {
		next_list = cur_list->next;
		if (cur_list->info) {
			memset(cur_list->info, 0, sizeof(struct lcd_vmode_info_s));
			free(cur_list->info);
		}
		memset(cur_list, 0, sizeof(struct lcd_vmode_list_s));
		free(cur_list);
		cur_list = next_list;
	}
	pdrv->vmode_mgr.vmode_list_header = NULL;
	pdrv->vmode_mgr.cur_vmode_info = NULL;

	return 0;
}

static void lcd_vmode_duration_add(struct lcd_vmode_info_s *vmode_find,
		struct lcd_duration_s *duration_tablet, unsigned int size)
{
	unsigned int n, i;

	memset(vmode_find->duration, 0, sizeof(struct lcd_duration_s) * LCD_DURATION_MAX);
	n = 0;
	for (i = 0; i < size; i++) {
		if (duration_tablet[i].frame_rate == 0)
			break;
		if (duration_tablet[i].frame_rate > vmode_find->dft_timing->frame_rate_max)
			continue;
		if (duration_tablet[i].frame_rate < vmode_find->dft_timing->frame_rate_min)
			break;

		vmode_find->duration[n].frame_rate = duration_tablet[i].frame_rate;
		vmode_find->duration[n].duration_num = duration_tablet[i].duration_num;
		vmode_find->duration[n].duration_den = duration_tablet[i].duration_den;
		vmode_find->duration[n].frac = duration_tablet[i].frac;
		n++;
		if (n >= LCD_DURATION_MAX)
			break;
	}
	vmode_find->duration_cnt = n;
}

//--default ref mode: 1080p60hz, 2160p60hz...
static struct lcd_vmode_info_s *lcd_vmode_default_find(struct lcd_detail_timing_s *ptiming)
{
	struct lcd_vmode_info_s *vmode_find = NULL;
	int i, dft_vmode_size = ARRAY_SIZE(lcd_vmode_ref);

	for (i = 0; i < dft_vmode_size; i++) {
		if (ptiming->h_active == lcd_vmode_ref[i].width &&
		    ptiming->v_active == lcd_vmode_ref[i].height &&
		    ptiming->frame_rate == lcd_vmode_ref[i].base_fr) {
			vmode_find = malloc(sizeof(struct lcd_vmode_info_s));
			if (!vmode_find)
				return NULL;
			memset(vmode_find, 0, sizeof(struct lcd_vmode_info_s));
			memcpy(vmode_find, &lcd_vmode_ref[i], sizeof(struct lcd_vmode_info_s));
			vmode_find->dft_timing = ptiming;
			lcd_vmode_duration_add(vmode_find,
				lcd_vmode_ref[i].duration, LCD_DURATION_MAX);
			break;
		}
	}

	return vmode_find;
}

//--general mode: (h)x(v)p(frame_rate)hz
static struct lcd_vmode_info_s *lcd_vmode_general_find(struct lcd_detail_timing_s *ptiming)
{
	struct lcd_vmode_info_s *vmode_find = NULL;
	unsigned int std_fr_size = ARRAY_SIZE(lcd_std_fr);

	vmode_find = malloc(sizeof(struct lcd_vmode_info_s));
	if (!vmode_find)
		return NULL;
	memset(vmode_find, 0, sizeof(struct lcd_vmode_info_s));
	vmode_find->width = ptiming->h_active;
	vmode_find->height = ptiming->v_active;
	vmode_find->base_fr = ptiming->frame_rate;
	snprintf(vmode_find->name, 32, "%dx%dp", ptiming->h_active, ptiming->v_active);
	vmode_find->dft_timing = ptiming;
	lcd_vmode_duration_add(vmode_find, lcd_std_fr, std_fr_size);

	return vmode_find;
}

static void lcd_output_vmode_init(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_info_s *vmode_find = NULL;
	struct lcd_detail_timing_s **timing_match;
	int i;

	if (!pdrv)
		return;

	lcd_vmode_remove_list(pdrv);

	//default ref timing
	//--default mode: 1080p60hz, 2160p60hz...
	vmode_find = lcd_vmode_default_find(&pdrv->config.timing.dft_timing);
	if (!vmode_find) {
		//--general mode: (h)x(v)p(frame_rate)hz
		vmode_find = lcd_vmode_general_find(&pdrv->config.timing.dft_timing);
		if (!vmode_find)
			return;
	}
	lcd_vmode_add_list(pdrv, vmode_find);

	timing_match = lcd_cus_ctrl_timing_match_get(pdrv);
	if (timing_match) {
		for (i = 0; i < pdrv->config.cus_ctrl.timing_cnt; i++) {
			if (!timing_match[i])
				break;

			vmode_find = lcd_vmode_default_find(timing_match[i]);
			if (!vmode_find) {
				//--general mode: (h)x(v)p(frame_rate)hz
				vmode_find = lcd_vmode_general_find(timing_match[i]);
				if (!vmode_find)
					continue;
			}
			lcd_vmode_add_list(pdrv, vmode_find);
		}
		memset(timing_match, 0,
			pdrv->config.cus_ctrl.timing_cnt * sizeof(struct lcd_detail_timing_s *));
		free(timing_match);
	}
}

/* ************************************************** *
 * vout server api
 * **************************************************
 */
static void lcd_list_support_mode(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_list_s *temp_list;
	unsigned int frame_rate;
	int i;

	temp_list = pdrv->vmode_mgr.vmode_list_header;
	while (temp_list) {
		if (!temp_list->info)
			continue;

		for (i = 0; i < LCD_DURATION_MAX; i++) {
			frame_rate = temp_list->info->duration[i].frame_rate;
			if (frame_rate == 0)
				break;
			printf("%s%dhz\n", temp_list->info->name, frame_rate);
		}
		temp_list = temp_list->next;
	}
}

static int lcd_outputmode_is_matched(struct aml_lcd_drv_s *pdrv, char *mode)
{
	struct lcd_vmode_list_s *temp_list;
	char mode_name[48];
	int i;

	if (!mode)
		return -1;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: outputmode=%s\n", pdrv->index, __func__, mode);

	temp_list = pdrv->vmode_mgr.vmode_list_header;
	while (temp_list) {
		for (i = 0; i < LCD_DURATION_MAX; i++) {
			if (temp_list->info->duration[i].frame_rate == 0)
				break;
			memset(mode_name, 0, 48);
			sprintf(mode_name, "%s%dhz", temp_list->info->name,
					temp_list->info->duration[i].frame_rate);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: %s: %s\n", pdrv->index, __func__, mode_name);
			if (strcmp(mode, mode_name))
				continue;

			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: match %s, %dx%d@%dhz\n",
					pdrv->index, __func__, temp_list->info->name,
					temp_list->info->width, temp_list->info->height,
					temp_list->info->duration[i].frame_rate);
			}
			temp_list->info->duration_index = i;
			if (pdrv->vmode_mgr.cur_vmode_info != temp_list->info)
				pdrv->vmode_mgr.next_vmode_info = temp_list->info;
			return 0;
		}
		temp_list = temp_list->next;
	}

	LCDERR("[%d]: %s: invalid mode: %s\n", pdrv->index, __func__, mode);
	return -1;
}

static void lcd_vmode_update(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming;
	unsigned int pre_pclk;
	int dur_index;

	/* clear clk flag */
	pdrv->config.timing.clk_change &= ~(LCD_CLK_PLL_RESET);
	if (pdrv->vmode_mgr.next_vmode_info) {
		pre_pclk = pdrv->config.timing.base_timing.pixel_clk;
		pdrv->vmode_mgr.cur_vmode_info = pdrv->vmode_mgr.next_vmode_info;
		pdrv->vmode_mgr.next_vmode_info = NULL;

		pdrv->std_duration = pdrv->vmode_mgr.cur_vmode_info->duration;
		ptiming = pdrv->vmode_mgr.cur_vmode_info->dft_timing;
		memcpy(&pdrv->config.timing.base_timing, ptiming,
			sizeof(struct lcd_detail_timing_s));
		lcd_cus_ctrl_config_update(pdrv, (void *)ptiming, LCD_CUS_CTRL_SEL_TIMMING);

		//update base_timing to act_timing
		lcd_enc_timing_init_config(pdrv);
		if (pdrv->config.timing.base_timing.pixel_clk != pre_pclk)
			pdrv->config.timing.clk_change |= LCD_CLK_PLL_RESET;
	}

	if (!pdrv->vmode_mgr.cur_vmode_info || !pdrv->std_duration) {
		LCDERR("[%d]: %s: cur_vmode_info or std_duration is null\n",
			pdrv->index, __func__);
		return;
	}
	dur_index = pdrv->vmode_mgr.cur_vmode_info->duration_index;

	pdrv->config.timing.act_timing.sync_duration_num =
		pdrv->std_duration[dur_index].duration_num;
	pdrv->config.timing.act_timing.sync_duration_den =
		pdrv->std_duration[dur_index].duration_den;
	pdrv->config.timing.act_timing.frac = pdrv->std_duration[dur_index].frac;
	pdrv->config.timing.act_timing.frame_rate = pdrv->std_duration[dur_index].frame_rate;
	lcd_frame_rate_change(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %dx%d, duration=%d:%d, dur_index=%d, clk_change=0x%x\n",
			pdrv->index, __func__,
			pdrv->config.timing.act_timing.h_active,
			pdrv->config.timing.act_timing.v_active,
			pdrv->config.timing.act_timing.sync_duration_num,
			pdrv->config.timing.act_timing.sync_duration_den,
			dur_index, pdrv->config.timing.clk_change);
	}
}

static int lcd_config_valid(struct aml_lcd_drv_s *pdrv, char *mode)
{
	int ret;

	ret = lcd_outputmode_is_matched(pdrv, mode);
	if (ret)
		return -1;

	lcd_vmode_update(pdrv);

	lcd_clk_generate_parameter(pdrv);

	return 0;
}

static void lcd_config_init(struct aml_lcd_drv_s *pdrv)
{
	char *mode_str;

	lcd_enc_timing_init_config(pdrv);
	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */

	lcd_output_vmode_init(pdrv);
	//assign default outputmode for multi timing
	mode_str = env_get("outputmode");
	if (mode_str)
		lcd_config_valid(pdrv, mode_str);
}

int lcd_mode_tv_init(struct aml_lcd_drv_s *pdrv)
{
	pdrv->list_support_mode = lcd_list_support_mode;
	pdrv->outputmode_check = lcd_outputmode_is_matched;
	pdrv->config_valid = lcd_config_valid;
	pdrv->driver_init_pre = lcd_tv_driver_init_pre;
	pdrv->driver_init = lcd_tv_driver_init;
	pdrv->driver_disable = lcd_tv_driver_disable;

	lcd_config_init(pdrv);
	pdrv->probe_done = 1;

	return 0;
}


