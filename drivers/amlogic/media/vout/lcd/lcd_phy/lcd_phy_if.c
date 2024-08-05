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

void lcd_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
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
	default:
		break;
	}

	return 0;
}
