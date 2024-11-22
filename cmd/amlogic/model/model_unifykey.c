// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <amlogic/keyunify.h>
#include <amlogic/aml_model.h>
#include "model_log.h"
#include "model_unifykey.h"
#include "model.h"

#define LOG_TAG     "model_unifykey"
#define LOG_NDEBUG  0

#ifdef CONFIG_UNIFY_KEY_MANAGE
int model_check_ukey_size(const char *key_name)
{
	int ret = 0, key_exist = 0, is_secure = 0;
	unsigned int key_len = 0;
	ssize_t key_size = 0;

	// start check the key is exist?
	ret = key_unify_query_exist(key_name, &key_exist);
	if (ret) {
		ALOGE("%s: %s query exist error\n", __func__, key_name);
		return -1;
	}
	if (key_exist == 0) {
		ALOGD("%s: %s is not exist\n", __func__, key_name);
		return 0;
	}
	// end check the key is exist?

	// start check the key is secure?
	ret = key_unify_query_secure(key_name, &is_secure);
	if (ret) {
		ALOGE("%s: %s query secure error\n", __func__, key_name);
		return -1;
	}
	if (is_secure) {
		ALOGE("%s: %s is secure key\n", __func__, key_name);
		return -1;
	}
	// end check the key is secure?

	// start read and check data integrity
	ret = key_unify_query_size(key_name, &key_size);
	if (ret) {
		ALOGE("%s: %s query size error\n", __func__, key_name);
		return -1;
	}
	//ALOGD("%s, %s size: %d\n",__func__, key_name, (int)key_size);

	key_len = (int)key_size;
	//ALOGD("%s, %s size: %d\n",__func__, key_name, key_len);

	return key_len;
}

unsigned char *model_read_ukey_data(const char *key_name, int *data_size)
{
	unsigned char *data_buf;
	int key_len = 0, ret = 0;

	if (!key_name) {
		ALOGE("%s: key_name is NULL\n", __func__);
		return NULL;
	}

	key_len = model_check_ukey_size(key_name);
	if (key_len <= 0) {
		ALOGE("%s: %s size %d error\n", __func__, key_name, key_len);
		return NULL;
	}

	data_buf = (unsigned char *)malloc(key_len);
	if (!data_buf) {
		ALOGE("%s: malloc buf failed\n", __func__);
		return NULL;
	}
	memset(data_buf, 0, key_len);

	ret = key_unify_read(key_name, data_buf, key_len);
	if (ret) {
		ALOGE("%s: %s unify read error\n", __func__, key_name);
		memset(data_buf, 0, key_len);
		free(data_buf);
		return NULL;
	}

	*data_size = key_len;
	return data_buf;
}

int model_write_ukey_data(const char *key_name, unsigned char *data_buf, int buf_size)
{
	if (key_unify_write(key_name, data_buf, buf_size) == 0)
		return 0;
	return -1;
}

#else
int model_check_ukey_size(const char *key_name)
{
	return -1;
}

unsigned char *model_read_ukey_data(const char *key_name, int *data_size)
{
	return NULL;
}

int model_write_ukey_data(const char *key_name, unsigned char *data_buf, int buf_size)
{
	return -1;
}

#endif
