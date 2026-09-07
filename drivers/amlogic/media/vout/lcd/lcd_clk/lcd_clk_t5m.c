// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "../connectors/lcd_connector.h"
#include "lcd_clk_config.h"
#include "lcd_clk_ctrl.h"
#include "lcd_clk_utils.h"

#ifdef CONFIG_MESON_T5M
static unsigned int tcon_div[5][3] = {
	/* div_mux, div2/4_sel, div4_bypass */
	{1, 0, 1},  /* div1 */
	{0, 0, 1},  /* div2 */
	{0, 1, 1},  /* div4 */
	{0, 0, 0},  /* div8 */
	{0, 1, 0},  /* div16 */
};

static void lcd_pll_ss_enable(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, offset;
	unsigned int flag;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	offset = cconf->pll_config[0].pll_offset;
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
		LCDPR("[%d]: pll ss enable: level: %d, %dppm\n",
		      pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, pll_ctrl2);
}

static void lcd_set_pll_ss(struct aml_lcd_drv_s *pdrv, unsigned int ss_flag)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl2, offset;
	char prt_str[64];
	int len = 0, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	offset = cconf->pll_config[0].pll_offset;
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2 + offset);

	if (ss_flag & LCD_SSC_LEVEL) {
		pll_ctrl2 &= ~((1 << 15) | (0xf << 16) | (0xf << 28));

		if (cconf->ss_level > 0) {
			ret = lcd_pll_ss_level_generate(cconf);
			if (ret == 0) {
				cconf->ss_en = 1;
				pll_ctrl2 |= ((1 << 15) |
					(cconf->ss_dep_sel << 28) |
					(cconf->ss_str_m << 16));
				len += sprintf(prt_str + len, "level level: %d, %dppm\n",
					       cconf->ss_level, cconf->ss_ppm);
			}
		} else {
			cconf->ss_en = 0;
			len += sprintf(prt_str + len, "disable\n");
		}
	}

	if (ss_flag & LCD_SSC_FREQ) {
		pll_ctrl2 &= ~(0x7 << 24); /* ss_freq */
		pll_ctrl2 |= (cconf->ss_freq << 24);
		len += sprintf(prt_str + len, "%sfreq=%d\n", len ? ", " : "", cconf->ss_freq);
	}

	if (ss_flag & LCD_SSC_MODE) {
		pll_ctrl2 &= ~(0x3 << 22); /* ss_mode */
		pll_ctrl2 |= (cconf->ss_mode << 22);
		len += sprintf(prt_str + len, "%smode=%d\n", len ? ", " : "", cconf->ss_mode);
	}

	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, pll_ctrl2);
	LCDPR("[%d]: set ssc: %s\n", pdrv->index, len ? prt_str : "none");
}

static void lcd_pll_frac_set(struct aml_lcd_drv_s *pdrv, unsigned int frac)
{
	struct lcd_clk_config_s *cconf;
	unsigned int offset, reg, val;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	offset = cconf->pll_config[0].pll_offset;
	reg = ANACTRL_TCON_PLL0_CNTL1 + offset;
	val = lcd_ana_read(reg);
	lcd_ana_setb(reg, frac, 0, 17);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: reg 0x%x: 0x%08x->0x%08x\n",
		      pdrv->index, __func__, reg, val, lcd_ana_read(reg));
	}
	LCDPR("[%d]: %s: pll_frac=0x%x\n", pdrv->index, __func__, frac);
}

static void lcd_set_pll_t5m(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl, pll_ctrl1, pll_stts, offset;
	unsigned int tcon_div_sel;
	int ret, cnt = 0;

	if (pdrv->index) /* clk_path1 invalid */
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	tcon_div_sel = cconf->pll_tcon_div_sel;
	pll_ctrl = ((0x3 << 17) | /* gate ctrl */
		(tcon_div[tcon_div_sel][2] << 16) |
		(cconf->pll_config[0].pll_n << LCD_PLL_N_TL1) |
		(cconf->pll_config[0].pll_m << LCD_PLL_M_TL1) |
		(cconf->pll_config[0].pll_od3_sel << LCD_PLL_OD3_T7) |
		(cconf->pll_config[0].pll_od2_sel << LCD_PLL_OD2_T7) |
		(cconf->pll_config[0].pll_od1_sel << LCD_PLL_OD1_T7));
	pll_ctrl1 = (1 << 28) |
		(tcon_div[tcon_div_sel][0] << 22) |
		(tcon_div[tcon_div_sel][1] << 21) |
		((1 << 20) | /* sdm_en */
		(cconf->pll_config[0].pll_frac << 0));

	offset = cconf->pll_config[0].pll_offset;
	pll_stts = ANACTRL_TCON_PLL0_STS;

set_pll_retry_t5m:
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0 + offset, pll_ctrl);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 1, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 1, LCD_PLL_EN_TL1, 1);
	udelay(10);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL1 + offset, pll_ctrl1);
	udelay(10);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, 0x0000110c);
	udelay(10);
	if (cconf->pll_config[0].pll_fvco < 3800000000ULL)
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL3 + offset, 0x10051100);
	else
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL3 + offset, 0x10051400);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL4 + offset, 0x0100c0, 0, 24);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL4 + offset, 0x8300c0, 0, 24);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 1, 26, 1);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 0, LCD_PLL_RST_TL1, 1);
	udelay(10);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2 + offset, 0x0000300c);

	ret = lcd_pll_wait_lock(cconf->pll_config[0].pll_id, pll_stts, LCD_PLL_LOCK_T7);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry_t5m;
		LCDERR("[%d]: pll lock failed\n", pdrv->index);
	} else {
		udelay(100);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL2 + offset, 1, 5, 1);
	}

	if (cconf->ss_level > 0)
		lcd_set_pll_ss(pdrv, (LCD_SSC_LEVEL | LCD_SSC_FREQ | LCD_SSC_MODE));
}

#ifdef CONFIG_AML_LCD_TCON
/* tcon run base clk, include register access */
static void lcd_set_tcon_clk_t5m(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int val;

	if (pconf->basic.lcd_type != LCD_MLVDS &&
	    pconf->basic.lcd_type != LCD_P2P)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		val = pconf->phy_cfg.act_phy->clk_phase;
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL1, (val & 0xf), 24, 4);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL4, ((val >> 4) & 0xf), 28, 4);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL4, ((val >> 8) & 0xf), 24, 4);

		/* tcon_clk */
		if (pconf->timing.enc_clk >= 100000000) /* 25M */
			lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0xf << 0));
		else /* 12.5M */
			lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0x1f << 0));
		break;
	case LCD_P2P:
		/* tcon_clk 50M */
		lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (7 << 0));
		break;
	default:
		break;
	}

	/* global reset tcon */
	lcd_tcon_global_reset(pdrv);
}
#endif

static void lcd_set_vid_pll_div_t5m(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int shift_val, shift_sel;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	if (cconf->pll_config[0].pll_id) {
		/* only crt_video valid for clk path1 */
		lcd_clk_setb(CLKCTRL_VIID_CLK1_CTRL, 0, VCLK2_EN, 1);
		udelay(5);
		return;
	}

	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, VCLK2_EN, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(ANACTRL_VID_PLL_CLK_DIV, 0, 15, 1);

	if (cconf->data->pll_data[0]->div_sel_max == CLK_DIV_SEL_1 ||
	    cconf->pll_config->div_sel > cconf->data->pll_data[0]->div_sel_max ||
	    cconf->pll_config->div_sel >= ARRAY_SIZE(lcd_clk_div_table)) {
		LCDERR("[%d]: invalid clk divider\n", pdrv->index);
		return;
	}

	shift_val = lcd_clk_div_table[cconf->pll_config->div_sel].shift_val;
	shift_sel = lcd_clk_div_table[cconf->pll_config->div_sel].shift_sel;

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

static void lcd_clk_set_t5m(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll_t5m(pdrv);
	lcd_set_vid_pll_div_t5m(pdrv);
}

static void lcd_set_vclk_crt(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_vid2_clk_div, reg_vid2_clk_ctrl, reg_vid_clk_ctrl2;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	reg_vid2_clk_div = CLKCTRL_VIID_CLK0_DIV;
	reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
	reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK0_CTRL2;

	lcd_clk_write(reg_vid_clk_ctrl2, 0);
	lcd_clk_write(reg_vid2_clk_ctrl, 0);
	lcd_clk_write(reg_vid2_clk_div, 0);
	udelay(5);

	lcd_clk_setb(CLKCTRL_HDMI_VID_PLL_CLK_DIV, 0, 24, 1);

#ifdef CONFIG_AML_LCD_PXP
	/* setup the XD divider value */
	lcd_clk_setb(reg_vid2_clk_div, cconf->xd, VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(reg_vid2_clk_ctrl, 7, VCLK2_CLK_IN_SEL, 3);
#else
	/* setup the XD divider value */
	lcd_clk_setb(reg_vid2_clk_div, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(reg_vid2_clk_ctrl, cconf->data->vclk_sel,
		     VCLK2_CLK_IN_SEL, 3);
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

//special setting by lcd interface
static void lcd_clktree_set(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_TCON
	lcd_set_tcon_clk_t5m(pdrv);
#endif
}

static void lcd_clk_disable(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_vid_clk_ctrl2, reg_vid2_clk_ctrl, offset;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK0_CTRL2;
	reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
	offset = cconf->pll_config[0].pll_offset;

	lcd_clk_setb(reg_vid_clk_ctrl2, 0, ENCL_GATE_VCLK, 1);

	/* close vclk2_div gate: [4:0] */
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, 0, 5);
	lcd_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_EN, 1);

	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 0, 28, 1);  //disable
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0 + offset, 1, 29, 1);  //reset
}

static void lcd_clk_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	int i;
	unsigned int *table = NULL, size = 0;
	unsigned int pll_reg_table[] = {
		ANACTRL_TCON_PLL0_CNTL0,
		ANACTRL_TCON_PLL0_CNTL1,
		ANACTRL_TCON_PLL0_CNTL2,
		ANACTRL_TCON_PLL0_CNTL3,
		ANACTRL_TCON_PLL0_CNTL4,
		ANACTRL_TCON_PLL0_STS,
		ANACTRL_VID_PLL_CLK_DIV
	};
	unsigned int clk_reg_table[] = {
		CLKCTRL_VIID_CLK0_DIV,
		CLKCTRL_VIID_CLK0_CTRL,
		CLKCTRL_VID_CLK0_CTRL2
	};

	table = pll_reg_table;
	size = ARRAY_SIZE(pll_reg_table);
	for (i = 0; i < size; i++)
		printf("pll [0x%08x] = 0x%08x\n", table[i], lcd_ana_read(table[i]));

	table = clk_reg_table;
	size = ARRAY_SIZE(clk_reg_table);
	for (i = 0; i < size; i++)
		printf("clk [0x%08x] = 0x%08x\n", table[i], lcd_clk_read(table[i]));

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
	if (cconf->pll_config[0].done == 0)
		return;

	lcd_clk_set_t5m(pdrv);
	lcd_set_vclk_crt(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s ok\n", pdrv->index, __func__);
}

static int lcd_prbs_test_t5m(struct aml_lcd_drv_s *pdrv, unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf;
	unsigned int reg_phy_tx_ctrl0, reg_phy_tx_ctrl1;
	int encl_msr_id, fifo_msr_id;
	unsigned int lcd_prbs_mode;
	unsigned int val1, val2, timeout;
	unsigned int cnt = 0;
	unsigned int clk_err_cnt = 0;
	unsigned int lcd_encl_clk_check_std = 0, lcd_fifo_clk_check_std = 0;
	int i, j, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return -1;

	reg_phy_tx_ctrl0 = ANACTRL_LVDS_TX_PHY_CNTL0;
	reg_phy_tx_ctrl1 = ANACTRL_LVDS_TX_PHY_CNTL1;

	encl_msr_id = cconf->data->enc_clk_msr_id;
	fifo_msr_id = -1;

	timeout = (ms > 1000) ? 1000 : ms;

	for (i = 0; i < LCD_PRBS_MODE_MAX; i++) {
		if ((mode_flag & (1 << i)) == 0)
			continue;

		lcd_ana_write(reg_phy_tx_ctrl0, 0);
		lcd_ana_write(reg_phy_tx_ctrl1, 0);

		cnt = 0;
		clk_err_cnt = 0;
		lcd_prbs_mode = (1 << i);
		LCDPR("[%d]: lcd_prbs_mode: %d\n", pdrv->index, lcd_prbs_mode);
		lcd_prbs_config_clk(pdrv, lcd_prbs_mode, &lcd_encl_clk_check_std,
				    &lcd_fifo_clk_check_std);
		udelay(500);

		/* set fifo_clk_sel: div 10 */
		lcd_ana_write(reg_phy_tx_ctrl0, (3 << 6));
		/* set cntl_ser_en:  8-channel to 1 */
		lcd_ana_setb(reg_phy_tx_ctrl0, 0xfff, 16, 12);
		lcd_ana_setb(reg_phy_tx_ctrl0, 1, 2, 1);
		/* decoupling fifo enable, gated clock enable */
		lcd_ana_write(reg_phy_tx_ctrl1, (1 << 30) | (1 << 24));
		/* decoupling fifo write enable after fifo enable */
		lcd_ana_setb(reg_phy_tx_ctrl1, 1, 31, 1);
		/* prbs_err en */
		lcd_ana_setb(reg_phy_tx_ctrl0, 1, 13, 1);
		lcd_ana_setb(reg_phy_tx_ctrl0, 1, 12, 1);

		while (cnt++ < timeout) {
			val1 = lcd_ana_getb(reg_phy_tx_ctrl1, 12, 12);
			udelay(1000);
			ret = 1;
			for (j = 0; j < 20; j++) {
				udelay(5);
				val2 = lcd_ana_getb(reg_phy_tx_ctrl1, 12, 12);
				if (val2 != val1) {
					ret = 0;
					break;
				}
			}
			if (ret) {
				LCDERR("[%d]: prbs check error 1(state), val:0x%03x, cnt:%d\n",
				       pdrv->index, val2, cnt);
				goto lcd_prbs_test_t5m_err;
			}
			if (lcd_ana_getb(reg_phy_tx_ctrl1, 0, 12)) {
				LCDERR("[%d]: prbs check error 2(prbs), cnt:%d\n",
				       pdrv->index, cnt);
				goto lcd_prbs_test_t5m_err;
			}
			if (lcd_prbs_clk_check(lcd_encl_clk_check_std, encl_msr_id,
					       lcd_fifo_clk_check_std, fifo_msr_id, cnt))
				clk_err_cnt++;
			else
				clk_err_cnt = 0;
			if (clk_err_cnt >= 10) {
				LCDERR("[%d]: prbs check error 3(clkmsr), cnt:%d\n",
				       pdrv->index, cnt);
				goto lcd_prbs_test_t5m_err;
			}
		}

		lcd_ana_write(reg_phy_tx_ctrl0, 0);
		lcd_ana_write(reg_phy_tx_ctrl1, 0);

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
		continue;

lcd_prbs_test_t5m_err:
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
	}

	lcd_ana_setb(reg_phy_tx_ctrl0, 0, 12, 2);

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

static const unsigned int tcon_div_table[5] = {1, 2, 4, 8, 16};

static int lcd_clk_generate_p2p_with_tcon_div_t5m(struct lcd_clk_config_s *cconf,
						  unsigned long long bit_rate)
{
	unsigned long long pll_fvco;
	unsigned int tcon_div_sel = 0;
	int done = 0;

	for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
		pll_fvco = bit_rate * tcon_div_table[tcon_div_sel];
		done = check_vco(cconf, 0, pll_fvco);
		if (done) {
			cconf->pll_tcon_div_sel = tcon_div_sel;
			cconf->phy_clk = bit_rate;
			break;
		}
	}

	return done;
}

static void lcd_clk_generate_t5m(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned long long bit_rate;
	int done = 0;

	if (pdrv->config.basic.lcd_type != LCD_P2P) {
		lcd_clk_generate_dft(pdrv);
		return;
	}

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;
	if (pdrv->config.timing.act_timing.clk_mode == 2) { //165hz
		//gp0 pll fixed clk: 1544.4MHz
		LCDPR("%s: dual pll config\n", __func__);
		cconf->pll_mode |= LCD_PLL_MODE_DUAL_PLL;

		cconf->fout = pdrv->config.timing.enc_clk;
		cconf->data->vclk_sel = 1; //gp0
		cconf->xd = 2;

		bit_rate = pdrv->config.timing.bit_rate;
		done = lcd_clk_generate_p2p_with_tcon_div_t5m(cconf, bit_rate);
		if (done) {
			pdrv->config.timing.pll_ctrl =
				(cconf->pll_config[0].pll_od1_sel << PLL_CTRL_OD1) |
				(cconf->pll_config[0].pll_od2_sel << PLL_CTRL_OD2) |
				(cconf->pll_config[0].pll_od3_sel << PLL_CTRL_OD3) |
				(cconf->pll_config[0].pll_n << PLL_CTRL_N)         |
				(cconf->pll_config[0].pll_m << PLL_CTRL_M);
			pdrv->config.timing.div_ctrl = (cconf->xd << DIV_CTRL_XD);
			pdrv->config.timing.clk_ctrl =
				(cconf->pll_config[0].pll_frac << CLK_CTRL_FRAC) |
				(cconf->pll_config[0].pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
			cconf->pll_config[0].done = 1;
		} else {
			pdrv->config.timing.pll_ctrl = 0;
			pdrv->config.timing.div_ctrl = 0;
			pdrv->config.timing.clk_ctrl = 0;
			cconf->pll_config[0].done = 0;
			LCDERR("[%d]: %s: Out of clock range\n", pdrv->index, __func__);
		}
	} else {
		cconf->pll_mode &= ~LCD_PLL_MODE_DUAL_PLL;
		cconf->data->vclk_sel = 0; //vid_pll
		lcd_clk_generate_dft(pdrv);
	}
}

static struct lcd_pll_data_s lcd_pll_data_t5m = {
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
	.pll_out_fmax = 3700000000ULL,
	.pll_out_fmin = 187500000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.div_in_fmax = 3700000000ULL,
	.div_out_fmax = 800000000,
	.div_sel_max = CLK_DIV_SEL_2p33,
};

static struct lcd_clk_data_s lcd_clk_data_t5m = {
	.pll_data[0] = &lcd_pll_data_t5m,
	.pll_data[1] = NULL,
	.xd_out_fmax = 800000000,
	.phy_clk_location = 0,

	.vclk_sel = 0,
	.enc_clk_msr_id = 222,
	.fifo_clk_msr_id = -1,

	.xd_max = 128,
	.phy_div_max = 128,

	.ss_support = 1,
	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_t5m,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss = lcd_set_pll_ss,
	.clk_ss_enable = lcd_pll_ss_enable,
	.pll_frac_set = lcd_pll_frac_set,
	.clk_set = lcd_clk_set_t5m,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable,
	.clktree_set = lcd_clktree_set,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.clk_reg_print = lcd_clk_reg_dump,
	.prbs_test = lcd_prbs_test_t5m,
};

struct lcd_clk_config_s *lcd_clk_config_chip_init_t5m(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = NULL;
	unsigned int size;

	if (!pdrv)
		return NULL;

	if (!pdrv->clk_conf) {
		cconf = (struct lcd_clk_config_s *)malloc(sizeof(struct lcd_clk_config_s));
		if (!cconf) {
			LCDERR("[%d]: %s: Not enough memory\n", pdrv->index, __func__);
			return NULL;
		}
	} else {
		cconf = (struct lcd_clk_config_s *)pdrv->clk_conf;
	}
	memset(cconf, 0, sizeof(struct lcd_clk_config_s));

	cconf->pll_conf_num = 1;
	size = cconf->pll_conf_num * sizeof(struct lcd_pll_config_s);
	cconf->pll_config = (struct lcd_pll_config_s *)malloc(size);
	if (!cconf->pll_config) {
		LCDERR("[%d]: %s: Not enough memory for pll config\n", pdrv->index, __func__);
		free(cconf);
		return NULL;
	}
	memset(cconf->pll_config, 0, size);

	cconf->data = &lcd_clk_data_t5m;
	cconf->pll_config[0].pll_od_fb = cconf->data->pll_data[0]->pll_od_fb;
	cconf->pll_config[0].pll_id = 0;
	return cconf;
}
#endif
