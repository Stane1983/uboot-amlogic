// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <ext4fs.h>
#include <fs.h>
#include <amlogic/partition_table.h>
#include <amlogic/emmc_partitions.h>
#include "model_log.h"
#include "model_size.h"
#include "model.h"

#define LOG_TAG     "model_fs"
#define LOG_NDEBUG  0

static char local_file_path[MODEL_FILE_PATH_LEN_MAX];
static char local_part_name[MODEL_FILE_PATH_LEN_MAX];
static char local_file_name[MODEL_FILE_PATH_LEN_MAX];

static int split_file_path(const char *file_path)
{
	char *tmp_start_ptr = NULL;
	char *tmp_end_ptr = NULL;
	char *slot_name;
	int i = 0;
	char tmp[MODEL_FILE_PATH_LEN_MAX];

	if (!file_path) {
		ALOGE("%s: file_path is NULL!\n", __func__);
		goto split_file_path_err;
	}

	memset((void *)local_file_path, 0, MODEL_FILE_PATH_LEN_MAX);
	memset((void *)local_part_name, 0, MODEL_FILE_PATH_LEN_MAX);
	memset((void *)local_file_name, 0, MODEL_FILE_PATH_LEN_MAX);

	tmp_start_ptr = strchr(file_path, '/');
	if (tmp_start_ptr != file_path) {
		ALOGE("%s: not abstract file_path: %s\n", __func__, file_path);
		goto split_file_path_err;
	}

	tmp_end_ptr = strchr(tmp_start_ptr + 1, '/');
	if (!tmp_end_ptr) {
		ALOGE("%s: no file_name in file_path: %s\n", __func__, file_path);
		goto split_file_path_err;
	}

	strncpy(local_part_name, tmp_start_ptr + 1, tmp_end_ptr - tmp_start_ptr - 1);
	local_part_name[tmp_end_ptr - tmp_start_ptr - 1] = '\0';
	if (has_boot_slot == 1) {
		slot_name = env_get("slot-suffixes");
		if (!slot_name) {
			run_command("get_valid_slot", 0);
			slot_name = env_get("slot-suffixes");
		}
		strlcpy(tmp, local_part_name, MODEL_FILE_PATH_LEN_MAX);
		if (strcmp(slot_name, "0") == 0)
			strcat(local_part_name, "_a");
		else if (strcmp(slot_name, "1") == 0)
			strcat(local_part_name, "_b");
		if (get_partition_num_by_name(local_part_name) <= 0) {
			ALOGE("local_part_name %s not exist, replace to %s\n", local_part_name, tmp);
			strlcpy(local_part_name, tmp, MODEL_FILE_PATH_LEN_MAX);
		}
	}

	tmp_start_ptr = tmp_end_ptr;

	i = 0;
	while (*tmp_end_ptr) {
		if (i >= MODEL_FILE_PATH_LEN_MAX - 1) {
			ALOGE("%s: file_path is too long (%d): %s\n", __func__, i, file_path);
			goto split_file_path_err;
		}
		tmp_end_ptr++;
		i++;
	}

	strncpy(local_file_name, tmp_start_ptr, i);
	local_file_name[i] = '\0';

	strlcpy(local_file_path, file_path, MODEL_FILE_PATH_LEN_MAX);

	ALOGD("%s: partition name: %s, file_name: %s\n",
		__func__, local_part_name, local_file_name);

	return 0;

split_file_path_err:
	memset((void *)local_part_name, 0, MODEL_FILE_PATH_LEN_MAX);
	memset((void *)local_file_name, 0, MODEL_FILE_PATH_LEN_MAX);
	return -1;
}

#define CS_BLOCK_DEV_INTERFACE    "mmc"
#define CS_BLOCK_DEV_MARJOR_NUM   "1"

static int set_block_device(const char *part_name)
{
	int part_no = 0;
	char part_buf[128] = {0};
	char tmp_buf[128] = {0};

	part_no = get_partition_num_by_name(part_name);
	//ALOGD("%s: part_no: %d\n", __func__, part_no);
	if (part_no >= 0) {
		strcpy(part_buf, CS_BLOCK_DEV_MARJOR_NUM);
		strcat(part_buf, ":");

		sprintf(tmp_buf, "%x", part_no);
		strcat(part_buf, tmp_buf);

		return fs_set_blk_dev(CS_BLOCK_DEV_INTERFACE, part_buf, FS_TYPE_EXT);
	}

	ALOGE("%s: partition num error: %s\n", __func__, part_name);
	return -1;
}

int model_file_is_exist(const char *file_path)
{
	if (!file_path)
		return 0;
	if (strcmp(file_path, local_file_path)) {
		if (split_file_path(file_path) < 0)
			return 0;
	}

	if (set_block_device(local_part_name) < 0)
		return 0;

	return fs_exists(local_file_name);
}

int model_get_file_size(const char *file_path)
{
	loff_t file_size = 0;

	if (!file_path)
		return -1;
	if (strcmp(file_path, local_file_path)) {
		if (split_file_path(file_path) < 0)
			return -1;
	}

	if (set_block_device(local_part_name) < 0)
		return -1;

	if (fs_size(local_file_name, &file_size)) {
		ALOGE("%s: file \"%s\" is not exist!\n", __func__, local_file_name);
		return -1;
	}

	if (file_size == 0)
		ALOGE("%s: file \"%s\" size error!\n", __func__, local_file_name);

	return file_size;
}

int model_read_file(const char *file_path, unsigned char *data_buf, int buf_size)
{
	loff_t rd_size = 0;
	int ret = -1;

	if (!file_path)
		return -1;
	if (strcmp(file_path, local_file_path)) {
		if (split_file_path(file_path) < 0)
			return -1;
	}

	if (set_block_device(local_part_name) < 0)
		return -1;

	ret = fs_read(local_file_name, (unsigned long)data_buf, 0, 0, &rd_size);
	if (ret < 0)
		return -1;

	if (rd_size > buf_size) {
		ALOGE("%s: %s: rd_size %lld out of buf_size %d\n",
			__func__, file_path, rd_size, buf_size);
		return -1;
	}

	flush_dcache_range((unsigned long)data_buf, (unsigned long)data_buf + rd_size);

	ALOGD("%s: %s: rd_size: %lld, buf_size: %d\n", __func__, file_path, rd_size, buf_size);

	return rd_size;
}

unsigned char *model_read_file_to_buffer(const char *file_path, int *buf_size)
{
	unsigned char *tmp_buf = NULL;
	int rd_size = 0, file_size = 0, new_size = 0;

	file_size = model_get_file_size(file_path);
	if (file_size <= 0)
		return NULL;

	new_size = file_size + 8; //avoid last char not '\0' issue
	tmp_buf = (unsigned char *)malloc(new_size);
	if (!tmp_buf)
		return NULL;
	memset(tmp_buf, 0, new_size);

	rd_size = model_read_file(file_path, tmp_buf, file_size);
	if (rd_size <= 0 || rd_size > file_size) {
		memset(tmp_buf, 0, new_size);
		free(tmp_buf);
		return NULL;
	}

	*buf_size = new_size;
	return tmp_buf;
}

unsigned char *model_read_bin_to_buffer(const char *file_path, int *buf_size)
{
	unsigned char *tmp_buf = NULL;
	int rd_size = 0, file_size = 0;

	file_size = model_get_file_size(file_path);
	if (file_size <= 0)
		return NULL;

	tmp_buf = (unsigned char *)malloc(file_size);
	if (!tmp_buf)
		return NULL;
	memset(tmp_buf, 0, file_size);

	rd_size = model_read_file(file_path, tmp_buf, file_size);
	if (rd_size <= 0 || rd_size > file_size) {
		memset(tmp_buf, 0, file_size);
		free(tmp_buf);
		return NULL;
	}

	*buf_size = file_size;
	return tmp_buf;
}
