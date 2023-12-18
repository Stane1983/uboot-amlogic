// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include "../../../lcd_common.h"
#include "../dsi_common.h"
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <../../../lcd_reg.h>

int mipi_dsi_check_state(struct aml_lcd_drv_s *pdrv, u8 reg, u8 cnt)
{
	int ret = 0, i;
	u8 *rd_data;
	u8 payload[3] = {DT_GEN_RD_1, 1, 0x04};
	struct dsi_config_s *dconf;
	u32 offset;

	dconf = &pdrv->config.control.mipi_cfg;
	if (dconf->check_en == 0)
		return 0;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	rd_data = (u8 *)malloc(sizeof(u8) * cnt);
	if (!rd_data) {
		LCDERR("[%d]: %s: rd_data malloc error\n", pdrv->index, __func__);
		return 0;
	}

	offset = pdrv->data->offset_venc_if[pdrv->index];

	payload[2] = reg;
	ret = dsi_read(pdrv, payload, rd_data, cnt);
	if (ret < 0)
		goto mipi_dsi_check_state_err;
	if (ret != cnt) {
		LCDERR("[%d]: %s: read back cnt is wrong\n", pdrv->index, __func__);
		goto mipi_dsi_check_state_err;
	}

	pr_info("read reg 0x%02x:", reg);
	for (i = 0; i < ret; i++)
		pr_info(" 0x%02x", rd_data[i]);
	pr_info("\n");

	dconf->check_state = 1;
	lcd_vcbus_setb(L_VCOM_VS_ADDR + offset, 1, 12, 1);
	pdrv->config.retry_enable_flag = 0;
	LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, dconf->check_state);
	free(rd_data);
	return 0;

mipi_dsi_check_state_err:
	dconf->check_state = 0;
	lcd_vcbus_setb(L_VCOM_VS_ADDR + offset, 0, 12, 1);
	LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, dconf->check_state);
	pdrv->config.retry_enable_flag = 1;
	free(rd_data);
	return -1;
}

void mipi_dsi_check_init(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	u32 status;

	/* check_state */
	if (dconf->check_en) {
		status = env_get_ulong("lcd_mipi_check", 10, 0xffff);
		if (status == 0) {
			dconf->check_en = 0;
			LCDPR("lcd_mipi_check disable check_state\n");
		}
	}
	dconf->check_state = 0;
}
