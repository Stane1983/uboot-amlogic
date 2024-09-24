// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#ifdef CONFIG_MESON_T7
static struct lcd_phy_ctrl_s *phy_ctrl_p;

static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{ANACTRL_DIF_PHY_CNTL1,  "PHY_CNTL1"},
		{ANACTRL_DIF_PHY_CNTL2,  "PHY_CNTL2"},
		{ANACTRL_DIF_PHY_CNTL3,  "PHY_CNTL3"},
		{ANACTRL_DIF_PHY_CNTL4,  "PHY_CNTL4"},
		{ANACTRL_DIF_PHY_CNTL5,  "PHY_CNTL5"},
		{ANACTRL_DIF_PHY_CNTL6,  "PHY_CNTL6"},
		{ANACTRL_DIF_PHY_CNTL7,  "PHY_CNTL7"},
		{ANACTRL_DIF_PHY_CNTL8,  "PHY_CNTL8"},
		{ANACTRL_DIF_PHY_CNTL9,  "PHY_CNTL9"},
		{ANACTRL_DIF_PHY_CNTL10, "PHY_CNTL10"},
		{ANACTRL_DIF_PHY_CNTL11, "PHY_CNTL11"},
		{ANACTRL_DIF_PHY_CNTL12, "PHY_CNTL12"},
		{ANACTRL_DIF_PHY_CNTL13, "PHY_CNTL13"},
		{ANACTRL_DIF_PHY_CNTL14, "PHY_CNTL14"},
		{ANACTRL_DIF_PHY_CNTL15, "PHY_CNTL15"},
		{ANACTRL_DIF_PHY_CNTL16, "PHY_CNTL16"},
		{ANACTRL_DIF_PHY_CNTL17, "PHY_CNTL17"},
		{ANACTRL_DIF_PHY_CNTL18, "PHY_CNTL18"},
		{ANACTRL_DIF_PHY_CNTL19, "PHY_CNTL19"},
		{ANACTRL_DIF_PHY_CNTL20, "PHY_CNTL20"},
		{ANACTRL_DIF_PHY_CNTL21, "PHY_CNTL21"}
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static void lcd_phy_cntl_set(unsigned int flag,
				unsigned int data_lane0_aux,
				unsigned int data_lane1_aux,
				unsigned int data_lane)
{
	if (flag & (1 << 0)) {
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL1, data_lane0_aux);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL2, data_lane1_aux);
	}
	if (flag & (1 << 1))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL3, data_lane);
	if (flag & (1 << 2))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL4, data_lane);
	if (flag & (1 << 3))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL5, data_lane);
	if (flag & (1 << 4))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL6, data_lane);
	if (flag & (1 << 5))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL7, data_lane);
	if (flag & (1 << 6))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL8, data_lane);
	if (flag & (1 << 7))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL9, data_lane);
	if (flag & (1 << 8)) {
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL10, data_lane0_aux);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL11, data_lane1_aux);
	}
	if (flag & (1 << 9))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL12, data_lane);
	if (flag & (1 << 10))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL13, data_lane);
	if (flag & (1 << 11))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, data_lane);
	if (flag & (1 << 12))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, data_lane);
	if (flag & (1 << 13))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL16, data_lane);
	if (flag & (1 << 14))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL17, data_lane);
	if (flag & (1 << 15))
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL18, data_lane);
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int data_lane0_aux, data_lane1_aux, data_lane;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	if (!phy_ctrl_p || !phy)
		return;

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("vswing_level=0x%x, preem_level=0x%x\n",
				phy_cfg->vswing_level, phy_cfg->preem_level);
		}

		data_lane0_aux = 0x06430028 | (phy_cfg->preem_level << 28);
		data_lane1_aux = 0x0100ffff;
		data_lane = 0x06530028 | (phy_cfg->preem_level << 28);
		lcd_phy_cntl_set(phy_cfg->lane_valid, data_lane0_aux, data_lane1_aux, data_lane);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0x00406240 | phy_cfg->vswing_level);
	} else {
		lcd_phy_cntl_set(phy_cfg->lane_valid, 0, 0, 0);
		if (phy_ctrl_p->lane_lock_total == 0)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0);
	}
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int data_lane0_aux, data_lane1_aux, data_lane;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	if (!phy_ctrl_p || !phy)
		return;

	if (pdrv->index == 2) {
		LCDERR("invalid drv_index %d for vbyone\n", pdrv->index);
		return;
	}

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("vswing_level=0x%x, preem_level=0x%x\n",
			       phy_cfg->vswing_level, phy_cfg->preem_level);
		}

		data_lane0_aux = 0x06430028 | (phy_cfg->preem_level << 28);
		data_lane1_aux = 0x0000ffff;
		data_lane = 0x06530028 | (phy_cfg->preem_level << 28);
		lcd_phy_cntl_set(phy_cfg->lane_valid, data_lane0_aux, data_lane1_aux, data_lane);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0x00401640 | phy_cfg->vswing_level);
	} else {
		lcd_phy_cntl_set(phy_cfg->lane_valid, 0, 0, 0);
		if (phy_ctrl_p->lane_lock_total == 0)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0);
	}
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int data_lane0_aux, data_lane1_aux, data_lane;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;

	if (!phy_ctrl_p)
		return;

	if (pdrv->index == 2) {
		LCDERR("invalid drv_index %d for mipi dsi\n", pdrv->index);
		return;
	}

	if (status) {
		data_lane0_aux = 0x022a0028;
		data_lane1_aux = 0x0000ffcf;
		data_lane = 0x022a0028;
		lcd_phy_cntl_set(phy->lane_valid, data_lane0_aux, data_lane1_aux, data_lane);
		if (pdrv->index)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL13, 0x822a0028);
		else
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL4, 0x822a0028);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0x1e406253);
		if (pdrv->index)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL21, 0xffff);
		else
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL20, (0xffff << 16));
	} else {
		lcd_phy_cntl_set(phy->lane_valid, 0, 0, 0);
		if (phy_ctrl_p->lane_lock_total == 0)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0);
		if (pdrv->index)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL21, 0);
		else
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL20, 0);
	}
}

static void lcd_edp_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int data_lane0_aux, data_lane1_aux, data_lane;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (!phy_ctrl_p || !phy)
		return;

	if (pdrv->index == 2) {
		LCDERR("invalid drv_index %d for edp\n", pdrv->index);
		return;
	}

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("vswing_level=0x%x, preem_level=0x%x\n",
			      phy_cfg->vswing_level, phy_cfg->preem_level);
		}

		data_lane0_aux = 0x46770038;
		data_lane1_aux = 0x0000ffff;
		data_lane = 0x06530028 | (phy_cfg->preem_level << 28);
		lcd_phy_cntl_set(phy_cfg->lane_valid, data_lane0_aux, data_lane1_aux, data_lane);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0x00406240 | phy_cfg->vswing_level);
	} else {
		lcd_phy_cntl_set(phy_cfg->lane_valid, 0, 0, 0);
		if (phy_ctrl_p->lane_lock_total == 0)
			lcd_ana_write(ANACTRL_DIF_PHY_CNTL19, 0);
	}
}

static unsigned int lcd_phy_preem_level_to_value_t7(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int preem_value = 0;

	preem_value = (level >= 0xf) ? 0xf : level;

	return preem_value;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t7 = {
	.lane_num = 16,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_value_t7,
	.phy_amp_dft_val = NULL,
	.phy_lane_phase_sel_def = NULL,
	.phy_glb_param_dft_val = NULL,
	.phy_param_get = NULL,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = NULL,
	.phy_set_p2p = NULL,
	.phy_set_mipi = lcd_mipi_phy_set,
	.phy_set_edp = lcd_edp_phy_set,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t7(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_t7;
	return phy_ctrl_p;
}
#endif
