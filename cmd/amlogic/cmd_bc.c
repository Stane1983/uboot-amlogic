// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/usb.h>

static int do_bc_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	print_aml_bc_port_status();
	return 0;
}

U_BOOT_CMD(bc_test, 1, 1, do_bc_test,
	   "Amlogic BC test",
	   "- Check BC Port status.\n"
);
