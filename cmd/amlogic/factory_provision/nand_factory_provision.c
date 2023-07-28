// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <fs.h>
#include <mapmem.h>
#include <u-boot/sha256.h>
#include "factory_provision_utils.h"
#include "nand_factory_provision.h"

#ifndef CONFIG_YAFFS_DIRECT
#define CONFIG_YAFFS_DIRECT
#endif

#ifndef CONFIG_YAFFSFS_PROVIDE_VALUES
#define CONFIG_YAFFSFS_PROVIDE_VALUES
#endif

#include <../../fs/yaffs2/yaffsfs.h>
#include <../../fs/yaffs2/yaffs_guts.h>

static char g_keybox[MAX_SIZE_KEYBOX] = { 0 };

static const char *yaffs_error_str(void)
{
	int error = yaffsfs_GetLastError();

	if (error < 0)
		error = -error;

	switch (error) {
	case EBUSY: return "Busy";
	case ENODEV: return "No such device";
	case EINVAL: return "Invalid parameter";
	case ENFILE: return "Too many open files";
	case EBADF:  return "Bad handle";
	case EACCES: return "Wrong permissions";
	case EXDEV:  return "Not on same device";
	case ENOENT: return "No such entry";
	case ENOSPC: return "Device full";
	case EROFS:  return "Read only file system";
	case ERANGE: return "Range error";
	case ENOTEMPTY: return "Not empty";
	case ENAMETOOLONG: return "Name too long";
	case ENOMEM: return "Out of memory";
	case EFAULT: return "Fault";
	case EEXIST: return "Name exists";
	case ENOTDIR: return "Not a directory";
	case EISDIR: return "Not permitted on a directory";
	case ELOOP:  return "Symlink loop";
	case 0: return "No error";
	default: return "Unknown error";
	}
}

extern int meson_yaffs2_mount(char *mtpoint, char *part_name);

static int nand_write(const char *file_path, const char *data, u32 size)
{
	int fd = -1;

	fd = yaffs_open(file_path, O_CREAT | O_RDWR | O_TRUNC, S_IREAD | S_IWRITE);
	if (fd < 0) {
		LOGE("open file '%s' failed, %s\n", file_path, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	if (yaffs_write(fd, data, size) != size) {
		LOGE("write file '%s' failed, %s\n", file_path, yaffs_error_str());
		yaffs_close(fd);
		return CMD_RET_UNKNOWN_ERROR;
	}

	if (yaffs_close(fd)) {
		LOGE("close file '%s' failed, %s\n", file_path, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	return CMD_RET_SUCCESS;
}

static int nand_read(const char *file_path, char *buf, u32 *size)
{
	int fd = -1;

	fd = yaffs_open(file_path, O_RDWR, 0);
	if (fd < 0) {
		LOGE("open file '%s' failed, %s\n", file_path, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	*size = yaffs_read(fd, buf, *size);
	if (*size <= 0) {
		LOGE("read file '%s' failed, %s\n", file_path, yaffs_error_str());
		yaffs_close(fd);
		return CMD_RET_UNKNOWN_ERROR;
	}

	if (yaffs_close(fd)) {
		LOGE("close file '%s' failed, %s\n", file_path, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	return CMD_RET_SUCCESS;
}

static int nand_verify_written_keybox(const char *keybox_name, const char *keybox, u32 keybox_size)
{
	int ret = CMD_RET_SUCCESS;
	char sha256[SHA256_SUM_LEN] = { 0 };
	char written_sha256[SHA256_SUM_LEN] = { 0 };
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };
	u32 read_size = MAX_SIZE_KEYBOX;

	calc_sha256(keybox, keybox_size, sha256, SHA256_SUM_LEN);

	memset(g_keybox, 0, sizeof(g_keybox));
	sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, keybox_name);
	ret = nand_read(file_path, g_keybox, &read_size);
	if (ret)
		return ret;
	if (keybox_size != read_size)
		return CMD_RET_UNKNOWN_ERROR;

	calc_sha256(g_keybox, read_size, written_sha256, SHA256_SUM_LEN);

	if (memcmp(sha256, written_sha256, SHA256_SUM_LEN))
		return CMD_RET_UNKNOWN_ERROR;

	return CMD_RET_SUCCESS;
}

static int nand_factory_mount(void)
{
	struct yaffs_dev *dev = yaffs_getdev(NAND_FTY_MOUNT_PT);

	if (!dev || (dev && dev->is_mounted == 0)) {
		if (meson_yaffs2_mount(NAND_FTY_MOUNT_PT, PART_NAME_FTY)) {
			LOGE("nand mount '%s' partition failed\n", PART_NAME_FTY);
			return CMD_RET_UNKNOWN_ERROR;
		}
		LOGI("nand mount '%s' partition success\n", PART_NAME_FTY);
		return CMD_RET_SUCCESS;
	}

	return CMD_RET_SUCCESS;
}

static int nand_remove_same_type_keybox(u32 key_type, const char *uuid)
{
	int ret = CMD_RET_SUCCESS;
	yaffs_DIR *dir = NULL;
	struct yaffs_dirent *dirt = NULL;
	struct yaffs_stat st = { 0 };
	struct keybox_header hdr = { 0 };
	u32 read_size = 0;
	char uuid_str[40] = { 0 };
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };

	ret = nand_factory_mount();
	if (ret)
		return ret;

	dir = yaffs_opendir(NAND_FTY_MOUNT_PT);
	if (!dir) {
		LOGE("open dir '%s' failed, %s\n", NAND_FTY_MOUNT_PT, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	convert_to_uuid_str(uuid, uuid_str);
	while ((dirt = yaffs_readdir(dir)) != NULL) {
		memset(file_path, 0, sizeof(file_path));
		sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, dirt->d_name);
		yaffs_stat(file_path, &st);
		if ((st.st_mode & S_IFREG) != S_IFREG) // not regular file
			continue;

		memset(&hdr, 0, sizeof(hdr));
		read_size = sizeof(hdr);
		if (nand_read(file_path, (char *)&hdr, &read_size) || read_size != sizeof(hdr)) {
			LOGE("read keybox '%s' failed\n", file_path);
			ret = CMD_RET_UNKNOWN_ERROR;
			goto exit;
		}

		if (!memcmp(uuid, hdr.ta_uuid, sizeof(hdr.ta_uuid)) && key_type == hdr.key_type) {
			if (yaffs_unlink(file_path)) {
				LOGE("remove the same type(uuid = %s, key_type = 0x%02X) keybox '%s' failed\n",
						uuid_str, key_type, file_path);
				ret = CMD_RET_UNKNOWN_ERROR;
				goto exit;
			}
		}
	}

exit:
	yaffs_closedir(dir);
	return ret;
}

static int nand_append_keybox(const char *keybox_name, const char *keybox, u32 keybox_size)
{
	int ret = CMD_RET_SUCCESS;
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };

	ret = nand_factory_mount();
	if (ret)
		return ret;

	sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, keybox_name);
	ret = nand_write(file_path, keybox, keybox_size);
	if (ret) {
		LOGE("write keybox '%s' failed\n", file_path);
		return ret;
	}

	ret = nand_verify_written_keybox(keybox_name, keybox, keybox_size);
	if (ret) {
		LOGE("verify written keybox '%s' failed\n", file_path);
		return ret;
	}

	LOGI("write keybox '%s' success\n", file_path);

	return ret;
}

static int nand_query_keybox(const char *keybox_name, char *ret_data)
{
	struct yaffs_stat st;
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };
	int ret = nand_factory_mount();

	if (ret)
		return ret;

	sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, keybox_name);
	if (yaffs_stat(file_path, &st)) {
		LOGI("keybox '%s' not exists\n", file_path);
		ret = CMD_RET_KEYBOX_NOT_EXIST;
	} else {
		*(u32 *)ret_data = st.st_size;
		LOGI("keybox '%s' size is %d\n", file_path, *(u32 *)ret_data);
	}

	return ret;
}

static int nand_remove_keybox(const char *keybox_name)
{
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };
	int ret = nand_factory_mount();

	if (ret)
		return ret;

	sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, keybox_name);
	if (yaffs_unlink(file_path)) {
		LOGE("remove '%s' failed\n", file_path);
		return CMD_RET_UNKNOWN_ERROR;
	}

	LOGI("remove '%s' success\n", file_path);

	return CMD_RET_SUCCESS;
}

static int nand_remove_all_keyboxes(void)
{
	int ret = CMD_RET_SUCCESS;
	yaffs_DIR *dir = NULL;
	struct yaffs_dirent *dirt = NULL;
	struct yaffs_stat st;
	char file_path[MAX_SIZE_FILE_PATH] = { 0 };

	ret = nand_factory_mount();
	if (ret)
		return ret;

	dir = yaffs_opendir(NAND_FTY_MOUNT_PT);
	if (!dir) {
		LOGE("open dir '%s' failed, %s\n", NAND_FTY_MOUNT_PT, yaffs_error_str());
		return CMD_RET_UNKNOWN_ERROR;
	}

	while ((dirt = yaffs_readdir(dir)) != NULL) {
		memset(file_path, 0, sizeof(file_path));
		sprintf(file_path, "%s/%s", NAND_FTY_MOUNT_PT, dirt->d_name);
		yaffs_stat(file_path, &st);
		if ((st.st_mode & S_IFREG) != S_IFREG) // not regular file
			continue;

		if (yaffs_unlink(file_path)) {
			LOGE("remove '%s' failed, %s\n", file_path, yaffs_error_str());
			ret = CMD_RET_UNKNOWN_ERROR;
		} else {
			LOGI("remove '%s' success\n", file_path);
		}
	}

	yaffs_closedir(dir);

	return ret;
}

static int nand_list_all_keyboxes(void)
{
	int ret = CMD_RET_SUCCESS;
	char cmd[MAX_SIZE_CMD] = { 0 };

	ret = nand_factory_mount();
	if (ret)
		return ret;

	sprintf(cmd, "yls %s", NAND_FTY_MOUNT_PT);
	if (run_command(cmd, 0)) {
		LOGE("command[%s] failed\n", cmd);
		ret = CMD_RET_UNKNOWN_ERROR;
	}

	return ret;
}

int nand_factory_provision(const struct input_param *params)
{
	int ret = CMD_RET_SUCCESS;
	char *in_kb = NULL;
	char *ret_data = NULL;
	const struct keybox_header *hdr = NULL;

	switch (params->action) {
	case ACTION_INIT:
		ret = nand_factory_mount();
		break;
	case ACTION_WRITE:
		in_kb = map_sysmem(params->keybox_phy_addr, 0);

		ret = check_keybox(in_kb, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		hdr = (const struct keybox_header *)in_kb;
		ret = nand_remove_same_type_keybox(hdr->key_type, (const char *)hdr->ta_uuid);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		memcpy(g_keybox, in_kb, params->keybox_size);

		ret = preprocess_keybox(g_keybox, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;

		ret = nand_append_keybox(params->keybox_name, g_keybox, params->keybox_size);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_QUERY:
		ret_data = map_sysmem(params->ret_data_addr, 0);
		ret = nand_query_keybox(params->keybox_name, ret_data);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_REMOVE:
		ret = nand_remove_keybox(params->keybox_name);
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_CLEAR:
		ret = nand_remove_all_keyboxes();
		if (ret != CMD_RET_SUCCESS)
			goto exit;
		break;
	case ACTION_LIST:
		ret = nand_list_all_keyboxes();
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
