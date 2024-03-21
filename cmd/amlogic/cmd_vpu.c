// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <amlogic/media/vpu/vpu.h>

static int do_vpu_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	run_command("mw fe010130 118", 0); /* TODO */
	run_command("mw fe000218 00080000", 0); /* TODO */
	vpu_probe();
	return 0;
}

static int do_vpu_disable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	vpu_remove();
	return 0;
}

static int do_vpu_clk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int level;
	int ret = 0;

	if (argc == 1) {
		return -1;
	}
	if (strcmp(argv[1], "set") == 0) {
		if (argc == 3) {
			level = (int)simple_strtoul(argv[2], NULL, 10);
			ret = vpu_clk_change(level);
		} else {
			ret = -1;
		}
	} else if (strcmp(argv[1], "get") == 0) {
		vpu_clk_get();
	} else {
		ret = -1;
	}
	return ret;
}

static int do_vpu_arb_l1_bind_change(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int level1_module;
	int level2_module;
	int ret = 0;

	if (argc < 3) {
		printf("vpu arb_bind1 x(index) x(bind_port)\n");
		return -1;
	}
	level1_module = (int)simple_strtoul(argv[1], NULL, 10);
	level2_module = (int)simple_strtoul(argv[2], NULL, 10);
	ret = vpu_rdarb_bind_l1(level1_module, level2_module);
	if (ret != -1)
		print_bind1_change_info(level1_module, level2_module);
	return ret;
}

static int do_vpu_arb_l2_bind_change(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int level2_module;
	int vpu_prot;
	int ret = 0;

	if (argc < 3) {
		printf("vpu arb_bind2 x(index) x(bind_port)\n");
		return -1;
	}
	level2_module = (int)simple_strtoul(argv[1], NULL, 10);
	vpu_prot = (int)simple_strtoul(argv[2], NULL, 10);
	ret = vpu_rdarb_bind_l2(level2_module, vpu_prot);
	if (ret != -1)
		print_bind2_change_info(level2_module, vpu_prot);
	return ret;
}

static int do_vpu_urgent_change(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int module;
	int urgent_val;
	int ret = 0;

	if (argc < 3) {
		printf("vpu urgent_set x(index) x(value)\n");
		return -1;
	}
	module = (int)simple_strtoul(argv[1], NULL, 10);
	urgent_val = (int)simple_strtoul(argv[2], NULL, 10);
	/*first parameter reserve*/
	ret = vpu_urgent_set(module, urgent_val);
	if (ret == -1)
		return ret;
	print_urgent_change_info(module, urgent_val);
	return ret;
}

static int do_vpu_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	vcbus_test();
	return 0;
}

static int do_vpu_arb_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	get_arb_module_info();
	return 0;
}

static int do_vpu_rdarb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dump_vpu_rdarb_table();
	return 0;
}

static int do_vpu_urgent(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dump_vpu_urgent_table();
	return 0;
}

static int do_vpu_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	vpu_info_print();
	return 0;
}

static int do_vpu_secure(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int temp;

	if (argc == 1)
		return -1;

	temp = (unsigned int)simple_strtoul(argv[1], NULL, 10);
	vpu_sec_debug(temp);
	return 0;
}

static cmd_tbl_t cmd_vpu_sub[] = {
	U_BOOT_CMD_MKENT(probe, 2, 0, do_vpu_enable, "", ""),
	U_BOOT_CMD_MKENT(remove, 2, 0, do_vpu_disable, "", ""),
	U_BOOT_CMD_MKENT(clk, 3, 0, do_vpu_clk, "", ""),
	U_BOOT_CMD_MKENT(test, 2, 0, do_vpu_test, "", ""),
	U_BOOT_CMD_MKENT(rdarb, 2, 0, do_vpu_rdarb, "", ""),
	U_BOOT_CMD_MKENT(urgent, 2, 0, do_vpu_urgent, "", ""),
	U_BOOT_CMD_MKENT(arb_bind1, 3, 0, do_vpu_arb_l1_bind_change, "", ""),
	U_BOOT_CMD_MKENT(arb_bind2, 3, 0, do_vpu_arb_l2_bind_change, "", ""),
	U_BOOT_CMD_MKENT(urgent_set, 3, 0, do_vpu_urgent_change, "", ""),
	U_BOOT_CMD_MKENT(arb_module_info, 2, 0, do_vpu_arb_info, "", ""),
	U_BOOT_CMD_MKENT(info, 2, 0, do_vpu_info, "", ""),
	U_BOOT_CMD_MKENT(sec, 2, 0, do_vpu_secure, "", ""),
};

static int do_vpu(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	/* Strip off leading 'bmp' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vpu_sub[0], ARRAY_SIZE(cmd_vpu_sub));

	if (c) {
		return c->cmd(cmdtp, flag, argc, argv);
	} else {
		cmd_usage(cmdtp);
		return 1;
	}
}

U_BOOT_CMD(
	vpu,	5,	0,	do_vpu,
	"vpu sub-system",
	"vpu probe           - enable vpu domain\n"
	"vpu remove          - disable vpu domain\n"
	"vpu test            - test vcbus access\n"
	"vpu rdarb           - get vpu rdarb info\n"
	"vpu arb_module_info - get vpu arb module info\n"
	"vpu urgent          - get vpu urgent info\n"
	"vpu arb_bind1       - change level1 arb bind\n"
	"vpu arb_bind2       - change level2 arb bind\n"
	"vpu urgent_set      - change urgent value\n"
);
