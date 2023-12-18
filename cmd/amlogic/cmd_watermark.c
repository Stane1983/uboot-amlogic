// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>

#define OPTEE_SMC_CHECK_WM_STATUS        0xB200E003
#define OPTEE_SMC_INIT_WM                0xB200E080

#define CMD_RET_SUCCESS                  0x00000000

static uint32_t check_wm_sts(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(OPTEE_SMC_CHECK_WM_STATUS, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static uint32_t init_wm(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(OPTEE_SMC_INIT_WM, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int do_wm_init(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (check_wm_sts() == 0)
		printf("WATERMARK: initialize Watermark return 0x%08X\n", init_wm());
	else
		printf("WATERMARK: Watermark is disabled\n");

	return CMD_RET_SUCCESS;
}

/* -------------------------------------------------------------------- */
U_BOOT_CMD(watermark_init, CONFIG_SYS_MAXARGS, 0, do_wm_init,
	"initialize watermark\n",
	"watermark_init");
