// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <fdtdec.h>
#include <dm.h>
#include <amlogic/cpu_id.h>
#include <amlogic/media/vout/aml_vout.h>
#include <amlogic/media/vout/aml_cvbs.h>
#include "cvbs_reg.h"
#include "cvbs_config.h"
#include "cvbs.h"
#include "vdac.h"

/*----------------------------------------------------------------------------*/
static struct cvbs_drv_s cvbs_drv = {
	.data = NULL,
};

static struct cvbs_data_s cvbs_data_g12a = {
	.chip_type = CVBS_CHIP_G12A,

	.reg_vid_pll_clk_div = HHI_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = HHI_VID_CLK_DIV,
	.reg_vid_clk_ctrl = HHI_VID_CLK_CNTL,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = HHI_VIID_CLK_CNTL,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_g12b = {
	.chip_type = CVBS_CHIP_G12B,

	.reg_vid_pll_clk_div = HHI_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = HHI_VID_CLK_DIV,
	.reg_vid_clk_ctrl = HHI_VID_CLK_CNTL,
	.reg_vid2_clk_div = HHI_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = HHI_VIID_CLK_CNTL,
	.reg_vid_clk_ctrl2 = HHI_VID_CLK_CNTL2,

	.vdac_vref_adj = 0xf,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_sc2 = {
	.chip_type = CVBS_CHIP_SC2,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_s4 = {
	.chip_type = CVBS_CHIP_S4,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_s4d = {
	.chip_type = CVBS_CHIP_S4D,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_s1a = {
	.chip_type = CVBS_CHIP_S1A,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x0,
};

static struct cvbs_data_s cvbs_data_s7 = {
	.chip_type = CVBS_CHIP_S7,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x5c,
};

static struct cvbs_data_s cvbs_data_s7d = {
	.chip_type = CVBS_CHIP_S7D,

	.reg_vid_pll_clk_div = CLKCTRL_VID_PLL_CLK_DIV,
	.reg_vid_clk_div = CLKCTRL_VID_CLK_DIV,
	.reg_vid_clk_ctrl = CLKCTRL_VID_CLK_CTRL,
	.reg_vid2_clk_div = CLKCTRL_VIID_CLK_DIV,
	.reg_vid2_clk_ctrl = CLKCTRL_VIID_CLK_CTRL,
	.reg_vid_clk_ctrl2 = CLKCTRL_VID_CLK_CTRL2,

	.vdac_vref_adj = 0x10,
	.vdac_gsw = 0x5c,
};

struct cvbs_drv_s *get_cvbs_drv(void)
{
	return &cvbs_drv;
}

unsigned int cvbs_mode = VMODE_MAX;
/*bit[0]: 0=vid_pll, 1=gp0_pll*/
/*bit[1]: 0=vid2_clk, 1=vid1_clk*/
/*path 0:vid_pll vid2_clk*/
/*path 1:gp0_pll vid2_clk*/
/*path 2:vid_pll vid1_clk*/
/*path 3:gp0_pll vid1_clk*/
static unsigned int s_enci_clk_path = 0;

/*----------------------------------------------------------------------------*/
// configuration for enci bist
int cvbs_set_bist(char* bist_mode)
{
	if (!strcmp(bist_mode, "off")) {
		cvbs_write_vcbus(ENCI_VIDEO_MODE_ADV, 0x26);
		cvbs_write_vcbus(ENCI_TST_EN, 0x0);
	} else {
		unsigned int mode = 0;

		if (!strcmp(bist_mode, "fixval") || !strcmp(bist_mode, "0"))
			mode = 0;
		else if (!strcmp(bist_mode, "colorbar") || !strcmp(bist_mode, "1"))
			mode = 1;
		else if (!strcmp(bist_mode, "thinline") || !strcmp(bist_mode, "2"))
			mode = 2;
		else if (!strcmp(bist_mode, "dotgrid") || !strcmp(bist_mode, "3"))
			mode = 3;

		cvbs_write_vcbus(ENCI_VIDEO_MODE_ADV, 0x2);
		cvbs_write_vcbus(ENCI_TST_MDSEL, mode);
		cvbs_write_vcbus(ENCI_TST_CLRBAR_STRT, 0x112);
		cvbs_write_vcbus(ENCI_TST_CLRBAR_WIDTH, 0xb4);
		cvbs_write_vcbus(ENCI_TST_EN, 0x1);
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
int cvbs_set_vdac(int status)
{
	switch (status) {
	case 0:// close vdac
		if (cvbs_drv.data)
			vdac_enable(0, VDAC_MODULE_CVBS_OUT);
		else
			printf("cvbs ERROR:need run cvbs init.\n");
		break;
	case 1:// from enci to vdac
		cvbs_set_vcbus_bits(VENC_VDAC_DACSEL0, 0, 5, 1);
		if (cvbs_drv.data) {
			vdac_ctrl_vref_adj(cvbs_drv.data->vdac_vref_adj);
			vdac_enable(1, VDAC_MODULE_CVBS_OUT);
		} else {
			printf("cvbs ERROR:need run cvbs init.\n");
		}
		break;
	default:
		break;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
// interface for debug
static void cvbs_dump_cvbs_regs(void)
{
	struct reg_s *p = NULL;

	if (VMODE_PAL == cvbs_mode) {
		// 576cvbs
		p = (struct reg_s*)&tvregs_576cvbs_enc[0];

	} else if (VMODE_NTSC == cvbs_mode) {
		// 480cvbs
		p = (struct reg_s*)&tvregs_480cvbs_enc[0];
	}

	if (NULL == p) {
		printf("it's not in cvbs mode!\n");
		return;
	}

	if (MREG_END_MARKER != p->reg)
		printf("cvbs enci registers:\n");
	while (MREG_END_MARKER != p->reg) {
		printf("    vcbus[0x%.2x] = 0x%.4x\n", p->reg, cvbs_read_vcbus(p->reg));
		p ++;
	}

	return;
}

unsigned int cvbs_clk_regs[] = {
	HHI_HDMI_PLL_CNTL0,
	HHI_HDMI_PLL_CNTL1,
	HHI_HDMI_PLL_CNTL2,
	HHI_HDMI_PLL_CNTL3,
	HHI_HDMI_PLL_CNTL4,
	HHI_HDMI_PLL_CNTL5,
	HHI_HDMI_PLL_CNTL6,
	HHI_VID_PLL_CLK_DIV,
	HHI_VIID_CLK_DIV,
	HHI_VIID_CLK_CNTL,
	HHI_VID_CLK_DIV,
	HHI_VID_CLK_CNTL2,
	MREG_END_MARKER
};

static void cvbs_dump_clock_regs(void)
{
	unsigned int *p = &cvbs_clk_regs[0];

	if (MREG_END_MARKER != *p)
		printf("cvbs clock registers:\n");
	while (MREG_END_MARKER != *p) {
		printf("    hiu[0x%.2x] = 0x%.4x\n", *p, cvbs_read_hiu(*p));
		p ++;
	}

	return;
}

int cvbs_reg_debug(int argc, char* const argv[])
{
	unsigned int value;

	if (!cvbs_drv.data) {
		printf("cvbs: error: %s: no cvbs data\n", __func__);
		return -1;
	}

	if (!strcmp(argv[1], "clock")) {
		if (argc != 2)
			goto fail_cmd;

		cvbs_dump_clock_regs();
	} else if (!strcmp(argv[1], "enci")) {
		if (argc != 2)
			goto fail_cmd;

		cvbs_dump_cvbs_regs();
	} else if (!strcmp(argv[1], "clkpath")) {
		if (argc != 3)
			goto fail_cmd;
		value = simple_strtoul(argv[2], NULL, 0);
		if ((cvbs_drv.data->chip_type == CVBS_CHIP_G12A) ||
		    (cvbs_drv.data->chip_type == CVBS_CHIP_G12B)) {
			if (value == 1 || value == 2 ||
				value == 3 || value == 0) {
				s_enci_clk_path = value;
				printf("path 0:vid_pll vid2_clk\n");
				printf("path 1:gp0_pll vid2_clk\n");
				printf("path 2:vid_pll vid1_clk\n");
				printf("path 3:gp0_pll vid1_clk\n");
				printf("you select path %d\n", s_enci_clk_path);
			} else {
				printf("invalid value, only 0/1/2/3\n");
				printf("bit[0]: 0=vid_pll, 1=gp0_pll\n");
				printf("bit[1]: 0=vid2_clk, 1=vid1_clk\n");
			}
		} else {
			printf("don't support for current chip\n");
		}
	}

	return 0;

fail_cmd:
	return 1;
}

/*----------------------------------------------------------------------------*/
// configuration for clock
#define WAIT_FOR_PLL_LOCKED(reg)                \
	do {                                    \
		unsigned int pll_lock;          \
		unsigned int time_out = 0;      \
		do {                            \
			udelay(20);             \
			pll_lock = cvbs_get_hiu_bits(reg, 31, 1);  \
			time_out ++;                               \
		} while ((pll_lock == 0) && (time_out < 10000));   \
		if (pll_lock == 0)                                 \
			printf("[notice]: cvbs pll locking\n"); \
	} while(0);

static int pll_wait_lock(unsigned int reg, unsigned int lock_bit)
{
	unsigned int pll_lock;
	int wait_loop = 2000;
	int ret = 0;

	do {
		udelay(50);
		pll_lock = cvbs_get_hiu_bits(reg, lock_bit, 1);
		wait_loop--;
	} while ((pll_lock == 0) && (wait_loop > 0));
	if (wait_loop == 0)
		ret = -1;
	return ret;
}

static void cvbs_config_hdmipll_g12a(void)
{
	printf("%s\n", __func__);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL0,	0x1a0504f7);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL1,	0x00010000);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL2,	0x00000000);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL3,	0x6a28dc00);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL4,	0x65771290);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL5,	0x39272000);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL6,	0x56540000);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL0,	0x3a0504f7);
	udelay(100);
	cvbs_write_hiu(HHI_HDMI_PLL_CNTL0,	0x1a0504f7);
	WAIT_FOR_PLL_LOCKED(HHI_HDMI_PLL_CNTL0);
	return;
}

static void cvbs_config_gp0pll_g12a(void)
{
	printf("%s\n", __func__);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL0,	0x180204f7);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL1,	0x00010000);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL2,	0x00000000);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL3,	0x6a28dc00);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL4,	0x65771290);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL5,	0x39272000);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL6,	0x56540000);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL0,	0x380204f7);
	udelay(100);
	cvbs_write_hiu(HHI_GP0_PLL_CNTL0,	0x180204f7);
	WAIT_FOR_PLL_LOCKED(HHI_GP0_PLL_CNTL0);
	return;
}

static void cvbs_config_hdmipll_sc2(void)
{
	printf("%s\n", __func__);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x3b01047b);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL1, 0x00018000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL5, 0x39270000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
	udelay(100);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x1b01047b);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
}

static void cvbs_config_hdmipll_s1a(void)
{
	printf("%s\n", __func__);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x030204F7);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL1, 0x00010000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL2, 0x01000000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL3, 0x00218000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL4, 0x04611001);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL5, 0x00039300);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL6, 0xf0410000);
	udelay(100);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x130204f7);
	udelay(100);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x330204f7);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL6, 0xf0400000);
	udelay(100);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
}

/* sync from s7 hdmitx setting */
/* htx pll VCO output: (3G, 6G), for tmds */
static void cvbs_s7_htxpll_clk_vco(const u32 clk)
{
	u32 quotient;
	u32 remainder;

	if (clk < 3000000 || clk > 6000000) {
		pr_err("%s[%d] clock should be 3~6G\n", __func__, __LINE__);
		return;
	}

	quotient = clk / 24000;
	remainder = clk - quotient * 24000;
	/* remainder range: 0 ~ 23999, 0x5dbf, 15bits */
	remainder *= 1 << 17;
	remainder /= 24000;

	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x00801000 | (quotient << 0));
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL1, 0x2c6011c8);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL2, 0x86801000);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL3, 0x00000000 | remainder);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 28, 1);
	udelay(10);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL2, 1, 29, 1);
	udelay(10);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL2, 0, 29, 1);
	udelay(80);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL1, 1, 2, 1);
	udelay(80);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
}

static void cvbs_s7_htxpll_clk_out(const u32 clk, u32 div)
{
	u32 pll_od1 = 0;
	u32 pll_od10 = 0;
	u32 pll_od11 = 0;
	u32 pll_od21 = 0;

	/* printf("%s[%d] htxpll vco %d div %d\n", __func__, __LINE__, clk, div); */

	if (clk < 3000000 || clk > 6000000) {
		pr_err("%s[%d] %d out of htxpll range(3~6G)\n", __func__, __LINE__, clk);
		return;
	}
	cvbs_s7_htxpll_clk_vco(clk);

	//pll_od10
	if ((div % 8) == 0) {
		pll_od10 = 3; //div8
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od10 = 2; //div4
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od10 = 1; //div2
		div = div / 2;
	}

	//pll_od11
	if ((div % 8) == 0) {
		pll_od11 = 3;
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od11 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od11 = 1;
		div = div / 2;
	}

	//pll_od1
	pll_od1 = (pll_od10 << 2) | pll_od11;

	/* od2 for divider for hdmi_clk_out2 */
	if ((div % 8) == 0) {
		pll_od21 = 3;
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od21 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od21 = 1;
		div = div / 2;
	}

	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 19, 1);
	/* printf("pll_od1 = %d, pll_od21 = %d\n", pll_od1, pll_od21); */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL2, pll_od21, 15, 2);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL2, pll_od1, 19, 4);
}

/* htx pll VCO output: (3G, 6G), for tmds */
static void cvbs_s7d_htxpll_clk_vco(const u32 clk)
{
	u32 quotient;
	u32 remainder;

	if (clk < 3000000 || clk > 6000000) {
		pr_err("%s[%d] clock should be 4~6G\n", __func__, __LINE__);
		return;
	}

	quotient = clk / 12000;
	remainder = clk - quotient * 12000;
	/* remainder range: 0 ~ 23999, 0x5dbf, 15bits */
	remainder *= 1 << 17;
	remainder /= 12000;
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL0, 0x00017000 | (quotient << 0));
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL1, 0x9040137d);
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL2, 0x04000000);
	/* bit[23:22] od1, bit[29:24] od2 */
	cvbs_write_hiu(ANACTRL_HDMIPLL_CTRL3, 0x00160000 | remainder);
	/* tx_spll_bias_en */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 28, 1);
	udelay(10);
	/* tx_spll_free_run_en 1 */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 18, 1);
	udelay(10);
	/* tx_spll_rstn release reset */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 30, 1);
	/* tx_spll_free_run_en 0 */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 0, 18, 1);
	udelay(80);
	/* tx_spll_rstn_lock release reset */
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
	udelay(80);
	pll_wait_lock(ANACTRL_HDMIPLL_CTRL0, 31);

	printf("%s[%d] ANACTRL_HDMIPLL_CTRL0: 0x%x, CTRL3: 0x%x\n",
		__func__, __LINE__,
		cvbs_read_hiu(ANACTRL_HDMIPLL_CTRL0),
		cvbs_read_hiu(ANACTRL_HDMIPLL_CTRL3));
}

static void cvbs_s7d_htxpll_clk_out(const u32 clk, u32 div)
{
	u32 pll_od0 = 0;
	u32 pll_od00 = 0;
	u32 pll_od01 = 0;
	u32 pll_od21 = 0;

	printf("%s[%d] htxpll vco %d div %d\n", __func__, __LINE__, clk, div);

	if (clk < 3000000 || clk > 6000000) {
		pr_err("%s[%d] %d out of htxpll range(3~6G]\n", __func__, __LINE__, clk);
		return;
	}
	cvbs_s7d_htxpll_clk_vco(clk);

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

	/* od21 for divider for hdmi_clk_out2 bit[1:0] */
	if ((div % 8) == 0) {
		pll_od21 = 3;
		div = div / 8;
	} else if ((div % 4) == 0) {
		pll_od21 = 2;
		div = div / 4;
	} else if ((div % 2) == 0) {
		pll_od21 = 1;
		div = div / 2;
	}

	//tx_spll_hdmi_clk_select
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL3, 1, 19, 1);
	printf("pll_od0 = %d, pll_od21 = %d\n", pll_od0, pll_od21);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL3, pll_od21, 24, 2);
	cvbs_set_hiu_bits(ANACTRL_HDMIPLL_CTRL0, pll_od0, 20, 6);
}

static void cvbs_set_vid1_clk(unsigned int src_pll)
{
	int sel = 0;

	if (!cvbs_drv.data) {
		printf("cvbs: error: %s: no cvbs data\n", __func__);
		return;
	}

	printf("%s\n", __func__);
	if (src_pll == 0) { /* hpll */
		/* divider: 1 */
		/* Disable the div output clock */
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 0, 19, 1);
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 0, 15, 1);

		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 1, 18, 1);
		/* Enable the final output clock */
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 1, 19, 1);
		sel = 0;
	} else { /* gp0_pll */
		sel = 1;
	}

	/* xd: 55 */
	/* setup the XD divider value */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_div, (55 - 1), VCLK_XD0, 8);
	//udelay(5);
	/*0x59[16]/0x5f[19]/0x5f[20]*/
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl, sel, VCLK_CLK_IN_SEL, 3);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl, 1, VCLK_EN0, 1);
	//udelay(2);

	/* vclk: 27M */
	/* [31:28]=0 enci_clk_sel, select vclk_div1 */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_div, 0, 28, 4);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_div, 0, 28, 4);
	/* release vclk_div_reset and enable vclk_div */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_div, 1, VCLK_XD_EN, 2);
	//udelay(5);

	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl, 1, VCLK_DIV1_EN, 1);

	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl, 1, VCLK_SOFT_RST, 1);
	//udelay(10);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl, 0, VCLK_SOFT_RST, 1);
	//udelay(5);
}

static void cvbs_set_vid2_clk(unsigned int src_pll)
{
	int sel = 0;

	if (!cvbs_drv.data) {
		printf("cvbs: error: %s: no cvbs data\n", __func__);
		return;
	}

	printf("%s\n", __func__);
	if (src_pll == 0) { /* hpll */
		/* divider: 1 */
		/* Disable the div output clock */
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 0, 19, 1);
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 0, 15, 1);

		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 1, 18, 1);
		/* Enable the final output clock */
		cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_pll_clk_div, 1, 19, 1);
		sel = 0;
	} else { /* gp0_pll */
		sel = 1;
	}

	/* xd: 55 */
	/* setup the XD divider value */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_div, (55 - 1), VCLK2_XD, 8);
	//udelay(5);
	/* Bit[18:16] - v2_cntl_clk_in_sel: vid_pll */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_ctrl, sel, VCLK2_CLK_IN_SEL, 3);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_ctrl, 1, VCLK2_EN, 1);
	//udelay(2);

	/* vclk: 27M */
	/* [31:28]=8 enci_clk_sel, select vclk2_div1 */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_div, 8, 28, 4);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_div, 8, 28, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_div, 1, VCLK2_XD_EN, 2);
	//udelay(5);

	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_ctrl, 1, VCLK2_DIV1_EN, 1);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_ctrl, 1, VCLK2_SOFT_RST, 1);
	//udelay(10);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid2_clk_ctrl, 0, VCLK2_SOFT_RST, 1);
	//udelay(5);
}

static int cvbs_config_clock(void)
{
	if (!cvbs_drv.data) {
		printf("cvbs: error: %s: no cvbs data\n", __func__);
		return -1;
	}

	/* pll output 1485M */
	switch (cvbs_drv.data->chip_type) {
	case CVBS_CHIP_G12A:
	case CVBS_CHIP_G12B:
		if (s_enci_clk_path & 0x1)
			cvbs_config_gp0pll_g12a();
		else
			cvbs_config_hdmipll_g12a();
		if (s_enci_clk_path & 0x2)
			cvbs_set_vid1_clk(s_enci_clk_path & 0x1);
		else
			cvbs_set_vid2_clk(s_enci_clk_path & 0x1);
		break;
	case CVBS_CHIP_SC2:
	case CVBS_CHIP_S4:
	case CVBS_CHIP_S4D:
		cvbs_config_hdmipll_sc2();
		cvbs_set_vid2_clk(0);
		break;
	case CVBS_CHIP_S1A:
		cvbs_config_hdmipll_s1a();
		cvbs_set_vid2_clk(0);
		break;
	case CVBS_CHIP_S7:
		/* hdmi_clk_out2: 1485Mhz */
		cvbs_s7_htxpll_clk_out(5940000, 4);
		/* 1485Mhz / 55 = 27Mhz */
		cvbs_set_vid2_clk(0);
		break;
	case CVBS_CHIP_S7D:
		/* hdmi_clk_out2: 1485Mhz */
		cvbs_s7d_htxpll_clk_out(5940000, 4);
		/* 1485Mhz / 55 = 27Mhz */
		cvbs_set_vid2_clk(0);
		break;
	default:
		printf("cvbs: %s: invalid chip type\n", __func__);
		return -1;
	}

	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl2, 1, 0, 1);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl2, 1, 4, 1);

	return 0;
}

static void cvbs_disable_clock(void)
{
	if (!cvbs_drv.data) {
		printf("cvbs: error: %s: no cvbs data\n", __func__);
		return;
	}

	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl2, 0, 4, 1);
	cvbs_set_hiu_bits(cvbs_drv.data->reg_vid_clk_ctrl2, 0, 0, 1);
}

/*----------------------------------------------------------------------------*/
// configuration for enci
static void cvbs_performance_enhancement(int mode)
{
	const struct reg_s *s = NULL;
	struct performance_config_s *perfconf = NULL;
	int i = 0;

	switch (mode) {
	case VMODE_PAL:
		if (cvbs_drv.data->sva_val)
			perfconf = &cvbs_drv.perf_conf_pal_sva;
		else
			perfconf = &cvbs_drv.perf_conf_pal;
		break;
	case VMODE_NTSC:
	case VMODE_NTSC_M:
		if (cvbs_drv.data->ntsc_ttc)
			perfconf = &cvbs_drv.perf_conf_ntsc_ttc;
		else
			perfconf = &cvbs_drv.perf_conf_ntsc;
		break;
	default:
		break;
	}
	if (!perfconf)
		return;

	if (!perfconf->reg_table) {
		printf("no performance table\n");
		return;
	}

	i = 0;
	s = perfconf->reg_table;
	while (i < perfconf->reg_cnt) {
		cvbs_write_vcbus(s->reg, s->val);
		//printf("vcbus reg[0x%04x] = 0x%08x\n", s->reg, s->val);

		s++;
		i++;
	}

	printf("%s\n", __func__);
}

static int cvbs_config_enci(int vmode)
{
	const struct reg_s *s = NULL;

	switch (vmode) {
	case VMODE_PAL:
		s = &tvregs_576cvbs_enc[0];
		break;
	case VMODE_NTSC:
	case VMODE_NTSC_M:
		s = &tvregs_480cvbs_enc[0];
		break;
	case VMODE_PAL_M:
		s = &tvregs_pal_m_enc[0];
		break;
	case VMODE_PAL_N:
		s = &tvregs_pal_n_enc[0];
		break;
	default:
		break;
	}
	if (s == NULL)
		return -1;

	while ((s->reg != MREG_END_MARKER)) {
		cvbs_write_vcbus(s->reg, s->val);
		//printf("reg[0x%.2x] = 0x%.4x\n", s->reg, s->val);
		s ++;
	}

	cvbs_performance_enhancement(vmode);

	return 0;
}

/*----------------------------------------------------------------------------*/
// configuration for output
// output vmode: 576cvbs, 480cvbs
int cvbs_set_vmode(char* vmode_name)
{
	if (!strncmp(vmode_name, "576cvbs", strlen("576cvbs"))) {
		cvbs_mode = VMODE_PAL;
		cvbs_config_enci(0);
		cvbs_config_clock();
		cvbs_set_vdac(1);
		return 0;
	} else if (!strncmp(vmode_name, "480cvbs", strlen("480cvbs"))) {
		cvbs_mode = VMODE_NTSC;
		cvbs_config_enci(1);
		cvbs_config_clock();
		cvbs_set_vdac(1);
		return 0;
	} else if (!strncmp(vmode_name, "ntsc_m", strlen("ntsc_m"))) {
		cvbs_mode = VMODE_NTSC_M;
		cvbs_config_enci(VMODE_NTSC_M);
		cvbs_config_clock();
		cvbs_set_vdac(1);
		return 0;
	} else if (!strncmp(vmode_name, "pal_m", strlen("pal_m"))) {
		cvbs_mode = VMODE_PAL_M;
		cvbs_config_enci(VMODE_PAL_M);
		cvbs_config_clock();
		cvbs_set_vdac(1);
		return 0;
	} else if (!strncmp(vmode_name, "pal_n", strlen("pal_n"))) {
		cvbs_mode = VMODE_PAL_N;
		cvbs_config_enci(VMODE_PAL_N);
		cvbs_config_clock();
		cvbs_set_vdac(1);
		return 0;
	} else if (!strncmp(vmode_name, "disable", strlen("disable"))) {
		cvbs_set_vdac(0);
		cvbs_write_vcbus(ENCI_VIDEO_EN, 0);
		cvbs_disable_clock();
		return 0;
	} else {
		printf("[%s] is invalid for cvbs.\n", vmode_name);
		return -1;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
#define CVBS_MODE_CNT    5
static char *cvbs_mode_str[CVBS_MODE_CNT] = {
	"576cvbs",
	"480cvbs",
	"ntsc_m",
	"pal_m",
	"pal_n",
};

/***********************************************
 * parameters:  vmode_name, such as 576cvbs, 480cvbs...
 * return:      viu_mux
 ************************************************/
unsigned int cvbs_outputmode_check(char *vmode_name)
{
	unsigned int i;

	for (i = 0; i < CVBS_MODE_CNT; i++) {
		if (!strncmp(vmode_name, cvbs_mode_str[i], strlen(cvbs_mode_str[i])))
			return VIU_MUX_ENCI;
	}

	//printf("cvbs: outputmode[%s] is invalid\n", vmode_name);
	return VIU_MUX_MAX;
}

// list for valid video mode
void cvbs_show_valid_vmode(void)
{
	unsigned int i;

	for (i = 0; i < CVBS_MODE_CNT; i++)
		printf("%s\n", cvbs_mode_str[i]);
}

static char *cvbsout_performance_str[] = {
	"performance", /* default for pal */
	"performance_pal",
	"performance_ntsc",
	"performance_ntsc_ttc"
};

static void cvbs_get_config(void)
{
	const void *dt_blob = NULL;
	int node;
	char *propdata;
	const char *str;
	struct reg_s *s;
	unsigned int i, j, temp, cnt;
	int ret;

	dt_blob = gd->fdt_blob;
	if (!dt_blob) {
		printf("cvbs: error: dt_blob is null, load default setting\n");
		return;
	}

	ret = fdt_check_header(dt_blob);
	if (ret < 0) {
		printf("cvbs: error: check dts: %s, load default setting\n",
			fdt_strerror(ret));
		return;
	}

	node = fdt_path_offset(dt_blob, "/cvbsout");
	if (node < 0) {
		printf("not find /cvbsout node: %s\n",
			fdt_strerror(node));
		return;
	}

	/* clk_path */
	propdata = (char *)fdt_getprop(dt_blob, node, "clk_path", NULL);
	if (propdata) {
		s_enci_clk_path = be32_to_cpup((u32*)propdata);
		printf("cvbs: find clk_path: 0x%x\n", s_enci_clk_path);
	}

	propdata = (char *)fdt_getprop(dt_blob, node, "sva_std", NULL);
	if (propdata) {
		cvbs_drv.data->sva_val = be32_to_cpup((u32 *)propdata);
		printf("cvbs: find sva_std: 0x%x\n", cvbs_drv.data->sva_val);
	} else {
		cvbs_drv.data->sva_val = 0;
	}

	propdata = (char *)fdt_getprop(dt_blob, node, "ntsc_ttc", NULL);
	if (propdata) {
		cvbs_drv.data->ntsc_ttc = be32_to_cpup((u32 *)propdata);
		printf("cvbs: find ntsc_ttc: 0x%x\n", cvbs_drv.data->ntsc_ttc);
	} else {
		cvbs_drv.data->ntsc_ttc = 0;
	}

	/* performance: PAL CTCC */
	str = cvbsout_performance_str[1];
	propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
	if (!propdata) {
		str = cvbsout_performance_str[0];
		propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
		if (!propdata)
			goto cvbs_performance_config_ntsc;
	}
	cnt = 0;
	while (cnt < CVBS_PERFORMANCE_CNT_MAX) {
		j = 2 * cnt;
		temp = be32_to_cpup((((u32*)propdata)+j));
		if (temp == MREG_END_MARKER) /* ending */
			break;
		cnt++;
	}
	if (cnt >= CVBS_PERFORMANCE_CNT_MAX)
		cnt = 0;
	if (cnt > 0) {
		printf("cvbs: find %s config\n", str);
		cvbs_drv.perf_conf_pal.reg_table = malloc(sizeof(struct reg_s) * cnt);
		if (!cvbs_drv.perf_conf_pal.reg_table) {
			printf("cvbs: error: failed to alloc %s table\n", str);
			cnt = 0;
		}
		memset(cvbs_drv.perf_conf_pal.reg_table, 0, (sizeof(struct reg_s) * cnt));
		cvbs_drv.perf_conf_pal.reg_cnt = cnt;

		i = 0;
		s = cvbs_drv.perf_conf_pal.reg_table;
		while (i < cvbs_drv.perf_conf_pal.reg_cnt) {
			j = 2 * i;
			s->reg = be32_to_cpup((((u32*)propdata)+j));
			s->val = be32_to_cpup((((u32*)propdata)+j+1));
			/* printf("%p: 0x%08x = 0x%x\n", s, s->reg, s->val); */

			s++;
			i++;
		}
	}

	/* performance: PAL SVA */
	str = cvbsout_performance_str[0];
	propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
	if (!propdata) {
		str = cvbsout_performance_str[1];
		propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
		if (!propdata)
			goto cvbs_performance_config_ntsc;
	}
	cnt = 0;
	while (cnt < CVBS_PERFORMANCE_CNT_MAX) {
		j = 2 * cnt;
		temp = be32_to_cpup((((u32 *)propdata) + j));
		if (temp == MREG_END_MARKER) /* ending */
			break;
		cnt++;
	}
	if (cnt >= CVBS_PERFORMANCE_CNT_MAX)
		cnt = 0;
	if (cnt > 0) {
		printf("cvbs: find %s config\n", str);
		cvbs_drv.perf_conf_pal_sva.reg_table = malloc(sizeof(struct reg_s) * cnt);
		if (!cvbs_drv.perf_conf_pal_sva.reg_table) {
			printf("cvbs: error: failed to alloc %s table\n", str);
			cnt = 0;
		}
		memset(cvbs_drv.perf_conf_pal_sva.reg_table, 0, (sizeof(struct reg_s) * cnt));
		cvbs_drv.perf_conf_pal_sva.reg_cnt = cnt;

		i = 0;
		s = cvbs_drv.perf_conf_pal_sva.reg_table;
		while (i < cvbs_drv.perf_conf_pal_sva.reg_cnt) {
			j = 2 * i;
			s->reg = be32_to_cpup((((u32 *)propdata) + j));
			s->val = be32_to_cpup((((u32 *)propdata) + j + 1));
			/* printf("%p: 0x%08x = 0x%x\n", s, s->reg, s->val); */

			s++;
			i++;
		}
	}

	/* performance: NTSC */
cvbs_performance_config_ntsc:
	str = cvbsout_performance_str[2];
	propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
	if (!propdata)
		return;
	cnt = 0;
	while (cnt < CVBS_PERFORMANCE_CNT_MAX) {
		j = 2 * cnt;
		temp = be32_to_cpup((((u32*)propdata)+j));
		if (temp == MREG_END_MARKER) /* ending */
			break;
		cnt++;
	}
	if (cnt >= CVBS_PERFORMANCE_CNT_MAX)
		cnt = 0;
	if (cnt > 0) {
		printf("cvbs: find %s config\n", str);
		cvbs_drv.perf_conf_ntsc.reg_table = malloc(sizeof(struct reg_s) * cnt);
		if (!cvbs_drv.perf_conf_ntsc.reg_table) {
			printf("cvbs: error: failed to alloc %s table\n", str);
			cnt = 0;
		}
		memset(cvbs_drv.perf_conf_ntsc.reg_table, 0, (sizeof(struct reg_s) * cnt));
		cvbs_drv.perf_conf_ntsc.reg_cnt = cnt;

		i = 0;
		s = cvbs_drv.perf_conf_ntsc.reg_table;
		while (i < cvbs_drv.perf_conf_ntsc.reg_cnt) {
			j = 2 * i;
			s->reg = be32_to_cpup((((u32*)propdata)+j));
			s->val = be32_to_cpup((((u32*)propdata)+j+1));
			/* printf("%p: 0x%08x = 0x%x\n", s, s->reg, s->val); */

			s++;
			i++;
		}
	}

	str = cvbsout_performance_str[3];
	propdata = (char *)fdt_getprop(dt_blob, node, str, NULL);
	if (!propdata)
		return;
	cnt = 0;
	while (cnt < CVBS_PERFORMANCE_CNT_MAX) {
		j = 2 * cnt;
		temp = be32_to_cpup((((u32 *)propdata) + j));
		if (temp == MREG_END_MARKER) /* ending */
			break;
		cnt++;
	}
	if (cnt >= CVBS_PERFORMANCE_CNT_MAX)
		cnt = 0;
	if (cnt > 0) {
		printf("cvbs: find %s config\n", str);
		cvbs_drv.perf_conf_ntsc_ttc.reg_table = malloc(sizeof(struct reg_s) * cnt);
		if (!cvbs_drv.perf_conf_ntsc_ttc.reg_table) {
			printf("cvbs: error: failed to alloc %s table\n", str);
			cnt = 0;
		}
		memset(cvbs_drv.perf_conf_ntsc_ttc.reg_table, 0, (sizeof(struct reg_s) * cnt));
		cvbs_drv.perf_conf_ntsc_ttc.reg_cnt = cnt;

		i = 0;
		s = cvbs_drv.perf_conf_ntsc_ttc.reg_table;
		while (i < cvbs_drv.perf_conf_ntsc_ttc.reg_cnt) {
			j = 2 * i;
			s->reg = be32_to_cpup((((u32 *)propdata) + j));
			s->val = be32_to_cpup((((u32 *)propdata) + j + 1));
			/* printf("%p: 0x%08x = 0x%x\n", s, s->reg, s->val); */

			s++;
			i++;
		}
	}
}

void vdac_data_config(void)
{
	printf("cvbs: cpuid:0x%x\n", get_cpu_id().family_id);
	switch (get_cpu_id().family_id) {
	case MESON_CPU_MAJOR_ID_G12A:
		cvbs_drv.data = &cvbs_data_g12a;
		break;
	case MESON_CPU_MAJOR_ID_G12B:
		cvbs_drv.data = &cvbs_data_g12b;
		break;
	case MESON_CPU_MAJOR_ID_SC2:
		cvbs_drv.data = &cvbs_data_sc2;
		break;
	case MESON_CPU_MAJOR_ID_S4:
		cvbs_drv.data = &cvbs_data_s4;
		break;
	case MESON_CPU_MAJOR_ID_S4D:
		cvbs_drv.data = &cvbs_data_s4d;
		break;
	case MESON_CPU_MAJOR_ID_S1A:
		cvbs_drv.data = &cvbs_data_s1a;
		break;
	case MESON_CPU_MAJOR_ID_S7:
		cvbs_drv.data = &cvbs_data_s7;
		break;
	case MESON_CPU_MAJOR_ID_S7D:
		cvbs_drv.data = &cvbs_data_s7d;
		break;
	default:
		cvbs_drv.data = &cvbs_data_s4d;
		break;
	}
}

void cvbs_init(void)
{
	vdac_data_config();
	vdac_ctrl_config_probe();
	cvbs_get_config();
}

