// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "dsi_common.h"
#include "dsi_ctrl/dsi_ctrl.h"

char *dsi_op_mode_table[] = {
	"Video",
	"Command",
};

char *dsi_video_mode_type_table[] = {
	"Non-Burst mode with sync pulse",
	"Non-Burst mode with sync event",
	"Burst mode",
};

char *dsi_video_data_type_table[] = {
	"COLOR_16BIT_CFG_1",
	"COLOR_16BIT_CFG_2",
	"COLOR_16BIT_CFG_3",
	"COLOR_18BIT_CFG_1",
	"COLOR_18BIT_CFG_2(loosely)",
	"COLOR_24BIT",
	"COLOR_20BIT_LOOSE",
	"COLOR_24_BIT_YCBCR",
	"COLOR_16BIT_YCBCR",
	"COLOR_30BIT",
	"COLOR_36BIT",
	"COLOR_12BIT",
	"COLOR_RGB_111",
	"COLOR_RGB_332",
	"COLOR_RGB_444",
	"un-support type",
};

char *dsi_phy_switch_table[] = {
	"STANDARD",
	"SLOW",
};

static void dsi_base_cfg_print(struct lcd_config_s *pconf)
{
	u32 esc_clk;
	struct dsi_config_s *dconf;

	dconf = &pconf->control.mipi_cfg;
	esc_clk = lcd_do_div(pconf->timing.bit_rate, (8 * dconf->dphy.lp_tesc));

	pr_info("MIPI DSI Config:\n"
		"  lane num:         %u\n"
		"  bit rate max:     %uMHz\n"
		"  bit rate:         %lluHz\n"
		"  lane_byte_clk:    %uhz\n"
		"  operation mode:   init:%s(%u), display:%s(%u)\n"
		"  video mode type:  %s(%d)\n"
		"  clk always hs:    %u\n"
		"  phy switch:       %s\n"
		"  data format:      %s\n"
		"  lp escape clock:  %d.%03dMHz\n\n",
		dconf->lane_num, dconf->bit_rate_max,
		pconf->timing.bit_rate,
		dconf->lane_byte_clk,
		dsi_op_mode_table[dconf->operation_mode_init],
		dconf->operation_mode_init,
		dsi_op_mode_table[dconf->operation_mode_display],
		dconf->operation_mode_display,
		dsi_video_mode_type_table[dconf->video_mode_type],
		dconf->video_mode_type,
		dconf->clk_always_hs,
		dsi_phy_switch_table[STOP_STATE_TO_HS_WAIT_TIME],
		dsi_video_data_type_table[dconf->dpi_data_format],
		(esc_clk / 1000000), (esc_clk % 1000000) / 1000);

	if (!dconf->check_en)
		return;
	pr_info("MIPI DSI check state:\n"
		"  check_reg:             0x%02x\n"
		"  check_cnt:             %d\n"
		"  check_state            %d\n\n",
		dconf->check_reg, dconf->check_cnt, dconf->check_state);
}

static void dsi_dphy_cfg_print(struct lcd_config_s *pconf)
{
	struct dsi_dphy_s *dphy_cfg = &pconf->control.mipi_cfg.dphy;
	u32 UI800;

	UI800 = lcd_do_div(pconf->timing.bit_rate, 1000);
	UI800 = ((1000000 * 100) / UI800) * 8;

	pr_info("MIPI DSI DPHY timing (unit: ns)\n"
		"  UI:          %u.%02u\n"
		"  LP TESC:     %u\n"
		"  LP LPX:      %u\n"
		"  LP TA_SURE:  %u\n"
		"  LP TA_GO:    %u\n"
		"  LP TA_GET:   %u\n"
		"  HS EXIT:     %u\n"
		"  HS TRAIL:    %u\n"
		"  HS ZERO:     %u\n"
		"  HS PREPARE:  %u\n"
		"  CLK TRAIL:   %u\n"
		"  CLK POST:    %u\n"
		"  CLK ZERO:    %u\n"
		"  CLK PREPARE: %u\n"
		"  CLK PRE:     %u\n"
		"  INIT:        %u\n"
		"  WAKEUP:      %u\n\n",
		(UI800 / 8 / 100), ((UI800 / 8) % 100),
		(UI800 * dphy_cfg->lp_tesc / 100),
		(UI800 * dphy_cfg->lp_lpx / 100),
		(UI800 * dphy_cfg->lp_ta_sure / 100),
		(UI800 * dphy_cfg->lp_ta_go / 100),
		(UI800 * dphy_cfg->lp_ta_get / 100),
		(UI800 * dphy_cfg->hs_exit / 100),
		(UI800 * dphy_cfg->hs_trail / 100),
		(UI800 * dphy_cfg->hs_zero / 100),
		(UI800 * dphy_cfg->hs_prepare / 100),
		(UI800 * dphy_cfg->clk_trail / 100),
		(UI800 * dphy_cfg->clk_post / 100),
		(UI800 * dphy_cfg->clk_zero / 100),
		(UI800 * dphy_cfg->clk_prepare / 100),
		(UI800 * dphy_cfg->clk_pre / 100),
		(UI800 * dphy_cfg->init / 100),
		(UI800 * dphy_cfg->wakeup / 100));
}

void dsi_table_print(u8 *dsi_table, u16 n_max)
{
	u16 i = 0, n = 0, j;

	while ((i + 1) < n_max) {
		n = dsi_table[i + 1];
		if (dsi_table[i] == LCD_EXT_CMD_TYPE_END) {
			pr_info("  0x%02x, %d\n", dsi_table[i], dsi_table[i + 1]);
			break;
		} else if ((dsi_table[i] == LCD_EXT_CMD_TYPE_GPIO) ||
			   (dsi_table[i] == LCD_EXT_CMD_TYPE_DELAY)) {
			pr_info("  0x%02x, %d,", dsi_table[i], n);
			for (j = 0; j < n; j++)
				pr_info(" %d,", dsi_table[i + 2 + j]);
			pr_info("\n");
		} else if ((dsi_table[i] & 0xf) == 0x0) {
			pr_info("wrong data_type: 0x%02x\n", dsi_table[i]);
			break;
		}
		pr_info("  0x%02x, %d,", dsi_table[i], n);
		for (j = 0; j < n; j++)
			pr_info(" 0x%02x,", dsi_table[i + 2 + j]);
		pr_info("\n");
		i += (n + 2);
	}
}

void dsi_init_table_print(struct dsi_config_s *dconf)
{
	LCDPR("MIPI DSI init on:\n");
	dsi_table_print(dconf->dsi_init_on, DSI_INIT_ON_MAX);
	LCDPR("MIPI DSI init off:\n");
	dsi_table_print(dconf->dsi_init_off, DSI_INIT_OFF_MAX);
}

static void dsi_extern_init_table_print(struct dsi_config_s *dconf, int on_off)
{
	if (dconf->extern_init != 0xff)
		pr_info("extern init:        %d\n\n", dconf->extern_init);
}

void lcd_dsi_info_print(struct lcd_config_s *pconf)
{
	dsi_base_cfg_print(pconf);
	dsi_dphy_cfg_print(pconf);

	dsi_host_config_print(pconf);

	dsi_init_table_print(&pconf->control.mipi_cfg);
	dsi_extern_init_table_print(&pconf->control.mipi_cfg, 1);
}

void lcd_dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, u8 op_mode)
{
	dsi_op_mode_switch(pdrv, op_mode);
	LCDPR("[%d]: %s: %s(%d)\n", pdrv->index, __func__, dsi_op_mode_table[op_mode], op_mode);
}

void lcd_dsi_write_cmd(struct aml_lcd_drv_s *pdrv, u8 *payload)
{
	dsi_run_oneline_cmd(pdrv, payload, NULL, 0);
}

u8 lcd_dsi_read(struct aml_lcd_drv_s *pdrv, u8 *payload, u8 *rd_data, u8 rd_byte_len)
{
	int dsi_back_len;
	char *string;
	u32 line_start = 0, line_end = 0;
	u8 n, k, is_DCS = payload[0] == DT_DCS_RD_0;

	string = (char *)malloc(256 * sizeof(char));
	if (!string)
		return 0;

	lcd_wait_vsync(pdrv);
	line_start = lcd_get_encl_line_cnt(pdrv);
	dsi_back_len = dsi_read(pdrv, payload, rd_data, rd_byte_len);
	line_end = lcd_get_encl_line_cnt(pdrv);

	n = snprintf(string, 255, "[%d]: encl line[%u, %u] DSI %s read [dt:0x%02x, n:%hu, (",
		pdrv->index, line_start, line_end,
		is_DCS ? "DCS" : "generic", payload[0], payload[1]);

	for (k = 0; k < payload[1]; k++)
		n += snprintf(string + n, 255 - n, "0x%02x ", payload[k + 2]);
	n += (snprintf(string + n - 1, 256 - n, "%s", ")]: ") - 1);

	if (dsi_back_len <= 0) {
		snprintf(string + n, 255 - n, "%s", "failed");
	} else {
		for (k = 0; k < dsi_back_len; k++)
			n += snprintf(string + n, 255 - n, "0x%02x ", rd_data[k]);
	}
	pr_info("%s\n", string);
	free(string);
	return dsi_back_len;
}
