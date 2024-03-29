// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#include <amlogic/media/vpp/vpp.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_tv.h"

static int lcd_type_supported(struct lcd_config_s *pconf)
{
	int lcd_type = pconf->basic.lcd_type;
	int ret = -1;

	switch (lcd_type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
	case LCD_P2P:
		ret = 0;
		break;
	default:
		LCDERR("invalid lcd type: %s(%d)\n",
			lcd_type_type_to_str(lcd_type), lcd_type);
		break;
	}
	return ret;
}

void lcd_tv_driver_init_pre(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	LCDPR("[%d]: tv driver init(ver %s): %s\n",
		pdrv->index,
		LCD_DRV_VERSION,
		lcd_type_type_to_str(pdrv->config.basic.lcd_type));
	ret = lcd_type_supported(&pdrv->config);
	if (ret)
		return;

	lcd_set_clk(pdrv);
	lcd_set_venc(pdrv);
}

int lcd_tv_driver_init(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_type_supported(&pdrv->config);
	if (ret)
		return -1;

#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	return 0;
#endif

	/* init driver */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_lvds_dphy_set(pdrv, 1);
		lcd_lvds_enable(pdrv);
		lcd_phy_set(pdrv, 1);
		break;
	case LCD_VBYONE:
		lcd_pinmux_set(pdrv, 1);
		lcd_vbyone_dphy_set(pdrv, 1);
		lcd_vbyone_enable(pdrv);
		lcd_vbyone_wait_hpd(pdrv);
		lcd_phy_set(pdrv, 1);
		lcd_vbyone_wait_stable(pdrv);
		break;
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_tcon_top_init(pdrv);
		lcd_pinmux_set(pdrv, 1);
		lcd_mlvds_dphy_set(pdrv, 1);
		lcd_tcon_enable(pdrv);
		lcd_phy_set(pdrv, 1);
		break;
	case LCD_P2P:
		lcd_tcon_top_init(pdrv);
		lcd_pinmux_set(pdrv, 1);
		lcd_phy_set(pdrv, 1);
		lcd_p2p_dphy_set(pdrv, 1);
		lcd_tcon_enable(pdrv);
		break;
#endif
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
	return 0;
}

void lcd_tv_driver_disable(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	LCDPR("[%d]: disable driver\n", pdrv->index);
	ret = lcd_type_supported(&pdrv->config);
	if (ret)
		return;

#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	return;
#endif

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_phy_set(pdrv, 0);
		lcd_lvds_disable(pdrv);
		lcd_lvds_dphy_set(pdrv, 0);
		break;
	case LCD_VBYONE:
		lcd_phy_set(pdrv, 0);
		lcd_pinmux_set(pdrv, 0);
		lcd_vbyone_disable(pdrv);
		lcd_vbyone_dphy_set(pdrv, 0);
		break;
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_tcon_disable(pdrv);
		lcd_mlvds_dphy_set(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_pinmux_set(pdrv, 0);
		break;
	case LCD_P2P:
		lcd_tcon_disable(pdrv);
		lcd_p2p_dphy_set(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_pinmux_set(pdrv, 0);
		break;
#endif
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
}

