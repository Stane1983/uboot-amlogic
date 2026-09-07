// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
// #include <asm/arch/io.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_venc.h"

#if defined (CONFIG_MESON_T5M) || defined(CONFIG_MESON_T6D) || defined (CONFIG_MESON_T6W)
static void lcd_venc_wait_vsync(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;
	int line_cnt, line_cnt_previous;
	int i = 0;

	reg = VPU_VENCP_STAT;

	line_cnt = 0x1fff;
	line_cnt_previous = lcd_vcbus_getb(reg, 16, 13);
	while (i++ < LCD_WAIT_VSYNC_TIMEOUT) {
		line_cnt = lcd_vcbus_getb(reg, 16, 13);
		if (line_cnt < line_cnt_previous)
			break;
		line_cnt_previous = line_cnt;
		udelay(2);
	}
	/*LCDPR("line_cnt=%d, line_cnt_previous=%d, i=%d\n",
	 *	line_cnt, line_cnt_previous, i);
	 */
}

static unsigned int lcd_venc_get_max_lint_cnt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int line_cnt;

	line_cnt = lcd_vcbus_read(ENCL_VIDEO_MAX_LNCNT) + 1;
	/*LCDPR("[%d]: %s: line_cnt=%d", pdrv->index, __func__, line_cnt); */

	return line_cnt;
}

struct lcd_enc_test_t {
	char *name;
	unsigned int mode;
	unsigned int y;
	unsigned int cb;
	unsigned int cr;
	unsigned int en;
	unsigned int vfifo_en;
	unsigned int rgb_in;
};

static struct lcd_enc_test_t lcd_enc_tst_comm[] = {
	{"0-None",         0,    0x200,   0x200, 0x200, 0, 1, 3},  /* 0 */
	{"1-Color Bar",    1,    0x200,   0x200, 0x200, 1, 0, 1},  /* 1 */
	{"2-Thin Line",    2,    0x200,   0x200, 0x200, 1, 0, 1},  /* 2 */
	{"3-Dot Grid",     3,    0x200,   0x200, 0x200, 1, 0, 1},  /* 3 */
	{"4-Gray",         0,    0x1ff,   0x1ff, 0x1ff, 1, 0, 3},  /* 4 */
	{"5-Red",          0,    0x3ff,     0x0,   0x0, 1, 0, 3},  /* 5 */
	{"6-Green",        0,      0x0,   0x3ff,   0x0, 1, 0, 3},  /* 6 */
	{"7-Blue",         0,      0x0,     0x0, 0x3ff, 1, 0, 3},  /* 7 */
	{"8-Black",        0,      0x0,     0x0,   0x0, 1, 0, 3},  /* 8 */
};

static struct lcd_enc_test_t lcd_enc_tst_t6d[] = {
	{"0-None",         0,    0x200,   0x200, 0x200, 0, 1, 3},  /* 0 */
	{"1-Color Bar",    1,    0x200,   0x200, 0x200, 1, 0, 1},  /* 1 */
	{"2-Thin Line",    2,    0x200,   0x200, 0x200, 1, 0, 1},  /* 2 */
	{"3-Dot Grid",     3,    0x200,   0x200, 0x200, 1, 0, 1},  /* 3 */
	{"4-Gray",         0,    0x1ff,   0x1ff, 0x1ff, 1, 0, 3},  /* 4 */
	{"5-Red",          0,    0x3ff,     0x0,   0x0, 1, 0, 3},  /* 5 */
	{"6-Green",        0,      0x0,   0x3ff,   0x0, 1, 0, 3},  /* 6 */
	{"7-Blue",         0,      0x0,     0x0, 0x3ff, 1, 0, 3},  /* 7 */
	{"8-Black",        0,      0x0,     0x0,   0x0, 1, 0, 3},  /* 8 */
	{"9-Not support",  0,    0x200,   0x200, 0x200, 0, 1, 3},  /* 9 */
	{"10-Gray Scale",  5, 0xffffffff,  0x7,   0x7, 1, 0, 3},  /* 10 */
	{"11-Red Scale",   5,      0x1,     0x0,   0x1, 1, 0, 3},  /* 11 */
	{"12-Green Scale", 5,      0x1,     0x0,   0x2, 1, 0, 3},  /* 12 */
	{"13-Blue Scale",  5,      0x1,     0x0,   0x4, 1, 0, 3},  /* 13 */
};

static struct lcd_enc_test_t lcd_enc_tst_t6w[] = {
	{"0-None",         0,    0x200,   0x200, 0x200, 0, 1, 3},  /* 0 */
	{"1-Color Bar",    1,    0x200,   0x200, 0x200, 1, 0, 1},  /* 1 */
	{"2-Thin Line",    2,    0x200,   0x200, 0x200, 1, 0, 1},  /* 2 */
	{"3-Dot Grid",     3,    0x200,   0x200, 0x200, 1, 0, 1},  /* 3 */
	{"4-Gray",         0,    0x1ff,   0x1ff, 0x1ff, 1, 0, 3},  /* 4 */
	{"5-Red",          0,    0x3ff,     0x0,   0x0, 1, 0, 3},  /* 5 */
	{"6-Green",        0,      0x0,   0x3ff,   0x0, 1, 0, 3},  /* 6 */
	{"7-Blue",         0,      0x0,     0x0, 0x3ff, 1, 0, 3},  /* 7 */
	{"8-Black",        0,      0x0,     0x0,   0x0, 1, 0, 3},  /* 8 */
	{"9-X icon",       4,    0x3ff,    0x10,   0x9, 1, 0, 2},  /* 9 */
	{"10-Gray Scale",   5, 0xffffffff,  0x7,   0x7, 1, 0, 3},  /* 10 */
	{"11-Red Scale",   5,      0x1,     0x0,   0x1, 1, 0, 3},  /* 11 */
	{"12-Green Scale", 5,      0x1,     0x0,   0x2, 1, 0, 3},  /* 12 */
	{"13-Blue Scale",  5,      0x1,     0x0,   0x4, 1, 0, 3},  /* 13 */
};

static int lcd_venc_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num)
{
	unsigned int start, width, height;
	struct lcd_enc_test_t *pcur_test = NULL;
	unsigned int cur_test_num = 0;
	unsigned gcd_num;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T6D:
		pcur_test = lcd_enc_tst_t6d;
		cur_test_num = ARRAY_SIZE(lcd_enc_tst_t6d);
		break;
	case LCD_CHIP_T6W:
		pcur_test = lcd_enc_tst_t6w;
		cur_test_num = ARRAY_SIZE(lcd_enc_tst_t6w);
		break;
	default:
		pcur_test = lcd_enc_tst_comm;
		cur_test_num = ARRAY_SIZE(lcd_enc_tst_comm);
		break;
	}

	if (num >= cur_test_num || !pcur_test)
		return -1;

	start = pdrv->config.timing.hstart;
	width = pdrv->config.timing.act_timing.h_active / 9;

	lcd_venc_wait_vsync(pdrv);
	lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, pcur_test[num].rgb_in);
	lcd_vcbus_write(ENCL_TST_Y, pcur_test[num].y);
	if (num == 9) {
		width = pdrv->config.timing.act_timing.h_active;
		height = pdrv->config.timing.act_timing.v_active;
		gcd_num = gcd(width, height);
		lcd_vcbus_write(ENCL_TST_CB, width / gcd_num);
		lcd_vcbus_write(ENCL_TST_CR, height / gcd_num);
		lcd_vcbus_write(ENCL_TST_CLRBAR_STRT, height);
		lcd_vcbus_write(ENCL_TST_CLRBAR_WIDTH, width);
	} else {
		lcd_vcbus_write(ENCL_TST_CB, pcur_test[num].cb);
		lcd_vcbus_write(ENCL_TST_CR, pcur_test[num].cr);
		lcd_vcbus_write(ENCL_TST_CLRBAR_STRT, start - 2);
		lcd_vcbus_write(ENCL_TST_CLRBAR_WIDTH, width);
	}
	lcd_vcbus_write(ENCL_TST_MDSEL, pcur_test[num].mode);
	lcd_vcbus_setb(ENCL_TST_EN, pcur_test[num].en, 0, 1);
	lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, pcur_test[num].vfifo_en, 3, 1);
	if (num > 0) {
		LCDPR("[%d]: show test pattern: %s\n",
		      pdrv->index, pcur_test[num].name);
	}

	return 0;
}

static void lcd_venc_set_tcon(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;

	lcd_vcbus_write(LCD_RGB_BASE_ADDR, 0x0);
	lcd_vcbus_write(LCD_RGB_COEFF_ADDR, 0x400);

	if (pconf->basic.lcd_type != LCD_P2P && pconf->basic.lcd_type != LCD_MLVDS) {
		switch (pconf->timing.act_timing.lcd_bits) {
		case 18:
			lcd_vcbus_write(LCD_DITH_CNTL_ADDR,  0x600);
			break;
		case 24:
			lcd_vcbus_write(LCD_DITH_CNTL_ADDR,  0x400);
			break;
		case 30:
		default:
			lcd_vcbus_write(LCD_DITH_CNTL_ADDR,  0x0);
			break;
		}
	} else {
		lcd_vcbus_write(LCD_DITH_CNTL_ADDR,  0x0);
	}

	switch (pconf->basic.lcd_type) {
	case LCD_LVDS:
		lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 1, 0, 3);
		// refs to lcd_lvds.c@lcd_lvds_enable
		if (pconf->timing.act_timing.vsync_pol == pconf->timing.act_timing.hsync_pol)
			lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 1, 1, 1);
		break;
	case LCD_VBYONE:
		if (pconf->timing.act_timing.hsync_pol)
			lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 1, 0, 1);
		if (pconf->timing.act_timing.vsync_pol)
			lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 1, 1, 1);
		break;
	case LCD_MIPI:
		//lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 0x3, 0, 2);
		/*lcd_vcbus_write(LCD_POL_CNTL_ADDR,
		 *	(lcd_vcbus_read(LCD_POL_CNTL_ADDR) |
		 *	 ((0 << 2) | (vs_pol_adj << 1) | (hs_pol_adj << 0))));
		 */
		/*lcd_vcbus_write(LCD_POL_CNTL_ADDR, (lcd_vcbus_read(LCD_POL_CNTL_ADDR) |
		 *	 ((1 << LCD_TCON_DE_SEL) | (1 << LCD_TCON_VS_SEL) |
		 *	  (1 << LCD_TCON_HS_SEL))));
		 */
		break;
	case LCD_EDP:
		lcd_vcbus_setb(LCD_POL_CNTL_ADDR, 1, 0, 1);
		break;
	default:
		break;
	}

	/* DE signal */
	lcd_vcbus_write(DE_HS_ADDR,    pconf->timing.de_hs_addr);
	lcd_vcbus_write(DE_HE_ADDR,    pconf->timing.de_he_addr);
	lcd_vcbus_write(DE_VS_ADDR,    pconf->timing.de_vs_addr);
	lcd_vcbus_write(DE_VE_ADDR,    pconf->timing.de_ve_addr);

	/* Hsync signal */
	lcd_vcbus_write(HSYNC_HS_ADDR, pconf->timing.hs_hs_addr);
	lcd_vcbus_write(HSYNC_HE_ADDR, pconf->timing.hs_he_addr);
	lcd_vcbus_write(HSYNC_VS_ADDR, pconf->timing.hs_vs_addr);
	lcd_vcbus_write(HSYNC_VE_ADDR, pconf->timing.hs_ve_addr);

	/* Vsync signal */
	lcd_vcbus_write(VSYNC_HS_ADDR, pconf->timing.vs_hs_addr);
	lcd_vcbus_write(VSYNC_HE_ADDR, pconf->timing.vs_he_addr);
	lcd_vcbus_write(VSYNC_VS_ADDR, pconf->timing.vs_vs_addr);
	lcd_vcbus_write(VSYNC_VE_ADDR, pconf->timing.vs_ve_addr);
}

static void lcd_venc_set_timing(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int hstart, hend, vstart, vend;
	unsigned int pre_hde, pre_vde, pre_de_vs, pre_de_ve, pre_de_hs, pre_de_he;

	hstart = pconf->timing.hstart;
	hend = pconf->timing.hend;
	vstart = pconf->timing.vstart;
	vend = pconf->timing.vend;

	lcd_vcbus_write(ENCL_VIDEO_MAX_PXCNT, pconf->timing.act_timing.h_period - 1);
	lcd_vcbus_write(ENCL_VIDEO_MAX_LNCNT, pconf->timing.act_timing.v_period - 1);
	lcd_vcbus_write(ENCL_VIDEO_HAVON_BEGIN, hstart);
	lcd_vcbus_write(ENCL_VIDEO_HAVON_END,   hend);
	lcd_vcbus_write(ENCL_VIDEO_VAVON_BLINE, vstart);
	lcd_vcbus_write(ENCL_VIDEO_VAVON_ELINE, vend);
	if (pconf->basic.lcd_type == LCD_P2P ||
	    pconf->basic.lcd_type == LCD_MLVDS) {
		pre_vde = pconf->timing.pre_de_v ? pconf->timing.pre_de_v : 8;
		pre_hde = pconf->timing.pre_de_h ? pconf->timing.pre_de_h : PRE_DE_DELAY;
		pre_de_vs = pconf->timing.vstart - pre_vde;
		pre_de_ve = pconf->timing.act_timing.v_active + pre_de_vs;
		pre_de_hs = pconf->timing.hstart + pre_hde;
		pre_de_he = pconf->timing.act_timing.h_active - 1 + pre_de_hs;
		lcd_vcbus_write(ENCL_VIDEO_V_PRE_DE_BLINE, pre_de_vs);
		lcd_vcbus_write(ENCL_VIDEO_V_PRE_DE_ELINE, pre_de_ve);
		lcd_vcbus_write(ENCL_VIDEO_H_PRE_DE_BEGIN, pre_de_hs);
		lcd_vcbus_write(ENCL_VIDEO_H_PRE_DE_END,   pre_de_he);
	}

	lcd_vcbus_write(ENCL_VIDEO_HSO_BEGIN, pconf->timing.hs_hs_addr);
	lcd_vcbus_write(ENCL_VIDEO_HSO_END,   pconf->timing.hs_he_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_BEGIN, pconf->timing.vs_hs_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_END,   pconf->timing.vs_he_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_BLINE, pconf->timing.vs_vs_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_ELINE, pconf->timing.vs_ve_addr);

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
		lcd_vcbus_write(ENCL_INBUF_CNTL1,
				(5 << 13) | (pconf->timing.act_timing.h_active - 1));
		lcd_vcbus_write(ENCL_INBUF_CNTL0, 0x200);
		break;
	case LCD_CHIP_T3:
	case LCD_CHIP_T5W:
	case LCD_CHIP_T5M:
	case LCD_CHIP_T6D:
	case LCD_CHIP_T6W:
		lcd_vcbus_write(ENCL_INBUF_CNTL1,
				(4 << 13) | (pconf->timing.act_timing.h_active - 1));
		lcd_vcbus_write(ENCL_INBUF_CNTL0, 0x200);
		break;
	default:
		break;
	}

	lcd_venc_set_tcon(pdrv);
}

static void lcd_venc_set(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg_disp_viu_ctrl;

	lcd_vcbus_write(ENCL_VIDEO_EN, 0);

	lcd_vcbus_write(ENCL_VIDEO_MODE, 0x8000); /* bit[15] shadown en */
	lcd_vcbus_write(ENCL_VIDEO_MODE_ADV, 0x0418); /* Sampling rate: 1 */
	lcd_vcbus_write(ENCL_VIDEO_FILT_CTRL, 0x1000); /* bypass filter */
	if (pdrv->data->chip_type >= LCD_CHIP_T6D) {
		lcd_vcbus_setb(ENCL_TST_EN, 3, 1, 2); /*vsync latch enable*/
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, 1, 8, 1); /*vfifo_en latch enable*/
	}
	lcd_venc_set_timing(pdrv);

	lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, 3);

	lcd_vcbus_write(ENCL_VIDEO_EN, 1);

	switch (pdrv->index) {
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
		LCDERR("[%d]: %s: invalid drv_index\n",
			pdrv->index, __func__);
		return;
	}

	/*
	 * bit31: lvds enable
	 * bit30: vx1 enable
	 * bit29: hdmitx enable
	 * bit28: dsi_edp enable
	 */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_vcbus_write(reg_disp_viu_ctrl, (1 << 31) |
						(0 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_VBYONE:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(1 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_MIPI:
	case LCD_EDP:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(0 << 30) |
						(0 << 29) |
						(1 << 28));
		break;
	default:
		break;
	}
	lcd_vcbus_write(VPU_VENC_CTRL, 2);
}

static void lcd_venc_enable_ctrl(struct aml_lcd_drv_s *pdrv, int flag)
{
	if (flag)
		lcd_vcbus_write(ENCL_VIDEO_EN, 1);
	else
		lcd_vcbus_write(ENCL_VIDEO_EN, 0);
}

/*static void lcd_venc_mute_set(struct aml_lcd_drv_s *pdrv, unsigned char flag)
 *{
 *	lcd_venc_wait_vsync(pdrv);
 *	if (flag) {
 *		lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL, 3);
 *		lcd_vcbus_write(ENCL_TST_MDSEL, 0);
 *		lcd_vcbus_write(ENCL_TST_Y, 0);
 *		lcd_vcbus_write(ENCL_TST_CB, 0);
 *		lcd_vcbus_write(ENCL_TST_CR, 0);
 *		lcd_vcbus_setb(ENCL_TST_EN, 1, 0, 1);
 *		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, 0, 3, 1);
 *	} else {
 *		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV, 1, 3, 1);
 *		lcd_vcbus_setb(ENCL_TST_EN, 0, 0, 1);
 *	}
 *}
 */

static unsigned int lcd_venc_get_encl_line_cnt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int cnt;

	cnt = lcd_vcbus_getb(VPU_VENCP_STAT, 16, 13);
	return cnt;
}

static void lcd_venc_reg_dump(struct aml_lcd_drv_s *pdrv)
{
	int i;
	unsigned int *reg_table = NULL, size_encl = 0;
	unsigned int encl_0_reg[] = {
		VPU_VIU_VENC_MUX_CTRL,
		ENCL_VIDEO_EN,
		ENCL_VIDEO_MODE,
		ENCL_VIDEO_MODE_ADV,
		ENCL_VIDEO_MAX_PXCNT,
		ENCL_VIDEO_MAX_LNCNT,
		ENCL_VIDEO_HAVON_BEGIN,
		ENCL_VIDEO_HAVON_END,
		ENCL_VIDEO_VAVON_BLINE,
		ENCL_VIDEO_VAVON_ELINE,
		ENCL_VIDEO_HSO_BEGIN,
		ENCL_VIDEO_HSO_END,
		ENCL_VIDEO_VSO_BEGIN,
		ENCL_VIDEO_VSO_END,
		ENCL_VIDEO_VSO_BLINE,
		ENCL_VIDEO_VSO_ELINE,
		ENCL_VIDEO_H_PRE_DE_BEGIN,
		ENCL_VIDEO_H_PRE_DE_END,
		ENCL_VIDEO_V_PRE_DE_BLINE,
		ENCL_VIDEO_V_PRE_DE_ELINE,
		ENCL_VIDEO_RGBIN_CTRL,
		LCD_GAMMA_CNTL_PORT0,
		LCD_RGB_BASE_ADDR,
		LCD_RGB_COEFF_ADDR,
		LCD_POL_CNTL_ADDR,
		LCD_DITH_CNTL_ADDR,
		VPU_DISP_VIU0_CTRL,
		VPU_VENC_CTRL,
		ENCL_INBUF_CNTL0,
		ENCL_INBUF_CNTL1
	};

	reg_table = encl_0_reg;
	size_encl = ARRAY_SIZE(encl_0_reg);
	for (i = 0; i < size_encl; i++)
		printf("vcbus [0x%04x] = 0x%08x\n", reg_table[i], lcd_vcbus_read(reg_table[i]));
}

static void lcd_venc_save_bootctrl_to_reg(struct aml_lcd_drv_s *pdrv)
{
	unsigned int val = 0;

	val = (pdrv->boot_ctrl.init_level & 0xf) | (!!(pdrv->status & LCD_STATUS_IF_ON) << 4)
		| ((pdrv->boot_ctrl.dccd_flag & 0x1) << 5)
		| ((pdrv->boot_ctrl.mute_flag & 0x1) << 6)
		| (0xa << 9);
	lcd_vcbus_write(L_STH1_HS_ADDR, val);

	val = pdrv->boot_ctrl.frame_rate & 0x1fff;
		lcd_vcbus_write(L_STH1_HS_ADDR + 4, val);

	val = (pdrv->boot_ctrl.lcd_type & 0xf) | ((pdrv->boot_ctrl.clk_mode & 0xf) << 4)
		| ((pdrv->boot_ctrl.ppc & 0x3) << 8)
		| ((pdrv->boot_ctrl.bl_state & 0x1) << 11);
	lcd_vcbus_write(L_STH1_HS_ADDR + 8, val);

	val = pdrv->boot_ctrl.advanced_flag & 0xff;
	lcd_vcbus_write(L_STH1_HS_ADDR + 12, val);
}

int lcd_venc_op_init_t5m(struct lcd_venc_op_s *venc_op)
{
	if (!venc_op)
		return -1;

	venc_op->wait_vsync = lcd_venc_wait_vsync;
	venc_op->get_max_lcnt = lcd_venc_get_max_lint_cnt;
	venc_op->venc_debug_test = lcd_venc_debug_test;
	venc_op->venc_set_timing = lcd_venc_set_timing;
	venc_op->venc_set = lcd_venc_set;
	venc_op->venc_enable = lcd_venc_enable_ctrl;
	//if (pdata->chip_type == LCD_CHIP_T6D || pdata->chip_type == LCD_CHIP_T6W)
	//	venc_op->mute_set = lcd_venc_mute_set; //match kernel setting
	venc_op->get_encl_line_cnt = lcd_venc_get_encl_line_cnt;
	venc_op->venc_reg_dump = lcd_venc_reg_dump;
	venc_op->bootctrl_to_regs = lcd_venc_save_bootctrl_to_reg;

	return 0;
};
#endif
