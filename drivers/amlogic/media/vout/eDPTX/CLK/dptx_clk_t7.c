// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/eDPTX/DPTX.h>
#include "./dptx_clk_ctrl.h"
#include "../dptx_common.h"
#include "../dptx_regs.h"

static uint32_t tcon_div[5][3] = {
	/* div_mux, div2/4_sel, div4_bypass */
	{1, 0, 1},  /* div1 */
	{0, 0, 1},  /* div2 */
	{0, 1, 1},  /* div4 */
	{0, 0, 0},  /* div8 */
	{0, 1, 0},  /* div16 */
};

static void dptx_set_link_pll_t7(struct dptx_drv_s *dptx, uint8_t port)
{
	struct dptx_clk_cfg_s *link_cconf = &dptx->link_clk;
	unsigned int pll_ctrl, pll_ctrl1;
	unsigned int tcon_div_sel;
	uint32_t pll_ctrl0_reg, pll_ctrl1_reg, pll_ctrl2_reg, pll_ctrl3_reg, pll_ctrl4_reg,
		 pll_stts_reg;
	int cnt = 0;

	DPTX_P_DBG(dptx, port, "%s",  __func__);

	// tcon_div_sel = link_cconf->pll_tcon_div_sel;
	tcon_div_sel = 1;
	pll_ctrl = ((0x3 << 17) | /* gate ctrl */
		(tcon_div[tcon_div_sel][2] << 16) |
		(link_cconf->pll_n << 10) |
		(link_cconf->pll_m << 0) |
		(link_cconf->pll_od_sel[2] << 23) |
		(link_cconf->pll_od_sel[1] << 21) |
		(link_cconf->pll_od_sel[0] << 19));
	pll_ctrl1 = (1 << 28) |
		(tcon_div[tcon_div_sel][0] << 22) |
		(tcon_div[tcon_div_sel][1] << 21) |
		((1 << 20) | /* sdm_en */
		(link_cconf->pll_frac << 0));

	if (dptx->idx == 0 && port == 0) {
		pll_ctrl0_reg = ANACTRL_TCON_PLL0_CNTL0;
		pll_ctrl1_reg = ANACTRL_TCON_PLL0_CNTL1;
		pll_ctrl2_reg = ANACTRL_TCON_PLL0_CNTL2;
		pll_ctrl3_reg = ANACTRL_TCON_PLL0_CNTL3;
		pll_ctrl4_reg = ANACTRL_TCON_PLL0_CNTL4;
		pll_stts_reg  = ANACTRL_TCON_PLL0_STS;
	} else {
		pll_ctrl0_reg = ANACTRL_TCON_PLL1_CNTL0;
		pll_ctrl1_reg = ANACTRL_TCON_PLL1_CNTL1;
		pll_ctrl2_reg = ANACTRL_TCON_PLL1_CNTL2;
		pll_ctrl3_reg = ANACTRL_TCON_PLL1_CNTL3;
		pll_ctrl4_reg = ANACTRL_TCON_PLL1_CNTL4;
		pll_stts_reg  = ANACTRL_TCON_PLL1_STS;
	}

set_pll_retry_t7:
	dptx_ana_write(pll_ctrl0_reg, pll_ctrl);
	udelay(10);
	dptx_ana_setb(pll_ctrl0_reg, 1, 29, 1);
	udelay(10);
	dptx_ana_setb(pll_ctrl0_reg, 1, 28, 1);
	udelay(10);
	dptx_ana_write(pll_ctrl1_reg, pll_ctrl1);
	udelay(10);
	dptx_ana_write(pll_ctrl2_reg, 0x0000110c);
	udelay(10);
	if (link_cconf->pll_fvco < 3800000000ULL)
		dptx_ana_write(pll_ctrl3_reg, 0x10051100);
	else
		dptx_ana_write(pll_ctrl3_reg, 0x10051400);
	udelay(10);
	dptx_ana_setb(pll_ctrl4_reg, 0x0100c0, 0, 24);
	udelay(10);
	dptx_ana_setb(pll_ctrl4_reg, 0x8300c0, 0, 24);
	udelay(10);
	dptx_ana_setb(pll_ctrl0_reg, 1, 26, 1);
	udelay(10);
	dptx_ana_setb(pll_ctrl0_reg, 0, 29, 1);
	udelay(10);
	dptx_ana_write(pll_ctrl2_reg, 0x0000300c);

	if (dptx_pll_wait_lock(pll_stts_reg, 31)) {
		udelay(100);
		dptx_ana_setb(pll_ctrl2_reg, 1, 5, 1);
	} else {
		if (cnt++ < PLL_RETRY_MAX)
			goto set_pll_retry_t7;
		DPTX_ERR(dptx, "pll lock failed");
	}
}

static void dptx_set_phy_dig_div(struct dptx_drv_s *dptx, uint8_t port_to_pll)
{
	struct dptx_clk_cfg_s *vid_cconf = &dptx->vid_clk;
	uint32_t reg_dphy_tx_ctrl1;
	uint32_t bit_div_en, bit_div0, bit_div1, bit_rst;

	DPTX_DBG(dptx, "%s(%u)", __func__, port_to_pll);

	if (dptx->idx == 0 && port_to_pll == 0) {
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		bit_div_en = 24;
		bit_div0 = 0;
		bit_div1 = 4;
		bit_rst = 19;
	} else {
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		bit_div_en = 25;
		bit_div0 = 8;
		bit_div1 = 12;
		bit_rst = 20;
	}

	dptx_reset_setb(RESETCTRL_RESET1_MASK, 0, bit_rst, 1);
	dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit_rst, 1);
	udelay(1);
	dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit_rst, 1);
	udelay(10);

	dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 0, 1); // Enable dphy clock
	dptx_combo_dphy_setb(COMBO_DPHY_EDP_PIXEL_CLK_DIV, 0, bit_div_en, 1); // Disable edp_div
	dptx_combo_dphy_setb(COMBO_DPHY_EDP_PIXEL_CLK_DIV, vid_cconf->div0, bit_div0, 4);
	dptx_combo_dphy_setb(COMBO_DPHY_EDP_PIXEL_CLK_DIV, vid_cconf->div1, bit_div1, 4);
	dptx_combo_dphy_setb(COMBO_DPHY_EDP_PIXEL_CLK_DIV, 1, bit_div_en, 1); // Enable edp_div
	dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 4, 1); // sel edp_div clock
	dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 5, 1); // sel tcon_pll clock
}

static void dptx_set_vid_pll_div_t7(struct dptx_drv_s *dptx, uint8_t port_to_pll)
{
	struct dptx_clk_cfg_s *vid_cconf = &dptx->vid_clk;
	struct dptx_pll_data_s *pll_data = (struct dptx_pll_data_s *)dptx->vid_clk.pll_data;
	uint32_t reg_vid_pll_div, reg_vid2_clk_ctrl, shift_val, shift_sel;
	uint8_t i;

	if (dptx->idx == 0 && port_to_pll == 0) {
		reg_vid_pll_div = COMBO_DPHY_VID_PLL0_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
	} else {
		reg_vid_pll_div = COMBO_DPHY_VID_PLL1_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK1_CTRL;
	}

	dptx_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_EN, 1);
	udelay(5);

	/* Disable the div output clock */
	dptx_combo_dphy_setb(reg_vid_pll_div, 0, 19, 1);
	dptx_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);

	for (i = 0; i < CLK_DIV_SEL_MAX; i++) {
		if (dptx_clk_div_table[i].divider >= pll_data->div_sel_max)
			break;
		if (vid_cconf->pll_clk_div_sel == dptx_clk_div_table[i].divider)
			break;
	}
	if (i == CLK_DIV_SEL_MAX) {
		DPTX_ERR(dptx, "invalid clk divider\n");
		i = 0;
	}
	shift_val = dptx_clk_div_table[i].shift_val;
	shift_sel = dptx_clk_div_table[i].shift_sel;

	if (shift_val == 0xffff) { /* if divide by 1 */
		dptx_combo_dphy_setb(reg_vid_pll_div, 1, 18, 1);
	} else {
		dptx_combo_dphy_setb(reg_vid_pll_div, 0, 18, 1);
		dptx_combo_dphy_setb(reg_vid_pll_div, 0, 16, 2);
		dptx_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);
		dptx_combo_dphy_setb(reg_vid_pll_div, 0, 0, 14);

		dptx_combo_dphy_setb(reg_vid_pll_div, shift_sel, 16, 2);
		dptx_combo_dphy_setb(reg_vid_pll_div, 1, 15, 1);
		dptx_combo_dphy_setb(reg_vid_pll_div, shift_val, 0, 15);
		dptx_combo_dphy_setb(reg_vid_pll_div, 0, 15, 1);
	}
	/* Enable the final output clock */
	dptx_combo_dphy_setb(reg_vid_pll_div, 1, 19, 1);
}

static void dptx_set_vclk_crt_by_path(struct dptx_drv_s *dptx, uint8_t path)
{
	struct dptx_clk_cfg_s *vid_cconf = &dptx->vid_clk;
	uint32_t reg_vid2_clk_div, reg_vid2_clk_ctrl, reg_vid_clk_ctrl2;

	DPTX_DBG(dptx, "%s(%u)", __func__, path);

	if (dptx->idx == 0 && path == 0) {
		reg_vid2_clk_div  = CLKCTRL_VIID_CLK0_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK0_CTRL;
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK0_CTRL2;
	} else {
		reg_vid2_clk_div  = CLKCTRL_VIID_CLK1_DIV;
		reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK1_CTRL;
		reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK1_CTRL2;
	}

	dptx_clk_write(reg_vid_clk_ctrl2, 0);
	dptx_clk_write(reg_vid2_clk_ctrl, 0);
	dptx_clk_write(reg_vid2_clk_div, 0);
	udelay(5);

	if (dptx->idx == 0 && path == 0)
		dptx_clk_setb(CLKCTRL_HDMI_VID_PLL_CLK_DIV, 0, 24, 1);
	// CLKCTRL_HDMI_VID_PLL_CLK_DIV
	// [24]Reg_vid_pll0_clk_sel_hdmi; [25]Reg_vid_pll2_clk_sel_hdmi

	/* setup the XD divider value */
	if (path == 1)
		dptx_clk_setb(reg_vid2_clk_div, (vid_cconf->xd * 2 - 1), VCLK2_XD, 8);
	else
		dptx_clk_setb(reg_vid2_clk_div, (vid_cconf->xd - 1), VCLK2_XD, 8);

	udelay(5);
	/* select vid_pll_clk */
	dptx_clk_setb(reg_vid2_clk_ctrl, vid_cconf->vclk_sel, VCLK2_CLK_IN_SEL, 3);

	dptx_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	dptx_clk_setb(reg_vid2_clk_div, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	dptx_clk_setb(reg_vid2_clk_div, 1, VCLK2_XD_EN, 2);
	udelay(5);

	dptx_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_DIV1_EN, 1);
	dptx_clk_setb(reg_vid2_clk_ctrl, 1, VCLK2_SOFT_RST, 1);
	udelay(10);
	dptx_clk_setb(reg_vid2_clk_ctrl, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	dptx_clk_setb(reg_vid_clk_ctrl2, 1, ENCL_GATE_VCLK, 1);

	if (dptx->sink.port_mask == 0x3)
		dptx_clk_setb(CLKCTRL_VID_CLK0_CTRL2, 1, 16, 1);
}

static void dptx_clk_cfg_print_t7(struct dptx_drv_s *dptx)
{
}

/****************** eDP 1PLL model T7 **********************/
/* PLL_VCO / tcon_div --> eDP_PHY_clk                      */
/*              '--edp_div0&1 / PLL_CLK_DIV == VID_PLL_CLK */
/* VID_PLL_CLK    -->  .                         */
/* GP0(no)        -->  |                         */
/* HiFi(no)       -->  |                         */
/* MP1(no)        -->  |                         */
/* fclk_div3(667M)-->  --> / enc_xd == ENCL_clk  */
/* fclk_div4(500M)-->  |                         */
/* fclk_div5(400M)-->  |                         */
/* fclk_div7(286M)-->  `                         */
static void dptx_link_clk_config_t7(struct dptx_drv_s *dptx, uint8_t port, uint8_t dptx_link_rate)
{
	struct dptx_clk_cfg_s *link_cconf = &dptx->link_clk;

	switch (dptx_link_rate) {
	case DP_LINK_RATE_HBR2:
		link_cconf->pll_m    = 225;
		link_cconf->pll_fvco = 5400000000ULL;
		link_cconf->pll_fout = 5400000000ULL;
		link_cconf->pll_od_sel[0] = 0;
		link_cconf->pll_od_sel[1] = 0; link_cconf->pll_od_sel[2] = 0;
		break;
	case DP_LINK_RATE_HBR:
		link_cconf->pll_m    = 225;
		link_cconf->pll_fvco = 5400000000ULL;
		link_cconf->pll_fout = 2700000000ULL;
		link_cconf->pll_od_sel[0] = 1;
		link_cconf->pll_od_sel[1] = 0; link_cconf->pll_od_sel[2] = 0;
		break;
	case DP_LINK_RATE_RBR:
	default:
		link_cconf->pll_m    = 135;
		link_cconf->pll_fvco = 3240000000ULL;
		link_cconf->pll_fout = 1620000000ULL;
		link_cconf->pll_od_sel[0] = 1;
		link_cconf->pll_od_sel[1] = 0; link_cconf->pll_od_sel[2] = 0;
		break;
	}
	link_cconf->pll_n    = 1;
	link_cconf->pll_frac = 0;
	link_cconf->pll_frac_half_shift = 0;
	link_cconf->pll_clk_div_sel = CLK_DIV_SEL_1;

	DPTX_P_PR(dptx, port, "PLL_M=%u, out=%llu", link_cconf->pll_m, link_cconf->pll_fout);
}

static void dptx_vid_clk_config_t7(struct dptx_drv_s *dptx, u32 pixel_clk)
{
	struct dptx_clk_cfg_s *link_cconf = &dptx->link_clk;
	struct dptx_clk_cfg_s *vid_cconf = &dptx->vid_clk;
	uint32_t min_err = U32_MAX;
	unsigned long long tmp_fout, tmp_fout2, error;
	uint8_t i, dptx_div0, dptx_div1, tmp_div;

	uint16_t dptx_div0_table[15] = {1, 2, 3, 4, 5, 7, 8, 9, 11, 13, 17, 19, 23, 29, 31};
	uint16_t dptx_div1_table[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 13};
	uint32_t enc_clk_src[][2] = {
		{4, 666666667}, //fclk_div3
		{5, 500000000}, //fclk_div4
		{6, 400000000}, //fclk_div5
		{7, 285714286}, //fclk_div7
	};

	for (i = 0; i < ARRAY_SIZE(enc_clk_src); i++) {
		for (tmp_div = 1; tmp_div < 255; tmp_div++) {
			tmp_fout = dptx_div_around(enc_clk_src[i][1], tmp_div);
			error = dptx_diff(tmp_fout, pixel_clk);
			if (error >= min_err)
				continue;

			min_err = error;
			vid_cconf->pll_clk_div_sel = 0;
			vid_cconf->div0 = 0;
			vid_cconf->div1 = 0;
			vid_cconf->xd = tmp_div;
			vid_cconf->vclk_sel = enc_clk_src[i][0];
			vid_cconf->clk_src = 1; //FIX_PLL
			vid_cconf->fin = enc_clk_src[i][1];
			vid_cconf->fout = tmp_fout;
		}
	}

	for (dptx_div0 = 0; dptx_div0 < ARRAY_SIZE(dptx_div0_table); dptx_div0++) {
		for (dptx_div1 = 0; dptx_div1 < ARRAY_SIZE(dptx_div1_table); dptx_div1++) {
			tmp_fout = dptx_div_around(link_cconf->pll_fout,
				(dptx_div0_table[dptx_div0] * dptx_div1_table[dptx_div1]));

			for (tmp_div = CLK_DIV_SEL_1; tmp_div < CLK_DIV_SEL_MAX; tmp_div++) {
				tmp_fout2 = dptx_clk_pll_div_calc(tmp_fout, tmp_div, CLK_DIV_I2O);
				error = dptx_diff(tmp_fout2, pixel_clk);
				if (error >= min_err)
					continue;

				min_err = error;
				vid_cconf->pll_clk_div_sel = tmp_div;
				vid_cconf->div0 = dptx_div0;
				vid_cconf->div1 = dptx_div1;
				vid_cconf->xd = 1;
				vid_cconf->vclk_sel = 0;
				vid_cconf->clk_src = 2; //LINK_CLK
				vid_cconf->fin = link_cconf->pll_fout;
				vid_cconf->fout = tmp_fout2;
			}
		}
	}

	if (min_err == U32_MAX) {
		DPTX_ERR(dptx, "invalid vid clk");
		return;
	}

	if (vid_cconf->clk_src == 1) { // fix_pll
		DPTX_PR(dptx, "vid_clk=%u: fix_clk:%lluHz xd[%u] fout:%llu error=%u",
			pixel_clk, vid_cconf->fin, vid_cconf->xd, vid_cconf->fout, min_err);
	} else if (vid_cconf->clk_src == 2) { //LINK_CLK
		DPTX_PR(dptx,
			"vid_clk=%u: link:%lluHz div=%u[%u:%u] pll_div:%s xd[%u] fout:%llu error=%u",
			pixel_clk, vid_cconf->fin,
			dptx_div0_table[vid_cconf->div0] * dptx_div1_table[vid_cconf->div1],
			vid_cconf->div0, vid_cconf->div1,
			dptx_clk_div_table[vid_cconf->pll_clk_div_sel].name,
			vid_cconf->xd, vid_cconf->fout, min_err);
	}
}

static void dptx_clktree_set_t7(struct dptx_drv_s *dptx)
{
}

static void dptx_link_clk_set_t7(struct dptx_drv_s *dptx, uint8_t port)
{
	dptx_set_link_pll_t7(dptx, port);
	// lcd_set_phy_dig_div(dptx);
}

static void dptx_vid_clk_set_t7(struct dptx_drv_s *dptx)
{
	dptx_set_phy_dig_div(dptx, 0);
	dptx_set_vid_pll_div_t7(dptx, 0);
	dptx_set_vclk_crt_by_path(dptx, 0);

	if (dptx->sink.port_mask == 0x3) {
		dptx_set_phy_dig_div(dptx, 1);
		dptx_set_vid_pll_div_t7(dptx, 1);
		dptx_set_vclk_crt_by_path(dptx, 1);

		dptx_clk_setb(CLKCTRL_VID_CLK0_CTRL2, 1, 16, 1);
	}
}

static void dptx_clk_ssc_switch_t7(struct dptx_drv_s *dptx, uint8_t port, u8 status)
{
}

static struct dptx_pll_data_s dptx_t7_vid_pll0_data = {
	.pll_od_fb = 0,
	.pll_m_range = {2, 511},
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	// .pll_od_sel_max = 3,
	// .pll_ref_range = {5000000,       25000000},
	.pll_vco_range = {3000000000ULL, 6000000000ULL},
	.pll_out_range = {187500000,     3100000000ULL},
	.pll_div_in_fmax = 3100000000ULL,
	.pll_div_out_fmax = 1500000000,
	.od_cnt = 3,
	.div_sel_max = CLK_DIV_SEL_MAX,

	.xd_out_fmax = 750000000,
	.xd_max = 256,

	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,
};

static struct dptx_pll_data_s dptx_t7_vid_pll1_data = {
	.pll_od_fb = 0,
	.pll_m_range = {2, 511},
	.pll_frac_range = (1 << 17),
	.pll_frac_sign_bit = 18,
	// .pll_od_sel_max = 3,
	// .pll_ref_range = {5000000,       25000000},
	.pll_vco_range = {3000000000ULL, 6000000000ULL},
	.pll_out_range = {  18750000ULL, 3100000000ULL},
	.pll_div_in_fmax = 3100000000ULL,
	.pll_div_out_fmax = 1500000000U,
	.od_cnt = 3,
	.div_sel_max = CLK_DIV_SEL_MAX,

	.xd_out_fmax = 750000000,
	.xd_max = 256,

	.ss_level_max = 60,
	.ss_freq_max = 6,
	.ss_mode_max = 2,
	.ss_dep_base = 500, //ppm
	.ss_dep_sel_max = 12,
	.ss_str_m_max = 10,
};

struct dptx_clk_op_s dptx_clk_op_t7 = {
	.clk_config_print = dptx_clk_cfg_print_t7,
	.clktree_set      = dptx_clktree_set_t7,
	.link_clk_config  = dptx_link_clk_config_t7,
	.link_clk_set     = dptx_link_clk_set_t7,
	.vid_clk_config   = dptx_vid_clk_config_t7,
	.vid_clk_set      = dptx_vid_clk_set_t7,
	.clk_ssc_switch   = dptx_clk_ssc_switch_t7,
	// void (*prbs)(struct dptx_drv_s *dptx);
};

struct dptx_clk_op_s *dptx_clk_op_init_t7(struct dptx_drv_s *dptx)
{
	if (dptx->idx == 0) {
		dptx->vid_clk.pll_data = &dptx_t7_vid_pll0_data;
		dptx->link_clk.pll_data = &dptx_t7_vid_pll0_data;
	} else {
		dptx->vid_clk.pll_data = &dptx_t7_vid_pll1_data;
		dptx->link_clk.pll_data = &dptx_t7_vid_pll1_data;
	}

	return &dptx_clk_op_t7;
}
