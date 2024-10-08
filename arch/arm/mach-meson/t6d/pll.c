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
	SECID_CPU_CLK_SEL,
	SECID_CPU_CLK_RD,
	SECID_CPU_CLK_DYN,
	SECID_PLL_TEST,
};


int t6d_sys0_pll_prepare(struct meson_clk_pll_data *pll)
{
	struct arm_smccc_res res;

	/* store rate */
	meson_pll_store_rate(pll);

	/* Set fixed clk to 1G, Switch to fixed clk first */
	arm_smccc_smc(SECURE_CPU_CLK, SECID_CPU_CLK_DYN,
		1, 0, 0, 0, 0, 0, &res);

	meson_switch_cpu_clk(SECURE_CPU_CLK, SECID_CPU_CLK_SEL, 0);

	return 0;
}

void t6d_sys0_pll_unprepare(struct meson_clk_pll_data *pll)
{
	/* restore rate */
	meson_pll_restore_rate(pll);

	/* Switch back cpu to sys pll */
	meson_switch_cpu_clk(SECURE_CPU_CLK, SECID_CPU_CLK_SEL, 1);
}

static const struct pll_params_table t6d_sys_pll_table[] = {
	PLL_PARAMS(75, 1, 0), /*DCO=1800M OD=1800M restore*/
	PLL_PARAMS(80, 1, 0), /*DCO=1920M OD=1920M */
	PLL_PARAMS(63, 1, 0), /*DCO=1512M OD=1512M*/
	{ /* sentinel */ }
};

static unsigned int t6d_sys_pll_default_rate[] = {1512, 1920};

static const struct reg_sequence t6d_sys0_pll_init_regs[] = {
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x20090a64 },
	{ .reg = ANACTRL_SYS0PLL_CTRL1, .def = 0x0332c800 },
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x30090a64, .delay_us = 20 },
	{ .reg = ANACTRL_SYS0PLL_CTRL0, .def = 0x10090a64, .delay_us = 20 },
	{ .reg = ANACTRL_SYS0PLL_CTRL1, .def = 0x03328800 },
};

struct meson_clk_pll_data t6d_sys0_pll = {
	.name = "sys0",
	.en = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift   = 11,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_SYS0PLL_CTRL0,
		.shift	 = 9,
		.width	 = 2,
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
		.pll_prepare = t6d_sys0_pll_prepare,
		.pll_unprepare = t6d_sys0_pll_unprepare,
		.pll_disable = meson_secure_pll_disable,
		.pll_set_rate = meson_secure_pll_set_rate,
		.pll_set_parm_rate = meson_secure_pll_set_parm_rate,
		.pll_rate_to_msr = meson_div16_to_msr,

	},
	.def_rate = t6d_sys_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6d_sys_pll_default_rate),
	.table = t6d_sys_pll_table,
	.init_regs = t6d_sys0_pll_init_regs,
	.init_count = ARRAY_SIZE(t6d_sys0_pll_init_regs),
	.smc_id = SECURE_PLL_CLK,
	.secid_disable = SECID_SYS0_DCO_PLL_DIS,
	.secid_test = SECID_PLL_TEST,
	.secid = SECID_SYS0_DCO_PLL,
	.clkmsr_id = 23,
	.clkmsr_margin = 10,
};

static unsigned int t6d_gp0_pll_default_rate[] = {576, 768};

/* set the od = 2, clkmsr can not measure 6000M */
static const struct pll_params_table t6d_gp0_pll_table[] = {
	PLL_PARAMS(192, 1, 1), /* DCO = 3024M  OD = 1152M for restore */
	PLL_PARAMS(192, 1, 2), /* DCO = 3024M  OD = 576M */
	PLL_PARAMS(256, 1, 2), /* DCO = 3072M  OD = 768M */
	{ /* sentinel */  }
};

static const struct reg_sequence t6d_gp0_init_regs[] = {
	{ .reg = ANACTRL_GP0PLL_CTRL0,  .def = 0x22530ac0 },
	{ .reg = ANACTRL_GP0PLL_CTRL1,  .def = 0x09900322 },
	{ .reg = ANACTRL_GP0PLL_CTRL2,  .def = 0x0 },
	{ .reg = ANACTRL_GP0PLL_CTRL3,  .def = 0x00000006 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,  .def = 0x32530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,  .def = 0x12530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_GP0PLL_CTRL0,  .def = 0x12520ac0 },
};

struct meson_clk_pll_data t6d_gp0_pll = {
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
		.shift   = 11,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_GP0PLL_CTRL0,
		.shift	 = 9,
		.width	 = 2,
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
	.def_rate = t6d_gp0_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6d_gp0_pll_default_rate),
	.table = t6d_gp0_pll_table,
	.init_regs = t6d_gp0_init_regs,
	.init_count = ARRAY_SIZE(t6d_gp0_init_regs),
	.clkmsr_id = 20,
	.clkmsr_margin = 2
};

static const struct pll_params_table t6d_hifi_pll_table[] = {
	PLL_PARAMS(150, 1, 2), /* DCO = 1800M OD = 4 PLL = 450M */
	PLL_PARAMS(163, 1, 2), /* DCO = 1944M OD = 4 PLL = 489M */
	{ /* sentinel */  }
};

static unsigned int t6d_hifi_pll_default_rate[] = {450, 489};

static const struct reg_sequence t6d_hifi_init_regs[] = {
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x22530ac0 },
	{ .reg = ANACTRL_HIFIPLL_CTRL1,  .def = 0x09900322 },
	{ .reg = ANACTRL_HIFIPLL_CTRL2,  .def = 0x0 },
	{ .reg = ANACTRL_HIFIPLL_CTRL3,  .def = 0x00000006 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x32530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x12530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_HIFIPLL_CTRL0,  .def = 0x12520ac0 },
};

struct meson_clk_pll_data t6d_hifi_pll = {
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
		.shift   = 11,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_HIFIPLL_CTRL0,
		.shift	 = 9,
		.width	 = 2,
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
	.def_rate = t6d_hifi_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6d_hifi_pll_default_rate),
	.table = t6d_hifi_pll_table,
	.init_regs = t6d_hifi_init_regs,
	.init_count = ARRAY_SIZE(t6d_hifi_init_regs),
	.clkmsr_id = 19,
	.clkmsr_margin = 2
};

static const struct reg_sequence t6d_hifi1_init_regs[] = {
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x22530ac0 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL1,  .def = 0x09900322 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL2,  .def = 0x0 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL3,  .def = 0x00000006 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x32530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x12530ac0, .delay_us = 20 },
	{ .reg = ANACTRL_HIFI1PLL_CTRL0,  .def = 0x12520ac0 },
};

struct meson_clk_pll_data t6d_hifi1_pll = {
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
		.shift   = 11,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_HIFI1PLL_CTRL0,
		.shift	 = 9,
		.width	 = 2,
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
		.pll_prepare = meson_pll_store_rate,
		.pll_unprepare = meson_pll_restore_rate,
	},
	.flags = CLK_MESON_PLL_FIXED_EN0P5,
	.def_rate = t6d_hifi_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6d_hifi_pll_default_rate),
	.table = t6d_hifi_pll_table,
	.init_regs = t6d_hifi1_init_regs,
	.init_count = ARRAY_SIZE(t6d_hifi1_init_regs),
	.clkmsr_id = 18,
	.clkmsr_margin = 2
};

unsigned long meson_div25_to_msr(struct meson_clk_pll_data *pll, unsigned long rate)
{
	/* msr is div16 */
	return rate / 25;
}

static unsigned int t6d_usb_pll_default_rate[] = {1200, 900};

static const struct pll_params_table t6d_usb_pll_table[] = {
	PLL_PARAMS(100, 1, 1), /* DCO = 2400M OD = 1 PLL = 1200M */
	PLL_PARAMS(75, 1, 1), /* DCO = 1800M OD = 1 PLL = 900M */
	{ /* sentinel */  }
};

static const struct reg_sequence t6d_usb_init_regs[] = {
	{ .reg = ANACTRL_USBPLL_CTRL0,  .def = 0x20090a64 },
	{ .reg = ANACTRL_USBPLL_CTRL1,  .def = 0x0332c800 },
	{ .reg = ANACTRL_USBPLL_CTRL0,  .def = 0x30090a64, .delay_us = 20 },
	{ .reg = ANACTRL_USBPLL_CTRL0,  .def = 0x10090a64, .delay_us = 20 },
	{ .reg = ANACTRL_USBPLL_CTRL1,  .def = 0x03328800 },
};

static unsigned int usb_ctrl0_store;

int t6d_usb_pll_prepare(struct meson_clk_pll_data *pll)
{
	/* store rate */
	meson_pll_store_rate(pll);

	usb_ctrl0_store = readl(CLKCTRL_USB_CLK_CTRL0);

	writel(readl(CLKCTRL_USB_CLK_CTRL0) | (1 << 24) | (1 << 26), CLKCTRL_USB_CLK_CTRL0);

	return 0;
}

void t6d_usb_pll_unprepare(struct meson_clk_pll_data *pll)
{
	/* restore rate */
	meson_pll_restore_rate(pll);
	writel(usb_ctrl0_store, ANACTRL_USBPLL_CTRL0);
}

struct meson_clk_pll_data t6d_usb_pll = {
	.name = "usb",
	.en = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift   = 28,
		.width   = 1,
	},
	.m = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift   = 0,
		.width   = 8,
	},
	.n = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift   = 11,
		.width   = 5,
	},
	.od = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift	 = 9,
		.width	 = 2,
	},
	.l = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift   = 31,
		.width   = 1,
	},
	.rst = {
		.reg = ANACTRL_USBPLL_CTRL0,
		.shift   = 29,
		.width   = 1,
	},
	.ops = &(const struct meson_pll_test_ops) {
		.pll_prepare = t6d_usb_pll_prepare,
		.pll_unprepare = t6d_usb_pll_unprepare,
		.pll_rate_to_msr = meson_div25_to_msr,
	},
	.def_rate = t6d_usb_pll_default_rate,
	.def_cnt = ARRAY_SIZE(t6d_usb_pll_default_rate),
	.table = t6d_usb_pll_table,
	.init_regs = t6d_usb_init_regs,
	.init_count = ARRAY_SIZE(t6d_usb_init_regs),
	.clkmsr_id = 122,
	.clkmsr_margin = 2
};

struct meson_clk_pll_data *t6d_pll_list[] = {
	&t6d_sys0_pll,
	&t6d_gp0_pll,
	&t6d_hifi_pll,
	&t6d_hifi1_pll,
	&t6d_usb_pll,
};

void t6d_meson_plls_all_test(void)
{
	for (int i = 0; i < ARRAY_SIZE(t6d_pll_list); i++)
		meson_pll_test(t6d_pll_list[i]);
}

void t6d_meson_hdmirx_plls_test(int argc, char * const argv[])
{
	/* todo: need add hdmirx plls test */
}

void t6d_meson_tcon_plls_test(int argc, char * const argv[])
{
	/* todo: need add tcon plls test */
}

void t6d_meson_plls_test(int argc, char * const argv[])
{
	struct meson_clk_pll_data *pll;

	pll = meson_pll_find_by_name(t6d_pll_list, ARRAY_SIZE(t6d_pll_list), argv[1]);
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
		t6d_meson_plls_all_test();
	else if (!strcmp("tcon", argv[1]))
		t6d_meson_tcon_plls_test(argc, argv);
	else if (!strcmp("hdmirx", argv[1]))
		t6d_meson_hdmirx_plls_test(argc, argv);
	else
		t6d_meson_plls_test(argc, argv);

	return 0;
}
