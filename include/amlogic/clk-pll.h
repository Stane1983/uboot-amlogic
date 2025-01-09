/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _CLK_PLL_H
#define _CLK_PLL_H

#include <clk.h>

/* PLL Parameters */
struct pll_parm {
	u16 reg_off;
	u8 shift;
	u8 width;
};

struct pll_params_table {
	unsigned long	rate;
	unsigned int	m;
	unsigned int	n;
	unsigned int	od;
	unsigned int	frac;
};

struct reg_sequence {
	unsigned int reg_off;
	unsigned int def;
	unsigned int delay_us;
};

#define CLK_AML_PLL_ROUND_CLOSEST			BIT(0)
#define CLK_AML_PLL_NOINIT_ENABLED			BIT(1)
#define CLK_AML_PLL_FIXED_FRAC_WEIGHT_PRECISION		BIT(2)
#define CLK_AML_PLL_POWER_OF_TWO			BIT(3)
#define CLK_AML_PLL_FIXED_N				BIT(4)
#define CLK_AML_PLL_FIXED_EN0P5				BIT(5)
#define CLK_AML_PLL_RSTN				BIT(6)
#define CLK_AML_PLL_L_RSTN				BIT(7)

struct aml_pll_data {
	struct pll_parm en;
	struct pll_parm l;
	struct pll_parm rst;
	struct pll_parm l_rst;
	struct pll_parm m;
	struct pll_parm n;
	struct pll_parm frac;
	struct pll_parm od;
	const struct reg_sequence *init_regs;
	unsigned int init_count;
	const struct pll_params_table *table;
	unsigned int table_count;
	unsigned int flags;
};

/* PLL Structure */
struct aml_pll {
	struct clk clk;
	void __iomem *addr;
	const struct aml_pll_data *data;
//	unsigned int parent_index;
};

extern const struct clk_ops clk_pll_ops;

struct clk *aml_clk_register_pll(const char *name, const char *parent_name,
				 unsigned long flags, void __iomem *addr,
				 const struct aml_pll_data *data);

#endif
