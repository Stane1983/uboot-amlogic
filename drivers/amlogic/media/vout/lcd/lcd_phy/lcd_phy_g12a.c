// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

#if defined(CONFIG_MESON_G12A) || defined(CONFIG_MESON_G12B) || defined(CONFIG_MESON_SM1)
static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{HHI_MIPI_CNTL0, "PHY_CNTL0"},
		{HHI_MIPI_CNTL1, "PHY_CNTL1"},
		{HHI_MIPI_CNTL2, "PHY_CNTL2"}
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_HHI_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int lane;

	if (status) {
		/* HHI_MIPI_CNTL0 */
		/* DIF_REF_CTL1:31-16bit, DIF_REF_CTL0:15-0bit */
		lcd_hiu_write(HHI_MIPI_CNTL0, 0xa4870008);

		/* HHI_MIPI_CNTL1 */
		/* DIF_REF_CTL2:15-0bit; bandgap bit16 */
		lcd_hiu_write(HHI_MIPI_CNTL1, 0x0001002e);

		/* HHI_MIPI_CNTL2 */
		/* DIF_TX_CTL1:31-16bit, DIF_TX_CTL0:15-0bit */
		lcd_hiu_write(HHI_MIPI_CNTL2, 0x2680045a);

		if (pdrv->config.control.mipi_cfg.lane_num == 1)
			lane = 0x18;
		else if (pdrv->config.control.mipi_cfg.lane_num == 2)
			lane = 0x1c;
		else if (pdrv->config.control.mipi_cfg.lane_num == 3)
			lane = 0x1e;
		else
			lane = 0x1f;

		lcd_hiu_setb(HHI_MIPI_CNTL2, lane, 11, 5);
	} else {
		lcd_hiu_write(HHI_MIPI_CNTL0, 0);
		lcd_hiu_write(HHI_MIPI_CNTL1, 0);
		lcd_hiu_write(HHI_MIPI_CNTL2, 0);
	}
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_g12a = {
	.lane_num = 5,

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

struct lcd_phy_ctrl_s *lcd_phy_config_init_g12a(struct aml_lcd_data_s *pdata)
{
	return &lcd_phy_ctrl_g12a;
}
#endif
