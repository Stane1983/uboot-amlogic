// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <linux/libfdt_env.h>
#include <dt-bindings/amlogic/thermal/thermal.h>
#include "cmd_pdvfs.h"

static int get_table_size(const char *path)
{
	int ret = 0;
	char cmdbuf[256] = {0};
	char *phandle_str = NULL;
	unsigned int phandle_val = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get size table_size %s", path);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("Error: Failed to get phandle for %s\n", path);
		return -EBADMSG;
	}

	phandle_str = env_get("table_size");
	if (phandle_str == NULL) {
		printf("Error: Failed to retrieve table_size from environment\n");
		return -EBADMSG;
	}

	phandle_val = strtoul(phandle_str, NULL, 10);
	return phandle_val;
}

static int get_phandle(const char *path)
{
	int ret = 0;
	char cmdbuf[256] = {0};
	char *phandle_str = NULL;
	unsigned int phandle_val = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value phandle_value %s phandle", path);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("Error: Failed to get phandle for %s\n", path);
		return -EBADMSG;
	}

	phandle_str = env_get("phandle_value");
	if (phandle_str == NULL) {
		printf("Error: Failed to retrieve phandle_value from environment\n");
		return -EBADMSG;
	}
	phandle_val = strtoul(phandle_str, NULL, 16);

	return phandle_val;
}

static int set_cooling_state(const char *path, unsigned int cpus_phandle,
			     unsigned int lower, unsigned int upper)
{
	int ret = 0;
	char cmdbuf[256] = {0};

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set %s cooling-device <%d  %d  %d>", path, cpus_phandle, lower, upper);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("Error: Failed to update cooling-device state.\n");
		return -EBADMSG;
	}

	return 0;
}

static int do_update_cooling_state(cmd_tbl_t *cmdtp, int flag1, int argc, char * const argv[])
{
	unsigned int cpus_phandle = 0;
	unsigned int state = 0;
	unsigned int table_size = 0;

	cpus_phandle = get_phandle("/cpus/cpu@0");
	if (IS_ENABLED(CONFIG_MESON_S6)) {
		if (update_pdvfs_flag())
			return 0;
	}

	table_size = get_table_size(CONFIG_CPU_OPP_TABLE);
	state = table_size - CONFIG_OPP_TABLE_667M_SIZE;
	set_cooling_state("/thermal-zones/soc_thermal/cooling-maps/cpufreq_cooling_map",
			  cpus_phandle, 0, state);

	return 0;
}

U_BOOT_CMD(
	update_cooling_state, 1, 0, do_update_cooling_state,
	"update cpu cooling_state",
	"update_cooling_state pos"
);

