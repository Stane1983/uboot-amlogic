// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_tablet.h"
#include <string.h>

// 1.vmode : vmode_add, vmode_remove, vmode_clear_all, vmode_set(parse, check, apply), vmode_list
//               '-<-.--------<------.                  .------------<-----------'
// 2.timing: dft_timing2vmode, exd_timing2vmode, timing_apply, (dft/exd)timing_add, timing_remove
//                  .----------------------->----------------------- '
// 3.eDP   : EDID_to_timing

//
//vmode_init: (remove all, add dft timing, add extend timing)

static struct lcd_duration_s lcd_std_fr[] = {
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

static int lcd_vmode_add_single(struct aml_lcd_drv_s *pdrv, struct lcd_vmode_info_s *vmode)
{
	struct lcd_vmode_list_s *temp_list;
	struct lcd_vmode_list_s *cur_list;

	if (!vmode)
		return -1;

	/* create list */
	cur_list = malloc(sizeof(struct lcd_vmode_list_s));
	if (!cur_list)
		return -1;
	memset(cur_list, 0, sizeof(struct lcd_vmode_list_s));
	cur_list->info = vmode;

	if (!pdrv->vmode_mgr.vmode_list_header) {
		pdrv->vmode_mgr.vmode_list_header = cur_list;
	} else {
		temp_list = pdrv->vmode_mgr.vmode_list_header;
		while (temp_list->next)
			temp_list = temp_list->next;
		temp_list->next = cur_list;
	}
	pdrv->vmode_mgr.vmode_cnt += vmode->duration_cnt + 1;

	LCDPR("%s: name:%s, base_fr:%dhz\n",
		__func__, cur_list->info->name, cur_list->info->base_fr);

	return 0;
}

static void lcd_vmode_add_duration(struct lcd_vmode_info_s *vmode, struct lcd_detail_timing_s *dtm)
{
	unsigned int n = 0, i;//, fr, ht, vt;
	unsigned long long qms_fr_min, qms_fr_max;

	memset(vmode->duration, 0, sizeof(struct lcd_duration_s) * LCD_DURATION_MAX);

	qms_fr_min = div_around(div_around(dtm->pclk_min, dtm->v_period_max), dtm->h_period_max);
	qms_fr_max = div_around(div_around(dtm->pclk_max, dtm->v_period_min), dtm->h_period_min);

	//if (dtm->fixed_type != 0xff) {
	//	for (i = 0; i < 8; i++) {
	//		if (dtm->fixed_val_set[i] == 0)
	//			break;
	//		if (n >= LCD_DURATION_MAX)
	//			break;
	//		fr = (dtm->fixed_type == 0) ? dtm->fixed_val_set[i] : dtm->pixel_clk;
	//		ht = (dtm->fixed_type == 1) ? dtm->fixed_val_set[i] : dtm->h_period;
	//		vt = (dtm->fixed_type == 2) ? dtm->fixed_val_set[i] : dtm->v_period;
	//		vmode->duration[n].frame_rate = (fr / ht) / vt;
	//		vmode->duration[n].duration_num = fr;
	//		vmode->duration[n].duration_den = ht * vt;
	//		vmode->duration[n].frac = 1;
	//		n++;
	//	}
	//}

	for (i = 0; i < ARRAY_SIZE(lcd_std_fr); i++) {
		if (lcd_std_fr[i].frame_rate == 0)
			break;
		if (lcd_std_fr[i].frame_rate > qms_fr_max)
			continue;
		if (lcd_std_fr[i].frame_rate < qms_fr_min)
			break;
		if (vmode->base_fr * lcd_std_fr[i].duration_den == lcd_std_fr[i].duration_num)
			continue;

		if (n >= LCD_DURATION_MAX)
			break;
		vmode->duration[n].frame_rate = lcd_std_fr[i].frame_rate;
		vmode->duration[n].duration_num = lcd_std_fr[i].duration_num;
		vmode->duration[n].duration_den = lcd_std_fr[i].duration_den;
		vmode->duration[n].frac = lcd_std_fr[i].frac;
		n++;
	}
	vmode->duration_cnt = n;
}

//--general mode: (h)x(v)p(frame_rate)hz
static struct lcd_vmode_info_s *lcd_detail_timing_to_vmode(struct lcd_detail_timing_s *ptiming)
{
	struct lcd_vmode_info_s *vmode_find = NULL;

	vmode_find = malloc(sizeof(struct lcd_vmode_info_s));
	if (!vmode_find)
		return NULL;
	memset(vmode_find, 0, sizeof(struct lcd_vmode_info_s));
	vmode_find->width = ptiming->h_active;
	vmode_find->height = ptiming->v_active;
	vmode_find->base_fr = ptiming->frame_rate;

	str_add_vmode(vmode_find->name, vmode_find, 0);

	vmode_find->dft_timing = ptiming;
	lcd_vmode_add_duration(vmode_find, ptiming);

	return vmode_find;
}

static int lcd_vmode_remove_all(struct aml_lcd_drv_s *pdrv)
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

static void lcd_tablet_add_all_vmode(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_info_s *vmode_find = NULL;
	struct lcd_detail_timing_s **timing_match;
	int i;

	if (!pdrv)
		return;

	lcd_vmode_remove_all(pdrv);

	vmode_find = lcd_detail_timing_to_vmode(&pdrv->config.timing.dft_timing);
	if (!vmode_find)
		return;
	lcd_vmode_add_single(pdrv, vmode_find);

	timing_match = lcd_cus_ctrl_timing_match_get(pdrv);
	if (timing_match) {
		for (i = 0; i < pdrv->config.cus_ctrl.timing_cnt; i++) {
			if (!timing_match[i])
				break;
			vmode_find = lcd_detail_timing_to_vmode(timing_match[i]);
			if (!vmode_find)
				continue;
			lcd_vmode_add_single(pdrv, vmode_find);
		}
		free(timing_match);
	}
}

/* ************************************************** *
 * vout server api
 * **************************************************
 */
static void lcd_list_support_timing(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_vmode_list_s *temp_list = pdrv->vmode_mgr.vmode_list_header;
	unsigned int frame_rate;
	int i;
	char mode_buf[32];

	while (temp_list) {
		if (!temp_list->info)
			break;
		memset(mode_buf, 0, 32 * sizeof(char));
		str_add_vmode(mode_buf, temp_list->info, temp_list->info->base_fr);
		printf("%s\n", mode_buf);

		for (i = 0; i < LCD_DURATION_MAX; i++) {
			frame_rate = temp_list->info->duration[i].frame_rate;
			if (frame_rate == 0)
				break;
			memset(mode_buf, 0, 32 * sizeof(char));
			str_add_vmode(mode_buf, temp_list->info, frame_rate);
			printf("%s\n", mode_buf);
		}
		temp_list = temp_list->next;
	}
}

char *get_current_env_connector(unsigned char cnt_idx)
{
	char cnt_name[20];

	sprintf(cnt_name, "connector%hu_type", cnt_idx);

	return env_get(cnt_name);
}

void sprintf_lcd_connector(char *buf, unsigned char lcd_idx, unsigned char lcd_type)
{
	char *connector_name_list[5] = {"LVDS", "VBYONE", "MIPI", "EDP", "NULL"};
	unsigned char name_idx;

	if (lcd_type == LCD_LVDS || lcd_type == LCD_MLVDS)
		name_idx = 0;
	else if (lcd_type == LCD_VBYONE || lcd_type == LCD_P2P)
		name_idx = 1;
	else if (lcd_type == LCD_MIPI)
		name_idx = 2;
	else if (lcd_type == LCD_EDP)
		name_idx = 3;
	else
		name_idx = 4;

	sprintf(buf, "%s-%c", connector_name_list[name_idx], 'A' + lcd_idx);
}

static int lcd_tablet_connector_check(struct aml_lcd_drv_s *pdrv, char *mode)
{
	char lcd_to_cnt_name[12], opt_mode_name[12] = "outputmode\0";
	char timing_name[32];
	unsigned char i, j;
	char *cnt_type_name, *outputmode;
	struct lcd_vmode_list_s *temp_list = pdrv->vmode_mgr.vmode_list_header;

	sprintf_lcd_connector(lcd_to_cnt_name, pdrv->index, pdrv->config.basic.lcd_type);

	for (i = 0; i < 3; i++) {
		cnt_type_name = get_current_env_connector(i);
		if (!cnt_type_name)
			continue;

		if (i)
			opt_mode_name[10] = '1' + i;
		outputmode = env_get(opt_mode_name);

		if (!strcmp(lcd_to_cnt_name, cnt_type_name) && outputmode) {
			pdrv->viu_sel = i + 1;
			goto check_connector_suggested_timing;
		} else if (!strcmp(lcd_to_cnt_name, cnt_type_name)) {
			LCDPR("[%d]: %s: connector[%hu] to lcd(%s)\n",
				pdrv->index, __func__, i, cnt_type_name);
			return 0;
		}

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: connector[%hu]: %s\n",
				pdrv->index, __func__, i, cnt_type_name);
		}
	}

	LCDERR("[%d]: %s: no connector to lcd: %s\n", pdrv->index, __func__, lcd_to_cnt_name);
	return -1;

check_connector_suggested_timing:
	while (temp_list) {
		memset(timing_name, 0, 32);
		str_add_vmode(timing_name, temp_list->info, temp_list->info->base_fr);
		if (strcmp(mode, timing_name) == 0) {
			temp_list->info->duration_index = 0xff;
			goto connector_suggested_timing_matched;
		}

		for (j = 0; j < LCD_DURATION_MAX; j++) {
			if (temp_list->info->duration[j].frame_rate == 0)
				break;
			memset(timing_name, 0, 32);
			str_add_vmode(timing_name, temp_list->info,
					temp_list->info->duration[j].frame_rate);
			if (strcmp(mode, timing_name))
				continue;
			temp_list->info->duration_index = j;

			goto connector_suggested_timing_matched;
		}
		temp_list = temp_list->next;
	}

	if (pdrv->mode == LCD_MODE_TABLET) {
		pdrv->vmode_mgr.next_vmode_info = NULL;
		LCDPR("[%d]: %s: connector[%hu] to lcd(%s), suggested timing: %s(not match)\n",
			pdrv->index, __func__, i, cnt_type_name, outputmode);
		return 0;
	}

	LCDPR("[%d]: %s: connector[%hu] to lcd(%s), timing: %s(wrong)\n",
		pdrv->index, __func__, i, cnt_type_name, outputmode);
	return -1;

connector_suggested_timing_matched:
	if (pdrv->vmode_mgr.cur_vmode_info != temp_list->info)
		pdrv->vmode_mgr.next_vmode_info = temp_list->info;

	LCDPR("[%d]: %s: connector[%hu] to lcd(%s), suggested timing: %s\n",
		pdrv->index, __func__, i, cnt_type_name, outputmode);
	return 0;
}

static void lcd_vmode_update(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming;
	unsigned char dur_index;

	if (pdrv->vmode_mgr.next_vmode_info) {
		pdrv->vmode_mgr.cur_vmode_info = pdrv->vmode_mgr.next_vmode_info;
		pdrv->vmode_mgr.next_vmode_info = NULL;

		pdrv->std_duration = pdrv->vmode_mgr.cur_vmode_info->duration;
		ptiming = pdrv->vmode_mgr.cur_vmode_info->dft_timing;
		memcpy(&pdrv->config.timing.base_timing, ptiming,
			sizeof(struct lcd_detail_timing_s));
		lcd_cus_ctrl_config_update(pdrv, (void *)ptiming, LCD_CUS_CTRL_SEL_TIMMING);

		//update base_timing to act_timing
		lcd_enc_timing_init_config(pdrv);
	}

	if (!pdrv->vmode_mgr.cur_vmode_info || !pdrv->std_duration) {
		LCDERR("[%d]: %s: cur_vmode_info or std_duration is null\n",
			pdrv->index, __func__);
		return;
	}
	dur_index = pdrv->vmode_mgr.cur_vmode_info->duration_index;

	if (dur_index != 0xff) {
		pdrv->config.timing.act_timing.sync_duration_num =
			pdrv->std_duration[dur_index].duration_num;
		pdrv->config.timing.act_timing.sync_duration_den =
			pdrv->std_duration[dur_index].duration_den;
		pdrv->config.timing.act_timing.frac =
			pdrv->std_duration[dur_index].frac;
		pdrv->config.timing.act_timing.frame_rate =
			pdrv->std_duration[dur_index].frame_rate;
	} else {
		pdrv->config.timing.act_timing.sync_duration_num =
			pdrv->vmode_mgr.cur_vmode_info->base_fr;
		pdrv->config.timing.act_timing.sync_duration_den = 1;
		pdrv->config.timing.act_timing.frac = 0;
		pdrv->config.timing.act_timing.frame_rate =
			pdrv->vmode_mgr.cur_vmode_info->base_fr;
	}
	lcd_frame_rate_change(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %dx%d, duration=%d:%d, dur_index=%d\n", pdrv->index, __func__,
			pdrv->config.timing.act_timing.h_active,
			pdrv->config.timing.act_timing.v_active,
			pdrv->config.timing.act_timing.sync_duration_num,
			pdrv->config.timing.act_timing.sync_duration_den,
			dur_index);
	}
}

static int lcd_config_valid(struct aml_lcd_drv_s *pdrv, char *mode)
{
	int ret;

	ret = lcd_tablet_connector_check(pdrv, mode);
	if (ret < 0)
		return -1;

	lcd_vmode_update(pdrv);

	return 0;
}

static void lcd_config_init(struct aml_lcd_drv_s *pdrv)
{
	lcd_enc_timing_init_config(pdrv);
	lcd_clk_generate_parameter(pdrv);
	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */
	lcd_tablet_add_all_vmode(pdrv);
}

int lcd_mode_tablet_init(struct aml_lcd_drv_s *pdrv)
{
	pdrv->list_support_mode = lcd_list_support_timing;
	pdrv->outputmode_check = lcd_tablet_connector_check;
	pdrv->config_valid = lcd_config_valid;
	pdrv->driver_init_pre = lcd_tablet_driver_init_pre;
	pdrv->driver_init = lcd_tablet_driver_init;
	pdrv->driver_disable = lcd_tablet_driver_disable;

	lcd_config_init(pdrv);
	pdrv->probe_done = 1;

	return 0;
}
