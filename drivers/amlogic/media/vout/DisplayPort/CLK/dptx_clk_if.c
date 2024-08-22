// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "../dptx_common.h"
#include "dptx_clk_ctrl.h"

struct dptx_clk_op_s *dptx_clk_op_s;

void dptx_clk_set_vid_clk(struct dptx_drv_s *dptx, uint32_t pixel_clk)
{
	uint8_t cnt = 0;

	if (dptx_clk_op_s->vid_clk_config)
		dptx_clk_op_s->vid_clk_config(dptx, pixel_clk);
	if (dptx_clk_op_s->link_clk_set)
		dptx_clk_op_s->vid_clk_set(dptx);

	mdelay(1);

	while (dptx_clk_msr_check(dptx->data->venc_clk_msr_id[dptx->idx], pixel_clk)) {
		if (cnt++ >= 10) {
			DPTXPR(dptx->idx, LOG_E, "%s timeout", __func__);
			break;
		}
	}
}

void dptx_clk_set_link_clk(struct dptx_drv_s *dptx, uint8_t dptx_link_rate)
{
	if (dptx_clk_op_s->link_clk_config)
		dptx_clk_op_s->link_clk_config(dptx, dptx_link_rate);

	if (dptx_clk_op_s->clktree_set)
		dptx_clk_op_s->clktree_set(dptx);

	if (dptx_clk_op_s->link_clk_set)
		dptx_clk_op_s->link_clk_set(dptx);
}

void dptx_clk_config_print(struct dptx_drv_s *dptx)
{
	if (dptx_clk_op_s->clk_config_print)
		dptx_clk_op_s->clk_config_print(dptx);
}

void dptx_clk_set_ssc(struct dptx_drv_s *dptx, uint8_t status)
{
}

void dptx_clk_config_probe(struct dptx_drv_s *dptx)
{
	switch (dptx->data->chip_type) {
	case DPTX_CHIP_T7:
		dptx_clk_op_s = dptx_clk_op_init_t7(dptx);
		break;
	default:
		DPTXPR(dptx->idx, LOG_E, "%s: invalid chip type", __func__);
		return;
	}
}
