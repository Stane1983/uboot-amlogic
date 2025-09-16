// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <dm.h>
#include <asm/io.h>
#include <common.h>
#include <command.h>
#include <errno.h>
#include <misc.h>
#include <asm/amlogic/arch/mailbox.h>
#include <linux/libfdt.h>

#define KEY_PARAMS_NUM  10
#define OTHER_PARAMS_NUM 1

#define MAX_KEY_NUM 16

enum IrMboxCmdType {
	IR_MBOX_CMD_SET_WAKEUP_LIST,
	IR_MBOX_CMD_SET_DEBUG_LOG,
	IR_MBOX_CMD_GET_WAKEUP_KEY,
	IR_MBOX_CMD_SET_STATUS,
	IR_MBOX_CMD_GET_PREBOOT_KEY
};

extern u32 get_time(void);

int update_for_dtb_wakeup_key(void)
{
	const void *nodep;	/* property node pointer */
	int  nodeoffset;	/* node offset from libfdt */
	int  len, j, i;		/* length of the property */
	const __be32 *p;
	u32 framecode, ret;
	u32 data[MAX_KEY_NUM + 3] = {IR_MBOX_CMD_SET_WAKEUP_LIST, 0, 0};

	nodeoffset = fdt_path_offset(working_fdt, "/ir_wakeup_key");
	if (nodeoffset < 0) {
		printf("libfdt fdt_path_offset() returned %s\n",
		       fdt_strerror(nodeoffset));
		return 1;
	}

	nodep = fdt_getprop(working_fdt, nodeoffset, "wakeup_key", &len);
	if (!nodep || len <= 0) {
		printf("libfdt fdt_getprop(): %s\n", fdt_strerror(len));
		return 1;
	}

	for (i = 0, j = 0, p = nodep; j < len / 4; j += 3) {
		data[i + 3] = fdt32_to_cpu(p[j]);
		i++;
		if (i != MAX_KEY_NUM)
			continue;

		i = 0;
		data[1] = 16;
		ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_IR_STATE, data,
				     sizeof(data), &framecode, sizeof(framecode));
		if (ret != 0)
			printf("CMD_SET_IR_STATE failed\n");
	}

	data[1] = i;
	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_IR_STATE, data,
			     sizeof(data), &framecode, sizeof(framecode));
	if (ret != 0)
		printf("CMD_SET_IR_STATE failed\n");

	printf("update bl30 wakeup key from DTB\n");

	return 0;
}

static int do_irkey(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 key_buf[KEY_PARAMS_NUM];
	u32 time_out = 0;
	u32 time_base = 0;
	struct udevice *ir_devp;
	u32 key;
	char *endp;
	char str[16] = {0};
	u8 i;

	/*at least set a key*/
	if (argc < 3)
		return -EINVAL;

	/*obtain timeout time*/
	time_out = simple_strtoul(argv[1], &endp, 0);
	printf("time_out = %d\n", time_out);
	if (*argv[1] == 0 || *endp != 0)
		return -EINVAL;

	/*obtain IR keys value which need to detect*/
	for (i = 2; i < argc; i++) {
		key_buf[i - 2] = simple_strtoul(argv[i], &endp, 0);
		printf("key[%d] = %x\n", i - 2, key_buf[i - 2]);
		if (*argv[i] == 0 || *endp != 0)
			return -EINVAL;
	}

	if (uclass_get_device_by_name(UCLASS_MISC, "meson-ir", &ir_devp)) {
		printf("failed to get ir udevice\n");
		return -EINVAL;
	}

	time_base = get_time();

	while ((get_time() - time_base) < time_out) {
		if (misc_read(ir_devp, 0, &key, 1))
			continue;

		printf("keycode = %x\n", key);
		for (i = 2; i < argc; i++) {
			if (key == key_buf[i - 2]) {
				snprintf(str, sizeof(str), "0x%x", key);
				env_set("irkey_value", str);
				return 0;
			}
		}
	}

	return -EINVAL;
}

static int do_irkey_wakeup(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *endp;
	u32 data[MAX_KEY_NUM + 3];
	u32 framecode, ret;
	char str[16] = {0};
	u8 i;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (strncmp(argv[1], "update4dtb", 8) == 0) {
		update_for_dtb_wakeup_key();
		return 0;
	}

	for (i = 1; i < argc; i++) {
		data[i - 1] = simple_strtoul(argv[i], &endp, 0);
		if (*argv[i] == 0 || *endp != 0)
			return -EINVAL;
	}

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_IR_STATE, data,
			     sizeof(data), &framecode, sizeof(framecode));
	if (ret != 0)
		printf("CMD_SET_IR_STATE failed\n");

	snprintf(str, sizeof(str), "0x%x", framecode);
	if (data[0] == IR_MBOX_CMD_GET_WAKEUP_KEY)
		env_set("ir_wakeup_key", str);
	else if (data[0] == IR_MBOX_CMD_GET_PREBOOT_KEY)
		env_set("ir_preboot_key", str);

	return 0;
}

/*Max key arguments: 10*/
U_BOOT_CMD(irkey, (KEY_PARAMS_NUM + OTHER_PARAMS_NUM + 1), 0, do_irkey,
	"irkey <timeout> <key1> ...<keyN> - maximum value of N: 10",
	NULL
);

U_BOOT_CMD(irkey_wakeup, (MAX_KEY_NUM + 3 + 1), 0, do_irkey_wakeup,
	"irkey_wakeup",
	"0 <key_num> <reason_bitmap> <key0> <key1> ... <key15>\n"
	"			- set wakeup table\n"
	"irkey_wakeup 1 <1|0>	- enable/disable bl30 ir debug log\n"
	"irkey_wakeup 2		- get ir_wakeup_key\n"
	"irkey_wakeup 3 <1|0>	- enable/disable bl30 ir\n"
	"irkey_wakeup 4		- get ir_preboot_key\n"
	"irkey_wakeup update4dtb	- update wakeup key for dtb\n"
);
