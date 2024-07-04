// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <tee.h>
#include <amlogic/tee_aml.h>

#define TEE_PARAM_NUM                  (4)

#define PTA_TVP_UUID { 0x1a658fe8, 0x894e, 0x4403, \
	{ 0xae, 0xa6, 0x5a, 0xe6, 0x91, 0xe8, 0xa3, 0x5f } }

/* tvp memory command */
#define TVP_CMD_PROTECT_MEM             0
#define TVP_CMD_UNPROTECT_MEM           1

u32 tee_protect_mem(u32 type, u32 level,
		phys_addr_t start, size_t size, u32 *handle)
{
	int ret = 0;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param param[TEE_PARAM_NUM] = { 0 };
	const struct tee_optee_ta_uuid uuid = PTA_TVP_UUID;

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("tee_find_device() failed");
		return -ENODEV;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		printf("tee_open_session() failed, ret = 0x%x\n", ret);
		return ret;
	}
	if (open_arg.ret) {
		printf("tee_open_session() failed, ret = 0x%x, ret_origin=0x%x \n",
				open_arg.ret, open_arg.ret_origin);
		return open_arg.ret;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = type;
	param[0].u.value.b = level;

	param[1].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = start & 0xffffffff;
	param[1].u.value.b = (sizeof(phys_addr_t) == sizeof(u32)) ? 0 : start >> 32;

	param[2].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[2].u.value.a = size & 0xffffffff;
	param[2].u.value.b = (sizeof(size_t) == sizeof(u32)) ? 0 : size >> 32;
	param[3].attr = TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT;

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = TVP_CMD_PROTECT_MEM;

	ret = tee_invoke_func(dev, &invoke_arg, ARRAY_SIZE(param), param);
	if (ret) {
		printf("tee_invoke_func() failed, ret 0x%x\n", ret);
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("tee_invoke_func() failed, ret 0x%x, origin 0x%x\n",
				invoke_arg.ret, invoke_arg.ret_origin);
		ret = invoke_arg.ret;
		goto exit;
	}

	*handle = (u32)param[3].u.value.a;

exit:
	tee_close_session(dev, open_arg.session);

	return ret;
}

void tee_unprotect_mem(u32 handle)
{
	int ret = 0;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param param[TEE_PARAM_NUM] = { 0 };
	const struct tee_optee_ta_uuid uuid = PTA_TVP_UUID;

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("tee_find_device() failed");
		return;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		printf("tee_open_session() failed, ret = 0x%x\n", ret);
		return;
	}
	if (open_arg.ret) {
		printf("tee_open_session() failed, ret = 0x%x, ret_origin=0x%x \n",
				open_arg.ret, open_arg.ret_origin);
		return;
	}

	memset(param, 0, sizeof(param));
	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = handle;

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = TVP_CMD_UNPROTECT_MEM;

	ret = tee_invoke_func(dev, &invoke_arg, ARRAY_SIZE(param), param);
	if (ret) {
		printf("tee_invoke_func() failed, ret 0x%x\n", ret);
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("tee_invoke_func() failed, ret 0x%x, origin 0x%x\n",
				invoke_arg.ret, invoke_arg.ret_origin);
		ret = invoke_arg.ret;
		goto exit;
	}

exit:
	tee_close_session(dev, open_arg.session);

	return;
}
