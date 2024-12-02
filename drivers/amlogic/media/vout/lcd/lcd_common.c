// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "env.h"
#include <command.h>

int strnum_get_num(const char *str, struct num_str_s *arr, int size_arr, int dft)
{
	int i = 0;

	if (!str || !arr)
		return dft;

	for (i = 0; i < size_arr; i++) {
		if (strcmp(str, arr[i].str) == 0)
			return arr[i].num;
	}
	return dft;
}

char *strnum_get_str(int num, struct num_str_s *arr, int size_arr, char *dft)
{
	int i = 0;

	if (!arr)
		return dft;

	for (i = 0; i < size_arr; i++) {
		if (num == arr[i].num)
			return arr[i].str;
	}
	return dft;
}

void mem_dump(unsigned char *addr, int size)
{
	int i = 0, j = 0, len = 0;
	char buf[128];

	for (j = 0; j < (size >> 4); j++) {
		for (i = 0, len = 0; i < 16; i++)
			len += sprintf(buf + len, "%02x ", (unsigned int)addr[j * 16 + i]);
		printf("0x%04x: %s\n", j * 16,  buf);
	}
	if (size & 0xf) {
		for (i = 0, len = 0; i < (size & 0xf); i++)
			len += sprintf(buf + len, "%02x ", (unsigned int)addr[j * 16 + i]);
		printf("0x%04x: %s\n", j * 16,  buf);
	}
}

__maybe_unused int string_to_numbers(const char *str, unsigned int nums[])
{
	int item_ind = 0, i = 0;
	char *token = NULL;
	char *tmp_buf = NULL;
	int str_len;

	if (!str)
		return 0;

	str_len = strlen(str);
	tmp_buf = (char *)malloc(str_len + 2);
	if (!tmp_buf)
		return 0;

	strcpy(tmp_buf, str);
	tmp_buf[str_len] = '\0';
	tmp_buf[str_len  + 1] = '\0';
	token = tmp_buf;
	while (i <= str_len) {
		if (tmp_buf[i] == ',' || i == str_len) {
			while (*token <= ' ')
				token++;
			tmp_buf[i] = '\0';
			nums[item_ind++] = strtoul(token, NULL, 0);
			token = tmp_buf + i + 1;
		}
		i++;
	}
	free(tmp_buf);
	return item_ind;
}

int path_name_compose(const char *path, const char *name, char *path_name)
{
	char *p1;
	const char *p2;
	int len1, len2, len, back = 0, k;

	if (!path || !name || !path_name)
		return -1;

	p2 = name;
	len2 = strlen(name);
	if (name[0] == '/') {//absolute path, ignore path
		strcpy(path_name, name);
		path_name[len2 + 1] = '\0';
		return 0;
	} else if (name[0] == '.' && name[1] == '/') {
		back = 0;
		p2 += 2;
	} else if (p2[0] == '.' && p2[1] == '.' && p2[2] == '/') {
		while (len2 > 0 && p2[0] == '.' && p2[1] == '.' && p2[2] == '/') {
			p2 += 3;
			len2 -= 3;
			back++;
		}
	}

	if (len2 <= 0) {
		path_name[0] = '\0';
		return -1;
	}

	p1 = path_name;
	len1 = strlen(path);
	len = len1;
	memcpy(path_name, path, len);
	path_name[len] = '\0';
	if (path_name[len - 1] != '/') {
		path_name[len] = '/';
		len += 1;
		path_name[len] = '\0';
	}
	back += 1;

	for (k = len - 1; k > 0; k--) {
		if (p1[k] == '/')
			back--;
		if (back == 0) {
			memcpy(p1 + k + 1, p2, len2);
			len = k + len2 + 1;
			p1[len] = '\0';
			return 0;
		}
	}
	return -1;
}

void lcd_detail_timing_print(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *dt)
{
	s32 herr, verr;
	char *ck[3] = {"(x)", "(!)", ""};
	char *ck_hbp, *ck_hfp, *ck_vbp, *ck_vfp;

	herr = dt->check_status & 0xf;
	verr = (dt->check_status >> 4) & 0xf;
	ck_hbp = (herr & 0x4) ? ck[0] : (herr & 0x8) ? ck[1] : ck[2];
	ck_hfp = (herr & 0x1) ? ck[0] : (herr & 0x2) ? ck[1] : ck[2];
	ck_vbp = (verr & 0x4) ? ck[0] : (verr & 0x8) ? ck[1] : ck[2];
	ck_vfp = (verr & 0x1) ? ck[0] : (verr & 0x2) ? ck[1] : ck[2];

	printf("ht:%4d(%4d ~ %4d), hact:%4d hbp:%d%s, hsw:%2d, hfp:%3d%s, hpol:%d\n"
	       "vt:%4d(%4d ~ %4d), vact:%4d vbp:%d%s, vsw:%2d, vfp:%3d%s, vpol:%d\n"
	       "lcd_bits:%d, cfmt:%d, fr_adj_type:%d, switch_type:0x%x\n"
	       "ss_level:%d, ss_mode:%d, ss_freq:%d, ss_force:%d\n"
	       "pclk:%d(%d ~ %d)\n"
	       "frame_rate:%d (%d ~ %d)\n"
	       "vrr_range:[%d ~ %d]\n\n",
	       dt->h_period, dt->h_period_min, dt->h_period_max, dt->h_active,
	       dt->hsync_bp, ck_hbp, dt->hsync_width, dt->hsync_fp, ck_hfp, dt->hsync_pol,
	       dt->v_period, dt->v_period_min, dt->v_period_max, dt->v_active,
	       dt->vsync_bp, ck_vbp, dt->vsync_width, dt->vsync_fp, ck_vfp, dt->vsync_pol,
	       dt->lcd_bits, dt->cfmt, dt->fr_adjust_type, dt->switch_type,
	       dt->ss_level, dt->ss_mode, dt->ss_freq, dt->ss_force,
	       dt->pixel_clk, dt->pclk_min, dt->pclk_max,
	       dt->frame_rate, dt->frame_rate_min, dt->frame_rate_max,
	       dt->vfreq_vrr_min, dt->vfreq_vrr_max);
}

void lcd_phy_cfg_print(struct phy_config_s *cfg)
{
	int m, n, i;

	printf("flag: 0x%x, lane_num: %d, nphys: %d\n"
	       "swap0: 0x%08x, swap1: 0x%08x, lane ofst: %d, mask: 0x%x, valid: 0x%x\n"
	       "ckdi: 0x%x, weakly_pd:0x%x, low_com:0x%x\n",
	       cfg->flag, cfg->lane_num, cfg->group_num,
	       cfg->ch_swap0, cfg->ch_swap1,
	       cfg->lane_offset, cfg->lane_mask, cfg->lane_valid,
	       cfg->ckdi, cfg->weakly_pull_down, cfg->low_common_mode);

	m = (cfg->lane_num + 1) / 2;
	n = m;
	printf("lane  sel   phase_sel    lane  sel   phase_sel\n");
	for (i = 0; i < m; i++, n++) {
		printf("[%2d]  0x%02x  0x%02x         [%02d]  0x%02x  0x%02x\n",
		       i, cfg->ch_ctrl[i].sel, cfg->ch_ctrl[i].phase_sel,
		       n, cfg->ch_ctrl[n].sel, cfg->ch_ctrl[n].phase_sel);
	}
	printf("\n");
}

void lcd_phy_attr_print(struct phy_attr_s *phy, u32 lane_num)
{
	int m, n, i;
	struct phy_lane_s *lane;

	printf("cv_mode:%d, vswing:0x%x, vcm:0x%x, odt:0x%x, ref_bias:0x%x\n"
	       "phy_clk:%d, clk_phase:%d, ss_level:%d, ss_freq:%d, ss_mode:%d\n",
	       phy->cv_mode, phy->vswing, phy->vcm, phy->odt, phy->ref_bias,
	       phy->phy_clk, phy->clk_phase, phy->ss.level, phy->ss.freq, phy->ss.mode);

	m = (lane_num + 1) / 2;
	n = m;
	lane = phy->lane;

	printf("lane  amp   preem     lane  amp   preem\n");
	for (i = 0; i < m; i++, n++) {
		printf("[%2d]  0x%02x  0x%02x      [%2d]  0x%02x  0x%02x\n",
		       i, lane[i].amp, lane[i].preem, n, lane[n].amp, lane[n].preem);
	}

	printf("\n");
}

//ret: bit[0]:hfp: fatal error, block driver
//     bit[1]:hfp: warning, only print warning message
//     bit[2]:hswbp: fatal error, block driver
//     bit[3]:hswbp: warning, only print warning message
//     bit[4]:vfp: fatal error, block driver
//     bit[5]:vfp: warning, only print warning message
//     bit[6]:vswbp: fatal error, block driver
//     bit[7]:vswbp: warning, only print warning message
unsigned int lcd_config_timing_check(struct aml_lcd_drv_s *pdrv,
				     struct lcd_detail_timing_s *ptiming)
{
	short hpw = ptiming->hsync_width;
	short hbp = ptiming->hsync_bp;
	short hfp = ptiming->hsync_fp;
	short vpw = ptiming->vsync_width;
	short vbp = ptiming->vsync_bp;
	short vfp = ptiming->vsync_fp;
	short hfp_min, vfp_min, vfp_cmpr_tail = 0, temp;
	char *ferr_str = NULL, *warn_str = NULL;
	int ferr_len = 0, warn_len = 0, ferr_left, warn_left;
	unsigned int ret = 0;

	ferr_str = malloc(PR_BUF_MAX);
	if (!ferr_str) {
		LCDERR("config_check fail for NOMEM\n");
		return 0;
	}
	memset(ferr_str, 0, PR_BUF_MAX);
	warn_str = malloc(PR_BUF_MAX);
	if (!warn_str) {
		LCDERR("config_check fail for NOMEM\n");
		free(ferr_str);
		return 0;
	}
	memset(warn_str, 0, PR_BUF_MAX);

	if (pdrv->config.basic.lcd_type == LCD_MLVDS ||
	    pdrv->config.basic.lcd_type == LCD_P2P) {
		vfp_cmpr_tail = 3;
	}

	if (hfp <= 0) {
		ferr_left = lcd_debug_info_len(ferr_len);
		ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
			"  hfp: %d, for panel, req: >0!!!\n", hfp);
		ret |= (1 << 0);
	}
	if (ptiming->h_period_min) {
		hfp_min = ptiming->h_period_min - ptiming->h_active - hpw - hbp;
		if (hfp_min <= 0) {
			warn_left = lcd_debug_info_len(warn_len);
			warn_len += snprintf(warn_str + warn_len, warn_left,
				"  hfp with h_period_min: %d, for panel, req: >0!!!\n",
				hfp_min);
			ret |= (1 << 1);
		}
	}

	if (vfp <= vfp_cmpr_tail) {
		ferr_left = lcd_debug_info_len(ferr_len);
		ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
			"  vfp: %d, for panel, req: >%d!!!\n", vfp, vfp_cmpr_tail);
		ret |= (1 << 4);
	}
	if (ptiming->v_period_min) {
		vfp_min = ptiming->v_period_min - ptiming->v_active - vpw - vbp;
		if (vfp_min <= vfp_cmpr_tail) {
			ferr_left = lcd_debug_info_len(ferr_len);
			ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
				"  vfp with v_period_min: %d, for panel, req: >%d!!!\n",
				vfp_min, vfp_cmpr_tail);
			ret |= (1 << 4);
		}
	}

	//display timing check
	//hswbp
	if (pdrv->disp_req.hswbp_vid == 0)
		goto lcd_config_timing_check_vid_hfp;
	temp = hpw + hbp;
	if (temp < pdrv->disp_req.hswbp_vid) {
		if (pdrv->disp_req.alert_level == 1) {
			warn_left = lcd_debug_info_len(warn_len);
			warn_len += snprintf(warn_str + warn_len, warn_left,
				"  hpw + hbp: %d, for display path, req: >=%d!\n",
				temp, pdrv->disp_req.hswbp_vid);
			ret |= (1 << 3);
		} else if (pdrv->disp_req.alert_level == 2) {
			ferr_left = lcd_debug_info_len(ferr_len);
			ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
				"  hpw + hbp: %d, for display path, req: >=%d!!!\n",
				temp, pdrv->disp_req.hswbp_vid);
			ret |= (1 << 2);
		}
	}

lcd_config_timing_check_vid_hfp:
	//hfp
	if (pdrv->disp_req.hfp_vid == 0)
		goto lcd_config_timing_check_vid_vswbp;
	if (hfp < pdrv->disp_req.hfp_vid) {
		if (pdrv->disp_req.alert_level == 1) {
			warn_left = lcd_debug_info_len(warn_len);
			warn_len += snprintf(warn_str + warn_len, warn_left,
				"  hfp: %d, for display path, req: >=%d!\n",
				hfp, pdrv->disp_req.hfp_vid);
			ret |= (1 << 5);
		} else if (pdrv->disp_req.alert_level == 2) {
			ferr_left = lcd_debug_info_len(ferr_len);
			ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
				"  hfp: %d, for display path, req: >=%d!!!\n",
				hfp, pdrv->disp_req.hfp_vid);
			ret |= (1 << 4);
		}
	}

lcd_config_timing_check_vid_vswbp:
	//vswbp
	if (pdrv->disp_req.vswbp_vid == 0)
		goto lcd_config_timing_check_vid_vfp;
	temp = vpw + vbp;
	if (temp < pdrv->disp_req.vswbp_vid) {
		if (pdrv->disp_req.alert_level == 1) {
			warn_left = lcd_debug_info_len(warn_len);
			warn_len += snprintf(warn_str + warn_len, warn_left,
				"  vpw + vbp: %d, for display path, req: >=%d!\n",
				temp, pdrv->disp_req.vswbp_vid);
			ret |= (1 << 7);
		} else if (pdrv->disp_req.alert_level == 2) {
			ferr_left = lcd_debug_info_len(ferr_len);
			ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
				"  vpw + vbp: %d, for display path, req: >=%d!!!\n",
				temp, pdrv->disp_req.vswbp_vid);
			ret |= (1 << 6);
		}
	}

lcd_config_timing_check_vid_vfp:
	//vfp
	if (pdrv->disp_req.vfp_vid == 0)
		goto lcd_config_timing_check_end;
	temp = pdrv->disp_req.vfp_vid + vfp_cmpr_tail;
	if (vfp < temp) {
		if (pdrv->disp_req.alert_level == 1) {
			warn_left = lcd_debug_info_len(warn_len);
			warn_len += snprintf(warn_str + warn_len, warn_left,
				"  vfp: %d, for display path, req: >=%d!\n",
				vfp, temp);
			ret |= (1 << 5);
		} else if (pdrv->disp_req.alert_level == 2) {
			ferr_left = lcd_debug_info_len(ferr_len);
			ferr_len += snprintf(ferr_str + ferr_len, ferr_left,
				"  vfp: %d, for display path, req: >=%d!!!\n",
				vfp, temp);
			ret |= (1 << 4);
		}
	}

lcd_config_timing_check_end:
	if (ret) {
		printf("**************** lcd config timing check ****************\n");
		if (ret & 0x55) {
			printf("lcd: FATAL ERROR:\n"
				"%s\n", ferr_str);
		}
		if (ret & 0xaa) {
			printf("lcd: WARNING:\n"
				"%s\n", warn_str);
		}
		printf("************** lcd config timing check end ****************\n");
	}
	memset(ferr_str, 0, PR_BUF_MAX);
	memset(warn_str, 0, PR_BUF_MAX);
	free(ferr_str);
	free(warn_str);

	ptiming->check_status = ret;

	return ret;
}

char *get_current_env_connector(unsigned char cnt_idx)
{
	char cnt_name[20];

	sprintf(cnt_name, "connector%hu_type", cnt_idx);

	return env_get(cnt_name);
}

void sprintf_lcd_connector(char *buf, unsigned char lcd_idx, unsigned char lcd_type)
{
	char *connector_name_list[5] = {"LVDS", "VBYONE", "MIPI", "EDP", "LCD"};
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

static unsigned int vbyone_lane_num[] = {
	1,
	2,
	4,
	8,
	8,
};

#define VBYONE_BIT_RATE_MAX		4100000000ULL //Hz
#define VBYONE_BIT_RATE_MIN		600000000
void lcd_vbyone_bit_rate_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int byte_mode, lane_count, minlane, phy_div;
	unsigned long long bit_rate, band_width;
	unsigned int temp, i;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	//auto calculate bandwidth, clock
	lane_count = pconf->control.vbyone_cfg.lane_count;
	byte_mode = pconf->control.vbyone_cfg.byte_mode;
	/* byte_mode * byte2bit * 8/10_encoding * pclk =
	   byte_mode * 8 * 10 / 8 * pclk */
	band_width = pconf->timing.act_timing.pixel_clk;
	band_width = byte_mode * 10 * band_width;

	temp = VBYONE_BIT_RATE_MAX;
	temp = lcd_do_div((band_width + temp - 1), temp);
	for (i = 0; i < 4; i++) {
		if (temp <= vbyone_lane_num[i])
			break;
	}
	minlane = vbyone_lane_num[i];
	if (lane_count < minlane) {
		LCDERR("vbyone lane_num(%d) is less than min(%d), change to min lane_num\n",
			lane_count, minlane);
		lane_count = minlane;
		pconf->control.vbyone_cfg.lane_count = lane_count;
	}

	bit_rate = lcd_do_div(band_width, lane_count);
	phy_div = lane_count / lane_count;
	if (phy_div == 8) {
		phy_div /= 2;
		bit_rate = lcd_do_div(bit_rate, 2);
	}
	if (bit_rate > VBYONE_BIT_RATE_MAX) {
		LCDERR("vbyone bit rate(%lldHz) is out of max(%lldHz)\n",
			bit_rate, VBYONE_BIT_RATE_MAX);
	}
	if (bit_rate < VBYONE_BIT_RATE_MIN) {
		LCDERR("vbyone bit rate(%lldHz) is out of min(%dHz)\n",
			bit_rate, VBYONE_BIT_RATE_MIN);
	}

	pconf->control.vbyone_cfg.phy_div = phy_div;
	pconf->timing.bit_rate = bit_rate;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("lane_count=%u, bit_rate = %lluHz, pclk=%uhz\n",
			lane_count, bit_rate, pconf->timing.act_timing.pixel_clk);
	}
}

void lcd_mlvds_bit_rate_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned long long bit_rate, band_width;
	unsigned int lcd_bits, channel_num;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lcd_bits = pconf->timing.act_timing.lcd_bits;
	channel_num = pconf->control.mlvds_cfg.channel_num;
	band_width = pconf->timing.act_timing.pixel_clk;
	band_width = lcd_bits * band_width;
	bit_rate = lcd_do_div(band_width, channel_num);
	pconf->timing.bit_rate = bit_rate;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("channel_num=%u, bit_rate=%lluHz, pclk=%uhz\n",
		      channel_num, bit_rate, pconf->timing.act_timing.pixel_clk);
	}
}

void lcd_p2p_bit_rate_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int p2p_type, lcd_bits, lane_num, clk_mode;
	unsigned long long bit_rate, band_width;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lcd_bits = pconf->timing.act_timing.lcd_bits;
	lane_num = pconf->control.p2p_cfg.lane_num;
	band_width = pconf->timing.act_timing.pixel_clk;
	p2p_type = pconf->control.p2p_cfg.p2p_type & 0x1f;
	clk_mode = pconf->timing.clk_mode;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("clk_mode=%d (%s)\n", clk_mode,
			(clk_mode == LCD_CLK_MODE_DEPENDENCE) ?
			"dependence" : "independence");
	}
	switch (p2p_type) {
	case P2P_CEDS:
	case P2P_EPI: /*24to28*/
		if (clk_mode == LCD_CLK_MODE_DEPENDENCE)
			band_width = band_width * lcd_bits;
		else //independence & dependence_adapt
			band_width = band_width * (lcd_bits + 4);
		break;
	case P2P_CHPI: /* 8/10 coding */
		band_width = lcd_do_div((band_width * lcd_bits * 10), 8);
		break;
	case P2P_CSPI:
	case P2P_ISP:
	case P2P_CMPI: /*24to27*/
		if (clk_mode == LCD_CLK_MODE_DEPENDENCE) {
			band_width = band_width * lcd_bits;
		} else { //independence & dependence_adapt
			/* 8/9 coding */
			band_width = lcd_do_div((band_width * lcd_bits * 9), 8);
		}
		break;
	case P2P_USIT: /*9to10*/
		if (clk_mode == LCD_CLK_MODE_DEPENDENCE)
			band_width = band_width * lcd_bits;
		else //independence & dependence_adapt
			band_width = lcd_do_div((band_width * lcd_bits * 10), 9);
		break;
	default:
		band_width = band_width * lcd_bits;
		break;
	}
	bit_rate = lcd_do_div(band_width, lane_num);
	pconf->timing.bit_rate = bit_rate;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: lane_num=%u, lcd_bits:%d bit_rate=%lluHz, pclk=%uhz\n",
		      pdrv->index, lane_num, lcd_bits,
		      bit_rate, pconf->timing.act_timing.pixel_clk);
	}
}

void lcd_mipi_dsi_bit_rate_config(struct aml_lcd_drv_s *pdrv)
{
	// none
}

void lcd_edp_bit_rate_config(struct aml_lcd_drv_s *pdrv)
{
	//todo
}

struct lcd_detail_timing_s *lcd_timing_alloc(struct aml_lcd_drv_s *pdrv)
{
	int n;
	struct lcd_detail_timing_s *dt;

	if (!pdrv || pdrv->config.timing.num_timings >= LCD_MAX_NUM_TIMINGS)
		return NULL;

	n = pdrv->config.timing.num_timings;
	if (n < LCD_MAX_NUM_TIMINGS) {
		dt = (struct lcd_detail_timing_s *)malloc(sizeof(*dt));
		if (!dt)
			return NULL;
		pdrv->config.timing.timings[n] = dt;
		pdrv->config.timing.num_timings++;
		return dt;
	}

	return NULL;
}

void lcd_timing_free_last(struct aml_lcd_drv_s *pdrv)
{
	if (!pdrv || pdrv->config.timing.num_timings <= 0)
		return;

	free(pdrv->config.timing.timings[pdrv->config.timing.num_timings - 1]);
	pdrv->config.timing.timings[pdrv->config.timing.num_timings - 1] = NULL;
	pdrv->config.timing.num_timings--;
	if (pdrv->config.timing.num_timings == 0) {
		pdrv->config.timing.dft_timing = NULL;
		pdrv->config.timing.base_timing = NULL;
	}
}

struct phy_attr_s *lcd_phy_alloc(struct aml_lcd_drv_s *pdrv)
{
	int n;
	struct phy_attr_s *phy;

	if (!pdrv || pdrv->config.phy_cfg.group_num >= MAX_NUM_PHY_CFGS)
		return NULL;

	n = pdrv->config.phy_cfg.group_num;
	if (n < MAX_NUM_PHY_CFGS) {
		phy = (struct phy_attr_s *)malloc(sizeof(*phy));
		if (!phy)
			return NULL;
		pdrv->config.phy_cfg.phys[n] = phy;
		pdrv->config.phy_cfg.group_num++;
		return phy;
	}

	return NULL;
}

void lcd_phy_free_last(struct aml_lcd_drv_s *pdrv)
{
	if (!pdrv || pdrv->config.phy_cfg.group_num <= 0)
		return;

	free(pdrv->config.phy_cfg.phys[pdrv->config.phy_cfg.group_num - 1]);
	pdrv->config.phy_cfg.phys[pdrv->config.phy_cfg.group_num - 1] = NULL;
	pdrv->config.phy_cfg.group_num--;
	if (pdrv->config.phy_cfg.group_num == 0)
		pdrv->config.phy_cfg.act_phy = NULL;
}

static void lcd_fr_range_update(struct lcd_detail_timing_s *ptiming)
{
	unsigned int htotal, vmin, vmax, hfreq;
	unsigned long long temp;
	int i = 1;

	ptiming->h_period_min = ptiming->h_period_min ? ptiming->h_period_min : ptiming->h_period;
	ptiming->h_period_max = ptiming->h_period_max ? ptiming->h_period_max : ptiming->h_period;
	ptiming->v_period_min = ptiming->v_period_min ? ptiming->v_period_min : ptiming->v_period;
	ptiming->v_period_max = ptiming->v_period_max ? ptiming->v_period_max : ptiming->v_period;
	temp = ptiming->pixel_clk;
	temp *= 10;
	htotal = ptiming->h_period;
	vmin = ptiming->v_period_min;
	vmax = ptiming->v_period_max;
	hfreq = lcd_do_div(temp, htotal);
	if (vmin > 0)
		ptiming->vfreq_vrr_max = ((hfreq / vmin) + 5) / 10;
	if (vmax > 0)
		ptiming->vfreq_vrr_min = ((hfreq / vmax) + 5) / 10;
	if (ptiming->frame_rate_max == 0) {
		ptiming->frame_rate_max = ptiming->vfreq_vrr_max;
		if (ptiming->frame_rate_max == 0)
			ptiming->frame_rate_max = ptiming->frame_rate;
	}
	if (ptiming->frame_rate_min == 0) {
		ptiming->frame_rate_min = ptiming->vfreq_vrr_min;
		if (ptiming->frame_rate_min == 0) {
			ptiming->frame_rate_min = ptiming->frame_rate_max / 2 + 10;
		} else {
			while ((ptiming->frame_rate_min * 2 * i) < ptiming->frame_rate_max) {
				ptiming->frame_rate_min = ptiming->frame_rate_min * 2 * i;
				i++;
			}
		}
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: pclk=%u, h_period=%d, range: v_period(%d %d), vrr(%d %d), fr(%d %d)\n",
			__func__, ptiming->pixel_clk, ptiming->h_period,
			ptiming->v_period_min, ptiming->v_period_max,
			ptiming->vfreq_vrr_min, ptiming->vfreq_vrr_max,
			ptiming->frame_rate_min, ptiming->frame_rate_max);
	}
}

void lcd_clk_frame_rate_init(struct lcd_detail_timing_s *ptiming)
{
	unsigned int sync_duration, h_period, v_period;
	unsigned long long temp;

	if (ptiming->pixel_clk == 0) /* default 0 for 60hz */
		ptiming->pixel_clk = 60;

	h_period = ptiming->h_period;
	v_period = ptiming->v_period;
	if (ptiming->pixel_clk < 500) { /* regard as frame_rate */
		sync_duration = ptiming->pixel_clk;
		ptiming->pixel_clk = sync_duration * h_period * v_period;
		ptiming->frame_rate = sync_duration;
		ptiming->sync_duration_num = sync_duration;
		ptiming->sync_duration_den = 1;
		ptiming->frac = 0;
	} else { /* regard as pixel clock */
		temp = ptiming->pixel_clk;
		temp *= 1000;
		sync_duration = lcd_do_div(temp, (v_period * h_period));
		ptiming->frame_rate = sync_duration / 1000;
		ptiming->sync_duration_num = sync_duration;
		ptiming->sync_duration_den = 1000;
		ptiming->frac = 0;
	}

	lcd_fr_range_update(ptiming);
}

void lcd_default_to_basic_timing_init_config(struct aml_lcd_drv_s *pdrv)
{
	pdrv->config.timing.base_timing = pdrv->config.timing.dft_timing;
}

//act_timing as enc_timing
void lcd_enc_timing_init_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_detail_timing_s *ptiming;
	unsigned short h_period, v_period, h_active, v_active;
	unsigned short hsync_bp, hsync_width, vsync_bp, vsync_width;
	unsigned short de_hstart, de_vstart;
	unsigned short hstart, hend, vstart, vend;
	unsigned short h_delay = 0;

	switch (pconf->basic.lcd_type) {
	case LCD_RGB:
		h_delay = RGB_DELAY;
		break;
	default:
		h_delay = 0;
		break;
	}

	if (!pdrv->config.timing.base_timing)
		return;

	ptiming = &pdrv->config.timing.act_timing;
	memcpy(ptiming, pdrv->config.timing.base_timing, sizeof(struct lcd_detail_timing_s));
	if (pconf->timing.ppc == 0)
		pconf->timing.ppc = 1;
	pconf->timing.enc_clk = pconf->timing.act_timing.pixel_clk / pconf->timing.ppc;
	if (pdrv->config.timing.ppc > 1) {
		LCDPR("[%d]: %s: ppc=%d, pixel_clk=%d, enc_clk=%d\n",
		      pdrv->index, __func__,
		      pdrv->config.timing.ppc,
		      pconf->timing.act_timing.pixel_clk,
		      pconf->timing.enc_clk);
	}

	h_period = ptiming->h_period;
	v_period = ptiming->v_period;
	h_active = ptiming->h_active;
	v_active = ptiming->v_active;
	hsync_bp = ptiming->hsync_bp;
	hsync_width = ptiming->hsync_width;
	vsync_bp = ptiming->vsync_bp;
	vsync_width = ptiming->vsync_width;

	de_hstart = hsync_bp + hsync_width;
	de_vstart = vsync_bp + vsync_width;

	pconf->timing.hstart = de_hstart - h_delay;
	pconf->timing.vstart = de_vstart;
	pconf->timing.hend = h_active + pconf->timing.hstart - 1;
	pconf->timing.vend = v_active + pconf->timing.vstart - 1;

	pconf->timing.de_hs_addr = de_hstart;
	pconf->timing.de_he_addr = de_hstart + h_active;
	pconf->timing.de_vs_addr = de_vstart;
	pconf->timing.de_ve_addr = de_vstart + v_active - 1;

	hstart = (de_hstart + h_period - hsync_bp - hsync_width) % h_period;
	hend = (de_hstart + h_period - hsync_bp) % h_period;
	pconf->timing.hs_hs_addr = hstart;
	pconf->timing.hs_he_addr = hend;
	pconf->timing.hs_vs_addr = 0;
	pconf->timing.hs_ve_addr = v_period - 1;

	pconf->timing.vs_hs_addr = (hstart + h_period) % h_period;
	pconf->timing.vs_he_addr = pconf->timing.vs_hs_addr;
	vstart = (de_vstart + v_period - vsync_bp - vsync_width) % v_period;
	vend = (de_vstart + v_period - vsync_bp) % v_period;
	pconf->timing.vs_vs_addr = vstart;
	pconf->timing.vs_ve_addr = vend;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("hs_hs_addr=%d, hs_he_addr=%d, hs_vs_addr=%d, hs_ve_addr=%d\n",
			pconf->timing.hs_hs_addr, pconf->timing.hs_he_addr,
			pconf->timing.hs_vs_addr, pconf->timing.hs_ve_addr);
		LCDPR("vs_hs_addr=%d, vs_he_addr=%d, vs_vs_addr=%d, vs_ve_addr=%d\n",
			pconf->timing.vs_hs_addr, pconf->timing.vs_he_addr,
			pconf->timing.vs_vs_addr, pconf->timing.vs_ve_addr);
	}
}

int lcd_fr_is_frac(struct aml_lcd_drv_s *pdrv, unsigned int frame_rate)
{
	int ret = 0;

	switch (frame_rate) {
	case 47:
	case 59:
	case 95:
	case 119:
	case 191:
	case 239:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

int lcd_vmode_frac_is_support(struct aml_lcd_drv_s *pdrv, unsigned int frame_rate)
{
	int ret = 0;

	switch (frame_rate) {
	case 48:
	case 60:
	case 96:
	case 120:
	case 192:
	case 240:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

int lcd_frame_rate_change(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	 /* use default value to avoid offset */
	unsigned int pclk = pconf->timing.base_timing->pixel_clk;
	unsigned int h_period = pconf->timing.base_timing->h_period;
	unsigned int v_period = pconf->timing.base_timing->v_period;
	/* use act value as condition */
	unsigned char type = pconf->timing.act_timing.fr_adjust_type;
	unsigned int pclk_min = pconf->timing.act_timing.pclk_min;
	unsigned int pclk_max = pconf->timing.act_timing.pclk_max;
	unsigned int duration_num = pconf->timing.act_timing.sync_duration_num;
	unsigned int duration_den = pconf->timing.act_timing.sync_duration_den;
	unsigned long long temp;
	char str[100];
	int len = 0;

	/* clear clk flag */
	pdrv->config.timing.clk_change &= ~(LCD_CLK_FRAC_UPDATE | LCD_CLK_PLL_CHANGE);
	switch (type) {
	case 0: /* pixel clk adjust */
		temp = duration_num;
		temp = temp * h_period * v_period;
		pclk = lcd_do_div(temp, duration_den);
		if (pconf->timing.act_timing.pixel_clk != pclk)
			pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
		break;
	case 1: /* htotal adjust */
		temp = pclk;
		temp =  temp * duration_den * 100;
		h_period = v_period * duration_num;
		h_period = lcd_do_div(temp, h_period);
		h_period = (h_period + 99) / 100; /* round off */
		if (pconf->timing.act_timing.h_period != h_period) {
			/* check clk frac update */
			temp = duration_num;
			temp = temp * h_period * v_period;
			pclk = lcd_do_div(temp, duration_den);
		}
		if (pconf->timing.act_timing.pixel_clk != pclk)
			pconf->timing.clk_change |= LCD_CLK_FRAC_UPDATE;
		break;
	case 2: /* vtotal adjust */
		temp = pclk;
		temp = temp * duration_den * 100;
		v_period = h_period * duration_num;
		v_period = lcd_do_div(temp, v_period);
		v_period = (v_period + 99) / 100; /* round off */
		if (pconf->timing.act_timing.v_period != v_period) {
			/* check clk frac update */
			temp = duration_num;
			temp = temp * h_period * v_period;
			pclk = lcd_do_div(temp, duration_den);
		}
		if (pconf->timing.act_timing.pixel_clk != pclk)
			pconf->timing.clk_change |= LCD_CLK_FRAC_UPDATE;
		break;
	case 3: /* free adjust, use min/max range to calculate */
		temp = pclk;
		temp = temp * duration_den * 100;
		v_period = h_period * duration_num;
		v_period = lcd_do_div(temp, v_period);
		v_period = (v_period + 99) / 100; /* round off */
		if (v_period > pconf->timing.act_timing.v_period_max) {
			v_period = pconf->timing.act_timing.v_period_max;
			h_period = v_period * duration_num;
			h_period = lcd_do_div(temp, h_period);
			h_period = (h_period + 99) / 100; /* round off */
			if (h_period > pconf->timing.act_timing.h_period_max) {
				h_period = pconf->timing.act_timing.h_period_max;
				temp = duration_num;
				temp = temp * h_period * v_period;
				pclk = lcd_do_div(temp, duration_den);
				if (pclk > pclk_max) {
					LCDERR("[%d]: %s: invalid vmode\n",
						pdrv->index, __func__);
					return -1;
				}
				if (pconf->timing.act_timing.pixel_clk != pclk)
					pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
			}
		} else if (v_period < pconf->timing.act_timing.v_period_min) {
			v_period = pconf->timing.act_timing.v_period_min;
			h_period = v_period * duration_num;
			h_period = lcd_do_div(temp, h_period);
			h_period = (h_period + 99) / 100; /* round off */
			if (h_period < pconf->timing.act_timing.h_period_min) {
				h_period = pconf->timing.act_timing.h_period_min;
				temp = duration_num;
				temp = temp * h_period * v_period;
				pclk = lcd_do_div(temp, duration_den);
				if (pclk < pclk_min) {
					LCDERR("[%d]: %s: invalid vmode\n",
						pdrv->index, __func__);
					return -1;
				}
				if (pconf->timing.act_timing.pixel_clk != pclk)
					pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
			}
		}
		/* check clk frac update */
		if ((pconf->timing.clk_change & LCD_CLK_PLL_CHANGE) == 0) {
			temp = duration_num;
			temp = temp * h_period * v_period;
			pclk = lcd_do_div(temp, duration_den);
			if (pconf->timing.act_timing.pixel_clk != pclk)
				pconf->timing.clk_change |= LCD_CLK_FRAC_UPDATE;
		}
		break;
	case 4: /* hdmi mode */
		if (((duration_num / duration_den) == 59) ||
		    ((duration_num / duration_den) == 119)) {
			/* pixel clk adjust */
			temp = duration_num;
			temp = temp * h_period * v_period;
			pclk = lcd_do_div(temp, duration_den);
			if (pconf->timing.act_timing.pixel_clk != pclk)
				pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
		} else if ((duration_num / duration_den) == 47) {
			/* htotal adjust */
			temp = pclk;
			h_period = v_period * 50;
			h_period = lcd_do_div(temp, h_period);
			if (pconf->timing.act_timing.h_period != h_period) {
				/* check clk adjust */
				temp = duration_num;
				temp = temp * h_period * v_period;
				pclk = lcd_do_div(temp, duration_den);
			}
			if (pconf->timing.act_timing.pixel_clk != pclk)
				pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
		} else if ((duration_num / duration_den) == 95) {
			/* htotal adjust */
			temp = pclk;
			h_period = v_period * 100;
			h_period = lcd_do_div(temp, h_period);
			if (pconf->timing.act_timing.h_period != h_period) {
				/* check clk adjust */
				temp = duration_num;
				temp = temp * h_period * v_period;
				pclk = lcd_do_div(temp, duration_den);
			}
			if (pconf->timing.act_timing.pixel_clk != pclk)
				pconf->timing.clk_change |= LCD_CLK_PLL_CHANGE;
		} else {
			/* htotal adjust */
			temp = pclk;
			temp = temp * duration_den * 100;
			h_period = v_period * duration_num;
			h_period = lcd_do_div(temp, h_period);
			h_period = (h_period + 99) / 100; /* round off */
			if (pconf->timing.act_timing.h_period != h_period) {
				/* check clk frac update */
				temp = duration_num;
				temp = temp * h_period * v_period;
				pclk = lcd_do_div(temp, duration_den);
			}
			if (pconf->timing.act_timing.pixel_clk != pclk)
				pconf->timing.clk_change |= LCD_CLK_FRAC_UPDATE;
		}
		break;
	default:
		LCDERR("[%d]: %s: invalid fr_adjust_type: %d\n",
		       pdrv->index, __func__, type);
		return 0;
	}

	if (pconf->timing.act_timing.v_period != v_period) {
		len += sprintf(str + len, "v_period %u->%u",
			pconf->timing.act_timing.v_period, v_period);
		/* update v_period */
		pconf->timing.act_timing.v_period = v_period;
	}
	if (pconf->timing.act_timing.h_period != h_period) {
		if (len > 0)
			len += sprintf(str + len, ", ");
		len += sprintf(str + len, "h_period %u->%u",
			pconf->timing.act_timing.h_period, h_period);
		/* update h_period */
		pconf->timing.act_timing.h_period = h_period;
	}
	if (pconf->timing.act_timing.pixel_clk != pclk) {
		if (len > 0)
			len += sprintf(str + len, ", ");
		len += sprintf(str + len, "pclk %uHz->%uHz, clk_change:0x%x",
			pconf->timing.act_timing.pixel_clk, pclk,
			pconf->timing.clk_change);
		pconf->timing.act_timing.pixel_clk = pclk;
		pconf->timing.enc_clk = pclk / pconf->timing.ppc;
		if (pdrv->config.timing.ppc > 1) {
			len += sprintf(str + len, ", ppc=%d, enc_clk=%d",
				pdrv->config.timing.ppc, pconf->timing.enc_clk);
		}
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		if (len > 0) {
			LCDPR("[%d]: %s: sync_duration: %d/%d, %s\n",
				pdrv->index, __func__,
				pconf->timing.act_timing.sync_duration_num,
				pconf->timing.act_timing.sync_duration_den,
				str);
		}
	}

	return 0;
}

void lcd_pinmux_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_config_s *pconf;
	int i;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, status);

	pconf = &pdrv->config;
	if (status) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (pconf->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("pinmux_clr: 0x%x, 0x%08x\n",
				      pconf->pinmux_clr[i][0], pconf->pinmux_clr[i][1]);
			}
			lcd_pinmux_clr_mask(pconf->pinmux_clr[i][0], pconf->pinmux_clr[i][1]);
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (pconf->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("pinmux_set: 0x%x, 0x%08x\n",
				      pconf->pinmux_set[i][0], pconf->pinmux_set[i][1]);
			}
			lcd_pinmux_set_mask(pconf->pinmux_set[i][0], pconf->pinmux_set[i][1]);
			i++;
		}
	} else {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (pconf->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("pinmux_clr: 0x%x, 0x%08x\n",
				      pconf->pinmux_clr[i][0], pconf->pinmux_clr[i][1]);
			}
			lcd_pinmux_clr_mask(pconf->pinmux_clr[i][0], pconf->pinmux_clr[i][1]);
			i++;
		}
	}
}

unsigned int str_add_vmode(char *buf, struct lcd_vmode_info_s *vm_info, unsigned short framerate)
{
	unsigned char i, use_short = 0;
	struct V_name_s { unsigned short h, v; unsigned short fr[5]; } V_name_table[] = {
		{3840, 2160, { 60, 59, 50, 48, 47}},
		{3840, 2160, { 30, 25, 24,  0,  0}},
		{1920, 1080, {120, 60, 59, 50, 48}},
		{1920, 1080, { 47, 30, 25, 24,  0}},
		{1366,  768, { 60, 59, 50, 48, 47}},
		{1280,  720, { 60, 50,  0,  0,  0}},
	};

	for (i = 0; i < 5 * ARRAY_SIZE(V_name_table); i++) {
		if (V_name_table[i / 5].h == vm_info->width &&
		    V_name_table[i / 5].v == vm_info->height) {
			if (V_name_table[i / 5].fr[i % 5] == 0)
				continue;
			if (framerate == V_name_table[i / 5].fr[i % 5] || framerate == 0) {
				use_short = 1;
				break;
			}
		}
	}

	i = 0;
	i += use_short ? sprintf(buf + i,    "%up",                 vm_info->height) :
			 sprintf(buf + i, "%ux%up", vm_info->width, vm_info->height);
	if (framerate)
		i += sprintf(buf + i, "%huhz", framerate);

	return i;
}

unsigned int lcd_crc32(unsigned int seed, const unsigned char *ptr, int buf_len)
{
	static const unsigned int s_crc32[16] = {
	    0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
	    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
	    0xbdbdf21c};

	unsigned int crcu32 = seed;
	unsigned char b;

	if (buf_len <= 0)
		return 0;

	if (!ptr)
		return 0;

	crcu32 = ~crcu32;
	while (buf_len--) {
		b = *ptr++;
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
	}

	return ~crcu32;
}

