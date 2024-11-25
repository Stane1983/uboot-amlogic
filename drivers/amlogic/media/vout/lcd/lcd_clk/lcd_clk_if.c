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

static char *lcd_ss_freq_table_dft[] = {
	"0, 29.5KHz",
	"1, 31.5KHz",
	"2, 50KHz",
	"3, 75KHz",
	"4, 100KHz",
	"5, 150KHz",
	"6, 200KHz",
};

static char *lcd_ss_mode_table_dft[] = {
	"0, center ss",
	"1, up ss",
	"2, down ss",
};

struct lcd_clk_config_s *get_lcd_clk_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	int i;

	if (!pdrv)
		return NULL;

	if (!pdrv->clk_conf) {
		LCDERR("[%d]: %s: clk_config is null\n", pdrv->index, __func__);
		return NULL;
	}
	cconf = (struct lcd_clk_config_s *)pdrv->clk_conf;
	for (i = 0; i < pdrv->clk_conf_num; i++) {
		if (!cconf[i].data) {
			LCDERR("[%d]: %s: clk config data is null\n",
				pdrv->index, __func__);
			return NULL;
		}
	}

	return cconf;
}

/* ****************************************************
 * lcd clk function api
 * ****************************************************
 */
void lcd_clk_frac_generate(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	/* update bit_rate by interface */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_VBYONE:
		lcd_vbyone_bit_rate_config(pdrv);
		break;
	case LCD_MLVDS:
		lcd_mlvds_bit_rate_config(pdrv);
		break;
	case LCD_P2P:
		lcd_p2p_bit_rate_config(pdrv);
		break;
	case LCD_MIPI:
		lcd_mipi_dsi_bit_rate_config(pdrv);
		break;
	case LCD_EDP:
		lcd_edp_bit_rate_config(pdrv);
	default:
		break;
	}
	if (cconf->data->pll_frac_generate)
		cconf->data->pll_frac_generate(pdrv);
}

static void lcd_bit_rate_match_phy(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy;
	int i = 0;
	unsigned int phy_clk;

	phy_cfg->act_phy = phy_cfg->phys[0];// if not matched, use default
	phy_clk = lcd_do_div(pdrv->config.timing.bit_rate, 1000000);
	for (i = 0; i < phy_cfg->group_num; i++) {
		phy = phy_cfg->phys[i];
		if (phy->phy_clk < phy_clk - 20 || phy->phy_clk > phy_clk + 20)
			continue;

		phy_cfg->act_phy = phy_cfg->phys[i];
		LCDPR("phy_clk=%d, match phy[%d]=%d\n", phy_clk, i, phy->phy_clk);
		return;
	}
	if (phy_cfg->phys[0]->phy_clk)
		LCDPR("no phy_clk matched, use default(phy[0])\n");
}

static void lcd_phy_match_ss(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy;
	struct lcd_clk_config_s *cconf;
	struct lcd_timing_s *tim = &pdrv->config.timing;

	phy = pdrv->config.phy_cfg.act_phy;
	if (!phy)
		return;
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (tim->act_timing.ss_force) {
		tim->ss_freq = tim->act_timing.ss_freq;
		tim->ss_level = tim->act_timing.ss_level;
		tim->ss_mode = tim->act_timing.ss_mode;
	} else {
		tim->ss_freq = phy->ss.freq;
		tim->ss_level = phy->ss.level;
		tim->ss_mode = phy->ss.mode;
	}

	cconf->ss_level = (tim->ss_level >= cconf->data->ss_level_max) ?
				cconf->data->ss_level_max : tim->ss_level;

	cconf->ss_freq = (tim->ss_freq >= cconf->data->ss_freq_max) ?
				cconf->data->ss_freq_max : tim->ss_freq;

	cconf->ss_mode = (tim->ss_mode >= cconf->data->ss_mode_max) ? 0 :
				tim->ss_mode;
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
		LCDPR("[%d]: %s: ss_level=%d, ss_freq=%d, ss_mode=%d\n",
		      pdrv->index, __func__,
		      cconf->ss_level, cconf->ss_freq, cconf->ss_mode);
	}
}

void lcd_clk_generate_parameter(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	/* update bit_rate by interface */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_VBYONE:
		lcd_vbyone_bit_rate_config(pdrv);
		break;
	case LCD_MLVDS:
		lcd_mlvds_bit_rate_config(pdrv);
		break;
	case LCD_P2P:
		lcd_p2p_bit_rate_config(pdrv);
		break;
	case LCD_MIPI:
		lcd_mipi_dsi_bit_rate_config(pdrv);
		break;
	case LCD_EDP:
		lcd_edp_bit_rate_config(pdrv);
	default:
		break;
	}

	if (cconf->data->clk_parameter_init)
		cconf->data->clk_parameter_init(pdrv);
	if (cconf->data->clk_generate_parameter)
		cconf->data->clk_generate_parameter(pdrv);

	lcd_bit_rate_match_phy(pdrv);//bitrate match phy

	lcd_phy_match_ss(pdrv);//phy match ss
}

void lcd_get_ss(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;
	if (cconf->data->ss_support == 0) {
		printf("[%d]: spread spectrum is not support\n", pdrv->index);
		return;
	}

	printf("ss_level: %d, %dppm, dep_sel=%d, str_m=%d\n",
		cconf->ss_level, cconf->ss_ppm,
		cconf->ss_dep_sel, cconf->ss_str_m);
	printf("ss_freq: %d, %s\n", cconf->ss_freq, lcd_ss_freq_table_dft[cconf->ss_freq]);
	printf("ss_mode: %d, %s\n", cconf->ss_mode, lcd_ss_mode_table_dft[cconf->ss_mode]);
}

int lcd_set_ss(struct aml_lcd_drv_s *pdrv, unsigned int level,
	       unsigned int freq, unsigned int mode)
{
	struct lcd_clk_config_s *cconf;
	unsigned int ss_flag = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return -1;
	if (cconf->data->ss_support == 0) {
		LCDERR("[%d]: %s: not support\n", pdrv->index, __func__);
		return -1;
	}

	if (level < 0xff) {
		if (level > cconf->data->ss_level_max) {
			LCDERR("%s: ss_level %d is out of support (max %d)\n",
			       __func__, level, cconf->data->ss_level_max);
			return -1;
		}
		cconf->ss_level = level;
		ss_flag |= LCD_SSC_LEVEL;
	}
	if (freq < 0xff) {
		if (freq > cconf->data->ss_freq_max) {
			LCDERR("%s: ss_freq %d is out of support (max %d)\n",
			       __func__, freq, cconf->data->ss_freq_max);
			return -1;
		}
		cconf->ss_freq = freq;
		ss_flag |= LCD_SSC_FREQ;
	}
	if (mode < 0xff) {
		if (mode > cconf->data->ss_mode_max) {
			LCDERR("%s: ss_mode %d is out of support (max %d)\n",
			       __func__, mode, cconf->data->ss_mode_max);
			return -1;
		}
		cconf->ss_mode = mode;
		ss_flag |= LCD_SSC_MODE;
	}

	if (cconf->data->set_ss && ss_flag)
		cconf->data->set_ss(pdrv, ss_flag);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);
	return 0;
}

/* for frame rate change */
void lcd_update_clk_frac(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (cconf->data->pll_frac_set)
		cconf->data->pll_frac_set(pdrv, cconf->pll_frac);

	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: clk_change=0x%x\n",
			pdrv->index, __func__, pdrv->config.timing.clk_change);
	}
}

/* for timing init */
void lcd_set_clk(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	int cnt = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

#ifdef CONFIG_AML_LCD_PXP
	if (cconf->data->vclk_crt_set)
		cconf->data->vclk_crt_set(pdrv);
	return;
#endif
lcd_set_clk_retry:
	if (cconf->data->clk_set)
		cconf->data->clk_set(pdrv);
	if (cconf->data->vclk_crt_set)
		cconf->data->vclk_crt_set(pdrv);
	mdelay(10);

	while (lcd_clk_msr_check(cconf->data->enc_clk_msr_id, cconf->fout)) {
		if (cnt++ >= 10) {
			LCDERR("[%d]: %s timeout\n", pdrv->index, __func__);
			break;
		}
		goto lcd_set_clk_retry;
	}

	if (cconf->data->clktree_set)
		cconf->data->clktree_set(pdrv);

	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: clk_change=0x%x\n",
			pdrv->index, __func__, pdrv->config.timing.clk_change);
	}
}

void lcd_disable_clk(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (cconf->data->clk_disable)
		cconf->data->clk_disable(pdrv);

	LCDPR("[%d]: %s\n", pdrv->index, __func__);
}

void lcd_clk_config_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (cconf->data->clk_config_print)
		cconf->data->clk_config_print(pdrv);
}

void lcd_clk_reg_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (cconf->data->clk_reg_print)
		cconf->data->clk_reg_print(pdrv);
}

int aml_lcd_prbs_test(struct aml_lcd_drv_s *pdrv, unsigned int ms, unsigned int mode_flag)
{
	struct lcd_clk_config_s *cconf;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return 0;

	if (cconf->data->prbs_test)
		cconf->data->prbs_test(pdrv, ms, mode_flag);
	return 0;
}

static int lcd_clk_config_chip_init(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf)
{
	unsigned int i;

	for (i = 0; i < pdrv->clk_conf_num; i++) {
		cconf[i].pll_id = pdrv->index + i;
		cconf[i].fin = FIN_FREQ;
	}

	switch (pdrv->data->chip_type) {
#ifdef CONFIG_MESON_T5M
	case LCD_CHIP_T5M: //same as t3, but only support 1 driver
		lcd_clk_config_chip_init_t5m(pdrv, cconf);
		break;
#endif
#ifdef CONFIG_MESON_T3X
	case LCD_CHIP_T3X:
		lcd_clk_config_chip_init_t3x(pdrv, cconf);
		break;
#endif
#ifdef CONFIG_MESON_A4
	case LCD_CHIP_A4:
		lcd_clk_config_chip_init_a4(pdrv, cconf);
		break;
#endif
#ifdef CONFIG_MESON_TXHD2
	case LCD_CHIP_TXHD2:
		lcd_clk_config_chip_init_txhd2(pdrv, cconf);
		break;
#endif
#ifdef CONFIG_MESON_S6
	case LCD_CHIP_S6:
		lcd_clk_config_chip_init_s6(pdrv, cconf);
		break;
#endif
#if (IS_ENABLED(CONFIG_MESON_T6D))
	case LCD_CHIP_T6D:
		lcd_clk_config_chip_init_t6d(pdrv, cconf);
		break;
#endif
	default:
		LCDPR("[%d]: %s: invalid chip type\n", pdrv->index, __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		if (cconf->data->clk_config_init_print)
			cconf->data->clk_config_init_print(pdrv);
	}

	return 0;
}

void lcd_clk_config_probe(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned int size;

	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE)
		pdrv->clk_conf_num = 2;
	else
		pdrv->clk_conf_num = 1;
	size = pdrv->clk_conf_num * sizeof(struct lcd_clk_config_s);

	if (!pdrv->clk_conf) {
		cconf = (struct lcd_clk_config_s *)malloc(size);
		if (!cconf) {
			LCDERR("[%d]: %s: Not enough memory\n", pdrv->index, __func__);
			return;
		}
		pdrv->clk_conf = (void *)cconf;
	} else {
		cconf = (struct lcd_clk_config_s *)pdrv->clk_conf;
	}
	memset(cconf, 0, size);

	lcd_clk_config_chip_init(pdrv, cconf);
}
