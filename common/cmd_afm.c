// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <common.h>
#include <command.h>
static int do_afm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *str_cmd, *str_value = NULL;

	if (argc != 3)
		return CMD_RET_USAGE;
	str_cmd = argv[1];
	/* Strip off leading 'afm' command argument */
	argc -= 2;
	argv += 2;
	//if (argc > 0)
		str_value = *argv;
	if (!strcmp(str_cmd, "otg_device")) {
		if (!strcmp(str_value, "1")) {
			run_command("setenv otg_device 1", 0);
		} else if (!strcmp(str_value, "0")) {
			run_command("setenv otg_device 0", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
		return 0;
	} else if (!strcmp(str_cmd, "mac")) {
		char result[50] = "keyman write mac str ";//size 21

		if (strlen(str_value) != 17) {//str_value size 17
			printf("invalid address\n");
			return CMD_RET_USAGE;
		}
		strcat(result, str_value);
		run_command(result, 0);
		run_command("keyman read mac $loadaddr str; printenv mac", 0);
		return 0;
	}
	return 0;
}

U_BOOT_CMD(afm, CONFIG_SYS_MAXARGS, 0, do_afm,
	"Factory related command/usage",
	"\n"
	"afm model_name <ModelName> - modify screen parameters\n"
	"afm serialno <serial no> - modify serial no\n"
	"afm mac <mac address> - modify mac address\n"
	"afm console <on|off> - modify console state\n"
	"afm model_name <ModelName> - modify screen parameters\n"
	"afm silent <on|off> - modify the print state of uboot bl33 serial port\n"
	"afm powermode <on|standby> - modify power on mode\n"
	"afm otg_device <1|0> - modify the USB host/device state\n"
	"afm selinux <0|1|2(permissive|enforcing|disabled)> - modify the selinux mode\n"
);
