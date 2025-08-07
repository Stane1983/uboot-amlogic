/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef _DPTX_CLK_CTRL_H
#define _DPTX_CLK_CTRL_H

#include <linux/types.h>
#include <amlogic/media/vout/eDPTX/DPTX.h>

#define PLL_CLK_CHECK_MAX    1000000000 /* Hz */
uint8_t dptx_clk_msr_check(uint32_t msr_id, uint32_t freq);
uint8_t dptx_pll_wait_lock(uint32_t reg, uint8_t lock_bit);

#define PLL_FVCO_ERR_MAX    2000 /* Hz */
unsigned long long dptx_clk_pll_div_calc(unsigned long long clk, uint8_t div_sel, uint8_t dir);

struct dptx_clk_op_s *dptx_clk_op_init_t7(struct dptx_drv_s *dptx);

#define PLL_RETRY_MAX        20

struct dptx_pll_data_s {
	/* clk path node parameters */
	u32 pll_od_fb;
	u16 pll_m_range[2];
	u32 pll_frac_range;
	u8  pll_frac_sign_bit;
	// u32 pll_od_sel_max;
	// u32 pll_ref_range[2];
	u64 pll_vco_range[2];
	u64 pll_out_range[2];
	u64 pll_div_in_fmax;
	u32 pll_div_out_fmax;
	u32 od_cnt;
	u32 div_sel_max;

	u32 xd_out_fmax;
	u16 xd_max;

	// u8 vclk_sel;
	// u16 venc_clk_msr_id;

	u16 ss_level_max;
	u16 ss_freq_max;
	u16 ss_mode_max;
	u16 ss_dep_base;
	u16 ss_dep_sel_max;
	u16 ss_str_m_max;
};

struct dptx_clk_op_s {
	void (*clk_config_print)(struct dptx_drv_s *dptx);
	void (*clktree_set)(struct dptx_drv_s *dptx);

	void (*link_clk_config)(struct dptx_drv_s *dptx, uint8_t port, u8 dptx_link_rate);
	void (*link_clk_set)(struct dptx_drv_s *dptx, uint8_t port);

	void (*vid_clk_config)(struct dptx_drv_s *dptx, u32 pixel_clk);
	void (*vid_clk_set)(struct dptx_drv_s *dptx);

	void (*clk_ssc_switch)(struct dptx_drv_s *dptx, uint8_t port, u8 status);

	// void (*prbs)(struct dptx_drv_s *dptx);
};

/* **********************************
 * Spread Spectrum
 * **********************************
 */
#define LCD_SS_STEP_BASE            500 /* ppm */

/* **********************************
 * pll & clk parameter
 * **********************************/
/* ******** clk calculation *********/
#define PLL_WAIT_LOCK_CNT           200
 /* frequency unit: kHz */
#define COMMON_OSC_FREQ             (24 * 1000000)
/* clk max error */
#define MAX_ERROR                   (2 * 1000000)

/* ******** register bit ******** */
/* divider */
#define DIV_PRE_SEL_MAX             6

#define CLK_DIV_I2O     0
#define CLK_DIV_O2I     1
enum div_sel_e {
	CLK_DIV_SEL_1    = 0,
	CLK_DIV_SEL_2    = 1,
	CLK_DIV_SEL_3    = 2,
	CLK_DIV_SEL_3p5  = 3,
	CLK_DIV_SEL_3p75 = 4,
	CLK_DIV_SEL_4    = 5,
	CLK_DIV_SEL_5    = 6,
	CLK_DIV_SEL_6    = 7,
	CLK_DIV_SEL_6p25 = 8,
	CLK_DIV_SEL_7    = 9,
	CLK_DIV_SEL_7p5  = 10,
	CLK_DIV_SEL_12   = 11,
	CLK_DIV_SEL_14   = 12,
	CLK_DIV_SEL_15   = 13,
	CLK_DIV_SEL_2p5  = 14,
	CLK_DIV_SEL_4p67 = 15,
	CLK_DIV_SEL_2p33 = 16,
	CLK_DIV_SEL_MAX  = 18,
};

struct dptx_clk_div_table_s {
	char *name;
	unsigned char divider;
	unsigned char num;
	unsigned char den;
	unsigned char shift_sel;
	unsigned short shift_val;
};

extern struct dptx_clk_div_table_s dptx_clk_div_table[CLK_DIV_SEL_MAX];

#endif
