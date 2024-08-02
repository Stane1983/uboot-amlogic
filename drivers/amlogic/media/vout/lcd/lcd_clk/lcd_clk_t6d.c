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

static unsigned int tcon_div[][3] = {
	/* vx1pll_div214h, vx1pll_phase_div_en, vx1pll_clk1x_selh */
	{0, 0, 1},  /* div1 */
	{0, 0, 0},  /* div2 */
	{0, 0, 0},  /* div4 */
	{0, 1, 0},  /* div8 */
	{1, 1, 0},  /* div16 */
};

static void lcd_pll_frac_set_t6d(struct aml_lcd_drv_s *pdrv, unsigned int frac)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg, val;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	reg = ANACTRL_TCON_PLL0_CNTL4;
	val = lcd_ana_read(reg);
	lcd_ana_setb(reg, frac, 0, 17);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: reg 0x%x: 0x%08x->0x%08x\n",
		      pdrv->index, __func__, reg, val, lcd_ana_read(reg));
	}
	LCDPR("[%d]: %s: pll_frac=0x%x\n", pdrv->index, __func__, frac);
}

static void lcd_set_pll_ss_level_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl2;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl0 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL0);
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2);

	pll_ctrl0 &= ~(1 << 11);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 16));

	if (cconf->ss_level > 0) {
		ret = lcd_pll_ss_level_generate(cconf);
		if (ret == 0) {
			cconf->ss_en = 1;
			pll_ctrl0 |= (1 << 11);
			pll_ctrl2 |= ((cconf->ss_dep_sel << 16) | (cconf->ss_str_m << 4));
			LCDPR("[%d]: set pll spread spectrum: level: %d, %dppm\n",
			      pdrv->index, cconf->ss_level, cconf->ss_ppm);
		}
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: set pll spread spectrum: disable\n", pdrv->index);
	}
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, pll_ctrl2);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0, pll_ctrl0);
}

static void lcd_set_pll_ss_advance_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2;
	unsigned int freq, mode;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	freq = cconf->ss_freq;
	mode = cconf->ss_mode;
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2);
	pll_ctrl2 &= ~(0x7 << 8); /* ss_freq */
	pll_ctrl2 |= (freq << 8);
	pll_ctrl2 &= ~(0x3 << 2); /* ss_mode */
	pll_ctrl2 |= (mode << 2);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, pll_ctrl2);

	LCDPR("set pll spread spectrum: freq=%d, mode=%d\n", freq, mode);
}

static void lcd_pll_ss_enable_t6d(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl2;
	unsigned int flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl0 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL0);
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2);

	pll_ctrl0 &= ~(1 << 11);
	pll_ctrl2 &= ~((0xf << 4) | (0xf << 16));

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
		pll_ctrl0 |= (1 << 11);
		pll_ctrl2 |= ((cconf->ss_dep_sel << 16) | (cconf->ss_str_m << 4));
		LCDPR("[%d]: pll ss enable: level: %d, %dppm\n",
		      pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, pll_ctrl2);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0, pll_ctrl0);
}

static void lcd_set_pll_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl3, pll_ctrl4;
	unsigned int tcon_div_sel;
	int ret, cnt = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	tcon_div_sel = cconf->pll_tcon_div_sel;
	pll_ctrl0 = (0x2 << 16) | (1 << 13) | (cconf->pll_m << 0);
	pll_ctrl3 = (0x7 << 7) |
		(cconf->pll_od1_sel << 10) |
		(cconf->pll_od2_sel << 14) |
		(cconf->pll_od3_sel << 12) |
		(tcon_div_sel << 24) |
		(0x1 << 31);
	if (pconf->basic.lcd_type != LCD_MLVDS) {
		pll_ctrl3 &= ~(1 << 27);
	} else {
		if (tcon_div_sel == 1 || tcon_div_sel == 2) {
			LCDERR("[%d]: div4/8 with phase not support\n",
			       pdrv->index);
			return;
		}
		pll_ctrl3 |= (1 << 27) |
			(tcon_div[tcon_div_sel][0] << 28) |
			(tcon_div[tcon_div_sel][1] << 29) |
			(tcon_div[tcon_div_sel][2] << 30);
	}
	pll_ctrl4 = cconf->pll_frac;

	do {
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0, pll_ctrl0);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL1, 0x12301635);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, 0x02000000);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL3, pll_ctrl3);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL4, pll_ctrl4);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 28, 1);
		udelay(10);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 18, 1);
		udelay(80);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 18, 1);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 3, 29, 2);
		ret = lcd_pll_wait_lock(ANACTRL_TCON_PLL0_STS, 31);
	} while (ret && ++cnt < PLL_RETRY_MAX);

	if (ret)
		LCDERR("[%d]: pll lock failed\n", pdrv->index);

	if (cconf->ss_level > 0) {
		lcd_set_pll_ss_level_t6d(pdrv);
		lcd_set_pll_ss_advance_t6d(pdrv);
	}
}

static void lcd_set_vid_pll_div_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int shift_val, shift_sel;
	int i;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, VCLK2_EN, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 15, 1);

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
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 1, 18, 1);
	} else {
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 18, 1);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 20, 1);/*div8_25*/
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 16, 2);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 15, 1);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 0, 15);

		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, shift_sel, 16, 2);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 1, 15, 1);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, shift_val, 0, 15);
		lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 15, 1);
	}
	/* Enable the final output clock */
	lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 1, 19, 1);
}

static void lcd_clk_set_t6d(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll_t6d(pdrv);
	lcd_set_vid_pll_div_t6d(pdrv);
}

static void lcd_set_vclk_crt_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	lcd_clk_write(CLKCTRL_VID_CLK0_CTRL2, 0);
	lcd_clk_write(CLKCTRL_VIID_CLK0_CTRL, 0);
	lcd_clk_write(CLKCTRL_VIID_CLK0_DIV, 0);
	udelay(5);

	lcd_clk_setb(CLKCTRL_HDMI_VID_PLL_CLK_DIV, 0, 24, 1);

#if (IS_ENABLED(CONFIG_AML_LCD_PXP))
	/* setup the XD divider value */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, cconf->xd, VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 7, VCLK2_CLK_IN_SEL, 3);
#else
	/* setup the XD divider value */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, cconf->data->vclk_sel,
		     VCLK2_CLK_IN_SEL, 3);
#endif
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 1, VCLK2_SOFT_RST, 1);
	udelay(10);
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(CLKCTRL_VID_CLK0_CTRL2, 1, ENCL_GATE_VCLK, 1);
}

#if (IS_ENABLED(CONFIG_AML_LCD_TCON))
/* tcon run base clk, include register access */
static void lcd_set_tcon_clk_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int val, p0, pa, pb;

	if (pconf->basic.lcd_type != LCD_MLVDS)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	p0 = val & 0xf;
	pa = (val >> 4) & 0xf;
	pb = (val >> 8) & 0xf;

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		val = ((pa << 4) | (pb << 0)) & 0xff;
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL2, p0,  28, 4);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, val, 24, 8);

		/* tcon_clk */
		if (pconf->timing.enc_clk >= 100000000) /* 25M */
			lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0xf << 0));
		else /* 12.5M */
			lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0x1f << 0));
		break;
	default:
		break;
	}

	/* global reset tcon */
	lcd_tcon_global_reset(pdrv);
}
#endif

static void lcd_clktree_set_t6d(struct aml_lcd_drv_s *pdrv)
{
#if (IS_ENABLED(CONFIG_AML_LCD_TCON))
	lcd_set_tcon_clk_t6d(pdrv);
#endif
}

static int lcd_clk_prbs_test_t6d(struct aml_lcd_drv_s *pdrv,
				 unsigned int ms, unsigned int mode_flag)
{
	LCDPR("TODO");
	return 0;
}

static void lcd_clk_disable_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	lcd_clk_setb(CLKCTRL_VID_CLK0_CTRL2, 0, ENCL_GATE_VCLK, 1);

	/* close vclk2_div gate: [4:0] */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, 0, 5);
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, VCLK2_EN, 1);

	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 30, 1);  //resetn
}

static struct lcd_clk_data_s lcd_clk_data_t6d = {
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
	.div_out_fmax = 750000000,
	.xd_out_fmax = 400000000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.have_pll_div = 1,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 3,

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
	.set_ss_level = lcd_set_pll_ss_level_t6d,
	.set_ss_advance = lcd_set_pll_ss_advance_t6d,
	.clk_ss_enable = lcd_pll_ss_enable_t6d,
	.pll_frac_set = lcd_pll_frac_set_t6d,
	.clk_set = lcd_clk_set_t6d,
	.vclk_crt_set = lcd_set_vclk_crt_t6d,
	.clk_disable = lcd_clk_disable_t6d,
	.clktree_set = lcd_clktree_set_t6d,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.prbs_clk_config = NULL,
	.prbs_test = lcd_clk_prbs_test_t6d,
};

void lcd_clk_config_chip_init_t6d(struct aml_lcd_drv_s *pdrv,
				  struct lcd_clk_config_s *cconf)
{
	cconf->data = &lcd_clk_data_t6d;
	cconf->pll_od_fb = lcd_clk_data_t6d.pll_od_fb;
}

