// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#ifdef CONFIG_MESON_C3
static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{ANACTRL_MIPIDSI_CTRL0, "PHY_CNTL0"},
		{ANACTRL_MIPIDSI_CTRL1, "PHY_CNTL1"},
		{ANACTRL_MIPIDSI_CTRL2, "PHY_CNTL2"}
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	if (status) {
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0xa4870008);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0x0001002e);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL2, 0x2680fc59);
	} else {
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0x04070000);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0x2e);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL2, 0x2680045a);
	}
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_c3 = {
	.lane_num = 5,
	.lane_lock = 0,
	.ctrl_bit_on = 1,
	.phy_vswing_level_to_val = NULL,
	.phy_preem_level_to_val = NULL,
	.phy_amp_dft_val = NULL,
	.phy_glb_param_dft_val = NULL,
	.phy_param_get = NULL,
	.phy_reg_dump = lcd_phy_reg_dump,

	.phy_set_lvds = NULL,
	.phy_set_vx1 = NULL,
	.phy_set_mlvds = NULL,
	.phy_set_p2p = NULL,
	.phy_set_mipi = lcd_mipi_phy_set,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_c3(struct aml_lcd_data_s *pdata)
{
	return &lcd_phy_ctrl_c3;
}
#endif
