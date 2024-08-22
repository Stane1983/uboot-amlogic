// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#include "dptx_venc.h"
#include "../dptx_regs.h"

static struct dptx_venc_op_s dptx_venc_op;

void dptx_wait_vsync(struct dptx_drv_s *dptx)
{
	if (!dptx_venc_op.wait_vsync || !dptx)
		return;

	dptx_venc_op.wait_vsync(dptx);
}

unsigned int dptx_get_encl_line_cnt(struct dptx_drv_s *dptx)
{
	unsigned int lcnt;

	if (!dptx_venc_op.get_encl_line_cnt || !dptx)
		return 0;

	lcnt = dptx_venc_op.get_encl_line_cnt(dptx);
	return lcnt;
}

unsigned int dptx_get_max_line_cnt(struct dptx_drv_s *dptx)
{
	unsigned int lcnt;

	if (!dptx)
		return 0;
	if (!dptx_venc_op.get_max_lcnt) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid\n", __func__);
		return 0;
	}

	lcnt = dptx_venc_op.get_max_lcnt(dptx);
	return lcnt;
}

void dptx_debug_test(struct dptx_drv_s *dptx, u8 num)
{
	int ret;

	if (!dptx)
		return;
	if (!dptx_venc_op.venc_debug_test) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid\n", __func__);
		return;
	}

	ret = dptx_venc_op.venc_debug_test(dptx, num);
	if (ret) {
		DPTXPR(dptx->idx, LOG_E, "%s: %d not support\n", __func__, num);
		return;
	}

	if (num == 0)
		DPTXPR(dptx->idx, LOG_I, "disable test pattern");
}

/*
 *static void dptx_gamma_init(struct dptx_drv_s *dptx)
 *{
 *	if (!dptx)
 *		return;
 *#ifdef CONFIG_AML_VPP
 *	dptx_wait_vsync(dptx);
 *	vpp_disable_lcd_gamma_table(dptx->idx);
 *
 *	vpp_init_lcd_gamma_table(dptx->idx);
 *
 *	dptx_wait_vsync(dptx);
 *	vpp_enable_lcd_gamma_table(dptx->idx);
 *#endif
 *}
 */

void dptx_set_venc_timing(struct dptx_drv_s *dptx)
{
	if (!dptx)
		return;
	if (!dptx_venc_op.venc_set_timing) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid", __func__);
		return;
	}

	DPTXPR(dptx->idx, LOG_I, "%s", __func__);
	dptx_venc_op.venc_set_timing(dptx);
}

void dptx_set_venc(struct dptx_drv_s *dptx)
{
	if (!dptx)
		return;
	if (!dptx_venc_op.venc_set) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid", __func__);
		return;
	}

	DPTXPR(dptx->idx, LOG_I, "%s", __func__);
	dptx_venc_op.venc_set(dptx);
	// lcd_gamma_init(dptx);
}

void dptx_venc_enable(struct dptx_drv_s *dptx, u8 flag)
{
	if (!dptx)
		return;
	if (!dptx_venc_op.venc_switch) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid", __func__);
		return;
	}

	DPTXPR(dptx->idx, LOG_I, "%s: %d", __func__, flag);
	dptx_venc_op.venc_switch(dptx, flag);
}

void dptx_mute_set(struct dptx_drv_s *dptx, u8 flag)
{
	if (!dptx)
		return;
	if (!dptx_venc_op.mute_set) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid", __func__);
		return;
	}

	DPTXPR(dptx->idx, LOG_I, "%s: %d", __func__, flag);
	dptx_venc_op.mute_set(dptx, flag);
}

int dptx_venc_probe(struct dptx_chip_data_s *dptx_chip_data)
{
	if (!dptx_chip_data)
		return -1;

	switch (dptx_chip_data->chip_type) {
	case DPTX_CHIP_T7:
		dptx_venc_op_init_t7(&dptx_venc_op);
		break;
	default:
		break;
	}

	return 0;
}
