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

static void lcd_phy_cntl14_update(struct phy_config_s *phy, unsigned int cntl14)
{
	/* vswing */
	cntl14 &= ~(0xf << 26);
	cntl14 |= (phy->vswing & 0xf) << 26;

	/* vcm */
	if ((phy->flag & (1 << 1))) {
		cntl14 &= ~(0xf << 12);
		cntl14 |= (phy->vcm & 0xf) << 12;
	}
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
}

/*
 *    chreg: channel ctrl
 *    bypass: 1=bypass
 *    mode: 1=normal mode, 0=low common mode
 *    ckdi: clk phase for minilvds
 */
static void lcd_phy_cntl_set(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy, int status,
			     unsigned int flag, int bypass, unsigned int mode, unsigned int ckdi)
{
	unsigned int cntl15 = 0;
	unsigned int chreg, reg_data = 0, chdig = 0, dig_data = 0;
	unsigned char i, bit;
	unsigned char is_mlvds = pdrv->config.basic.lcd_type == LCD_MLVDS;
	unsigned char clk_phase_sel = 2;  //phase a

	u32 chreg_reg[5] = {
		ANACTRL_DIF_PHY_CNTL1, ANACTRL_DIF_PHY_CNTL2,
		ANACTRL_DIF_PHY_CNTL3, ANACTRL_DIF_PHY_CNTL4,
		ANACTRL_DIF_PHY_CNTL6,
	};
	u32 chdig_reg[5] = {
		ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
		ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
		ANACTRL_DIF_PHY_CNTL12,
	};

	if (!phy_ctrl_p)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d, bypass:%d\n", __func__, status, bypass);

	if (status) {
		reg_data = phy_ctrl_p->ctrl_bit_on << 0;
		dig_data = phy_ctrl_p->ctrl_bit_on << 15;
		cntl15 = 0x17300000;
		/* odt */
		if ((phy->flag & (1 << 3))) {
			cntl15 &= ~(0x1f << 24);
			cntl15 |= (phy->odt & 0x1f) << 24;
		}
	} else {
		cntl15 = 0x17900000;
		reg_data = (!phy_ctrl_p->ctrl_bit_on) << 0;
		dig_data = (!phy_ctrl_p->ctrl_bit_on) << 15;
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, 0x1300d100);
	}

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);

	for (i = 0; i < 10; i++) {
		if (flag & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			chreg = reg_data;
			if (ckdi & (1 << i)) {
				chreg |= clk_phase_sel << 1;
				clk_phase_sel ++;
			}
			chreg |= (ckdi & (0x1 << i)) ? (0x2 << 1) : 0x0;
			chdig = dig_data | 0x400 |
				((is_mlvds ? 0xf : 0) << 2); //pn swap

			if (status) {
				chreg |= (phy->lane[i].preem & 0xf) << 12;
				chreg |= (phy->lane[i].amp & 0xf) << 8;
			}
			lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
		}
	}
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0, flag;
	unsigned short lvds_flag_5lane_map0[2][2] = {
		{0x000f, 0x001f}, {0x01e0, 0x03e0}};
	unsigned char bit_idx = 0;

	if (pdrv->config.basic.lcd_bits == 6)
		bit_idx = 0;
	else if (pdrv->config.basic.lcd_bits == 8)
		bit_idx = 1;

	if (pdrv->config.control.lvds_cfg.dual_port) {
		flag = lvds_flag_5lane_map0[0][bit_idx] | lvds_flag_5lane_map0[1][bit_idx];
	} else {
		if (pdrv->config.control.lvds_cfg.port_swap)
			flag = lvds_flag_5lane_map0[1][bit_idx];
		else
			flag = lvds_flag_5lane_map0[0][bit_idx];
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d, flag=0x%04x\n", pdrv->index, __func__, status, flag);

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 =  0x1310d107;
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 1, 0);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 0, 0);
	}
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct mlvds_config_s *mlvds_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0, flag = 0;
	u8 i;
	u64 channel_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d\n", __func__, status);

	mlvds_conf = &pdrv->config.control.mlvds_cfg;
	channel_sel = mlvds_conf->channel_sel1;
	channel_sel = channel_sel << 32 | mlvds_conf->channel_sel0;
	for (i = 0; i < 12; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 =  0x1310d107;
		if (pdrv->config.basic.lcd_bits == 6)
			cntl14 |= (1 << 16);
		else if (pdrv->config.basic.lcd_bits == 8)
			cntl14 |= (2 << 16);
		else
			cntl14 |= (3 << 16);
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 1, mlvds_conf->pi_clk_sel);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 0, 0);
	}
}

static unsigned int lcd_phy_preem_level_to_val_t6d(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int preem_value = 0;

	if (pdrv->config.basic.lcd_type == LCD_LVDS || pdrv->config.basic.lcd_type == LCD_MLVDS)
		preem_value = (level >= 0xf) ? 0xf : level;

	return preem_value;
}

static unsigned int lcd_phy_amp_dft_t6d(struct aml_lcd_drv_s *pdrv)
{
	unsigned int amp_value = 0;

	if (pdrv->config.basic.lcd_type == LCD_LVDS || pdrv->config.basic.lcd_type == LCD_MLVDS)
		amp_value = 0x5;

	return amp_value;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t6d = {
	.lane_num = 12,
	.lane_lock = 0,
	.ctrl_bit_on = 1,
	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_amp_dft_val = lcd_phy_amp_dft_t6d,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_t6d,
	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = NULL,
	.phy_set_mipi = NULL,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t6d(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_t6d;
	return phy_ctrl_p;
}
