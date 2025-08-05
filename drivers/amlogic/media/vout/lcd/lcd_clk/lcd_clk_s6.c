// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_clk_config.h"
#include "lcd_clk_ctrl.h"
#include "lcd_clk_utils.h"
#include "../connectors/lcd_connector.h"
#include <amlogic/clk_measure.h>

#ifdef CONFIG_MESON_S6
static void lcd_set_pll(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl0;
	int ret, cnt = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl0 = (cconf->pll_config[0].pll_m << 0) | (cconf->pll_config[0].pll_od1_sel << 20);
set_pll_retry:
	lcd_ana_write(ANACTRL_DSIPLL_CTRL0, pll_ctrl0 | 0x00010000);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL0, pll_ctrl0 | 0x10010000);

	lcd_ana_write(ANACTRL_DSIPLL_CTRL1, 0x11480000 | cconf->pll_config[0].pll_frac);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL2, 0x121db010);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL3, 0x00008010);

	lcd_ana_write(ANACTRL_DSIPLL_CTRL1, 0x19480000 | cconf->pll_config[0].pll_frac);

	lcd_ana_write(ANACTRL_DSIPLL_CTRL0, pll_ctrl0 |  0x30010000);

	udelay(10);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL3, 0x20008010);
	udelay(20);

	ret = lcd_pll_wait_lock(cconf->pll_config[0].pll_id, ANACTRL_DSIPLL_CTRL0, 31);
	if (ret) {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry;
		LCDERR("[%d]: pll lock failed\n", pdrv->index);
	}
}

static void lcd_clk_set_s6(struct aml_lcd_drv_s *pdrv)
{
	lcd_set_pll(pdrv);
}

static void lcd_set_vid_pll_div_s6(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int shift_val, shift_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	if (cconf->data->pll_data[0]->div_sel_max == CLK_DIV_SEL_1 ||
	    cconf->pll_config->div_sel > cconf->data->pll_data[0]->div_sel_max ||
	    cconf->pll_config->div_sel >= ARRAY_SIZE(lcd_clk_div_table)) {
		LCDERR("[0]: invalid clk divider\n");
		return;
	}

	shift_val = lcd_clk_div_table[cconf->pll_config->div_sel].shift_val;
	shift_sel = lcd_clk_div_table[cconf->pll_config->div_sel].shift_sel;

	if (shift_val == 0xffff) { /* if divide by 1 */
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, 1, 18, 1);
	} else {
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, 0, 18, 1);
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, shift_sel, 16, 2);
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, 1, 15, 1);
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, shift_val, 0, 15);
		lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, 0, 15, 1);
	}
	/* Enable the final output clock */
	lcd_combo_dphy_setb(CLKCTRL_DSI_PLL_CLK_DIV, 1, 19, 1);
}

static void lcd_set_vclk_crt(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 0, VCLK2_EN, 1);
	udelay(2);

	lcd_set_vid_pll_div_s6(pdrv);

	/* setup the XD divider value */
	lcd_clk_setb(CLKCTRL_VIID_CLK_DIV, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);

	/* select dsi_pll_clk */
	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, cconf->data->vclk_sel, VCLK2_CLK_IN_SEL, 3);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(CLKCTRL_VIID_CLK_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(CLKCTRL_VIID_CLK_DIV, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 1, VCLK2_SOFT_RST, 1);
	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(CLKCTRL_VID_CLK_CTRL2, 1, ENCL_GATE_VCLK, 1);

	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 1, VCLK2_EN, 1);
	udelay(2);
}

static void lcd_clk_disable_s6(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_setb(CLKCTRL_VIID_CLK_CTRL, 0, VCLK2_EN, 1);
	lcd_clk_setb(CLKCTRL_VID_CLK_CTRL2, 0, ENCL_GATE_VCLK, 1);

	lcd_ana_write(ANACTRL_DSIPLL_CTRL0, 0x0);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL1, 0x0);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL2, 0x0);
	lcd_ana_write(ANACTRL_DSIPLL_CTRL3, 0x0);
}

// ANACTRL_DSIPLL_CTRL3
//  [8]    : dsi_pll_ssc_en
//  [13:10]: dsi_pll_ssc_str_m
//  [19:16]: dsi_pll_ssc_dep_sel
//  [23:22]: dsi_pll_ss_mode
static void lcd_pll_ss_enable(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_clk_config_s *cconf;
	unsigned int pll_ctrl3;
	unsigned int flag = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	pll_ctrl3 = lcd_ana_read(ANACTRL_DSIPLL_CTRL3);
	pll_ctrl3 &= ~((1 << 8) | (0xf << 10) | (0xf << 16) | (0x3 << 22));

	if (status && (cconf->ss_level > 0))
		flag = 1;

	if (flag) {
		cconf->ss_en = 1;
		pll_ctrl3 |= ((1 << 8) | (cconf->ss_str_m << 10) |
			      (cconf->ss_dep_sel << 16) | (cconf->ss_mode < 22));
		LCDPR("[%d]: pll ss enable: level: %d, %dppm\n",
		      pdrv->index, cconf->ss_level, cconf->ss_ppm);
	} else {
		cconf->ss_en = 0;
		LCDPR("[%d]: pll ss disable\n", pdrv->index);
	}
	lcd_ana_write(ANACTRL_DSIPLL_CTRL3, pll_ctrl3);
}

static int lcd_prbs_test_s6(struct aml_lcd_drv_s *pdrv, unsigned int ms,
			    unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf;
	unsigned int val, timeout;
	unsigned int cnt = 0;
	unsigned int clk_err_cnt = 0;
	unsigned long lcd_encl_clk_check_std, lcd_byte_clk_check_std;
	int j, ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return -1;

	cconf->pll_config[0].pll_m = 0x68; //2496MHz
	cconf->pll_config[0].pll_od1_sel = 0; //analog div 0
	cconf->pll_config[0].pll_frac = 0;
	cconf->pll_config->div_sel = CLK_DIV_SEL_3;
	cconf->xd = 2;
	lcd_encl_clk_check_std = 416000000;
	lcd_byte_clk_check_std = 312000000;

	cconf->data->vclk_crt_set(pdrv);
	cconf->data->clk_set(pdrv);

	lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0x393b2c55);
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0xc134061f);
	udelay(20);
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0xe134061f);
	udelay(20);
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0x393bcc55);

	timeout = (ms > 1000) ? 1000 : ms;

	udelay(1000);

	while (cnt++ < timeout) {
		udelay(10);
		ret = 1;
		for (j = 0; j < 20; j++) {
			udelay(5);
			val = dsi_phy_getb(pdrv, 0, MIPI_DSI_TEST_CTRL0, 28, 4);
			if (val == 0x0f) {
				ret = 0;
				break;
			}
		}
		if (ret) {
			LCDERR("[%d]: prbs check error, val:0x%x, cnt:%d\n", pdrv->index, val, cnt);
			break;
		}

		if (lcd_prbs_clk_check(lcd_encl_clk_check_std, cconf->data->enc_clk_msr_id,
				       lcd_byte_clk_check_std, cconf->data->fifo_clk_msr_id, cnt))
			clk_err_cnt++;
		else
			clk_err_cnt = 0;
		if (clk_err_cnt >= 10) {
			LCDERR("[%d]: prbs check error (clkmsr), cnt:%d\n", pdrv->index, cnt);
			ret = 1;
			break;
		}
	}

	printf("\n[[%d]: lcd prbs result]:\n", pdrv->index);
	printf("  MIPI-DSI prbs performed: 1, error: %d\n", ret);

	return 0;
}

static void lcd_clk_reg_dump_s6(struct aml_lcd_drv_s *pdrv)
{
	int i;
	unsigned int *table = NULL, size = 0;
	unsigned int pll_reg_table[] = {
		ANACTRL_DSIPLL_CTRL0,
		ANACTRL_DSIPLL_CTRL1,
		ANACTRL_DSIPLL_CTRL2,
		ANACTRL_DSIPLL_CTRL3,
	};
	unsigned int clk_reg_table[] = {
		CLKCTRL_DSI_PLL_CLK_DIV,
		CLKCTRL_VID_CLK_CTRL2,
		CLKCTRL_VIID_CLK_DIV,
		CLKCTRL_VIID_CLK_CTRL,
	};
	unsigned int key_clk_msr_id[] = {29, 30, 53, 70, 71};

	table = pll_reg_table;
	size = ARRAY_SIZE(pll_reg_table);
	for (i = 0; i < size; i++)
		printf("pll [0x%08x] = 0x%08x\n", table[i], lcd_ana_read(table[i]));

	table = clk_reg_table;
	size = ARRAY_SIZE(clk_reg_table);
	for (i = 0; i < size; i++)
		printf("clk [0x%08x] = 0x%08x\n", table[i], lcd_clk_read(table[i]));

	table = key_clk_msr_id;
	size = ARRAY_SIZE(key_clk_msr_id);
	for (i = 0; i < size; i++)
		clk_msr(table[i]);

	printf("combo_dphy [0x%08x] = 0x%08x\n",
		CLKCTRL_DSI_PLL_CLK_DIV, lcd_combo_dphy_read(CLKCTRL_DSI_PLL_CLK_DIV));
}

static struct lcd_pll_data_s lcd_pll_data_s6 = {
	.pll_od_fb = 0,
	.pll_m_max = 511,
	.pll_m_min = 2,
	.pll_n_max = 1,
	.pll_n_min = 1,
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	.pll_od_sel_max = 5,
	.pll_ref_fmax = 24000000,
	.pll_ref_fmin = 24000000,
	.pll_vco_fmax = 2800000000ULL,
	.pll_vco_fmin = 1400000000ULL,
	.pll_out_fmax = 2800000000ULL,
	.pll_out_fmin = 1400000000ULL,
	.od_cnt = 0,
	.have_tcon_div = 0,
	.div_in_fmax = 2800000000ULL,
	.div_out_fmax = 2800000000,
	.div_sel_max = CLK_DIV_SEL_15,
};

static struct lcd_clk_data_s lcd_clk_data_s6 = {
	.pll_data[0] = &lcd_pll_data_s6,
	.pll_data[1] = NULL,
	.xd_out_fmax = 2800000000,
	.phy_clk_location = 1,

	.vclk_sel = 3, // dsi_pll_clk
	.enc_clk_msr_id = 53,
	.fifo_clk_msr_id = 71,

	.xd_max = 128,
	.phy_div_max = 128,

	.ss_support = 1,

	.clk_parameter_init = NULL,
	.clk_generate_parameter = lcd_clk_generate_dft,
	.pll_frac_generate = NULL,
	.set_ss = NULL,
	.clk_ss_enable = lcd_pll_ss_enable,
	.pll_frac_set = NULL,
	.clk_set = lcd_clk_set_s6,
	.vclk_crt_set = lcd_set_vclk_crt,
	.clk_disable = lcd_clk_disable_s6,
	.clktree_set = NULL,
	.clk_config_init_print = lcd_clk_config_init_print_dft,
	.clk_config_print = NULL,
	.clk_reg_print = lcd_clk_reg_dump_s6,
	.prbs_test = lcd_prbs_test_s6,
};

struct lcd_clk_config_s *lcd_clk_config_chip_init_s6(struct aml_lcd_drv_s *pdrv)
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

	cconf->data = &lcd_clk_data_s6;
	cconf->data->pll_data[0] = &lcd_pll_data_s6;
	cconf->pll_config[0].pll_id = 0;
	return cconf;
}
#endif
