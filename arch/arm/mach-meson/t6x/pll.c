// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <../clk-pll.h>
#include <linux/kernel.h>

#define SECURE_PLL_CLK		0x82000098
#define SECURE_CPU_CLK		0x82000099

/* PLL secure clock index */
enum sec_pll {
	SECID_SYS0_DCO_PLL = 0,
	SECID_SYS0_DCO_PLL_DIS,
	SECID_SYS0_PLL_OD,
	SECID_SYS1_DCO_PLL,
	SECID_SYS1_DCO_PLL_DIS,
	SECID_SYS1_PLL_OD,
	SECID_CPU_CLK_SEL,
	SECID_CPU_CLK_RD,
	SECID_CPU_CLK_DYN,
	SECID_DSU_CLK_SEL,
	SECID_DSU_CLK_RD,
	SECID_DSU_CLK_DYN,
	SECID_DSU_FINAL_CLK_SEL,
	SECID_DSU_FINAL_CLK_RD,
	SECID_PLL_TEST,
};

int t6x_sys0_pll_prepare(struct meson_clk_pll_data *pll)
{
	struct arm_smccc_res res;

	/* store rate */
	meson_pll_store_rate(pll);

	/* Set fixed clk to 1G, Switch to fixed clk first */
	arm_smccc_smc(SECURE_CPU_CLK, SECID_CPU_CLK_DYN, 1, 0, 0, 0, 0, 0, &res);

	meson_switch_cpu_clk(SECURE_CPU_CLK, SECID_CPU_CLK_SEL, 0);

	return 0;
}

void t6x_sys0_pll_unprepare(struct meson_clk_pll_data *pll)
{
	/* restore rate */
	meson_pll_restore_rate(pll);

	/* Switch back cpu to sys pll */
	meson_switch_cpu_clk(SECURE_CPU_CLK, SECID_CPU_CLK_SEL, 1);
}

static const struct pll_params_table t6x_sys_pll_table[] = {
	PLL_PARAMS(75, 1, 0), /*DCO=1800M OD=1800M restore*/
	PLL_PARAMS(80, 1, 0), /*DCO=1920M OD=1920M */
	PLL_PARAMS(126, 1, 1), /*DCO=1512M OD=1512M*/
	{ /* sentinel */ }
};

static unsigned int t6x_sys_pll_default_rate[] = {1512, 1920};

static const struct reg_sequence t6x_sys0_pll_init_regs[] = {
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x20011086 },
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x30011086 },
	{ .reg = ANACTRL_SYS0PLL_CTRL1, .def = 0x3420500f },
	{ .reg = ANACTRL_SYS0PLL_CTRL2, .def = 0x00023041 },
	{ .reg = ANACTRL_SYS0PLL_CTRL3, .def = 0x00000000, .delay_us = 20 },
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x10011086, .delay_us = 20  },
	{ .reg = ANACTRL_SYS0PLL_CTRL2, .def = 0x00023001 }
};

struct meson_clk_pll_data t6x_sys0_pll = {
	.name = "sys0",
	.en = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 0,
		.width   = 8,
	},
	.n = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift	 = 12,
		.width	 = 3,
	},
	.l = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = t6x_sys0_pll_prepare,
		.pll_unprepare = t6x_sys0_pll_unprepare,
		.pll_disable = meson_secure_pll_disable,
		.pll_set_rate = meson_secure_pll_set_rate,
		.pll_set_parm_rate = meson_secure_pll_set_parm_rate,
		.pll_rate_to_msr = meson_div16_to_msr,

	},
	.def_rate = t6x_sys_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_sys_pll_default_rate),
	.table = t6x_sys_pll_table,
	.init_regs = t6x_sys0_pll_init_regs,
	.init_count = ARRAY_SIZE(t6x_sys0_pll_init_regs),
	.smc_id = SECURE_PLL_CLK,
	.secid_disable = SECID_SYS0_DCO_PLL_DIS,
	.secid_test = SECID_PLL_TEST,
	.secid = SECID_SYS0_DCO_PLL,
	.clkmsr_id = 23,
	.clkmsr_margin = 10,
};

int t6x_sys1_pll_prepare(struct meson_clk_pll_data *pll)
{
	struct arm_smccc_res res;

	/* store rate */
	meson_pll_store_rate(pll);

	/* Set fixed clk to 1G, Switch to fixed clk first */
	arm_smccc_smc(SECURE_CPU_CLK, SECID_DSU_CLK_DYN, 1, 0, 0, 0, 0, 0, &res);

	arm_smccc_smc(SECURE_CPU_CLK, SECID_DSU_CLK_SEL, 1, 0, 0, 0, 0, 0, &res);

	return 0;
}

void t6x_sys1_pll_unprepare(struct meson_clk_pll_data *pll)
{
	struct arm_smccc_res res;

	/* restore rate */
	meson_pll_restore_rate(pll);

	/* Switch back cpu to sys1 pll */
	arm_smccc_smc(SECURE_CPU_CLK, SECID_DSU_CLK_SEL, 3, 0, 0, 0, 0, 0, &res);
}

static const struct pll_params_table t6x_sys1_pll_table[] = {
	PLL_PARAMS(127, 1, 1), /*DCO=3048M OD=1524M restore*/
	PLL_PARAMS(80, 1, 0), /*DCO=1920M OD=1920M */
	PLL_PARAMS(126, 1, 1), /*DCO=1512M OD=1512M*/
	{ /* sentinel */ }
};

static unsigned int t6x_sys1_pll_default_rate[] = {1512, 1920};

static const struct reg_sequence t6x_sys1_pll_init_regs[] = {
	{ .reg = ANACTRL_SYS1PLL_CTRL0, .def = 0x20011086 },
	{ .reg = ANACTRL_SYS1PLL_CTRL0, .def = 0x30011086 },
	{ .reg = ANACTRL_SYS1PLL_CTRL1, .def = 0x3420500f },
	{ .reg = ANACTRL_SYS1PLL_CTRL2, .def = 0x00023041 },
	{ .reg = ANACTRL_SYS1PLL_CTRL3, .def = 0x00000000, .delay_us = 20 },
	{ .reg = ANACTRL_SYS1PLL_CTRL0, .def = 0x10011086, .delay_us = 20  },
	{ .reg = ANACTRL_SYS1PLL_CTRL2, .def = 0x00023001 }
};

struct meson_clk_pll_data t6x_sys1_pll = {
	.name = "sys1",
	.en = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift   = 0,
		.width   = 8,
	},
	.n = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift	 = 12,
		.width	 = 3,
	},
	.l = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_SYS1PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = t6x_sys1_pll_prepare,
		.pll_unprepare = t6x_sys1_pll_unprepare,
		.pll_disable = meson_secure_pll_disable,
		.pll_set_rate = meson_secure_pll_set_rate,
		.pll_set_parm_rate = meson_secure_pll_set_parm_rate,
		.pll_rate_to_msr = meson_div16_to_msr,

	},
	.def_rate = t6x_sys1_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_sys1_pll_default_rate),
	.table = t6x_sys1_pll_table,
	.init_regs = t6x_sys1_pll_init_regs,
	.init_count = ARRAY_SIZE(t6x_sys1_pll_init_regs),
	.smc_id = SECURE_PLL_CLK,
	.secid_disable = SECID_SYS1_DCO_PLL_DIS,
	.secid_test = SECID_PLL_TEST,
	.secid = SECID_SYS1_DCO_PLL,
	.clkmsr_id = 23,
	.clkmsr_margin = 10,
};

static unsigned int t6x_gp0_pll_default_rate[] = {576, 768};

/* set the od = 2, clkmsr can not measure 6000M */
static const struct pll_params_table t6x_gp0_pll_table[] = {
	PLL_PARAMS(192, 1, 1), /* DCO = 3024M  OD = 1152M for restore */
	PLL_PARAMS(192, 1, 2), /* DCO = 3024M  OD = 576M */
	PLL_PARAMS(256, 1, 2), /* DCO = 3072M  OD = 768M */
	{ /* sentinel */  }
};

static const struct reg_sequence t6x_gp0_init_regs[] = {
	{ .reg = ANACTRL_GP0PLL_CTRL0, .def = 0x200104c0 },
	{ .reg = ANACTRL_GP0PLL_CTRL0, .def = 0x300104c0 },
	{ .reg = ANACTRL_GP0PLL_CTRL1, .def = 0x00000000 },
	{ .reg = ANACTRL_GP0PLL_CTRL2, .def = 0x00040000 },
	{ .reg = ANACTRL_GP0PLL_CTRL3, .def = 0x0f0da000 },
	{ .reg = ANACTRL_GP0PLL_CTRL0, .def = 0x100104c0 },
	{ .reg = ANACTRL_GP0PLL_CTRL3, .def = 0x0f0da200 }
};

struct meson_clk_pll_data t6x_gp0_pll = {
	.name = "gp0",
	.en = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift	 = 10,
		.width	 = 3,
	},
	.en0p5 = {
		.reg = ANACTRL_GP0PLL_CTRL3,
		.shift	 = 26,
		.width	 = 1,
	},
	.l = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = meson_pll_store_rate,
		.pll_unprepare = meson_pll_restore_rate,
	},
	.flags = CLK_MESON_PLL_FIXED_EN0P5,
	.def_rate = t6x_gp0_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_gp0_pll_default_rate),
	.table = t6x_gp0_pll_table,
	.init_regs = t6x_gp0_init_regs,
	.init_count = ARRAY_SIZE(t6x_gp0_init_regs),
	.clkmsr_id = 20,
	.clkmsr_margin = 2
};

static unsigned int t6x_gp1_pll_default_rate[] = {576, 768};

/* set the od = 2, clkmsr can not measure 6000M */
static const struct pll_params_table t6x_gp1_pll_table[] = {
	PLL_PARAMS(70, 1, 0), /* DCO = 1680M  OD = 1680M for restore */
	PLL_PARAMS(96, 1, 2), /* DCO = 3024M  OD = 576M */
	PLL_PARAMS(128, 1, 2), /* DCO = 3072M  OD = 768M */
	{ /* sentinel */  }
};

static const struct reg_sequence t6x_gp1_init_regs[] = {
	{ .reg = ANACTRL_GP1PLL_CTRL0, .def = 0x20010046 },
	{ .reg = ANACTRL_GP1PLL_CTRL0, .def = 0x30010046 },
	{ .reg = ANACTRL_GP1PLL_CTRL1, .def = 0x3420500f },
	{ .reg = ANACTRL_GP1PLL_CTRL2, .def = 0x00023041 },
	{ .reg = ANACTRL_GP1PLL_CTRL3, .def = 0x00000000, .delay_us = 20 },
	{ .reg = ANACTRL_GP1PLL_CTRL0, .def = 0x10010046, .delay_us = 20 },
	{ .reg = ANACTRL_GP1PLL_CTRL2, .def = 0x00023001 }
};

struct meson_clk_pll_data t6x_gp1_pll = {
	.name = "gp1",
	.en = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift	 = 12,
		.width	 = 3,
	},
	.l = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_GP1PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = meson_pll_store_rate,
		.pll_unprepare = meson_pll_restore_rate,
	},
	.def_rate = t6x_gp1_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_gp1_pll_default_rate),
	.table = t6x_gp1_pll_table,
	.init_regs = t6x_gp1_init_regs,
	.init_count = ARRAY_SIZE(t6x_gp1_init_regs),
	.clkmsr_id = 21,
	.clkmsr_margin = 2
};

static unsigned int t6x_gp2_pll_default_rate[] = {576, 768};

/* set the od = 2, clkmsr can not measure 6000M */
static const struct pll_params_table t6x_gp2_pll_table[] = {
	PLL_PARAMS(198, 1, 2), /* DCO = 2376M  OD = 594M for restore */
	PLL_PARAMS(192, 1, 2), /* DCO = 3024M  OD = 576M */
	PLL_PARAMS(256, 1, 2), /* DCO = 3072M  OD = 768M */
	{ /* sentinel */  }
};

static const struct reg_sequence t6x_gp2_init_regs[] = {
	{ .reg = ANACTRL_GP2PLL_CTRL0, .def = 0x2001110b },
	{ .reg = ANACTRL_GP2PLL_CTRL0, .def = 0x3001110b },
	{ .reg = ANACTRL_GP2PLL_CTRL1, .def = 0x03a00000 },
	{ .reg = ANACTRL_GP2PLL_CTRL2, .def = 0x00040000 },
	{ .reg = ANACTRL_GP2PLL_CTRL3, .def = 0x0f0da000 },
	{ .reg = ANACTRL_GP2PLL_CTRL0, .def = 0x1001110b },
	{ .reg = ANACTRL_GP2PLL_CTRL3, .def = 0x0f0da200 }
};

struct meson_clk_pll_data t6x_gp2_pll = {
	.name = "gp2",
	.en = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift	 = 10,
		.width	 = 3,
	},
	.en0p5 = {
		.reg = ANACTRL_GP2PLL_CTRL3,
		.shift	 = 26,
		.width	 = 1,
	},
	.l = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_GP2PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = meson_pll_store_rate,
		.pll_unprepare = meson_pll_restore_rate,
	},
	.flags = CLK_MESON_PLL_FIXED_EN0P5,
	.def_rate = t6x_gp2_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_gp2_pll_default_rate),
	.table = t6x_gp2_pll_table,
	.init_regs = t6x_gp2_init_regs,
	.init_count = ARRAY_SIZE(t6x_gp2_init_regs),
	.clkmsr_id = 25,
	.clkmsr_margin = 2
};

static const struct pll_params_table t6x_hifi_pll_table[] = {
	PLL_PARAMS(150, 1, 2), /* DCO = 1800M OD = 4 PLL = 450M */
	PLL_PARAMS(163, 1, 2), /* DCO = 1944M OD = 4 PLL = 489M */
	{ /* sentinel */  }
};

static unsigned int t6x_hifi_pll_default_rate[] = {450, 489};

int t6x_hifi1_pll_prepare(struct meson_clk_pll_data *pll)
{
	writel(readl(EE_AUDIO_CLK_GATE_EN1) | 0x20, EE_AUDIO_CLK_GATE_EN1);
	writel(readl(EARCTX_ANA_CTRL1) | (1 << 19), EARCTX_ANA_CTRL1);
	writel(readl(EARCTX_ANA_CTRL2) | 0x1, EARCTX_ANA_CTRL2);

	/* store rate, do it after the audio register set*/
	meson_pll_store_rate(pll);

	return 0;
}

void t6x_hifi1_pll_unprepare(struct meson_clk_pll_data *pll)
{
	unsigned int reg;

	/* restore rate, do it before audio register clear */
	meson_pll_restore_rate(pll);

	reg = readl(EE_AUDIO_CLK_GATE_EN1);
	reg &= ~0x20;
	writel(reg, EE_AUDIO_CLK_GATE_EN1);
	reg = readl(EARCTX_ANA_CTRL1);
	reg &= ~(1 << 19);
	writel(reg, EARCTX_ANA_CTRL1);
	reg = readl(EARCTX_ANA_CTRL2);
	reg &= ~0x1;
	writel(reg, EARCTX_ANA_CTRL2);
}

static const struct reg_sequence t6x_hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x2001110b },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x3001110b },
	{ .reg = ANACTRL_HIFIPLL_CTRL1,  .def = 0x3a000000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,  .def = 0x00040000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,  .def = 0x0f0da000 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x1001110b },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,  .def = 0x0f0da200 },
};

struct meson_clk_pll_data t6x_hifi_pll = {
	.name = "hifi",
	.en = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift	 = 10,
		.width	 = 3,
	},
	.en0p5 = {
		.reg = ANACTRL_HIFIPLL_CTRL3,
		.shift	 = 26,
		.width	 = 1,
	},
	.l = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = meson_pll_store_rate,
		.pll_unprepare = meson_pll_restore_rate,
	},
	.flags = CLK_MESON_PLL_FIXED_EN0P5,
	.def_rate = t6x_hifi_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_hifi_pll_default_rate),
	.table = t6x_hifi_pll_table,
	.init_regs = t6x_hifi_init_regs,
	.init_count = ARRAY_SIZE(t6x_hifi_init_regs),
	.clkmsr_id = 19,
	.clkmsr_margin = 2
};

static const struct reg_sequence t6x_hifi1_init_regs[] = {
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x2001110b },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x3001110b },
	{ .reg = ANACTRL_HIFI1PLL_CTRL1,  .def = 0x3a000000 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL2,  .def = 0x00040000 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL3,  .def = 0x0f0da000 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x1001110b },
	{ .reg = ANACTRL_HIFI1PLL_CTRL3,  .def = 0x0f0da200 },
};

struct meson_clk_pll_data t6x_hifi1_pll = {
	.name = "hifi1",
	.en = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift   = 16,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift	 = 10,
		.width	 = 3,
	},
	.en0p5 = {
		.reg = ANACTRL_HIFI1PLL_CTRL3,
		.shift	 = 26,
		.width	 = 1,
	},
	.l = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = t6x_hifi1_pll_prepare,
		.pll_unprepare = t6x_hifi1_pll_unprepare,
	},
	.flags = CLK_MESON_PLL_FIXED_EN0P5,
	.def_rate = t6x_hifi_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6x_hifi_pll_default_rate),
	.table = t6x_hifi_pll_table,
	.init_regs = t6x_hifi1_init_regs,
	.init_count = ARRAY_SIZE(t6x_hifi1_init_regs),
	.clkmsr_id = 18,
	.clkmsr_margin = 2
};

struct meson_clk_pll_data *t6x_pll_list[] = {
	&t6x_sys0_pll,
	&t6x_sys1_pll,
	&t6x_gp0_pll,
	&t6x_gp1_pll,
	&t6x_gp2_pll,
	&t6x_hifi_pll,
	&t6x_hifi1_pll,
};

void t6x_meson_plls_all_test(void)
{
	for (int i = 0; i < ARRAY_SIZE(t6x_pll_list); i++)
		meson_pll_test(t6x_pll_list[i]);
}

void t6x_meson_hdmirx_plls_test(int argc, char * const argv[])
{
	/* todo: need add hdmirx plls test */
}

void t6x_meson_tcon_plls_test(int argc, char * const argv[])
{
	/* todo: need add tcon plls test */
}

void t6x_meson_plls_test(int argc, char * const argv[])
{
	struct meson_clk_pll_data *pll;

	pll = meson_pll_find_by_name(t6x_pll_list, ARRAY_SIZE(t6x_pll_list), argv[1]);
	if (!pll) {
		printf("The pll is not supported Or wrong pll name\n");
		return;
	}

	if ((argc - 2) == pll->init_count)
		meson_pll_parm_test(pll, argv);
	else if (argc == 2)
		meson_pll_test(pll);
	else
		printf("The pll cmd argc is error!\n");

	return;
}

int pll_test(int argc, char * const argv[])
{
	if (!strcmp("all", argv[1]))
		t6x_meson_plls_all_test();
	else if (!strcmp("tcon", argv[1]))
		t6x_meson_tcon_plls_test(argc, argv);
	else if (!strcmp("hdmirx", argv[1]))
		t6x_meson_hdmirx_plls_test(argc, argv);
	else
		t6x_meson_plls_test(argc, argv);

	return 0;
}
