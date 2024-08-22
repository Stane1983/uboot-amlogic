// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/clk_measure.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "dptx_clk_ctrl.h"
#include "../dptx_common.h"
#include "../dptx_regs.h"

struct dptx_clk_div_table_s dptx_clk_div_table[CLK_DIV_SEL_MAX] = {
	/* name,    divider,         num, den, shift_sel, shift_val*/
	{"1",       CLK_DIV_SEL_1,    1,   1,    0,       0xffff},
	{"2",       CLK_DIV_SEL_2,    1,   2,    0,       0x0aaa},
	{"3",       CLK_DIV_SEL_3,    1,   3,    0,       0x0db6},
	{"3.5",     CLK_DIV_SEL_3p5,  2,   7,    1,       0x36cc},
	{"3.75",    CLK_DIV_SEL_3p75, 4,   15,   2,       0x6666},
	{"4",       CLK_DIV_SEL_4,    1,   4,    0,       0x0ccc},
	{"5",       CLK_DIV_SEL_5,    1,   5,    2,       0x739c},
	{"6",       CLK_DIV_SEL_6,    1,   6,    0,       0x0e38},
	{"6.25",    CLK_DIV_SEL_6p25, 4,   25,   3,       0x0000},
	{"7",       CLK_DIV_SEL_7,    1,   7,    1,       0x3c78},
	{"7.5",     CLK_DIV_SEL_7p5,  2,   15,   2,       0x78f0},
	{"12",      CLK_DIV_SEL_12,   1,   12,   0,       0x0fc0},
	{"14",      CLK_DIV_SEL_14,   1,   14,   1,       0x3f80},
	{"15",      CLK_DIV_SEL_15,   1,   15,   2,       0x7f80},
	{"2.5",     CLK_DIV_SEL_2p5,  2,   5,    2,       0x5294},
	{"4.67",    CLK_DIV_SEL_4p67, 3,   14,   1,       0x0ccc},
	{"2.33",    CLK_DIV_SEL_2p33, 3,   7,    1,       0x1aaa},
	{"invalid", CLK_DIV_SEL_MAX,  1,   1,    0,       0xffff},
};

// static const unsigned int od_fb_table[2] = {1, 2};
// static const unsigned int od_table[6] = {1, 2, 4, 8, 16, 32};

/* **********************************
 * lcd controller operation
 * **********************************/
uint8_t dptx_clk_msr_check(uint32_t msr_id, uint32_t freq)
{
	unsigned int clk_msrd;

	if (msr_id == -1)
		return 0;

	clk_msrd = clk_util_clk_msr(msr_id) * 1000000;
	if (dptx_diff(freq, clk_msrd) >= PLL_CLK_CHECK_MAX) {
		DPTXPR(0, LOG_E, "%s[%d]: exp:%d, msr:%d\n", __func__, msr_id, freq, clk_msrd);
		return 1;
	}

	return 0;
}

uint8_t dptx_pll_wait_lock(uint32_t reg, uint8_t lock_bit)
{
	uint16_t pll_lock, wait_loop = PLL_WAIT_LOCK_CNT; /* 200 */

	do {
		udelay(50);
		pll_lock = dptx_ana_getb(reg, lock_bit, 1);
		wait_loop--;
	} while ((pll_lock == 0) && (wait_loop > 0));
	if (pll_lock == 0)
		return 0;

	return 1;
}

/* ****************************************************
 * lcd clk parameters calculate
 * ****************************************************
 */
unsigned long long dptx_clk_pll_div_calc(unsigned long long clk, uint8_t div_sel, uint8_t dir)
{
	unsigned long long clk_ret, num, den;

	if (div_sel >= CLK_DIV_SEL_MAX) {
		DPTXPR(0, LOG_E, "clk_div_sel: Invalid parameter\n");
		return 0;
	}

	if (dir == CLK_DIV_I2O) {
		num = dptx_clk_div_table[div_sel].num;
		den = dptx_clk_div_table[div_sel].den;
	} else {
		num = dptx_clk_div_table[div_sel].den;
		den = dptx_clk_div_table[div_sel].num;
	}
	clk_ret = dptx_div_around(clk * num, den);

	return clk_ret;
}
