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
	printf("MIPI DSI check state:\n"
		"  check_reg:             0x%02x\n"
		"  check_cnt:             %d\n"
		"  check_state            %d\n\n",
		dconf->check_reg, dconf->check_cnt, dconf->check_state);
}

void dsi_table_print(uint8_t *dsi_table, uint16_t n_max)
{
	uint16_t i = 0, n = 0, j;
	if (!dsi_table)
		return;

	while ((i + 1) < n_max) {
		n = dsi_table[i + 1];
		if (dsi_table[i] == LCD_EXT_CMD_TYPE_END) {
			printf("  0x%02x, %d\n", dsi_table[i], dsi_table[i + 1]);
			break;
		} else if ((dsi_table[i] == LCD_EXT_CMD_TYPE_GPIO) ||
			   (dsi_table[i] == LCD_EXT_CMD_TYPE_DELAY)) {
			printf("  0x%02x, %d,", dsi_table[i], n);
			for (j = 0; j < n; j++)
				printf(" %d,", dsi_table[i + 2 + j]);
			printf("\n");
		} else if ((dsi_table[i] & 0xf) == 0x0) {
			printf("wrong data_type: 0x%02x\n", dsi_table[i]);
			break;
		}
		printf("  0x%02x, %d,", dsi_table[i], n);
		for (j = 0; j < n; j++)
			printf(" 0x%02x,", dsi_table[i + 2 + j]);
		printf("\n");
		i += (n + 2);
	}
}

void dsi_init_table_print(struct dsi_config_s *dconf)
{
	LCDPR("MIPI DSI init-on: %s\n", dconf->dsi_init_on == NULL ? "NULL" : "");
	dsi_table_print(dconf->dsi_init_on, DSI_INIT_ON_MAX);

	LCDPR("MIPI DSI suspend: %s\n", dconf->dsi_suspend == NULL ? "NULL" : "");
	dsi_table_print(dconf->dsi_suspend, DSI_SUSPEND_MAX);

	LCDPR("MIPI DSI resume: %s\n", dconf->dsi_resume == NULL ? "NULL" : "");
	dsi_table_print(dconf->dsi_resume, DSI_RESUME_MAX);

	LCDPR("MIPI DSI init-off: %s\n", dconf->dsi_init_off == NULL ? "NULL" : "");
	dsi_table_print(dconf->dsi_init_off, DSI_INIT_OFF_MAX);
}

#ifdef TRY_TO_REMOVE_DSI_EXTERN
static void dsi_extern_init_table_print(struct dsi_config_s *dconf, int on_off)
{
	if (dconf->extern_init != 0xff)
		printf("extern init:        %d\n\n", dconf->extern_init);
}
#endif

void lcd_dsi_info_print(struct lcd_config_s *pconf)
{
	dsi_config_print_helper(pconf, 0xff);

	dsi_base_cfg_print(pconf);

	dsi_init_table_print(&pconf->control.mipi_cfg);

#ifdef TRY_TO_REMOVE_DSI_EXTERN
	dsi_extern_init_table_print(&pconf->control.mipi_cfg, 1);
#endif
}

void lcd_dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, uint8_t op_mode)
{
	dsi_op_mode_switch(pdrv, 0, op_mode);
	LCDPR("[%d]: %s: %s(%d)\n", pdrv->index, __func__, dsi_op_mode_table[op_mode], op_mode);
}

void lcd_dsi_write_cmd(struct aml_lcd_drv_s *pdrv, uint8_t *payload)
{
	dsi_run_oneline_cmd(pdrv, 0, payload, NULL, 0);
}

uint8_t lcd_dsi_read(struct aml_lcd_drv_s *pdrv, uint8_t *payload,
			uint8_t *rd_data, uint8_t rd_byte_len)
{
	int dsi_back_len;
	char *string;
	uint32_t line_start = 0, line_end = 0;
	uint8_t n, k, is_DCS = payload[0] == DT_DCS_RD_0;

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
	printf("%s\n", string);
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
