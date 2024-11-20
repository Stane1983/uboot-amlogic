// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <linux/arm-smccc.h>
#include <amlogic/cpu_id.h>
#include <linux/libfdt_env.h>

#define cmd_pdvfs_info(fmt...)	printf("[cmd_pdvfs] "fmt)
#define cmd_pdvfs_err(fmt...)	printf("[cmd_pdvfs] "fmt)
#define GET_DVFS_TABLE_INDEX           0x82000088

int get_board_id(void)
{
	int board_id = 0;

	board_id = (readl(SYSCTRL_SEC_STATUS_REG4) >> 8) & 0XFF;
	return board_id;
}

static int get_chip_rev(void)
{
	unsigned int chip_rev;

	chip_rev = get_cpu_id().chip_rev;
	return chip_rev;
}

static int set_regulator_status(const char *node, const char *status) {
	int ret = 0;
	char cmdbuf[256] = {0};

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set %s status %s;", node, status);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to set regulator status\n");
		return -EBADMSG;
	}

	return 0;
}

int get_phandle(const char *path) {
	int ret = 0;
	char cmdbuf[256] = {0};
	char *phandle_str = NULL;
	unsigned int phandle_val = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value phandle_value %s phandle", path);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to get phandle for %s\n", path);
		return -EBADMSG;
	}

	phandle_str = env_get("phandle_value");
	if (phandle_str == NULL) {
		cmd_pdvfs_err("Error: Failed to retrieve phandle_value from environment\n");
		return -EBADMSG;
	}
	phandle_val = strtoul(phandle_str, NULL, 16);

	return phandle_val;
}

static int update_pwm_f_board_regulator(void) {
	int ret = 0;
	char cmdbuf[256] = {0};
	unsigned int pwm_f_board0_phandle = 0;

	pwm_f_board0_phandle = get_phandle("/pwm_f_board0-regulator");
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set /meson-cpufreq cluster0-cpu-supply <0x%x>;", pwm_f_board0_phandle);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to update cluster0-cpu-supply.\n");
		return -EBADMSG;
	}
	ret = set_regulator_status("/pwm_f_board0-regulator", "okay");
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to set pwm_f_board0 okay\n");
		return -EBADMSG;
	}
	ret = set_regulator_status("/pwm_f_board2-regulator", "disabled");
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to set pwm_f_board2 disabled\n");
		return -EBADMSG;
	}

	return 0;
}


unsigned int get_cpufreq_table_index(u64 function_id,
				     u64 arg0, u64 arg1, u64 arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc((unsigned long)function_id,
		      (unsigned long)arg0,
		      (unsigned long)arg1,
		      (unsigned long)arg2,
		      0, 0, 0, 0, &res);
	return res.a0;
}

static int set_cpu_opp_tbl(void)
{
	int ret = 0;
	unsigned int opp_table_0_phandle = 0;
	unsigned int opp_table_1_phandle = 0;
	unsigned int opp_table_2_phandle = 0;
	unsigned int opp_table_3_phandle = 0;
	char cmdbuf[256] = {0};

	opp_table_0_phandle = get_phandle("/cpu_opp_table0");
	opp_table_1_phandle = get_phandle("/cpu_opp_table1");
	opp_table_2_phandle = get_phandle("/cpu_opp_table2_2000");
	opp_table_3_phandle = get_phandle("/cpu_opp_table3_2000");

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set /cpus/cpu@0 operating-points-v2 <0x%x 0x%x 0x%x 0x%x>;", \
		opp_table_0_phandle, opp_table_1_phandle, \
		opp_table_2_phandle, opp_table_3_phandle);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		cmd_pdvfs_err("Error: Failed to update operating-points-v2/n");
		return -EBADMSG;
	}

	return 0;
}

static int update_pdvfs_tbl(cmd_tbl_t *cmdtp, int flag, int argc,
			char *const argv[])
{
	unsigned int ret = 0;
	unsigned int board_id = 0;
	unsigned int board_rev = 0;
	unsigned int pdvfs_index = 0;

	board_id = get_board_id();
	board_rev = get_chip_rev();
	pdvfs_index = get_cpufreq_table_index(GET_DVFS_TABLE_INDEX, 0, 0, 0);
	cmd_pdvfs_info("update_pdvfs dtb\n");
	if (board_id < 2) {
		ret = update_pwm_f_board_regulator();
		if (ret != 0)
			cmd_pdvfs_err("fix_regulator_tbl fail\n");
		ret = set_cpu_opp_tbl();
		if (ret != 0)
			cmd_pdvfs_err("fix_cpu_opp_tbl fail\n");
	}
	else if (board_id == 2) {
		if (pdvfs_index == 0 || pdvfs_index == 1) {
			ret = set_cpu_opp_tbl();
			if (ret != 0)
				cmd_pdvfs_err("fix_cpu_opp_tbl fail\n");
		}
	}
	else
		cmd_pdvfs_err("get board_id  fail,board_id = %d\n", board_id);

	return 0;
}

U_BOOT_CMD(
	update_pdvfs, 1, 0,	update_pdvfs_tbl,
	"update pdvfs",
	"set_regulator_tbl        - set board_A regulator_pwm_tbl\n"
	"set_cpu_opp_tbl              - set 2.0g cpu_opp_tbl\n"
);
