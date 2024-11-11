// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <amlogic/watchkey.h>

static int watchkey_show_list(void)
{
	char * const *wk_list = amlogic_watchkey_list();
	int index;

	if (!wk_list) {
		pr_err("Failed: Unsupported Features.\n");
		return CMD_RET_FAILURE;
	}

	printf("Watch-Key list:\n");
	for (index = 0; wk_list[index]; index++)
		printf("  [%2d]: %s\n", index, wk_list[index]);

	return CMD_RET_SUCCESS;
}

static int do_watchkey(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *cmd;
	const char *str_id;
	char *endp;
	int id;
	int ret;
	char key[16];
	char value[32];
	u32 d0, d1, d2;

	if (argc < 2)
		return CMD_RET_USAGE;
	cmd = argv[1];

	if (!strcmp(cmd, "list"))
		return watchkey_show_list();

	if (argc != 3)
		return CMD_RET_USAGE;
	str_id = argv[2];

	/* Parse watchkey id */
	id = simple_strtoul(str_id, &endp, 0);
	if (endp == str_id || id < 0)
		return CMD_RET_USAGE;

	if (!strcmp(cmd, "enable")) {
		ret = amlogic_watchkey_enable((u16)id);
	} else if (!strcmp(cmd, "disable")) {
		ret = amlogic_watchkey_disable((u16)id);
	} else if (!strcmp(cmd, "test")) {
		ret = amlogic_watchkey_test((u16)id);
		if (ret >= 0) {
			snprintf(value, sizeof(value), "%s", ret > 0 ? "yes" : "no");
			printf("%s\n", value);
			snprintf(key, sizeof(key), "wk%d_test", id);
			env_set(key, value);
		}
	} else if (!strcmp(cmd, "state")) {
		ret = amlogic_watchkey_state((u16)id);
		if (ret >= 0) {
			snprintf(value, sizeof(value), "%s", ret > 0 ?  "enabled" : "disabled");
			printf("%s\n", value);
			snprintf(key, sizeof(key), "wk%d_state", id);
			env_set(key, value);
		}
	} else if (!strcmp(cmd, "dump")) {
		ret = amlogic_watchkey_dump((u16)id, &d0, &d1, &d2);
		if (ret >= 0) {
			snprintf(value, sizeof(value), "%08x,%08x,%08x", d0, d1, d2);
			printf("%s\n", value);
			snprintf(key, sizeof(key), "wk%d_dump", id);
			env_set(key, value);
		}
	} else {
		pr_err("Unknown command: %s\n", cmd);
		return CMD_RET_USAGE;
	}

	if (ret < 0) {
		pr_err("Operation failed\n");
		return CMD_RET_USAGE;
	}

	return CMD_RET_SUCCESS;
}

static char watchkey_help_text[] =
	"Sub-System\n"
	"watchkey list           - list all supported ids\n"
	"watchkey enable <id>    - enable watchkey\n"
	"watchkey disable <id>   - disable watchkey\n"
	"watchkey test <id>      - read key detection status\n"
	"watchkey state <id>     - get the active state of watchkey\n"
	"watchkey dump <id>      - get additional data of watchkey\n";

U_BOOT_CMD(watchkey, 3, 0, do_watchkey, "Control watchkey functions", watchkey_help_text);
