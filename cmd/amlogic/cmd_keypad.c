// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/mailbox.h>

union __mbox_load {
	struct {
		u32 code;
		u32 value;
		u32 channel;
		u32 event;
	} adc_key;
	struct {
		u32 id;
		u32 level;
		u32 event;
		u32 event_reserved;
	} gpio_key;
	u32 op;
};

static int __keypad_mbox_write(union __mbox_load *load)
{
	int ret;

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_KEYPAD,
			     load, sizeof(*load), NULL, 0);
	if (ret) {
		pr_err("mbox write failed\n");
		return ret;
	}

	return 0;
}

static int do_keypad(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *cmd;
	const char *wakeup_type;
	char *endp;
	int ret;
	union __mbox_load load = { 0 };

	if (argc < 2)
		return CMD_RET_USAGE;
	cmd = argv[1];

	if (!strcmp(cmd, "wakeup")) {
		if (argc < 3)
			return CMD_RET_USAGE;
		wakeup_type = argv[2];

		if (!strcmp(wakeup_type, "adc")) {
			if (argc < 7)
				return CMD_RET_USAGE;

			/* Parse ADC keypad param */
			load.adc_key.code = simple_strtoul(argv[3], &endp, 0);
			if (endp == argv[3])
				return CMD_RET_USAGE;
			load.adc_key.value = simple_strtoul(argv[4], &endp, 0);
			load.adc_key.value <<= 2;
			if (endp == argv[4])
				return CMD_RET_USAGE;
			load.adc_key.channel = simple_strtoul(argv[5], &endp, 0);
			if (endp == argv[5])
				return CMD_RET_USAGE;
			load.adc_key.event = simple_strtoul(argv[6], &endp, 0);
			if (endp == argv[6])
				return CMD_RET_USAGE;

			ret = __keypad_mbox_write(&load);
			if (ret)
				return CMD_RET_USAGE;

			return CMD_RET_SUCCESS;
		} else if (!strcmp(wakeup_type, "gpio")) {
			if (argc < 6)
				return CMD_RET_USAGE;

			/* Parse GPIO keypad param */
			load.gpio_key.id = simple_strtoul(argv[3], &endp, 0);
			if (endp == argv[3])
				return CMD_RET_USAGE;
			load.gpio_key.level = simple_strtoul(argv[4], &endp, 0);
			if (endp == argv[4])
				return CMD_RET_USAGE;
			load.gpio_key.event = simple_strtoul(argv[5], &endp, 0);
			if (endp == argv[5])
				return CMD_RET_USAGE;
			load.gpio_key.event_reserved = load.gpio_key.event;
			ret = __keypad_mbox_write(&load);
			if (ret)
				return CMD_RET_USAGE;

			return CMD_RET_SUCCESS;
		} else if (!strcmp(wakeup_type, "remove")) {
			load.op = 0xffffffff;

			ret = __keypad_mbox_write(&load);
			if (ret)
				return CMD_RET_USAGE;

			return CMD_RET_SUCCESS;
		}
	}

	return CMD_RET_USAGE;
}

static char keypad_help_text[] =
	"Sub-System\n"
	"keypad wakeup adc <code> <value> <channel> <event>  - set adc wakeup key\n"
	"                  - code: the value range is >= 512.           e.g. 512\n"
	"                  - value: the value range is 0 ~ 1023.        e.g. 2048\n"
	"                  - channel: the value range is 0 ~ 7.         e.g. 2\n"
	"                  - event: 1 is short press, 2 is long press.  e.g. 1\n"
	"keypad wakeup gpio <id> <level> <event>             - set gpio wakeup key\n"
	"                  - id: refs rtos_sdk/soc/riscv/<family>/gpio-data.h\n"
	"                  - level: 0 or 1, meaning the level status when inactive.\n"
	"                  - event: 1 is short press, 2 is long press.\n"
	"keypad wakeup remove                                - remove all wakeup keys\n";

U_BOOT_CMD(keypad, 7, 0, do_keypad, "Control keypad functions", keypad_help_text);
