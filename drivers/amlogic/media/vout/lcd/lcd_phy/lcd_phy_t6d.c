// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/aml_efuse.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#ifdef CONFIG_MESON_T6D
#define PHY_DEF_ODT  0x17
#define PHY_DEF_BIAS 0x10
static unsigned int cali_bias, cali_odt, phy_ctrl_bit_on;
static u32 chreg_reg[] = {
	ANACTRL_DIF_PHY_CNTL1, ANACTRL_DIF_PHY_CNTL2,
	ANACTRL_DIF_PHY_CNTL3, ANACTRL_DIF_PHY_CNTL4,
	ANACTRL_DIF_PHY_CNTL6,
};
static u32 chdig_reg[] = {
	ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
	ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
	ANACTRL_DIF_PHY_CNTL12,
};

static unsigned int lcd_phy_get_def_odt(void)
{
	int efuse_odt = 0;

	efuse_odt = efuse_get_cali_item("p2p_vinlp");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse odt=%#x\n", efuse_odt);
	if (efuse_odt < 0) {
		efuse_odt = PHY_DEF_ODT;
		LCDERR("odt uncalibrated, use odt=%#x\n", efuse_odt);
	}
	return (unsigned int)efuse_odt;
}

static unsigned int lcd_phy_get_def_bias(void)
{
	int efuse_bias = 0;

	efuse_bias = efuse_get_cali_item("p2p_common");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse bias=%#x\n", efuse_bias);
	if (efuse_bias < 0) {
		efuse_bias = PHY_DEF_BIAS;
		LCDERR("bias uncalibrated, use bias=%#x\n", efuse_bias);
	}
	return (unsigned int)efuse_bias;
}

static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{ANACTRL_DIF_PHY_CNTL1,  "PHY_CNTL1"},
		{ANACTRL_DIF_PHY_CNTL2,  "PHY_CNTL2"},
		{ANACTRL_DIF_PHY_CNTL3,  "PHY_CNTL3"},
		{ANACTRL_DIF_PHY_CNTL4,  "PHY_CNTL4"},
		{ANACTRL_DIF_PHY_CNTL6,  "PHY_CNTL6"},
		{ANACTRL_DIF_PHY_CNTL8,  "PHY_CNTL8"},
		{ANACTRL_DIF_PHY_CNTL9,  "PHY_CNTL9"},
		{ANACTRL_DIF_PHY_CNTL10, "PHY_CNTL10"},
		{ANACTRL_DIF_PHY_CNTL11, "PHY_CNTL11"},
		{ANACTRL_DIF_PHY_CNTL12, "PHY_CNTL12"},
		{ANACTRL_DIF_PHY_CNTL14, "PHY_CNTL14"},
		{ANACTRL_DIF_PHY_CNTL15, "PHY_CNTL15"},
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0,
			reg_table, ARRAY_SIZE(reg_table));
}

#define PHY_GET_PHASE_REG_BY_SEL 0
#define PHY_GET_PHASE_SEL_BY_REG 1
static int lcd_phy_get_phase(struct phy_config_s *phy_cfg, int opr, unsigned char sel)
{
	int i, res = -1, temp1, temp2;
	struct {
		unsigned char sel;
		unsigned char reg;
	} clk_phase_tbl[] = {
		{PHY_PHASE_0, 0x0},  //phase_0
		{PHY_PHASE_A, 0x2},  //phase_a
		{PHY_PHASE_B, 0x3},  //phase_b
	};

	for (i = 0; i < ARRAY_SIZE(clk_phase_tbl); i++) {
		if (opr == PHY_GET_PHASE_REG_BY_SEL) {
			temp1 = clk_phase_tbl[i].sel;
			temp2 = clk_phase_tbl[i].reg;
		} else {
			temp2 = clk_phase_tbl[i].sel;
			temp1 = clk_phase_tbl[i].reg;
		}
		if (temp1 == sel) {
			res = temp2;
			break;
		}
	}
	return res;
}

/*
 * update odt based on efuse for display
 *   display_odt = DEF_ODT + (read_odt - cali_odt)
 */
static unsigned int lcd_phy_get_display_odt(struct phy_attr_s *phy)
{
	int disp_odt;
	unsigned int read_odt, data32;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	read_odt = (data32 >> 24) & 0x1f;
	disp_odt = PHY_DEF_ODT + read_odt - cali_odt;
	disp_odt = disp_odt < 0 ? 0 : disp_odt;
	disp_odt = disp_odt > 0x1f ? 0x1f : disp_odt;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("cali odt=0x%x, display odt=0x%x\n", cali_odt, disp_odt);
	return disp_odt;
}

/*
 * update odt based on efuse trim value
 *   write_odt = cali_odt + (custom_odt - DEF_ODT)
 */
static unsigned int lcd_phy_get_write_odt(struct phy_attr_s *phy)
{
	int odt = cali_odt + phy->odt - PHY_DEF_ODT;

	odt = odt < 0 ? 0 : odt;
	odt = odt > 0x1f ? 0x1f : odt;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("cali odt=0x%x, write odt=0x%x\n", cali_odt, odt);
	return odt;
}

static int lcd_phy_param_get_from_reg(struct aml_lcd_drv_s *pdrv,
				      struct phy_config_s *phy_cfg, struct phy_attr_s *phy)
{
	unsigned int data32, chreg, chdig, bit, i;
	unsigned char phase_sel, pn_swap;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL14);
	phy->vswing = (data32 >> 26) & 0xf;
	phy->vcm = (data32 >> 12) & 0xf;
	phy->ref_bias = 0;
	phy->cv_mode = 0;
	phy->odt = lcd_phy_get_display_odt(phy) & 0x1f;

	for (i = 0; i < phy_cfg->lane_num; i++) {
		bit = i & 0x1 ? 16 : 0;
		chreg = lcd_ana_getb(chreg_reg[i >> 1], bit, 16);
		chdig = lcd_ana_getb(chdig_reg[i >> 1], bit, 16);

		phy->lane[i].preem = (chreg >> 12) & 0xf;
		phy->lane[i].amp = (chreg >> 8) & 0xf;
		phase_sel = (chreg >> 1) & 0x3;
		pn_swap = (chdig >> 2) & 0xf;
		phy_cfg->ch_ctrl[i].phase_sel = lcd_phy_get_phase(phy_cfg,
								  PHY_GET_PHASE_SEL_BY_REG,
								  phase_sel);
		phy_cfg->ch_ctrl[i].pn_swap = (pn_swap == 0xf) ? 1 : 0;
	}

	return 0;
}

static void lcd_phy_reset_t6d(struct aml_lcd_drv_s *pdrv)
{
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 19, 2);  //en=0 & reset
	udelay(50);
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 20, 1);  //en=1
	udelay(50);
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);  //work
}

static void lcd_phy_common_update(struct aml_lcd_drv_s *pdrv, unsigned int cntl14)
{
	unsigned int cntl15 = 0x17300000;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (phy_ctrl_bit_on)
		cntl15 = 0x17b00000;

	/* vswing */
	cntl14 &= ~(0xf << 26);
	cntl14 |= (phy->vswing & 0xf) << 26;

	/* vcm */
	cntl14 &= ~(0xf << 12);
	cntl14 |= (phy->vcm & 0xf) << 12;

	/* ref_bias */
	cntl14 &= ~(0x1f << 4);
	cntl14 |= (cali_bias & 0x1f) << 4;

	/* odt */
	cntl15 &= ~(0x1f << 24);
	cntl15 |= (lcd_phy_get_write_odt(phy) & 0x1f) << 24;

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);
}

static void lcd_phy_ch_set(struct aml_lcd_drv_s *pdrv)
{
	int sel = -1;
	unsigned int chreg, chdig = 0;
	unsigned char i, bit;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: ckdi:0x%x\n", __func__, phy_cfg->ckdi);

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if ((phy_cfg->lane_valid & (1 << i)) == 0)
			continue;

		bit = i & 0x1 ? 16 : 0;
		chreg = 1 << 0;
		chdig = 1 << 15;
		chreg |= (phy->lane[i].preem & 0xf) << 12;
		chreg |= (phy->lane[i].amp & 0xf) << 8;
		if (phy_cfg->ch_ctrl[i].pn_swap && phy_cfg->ch_ctrl[i].pn_swap != 0xff)
			chdig |= 0xf << 2;
		if (phy_cfg->ch_clk_inv)
			chdig |= 1 << 10;
		if (phy_cfg->ch_phase_en) {
			sel = lcd_phy_get_phase(phy_cfg, PHY_GET_PHASE_REG_BY_SEL,
						phy_cfg->ch_ctrl[i].phase_sel);
			if (sel < 0) {
				LCDERR("[%d]: err ch[%d].phase_sel=%#x\n",
				       pdrv->index, i, phy_cfg->ch_ctrl[i].phase_sel);
			} else {
				chreg |= ((unsigned char)sel) << 1;
			}
		}
		lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
		lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
	}
}

static void lcd_phy_cntl_disable(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	unsigned char i, bit;

	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 0, 3);  //common0[2:0]=0
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 19, 2); //common1[4:3]=0
	if (phy->cv_mode == PHY_VMODE)
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 0, 24, 5); //vinlp[12:8]=0
	if (phy_ctrl_bit_on)
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 2, 22, 2); //vinlp[7:6]=2
	else
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 0, 22, 2); //vinlp[7:6]=0

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if ((phy_cfg->lane_valid & (1 << i)) == 0)
			continue;
		bit = i & 0x1 ? 16 : 0;
		lcd_ana_setb(chreg_reg[i >> 1], 0, bit, 16);
		lcd_ana_setb(chdig_reg[i >> 1], 0, bit, 16);
	}
}

static void lcd_phy_cntl_output(struct aml_lcd_drv_s *pdrv, int status)
{
	if (status)
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 7, 0, 3);  //common0[2:0]=7
	else
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 0, 3);
}

static void lcd_phy_init_off_t6d(struct aml_lcd_drv_s *pdrv)
{
	unsigned int cntl14 = 0x1300d100, cntl15 = 0x17300000;
	struct phy_attr_s *phy = NULL;
	int i;

	if (!pdrv || !pdrv->config.phy_cfg.act_phy) {
		LCDERR("%s: invalid phy config\n", __func__);
		return ;
	}
	phy = pdrv->config.phy_cfg.act_phy;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_MLVDS:
		if (pdrv->config.timing.dft_timing->lcd_bits == 18)
			cntl14 |= (1 << 16);
		else if (pdrv->config.timing.dft_timing->lcd_bits == 24)
			cntl14 |= (2 << 16);
		else
			cntl14 |= (3 << 16);
		break;
	default:
		break;
	}

	/* vswing */
	cntl14 &= ~(0xf << 26);
	cntl14 |= (phy->vswing & 0xf) << 26;

	/* vcm */
	cntl14 &= ~(0xf << 12);
	cntl14 |= (phy->vcm & 0xf) << 12;

	/* ref_bias */
	cntl14 &= ~(0x1f << 4);
	cntl14 |= (cali_bias & 0x1f) << 4;

	if (phy->cv_mode == PHY_VMODE) {
		cntl15 &= ~(0x1 << 22);
		cntl15 &= ~(0x1f << 24);
		cntl15 |= 1 << 19;  //vinlp[3]=1
	} else {
		cntl15 &= ~(1 << 19);  //vinlp[3]=0
	}
	cntl15 &= ~(3 << 22);  //vinlp[7:6]=0
	if (phy_ctrl_bit_on)
		cntl15 |= 2 << 22; //vinlp[7:6]=2

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);

	for (i = 0; i < ARRAY_SIZE(chreg_reg); i++) {
		lcd_ana_write(chreg_reg[i], 0);
		lcd_ana_write(chdig_reg[i], 0);
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: set cntl14=%#x, cntl15=%#x\n", __func__, cntl14, cntl15);
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x0300d100;

	pdrv->config.phy_cfg.ch_clk_inv = 1;
	switch (status) {
	case LCD_PHY_ON:
		lcd_phy_cntl_output(pdrv, 1);
		break;
	case LCD_PHY_OFF:
		lcd_phy_cntl_output(pdrv, 0);
		break;
	case LCD_PHY_PWR_UP:
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_reset_t6d(pdrv);
		lcd_phy_ch_set(pdrv);
		break;
	case LCD_PHY_PWR_DOWN:
		lcd_phy_cntl_disable(pdrv);
		break;
	default:
		break;
	}
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x0300d100;

	pdrv->config.phy_cfg.ch_phase_en = 1;
	switch (status) {
	case LCD_PHY_ON:
		lcd_phy_cntl_output(pdrv, 1);
		break;
	case LCD_PHY_OFF:
		lcd_phy_cntl_output(pdrv, 0);
		break;
	case LCD_PHY_PWR_UP:
		if (pdrv->config.timing.act_timing.lcd_bits == 18)
			cntl14 |= (1 << 16);
		else if (pdrv->config.timing.act_timing.lcd_bits == 24)
			cntl14 |= (2 << 16);
		else
			cntl14 |= (3 << 16);
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_reset_t6d(pdrv);
		lcd_phy_ch_set(pdrv);
		break;
	case LCD_PHY_PWR_DOWN:
		lcd_phy_cntl_disable(pdrv);
		break;
	default:
		break;
	}
}

static unsigned int lcd_phy_preem_level_to_val_t6d(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	return (level >= 0xf) ? 0xf : level;
}

static unsigned int lcd_phy_amp_dft_t6d(struct aml_lcd_drv_s *pdrv)
{
	return 0x5;
}

static unsigned char lcd_phy_lane_phase_sel_def(struct aml_lcd_drv_s *pdrv, unsigned int lane)
{
	unsigned char phase_sel_tbl[] = {
		PHY_PHASE_0,  //lane_0  d0_a
		PHY_PHASE_0,  //lane_1  d1_a
		PHY_PHASE_0,  //lane_2  d2_a
		PHY_PHASE_A,  //lane_3  clk_a
		PHY_PHASE_0,  //lane_4  d3_a
		PHY_PHASE_0,  //lane_5  d0_b
		PHY_PHASE_0,  //lane_6  d1_b
		PHY_PHASE_0,  //lane_7  d2_b
		PHY_PHASE_B,  //lane_8  clk_b
		PHY_PHASE_0,  //lane_9  d3_b
	};

	if (pdrv->config.basic.lcd_type != LCD_MLVDS ||
			lane >= ARRAY_SIZE(phase_sel_tbl))
		return 0xff;

	return phase_sel_tbl[lane];
}

static void phy_glb_param_dft_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	phy->ref_bias = 0;
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
	case LCD_MLVDS:
		phy->vcm = 0xd;
		phy->odt = PHY_DEF_ODT;
		phy->cv_mode = 0;
		break;
	default:
		break;
	}
}

static unsigned char lcd_phy_lane_pn_swap_def(struct aml_lcd_drv_s *pdrv, unsigned int lane)
{
	unsigned char pn_swap = 0;

	if (lane >= 10)
		return 0xff;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_MLVDS:
		pn_swap = 1;
		break;
	default:
		pn_swap = 0;
		break;
	}

	return pn_swap;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t6d = {
	.lane_num = 10,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_amp_dft_val = lcd_phy_amp_dft_t6d,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_t6d,
	.phy_lane_phase_sel_def = lcd_phy_lane_phase_sel_def,
	.phy_lane_pn_swap_dft = lcd_phy_lane_pn_swap_def,
	.phy_glb_param_dft_val = phy_glb_param_dft_t6d,
	.phy_param_get = lcd_phy_param_get_from_reg,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_reset = lcd_phy_reset_t6d,
	.phy_default_off = lcd_phy_init_off_t6d,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = NULL,
	.phy_set_mipi = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t6d(struct aml_lcd_data_s *pdata)
{
	cali_odt = lcd_phy_get_def_odt();
	cali_bias = lcd_phy_get_def_bias();
	phy_ctrl_bit_on = (pdata->rev_type > 0xa) ? 1 : 0;

	return &lcd_phy_ctrl_t6d;
}
#endif
