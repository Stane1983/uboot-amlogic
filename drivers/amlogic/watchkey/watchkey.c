// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/amlogic/arch/mailbox.h>
#include <amlogic/watchkey.h>

static char * const wk_list[] = {
	[WATCHKEY_ID_ADCKEY] = "adc-keypad",
	[WATCHKEY_ID_GPIOKEY] = "gpio-keypad",
	NULL
};

char * const *amlogic_watchkey_list(void)
{
	return wk_list;
}

#define WATCHKEY_CMD_STATE		0x0001
#define WATCHKEY_CMD_DISABLE		0x0002
#define WATCHKEY_CMD_ENABLE		0x0003
#define WATCHKEY_CMD_DUMP		0x0004
#define WATCHKEY_CMD_TEST		0x0005

#pragma pack(push, 1)
struct __mbox_load {
	u16 id;
	u16 cmd;
	u32 data[3];
};

#pragma pack(pop)

static int __watchkey_mbox_read(struct __mbox_load *load)
{
	int ret;

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_WATCHKEY_CTRL,
			     load, sizeof(*load), load, sizeof(*load));
	if (ret) {
		pr_err("mbox read failed\n");
		return ret;
	}

	return 0;
}

static int __watchkey_mbox_write(struct __mbox_load *load)
{
	int ret;

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_WATCHKEY_CTRL,
			     load, sizeof(*load), NULL, 0);
	if (ret) {
		pr_err("mbox write failed\n");
		return ret;
	}

	return 0;
}

static int __watchkey_bool_get(u16 id, u16 cmd)
{
	int ret;
	struct __mbox_load load = {
		.id = id,
		.cmd = cmd,
		.data = { 0 },
	};

	ret = __watchkey_mbox_read(&load);
	if (ret)
		return ret;

	return (int)load.data[0];
}

static int __watchkey_id_check(u16 id)
{
	if (id >= (ARRAY_SIZE(wk_list) - 1))
		return -EINVAL;

	return 0;
}

int amlogic_watchkey_state(u16 id)
{
	int ret;

	ret = __watchkey_id_check(id);
	if (ret)
		return ret;

	return __watchkey_bool_get(id, WATCHKEY_CMD_STATE);
}

int amlogic_watchkey_enable(u16 id)
{
	int ret;
	struct __mbox_load load = {
		.id = id,
		.cmd = WATCHKEY_CMD_ENABLE,
		.data = { 0 },
	};

	ret = __watchkey_id_check(id);
	if (ret)
		return ret;

	return __watchkey_mbox_write(&load);
}

int amlogic_watchkey_disable(u16 id)
{
	int ret;
	struct __mbox_load load = {
		.id = id,
		.cmd = WATCHKEY_CMD_DISABLE,
		.data = { 0 },
	};

	ret = __watchkey_id_check(id);
	if (ret)
		return ret;

	return __watchkey_mbox_write(&load);
}

int amlogic_watchkey_dump(u16 id, u32 *d0, u32 *d1, u32 *d2)
{
	int ret;
	struct __mbox_load load = {
		.id = id,
		.cmd = WATCHKEY_CMD_DUMP,
		.data = { 0 },
	};

	ret = __watchkey_id_check(id);
	if (ret)
		return ret;

	ret = __watchkey_mbox_read(&load);
	if (ret)
		return ret;

	if (d0)
		*d0 = load.data[0];
	if (d1)
		*d1 = load.data[1];
	if (d2)
		*d2 = load.data[2];

	return 0;
}

int amlogic_watchkey_test(u16 id)
{
	int ret;

	ret = __watchkey_id_check(id);
	if (ret)
		return ret;

	return __watchkey_bool_get(id, WATCHKEY_CMD_TEST);
}
