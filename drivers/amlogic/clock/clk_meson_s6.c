// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/amlogic/arch-s6/clock.h>
#include <asm/io.h>
#include <linux/clk-provider.h>
#include <div64.h>
#include <dm.h>
#include <dt-bindings/amlogic/clock/s6-clkc.h>
#include <amlogic/clk_meson.h>
#include <amlogic/clk-pll.h>

/* change it later */
#define SYS_CLK		166666666
#define FIXED_PLL	2000000000ULL  /* fix_pll is fixed to 2G */

static struct clk *gp0_pll, *hifi_pll;

/* clk81 gates, sys_clk */
static struct meson_gate gates[] = {
	{CLKID_SPICC_0, S6_CLKCTRL_SPICC_CLK_CTRL, 6},
	{CLKID_SARADC, S6_CLKCTRL_SAR_CLK_CTRL, 8},
	{CLKID_SD_EMMC_A, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 7},
	{CLKID_SD_EMMC_B, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 23},
	{CLKID_SD_EMMC_C, S6_CLKCTRL_NAND_CLK_CTRL, 7},
};

static unsigned int saradc_parents[] = {CLKID_XTAL, CLKID_SYS_CLK,
					CLKID_FCLK_DIV4, CLKID_FCLK_DIV5};

static unsigned int sd_emmc_parents[] = {CLKID_XTAL, CLKID_FCLK_DIV2,
	CLKID_FCLK_DIV3, CLKID_UNREALIZED, CLKID_FCLK_DIV2P5,
	CLKID_UNREALIZED, CLKID_UNREALIZED, CLKID_GP0_PLL};

static unsigned int spicc_parents[] = {CLKID_XTAL, CLKID_SYS_CLK,
	CLKID_FCLK_DIV4, CLKID_FCLK_DIV3, CLKID_FCLK_DIV2,
	CLKID_FCLK_DIV5, CLKID_FCLK_DIV7, CLKID_UNREALIZED};

static struct meson_mux muxes[] = {
	{CLKID_SPICC_0_MUX, S6_CLKCTRL_SPICC_CLK_CTRL, 7,  0x7, spicc_parents, ARRAY_SIZE(spicc_parents)},
	{CLKID_SARADC_MUX, S6_CLKCTRL_SAR_CLK_CTRL, 9, 0x3, saradc_parents, ARRAY_SIZE(saradc_parents)},
	{CLKID_SD_EMMC_A_MUX, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 9, 0x7, sd_emmc_parents, ARRAY_SIZE(sd_emmc_parents)},
	{CLKID_SD_EMMC_B_MUX, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 25, 0x7, sd_emmc_parents, ARRAY_SIZE(sd_emmc_parents)},
	{CLKID_SD_EMMC_C_MUX, S6_CLKCTRL_NAND_CLK_CTRL, 9, 0x7, sd_emmc_parents, ARRAY_SIZE(sd_emmc_parents)},
};

static struct meson_div divs[] = {
	{CLKID_SPICC_0_DIV, S6_CLKCTRL_SPICC_CLK_CTRL, 0, 6, CLKID_SPICC_0_MUX},
	{CLKID_SARADC_DIV, S6_CLKCTRL_SAR_CLK_CTRL, 0, 8, CLKID_SARADC_MUX},
	{CLKID_SD_EMMC_A_DIV, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 0, 7, CLKID_SD_EMMC_A_MUX},
	{CLKID_SD_EMMC_B_DIV, S6_CLKCTRL_SD_EMMC_CLK_CTRL, 16, 7, CLKID_SD_EMMC_B_MUX},
	{CLKID_SD_EMMC_C_DIV, S6_CLKCTRL_NAND_CLK_CTRL, 0, 7, CLKID_SD_EMMC_C_MUX},
};

static struct parm meson_gp0_pll_parm[5] = {
	{S6_ANACTRL_GP0PLL_CTRL0, 0, 9}, /* pm */
	{S6_ANACTRL_GP0PLL_CTRL0, 12, 3}, /* pn */
	{S6_ANACTRL_GP0PLL_CTRL0, 16, 3}, /* pod */
	{S6_ANACTRL_GP0PLL_CTRL1, 0, 19}, /* pfrac */
	{S6_ANACTRL_GP0PLL_CTRL4, 12, 1}, /* pen0p5 */
};

static const struct pll_params_table gp0_pll_table[] = {
	{ 1536000000, 128, 0, 0, 0},
	{ 1152000000, 192, 0, 1, 0}
};

static const struct reg_sequence gp0_pll_init_regs[] = {
	{ .reg_off = S6_ANACTRL_GP0PLL_CTRL0, .def = 0x01010000 },
	{ .reg_off = S6_ANACTRL_GP0PLL_CTRL1, .def = 0x11480000 },
	{ .reg_off = S6_ANACTRL_GP0PLL_CTRL2, .def = 0x1215b010 },
	{ .reg_off = S6_ANACTRL_GP0PLL_CTRL3, .def = 0x20008010 },
	{ .reg_off = S6_ANACTRL_GP0PLL_CTRL4, .def = 0x00001000 }
};

static const struct aml_pll_data gp0_pll_data = {
	.en = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 12,
		.width   = 3,
	},
	.frac = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL1,
		.shift   = 0,
		.width   = 19,
	},
	.od = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 20,
		.width   = 3,
	},
	.l = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.l_rst = {
		.reg_off = S6_ANACTRL_GP0PLL_CTRL3,
		.shift   = 29,
		.width   = 1,
	},
	.table = gp0_pll_table,
	.table_count = ARRAY_SIZE(gp0_pll_table),
	.init_regs = gp0_pll_init_regs,
	.init_count = ARRAY_SIZE(gp0_pll_init_regs),
	.flags = CLK_AML_PLL_POWER_OF_TWO |
		 CLK_AML_PLL_RSTN |
		 CLK_AML_PLL_FIXED_EN0P5 |
		 CLK_AML_PLL_NOINIT_ENABLED,
};

static const struct pll_params_table hifi_pll_table[] = {
	{ 491520000, 163, 0, 2, 84000 }
};

static const struct reg_sequence hifi_pll_init_regs[] = {
	{ .reg_off = S6_ANACTRL_HIFI0PLL_CTRL0, .def = 0x01010000 },
	{ .reg_off = S6_ANACTRL_HIFI0PLL_CTRL1, .def = 0x11480000 },
	{ .reg_off = S6_ANACTRL_HIFI0PLL_CTRL2, .def = 0x121db010 },
	{ .reg_off = S6_ANACTRL_HIFI0PLL_CTRL3, .def = 0x20008010 },
	{ .reg_off = S6_ANACTRL_HIFI0PLL_CTRL4, .def = 0x00001000 }
};

static const struct aml_pll_data hifi_pll_data = {
	.en = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 12,
		.width   = 3,
	},
	.frac = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL1,
		.shift   = 0,
		.width   = 19,
	},
	.od = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 20,
		.width   = 3,
	},
	.l = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.l_rst = {
		.reg_off = S6_ANACTRL_HIFI0PLL_CTRL3,
		.shift   = 29,
		.width   = 1,
	},
	.table = hifi_pll_table,
	.table_count = ARRAY_SIZE(hifi_pll_table),
	.init_regs = hifi_pll_init_regs,
	.init_count = ARRAY_SIZE(hifi_pll_init_regs),
	.flags = CLK_AML_PLL_ROUND_CLOSEST |
		 CLK_AML_PLL_POWER_OF_TWO |
		 CLK_AML_PLL_FIXED_FRAC_WEIGHT_PRECISION |
		 CLK_AML_PLL_FIXED_EN0P5 |
		 CLK_AML_PLL_RSTN |
		 CLK_AML_PLL_NOINIT_ENABLED,
};

static int meson_clk_enable(struct clk *clk)
{
	if (clk->id == CLKID_GP0_PLL)
		return clk_pll_ops.enable(gp0_pll);
	else if (clk->id == CLKID_HIFI_PLL)
		return clk_pll_ops.enable(hifi_pll);

	return meson_set_gate_by_id(clk, gates, ARRAY_SIZE(gates), true);
}

static int meson_clk_disable(struct clk *clk)
{
	if (clk->id == CLKID_GP0_PLL)
		return clk_pll_ops.disable(gp0_pll);
	else if (clk->id == CLKID_HIFI_PLL)
		return clk_pll_ops.disable(hifi_pll);

	return meson_set_gate_by_id(clk, gates, ARRAY_SIZE(gates), false);
}

static ulong meson_pll_get_rate(struct clk *clk, unsigned long id)
{
	struct meson_clk *priv = dev_get_priv(clk->dev);
	struct parm *pm, *pn, *pod, *pfrac, *pen0p5;
	unsigned long parent_rate_mhz = clk_get_rate(&priv->clkin)/1000000;
	unsigned long rate;
	unsigned long frac_rate;
	u16 n, m, od, frac = 0, en0p5 = 0;
	u32 reg;

	switch (id) {
	case CLKID_GP0_PLL:
		pm = &meson_gp0_pll_parm[0];
		pn = &meson_gp0_pll_parm[1];
		pod = &meson_gp0_pll_parm[2];
		pfrac = &meson_gp0_pll_parm[3];
		pen0p5 = &meson_gp0_pll_parm[4];
		break;
	default:
		return -ENOENT;
	}

	reg = readl(priv->addr + pn->reg_off);
	n = PARM_GET(pn->width, pn->shift, reg);

	reg = readl(priv->addr + pm->reg_off);
	m = PARM_GET(pm->width, pm->shift, reg);

	/* there is OD in C1 */
	reg = readl(priv->addr + pod->reg_off);
	od = PARM_GET(pod->width, pod->shift, reg);

	rate = parent_rate_mhz * m;
	if (pfrac->width > 2) {
		reg = readl(priv->addr + pfrac->reg_off);
		frac = PARM_GET(pfrac->width, pfrac->shift, reg);
		if (frac) {
			frac_rate = parent_rate_mhz * frac;
			if ((frac >> (pfrac->width - 1)) & 0x1)
				rate -= frac_rate / (1 << (pfrac->width - 2));
			else
				rate += frac_rate / (1 << (pfrac->width - 2));
		}
	}

	if (pen0p5->width) {
		reg = readl(priv->addr + pen0p5->reg_off);
		en0p5 = PARM_GET(pen0p5->width, pen0p5->shift, reg);
		if (en0p5)
			rate = rate >> 1;
	}

	return ((rate >> n) >> od) * 1000000;
}

static ulong meson_clk_get_rate_by_id(struct clk *clk, ulong id)
{
	ulong rate;
	struct meson_clk *priv = dev_get_priv(clk->dev);

	switch (id) {
	case CLKID_XTAL:
		rate = clk_get_rate(&priv->clkin);
		break;
	case CLKID_GP0_PLL:
		rate = meson_pll_get_rate(clk, id);
		break;
	case CLKID_FCLK_DIV2:
		rate = FIXED_PLL / 2;
		break;
	case CLKID_FCLK_DIV3:
		rate = FIXED_PLL / 3;
		break;
	case CLKID_FCLK_DIV4:
		rate = FIXED_PLL / 4;
		break;
	case CLKID_FCLK_DIV5:
		rate = FIXED_PLL / 5;
		break;
	case CLKID_FCLK_DIV7:
		rate = FIXED_PLL / 7;
		break;
	case CLKID_FCLK_DIV2P5:
		rate = FIXED_PLL * 2 / 5;
		break;
	/* sys clk has realized in rom code*/
	case CLKID_SYS_CLK:
		rate = SYS_CLK;
		break;
	default:
		/* only for DIV type, others such as GATE/MUX is always 0 */
		rate = priv->actual_rate;
		break;
	}

	return rate;
}

static ulong meson_clk_get_rate(struct clk *clk)
{
	if (clk->id == CLKID_GP0_PLL)
		return clk_pll_ops.get_rate(gp0_pll);
	else if (clk->id == CLKID_HIFI_PLL)
		return clk_pll_ops.get_rate(hifi_pll);

	return meson_clk_get_rate_by_id(clk, clk->id);
}

static ulong meson_clk_set_rate(struct clk *clk, ulong rate)
{
	ulong parent_rate;
	int parent;
	unsigned int div_val;
	struct meson_clk *priv = dev_get_priv(clk->dev);
	unsigned int i;
	struct meson_div *div = NULL;

	if (clk->id == CLKID_GP0_PLL)
		return clk_pll_ops.set_rate(gp0_pll, rate);
	else if (clk->id == CLKID_HIFI_PLL)
		return clk_pll_ops.set_rate(hifi_pll, rate);

	for (i = 0; i < ARRAY_SIZE(divs); i++) {
		if (clk->id == divs[i].index)
			div = &divs[i];
	}
	parent = meson_clk_get_mux_parent(clk, muxes,
					ARRAY_SIZE(muxes), div->parent_index);
	if (parent == -1)
		parent = div->parent_index;

	parent_rate = meson_clk_get_rate_by_id(clk, parent);

	div_val = DIV_ROUND_CLOSEST(parent_rate, rate) - 1;

	priv->actual_rate = DIV_ROUND_CLOSEST(parent_rate, div_val + 1);
	meson_clk_set_div(priv, div, div_val);

	return 0;
}

static int meson_clk_set_parent(struct clk* clk, struct clk* parent_clk)
{
	return meson_mux_set_parent_by_id(clk, muxes, ARRAY_SIZE(muxes), parent_clk);
}

static struct clk_ops meson_clk_ops = {
	.disable	= meson_clk_disable,
	.enable		= meson_clk_enable,
	.get_rate	= meson_clk_get_rate,
	.set_rate 	= meson_clk_set_rate,
	.set_parent 	= meson_clk_set_parent,
};

static int meson_clk_probe(struct udevice *dev)
{
	struct meson_clk *priv = dev_get_priv(dev);

	clk_get_by_name(dev, "xtal", &priv->clkin);
	priv->addr = dev_read_addr_ptr(dev);

	debug("meson-clk: probed at addr %p\n", priv->addr);

	gp0_pll = aml_clk_register_pll("gp0_pll", "xtal-clk", 0, priv->addr,
				       &gp0_pll_data);
	clk_dm(CLKID_GP0_PLL, gp0_pll);
	hifi_pll = aml_clk_register_pll("hifi_pll", "xtal-clk", 0, priv->addr,
					&hifi_pll_data);
	clk_dm(CLKID_HIFI_PLL, hifi_pll);

	return 0;
}

static const struct udevice_id meson_clk_ids[] = {
	{ .compatible = "amlogic,s7d-clkc" },
	{ }
};

U_BOOT_DRIVER(meson_clk) = {
	.name		= "meson-clk-s7d",
	.id		= UCLASS_CLK,
	.of_match	= meson_clk_ids,
	.priv_auto  = sizeof(struct meson_clk),
	.ops		= &meson_clk_ops,
	.probe		= meson_clk_probe,
};
