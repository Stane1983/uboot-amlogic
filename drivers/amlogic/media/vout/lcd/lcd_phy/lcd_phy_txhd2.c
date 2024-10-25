// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#ifdef CONFIG_MESON_TXHD2
static unsigned int p2p_phy_ch_reg_mipi_dsi = 0x0002;
static unsigned int p2p_phy_ch_dig_mipi_dsi = 0x0174;
static unsigned int p2p_phy_ch_reg_lvds = 0x002a;
static unsigned int p2p_phy_ch_dig_lvds = 0x0014;
static unsigned int p2p_phy_ch_dig_mlvds = 0x0010;

static unsigned int chreg_reg[5] = {
	HHI_DIF_CSI_PHY_CNTL1,
	HHI_DIF_CSI_PHY_CNTL2,
	HHI_DIF_CSI_PHY_CNTL3,
	HHI_DIF_CSI_PHY_CNTL4,
	HHI_DIF_CSI_PHY_CNTL6
};

static unsigned int chdig_reg[5] = {
	HHI_DIF_CSI_PHY_CNTL8,
	HHI_DIF_CSI_PHY_CNTL9,
	HHI_DIF_CSI_PHY_CNTL10,
	HHI_DIF_CSI_PHY_CNTL11,
	HHI_DIF_CSI_PHY_CNTL12
};

static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{HHI_DIF_CSI_PHY_CNTL1,  "PHY_CNTL1"},
		{HHI_DIF_CSI_PHY_CNTL2,  "PHY_CNTL2"},
		{HHI_DIF_CSI_PHY_CNTL3,  "PHY_CNTL3"},
		{HHI_DIF_CSI_PHY_CNTL4,  "PHY_CNTL4"},
		{HHI_DIF_CSI_PHY_CNTL6,  "PHY_CNTL6"},
		{HHI_DIF_CSI_PHY_CNTL8,  "PHY_CNTL8"},
		{HHI_DIF_CSI_PHY_CNTL9,  "PHY_CNTL9"},
		{HHI_DIF_CSI_PHY_CNTL10, "PHY_CNTL10"},
		{HHI_DIF_CSI_PHY_CNTL11, "PHY_CNTL11"},
		{HHI_DIF_CSI_PHY_CNTL12, "PHY_CNTL12"},
		{HHI_DIF_CSI_PHY_CNTL13, "PHY_CNTL13"},
		{HHI_DIF_CSI_PHY_CNTL14, "PHY_CNTL14"},
		{HHI_DIF_CSI_PHY_CNTL15, "PHY_CNTL15"}
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static int lcd_phy_param_get_from_reg(struct aml_lcd_drv_s *pdrv,
				      struct phy_config_s *phy_cfg, struct phy_attr_s *phy)
{
	unsigned int data32, chreg, bit;
	int i;

	data32 = lcd_ana_read(HHI_DIF_CSI_PHY_CNTL14);
	phy->vswing = (data32 >> 12) & 0xf;
	phy->vcm = (data32 >> 4) & 0xff;
	phy->odt = (data32 >> 23) & 0xff;
	phy->ref_bias = 0;
	phy->cv_mode = 0;

	data32 = lcd_ana_read(HHI_DIF_CSI_PHY_CNTL13);
	phy_cfg->ckdi = (data32 >> 16) & 0x3ff;

	for (i = 0; i < phy_cfg->lane_num; i++) {
		bit = i & 0x1 ? 16 : 0;
		chreg = lcd_ana_getb(chreg_reg[i >> 1], bit, 16);

		phy->lane[i].preem = (chreg >> 9) & 0xf;
		phy->lane[i].amp = (chreg >> 3) & 0x7;
	}

	return 0;
}

static void lcd_phy_common_update(struct aml_lcd_drv_s *pdrv, unsigned int cntl14)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	/* vswing */
	cntl14 &= ~(0xf << 12);
	cntl14 |= (phy->vswing << 12);
	/* vcm */
	cntl14 &= ~(0xff << 4);
	cntl14 |= (phy->vcm & 0xff) << 4;
	/* odt */
	cntl14 &= ~(0xff << 23);
	cntl14 |= (phy->odt & 0xff) << 23;
	/* bandgap */
	cntl14 |= (1 << 0);

	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, cntl14);
}

static void lcd_phy_cntl_lvds_set(struct aml_lcd_drv_s *pdrv, unsigned int status)
{
	unsigned int chreg = 0, chdig = 0;
	unsigned int i, bit, reg_data, dig_data;
	unsigned char is_mlvds = pdrv->config.basic.lcd_type == LCD_MLVDS;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d, ckdi:0x%x\n", __func__, status, phy_cfg->ckdi);

	if (status) {
		if (is_mlvds) {
			reg_data = p2p_phy_ch_reg_lvds & 0xe1c7;
			dig_data = p2p_phy_ch_dig_mlvds;
			lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL13, phy_cfg->ckdi & 0x3ff, 16, 10);
			lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL15, 1, 31, 1);
		} else { // LVDS
			reg_data = (p2p_phy_ch_reg_lvds & 0xe1c5) | 0x1;
			dig_data = p2p_phy_ch_dig_lvds;
			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL13, 0);
			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL15, 0);
		}
	} else {
		reg_data = 3 << 1;
		dig_data = 0;
	}

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if (phy_cfg->lane_valid & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			chreg = reg_data;
			chdig = dig_data;
			if (status) {
				if (((phy_cfg->ckdi & (1 << i)) == 0) && is_mlvds) { //data lane
					chdig |= (1 << 2);
					chreg = (chreg | (1 << 0)) & ~(1 << 1);
				}
				chreg |= (phy->lane[i].preem & 0xf) << 9;
				chreg |= (phy->lane[i].amp & 0x7) << 3;
			}
			lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
		}
	}
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	if (status)
		lcd_phy_common_update(pdrv, 0x106f1);
	else
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x0);

	lcd_phy_cntl_lvds_set(pdrv, status);
	lcd_combo_dphy_write(COMBO_DPHY_CNTL0, status ? 0x55555 : 0xaaaaa);
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	if (status) {
		lcd_phy_common_update(pdrv, 0x106f1);
		lcd_phy_cntl_lvds_set(pdrv, status);
		lcd_combo_dphy_write(COMBO_DPHY_CNTL0, 0x55555);
	} else {
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x0);
		lcd_phy_cntl_lvds_set(pdrv, status);
		lcd_combo_dphy_write(COMBO_DPHY_CNTL0, 0xaaaaa);
	}
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	unsigned char bit, i;

	if (status) {
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL13, 0x00000099);
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x7f820613);
	}
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL15, 0);

	for (i = 0; i < 10; i++) {
		bit = i % 2 ? 16 : 0;
		if (phy_cfg->lane_valid & (1 << i) && status) {
			lcd_ana_setb(chreg_reg[i >> 1], p2p_phy_ch_reg_mipi_dsi, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], p2p_phy_ch_dig_mipi_dsi, bit, 16);
		} else {
			lcd_ana_setb(chreg_reg[i >> 1], 0x6, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], 0, bit, 16);
		}
	}
	lcd_combo_dphy_write(COMBO_DPHY_CNTL0, status ? 0x0 : 0xaaaaa);
}

static unsigned int lcd_phy_preem_level_to_val_txhd2(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int preem_value = 0;

	preem_value = (level >= 0xf) ? 0xf : level;

	return preem_value;
}

static unsigned int lcd_phy_amp_dft_txhd2(struct aml_lcd_drv_s *pdrv)
{
	unsigned int amp_value = 0;

	amp_value = 0x5;

	return amp_value;
}

static void lcd_phy_glb_param_dft_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	phy->cv_mode = 0;
	phy->ref_bias = 0;
	phy->vcm = 0x6f;
	phy->odt = 0x0;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_txhd2 = {
	.lane_num = 10,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_txhd2,
	.phy_amp_dft_val = lcd_phy_amp_dft_txhd2,
	.phy_lane_phase_sel_def = NULL,
	.phy_glb_param_dft_val = lcd_phy_glb_param_dft_txhd2,
	.phy_param_get = lcd_phy_param_get_from_reg,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = NULL,
	.phy_set_mipi = lcd_mipi_phy_set,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_txhd2(struct aml_lcd_data_s *pdata)
{
	return &lcd_phy_ctrl_txhd2;
}
#endif
