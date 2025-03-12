/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _LCD_TX_CONNECTOR_
#define _LCD_TX_CONNECTOR_

#include <amlogic/media/vout/lcd/aml_lcd.h>

/* MIPI_DPI */
void lcd_rgb_control_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
void lcd_bt_control_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);

/* lvds */
void lcd_lvds_enable(struct aml_lcd_drv_s *pdrv);
void lcd_lvds_disable(struct aml_lcd_drv_s *pdrv);

/* vbyone */
#ifdef CONFIG_AML_LCD_VBYONE
void lcd_vbyone_enable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_disable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_wait_hpd(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_wait_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_rst(struct aml_lcd_drv_s *pdrv);
int lcd_vbyone_cdr(struct aml_lcd_drv_s *pdrv);
int lcd_vbyone_lock(struct aml_lcd_drv_s *pdrv);
#endif

/* tcon */
#ifdef CONFIG_AML_LCD_TCON
void lcd_tcon_info_print(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_top_init(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_enable(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_disable(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_global_reset(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_dbg_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming);
int lcd_tcon_probe(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id);
int lcd_tcon_is_dccd_flow(void);
#endif

/* MIPI_DSI */
#ifdef CONFIG_AML_LCD_MIPI_DSI
/* @dsi_common.c */
void lcd_dsi_init_table_load_dts(char *dtaddr, int offset, struct dsi_config_s *dconf);
void lcd_dsi_init_table_load_bsp(struct dsi_config_s *dconf);
void lcd_dsi_tx_ctrl(struct aml_lcd_drv_s *pdrv, unsigned char en);
unsigned long long lcd_dsi_get_min_bitrate(struct aml_lcd_drv_s *pdrv);
/* @dsi_debug.c */
void lcd_dsi_info_print(struct lcd_config_s *pconf);
void lcd_dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, unsigned char op_mode);
void lcd_dsi_dphy_test(struct aml_lcd_drv_s *pdrv, unsigned char test_item);
void lcd_dsi_write_cmd(struct aml_lcd_drv_s *pdrv, unsigned char *payload);
unsigned char lcd_dsi_read(struct aml_lcd_drv_s *pdrv,
			unsigned char *payload, unsigned char *rd_data, unsigned char rd_byte_len);
/* @dsi_addons/dsi_check_panel.c */
int mipi_dsi_check_state(struct aml_lcd_drv_s *pdrv, unsigned char reg, unsigned char cnt);

#endif

#endif