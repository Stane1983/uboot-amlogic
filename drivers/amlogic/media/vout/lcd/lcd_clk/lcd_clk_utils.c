// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_clk_config.h"
#include "lcd_clk_ctrl.h"
#include "lcd_clk_utils.h"

static unsigned int edp_div0_table[15] = {
	1, 2, 3, 4, 5, 7, 8, 9, 11, 13, 17, 19, 23, 29, 31
};

static unsigned int edp_div1_table[10] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 13
};

static char *lcd_clk_div_sel_table[] = {
	"1",
	"2",
	"3",
	"3.5",
	"3.75",
	"4",
	"5",
	"6",
	"6.25",
	"7",
	"7.5",
	"12",
	"14",
	"15",
	"2.5",
	"4.67",
	"2.33",
	"invalid",
};

unsigned int lcd_clk_div_table[][5] = {//shifter:0=12, 1=14, 2=15
	/* divider,        shift_val,  shift_sel, num, den */
	{CLK_DIV_SEL_1,    0xffff,     0,         1,   1 },
	{CLK_DIV_SEL_2,    0x0aaa,     0,         1,   2 },
	{CLK_DIV_SEL_3,    0x0db6,     0,         1,   3 },
	{CLK_DIV_SEL_3p5,  0x36cc,     1,         2,   7 },
	{CLK_DIV_SEL_3p75, 0x6666,     2,         4,   15},
	{CLK_DIV_SEL_4,    0x0ccc,     0,         1,   4 },
	{CLK_DIV_SEL_5,    0x739c,     2,         1,   5 },
	{CLK_DIV_SEL_6,    0x0e38,     0,         1,   6 },
	{CLK_DIV_SEL_6p25, 0x0000,     3,         4,   25},
	{CLK_DIV_SEL_7,    0x3c78,     1,         1,   7 },
	{CLK_DIV_SEL_7p5,  0x78f0,     2,         2,   15},
	{CLK_DIV_SEL_12,   0x0fc0,     0,         1,   12},
	{CLK_DIV_SEL_14,   0x3f80,     1,         1,   14},
	{CLK_DIV_SEL_15,   0x7f80,     2,         1,   15},
	{CLK_DIV_SEL_2p5,  0x5294,     2,         2,   5 },
	{CLK_DIV_SEL_4p67, 0x0ccc,     1,         3,   14},
	{CLK_DIV_SEL_2p33, 0x1aaa,     1,         3,   7 },
	{CLK_DIV_SEL_MAX,  0xffff,     0,         1,   1 },
};

static const unsigned int od_fb_table[2] = {1, 2};
static const unsigned int od_table[6] = {1, 2, 4, 8, 16, 32};
static const unsigned int tcon_div_table[5] = {1, 2, 4, 8, 16};

/* **********************************
 * lcd controller operation
 * **********************************/
int lcd_clk_msr_check(int msr_id, unsigned int freq)
{
	unsigned int encl_clk_msr;

	if (msr_id == -1)
		return 0;

	encl_clk_msr = clk_util_clk_msr(msr_id) * 1000000;
	if (lcd_diff(freq, encl_clk_msr) >= PLL_CLK_CHECK_MAX) {
		LCDERR("%s[%d]: msr_id, expected:%d, msr:%d\n",
		       __func__, msr_id, freq, encl_clk_msr);
		return -1;
	}

	return 0;
}

int lcd_pll_ss_level_generate(struct lcd_clk_config_s *cconf)
{
	unsigned int dep_sel, str_m, err, min, done = 0;
	unsigned long long target, ss_ppm, dep_base;

	if (!cconf)
		return -1;

	target = cconf->ss_level;
	target *= 1000;
	min = cconf->data->ss_dep_base * 5;
	dep_base = cconf->data->ss_dep_base;
	for (str_m = 1; str_m <= cconf->data->ss_str_m_max; str_m++) { //str_m
		for (dep_sel = 1; dep_sel <= cconf->data->ss_dep_sel_max; dep_sel++) { //dep_sel
			ss_ppm = dep_sel * str_m * dep_base;
			if (ss_ppm > target)
				break;
			err = target - ss_ppm;
			if (err < min) {
				min = err;
				cconf->ss_dep_sel = dep_sel;
				cconf->ss_str_m = str_m;
				cconf->ss_ppm = ss_ppm;
				done++;
			}
		}
	}
	if (done == 0) {
		LCDERR("%s: invalid ss_level %d\n", __func__, cconf->ss_level);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("%s: dep_sel=%d, str_m=%d, error=%d\n",
			__func__, cconf->ss_dep_sel, cconf->ss_str_m, min);
	}

	return 0;
}

int lcd_pll_wait_lock(unsigned int reg, unsigned int lock_bit)
{
	unsigned int pll_lock;
	int wait_loop = PLL_WAIT_LOCK_CNT; /* 200 */
	int ret = 0;

	do {
		udelay(50);
		pll_lock = lcd_ana_getb(reg, lock_bit, 1);
		wait_loop--;
	} while ((pll_lock == 0) && (wait_loop > 0));
	if (pll_lock == 0)
		ret = -1;
	LCDPR("%s: pll_lock=%d, wait_loop=%d\n",
	      __func__, pll_lock, (PLL_WAIT_LOCK_CNT - wait_loop));

	return ret;
}

/* ****************************************************
 * lcd clk parameters calculate
 * ****************************************************
 */
unsigned long long clk_vid_pll_div_calc(unsigned long long clk, unsigned int div_sel, int dir)
{
	unsigned long long clk_ret, num, den;

	if (div_sel >= CLK_DIV_SEL_MAX) {
		LCDERR("clk_div_sel: Invalid parameter\n");
		return 0;
	}

	if (dir == CLK_DIV_I2O) {
		num = lcd_clk_div_table[div_sel][3];
		den = lcd_clk_div_table[div_sel][4];
	} else {
		num = lcd_clk_div_table[div_sel][4];
		den = lcd_clk_div_table[div_sel][3];
	}
	clk_ret = div_around(clk * num, den);

	return clk_ret;
}

int lcd_pll_get_frac(struct lcd_clk_config_s *cconf, unsigned long long pll_fvco)
{
	unsigned int frac_range, frac, offset;
	unsigned long long fvco_calc, temp;

	frac_range = cconf->data->pll_frac_range;

	fvco_calc = lcd_do_div(pll_fvco, od_fb_table[cconf->pll_od_fb]);
	temp = cconf->fin;
	temp = lcd_do_div((temp * cconf->pll_m), cconf->pll_n);
	if (fvco_calc >= temp) {
		temp = fvco_calc - temp;
		offset = 0;
	} else {
		temp = temp - fvco_calc;
		offset = 1;
	}
	if (temp >= (2 * cconf->fin)) {
		LCDERR("%s: pll changing %lldHz is too much\n", __func__, temp);
		return -1;
	}

	frac = lcd_do_div((temp * frac_range * cconf->pll_n * 10), cconf->fin) + 5;
	frac /= 10;
	if (cconf->pll_mode & LCD_PLL_MODE_FRAC_SHIFT) {
		if ((frac == (frac_range >> 1)) || (frac == (frac_range >> 2))) {
			frac |= 0x66;
			cconf->pll_frac_half_shift = 1;
		} else {
			cconf->pll_frac_half_shift = 0;
		}
	}
	cconf->pll_frac = frac | (offset << cconf->data->pll_frac_sign_bit);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: 0x%x\n", __func__, cconf->pll_frac);

	return 0;
}

/******* calculate 1od and 3od setting @pll_od_setting_generate *******/
static int generate_pll_3od_setting(struct lcd_clk_config_s *cconf, unsigned long long pll_fout)
{
	struct lcd_clk_data_s *data = cconf->data;
	unsigned int m, n;
	unsigned int od1_sel, od2_sel, od3_sel, od1, od2, od3;
	unsigned long long pll_fod2_in, pll_fod3_in, pll_fvco, temp;
	unsigned int od_fb = 0, pll_frac, frac_range;
	int done;

	done = 0;
	if (pll_fout > data->pll_out_fmax || pll_fout < data->pll_out_fmin)
		return done;

	frac_range = data->pll_frac_range;
	for (od3_sel = data->pll_od_sel_max; od3_sel > 0; od3_sel--) {
		od3 = od_table[od3_sel - 1];
		pll_fod3_in = pll_fout * od3;
		for (od2_sel = od3_sel; od2_sel > 0; od2_sel--) {
			od2 = od_table[od2_sel - 1];
			pll_fod2_in = pll_fod3_in * od2;
			for (od1_sel = od2_sel; od1_sel > 0; od1_sel--) {
				od1 = od_table[od1_sel - 1];
				pll_fvco = pll_fod2_in * od1;
				if (pll_fvco < data->pll_vco_fmin ||
					pll_fvco > data->pll_vco_fmax) {
					continue;
				}
				cconf->pll_od1_sel = od1_sel - 1;
				cconf->pll_od2_sel = od2_sel - 1;
				cconf->pll_od3_sel = od3_sel - 1;
				cconf->pll_fout = pll_fout;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("od1=%d, od2=%d, od3=%d, pll_fvco=%lld\n",
						(od1_sel - 1), (od2_sel - 1),
						(od3_sel - 1), pll_fvco);
				}
				cconf->pll_fvco = pll_fvco;
				n = 1;
				od_fb = cconf->pll_od_fb;
				pll_fvco = lcd_do_div(pll_fvco, od_fb_table[od_fb]);
				m = lcd_do_div(pll_fvco, cconf->fin);
				temp = cconf->fin;
				temp *= m;
				temp = pll_fvco - temp;
				pll_frac = lcd_do_div((temp * frac_range * 10), cconf->fin) + 5;
				pll_frac /= 10;
				if (cconf->pll_mode & LCD_PLL_MODE_FRAC_SHIFT) {
					if ((pll_frac == (frac_range >> 1)) ||
					    (pll_frac == (frac_range >> 2))) {
						pll_frac |= 0x66;
						cconf->pll_frac_half_shift = 1;
					} else {
						cconf->pll_frac_half_shift = 0;
					}
				}
				cconf->pll_m = m;
				cconf->pll_n = n;
				cconf->pll_frac = pll_frac;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
					LCDPR("m=%d, n=%d, frac=0x%x\n", m, n, pll_frac);
				done = 1;
				break;
			}
		}
	}
	return done;
}

static int generate_pll_1od_setting(struct lcd_clk_config_s *cconf, unsigned long long pll_fout)
{
	struct lcd_clk_data_s *data = cconf->data;
	unsigned int m, n, od_sel, od;
	unsigned long long pll_fvco, temp;
	unsigned int od_fb = 0, pll_frac;
	int done = 0;

	if (pll_fout > data->pll_out_fmax || pll_fout < data->pll_out_fmin)
		return done;

	for (od_sel = data->pll_od_sel_max; od_sel > 0; od_sel--) {
		od = od_table[od_sel - 1];
		pll_fvco = pll_fout * od;
		if (pll_fvco < data->pll_vco_fmin || pll_fvco > data->pll_vco_fmax)
			continue;
		cconf->pll_od1_sel = od_sel - 1;
		cconf->pll_fout = pll_fout;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("od_sel=%d, pll_fvco=%lld\n", (od_sel - 1), pll_fvco);

		cconf->pll_fvco = pll_fvco;
		n = 1;
		od_fb = cconf->pll_od_fb;
		pll_fvco = lcd_do_div(pll_fvco, od_fb_table[od_fb]);
		m = lcd_do_div(pll_fvco, cconf->fin);
		temp = cconf->fin;
		temp *= m;
		temp = pll_fvco - temp;
		pll_frac = lcd_do_div((temp * data->pll_frac_range * 10), cconf->fin) + 5;
		pll_frac /= 10;
		cconf->pll_m = m;
		cconf->pll_n = n;
		cconf->pll_frac = pll_frac;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_m=%d, pll_n=%d, pll_frac=0x%x\n", m, n, pll_frac);
		done = 1;
		break;
	}
	return done;
}

static int pll_od_setting_generate(struct lcd_clk_config_s *cconf, unsigned long long pll_fout)
{
	if (cconf->data->od_cnt == 3)
		return generate_pll_3od_setting(cconf, pll_fout);
	else
		return generate_pll_1od_setting(cconf, pll_fout);
}

/***** calculate 1od and 3od setting @pll_od_setting_generate Done *****/

static int check_vco(struct lcd_clk_config_s *cconf, unsigned long long pll_fvco)
{
	struct lcd_clk_data_s *data = cconf->data;
	unsigned int m, n;
	unsigned int od_fb = 0, pll_frac;
	unsigned long long temp;
	int done = 0;

	if (pll_fvco < data->pll_vco_fmin || pll_fvco > data->pll_vco_fmax) {
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_fvco %lld is out of range\n", pll_fvco);
		return done;
	}

	cconf->pll_fvco = pll_fvco;
	n = 1;
	od_fb = cconf->pll_od_fb;
	pll_fvco = lcd_do_div(pll_fvco, od_fb_table[od_fb]);
	m = lcd_do_div(pll_fvco, cconf->fin);
	temp = cconf->fin;
	temp *= m;
	temp = pll_fvco - temp;
	pll_frac = lcd_do_div((temp * data->pll_frac_range * 10), cconf->fin) + 5;
	pll_frac /= 10;
	cconf->pll_m = m;
	cconf->pll_n = n;
	cconf->pll_frac = pll_frac;
	if (cconf->pll_mode & LCD_PLL_MODE_FRAC_SHIFT) {
		if (pll_frac == (data->pll_frac_range >> 1) ||
		    pll_frac == (data->pll_frac_range >> 2)) {
			pll_frac |= 0x66;
			cconf->pll_frac_half_shift = 1;
		} else {
			cconf->pll_frac_half_shift = 0;
		}
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("m=%d, n=%d, frac=0x%x, pll_fvco=%lld\n",
		      m, n, pll_frac, pll_fvco);
	}
	done = 1;

	return done;
}

static int check_3od(struct lcd_clk_config_s *cconf, unsigned long long pll_fout)
{
	struct lcd_clk_data_s *data = cconf->data;
	unsigned int od1_sel, od2_sel, od3_sel, od1, od2, od3;
	unsigned long long pll_fod2_in, pll_fod3_in, pll_fvco;
	int done = 0;

	if (pll_fout > data->pll_out_fmax || pll_fout < data->pll_out_fmin)
		return done;

	for (od3_sel = data->pll_od_sel_max; od3_sel > 0; od3_sel--) {
		od3 = od_table[od3_sel - 1];
		pll_fod3_in = pll_fout * od3;
		for (od2_sel = od3_sel; od2_sel > 0; od2_sel--) {
			od2 = od_table[od2_sel - 1];
			pll_fod2_in = pll_fod3_in * od2;
			for (od1_sel = od2_sel; od1_sel > 0; od1_sel--) {
				od1 = od_table[od1_sel - 1];
				pll_fvco = pll_fod2_in * od1;
				if (pll_fvco < data->pll_vco_fmin ||
				    pll_fvco > data->pll_vco_fmax) {
					continue;
				}
				if (pll_fvco == cconf->pll_fvco) {
					cconf->pll_od1_sel = od1_sel - 1;
					cconf->pll_od2_sel = od2_sel - 1;
					cconf->pll_od3_sel = od3_sel - 1;
					cconf->pll_fout = pll_fout;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
						LCDPR("od1=%d, od2=%d, od3=%d\n",
						      (od1_sel - 1), (od2_sel - 1),
						      (od3_sel - 1));
					}
					done = 1;
					break;
				}
			}
		}
	}
	return done;
}

static int lcd_clk_generate_p2p_with_tcon_div(struct lcd_clk_config_s *cconf,
		unsigned long long bit_rate)
{
	unsigned long long pll_fout, pll_fvco, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd, tcon_div_sel = 0;
	int done = 0;

	for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
		pll_fvco = bit_rate * tcon_div_table[tcon_div_sel];
		done = check_vco(cconf, pll_fvco);
		if (done == 0)
			continue;
		for (xd = 1; xd <= cconf->data->xd_max; xd++) {
			clk_div_out = cconf->fout * xd;
			if (clk_div_out > cconf->data->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("fout=%d, xd=%d, clk_div_out=%d, tcon_div_sel=%d\n",
					cconf->fout, xd, clk_div_out, tcon_div_sel);
			}
			for (clk_div_sel = CLK_DIV_SEL_1; clk_div_sel < cconf->data->div_sel_max;
				clk_div_sel++) {
				clk_div_in = clk_vid_pll_div_calc(clk_div_out,
						clk_div_sel, CLK_DIV_O2I);
				if (clk_div_in > cconf->data->div_in_fmax)
					continue;
				cconf->xd = xd;
				cconf->div_sel = clk_div_sel;
				cconf->pll_div_fout = clk_div_out;
				cconf->pll_tcon_div_sel = tcon_div_sel;
				pll_fout = clk_div_in;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("clk_div_sel=%s(%d), pll_fout=%lld\n",
						lcd_clk_div_sel_table[clk_div_sel],
						clk_div_sel, pll_fout);
				}
				done = check_3od(cconf, pll_fout);
				if (done)
					goto p2p_clk_with_tcon_div_done;
			}
		}
	}

p2p_clk_with_tcon_div_done:
	return done;
}

static int lcd_clk_generate_p2p_without_tcon_div(struct lcd_clk_config_s *cconf,
		unsigned long long bit_rate)
{
	unsigned long long pll_fout, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd;
	int done = 0;

	for (xd = 1; xd <= cconf->data->xd_max; xd++) {
		clk_div_out = cconf->fout * xd;
		if (clk_div_out > cconf->data->div_out_fmax)
			continue;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("fout=%d, xd=%d, clk_div_out=%d\n",
				cconf->fout, xd, clk_div_out);
		}
		for (clk_div_sel = CLK_DIV_SEL_1; clk_div_sel < cconf->data->div_sel_max;
			clk_div_sel++) {
			clk_div_in = clk_vid_pll_div_calc(clk_div_out,
					clk_div_sel, CLK_DIV_O2I);
			if (clk_div_in > cconf->data->div_in_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("clk_div_sel=%s(%d), clk_div_in=%lld, bit_rate=%lld\n",
					lcd_clk_div_sel_table[clk_div_sel],
					clk_div_sel, clk_div_in, bit_rate);
			}
			if (clk_div_in == bit_rate) {
				cconf->xd = xd;
				cconf->div_sel = clk_div_sel;
				cconf->pll_div_fout = clk_div_out;
				pll_fout = clk_div_in;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
					LCDPR("pll_fout=%lld\n", pll_fout);

				done = pll_od_setting_generate(cconf, pll_fout);
				if (done)
					goto p2p_clk_without_tcon_div_done;
			}
		}
	}

p2p_clk_without_tcon_div_done:
	return done;
}

#define DSI_CLK_TB_SIZE 32
/********************** DSI 1PLL model **********************/
/* PLL_VCO / OD[1/3] / PLL_CLK_DIV(optional) == VID_PLL_CLK */
/* VID_PLL_CLK --> / enc_xd  == ENCL_clk                    */
/*      '--------> / PHY_div == PHY HS clk                  */
/************************************************************/
static unsigned char lcd_clk_generate_DSI_1PLL(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);

	if (!cconf)
		return 0;

	struct dsi_clk_tb_s {
		unsigned long long pllout;
		unsigned long long phy_clk;
		unsigned short enc_xd;
		unsigned short phy_n;
	};

	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	unsigned long long pll_out, phy_clk;
	unsigned short enc_xd, phy_N;
	unsigned char done, tb_idx = 0, x, new_high_bitrate;
	struct dsi_clk_tb_s *clk_div_tb;

	unsigned long long bitrate_min, bitrate_max;

	bitrate_min = lcd_dsi_get_min_bitrate(pdrv);
	bitrate_max = dconf->bit_rate_max;
	bitrate_max = bitrate_max * 1000000;

	clk_div_tb = (struct dsi_clk_tb_s *)malloc(DSI_CLK_TB_SIZE * sizeof(struct dsi_clk_tb_s));
	if (!clk_div_tb) {
		LCDERR("[%d]: %s: kcalloc failed\n", pdrv->index, __func__);
		return 0;
	}
	memset(clk_div_tb, 0, DSI_CLK_TB_SIZE * sizeof(struct dsi_clk_tb_s));

	cconf->pll_tcon_div_sel = 2;
	cconf->div_sel = CLK_DIV_SEL_1;

	for (enc_xd = 1; enc_xd < cconf->data->xd_max; enc_xd++) {
		pll_out = enc_xd;
		pll_out = pll_out * cconf->fout;
		if (pll_out > cconf->data->pll_out_fmax || pll_out < cconf->data->pll_out_fmin)
			continue;
		for (phy_N = 1; phy_N < cconf->data->phy_div_max; phy_N++) {
			phy_clk = div_around(pll_out, phy_N);
			if (phy_clk > bitrate_max || phy_clk < bitrate_min)
				continue;

			new_high_bitrate = 1;
			for (x = 0; x < tb_idx; x++) {
				if (phy_clk < clk_div_tb[x].phy_clk)
					new_high_bitrate = 0;
			}
			if (!new_high_bitrate)
				continue;

			if (tb_idx == DSI_CLK_TB_SIZE) {
				LCDERR("[%d]: dsi clk table full!\n", pdrv->index);
				goto dsi_clk_tabel_buffer_full;
			}

			clk_div_tb[tb_idx].pllout = pll_out;
			clk_div_tb[tb_idx].enc_xd = enc_xd;
			clk_div_tb[tb_idx].phy_n = phy_N;
			clk_div_tb[tb_idx].phy_clk = phy_clk;
			tb_idx++;
		}
	}

	if (!tb_idx) {
		LCDERR("[%d]: %s: no div for pll_out:(%lluHz~%lluHz), bit_rate:(%lluHz~%uMHz)\n",
		       pdrv->index, __func__, cconf->data->pll_out_fmin,
		       cconf->data->pll_out_fmax, bitrate_min, dconf->bit_rate_max);
		free(clk_div_tb);
		return 0;
	}

dsi_clk_tabel_buffer_full:
	x = tb_idx - 1;
	while (1) {
		done = pll_od_setting_generate(cconf, clk_div_tb[x].pllout);
		if (done || x == 0)
			break;
		x--;
	}
	if (!done) {
		LCDERR("[%d]: %s: no pll setting available\n", pdrv->index, __func__);
		free(clk_div_tb);
		return 0;
	}

	LCDPR("[%d]: fvco=%lluHz -> pll_out:%lluHz: xd[%hu]->fout=%uhz, div[%hu]->phy=%lluhz\n",
		pdrv->index, cconf->pll_fvco, clk_div_tb[x].pllout,
		clk_div_tb[x].enc_xd, cconf->fout, clk_div_tb[x].phy_n, clk_div_tb[x].phy_clk);

	cconf->phy_clk = clk_div_tb[x].phy_clk;
	pdrv->config.timing.bit_rate = clk_div_tb[x].phy_clk;

	cconf->phy_div = clk_div_tb[x].phy_n;
	cconf->xd = clk_div_tb[x].enc_xd; //PLL2enc

	// should lane_byte_clk = (be phy_clk == phy_bitrate / 2) / 4
	dconf->lane_byte_clk = div_around(clk_div_tb[x].phy_clk, 8);

	// should lane_byte_clk = (be phy_clk == phy_bitrate / 2) / pclk
	dconf->factor_numerator = cconf->xd;
	dconf->factor_denominator = cconf->phy_div * 8;

	free(clk_div_tb);
	return 1;
}

/* Func: lcd_DP_1PLL_clk_para_cal, strategy: keep phy clk, find the closest pclk */
/************************* eDP 1PLL model T7 ***********************/
/* PLL_VCO / tcon_div / edp_div0&1 / PLL_CLK_DIV == VID_PLL_CLK */
/*              '--> eDP_PHY_clk   .---------------------'      */
/*                                 '--> / enc_xd == ENCL_clk    */
/****************************************************************/
static int lcd_clk_generate_DP_1PLL(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	unsigned char clk_level_sel = 0;
	unsigned int edp_div0, edp_div1, tmp_div;
	unsigned int min_err_div0 = 0, min_err_div1 = 0, min_err_div = 0;
	unsigned long long tmp_fout, error, min_err = 1000000000;
	unsigned long long eDP_PLL_setting_t7[2][3] = {
		{135, 3240000000ULL, 1620000000ULL},
		{225, 5400000000ULL, 2700000000ULL},
	};

	if (!cconf)
		return 0;

	if (pdrv->config.control.edp_cfg.link_rate == 0x0a) //2.7G
		clk_level_sel = 1;
	else
		clk_level_sel = 0;

	cconf->pll_m    = eDP_PLL_setting_t7[clk_level_sel][0];
	cconf->pll_fvco = eDP_PLL_setting_t7[clk_level_sel][1];
	cconf->pll_fout = eDP_PLL_setting_t7[clk_level_sel][2];
	cconf->phy_clk  = eDP_PLL_setting_t7[clk_level_sel][2];
	cconf->pll_n    = 1;
	cconf->pll_frac = 0;
	cconf->pll_od1_sel = 1;
	cconf->pll_od2_sel = 0;
	cconf->pll_od3_sel = 0;
	cconf->pll_tcon_div_sel = 1;
	cconf->pll_frac_half_shift = 0;
	cconf->div_sel = CLK_DIV_SEL_1;
	cconf->xd = 1;

	for (edp_div0 = 0; edp_div0 < 15; edp_div0++) {
		for (edp_div1 = 0; edp_div1 < 10; edp_div1++) {
			tmp_div = edp_div0_table[edp_div0] * edp_div1_table[edp_div1];
			tmp_fout = lcd_do_div(cconf->pll_fout, tmp_div);
			error = lcd_diff(tmp_fout, cconf->fout);
			if (error >= min_err)
				continue;

			min_err = error;
			min_err_div0 = edp_div0;
			min_err_div1 = edp_div1;
			min_err_div = tmp_div;
		}
	}
	cconf->err_fmin = min_err;
	cconf->edp_div0 = min_err_div0;
	cconf->edp_div1 = min_err_div1;

	cconf->fout = lcd_do_div(cconf->pll_fout, min_err_div);
	pdrv->config.timing.enc_clk = cconf->fout;

	LCDPR("[%d]: PLL_out=%llu div=%u [%u, %u] fout:%u enc_clk=%u error=%llu\n",
		pdrv->index, cconf->pll_fout, min_err_div, edp_div0_table[min_err_div0],
		edp_div1_table[min_err_div1], cconf->fout, cconf->fout, min_err);
	return 1;
}

static int lcd_pll_frac_generate_dependence(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned long long pll_fout, pll_fvco, clk_div_in;
	unsigned int enc_clk, clk_div_out, clk_div_sel;
	unsigned int od1 = 1, od2 = 1, od3 = 1;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return -1;

	enc_clk = pconf->timing.enc_clk;
	clk_div_sel = cconf->div_sel;
	if (cconf->data->od_cnt == 3) {
		od1 = od_table[cconf->pll_od1_sel];
		od2 = od_table[cconf->pll_od2_sel];
		od3 = od_table[cconf->pll_od3_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("m=%d, od1=%d, od2=%d, od3=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_m, cconf->pll_od1_sel,
				cconf->pll_od2_sel, cconf->pll_od3_sel,
				lcd_clk_div_sel_table[clk_div_sel],
				clk_div_sel, cconf->xd);
		}
	} else {
		od1 = od_table[cconf->pll_od1_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("m=%d, od=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_m, cconf->pll_od1_sel,
				lcd_clk_div_sel_table[clk_div_sel],
				clk_div_sel, cconf->xd);
		}
	}
	if (enc_clk > cconf->data->xd_out_fmax) {
		LCDERR("%s: wrong enc_clk value %uHz\n", __func__, enc_clk);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s enc_clk=%d\n", __func__, enc_clk);

	clk_div_out = enc_clk * cconf->xd;
	if (clk_div_out > cconf->data->div_out_fmax) {
		LCDERR("%s: wrong clk_div_out value %uHz\n", __func__, clk_div_out);
		return -1;
	}

	clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
	if (clk_div_in > cconf->data->div_in_fmax) {
		LCDERR("%s: wrong clk_div_in value %lldHz\n", __func__, clk_div_in);
		return -1;
	}

	pll_fout = clk_div_in;
	if (pll_fout > cconf->data->pll_out_fmax ||
	    pll_fout < cconf->data->pll_out_fmin) {
		LCDERR("%s: wrong pll_fout value %lldHz\n", __func__, pll_fout);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fout=%lld\n", __func__, pll_fout);

	if (cconf->data->od_cnt == 3)
		pll_fvco = pll_fout * od1 * od2 * od3;
	else
		pll_fvco = pll_fout * od1;
	if (pll_fvco < cconf->data->pll_vco_fmin ||
	    pll_fvco > cconf->data->pll_vco_fmax) {
		LCDERR("%s: wrong pll_fvco value %lldHz\n", __func__, pll_fvco);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fvco=%lld\n", __func__, pll_fvco);

	ret = lcd_pll_get_frac(cconf, pll_fvco);
	if (ret == 0) {
		cconf->fout = enc_clk;
		cconf->pll_div_fout = clk_div_out;
		cconf->pll_fout = pll_fout;
		cconf->pll_fvco = pll_fvco;
		pconf->timing.clk_ctrl &= ~(0x1ffffff);
		pconf->timing.clk_ctrl |=
			(cconf->pll_frac << CLK_CTRL_FRAC) |
			(cconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
	}

	return ret;
}

static int lcd_pll_frac_generate_independence(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf, *phyconf = NULL, *pixconf = NULL;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned long long pll_fout, pll_fvco, clk_div_in;
	unsigned int enc_clk, clk_div_out, clk_div_sel;
	unsigned int od1 = 1, od2 = 1, od3 = 1;
	int ret;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return -1;

	phyconf = &cconf[0];
	pixconf = &cconf[1];
	enc_clk = pconf->timing.enc_clk;
	if (enc_clk > pixconf->data->xd_out_fmax) {
		LCDERR("%s: wrong enc_clk value %uHz\n", __func__, enc_clk);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s enc_clk=%u\n", __func__, enc_clk);

	//handle phyconf
	if (phyconf->data->od_cnt == 3) {
		od1 = od_table[phyconf->pll_od1_sel];
		od2 = od_table[phyconf->pll_od2_sel];
		od3 = od_table[phyconf->pll_od3_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("phyconf m=%d, od1=%d, od2=%d, od3=%d\n",
				phyconf->pll_m, phyconf->pll_od1_sel,
				phyconf->pll_od2_sel, phyconf->pll_od3_sel);
		}
	} else {
		od1 = od_table[phyconf->pll_od1_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("phyconf m=%d, od=%d\n",
				phyconf->pll_m, phyconf->pll_od1_sel);
		}
	}
	pll_fout = pconf->timing.bit_rate;
	if (phyconf->data->od_cnt == 3)
		pll_fvco = pll_fout * od1 * od2 * od3;
	else
		pll_fvco = pll_fout * od1;
	if (pll_fvco < phyconf->data->pll_vco_fmin ||
	    pll_fvco > phyconf->data->pll_vco_fmax) {
		LCDERR("%s: wrong phy pll_fvco value %lldHz\n", __func__, pll_fvco);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s: phy pll_fvco=%lld\n", __func__, pll_fvco);

	clk_div_in = pll_fout;
	clk_div_sel = phyconf->div_sel;
	clk_div_out = clk_vid_pll_div_calc(clk_div_in, clk_div_sel, CLK_DIV_I2O);

	ret = lcd_pll_get_frac(phyconf, pll_fvco);
	if (ret == 0) {
		pixconf->pll_div_fout = clk_div_out;
		phyconf->fout = enc_clk;
		phyconf->pll_fout = pll_fout;
		phyconf->pll_fvco = pll_fvco;
		pconf->timing.clk_ctrl &= ~(0x1ffffff);
		pconf->timing.clk_ctrl |=
			(phyconf->pll_frac << CLK_CTRL_FRAC) |
			(phyconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
	} else {
		return ret;
	}

	//handle pixconf
	clk_div_sel = pixconf->div_sel;
	if (pixconf->data->od_cnt == 3) {
		od1 = od_table[pixconf->pll_od1_sel];
		od2 = od_table[pixconf->pll_od2_sel];
		od3 = od_table[pixconf->pll_od3_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("pixconf m=%d, od1=%d, od2=%d, od3=%d, clk_div_sel=%s(%d), xd=%d\n",
				pixconf->pll_m, pixconf->pll_od1_sel,
				pixconf->pll_od2_sel, pixconf->pll_od3_sel,
				lcd_clk_div_sel_table[clk_div_sel],
				clk_div_sel, pixconf->xd);
		}
	} else {
		od1 = od_table[pixconf->pll_od1_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("pixconf m=%d, od=%d, clk_div_sel=%s(%d), xd=%d\n",
				pixconf->pll_m, pixconf->pll_od1_sel,
				lcd_clk_div_sel_table[clk_div_sel],
				clk_div_sel, pixconf->xd);
		}
	}
	clk_div_out = enc_clk * pixconf->xd;
	if (clk_div_out > pixconf->data->div_out_fmax) {
		LCDERR("%s: wrong pix clk_div_out value %uHz\n", __func__, clk_div_out);
		return -1;
	}

	clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
	if (clk_div_in > pixconf->data->div_in_fmax) {
		LCDERR("%s: wrong pix clk_div_in value %lldHz\n", __func__, clk_div_in);
		return -1;
	}

	pll_fout = clk_div_in;
	if (pll_fout > pixconf->data->pll_out_fmax ||
	pll_fout < pixconf->data->pll_out_fmin) {
		LCDERR("%s: wrong pix pll_fout value %lldHz\n", __func__, pll_fout);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s: pix pll_fout=%lld\n", __func__, pll_fout);

	if (pixconf->data->od_cnt == 3)
		pll_fvco = pll_fout * od1 * od2 * od3;
	else
		pll_fvco = pll_fout * od1;
	if (pll_fvco < pixconf->data->pll_vco_fmin ||
	pll_fvco > pixconf->data->pll_vco_fmax) {
		LCDERR("%s: wrong pix pll_fvco value %lldHz\n", __func__, pll_fvco);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s: pix pll_fvco=%lld\n", __func__, pll_fvco);

	ret = lcd_pll_get_frac(pixconf, pll_fvco);
	if (ret == 0) {
		pixconf->fout = enc_clk;
		phyconf->fout = enc_clk; //restore pixconf val to phyconf for enc0_clk
		pixconf->pll_fout = pll_fout;
		pixconf->pll_fvco = pll_fvco;
		pixconf->pll_div_fout = clk_div_out;
		pconf->timing.clk_ctrl2 &= ~(0x1ffffff);
		pconf->timing.clk_ctrl2 |=
			(pixconf->pll_frac << CLK_CTRL_FRAC) |
			(pixconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
	}

	return ret;
}

void lcd_pll_frac_generate_dft(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE)
		ret = lcd_pll_frac_generate_independence(pdrv);
	else
		ret = lcd_pll_frac_generate_dependence(pdrv);
	if (ret)
		LCDERR("%s: failed\n", __func__);
}

void lcd_clk_generate_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf, *phyconf = NULL, *pixconf = NULL;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned long long pll_fout, pll_fvco, bit_rate = 0, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd, tcon_div_sel = 0, phy_div = 1;
	unsigned int od1, od2, od3, tmp_clk;
	int done = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;
	phyconf = cconf;
	pixconf = cconf;

	if (pdrv->config.timing.clk_mode != LCD_CLK_MODE_INDEPENDENCE) {
		cconf->pll_mode = pconf->timing.pll_flag;
		cconf->fout = pconf->timing.enc_clk;
		if (cconf->fout > cconf->data->xd_out_fmax) {
			LCDERR("%s: wrong enc_clk value %uHz\n", __func__, cconf->fout);
			goto generate_clk_dft_done;
		}
	}
	bit_rate = pconf->timing.bit_rate;

	switch (pconf->basic.lcd_type) {
	case LCD_RGB:
		clk_div_sel = CLK_DIV_SEL_1;
		for (xd = 1; xd <= cconf->data->xd_max; xd++) {
			clk_div_out = cconf->fout * xd;
			if (clk_div_out > cconf->data->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
				      cconf->fout, xd, clk_div_out);
			}
			clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
			if (clk_div_in > cconf->data->div_in_fmax)
				continue;
			cconf->xd = xd;
			cconf->div_sel = clk_div_sel;
			cconf->pll_div_fout = clk_div_out;
			pll_fout = clk_div_in;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("clk_div_sel=%s(index %d), pll_fout=%lld\n",
				      lcd_clk_div_sel_table[clk_div_sel],
				      clk_div_sel, pll_fout);
			}

			done = pll_od_setting_generate(cconf, pll_fout);
			if (done)
				goto generate_clk_dft_done;
		}
		break;
	case LCD_LVDS:
		if (pdrv->data->chip_type == LCD_CHIP_T3X) {
			if (pconf->control.lvds_cfg.dual_port)
				clk_div_sel = CLK_DIV_SEL_3p5;
			else
				clk_div_sel = CLK_DIV_SEL_7;
			phy_div = 1;
		} else {
			if (pconf->control.lvds_cfg.dual_port)
				phy_div = 2;
			else
				phy_div = 1;
			clk_div_sel = CLK_DIV_SEL_7;
		}
		xd = 1;
		clk_div_out = cconf->fout * xd;
		if (clk_div_out > cconf->data->div_out_fmax)
			goto generate_clk_dft_done;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
			      cconf->fout, xd, clk_div_out);
		}
		clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
		if (clk_div_in > cconf->data->div_in_fmax)
			goto generate_clk_dft_done;
		cconf->xd = xd;
		cconf->div_sel = clk_div_sel;
		cconf->pll_div_fout = clk_div_out;
		pll_fout = clk_div_in;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("clk_div_sel=%s(index %d), pll_fout=%lld\n",
			      lcd_clk_div_sel_table[clk_div_sel], clk_div_sel, pll_fout);
		}

		done = pll_od_setting_generate(cconf, pll_fout);
		if (done == 0)
			goto generate_clk_dft_done;

		if (cconf->data->have_tcon_div) {
			done = 0;
			if (cconf->data->od_cnt == 3) {
				od1 = od_table[cconf->pll_od1_sel];
				od2 = od_table[cconf->pll_od2_sel];
				od3 = od_table[cconf->pll_od3_sel];
			} else {
				od1 = od_table[cconf->pll_od1_sel];
				od2 = 1;
				od3 = 1;
			}
			for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
				if (tcon_div_table[tcon_div_sel] == phy_div * od1 * od2 * od3) {
					cconf->pll_tcon_div_sel = tcon_div_sel;
					done = 1;
					break;
				}
			}
		}
		break;
	case LCD_VBYONE:
		pll_fout = bit_rate;
		clk_div_in = pll_fout;
		if (clk_div_in > cconf->data->div_in_fmax)
			goto generate_clk_dft_done;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_fout=%lld, clk_div_in=%lld\n", pll_fout, clk_div_in);

		clk_div_sel = CLK_DIV_SEL_1;
		for (; clk_div_sel < cconf->data->div_sel_max; clk_div_sel++) {
			clk_div_out = clk_vid_pll_div_calc(clk_div_in, clk_div_sel, CLK_DIV_I2O);
			if (clk_div_out > cconf->data->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("clk_div_out=%u, clk_div_sel=%s(%d)\n",
					clk_div_out,
					lcd_clk_div_sel_table[clk_div_sel],
					clk_div_sel);
			}

			done = 0;
			for (xd = 1; xd <= cconf->data->xd_max; xd++) {
				tmp_clk = cconf->fout * xd;
				if (tmp_clk > clk_div_out)
					break;
				if (tmp_clk == clk_div_out) {
					cconf->xd = xd;
					cconf->div_sel = clk_div_sel;
					cconf->pll_div_fout = clk_div_out;
					done = 1;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
						LCDPR("fout=%u, xd=%d\n", cconf->fout, xd);
					break;
				}
			}

			if (done)
				break;
		}

		done = pll_od_setting_generate(cconf, pll_fout);
		if (done == 0)
			goto generate_clk_dft_done;

		if (cconf->data->have_tcon_div) {
			done = 0;
			if (cconf->data->od_cnt == 3) {
				od1 = od_table[cconf->pll_od1_sel];
				od2 = od_table[cconf->pll_od2_sel];
				od3 = od_table[cconf->pll_od3_sel];
			} else {
				od1 = od_table[cconf->pll_od1_sel];
				od2 = 1;
				od3 = 1;
			}
			for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
				if (tcon_div_table[tcon_div_sel] == od1 * od2 * od3) {
					cconf->pll_tcon_div_sel = tcon_div_sel;
					done = 1;
					break;
				}
			}
		}
		break;
	case LCD_MLVDS:
		/* must go through div4 for clk phase */
		if (cconf->data->have_tcon_div == 0) {
			LCDERR("%s: no tcon_div for minilvds\n", __func__);
			goto generate_clk_dft_done;
		}
		for (tcon_div_sel = 3; tcon_div_sel < 5; tcon_div_sel++) {
			pll_fvco = bit_rate * tcon_div_table[tcon_div_sel];
			done = check_vco(cconf, pll_fvco);
			if (done == 0)
				continue;
			for (xd = 1; xd <= cconf->data->xd_max; xd++) {
				clk_div_out = cconf->fout * xd;
				if (clk_div_out > cconf->data->div_out_fmax)
					continue;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
					      cconf->fout, xd, clk_div_out);
				}
				clk_div_sel = CLK_DIV_SEL_1;
				for (; clk_div_sel < cconf->data->div_sel_max; clk_div_sel++) {
					clk_div_in = clk_vid_pll_div_calc(clk_div_out,
							     clk_div_sel, CLK_DIV_O2I);
					if (clk_div_in > cconf->data->div_in_fmax)
						continue;
					cconf->xd = xd;
					cconf->div_sel = clk_div_sel;
					cconf->pll_tcon_div_sel = tcon_div_sel;
					cconf->pll_div_fout = clk_div_out;
					pll_fout = clk_div_in;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
						LCDPR("clk_div_sel=%s(%d)\n",
						      lcd_clk_div_sel_table[clk_div_sel],
						      clk_div_sel);
						LCDPR("pll_fout=%lld, tcon_div_sel=%d\n",
						      pll_fout, tcon_div_sel);
					}
					done = check_3od(cconf, pll_fout);
					if (done)
						goto generate_clk_dft_done;
				}
			}
		}
		break;
	case LCD_P2P:
		if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
			phyconf = &cconf[0];
			pixconf = &cconf[1];

			pixconf->fout = pconf->timing.enc_clk;
			pixconf->pll_mode = pconf->timing.pll_flag;
			if (pixconf->fout > pixconf->data->xd_out_fmax) {
				LCDERR("%s: wrong enc_clk value %uHz\n", __func__, pixconf->fout);
				goto generate_clk_dft_done;
			}

			//handle phyconf
			phyconf->pll_mode = pconf->timing.pll_flag;
			phyconf->phy_clk = pconf->timing.bit_rate;
			phyconf->pll_fout = phyconf->phy_clk;
			//fix phy pll_div for clkmsr phy_clk
			phyconf->div_sel = CLK_DIV_SEL_5;
			clk_div_in = phyconf->pll_fout;
			clk_div_out = clk_vid_pll_div_calc(clk_div_in,
						phyconf->div_sel, CLK_DIV_I2O);
			phyconf->pll_div_fout = clk_div_out;
			//store final encl_clk for clkmsr check
			phyconf->fout = pconf->timing.enc_clk;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("phyconf: clk_div_sel=%s(%d), phy_clk=%lld, clk_div_out=%u\n",
					lcd_clk_div_sel_table[phyconf->div_sel],
					phyconf->div_sel, phyconf->phy_clk,
					phyconf->pll_div_fout);
			}

			done = pll_od_setting_generate(phyconf, phyconf->phy_clk);
			if (done == 0) {
				LCDERR("%s: wrong phy_clk %lldHz\n", __func__, phyconf->phy_clk);
				goto generate_clk_dft_done;
			}

			//handle pixconf
			pixconf->data->xd_max = cconf->data->xd_max;
			for (xd = 1; xd <= pixconf->data->xd_max; xd++) {
				clk_div_out = pixconf->fout * xd;
				if (clk_div_out > pixconf->data->div_out_fmax)
					continue;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
						pixconf->fout, xd, clk_div_out);
				}
				for (clk_div_sel = CLK_DIV_SEL_1;
					clk_div_sel < cconf->data->div_sel_max; clk_div_sel++) {
					clk_div_in = clk_vid_pll_div_calc(clk_div_out,
							clk_div_sel, CLK_DIV_O2I);
					if (clk_div_in > pixconf->data->div_in_fmax)
						continue;
					pixconf->xd = xd;
					pixconf->div_sel = clk_div_sel;
					pixconf->pll_div_fout = clk_div_out;
					pll_fout = clk_div_in;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
						LCDPR("clk_div_sel=%s(%d), pll_fout=%lld\n",
							lcd_clk_div_sel_table[clk_div_sel],
							clk_div_sel, pll_fout);
					}

					//restore pixconf val to phyconf for enc0_clk
					phyconf->xd = pixconf->xd;
					phyconf->fout = pixconf->fout;

					done = pll_od_setting_generate(pixconf, pll_fout);
					if (done)
						goto generate_clk_dft_done;
				}
			}
		} else {
			if (cconf->data->have_tcon_div)
				done = lcd_clk_generate_p2p_with_tcon_div(cconf, bit_rate);
			else
				done = lcd_clk_generate_p2p_without_tcon_div(cconf, bit_rate);
		}
		break;
	case LCD_MIPI:
		done = lcd_clk_generate_DSI_1PLL(pdrv);
		break;
	case LCD_EDP:
		done = lcd_clk_generate_DP_1PLL(pdrv);
		break;
	default:
		break;
	}

generate_clk_dft_done:
	if (done) {
		if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
			pconf->timing.pll_ctrl =
				(phyconf->pll_od1_sel << PLL_CTRL_OD1) |
				(phyconf->pll_od2_sel << PLL_CTRL_OD2) |
				(phyconf->pll_od3_sel << PLL_CTRL_OD3) |
				(phyconf->pll_n << PLL_CTRL_N)         |
				(phyconf->pll_m << PLL_CTRL_M);
			pconf->timing.div_ctrl =
				(phyconf->div_sel << DIV_CTRL_DIV_SEL) |
				(phyconf->xd << DIV_CTRL_XD);
			pconf->timing.clk_ctrl =
				(phyconf->pll_frac << CLK_CTRL_FRAC) |
				(phyconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
			phyconf->done = 1;

			pconf->timing.pll_ctrl2 =
				(pixconf->pll_od1_sel << PLL_CTRL_OD1) |
				(pixconf->pll_od2_sel << PLL_CTRL_OD2) |
				(pixconf->pll_od3_sel << PLL_CTRL_OD3) |
				(pixconf->pll_n << PLL_CTRL_N)         |
				(pixconf->pll_m << PLL_CTRL_M);
			pconf->timing.div_ctrl2 =
				(pixconf->div_sel << DIV_CTRL_DIV_SEL) |
				(pixconf->xd << DIV_CTRL_XD);
			pconf->timing.clk_ctrl2 =
				(pixconf->pll_frac << CLK_CTRL_FRAC) |
				(pixconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
			pixconf->done = 1;
		} else {
			pconf->timing.pll_ctrl =
				(cconf->pll_od1_sel << PLL_CTRL_OD1) |
				(cconf->pll_od2_sel << PLL_CTRL_OD2) |
				(cconf->pll_od3_sel << PLL_CTRL_OD3) |
				(cconf->pll_n << PLL_CTRL_N)         |
				(cconf->pll_m << PLL_CTRL_M);
			pconf->timing.div_ctrl =
				(cconf->div_sel << DIV_CTRL_DIV_SEL) |
				(cconf->xd << DIV_CTRL_XD);
			pconf->timing.clk_ctrl =
				(cconf->pll_frac << CLK_CTRL_FRAC) |
				(cconf->pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
			cconf->done = 1;
		}
	} else {
		if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
			pconf->timing.pll_ctrl = 0;
			pconf->timing.div_ctrl = 0;
			pconf->timing.clk_ctrl = 0;
			phyconf->done = 0;

			pconf->timing.pll_ctrl2 = 0;
			pconf->timing.div_ctrl2 = 0;
			pconf->timing.clk_ctrl2 = 0;
			pixconf->done = 0;
		} else {
			pconf->timing.pll_ctrl = 0;
			pconf->timing.div_ctrl = 0;
			pconf->timing.clk_ctrl = 0;
			cconf->done = 0;
		}
		LCDERR("[%d]: %s: Out of clock range\n", pdrv->index, __func__);
	}
}

void lcd_set_vid_pll_div_dft(struct lcd_clk_config_s *cconf)
{
	unsigned int shift_val, shift_sel;
	int i;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	i = 0;
	while (lcd_clk_div_table[i][0] < cconf->data->div_sel_max) {
		if (cconf->div_sel == lcd_clk_div_table[i][0])
			break;
		i++;
	}
	if (lcd_clk_div_table[i][0] == cconf->data->div_sel_max)
		LCDERR("invalid clk divider\n");
	shift_val = lcd_clk_div_table[i][1];
	shift_sel = lcd_clk_div_table[i][2];

	if (shift_val == 0xffff) { /* if divide by 1 */
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 18, 1);
	} else {
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 18, 1);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 16, 2);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 0, 14);

		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, shift_sel, 16, 2);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 15, 1);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, shift_val, 0, 15);
		lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);
	}
	/* Enable the final output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 1, 19, 1);
}

void lcd_set_vclk_crt_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);
	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

#ifdef CONFIG_AML_LCD_PXP
	/* setup the XD divider value */
	lcd_clk_setb(HHI_VIID_CLK_DIV, cconf->xd, VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 7, VCLK2_CLK_IN_SEL, 3);
#else
	/* setup the XD divider value */
	lcd_clk_setb(HHI_VIID_CLK_DIV, (cconf->xd - 1), VCLK2_XD, 8);
	udelay(5);
	/* select vid_pll_clk */
	lcd_clk_setb(HHI_VIID_CLK_CNTL, cconf->data->vclk_sel, VCLK2_CLK_IN_SEL, 3);
#endif
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_EN, 1);
	udelay(2);

	/* [15:12] encl_clk_sel, select vclk2_div1 */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 8, ENCL_CLK_SEL, 4);
	/* release vclk2_div_reset and enable vclk2_div */
	lcd_clk_setb(HHI_VIID_CLK_DIV, 1, VCLK2_XD_EN, 2);
	udelay(5);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_DIV1_EN, 1);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 1, VCLK2_SOFT_RST, 1);
	udelay(10);
	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_SOFT_RST, 1);
	udelay(5);

	/* enable CTS_ENCL clk gate */
	lcd_clk_setb(HHI_VID_CLK_CNTL2, 1, ENCL_GATE_VCLK, 1);
}

void lcd_clk_config_init_print_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	struct lcd_clk_data_s *data;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	data = cconf->data;
	LCDPR("[%d]: lcd clk config data init:\n"
		"pll_m_max:           %d\n"
		"pll_m_min:           %d\n"
		"pll_n_max:           %d\n"
		"pll_n_min:           %d\n"
		"pll_od_fb:           %d\n"
		"pll_frac_range:      %d\n"
		"pll_od_sel_max:      %d\n"
		"pll_ref_fmax:        %d\n"
		"pll_ref_fmin:        %d\n"
		"pll_vco_fmax:        %lld\n"
		"pll_vco_fmin:        %lld\n"
		"pll_out_fmax:        %lld\n"
		"pll_out_fmin:        %lld\n"
		"div_in_fmax:         %lld\n"
		"div_out_fmax:        %d\n"
		"xd_out_fmax:         %d\n"
		"ss_level_max:        %d\n"
		"ss_dep_base:         %d\n"
		"ss_dep_sel_max:      %d\n"
		"ss_str_m_max:        %d\n"
		"ss_freq_max:         %d\n"
		"ss_mode_max:         %d\n\n",
		pdrv->index,
		data->pll_m_max, data->pll_m_min,
		data->pll_n_max, data->pll_n_min,
		data->pll_od_fb, data->pll_frac_range,
		data->pll_od_sel_max,
		data->pll_ref_fmax, data->pll_ref_fmin,
		data->pll_vco_fmax, data->pll_vco_fmin,
		data->pll_out_fmax, data->pll_out_fmin,
		data->div_in_fmax, data->div_out_fmax,
		data->xd_out_fmax, data->ss_level_max,
		data->ss_dep_base, data->ss_dep_sel_max,
		data->ss_str_m_max,
		data->ss_freq_max, data->ss_mode_max);
}

void lcd_clk_config_print_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = NULL, *cconf_tbl = NULL;
	int i = 0;

	cconf_tbl = get_lcd_clk_config(pdrv);
	if (!cconf_tbl)
		return;

	for (i = 0; i < pdrv->clk_conf_num; i++) {
		cconf = &cconf_tbl[i];
		LCDPR("[%d]: lcd clk[%d] config:\n"
			"  pll_id:     %d\n"
			"  pll_offset: 0x%x\n"
			"  pll_mode:   %d\n"
			"  pll_m:      %d\n"
			"  pll_n:      %d\n"
			"  pll_frac:   0x%x\n"
			"  pll_frac_half_shift: %d\n"
			"  pll_fvco:   %lluHz\n"
			"  pll_od1:    %d\n"
			"  pll_od2:    %d\n"
			"  pll_od3:    %d\n"
			"  pll_tcon_div_sel: %d\n"
			"  pll_out:    %lldHz\n"
			"  phy_clk:    %lldHz\n"
			"  edp_div0:   %d\n"
			"  edp_div1:   %d\n"
			"  div_sel:    %s(index %d)\n"
			"  pll_div_fout: %uHz\n"
			"  xd:         %d\n"
			"  fout:       %uHz\n\n",
			pdrv->index, cconf->pll_id,
			cconf->pll_id, cconf->pll_offset,
			cconf->pll_mode, cconf->pll_m, cconf->pll_n,
			cconf->pll_frac, cconf->pll_frac_half_shift,
			cconf->pll_fvco,
			cconf->pll_od1_sel, cconf->pll_od2_sel,
			cconf->pll_od3_sel, cconf->pll_tcon_div_sel,
			cconf->pll_fout, cconf->phy_clk,
			edp_div0_table[cconf->edp_div0], edp_div1_table[cconf->edp_div1],
			lcd_clk_div_sel_table[cconf->div_sel],
			cconf->div_sel, cconf->pll_div_fout,
			cconf->xd, cconf->fout);
		if (cconf->data && cconf->data->ss_support) {
			printf("  ss_level:   %d\n"
				"  ss_dep_sel: %d\n"
				"  ss_str_m:   %d\n"
				"  ss_ppm:     %d\n"
				"  ss_freq:    %d\n"
				"  ss_mode:    %d\n"
				"  ss_en:      %d\n\n",
				cconf->ss_level, cconf->ss_dep_sel,
				cconf->ss_str_m, cconf->ss_ppm,
				cconf->ss_freq, cconf->ss_mode, cconf->ss_en);
		}
	}
}

int lcd_prbs_clk_check(unsigned long encl_clk, int encl_msr_id,
			unsigned long fifo_clk, int fifo_msr_id, unsigned int cnt)
{
	unsigned long clk_check, temp;

	if (encl_msr_id == -1)
		goto lcd_prbs_clk_check_next;
	clk_check = clk_util_clk_msr(encl_msr_id) * 1000000;
	if (clk_check != encl_clk) {
		temp = lcd_diff(clk_check, encl_clk);
		if (temp >= PLL_CLK_CHECK_MAX) {
			if (lcd_debug_print_flag & LCD_DBG_PR_TEST) {
				LCDERR("encl clkmsr error %ld, cnt: %d\n",
				       clk_check, cnt);
			}
			return -1;
		}
	}

lcd_prbs_clk_check_next:
	if (fifo_msr_id == -1)
		return 0;
	clk_check = clk_util_clk_msr(fifo_msr_id) * 1000000;
	if (clk_check != fifo_clk) {
		temp = lcd_diff(clk_check, fifo_clk);
		if (temp >= PLL_CLK_CHECK_MAX) {
			if (lcd_debug_print_flag & LCD_DBG_PR_TEST) {
				LCDERR("fifo clkmsr error %ld, cnt:%d\n",
				       clk_check, cnt);
			}
			return -1;
		}
	}

	return 0;
}
