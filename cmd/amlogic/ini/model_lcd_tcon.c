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
static unsigned int g_lcd_tcon_bin_block_cnt;
static unsigned char *g_lcd_tcon_bin_path_mem, *g_lcd_tcon_bin_path_resv_mem;

static unsigned int handle_tcon_char_data_size_align(unsigned int size)
{
	unsigned int new_size;

	if (size % 4)
		new_size = (size / 4 + 1) * 4;
	else
		new_size = size;

	return new_size;
}

void *handle_tcon_path_mem_get(unsigned int size)
{
	unsigned int data_size = 0;

	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, buf is null\n", __func__);
		return NULL;
	}

	data_size = g_lcd_tcon_bin_path_mem[4] |
		(g_lcd_tcon_bin_path_mem[5] << 8) |
		(g_lcd_tcon_bin_path_mem[6] << 16) |
		(g_lcd_tcon_bin_path_mem[7] << 24);
	if (data_size > size) {
		ALOGE("%s, buf size invalid\n", __func__);
		return NULL;
	}

	return g_lcd_tcon_bin_path_mem;
}

void *handle_tcon_path_resv_mem_get(unsigned int size)
{
	unsigned int data_size = 0;

	if (!g_lcd_tcon_bin_path_resv_mem) {
		ALOGE("%s, buf is null\n", __func__);
		return NULL;
	}

	data_size = g_lcd_tcon_bin_path_resv_mem[4] |
		(g_lcd_tcon_bin_path_resv_mem[5] << 8) |
		(g_lcd_tcon_bin_path_resv_mem[6] << 16) |
		(g_lcd_tcon_bin_path_resv_mem[7] << 24);
	if (data_size > size) {
		ALOGE("%s, buf size invalid\n", __func__);
		return NULL;
	}

	return g_lcd_tcon_bin_path_resv_mem;
}

static char *handle_tcon_path_file_name_get(unsigned int index)
{
	unsigned int n;
	char *str;

	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, tcon_path buf is null\n", __func__);
		return NULL;
	}

	if (index >= g_lcd_tcon_bin_block_cnt) {
		ALOGE("%s, invalid index %d\n", __func__, index);
		return NULL;
	}

	n = 32 + (index * 256) + 4;
	str = (char *)&g_lcd_tcon_bin_path_mem[n];
	return str;
}

#define TCON_VAC_SET_PARAM_NUM    3
#define TCON_VAC_LUT_PARAM_NUM    256
int handle_tcon_vac(unsigned char *vac_data, unsigned int vac_mem_size)
{
	int i, n, tmp_cnt, len;
	char *file_name;
	const char *ini_value = NULL;
	unsigned int tmp_buf[512];
	unsigned int data_cnt = 0;

	file_name = env_get("model_tcon_vac");
	if (!file_name) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_vac path\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_vac: %s\n", __func__, file_name);
	if (!vac_data || !vac_mem_size) {
		ALOGE("%s, buffer memory or data size error!!!\n", __func__);
		return -1;
	}

	ini_parser_init();

	if (ini_parse_file(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		ini_parser_uninit();
		free(vac_data);
		vac_data = NULL;
		return -1;
	}
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("vac_data addr: 0x%p\n", vac_data);

	n = 8;
	len = TCON_VAC_SET_PARAM_NUM;

	ini_value = ini_get_string("lcd_tcon_vac", "vac_set", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt = tmp_cnt;

	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_set data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if (model_debug_flag & DEBUG_TCON) {
			ALOGD("vac_set: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1],
			      tmp_buf[i]);
		}
	}

	len = TCON_VAC_LUT_PARAM_NUM;

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt1", "null");
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt1 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (TCON_VAC_SET_PARAM_NUM * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt1_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt2", "null");
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt2 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt2_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_1", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_1 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_1_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_2", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_2 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_2_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_3", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (data_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_3 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_3_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_4", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_4 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_4_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_5", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_5 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_5_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	ini_value = ini_get_string("lcd_tcon_vac", "vac_ramt3_6", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if (tmp_cnt > CC_MAX_TCON_VAC_SIZE || tmp_cnt < len) {
		ALOGE("%s: invalid vac_ramt3_6 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n + i * 2] = tmp_buf[i] & 0xff;
		vac_data[n + i * 2 + 1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && i < 30) {
			ALOGD("vac_ramt3_6_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n + i * 2], vac_data[n + i * 2 + 1], tmp_buf[i]);
		}
	}

	/*add check data: total_size(4byte) + crc(4byte) +
	 *crc todo
	 */
	vac_data[0] = data_cnt & 0xff;
	vac_data[1] = (data_cnt >> 8) & 0xff;
	vac_data[2] = (data_cnt >> 16) & 0xff;
	vac_data[3] = (data_cnt >> 24) & 0xff;

	vac_data[4] = model_data_checksum(&vac_data[8], data_cnt);
	vac_data[5] = model_data_lrc(&vac_data[8], data_cnt);
	vac_data[6] = 0x55;
	vac_data[7] = 0xaa;

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s finish\n", __func__);

	ini_parser_uninit();
	return 0;
}

int handle_tcon_demura_set(unsigned char *demura_set_data, unsigned int demura_set_size)
{
	unsigned long bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_demura_set");
	if (!file_name) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_demura_set path\n", __func__);
		return -1;
	}

	if (!demura_set_data || !demura_set_size) {
		ALOGE("%s, buffer or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_demura_set: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_DEMURA_SET_SIZE);
	if (!bin_size || bin_size > demura_set_size) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, demura_set_size);
		return -1;
	}

	n = 8;
	get_bin_data(&demura_set_data[n], bin_size);

	demura_set_data[0] = bin_size & 0xff;
	demura_set_data[1] = (bin_size >> 8) & 0xff;
	demura_set_data[2] = (bin_size >> 16) & 0xff;
	demura_set_data[3] = (bin_size >> 24) & 0xff;

	demura_set_data[4] = model_data_checksum(&demura_set_data[8], bin_size);
	demura_set_data[5] = model_data_lrc(&demura_set_data[8], bin_size);
	demura_set_data[6] = 0x55;
	demura_set_data[7] = 0xaa;

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s finish\n", __func__);

	bin_file_uninit();

	return 0;
}

int handle_tcon_demura_lut(unsigned char *demura_lut_data, unsigned int demura_lut_size)
{
	unsigned long bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_demura_lut");
	if (!file_name) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_demura_lut path\n", __func__);
		return -1;
	}

	if (!demura_lut_data || !demura_lut_size) {
		ALOGE("%s, buffer memory or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_demura_lut: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_DEMURA_LUT_SIZE);
	if (!bin_size || bin_size > demura_lut_size) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, demura_lut_size);
		return -1;
	}

	n = 8;
	get_bin_data(&demura_lut_data[n], bin_size);

	demura_lut_data[0] = bin_size & 0xff;
	demura_lut_data[1] = (bin_size >> 8) & 0xff;
	demura_lut_data[2] = (bin_size >> 16) & 0xff;
	demura_lut_data[3] = (bin_size >> 24) & 0xff;

	demura_lut_data[4] = model_data_checksum(&demura_lut_data[8], bin_size);
	demura_lut_data[5] = model_data_lrc(&demura_lut_data[8], bin_size);
	demura_lut_data[6] = 0x55;
	demura_lut_data[7] = 0xaa;

	if (model_debug_flag)
		ALOGD("%s finish, bin_size = 0x%lx\n", __func__, bin_size);

	bin_file_uninit();

	return 0;
}

int handle_tcon_acc_lut(unsigned char *acc_lut_data, unsigned int acc_lut_size)
{
	unsigned long bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_acc_lut");
	if (!file_name) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_acc_lut path\n", __func__);
		return -1;
	}

	if (!acc_lut_data || acc_lut_size == 0) {
		ALOGE("%s, buffer memory or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_acc_lut: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_ACC_LUT_SIZE);
	if (!bin_size || bin_size > acc_lut_size) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, acc_lut_size);
		return -1;
	}

	n = 8;
	get_bin_data(&acc_lut_data[n], bin_size);

	acc_lut_data[0] = bin_size & 0xff;
	acc_lut_data[1] = (bin_size >> 8) & 0xff;
	acc_lut_data[2] = (bin_size >> 16) & 0xff;
	acc_lut_data[3] = (bin_size >> 24) & 0xff;

	acc_lut_data[4] = model_data_checksum(&acc_lut_data[8], bin_size);
	acc_lut_data[5] = model_data_lrc(&acc_lut_data[8], bin_size);
	acc_lut_data[6] = 0x55;
	acc_lut_data[7] = 0xaa;

	if (model_debug_flag)
		ALOGD("%s finish, bin_size = 0x%lx\n", __func__, bin_size);

	bin_file_uninit();

	return 0;
}

int handle_tcon_data_load(unsigned char **buf, unsigned int index)
{
	unsigned char *data_buf;
	unsigned long bin_size, new_size;
	unsigned int data_size;
	unsigned int data_crc32, temp_crc32;
	char *file_name;

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}

	file_name = handle_tcon_path_file_name_get(index);
	if (!file_name)
		return -1;

	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s: tcon_data[%d] file name: %s\n", __func__, index, file_name);

	bin_size = handle_read_bin_file(file_name, CC_MAX_DATA_SIZE);
	if (bin_size == 0) {
		ALOGE("%s, bin_size 0x%lx error!\n", __func__, bin_size);
		return -1;
	}

	data_buf = buf[index];
	if (data_buf) { /* already exist for reload */
		data_size = data_buf[8] |
			(data_buf[9] << 8) |
			(data_buf[10] << 16) |
			(data_buf[11] << 24);
		if (data_size >= bin_size) {
			memset(data_buf, 0, data_size);
			goto handle_tcon_data_load_next;
		}
		free(data_buf);
		buf[index] = NULL;
	}
	/* note: all the tcon data buf size must align to 32byte */
	new_size = handle_tcon_char_data_size_align(bin_size);
	data_buf = (unsigned char *)malloc(new_size);
	if (!data_buf) {
		ALOGE("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(data_buf, 0, new_size);
	buf[index] = data_buf;

handle_tcon_data_load_next:
	get_bin_data(data_buf, bin_size);
	data_size = data_buf[8] |
		(data_buf[9] << 8) |
		(data_buf[10] << 16) |
		(data_buf[11] << 24);
	if (data_size > bin_size || data_size == 0) {
		ALOGE("%s: data_size 0x%x invalid, bin_size 0x%lx\n",
		      __func__, data_size, bin_size);
		free(data_buf);
		buf[index] = NULL;
		return -1;
	}

	/* data check */
	data_crc32 = data_buf[0] |
		(data_buf[1] << 8) |
		(data_buf[2] << 16) |
		(data_buf[3] << 24);
	temp_crc32 = cal_CRC32(0, &data_buf[4], (data_size - 4));

	if (model_debug_flag & DEBUG_TCON) {
		ALOGD("%s: tcon_data[%d] crc32=0x%08x(0x%02x)\n",
		      __func__, index, temp_crc32, data_crc32);
	}
	if (data_crc32 != temp_crc32) {
		ALOGE("%s: tcon_data[%d] crc32 check error\n", __func__, index);
		free(data_buf);
		buf[index] = NULL;
		return -1;
	}

	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s %d finish, bin_size = 0x%lx\n", __func__, index, bin_size);

	bin_file_uninit();

	return 0;
}

static int handle_tcon_path_default(unsigned int version)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *buf;
	char str[30];
	const char *ini_value = NULL;
	unsigned int i, n, block_cnt, data_size, crc32;

	/* tcon data bin path */
	g_lcd_tcon_bin_path_mem = (unsigned char *)malloc(CC_MAX_TCON_BIN_PATH_SIZE);
	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(g_lcd_tcon_bin_path_mem, 0, CC_MAX_TCON_BIN_PATH_SIZE);
	buf = g_lcd_tcon_bin_path_mem;
	header = (struct lcd_tcon_bin_path_header_s *)buf;

	header->version = version;

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

	if (version == 0) {/* tcon data bin: old data format */
		ini_value = ini_get_string("tcon_Path", "TCON_VAC_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no vac ini file\n", __func__);
		}
		env_set("model_tcon_vac", ini_value);
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_DEMURA_SET_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_set file\n", __func__);
		}
		env_set("model_tcon_demura_set", ini_value);
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_DEMURA_LUT_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_lut file\n", __func__);
		}
		env_set("model_tcon_demura_lut", ini_value);
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_ACC_LUT_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no acc_lut file\n", __func__);
		}
		env_set("model_tcon_acc_lut", ini_value);
		strlcpy((char *)&buf[n + 4], ini_value, 256);

		/* block cnt */
		block_cnt = 4;
	} else {/* tcon data bin: new data format */
		for (i = 0; i < 32; i++) {
			snprintf(str, 30, "TCON_DATA_%d_BIN_PATH", i);
			ini_value = ini_get_string("tcon_Path", str, "null");
			if (strcmp(ini_value, "null") == 0)
				break;

			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, tcon_path %d is (%s)\n", __func__, i, ini_value);
			strlcpy((char *)&buf[n + 4], ini_value, 252);
			block_cnt++;
			n += 256;
		}
	}
	header->block_cnt = block_cnt;
	header->ready = 1;

	/* g_lcd_tcon_bin_block_cnt default for uboot load */
	g_lcd_tcon_bin_block_cnt = block_cnt;

	/* data size */
	data_size = 32 + block_cnt * 256;
	header->data_size = data_size;

	/* data check */
	crc32 = cal_CRC32(0, &buf[4], (data_size - 4));
	header->crc32 = crc32;

	return 0;
}

static int handle_tcon_path_resv_for_kernel(unsigned int version)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *buf;
	char str[30];
	const char *ini_value = NULL;
	unsigned int i, n, block_cnt, data_size, crc32;

	g_lcd_tcon_bin_path_resv_mem = (unsigned char *)malloc(CC_MAX_TCON_BIN_PATH_SIZE);
	if (!g_lcd_tcon_bin_path_resv_mem) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(g_lcd_tcon_bin_path_resv_mem, 0, CC_MAX_TCON_BIN_PATH_SIZE);
	buf = g_lcd_tcon_bin_path_resv_mem;
	header = (struct lcd_tcon_bin_path_header_s *)buf;

	/* detect tcon_path resv_for_kernel exist or not */
	ini_value = ini_get_string("tcon_Path", "TCON_BIN_PATH_K", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, PATH_K not exist, use default path\n", __func__);
		return -1;
	}

	header->version = version;

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

	if (version == 0) {/* tcon data bin: old data format */
		ini_value = ini_get_string("tcon_Path", "TCON_VAC_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no vac ini file\n", __func__);
		}
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_DEMURA_SET_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_set file\n", __func__);
		}
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_DEMURA_LUT_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_lut file\n", __func__);
		}
		strlcpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = ini_get_string("tcon_Path", "TCON_ACC_LUT_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no acc_lut file\n", __func__);
		}
		strlcpy((char *)&buf[n + 4], ini_value, 256);

		/* block cnt */
		block_cnt = 4;
	} else {/* tcon data bin: new data format */
		for (i = 0; i < 32; i++) {
			snprintf(str, 30, "TCON_DATA_%d_BIN_PATH_K", i);
			ini_value = ini_get_string("tcon_Path", str, "null");
			if (strcmp(ini_value, "null") == 0)
				break;

			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, tcon_path %d is (%s)\n", __func__, i, ini_value);
			strlcpy((char *)&buf[n + 4], ini_value, 252);
			block_cnt++;
			n += 256;
		}
	}
	header->block_cnt = block_cnt;
	header->ready = 1;

	/* data size */
	data_size = 32 + block_cnt * 256;
	header->data_size = data_size;

	/* data check */
	crc32 = cal_CRC32(0, &buf[4], (data_size - 4));
	header->crc32 = crc32;

	return 0;
}

static int handle_tcon_spi_v0(unsigned char *buff)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned int null_cnt = 0, block_cnt = 4;
	unsigned int temp, i, j, n;

	/* header */
	/* version */
	temp = 0;
	buff[8] = temp & 0xff;
	buff[9] = (temp >> 8) & 0xff;
	buff[10] = (temp >> 16) & 0xff;
	buff[11] = (temp >> 24) & 0xff;

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
	buff[12] = block_cnt & 0xff;
	buff[13] = (block_cnt >> 8) & 0xff;
	buff[14] = (block_cnt >> 16) & 0xff;
	buff[15] = (block_cnt >> 24) & 0xff;

	/* data size */
	buff[4] = g_lcd_tcon_spi_cnt & 0xff;
	buff[5] = (g_lcd_tcon_spi_cnt >> 8) & 0xff;
	buff[6] = (g_lcd_tcon_spi_cnt >> 16) & 0xff;
	buff[7] = (g_lcd_tcon_spi_cnt >> 24) & 0xff;

	/* crc */
	temp = cal_CRC32(0, (buff + 4), g_lcd_tcon_spi_cnt - 4);
	buff[0] = temp & 0xff;
	buff[1] = (temp >> 8) & 0xff;
	buff[2] = (temp >> 16) & 0xff;
	buff[3] = (temp >> 24) & 0xff;

	return 0;
}

static int handle_tcon_spi(unsigned char *buff)
{
	unsigned char *p;
	const char *ini_value = NULL;
	char str[30];
	unsigned int data_size, block_cnt, param_cnt;
	unsigned int temp, i, j, k, n;

	/* header */
	/* version */
	ini_value = ini_get_string("tcon_spi_Attr", "version", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	if (temp == 0) {
		handle_tcon_spi_v0(buff);
		return 0;
	}

	/* new data format */
	/* version */
	buff[8] = temp & 0xff;
	buff[9] = (temp >> 8) & 0xff;

	/* block cnt */
	ini_value = ini_get_string("tcon_spi_Attr", "block_cnt", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, block_cnt is (%s)\n", __func__, ini_value);
	block_cnt = strtoul(ini_value, NULL, 0);
	buff[14] = block_cnt & 0xff;
	buff[15] = (block_cnt >> 8) & 0xff;

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
	buff[4] = data_size & 0xff;
	buff[5] = (data_size >> 8) & 0xff;
	buff[6] = (data_size >> 16) & 0xff;
	buff[7] = (data_size >> 24) & 0xff;
	g_lcd_tcon_spi_cnt = data_size;

	/* crc */
	temp = cal_CRC32(0, (buff + 4), g_lcd_tcon_spi_cnt - 4);
	buff[0] = temp & 0xff;
	buff[1] = (temp >> 8) & 0xff;
	buff[2] = (temp >> 16) & 0xff;
	buff[3] = (temp >> 24) & 0xff;

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
	unsigned int version, header;
	int tmp_len, i, j, ret;

	/* version */
	ini_value = ini_get_string("tcon_Path", "version", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	version = strtoul(ini_value, NULL, 0);

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

	handle_tcon_path_default(version);
	ret = handle_tcon_path_resv_for_kernel(version);
	if (ret) {
		if (g_lcd_tcon_bin_path_resv_mem && g_lcd_tcon_bin_path_mem) {
			memcpy(g_lcd_tcon_bin_path_resv_mem,
			       g_lcd_tcon_bin_path_mem,
			       CC_MAX_TCON_BIN_PATH_SIZE);
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, tcon bin path kernel same as uboot\n", __func__);
		}
	}

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
