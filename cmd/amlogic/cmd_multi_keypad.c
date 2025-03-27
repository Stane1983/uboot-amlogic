// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <amlogic/ini.h>
#include <amlogic/keyunify.h>

static int do_multi_keypad(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	if (argc > 1)
		return CMD_RET_USAGE;

	ret = handle_multi_keypad();
	return ret;
}

U_BOOT_CMD
(multi_keypad, 4, 0, do_multi_keypad,
	"handle multi keypad",
	" "
);

