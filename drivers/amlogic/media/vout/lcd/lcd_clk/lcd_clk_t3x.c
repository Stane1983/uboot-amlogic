// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_clk_config.h"
#include "lcd_clk_ctrl.h"
#include "lcd_clk_utils.h"

#ifdef CONFIG_MESON_T3X
static void lcd_pll_ss_enable(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, offset;
	unsigned int flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	offset = cconf->pll_offset;
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2 + offset);
	pll_ctrl2 &= ~((1 << 15) | (0xf << 16) | (0xf << 28));

	if (status) {
		if (cconf->ss_level > 0)
			flag = 1;
		else
			flag = 0;
	} else {
		flag = 0;
	}

	if (flag) {
		cconf->ss_en = 1;
		pll_ctrl2 |= ((1 << 15) | (cconf->ss_dep_sel << 28) | (cconf->ss_str_m << 16));
		LCDPR("[%d]: pll ss enable: level %d, %dppm\n",
			pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, pll_ctrl2);
}

static void lcd_set_pll_ss_level(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, offset;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	offset = cconf->pll_offset;
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2 + offset);
	pll_ctrl2 &= ~((1 << 15) | (0xf << 16) | (0xf << 28));

	if (cconf->ss_level > 0) {
		ret = lcd_pll_ss_level_generate(cconf);
		if (ret == 0) {
			cconf->ss_en = 1;
			pll_ctrl2 |= ((1 << 15) |
				      (cconf->ss_dep_sel << 28) |
				      (cconf->ss_str_m << 16));
			LCDPR("[%d]: set pll spread spectrum: level %d, %dppm\n",
				pdrv->index, cconf->ss_level, cconf->ss_ppm);
		}
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: set pll spread spectrum: disable\n", pdrv->index);
	}

	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, pll_ctrl2);
}

static void lcd_set_pll_ss_advance(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, offset;
	unsigned int freq, mode;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	freq = cconf->ss_freq;
	mode = cconf->ss_mode;
	offset = cconf->pll_offset;
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2 + offset);
	pll_ctrl2 &= ~(0x7 << 24); /* ss_freq */
	pll_ctrl2 |= (freq << 24);
	pll_ctrl2 &= ~(0x3 << 22); /* ss_mode */
	pll_ctrl2 |= (mode << 22);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, pll_ctrl2);

	LCDPR("[%d]: set pll spread spectrum: freq=%d, mode=%d\n",
	      pdrv->index, freq, mode);
}

static void lcd_pll_frac_set(struct aml_lcd_drv_s *pdrv, unsigned int frac)
{
	struct lcd_clk_config_s *cconf, *phyconf, *pixconf;
	unsigned int offset[2], val[2];

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
		phyconf = &cconf[0];
		pixconf = &cconf[1];
		offset[0] = phyconf->pll_offset;
		offset[1] = pixconf->pll_offset;
		val[0] = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[0]);
		val[1] = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[1]);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL1 + offset[0], phyconf->pll_frac, 0, 17);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL1 + offset[1], pixconf->pll_frac, 0, 17);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: phyconf reg 0x%x: 0x%08x->0x%08x\n",
				pdrv->index, __func__, ANACTRL_TCON_PLL0_CNTL1 + offset[0],
				val[0], lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[0]));
			LCDPR("[%d]: %s: pixconf reg 0x%x: 0x%08x->0x%08x\n",
				pdrv->index, __func__, ANACTRL_TCON_PLL0_CNTL1 + offset[1],
				val[1], lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[1]));
		}
		LCDPR("[%d]: %s: phy pll_frac=0x%x, pix pll_frac=0x%x\n",
			pdrv->index, __func__, phyconf->pll_frac, pixconf->pll_frac);
	} else {
		offset[0] = cconf->pll_offset;
		val[0] = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[0]);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL1 + offset[0], frac, 0, 17);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: reg 0x%x: 0x%08x->0x%08x\n",
				pdrv->index, __func__, ANACTRL_TCON_PLL0_CNTL1 + offset[0],
				val[0], lcd_ana_read(ANACTRL_TCON_PLL0_CNTL1 + offset[0]));
		}
		LCDPR("[%d]: %s: pll_frac=0x%x\n", pdrv->index, __func__, frac);
	}
}

static int _lcd_set_pll_by_cconf(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	unsigned int pll_ctrl, pll_ctrl1, pll_stts;
	unsigned int reg_ctrl0, reg_ctrl1, reg_ctrl2, reg_ctrl3, reg_ctrl4;
	int ret, cnt = 0, done = 0;

	pll_ctrl = ((0x3 << 17) | /* gate ctrl */
		(cconf->pll_n << LCD_PLL_N_TL1) |
		(cconf->pll_m << LCD_PLL_M_TL1) |
		(cconf->pll_od3_sel << LCD_PLL_OD3_T7) |
		(cconf->pll_od2_sel << LCD_PLL_OD2_T7) |
		(cconf->pll_od1_sel << LCD_PLL_OD1_T7));
	pll_ctrl1 = (1 << 28) |
		((1 << 20) | /* sdm_en */
		(cconf->pll_frac << 0));

	switch (cconf->pll_id) {
	case 1:
		pll_stts = ANACTRL_TCON_PLL1_STS;
		reg_ctrl0 = ANACTRL_TCON_PLL1_CNTL0;
		reg_ctrl1 = ANACTRL_TCON_PLL1_CNTL1;
		reg_ctrl2 = ANACTRL_TCON_PLL1_CNTL2;
		reg_ctrl3 = ANACTRL_TCON_PLL1_CNTL3;
		reg_ctrl4 = ANACTRL_TCON_PLL1_CNTL4;
		break;
	case 0:
	default:
		pll_stts = ANACTRL_TCON_PLL0_STS;
		reg_ctrl0 = ANACTRL_TCON_PLL0_CNTL0;
		reg_ctrl1 = ANACTRL_TCON_PLL0_CNTL1;
		reg_ctrl2 = ANACTRL_TCON_PLL0_CNTL2;
		reg_ctrl3 = ANACTRL_TCON_PLL0_CNTL3;
		reg_ctrl4 = ANACTRL_TCON_PLL0_CNTL4;
		break;
	}

set_pll_retry:
	lcd_ana_write(reg_ctrl0, pll_ctrl);
	udelay(10);
	lcd_ana_setb(reg_ctrl0, 1, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_setb(reg_ctrl0, 1, LCD_PLL_EN_TL1, 1);
	udelay(10);
	lcd_ana_write(reg_ctrl1, pll_ctrl1);
	udelay(10);
	lcd_ana_write(reg_ctrl2, 0x0000110c);
	udelay(10);
	lcd_ana_write(reg_ctrl3, 0x10051100);
	udelay(10);
	lcd_ana_setb(reg_ctrl4, 0x0100c0, 0, 24);
	udelay(10);
	lcd_ana_setb(reg_ctrl4, 0x8300c0, 0, 24);
	udelay(10);
	lcd_ana_setb(reg_ctrl0, 1, 26, 1);
	udelay(10);
	lcd_ana_setb(reg_ctrl0, 0, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_write(reg_ctrl2, 0x0000300c);

	ret = lcd_pll_wait_lock(pll_stts, LCD_PLL_LOCK_T7);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry;
		LCDERR("[%d]: pll lock failed\n", pdrv->index);
	} else {
		udelay(100);
		lcd_ana_setb(reg_ctrl2, 1, 5, 1);
		done = 1;
	}

	return done;
}

static void lcd_set_pll_t3x(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	_lcd_set_pll_by_cconf(pdrv, &cconf[0]);
	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE)
		_lcd_set_pll_by_cconf(pdrv, &cconf[1]);

	if (cconf[0].ss_level > 0) {
		lcd_set_pll_ss_level(pdrv);
		lcd_set_pll_ss_advance(pdrv);
	}
}

static void lcd_set_phy_dig_div_t3x(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_dphy_tx_ctrl1;
	unsigned int reg_dphy_tx_ctrl0;
	unsigned int bit_rst;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	switch (cconf->pll_id) {
	case 1:
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		bit_rst = 20;
		break;
	case 0:
	default:
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		bit_rst = 19;
		break;
	}

	lcd_reset_setb(RESETCTRL_RESET1_MASK, 0, bit_rst, 1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit_rst, 1);
	udelay(1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit_rst, 1);
	udelay(10);

	lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, 5, 1);// disp1 clk form tcon_pll1

	// Enable dphy clock
	lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 0, 1);

	if (pdrv->config.basic.lcd_type == LCD_P2P) {
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 2, 5, 2);

		/* set cntl_ser_en */
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0xfff, 16, 12);

		/* decoupling fifo enable */
		lcd_combo_dphy_write(reg_dphy_tx_ctrl1, (1 << 6));

		/* decoupling fifo write enable after fifo enable */
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 1);
	}

	// sel pll clock
	lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 4, 1);

	// sel tcon_pll clock
	lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 5, 1);
}

static void _lcd_set_vid_pll_div_by_cconf(struct aml_lcd_drv_s *pdrv,
		struct lcd_clk_config_s *cconf)
{
	unsigned int reg_vid_pll_div, shift_val, shift_sel;
	int i;

	switch (cconf->pll_id) {
	case 1:
		reg_vid_pll_div = COMBO_DPHY_VID_PLL1_DIV;
		break;
	case 0:
	default:
		reg_vid_pll_div = COMBO_DPHY_VID_PLL0_DIV;
		break;
	}

	/* Disable the div output clock */
	lcd_combo_dphy_setb(reg_vid_pll_div, 0, 19, 1);
	lcd_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);

	i = 0;
	while (lcd_clk_div_table[i].divider < cconf->data->div_sel_max) {
		if (cconf->div_sel == lcd_clk_div_table[i].divider)
			break;
		i++;
	}
	if (lcd_clk_div_table[i].divider == cconf->data->div_sel_max)
		LCDERR("[%d]: invalid clk divider\n", pdrv->index);
	shift_val = lcd_clk_div_table[i].shift_val;
	shift_sel = lcd_clk_div_table[i].shift_sel;

	if (shift_val == 0xffff) { /* if divide by 1 */
		lcd_combo_dphy_setb(reg_vid_pll_div, 1, 18, 1);
	} else {
		lcd_combo_dphy_setb(reg_vid_pll_div, 0, 18, 1);
		lcd_combo_dphy_setb(reg_vid_pll_div, 0, 16, 2);
		lcd_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);
		lcd_combo_dphy_setb(reg_vid_pll_div, 0, 0, 14);

		lcd_combo_dphy_setb(reg_vid_pll_div, shift_sel, 16, 2);
		lcd_combo_dphy_setb(reg_vid_pll_div, 1, 15, 1);
		lcd_combo_dphy_setb(reg_vid_pll_div, shift_val, 0, 15);
		lcd_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);
	}

	/* Enable the final output clock */
	lcd_combo_dphy_setb(reg_vid_pll_div, 1, 19, 1);
}

static void lcd_set_vid_pll_div_t3x(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_vid2_clk_ctrl;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	switch (pdrv->index) {
	case 1:
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK1_CTRL;
		break;
	case 0:
	default:
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
		break;
	}
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_EN, 1);
	udelay(5);

	if (cconf->prbs_mode) {
		LCDPR("[%d]: %s prbs mode\n", pdrv->index, __func__);
		_lcd_set_vid_pll_div_by_cconf(pdrv, cconf);
		return;
	}
	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
		_lcd_set_vid_pll_div_by_cconf(pdrv, &cconf[0]);
		_lcd_set_vid_pll_div_by_cconf(pdrv, &cconf[1]);
	} else {
		_lcd_set_vid_pll_div_by_cconf(pdrv, cconf);
	}
}

static void lcd_clk_set_t3x(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll_t3x(pdrv);
	lcd_set_phy_dig_div_t3x(pdrv);
	lcd_set_vid_pll_div_t3x(pdrv);
}

static void lcd_set_vclk_crt(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_vid2_clk_div, reg_vid2_clk_ctrl, reg_vid_clk_ctrl2;
	unsigned int venc_clk_sel_bit = 0xff;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	switch (pdrv->index) {
	case 1:
		reg_vid2_clk_div = CLKCTRL_VIID_CLK1_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK1_CTRL;
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK1_CTRL2;
		break;
	case 0:
	default:
		reg_vid2_clk_div = CLKCTRL_VIID_CLK0_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK0_CTRL2;
		venc_clk_sel_bit = 24;
		break;
	}

	lcd_clk_write(reg_vid_clk_ctrl2, 0);
	lcd_clk_write(reg_vid2_clk_ctrl, 0);
	lcd_clk_write(reg_vid2_clk_div, 0);
	udelay(5);

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: vclk_sel=%d, xd=%d\n", pdrv->index, cconf->data->vclk_sel, cconf->xd);

	if (venc_clk_sel_bit < 0xff)
		lcd_clk_setb(CLKCTRL_HDMI_VID_PLL_CLK_DIV, 0, venc_clk_sel_bit, 1);

#ifdef CONFIG_AML_LCD_PXP
	/* setup the XD divider value */
	lcd_clk_setb(reg_vid2_clk_div, cconf->xd, VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(reg_vid2_clk_ctrl, 7, 16, 3);
#else
	/* setup the XD divider value */
	lcd_clk_setb(reg_vid2_clk_div, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);

	/* select vid_pll_clk */
	lcd_clk_setb(reg_vid2_clk_ctrl, cconf->data->vclk_sel, 16, 3);
#endif
	lcd_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(reg_vid2_clk_div, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(reg_vid2_clk_div, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_SOFT_RST, 1);
	udelay(10);
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(reg_vid_clk_ctrl2, 1, ENCL_GATE_VCLK, 1);
}

#ifdef CONFIG_AML_LCD_TCON
/* tcon run base clk, include register access */
static void lcd_set_tcon_clk_t3x(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;

	if (pconf->basic.lcd_type != LCD_P2P)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	/* tcon_clk 50M */
	lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (7 << 0));

	/* global reset tcon, take effect when pixel_clk ready */
	lcd_tcon_global_reset(pdrv);
}
#endif

//special setting by lcd interface
static void lcd_clktree_set(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_TCON
	if (pdrv->index == 0) /* tcon_clk invalid for lcd1 */
		lcd_set_tcon_clk_t3x(pdrv);
#endif
}

static void lcd_clk_disable(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf, *phyconf, *pixconf;
	unsigned int reg_vid_clk_ctrl2, reg_vid2_clk_ctrl, offset[2];

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	switch (pdrv->index) {
	case 1:
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK1_CTRL2;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK1_CTRL;
		break;
	case 0:
	default:
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK0_CTRL2;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
		break;
	}

	lcd_clk_setb(reg_vid_clk_ctrl2, 0, ENCL_GATE_VCLK, 1);

	/* close vclk2_div gate: [4:0] */
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, 0, 5);
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_EN, 1);

	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
		phyconf = &cconf[0];
		pixconf = &cconf[1];
		offset[0] = phyconf->pll_offset;
		offset[1] = pixconf->pll_offset;
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[0], 0, 28, 1); //pll0 disable
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[0], 0, 29, 1); //pll0 reset
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[1], 0, 28, 1); //pll1 disable
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[1], 0, 29, 1); //pll1 reset
	} else {
		offset[0] = cconf->pll_offset;
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[0], 0, 28, 1); //disable
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset[0], 0, 29, 1); //reset
	}
}

static void lcd_clk_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	int i;
	unsigned int *table_pll = NULL, *table_clk = NULL, *table_combo_dphy = NULL;
	unsigned int size_pll = 0, size_clk = 0, size_combo_dphy = 0;
	unsigned int pll_reg_table[] = {
		ANACTRL_TCON_PLL0_CNTL0,
		ANACTRL_TCON_PLL0_CNTL1,
		ANACTRL_TCON_PLL0_CNTL2,
		ANACTRL_TCON_PLL0_CNTL3,
		ANACTRL_TCON_PLL0_CNTL4,
		ANACTRL_TCON_PLL0_STS,
		ANACTRL_TCON_PLL1_CNTL0,
		ANACTRL_TCON_PLL1_CNTL1,
		ANACTRL_TCON_PLL1_CNTL2,
		ANACTRL_TCON_PLL1_CNTL3,
		ANACTRL_TCON_PLL1_CNTL4,
		ANACTRL_TCON_PLL1_STS
	};
	unsigned int clk_reg_table[][3] = {
		{
			CLKCTRL_VIID_CLK0_DIV,
			CLKCTRL_VIID_CLK0_CTRL,
			CLKCTRL_VID_CLK0_CTRL2,
		},
		{
			CLKCTRL_VIID_CLK1_DIV,
			CLKCTRL_VIID_CLK1_CTRL,
			CLKCTRL_VID_CLK1_CTRL2
		}
	};
	unsigned int combo_dphy_reg_table[] = {
		COMBO_DPHY_VID_PLL0_DIV,
		COMBO_DPHY_VID_PLL1_DIV
	};

	if (!pdrv || pdrv->index > 1)
		return;

	if (pdrv->index == 1) {
		table_pll = &pll_reg_table[6];
		size_pll = 6;
		table_combo_dphy = &combo_dphy_reg_table[1];
		size_combo_dphy = 1;
	} else {
		table_pll = &pll_reg_table[0];
		table_combo_dphy = &combo_dphy_reg_table[0];
		if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
			size_pll = 12;
			size_combo_dphy = 2;
		} else {
			size_pll = 6;
			size_combo_dphy = 1;
		}
	}
	table_clk = clk_reg_table[pdrv->index];
	size_clk = ARRAY_SIZE(clk_reg_table[pdrv->index]);

	for (i = 0; i < size_pll; i++)
		printf("pll [0x%08x] = 0x%08x\n", table_pll[i], lcd_ana_read(table_pll[i]));

	for (i = 0; i < size_clk; i++)
		printf("clk [0x%08x] = 0x%08x\n", table_clk[i], lcd_clk_read(table_clk[i]));

	for (i = 0; i < size_combo_dphy; i++) {
		printf("combo_dphy [0x%08x] = 0x%08x\n",
		       table_combo_dphy[i], lcd_combo_dphy_read(table_combo_dphy[i]));
	}

	if (pdrv->index == 0) {
		printf("clk [0x%08x] = 0x%08x\n",
		       CLKCTRL_TCON_CLK_CNTL, lcd_clk_read(CLKCTRL_TCON_CLK_CNTL));
	}
}

static void lcd_prbs_config_clk(struct aml_lcd_drv_s *pdrv, unsigned int lcd_prbs_mode,
				unsigned int *encl_clk, unsigned int *fifo_clk)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned long long bit_rate = 0;

	if (!cconf)
		return;

	if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
		bit_rate = 2970000000ULL;
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
		bit_rate = 550000000ULL;
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_FREQ) {
		bit_rate = lcd_prbs_freq * 1000000ULL;
	} else {
		LCDERR("[%d]: %s: unsupport lcd_prbs_mode %d\n",
		       pdrv->index, __func__, lcd_prbs_mode);
		return;
	}

	*encl_clk = lcd_do_div(bit_rate, 5);
	*fifo_clk = lcd_do_div(bit_rate, 10);
	lcd_clk_generate_prbs_clk(pdrv, *encl_clk, bit_rate);
	if (cconf->done == 0)
		return;

	cconf->prbs_mode = 1;
	lcd_clk_set_t3x(pdrv);
	lcd_set_vclk_crt(pdrv);
	cconf->prbs_mode = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s ok\n", pdrv->index, __func__);
}

static int lcd_prbs_test_process(struct aml_lcd_drv_s *pdrv, unsigned int timeout,
				 unsigned int encl_clk_check_std, unsigned int fifo_clk_check_std)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned int reg_phy_tx_ctrl0, reg_phy_tx_ctrl1, reg_ctrl_out;
	int encl_msr_id, fifo_msr_id;
	unsigned int lcd_prbs_cnt, clk_err_cnt;
	unsigned int val1, val2;
	int i, ret = 0;

	if (!cconf)
		return 0;

	encl_msr_id = cconf->data->enc_clk_msr_id;
	fifo_msr_id = cconf->data->fifo_clk_msr_id;

	switch (pdrv->index) {
	case 1:
		reg_phy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
		reg_phy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		reg_ctrl_out = COMBO_DPHY_RO_EDP_LVDS_TX_PHY1_CNTL1;
		break;
	default:
		reg_phy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_phy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		reg_ctrl_out = COMBO_DPHY_RO_EDP_LVDS_TX_PHY0_CNTL1;
		break;
	}

	lcd_combo_dphy_write(reg_phy_tx_ctrl0, 0);
	lcd_combo_dphy_write(reg_phy_tx_ctrl1, 0);
	lcd_prbs_cnt = 0;
	clk_err_cnt = 0;

	/* set fifo_clk_sel: div 10 */
	lcd_combo_dphy_write(reg_phy_tx_ctrl0, (3 << 5));
	/* set cntl_ser_en:  16-channel */
	lcd_combo_dphy_setb(reg_phy_tx_ctrl0, 0xffff, 16, 16);
	lcd_combo_dphy_setb(reg_phy_tx_ctrl0, 1, 2, 1);
	/* decoupling fifo enable, gated clock enable */
	lcd_combo_dphy_write(reg_phy_tx_ctrl1, (1 << 6) | (1 << 0));
	/* decoupling fifo write enable after fifo enable */
	lcd_combo_dphy_setb(reg_phy_tx_ctrl1, 1, 7, 1);
	/* prbs_err en */
	lcd_combo_dphy_setb(reg_phy_tx_ctrl0, 1, 13, 1);
	lcd_combo_dphy_setb(reg_phy_tx_ctrl0, 1, 12, 1);

	while (lcd_prbs_cnt++ < timeout) {
		ret = -1;
		val1 = lcd_combo_dphy_getb(reg_ctrl_out, 16, 16);
		udelay(1000);

		for (i = 0; i < 20; i++) {
			udelay(5);
			val2 = lcd_combo_dphy_getb(reg_ctrl_out, 16, 16);
			if (val2 != val1) {
				ret = 0;
				break;
			}
		}
		if (ret) {
			ret = -1;
			LCDERR("[%d]: prbs check error 1(running state), val:0x%04x, cnt:%d\n",
			       pdrv->index, val2, lcd_prbs_cnt);
			break;
		}
		if (lcd_combo_dphy_getb(reg_ctrl_out, 0, 16)) {
			ret = -1;
			LCDERR("[%d]: prbs check error 2(prbs), cnt:%d\n",
			       pdrv->index, lcd_prbs_cnt);
			break;
		}

		if (lcd_prbs_clk_check(encl_clk_check_std, encl_msr_id,
				       fifo_clk_check_std, fifo_msr_id, lcd_prbs_cnt))
			clk_err_cnt++;
		else
			clk_err_cnt = 0;
		if (clk_err_cnt >= 10) {
			ret = -1;
			LCDERR("[%d]: prbs check error 3(clkmsr), cnt: %d\n",
			       pdrv->index, lcd_prbs_cnt);
			break;
		}
	}

	lcd_combo_dphy_write(reg_phy_tx_ctrl0, 0);
	lcd_combo_dphy_write(reg_phy_tx_ctrl1, 0);

	return ret;
}

static int lcd_clk_prbs_test(struct aml_lcd_drv_s *pdrv, unsigned int ms, unsigned int mode_flag)
{
	unsigned int lcd_prbs_mode, timeout;
	unsigned int lcd_encl_clk_check_std = 0, lcd_fifo_clk_check_std = 0;
	int i, ret;

	timeout = (ms > 1000) ? 1000 : ms;

	for (i = 0; i < LCD_PRBS_MODE_MAX; i++) {
		if ((mode_flag & (1 << i)) == 0)
			continue;

		lcd_prbs_mode = (1 << i);
		LCDPR("[%d]: lcd_prbs_mode: %d\n", pdrv->index, lcd_prbs_mode);
		lcd_prbs_config_clk(pdrv, lcd_prbs_mode, &lcd_encl_clk_check_std,
				    &lcd_fifo_clk_check_std);
		udelay(500);

		ret = lcd_prbs_test_process(pdrv, timeout, lcd_encl_clk_check_std,
					    lcd_fifo_clk_check_std);
		if (ret) {
			if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
				lcd_prbs_performed |= LCD_PRBS_MODE_LVDS;
				lcd_prbs_err |= LCD_PRBS_MODE_LVDS;
			} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
				lcd_prbs_performed |= LCD_PRBS_MODE_VX1;
				lcd_prbs_err |= LCD_PRBS_MODE_VX1;
			} else if (lcd_prbs_mode == LCD_PRBS_MODE_FREQ) {
				lcd_prbs_performed |= LCD_PRBS_MODE_FREQ;
				lcd_prbs_err |= LCD_PRBS_MODE_FREQ;
			}
		} else {
			if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
				lcd_prbs_performed |= LCD_PRBS_MODE_LVDS;
				lcd_prbs_err &= ~(LCD_PRBS_MODE_LVDS);
				LCDPR("[%d]: lvds prbs check ok\n", pdrv->index);
			} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
				lcd_prbs_performed |= LCD_PRBS_MODE_VX1;
				lcd_prbs_err &= ~(LCD_PRBS_MODE_VX1);
				LCDPR("[%d]: vx1 prbs check ok\n", pdrv->index);
			} else if (lcd_prbs_mode == LCD_PRBS_MODE_FREQ) {
				lcd_prbs_performed |= LCD_PRBS_MODE_FREQ;
				lcd_prbs_err &= ~(LCD_PRBS_MODE_FREQ);
				LCDPR("[%d]: freq %dMhz prbs check ok\n",
				      pdrv->index, lcd_prbs_freq);
			} else {
				LCDPR("[%d]: prbs check: unsupport mode\n", pdrv->index);
			}
		}
	}

	printf("\n[[%d]: lcd prbs result]:\n", pdrv->index);
	printf("  lvds performed: %d, error: %d\n"
	       "  vx1 performed: %d, error: %d\n",
	       (lcd_prbs_performed & LCD_PRBS_MODE_LVDS) ? 1 : 0,
	       (lcd_prbs_err & LCD_PRBS_MODE_LVDS) ? 1 : 0,
	       (lcd_prbs_performed & LCD_PRBS_MODE_VX1) ? 1 : 0,
	       (lcd_prbs_err & LCD_PRBS_MODE_VX1) ? 1 : 0);
	if (lcd_prbs_performed & LCD_PRBS_MODE_FREQ) {
		printf("  freq %dMHz performed: %d, error: %d\n",
		       lcd_prbs_freq,
		       (lcd_prbs_performed & LCD_PRBS_MODE_FREQ) ? 1 : 0,
		       (lcd_prbs_err & LCD_PRBS_MODE_FREQ) ? 1 : 0);
	}

	return 0;
}

static struct lcd_clk_data_s lcd_clk_data_t3x_0 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 4100000000,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 4100000000ULL,
	.div_out_fmax = 820000000,
	.xd_out_fmax = 820000000,
	.od_cnt = 3,
	.have_tcon_div = 0,
	.have_pll_div = 1,
	.phy_clk_location = 1,

	.vclk_sel = 0,
	.enc_clk_msr_id = 62,
	.fifo_clk_msr_id = -1,

	.div_sel_max = CLK_DIV_SEL_MAX,
	.xd_max = 256,
	.phy_div_max = 256,

	.ss_support = 1,
	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.pll_frac_set = lcd_pll_frac_set,
	.clk_set = lcd_clk_set_t3x,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clktree_set = lcd_clktree_set,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.clk_reg_print = lcd_clk_reg_dump,
	.prbs_test = lcd_clk_prbs_test,
};

static struct lcd_clk_data_s lcd_clk_data_t3x_1 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 6000000000ULL,
	.pll_vco_fmin = 3000000000ULL,
	.pll_out_fmax = 4100000000,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 4100000000ULL,
	.div_out_fmax = 820000000,
	.xd_out_fmax = 820000000,
	.od_cnt = 3,
	.have_tcon_div = 0,
	.have_pll_div = 1,
	.phy_clk_location = 1,

	.vclk_sel = 0,
	.enc_clk_msr_id = 60,
	.fifo_clk_msr_id = -1,

	.div_sel_max = CLK_DIV_SEL_MAX,
	.xd_max = 256,
	.phy_div_max = 256,

	.ss_support = 1,
	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss_level = lcd_set_pll_ss_level,
	.set_ss_advance = lcd_set_pll_ss_advance,
	.clk_ss_enable = lcd_pll_ss_enable,
	.pll_frac_set = lcd_pll_frac_set,
	.clk_set = lcd_clk_set_t3x,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clktree_set = lcd_clktree_set,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.clk_reg_print = lcd_clk_reg_dump,
	.prbs_test = lcd_clk_prbs_test,
};

void lcd_clk_config_chip_init_t3x(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	if (!pdrv || !cconf)
		return;

	switch (pdrv->index) {
	case 1:
		cconf->data = &lcd_clk_data_t3x_1;
		cconf->pll_offset = (0x5 << 2);
		cconf->pll_od_fb = cconf->data->pll_od_fb;
		cconf->pll_id = 1;
		break;
	case 0:
	default:
		cconf[0].data = &lcd_clk_data_t3x_0;
		cconf[0].pll_od_fb = cconf[0].data->pll_od_fb;
		if (pdrv->config.basic.lcd_type == LCD_P2P &&
		    pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
			// pll1 as pixel clock, mux to venc0_clk
			cconf[0].data->vclk_sel = 4;

			cconf[1].data = &lcd_clk_data_t3x_1;
			cconf[1].pll_offset = (0x5 << 2);
			cconf[1].pll_od_fb = cconf[1].data->pll_od_fb;
		}
		break;
	}
}
#endif
