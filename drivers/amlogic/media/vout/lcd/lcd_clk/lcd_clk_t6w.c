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

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
#if defined(CONFIG_MESON_T6W)
========
#if defined(CONFIG_MESON_T6D)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c

static unsigned int tcon_div[][3] = {
	/* vx1pll_div214h, vx1pll_phase_div_en, vx1pll_clk1x_selh */
	{0, 0, 1},  /* div1 */
	{0, 0, 0},  /* div2 */
	{0, 0, 0},  /* div4 */
	{0, 1, 0},  /* div8 */
	{1, 1, 0},  /* div16 */
};

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static unsigned char lcd_ss_freq_dep_opt_t6w[] = {
/*             freq, cnt, dep values       */
/* 0-29.5k */	0,    3,   4, 7, 10,
/* 1-31.5k */	1,    3,   3, 8, 11,
/* 2-50.0k */	2,    3,   5, 6, 11,
/* 3-29.5k */	3,    12,  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
/* 4-100k  */	4,    5,   3, 5, 6, 8, 11,
/* 5-150k       5,    12,  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, */
/* 6-200k  */	6,    12,  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
/* 7-0 end */   7,    0,
};

static void lcd_pll_frac_set_t6w(struct aml_lcd_drv_s *pdrv, unsigned int frac)
========
static void lcd_pll_frac_set_t6d(struct aml_lcd_drv_s *pdrv, unsigned int frac)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
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
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL_VLOCK, 1, 4, 1);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL_VLOCK, 0, 4, 1);
	LCDPR("[%d]: %s: pll_frac=0x%x\n", pdrv->index, __func__, frac);
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c

	if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL) {
		reg = ANACTRL_GP2PLL_CTRL2;
		val = lcd_ana_read(reg);
		lcd_ana_setb(reg, cconf->pll_config[1].pll_frac, 0, 19);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: reg 0x%x: 0x%08x->0x%08x\n",
				pdrv->index, __func__, reg, val, lcd_ana_read(reg));
		}
		LCDPR("[%d]: %s: gp2 pll_frac=0x%x\n", pdrv->index,
			__func__, cconf->pll_config[1].pll_frac);
	}
}

static void lcd_set_pll_ss_t6w(struct aml_lcd_drv_s *pdrv, unsigned int ss_flag)
========
}

static void lcd_set_pll_ss_t6d(struct aml_lcd_drv_s *pdrv, unsigned int ss_flag)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl2;
	char prt_str[64] = {0};
	int len = 0, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl0 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL0);
	pll_ctrl2 = lcd_ana_read(ANACTRL_TCON_PLL0_CNTL2);

	if (ss_flag & LCD_SSC_LEVEL) {
		pll_ctrl0 &= ~(1 << 11);
		pll_ctrl2 &= ~((0xf << 4) | (0xf << 16));

		if (cconf->ss_level > 0) {
			ret = lcd_pll_ss_level_generate(cconf);
			if (ret == 0) {
				cconf->ss_en = 1;
				pll_ctrl0 |= (1 << 11);
				pll_ctrl2 |= ((cconf->ss_dep_sel << 16) | (cconf->ss_str_m << 4));
				len += sprintf(prt_str + len, "level: %d, %dppm",
					       cconf->ss_level, cconf->ss_ppm);
			}
		} else {
			cconf->ss_en = 0;
			len += sprintf(prt_str + len, "disable");
		}
	}

	if (ss_flag & LCD_SSC_FREQ) {
		pll_ctrl2 &= ~(0x7 << 8); /* ss_freq */
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
		if (cconf->ss_freq)
			pll_ctrl2 |= (cconf->ss_freq << 8);
		else
			pll_ctrl2 |= (0x3 << 8);
========
		pll_ctrl2 |= (cconf->ss_freq << 8);
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
		len += sprintf(prt_str + len, "%sfreq=%d", len ? ", " : "", cconf->ss_freq);
	}

	if (ss_flag & LCD_SSC_MODE) {
		pll_ctrl2 &= ~(0x3 << 2); /* ss_mode */
		pll_ctrl2 |= (cconf->ss_mode << 2);
		len += sprintf(prt_str + len, "%smode=%d", len ? ", " : "", cconf->ss_mode);
	}

	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, pll_ctrl2);
	lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0, pll_ctrl0);

	LCDPR("[%d]: pll set ssc: %s\n", pdrv->index, len ? prt_str : "none");
}

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static void lcd_pll_ss_enable_t6w(struct aml_lcd_drv_s *pdrv, int status)
========
static void lcd_pll_ss_enable_t6d(struct aml_lcd_drv_s *pdrv, int status)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
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

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static void lcd_set_pll_t6w(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl1, pll_ctrl3, pll_ctrl4;
	unsigned int pll_ctrl2;
========
static void lcd_set_pll_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0, pll_ctrl3, pll_ctrl4;
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	unsigned int tcon_div_sel;
	int ret, cnt = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	tcon_div_sel = cconf->pll_tcon_div_sel;
	pll_ctrl0 = 0x22000 | (cconf->pll_config[0].pll_m << 0);
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	if (cconf->pll_config[0].pll_fvco <= 4750000000ULL &&
			cconf->pll_config[0].pll_fvco >= 3750000000ULL)
		pll_ctrl1 = 0x12301535;
	else if (cconf->pll_config[0].pll_fvco < 3750000000ULL)
		pll_ctrl1 = 0x12301335;
	else
		pll_ctrl1 = 0x12301435;
	pll_ctrl2 = 0x02000000;
========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	pll_ctrl3 = 0x180 |
		(cconf->pll_config[0].pll_od1_sel << 10) |
		(cconf->pll_config[0].pll_od2_sel << 14) |
		(cconf->pll_config[0].pll_od3_sel << 12) |
		(tcon_div_sel << 24) | /*tcon_div without phase*/
		(1 << 9); /*tcon_od_en*/
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
	pll_ctrl4 = cconf->pll_config[0].pll_frac;

	do {
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL0, pll_ctrl0);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL1, pll_ctrl1);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL2, pll_ctrl2);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL3, pll_ctrl3);
		lcd_ana_write(ANACTRL_TCON_PLL0_CNTL4, pll_ctrl4);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 28, 1);
		udelay(10);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 30, 1);
		udelay(80);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 18, 1);
		udelay(80);
		lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 29, 1);
		ret = lcd_pll_wait_lock(cconf->pll_config[0].pll_id, ANACTRL_TCON_PLL0_STS, 31);
	} while (ret && ++cnt < PLL_RETRY_MAX);
	if (ret)
		LCDERR("[%d]: vx1 pll lock failed\n", pdrv->index);

	//set phase load sequence
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, 0, 31, 1);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, 1, 31, 1);

	/* set load to 0 */
	lcd_ana_setb(ANACTRL_TCON_PLL_VLOCK, 0, 4, 1);
	/* select ANACTRL_TCON_PLL_VLOCK[4] as load */
	lcd_ana_setb(ANACTRL_TCON_PLL_VLOCK, 0, 3, 1);
	/* enable load en*/
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 14, 1);

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL) {
		cnt = 0;
		ret = 0;
		do {
			pll_ctrl0 = 0x22530000 |
				((cconf->pll_config[1].pll_od1_sel & 0x3) << 9) |
				(cconf->pll_config[1].pll_m & 0x1ff) |
				((cconf->pll_config[1].pll_n & 0x1f) << 11);
			lcd_ana_write(ANACTRL_GP2PLL_CTRL0, pll_ctrl0);
			lcd_ana_write(ANACTRL_GP2PLL_CTRL1, 0x1900322);
			pll_ctrl2 = (cconf->pll_config[1].pll_frac & 0x7ffff);
			lcd_ana_write(ANACTRL_GP2PLL_CTRL2, pll_ctrl2);
			lcd_ana_write(ANACTRL_GP2PLL_CTRL3, 0x6);
			lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 1, 28, 1);
			udelay(20);
			lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 0, 29, 1);
			udelay(20);
			lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 0, 16, 1);
			ret = lcd_pll_wait_lock(cconf->pll_config[1].pll_id,
						ANACTRL_GP2PLL_STS, 31);
		} while (ret && ++cnt < PLL_RETRY_MAX);
		if (ret)
			LCDERR("[%d]: gp2 pll lock failed\n", pdrv->index);
	}

	if (cconf->ss_level > 0)
		lcd_set_pll_ss_t6w(pdrv, (LCD_SSC_LEVEL | LCD_SSC_FREQ | LCD_SSC_MODE));
}

static void lcd_disable_pll_t6w(struct aml_lcd_drv_s *pdrv)
{
#ifndef CONFIG_AML_LCD_PXP
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);

	if (!cconf)
		return;

	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 29, 1);  //lock resetn
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 30, 1);  //pll resetn

	if ((cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL)) {
		lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 0, 28, 1);
		lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 1, 29, 1);
		lcd_ana_setb(ANACTRL_GP2PLL_CTRL0, 1, 16, 1);
	}
#endif
}

static void lcd_set_vid_pll_div_t6w(struct aml_lcd_drv_s *pdrv)
========
	if (cconf->ss_level > 0)
		lcd_set_pll_ss_t6d(pdrv, (LCD_SSC_LEVEL | LCD_SSC_FREQ | LCD_SSC_MODE));
}

static void lcd_set_vid_pll_div_t6d(struct aml_lcd_drv_s *pdrv)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
	struct lcd_clk_config_s *cconf;
	unsigned int shift_val, shift_sel;

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

	if (cconf->data->pll_data[0]->div_sel_max == CLK_DIV_SEL_1 ||
	    cconf->pll_config[0].div_sel > cconf->data->pll_data[0]->div_sel_max ||
	    cconf->pll_config[0].div_sel >= ARRAY_SIZE(lcd_clk_div_table)) {
		LCDERR("[%d]: invalid clk divider\n", pdrv->index);
		return;
	}

	shift_val = lcd_clk_div_table[cconf->pll_config[0].div_sel].shift_val;
	shift_sel = lcd_clk_div_table[cconf->pll_config[0].div_sel].shift_sel;

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

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static void lcd_clk_set_t6w(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll_t6w(pdrv);
	lcd_set_vid_pll_div_t6w(pdrv);
========
static void lcd_clk_set_t6d(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll_t6d(pdrv);
	lcd_set_vid_pll_div_t6d(pdrv);
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
}

#if (IS_ENABLED(CONFIG_AML_LCD_PXP))
static unsigned int pxp_fclk_div_clk[] = {
	4, 666000000,
	5, 500000000,
	6, 400000000,
	7, 285000000
};

#define pxp_abs_diff(a, b) ((a) > (b) ? ((a) - (b)) : ((b) - (a)))
static void pxp_clk_div_gen(unsigned int pclk, unsigned int *xd, unsigned int *clk_sel)
{
	int i = 0, j = 0;
	unsigned int clk_in;
	unsigned int prox_xd = 0, prox_id = 0, prox_diff = 0xffffffff, diff;

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	for (j = 0; j < (sizeof(pxp_fclk_div_clk)/sizeof(unsigned int) >> 1); j++) {
========
	for (j = 0; j < (sizeof(pxp_fclk_div_clk) / sizeof(unsigned int) >> 1); j++) {
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
		clk_in = pxp_fclk_div_clk[(j << 1) | 1];
		for (i = 1; i < 9; i++) {
			diff = pxp_abs_diff(clk_in / i, pclk);
			if (diff <= prox_diff) {
				prox_id = j;
				prox_xd = i;
				prox_diff = diff;
			}
		}
	}
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	*clk_sel = pxp_fclk_div_clk[(prox_id << 1 ) | 0];
	*xd = prox_xd;

	LCDPR("pxp: pclk:%d, xd:%d, clk_sel:%d, src_clk:%d real_clk:%d\n",
		pclk, *xd, *clk_sel, pxp_fclk_div_clk[(prox_id << 1 ) | 1],
		pxp_fclk_div_clk[(prox_id << 1 ) | 1] / *xd);
}
#endif

static void lcd_set_vclk_crt_t6w(struct aml_lcd_drv_s *pdrv)
========
	*clk_sel = pxp_fclk_div_clk[(prox_id << 1) | 0];
	*xd = prox_xd;

	LCDPR("pxp: pclk:%d, xd:%d, clk_sel:%d, src_clk:%d real_clk:%d\n",
		pclk, *xd, *clk_sel, pxp_fclk_div_clk[(prox_id << 1) | 1],
		pxp_fclk_div_clk[(prox_id << 1) | 1] / *xd);
}
#endif

static void lcd_set_vclk_crt_t6d(struct aml_lcd_drv_s *pdrv)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
	struct lcd_clk_config_s *cconf;
	unsigned int encl_clk_sel = 8;
#if (IS_ENABLED(CONFIG_AML_LCD_PXP))
	unsigned int pclk = 0, xd = 1, clk_sel = 7;
#endif

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
	pclk = pdrv->config.timing.enc_clk;
	pxp_clk_div_gen(pclk * 2, &xd, &clk_sel);// *2 because encl use div 2

	/* setup the XD divider value */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, xd - 1, VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, clk_sel, VCLK2_CLK_IN_SEL, 3); //fclk_div3=666
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 1, VCLK2_DIV2_EN, 1); // t6x 2ppc require
========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	encl_clk_sel = 9;// t6x 2ppc require div2
#else
	/* setup the XD divider value */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, cconf->data->vclk_sel, VCLK2_CLK_IN_SEL, 3);
	encl_clk_sel = 8; // div1
#endif
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_DIV, encl_clk_sel, ENCL_CLK_SEL, 4);
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
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static int lcd_set_mlvds_clk_phase_t6w(struct aml_lcd_drv_s *pdrv)
========
static int lcd_set_mlvds_clk_phase_t6d(struct aml_lcd_drv_s *pdrv)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int val, p0, pa, pb;

	val = pconf->phy_cfg.act_phy->clk_phase;
	p0 = val & 0xf;
	pa = (val >> 8) & 0xf;
	pb = (val >> 4) & 0xf;

	val = ((pa << 4) | (pb << 0)) & 0xff;
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL2, p0,  28, 4);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, val, 16, 8);

	//set phase load sequence
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, 0, 31, 1);
	udelay(10);
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL3, 1, 31, 1);

	return 0;
}
#endif

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static void lcd_set_tcon_clk_t6w(struct aml_lcd_drv_s *pdrv)
========
/* tcon run base clk, include register access */
static void lcd_set_tcon_clk_t6d(struct aml_lcd_drv_s *pdrv)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
#if (IS_ENABLED(CONFIG_AML_LCD_TCON))
	if (pdrv->config.basic.lcd_type != LCD_MLVDS &&
	    pdrv->config.basic.lcd_type != LCD_P2P)
		return;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	switch (pdrv->config.basic.lcd_type) {
	case LCD_MLVDS:
		lcd_set_mlvds_clk_phase_t6w(pdrv);

		/* tcon_clk */
		if (pdrv->config.timing.enc_clk >= 100000000) /* 25M */
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
#endif
}

static void lcd_clk_disable_t6w(struct aml_lcd_drv_s *pdrv)
========
	lcd_set_mlvds_clk_phase_t6d(pdrv);

	/* tcon_clk */
	if (pconf->timing.enc_clk >= 100000000) /* 25M */
		lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0xf << 0));
	else /* 12.5M */
		lcd_clk_write(CLKCTRL_TCON_CLK_CNTL, (1 << 7) | (1 << 6) | (0x1f << 0));

	/* global reset tcon */
	lcd_tcon_global_reset(pdrv);
}
#endif

static void lcd_clktree_set_t6d(struct aml_lcd_drv_s *pdrv)
{
#if (IS_ENABLED(CONFIG_AML_LCD_TCON))
	if (pdrv->config.basic.lcd_type == LCD_MLVDS)
		lcd_set_tcon_clk_t6d(pdrv);
#endif
}

static void lcd_prbs_config_clk_t6d(struct aml_lcd_drv_s *pdrv, unsigned int lcd_prbs_mode,
				    unsigned int *encl_clk, unsigned int *fifo_clk)
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
{
	lcd_clk_setb(CLKCTRL_VID_CLK0_CTRL2, 0, ENCL_GATE_VCLK, 1);

	/* close vclk2_div gate: [4:0] */
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, 0, 5);
	lcd_clk_setb(CLKCTRL_VIID_CLK0_CTRL, 0, VCLK2_EN, 1);

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	lcd_disable_pll_t6w(pdrv);
}

static void lcd_prbs_config_clk_t6w(struct aml_lcd_drv_s *pdrv, unsigned int lcd_prbs_mode,
				    unsigned int *encl_clk, unsigned int *fifo_clk)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned long long bit_rate = 0;

	if (!cconf)
		return;

	if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
		bit_rate = 519750000ULL;
		*encl_clk = lcd_do_div(bit_rate * 2, 7);
		*fifo_clk = lcd_do_div(bit_rate, 7);
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_VX1) {
		bit_rate = 2970000000ULL;
		*encl_clk = lcd_do_div(bit_rate, 5);
		*fifo_clk = lcd_do_div(bit_rate, 10);
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_FREQ) {
		bit_rate = lcd_prbs_freq * 1000000ULL;
		*encl_clk = lcd_do_div(bit_rate, 5);
		*fifo_clk = lcd_do_div(bit_rate, 10);
========
	if (lcd_prbs_mode == LCD_PRBS_MODE_LVDS) {
		bit_rate = 550000000ULL;
	} else if (lcd_prbs_mode == LCD_PRBS_MODE_FREQ) {
		bit_rate = lcd_prbs_freq * 1000000ULL;
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	} else {
		LCDERR("[%d]: %s: unsupport lcd_prbs_mode %d\n",
		       pdrv->index, __func__, lcd_prbs_mode);
		return;
	}

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	lcd_clk_generate_prbs_clk(pdrv, *encl_clk, bit_rate);
	if (cconf->pll_config[0].done == 0)
		return;
	cconf->data->vclk_sel = 0;
	lcd_clk_set_t6w(pdrv);
	lcd_set_vclk_crt_t6w(pdrv);
========
	*encl_clk = lcd_do_div(bit_rate, 5);
	*fifo_clk = lcd_do_div(bit_rate, 7);
	lcd_clk_generate_prbs_clk(pdrv, *encl_clk, bit_rate);
	if (cconf->pll_config[0].done == 0)
		return;

	lcd_clk_set_t6d(pdrv);
	lcd_set_vclk_crt_t6d(pdrv);
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s ok\n", pdrv->index, __func__);
}

static int lcd_clk_prbs_test_t6w(struct aml_lcd_drv_s *pdrv,
				 unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned int reg_phy_tx_ctrl1;
	int encl_msr_id, fifo_msr_id;
	unsigned int lcd_prbs_mode, lcd_prbs_cnt;
	unsigned int val, timeout;
	unsigned int clk_err_cnt = 0;
	unsigned int lcd_encl_clk_check_std = 0, lcd_fifo_clk_check_std = 0;
	int i, j, bit;
	unsigned int chdig_reg[6] = {
		ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
		ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
		ANACTRL_DIF_PHY_CNTL12, ANACTRL_DIF_PHY_CNTL13,
	};
	unsigned char is_mlvds = pdrv->config.basic.lcd_type == LCD_MLVDS;

	if (!cconf)
		return -1;

	switch (pdrv->index) {
	case 0:
		reg_phy_tx_ctrl1 = ANACTRL_LVDS_TX_PHY_CNTL1;
		break;
	default:
		LCDERR("[%d]: %s: invalid drv_index\n", pdrv->index, __func__);
		return -1;
	}

	encl_msr_id = cconf->data->enc_clk_msr_id;
	fifo_msr_id = cconf->data->fifo_clk_msr_id;
	timeout = (ms > 1000) ? 1000 : ms;
	LCDPR("[%d]: ms:%d, mode_flag:0x%x, timeout:%d\n", pdrv->index, ms, mode_flag, timeout);

	for (i = 0; i < LCD_PRBS_MODE_MAX; i++) {
		if ((mode_flag & (1 << i)) == 0)
			continue;

		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 19, 1);
		lcd_ana_write(reg_phy_tx_ctrl1, 0);

		lcd_prbs_cnt = 0;
		clk_err_cnt = 0;
		lcd_prbs_mode = (1 << i);
		LCDPR("[%d]: lcd_prbs_mode: 0x%x\n", pdrv->index, lcd_prbs_mode);
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
		lcd_prbs_config_clk_t6w(pdrv, lcd_prbs_mode, &lcd_encl_clk_check_std,
========
		lcd_prbs_config_clk_t6d(pdrv, lcd_prbs_mode, &lcd_encl_clk_check_std,
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
					&lcd_fifo_clk_check_std);
		udelay(500);

		for (j = 0; j < 12; j++) {
			bit = j & 0x1 ? 16 : 0;
			lcd_ana_setb(chdig_reg[j >> 1], 3, bit + 6, 2);
			lcd_ana_setb(chdig_reg[j >> 1], 3, bit + 14, 2);
		}

		lcd_ana_write(reg_phy_tx_ctrl1, 0x3000000);
		lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);
		if (lcd_prbs_mode & LCD_PRBS_MODE_LVDS)
			lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 16, 2);
		else if (lcd_prbs_mode & LCD_PRBS_MODE_VX1)
			lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 3, 16, 2);

		while (lcd_prbs_cnt++ < timeout) {
			udelay(1000);
			if (lcd_prbs_clk_check(lcd_encl_clk_check_std, encl_msr_id,
					       lcd_fifo_clk_check_std, fifo_msr_id,
					       lcd_prbs_cnt))
				clk_err_cnt++;
			else
				clk_err_cnt = 0;
			if (clk_err_cnt >= 100) {
				LCDERR("[%d]: prbs check error 1(clkmsr), cnt: %d\n",
				       pdrv->index, lcd_prbs_cnt);
				goto lcd_prbs_test_err_t3;
			}
			val = lcd_ana_getb(reg_phy_tx_ctrl1, 0, 12);
			if (val != 0xfff) {
				LCDERR("[%d]: prbs check error 2(prbs), val:%d\n",
				       pdrv->index, val);
				goto lcd_prbs_test_err_t3;
			}
		}
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
			LCDPR("[%d]: freq %dMHz prbs check ok\n", pdrv->index, lcd_prbs_freq);
		} else {
			LCDPR("[%d]: prbs check: unsupport mode\n", pdrv->index);
		}
		continue;

lcd_prbs_test_err_t3:
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
	for (j = 0; j < 12; j++) {
		bit = j & 0x1 ? 16 : 0;
		lcd_ana_setb(chdig_reg[j >> 1],
			     0x8400 | ((is_mlvds ? 0xff : 0) << 2),
			     bit, 16);
	}
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 0, 19, 1);
	lcd_ana_write(reg_phy_tx_ctrl1, 0);
	lcd_clk_generate_parameter(pdrv);
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	lcd_clk_set_t6w(pdrv);
	lcd_set_vclk_crt_t6w(pdrv);
	lcd_ana_write(reg_phy_tx_ctrl1, 0x03000000);
========
	lcd_clk_set_t6d(pdrv);
	lcd_set_vclk_crt_t6d(pdrv);
	lcd_ana_write(reg_phy_tx_ctrl1, 0xc3000000);
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	lcd_ana_setb(ANACTRL_DIF_PHY_CNTL14, 1, 19, 1);

	return 0;
}

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
========
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

#ifndef CONFIG_AML_LCD_PXP
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 0, 28, 1);  //disable
	lcd_ana_setb(ANACTRL_TCON_PLL0_CNTL0, 1, 30, 1);  //resetn
#endif
}

>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
static void lcd_clk_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	int i;
	unsigned int *table = NULL, size = 0;
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	unsigned int pll_reg_table[] = {
		ANACTRL_TCON_PLL0_CNTL0,
		ANACTRL_TCON_PLL0_CNTL1,
		ANACTRL_TCON_PLL0_CNTL2,
		ANACTRL_TCON_PLL0_CNTL3,
		ANACTRL_TCON_PLL0_CNTL4,
		ANACTRL_TCON_PLL0_STS,
		ANACTRL_VID_PLL_CLK_DIV
	};
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	unsigned int gp2_pll_reg_table[] = {
		ANACTRL_GP2PLL_CTRL0,
		ANACTRL_GP2PLL_CTRL1,
		ANACTRL_GP2PLL_CTRL2,
		ANACTRL_GP2PLL_CTRL3,
	};
========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	unsigned int clk_reg_table[] = {
		CLKCTRL_VIID_CLK0_DIV,
		CLKCTRL_VIID_CLK0_CTRL,
		CLKCTRL_VID_CLK0_CTRL2,
		CLKCTRL_HDMI_VID_PLL_CLK_DIV,
		CLKCTRL_TCON_CLK_CNTL
	};

	table = pll_reg_table;
	size = ARRAY_SIZE(pll_reg_table);
	for (i = 0; i < size; i++)
		printf("pll [0x%08x] = 0x%08x\n", table[i], lcd_ana_read(table[i]));
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	if (cconf && (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL)) {
		table = gp2_pll_reg_table;
		size = ARRAY_SIZE(gp2_pll_reg_table);
		for (i = 0; i < size; i++)
			printf("pll [0x%08x] = 0x%08x\n", table[i], lcd_ana_read(table[i]));
	}
========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	table = clk_reg_table;
	size = ARRAY_SIZE(clk_reg_table);
	for (i = 0; i < size; i++)
		printf("clk [0x%08x] = 0x%08x\n", table[i], lcd_clk_read(table[i]));
}

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
static void lcd_clk_generate_t6w(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;
	if (pdrv->config.basic.lcd_type == LCD_P2P &&
	    pdrv->config.timing.act_timing.clk_mode == LCD_BIT_RATE_ADAPT) {
		LCDPR("[%d]: %s: dual pll config\n", pdrv->index, __func__);
		if (cconf->pll_conf_num < 2) {
			LCDERR("[%d]: %s: clk_conf_num %d error\n",
				pdrv->index, __func__, cconf->pll_conf_num);
			return;
		}
		cconf->pll_mode |= LCD_PLL_MODE_DUAL_PLL;
		cconf->data->vclk_sel = 3;
	} else {
		cconf->pll_mode &= ~LCD_PLL_MODE_DUAL_PLL;
		cconf->data->vclk_sel = 0;
	}

	lcd_clk_generate_dft(pdrv);
}

static struct lcd_pll_data_s lcd_tcon_pll_data_t6w = {
========
static struct lcd_pll_data_s lcd_pll_data_t6d = {
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	.pll_od_fb = 0,
	.pll_0_5_div_en = 0,
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
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	.pll_out_fmax = 4250000000ULL,
	.pll_out_fmin = 5859375,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.div_in_fmax = 4250000000ULL,
	.div_out_fmax = 840000000,
	.div_sel_max = CLK_DIV_SEL_2p25,
};

static struct lcd_pll_data_s lcd_gp2_pll_data_t6w = {
	.pll_od_fb = 0,
	.pll_0_5_div_en = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 3,
	.pll_ref_fmax = 25000000,
	.pll_ref_fmin = 5000000,
	.pll_vco_fmax = 3200000000ULL,
	.pll_vco_fmin = 1600000000ULL,
	.pll_out_fmax = 3200000000ULL,
	.pll_out_fmin = 200000000,
	.od_cnt = 1,
	.have_tcon_div = 0,
	.div_in_fmax = 3200000000ULL,
	.div_out_fmax = 840000000,
	.div_sel_max = CLK_DIV_SEL_1,
};

static struct lcd_clk_data_s lcd_clk_data_t6w = {
	.pll_data[0] = &lcd_tcon_pll_data_t6w,
	.pll_data[1] = &lcd_gp2_pll_data_t6w,

	.xd_out_fmax = 820000000,
	.phy_clk_location = 0,
========
	.pll_out_fmax = 3100000000ULL,
	.pll_out_fmin = 187500000,
	.od_cnt = 3,
	.have_tcon_div = 1,
	.div_in_fmax = 3100000000ULL,
	.div_out_fmax = 750000000,
	.div_sel_max = CLK_DIV_SEL_2p33,
};

static struct lcd_clk_data_s lcd_clk_data_t6d = {
	.pll_data[0] = &lcd_pll_data_t6d,
	.pll_data[1] = NULL,
	.xd_out_fmax = 200000000,
	.phy_clk_location = 0,
	.xd_max = 256,
	.phy_div_max = 256,
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c

	.vclk_sel = 0,
	.enc_clk_msr_id = 222,
	.fifo_clk_msr_id = 86,

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	.xd_max = 256,
	.phy_div_max = 256,

========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	.ss_support = 1,
	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,
<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	.ss_freq_dep_opt = lcd_ss_freq_dep_opt_t6w,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_t6w,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss = lcd_set_pll_ss_t6w,
	.clk_ss_enable = lcd_pll_ss_enable_t6w,
	.pll_frac_set = lcd_pll_frac_set_t6w,
	.clk_set = lcd_clk_set_t6w,
	.vclk_crt_set = lcd_set_vclk_crt_t6w,
	.clk_disable = lcd_clk_disable_t6w,
	.clktree_set = lcd_set_tcon_clk_t6w,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.clk_reg_print = lcd_clk_reg_dump,
	.prbs_test = lcd_clk_prbs_test_t6w,
};

struct lcd_clk_config_s *lcd_clk_config_chip_init_t6w(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = NULL;
	unsigned int size;
	struct lcd_config_s *pconf;
========

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = lcd_pll_frac_generate_dft,
	.set_ss = lcd_set_pll_ss_t6d,
	.clk_ss_enable = lcd_pll_ss_enable_t6d,
	.pll_frac_set = lcd_pll_frac_set_t6d,
	.clk_set = lcd_clk_set_t6d,
	.vclk_crt_set = lcd_set_vclk_crt_t6d,
	.clk_disable = lcd_clk_disable_t6d,
	.clktree_set = lcd_clktree_set_t6d,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = lcd_clk_config_print_dft,
	.clk_reg_print = lcd_clk_reg_dump,
	.prbs_test = lcd_clk_prbs_test_t6d,
};

struct lcd_clk_config_s *lcd_clk_config_chip_init_t6d(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = NULL;
	unsigned int size;
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c

	if (!pdrv)
		return NULL;

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	pconf = &pdrv->config;

========
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
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

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	if (pconf->basic.lcd_type == LCD_P2P)
		cconf->pll_conf_num = 2;
	else
		cconf->pll_conf_num = 1;
========
	cconf->pll_conf_num = 1;
>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	size = cconf->pll_conf_num * sizeof(struct lcd_pll_config_s);
	cconf->pll_config = (struct lcd_pll_config_s *)malloc(size);
	if (!cconf->pll_config) {
		LCDERR("[%d]: %s: Not enough memory for pll config\n", pdrv->index, __func__);
		free(cconf);
		return NULL;
	}
	memset(cconf->pll_config, 0, size);

<<<<<<<< HEAD:drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6w.c
	cconf->data = &lcd_clk_data_t6w;
	cconf->pll_config[0].pll_id = 0;
	if (cconf->pll_conf_num > 1)
		cconf->pll_config[1].pll_id = 1;
========
	cconf->data = &lcd_clk_data_t6d;
	cconf->pll_config[0].pll_id = 0;

>>>>>>>> de100eab9965 (lcd: optimize t6 phy & dphy init flow [1/2]):drivers/amlogic/media/vout/lcd/lcd_clk/lcd_clk_t6d.c
	return cconf;
}

#endif
