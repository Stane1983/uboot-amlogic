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

#if defined(CONFIG_MESON_T6X)
#define PHY_DFT_ODT_CM  0x30
#define PHY_DFT_ODT_VM  0x9
#define PHY_DFT_BIAS    0x10
static unsigned int cali_bias, cali_odt_cm, cali_odt_vm;

static unsigned int chreg_reg[8] = {
	ANACTRL_DIF_PHY_CNTL1, ANACTRL_DIF_PHY_CNTL2,
	ANACTRL_DIF_PHY_CNTL3, ANACTRL_DIF_PHY_CNTL4,
	ANACTRL_DIF_PHY_CNTL6, ANACTRL_DIF_PHY_CNTL7,
	ANACTRL_DIF_PHY_CNTL16, ANACTRL_DIF_PHY_CNTL17
};

static unsigned int chdig_reg[8] = {
	ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
	ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
	ANACTRL_DIF_PHY_CNTL12, ANACTRL_DIF_PHY_CNTL13,
	ANACTRL_DIF_PHY_CNTL18, ANACTRL_DIF_PHY_CNTL19
};

static unsigned int lcd_phy_get_dft_odt_cm(void)
{
	int efuse_odt = 0;

	efuse_odt = efuse_get_cali_item("p2p_vinlpcml");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse odt_cm=%#x\n", efuse_odt);
	if (efuse_odt <= 0) {
		efuse_odt = PHY_DFT_ODT_CM;
		LCDERR("odt_cm uncalibrated, use odt=%#x\n", efuse_odt);
	}
	return (unsigned int)efuse_odt;
}

static unsigned int lcd_phy_get_dft_odt_vm(void)
{
	int efuse_odt = 0;

	efuse_odt = efuse_get_cali_item("p2p_vinlpvml");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse odt_vm=%#x\n", efuse_odt);
	if (efuse_odt <= 0) {
		efuse_odt = PHY_DFT_ODT_VM;
		LCDERR("odt_vm uncalibrated, use odt=%#x\n", efuse_odt);
	}

	return (unsigned int)efuse_odt;
}

static unsigned int lcd_phy_get_dft_bias(void)
{
	int efuse_bias = 0;

	efuse_bias = efuse_get_cali_item("p2p_common");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse bias=%#x\n", efuse_bias);
	if (efuse_bias <= 0) {
		efuse_bias = PHY_DFT_BIAS;
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
		{ANACTRL_DIF_PHY_CNTL19, "PHY_CNTL19"}
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

/*
 * update odt based on efuse for display
 *   display_odt = DEF_ODT + (read_odt - cali_odt)
 */
static unsigned int lcd_phy_get_custom_odt_cm(struct phy_attr_s *phy)
{
	int cus_odt;
	unsigned int read_odt, data32;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	read_odt = (data32 >> 24) & 0x3f;
	if (read_odt) {
		cus_odt = PHY_DFT_ODT_CM + read_odt - cali_odt_cm;
		cus_odt = cus_odt < 0 ? 0 : cus_odt;
		cus_odt = cus_odt > 0x3f ? 0x3f : cus_odt;
	} else {
		cus_odt = 0;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("odt cali=0x%x, custom=0x%x\n", cali_odt_cm, cus_odt);
	return cus_odt;
}

static unsigned int lcd_phy_get_custom_odt_vm(struct phy_attr_s *phy)
{
	int cus_odt;
	unsigned int read_odt, data32;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	read_odt = (data32 >> 16) & 0xf;
	cus_odt = PHY_DFT_ODT_VM + read_odt - cali_odt_vm;
	cus_odt = cus_odt < 0 ? 0 : cus_odt;
	cus_odt = cus_odt > 0xf ? 0xf : cus_odt;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("odt cali=0x%x, custom=0x%x\n", cali_odt_vm, cus_odt);
	return cus_odt;
}

/*
 * update odt based on efuse trim value
 *   final_odt = cali_odt + (custom_odt - DEF_ODT)
 */
static unsigned int lcd_phy_get_final_odt_cm(struct phy_attr_s *phy)
{
	int odt = cali_odt_cm + phy->odt - PHY_DFT_ODT_CM;

	if (phy->odt) {
		odt = odt < 0 ? 0 : odt;
		odt = odt > 0x3f ? 0x3f : odt;
	} else {
		odt = 0;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("odt cali=0x%x, final=0x%x\n", cali_odt_cm, odt);
	return odt;
}

static unsigned int lcd_phy_get_final_odt_vm(struct phy_attr_s *phy)
{
	int odt = cali_odt_vm + phy->odt - PHY_DFT_ODT_VM;

	odt = odt < 0 ? 0 : odt;
	odt = odt > 0xf ? 0xf : odt;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("odt cali=0x%x, final=0x%x\n", cali_odt_vm, odt);
	return odt;
}

static int lcd_phy_param_get_from_reg(struct aml_lcd_drv_s *pdrv,
				      struct phy_config_s *phy_cfg, struct phy_attr_s *phy)
{
	unsigned int data32, chreg, chdig, bit, i;
	unsigned char vcm_boost_en, pn_swap;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	phy->cv_mode = (data32 >> 20) & 0x1;
	vcm_boost_en = (data32 >> 22) & 0x1;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL14);
	if (phy->cv_mode == PHY_VMODE) {
		phy->vcm = 0;
		phy->odt = lcd_phy_get_custom_odt_vm(phy);
	} else {
		phy->vcm = (data32 >> 12) & 0xf;
		phy->odt = lcd_phy_get_custom_odt_cm(phy);
	}
	phy->ref_bias = 0;
	phy->vswing = (data32 >> 26) & 0xf;

	for (i = 0; i < phy_cfg->lane_num; i++) {
		bit = i & 0x1 ? 16 : 0;
		chreg = lcd_ana_getb(chreg_reg[i >> 1], bit, 16);
		chdig = lcd_ana_getb(chdig_reg[i >> 1], bit, 16);

		phy_cfg->ch_ctrl[i].en = (chdig >> 15) & 0x1;

		if (phy->cv_mode == PHY_VMODE) {
			if (vcm_boost_en)
				phy->lane[i].amp = (chreg >> 1) & 0x7;
			else
				phy->lane[i].amp = 0;
			phy->lane[i].rterm = (chreg >> 8) & 0x7;
			phy->lane[i].preem = (chreg >> 12) & 0x7;
		} else {//default cmode
			phy->lane[i].amp = (chreg >> 8) & 0xf;
			phy->lane[i].rterm = 0; //only for vm
			phy->lane[i].preem = (chreg >> 12) & 0xf;
		}

		if (pdrv->config.basic.lcd_type == LCD_LVDS) {
			pn_swap = (chdig >> 3) & 0x1;
			phy_cfg->ch_ctrl[i].pn_swap = pn_swap;
		} else {
			pn_swap = (chdig >> 8) & 0x3;
			phy_cfg->ch_ctrl[i].pn_swap = (pn_swap == 0x2) ? 1 : 0;
		}
		phy_cfg->ch_ctrl[i].phase_sel = 0xff;
	}

	return 0;
}

static void lcd_phy_reset_t6x(struct aml_lcd_drv_s *pdrv)
{
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 19, 2);  //en=0 & reset
	udelay(50);
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 20, 1);  //en=1
	udelay(50);
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);  //work
}

static void lcd_phy_common_update(struct aml_lcd_drv_s *pdrv, unsigned int cntl14)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	unsigned int cntl15 = 0x30090000, boost_sel = 0;
	int i;

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
	cntl15 &= ~((0x3f << 24) | (0xf << 16));
	if (phy->cv_mode == PHY_VMODE)
		cntl15 |= (lcd_phy_get_final_odt_vm(phy)) << 16;
	else
		cntl15 |= (lcd_phy_get_final_odt_cm(phy)) << 24;

	/*vm control*/
	if (phy->cv_mode == PHY_VMODE) {
		for (i = 0; i < phy_cfg->lane_num; i++) {
			if ((phy_cfg->lane_valid & (1 << i)) == 0)
				continue;
			boost_sel += phy->lane[i].amp;
		}
		cntl15 |= (1 << 23) | (1 << 20);
		if (boost_sel)
			cntl15 |= (1 << 22);
	}

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);
}

static void lcd_phy_ch_set(struct aml_lcd_drv_s *pdrv)
{
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
		if (phy->cv_mode == PHY_CMODE) {
			chreg |= (phy->lane[i].preem & 0xf) << 12;
			chreg |= (phy->lane[i].amp & 0xf) << 8;
		} else {
			chreg |= (phy->lane[i].preem & 0x7) << 12;
			chreg |= (phy->lane[i].rterm & 0x7) << 8;
			chreg |= (phy->lane[i].amp & 0x7) << 1;
		}
		if (phy->cv_mode == PHY_CMODE)
			chdig |= 3 << 4;
		if (phy_cfg->ch_ctrl[i].pn_swap && phy_cfg->ch_ctrl[i].pn_swap != 0xff) {
			if (pdrv->config.basic.lcd_type == LCD_LVDS)
				chdig |= 1 << 3;
			else
				chdig |= 0x2 << 8;
		}
		if (phy_cfg->ch_clk_inv)
			chdig |= 1 << 10;

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
	if (phy->cv_mode == PHY_VMODE) {
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 0, 16, 4); //vinlp[3:0]=0
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 0, 22, 1); //vinlp[6]=0
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 0, 24, 6); //vinlp[13:8]=0
	} else {
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL15, 2, 22, 2); //vinlp[7:6]=2
	}

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

static void lcd_phy_init_off(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = NULL;
	struct p2p_config_s *p2p_conf;
	unsigned int cntl14 = 0x1140d100;
	unsigned int cntl15 = 0x30090000;
	unsigned int p2p_type;
	int i;

	if (!pdrv || !pdrv->config.phy_cfg.act_phy) {
		LCDERR("%s: invalid phy config\n", __func__);
		return;
	}
	phy = pdrv->config.phy_cfg.act_phy;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		cntl14 = 0x1140d100;
		break;
	case LCD_VBYONE:
		cntl14 = 0x1183a100;
		break;
	case LCD_P2P:
		cntl14 = 0x1180d100;
		p2p_conf = &pdrv->config.control.p2p_cfg;
		p2p_type = p2p_conf->p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_EPI:
			cntl14 |= (2 << 16); //div8
			break;
		case P2P_CHPI: /* 8/10 coding */
			cntl14 |= (3 << 16); //div10
			break;
		case P2P_USIT: /* 8/10 coding */
			cntl14 |= (3 << 16); //div10
			break;
		case P2P_CSPI:
		case P2P_ISP:
			cntl14 |= (2 << 16); //div8
			break;
		default:
			break;
		}
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
		cntl15 &= ~(0xf << 16);
		cntl15 &= ~(0x1 << 22);
		cntl15 &= ~(0x3f << 24);
		cntl15 |= 1 << 20;  //vinlp[4]=1
		cntl15 |= 1 << 23;  //vinlp[7]=1
	} else {
		cntl15 &= ~(0xf << 16);
		cntl15 &= ~(1 << 20);  //vinlp[4]=0
		cntl15 &= ~(1 << 22);  //vinlp[6]=0
		cntl15 |= 1 << 23;  //vinlp[7]=1
	}
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
	unsigned int cntl14 = 0x01400100; //div7

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
		lcd_phy_reset_t6x(pdrv);
		lcd_phy_ch_set(pdrv);
		break;
	case LCD_PHY_PWR_DOWN:
		lcd_phy_cntl_disable(pdrv);
		break;
	default:
		break;
	}
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x01830100; //div10

	switch (status) {
	case LCD_PHY_ON:
		lcd_phy_cntl_output(pdrv, 1);
		break;
	case LCD_PHY_OFF:
		lcd_phy_cntl_output(pdrv, 0);
		break;
	case LCD_PHY_PWR_UP:
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_reset_t6x(pdrv);
		lcd_phy_ch_set(pdrv);
		break;
	case LCD_PHY_PWR_DOWN:
		lcd_phy_cntl_disable(pdrv);
		break;
	default:
		break;
	}
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct p2p_config_s *p2p_conf;
	unsigned int p2p_type;
	unsigned int cntl14 = 0x01800100;

	p2p_conf = &pdrv->config.control.p2p_cfg;
	switch (status) {
	case LCD_PHY_ON:
		lcd_phy_cntl_output(pdrv, 1);
		break;
	case LCD_PHY_OFF:
		lcd_phy_cntl_output(pdrv, 0);
		break;
	case LCD_PHY_PWR_UP:
		p2p_type = p2p_conf->p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_EPI:
			pdrv->config.phy_cfg.low_common_mode = 0;
			cntl14 |= (2 << 16); //div8
			break;
		case P2P_CHPI: /* 8/10 coding */
			pdrv->config.phy_cfg.low_common_mode = 1;
			pdrv->config.phy_cfg.weakly_pull_down = 1;
			cntl14 |= (3 << 16); //div10
			break;
		case P2P_USIT: /* 8/10 coding */
			pdrv->config.phy_cfg.low_common_mode = 1;
			cntl14 |= (3 << 16); //div10
			break;
		case P2P_CSPI:
		case P2P_ISP:
			pdrv->config.phy_cfg.low_common_mode = 1;
			cntl14 |= (2 << 16); //div8
			break;
		default:
			LCDERR("%s: invalid p2p_type 0x%x\n", __func__, p2p_type);
			return;
		}
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_reset_t6x(pdrv);
		lcd_phy_ch_set(pdrv);
		break;
	case LCD_PHY_PWR_DOWN:
		lcd_phy_cntl_disable(pdrv);
		break;
	default:
		break;
	}
}

static unsigned int lcd_phy_vswing_level_to_val_t6x(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	if (level < 0xf)
		return level;
	LCDERR("[%d]: %s: level %d invalid\n", pdrv->index, __func__, level);
	return 0;
}

static unsigned int lcd_phy_preem_level_to_val_t6x(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	unsigned int phy_cmode_preem[8] = {0x0, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xf};

	if (phy->cv_mode == PHY_VMODE) {
		if (level < 8)
			return level;
	} else { //default cmode
		if (level < 8)
			return phy_cmode_preem[level];
	}

	LCDERR("[%d]: %s: level %d invalid\n", pdrv->index, __func__, level);
	return 0;
}

static unsigned int lcd_phy_amp_dft_t6x(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.basic.lcd_type == LCD_VBYONE)
		return 0x4;

	return 0x0;
}

static unsigned int lcd_phy_rterm_dft_t6x(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (phy->cv_mode == PHY_VMODE)
		return 0x4;

	return 0x0;
}

static void phy_glb_param_dft_t6x(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	phy->ref_bias = 0;
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
	case LCD_MLVDS:
		phy->vcm = 0xd;
		phy->odt = PHY_DFT_ODT_CM;
		phy->cv_mode = PHY_CMODE;
		break;
	case LCD_VBYONE:
		phy->vcm = 0xa;
		phy->odt = PHY_DFT_ODT_CM;
		phy->cv_mode = PHY_CMODE;
		break;
	case LCD_P2P:
		switch (pdrv->config.control.p2p_cfg.p2p_type & 0x1f) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			phy->vcm = 0xd;
			phy->odt = PHY_DFT_ODT_CM;
			phy->cv_mode = PHY_CMODE;
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			phy->vcm = 0;
			phy->odt = PHY_DFT_ODT_VM; /* default 400mV */
			phy->cv_mode = PHY_VMODE;
			break;
		default:
			return;
		}
		break;
	default:
		break;
	}
}

static unsigned char lcd_phy_lane_pn_swap_def(struct aml_lcd_drv_s *pdrv, unsigned int lane)
{
	unsigned char pn_swap = 0;

	if (lane >= 16)
		return 0xff;

	return pn_swap;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t6x = {
	.lane_num = 16,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_val_t6x,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_t6x,
	.phy_amp_dft_val = lcd_phy_amp_dft_t6x,
	.phy_rterm_dft_val = lcd_phy_rterm_dft_t6x,
	.phy_glb_param_dft_val = phy_glb_param_dft_t6x,
	.phy_lane_phase_sel_def = NULL,
	.phy_lane_pn_swap_dft = lcd_phy_lane_pn_swap_def,
	.phy_param_get = lcd_phy_param_get_from_reg,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_reset = lcd_phy_reset_t6x,
	.phy_default_off = lcd_phy_init_off,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = NULL,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t6x(struct aml_lcd_data_s *pdata)
{
	cali_odt_cm = lcd_phy_get_dft_odt_cm();
	cali_odt_vm = lcd_phy_get_dft_odt_vm();
	cali_bias = lcd_phy_get_dft_bias();

	//lcd_phy_init_off();

	return &lcd_phy_ctrl_t6x;
}
#endif
