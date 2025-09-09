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
#include "../connectors/lcd_connector.h"
#include <amlogic/clk_measure.h>

#define LCD_PLL_SEL_PHY 0
#define LCD_PLL_SEL_PIX 1

static unsigned int edp_div0_table[15] = {1, 2, 3, 4, 5, 7, 8, 9, 11, 13, 17, 19, 23, 29, 31};
static unsigned int edp_div1_table[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 13};

struct lcd_clk_div_table_s lcd_clk_div_table[] = {
	/* name, num, den, shift_sel, shift_val    divider */
	{"1",     1,   1,    0,       0xffff},  //CLK_DIV_SEL_1,
	{"2",     1,   2,    0,       0x0aaa},  //CLK_DIV_SEL_2,
	{"3",     1,   3,    0,       0x0db6},  //CLK_DIV_SEL_3,
	{"3.5",   2,   7,    1,       0x36cc},  //CLK_DIV_SEL_3p5,
	{"3.75",  4,   15,   2,       0x6666},  //CLK_DIV_SEL_3p75,
	{"4",     1,   4,    0,       0x0ccc},  //CLK_DIV_SEL_4,
	{"5",     1,   5,    2,       0x739c},  //CLK_DIV_SEL_5,
	{"6",     1,   6,    0,       0x0e38},  //CLK_DIV_SEL_6,
	{"6.25",  4,   25,   3,       0x0000},  //CLK_DIV_SEL_6p25,
	{"7",     1,   7,    1,       0x3c78},  //CLK_DIV_SEL_7,
	{"7.5",   2,   15,   2,       0x78f0},  //CLK_DIV_SEL_7p5,
	{"12",    1,   12,   0,       0x0fc0},  //CLK_DIV_SEL_12,
	{"14",    1,   14,   1,       0x3f80},  //CLK_DIV_SEL_14,
	{"15",    1,   15,   2,       0x7f80},  //CLK_DIV_SEL_15,
	{"2.5",   2,   5,    2,       0x5294},  //CLK_DIV_SEL_2p5,
	{"4.67",  3,   14,   1,       0x0ccc},  //CLK_DIV_SEL_4p67,
	{"2.33",  3,   7,    1,       0x1aaa},  //CLK_DIV_SEL_2p33,
	{"2.22",  9,   20,   4,       0xa554a}, //CLK_DIV_SEL_2p22,
	{"2.25",  4,   9,    5,       0x2aa5549},  //CLK_DIV_SEL_2p25,
};

static const unsigned int od_fb_table[2] = {1, 2};
static const unsigned int od_table[6] = {1, 2, 4, 8, 16, 32};
static const unsigned int tcon_div_table[5] = {1, 2, 4, 8, 16};

static unsigned char lcd_ss_freq_dep_opt[] = {
/*             freq, cnt, dep values       */
/* 0-29.5k */	0,    3,   4, 7, 10,
/* 1-31.5k */	1,    3,   3, 8, 11,
/* 2-50.0k */	2,    3,   5, 6, 11,
/* 3-75.0k */	3,    3,   4, 7, 11,
/* 4-100k  */	4,    5,   3, 5, 6, 8, 11,
/* 5-150k  */	5,    5,   2, 4, 7, 9, 11,
/* 6-200k  */	6,    12,  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
/* 7-0 end */   7,    0,
};

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

int lcd_pll_ss_level_generate_optimized(struct lcd_clk_config_s *cconf)
{
	int dep_sel, str_m, target, ss_ppm, dep_base, err = 0, done = 0;
	unsigned int freq, dep_cnt = 0, i = 0;
	unsigned char *freq_dep = lcd_ss_freq_dep_opt;

	if (!cconf)
		return -1;

	if (cconf->data->ss_freq_dep_opt)
		freq_dep = cconf->data->ss_freq_dep_opt;

	target = cconf->ss_level;
	target *= 1000;
	dep_base = cconf->data->ss_dep_base;
	freq = cconf->ss_freq;
	for (; freq != (unsigned int)freq_dep[0]; freq_dep += (freq_dep[1] + 2)) {
		if (freq_dep[1] == 0) {
			LCDPR("no optimized ssc freq matched\n");
			return 0;
		}
	}
	dep_cnt = freq_dep[1];
	freq_dep += 2;
	for (i = 0; i < dep_cnt; i++) {
		dep_sel = freq_dep[i];
		for (str_m = 1; str_m <= cconf->data->ss_str_m_max; str_m++) {
			ss_ppm = dep_sel * str_m * dep_base;

			err = target - ss_ppm;
			if (err <= dep_base && err >= -dep_base) {
				cconf->ss_dep_sel = dep_sel;
				cconf->ss_str_m = str_m;
				cconf->ss_ppm = ss_ppm;
				cconf->ss_freq_stable = 1;
				done = 1;
				if (err == 0)
					return 1;
			}
		}
	}

	return done;
}

int lcd_pll_ss_level_generate(struct lcd_clk_config_s *cconf)
{
	unsigned int dep_sel, str_m, err = 0, min = 0, done = 0;
	unsigned long long target, ss_ppm, dep_base;

	if (!cconf)
		return -1;

	if (lcd_pll_ss_level_generate_optimized(cconf) > 0)
		goto lcd_pll_ss_level_generate_exit;

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
	cconf->ss_freq_stable = 0;

lcd_pll_ss_level_generate_exit:
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("%s: dep_sel=%d, str_m=%d, error=%d\n",
			__func__, cconf->ss_dep_sel, cconf->ss_str_m, min);
	}

	return 0;
}

int lcd_pll_wait_lock(int id, unsigned int reg, unsigned int lock_bit)
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
	LCDPR("%s: [%d]: pll_lock=%d, wait_loop=%d\n",
	      __func__, id, pll_lock, (PLL_WAIT_LOCK_CNT - wait_loop));

	return ret;
}

/* ****************************************************
 * lcd clk parameters calculate
 * ****************************************************
 */
unsigned long long clk_vid_pll_div_calc(unsigned long long clk, unsigned int div_sel, int dir)
{
	unsigned long long clk_ret, num, den;

	if (div_sel > CLK_DIV_SEL_MAX) {
		LCDERR("clk_div_sel: Invalid parameter\n");
		return 0;
	}

	if (dir == CLK_DIV_I2O) {
		num = lcd_clk_div_table[div_sel].num;
		den = lcd_clk_div_table[div_sel].den;
	} else {
		num = lcd_clk_div_table[div_sel].den;
		den = lcd_clk_div_table[div_sel].num;
	}
	clk_ret = div_around(clk * num, den);

	return clk_ret;
}

int lcd_pll_get_frac(struct lcd_clk_config_s *cconf, int pll_sel, unsigned long long pll_fvco)
{
	unsigned int frac_range, frac, offset;
	unsigned long long fvco_calc, temp;
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];

	frac_range = pll_data->pll_frac_range;

	fvco_calc = lcd_do_div(pll_fvco, od_fb_table[pll_config->pll_od_fb]);
	temp = cconf->fin;
	temp = lcd_do_div((temp * pll_config->pll_m), pll_config->pll_n);
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

	frac = lcd_do_div((temp * frac_range * pll_config->pll_n * 10), cconf->fin) + 5;
	frac /= 10;
	if (cconf->pll_mode & LCD_PLL_MODE_FRAC_SHIFT) {
		if ((frac == (frac_range >> 1)) || (frac == (frac_range >> 2))) {
			frac |= 0x66;
			pll_config->pll_frac_half_shift = 1;
		} else {
			pll_config->pll_frac_half_shift = 0;
		}
	}
	pll_config->pll_frac = frac | (offset << pll_data->pll_frac_sign_bit);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: 0x%x\n", __func__, pll_config->pll_frac);

	return 0;
}

/******* calculate 1od and 3od setting @pll_od_setting_generate *******/
static int generate_pll_3od_setting(struct lcd_clk_config_s *cconf,
					int pll_sel, unsigned long long pll_fout)
{
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];
	unsigned int m, n;
	unsigned int od1_sel, od2_sel, od3_sel, od1, od2, od3;
	unsigned long long pll_fod2_in, pll_fod3_in, pll_fvco, temp;
	unsigned int od_fb = 0, pll_frac, frac_range;
	int done;

	done = 0;
	if (pll_fout > pll_data->pll_out_fmax || pll_fout < pll_data->pll_out_fmin)
		return done;

	frac_range =  pll_data->pll_frac_range;
	for (od3_sel =  pll_data->pll_od_sel_max; od3_sel > 0; od3_sel--) {
		od3 = od_table[od3_sel - 1];
		pll_fod3_in = pll_fout * od3;
		for (od2_sel = od3_sel; od2_sel > 0; od2_sel--) {
			od2 = od_table[od2_sel - 1];
			pll_fod2_in = pll_fod3_in * od2;
			for (od1_sel = od2_sel; od1_sel > 0; od1_sel--) {
				od1 = od_table[od1_sel - 1];
				pll_fvco = pll_fod2_in * od1;
				if (pll_fvco < pll_data->pll_vco_fmin ||
					pll_fvco > pll_data->pll_vco_fmax) {
					continue;
				}
				pll_config->pll_od1_sel = od1_sel - 1;
				pll_config->pll_od2_sel = od2_sel - 1;
				pll_config->pll_od3_sel = od3_sel - 1;
				pll_config->pll_fout = pll_fout;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("od1=%d, od2=%d, od3=%d, pll_fvco=%lld\n",
						(od1_sel - 1), (od2_sel - 1),
						(od3_sel - 1), pll_fvco);
				}
				pll_config->pll_fvco = pll_fvco;
				n = 1;
				od_fb = pll_config->pll_od_fb;
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
						pll_config->pll_frac_half_shift = 1;
					} else {
						pll_config->pll_frac_half_shift = 0;
					}
				}
				pll_config->pll_m = m;
				pll_config->pll_n = n;
				pll_config->pll_frac = pll_frac;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
					LCDPR("m=%d, n=%d, frac=0x%x\n", m, n, pll_frac);
				done = 1;
				break;
			}
		}
	}
	return done;
}

static int generate_pll_1od_setting(struct lcd_clk_config_s *cconf,
					int pll_sel, unsigned long long pll_fout)
{
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];
	unsigned int m, n, od_sel, od;
	unsigned long long pll_fvco, temp;
	unsigned int od_fb = 0, pll_frac;
	int done = 0;

	if (pll_fout > pll_data->pll_out_fmax || pll_fout < pll_data->pll_out_fmin)
		return done;

	for (od_sel = pll_data->pll_od_sel_max; od_sel > 0; od_sel--) {
		od = od_table[od_sel - 1];
		pll_fvco = pll_fout * od;
		if (pll_fvco < pll_data->pll_vco_fmin || pll_fvco > pll_data->pll_vco_fmax)
			continue;
		pll_config->pll_od1_sel = od_sel - 1;
		pll_config->pll_fout = pll_fout;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("od_sel=%d, pll_fvco=%lld\n", (od_sel - 1), pll_fvco);

		pll_config->pll_fvco = pll_fvco;
		n = 1;
		od_fb = pll_config->pll_od_fb;
		pll_fvco = lcd_do_div(pll_fvco, od_fb_table[od_fb]);
		m = lcd_do_div(pll_fvco, cconf->fin);
		temp = cconf->fin;
		temp *= m;
		temp = pll_fvco - temp;
		pll_frac = lcd_do_div((temp * pll_data->pll_frac_range * 10), cconf->fin) + 5;
		pll_frac /= 10;
		pll_config->pll_m = m;
		pll_config->pll_n = n;
		pll_config->pll_frac = pll_frac;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_m=%d, pll_n=%d, pll_frac=0x%x\n", m, n, pll_frac);
		done = 1;
		break;
	}
	return done;
}

static int generate_pll_0od_setting(struct lcd_clk_config_s *cconf,
					int pll_sel, unsigned long long pll_fout)
{
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];
	unsigned int m, n;
	unsigned long long pll_fvco, temp;
	unsigned int pll_frac;
	int done = 0;

	if (pll_fout > pll_data->pll_out_fmax || pll_fout < pll_data->pll_out_fmin)
		return done;

	pll_fvco = pll_fout;
	if (pll_fvco < pll_data->pll_vco_fmin || pll_fvco > pll_data->pll_vco_fmax)
		return done;

	pll_config->pll_fout = pll_fout;
	pll_config->pll_fvco = pll_fvco;

	n = 1;
	m = lcd_do_div(pll_fvco, cconf->fin);
	temp = cconf->fin;
	temp *= m;
	temp = pll_fvco - temp;
	pll_frac = lcd_do_div((temp * pll_data->pll_frac_range * 10), cconf->fin) + 5;
	pll_frac /= 10;
	pll_config->pll_m = m;
	pll_config->pll_n = n;
	pll_config->pll_frac = pll_frac;
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("pll_m=%d, pll_n=%d, pll_frac=0x%x\n", m, n, pll_frac);
	done = 1;

	return done;
}

int pll_od_setting_generate(struct lcd_clk_config_s *cconf,
				int pll_sel, unsigned long long pll_fout)
{
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];

	if (pll_data->od_cnt == 3)
		return generate_pll_3od_setting(cconf, pll_sel, pll_fout);
	else if (pll_data->od_cnt == 0)
		return generate_pll_0od_setting(cconf, pll_sel, pll_fout);
	else
		return generate_pll_1od_setting(cconf, pll_sel, pll_fout);
}

/***** calculate 1od and 3od setting @pll_od_setting_generate Done *****/
int check_vco(struct lcd_clk_config_s *cconf, int pll_sel, unsigned long long pll_fvco)
{
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];
	unsigned int m, n;
	unsigned int od_fb = 0, pll_frac;
	unsigned long long temp;
	int done = 0;

	if (pll_fvco < pll_data->pll_vco_fmin || pll_fvco > pll_data->pll_vco_fmax) {
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_fvco %lld is out of range\n", pll_fvco);
		return done;
	}

	pll_config->pll_fvco = pll_fvco;
	n = 1;
	od_fb = pll_config->pll_od_fb;
	pll_fvco = lcd_do_div(pll_fvco, od_fb_table[od_fb]);
	m = lcd_do_div(pll_fvco, cconf->fin);
	temp = cconf->fin;
	temp *= m;
	temp = pll_fvco - temp;
	pll_frac = lcd_do_div((temp * pll_data->pll_frac_range * 10), cconf->fin) + 5;
	pll_frac /= 10;
	pll_config->pll_m = m;
	pll_config->pll_n = n;
	pll_config->pll_frac = pll_frac;
	if (cconf->pll_mode & LCD_PLL_MODE_FRAC_SHIFT) {
		if (pll_frac == (pll_data->pll_frac_range >> 1) ||
		    pll_frac == (pll_data->pll_frac_range >> 2)) {
			pll_frac |= 0x66;
			pll_config->pll_frac_half_shift = 1;
		} else {
			pll_config->pll_frac_half_shift = 0;
		}
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("m=%d, n=%d, frac=0x%x, pll_fvco=%lld\n",
		      m, n, pll_frac, pll_fvco);
	}
	done = 1;

	return done;
}

#ifdef CONFIG_AML_LCD_TCON
static int check_3od(struct lcd_clk_config_s *cconf, int pll_sel, unsigned long long pll_fout)
{
	struct lcd_pll_config_s *pll_config = &cconf->pll_config[pll_sel];
	struct lcd_pll_data_s *pll_data = cconf->data->pll_data[pll_sel];
	unsigned int od1_sel, od2_sel, od3_sel, od1, od2, od3;
	unsigned long long pll_fod2_in, pll_fod3_in, pll_fvco;
	int done = 0;

	if (pll_fout > pll_data->pll_out_fmax || pll_fout < pll_data->pll_out_fmin)
		return done;

	for (od3_sel = pll_data->pll_od_sel_max; od3_sel > 0; od3_sel--) {
		od3 = od_table[od3_sel - 1];
		pll_fod3_in = pll_fout * od3;
		for (od2_sel = od3_sel; od2_sel > 0; od2_sel--) {
			od2 = od_table[od2_sel - 1];
			pll_fod2_in = pll_fod3_in * od2;
			for (od1_sel = od2_sel; od1_sel > 0; od1_sel--) {
				od1 = od_table[od1_sel - 1];
				pll_fvco = pll_fod2_in * od1;
				if (pll_fvco < pll_data->pll_vco_fmin ||
				    pll_fvco > pll_data->pll_vco_fmax) {
					continue;
				}
				if (pll_fvco == pll_config->pll_fvco) {
					pll_config->pll_od1_sel = od1_sel - 1;
					pll_config->pll_od2_sel = od2_sel - 1;
					pll_config->pll_od3_sel = od3_sel - 1;
					pll_config->pll_fout = pll_fout;
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
		done = check_vco(cconf, LCD_PLL_SEL_PHY, pll_fvco);
		if (done == 0)
			continue;
		for (xd = 1; xd <= cconf->data->xd_max; xd++) {
			clk_div_out = cconf->fout * xd;
			if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("fout=%d, xd=%d, clk_div_out=%d, tcon_div_sel=%d\n",
					cconf->fout, xd, clk_div_out, tcon_div_sel);
			}
			for (clk_div_sel = CLK_DIV_SEL_1;
				clk_div_sel <= cconf->data->pll_data[0]->div_sel_max;
				clk_div_sel++) {
				clk_div_in = clk_vid_pll_div_calc(clk_div_out,
						clk_div_sel, CLK_DIV_O2I);
				if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
					continue;
				cconf->xd = xd;
				cconf->pll_config[0].div_sel = clk_div_sel;
				cconf->pll_config[0].pll_div_fout = clk_div_out;
				cconf->pll_tcon_div_sel = tcon_div_sel;
				cconf->phy_clk = bit_rate;
				pll_fout = clk_div_in;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("clk_div_sel=%s(%d), pll_fout=%lld\n",
						lcd_clk_div_table[clk_div_sel].name,
						clk_div_sel, pll_fout);
				}
				done = check_3od(cconf, LCD_PLL_SEL_PHY, pll_fout);
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

	if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL) {
		cconf->phy_clk = bit_rate;
		cconf->pll_config[0].pll_fout = cconf->phy_clk;
		//fix phy pll_div for clkmsr phy_clk
		cconf->pll_config[0].div_sel = CLK_DIV_SEL_5;
		clk_div_in = cconf->pll_config[0].pll_fout;
		clk_div_out = clk_vid_pll_div_calc(clk_div_in,
				cconf->pll_config[0].div_sel, CLK_DIV_I2O);
		cconf->pll_config[0].pll_div_fout = clk_div_out;
		//store final encl_clk for clkmsr check
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("cconf: clk_div_sel=%s(%d), phy_clk=%lld, clk_div_out=%u\n",
				lcd_clk_div_table[cconf->pll_config[0].div_sel].name,
				cconf->pll_config[0].div_sel, cconf->phy_clk,
				cconf->pll_config[0].pll_div_fout);
		}
		done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, cconf->phy_clk);
		if (done == 0) {
			LCDERR("%s: wrong phy_clk %lldHz\n", __func__, cconf->phy_clk);
			goto p2p_clk_without_tcon_div_done;
		}
	} else {
		for (xd = 1; xd <= cconf->data->xd_max; xd++) {
			clk_div_out = cconf->fout * xd;
			if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("fout=%d, xd=%d, clk_div_out=%d\n",
					cconf->fout, xd, clk_div_out);
			}
			for (clk_div_sel = CLK_DIV_SEL_1;
				clk_div_sel <= cconf->data->pll_data[0]->div_sel_max;
				clk_div_sel++) {
				clk_div_in = clk_vid_pll_div_calc(clk_div_out,
						clk_div_sel, CLK_DIV_O2I);
				if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
					continue;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("clk_div_sel=%s(%d), clk_div_in=%lld,bit_rate=%lld\n",
						lcd_clk_div_table[clk_div_sel].name,
						clk_div_sel, clk_div_in, bit_rate);
				}
				if (clk_div_in == bit_rate) {
					cconf->xd = xd;
					cconf->pll_config[0].div_sel = clk_div_sel;
					cconf->pll_config[0].pll_div_fout = clk_div_out;
					cconf->phy_clk = bit_rate;
					pll_fout = clk_div_in;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
						LCDPR("pll_fout=%lld\n", pll_fout);

					done = pll_od_setting_generate(cconf,
						LCD_PLL_SEL_PHY, pll_fout);
					if (done)
						goto p2p_clk_without_tcon_div_done;
				}
			}
		}
	}

p2p_clk_without_tcon_div_done:
	return done;
}
#endif


#ifdef CONFIG_AML_LCD_DPI
static unsigned char lcd_clk_generate_DPI(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	unsigned long long pll_fout, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd;
	int done = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return 0;

	clk_div_sel = CLK_DIV_SEL_1;
	for (xd = 1; xd <= cconf->data->xd_max; xd++) {
		clk_div_out = cconf->fout * xd;
		if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
			continue;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("fout=%u, xd=%d, clk_div_out=%u\n", cconf->fout, xd, clk_div_out);
		clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
		if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
			continue;
		cconf->xd = xd;
		cconf->pll_config[0].div_sel = clk_div_sel;
		cconf->pll_config[0].pll_div_fout = clk_div_out;
		pll_fout = clk_div_in;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("clk_div_sel=%s(index %d), pll_fout=%lld\n",
				lcd_clk_div_table[clk_div_sel].name, clk_div_sel, pll_fout);
		}

		done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, pll_fout);
		if (done)
			return 1;
	}
	return 0;
}
#endif

#ifdef CONFIG_AML_LCD_MIPI_DSI
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
		unsigned char phy_n;
		unsigned char frac_sel;
	};

	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	unsigned long long pll_out, phy_clk;
	unsigned short enc_xd, phy_N;
	unsigned char done, tb_idx = 0, x, new_high_bitrate, frac_sel;
	struct dsi_clk_tb_s *clk_div_tb;

	unsigned long long bitrate_min = 0, bitrate_max;
	uint8_t port_cnt = dconf->multi_port_cfg & BIT(0) ? 2 : 1;

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
	cconf->pll_config->div_sel = CLK_DIV_SEL_1;

	for (enc_xd = 1; enc_xd < cconf->data->xd_max; enc_xd++) {
		pll_out = enc_xd;
		pll_out = pll_out * cconf->fout;
		if (pll_out > cconf->data->pll_data[0]->div_out_fmax)
			continue;
		for (frac_sel = CLK_DIV_SEL_1;
			frac_sel <= cconf->data->pll_data[0]->div_sel_max; frac_sel++) {
			pll_out = clk_vid_pll_div_calc(pll_out, frac_sel, CLK_DIV_O2I);

			if (pll_out > cconf->data->pll_data[0]->pll_out_fmax ||
			    pll_out < cconf->data->pll_data[0]->pll_out_fmin)
				continue;

			for (phy_N = 1; phy_N < cconf->data->phy_div_max; phy_N++) {
				phy_clk = clk_vid_pll_div_calc(pll_out, frac_sel, CLK_DIV_I2O);
				phy_clk = div_around(phy_clk, phy_N);
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
				clk_div_tb[tb_idx].frac_sel = frac_sel;
				tb_idx++;
			}
		}
	}

	if (!tb_idx) {
		LCDERR("[%d]: %s: no div for pll_out:(%lluHz~%lluHz), bit_rate:(%lluHz~%uMHz)\n",
			pdrv->index, __func__, cconf->data->pll_data[0]->pll_out_fmin,
			cconf->data->pll_data[0]->pll_out_fmax, bitrate_min, dconf->bit_rate_max);
		free(clk_div_tb);
		return 0;
	}

dsi_clk_tabel_buffer_full:
	x = tb_idx - 1;
	while (1) {
		done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, clk_div_tb[x].pllout);
		if (done || x == 0)
			break;
		x--;
	}
	if (!done) {
		LCDERR("[%d]: %s: no pll setting available\n", pdrv->index, __func__);
		free(clk_div_tb);
		return 0;
	}

	LCDPR("[%d]: vco=%lluHz pll_out:%lluHz div[%s] xd[%hu]->fout=%uhz div[%hu]->phy=%lluhz\n",
	      pdrv->index, cconf->pll_config[0].pll_fvco, clk_div_tb[x].pllout,
	      lcd_clk_div_table[clk_div_tb[x].frac_sel].name, clk_div_tb[x].enc_xd, cconf->fout,
	      clk_div_tb[x].phy_n, clk_div_tb[x].phy_clk);

	cconf->phy_clk = clk_div_tb[x].phy_clk;
	pdrv->config.timing.bit_rate = clk_div_tb[x].phy_clk;

	cconf->phy_div = clk_div_tb[x].phy_n;
	cconf->xd = clk_div_tb[x].enc_xd; //PLL2enc
	cconf->pll_config->div_sel = clk_div_tb[x].frac_sel;
	// should lane_byte_clk = (be phy_clk == phy_bitrate / 2) / 4
	dconf->lane_byte_clk = div_around(clk_div_tb[x].phy_clk, 8);

	dconf->factor_numerator = cconf->xd * port_cnt;
	dconf->factor_denominator = cconf->phy_div * 8;

	free(clk_div_tb);
	return 1;
}

#ifdef CONFIG_MESON_S6
/*
 *  DSI_VCO(2.8G) --- 3od --- dsi_phy_clk(2div)
 *        |                        '--- host clk
 *        '--- /enc_xd --- encl_clk
 * -----------------------------------------------------
 * SW arch:
 * PLL: 1.4~2.8G VCO, 0 od
 * 3 od + dsi_phy_clk(2div) as special for phy div
 */
static unsigned char lcd_dsi_generate_DSI_PLL_s6_model(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);

	if (!cconf)
		return 0;

	struct dsi_clk_tb_s {
		unsigned long long pll_out;
		unsigned long long phy_clk;
		unsigned short enc_xd;
		unsigned short phy_div;
		unsigned char frac_div_sel;
	};

	unsigned char vco_to_phy_div_table[5][2] = {
		{1, 0x0}, {2, 0x1}, {4, 0x2}, {8, 0x3}, {16, 0x4}
	};

	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	unsigned long long pll_out, phy_clk;
	unsigned short enc_xd, phy_N;
	unsigned char tb_idx = 0, x, new_high_bitrate, phy_div, frac_sel;
	struct dsi_clk_tb_s *clk_div_tb;

	unsigned long long bitrate_min = 0, bitrate_max;

#ifdef CONFIG_AML_LCD_MIPI_DSI
	bitrate_min = lcd_dsi_get_min_bitrate(pdrv);
#endif
	bitrate_max = dconf->bit_rate_max;
	bitrate_max = bitrate_max * 1000000;

	clk_div_tb = (struct dsi_clk_tb_s *)malloc(32 * sizeof(struct dsi_clk_tb_s));
	if (!clk_div_tb) {
		LCDERR("[%d]: %s: kcalloc failed\n", pdrv->index, __func__);
		return 0;
	}
	memset(clk_div_tb, 0, 32 * sizeof(struct dsi_clk_tb_s));

	cconf->pll_tcon_div_sel = 2;

	for (enc_xd = 1; enc_xd < cconf->data->xd_max; enc_xd++) {
		for (frac_sel = CLK_DIV_SEL_1;
			frac_sel <= cconf->data->pll_data[0]->div_sel_max; frac_sel++) {
		// for (frac_sel = CLK_DIV_SEL_1; frac_sel <= CLK_DIV_SEL_1; frac_sel++) {
			pll_out = enc_xd;
			pll_out = pll_out * cconf->fout;
			pll_out = clk_vid_pll_div_calc(pll_out, frac_sel, CLK_DIV_O2I);

			if (pll_out > cconf->data->pll_data[0]->pll_vco_fmax ||
			    pll_out < cconf->data->pll_data[0]->pll_vco_fmin)
				continue;
			for (phy_div = 0; phy_div < ARRAY_SIZE(vco_to_phy_div_table); phy_div++) {
				phy_N = vco_to_phy_div_table[phy_div][0];
				phy_clk = div_around(pll_out, phy_N);
				if (phy_clk > bitrate_max || phy_clk < bitrate_min)
					continue;

				new_high_bitrate = 1;
				for (x = 0; x < tb_idx; x++) {
					if (phy_clk <= clk_div_tb[x].phy_clk)
						new_high_bitrate = 0;
				}
				if (!new_high_bitrate)
					continue;

				if (tb_idx == 32) {
					LCDERR("[%d]: dsi clk table full!\n", pdrv->index);
					goto dsi_clk_tabel_buffer_full;
				}

				clk_div_tb[tb_idx].pll_out = pll_out;
				clk_div_tb[tb_idx].enc_xd = enc_xd;
				clk_div_tb[tb_idx].phy_div = phy_div;
				clk_div_tb[tb_idx].phy_clk = phy_clk;
				clk_div_tb[tb_idx].frac_div_sel = frac_sel;
				tb_idx++;
			}
		}
	}

	if (!tb_idx) {
		LCDERR("[%d]: %s: no div for pll_out:(%lluHz~%lluHz), bit_rate:(%lluHz~%uMHz)\n",
			pdrv->index, __func__, cconf->data->pll_data[0]->pll_out_fmin,
			cconf->data->pll_data[0]->pll_out_fmax, bitrate_min, dconf->bit_rate_max);
		free(clk_div_tb);
		return 0;
	}

dsi_clk_tabel_buffer_full:
	x = tb_idx - 1;

	LCDPR("[%d]: DSI_PLL: pll_out:%lluHz: xd[%hu]*frac[%s]->fout=%uhz, div[%hu]->phy=%lluhz\n",
		pdrv->index, clk_div_tb[x].pll_out,
		clk_div_tb[x].enc_xd, lcd_clk_div_table[clk_div_tb[x].frac_div_sel].name,
		cconf->fout, clk_div_tb[x].phy_div, clk_div_tb[x].phy_clk);

	cconf->pll_config[0].pll_fout = clk_div_tb[x].pll_out;

	cconf->xd = clk_div_tb[x].enc_xd; //PLL2enc
	cconf->pll_config->div_sel = clk_div_tb[x].frac_div_sel;

	cconf->pll_config[0].pll_od1_sel = vco_to_phy_div_table[clk_div_tb[x].phy_div][1];
	// should lane_byte_clk = (be phy_clk == phy_bitrate / 2) / 4
	cconf->phy_div = 1;
	cconf->phy_clk = clk_div_tb[x].phy_clk;
	pdrv->config.timing.bit_rate = clk_div_tb[x].phy_clk;
	dconf->lane_byte_clk = div_around(clk_div_tb[x].phy_clk, 8);

	// should lane_byte_clk = (be phy_clk == phy_bitrate / 2) / pclk
	dconf->factor_numerator = cconf->xd * lcd_clk_div_table[cconf->pll_config->div_sel].den;
	dconf->factor_denominator = vco_to_phy_div_table[clk_div_tb[x].phy_div][0] * 8 *
				    lcd_clk_div_table[cconf->pll_config->div_sel].num;

	free(clk_div_tb);
	return 1;
}
#endif
#endif

static int lcd_pll_frac_generate_phy(struct aml_lcd_drv_s *pdrv)
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
	clk_div_sel = cconf->pll_config[0].div_sel;
	if (cconf->data->pll_data[0]->od_cnt == 3) {
		od1 = od_table[cconf->pll_config[0].pll_od1_sel];
		od2 = od_table[cconf->pll_config[0].pll_od2_sel];
		od3 = od_table[cconf->pll_config[0].pll_od3_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("m=%d, od1=%d, od2=%d, od3=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_config[0].pll_m, cconf->pll_config[0].pll_od1_sel,
				cconf->pll_config[0].pll_od2_sel, cconf->pll_config[0].pll_od3_sel,
				lcd_clk_div_table[clk_div_sel].name,
				clk_div_sel, cconf->xd);
		}
	} else {
		od1 = od_table[cconf->pll_config[0].pll_od1_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("m=%d, od=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_config[0].pll_m, cconf->pll_config[0].pll_od1_sel,
				lcd_clk_div_table[clk_div_sel].name,
				clk_div_sel, cconf->xd);
		}
	}
	if (enc_clk > cconf->data->xd_out_fmax) {
		LCDERR("%s: wrong enc_clk value %uHz\n", __func__, enc_clk);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s enc_clk=%d\n", __func__, enc_clk);

	pll_fvco = pconf->timing.bit_rate * tcon_div_table[cconf->pll_tcon_div_sel];
	if (pll_fvco < cconf->data->pll_data[0]->pll_vco_fmin ||
	    pll_fvco > cconf->data->pll_data[0]->pll_vco_fmax) {
		LCDERR("%s: wrong pll_fvco value %lldHz\n", __func__, pll_fvco);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fvco=%lld\n", __func__, pll_fvco);

	clk_div_in = lcd_do_div(pll_fvco, od1 * od2 * od3);
	if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax) {
		LCDERR("%s: wrong clk_div_in value %lldHz\n", __func__, clk_div_in);
		return -1;
	}
	pll_fout = clk_div_in;
	if (pll_fout > cconf->data->pll_data[0]->pll_out_fmax ||
	    pll_fout < cconf->data->pll_data[0]->pll_out_fmin) {
		LCDERR("%s: wrong pll_fout value %lldHz\n", __func__, pll_fout);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fout=%lld\n", __func__, pll_fout);

	clk_div_out = clk_vid_pll_div_calc(clk_div_in, clk_div_sel, CLK_DIV_I2O);
	if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax) {
		LCDERR("%s: wrong clk_div_out value %uHz\n", __func__, clk_div_out);
		return -1;
	}

	ret = lcd_pll_get_frac(cconf, LCD_PLL_SEL_PHY, pll_fvco);
	if (ret == 0) {
		cconf->fout = enc_clk;
		cconf->pll_config[0].pll_div_fout = clk_div_out;
		cconf->pll_config[0].pll_fout = pll_fout;
		cconf->pll_config[0].pll_fvco = pll_fvco;
		pconf->timing.clk_ctrl &= ~(0x1ffffff);
		pconf->timing.clk_ctrl |=
			(cconf->pll_config[0].pll_frac << CLK_CTRL_FRAC) |
			(cconf->pll_config[0].pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
	}

	return ret;
}

static int lcd_pll_frac_generate_pix(struct aml_lcd_drv_s *pdrv)
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

	if (!(cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL)) {
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("%s: no need to generate pix pll\n", __func__);
		return 0;
	}

	enc_clk = pconf->timing.enc_clk;
	clk_div_sel = cconf->pll_config[1].div_sel;
	if (cconf->data->pll_data[1]->od_cnt == 3) {
		od1 = od_table[cconf->pll_config[1].pll_od1_sel];
		od2 = od_table[cconf->pll_config[1].pll_od2_sel];
		od3 = od_table[cconf->pll_config[1].pll_od3_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("cconf m=%d, od1=%d, od2=%d, od3=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_config[1].pll_m, cconf->pll_config[1].pll_od1_sel,
				cconf->pll_config[1].pll_od2_sel, cconf->pll_config[1].pll_od3_sel,
				lcd_clk_div_table[clk_div_sel].name, clk_div_sel, cconf->xd);
		}
	} else {
		od1 = od_table[cconf->pll_config[1].pll_od1_sel];
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("cconf m=%d, od=%d, clk_div_sel=%s(%d), xd=%d\n",
				cconf->pll_config[1].pll_m, cconf->pll_config[1].pll_od1_sel,
				lcd_clk_div_table[clk_div_sel].name, clk_div_sel, cconf->xd);
		}
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("m=%d, od=%d, clk_div_sel=%s(%d), xd=%d\n",
			cconf->pll_config[1].pll_m, cconf->pll_config[1].pll_od1_sel,
			lcd_clk_div_table[clk_div_sel].name, clk_div_sel, cconf->xd);
	}
	if (enc_clk > cconf->data->xd_out_fmax) {
		LCDERR("%s: wrong enc_clk value %uHz\n", __func__, enc_clk);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s enc_clk=%d\n", __func__, enc_clk);

	clk_div_out = enc_clk * cconf->xd;
	if (clk_div_out > cconf->data->pll_data[1]->div_out_fmax) {
		LCDERR("%s: wrong clk_div_out value %uHz\n", __func__, clk_div_out);
		return -1;
	}

	clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
	if (clk_div_in > cconf->data->pll_data[1]->div_in_fmax) {
		LCDERR("%s: wrong clk_div_in value %lldHz\n", __func__, clk_div_in);
		return -1;
	}

	pll_fout = clk_div_in;
	if (pll_fout > cconf->data->pll_data[1]->pll_out_fmax ||
	    pll_fout < cconf->data->pll_data[1]->pll_out_fmin) {
		LCDERR("%s: wrong pll_fout value %lldHz\n", __func__, pll_fout);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fout=%lld\n", __func__, pll_fout);

	if (cconf->data->pll_data[1]->od_cnt == 3)
		pll_fvco = pll_fout * od1 * od2 * od3;
	else
		pll_fvco = pll_fout * od1;
	if (pll_fvco < cconf->data->pll_data[1]->pll_vco_fmin ||
	    pll_fvco > cconf->data->pll_data[1]->pll_vco_fmax) {
		LCDERR("%s: wrong pll_fvco value %lldHz\n", __func__, pll_fvco);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
		LCDPR("%s pll_fvco=%lld\n", __func__, pll_fvco);

	ret = lcd_pll_get_frac(cconf, LCD_PLL_SEL_PIX, pll_fvco);
	if (ret == 0) {
		cconf->fout = enc_clk;
		cconf->pll_config[1].pll_div_fout = clk_div_out;
		cconf->pll_config[1].pll_fout = pll_fout;
		cconf->pll_config[1].pll_fvco = pll_fvco;
		pconf->timing.clk_ctrl &= ~(0x1ffffff);
		pconf->timing.clk_ctrl |=
			(cconf->pll_config[1].pll_frac << CLK_CTRL_FRAC) |
			(cconf->pll_config[1].pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
	}
	return ret;
}

void lcd_pll_frac_generate_dft(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_pll_frac_generate_phy(pdrv);
	if (ret)
		LCDERR("%s: phy generate failed\n", __func__);

	ret = lcd_pll_frac_generate_pix(pdrv);
	if (ret)
		LCDERR("%s: pix generate failed\n", __func__);
}

#ifdef CONFIG_AML_LCD_TCON
static int lcd_clk_generate_pix_clk(struct lcd_clk_config_s *cconf)
{
	unsigned long long pll_fout, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd;
	int done = 0;

	for (xd = 1; xd <= cconf->data->xd_max; xd++) {
		clk_div_out = cconf->fout * xd;
		if (clk_div_out > cconf->data->pll_data[1]->div_out_fmax)
			continue;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("fout=%d, xd=%d, clk_div_out=%d\n",
				cconf->fout, xd, clk_div_out);
		}

		for (clk_div_sel = CLK_DIV_SEL_1;
			clk_div_sel <= cconf->data->pll_data[1]->div_sel_max;
			clk_div_sel++) {
			clk_div_in = clk_vid_pll_div_calc(clk_div_out,
					clk_div_sel, CLK_DIV_O2I);
			if (clk_div_in > cconf->data->pll_data[1]->div_in_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("clk_div_sel=%s(%d), clk_div_in=%lld\n",
					lcd_clk_div_table[clk_div_sel].name,
					clk_div_sel, clk_div_in);
			}
			cconf->xd = xd;
			cconf->pll_config[1].div_sel = clk_div_sel;
			cconf->pll_config[1].pll_div_fout = clk_div_out;
			pll_fout = clk_div_in;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
				LCDPR("pll_fout=%lld\n", pll_fout);

			done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PIX, pll_fout);
			if (done)
				goto clk_generate_pix_clk_done;
		}
	}

clk_generate_pix_clk_done:
	return done;
}
#endif

void lcd_clk_generate_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf;
	struct lcd_config_s *pconf = &pdrv->config;
#if defined(CONFIG_AML_LCD_VBYONE) || defined(CONFIG_AML_LCD_LVDS) || defined(CONFIG_AML_LCD_TCON)
	unsigned long long pll_fout, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd;
	unsigned int  tcon_div_sel = 0;
#endif
#if defined(CONFIG_AML_LCD_VBYONE) || defined(CONFIG_AML_LCD_LVDS)
	unsigned int od1, od2, od3;
#endif
	int done = 0;
#ifdef CONFIG_AML_LCD_LVDS
	unsigned int phy_div = 1;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	unsigned int tmp_clk;
#endif
#ifdef CONFIG_AML_LCD_TCON
	unsigned long long pll_fvco, bit_rate = pconf->timing.bit_rate;
#endif
	int done_pix = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	cconf->pll_mode |= pconf->timing.pll_flag;
	cconf->fout = pconf->timing.enc_clk;
	if (cconf->fout > cconf->data->xd_out_fmax) {
		LCDERR("%s: enc_clk %uHz out of %uHz\n",
		       __func__, cconf->fout, cconf->data->xd_out_fmax);
		goto generate_clk_dft_done;
	}

	switch (pconf->basic.lcd_type) {
#if defined(CONFIG_AML_LCD_DPI)
	case LCD_RGB:
		done = lcd_clk_generate_DPI(pdrv);
		break;
#endif
#if defined(CONFIG_AML_LCD_LVDS)
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
		if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
			goto generate_clk_dft_done;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
			      cconf->fout, xd, clk_div_out);
		}
		clk_div_in = clk_vid_pll_div_calc(clk_div_out, clk_div_sel, CLK_DIV_O2I);
		if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
			goto generate_clk_dft_done;
		cconf->xd = xd;
		cconf->pll_config[0].div_sel = clk_div_sel;
		cconf->pll_config[0].pll_div_fout = clk_div_out;
		pll_fout = clk_div_in;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("clk_div_sel=%s(index %d), pll_fout=%lld\n",
			      lcd_clk_div_table[clk_div_sel].name, clk_div_sel, pll_fout);
		}

		done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, pll_fout);
		if (done == 0)
			goto generate_clk_dft_done;

		if (cconf->data->pll_data[0]->have_tcon_div) {
			done = 0;
			if (cconf->data->pll_data[0]->od_cnt == 3) {
				od1 = od_table[cconf->pll_config[0].pll_od1_sel];
				od2 = od_table[cconf->pll_config[0].pll_od2_sel];
				od3 = od_table[cconf->pll_config[0].pll_od3_sel];
			} else {
				od1 = od_table[cconf->pll_config[0].pll_od1_sel];
				od2 = 1;
				od3 = 1;
			}
			for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
				if (tcon_div_table[tcon_div_sel] == phy_div * od1 * od2 * od3) {
					cconf->pll_tcon_div_sel = tcon_div_sel;
					cconf->phy_clk = lcd_do_div(cconf->pll_config[0].pll_fvco,
								    tcon_div_table[tcon_div_sel]);
					done = 1;
					break;
				}
			}
		} else {
			cconf->phy_clk = cconf->pll_config[0].pll_fout;
		}
		break;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	case LCD_VBYONE:
		pll_fout = pconf->timing.bit_rate;
		clk_div_in = pll_fout;
		if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
			goto generate_clk_dft_done;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
			LCDPR("pll_fout=%lld, clk_div_in=%lld\n", pll_fout, clk_div_in);

		clk_div_sel = CLK_DIV_SEL_1;
		for (; clk_div_sel <= cconf->data->pll_data[0]->div_sel_max; clk_div_sel++) {
			clk_div_out = clk_vid_pll_div_calc(clk_div_in, clk_div_sel, CLK_DIV_I2O);
			if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
				continue;
			if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
				LCDPR("clk_div_out=%u, clk_div_sel=%s(%d)\n",
					clk_div_out,
					lcd_clk_div_table[clk_div_sel].name,
					clk_div_sel);
			}

			done = 0;
			for (xd = 1; xd <= cconf->data->xd_max; xd++) {
				tmp_clk = cconf->fout * xd;
				if (tmp_clk > clk_div_out)
					break;
				if (tmp_clk == clk_div_out) {
					cconf->xd = xd;
					cconf->pll_config[0].div_sel = clk_div_sel;
					cconf->pll_config[0].pll_div_fout = clk_div_out;
					done = 1;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
						LCDPR("fout=%u, xd=%d\n", cconf->fout, xd);
					break;
				}
			}

			if (done)
				break;
		}

		done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, pll_fout);
		if (done == 0)
			goto generate_clk_dft_done;

		if (cconf->data->pll_data[0]->have_tcon_div) {
			done = 0;
			if (cconf->data->pll_data[0]->od_cnt == 3) {
				od1 = od_table[cconf->pll_config[0].pll_od1_sel];
				od2 = od_table[cconf->pll_config[0].pll_od2_sel];
				od3 = od_table[cconf->pll_config[0].pll_od3_sel];
			} else {
				od1 = od_table[cconf->pll_config[0].pll_od1_sel];
				od2 = 1;
				od3 = 1;
			}
			for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
				if (tcon_div_table[tcon_div_sel] == od1 * od2 * od3) {
					cconf->pll_tcon_div_sel = tcon_div_sel;
					cconf->phy_clk =
						lcd_do_div(cconf->pll_config[0].pll_fvco,
							   tcon_div_table[tcon_div_sel]);
					done = 1;
					break;
				}
			}
		} else {
			cconf->phy_clk = cconf->pll_config[0].pll_fout;
		}
		break;
#endif
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		/* must go through div4 for clk phase */
		if (cconf->data->pll_data[0]->have_tcon_div == 0) {
			LCDERR("%s: no tcon_div for minilvds\n", __func__);
			goto generate_clk_dft_done;
		}
		for (tcon_div_sel = 3; tcon_div_sel < 5; tcon_div_sel++) {
			pll_fvco = bit_rate * tcon_div_table[tcon_div_sel];
			done = check_vco(cconf, LCD_PLL_SEL_PHY, pll_fvco);
			if (done == 0)
				continue;
			for (xd = 1; xd <= cconf->data->xd_max; xd++) {
				clk_div_out = cconf->fout * xd;
				if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
					continue;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
					LCDPR("fout=%u, xd=%d, clk_div_out=%u\n",
					      cconf->fout, xd, clk_div_out);
				}
				clk_div_sel = CLK_DIV_SEL_1;
				for (; clk_div_sel <= cconf->data->pll_data[0]->div_sel_max;
					clk_div_sel++) {
					clk_div_in = clk_vid_pll_div_calc(clk_div_out,
							     clk_div_sel, CLK_DIV_O2I);
					if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
						continue;
					cconf->xd = xd;
					cconf->pll_config[0].div_sel = clk_div_sel;
					cconf->pll_tcon_div_sel = tcon_div_sel;
					cconf->pll_config[0].pll_div_fout = clk_div_out;
					cconf->phy_clk = lcd_do_div(pll_fvco,
								    tcon_div_table[tcon_div_sel]);
					pll_fout = clk_div_in;
					if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
						LCDPR("clk_div_sel=%s(%d)\n",
						      lcd_clk_div_table[clk_div_sel].name,
						      clk_div_sel);
						LCDPR("pll_fout=%lld, tcon_div_sel=%d\n",
						      pll_fout, tcon_div_sel);
					}
					done = check_3od(cconf, LCD_PLL_SEL_PHY, pll_fout);
					if (done)
						goto generate_clk_dft_done;
				}
			}
		}
		break;
	case LCD_P2P:
		if (cconf->data->pll_data[0]->have_tcon_div)
			done = lcd_clk_generate_p2p_with_tcon_div(cconf, bit_rate);
		else
			done = lcd_clk_generate_p2p_without_tcon_div(cconf, bit_rate);
		if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL) {
			done_pix = lcd_clk_generate_pix_clk(cconf);
		}
		break;
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	case LCD_MIPI:
		switch (pdrv->data->chip_type) {
#ifdef CONFIG_MESON_S6
		case LCD_CHIP_S6:
			done = lcd_dsi_generate_DSI_PLL_s6_model(pdrv);
			if (done)
				done = pll_od_setting_generate(cconf,
						LCD_PLL_SEL_PHY, cconf->pll_config[0].pll_fout);
			break;
#endif
		default:
			done = lcd_clk_generate_DSI_1PLL(pdrv);
			// common DSI PLL model already had pll_od_setting_generate
			break;
		}
		break;
#endif
	default:
		break;
	}
generate_clk_dft_done:
	if (done) {
		pconf->timing.pll_ctrl =
			(cconf->pll_config[0].pll_od1_sel << PLL_CTRL_OD1) |
			(cconf->pll_config[0].pll_od2_sel << PLL_CTRL_OD2) |
			(cconf->pll_config[0].pll_od3_sel << PLL_CTRL_OD3) |
			(cconf->pll_config[0].pll_n << PLL_CTRL_N)         |
			(cconf->pll_config[0].pll_m << PLL_CTRL_M);
		pconf->timing.div_ctrl =
			(cconf->pll_config[0].div_sel << DIV_CTRL_DIV_SEL) |
			(cconf->xd << DIV_CTRL_XD);
		pconf->timing.clk_ctrl =
			(cconf->pll_config[0].pll_frac << CLK_CTRL_FRAC) |
			(cconf->pll_config[0].pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
		cconf->pll_config[0].done = 1;
	} else {
		pconf->timing.pll_ctrl = 0;
		pconf->timing.div_ctrl = 0;
		pconf->timing.clk_ctrl = 0;
		cconf->pll_config[0].done = 0;
		LCDERR("[%d]: %s: Out of clock range\n", pdrv->index, __func__);
	}
	if (done_pix && cconf->pll_conf_num > 1) {
		pconf->timing.pll_ctrl2 =
			(cconf->pll_config[1].pll_od1_sel << PLL_CTRL_OD1) |
			(cconf->pll_config[1].pll_od2_sel << PLL_CTRL_OD2) |
			(cconf->pll_config[1].pll_od3_sel << PLL_CTRL_OD3) |
			(cconf->pll_config[1].pll_n << PLL_CTRL_N)         |
			(cconf->pll_config[1].pll_m << PLL_CTRL_M);
		pconf->timing.div_ctrl2 =
			(cconf->pll_config[1].div_sel << DIV_CTRL_DIV_SEL) |
			(cconf->xd << DIV_CTRL_XD);
		pconf->timing.clk_ctrl2 =
			(cconf->pll_config[1].pll_frac << CLK_CTRL_FRAC) |
			(cconf->pll_config[1].pll_frac_half_shift << CLK_CTRL_FRAC_SHIFT);
		cconf->pll_config[1].done = 1;
	} else {
		pconf->timing.pll_ctrl = 0;
		pconf->timing.div_ctrl = 0;
		pconf->timing.clk_ctrl = 0;
		if (cconf->pll_conf_num > 1)
			cconf->pll_config[1].done = 0;
		if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL)
			LCDERR("[%d]: %s: Out of clock range\n", pdrv->index, __func__);
	}
}

void lcd_clk_generate_prbs_clk(struct aml_lcd_drv_s *pdrv,
			       unsigned int enc_clk, unsigned long long bit_rate)
{
	struct lcd_clk_config_s *cconf;
	unsigned long long pll_fout, clk_div_in;
	unsigned int clk_div_out, clk_div_sel, xd, tcon_div_sel = 0;
	unsigned int od1, od2, od3, tmp_clk;
	int done = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	cconf->fout = enc_clk;
	pll_fout = bit_rate;
	clk_div_in = pll_fout;
	if (clk_div_in > cconf->data->pll_data[0]->div_in_fmax)
		goto lcd_clk_generate_prbs_clk_done;
	if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
		LCDPR("enc_clk=%d, pll_fout=%lld, clk_div_in=%lld\n",
		      cconf->fout, pll_fout, clk_div_in);
	}

	for (clk_div_sel = CLK_DIV_SEL_1;
		clk_div_sel <= cconf->data->pll_data[0]->div_sel_max; clk_div_sel++) {
		clk_div_out = clk_vid_pll_div_calc(clk_div_in, clk_div_sel, CLK_DIV_I2O);
		if (clk_div_out > cconf->data->pll_data[0]->div_out_fmax)
			continue;
		if (lcd_debug_print_flag & LCD_DBG_PR_CLK) {
			LCDPR("clk_div_out=%u, clk_div_sel=%s(%d)\n",
			      clk_div_out, lcd_clk_div_table[clk_div_sel].name, clk_div_sel);
		}

		done = 0;
		for (xd = 1; xd <= cconf->data->xd_max; xd++) {
			tmp_clk = cconf->fout * xd;
			if (tmp_clk > clk_div_out)
				break;
			if (tmp_clk == clk_div_out) {
				cconf->xd = xd;
				cconf->pll_config[0].div_sel = clk_div_sel;
				cconf->pll_config[0].pll_div_fout = clk_div_out;
				done = 1;
				if (lcd_debug_print_flag & LCD_DBG_PR_CLK)
					LCDPR("fout=%u, xd=%d\n", cconf->fout, xd);
				break;
			}
		}

		if (done)
			break;
	}
	if (done == 0)
		goto lcd_clk_generate_prbs_clk_done;

	done = pll_od_setting_generate(cconf, LCD_PLL_SEL_PHY, pll_fout);
	if (done == 0)
		goto lcd_clk_generate_prbs_clk_done;

	if (cconf->data->pll_data[0]->have_tcon_div) {
		done = 0;
		if (cconf->data->pll_data[0]->od_cnt == 3) {
			od1 = od_table[cconf->pll_config[0].pll_od1_sel];
			od2 = od_table[cconf->pll_config[0].pll_od2_sel];
			od3 = od_table[cconf->pll_config[0].pll_od3_sel];
		} else {
			od1 = od_table[cconf->pll_config[0].pll_od1_sel];
			od2 = 1;
			od3 = 1;
		}
		for (tcon_div_sel = 0; tcon_div_sel < 5; tcon_div_sel++) {
			if (tcon_div_table[tcon_div_sel] == od1 * od2 * od3) {
				cconf->pll_tcon_div_sel = tcon_div_sel;
					cconf->phy_clk =
						lcd_do_div(cconf->pll_config[0].pll_fvco,
							   tcon_div_table[tcon_div_sel]);
				done = 1;
				break;
			}
		}
	} else {
		cconf->phy_clk = cconf->pll_config[0].pll_fout;
	}

lcd_clk_generate_prbs_clk_done:
	if (done) {
		cconf->pll_config[0].done = 1;
	} else {
		cconf->pll_config[0].done = 0;
		LCDERR("[%d]: %s: Out of clock range\n", pdrv->index, __func__);
	}
}

int lcd_prbs_clk_check(unsigned int encl_clk, int encl_msr_id,
		       unsigned int fifo_clk, int fifo_msr_id, unsigned int cnt)
{
	unsigned long clk_check, clk_msr, temp;

	if (encl_msr_id == -1)
		goto lcd_prbs_clk_check_next;
	clk_check = encl_clk;
	clk_msr = clk_util_clk_msr(encl_msr_id) * 1000000;
	if (clk_check != clk_msr) {
		temp = lcd_diff(clk_check, clk_msr);
		if (temp >= PLL_CLK_CHECK_MAX) {
			if (lcd_debug_print_flag & LCD_DBG_PR_TEST) {
				LCDERR("encl_clk error: chk %ld, msr %ld, cnt: %d\n",
				       clk_check, clk_msr, cnt);
			}
			return -1;
		}
	}

lcd_prbs_clk_check_next:
	if (fifo_msr_id == -1)
		return 0;
	clk_check = fifo_clk;
	clk_msr = clk_util_clk_msr(fifo_msr_id) * 1000000;
	if (clk_check != clk_msr) {
		temp = lcd_diff(clk_check, clk_msr);
		if (temp >= PLL_CLK_CHECK_MAX) {
			if (lcd_debug_print_flag & LCD_DBG_PR_TEST) {
				LCDERR("fifo_clk error: chk %ld, msr %ld, cnt:%d\n",
				       clk_check, clk_msr, cnt);
			}
			return -1;
		}
	}

	return 0;
}

void lcd_set_vid_pll_div_dft(struct lcd_clk_config_s *cconf)
{
	unsigned int shift_val, shift_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV2)
		LCDPR("%s\n", __func__);

	lcd_clk_setb(HHI_VIID_CLK_CNTL, 0, VCLK2_EN, 1);
	udelay(5);

	/* Disable the div output clock */
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 19, 1);
	lcd_ana_setb(HHI_VID_PLL_CLK_DIV, 0, 15, 1);

	if (cconf->data->pll_data[0]->div_sel_max == CLK_DIV_SEL_1 ||
	    cconf->pll_config[0].div_sel > cconf->data->pll_data[0]->div_sel_max ||
	    cconf->pll_config[0].div_sel >= ARRAY_SIZE(lcd_clk_div_table)) {
		LCDERR("[0]: invalid clk divider\n");
		return;
	}

	shift_val = lcd_clk_div_table[cconf->pll_config[0].div_sel].shift_val;
	shift_sel = lcd_clk_div_table[cconf->pll_config[0].div_sel].shift_sel;

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
	struct lcd_pll_data_s *pll_data;
	struct lcd_pll_config_s *pll_config = NULL;
	int i;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf)
		return;

	data = cconf->data;
	LCDPR("[%d]: clk data init:\n"
		"xd_out_fmax:         %d\n"
		"ss_level_max:        %d\n"
		"ss_dep_base:         %d\n"
		"ss_dep_sel_max:      %d\n"
		"ss_str_m_max:        %d\n"
		"ss_freq_max:         %d\n"
		"ss_mode_max:         %d\n\n",
		pdrv->index,
		data->xd_out_fmax, data->ss_level_max,
		data->ss_dep_base, data->ss_dep_sel_max,
		data->ss_str_m_max,
		data->ss_freq_max, data->ss_mode_max);
	for (i = 0; i < cconf->pll_conf_num; i++) {
		if (!cconf->data->pll_data[i]) {
			LCDERR("[%d]: %s: cconf[%d] data is NULL\n", pdrv->index, __func__, i);
			return;
		}
		pll_config = &cconf->pll_config[i];
		pll_data = cconf->data->pll_data[i];
		LCDPR("[%d]: pll[%d] data init:\n"
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
			"div_out_fmax:        %d\n\n",
			pdrv->index, pll_config->pll_id,
			pll_data->pll_m_max, pll_data->pll_m_min,
			pll_data->pll_n_max, pll_data->pll_n_min,
			pll_data->pll_od_fb, pll_data->pll_frac_range,
			pll_data->pll_od_sel_max,
			pll_data->pll_ref_fmax, pll_data->pll_ref_fmin,
			pll_data->pll_vco_fmax, pll_data->pll_vco_fmin,
			pll_data->pll_out_fmax, pll_data->pll_out_fmin,
			pll_data->div_in_fmax, pll_data->div_out_fmax);
	}
}

void lcd_clk_config_print_dft(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_clk_config_s *cconf = NULL;
	struct lcd_pll_config_s *pll_config = NULL;
	int pll_num = 1, i = 0;

	cconf = get_lcd_clk_config(pdrv);
	if (!cconf || !cconf->data)
		return;

	if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL)
		pll_num = cconf->pll_conf_num;
	for (i = 0; i < pll_num; i++) {
		pll_config = &cconf->pll_config[i];
		LCDPR("[%d]: pll[%d] config:\n"
			"  pll_offset: 0x%x\n"
			"  pll_m:      %d\n"
			"  pll_n:      %d\n"
			"  pll_frac:   0x%x\n"
			"  pll_frac_half_shift: %d\n"
			"  pll_fvco:   %lluHz\n"
			"  pll_od1:    %d\n"
			"  pll_od2:    %d\n"
			"  pll_od3:    %d\n"
			"  pll_out:    %lldHz\n"
			"  div_sel:    %s(index %d)\n"
			"  pll_div_fout: %uHz\n\n",
			pdrv->index, pll_config->pll_id,
			pll_config->pll_offset,
			pll_config->pll_m, pll_config->pll_n,
			pll_config->pll_frac, pll_config->pll_frac_half_shift,
			pll_config->pll_fvco,
			pll_config->pll_od1_sel, pll_config->pll_od2_sel,
			pll_config->pll_od3_sel, pll_config->pll_fout,
			lcd_clk_div_table[pll_config->div_sel].name,
			pll_config->div_sel, pll_config->pll_div_fout);
	}

	LCDPR("[%d]: clk config:\n"
		"  pll_mode:   0x%x\n"
		"  pll_tcon_div_sel: %d\n"
		"  phy_clk:    %lldHz\n"
		"  edp_div0:   %d\n"
		"  edp_div1:   %d\n"
		"  xd:         %d\n"
		"  fout:       %uHz\n"
		"  vclk_sel:   %d\n",
		pdrv->index, cconf->pll_mode,
		cconf->pll_tcon_div_sel, cconf->phy_clk,
		edp_div0_table[cconf->edp_div0], edp_div1_table[cconf->edp_div1],
		cconf->xd, cconf->fout, cconf->data->vclk_sel);
	if (cconf->data->ss_support) {
		LCDPR("  ss_level:   %d\n"
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
