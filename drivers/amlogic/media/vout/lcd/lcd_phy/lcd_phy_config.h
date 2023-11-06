/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_PHY_CONFIG_H
#define _AML_LCD_PHY_CONFIG_H
#include <amlogic/media/vout/lcd/lcd_vout.h>

struct lcd_phy_ctrl_s {
	unsigned int lane_lock;
	unsigned int ctrl_bit_on;
	unsigned int (*phy_vswing_level_to_val)(struct aml_lcd_drv_s *pdrv, unsigned int level);
	unsigned int (*phy_amp_dft_val)(struct aml_lcd_drv_s *pdrv);
	unsigned int (*phy_preem_level_to_val)(struct aml_lcd_drv_s *pdrv, unsigned int level);
	void (*phy_set_lvds)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_vx1)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_mlvds)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_p2p)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_mipi)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_edp)(struct aml_lcd_drv_s *pdrv, int status);
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_c3(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_g12a(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_t3_t5m(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_t5(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_t5w(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_t7(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_tl1(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_t3x(struct aml_lcd_data_s *pdata);
struct lcd_phy_ctrl_s *lcd_phy_config_init_txhd2(struct aml_lcd_data_s *pdata);

unsigned int lcd_phy_vswing_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level);
unsigned int lcd_phy_preem_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level);
unsigned int lcd_phy_preem_level_to_value_legacy(struct aml_lcd_drv_s *pdrv, unsigned int level);

#endif
