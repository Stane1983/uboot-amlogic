// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <command.h>
#include <linux/arm-smccc.h>
#include <vsprintf.h>
#include <linux/errno.h>
#include <stdio.h>

#define DMC_MON_RW			0x8200004A

unsigned long dmc_rw(uint64_t addr, uint64_t value, uint64_t rw)
{
	struct arm_smccc_res res;

	arm_smccc_smc(DMC_MON_RW, addr, value, rw, 0, 0, 0, 0, &res);

	return res.a0;
}

int do_dmc_rw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 4 || argc > 5)
		return cmd_usage(cmdtp);

	uint64_t addr = simple_strtoul(argv[1], NULL, 16);
	uint64_t value = simple_strtoul(argv[2], NULL, 16);
	uint64_t rw = simple_strtoul(argv[3], NULL, 16);
	uint64_t dmc_back;

	dmc_back = dmc_rw(addr, value, rw);

	printf("return value is %llx\n", dmc_back);

	return dmc_back;
}

static char dmc_help_text[] =
	"\n"
	"dmc_rw fe037470 ffffffff 0 - show dmc read information\n"
	"dmc_rw fe037470 ffffffff 1 - show dmc write information\n";

U_BOOT_CMD(dmc_rw, 4, 0, do_dmc_rw,
	"dmc read write function",
	dmc_help_text
);
