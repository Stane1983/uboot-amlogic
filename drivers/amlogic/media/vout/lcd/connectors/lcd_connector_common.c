// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <malloc.h>
// #include <asm/arch/io.h>
#include <amlogic/media/vpp/vpp.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "./lcd_connector.h"

static int lcd_type_supported(struct aml_lcd_drv_s *pdrv)
{
	switch (pdrv->config.basic.lcd_type) {
#if defined(CONFIG_AML_LCD_LVDS)
	case LCD_LVDS:
		return 0;
#endif
#if defined(CONFIG_AML_LCD_DPI)
	case LCD_RGB:
	case LCD_BT656:
	case LCD_BT1120:
		return 0;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	case LCD_VBYONE:
		return 0;
#endif
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
	case LCD_P2P:
		return 0;
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	case LCD_MIPI:
		return 0;
#endif
	default:
		break;
	}
	LCDERR("[%d]: invalid lcd type: %s(%d)", pdrv->index,
		lcd_type_type_to_str(pdrv->config.basic.lcd_type), pdrv->config.basic.lcd_type);
	return 1;
}

void lcd_connector_driver_init_pre(struct aml_lcd_drv_s *pdrv)
{
	if (lcd_type_supported(pdrv))
		return;

	lcd_set_clk(pdrv);
	lcd_set_venc(pdrv);
}

void lcd_connector_driver_init(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	return;
#endif
	if (lcd_type_supported(pdrv))
		return;

	/* init driver */
	switch (pdrv->config.basic.lcd_type) {
#if defined(CONFIG_AML_LCD_DPI)
	case LCD_RGB:
		lcd_rgb_control_set(pdrv, 1);
		lcd_rgb_pinmux_set(pdrv, 1);
		break;
	case LCD_BT656:
	case LCD_BT1120:
		lcd_bt_control_set(pdrv, 1);
		lcd_bt_pinmux_set(pdrv, 1);
		break;
#endif
#if defined(CONFIG_AML_LCD_LVDS)
	case LCD_LVDS:
		lcd_lvds_dphy_set(pdrv, 1);
		lcd_lvds_enable(pdrv);
		lcd_phy_set(pdrv, 1);
		break;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	case LCD_VBYONE:
		lcd_vbyone_pinmux_set(pdrv, 1);
		lcd_vbyone_dphy_set(pdrv, 1);
		lcd_vbyone_enable(pdrv);
		lcd_vbyone_wait_hpd(pdrv);
		lcd_phy_set(pdrv, 1);
		lcd_vbyone_wait_stable(pdrv);
		break;
#endif
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_tcon_top_init(pdrv);
		lcd_mlvds_pinmux_set(pdrv, 1);
		lcd_mlvds_dphy_set(pdrv, 1);
		lcd_tcon_enable(pdrv);
		lcd_phy_set(pdrv, 1);
		break;
	case LCD_P2P:
		lcd_tcon_top_init(pdrv);
		lcd_p2p_pinmux_set(pdrv, 1);
		lcd_phy_set(pdrv, 1);
		lcd_p2p_dphy_set(pdrv, 1);
		lcd_tcon_enable(pdrv);
		break;
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	case LCD_MIPI:
		lcd_phy_set(pdrv, 1);
		lcd_mipi_dphy_set(pdrv, 1);
		lcd_dsi_tx_ctrl(pdrv, 1);
		break;
#endif
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
}

void lcd_connector_driver_disable(struct aml_lcd_drv_s *pdrv)
{
	LCDPR("[%d]: disable driver\n", pdrv->index);

#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	return;
#endif
	if (lcd_type_supported(pdrv))
		return;

	switch (pdrv->config.basic.lcd_type) {
#if defined(CONFIG_AML_LCD_DPI)
	case LCD_RGB:
		lcd_rgb_pinmux_set(pdrv, 0);
		lcd_rgb_control_set(pdrv, 0);
		break;
	case LCD_BT656:
	case LCD_BT1120:
		lcd_bt_pinmux_set(pdrv, 0);
		lcd_bt_control_set(pdrv, 0);
		break;
#endif
#if defined(CONFIG_AML_LCD_LVDS)
	case LCD_LVDS:
		lcd_phy_set(pdrv, 0);
		lcd_lvds_disable(pdrv);
		lcd_lvds_dphy_set(pdrv, 0);
		break;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	case LCD_VBYONE:
		lcd_phy_set(pdrv, 0);
		lcd_vbyone_pinmux_set(pdrv, 0);
		lcd_vbyone_disable(pdrv);
		lcd_vbyone_dphy_set(pdrv, 0);
		break;
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	case LCD_MIPI:
		lcd_dsi_tx_ctrl(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_mipi_dphy_set(pdrv, 0);
		break;
#endif
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_tcon_disable(pdrv);
		lcd_mlvds_dphy_set(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_mlvds_pinmux_set(pdrv, 0);
		break;
	case LCD_P2P:
		lcd_tcon_disable(pdrv);
		lcd_p2p_dphy_set(pdrv, 0);
		lcd_phy_set(pdrv, 0);
		lcd_p2p_pinmux_set(pdrv, 0);
		break;
#endif
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s finished\n", pdrv->index, __func__);
}
