// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include <amlogic/aml_efuse.h>

#ifdef CONFIG_MESON_S6

#define DSI_PHY_LPRX_HI   1   // CH0 LPRX hi : 0=0.82v 1=0.86v 2=0.89v 3=0.93v
#define DSI_PHY_LPRX_LOW  1   // CH0 LPRX low: 0=0.52v 1=0.56v 2=0.60v 3=0.64v
#define DSI_PHY_LPCD_HI   1   // CH0 LPCD hi : 0=0.37v 1=0.41v 2=0.45v 3=0.49v
#define DSI_PHY_LPCD_LOW  1   // CH0 LPCD low: 0=0.17v 1=0.20v 2=0.24v 3=0.27v

//0.4V setting: 0=0.375v 1=0.4v 2=0.425v 3=0.45v 4=0.475v 5=0.5v 6=0.525v 7=0.551v
#define DSI_PHY_DATA_HS_400mV    1
#define DSI_PHY_CLK_HS_400mV     1
//1.2V setting: 0=1.11v 1=1.13v 2=1.15v 3=1.18v 4=1.21v 5=1.23v 6=1.26v 7=1.28v
#define DSI_PHY_DATA_LP_1200mV   4

static void lcd_phy_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{ANACTRL_MIPIDSI_CTRL0,  "MIPIDSI_CTRL0"},
		{ANACTRL_MIPIDSI_CTRL1,  "MIPIDSI_CTRL1"},
	};

	str_add_reg_sets(pdrv, LCD_REG_DBG_ANA_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}

static void lcd_mipi_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int mipi_dsi_ctl0, mipi_dsi_ctl1, mipi_dsi_ctl2, mipi_dsi_ctl3;
	unsigned short CHCK_HS_driver_ability;
	int HSTX_50R_calid_val;

	if (status == 0) {
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, 0);
		lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, 0);
		return;
	}

	// 1.5G: PHY_CNTL0[0xfe0083d4] = 0x393b8055; PHY_CNTL1[0xfe0083d8] = 0xe134031f;
	// 2.5G: PHY_CNTL0[0xfe0083d4] = 0x393b8055; PHY_CNTL1[0xfe0083d8] = 0xe134061f;

	CHCK_HS_driver_ability = pdrv->config.control.mipi_cfg.bit_rate_max > 1500 ? 0x6 : 0x3;
	HSTX_50R_calid_val = efuse_get_cali_item("dsi");
	if (HSTX_50R_calid_val < 0) {
		LCDERR("[%u]: DSI HSTX 50Ω resistance uncalibrated\n", pdrv->index);
		HSTX_50R_calid_val = 0xb;
	}

	mipi_dsi_ctl0 = (0x0 << 15) | //serlizer reset
			(0x0 << 14) | // ccp reset;
			(0x0 << 13) | // input data select: 0=external input; 1=internal prbs7;
			(0x0 << 12) | // force output "1" data: 0=work; 1=FT triming 50Ω resistance
			(0x0 << 11) | (0x0 << 10) | // PRBS7 check en; PRBS7 generate en;
			(0x0 << 9)  | // CH0 LPCD reference low voltage: 0=REFL 0.6v 1=ULV 0.3v
			(0x0 << 8)  | // CH0 LPCD/LPRX reference voltage: 0=vbg 1=avdd18
			(DSI_PHY_LPRX_HI  << 6)  | // LPRX hi : 00=0.82v 01=0.86v 10=0.89v 11=0.93v
			(DSI_PHY_LPRX_LOW << 4)  | // LPRX low: 00=0.52v 01=0.56v 10=0.60v 11=0.64v
			(DSI_PHY_LPCD_HI  << 2)  | // LPCD hi : 00=0.37v 01=0.41v 10=0.45v 11=0.49v
			(DSI_PHY_LPCD_LOW << 0);   // LPCD low: 00=0.17v 01=0.20v 10=0.24v 11=0.27v
	mipi_dsi_ctl1 = (0x0 << 14) | // CH 0/1/2/3 hstx slew setting (2bit)
			(0x7 << 11) | // CHCK/CH 0/1/2/3/ lptx slew setting (3bit)
			(0x1 << 8)  | // CH0/1/2/3 hstx post setting (3bit)
			(0x3 << 4)  | // CH0/1/2/3 hstx driver ability setting (3bit)
			(HSTX_50R_calid_val << 0); // hstx 50Ω resistance calibration setting(4bit)
	mipi_dsi_ctl2 = (0x0 << 15) | // LPRX ref voltage: 1=0.74V 0=0.88V
			(0x0 << 12) | // CHCK hstx post setting (3bit)
			(0x0 << 11) | // CHCK half clk inversion: 1=inversion 0=no inversion
			(CHCK_HS_driver_ability << 8)  | // CHCK hstx driver ability setting (3bit)
			(0x0 << 7)  | // CHCK input full clk inversion: 0=inversion 1=no inversion
			(0x0 << 5)  | // CHCK HSTX slew setting (2bit)
			(0x1 << 4)  | // CHCK channel enable
			(0x1 << 3)  | (0x1 << 2) | (0x1 << 1) | (0x1 << 0); // CH 0/1/2/3 enable
	mipi_dsi_ctl3 = (0x1 << 15) | // full clk enable
			(0x1 << 14) | // full clk buffer inversion;
			(0x0 << 13) | // full clk half divider reset
			(0x0 << 12) | // LP 1.2v reference voltage select: 1=avdd18 0=vbg
			(0x0 << 11) | // HS 0.4v reference voltage select: 1=avdd18 0=vbg
			(DSI_PHY_DATA_HS_400mV << 8) | // CH0/1/2/3 0.4V setting
			(DSI_PHY_CLK_HS_400mV  << 5) | // CHCK 0.4V setting
			(0x1 << 4)  | (0x0 << 3) | // vbg enable; vbg select: 1=avdd18  0=vbg
			(DSI_PHY_DATA_LP_1200mV << 0); // LP 1.2V setting;

	lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, mipi_dsi_ctl1 << 16 | mipi_dsi_ctl0);
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, mipi_dsi_ctl3 << 16 | mipi_dsi_ctl2);

	udelay(20);
	mipi_dsi_ctl3 |= (0x1 << 13); //full clk half divider reset
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL1, mipi_dsi_ctl3 << 16 | mipi_dsi_ctl2);
	udelay(20);
	mipi_dsi_ctl0 |= (0x1 << 15); //serlizer reset
	lcd_ana_write(ANACTRL_MIPIDSI_CTRL0, mipi_dsi_ctl1 << 16 | mipi_dsi_ctl0);
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_s6 = {
	.lane_num = 5,

	.phy_vswing_level_to_val = NULL,
	.phy_preem_level_to_val = NULL,
	.phy_amp_dft_val = NULL,
	.phy_lane_phase_sel_def = NULL,
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

struct lcd_phy_ctrl_s *lcd_phy_config_init_s6(struct aml_lcd_data_s *pdata)
{
	return &lcd_phy_ctrl_s6;
}
#endif
