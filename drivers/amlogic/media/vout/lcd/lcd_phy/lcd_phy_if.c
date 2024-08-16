// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

static struct lcd_phy_ctrl_s *lcd_phy_ctrl;

int lcd_phy_param_preset(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int amp = 0, preem = 0;
	int i;

#ifdef CONFIG_AML_LCD_PXP
	return 0;
#endif
	if (!lcd_phy_ctrl)
		return -1;

	phy->lane_num = lcd_phy_ctrl->lane_num;
	if (lcd_phy_ctrl->phy_glb_param_dft_val)
		lcd_phy_ctrl->phy_glb_param_dft_val(pdrv);
	if (lcd_phy_ctrl->phy_vswing_level_to_val)
		phy->vswing = lcd_phy_ctrl->phy_vswing_level_to_val(pdrv, phy->vswing_level);
	if (lcd_phy_ctrl->phy_preem_level_to_val)
		preem = lcd_phy_ctrl->phy_preem_level_to_val(pdrv, phy->preem_level);
	if (lcd_phy_ctrl->phy_amp_dft_val)
		amp = lcd_phy_ctrl->phy_amp_dft_val(pdrv);
	for (i = 0; i < phy->lane_num; i++) {
		phy->lane[i].amp = amp;
		phy->lane[i].preem = preem;
		phy->lane[i].sel = i;
	}

	return 0;
}

int lcd_phy_param_get(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy)
{
	int ret;

#ifdef CONFIG_AML_LCD_PXP
	return 0;
#endif
	if (!pdrv || !phy)
		return -1;
	if (!lcd_phy_ctrl || !lcd_phy_ctrl->phy_param_get)
		return -1;

	phy->flag = pdrv->config.phy_cfg.flag;
	phy->lane_num = pdrv->config.phy_cfg.lane_num;
	phy->ch_swap0 = pdrv->config.phy_cfg.ch_swap0;
	phy->ch_swap1 = pdrv->config.phy_cfg.ch_swap1;
	phy->vswing_level = pdrv->config.phy_cfg.vswing_level;
	phy->ext_pullup = pdrv->config.phy_cfg.ext_pullup;
	phy->preem_level = pdrv->config.phy_cfg.preem_level;
	phy->weakly_pull_down = pdrv->config.phy_cfg.weakly_pull_down;
	phy->low_common_mode = pdrv->config.phy_cfg.low_common_mode;
	phy->valid_lane = pdrv->config.phy_cfg.valid_lane;
	ret = lcd_phy_ctrl->phy_param_get(pdrv, phy);
	return ret;
}

void lcd_phy_param_print(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s local_phy, *phy;
	int i, ret;

#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!pdrv)
		return;
	ret = lcd_phy_param_get(pdrv, &local_phy);
	if (ret)
		return;
	lcd_lane_sel_get(pdrv, &local_phy);

	phy = &pdrv->config.phy_cfg;
	printf("vswing  = 0x%x(0x%x)\n"
		"odt     = 0x%x(0x%x)\n"
		"vcm     = 0x%x(0x%x)\n"
		"cv_mode = %d(%d)\n"
		"ref_bias= %d(%d)\n",
		phy->vswing, local_phy.vswing,
		phy->odt, local_phy.odt,
		phy->vcm, local_phy.vcm,
		phy->cv_mode, local_phy.cv_mode,
		phy->ref_bias, local_phy.ref_bias);
	printf("  lane  sel       amp       preem\n");
	for (i = 0; i < local_phy.lane_num; i++) {
		printf("  [%2d]: 0x%x(0x%x), 0x%x(0x%x), 0x%x(0x%x)\n",
		       i, phy->lane[i].sel, local_phy.lane[i].sel,
		       phy->lane[i].amp, local_phy.lane[i].amp,
		       phy->lane[i].preem, local_phy.lane[i].preem);
	}
	printf("flag=0x%x, lane_num=%d, valid_lane=0x%x, ",
	       phy->flag, phy->lane_num, phy->valid_lane);
	printf("ch_swap0=0x%x, ch_swap1=0x%x, ckdi=0x%x\n",
	       phy->ch_swap0, phy->ch_swap1, phy->ckdi);
}

void lcd_phy_analog_reg_print(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!pdrv)
		return;
	if (!lcd_phy_ctrl || !lcd_phy_ctrl->phy_reg_dump)
		return;

	printf("\nphy analog regs:\n");
	lcd_phy_ctrl->phy_reg_dump(pdrv);
}

void lcd_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!pdrv->phy_set) {
		LCDPR("[%d]: %s: phy_set is null\n", pdrv->index, __func__);
		return;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %d, flag=0x%x\n",
		      pdrv->index, __func__, status, pdrv->config.phy_cfg.flag);
	}
	pdrv->phy_set(pdrv, status);
}

int lcd_phy_probe(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	pdrv->phy_set = NULL;
	return 0;
#endif
	if (!lcd_phy_ctrl)
		return 0;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_lvds;
		break;
	case LCD_VBYONE:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_vx1;
		break;
	case LCD_MLVDS:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_mlvds;
		break;
	case LCD_P2P:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_p2p;
		break;
	case LCD_MIPI:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_mipi;
		break;
	case LCD_EDP:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_edp;
		break;
	default:
		pdrv->phy_set = NULL;
		break;
	}

	return 0;
}

int lcd_phy_config_init(struct aml_lcd_data_s *pdata)
{
	lcd_phy_ctrl = NULL;

	switch (pdata->chip_type) {
#ifdef CONFIG_MESON_T5M
	case LCD_CHIP_T5M:
		lcd_phy_ctrl = lcd_phy_config_init_t5m(pdata);
		break;
#endif
#ifdef CONFIG_MESON_T3X
	case LCD_CHIP_T3X:
		lcd_phy_ctrl = lcd_phy_config_init_t3x(pdata);
		break;
#endif
#ifdef CONFIG_MESON_TXHD2
	case LCD_CHIP_TXHD2:
		lcd_phy_ctrl = lcd_phy_config_init_txhd2(pdata);
		break;
#endif
#ifdef CONFIG_MESON_S6
	case LCD_CHIP_S6:
		lcd_phy_ctrl = lcd_phy_config_init_s6(pdata);
		break;
#endif
#if (IS_ENABLED(CONFIG_MESON_T6D))
	case LCD_CHIP_T6D:
		lcd_phy_ctrl = lcd_phy_config_init_t6d(pdata);
		break;
#endif
	default:
		break;
	}

	return 0;
}
