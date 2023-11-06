// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

static struct lcd_phy_ctrl_s *phy_ctrl_p;

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

static void lcd_phy_cntl14_update(struct phy_config_s *phy, unsigned int cntl14)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("vswing=0x%x\n", phy->vswing);

	/* vswing */
	cntl14 &= ~(0xf << 12);
	cntl14 |= (phy->vswing << 12);

	/* vcm */
	if ((phy->flag & (1 << 1))) {
		cntl14 &= ~(0xff << 4);
		cntl14 |= (phy->vcm & 0xff) << 4;
	}
	/* odt */
	if ((phy->flag & (1 << 3))) {
		cntl14 &= ~(0xff << 23);
		cntl14 |= (phy->odt & 0xff) << 23;
	}
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, cntl14);
}

static void lcd_phy_cntl_lvds_set(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy,
			unsigned int status, unsigned int flag, unsigned int ckdi)
{
	unsigned int chreg = 0, chdig = 0;
	unsigned int i, bit, reg_data, dig_data;
	unsigned char is_mlvds = pdrv->config.basic.lcd_type == LCD_MLVDS;

	if (!phy_ctrl_p)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d, ckdi:0x%x\n", __func__, status, ckdi);

	if (status) {
		if (is_mlvds) {
			reg_data = p2p_phy_ch_reg_lvds & 0xe1c7;
			dig_data = p2p_phy_ch_dig_mlvds;
			lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL13, ckdi & 0x3ff, 16, 10);
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

	for (i = 0; i < 10; i++) {
		if (flag & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			chreg = reg_data;
			chdig = dig_data;
			if (status) {
				if (((ckdi & (1 << i)) == 0) && is_mlvds) { //data lane
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
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int flag;
	unsigned short lvds_flag_5lane[2][2] = {{0x000f, 0x001f}, {0x01e0, 0x03e0}};
	unsigned char bit_idx;

	bit_idx = pdrv->config.basic.lcd_bits == 6 ? 0 : 1;

	if (pdrv->config.control.lvds_cfg.dual_port) {
		flag = lvds_flag_5lane[0][bit_idx] | lvds_flag_5lane[1][bit_idx];
	} else {
		if (pdrv->config.control.lvds_cfg.port_swap)
			flag = lvds_flag_5lane[1][bit_idx];
		else
			flag = lvds_flag_5lane[0][bit_idx];
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d, flag=0x%04x\n", pdrv->index, __func__, status, flag);

	if (status)
		lcd_phy_cntl14_update(phy, 0x106f1);
	else
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x0);

	lcd_phy_cntl_lvds_set(pdrv, phy, status, flag, 0);
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct mlvds_config_s *mlvds_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int flag = 0;
	unsigned char i;
	unsigned long long channel_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	mlvds_conf = &pdrv->config.control.mlvds_cfg;
	channel_sel = mlvds_conf->channel_sel1;
	channel_sel = channel_sel << 32 | mlvds_conf->channel_sel0;
	for (i = 0; i < 10; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		lcd_phy_cntl14_update(phy, 0x106f1);
		lcd_phy_cntl_lvds_set(pdrv, phy, status, flag, mlvds_conf->pi_clk_sel);
		lcd_combo_dphy_write(COMBO_DPHY_CNTL0, 0x55555);
	} else {
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x0);
		lcd_phy_cntl_lvds_set(pdrv, phy, status, flag, 0);
	}
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned char bit, i, flag;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	if (status) {
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL13, 0x00000099);
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0x7f820613);
	}
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL15, 0);

	if (pdrv->config.control.mipi_cfg.lane_num == 1)
		flag = 0x5;
	else if (pdrv->config.control.mipi_cfg.lane_num == 2)
		flag = 0x7;
	else if (pdrv->config.control.mipi_cfg.lane_num == 3)
		flag = 0xf;
	else
		flag = 0x1f;

	for (i = 0; i < 10; i++) {
		bit = i % 2 ? 16 : 0;
		if (flag & (1 << i) && status) {
			lcd_ana_setb(chreg_reg[i >> 1], p2p_phy_ch_reg_mipi_dsi, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], p2p_phy_ch_dig_mipi_dsi, bit, 16);
		} else {
			lcd_ana_setb(chreg_reg[i >> 1], 0x6, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], 0, bit, 16);
		}
	}
}

static unsigned int lcd_phy_preem_level_to_val_txhd2(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int preem_value = 0;

	if (pdrv->config.basic.lcd_type == LCD_LVDS || pdrv->config.basic.lcd_type == LCD_MLVDS)
		preem_value = (level >= 0xf) ? 0xf : level;

	return preem_value;
}

static unsigned int lcd_phy_amp_dft_txhd2(struct aml_lcd_drv_s *pdrv)
{
	unsigned int amp_value = 0;

	if (pdrv->config.basic.lcd_type == LCD_LVDS || pdrv->config.basic.lcd_type == LCD_MLVDS)
		amp_value = 0x5;

	return amp_value;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_txhd2 = {
	.lane_lock = 0,
	.ctrl_bit_on = 0,
	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_amp_dft_val = lcd_phy_amp_dft_txhd2,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_txhd2,
	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = NULL,
	.phy_set_mipi = lcd_mipi_phy_set,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_txhd2(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_txhd2;
	return phy_ctrl_p;
}
