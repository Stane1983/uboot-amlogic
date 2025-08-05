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

#ifdef CONFIG_MESON_T6W
#define PHY_DFT_ODT_CM  0x30
#define PHY_DFT_ODT_VM  0x9
#define PHY_DFT_BIAS    0x10
static unsigned int cali_bias, cali_odt_cm, cali_odt_vm;
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

static unsigned int lcd_phy_get_dft_odt_cm(void)
{
	int efuse_odt = 0;

	efuse_odt = efuse_get_cali_item("p2p_vinlp");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse odt_cm=%#x\n", efuse_odt);
	if (efuse_odt < 0) {
		efuse_odt = PHY_DFT_ODT_CM;
		LCDERR("odt_cm uncalibrated, use odt=%#x\n", efuse_odt);
	}
	return (unsigned int)efuse_odt;
}

static unsigned int lcd_phy_get_dft_odt_vm(void)
{
	int efuse_odt = 0;

	efuse_odt = PHY_DFT_ODT_VM;
	LCDERR("TODO: odt_vm uncalibrated, use odt=%#x\n", efuse_odt);

	return (unsigned int)efuse_odt;
}

static unsigned int lcd_phy_get_dft_bias(void)
{
	int efuse_bias = 0;

	efuse_bias = efuse_get_cali_item("p2p_common");
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("efuse bias=%#x\n", efuse_bias);
	if (efuse_bias < 0) {
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
		{ANACTRL_DIF_PHY_CNTL15, "PHY_CNTL15"}
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
static unsigned int lcd_phy_get_custom_odt_cm(struct phy_attr_s *phy)
{
	int cus_odt;
	unsigned int read_odt, data32;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	read_odt = (data32 >> 24) & 0x3f;
	cus_odt = PHY_DFT_ODT_CM + read_odt - cali_odt_cm;
	cus_odt = cus_odt < 0 ? 0 : cus_odt;
	cus_odt = cus_odt > 0x3f ? 0x3f : cus_odt;

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

	odt = odt < 0 ? 0 : odt;
	odt = odt > 0x3f ? 0x3f : odt;

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
	unsigned char vcm_boost_en, phase_sel;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL14);
	if (phy->cv_mode == PHY_VMODE) {
		phy->vswing = 0;
		phy->vcm = 0;
		phy->odt = lcd_phy_get_custom_odt_vm(phy);
	} else {
		phy->vswing = (data32 >> 26) & 0xf;
		phy->vcm = (data32 >> 12) & 0xf;
		phy->odt = lcd_phy_get_custom_odt_cm(phy);
	}
	phy->ref_bias = 0;

	data32 = lcd_ana_read(ANACTRL_DIF_PHY_CNTL15);
	phy->cv_mode = (data32 >> 20) & 0x1;
	vcm_boost_en = (data32 >> 22) & 0x1;

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

		phase_sel = (chdig >> 0) & 0x3;
		phy_cfg->ch_ctrl[i].phase_sel =
			lcd_phy_get_phase(phy_cfg, PHY_GET_PHASE_SEL_BY_REG, phase_sel);
	}

	return 0;
}

static void lcd_phy_common_update(struct aml_lcd_drv_s *pdrv, unsigned int cntl14)
{
	unsigned int cntl15 = 0x30090000;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

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
	if (phy->cv_mode == PHY_VMODE)
		cntl15 |= (1 << 23) | (1 << 20);

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);
}

static void lcd_phy_cntl_set(struct aml_lcd_drv_s *pdrv, int status)
{
	int sel = -1;
	unsigned int chreg, reg_data = 0, chdig = 0;
	unsigned char i, bit;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d, ckdi:0x%x\n", __func__, status, phy_cfg->ckdi);

	if (status) {
		reg_data = 1 << 0;
	} else {
		reg_data = 0;
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, 0x1300d100);
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, 0x30800000);
	}

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if ((phy_cfg->lane_valid & (1 << i)) == 0)
			continue;
		bit = i & 0x1 ? 16 : 0;
		chreg = reg_data;

		if (status) {
			if (phy->cv_mode == PHY_VMODE) {
				chreg |= (phy->lane[i].preem & 0xf) << 12;
				chreg |= (phy->lane[i].amp & 0xf) << 8;
			} else {
				chreg |= (phy->lane[i].preem & 0x7) << 12;
				chreg |= (phy->lane[i].rterm & 0x7) << 8;
				chreg |= (phy->lane[i].amp & 0x7) << 1;
			}

			chdig |= 1 << 15;
			switch (pdrv->config.basic.lcd_type) {
			case LCD_LVDS:
				chdig |= 1 << 10;   //clk inv
				break;
			case LCD_MLVDS:
				chdig |= 0x3 << 4;  //pn swap

				// check lane phase select
				sel = lcd_phy_get_phase(phy_cfg, PHY_GET_PHASE_REG_BY_SEL,
							phy_cfg->ch_ctrl[i].phase_sel);
				if (sel < 0) {
					LCDERR("[%d]: err lane[%d].phase_sel=%#x\n",
					       pdrv->index, i, phy_cfg->ch_ctrl[i].phase_sel);
				} else {
					chdig |= (sel & 0x1) << 0;
				}
				break;
			default:
				break;
			}
		}
		lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
		lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
	}
}

static void lcd_phy_init_off(void)
{
	int i;

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, 0x1300d100);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, 0x30800000);

	for (i = 0; i < 6; i++) {
		lcd_ana_write(chreg_reg[i], 0);
		lcd_ana_write(chdig_reg[i], 0);
	}

	LCDPR("%s done\n", __func__);
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x1310d107;

	if (status) {
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_cntl_set(pdrv, status);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, status);
	}
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x1310d107;

	if (status) {
		cntl14 |= (3 << 16); //div10
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_cntl_set(pdrv, status);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, status);
	}
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int cntl14 = 0x1310d107;

	if (status) {
		if (pdrv->config.timing.act_timing.lcd_bits == 18)
			cntl14 |= (1 << 16); //div6
		else if (pdrv->config.timing.act_timing.lcd_bits == 24)
			cntl14 |= (2 << 16); //div8
		else
			cntl14 |= (3 << 16); //div10
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_cntl_set(pdrv, status);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, status);
	}
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct p2p_config_s *p2p_conf;
	unsigned int p2p_type;
	unsigned int cntl14 = 0x1310d107;

	p2p_conf = &pdrv->config.control.p2p_cfg;
	if (status) {
		p2p_type = p2p_conf->p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
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
			pdrv->config.phy_cfg.low_common_mode = 1;
			cntl14 |= (2 << 16); //div8
			break;
		default:
			LCDERR("%s: invalid p2p_type 0x%x\n", __func__, p2p_type);
			return;
		}
		lcd_phy_common_update(pdrv, cntl14);
		lcd_phy_cntl_set(pdrv, status);
		udelay(1);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
	} else {
		lcd_phy_cntl_set(pdrv, status);
	}
}

static unsigned int lcd_phy_vswing_level_to_val_t6w(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (phy->cv_mode == PHY_VMODE)
		return 0;

	//default cmode
	if (level < 0xf)
		return level;
	LCDERR("[%d]: %s: level %d invalid\n", pdrv->index, __func__, level);
	return 0;
}

static unsigned int lcd_phy_preem_level_to_val_t6w(struct aml_lcd_drv_s *pdrv, unsigned int level)
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

static unsigned int lcd_phy_amp_dft_t6w(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.basic.lcd_type == LCD_VBYONE)
		return 0x4;

	return 0x0;
}

static unsigned int lcd_phy_rterm_dft_t6w(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;

	if (phy->cv_mode == PHY_VMODE)
		return 0x4;

	return 0x0;
}

static void phy_glb_param_dft_t6w(struct aml_lcd_drv_s *pdrv)
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
			phy->vswing = 0;
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
		PHY_PHASE_0,  //lane_10  invalid
		PHY_PHASE_0,  //lane_11  invalid
	};

	if (pdrv->config.basic.lcd_type != LCD_MLVDS || lane >= ARRAY_SIZE(phase_sel_tbl))
		return 0xff;

	return phase_sel_tbl[lane];
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t6w = {
	.lane_num = 12,

	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_val_t6w,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_val_t6w,
	.phy_amp_dft_val = lcd_phy_amp_dft_t6w,
	.phy_rterm_dft_val = lcd_phy_rterm_dft_t6w,
	.phy_glb_param_dft_val = phy_glb_param_dft_t6w,
	.phy_lane_phase_sel_def = lcd_phy_lane_phase_sel_def,
	.phy_param_get = lcd_phy_param_get_from_reg,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t6w(struct aml_lcd_data_s *pdata)
{
	cali_odt_cm = lcd_phy_get_dft_odt_cm();
	cali_odt_vm = lcd_phy_get_dft_odt_vm();
	cali_bias = lcd_phy_get_dft_bias();

	lcd_phy_init_off();

	return &lcd_phy_ctrl_t6w;
}
#endif
