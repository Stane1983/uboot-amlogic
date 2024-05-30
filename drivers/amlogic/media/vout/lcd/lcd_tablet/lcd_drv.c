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
#include "lcd_tablet.h"

static int lcd_type_supported(struct lcd_config_s *pconf)
{
	int lcd_type = pconf->basic.lcd_type;
	int ret = -1;

	switch (lcd_type) {
	case LCD_RGB:
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MIPI:
	case LCD_EDP:
	case LCD_BT656:
	case LCD_BT1120:
		ret = 0;
		break;
	default:
		LCDERR("invalid lcd type: %s(%d)\n", lcd_type_type_to_str(lcd_type), lcd_type);
		break;
	}
	return ret;
}

static void lcd_rgb_control_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	//todo
}

static void lcd_bt_control_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int field_type, mode_422, yc_swap, cbcr_swap;

	field_type = pconf->control.bt_cfg.field_type;
	mode_422 = pconf->control.bt_cfg.mode_422;
	yc_swap = pconf->control.bt_cfg.yc_swap;
	cbcr_swap = pconf->control.bt_cfg.cbcr_swap;

	if (on_off) {
		lcd_vcbus_setb(VPU_VOUT_BT_CTRL, field_type, 12, 1);
		lcd_vcbus_setb(VPU_VOUT_BT_CTRL, yc_swap, 0, 1);
		//0:cb first   1:cr first
		lcd_vcbus_setb(VPU_VOUT_BT_CTRL, cbcr_swap, 1, 1);
		//0:left, 1:right, 2:average
		lcd_vcbus_setb(VPU_VOUT_BT_CTRL, mode_422, 2, 2);
	}
}

void lcd_tablet_driver_init_pre(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	LCDPR("[%d]: tablet driver init(ver %s): %s\n", pdrv->index, LCD_DRV_VERSION,
		lcd_type_type_to_str(pdrv->config.basic.lcd_type));
	ret = lcd_type_supported(&pdrv->config);
	if (ret)
		return;

	lcd_set_clk(pdrv);
	lcd_set_venc(pdrv);
}

int lcd_tablet_driver_init(struct aml_lcd_drv_s *pdrv)
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
	case LCD_RGB:
		lcd_rgb_control_set(pdrv, 1);
		lcd_pinmux_set(pdrv, 1);
		break;
	case LCD_BT656:
	case LCD_BT1120:
		lcd_bt_control_set(pdrv, 1);
		lcd_pinmux_set(pdrv, 1);
		break;
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
	case LCD_MIPI:
		lcd_phy_set(pdrv, 1);
		lcd_mipi_dphy_set(pdrv, 1);
		lcd_dsi_tx_ctrl(pdrv, 1);
		break;
	case LCD_EDP:
		lcd_pinmux_set(pdrv, 1);
		lcd_phy_set(pdrv, 1);
		lcd_edp_dphy_set(pdrv, 1);
		edp_tx_ctrl(pdrv, 1);
		break;
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
	return 0;
}

void lcd_tablet_driver_disable(struct aml_lcd_drv_s *pdrv)
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
	case LCD_RGB:
		lcd_pinmux_set(pdrv, 0);
		lcd_rgb_control_set(pdrv, 0);
		break;
	case LCD_BT656:
	case LCD_BT1120:
		lcd_pinmux_set(pdrv, 0);
		lcd_bt_control_set(pdrv, 0);
		break;
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
	case LCD_MIPI:
		lcd_dsi_tx_ctrl(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_mipi_dphy_set(pdrv, 0);
		break;
	case LCD_EDP:
		edp_tx_ctrl(pdrv, 0);
		lcd_edp_dphy_set(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_pinmux_set(pdrv, 0);
		break;
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
}

