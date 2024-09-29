// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#ifdef CONFIG_MESON_T5M
static unsigned int chreg_reg[6] = {
	ANACTRL_DIF_PHY_CNTL1, ANACTRL_DIF_PHY_CNTL2,
	ANACTRL_DIF_PHY_CNTL3, ANACTRL_DIF_PHY_CNTL4,
	ANACTRL_DIF_PHY_CNTL6, ANACTRL_DIF_PHY_CNTL7
};

static unsigned int chdig_reg[6] = {
	ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
	ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
	ANACTRL_DIF_PHY_CNTL12, ANACTRL_DIF_PHY_CNTL13
};

static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{ANACTRL_DIF_PHY_CNTL1,  "PHY_CNTL1"},
		{ANACTRL_DIF_PHY_CNTL2,  "PHY_CNTL2"},
		{ANACTRL_DIF_PHY_CNTL3,  "PHY_CNTL3"},
		{ANACTRL_DIF_PHY_CNTL4,  "PHY_CNTL4"},
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
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static int lcd_phy_param_get_from_reg(struct aml_lcd_drv_s *pdrv,
				      struct phy_config_s *phy_cfg, struct phy_attr_s *phy)
{
	unsigned int data32, chreg, chdig, bit;
	int i;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL14);
	phy->vswing = data32 & 0xf;
	phy->vcm = (data32 >> 4) & 0x7df;
	phy->ref_bias = (data32 >> 15) & 0x1;
	phy->odt = (data32 >> 24) & 0xff;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	phy->cv_mode = (data32 >> 19) & 0x1;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL16);
	phy_cfg->ckdi = (data32 >> 12) & 0xfff;

	for (i = 0; i < phy_cfg->lane_num; i++) {
		bit = i & 0x1 ? 16 : 0;
		chreg = lcd_ana_getb(chreg_reg[i >> 1], bit, 16);
		chdig = lcd_ana_getb(chdig_reg[i >> 1], bit, 16);

		phy->lane[i].preem = (chreg >> 8) & 0xff;
		phy->lane[i].amp = (chdig >> 3) & 0x7;
	}

	return 0;
}

static void lcd_phy_common_update(struct aml_lcd_drv_s *pdrv, unsigned int cntl14)
{
	unsigned int cntl15 = 0, cntl16 = 0;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	cntl14 &= ~(0xf);
	cntl14 |= phy->vswing;

	/* vcm */
	cntl14 &= ~(0x7df << 4); //left bit[9] for bandgap
	cntl14 |= (phy->vcm & 0x7df) << 4;
	/* ref bias switch */
	cntl14 &= ~(1 << 15);
	cntl14 |= (phy->ref_bias & 0x1) << 15;
	/* odt */
	cntl14 &= ~(0xff << 24);
	cntl14 |= (phy->odt & 0xff) << 24;
	//bandgap
	cntl14 |= (1 << 9);

	if (phy->cv_mode == PHY_VMODE) //0=cm, 1=vm
		cntl15 = 0x000e0000;
	else
		cntl15 = 0x00070000;

	cntl16 = (phy_cfg->ckdi << 12) | 0x80000000;

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL16, cntl16);
}

static void lcd_phy_cntl_set(struct aml_lcd_drv_s *pdrv, int status, int bypass)
{
	unsigned int chreg, reg_data = 0, chdig = 0;
	unsigned char i, bit;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d, bypass:%d\n", __func__, status, bypass);

	if (status) {
		reg_data = 1; //bit[0]=1
		if (phy->cv_mode == PHY_VMODE)
			reg_data |= 0x000b;
		else
			reg_data |= 0x0002;
		if (phy_cfg->weakly_pull_down)
			reg_data &= ~(1 << 3);
	} else {
		reg_data = 0; //bit[0]=0
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, 0);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, 0);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL16, 0);
	}

	for (i = 0; i < 12; i++) {
		if (phy_cfg->lane_valid & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			chreg = reg_data;
			chdig = ((phy_cfg->ckdi & (0x1 << i)) == 0) && bypass ? 0x4 : 0;
			if (status) {
				chreg |= (phy->lane[i].preem & 0xff) << 8;
				chdig |= (phy->lane[i].amp & 0x7) << 3;
			}
			lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
		}
	}
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0;

	if (status) {
		cntl14 = 0xff2027e0;
		lcd_phy_common_update(pdrv, cntl14);
	}
	lcd_phy_cntl_set(pdrv, status, 1);
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0;

	if (status) {
		if (pdrv->config.phy_cfg.ext_pullup)
			cntl14 = 0xff2027e0;
		else
			cntl14 = 0xf02027a0;
		lcd_phy_common_update(pdrv, cntl14);
	}
	lcd_phy_cntl_set(pdrv, status, 1);
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0;

	if (status) {
		cntl14 = 0xff2027e0;
		lcd_phy_common_update(pdrv, cntl14);
	}
	lcd_phy_cntl_set(pdrv, status, 1);
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct p2p_config_s *p2p_conf;
	unsigned int p2p_type, vcm_flag;
	unsigned int cntl14;

	p2p_conf = &pdrv->config.control.p2p_cfg;
	if (status) {
		p2p_type = p2p_conf->p2p_type & 0x1f;
		vcm_flag = (p2p_conf->p2p_type >> 5) & 0x1;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			phy_cfg->low_common_mode = 0;
			cntl14 = 0xff2027a0;
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			phy_cfg->low_common_mode = 1;
			if (p2p_type == P2P_CHPI)
				phy_cfg->weakly_pull_down = 1;

			if (vcm_flag) /* 580mV */
				cntl14 = 0xe0600272;
			else /* default 385mV */
				cntl14 = 0xfe60027f;
			break;
		default:
			LCDERR("%s: invalid p2p_type %d\n", __func__, p2p_type);
			return;
		}
		lcd_phy_common_update(pdrv, cntl14);
	}
	lcd_phy_cntl_set(pdrv, status, 1);
}

static unsigned int lcd_phy_amp_dft_t5m(struct aml_lcd_drv_s *pdrv)
{
	unsigned int amp_value = 0x7;

	return amp_value;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t5m = {
	.lane_num = 12,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_value_dft,
	.phy_amp_dft_val = lcd_phy_amp_dft_t5m,
	.phy_lane_phase_sel_def = NULL,
	.phy_glb_param_dft_val = lcd_phy_glb_param_dft,
	.phy_param_get = lcd_phy_param_get_from_reg,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t5m(struct aml_lcd_data_s *pdata)
{
	return &lcd_phy_ctrl_t5m;
}
#endif
