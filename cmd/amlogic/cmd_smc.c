// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <linux/arm-smccc.h>
//#include <asm/arch/io.h>

//for later use
unsigned int smc_id_table_output_needed[] = {
	0x8200002a,  // GET_SHARE_MEM_INFORMATION
	0x82000030,  // EFUSE_READ_USER
	0x8200003B,  // EFUSE_OBJ_READ
	0x8200003D,  // EFUSE_READ_CALI
	0x8200003F,  // EFUSE_GET_MRK_CHECKNUM
	0x82000070,  // AES_GCM_SCP
	0x82000044,  // GET_CHIP_ID
	0x82000045,  // GET_ENC_KEY
	0x820000a7,  // KL_MSR_LOAD_CCCK
	0x82000047,  // TSENSOR_CALI_READ
	0x8200004D,  // OSCRING_EFUSE_GET
	0x82000038,  // EFUSE_CHK_MRK
	0x82000039,  // EFUSE_CALC_HASH
	0x8200003A,  // EFUSE_CAS_CHIPID
	0x82000039,  // NAGRA_EFUSE_READ
	0x82000075,  // NOCS_FUNCTIONS
	0x82000076,  // NOCS_DEMUX_IOCTL
	0x820000b0,  // GET_AVBKEY_FROM_FIP
	0x820000F0,  // AUTH_REG_SET
	0x8200006D,  // SYS_ANTIROLLBACK_ITEM
};

static int smc_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct arm_smccc_res res;
	unsigned int function_id;
	int i;
	int param[4] = {0};

	if (argc < 2) {
		printf("plese input smc id and params!\n");
		return CMD_RET_USAGE;
	}

	function_id = simple_strtoul(argv[1], NULL, 16);

	for (i = 0; i < argc - 2; i++)
		param[i] = simple_strtoul(argv[2 + i], NULL, 16);

	arm_smccc_smc(function_id, param[0], param[1], param[2], param[3], 0, 0, 0, &res);

	if ((int)res.a0 == -1)
		printf("sm test error");
	else
		printf("smc test success %lx\n", res.a0);
	return 0;
}

U_BOOT_CMD(smc, 6, 1, smc_test,
	"smc tst\n",
	"arg[1]: smc id\n"
);
