// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "lcd_tcon.h"

/*
 * mode: 0-normal mode, 1-fix mode
 */
void tcon_fr_detect_config(struct aml_lcd_drv_s *pdrv, unsigned int mode,
			unsigned int fr_levels[], unsigned int num_level,
			unsigned int step_counter)
{
	unsigned int h_blank = 0, i = 0, nvfp = 0, pclk, ht, vt = 0;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int levels[FR_DETECT_LEVEL_MAX];

	if (mode == 0 && num_level == 0)
		return;
	if (num_level > 20)
		num_level = 20;

	pclk = pdrv->config.timing.base_timing->pixel_clk;
	ht = pdrv->config.timing.base_timing->h_period;

	h_blank = pconf->timing.base_timing->h_period - pconf->timing.base_timing->h_active;
	nvfp = pconf->timing.base_timing->v_active + pconf->timing.base_timing->vsync_bp +
		pconf->timing.base_timing->vsync_width;

	for (i = 0; i < num_level; i++) {
		vt = pclk / ht / fr_levels[i];
		levels[i] = vt - nvfp - 1; //vfp
	}

	lcd_tcon_setb(0x232, mode, 14, 1);//0=normal mode, calculate frame rate and trig intr

	lcd_tcon_setb(0x232, h_blank - 1, 15, 10);// ht - hact - 1
	if (mode == 1) { //fix mode only set step_counter
		lcd_tcon_setb(0x233, step_counter,  0, 13);
		return;
	}

	for (i = 0; i < num_level / 2; i++) {
		// frame rate level0[12:0] / level1[28:16]
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 0],  0, 13);
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 1], 16, 13);
	}

	if (num_level & 0x1)
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 0],  0, 13);
}

void tcon_fr_detect_enable(struct aml_lcd_drv_s *pdrv, int enable)
{
	if (enable)
		lcd_tcon_setb(0x232, 1,  13, 1);
	else
		lcd_tcon_setb(0x232, 0,  13, 1);
}

void lcd_tcon_data_parse_vrr(struct lcd_tcon_vrr_data_s *vrr,
				unsigned char *p, unsigned int data_cnt, unsigned int data_width)
{
	unsigned int d = 0, m = 0, temp = 0, k;

	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_h_active = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_v_active = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_frame_rate_min = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_frame_rate_max = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->part = temp;

	if (vrr->part >= FR_DETECT_LEVEL_MAX) {
		vrr->part = 0;
		return;
	}
	for (k = 0; k < vrr->part; k++) {
		m += data_width;
		for (d = 0, temp = 0;  d < data_width; d++)
			temp |= (p[m + d] << (d * 8));
		vrr->fr_level[k] = temp;
	}
	vrr->fr_count = vrr->part;
	for (k = vrr->fr_count; k < 20; k++)
		vrr->fr_level[k] = 0x1fff;
	vrr->support = 1;
}

int lcd_tcon_vrr_fr_sw_match(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_vrr_data_s *vrr)
{
	int i = 0, fr;

	fr = pdrv->config.timing.act_timing.frame_rate;

	for (i = 0; i < vrr->fr_count; i++)
		if (fr >= vrr->fr_level[i])
			break;

	return (i >= vrr->part ? 0 : i);
}