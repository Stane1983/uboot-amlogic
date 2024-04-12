// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <stdlib.h>
#include <linux/arm-smccc.h>

#define OPTEE_SMC_CHECK_WM_STATUS        0xB200E003
#define OPTEE_SMC_INIT_WM                0xB200E080

#define SMC_TYPE_VXWM                    0xFFFF0010
#define SMC_TYPE_NGWM                    0xFFFF0020

#define CMD_RET_SUCCESS                  0x00000000
#define CMD_RET_UNKNOWN                  0xFFFFFFFF

static bool check_vxwm_sts(void)
{
	struct arm_smccc_res vxwm_res;

	memset(&vxwm_res, 0, sizeof(struct arm_smccc_res));

	arm_smccc_smc(OPTEE_SMC_CHECK_WM_STATUS, SMC_TYPE_VXWM, 0, 0, 0, 0, 0, 0, &vxwm_res);

	return vxwm_res.a0 == 0;
}

static bool init_vxwm(void)
{
	struct arm_smccc_res vxwm_res;

	memset(&vxwm_res, 0, sizeof(struct arm_smccc_res));

	arm_smccc_smc(OPTEE_SMC_INIT_WM, SMC_TYPE_VXWM, 0, 0, 0, 0, 0, 0, &vxwm_res);

	return vxwm_res.a0 == 0;
}

static int do_wm_init(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int res = CMD_RET_UNKNOWN;

	if (check_vxwm_sts()) {
		if (init_vxwm()) {
			printf("WATERMARK: success to initialize vxwm\n");
			res = CMD_RET_SUCCESS;
		} else {
			printf("WATERMARK: failed to initialize vxwm\n");
		}
	} else {
		printf("WATERMARK: vxwm is disabled\n");
	}

	return res;
}

/* -------------------------------------------------------------------- */
U_BOOT_CMD(watermark_init, CONFIG_SYS_MAXARGS, 0, do_wm_init,
	"initialize watermark",
	"");
