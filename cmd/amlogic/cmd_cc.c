// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/usb.h>

static int do_cc_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	print_aml_cc_ufp_current_type();
	return 0;
}

U_BOOT_CMD(cc_test, 1, 1, do_cc_test,
	   "Amlogic CC test",
	   "- Check CC UFP current status.\n"
);
