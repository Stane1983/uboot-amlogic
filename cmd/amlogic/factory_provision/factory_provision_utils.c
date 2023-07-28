// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <common.h>
#include <tee.h>
#include <u-boot/sha256.h>
#include <amlogic/emmc_partitions.h>
#include "factory_provision_utils.h"

void usage(void)
{
	printf("factory_provision -- provision keybox\n\n"
	"Usage:\n"
	"factory_provision write <keybox_name> <keybox_addr> <keybox_size>\n"
	"	- write keybox to key partition\n\n"
	"factory_provision query <keybox_name> [ret_data_addr]\n"
	"	- query whether the keybox exists by keybox name\n"
	"	- when keybox exists, return data: keybox_size(4bytes)\n\n"
	"factory_provision list\n"
	"	- list all keyboxs of key partition\n\n"
	"factory_provision remove <keybox_name>\n"
	"	- remove the keybox by keybox name\n\n"
	"factory_provision clear\n"
	"	- clear all keyboxs of key partition\n\n"
	"factory_provision version\n"
	"	- show version of factory provision\n");
}

void show_version(void)
{
	LOGI("version %s\n", FACTORY_PROVISION_VERSION);
}

void parse_params(int argc, char * const argv[], struct input_param *params)
{
	memset(params, 0, sizeof(struct input_param));
	switch (argc) {
	case 5:
		if (!memcmp(argv[1], "write", strlen("write"))) {
			params->action = ACTION_WRITE;
			params->keybox_name = argv[2];
			params->keybox_phy_addr =
				(u32)simple_strtoul(argv[3], NULL, 0);
			params->keybox_size =
				(u32)simple_strtoul(argv[4], NULL, 0);
		}
		break;
	case 4:
		if (!memcmp(argv[1], "query", strlen("query"))) {
			params->action = ACTION_QUERY;
			params->keybox_name = argv[2];
			params->ret_data_addr =
				(u32)simple_strtoul(argv[3], NULL, 0);
		}
		break;
	case 3:
		if (!memcmp(argv[1], "query", strlen("query"))) {
			params->action = ACTION_QUERY;
			params->keybox_name = argv[2];
			if (env_get("loadaddr"))
				params->ret_data_addr =
					(u32)simple_strtoul(
					(char * const)env_get("loadaddr"),
					NULL,
					0);
			else
				params->ret_data_addr = CONFIG_SYS_LOAD_ADDR;
		} else if (!memcmp(argv[1], "remove", strlen("remove"))) {
			params->action = ACTION_REMOVE;
			params->keybox_name = argv[2];
		}
		break;
	case 2:
		if (!memcmp(argv[1], "init", strlen("init")))
			params->action = ACTION_INIT;
		else if (!memcmp(argv[1], "clear", strlen("clear")))
			params->action = ACTION_CLEAR;
		else if (!memcmp(argv[1], "list", strlen("list")))
			params->action = ACTION_LIST;
		else if (!memcmp(argv[1], "version", strlen("version")))
			params->action = ACTION_VERSION;
		break;
	default:
		break;
	}
}

int check_params(const struct input_param *params)
{
	int ret = CMD_RET_SUCCESS;

	switch (params->action) {
	case ACTION_INIT:
	case ACTION_CLEAR:
	case ACTION_LIST:
	case ACTION_VERSION:
		break;
	case ACTION_WRITE:
		if (strlen(params->keybox_name) > MAX_SIZE_KEYBOX_NAME) {
			LOGE("keybox name is too long, and max length is %d\n", MAX_SIZE_KEYBOX_NAME);
			ret = CMD_RET_KEYBOX_NAME_TOO_LONG;
			goto exit;
		}
		if (params->keybox_size > MAX_SIZE_KEYBOX) {
			LOGE("keybox size is too large, and max size is %d\n", MAX_SIZE_KEYBOX);
			ret = CMD_RET_KEYBOX_TOO_LARGE;
			goto exit;
		}
		if (!params->keybox_phy_addr) {
			LOGE("keybox addr error\n");
			ret = CMD_RET_BAD_PARAMETER;
			goto exit;
		}
		break;
	case ACTION_QUERY:
	case ACTION_REMOVE:
		if (strlen(params->keybox_name) > MAX_SIZE_KEYBOX_NAME) {
			LOGE("keybox name is too long, and max length is %d\n", MAX_SIZE_KEYBOX_NAME);
			ret = CMD_RET_KEYBOX_NAME_TOO_LONG;
			goto exit;
		}
		break;
	default:
		usage();
		ret = CMD_RET_BAD_PARAMETER;
		break;
	}

exit:
	return ret;
}

void convert_to_uuid_str(const char uuid[16], char uuid_str[40])
{
	int i = 0;
	const char *uuid_ptr = uuid;
	char *str_ptr = uuid_str;

	for (i = 0; i < 4; i++) {
		sprintf(str_ptr, "%02x", *uuid_ptr);
		str_ptr += 2;
		uuid_ptr++;
	}
	*str_ptr++ = '-';
	for (i = 0; i < 2; i++) {
		sprintf(str_ptr, "%02x", *uuid_ptr);
		str_ptr += 2;
		uuid_ptr++;
	}
	*str_ptr++ = '-';
	for (i = 0; i < 2; i++) {
		sprintf(str_ptr, "%02x", *uuid_ptr);
		str_ptr += 2;
		uuid_ptr++;
	}
	*str_ptr++ = '-';
	for (i = 0; i < 8; i++) {
		sprintf(str_ptr, "%02x", *uuid_ptr);
		str_ptr += 2;
		uuid_ptr++;
	}
}

int check_keybox(const char *keybox, u32 size)
{
	const struct keybox_header *hdr = (const struct keybox_header *)keybox;
	u32 hdr_cxt_size = sizeof(struct keybox_header) + sizeof(struct encryption_context);

	if (hdr->magic != KEYBOX_HDR_MAGIC) {
		LOGE("keybox header magic error(expected magic: 0x%08X; wrong magic: 0x%08X)\n",
			KEYBOX_HDR_MAGIC, hdr->magic);
		return CMD_RET_KEYBOX_BAD_FORMAT;
	}
	if (hdr->version < KEYBOX_HDR_MIN_VERSION) {
		LOGE("keybox header version error(min version: %d; wrong version: %d)\n",
			KEYBOX_HDR_MIN_VERSION, hdr->version);
		return CMD_RET_KEYBOX_BAD_FORMAT;
	}
	if (size > MAX_SIZE_KEYBOX || size <= hdr_cxt_size
			|| hdr->key_size != size - hdr_cxt_size) {
		LOGE("keybox length error\n");
		return CMD_RET_KEYBOX_BAD_FORMAT;
	}
	if (hdr->provision_type != PROVISION_TYPE_FACTORY) {
		LOGE("keybox provision type error(expected type: %d; wrong type: %d)\n",
			PROVISION_TYPE_FACTORY, hdr->provision_type);
		return CMD_RET_KEYBOX_BAD_FORMAT;
	}

	return CMD_RET_SUCCESS;
}

void calc_sha256(const char *data, u32 data_size, char *sha256, u32 size)
{
	sha256_csum_wd((const unsigned char *)data, data_size, (unsigned char *)sha256, size);
}

void get_medium_name(u32 medium_type, char medium_name[MAX_SIZE_MEDIUM_NAME])
{
	memset(medium_name, 0, MAX_SIZE_MEDIUM_NAME);
	switch (medium_type) {
	case BOOT_EMMC:
		strcpy(medium_name, "EMMC");
		break;
	case BOOT_SD:
		strcpy(medium_name, "SD");
		break;
	case BOOT_NAND_NFTL:
		strcpy(medium_name, "NAND_NFTL");
		break;
	case BOOT_NAND_MTD:
		strcpy(medium_name, "NAND_MTD");
		break;
	case BOOT_SNAND:
		strcpy(medium_name, "SNAND");
		break;
	case BOOT_SNOR:
		strcpy(medium_name, "SNOR");
		break;
	default:
		strcpy(medium_name, "Unknown Storage Medium");
		break;
	}
}

int is_storage_medium_supported(void)
{
	int ret = CMD_RET_DEVICE_NOT_AVAILABLE;
	int medium_type = store_get_type();
	char medium_name[MAX_SIZE_MEDIUM_NAME] = { 0 };
#ifdef CONFIG_NAND_FACTORY_PROVISION
	u32 supported_types[] = {
		BOOT_EMMC, BOOT_NAND_NFTL, BOOT_NAND_MTD, BOOT_SNAND
	};
#else
	u32 supported_types[] = {
		BOOT_EMMC
	};
#endif
	int i = 0;

	for (; i < sizeof(supported_types) / sizeof(u32); i++) {
		if (medium_type == supported_types[i]) {
			ret = CMD_RET_SUCCESS;
			break;
		}
	}

	get_medium_name(medium_type, medium_name);
	if (ret != CMD_RET_SUCCESS)
		LOGE("Provision function is not supported on '%s' storage medium\n", medium_name);
	else
		LOGI("Being '%s' storage medium\n", medium_name);

	return ret;
}

int preprocess_keybox(char *keybox, u32 size)
{
	int ret = CMD_RET_SUCCESS;
	struct encryption_context *enc_cxt = (struct encryption_context *)
		(keybox + sizeof(struct keybox_header));
	u32 epek_size = sizeof(enc_cxt->epek);
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	struct tee_param param[TEE_PARAM_NUM] = { 0 };
	const struct tee_optee_ta_uuid uuid = PROVISION_PTA_UUID;

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		LOGE("tee find device failed\n");
		return CMD_RET_DEVICE_NOT_AVAILABLE;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		LOGE("open session failed, ret = 0x%x\n", ret);
		return CMD_RET_BAD_PARAMETER;
	}

	if (open_arg.ret) {
		LOGE("open session failed, ret = 0x%x, ret_origin = 0x%x\n",
			open_arg.ret, open_arg.ret_origin);
		return CMD_RET_BAD_PARAMETER;
	}

	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.size = sizeof(enc_cxt->iv);
	ret = tee_shm_alloc(dev, sizeof(enc_cxt->iv), 0, &param[0].u.memref.shm);
	if (ret) {
		LOGE("tee shm alloc for iv failed, ret = 0x%x\n", ret);
		ret = CMD_RET_DEVICE_NO_SPACE;
		goto exit;
	}

	memcpy(param[0].u.memref.shm->addr, enc_cxt->iv, sizeof(enc_cxt->iv));

	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[1].u.memref.size = epek_size;
	ret = tee_shm_alloc(dev, epek_size, 0, &param[1].u.memref.shm);
	if (ret) {
		LOGE("tee shm alloc for epek failed, ret = 0x%x\n", ret);
		ret = CMD_RET_DEVICE_NO_SPACE;
		goto exit;
	}

	memcpy(param[1].u.memref.shm->addr, enc_cxt->epek, epek_size);

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = PROVISION_PTA_CMD_EPEK_ENCRYPT;

	ret = tee_invoke_func(dev, &invoke_arg, TEE_PARAM_NUM, param);
	if (ret) {
		LOGE("invoke failed, cmd = 0x%x, ret = 0x%x\n", invoke_arg.func, ret);
		ret = CMD_RET_SMC_CALL_FAILED;
		goto exit;
	}

	if (invoke_arg.ret) {
		LOGE("invoke failed, cmd = 0x%x, ret = 0x%x, origin = 0x%x\n",
			invoke_arg.func, invoke_arg.ret, invoke_arg.ret_origin);
		ret = CMD_RET_SMC_CALL_FAILED;
		goto exit;
	}

	memcpy(enc_cxt->epek, param[1].u.memref.shm->addr, param[1].u.memref.size);

exit:
	if (param[0].u.memref.shm)
		tee_shm_free(param[0].u.memref.shm);

	if (param[1].u.memref.shm)
		tee_shm_free(param[1].u.memref.shm);

	tee_close_session(dev, open_arg.session);

	return ret;
}
