// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include "hdmitx_clk.h"
#include "hdmitx_misc.h"

#define MIN_HTXPLL_VCO 3000000 /* Min 3GHz */
#define MAX_HTXPLL_VCO 6000000 /* Max 6GHz */

#define WAIT_FOR_PLL_LOCKED(_reg) \
	do { \
		unsigned int st = 0; \
		int cnt = 10; \
		unsigned int reg = _reg; \
		while (cnt--) { \
			usleep_range(50, 60); \
			st = (((hd21_read_reg(reg) >> 31) & 0x1) == 1); \
			if (st) \
				break; \
			else { \
				/* reset hpll */ \
				hd21_set_reg_bits(reg, 1, 30, 1); \
				hd21_set_reg_bits(reg, 0, 30, 1); \
			} \
		} \
		if (cnt < 9) \
			pr_info("pll[0x%x] reset %d times\n", reg, 9 - cnt);\
	} while (0)

#define usleep_range(a, b) udelay(a)

static void set_hpll_sspll_s7d(enum hdmi_vic vic);
/* local frac_rate flag */
static u32 frac_rate;

void disable_hdmitx_s7d_plls(struct hdmitx_dev *hdev)
{
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0);
}

/* htx pll VCO output: (3G, 6G), for tmds */
static void set_s7d_htxpll_clk_other(const u32 clk, const bool frl_en)
{
	u32 quotient;
	u32 remainder;

	if (clk < 3000000 || clk >= 6000000) {
		pr_err("%s[%d] clock should be 4~6G\n", __func__, __LINE__);
		return;
	}

	quotient = clk / 12000;
	remainder = clk - quotient * 12000;
	/* remainder range: 0 ~ 23999, 0x5dbf, 15bits */
	remainder *= 1 << 17;
	remainder /= 12000;

	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x00017000 | (quotient << 0));
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x9040137d);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x04000000);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x01160000 | remainder);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 28, 1);
	usleep_range(10, 20);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 18, 1);
	usleep_range(10, 20);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 30, 1);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0, 18, 1);
	usleep_range(80, 90);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
	usleep_range(80, 90);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);

}

void set21_s7d_htxpll_clk_out(const u32 clk, u32 div)
{
	u32 pll_od0 = 0;
	u32 pll_od00 = 0;
	u32 pll_od01 = 0;
	u32 pll_od2 = 0;
	u32 pll_od20 = 0;
	u32 pll_od21 = 0;
	u32 pll_od1 = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	enum hdmi_colorspace cs = HDMI_COLORSPACE_YUV444;
	enum hdmi_color_depth cd = COLORDEPTH_24B;

	if (!hdev || !hdev->para)
		return;

	cs = hdev->para->cs;
	cd = hdev->para->cd;

	pr_info("%s[%d] htxpll vco %d div %d\n", __func__, __LINE__, clk, div);

	if (clk <= 3000000 || clk > 6000000) {
		pr_info("%s[%d] %d out of htxpll range(3~6G]\n", __func__, __LINE__, clk);
		return;
	}
	set_s7d_htxpll_clk_other(clk, hdev->para->frl_rate ? 1 : 0);

	//pll_od00
	if ((div % 8) == 0) {
		pll_od00 = 3; //div8
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od00 = 2; //div4
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od00 = 1; //div2
		div = div / 2;
	}

	//pll_od01
	if ((div % 8) == 0) {
		pll_od01 = 3;
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od01 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od01 = 1;
		div = div / 2;
	}

	//pll_od0
	pll_od0 = (pll_od01 << 3) | pll_od00;

	//pll_od20 for clk to phy
	if ((div % 4) == 0) {
		pll_od20 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od20 = 1;
		div = div / 2;
	}

	//pll_od21 for clk_out2
	if (cs == HDMI_COLORSPACE_YUV420)
		pll_od21 = pll_od20;
	else
		pll_od21 = pll_od20 + 1;

	pll_od2 = (pll_od20 << 3) | pll_od21;

	//pll_od1
	if (cs != HDMI_COLORSPACE_YUV422) {
		if (cd == COLORDEPTH_24B)
			pll_od1 = 0;//pll_div3 = 5;
		else if (cd == COLORDEPTH_30B)
			pll_od1 = 1;//pll_div3 = 6.25;
		else if (cd == COLORDEPTH_36B)
			pll_od1 = 2;//pll_div3 = 7.5;
	}

	//tx_spll_hdmi_clk_select
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL3, 1, 19, 1);
	pr_info("pll_od0 = %d, pll_od2 = %d, pll_od1 = %d\n",
		pll_od0, pll_od2, pll_od1);
	//tx_spll_lock_by_pass_alo
	if (hdev->clk_analog_path)
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL3, pll_od1, 22, 2);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL3, pll_od2, 24, 6);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, pll_od0, 20, 6);
}

void hdmitx21_set_audioclk(bool en)
{
	/* cts_hdmitx_aud_clk is used by spdif, need to be more than 6 times
	 * of spdif_clk. Configuring it to 200M can basically cover the currently
	 * used cases
	 *
	 * Enable hdmitx_aud_clk
	 * [10: 9] clk_sel for cts_hdmitx_aud_clk: 2=fclk_div3
	 * [    8] clk_en for cts_hdmitx_aud_clk
	 * [ 6: 0] clk_div for cts_hdmitx_aud_clk: fclk_div3/aud_clk_div
	 */
	hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL1, 3, 9, 2);// FIXPLL/5
	hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL1, 1, 0, 8);//div2
	// [    8] clk_en for cts_hdmitx_aud_clk
	hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL1, en, 8, 1);
}

void hdmitx21_set_default_clk(void)
{
	u32 data32;

	// Enable clk81_hdmitx_pclk
	hd21_set_reg_bits(CLKCTRL_SYS_CLK_EN0_REG2, 1, 4, 1);

	// Enable fixed hdmitx_sys_clk
	data32 = 0;
	data32 |= (3 << 9); // [10: 9] clk_sel for cts_hdmitx_sys_clk: 3=fclk_div5
	data32 |= (0 << 8); // [    8] clk_en for cts_hdmitx_sys_clk
	data32 |= (1 << 0); // [ 6: 0] clk_div for cts_hdmitx_sys_clk: fclk_dvi5/2=400/2=200M
	hd21_write_reg(CLKCTRL_HDMI_CLK_CTRL, data32);
	data32 |= (1 << 8); // [    8] clk_en for cts_hdmitx_sys_clk
	hd21_write_reg(CLKCTRL_HDMI_CLK_CTRL, data32);

	// Enable fixed hdmitx_prif_clk, hdmitx_200m_clk
	data32 = 0;
	data32 |= (3 << 25); // [26:25] clk_sel for cts_hdmitx_200m_clk: 3=fclk_div5
	data32 |= (0 << 24); // [   24] clk_en for cts_hdmitx_200m_clk
	data32 |= (1 << 16); // [22:16] clk_div for cts_hdmitx_200m_clk: fclk_dvi5/16=400/16=25M
	data32 |= (3 << 9); // [10: 9] clk_sel for cts_hdmitx_prif_clk: 3=fclk_div5
	data32 |= (0 << 8); // [    8] clk_en for cts_hdmitx_prif_clk
	data32 |= (1 << 0); // [ 6: 0] clk_div for cts_hdmitx_prif_clk: fclk_dvi5/2=400/2=200M
	hd21_write_reg(CLKCTRL_HTX_CLK_CTRL0, data32);
	data32 |= (1 << 24); // [   24] clk_en for cts_hdmitx_200m_clk
	data32 |= (1 << 8); // [    8] clk_en for cts_hdmitx_prif_clk
	hd21_write_reg(CLKCTRL_HTX_CLK_CTRL0, data32);

	//hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 0, 0, 5);

	// Bring HDMITX MEM output of power down
	hd21_set_reg_bits(PWRCTRL_MEM_PD11, 0, 8, 8);
	// Bring out of reset
	hdmitx21_wr_reg(HDMITX_TOP_SW_RESET, 0);
	// Test after initial out of reset, cannot write to IP register, unless enable access
	hdmitx21_wr_reg(INTR3_MASK_IVCTX, 0xff);
	hdmitx21_wr_reg(HDMITX_TOP_SEC_SCRATCH, 1);
}

void hdmitx21_set_cts_hdcp22_clk(struct hdmitx_dev *hdev)
{
	//hd21_write_reg(CLKCTRL_HDCP22_CLK_CTRL, 0x01000100);
}

void hdmitx21_set_hdcp_pclk(struct hdmitx_dev *hdev)
{
	/* top hdcp pixel clock */
	hd21_set_reg_bits(CLKCTRL_SYS_CLK_EN0_REG2, 1, 3, 1);
}

/* --------------------------------------------------
 *             set_tmds_vid_clk_div
 * --------------------------------------------------
 * wire            clk_final_en    = control[19];
 * wire            clk_div1        = control[18];
 * wire    [1:0]   clk_sel         = control[17:16];
 * wire            set_preset      = control[15];
 * wire    [14:0]  shift_preset    = control[14:0];
 */
static void set_tmds_vid_clk_div(u32 div_val)
{
	u32 div_reg;
	u32 shift_val = 0;
	u32 shift_sel = 0;

	div_reg = CLKCTRL_VID_PLL_CLK_DIV;

	// Disable the output clock
	hd21_set_reg_bits(div_reg, 0, 18, 2);
	hd21_set_reg_bits(div_reg, 0, 15, 1);

	switch (div_val) {
	case VID_PLL_DIV_1:
		shift_val = 0xFFFF;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_2:
		shift_val = 0x0aaa;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3:
		shift_val = 0x0db6;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3p5:
		shift_val = 0x36cc;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_3p75:
		shift_val = 0x6666;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_4:
		shift_val = 0x0ccc;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_5:
		shift_val = 0x0c63;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_6:
		shift_val = 0x0e38;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_6p25:
		shift_val = 0x0000;
		shift_sel = 3;
		break;
	case VID_PLL_DIV_7:
		shift_val = 0x3c78;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_7p5:
		shift_val = 0x78f0;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_12:
		shift_val = 0x0fc0;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_14:
		shift_val = 0x3f80;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_15:
		shift_val = 0x7f80;
		shift_sel = 2;
		break;
	default:
		pr_err("%s[%d] invalid div %d\n", __func__, __LINE__, div_val);
	}

	if (shift_val == 0xffff) { // if divide by 1
		hd21_set_reg_bits(div_reg, 1, 18, 1);
	} else {
		hd21_set_reg_bits(div_reg, 0, 18, 1);
		hd21_set_reg_bits(div_reg, 0, 16, 2);
		hd21_set_reg_bits(div_reg, 0, 15, 1);
		hd21_set_reg_bits(div_reg, 0, 0, 15);

		hd21_set_reg_bits(div_reg, shift_sel, 16, 2);
		hd21_set_reg_bits(div_reg, 1, 15, 1);
		hd21_set_reg_bits(div_reg, shift_val, 0, 15);
		hd21_set_reg_bits(div_reg, 0, 15, 1);
	}
	// Enable the final output clock
	hd21_set_reg_bits(div_reg, 1, 19, 1);
}

/* if vsync likes 24000, 30000, ... etc, return 1 */
static bool is_vsync_int(u32 clk)
{
	if (clk % 3000 == 0)
		return 1;
	return 0;
}

/* if vsync likes 59940, ... etc, return 1 */
static bool is_vsync_frac(u32 clk)
{
	clk += clk / 1000;
	if (is_vsync_int(clk) || is_vsync_int(clk + 1))
		return 1;
	return 0;
}

/* for varied hdmi basic modes, such as
 * vic/16, the vsync is 60, and may shift to 59.94
 * but vic/2, the vsync is 59.94, and may shift to 60
 * return values:
 *    0: no any shift
 *    1: shift down 0.1%
 *    2: shift up 0.1%
 */
static u32 check_clock_shift(enum hdmi_vic vic, u32 frac_policy)
{
	const struct hdmi_timing *timing = NULL;

	timing = hdmitx21_gettiming_from_vic(vic);
	if (!timing) {
		pr_err("%s[%d] not valid vic %d\n", __func__, __LINE__, vic);
		return 0;
	}

	/* only check such as 24hz, 30hz, 60hz, ... */
	if (!hdmitx_likely_frac_rate_mode(timing->name))
		return 0;

	if (is_vsync_int(timing->v_freq)) {
		if (frac_policy)
			return 1;
		else
			return 0;
	}
	if (is_vsync_frac(timing->v_freq)) {
		if (frac_policy)
			return 0;
		else
			return 2;
	}
	return 0;
}

static void set_hdmitx_s7d_htx_pll(struct hdmitx_dev *hdev)
{
	enum hdmi_vic vic = HDMI_0_UNKNOWN;
	enum hdmi_colorspace cs = HDMI_COLORSPACE_YUV444;
	enum hdmi_color_depth cd = COLORDEPTH_24B;
	u32 base_pixel_clk = 25200;
	u32 htx_vco = 5940000;
	u32 div = 1;

	if (!hdev || !hdev->para)
		return;

	vic = hdev->para->timing.vic;
	cs = hdev->para->cs;
	cd = hdev->para->cd;
	if (vic == HDMI_0_UNKNOWN) {
		pr_err("%s[%d] not valid vic %d\n", __func__, __LINE__, vic);
		return;
	}

	base_pixel_clk = hdev->para->timing.pixel_freq;
	if (base_pixel_clk < 25175 || base_pixel_clk > 5940000) {
		pr_err("%s[%d] not valid pixel clock %d\n", __func__, __LINE__, base_pixel_clk);
		return;
	}

	pr_info("%s[%d] base_pixel_clk %d  cs %d  cd %d  frac_rate %d\n",
		__func__, __LINE__, base_pixel_clk, cs, cd, frac_rate);
	/* for legacy TMDS modes */
	if (cs != HDMI_COLORSPACE_YUV422) {
		switch (cd) {
		case COLORDEPTH_48B:
			base_pixel_clk = base_pixel_clk * 2;
			break;
		case COLORDEPTH_36B:
			base_pixel_clk = base_pixel_clk * 3 / 2;
			break;
		case COLORDEPTH_30B:
			base_pixel_clk = base_pixel_clk * 5 / 4;
			break;
		case COLORDEPTH_24B:
		default:
			base_pixel_clk = base_pixel_clk * 1;
			break;
		}
	}
	if (check_clock_shift(vic, frac_rate) == 1)
		base_pixel_clk = base_pixel_clk - base_pixel_clk / 1001;
	if (check_clock_shift(vic, frac_rate) == 2)
		base_pixel_clk = base_pixel_clk + base_pixel_clk / 1000;
	base_pixel_clk = base_pixel_clk * 10; /* for tmds modes, here should multi 10 */
	if (cs == HDMI_COLORSPACE_YUV420)
		base_pixel_clk /= 2;
	pr_info("%s[%d] calculate pixel_clk to %d\n", __func__, __LINE__, base_pixel_clk);
	if (base_pixel_clk > MAX_HTXPLL_VCO) {
		pr_err("%s[%d] base_pixel_clk %d over MAX_HTXPLL_VCO %d\n",
			__func__, __LINE__, base_pixel_clk, MAX_HTXPLL_VCO);
	}

	div = 1;
	/* the base pixel_clk range should be 250M ~ 5940M? */
	htx_vco = base_pixel_clk;
	do {
		if (htx_vco >= MIN_HTXPLL_VCO && htx_vco < MAX_HTXPLL_VCO)
			break;
		div *= 2;
		htx_vco *= 2;
	} while (div <= 32);

	set21_s7d_htxpll_clk_out(htx_vco, div);
}

static void set_hdmitx_htx_pll(struct hdmitx_dev *hdev)
{
	enum hdmi_colorspace cs = hdev->para->cs;
	enum hdmi_color_depth cd = hdev->para->cd;
	u8 clk_div_val = VID_PLL_DIV_5;
	char *sspll_dis = NULL;

	if (hdev->pxp_mode) /* skip VCO setting */
		return;

	set_hdmitx_s7d_htx_pll(hdev);
	sspll_dis = env_get("sspll_dis");
	if ((!sspll_dis || !strcmp(sspll_dis, "0")) && cd == COLORDEPTH_24B)
		set_hpll_sspll_s7d(hdev->para->vic);
	if (hdev->clk_analog_path) {
		pr_info("select vid_pix_clk source for encp/pixel_clk\n");
		return;
	}
	if (cs != HDMI_COLORSPACE_YUV422) {
		if (cd == COLORDEPTH_36B)
			clk_div_val = VID_PLL_DIV_7p5;
		else if (cd == COLORDEPTH_30B)
			clk_div_val = VID_PLL_DIV_6p25;
		else
			clk_div_val = VID_PLL_DIV_5;
	}
	set_tmds_vid_clk_div(clk_div_val);
	return;

}

void hdmitx_set_clkdiv(struct hdmitx_dev *hdev)
{
}

void hdmitx21_set_clk(struct hdmitx_dev *hdev)
{
	frac_rate = hdmitx_check_frac_rate(hdev);
	disable_hdmitx_s7d_plls(hdev);
	set_hdmitx_htx_pll(hdev);
}

static void set_hpll_sspll_s7d(enum hdmi_vic vic)
{
	switch (vic) {
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_31_1920x1080p50_16x9:
	case HDMI_4_1280x720p60_16x9:
	case HDMI_19_1280x720p50_16x9:
	case HDMI_5_1920x1080i60_16x9:
	case HDMI_20_1920x1080i50_16x9:
		/* enable ssc, need update electric to 0x0100 */
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL3, 3, 20, 2);//enable ssc
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 2, 16, 4);//set ssc 1000 ppm
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 1, 4, 1);//SS strength multiplier
		break;
	default:
		break;
	}
}
