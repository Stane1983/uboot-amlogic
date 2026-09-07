/*
* Copyright (C) 2025 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: this source is used to provision function
* Update history:
*/

#include <common.h>
#include <command.h>
#include <tee.h>
#include "factory_provision_utils.h"

static int provision_regen_pcpk(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = CMD_RET_FAILURE;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param params[TEE_PARAM_NUM] = { 0 };
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

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = PROVISION_PTA_CMD_DERIVE_KEY;

	memset(params, 0, sizeof(params));
	params[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	params[0].u.value.a = PROVISION_DERIVE_PCPK;

	ret = tee_invoke_func(dev, &invoke_arg, TEE_PARAM_NUM, params);
	if (ret) {
		printf("invoke failed, cmd = 0x%x, key type = %lld, ret = 0x%x\n",
			invoke_arg.func, params[0].u.value.a, ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("invoke failed, cmd = 0x%x, key type = %lld, ret = 0x%x, origin = 0x%x\n",
			invoke_arg.func, params[0].u.value.a, invoke_arg.ret, invoke_arg.ret_origin);
		ret = CMD_RET_FAILURE;
		goto exit;
	}
	printf("Provision regenerate PCPK successfully!\n");

exit:
	tee_close_session(dev, open_arg.session);

	return ret;
}

static int provision_regen_pfpk(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = CMD_RET_FAILURE;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param params[TEE_PARAM_NUM] = { 0 };
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

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = PROVISION_PTA_CMD_DERIVE_KEY;

	memset(params, 0, sizeof(params));
	params[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	params[0].u.value.a = PROVISION_DERIVE_PFPK;

	ret = tee_invoke_func(dev, &invoke_arg, TEE_PARAM_NUM, params);
	if (ret) {
		printf("invoke failed, cmd = 0x%x, key type = %lld, ret = 0x%x\n",
			invoke_arg.func, params[0].u.value.a, ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	if (invoke_arg.ret) {
		printf("invoke failed, cmd = 0x%x, key type = %lld, ret = 0x%x, origin = 0x%x\n",
			invoke_arg.func, params[0].u.value.a, invoke_arg.ret, invoke_arg.ret_origin);
		ret = CMD_RET_FAILURE;
		goto exit;
	}
	printf("Provision regenerate PFPK successfully!\n");

exit:
	tee_close_session(dev, open_arg.session);

	return ret;
}

static cmd_tbl_t cmd_provision_sub[] = {
	U_BOOT_CMD_MKENT(regen_pcpk, 1, 0, provision_regen_pcpk, "", ""),
	U_BOOT_CMD_MKENT(regen_pfpk, 1, 0, provision_regen_pfpk, "", ""),
};

static int do_provision(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	/* Check uboot sub command */
	c = find_cmd_tbl(argv[0], &cmd_provision_sub[0], ARRAY_SIZE(cmd_provision_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

static char provision_help_text[] =
	"\n"
	"[provision regen_pcpk] -Provision regenerate PCPK\n"
	"[provision regen_pfpk] -Provision regenerate PFPK\n";

U_BOOT_CMD(provision, CONFIG_SYS_MAXARGS, 0, do_provision,
	"Provision Function", provision_help_text);