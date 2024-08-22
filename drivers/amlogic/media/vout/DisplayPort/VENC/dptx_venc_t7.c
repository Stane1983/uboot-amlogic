// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "dptx_venc.h"
#include "../dptx_regs.h"

static void dptx_venc_wait_vsync(struct dptx_drv_s *dptx)
{
	unsigned int offset, reg;
	int line_cnt, line_cnt_previous;
	int i = 0;

	offset = dptx->data->offset_venc[dptx->idx];
	reg = VPU_VENCP_STAT + offset;

	line_cnt = 0x1fff;
	line_cnt_previous = dptx_vcbus_getb(reg, 16, 13);
	while (i++ < LCD_WAIT_VSYNC_TIMEOUT) {
		line_cnt = dptx_vcbus_getb(reg, 16, 13);
		if (line_cnt < line_cnt_previous)
			break;
		line_cnt_previous = line_cnt;
		udelay(1);
	}
	/*DPTXPR(dptx->idx, LOG_I, "line_cnt=%d, line_cnt_previous=%d, i=%d\n",
	 *	line_cnt, line_cnt_previous, i);
	 */
}

static unsigned int dptx_venc_get_max_lint_cnt(struct dptx_drv_s *dptx)
{
	unsigned int line_cnt;

	line_cnt = dptx_vcbus_read(ENCL_VIDEO_MAX_LNCNT + dptx->data->offset_venc[dptx->idx]) + 1;
	/*DPTXPR(dptx->idx, LOG_I, "[%d]: %s: line_cnt=%d", dptx->idx, __func__, line_cnt); */

	return line_cnt;
}

#define LCD_ENC_TST_NUM_MAX    9

struct venc_test_pattern_s {
	char *name;
	uint8_t mode, tst_en, vfifo_en, rgbin;
	uint16_t Y, Cb, Cr;
};

static struct venc_test_pattern_s dptx_enc_tst[] = {
/*       name,          mode, tst_en, vfifo_en, rgbin, Y,     Cb,    Cr,   */
	{"0-None",      0,    0,      1,        3,     0x200, 0x200, 0x200},  /* 0 */
	{"1-Color Bar", 1,    1,      0,        1,     0x200, 0x200, 0x200},  /* 1 */
	{"2-Thin Line", 2,    1,      0,        1,     0x200, 0x200, 0x200},  /* 2 */
	{"3-Dot Grid",  3,    1,      0,        1,     0x200, 0x200, 0x200},  /* 3 */
	{"4-Gray",      0,    1,      0,        3,     0x1ff, 0x1ff, 0x1ff},  /* 4 */
	{"5-Red",       0,    1,      0,        3,     0x3ff,   0x0,   0x0},  /* 5 */
	{"6-Green",     0,    1,      0,        3,       0x0, 0x3ff,   0x0},  /* 6 */
	{"7-Blue",      0,    1,      0,        3,       0x0,   0x0, 0x3ff},  /* 7 */
	{"8-Black",     0,    1,      0,        3,       0x0,   0x0,   0x0},  /* 8 */
	{"9-White",     0,    1,      0,        3,     0x3ff, 0x3ff, 0x3ff},  /* 9 */
};

static int dptx_venc_debug_test(struct dptx_drv_s *dptx, u8 num)
{
	u32 start, width, offset;

	if (num > LCD_ENC_TST_NUM_MAX)
		return -1;

	offset = dptx->data->offset_venc[dptx->idx];
	start = dptx->venc_cfg.hstart;
	width = dptx->act_timing.h_act / 8;

	dptx_venc_wait_vsync(dptx);
	dptx_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, dptx_enc_tst[num].rgbin);
	dptx_vcbus_write(ENCL_TST_MDSEL        + offset, dptx_enc_tst[num].mode);
	dptx_vcbus_write(ENCL_TST_Y            + offset, dptx_enc_tst[num].Y);
	dptx_vcbus_write(ENCL_TST_CB           + offset, dptx_enc_tst[num].Cb);
	dptx_vcbus_write(ENCL_TST_CR           + offset, dptx_enc_tst[num].Cr);
	dptx_vcbus_write(ENCL_TST_CLRBAR_STRT  + offset, start);
	dptx_vcbus_write(ENCL_TST_CLRBAR_WIDTH + offset, width);
	dptx_vcbus_write(ENCL_TST_EN           + offset, dptx_enc_tst[num].tst_en);
	dptx_vcbus_setb(ENCL_VIDEO_MODE_ADV    + offset, dptx_enc_tst[num].vfifo_en, 3, 1);
	if (num > 0)
		DPTXPR(dptx->idx, LOG_I, "show test pattern: %s", dptx_enc_tst[num].name);

	return 0;
}

static void dptx_venc_set_tcon(struct dptx_drv_s *dptx)
{
	unsigned int offset_if, offset_data;

	offset_data   = dptx->data->offset_venc_data[dptx->idx];
	offset_if    = dptx->data->offset_venc_if[dptx->idx];

	dptx_vcbus_write(LCD_RGB_BASE_ADDR  + offset_data, 0x0);
	dptx_vcbus_write(LCD_RGB_COEFF_ADDR + offset_data, 0x400);

	dptx_vcbus_write(LCD_DITH_CNTL_ADDR + offset_data, 0x0);
	dptx_vcbus_setb(LCD_POL_CNTL_ADDR + offset_data, 1, 0, 1);

	/* DE signal */
	dptx_vcbus_write(DE_HS_ADDR + offset_if, dptx->venc_cfg.de_hs_addr);
	dptx_vcbus_write(DE_HE_ADDR + offset_if, dptx->venc_cfg.de_he_addr);
	dptx_vcbus_write(DE_VS_ADDR + offset_if, dptx->venc_cfg.de_vs_addr);
	dptx_vcbus_write(DE_VE_ADDR + offset_if, dptx->venc_cfg.de_ve_addr);
	/* Hsync signal */
	dptx_vcbus_write(HSYNC_HS_ADDR + offset_if, dptx->venc_cfg.hs_hs_addr);
	dptx_vcbus_write(HSYNC_HE_ADDR + offset_if, dptx->venc_cfg.hs_he_addr);
	dptx_vcbus_write(HSYNC_VS_ADDR + offset_if, dptx->venc_cfg.hs_vs_addr);
	dptx_vcbus_write(HSYNC_VE_ADDR + offset_if, dptx->venc_cfg.hs_ve_addr);
	/* Vsync signal */
	dptx_vcbus_write(VSYNC_HS_ADDR + offset_if, dptx->venc_cfg.vs_hs_addr);
	dptx_vcbus_write(VSYNC_HE_ADDR + offset_if, dptx->venc_cfg.vs_he_addr);
	dptx_vcbus_write(VSYNC_VS_ADDR + offset_if, dptx->venc_cfg.vs_vs_addr);
	dptx_vcbus_write(VSYNC_VE_ADDR + offset_if, dptx->venc_cfg.vs_ve_addr);
}

static void dptx_venc_set_timing(struct dptx_drv_s *dptx)
{
	unsigned int offset = dptx->data->offset_venc[dptx->idx];

	dptx_vcbus_write(ENCL_VIDEO_MAX_PXCNT   + offset, dptx->act_timing.h_period - 1);
	dptx_vcbus_write(ENCL_VIDEO_MAX_LNCNT   + offset, dptx->act_timing.v_period - 1);
	dptx_vcbus_write(ENCL_VIDEO_HAVON_BEGIN + offset, dptx->venc_cfg.hstart);
	dptx_vcbus_write(ENCL_VIDEO_HAVON_END   + offset, dptx->venc_cfg.hend);
	dptx_vcbus_write(ENCL_VIDEO_VAVON_BLINE + offset, dptx->venc_cfg.vstart);
	dptx_vcbus_write(ENCL_VIDEO_VAVON_ELINE + offset, dptx->venc_cfg.vend);

	dptx_vcbus_write(ENCL_VIDEO_HSO_BEGIN + offset, dptx->venc_cfg.hs_hs_addr);
	dptx_vcbus_write(ENCL_VIDEO_HSO_END   + offset, dptx->venc_cfg.hs_he_addr);
	dptx_vcbus_write(ENCL_VIDEO_VSO_BEGIN + offset, dptx->venc_cfg.vs_hs_addr);
	dptx_vcbus_write(ENCL_VIDEO_VSO_END   + offset, dptx->venc_cfg.vs_he_addr);
	dptx_vcbus_write(ENCL_VIDEO_VSO_BLINE + offset, dptx->venc_cfg.vs_vs_addr);
	dptx_vcbus_write(ENCL_VIDEO_VSO_ELINE + offset, dptx->venc_cfg.vs_ve_addr);

	switch (dptx->data->chip_type) {
	case DPTX_CHIP_T7:
		dptx_vcbus_write(ENCL_INBUF_CNTL1 + offset, (5 << 13) |
					(dptx->act_timing.h_act - 1));
		dptx_vcbus_write(ENCL_INBUF_CNTL0 + offset, 0x200);
		break;
	default:
		break;
	}

	dptx_venc_set_tcon(dptx);
}

static void dptx_venc_set(struct dptx_drv_s *dptx)
{
	unsigned int reg_disp_viu_ctrl, offset;

	offset = dptx->data->offset_venc[dptx->idx];

	dptx_vcbus_write(ENCL_VIDEO_EN + offset, 0);

	dptx_vcbus_write(ENCL_VIDEO_MODE      + offset, 0x8000); /* bit[15] shadow en */
	dptx_vcbus_write(ENCL_VIDEO_MODE_ADV  + offset, 0x0418); /* Sampling rate: 1 */
	dptx_vcbus_write(ENCL_VIDEO_FILT_CTRL + offset, 0x1000); /* bypass filter */

	dptx_venc_set_timing(dptx);

	dptx_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, 3);
	dptx_vcbus_write(ENCL_VIDEO_EN + offset, 1);

	switch (dptx->idx) {
	case 0:
		reg_disp_viu_ctrl = VPU_DISP_VIU0_CTRL;
		break;
	case 1:
		reg_disp_viu_ctrl = VPU_DISP_VIU1_CTRL;
		break;
	case 2:
		reg_disp_viu_ctrl = VPU_DISP_VIU2_CTRL;
		break;
	default:
		DPTXPR(dptx->idx, LOG_E, "%s: invalid drv_index", __func__);
		return;
	}

	dptx_vcbus_write(reg_disp_viu_ctrl, (0 << 31) | //bit31: lvds enable
					    (0 << 30) | //bit30: vx1 enable
					    (0 << 29) | //bit29: hdmitx enable
					    (1 << 28)); //bit28: dsi_edp enable

	dptx_vcbus_write(VPU_VENC_CTRL + offset, 2);
}

static void dptx_venc_enable_ctrl(struct dptx_drv_s *dptx, unsigned char flag)
{
	dptx_vcbus_write(ENCL_VIDEO_EN + dptx->data->offset_venc[dptx->idx], flag ? 1 : 0);
}

static void dptx_venc_mute_set(struct dptx_drv_s *dptx, unsigned char flag)
{
	unsigned int offset;

	offset = dptx->data->offset_venc[dptx->idx];

	dptx_venc_wait_vsync(dptx);
	if (flag) {
		dptx_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, 3);
		dptx_vcbus_write(ENCL_TST_MDSEL        + offset, 0);
		dptx_vcbus_write(ENCL_TST_Y            + offset, 0);
		dptx_vcbus_write(ENCL_TST_CB           + offset, 0);
		dptx_vcbus_write(ENCL_TST_CR           + offset, 0);
		dptx_vcbus_write(ENCL_TST_EN           + offset, 1);
		dptx_vcbus_setb(ENCL_VIDEO_MODE_ADV    + offset, 0, 3, 1);
	} else {
		dptx_vcbus_setb(ENCL_VIDEO_MODE_ADV    + offset, 1, 3, 1);
		dptx_vcbus_write(ENCL_TST_EN           + offset, 0);
	}
}

static unsigned int dptx_venc_get_encl_line_cnt(struct dptx_drv_s *dptx)
{
	unsigned int cnt;

	cnt = dptx_vcbus_getb(VPU_VENCP_STAT + dptx->data->offset_venc[dptx->idx], 16, 13);
	return cnt;
}

void dptx_venc_op_init_t7(struct dptx_venc_op_s *venc_op)
{
	if (!venc_op)
		return;

	venc_op->wait_vsync        = dptx_venc_wait_vsync;
	venc_op->get_max_lcnt      = dptx_venc_get_max_lint_cnt;
	venc_op->venc_debug_test   = dptx_venc_debug_test;
	venc_op->venc_set_timing   = dptx_venc_set_timing;
	venc_op->venc_set          = dptx_venc_set;
	venc_op->venc_switch       = dptx_venc_enable_ctrl;
	venc_op->mute_set          = dptx_venc_mute_set;
	venc_op->get_encl_line_cnt = dptx_venc_get_encl_line_cnt;
};
