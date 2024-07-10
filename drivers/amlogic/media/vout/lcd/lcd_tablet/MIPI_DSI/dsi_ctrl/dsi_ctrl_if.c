// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../../lcd_common.h"
#include "../dsi_common.h"
#include "dsi_ctrl.h"

static struct dsi_ctrl_s *dsi_ctrl_op;

void dsi_tx_ready(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->tx_ready) {
		LCDERR("[%d]: %s not supported\n", pdrv->index, __func__);
		return;
	}
	dsi_ctrl_op->tx_ready(pdrv);
}

void dsi_disp_on(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->disp_on) {
		LCDERR("[%d]: %s not supported\n", pdrv->index, __func__);
		return;
	}
	dsi_ctrl_op->disp_on(pdrv);
}

void dsi_disp_off(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->disp_off) {
		LCDERR("[%d]: %s not supported\n", pdrv->index, __func__);
		return;
	}
	dsi_ctrl_op->disp_off(pdrv);
}

void dsi_tx_close(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->tx_close) {
		LCDERR("[%d]: %s not supported\n", pdrv->index, __func__);
		return;
	}
	dsi_ctrl_op->tx_close(pdrv);
}

void dsi_fr_change_pre(struct aml_lcd_drv_s *pdrv, u16 fr100)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->fr_change_pre)
		return;

	dsi_ctrl_op->fr_change_pre(pdrv, fr100);
}

void dsi_fr_change_post(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->fr_change_post)
		return;

	dsi_ctrl_op->fr_change_post(pdrv);
}

void dsi_host_config_print(struct lcd_config_s *pconf)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->host_config_print)
		return;

	dsi_ctrl_op->host_config_print(pconf);
}

int dsi_DT_generic_short_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_generic_short_write)
		return -1;

	return dsi_ctrl_op->DT_generic_short_write(pdrv, req);
}

int dsi_DT_generic_read(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_generic_read)
		return -1;

	return dsi_ctrl_op->DT_generic_read(pdrv, req);
}

int dsi_DT_DCS_short_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_DCS_short_write)
		return -1;

	return dsi_ctrl_op->DT_DCS_short_write(pdrv, req);
}

int dsi_DT_DCS_read(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_DCS_read)
		return -1;

	return dsi_ctrl_op->DT_DCS_read(pdrv, req);
}

int dsi_DT_set_max_return_pkt_size(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_set_max_return_pkt_size)
		return -1;

	return dsi_ctrl_op->DT_set_max_return_pkt_size(pdrv, req);
}

int dsi_DT_generic_long_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_generic_long_write)
		return -1;

	return dsi_ctrl_op->DT_generic_long_write(pdrv, req);
}

int dsi_DT_DCS_long_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_DCS_long_write)
		return -1;

	return dsi_ctrl_op->DT_DCS_long_write(pdrv, req);
}

void dsi_DT_sink_shut_down(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_sink_shut_down)
		return;

	dsi_ctrl_op->DT_sink_shut_down(pdrv);
}

void dsi_DT_sink_turn_on(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->DT_sink_turn_on)
		return;

	dsi_ctrl_op->DT_sink_turn_on(pdrv);
}

void dsi_op_mode_switch(struct aml_lcd_drv_s *pdrv, u8 op_mode)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->op_mode_switch)
		return;

	dsi_ctrl_op->op_mode_switch(pdrv, op_mode);
}

void dsi_dphy_reset(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->dphy_reset)
		return;

	dsi_ctrl_op->dphy_reset(pdrv);
}

void dsi_host_reset(struct aml_lcd_drv_s *pdrv)
{
	if (!dsi_ctrl_op || !dsi_ctrl_op->host_reset)
		return;

	dsi_ctrl_op->host_reset(pdrv);
}

void lcd_dsi_if_bind(struct aml_lcd_drv_s *pdrv)
{
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_MAX:
		dsi_ctrl_op = dsi_bind_v2(pdrv);
		break;
	case LCD_CHIP_S6:
	default:
		dsi_ctrl_op = dsi_bind_v1(pdrv);
		break;
	}
}
