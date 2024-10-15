// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

unsigned int lcd_phy_vswing_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	unsigned int vswing_value = 0;

	vswing_value = (level >= 0xf) ? 0xf : level;

	return vswing_value;
}

unsigned int lcd_phy_preem_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	unsigned int phy_vmode_preem[6] = {0x07, 0x17, 0x37, 0x77, 0xf7, 0xff};
	unsigned int phy_cmode_preem[7] = {0x06, 0x26, 0x46, 0x66, 0x86, 0xa6, 0xf6};

	if (phy->cv_mode == PHY_VMODE) {
		if (level < 6)
			return phy_vmode_preem[level];
	} else { //default cmode
		if (level < 7)
			return phy_cmode_preem[level];
	}

	LCDERR("[%d]: %s: level %d invalid\n", pdrv->index, __func__, level);
	return 0;
}

unsigned int lcd_phy_amp_dft(struct aml_lcd_drv_s *pdrv)
{
	return 0x7;
}

void lcd_phy_glb_param_dft(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = phy_cfg->act_phy;

	//phys[0]:base phy config, copy to phys[N] before phys[N] parse
	if (!phy)
		return;

	phy->ref_bias = 0;
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		phy->vcm = 0x27e;
		phy->odt = 0xff;
		phy->cv_mode = PHY_CMODE;
		break;
	case LCD_VBYONE:
		if (phy_cfg->ext_pullup) {
			phy->vcm = 0x27e;
			phy->odt = 0xff;
		} else {
			phy->vcm = 0x27a;
			phy->odt = 0xf0;
		}
		phy->cv_mode = PHY_CMODE;
		break;
	case LCD_MLVDS:
		phy->vcm = 0x27e;
		phy->odt = 0xff;
		phy->cv_mode = PHY_CMODE;
		break;
	case LCD_P2P:
		switch (pdrv->config.control.p2p_cfg.p2p_type & 0x1f) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			phy->vcm = 0x27a;
			phy->odt = 0xff;
			phy->cv_mode = PHY_CMODE;
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			phy->vcm = 0x027;
			if ((pdrv->config.control.p2p_cfg.p2p_type >> 5) & 0x1)
				phy->odt = 0xe0; /* 580mV */
			else
				phy->odt = 0xfe; /* default 385mV */
			phy->cv_mode = PHY_VMODE;
			break;
		default:
			return;
		}
		break;
	default:
		break;
	}
}
