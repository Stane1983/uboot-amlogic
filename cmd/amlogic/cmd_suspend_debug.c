// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <linux/arm-smccc.h>

#define cmd_suspend_debug_info(fmt...)	printf("[cmd_suspend_debug] "fmt)
#define cmd_suspend_debug_err(fmt...)	printf("[cmd_suspend_debug] "fmt)
#define SUSPEND_DEBUG_LOGLEVEL (0xf << (0))
#define SUSPEND_DEBUG_INITCALL_DEBUG (1U << (4))
#define MAX_ENTRIES 32
#define SUSPEND_DEBUG_FUNCTION_ID           0x820000F3

static unsigned long __invoke_psci_fn_smc(unsigned long function_id,
						unsigned long arg0,
						unsigned long arg1,
						unsigned long arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, 0, 0, 0, 0, &res);
	return res.a0;
}

struct Suspend_Debug_Mode {
	const char *description;
	int end_bit;
	int start_bit;
	int state;
};

struct Suspend_Debug_Mode suspend_debug_mode[MAX_ENTRIES] = {
	{"[BL30] clk/pll status check and show clk gate setting", 31, 31, 0},
	{"[BL30] mask RTC Wakeup source", 30, 30, 0},
	{"[BL30] mask SARADC Wakeup source", 29, 29, 0},
	{"[BL30] mask IR Wakeup source", 28, 28, 0},
	{"[BL30] show pwm state and voltage", 27, 27, 0},
	{"[BL30] function call log", 26, 26, 0},
	{"[BL30] open dmc monitor log", 25, 25, 0},
	{"[BL30] bl30 dump boot cpu's fsm and dsu's fsm", 24, 24, 0},
	{"[BL30] aocpu do ddr access(reserved)", 23, 23, 0},
	{"[BL30] skip power switch(vddee vddcpu vcc5v vcc3.3V etc)", 22, 22, 0},
	{"[BL30] skip ddr suspend in bl30", 21, 21, 0},
	{"[BL30] start debug task", 20, 20, 0},

	{"[BL31] reserved", 19, 17, 0},
	{"[BL31] skip clk gate switch", 16, 16, 0},
	{"[BL31] dump nonboot cpu's fsm state", 15, 15, 0},
	{"[BL31] skip ddr suspend in tlpm", 14, 14, 0},
	{"[BL31] power domain clk/pll status check and show clk gate setting", 13, 13, 0},
	{"[BL31] do clk reg check and rate check", 12, 12, 0},
	{"[BL31] do bl31 ddr checksum in tlpm", 11, 11, 0},
	{"[BL31] open dmc monitor log", 10, 10, 0},
	{"[BL31] bl31 debug loglevel(0:without log;1: normal;2: function call log)", 9, 8, 0},

	{"[KERNEL] reserved for dmc monitor", 7, 4, 0},
	{"[KERNEL] set kernel printk loglevel to 0~9", 3, 0, 0},
};

static unsigned int suspend_debug_flag;

static void check_suspend_debug_mode(void)
{
	int i;

	for (i = 0; i < MAX_ENTRIES; i++) {
		int mask = (1 << (suspend_debug_mode[i].end_bit + 1 -
				  suspend_debug_mode[i].start_bit)) - 1;
		int value = (suspend_debug_flag >> suspend_debug_mode[i].start_bit) & mask;

		suspend_debug_mode[i].state = value;
	}
}

static int do_show_suspend_debug(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	char buf[256];
	int len = 0;

	len += sprintf(buf + len, "\n suspend_debug_flag = 0x%08x\n", suspend_debug_flag);
	len += sprintf(buf + len, "-----------------------------------------\n");
	len += sprintf(buf + len, "debug bit summary:\n");
	check_suspend_debug_mode();
	for (int i = 0; i < MAX_ENTRIES; i++) {
		if (!suspend_debug_mode[i].description)
			continue;
		if (suspend_debug_mode[i].start_bit == suspend_debug_mode[i].end_bit) {
			len += snprintf(buf + len, sizeof(buf) - len, "%-60s bit[%2d] : %d\n",
				suspend_debug_mode[i].description,
				suspend_debug_mode[i].start_bit,
				suspend_debug_mode[i].state);
		} else {
			len += snprintf(buf + len, sizeof(buf) - len, "%-60s bit[%2d:%-2d] : 0x%x\n",
				suspend_debug_mode[i].description,
				suspend_debug_mode[i].start_bit,
				suspend_debug_mode[i].end_bit,
				suspend_debug_mode[i].state);
		}
		if (len > 200) {
			printf("%s", buf);
			len = 0;
		}
	}
	if (len > 0)
		printf("%s", buf);

	return CMD_RET_SUCCESS;
}

void set_suspend_debug_flag(int suspend_flag)
{
	__invoke_psci_fn_smc(SUSPEND_DEBUG_FUNCTION_ID, 0, suspend_debug_flag &
		(~(SUSPEND_DEBUG_LOGLEVEL | SUSPEND_DEBUG_INITCALL_DEBUG)),
		  0);
}

static int do_set_suspend_debug(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc != 2) {
		cmd_suspend_debug_err("Usage: set_suspend_debug <hex_value>\n");
		return CMD_RET_USAGE;
	}
	if (strict_strtoul(argv[1], 16, (unsigned long *)&suspend_debug_flag)) {
		cmd_suspend_debug_err("Invalid hex value: %s\n", argv[1]);
		return CMD_RET_FAILURE;
	}
	set_suspend_debug_flag(suspend_debug_flag);
	cmd_suspend_debug_info("Set suspend_debug_flag to 0x%08x\n", suspend_debug_flag);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	show_suspend_debug, 1, 0, do_show_suspend_debug,
	"Display current suspend debug flags",
	"\nExample: show_suspend_debug\n"
);

U_BOOT_CMD(
	set_suspend_debug, 2, 0, do_set_suspend_debug,
	"Set suspend debug flags for BL30/BL31",
	"<hex_value>\n"
	"Example: set_suspend_debug 0x04000219\n"
);

