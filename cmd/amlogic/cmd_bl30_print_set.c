// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/arch/mailbox.h>
#include <asm/arch/secure_apb.h>

#define STICK_MEM_EN_BL30_PRINT_FLAG 0x30301111
#define STICK_MEM_DIS_BL30_PRINT_FLAG 0x30300000

/* Applied to enable or disable bl30 start logs before first
 * suspend or shutdown when compile with '--noverbose'.
 */
static int do_bl30_print_set(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CMD_SET_BL30_PRINT
	uint32_t ret, enable, bl30_print_flag;

	if (argc <= 1) {
		printf("please input: bl30_print_set [1 or 0]\n");
		return CMD_RET_USAGE;
	}

	enable = simple_strtoul(argv[1], NULL, 16);
	if (enable == 1)
		bl30_print_flag = STICK_MEM_EN_BL30_PRINT_FLAG;
	else if (enable == 0)
		bl30_print_flag = STICK_MEM_DIS_BL30_PRINT_FLAG;
	else {
		printf("bl30 log print set failed: invalid value\n");
		return -1;
	}

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_BL30_PRINT, &bl30_print_flag, 4, NULL, 0);

	if (ret != 0) {
		printf("set bl30 log print failed\n");
	} else {
		if (enable)
			printf("enable bl30 log print\n");
		else
			printf("disable bl30 log print\n");
	}

#else
	printf("bl30 log print set failed: not supported\n");
#endif

	return 0;
}

U_BOOT_CMD(
	bl30_print_set, 2, 1, do_bl30_print_set,
	"bl30 log print en set when build uboot with --noverbose",
	"\narg[0]: cmd\n"
	"arg[1]: 1: enable 0: disable\n"
);
