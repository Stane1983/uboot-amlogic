// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <common.h>
#include <fs.h>
#include <fat.h>
#include <mapmem.h>
#include <u-boot/sha256.h>
#include <amlogic/storage.h>
#include <amlogic/emmc_partitions.h>
#include "factory_provision_utils.h"
#include "emmc_factory_provision.h"

static char g_keybox[MAX_SIZE_KEYBOX] = { 0 };
static char g_fs_data[SIZE_1K] = { 0 };
static char g_part_name[MAX_SIZE_PART_NAME] = { 0 };

static struct fs_value g_fs_vals_rsv[MAX_CNT_FS_VALUE] = {
	{ 0, 0xEB }, { 1, 0x3C }, { 2, 0x90 }, { 3, 0x6D },
	{ 4, 0x6B }, { 5, 0x66 }, { 6, 0x73 }, { 7, 0x2E },
	{ 8, 0x66 }, { 9, 0x61 }, { 10, 0x74 }, { 12, 0x02 },
	{ 13, 0x10 }, { 14, 0x01 }, { 16, 0x02 }, { 18, 0x02 },
	{ 20, 0x80 }, { 21, 0xF8 }, { 22, 0x10 }, { 24, 0x20 },
	{ 26, 0x40 }, { 36, 0x80 }, { 38, 0x29 }, { 39, 0x13 },
	{ 40, 0x4C }, { 41, 0x98 }, { 42, 0x6A }, { 43, 0x4B },
	{ 44, 0x45 }, { 45, 0x59 }, { 46, 0x42 }, { 47, 0x4F },
	{ 48, 0x58 }, { 49, 0x20 }, { 50, 0x50 }, { 51, 0x41 },
	{ 52, 0x52 }, { 53, 0x54 }, { 54, 0x46 }, { 55, 0x41 },
	{ 56, 0x54 }, { 57, 0x31 }, { 58, 0x32 }, { 59, 0x20 },
	{ 60, 0x20 }, { 61, 0x20 }, { 62, 0x0E }, { 63, 0x1F },
	{ 64, 0xBE }, { 65, 0x5B }, { 66, 0x7C }, { 67, 0xAC },
	{ 68, 0x22 }, { 69, 0xC0 }, { 70, 0x74 }, { 71, 0x0B },
	{ 72, 0x56 }, { 73, 0xB4 }, { 74, 0x0E }, { 75, 0xBB },
	{ 76, 0x07 }, { 78, 0xCD }, { 79, 0x10 }, { 80, 0x5E },
	{ 81, 0xEB }, { 82, 0xF0 }, { 83, 0x32 }, { 84, 0xE4 },
	{ 85, 0xCD }, { 86, 0x16 }, { 87, 0xCD }, { 88, 0x19 },
	{ 89, 0xEB }, { 90, 0xFE }, { 91, 0x54 }, { 92, 0x68 },
	{ 93, 0x69 }, { 94, 0x73 }, { 95, 0x20 }, { 96, 0x69 },
	{ 97, 0x73 }, { 98, 0x20 }, { 99, 0x6E }, { 100, 0x6F },
	{ 101, 0x74 }, { 102, 0x20 }, { 103, 0x61 }, { 104, 0x20 },
	{ 105, 0x62 }, { 106, 0x6F }, { 107, 0x6F }, { 108, 0x74 },
	{ 109, 0x61 }, { 110, 0x62 }, { 111, 0x6C }, { 112, 0x65 },
	{ 113, 0x20 }, { 114, 0x64 }, { 115, 0x69 }, { 116, 0x73 },
	{ 117, 0x6B }, { 118, 0x2E }, { 119, 0x20 }, { 120, 0x20 },
	{ 121, 0x50 }, { 122, 0x6C }, { 123, 0x65 }, { 124, 0x61 },
	{ 125, 0x73 }, { 126, 0x65 }, { 127, 0x20 }, { 128, 0x69 },
	{ 129, 0x6E }, { 130, 0x73 }, { 131, 0x65 }, { 132, 0x72 },
	{ 133, 0x74 }, { 134, 0x20 }, { 135, 0x61 }, { 136, 0x20 },
	{ 137, 0x62 }, { 138, 0x6F }, { 139, 0x6F }, { 140, 0x74 },
	{ 141, 0x61 }, { 142, 0x62 }, { 143, 0x6C }, { 144, 0x65 },
	{ 145, 0x20 }, { 146, 0x66 }, { 147, 0x6C }, { 148, 0x6F },
	{ 149, 0x70 }, { 150, 0x70 }, { 151, 0x79 }, { 152, 0x20 },
	{ 153, 0x61 }, { 154, 0x6E }, { 155, 0x64 }, { 156, 0x0D },
	{ 157, 0x0A }, { 158, 0x70 }, { 159, 0x72 }, { 160, 0x65 },
	{ 161, 0x73 }, { 162, 0x73 }, { 163, 0x20 }, { 164, 0x61 },
	{ 165, 0x6E }, { 166, 0x79 }, { 167, 0x20 }, { 168, 0x6B },
	{ 169, 0x65 }, { 170, 0x79 }, { 171, 0x20 }, { 172, 0x74 },
	{ 173, 0x6F }, { 174, 0x20 }, { 175, 0x74 }, { 176, 0x72 },
	{ 177, 0x79 }, { 178, 0x20 }, { 179, 0x61 }, { 180, 0x67 },
	{ 181, 0x61 }, { 182, 0x69 }, { 183, 0x6E }, { 184, 0x20 },
	{ 185, 0x2E }, { 186, 0x2E }, { 187, 0x2E }, { 188, 0x20 },
	{ 189, 0x0D }, { 190, 0x0A }, { 510, 0x55 }, { 511, 0xAA },
	{ 512, 0xF8 }, { 513, 0xFF }, { 514, 0xFF }, { 8704, 0xF8 },
	{ 8705, 0xFF }, { 8706, 0xFF }, { 16896, 0x4B }, { 16897, 0x45 },
	{ 16898, 0x59 }, { 16899, 0x42 }, { 16900, 0x4F }, { 16901, 0x58 },
	{ 16902, 0x20 }, { 16903, 0x50 }, { 16904, 0x41 }, { 16905, 0x52 },
	{ 16906, 0x54 }, { 16907, 0x08 }, { 16910, 0x1C }, { 16911, 0x78 },
	{ 16912, 0x44 }, { 16913, 0x50 }, { 16914, 0x44 }, { 16915, 0x50 },
	{ 16918, 0x1C }, { 16919, 0x78 }, { 16920, 0x44 }, { 16921, 0x50 },
	{ 0, 0 },
};

static struct fs_value g_fs_vals_fty[MAX_CNT_FS_VALUE] = {
	{ 0, 0xEB }, { 1, 0x3C }, { 2, 0x90 }, { 3, 0x6D },
	{ 4, 0x6B }, { 5, 0x66 }, { 6, 0x73 }, { 7, 0x2E },
	{ 8, 0x66 }, { 9, 0x61 }, { 10, 0x74 }, { 12, 0x02 },
	{ 13, 0x08 }, { 14, 0x01 }, { 16, 0x02 }, { 18, 0x02 },
	{ 20, 0x40 }, { 21, 0xF8 }, { 22, 0x08 }, { 24, 0x20 },
	{ 26, 0x40 }, { 36, 0x80 }, { 38, 0x29 }, { 39, 0xDE },
	{ 40, 0x62 }, { 41, 0xA6 }, { 42, 0xD0 }, { 43, 0x4B },
	{ 44, 0x45 }, { 45, 0x59 }, { 46, 0x42 }, { 47, 0x4F },
	{ 48, 0x58 }, { 49, 0x20 }, { 50, 0x50 }, { 51, 0x41 },
	{ 52, 0x52 }, { 53, 0x54 }, { 54, 0x46 }, { 55, 0x41 },
	{ 56, 0x54 }, { 57, 0x31 }, { 58, 0x32 }, { 59, 0x20 },
	{ 60, 0x20 }, { 61, 0x20 }, { 62, 0x0E }, { 63, 0x1F },
	{ 64, 0xBE }, { 65, 0x5B }, { 66, 0x7C }, { 67, 0xAC },
	{ 68, 0x22 }, { 69, 0xC0 }, { 70, 0x74 }, { 71, 0x0B },
	{ 72, 0x56 }, { 73, 0xB4 }, { 74, 0x0E }, { 75, 0xBB },
	{ 76, 0x07 }, { 78, 0xCD }, { 79, 0x10 }, { 80, 0x5E },
	{ 81, 0xEB }, { 82, 0xF0 }, { 83, 0x32 }, { 84, 0xE4 },
	{ 85, 0xCD }, { 86, 0x16 }, { 87, 0xCD }, { 88, 0x19 },
	{ 89, 0xEB }, { 90, 0xFE }, { 91, 0x54 }, { 92, 0x68 },
	{ 93, 0x69 }, { 94, 0x73 }, { 95, 0x20 }, { 96, 0x69 },
	{ 97, 0x73 }, { 98, 0x20 }, { 99, 0x6E }, { 100, 0x6F },
	{ 101, 0x74 }, { 102, 0x20 }, { 103, 0x61 }, { 104, 0x20 },
	{ 105, 0x62 }, { 106, 0x6F }, { 107, 0x6F }, { 108, 0x74 },
	{ 109, 0x61 }, { 110, 0x62 }, { 111, 0x6C }, { 112, 0x65 },
	{ 113, 0x20 }, { 114, 0x64 }, { 115, 0x69 }, { 116, 0x73 },
	{ 117, 0x6B }, { 118, 0x2E }, { 119, 0x20 }, { 120, 0x20 },
	{ 121, 0x50 }, { 122, 0x6C }, { 123, 0x65 }, { 124, 0x61 },
	{ 125, 0x73 }, { 126, 0x65 }, { 127, 0x20 }, { 128, 0x69 },
	{ 129, 0x6E }, { 130, 0x73 }, { 131, 0x65 }, { 132, 0x72 },
	{ 133, 0x74 }, { 134, 0x20 }, { 135, 0x61 }, { 136, 0x20 },
	{ 137, 0x62 }, { 138, 0x6F }, { 139, 0x6F }, { 140, 0x74 },
	{ 141, 0x61 }, { 142, 0x62 }, { 143, 0x6C }, { 144, 0x65 },
	{ 145, 0x20 }, { 146, 0x66 }, { 147, 0x6C }, { 148, 0x6F },
	{ 149, 0x70 }, { 150, 0x70 }, { 151, 0x79 }, { 152, 0x20 },
	{ 153, 0x61 }, { 154, 0x6E }, { 155, 0x64 }, { 156, 0x0D },
	{ 157, 0x0A }, { 158, 0x70 }, { 159, 0x72 }, { 160, 0x65 },
	{ 161, 0x73 }, { 162, 0x73 }, { 163, 0x20 }, { 164, 0x61 },
	{ 165, 0x6E }, { 166, 0x79 }, { 167, 0x20 }, { 168, 0x6B },
	{ 169, 0x65 }, { 170, 0x79 }, { 171, 0x20 }, { 172, 0x74 },
	{ 173, 0x6F }, { 174, 0x20 }, { 175, 0x74 }, { 176, 0x72 },
	{ 177, 0x79 }, { 178, 0x20 }, { 179, 0x61 }, { 180, 0x67 },
	{ 181, 0x61 }, { 182, 0x69 }, { 183, 0x6E }, { 184, 0x20 },
	{ 185, 0x2E }, { 186, 0x2E }, { 187, 0x2E }, { 188, 0x20 },
	{ 189, 0x0D }, { 190, 0x0A }, { 510, 0x55 }, { 511, 0xAA },
	{ 512, 0xF8 }, { 513, 0xFF }, { 514, 0xFF }, { 4608, 0xF8 },
	{ 4609, 0xFF }, { 4610, 0xFF }, { 8704, 0x4B }, { 8705, 0x45 },
	{ 8706, 0x59 }, { 8707, 0x42 }, { 8708, 0x4F }, { 8709, 0x58 },
	{ 8710, 0x20 }, { 8711, 0x50 }, { 8712, 0x41 }, { 8713, 0x52 },
	{ 8714, 0x54 }, { 8715, 0x08 }, { 8718, 0x49 }, { 8719, 0xA2 },
	{ 8720, 0x4C }, { 8721, 0x50 }, { 8722, 0x4C }, { 8723, 0x50 },
	{ 8726, 0x49 }, { 8727, 0xA2 }, { 8728, 0x4C }, { 8729, 0x50 },
	{ 0, 0 },
};

static const char *get_valid_part_name(void)
{
	int part_num = get_partition_num_by_name(PART_NAME_FTY);

	memset(g_part_name, 0, sizeof(g_part_name));
	if (part_num >= 0)
		strcpy(g_part_name, PART_NAME_FTY);
	else
		strcpy(g_part_name, PART_NAME_RSV);

	return g_part_name;
}

static const struct fs_value *get_fs_values(void)
{
	int part_num = get_partition_num_by_name(PART_NAME_FTY);

	if (part_num >= 0)
		return g_fs_vals_fty;
	else
		return g_fs_vals_rsv;
}

static int make_fat_fs(void)
{
	char cmd[MAX_SIZE_CMD] = { 0 };
	int i = 0;
	const char *part_name = get_valid_part_name();
	const struct fs_value *fs_vals = get_fs_values();
	u32 data_phy_addr = (u32)virt_to_phys(g_fs_data);
	u32 offset = 0;

	/* erase partition */
	sprintf(cmd, "mmc dev %d;amlmmc switch %d %s;amlmmc erase %s;",
		DEV_NO, DEV_NO, PART_TYPE, part_name);
	if (run_command(cmd, 0)) {
		LOGE("command[%s] failed\n", cmd);
		return CMD_RET_UNKNOWN_ERROR;
	}

	/* write fat file system */
	memset(g_fs_data, 0, SIZE_1K);
	for (i = 0; i < MAX_CNT_FS_VALUE && fs_vals[i].value; i++) {
		if (i > 0 && (fs_vals[i].idx / SIZE_1K != fs_vals[i - 1].idx / SIZE_1K)) {
			memset(cmd, 0, sizeof(cmd));
			offset = fs_vals[i - 1].idx / SIZE_1K * SIZE_1K;
			sprintf(cmd, "amlmmc write %s 0x%08X 0x%X 0x%X;",
				part_name, data_phy_addr, offset, SIZE_1K);
			if (run_command(cmd, 0)) {
				LOGE("command[%s] failed\n", cmd);
				return CMD_RET_UNKNOWN_ERROR;
			}
		}
		g_fs_data[fs_vals[i].idx % SIZE_1K] = fs_vals[i].value;
	}
	memset(cmd, 0, sizeof(cmd));
	offset = fs_vals[i - 1].idx / SIZE_1K * SIZE_1K;
	sprintf(cmd, "amlmmc write %s 0x%08X 0x%X 0x%X;",
		part_name, data_phy_addr, offset, SIZE_1K);
	if (run_command(cmd, 0)) {
		LOGE("command[%s] failed\n", cmd);
		return CMD_RET_UNKNOWN_ERROR;
	}

	return CMD_RET_SUCCESS;
}

static int dev_writable(void)
{
	char cmd[MAX_SIZE_CMD] = { 0 };

	sprintf(cmd, "fatinfo %s 0x%X:0x%X", DEV_NAME, DEV_NO,
		get_partition_num_by_name((char *)get_valid_part_name()));
	if (run_command(cmd, 0)) {
		LOGD("command[%s] failed\n", cmd);
		return CMD_RET_DEVICE_NOT_AVAILABLE;
	}

	return CMD_RET_SUCCESS;
}

static int dev_write(const char *name, const char *data, u32 size)
{
	char cmd[MAX_SIZE_CMD] = { 0 };

	sprintf(cmd, "fatwrite %s 0x%X:0x%X 0x%08X %s 0x%X", DEV_NAME, DEV_NO,
		get_partition_num_by_name((char *)get_valid_part_name()),
		(u32)virt_to_phys((void *)data), name, size);
	if (run_command(cmd, 0)) {
		LOGD("command[%s] failed\n", cmd);
		return CMD_RET_DEVICE_NOT_AVAILABLE;
	}

	return CMD_RET_SUCCESS;
}

static int init_partition(void)
{
	int ret = CMD_RET_SUCCESS;
	char dev_part_str[16] = { 0 };

	ret = dev_writable();
	if (ret) {
		ret = make_fat_fs();
		if (ret) {
			LOGE("make fat file system failed\n");
			return ret;
		}
		ret = dev_writable();
		if (ret) {
			LOGE("device not available\n");
			return ret;
		}
	}

	sprintf(dev_part_str, "0x%X:0x%X", DEV_NO,
		get_partition_num_by_name((char *)get_valid_part_name()));
	if (fs_set_blk_dev(DEV_NAME, dev_part_str, FS_TYPE_FAT)) {
		LOGE("set block device failed\n");
		ret = CMD_RET_UNKNOWN_ERROR;
	}

	return ret;
}

static int verify_written_keybox(const char *keybox_name, const char *keybox, u32 keybox_size)
{
	char sha256[SHA256_SUM_LEN] = { 0 };
	char written_sha256[SHA256_SUM_LEN] = { 0 };
	loff_t act_read = 0;

	calc_sha256(keybox, keybox_size, sha256, SHA256_SUM_LEN);

	memset(g_keybox, 0, sizeof(g_keybox));
	if (fat_read_file(keybox_name, g_keybox, 0, MAX_SIZE_KEYBOX, &act_read))
		return CMD_RET_UNKNOWN_ERROR;
	if (keybox_size != act_read)
		return CMD_RET_UNKNOWN_ERROR;

	calc_sha256(g_keybox, act_read, written_sha256, SHA256_SUM_LEN);

	if (memcmp(sha256, written_sha256, SHA256_SUM_LEN))
		return CMD_RET_UNKNOWN_ERROR;

	return CMD_RET_SUCCESS;
}

static int append_keybox(const char *keybox_name, const char *keybox, u32 keybox_size)
{
	int ret = CMD_RET_SUCCESS;

	ret = init_partition();
	if (ret)
		return ret;

	ret = dev_write(keybox_name, keybox, keybox_size);
	if (ret)
		return ret;

	ret = verify_written_keybox(keybox_name, keybox, keybox_size);
	if (ret) {
		LOGE("verify written keybox '%s' failed\n", keybox_name);
		return ret;
	}

	LOGI("write keybox '%s' success\n", keybox_name);

	return ret;
}

static int get_keybox_size(const char *keybox_name, u32 *size)
{
	loff_t keybox_size = 0;

	if (fat_size(keybox_name, &keybox_size)) {
		LOGE("get keybox '%s' size failed\n", keybox_name);
		return CMD_RET_UNKNOWN_ERROR;
	}

	*size = (u32)keybox_size;
	LOGI("keybox '%s' size is %d\n", keybox_name, *size);
	return CMD_RET_SUCCESS;
}

static int query_keybox(const char *keybox_name, char *ret_data)
{
	int ret = CMD_RET_SUCCESS;

	ret = init_partition();
	if (ret)
		return ret;

	if (!fat_exists(keybox_name)) {
		LOGI("keybox '%s' not exists\n", keybox_name);
		return CMD_RET_KEYBOX_NOT_EXIST;
	}

	return get_keybox_size(keybox_name, (u32 *)ret_data);
}

static int remove_keybox(const char *keybox_name)
{
	int ret = init_partition();

	if (ret)
		return ret;

	if (fs_unlink(keybox_name)) {
		LOGE("remove '%s' failed\n", keybox_name);
		return CMD_RET_UNKNOWN_ERROR;
	}

	LOGI("remove '%s' success\n", keybox_name);
	return CMD_RET_SUCCESS;
}

static int remove_all_keyboxes(void)
{
	int ret = CMD_RET_SUCCESS;
	struct fs_dir_stream *dirs = NULL;
	struct fs_dirent *dent = NULL;
	char cmd[MAX_SIZE_CMD] = { 0 };

	ret = init_partition();
	if (ret)
		return ret;

	if (fat_opendir("/", &dirs)) {
		LOGE("open '/' failed\n");
		return CMD_RET_UNKNOWN_ERROR;
	}

	while (!fat_readdir(dirs, &dent)) {
		if (dent->type != FS_DT_REG) // not regular file
			continue;

		sprintf(cmd, "fatrm %s 0x%X:0x%X %s", DEV_NAME, DEV_NO,
			get_partition_num_by_name((char *)get_valid_part_name()),
			dent->name);
		if (run_command(cmd, 0)) {
			ret = CMD_RET_UNKNOWN_ERROR;
			LOGE("remove '%s' failed\n", dent->name);
		} else {
			LOGI("remove '%s' success\n", dent->name);
		}
	}

	fat_closedir(dirs);
	return ret;
}

static int list_all_keyboxes(void)
{
	int ret = CMD_RET_SUCCESS;
	char cmd[MAX_SIZE_CMD] = { 0 };

	ret = init_partition();
	if (ret)
		return ret;

	sprintf(cmd, "fatls %s 0x%X:0x%X", DEV_NAME, DEV_NO,
		get_partition_num_by_name((char *)get_valid_part_name()));
	if (run_command(cmd, 0)) {
		LOGE("command[%s] failed\n", cmd);
		ret = CMD_RET_UNKNOWN_ERROR;
	}

	return ret;
}

static int remove_same_type_keybox(u32 key_type, const char *uuid)
{
	int ret = CMD_RET_SUCCESS;
	struct fs_dir_stream *dirs = NULL;
	struct fs_dirent *dent = NULL;
	struct keybox_header hdr = { 0 };
	loff_t act_read = 0;
	char uuid_str[40] = { 0 };

	ret = init_partition();
	if (ret)
		return ret;

	if (fat_opendir("/", &dirs)) {
		LOGE("open '/' failed\n");
		return CMD_RET_UNKNOWN_ERROR;
	}

	convert_to_uuid_str(uuid, uuid_str);
	while (!fat_readdir(dirs, &dent)) {
		if (dent->type != FS_DT_REG) // not regular file
			continue;

		memset(&hdr, 0, sizeof(hdr));
		if (fat_read_file(dent->name, &hdr, 0, sizeof(hdr), &act_read) || act_read != sizeof(hdr)) {
			LOGE("read keybox '%s' failed\n", dent->name);
			ret = CMD_RET_UNKNOWN_ERROR;
			goto exit;
		}

		if (!memcmp(uuid, hdr.ta_uuid, sizeof(hdr.ta_uuid)) && key_type == hdr.key_type) {
			ret = remove_keybox(dent->name);
			if (ret != CMD_RET_SUCCESS) {
				LOGE("remove the same type(uuid = %s, key_type = 0x%02X) keybox '%s' failed\n",
					uuid_str, key_type, dent->name);
				goto exit;
			}
		}
	}

exit:
	fat_closedir(dirs);
	return ret;
}

int emmc_factory_provision(const struct input_param *params)
{
	int ret = CMD_RET_SUCCESS;
	char *in_kb = NULL;
	char *ret_data = NULL;
	const struct keybox_header *hdr = NULL;

	switch (params->action) {
	case ACTION_INIT:
		ret = init_partition();
		break;
	case ACTION_WRITE:
		in_kb = map_sysmem(params->keybox_phy_addr, 0);

		ret = check_keybox(in_kb, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		hdr = (const struct keybox_header *)in_kb;
		ret = remove_same_type_keybox(hdr->key_type, (const char *)hdr->ta_uuid);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		memcpy(g_keybox, in_kb, params->keybox_size);

		ret = preprocess_keybox(g_keybox, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		ret = append_keybox(params->keybox_name, g_keybox, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_QUERY:
		ret_data = map_sysmem(params->ret_data_addr, 0);
		ret = query_keybox(params->keybox_name, ret_data);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_REMOVE:
		ret = remove_keybox(params->keybox_name);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_CLEAR:
		ret = remove_all_keyboxes();
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_LIST:
		ret = list_all_keyboxes();
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_VERSION:
		show_version();
		break;
	default:
		break;
	}

exit:
	if (in_kb)
		unmap_sysmem(in_kb);
	if (ret_data)
		unmap_sysmem(ret_data);
	if (ret == CMD_RET_BAD_PARAMETER)
		usage();
	return ret;
}
