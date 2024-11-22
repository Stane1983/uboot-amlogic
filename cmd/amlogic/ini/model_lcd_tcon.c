// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "ini_config.h"

#define LOG_TAG "model"
#define LOG_NDEBUG 0

#include "ini_log.h"
#include "ini_proxy.h"
#include "ini_handler.h"
#include "ini_platform.h"
#include "ini_io.h"
#include "model.h"
#include <amlogic/partition_table.h>

#ifdef CONFIG_AML_LCD
#ifdef CONFIG_AML_LCD_TCON
static int g_lcd_tcon_spi_cnt;

static int handle_tcon_path_default(void)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *buf;
	char str[30];
	const char *ini_value = NULL;
	unsigned int i, n, block_cnt, data_size, crc32;

	buf = get_tcon_bin_path_mem();
	if (!buf)
		return -1;

	header = (struct lcd_tcon_bin_path_header_s *)buf;

	header->version = 1;

	/* data_load_level */
	ini_value = ini_get_string("tcon_Path", "data_load_level", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, data_load_level is (%s)\n", __func__, ini_value);
	header->data_load_level = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("tcon_Path", "init_load", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, init_load is (%s)\n", __func__, ini_value);
	header->init_load = strtoul(ini_value, NULL, 0);

	block_cnt = 0;
	n = 32;

	for (i = 0; i < 32; i++) {
		snprintf(str, 30, "TCON_DATA_%d_BIN_PATH", i);
		ini_value = ini_get_string("tcon_Path", str, "null");
		if (strcmp(ini_value, "null") == 0)
			break;

		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, tcon_path %d is (%s)\n", __func__, i, ini_value);
		strncpy((char *)&buf[n + 4], ini_value, 252);
		block_cnt++;
		n += 256;
	}
	header->block_cnt = block_cnt;
	header->ready = 1;

	data_size = 32 + block_cnt * 256;
	header->data_size = data_size;

	/* data check */
	crc32 = cal_CRC32(0, &buf[4], (data_size - 4));
	header->crc32 = crc32;

	return 0;
}

int handle_tcon_path_resv_for_kernel(void)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *buf, *buf_dft;
	char str[30];
	const char *ini_value = NULL;
	unsigned int i, n, block_cnt, data_size, crc32;

	buf = get_tcon_bin_path_k_mem();
	if (!buf)
		return -1;
	header = (struct lcd_tcon_bin_path_header_s *)buf;

	/* detect tcon_path resv_for_kernel exist or not */
	ini_value = ini_get_string("tcon_Path", "TCON_BIN_PATH_K", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, PATH_K not exist, use default path\n", __func__);

		buf_dft = get_tcon_bin_path_mem();
		if (!buf_dft) {
			ALOGE("%s, default path is NULL\n", __func__);
			return -1;
		}
		header = (struct lcd_tcon_bin_path_header_s *)buf_dft;
		memcpy(buf, buf_dft, header->data_size);
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, tcon bin path kernel same as uboot\n", __func__);
		return 0;
	}

	header->version = 1;
	header->ready = 1;

	/* data_load_level */
	ini_value = ini_get_string("tcon_Path", "data_load_level", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, data_load_level is (%s)\n", __func__, ini_value);
	header->data_load_level = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("tcon_Path", "init_load", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, init_load is (%s)\n", __func__, ini_value);
	header->init_load = strtoul(ini_value, NULL, 0);

	block_cnt = 0;
	n = 32;

	for (i = 0; i < 32; i++) {
		snprintf(str, 30, "TCON_DATA_%d_BIN_PATH_K", i);
		ini_value = ini_get_string("tcon_Path", str, "null");
		if (strcmp(ini_value, "null") == 0)
			break;

		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, tcon_path %d is (%s)\n", __func__, i, ini_value);
		if (n + 256 > header->mem_total_size) {
			ALOGE("%s, tcon_path_resv mem out of size %d\n",
				__func__, header->mem_total_size);
			break;
		}
		strncpy((char *)&buf[n + 4], ini_value, 252);
		block_cnt++;
		n += 256;
	}
	header->block_cnt = block_cnt;

	data_size = 32 + block_cnt * 256;
	header->data_size = data_size;

	/* data check */
	crc32 = cal_CRC32(0, &buf[4], (data_size - 4));
	header->crc32 = crc32;

	return 0;
}

static int handle_tcon_spi_v0(unsigned char *buff)
{
	struct lcd_tcon_spi_header_s *header;
	const char *ini_value = NULL;
	char str[30];
	unsigned int null_cnt = 0, block_cnt = 4;
	unsigned int temp, i, j, n;

	header = (struct lcd_tcon_spi_header_s *)buff;
	header->version = 0;

	/* block 0: demura_lut */
	n = 16;
	ini_value = ini_get_string("tcon_spi_Attr", "demura_lut_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, demura_lut_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = ini_get_string("tcon_spi_Attr", "demura_lut_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, demura_lut_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block0_param_%d", j);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_1:
	/* block 1: p_gamma */
	ini_value = ini_get_string("tcon_spi_Attr", "p_gamma_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, p_gamma_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = ini_get_string("tcon_spi_Attr", "p_gamma_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, p_gamma_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block1_param_%d", j);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_2:
	/* block 2: acc_lut */
	ini_value = ini_get_string("tcon_spi_Attr", "acc_lut_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, acc_lut_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = ini_get_string("tcon_spi_Attr", "acc_lut_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, acc_lut_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block2_param_%d", j);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_3:
	/* block 3: auto_flicker */
	ini_value = ini_get_string("tcon_spi_Attr", "auto_flicker_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, auto_flicker_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = ini_get_string("tcon_spi_Attr", "auto_flicker_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, auto_flicker_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block3_param_%d", j);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_next:
	if (null_cnt >= 4) {
		block_cnt = 0;
		g_lcd_tcon_spi_cnt = 0;
	} else {
		block_cnt = 4;
		g_lcd_tcon_spi_cnt = (16 + 32 * block_cnt);
	}

	/* block cnt */
	header->block_cnt = block_cnt;
	header->block_cnt = g_lcd_tcon_spi_cnt;
	/* crc */
	header->crc32 = cal_CRC32(0, (buff + 4), g_lcd_tcon_spi_cnt - 4);

	return 0;
}

static int handle_tcon_spi(unsigned char *buff)
{
	struct lcd_tcon_spi_header_s *header;
	unsigned char *p;
	const char *ini_value = NULL;
	char str[30];
	unsigned int data_size, block_cnt, param_cnt;
	unsigned int temp, i, j, k, n;

	header = (struct lcd_tcon_spi_header_s *)buff;

	/* header */
	/* version */
	ini_value = ini_get_string("tcon_spi_Attr", "version", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	header->version = strtoul(ini_value, NULL, 0);
	if (header->version == 0) {
		handle_tcon_spi_v0(buff);
		return 0;
	}

	/* new data format */
	/* block cnt */
	ini_value = ini_get_string("tcon_spi_Attr", "block_cnt", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, block_cnt is (%s)\n", __func__, ini_value);
	block_cnt = strtoul(ini_value, NULL, 0);
	header->block_cnt = block_cnt;

	p = &buff[16];
	n = 0;
	for (i = 0; i < block_cnt; i++) {
		snprintf(str, 30, "block%d_data_type", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n] = temp & 0xff;
		p[n + 1] = (temp >> 8) & 0xff;

		snprintf(str, 30, "block%d_data_index", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0xff");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 2] = temp & 0xff;
		p[n + 3] = (temp >> 8) & 0xff;

		snprintf(str, 30, "block%d_data_flag", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0xff");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 4] = temp & 0xff;
		p[n + 5] = (temp >> 8) & 0xff;
		p[n + 6] = (temp >> 16) & 0xff;
		p[n + 7] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_spi_data_offset", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 8] = temp & 0xff;
		p[n + 9] = (temp >> 8) & 0xff;
		p[n + 10] = (temp >> 16) & 0xff;
		p[n + 11] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_spi_data_size", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 12] = temp & 0xff;
		p[n + 13] = (temp >> 8) & 0xff;
		p[n + 14] = (temp >> 16) & 0xff;
		p[n + 15] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_param_cnt", i);
		ini_value = ini_get_string("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		param_cnt = strtoul(ini_value, NULL, 0);
		p[n + 16] = param_cnt & 0xff;
		p[n + 17] = (param_cnt >> 8) & 0xff;
		p[n + 18] = (param_cnt >> 16) & 0xff;
		p[n + 19] = (param_cnt >> 24) & 0xff;

		/* conversion parameters */
		k = n + 20;
		for (j = 0; j < param_cnt; j++) {
			snprintf(str, 30, "block%d_param_%d", i, j);
			ini_value = ini_get_string("tcon_spi_Attr", str, "0");
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
			temp = strtoul(ini_value, NULL, 0);
			p[k] = temp & 0xff;
			p[k + 1] = (temp >> 8) & 0xff;
			p[k + 2] = (temp >> 16) & 0xff;
			p[k + 3] = (temp >> 24) & 0xff;
			k += 4;
		}
		n += (20 + param_cnt * 4);
	}

	/* data size */
	data_size = 16 + n;
	g_lcd_tcon_spi_cnt = data_size;
	header->data_size = data_size;

	/* crc */
	header->crc32 = cal_CRC32(0, (buff + 4), g_lcd_tcon_spi_cnt - 4);

	return 0;
}

static int handle_read_bin_file_with_header(const char *file_name, unsigned long max_len)
{
	unsigned char buf[16];
	int bin_size, data_size = 0;

	bin_file_init();

	bin_size = read_bin_file(file_name);
	if (bin_size < 64) {
		ALOGE("%s, load bin file error!\n", __func__);
		bin_file_uninit();
		return 0;
	}

	get_bin_data(buf, 16);
	data_size = (buf[8] | (buf[9] << 8) |
		     (buf[10] << 16) | (buf[11] << 24));
	if (data_size > bin_size || data_size < 64) {
		ALOGE("%s, bin file size less than expectation!\n", __func__);
		bin_file_uninit();
		return 0;
	}
	if (data_size > max_len) {
		ALOGE("%s, bin file size out of support!\n", __func__);
		bin_file_uninit();
		return 0;
	}

	return data_size;
}

static inline void get_and_set_env_tcon_path(char *env)
{
	const char *ini_value = NULL;
	char env_str[64];

	ini_value = ini_get_string("tcon_Path", env, "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, %s is (%s)\n", __func__, env, ini_value);
	if (strcmp(ini_value, "null")) {
		env_set(env, ini_value);
		sprintf(env_str, "update_env_part -p -f %s", env);
		run_command(env_str, 0);
	}
}

static void handle_tcon_dccd_path(void)
{
	char env_str[50];
	int i = 0;

	// save dccd bin path
	get_and_set_env_tcon_path("DCCD_BIN_PATH_K");

	// save tcon base bin path
	for (i = 0; i < 4; i++) {
		sprintf(env_str, "TCON_BASE_BIN_%d_PATH_K", i);
		get_and_set_env_tcon_path(env_str);
	}
}

int handle_tcon_path(void)
{
	char str[50], env_str[50];
	const char *ini_value = NULL;
	unsigned char *tmp_buf = NULL;
	unsigned char *tcon_spi = NULL;
	unsigned int tmp_buf_size = CC_MAX_TCON_BIN_SIZE;
	unsigned int header;
	int tmp_len, i, j;

	/* tcon_bin_header */
	ini_value = ini_get_string("tcon_Path", "header", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, header is (%s)\n", __func__, ini_value);
	header = strtoul(ini_value, NULL, 0);
	snprintf(str, 50, "%d", header);
	env_set("model_tcon_bin_header", str);

	/* tcon regs bin */
	ini_value = ini_get_string("tcon_Path", "TCON_BIN_PATH", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGE("%s, tcon bin load file error!\n", __func__);
	}
	env_set("model_tcon", ini_value);

	handle_tcon_path_default();
	handle_tcon_path_resv_for_kernel();

	handle_tcon_dccd_path();

	/* pmu bin */
	for (i = 0; i < 4; i++) {
		snprintf(str, 50, "TCON_EXT_B%d_BIN_PATH", i);
		snprintf(env_str, 50, "model_tcon_ext_b%d", i);
		ini_value = ini_get_string("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			goto handle_tcon_path_pmu_bin_multi;
		}
		env_set(env_str, ini_value);
	}

handle_tcon_path_pmu_bin_multi:
	for (j = 0; j < 10; j++) {
		snprintf(str, 50, "TCON_EXT_B%d_%d_BIN_PATH", i, j);
		snprintf(env_str, 50, "model_tcon_ext_b%d_%d", i, j);
		ini_value = ini_get_string("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			continue;
		}
		env_set(env_str, ini_value);
	}

	for (i = 0; i < 4; i++) {
		snprintf(str, 50, "TCON_EXT_B%d_SPI_BIN_PATH", i);
		snprintf(env_str, 50, "model_tcon_ext_b%d_spi", i);
		ini_value = ini_get_string("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			goto handle_tcon_path_pmu_spi_bin_multi;
		}
		env_set(env_str, ini_value);
	}

handle_tcon_path_pmu_spi_bin_multi:
	for (j = 0; j < 10; j++) {
		snprintf(str, 50, "TCON_EXT_B%d_%d_SPI_BIN_PATH", i, j);
			snprintf(env_str, 50, "model_tcon_ext_b%d_%d_spi", i, j);
			ini_value = ini_get_string("tcon_Path", str, "null");
			if (!strcmp(ini_value, "null")) {
				if (model_debug_flag & DEBUG_TCON)
					ALOGD("%s, no %s file\n", __func__, str);
				continue;
			}
			env_set(env_str, ini_value);
	}

	/* tcon base bin handle */
	ini_value = ini_get_string("tcon_Path", "TCON_BASE_BIN_PATH", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGE("%s, tcon bin load file error!\n", __func__);
	} else {
		env_set("model_tcon_base", ini_value);
	}

	// start handle tcon_spi param
	tmp_buf = (unsigned char *)malloc(tmp_buf_size);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	tcon_spi = (unsigned char *)malloc(CC_MAX_TCON_SPI_SIZE);
	if (!tcon_spi) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_tcon_path_end;
	}
	memset(tcon_spi, 0, CC_MAX_TCON_SPI_SIZE);

	handle_tcon_spi(tcon_spi);
	if (g_lcd_tcon_spi_cnt) {
		panel_param_mem_put((u8 *)tcon_spi, "lcd_tcon_spi", g_lcd_tcon_spi_cnt);

		memset((void *)tmp_buf, 0, tmp_buf_size);
		tmp_len = read_tcon_spi_param(tmp_buf);
		//ALOGD("%s, start check lcd_tcon_spi param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, g_lcd_tcon_spi_cnt, tcon_spi, tmp_len, tmp_buf) ==
		    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check lcd_tcon_spi param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			save_tcon_spi_param(g_lcd_tcon_spi_cnt, tcon_spi);
		}
	}

	memset(tcon_spi, 0, CC_MAX_TCON_SPI_SIZE);
	free(tcon_spi);
	// end handle lcd_tcon_spi param

handle_tcon_path_end:
	memset((void *)tmp_buf, 0, tmp_buf_size);
	free(tmp_buf);

	return 0;
}

int handle_tcon_bin(void)
{
	int tmp_len = 0;
	unsigned int tcon_bin_size;
	unsigned int size = 0, tmp_buf_size = 0;
	unsigned char *tmp_buf = NULL;
	unsigned char *tcon_buf = NULL;
	char *file_name;
	unsigned int bypass, header, data_crc32, temp_crc32;
	int tmp;
	struct dccd_info_s *dccd_info = get_dccd_info();

	tmp = env_get_ulong("model_tcon_bypass", 10, 0xffff);
	if (tmp != 0xffff) {
		bypass = tmp;
		if (bypass) {
			ALOGI("model_tcon_bypass\n");
			return 0;
		}
	}

	header = env_get_ulong("model_tcon_bin_header", 10, 0);

	file_name = env_get("model_tcon");
	if (!file_name) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, no model_tcon path\n", __func__);
		return 0;
	}

	tmp_buf_size = CC_MAX_TCON_BIN_SIZE;
	tmp_buf = (unsigned char *)malloc(tmp_buf_size);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	// start handle lcd_tcon param
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s: model_tcon: %s\n", __func__, file_name);

	if (header)
		size = handle_read_bin_file_with_header(file_name, tmp_buf_size);
	else
		size = handle_read_bin_file(file_name, tmp_buf_size);
	if (size == 0) {
		free(tmp_buf);
		tmp_buf = NULL;
		return -1;
	}

	dccd_info->has_tcon_file = 1;

	get_bin_data(tmp_buf, size);
	if (header) {
		data_crc32 = tmp_buf[0] | (tmp_buf[1] << 8) |
			(tmp_buf[2] << 16) | (tmp_buf[3] << 24);
		temp_crc32 = cal_CRC32(0, &tmp_buf[4], (size - 4));
		if (data_crc32 != temp_crc32) {
			free(tmp_buf);
			tmp_buf = NULL;
			if (model_debug_flag & DEBUG_TCON) {
				ALOGE("%s, tcon bin crc error! raw:0x%08x, temp:0x%08x\n",
						__func__, data_crc32, temp_crc32);
			} else {
				ALOGE("%s, tcon bin crc error!!!\n", __func__);
			}
			return -1;
		}
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s: load tcon bin with header, size:0x%x\n", __func__, size);
	} else {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s: load tcon bin, size:0x%x\n", __func__, size);
	}

	tcon_bin_size = size;
	tcon_buf = (unsigned char *)malloc(tcon_bin_size);
	if (!tcon_buf) {
		free(tmp_buf);
		tmp_buf = NULL;
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memcpy(tcon_buf, tmp_buf, tcon_bin_size);

	bin_file_uninit();

	panel_param_mem_put((u8 *)tcon_buf, "lcd_tcon", tcon_bin_size);

	memset((void *)tmp_buf, 0, tmp_buf_size);
	tmp_len = read_tcon_bin_param(tmp_buf);
	//ALOGD("%s, start check lcd_tcon param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(1, tcon_bin_size, tcon_buf, tmp_len, tmp_buf) ==
		CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check tcon bin data diff (0x%x), save tcon bin data.\n",
		      __func__, tmp_len);
		save_tcon_bin_param(tcon_bin_size, tcon_buf);
	}

	memset((void *)tcon_buf, 0, tcon_bin_size);
	free(tcon_buf);
	// end handle lcd_tcon param

	memset((void *)tmp_buf, 0, tmp_buf_size);
	free(tmp_buf);

	return 0;
}
#endif
#endif
