// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#ifndef __FACTORY_PROVISION_UTILS_H__
#define __FACTORY_PROVISION_UTILS_H__

#include <stdlib.h>
#include <common.h>

#define CMD_DEBUG         (0)
#define CMD_LOG_TAG       "[FACTORY-PROVISION] "

#if CMD_DEBUG
#define LOGD(fmt, ...)    printf("%s"fmt, CMD_LOG_TAG, ##__VA_ARGS__)
#else
#define LOGD(fmt, ...)
#endif

#define LOGE(fmt, ...)    printf("%sERROR: "fmt, CMD_LOG_TAG, ##__VA_ARGS__)
#define LOGI(fmt, ...)    printf("%s"fmt, CMD_LOG_TAG, ##__VA_ARGS__)

#ifndef getenv
#define getenv env_get
#endif

#define SIZE_1K                 (1024)
#define SIZE_1M                 (SIZE_1K * SIZE_1K)
#define SIZE_TAG                (16)
#define SIZE_IV                 (16)
#define SIZE_AES_KEY            (16)
#define SIZE_HMAC_KEY           (32)
#define SIZE_HMAC_DIGEST        (32)
#define SIZE_BLOCK              (512)

#define MAX_SIZE_CMD            (256)
#define MAX_SIZE_FILE_PATH      (256)
#define MAX_SIZE_KEYBOX_NAME    (256)
#define MAX_SIZE_PART_NAME      (32)
#define MAX_SIZE_MEDIUM_NAME    (64)
#define MAX_SIZE_KEYBOX         (SIZE_1K * 16)

#define MAX_CNT_FS_VALUE        (256)

#define KEYBOX_HDR_MAGIC        (0x6B626F78) //"kbox"
#define KEYBOX_HDR_MIN_VERSION  (5)

#define PROVISION_TYPE_FACTORY  (0)

#define ACTION_UNKNOWN          (0x00)
#define ACTION_INIT             (0x01)
#define ACTION_WRITE            (0x02)
#define ACTION_QUERY            (0x03)
#define ACTION_REMOVE           (0x04)
#define ACTION_CLEAR            (0x05)
#define ACTION_LIST             (0x06)
#define ACTION_VERSION          (0x07)

#define DEV_NAME                "mmc"
#define DEV_NO                  (1)
#define PART_TYPE               "user"
#define PART_NAME_RSV           "rsv"
#define PART_NAME_FTY           "factory"
#define NAND_FTY_MOUNT_PT       "mnt"

#define FACTORY_PROVISION_VERSION                      "1.1"

#define TEE_PARAM_NUM                                  (4)

#define PROVISION_PTA_UUID \
		{ 0x24b17b16, 0x89d1, 0x43a3, \
		{ 0x95, 0xed, 0x19, 0x3c, 0xe1, 0xed, 0xa7, 0x15 } }

#define PROVISION_PTA_CMD_EPEK_ENCRYPT                 (0)

#define CMD_RET_SUCCESS                                0x00000000
#define CMD_RET_KEYBOX_NOT_EXIST                       0x00000001
#define CMD_RET_KEYBOX_TOO_LARGE                       0x00000002
#define CMD_RET_KEYBOX_NAME_TOO_LONG                   0x00000003
#define CMD_RET_KEYBOX_BAD_FORMAT                      0x00000004
#define CMD_RET_DEVICE_NO_SPACE                        0x00000011
#define CMD_RET_DEVICE_NOT_AVAILABLE                   0x00000012
#define CMD_RET_BAD_PARAMETER                          0x00000021
#define CMD_RET_SMC_CALL_FAILED                        0x00000031
#define CMD_RET_UNKNOWN_ERROR                          0x0000FFFF

struct keybox_header {
	u32 magic;
	u32 version;
	u32 key_type;
	u32 key_size;
	u32 provision_type;
	u32 ta_uuid[4];
	u32 reserved[4];
};

struct encryption_context {
	char iv[SIZE_IV];
	char tag[SIZE_TAG];
	char epek[SIZE_AES_KEY];
	char rsv[64];
};

struct input_param {
	u32 action;
	const char *keybox_name;
	u32 keybox_phy_addr;
	u32 keybox_size;
	u32 ret_data_addr;
};

struct fs_value {
	int idx;
	u32 value;
};

void usage(void);

void show_version(void);

void parse_params(int argc, char * const argv[], struct input_param *params);

int check_params(const struct input_param *params);

void convert_to_uuid_str(const char uuid[16], char uuid_str[40]);

void calc_sha256(const char *data, u32 data_size, char *sha256, u32 size);

void get_medium_name(u32 medium_type, char medium_name[64]);

int is_storage_medium_supported(void);

int check_keybox(const char *keybox, u32 size);

int preprocess_keybox(char *keybox, u32 size);

#endif
