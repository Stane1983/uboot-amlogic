// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/lcd_cus_ctrl.h>
#include "lcd_common.h"

static int lcd_cus_ctrl_parse_clk_adv_dts(struct aml_lcd_drv_s *pdrv,
					  struct lcd_cus_ctrl_attr_config_s *attr_conf)
{
	char *propdata, *dt_addr = lcd_get_dt_addr();
	int node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);

	propdata = (char *)fdt_getprop(dt_addr, node_ofst, "clk_advanced", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get clk_advanced\n", pdrv->index);
		return -1;
	}
	pdrv->config.timing.ss_freq = be32_to_cpup(((u32 *)propdata) + 0);
	pdrv->config.timing.ss_mode = be32_to_cpup(((u32 *)propdata) + 1);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: cus_ctrl:CLK_ADV, freq=%hu, mode=%hu\n",
			pdrv->index, __func__, pdrv->config.timing.ss_freq,
			pdrv->config.timing.ss_mode);
	}

	return 0;
}

#define LCD_EXT_TIMING_MAX 8
static int lcd_cus_ctrl_parse_extend_tmg_dts(struct aml_lcd_drv_s *pdrv,
					     struct lcd_cus_ctrl_attr_config_s *attr_conf)
{
	struct lcd_detail_timing_s *ptiming;
	struct lcd_timing_s *tims = &pdrv->config.timing;
	char *propdata, snode[32], dbg_buf[128], *dt_addr = lcd_get_dt_addr();
	int node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);
	unsigned char etmg_idx, c_bit;

	if (!tims->timings[0] || node_ofst < 0)
		return -1;

	for (etmg_idx = 0; etmg_idx < LCD_EXT_TIMING_MAX; etmg_idx++) {
		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (!propdata)
			break;

		ptiming = lcd_timing_alloc(pdrv);
		if (!ptiming)
			break;
		memcpy(ptiming, tims->timings[0], sizeof(*ptiming));
		ptiming->ss_force = 0;

		ptiming->h_period    = be32_to_cpup(((u32 *)propdata) + 0);
		ptiming->h_active    = be32_to_cpup(((u32 *)propdata) + 1);
		ptiming->hsync_width = be32_to_cpup(((u32 *)propdata) + 2);
		ptiming->hsync_bp    = be32_to_cpup(((u32 *)propdata) + 3);
		ptiming->hsync_pol   = be32_to_cpup(((u32 *)propdata) + 4);
		ptiming->hsync_fp    = ptiming->h_period - ptiming->h_active -
						ptiming->hsync_width - ptiming->hsync_bp;
		ptiming->v_period    = be32_to_cpup(((u32 *)propdata) + 5);
		ptiming->v_active    = be32_to_cpup(((u32 *)propdata) + 6);
		ptiming->vsync_width = be32_to_cpup(((u32 *)propdata) + 7);
		ptiming->vsync_bp    = be32_to_cpup(((u32 *)propdata) + 8);
		ptiming->vsync_pol   = be32_to_cpup(((u32 *)propdata) + 9);
		ptiming->vsync_fp    = ptiming->v_period - ptiming->v_active -
						ptiming->vsync_width - ptiming->vsync_bp;

		c_bit = be32_to_cpup(((u32 *)propdata) + 10);
		ptiming->pixel_clk = be32_to_cpup(((u32 *)propdata) + 13);

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_frame_rate", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			ptiming->frame_rate_min = be32_to_cpup(((u32 *)propdata) + 0);
			ptiming->frame_rate_max = be32_to_cpup(((u32 *)propdata) + 1);
		}

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_range_setting", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			ptiming->h_period_min = be32_to_cpup(((u32 *)propdata) + 0);
			ptiming->h_period_max = be32_to_cpup(((u32 *)propdata) + 1);
			ptiming->v_period_min = be32_to_cpup(((u32 *)propdata) + 2);
			ptiming->v_period_max = be32_to_cpup(((u32 *)propdata) + 3);
			ptiming->pclk_min     = be32_to_cpup(((u32 *)propdata) + 4);
			ptiming->pclk_max     = be32_to_cpup(((u32 *)propdata) + 5);
		} else {
			ptiming->h_period_min = ptiming->h_period;
			ptiming->h_period_max = ptiming->h_period;
			ptiming->v_period_min = ptiming->v_period;
			ptiming->v_period_max = ptiming->v_period;
			ptiming->pclk_min     = ptiming->pixel_clk;
			ptiming->pclk_max     = ptiming->pixel_clk;
		}
		ptiming->fr_adjust_type = propdata ? 3 : 0xff;
		ptiming->switch_type = attr_conf->attr_flag;

		lcd_clk_frame_rate_init(ptiming);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			memset(dbg_buf, 0, sizeof(char) * 128);
			dtimg_info_add(dbg_buf, ptiming, c_bit);
			LCDPR("[%d]: %s: extend timing[%d]: %s\n",
				pdrv->index, __func__, etmg_idx, dbg_buf);
		}

		lcd_config_timing_check(pdrv, ptiming);
	}
	return 0;
}

int lcd_cus_ctrl_load_from_dts(struct aml_lcd_drv_s *pdrv)
{
	int para_size, node_ofst;
	char *dt_addr = lcd_get_dt_addr();
	char *propdata, node[25];
	unsigned char cus_idx, cus_type;
	struct lcd_cus_ctrl_attr_config_s cus_cfg;
	int ret = 0;

	node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);

	if (node_ofst < 0)
		return -1;

	for (cus_idx = 0; cus_idx < LCD_CUS_CTRL_ATTR_CNT_MAX; cus_idx++) {
		sprintf(node, "cus_ctrl_%hu_attr", cus_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, node, &para_size);
		if (!propdata)
			break;

		cus_cfg.attr_index = cus_idx;
		cus_cfg.param_flag = 0;
		cus_cfg.param_size = 1;
		cus_cfg.priv_sel = 0;

		cus_type = be32_to_cpup(((u32 *)propdata) + 0);
		cus_cfg.attr_type = cus_type;
		switch (cus_type) {
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			cus_cfg.attr_flag = be32_to_cpup(((u32 *)propdata) + 1);
			ret = lcd_cus_ctrl_parse_extend_tmg_dts(pdrv, &cus_cfg);
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			ret = lcd_cus_ctrl_parse_clk_adv_dts(pdrv, &cus_cfg);
			break;
		default:
			LCDERR("[%d]: %s: invalid cus_type: %d\n",
				pdrv->index, __func__, cus_type);
			break;
		}
	}

	return ret;
}

static int lcd_cus_ctrl_attr_parse_ufr_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_detail_timing_s *ptiming;
	struct lcd_timing_s *tims = &pdrv->config.timing;
	unsigned int hfp, vfp, offset = 0;

	if (!tims->timings[0])
		return -1;

	ptiming = lcd_timing_alloc(pdrv);
	if (!ptiming)
		return -1;

	memcpy(ptiming, tims->timings[0], sizeof(*ptiming));
	ptiming->ss_force = 0;

	ptiming->v_period_min = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->v_period_max = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;
	if (attr_conf->param_size == offset) {
		//frame_rate range is update for compatibility in lcd_fr_range_update
		ptiming->frame_rate_min = 0;
		ptiming->frame_rate_max = 0;
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_next;
	}

	ptiming->frame_rate_min = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->frame_rate_max = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;
	if (attr_conf->param_size == offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_next;

	ptiming->vsync_width = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->vsync_bp = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;

lcd_cus_ctrl_attr_parse_ufr_ukey_next:
	ptiming->v_active /= 2;
	ptiming->v_period /= 2;
	hfp = ptiming->h_period - ptiming->h_active - ptiming->hsync_width - ptiming->hsync_bp;
	vfp = ptiming->v_period - ptiming->v_active - ptiming->vsync_width - ptiming->vsync_bp;
	ptiming->hsync_fp = hfp;
	ptiming->vsync_fp = vfp;
	ptiming->switch_type = attr_conf->attr_flag;
	lcd_clk_frame_rate_init(ptiming);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %dx%dp%dhz\n",
			pdrv->index, __func__,
			ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
	}

	lcd_config_timing_check(pdrv, ptiming);

	return 0;

lcd_cus_ctrl_attr_parse_ufr_ukey_err:
	memset(ptiming, 0, sizeof(*ptiming));
	lcd_timing_free_last(pdrv);
	LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
		pdrv->index, __func__, attr_conf->param_size, offset);
	return -1;
}

static int lcd_cus_ctrl_attr_parse_dfr_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_detail_timing_s *ptiming;
	struct lcd_timing_s *tims = &pdrv->config.timing;
	unsigned int offset = 0;
	unsigned int temp, fr_size, tmg_size = 0;
	int i, n, ret = 0, fr_cnt = 0, tmg_group_cnt = 0;
	struct lcd_dfr_fr_s *fr = NULL;
	struct lcd_dfr_timing_s *dfr_timing = NULL;

	if (!tims->timings[0])
		return -1;

	fr_cnt = *(p + offset);
	offset += 1;
	tmg_group_cnt = *(p + offset);
	offset += 1;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: fr_cnt: %d, tmg_group_cnt: %d\n",
		      pdrv->index, __func__, fr_cnt, tmg_group_cnt);
	}

	if (!fr_cnt) {
		LCDERR("[%d]: %s: fr_cnt(%d) error!\n", pdrv->index, __func__, fr_cnt);
		return 0;
	}
	fr_size = fr_cnt * sizeof(struct lcd_dfr_fr_s);
	fr = malloc(fr_size);
	if (!fr)
		return -1;
	memset(fr, 0, fr_size);

	if (tmg_group_cnt) {
		tmg_size = tmg_group_cnt * sizeof(struct lcd_dfr_timing_s);
		dfr_timing = malloc(tmg_size);
		if (!dfr_timing) {
			free(fr);
			return -1;
		}
		memset(dfr_timing, 0, tmg_size);
	}

	for (i = 0; i < fr_cnt; i++) {
		temp = *(unsigned short *)(p + offset);
		fr[i].frame_rate = temp & 0xfff;
		fr[i].timing_index = (temp >> 12) & 0xf;
		offset += 2;
		fr[i].frame_rate_min = *(unsigned short *)(p + offset);
		offset += 2;
		fr[i].frame_rate_max = *(unsigned short *)(p + offset);
		offset += 2;
	}

	for (i = 0;  i < tmg_group_cnt; i++) {
		dfr_timing[i].vtotal = *(unsigned short *)(p + offset);
		offset += 2;
		dfr_timing[i].vtotal_min = *(unsigned short *)(p + offset);
		offset += 2;
		dfr_timing[i].vtotal_max = *(unsigned short *)(p + offset);
		offset += 2;
		dfr_timing[i].vpw = *(unsigned short *)(p + offset);
		offset += 2;
		dfr_timing[i].vbp = *(unsigned short *)(p + offset);
		offset += 2;
	}

	if (attr_conf->param_size < offset) {
		LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
			pdrv->index, __func__, attr_conf->param_size, offset);
		ret = -1;
		goto parse_dfr_timing_end;
	}

	for (i = 0; i < fr_cnt; i++) {
		ptiming = lcd_timing_alloc(pdrv);
		if (!ptiming) {
			ret = -1;
			goto parse_dfr_timing_end;
		}

		memcpy(ptiming, tims->timings[0], sizeof(*ptiming));
		ptiming->ss_force = 0;

		ptiming->frame_rate = fr[i].frame_rate;
		ptiming->frame_rate_min = fr[i].frame_rate_min;
		ptiming->frame_rate_max = fr[i].frame_rate_max;
		if (fr[i].timing_index == 0) {
			ptiming->pixel_clk = ptiming->frame_rate *
				ptiming->h_period * ptiming->v_period;
		} else {
			n = fr[i].timing_index - 1;
			if (n >= tmg_group_cnt || !dfr_timing) {
				LCDERR("[%d]: %s: timing_index %d err, tmg_group_cnt %d\n",
					pdrv->index, __func__, fr[i].timing_index,
					tmg_group_cnt);
				ret = -1;
				lcd_timing_free_last(pdrv);
				goto parse_dfr_timing_end;
			}
			ptiming->v_period = dfr_timing[n].vtotal;
			ptiming->v_period_min = dfr_timing[n].vtotal_min;
			ptiming->v_period_max = dfr_timing[n].vtotal_max;
			ptiming->vsync_width = dfr_timing[n].vpw;
			ptiming->vsync_bp = dfr_timing[n].vbp;
			ptiming->vsync_fp = ptiming->v_period - ptiming->v_active -
				ptiming->vsync_width - ptiming->vsync_bp;
			ptiming->pixel_clk = ptiming->frame_rate *
				ptiming->h_period * ptiming->v_period;
		}
		ptiming->switch_type = attr_conf->attr_flag;
		lcd_clk_frame_rate_init(ptiming);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: dfr[%d]: %dx%dp%dhz\n",
				pdrv->index, __func__, i,
				ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
		}

		lcd_config_timing_check(pdrv, ptiming);
	}

parse_dfr_timing_end:
	if (tmg_group_cnt) {
		memset(dfr_timing, 0, tmg_size);
		free(dfr_timing);
	}
	memset(fr, 0, fr_size);
	free(fr);

	return ret;
}

static int lcd_cus_ctrl_attr_parse_extend_tmg_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_detail_timing_s *ptiming;
	struct lcd_timing_s *tims = &pdrv->config.timing;
	unsigned int offset = 0;
	unsigned int hfp, vfp, temp;
	int i, group_cnt;

	if (!tims->timings[0])
		return -1;

	group_cnt = attr_conf->param_flag;
	if (!group_cnt)
		return 0;

	for (i = 0;  i < group_cnt; i++) {
		ptiming = lcd_timing_alloc(pdrv);
		if (!ptiming)
			return -1;
		memcpy(ptiming, tims->timings[0], sizeof(*ptiming));
		ptiming->ss_force = 0;

		ptiming->h_active = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->v_active = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->h_period = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->v_period = *(unsigned short *)(p + offset);
		offset += 2;
		temp = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->hsync_width = temp & 0xfff;
		ptiming->hsync_pol = (temp >> 12) & 0xf;
		ptiming->hsync_bp = *(unsigned short *)(p + offset);
		offset += 2;
		temp = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->vsync_width = temp & 0xfff;
		ptiming->vsync_pol = (temp >> 12) & 0xf;
		ptiming->vsync_bp = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->fr_adjust_type = *(p + offset);
		offset += 1;
		ptiming->pixel_clk = *(unsigned int *)(p + offset);
		offset += 4;

		ptiming->h_period_min = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->h_period_max = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->v_period_min = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->v_period_max = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->frame_rate_min = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->frame_rate_max = *(unsigned short *)(p + offset);
		offset += 2;
		ptiming->pclk_min = *(unsigned int *)(p + offset);
		offset += 4;
		ptiming->pclk_max = *(unsigned int *)(p + offset);
		offset += 4;

		hfp = ptiming->h_period - ptiming->h_active -
			ptiming->hsync_width - ptiming->hsync_bp;
		vfp = ptiming->v_period - ptiming->v_active -
			ptiming->vsync_width - ptiming->vsync_bp;
		ptiming->hsync_fp = hfp;
		ptiming->vsync_fp = vfp;
		ptiming->switch_type = attr_conf->attr_flag;

		lcd_clk_frame_rate_init(ptiming);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: extend_tmg[%d]: %dx%dp%dhz\n",
			      pdrv->index, __func__, i,
			      ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
		}

		lcd_config_timing_check(pdrv, ptiming);
	}

	if (attr_conf->param_size < offset) {
		for (i = 0;  i < group_cnt; i++)
			lcd_timing_free_last(pdrv);
		LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
			pdrv->index, __func__, attr_conf->param_size, offset);
		return -1;
	}

	return 0;
}

static int lcd_cus_ctrl_attr_parse_clk_adv_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	unsigned int offset = 0;

	if (attr_conf->attr_flag & (1 << 0)) {
		pdrv->config.timing.ss_freq = *(unsigned char *)(p + offset);
		offset += 1;
		pdrv->config.timing.ss_mode = *(unsigned char *)(p + offset);
		offset += 1;
	}

	return 0;
}

static int lcd_cus_ctrl_attr_parse_tuning_attr_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	unsigned int offset = 0;
	unsigned short lane_cnt, group_cnt = 0;
	int i, j;
	struct phy_config_s *phy_cfg;
	struct phy_attr_s *phy;
	unsigned char pn_phase;

	if (!pdrv->config.phy_cfg.phys[0])
		return -1;

	if (attr_conf->param_size == 0)
		return 0;

	group_cnt = attr_conf->param_flag;
	if (!group_cnt)
		return -1;

	lane_cnt = *(unsigned short *)(p + offset);
	offset += 2;

	phy_cfg = &pdrv->config.phy_cfg;
	phy_cfg->lane_num = lane_cnt;
	for (j = 0; j < lane_cnt; j++) {
		pn_phase = *(p + offset);
		phy_cfg->ch_ctrl[j].pn_swap = pn_phase & 0x1;
		phy_cfg->ch_ctrl[j].phase_sel = (pn_phase >> 1) & 0xf;
		if (phy_cfg->ch_ctrl[j].phase_sel == 0xf)
			phy_cfg->ch_ctrl[j].phase_sel = 0xff;
		offset += 1;
		phy_cfg->ch_ctrl[j].sel = *(p + offset);
		offset += 1;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: lane[%d]: sel=0x%x, pn_swap=%d, phase_sel=0x%x\n",
			      __func__, j, phy_cfg->ch_ctrl[j].sel,
			      phy_cfg->ch_ctrl[j].pn_swap, phy_cfg->ch_ctrl[j].phase_sel);
	}

	for (i = 0; i < group_cnt; i++) {
		phy = lcd_phy_alloc(pdrv);
		if (!phy)
			goto lcd_cus_parse_phy_exit;
		memcpy(phy, phy_cfg->phys[0], sizeof(*phy));

		phy->phy_clk = *(unsigned short *)(p + offset);
		offset += 2;
		offset += 4; //phy_clk_min_max reserved
		phy->ss.level = *(unsigned short *)(p + offset);
		offset += 2;
		phy->ss.freq = *(unsigned short *)(p + offset);
		offset += 2;
		phy->ss.mode = *(unsigned short *)(p + offset);
		offset += 2;
		phy->clk_phase = *(unsigned short *)(p + offset);
		offset += 2;
		phy->vswing = *(unsigned short *)(p + offset);
		offset += 2;
		phy->vcm = *(unsigned short *)(p + offset);
		offset += 2;
		phy->ref_bias = *(unsigned short *)(p + offset);
		offset += 2;
		phy->odt = *(unsigned short *)(p + offset);
		offset += 2;
		phy->cv_mode = *(unsigned short *)(p + offset);
		offset += 2;
		offset += 14; //phy_attr_5~11
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: vswing=0x%x, vcm=0x%x, bias=0x%x, odt=0x%x, cv_mode=%d\n",
			      __func__, phy->vswing, phy->vcm,
			      phy->ref_bias, phy->odt,
			      phy->cv_mode);
		}

		for (j = 0; j < lane_cnt; j++) {
			phy->lane[j].preem = *(unsigned short *)(p + offset);
			offset += 2;
			phy->lane[j].amp = *(unsigned short *)(p + offset);
			offset += 2;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: lane[%d]: preem=0x%x, amp=0x%x\n",
				      __func__, i, phy->lane[j].preem, phy->lane[j].amp);
			}
		}
		if (attr_conf->param_size < offset) {
			lcd_phy_free_last(pdrv);
			return 0;
		}

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: tuning_param[%d]: phy_clk:%d, lane_cnt:%d\n",
			      pdrv->index, __func__, i,
			      phy->phy_clk, lane_cnt);
		}
	}

	if (phy_cfg->phys[0] && phy_cfg->phys[1] &&
	    phy_cfg->phys[0]->phy_clk == phy_cfg->phys[1]->phy_clk) {
		free(phy_cfg->phys[0]);
		for (i = 0; i < phy_cfg->group_num - 1; i++)
			phy_cfg->phys[i] = phy_cfg->phys[i + 1];
		phy_cfg->phys[phy_cfg->group_num - 1] = NULL;
		phy_cfg->group_num--;
		phy_cfg->act_phy = phy_cfg->phys[0];
	}

lcd_cus_parse_phy_exit:

	return 0;
}

static int lcd_cus_ctrl_attr_parse_tcon_sw_pol_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	return 0;
}

static int lcd_cus_ctrl_attr_parse_tcon_sw_pdf_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	return 0;
}

int lcd_cus_ctrl_load_from_unifykey(struct aml_lcd_drv_s *pdrv, unsigned char *buf,
		unsigned int max_size, unsigned char version)
{
	unsigned char *p;
	struct lcd_cus_ctrl_attr_config_s attr_conf;
	unsigned int ctrl_en, ctrl_attr, ctrl_cnt = 0;
	unsigned int offset = 0, param_size, i, n;
	char str[128];
	int len, ret;

	ctrl_en = *(unsigned int *)buf;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		if (ctrl_en & (1 << i))
			ctrl_cnt = i + 1;
	}
	if (ctrl_cnt == 0)
		return 0;

	memset(&attr_conf, 0, sizeof(attr_conf));
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ctrl_en=0x%x, ctrl_cnt=%d\n",
			pdrv->index, __func__, ctrl_en, ctrl_cnt);
	}

	offset = 4; //ctrl_en size
	n = 0;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		if (n >= ctrl_cnt)
			break;
		if (offset >= max_size) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: offset %d reach max_size %d, exit\n",
					pdrv->index, __func__, offset, max_size);
			}
			break;
		}
		p = buf + offset;
		if (unlikely(!(ctrl_en & (1 << i)))) {
			offset +=  4; //4 for attr_x and param_size
			continue;
		}

		ctrl_attr = *(unsigned short *)p;
		attr_conf.attr_index = i;
		attr_conf.attr_flag = ctrl_attr & 0xf;
		attr_conf.param_flag = (ctrl_attr >> 4) & 0xf;
		attr_conf.attr_type = (ctrl_attr >> 8) & 0xff;
		param_size = *(unsigned short *)(p + 2);
		attr_conf.param_size = param_size;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			len = sprintf(str, "attr_type=%x, attr_flag=%d, ",
				      attr_conf.attr_type,
				      attr_conf.attr_flag);
			sprintf(str + len, "param_flag=%d, param_size=%d",
				attr_conf.param_flag,
				attr_conf.param_size);
			LCDPR("[%d]: %s: attr[%d]: %s\n",
			      pdrv->index, __func__, i, str);
		}

		switch (attr_conf.attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			ret = lcd_cus_ctrl_attr_parse_ufr_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			ret = lcd_cus_ctrl_attr_parse_dfr_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			ret = lcd_cus_ctrl_attr_parse_extend_tmg_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			ret = lcd_cus_ctrl_attr_parse_clk_adv_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_TUNING_ATTR:
			if (version < 3) {
				ret = -1;
				LCDERR("[%d]: %s, invalid tuning_attr with ukey_ver %d\n",
				       pdrv->index, __func__, version);
				break;
			}
			ret = lcd_cus_ctrl_attr_parse_tuning_attr_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			LCDERR("todo\n");
			ret = lcd_cus_ctrl_attr_parse_tcon_sw_pol_ukey(pdrv, &attr_conf, (p + 4));
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			LCDERR("todo\n");
			ret = lcd_cus_ctrl_attr_parse_tcon_sw_pdf_ukey(pdrv, &attr_conf, (p + 4));
			break;
		default:
			LCDERR("[%d]: %s: invalid attr_type: 0x%x\n",
			       pdrv->index, __func__, attr_conf.attr_type);
			goto lcd_cus_ctrl_load_from_unifykey_err;
		}
		n++;
		if (ret)
			goto lcd_cus_ctrl_load_from_unifykey_err;
		offset += (param_size + 4); //4 for attr_x and param_size
	}

	return 0;

lcd_cus_ctrl_load_from_unifykey_err:
	return -1;
}
