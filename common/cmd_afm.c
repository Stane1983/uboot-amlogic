// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <common.h>
#include <command.h>
#include "../include/amlogic/ini.h"

static int do_afm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *str_cmd, *str_value = NULL;
	if (argc == 2 && !strcmp(argv[1], "print"))
	{
		char *serialno = env_get("usid");
		char *mac = env_get("mac");
		char *console = env_get("console");
		char *silent = env_get("silent");
		char *powermode = env_get("powermode");
		char *otg_device = env_get("otg_device");
		char *selinux = env_get("EnableSelinux");
		char model_name[50] = {0}, model1_name[50] = {0}, model2_name[50] = {0}, modelt_name[150] = {0};
		handle_model_get("0", model_name);
		handle_model_get("1", model1_name);
		handle_model_get("2", model2_name);
		if (strlen(model_name) != 0)
		{
			strcat(modelt_name, model_name);
		}
		if (strlen(model1_name) != 0) {
			strcat(modelt_name, "|");
			strcat(modelt_name, model1_name);
		}
		if (strlen(model2_name) !=0 ) {
			strcat(modelt_name, "|");
			strcat(modelt_name, model2_name);
		}
		//printf("current %s: %s\n", str, model_name);
		printf("\n"
			"model_name :     %s\n"
			"serialno :       %s\n"
			"mac :            %s\n"
			"console :        %s\n"
			"silent :         %s\n"
			"powermode :      %s\n"
			"otg_device :     %s\n"
			"selinux :        %s\n"
			, strlen(modelt_name) != 0 ? modelt_name : ""
			, serialno != NULL? serialno : ""
			, mac != NULL? mac : ""
			, console != NULL && !strcmp(console, "ttyS0,115200") ? "on" : "off"
			, silent != NULL? (!strcmp(silent, "0")? "on" : "off") : "on"
			, powermode != NULL? powermode : ""
			, otg_device != NULL? otg_device : ""
			, selinux != NULL? selinux : "");
		return 0;
	}
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
	} else if (!strcmp(str_cmd, "serialno")) {
		char result[50] = "keyman write usid str ";//size 21
		if (strlen(str_value) > 17) {//str_value size 17
			printf("invalid no\n");
			return CMD_RET_USAGE;
		}
		strcat(result, str_value);
		run_command(result, 0);
		run_command("keyman read usid $loadaddr str; printenv usid", 0);
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
	} else if (!strcmp(str_cmd, "silent")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv silent 0", 0);
		} else if (!strcmp(str_value, "off")) {
			run_command("setenv silent 1", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "powermode")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv powermode on", 0);
		} else if (!strcmp(str_value, "standby")) {
			run_command("setenv powermode standby", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "selinux")) {
		if (!strcmp(str_value, "enforcing")) {
			run_command("setenv EnableSelinux enforcing", 0);
		} else if (!strcmp(str_value, "permissive")) {
			run_command("setenv EnableSelinux permissive", 0);
		} else if (!strcmp(str_value, "disabled")) {
			run_command("setenv EnableSelinux disabled", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else if (!strcmp(str_cmd, "model_name")) {
		handle_model_set("model_name", str_value);
	} else if (!strcmp(str_cmd, "model1_name")) {
		handle_model_set("model1_name", str_value);
	} else if (!strcmp(str_cmd, "model2_name")) {
		handle_model_set("model2_name", str_value);
	} else if (!strcmp(str_cmd, "console")) {
		if (!strcmp(str_value, "on")) {
			run_command("setenv console ttyS0,115200", 0);
			run_command("setenv loglevel 7", 0);
		} else if (!strcmp(str_value, "off")) {
			run_command("setenv console off", 0);
			run_command("setenv loglevel 0", 0);
		} else {
			printf("invalid value\n");
			return CMD_RET_USAGE;
		}
		run_command("saveenv", 0);
	} else {
		printf("invalid value\n");
		return CMD_RET_USAGE;
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
	"afm silent <on|off> - modify the print state of uboot bl33 serial port\n"
	"afm powermode <on|standby> - modify power on mode\n"
	"afm otg_device <1|0> - modify the USB host/device state\n"
	"afm selinux <0|1|2(permissive|enforcing|disabled)> - modify the selinux mode\n"
	"afm print - Print the values of all parameters\n"
);
