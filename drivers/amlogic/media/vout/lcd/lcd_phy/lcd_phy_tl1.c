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

static unsigned int lvds_vx1_p2p_phy_preem_tl1[7] = {0x06, 0x26, 0x46, 0x66, 0x86, 0xa6, 0xf6};
static unsigned int p2p_low_common_phy_preem_tl1[6] = {0x07, 0x17, 0x37, 0x77, 0xf7, 0xff};

static void lcd_phy_cntl_set(int status, unsigned int flag, unsigned int preem,
			unsigned int mode, unsigned int pull_up, int bypass, unsigned int ckdi)
{
	unsigned int cntl16 = 0, chreg, chdig;
	unsigned char i, bit;

	uint32_t chreg_reg[6] = {
		HHI_DIF_CSI_PHY_CNTL1, HHI_DIF_CSI_PHY_CNTL2,
		HHI_DIF_CSI_PHY_CNTL3, HHI_DIF_CSI_PHY_CNTL4,
		HHI_DIF_CSI_PHY_CNTL6, HHI_DIF_CSI_PHY_CNTL7,
	};
	uint32_t chdig_reg[6] = {
		HHI_DIF_CSI_PHY_CNTL8, HHI_DIF_CSI_PHY_CNTL9,
		HHI_DIF_CSI_PHY_CNTL10, HHI_DIF_CSI_PHY_CNTL11,
		HHI_DIF_CSI_PHY_CNTL12, HHI_DIF_CSI_PHY_CNTL13,
	};

	if (!phy_ctrl_p)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d, bypass:%d\n", __func__, status, bypass);

	if (status) {
		chreg = phy_ctrl_p->ctrl_bit_on << 0;
		cntl16 = (ckdi << 12) | 0x80000000;

		if (mode) { //lvds_vx1_p2p_phy_preem_tl1
			if (preem >= 7) {
				LCDERR("%s: invalid preem=0x%x, use default\n", __func__, preem);
				preem = 1;
			}
			chreg |= lvds_vx1_p2p_phy_preem_tl1[preem];
		} else { //p2p_low_common_phy_preem_tl1
			if (preem >= 6) {
				LCDERR("%s: invalid preem=0x%x, use default\n", __func__, preem);
				preem = 1;
			}
			chreg |= p2p_low_common_phy_preem_tl1[preem];
		}

		if (pull_up)
			chreg &= ~(1 << 3);
	} else {
		chreg = (!phy_ctrl_p->ctrl_bit_on) << 0;
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0);
	}

	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL15, 0);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL16, cntl16);

	for (i = 0; i < 12; i++) {
		if (flag & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
			chdig = (ckdi & (0x1 << i)) && bypass ? 0 : 0x4;
			lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
		}
	}
}

void lcd_phy_tcon_chpi_bbc_init_tl1(struct lcd_config_s *pconf)
{
	unsigned int data32 = 0x06020602;
	unsigned int preem;
	unsigned int n = 10;
	struct p2p_config_s *p2p_conf;

	if (!phy_ctrl_p)
		return;

	n = env_get_ulong("tcon_delay", 10, 10);
	p2p_conf = &pconf->control.p2p_cfg;

	/*get tcon tx pre_emphasis*/
	preem = p2p_conf->phy_preem & 0xf;

	/*check tx pre_emphasis ok or no*/
	if (preem >= 6) {
		LCDERR("%s: invalid preem=0x%x, use default\n", __func__, preem);
		preem = 0x1;
	}

	udelay(n);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL1, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL1, 1, 19, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL2, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL2, 1, 19, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL3, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL3, 1, 19, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL4, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL4, 1, 19, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL6, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL6, 1, 19, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL7, 1, 3, 1);
	lcd_ana_setb(HHI_DIF_CSI_PHY_CNTL7, 1, 19, 1);
	LCDPR("%s: delay: %dus\n", __func__, n);

	/*follow pre-emphasis*/
	data32 = p2p_low_common_phy_preem_tl1[preem];

	if (phy_ctrl_p->ctrl_bit_on)
		data32 &= ~((1 << 16) | (1 << 0));
	else
		data32 |= ((1 << 16) | (1 << 0));

	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL1, data32);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL2, data32);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL3, data32);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL4, data32);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL6, data32);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL7, data32);
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int vswing, preem = 0, flag;
	// {Port A: 6bit, 8bit, 10bit}, {Port B: 6bit, 8bit, 10bit}
	unsigned short lvds_flag_6lane_map0[2][3] = {
		{0x000f, 0x001f, 0x003f}, {0x03c0, 0x07c0, 0x0fc0}};
	unsigned char bit_idx;

	if (pdrv->config.basic.lcd_bits == 6)
		bit_idx = 0;
	else if (pdrv->config.basic.lcd_bits == 8)
		bit_idx = 1;
	else //pdrv->config.basic.lcd_bits == 10
		bit_idx = 2;

	if (pdrv->config.control.lvds_cfg.dual_port) {
		flag = lvds_flag_6lane_map0[0][bit_idx] | lvds_flag_6lane_map0[1][bit_idx];
	} else {
		if (pdrv->config.control.lvds_cfg.port_swap)
			flag = lvds_flag_6lane_map0[1][bit_idx];
		else
			flag = lvds_flag_6lane_map0[0][bit_idx];
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d, flag=0x%04x\n", pdrv->index, __func__, status, flag);

	if (status) {
		vswing = pdrv->config.control.lvds_cfg.phy_vswing & 0xf;
		preem = pdrv->config.control.lvds_cfg.phy_preem & 0xf;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing=0x%x, preem=0x%x\n", vswing, preem);

		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xff2027e0 | vswing);
	}
	lcd_phy_cntl_set(status, flag, preem, 0, 0, 1, 0);
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int vswing, preem = 0;
	struct vbyone_config_s *vbyone_conf;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	vbyone_conf = &pdrv->config.control.vbyone_cfg;
	if (status) {
		vswing = vbyone_conf->phy_vswing & 0xf;
		preem = vbyone_conf->phy_preem & 0xf;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("vswing=0x%x, preem=0x%x\n", vswing, preem);
		}

		if (vbyone_conf->phy_vswing & 0x30)
			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xff2027e0 | vswing);
		else
			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xf02027a0 | vswing);
	}

	lcd_phy_cntl_set(status, 0xff, preem, 0, 0, 1, 0);
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int vswing, preem = 0, flag = 0;
	struct mlvds_config_s *mlvds_conf;
	unsigned char i, bypass = 1;
	unsigned long long channel_sel;


	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	mlvds_conf = &pdrv->config.control.mlvds_cfg;
	channel_sel = mlvds_conf->channel_sel1;
	channel_sel = channel_sel << 32 | mlvds_conf->channel_sel0;
	for (i = 0; i < 12; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		vswing = mlvds_conf->phy_vswing & 0xf;
		preem = mlvds_conf->phy_preem & 0xf;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing=0x%x, preem=0x%x\n", vswing, preem);

		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xff2027e0 | vswing);
		bypass = (mlvds_conf->clk_phase >> 12) & 0x1;
	}
	lcd_phy_cntl_set(status, flag, preem, 0, 0, bypass, mlvds_conf->pi_clk_sel);
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int vswing, preem = 0, p2p_type;
	struct p2p_config_s *p2p_conf;
	unsigned int flag = 0;
	unsigned char i, mode = 0, pull_up = 0;
	unsigned long long channel_sel;


	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	p2p_conf = &pdrv->config.control.p2p_cfg;
	channel_sel = p2p_conf->channel_sel1;
	channel_sel = channel_sel << 32 | p2p_conf->channel_sel0;
	for (i = 0; i < 12; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		vswing = p2p_conf->phy_vswing & 0xf;
		preem = p2p_conf->phy_preem & 0xf;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing=0x%x, preem=0x%x\n", vswing, preem);

		p2p_type = p2p_conf->p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			mode = 0;
			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xff2027a0 | vswing);
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			mode = 1;
			pull_up = p2p_type == P2P_CHPI;

			lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0xfe60027f);
			break;
		default:
			LCDERR("%s: invalid p2p_type %d\n", __func__, p2p_type);
			return;
		}
	}
	lcd_phy_cntl_set(status, flag, preem, mode, pull_up, 1, 0);
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_tl1 = {
	.lane_lock = 0,
	.ctrl_bit_on = 1,
	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_amp_dft_val = NULL,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_value_legacy,
	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_tl1(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_tl1;
	if (pdata->chip_type == LCD_CHIP_TL1 &&
		(pdata->rev_type == 0xA || pdata->rev_type == 0xB)) {
		phy_ctrl_p->ctrl_bit_on = 0;
	} else if (pdata->chip_type == LCD_CHIP_TM2 && pdata->rev_type == 0xA) {
		phy_ctrl_p->ctrl_bit_on = 0;
	}

	return phy_ctrl_p;
}
