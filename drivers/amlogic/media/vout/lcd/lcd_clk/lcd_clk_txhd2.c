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

#ifdef CONFIG_MESON_TXHD2
static unsigned int tcon_div[5][3] = {
	/* div_mux, div2/4_sel, div4_bypass */
	{1, 0, 1},  /* div1 */
	{0, 0, 1},  /* div2 */
	{0, 1, 1},  /* div4 */
	{0, 0, 0},  /* div8 */
	{0, 1, 0},  /* div16 */
};

static void lcd_pll_frac_set(struct aml_lcd_drv_s *pdrv, unsigned int frac)
{
	struct lcd_clk_config_s *cconf;
	unsigned int val;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	val = lcd_ana_read(HHI_TCON_PLL_CNTL1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL1, frac, 0, 17);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: reg 0x%x: 0x%08x->0x%08x\n",
		      __func__, HHI_TCON_PLL_CNTL1, val, lcd_ana_read(HHI_TCON_PLL_CNTL1));
	}
	LCDPR("[%d]: %s: pll_frac=0x%x\n", pdrv->index, __func__, frac);
}

static void lcd_pll_ss_enable_txhd2(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 12));

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
		pll_ctrl2 |= ((cconf->ss_dep_sel << 4) | (cconf->ss_str_m << 12));
		LCDPR("[%d]: pll ss enable: level %d, %dppm\n",
		      pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_level_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 12));

	if (cconf->ss_level > 0) {
		ret = lcd_pll_ss_level_generate(cconf);
		if (ret == 0) {
			cconf->ss_en = 1;
			pll_ctrl2 |= ((cconf->ss_dep_sel << 4) | (cconf->ss_str_m << 12));
			LCDPR("[%d]: set pll spread spectrum: level %d, %dppm\n",
			      pdrv->index, cconf->ss_level, cconf->ss_ppm);
		}
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: set pll spread spectrum: disable\n", pdrv->index);
	}
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);
}

static void lcd_set_pll_ss_advance_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	unsigned int freq, mode;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	freq = cconf->ss_freq;
	mode = cconf->ss_mode;
	pll_ctrl2 = lcd_ana_read(HHI_TCON_PLL_CNTL2);
	pll_ctrl2 &= ~(0x7 << 20); /* ss_freq */
	pll_ctrl2 |= (freq << 20);
	pll_ctrl2 &= ~(0x3 << 0); /* ss_mode */
	pll_ctrl2 |= (mode << 0);
	lcd_ana_write(HHI_TCON_PLL_CNTL2, pll_ctrl2);

	LCDPR("set pll spread spectrum: freq=%d, mode=%d\n", freq, mode);
}

static void lcd_set_pll_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl, pll_ctrl1, pll_ctrl5;
	unsigned int tcon_div_sel;
	int ret, cnt = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	tcon_div_sel = cconf->pll_tcon_div_sel;
	pll_ctrl =
		(cconf->pll_n << 10) |
		(cconf->pll_m << 0) |
		(cconf->pll_od1_sel << 16) |
		(cconf->pll_od2_sel << 18) |
		(cconf->pll_od3_sel << 20) |
		(tcon_div[tcon_div_sel][1] << 22) |
		(tcon_div[tcon_div_sel][2] << 24) |
		(1 << 25);

	pll_ctrl1 = cconf->pll_frac;
	pll_ctrl5 = 0x00150500 | (tcon_div[tcon_div_sel][0] << 0);

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
		LCDPR("pll_m=0x%x, pll_n=0x%x, frac=0x%x, od1=%d, od2=%d, od3=%d\n",
		      cconf->pll_m, cconf->pll_n, cconf->pll_frac,
		      cconf->pll_od1_sel, cconf->pll_od2_sel, cconf->pll_od3_sel);
	}

	lcd_ana_write(HHI_TCON_PLL_CNTL0, 1 << 29);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, pll_ctrl);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, pll_ctrl1);
set_pll_retry_txhd2:
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x01000000);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x00258000);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x05501000);
	lcd_ana_write(HHI_TCON_PLL_CNTL5, pll_ctrl5);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50450000);
	udelay(50);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 28, 1);
	udelay(50);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 25, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	udelay(50);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 15, 1);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50440000);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_STS, 31);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry_txhd2;
		LCDERR("hpll lock failed\n");
	}

	if (cconf->ss_level > 0) {
		lcd_set_pll_ss_level_txhd2(pdrv);
		lcd_set_pll_ss_advance_txhd2(pdrv);
	}
}

static void lcd_set_vid_pll_div_txhd2(struct lcd_clk_config_s *cconf)
{
	unsigned int shift_val, shift_sel;
	int i;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 19, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 19, 1);
	lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);

	i = 0;
	while (lcd_clk_div_table[i].divider < cconf->data->div_sel_max) {
		if (cconf->div_sel == lcd_clk_div_table[i].divider)
			break;
		i++;
	}
	if (lcd_clk_div_table[i].divider == cconf->data->div_sel_max)
		LCDERR("invalid clk divider\n");
	shift_val = lcd_clk_div_table[i].shift_val;
	shift_sel = lcd_clk_div_table[i].shift_sel;
	if (shift_val == 0xffff) { /* if divide by 1 */
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 18, 1);
	} else {
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 16, 2);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 0, 14);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, shift_sel, 16, 2);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 15, 1);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, shift_val, 0, 14);
		lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);
	}
	/* Enable the final output clock */
	lcd_ana_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 19, 1);
}

#ifdef CONFIG_AML_LCD_TCON
static void lcd_set_clk_phase_txhd2(unsigned int phase_value)
{
	// set clock phase value
	lcd_ana_setb(HHI_TCON_PLL_CNTL1, phase_value, 20, 12);

	// set clock phase load sequence
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 25, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 25, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 25, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 25, 1);
}

static void lcd_set_tcon_clk_txhd2(struct aml_lcd_drv_s *pdrv)
{
	unsigned int val = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	if (pdrv->config.basic.lcd_type != LCD_MLVDS)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		val = pconf->control.mlvds_cfg.clk_phase & 0xfff;
		lcd_set_clk_phase_txhd2(val);

		/* tcon_clk */
		if (pconf->timing.enc_clk >= 100000000) /* 25M */
			lcd_clk_write(HHI_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0xf << 0));
		else /* 12.5M */
			lcd_clk_write(HHI_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0x1f << 0));
		break;
	default:
		break;
	}

	/* global reset tcon, take effect when pixel_clk ready */
	lcd_tcon_global_reset(pdrv);
}
#endif

static void lcd_set_dsi_phy_clk(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);

	if (!cconf)
		return;
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, cconf->phy_div - 1, 0, 7);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, 0, 12, 3);
	lcd_clk_setb(HHI_MIPIDSI_PHY_CLK_CNTL, 1, 8, 1);
}

static void lcd_clk_set_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	lcd_set_pll_txhd2(pdrv);
	lcd_set_vid_pll_div_txhd2(cconf);

	if (pdrv->config.basic.lcd_type == LCD_MIPI) {
		// lcd_set_dsi_meas_clk(pdrv->index);
		lcd_set_dsi_phy_clk(pdrv);
	}
}

static void lcd_clktree_set_txhd2(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_TCON
	lcd_set_tcon_clk_txhd2(pdrv);
#endif
}

static void lcd_clk_disable_txhd2(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 0, 3, 1);

	/* close vclk2_div gate: 0x104b[4:0] */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 0, 5);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, 19, 1);

	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 29, 1);  //resetn
}

static void lcd_prbs_set_pll_lvds_txhd2(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	int cnt = 0, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	//3840 / 8 / 10 = 48M
	//3840 / 4 / 4.67 = 137M
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 1 << 29);
	lcd_ana_write(HHI_TCON_PLL_CNTL0, 0x031484a0);
	lcd_ana_write(HHI_TCON_PLL_CNTL1, 0x00008000);
lcd_prbs_retry_pll_lvds_txhd2:
	lcd_ana_write(HHI_TCON_PLL_CNTL2, 0x01000000);
	lcd_ana_write(HHI_TCON_PLL_CNTL3, 0x00258000);
	lcd_ana_write(HHI_TCON_PLL_CNTL4, 0x05501000);
	lcd_ana_write(HHI_TCON_PLL_CNTL5, 0x00150500);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50450000);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 28, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 29, 1);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 0, 24, 2);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 23, 1);
	udelay(10);
	lcd_ana_setb(HHI_TCON_PLL_CNTL0, 1, 15, 1);
	lcd_ana_write(HHI_TCON_PLL_CNTL6, 0x50440000);

	ret = lcd_pll_wait_lock(HHI_TCON_PLL_STS, 31);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto lcd_prbs_retry_pll_lvds_txhd2;
		LCDERR("[%d]: hpll lock failed\n", pdrv->index);
	}

	/* pll_div */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	udelay(10);

	/* Disable the div output clock */
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 19, 1);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);

	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 18, 1);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 16, 2);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 0, 14);

	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 16, 2);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 15, 1);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0x0ccc, 0, 15);
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 0, 15, 1);

	/* Enable the final output clock */
	lcd_combo_dphy_setb(COMBO_DPHY_VID_PLL0_DIV, 1, 19, 1);
}

static void lcd_prbs_config_clk(struct aml_lcd_drv_s *pdrv, unsigned int lcd_prbs_mode)
{
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	lcd_prbs_set_pll_lvds_txhd2(pdrv);

	lcd_clk_setb(HHI_VIID_CLK_DIV, 0, VCLK2_XD, 8);
	udelay(5);

	/* select vid_pll_clk */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_CLK_IN_SEL, 3);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_EN, 1);
	udelay(5);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_SOFT_RST, 1);
	udelay(10);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 1, ENCL_GATE_VCLK, 1);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s ok\n", __func__);
}

static int lcd_clk_prbs_test_txhd2(struct aml_lcd_drv_s *pdrv,
				   unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned int combo_dphy_ctrl0, combo_dphy_ctrl1, bit_width;
	int encl_msr_id, fifo_msr_id;
	unsigned int lcd_prbs_cnt;
	unsigned int val1, val2, timeout;
	unsigned int clk_err_cnt = 0;
	unsigned long lcd_encl_clk_check_std, lcd_fifo_clk_check_std;
	int j, ret;

	if (!cconf)
		return -1;
	if (!(mode_flag & LCD_PRBS_MODE_LVDS)) {
		LCDPR("%s: not support\n", __func__);
		goto lcd_prbs_test_err_txhd2;
	}

	//bit[15:0]: reg_hi_edp_lvds_tx_phy0_cntl0
	combo_dphy_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
	//bit[31:24]: reg_hi_edp_lvds_tx_phy0_cntl1
	//bit[19:0]: ro_hi_edp_lvds_tx_phy0_cntl1_o
	combo_dphy_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
	bit_width = 10;

	encl_msr_id = cconf->data->enc_clk_msr_id;
	fifo_msr_id = -1;

	timeout = (ms > 1000) ? 1000 : ms;

	lcd_combo_dphy_write(combo_dphy_ctrl0, 0);
	lcd_combo_dphy_write(combo_dphy_ctrl1, 0);

	lcd_prbs_cnt = 0;
	clk_err_cnt = 0;
	LCDPR("[%d]: lcd_prbs_mode: 0x%lx\n", pdrv->index, LCD_PRBS_MODE_LVDS);
	lcd_encl_clk_check_std = 136000000;
	lcd_fifo_clk_check_std = 48000000;

	lcd_prbs_config_clk(pdrv, LCD_PRBS_MODE_LVDS);
	udelay(500);

	/* set fifo_clk_sel: div 10 */
	// COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0[7:6]: Fifo_clk_sel
	lcd_combo_dphy_write(combo_dphy_ctrl0, (3 << 6));
	/* set cntl_ser_en:  10-channel */
	lcd_combo_dphy_setb(combo_dphy_ctrl0, 0x3ff, 16, 10);
	lcd_combo_dphy_setb(combo_dphy_ctrl0, 1, 2, 1);
	/* decoupling fifo enable, gated clock enable */
	lcd_combo_dphy_write(combo_dphy_ctrl1, (1 << 30) | (1 << 24));
	/* decoupling fifo write enable after fifo enable */
	lcd_combo_dphy_setb(combo_dphy_ctrl1, 1, 31, 1);

	/* cntl_prbs_en & cntl_prbs_err_en*/
	lcd_combo_dphy_setb(combo_dphy_ctrl0, 1, 13, 1);
	lcd_combo_dphy_setb(combo_dphy_ctrl0, 1, 12, 1);

	while (lcd_prbs_flag) {
		if (lcd_prbs_cnt++ >= timeout)
			break;
		ret = 1;
		val1 = lcd_combo_dphy_getb(combo_dphy_ctrl1, bit_width, bit_width);
		udelay(1000);

		for (j = 0; j < 20; j++) {
			val2 = lcd_combo_dphy_getb(combo_dphy_ctrl1, bit_width, bit_width);
			udelay(5);
			if (val2 != val1) {
				ret = 0;
				break;
			}
		}
		if (ret) {
			LCDERR("[%d]: prbs error 1, val:0x%03x, cnt:%d\n",
			       pdrv->index, val2, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}
		if (lcd_combo_dphy_getb(combo_dphy_ctrl1, 0, bit_width)) {
			LCDERR("[%d]: prbs error 2, cnt:%d\n", pdrv->index, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}

		if (lcd_prbs_clk_check(lcd_encl_clk_check_std, encl_msr_id,
				       lcd_fifo_clk_check_std, fifo_msr_id, lcd_prbs_cnt))
			clk_err_cnt++;
		else
			clk_err_cnt = 0;
		if (clk_err_cnt >= 10) {
			LCDERR("[%d]: prbs error 3(clkmsr), cnt:%d\n", pdrv->index, lcd_prbs_cnt);
			goto lcd_prbs_test_err_txhd2;
		}
	}

	lcd_combo_dphy_write(combo_dphy_ctrl0, 0);
	lcd_combo_dphy_write(combo_dphy_ctrl1, 0);

	lcd_prbs_performed = LCD_PRBS_MODE_LVDS;
	lcd_prbs_err = 0;
	lcd_prbs_flag = 0;
	LCDPR("[%d]: lvds prbs check ok\n", pdrv->index);
	return 0;

lcd_prbs_test_err_txhd2:
	lcd_prbs_performed = LCD_PRBS_MODE_LVDS;
	lcd_prbs_err = LCD_PRBS_MODE_LVDS;
	lcd_prbs_flag = 0;
	return -1;
}

static struct lcd_clk_data_s lcd_clk_data_txhd2 = {
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
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 1500000000U,
	.xd_out_fmax = 400000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 9,
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
	.set_ss_level = lcd_set_pll_ss_level_txhd2,
	.set_ss_advance = lcd_set_pll_ss_advance_txhd2,
	.clk_ss_enable = lcd_pll_ss_enable_txhd2,
	.pll_frac_set = lcd_pll_frac_set,
	.clk_set = lcd_clk_set_txhd2,
	.vclk_crt_set = lcd_set_vclk_crt_dft,
	.clk_disable = lcd_clk_disable_txhd2,
	.clktree_set = lcd_clktree_set_txhd2,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_clk_config = NULL,
	.prbs_test = lcd_clk_prbs_test_txhd2,
};

void lcd_clk_config_chip_init_txhd2(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_txhd2;
	cconf->pll_od_fb = lcd_clk_data_txhd2.pll_od_fb;
}
#endif
