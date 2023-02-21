// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

static unsigned int p2p_low_common_phy_preem_tl1[] = {
	0x07,
	0x17,
	0x37,
	0x77,
	0xf7,
	0xff,
};

static unsigned int lvds_vx1_p2p_phy_preem_tl1[] = {
	0x06,
	0x26,
	0x46,
	0x66,
	0x86,
	0xa6,
	0xf6,
};

struct lcd_phy_ctrl_s *lcd_phy_ctrl;

unsigned int lcd_phy_vswing_level_to_value(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int vswing_value = 0;

	vswing_value = level;

	return vswing_value;
}

unsigned int lcd_phy_preem_level_to_value(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int p2p_type, size, preem_value = 0;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
		size = sizeof(lvds_vx1_p2p_phy_preem_tl1) / sizeof(unsigned int);
		if (level >= size) {
			LCDERR("[%d]: %s: level %d invalid\n",
			       pdrv->index, __func__, level);
		} else {
			preem_value = lvds_vx1_p2p_phy_preem_tl1[level];
		}
		break;
	case LCD_P2P:
		p2p_type = pdrv->config.control.p2p_cfg.p2p_type & 0x1f;
		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			size = sizeof(lvds_vx1_p2p_phy_preem_tl1) / sizeof(unsigned int);
			if (level >= size) {
				LCDERR("[%d]: %s: level %d invalid\n",
				pdrv->index, __func__, level);
			} else {
				preem_value = lvds_vx1_p2p_phy_preem_tl1[level];
			}
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			size = sizeof(p2p_low_common_phy_preem_tl1) / sizeof(unsigned int);
			if (level >= size) {
				LCDERR("[%d]: %s: level %d invalid\n",
				pdrv->index, __func__, level);
			} else {
				preem_value = p2p_low_common_phy_preem_tl1[level];
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return preem_value;
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
	case LCD_CHIP_T5:
	case LCD_CHIP_T5D:
		lcd_phy_config_init_t5(pdata);
		break;
	case LCD_CHIP_T5W:
		lcd_phy_config_init_t5w(pdata);
		break;
	case LCD_CHIP_T7:
		lcd_phy_config_init_t7(pdata);
		break;
	case LCD_CHIP_T3:
	case LCD_CHIP_T5M:
		lcd_phy_config_init_t3_t5m(pdata);
		break;
	case LCD_CHIP_C3:
		lcd_phy_config_init_c3(pdata);
		break;
	default:
		break;
	}

	return 0;
}
