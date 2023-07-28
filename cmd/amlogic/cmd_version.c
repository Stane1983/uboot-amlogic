// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>
#include <amlogic/cpu_id.h>

static int do_bootloader_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char s_version[64];
	strcpy(s_version, "01.01.");
	strcat(s_version, U_BOOT_DATE_TIME);
	printf("s_version: %s\n", s_version);
	env_set("bootloader_version", s_version);
	return 0;
}

U_BOOT_CMD(
	get_bootloaderversion,	1,		0,	do_bootloader_version,
	"print bootloader version",
	""
);

static int do_get_cpuid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char chipid[16];
	char chipid_str[32];
	int i, j;
	char buf_tmp[4];
	char *buff = NULL;

	memset(chipid, 0, 16);
	if (get_chip_id(chipid, 16)) {
		env_set("cpu_id", "1122334455667788"); //default
		return 0;
	}

	memset(chipid_str, 0, 32);
	buff = &chipid_str[0];
	for (i = 0, j = 0; i < 12; ++i) {
		sprintf(&buf_tmp[0], "%02x", chipid[15 - i]);
		if (strcmp(buf_tmp, "00") != 0) {
			sprintf(buff + j, "%02x", chipid[15 - i]);
			j = j + 2;
		}
	}
	env_set("cpu_id", chipid_str);
	printf("buff: %s\n", buff);

	return 0;
}

U_BOOT_CMD_COMPLETE(
	get_cpuid,         //get cpu id to env cpu_id
	1,                 //maxargs
	0,                 //repeatable
	do_get_cpuid,      //command function
	"read cpuid to env cpu_id",           //description
	"    argv: get_cpuid\n",   //usage
	var_complete
);
