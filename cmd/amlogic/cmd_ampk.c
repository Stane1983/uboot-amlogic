// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <tee.h>

#define PROVISION_PTA_UUID \
		{ 0x24b17b16, 0x89d1, 0x43a3, \
		{ 0x95, 0xed, 0x19, 0x3c, 0xe1, 0xed, 0xa7, 0x15 } }

#define PROVISION_PTA_CMD_AMPK_GET_DEVICE_PUBLIC_KEY   (1)
#define PROVISION_PTA_CMD_AMPK_REGEN_ENC_KEY           (2)
#define TEE_PARAM_NUM                                  (4)

static int get_device_pub_key(u8 *public_key, u32 public_key_size)
{
	int ret = CMD_RET_FAILURE;
	u8 input[4] = {0};
	u32 input_size = sizeof(input);
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param param[TEE_PARAM_NUM] = { 0 };
	const struct tee_optee_ta_uuid uuid = PROVISION_PTA_UUID;

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("tee find device failed\n");
		return CMD_RET_FAILURE;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		printf("open session failed, ret = 0x%x\n", ret);
		return CMD_RET_FAILURE;
	}

	if (open_arg.ret) {
		printf("open session failed, ret = 0x%x, ret_origin = 0x%x\n",
		       open_arg.ret, open_arg.ret_origin);
		return CMD_RET_FAILURE;
	}

	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.size = input_size;
	ret = tee_shm_alloc(dev, input_size, 0, &param[0].u.memref.shm);
	if (ret) {
		printf("tee shm alloc input failed, ret = 0x%x\n", ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}
	memcpy(param[0].u.memref.shm->addr, input, input_size);

	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[1].u.memref.size = public_key_size;
	ret = tee_shm_alloc(dev, public_key_size, 0, &param[1].u.memref.shm);
	if (ret) {
		printf("tee shm alloc for key failed, ret = 0x%x\n", ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = PROVISION_PTA_CMD_AMPK_GET_DEVICE_PUBLIC_KEY;

	ret = tee_invoke_func(dev, &invoke_arg, TEE_PARAM_NUM, param);
	if (ret) {
		printf("invoke failed, cmd = 0x%x, ret = 0x%x\n", invoke_arg.func, ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("invoke failed, cmd = 0x%x, ret = 0x%x, origin = 0x%x\n",
		       invoke_arg.func, invoke_arg.ret, invoke_arg.ret_origin);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	if (param[1].u.memref.size <= public_key_size) {
		memcpy(public_key, param[1].u.memref.shm->addr, param[1].u.memref.size);
		ret = CMD_RET_SUCCESS;
	} else {
		ret = CMD_RET_FAILURE;
	}

exit:
	if (param[0].u.memref.shm)
		tee_shm_free(param[0].u.memref.shm);

	if (param[1].u.memref.shm)
		tee_shm_free(param[1].u.memref.shm);

	tee_close_session(dev, open_arg.session);

	return ret;
}

static int regen_enc_key(void)
{
	int ret = CMD_RET_FAILURE;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param param[TEE_PARAM_NUM] = { 0 };
	const struct tee_optee_ta_uuid uuid = PROVISION_PTA_UUID;

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("tee find device failed\n");
		return CMD_RET_FAILURE;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		printf("open session failed, ret = 0x%x\n", ret);
		return CMD_RET_FAILURE;
	}

	if (open_arg.ret) {
		printf("open session failed, ret = 0x%x, ret_origin = 0x%x\n",
		       open_arg.ret, open_arg.ret_origin);
		return CMD_RET_FAILURE;
	}

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = 0;
	param[0].u.value.b = 0;
	param[0].u.value.c = 0;

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = PROVISION_PTA_CMD_AMPK_REGEN_ENC_KEY;

	ret = tee_invoke_func(dev, &invoke_arg, TEE_PARAM_NUM, param);
	if (ret) {
		printf("invoke failed, cmd = 0x%x, ret = 0x%x\n", invoke_arg.func, ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("invoke failed, cmd = 0x%x, ret = 0x%x, origin = 0x%x\n",
		       invoke_arg.func, invoke_arg.ret, invoke_arg.ret_origin);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

exit:
	tee_close_session(dev, open_arg.session);

	return ret;
}

static int do_apmk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8 public_key[64];
	u32 public_key_size = sizeof(public_key);

	if (argc < 2)
		return CMD_RET_USAGE;
	if (strcmp(argv[1], "get_device_pub_key") == 0) {
		if (get_device_pub_key(public_key, public_key_size) ==
				CMD_RET_SUCCESS) {
			u32 i = 0;

			printf("ampk device public key ");
			for (i = 0; i < public_key_size; i++)
				printf("%02x", public_key[i]);
			printf("\n");
			return CMD_RET_SUCCESS;
		}
	} else if (strcmp(argv[1], "regen_enc_key") == 0) {
		return (regen_enc_key() == CMD_RET_SUCCESS);
	}
	return CMD_RET_USAGE;
}

U_BOOT_CMD(ampk, 2, 0, do_apmk,
	   "AMPK cmds",
	   "ampk get_device_pub_key\n"
	   "ampk regen_enc_key\n"
);
