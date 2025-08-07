// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <amlogic/media/vout/eDPTX/DPTX_export.h>

static int do_dptx_probe(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_probe();
	return 0;
}

static int do_dptx0_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_enable(0, "NULL");
	return 0;
}

static int do_dptx0_disable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_disable(0);
	return 0;
}

static int do_dptx0_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_info(0);
	return 0;
}

static int do_dptx0_reg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_reg_print(0);
	return 0;
}

static int do_dptx0_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_test(0, num);
	return 0;
}

static int do_dptx0_list_vmode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_list_vmode(0);
	return 0;
}

static int do_dptx0_set_vmode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_set_vmode(0, num);
	return 0;
}

static int do_dptx0_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_reset(0, num);
	return 0;
}

static int do_dptx0_PSR1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_PSR1_en(0, num);
	return 0;
}

static int do_dptx0_PSR2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_PSR2_en(0, num);
	return 0;
}

static int do_dptx1_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_enable(1, "NULL");
	return 0;
}

static int do_dptx1_disable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_disable(1);
	return 0;
}

static int do_dptx1_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_info(1);
	return 0;
}

static int do_dptx1_reg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_reg_print(1);
	return 0;
}

static int do_dptx1_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_test(1, num);
	return 0;
}

static int do_dptx1_list_vmode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	dptx_driver_list_vmode(1);
	return 0;
}

static int do_dptx1_set_vmode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_set_vmode(1, num);
	return 0;
}

static int do_dptx1_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_reset(1, num);
	return 0;
}

static int do_dptx1_PSR1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_PSR1_en(1, num);
	return 0;
}

static int do_dptx1_PSR2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char num;

	if (argc == 1)
		return -1;

	num = (unsigned char)simple_strtoul(argv[1], NULL, 10);
	dptx_driver_PSR2_en(1, num);
	return 0;
}

static cmd_tbl_t cmd_dptx0_sub[] = {
	U_BOOT_CMD_MKENT(probe,   2, 0, do_dptx_probe,       "", ""),
	U_BOOT_CMD_MKENT(enable,  2, 0, do_dptx0_enable,     "", ""),
	U_BOOT_CMD_MKENT(disable, 2, 0, do_dptx0_disable,    "", ""),
	U_BOOT_CMD_MKENT(test,    3, 0, do_dptx0_test,       "", ""),
	U_BOOT_CMD_MKENT(info,    2, 0, do_dptx0_info,       "", ""),
	U_BOOT_CMD_MKENT(reg,     2, 0, do_dptx0_reg,        "", ""),
	U_BOOT_CMD_MKENT(list,    2, 0, do_dptx0_list_vmode, "", ""),
	U_BOOT_CMD_MKENT(set,     2, 0, do_dptx0_set_vmode,  "", ""),
	U_BOOT_CMD_MKENT(rst,     2, 0, do_dptx0_reset,      "", ""),
	U_BOOT_CMD_MKENT(psr1,    2, 0, do_dptx0_PSR1,       "", ""),
	U_BOOT_CMD_MKENT(psr2,    2, 0, do_dptx0_PSR2,       "", ""),
};

static cmd_tbl_t cmd_dptx1_sub[] = {
	U_BOOT_CMD_MKENT(probe,   2, 0, do_dptx_probe,       "", ""),
	U_BOOT_CMD_MKENT(enable,  2, 0, do_dptx1_enable,     "", ""),
	U_BOOT_CMD_MKENT(disable, 2, 0, do_dptx1_disable,    "", ""),
	U_BOOT_CMD_MKENT(test,    3, 0, do_dptx1_test,       "", ""),
	U_BOOT_CMD_MKENT(info,    2, 0, do_dptx1_info,       "", ""),
	U_BOOT_CMD_MKENT(reg,     2, 0, do_dptx1_reg,        "", ""),
	U_BOOT_CMD_MKENT(list,    2, 0, do_dptx1_list_vmode, "", ""),
	U_BOOT_CMD_MKENT(set,     2, 0, do_dptx1_set_vmode,  "", ""),
	U_BOOT_CMD_MKENT(rst,     2, 0, do_dptx1_reset,      "", ""),
	U_BOOT_CMD_MKENT(psr1,    2, 0, do_dptx1_PSR1,       "", ""),
	U_BOOT_CMD_MKENT(psr2,    2, 0, do_dptx1_PSR2,       "", ""),
};

static int do_dptx0(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	// /* Strip off leading 'bmp' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_dptx0_sub[0], ARRAY_SIZE(cmd_dptx0_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);

	cmd_usage(cmdtp);
	return 1;
}

static int do_dptx1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	// /* Strip off leading 'bmp' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_dptx1_sub[0], ARRAY_SIZE(cmd_dptx1_sub));

	if (c)
		return c->cmd(cmdtp, flag, argc, argv);

	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(dptx, 8, 0, do_dptx0, "DisplayPort TX 0 module",
	" - probe        - probe   DPTX Driver\n"
	" - enable       - enable  DPTX module\n"
	" - disable      - disable DPTX module\n"
	" - list         - list DPTX all vmode\n"
	" - set          - change DPTX vmode\n"
	" - info         - show DPTX parameters\n"
	" - reg          - dump DPTX registers\n"
	" - test         - show DPTX bist pattern\n");

U_BOOT_CMD(dptx0, 8, 0, do_dptx0, "DisplayPort TX 0 module",
	" - probe / enable / disable / list / set / info / reg / test\n");

U_BOOT_CMD(dptx1, 8, 0, do_dptx1, "DisplayPort TX 1 module",
	" - probe / enable / disable / list / set / info / reg / test\n");

