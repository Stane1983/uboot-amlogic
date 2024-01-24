// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/arch/secure_apb.h>
#include <amlogic/auge_sound.h>
#include <asm/io.h>
#include <common.h>

void update_bits(unsigned int reg, unsigned int mask, unsigned int val)
{
	unsigned int tmp, orig;

	orig = readl(reg);
	tmp = orig & ~mask;
	tmp |= val & mask;
	writel(tmp, reg);
}

static void earcrx_cmdc_set_clk(void)
{
	/* clock gate */
	update_bits(EE_AUDIO_CLK_GATE_EN1, 0x1 << 6, 0x1 << 6);
	/* cmdc clock, 10m */
	writel(0x86000031, EE_AUDIO_EARCRX_CMDC_CLK_CTRL);
}

static void earcrx_pll_refresh(void)
{
	/* pll tdc mode */
	update_bits(EARCRX_PLL_CTRL3,
			 0x1 << 15, 0x1 << 15);

	/* pll self reset */
	update_bits(EARCRX_PLL_CTRL0,
			 0x1 << 29, 0x1 << 29);
	update_bits(EARCRX_PLL_CTRL0,
			 0x1 << 29, 0x0 << 29);

	update_bits(EARCRX_PLL_CTRL3,
			 0x1 << 15, 0x0 << 15);
}

static void earcrx_cmdc_int_mask(void)
{
	/* set irq mask */
	writel((1 << 15) |  /* idle2_int */
		   (1 << 14) |  /* idle1_int */
		   (1 << 13) |  /* disc2_int */
		   (1 << 12) |  /* disc1_int */
		   (1 << 11) |  /* earc_int */
		   (1 << 10) |  /* hb_status_int */
		   (1 <<  9) |  /* losthb_int */
		   (1 <<  8) |  /* timeout_int */
		   (0 <<  7) |  /* status_ch_int */
		   (0 <<  6) |  /* int_rec_invalid_id */
		   (0 <<  5) |  /* int_rec_invalid_offset */
		   (0 <<  4) |  /* int_rec_unexp */
		   (0 <<  3) |  /* int_rec_ecc_err */
		   (0 <<  2) |  /* int_rec_parity_err */
		   (0 <<  1) |  /* int_recv_packet */
		   (0 <<  0),	 /* int_rec_time_out */
		   EARCRX_CMDC_INT_MASK);
}

static void earcrx_cmdc_init(int version)
{
	if (version == EARC_RX_ANA_V1) {
		writel(0x1  << 31 | /* earcrx_en_d2a */
			   0x10 << 24 | /* earcrx_cmdcrx_reftrim */
			   0x8  << 20 | /* earcrx_idr_trim */
			   0x10 << 15 | /* earcrx_rterm_trim */
			   0x4  << 12 | /* earcrx_cmdctx_ack_hystrim */
			   0x10 << 7  | /* earcrx_cmdctx_ack_reftrim */
			   0x1  << 4  | /* earcrx_cmdcrx_rcfilter_sel */
			   0x4  << 0,   /* earcrx_cmdcrx_hystrim */
			   EARCRX_ANA_CTRL0);
	} else {
		writel(0x1  << 31 | /* earcrx_en_d2a */
			   0x10 << 25 | /* earcrx_cmdcrx_reftrim */
			   0x10  << 20 | /* earcrx_idr_trim */
			   0x10 << 15 | /* earcrx_rterm_trim */
			   0x4  << 12 | /* earcrx_cmdctx_ack_hystrim */
			   0x10 << 7  | /* earcrx_cmdctx_ack_reftrim */
			   0x1  << 4  | /* earcrx_cmdcrx_rcfilter_sel */
			   0x4  << 0,   /* earcrx_cmdcrx_hystrim */
			   EARCRX_ANA_CTRL0);
		writel(0x1 << 11 | 0x1 << 10 | 0x8 << 4 | 0x8 << 0, EARCRX_ANA_CTRL1);
	}

	writel(0x2 << 20 | /* earcrx_pll_bias_adj */
		   0x4 << 16 | /* earcrx_pll_rou */
		   0x1 << 13,  /* earcrx_pll_dco_sdm_e */
		   EARCRX_PLL_CTRL3);

	writel(0x1 << 28 | /* earcrx_pll_en */
		   0x1 << 23 | /* earcrx_pll_dmacrx_sqout_rstn_sel */
		   0x1 << 10,  /* earcrx_pll_n */
		   EARCRX_PLL_CTRL0);

	/* eARC_RX_CONN_START Max time 200ms */
	update_bits(EARC_RX_CMDC_VSM_CTRL1,
			 0xfffff << 12,
			 200 << 12
			);
	update_bits(EARC_RX_CMDC_VSM_CTRL0,
			 0x1 << 19,
			 0x1 << 19
			);
}

void earcrx_init(int version)
{
	/* cmdc init */
	earcrx_cmdc_set_clk();
	earcrx_pll_refresh();
	earcrx_cmdc_init(version);
	earcrx_cmdc_int_mask();

	printf("%s done\n", __func__);
}
