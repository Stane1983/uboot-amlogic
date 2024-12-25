// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <string.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPCD_REG.h>
#include "dptx_common.h"

#define VOUT_CONNECTOR_MAX 3

void dptx_act_timing_to_venc_config(struct dptx_drv_s *dptx)
{
	unsigned short h_period, v_period, h_active, v_active;
	unsigned short hsync_bp, hsync_width, vsync_bp, vsync_width;
	unsigned short de_hstart, de_vstart;
	unsigned short hstart, hend, vstart, vend;
	// unsigned short h_delay = 0;

	dptx->venc_cfg.ppc = 1;
	dptx->venc_cfg.enc_clk = dptx->act_timing.pclk / dptx->venc_cfg.ppc;
	if (dptx->venc_cfg.ppc > 1) {
		DPTXPR(dptx->idx, LOG_I, "%s: ppc=%d, pixel_clk=%d, enc_clk=%d\n",
		      __func__, dptx->venc_cfg.ppc, dptx->act_timing.pclk, dptx->venc_cfg.enc_clk);
	}

	h_period    = dptx->act_timing.h_period;
	v_period    = dptx->act_timing.v_period;
	h_active    = dptx->act_timing.h_act;
	v_active    = dptx->act_timing.v_act;
	hsync_bp    = dptx->act_timing.h_bp;
	hsync_width = dptx->act_timing.h_pw;
	vsync_bp    = dptx->act_timing.v_bp;
	vsync_width = dptx->act_timing.v_pw;

	de_hstart = hsync_bp + hsync_width;
	de_vstart = vsync_bp + vsync_width;

	// dptx->venc_cfg.hstart = de_hstart - h_delay;
	dptx->venc_cfg.hstart = de_hstart;
	dptx->venc_cfg.vstart = de_vstart;
	dptx->venc_cfg.hend   = h_active + dptx->venc_cfg.hstart - 1;
	dptx->venc_cfg.vend   = v_active + dptx->venc_cfg.vstart - 1;

	dptx->venc_cfg.de_hs_addr = de_hstart;
	dptx->venc_cfg.de_he_addr = de_hstart + h_active;
	dptx->venc_cfg.de_vs_addr = de_vstart;
	dptx->venc_cfg.de_ve_addr = de_vstart + v_active - 1;

	hstart = (de_hstart + h_period - hsync_bp - hsync_width) % h_period;
	hend   = (de_hstart + h_period - hsync_bp) % h_period;
	dptx->venc_cfg.hs_hs_addr = hstart;
	dptx->venc_cfg.hs_he_addr = hend;
	dptx->venc_cfg.hs_vs_addr = 0;
	dptx->venc_cfg.hs_ve_addr = v_period - 1;

	dptx->venc_cfg.vs_hs_addr = (hstart + h_period) % h_period;
	dptx->venc_cfg.vs_he_addr = dptx->venc_cfg.vs_hs_addr;
	vstart = (de_vstart + v_period - vsync_bp - vsync_width) % v_period;
	vend   = (de_vstart + v_period - vsync_bp) % v_period;
	dptx->venc_cfg.vs_vs_addr = vstart;
	dptx->venc_cfg.vs_ve_addr = vend;

	DPTXPR(dptx->idx, LOG_I,
		"hs_hs=%d hs_he=%d hs_vs=%d hs_ve=%d vs_hs=%d vs_he=%d vs_vs=%d vs_ve=%d",
		dptx->venc_cfg.hs_hs_addr, dptx->venc_cfg.hs_he_addr,
		dptx->venc_cfg.hs_vs_addr, dptx->venc_cfg.hs_ve_addr,
		dptx->venc_cfg.vs_hs_addr, dptx->venc_cfg.vs_he_addr,
		dptx->venc_cfg.vs_vs_addr, dptx->venc_cfg.vs_ve_addr);
}

void __dptx_set_phy_config(struct dptx_drv_s *dptx, uint8_t use_preset)
{
	uint8_t i, data[4];

	for (i = 0; i < 4; i++)
		data[i] = use_preset ? dptx->link_cfg.preset_ds[i] : dptx->link_cfg.curr_ds[i];

	dptx_if_set_phy_cfg(dptx, use_preset);

	dptx_phy_set_lane(dptx, 0xf);

	//write the preset values to the sink device
	data[0] = ds_to_DPCD_LANESET(data[0]);
	data[1] = ds_to_DPCD_LANESET(data[1]);
	data[2] = ds_to_DPCD_LANESET(data[2]);
	data[3] = ds_to_DPCD_LANESET(data[3]);
	if (dptx_if_aux_write(dptx, DPCD_TRAINING_LANE0_SET, 4, data))
		DPTXPR(dptx->idx, LOG_E, "DP sink set phy failed");
}

void __dptx_set_lane_config(struct dptx_drv_s *dptx)
{
	unsigned char auxdata[2];

	DPTXPR(dptx->idx, LOG_I, "%s: %d lane, %u.%u GHz, ss_en: %d, enhanced_frame: %d",
		__func__, dptx->link_cfg.lane_count,
		(dptx->link_cfg.link_rate * 27) / 100, (dptx->link_cfg.link_rate * 27) % 100,
		dptx->link_cfg.down_ss, dptx->link_cfg.enhanced_framing_en);

	dptx_if_set_lane_cfg(dptx);

	// sink Link-rate and Lane_count
	auxdata[0] = dptx->link_cfg.link_rate;  //DPCD_LINK_BANDWIDTH_SET
	//DPCD_LANE_COUNT_SET
	auxdata[1] = dptx->link_cfg.lane_count | dptx->link_cfg.enhanced_framing_en << 7;

	if (dptx_if_aux_write(dptx, DPCD_LINK_BW_SET, 2, auxdata))
		DPTXPR(dptx->idx, LOG_E, "sink set lane rate & count failed");

	auxdata[0] = (dptx->link_cfg.down_ss << 4);
	if (dptx_if_aux_write(dptx, DPCD_DOWNSPREAD_CONTROL, 1, auxdata))
		DPTXPR(dptx->idx, LOG_E, "sink set down-spread failed.");
}

/* ************************************************** *
 * vout server api
 * **************************************************
 */
void dptx_list_support_vmode(struct dptx_drv_s *dptx)
{
	dptx_print_vmode(dptx, 0xff);
}

void dptx_user_set_vmode(struct dptx_drv_s *dptx, uint8_t vmd_idx)
{
	struct dptx_vmode_s *dp_vmode;

	dp_vmode = dptx_get_vmode(dptx, vmd_idx);
	if (!dp_vmode) {
		DPTXPR(dptx->idx, LOG_E, "vmode[%u] not available", vmd_idx);
		return;
	}
	dptx_venc_enable(dptx, 0);

	dptx_vmode_apply_to_act_timing(dptx, dp_vmode);
	dptx_act_timing_apply(dptx);

	__dptx_update_ctrl_bootargs(dptx);

	dptx_set_content_protection(dptx);

	dptx_if_set_MSA(dptx);

	dptx_venc_enable(dptx, 1);

	dptx_if_transmitter_output(dptx, 1);
}
