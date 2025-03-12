// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "dsi_common.h"
#include "dsi_ctrl/dsi_ctrl.h"
#include "../lcd_connector.h"

static void dsi_base_cfg_print(struct lcd_config_s *pconf)
{
	struct dsi_config_s *dconf;

	dconf = &pconf->control.mipi_cfg;

	if (!dconf->check_en)
		return;
	pr_info("MIPI DSI check state:\n"
		"  check_reg:             0x%02x\n"
		"  check_cnt:             %d\n"
		"  check_state            %d\n\n",
		dconf->check_reg, dconf->check_cnt, dconf->check_state);
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
	if (dconf->dsi_init_on) {
		LCDPR("MIPI DSI init on:\n");
		dsi_table_print(dconf->dsi_init_on, DSI_INIT_ON_MAX);
	}
	if (dconf->dsi_init_off) {
		LCDPR("MIPI DSI init off:\n");
		dsi_table_print(dconf->dsi_init_off, DSI_INIT_OFF_MAX);
	}
}

static void dsi_extern_init_table_print(struct dsi_config_s *dconf, int on_off)
{
	if (dconf->extern_init != 0xff)
		pr_info("extern init:        %d\n\n", dconf->extern_init);
}

void lcd_dsi_info_print(struct lcd_config_s *pconf)
{
	dsi_config_print_helper(pconf, 0xff);

	dsi_base_cfg_print(pconf);

	dsi_init_table_print(&pconf->control.mipi_cfg);
	dsi_extern_init_table_print(&pconf->control.mipi_cfg, 1);
}

void lcd_dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, u8 op_mode)
{
	dsi_op_mode_switch(pdrv, 0, op_mode);
	LCDPR("[%d]: %s: %s(%d)\n", pdrv->index, __func__, dsi_op_mode_table[op_mode], op_mode);
}

void lcd_dsi_write_cmd(struct aml_lcd_drv_s *pdrv, u8 *payload)
{
	dsi_run_oneline_cmd(pdrv, 0, payload, NULL, 0);
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

void lcd_dsi_dphy_test(struct aml_lcd_drv_s *pdrv, unsigned char test_item)
{
	switch (test_item) {
	case 0x10: // HS HIGH
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL0, 0x0a600000);
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL1, 0x000003ff);
		break;
	case 0x11: //HS LOW
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL0, 0x08600000);
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL1, 0x000003ff);
		break;
	case 0x12: //HS PRBS7
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL0, 0x0c600000);
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL1, 0x008003ff);
		break;
	case 0x13: //HS PRBS11
	case 0x14: //HS PRBS15
	case 0x00: //LP HIGH
	case 0x01: //LP LOW
		break;
	case 0x02: //LP PRBS7
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL0, 0x0c200000);
		dsi_phy_write(pdrv, 0, MIPI_DSI_TEST_CTRL1, 0x008ffc00);
		break;
	case 0x03: //LP PRBS11
	case 0x04: //LP PRBS15
	default: //LP PRBS15
		break;
	}
}
