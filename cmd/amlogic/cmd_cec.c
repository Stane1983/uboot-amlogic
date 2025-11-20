// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/arch/io.h>
#include <asm/arch/clock.h>
#include <amlogic/aml_cec.h>
#include <asm/arch/secure_apb.h>
#include <amlogic/aml_hdmirx.h>

static void cec_init(int logic_addr, unsigned char fun_cfg);

int meson_cec_suspend(void *dev_info)
{
	char *str;
	int logic_addr = 0;
	int fun_cfg = 0x2f;
	int phy_addr = 0;
	unsigned int tmp;

	str = env_get("reboot_mode");
	if (str && !strcmp(str, "aocpu_watchdog")) {
		str = env_get("cec_fun");
		printf("env_get cec_fun: %s\n", str);
		fun_cfg = simple_strtoul(str, NULL, 16);
#if (CONFIG_MESON_T5W == 1 || CONFIG_MESON_TXHD2 == 1 || CONFIG_AML_HDMIRX == 1)
		(void)tmp;
		(void)phy_addr;
		logic_addr = 0;
		hdmirx_hw_init(0, NULL, 0);
#else
		logic_addr = 4;
		str = env_get("phy_addr");
		if (str) {
			phy_addr = simple_strtoul(str, NULL, 16);
			tmp = readl(SYSCTRL_STATUS_REG1);
			tmp &= 0xf0000;
			tmp |= phy_addr;
			writel(tmp, SYSCTRL_STATUS_REG1);
		}
#endif
		printf("logic_addr=0x%x, fun_cfg=0x%x\n", logic_addr, fun_cfg);
		cec_init(logic_addr, (unsigned char)fun_cfg);
	}

	return 0;
}

int meson_cec_resume(void *dev_info)
{
	return 0;
}

int meson_cec_poweroff(void *dev_info)
{
	return 0;
}

static void cec_init(int logic_addr, unsigned char fun_cfg)
{
#ifndef CONFIG_MESON_T5W
	unsigned int tmp;

	writel(fun_cfg, SYSCTRL_STATUS_REG0);
	tmp = readl(SYSCTRL_STATUS_REG1);
	tmp &= (~0xf0000);
	tmp |= ((logic_addr & 0xf) << 16);
	writel(tmp, SYSCTRL_STATUS_REG1);
	printf("cec func:%#x, parm:%#x\n", readl(SYSCTRL_STATUS_REG0),
		readl(SYSCTRL_STATUS_REG1));
#endif
}

static int do_cec(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int logic_addr = 0;
	int fun_cfg = 0x2f;

	if (argc >=  2)
		logic_addr = simple_strtoul(argv[1], NULL, 10);
	if (argc >=  3)
		fun_cfg = simple_strtoul(argv[2], NULL, 16);

	printf("logic_addr=0x%x, fun_cfg=0x%x\n",
		logic_addr, fun_cfg);

	cec_init(logic_addr, (unsigned char)fun_cfg);

	return 0;
}

U_BOOT_CMD(cec, CONFIG_SYS_MAXARGS, 0, do_cec,
		"Amlogic cec",
		"	- hdmi cec function\n"
		"	- param: logic addr;fun_cfg\n"
		"\n"
);

