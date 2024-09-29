/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_PHY_CONFIG_H
#define _AML_LCD_PHY_CONFIG_H
#include <amlogic/media/vout/lcd/lcd_vout.h>

struct lcd_phy_ctrl_s {
	unsigned int lane_num;
	unsigned int lane_lock_total;
	unsigned int lane_lock[LCD_MAX_DRV];
	unsigned int (*phy_vswing_level_to_val)(struct aml_lcd_drv_s *pdrv, unsigned int level);
	unsigned int (*phy_amp_dft_val)(struct aml_lcd_drv_s *pdrv);
	unsigned int (*phy_preem_level_to_val)(struct aml_lcd_drv_s *pdrv, unsigned int level);
	unsigned char (*phy_lane_phase_sel_def)(struct aml_lcd_drv_s *pdrv, unsigned int lane);
	void (*phy_glb_param_dft_val)(struct aml_lcd_drv_s *pdrv);
	int (*phy_param_get)(struct aml_lcd_drv_s *pdrv,
			     struct phy_config_s *phy_cfg, struct phy_attr_s *phy);
	void (*phy_reg_dump)(struct aml_lcd_drv_s *pdrv);

	void (*phy_set_lvds)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_vx1)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_mlvds)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_p2p)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_mipi)(struct aml_lcd_drv_s *pdrv, int status);
	void (*phy_set_edp)(struct aml_lcd_drv_s *pdrv, int status);
};

#ifdef CONFIG_MESON_T5M
struct lcd_phy_ctrl_s *lcd_phy_config_init_t5m(struct aml_lcd_data_s *pdata);
#endif
#ifdef CONFIG_MESON_T3X
struct lcd_phy_ctrl_s *lcd_phy_config_init_t3x(struct aml_lcd_data_s *pdata);
#endif
#ifdef CONFIG_MESON_TXHD2
struct lcd_phy_ctrl_s *lcd_phy_config_init_txhd2(struct aml_lcd_data_s *pdata);
#endif
#ifdef CONFIG_MESON_S6
struct lcd_phy_ctrl_s *lcd_phy_config_init_s6(struct aml_lcd_data_s *pdata);
#endif
#ifdef CONFIG_MESON_T6D
struct lcd_phy_ctrl_s *lcd_phy_config_init_t6d(struct aml_lcd_data_s *pdata);
#endif

unsigned int lcd_phy_vswing_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level);
unsigned int lcd_phy_preem_level_to_value_dft(struct aml_lcd_drv_s *pdrv, unsigned int level);
unsigned int lcd_phy_amp_dft(struct aml_lcd_drv_s *pdrv);
void lcd_phy_glb_param_dft(struct aml_lcd_drv_s *pdrv);

#endif
