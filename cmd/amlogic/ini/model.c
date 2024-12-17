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

#ifndef CONFIG_YOCTO
#define DEFAULT_MODEL_SUM_PATH1 "/odm/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/odm/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/odm/etc/tvconfig/model/model2_sum.ini"
#else
#define DEFAULT_MODEL_SUM_PATH1 "/vendor/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/vendor/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/vendor/etc/tvconfig/model/model2_sum.ini"
#endif
#define DEFAULT_MODEL_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model2_sum.ini"
#define AML_START		"amlogic_start"
#define AML_END			"amlogic_end"

int model_debug_flag;

#ifdef CONFIG_AML_LCD
struct dccd_base_info_s {
	unsigned int dccd;
	unsigned char minor_ver:4;
	unsigned char major_ver:4;
	unsigned char port_idx:4;
	unsigned char reserved0:2;
	unsigned char port_type:2;
	unsigned char capability1:4;
	unsigned char dev_type:4;
	unsigned char capability2;
	unsigned char capability3;
	unsigned char capability4;
	unsigned short len;  //all others info len
} __packed;

static int glcd_dcnt, glcd_ext_dcnt, gbl_dcnt, glcd_optical_dcnt;
static int g_lcd_pwr_on_seq_cnt, g_lcd_pwr_off_seq_cnt;
#ifdef CONFIG_AML_LCD_BL_LDIM
static int gldim_dev_dcnt, g_ldim_dev_init_on_cnt, g_ldim_dev_init_off_cnt;
static unsigned int g_ldim_dev_valid;
#endif
static int glcd_ext_init_on_cnt, glcd_ext_init_off_cnt, glcd_ext_cmd_size;
static struct lcd_ext_attr_s *lcd_ext_attr;
static unsigned int g_lcd_if, g_lcd_tcon_valid;
static struct dccd_info_s dccd_info;

static unsigned char *glcd_panel_file[3] = {NULL, NULL, NULL};
static int glcd_panel_file_size[3] = {0, 0, 0};
static unsigned char glcd_panel_file_type[3] = {0, 0, 0};

#define PANEL_PARAM_MEM_RSVD_SIZE CC_MAX_PANEL_ALL_DATA_SIZE
#define PANEL_PARAM_KEY_NUM_MAX (64 - 1)
#define PANEL_PARAM_KEY_SIZE (64)
#define PANEL_PARAM_HEAD_SIZE PANEL_PARAM_KEY_SIZE
#define PANEL_PARAM_KEY_NAME_SIZE (PANEL_PARAM_KEY_SIZE - 8)

#define PANEL_PARAM_KEY_MEM_OFST (PANEL_PARAM_KEY_NUM_MAX * PANEL_PARAM_KEY_SIZE +\
	PANEL_PARAM_HEAD_SIZE)

struct panel_param_key_s {
	unsigned int size;
	unsigned int mem_pos;
	char name[PANEL_PARAM_KEY_NAME_SIZE];
};

struct panel_param_head_s {
	unsigned int _crc32;
	unsigned int size;
	unsigned short key_cnt;
	unsigned short ukey_exist;
	unsigned char rsvd[PANEL_PARAM_HEAD_SIZE - 12];
};

struct panel_param_mem_s {
	unsigned int key_mem_pos;
	struct panel_param_head_s *head;
	unsigned char *mem;
	unsigned char *key_mem;
	struct panel_param_key_s *keys;
};

static struct panel_param_mem_s panel_param_mem = {0, NULL, NULL, NULL, NULL};
#endif

int trans_buffer_data(const char *data_str, unsigned int data_buf[])
{
	int item_ind = 0;
	char *token = NULL;
	char *psave = NULL;
	char *tmp_buf = NULL;

	if (!data_str)
		return 0;

	tmp_buf = (char *)malloc(CC_MAX_TEMP_BUF_SIZE);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
	strlcpy(tmp_buf, data_str, CC_MAX_TEMP_BUF_SIZE - 1);
	token = plat_strtok_r(tmp_buf, ",", &psave);
	while (token) {
		data_buf[item_ind] = strtoul(token, NULL, 0);
		item_ind++;
		token = plat_strtok_r(NULL, ",", &psave);
	}

	free(tmp_buf);
	tmp_buf = NULL;

	return item_ind;
}

#ifdef CONFIG_AML_LCD
static void mem_dump(unsigned char *addr, int size)
{
	int i = 0, j = 0, len = 0;
	char buf[128];

	for (j = 0; j < (size >> 4); j++) {
		for (i = 0, len = 0; i < 16; i++)
			len += sprintf(buf + len, "%02x ", (unsigned int)addr[j * 16 + i]);
		printf("0x%04x: %s\n", j * 16,  buf);
	}
	if (size & 0xf) {
		for (i = 0, len = 0; i < (size & 0xf); i++)
			len += sprintf(buf + len, "%02x ", (unsigned int)addr[j * 16 + i]);
		printf("0x%04x: %s\n", j * 16,  buf);
	}
}

void panel_param_mem_dump(void)
{
	int i = 0;
	struct panel_param_key_s *key;

	if (!panel_param_mem.mem || !panel_param_mem.head->key_cnt)
		return;

	printf("\npanel param dump: key_cnt:%d\n", panel_param_mem.head->key_cnt);
	for (i = 0; i < panel_param_mem.head->key_cnt; i++) {
		key = &panel_param_mem.keys[i];
		printf("[%02d]: size:0x%x, mem_ofst:0x%x, name:%s\n",
		       i, key->size, key->mem_pos, key->name);

		mem_dump(panel_param_mem.key_mem + key->mem_pos, key->size);
		printf("\n");
	}
}

unsigned char *get_panel_param_mem(void)
{
	if (panel_param_mem.mem && panel_param_mem.head->key_cnt)
		return panel_param_mem.mem;

	return NULL;
}

int is_panel_param_mem_ok(void)
{
	return (panel_param_mem.mem && panel_param_mem.head->key_cnt) ? 1 : 0;
}

int is_ukey_in_param_mem(void)
{
	return (panel_param_mem.head && panel_param_mem.head->ukey_exist) ? 1 : 0;
}

void panel_param_mem_set_ukey_flag(void)
{
	panel_param_mem.head->ukey_exist = 1;
}

/*head(64byte)|keys(64 * N)......|key_mems......*/
int panel_param_mem_put(unsigned char *mem, const char *name, u32 len)
{
	struct panel_param_key_s *key;

	if (!panel_param_mem.mem) {
		panel_param_mem.mem = (unsigned char *)malloc(PANEL_PARAM_MEM_RSVD_SIZE);
		if (panel_param_mem.mem) {
			memset(panel_param_mem.mem, 0, PANEL_PARAM_MEM_RSVD_SIZE);
			panel_param_mem.head = (struct panel_param_head_s *)panel_param_mem.mem;
			panel_param_mem.keys = (struct panel_param_key_s *)(panel_param_mem.mem +
				PANEL_PARAM_HEAD_SIZE);
			panel_param_mem.key_mem = (unsigned char *)(panel_param_mem.mem +
				PANEL_PARAM_KEY_MEM_OFST);
			panel_param_mem.head->size = PANEL_PARAM_KEY_MEM_OFST;
			panel_param_mem.key_mem_pos = 0;
		} else {
			printf("%s, no memory alloc\n", __func__);
			return -1;
		}
	}

	key = &panel_param_mem.keys[panel_param_mem.head->key_cnt];
	key->size = len;
	key->mem_pos = panel_param_mem.key_mem_pos;
	if (key->mem_pos + key->size > PANEL_PARAM_MEM_RSVD_SIZE - PANEL_PARAM_KEY_MEM_OFST) {
		printf("%s, memory not enough\n", __func__);
		return -1;
	}
	strlcpy(key->name, name, sizeof(key->name));
	memcpy(panel_param_mem.key_mem + key->mem_pos, mem, len);
	panel_param_mem.key_mem_pos += len;
	panel_param_mem.key_mem_pos = ALIGN(panel_param_mem.key_mem_pos, 16);
	panel_param_mem.head->key_cnt++;
	panel_param_mem.head->size = panel_param_mem.key_mem_pos + PANEL_PARAM_KEY_MEM_OFST;

	return 0;
}

unsigned char *panel_param_mem_get(const char *name, u32 *len)
{
	int i = 0;
	struct panel_param_key_s *key;

	if (!panel_param_mem.key_mem || !panel_param_mem.head->key_cnt)
		return NULL;

	for (i = 0; i < panel_param_mem.head->key_cnt; i++) {
		key = &panel_param_mem.keys[i];
		if (strncmp(key->name, name, sizeof(key->name)) == 0) {
			*len = key->size;
			return panel_param_mem.key_mem + key->mem_pos;
		}
	}

	return NULL;
}

int panel_param_mem_modify(unsigned char *mem, const char *name, u32 len)
{
	struct panel_param_key_s *key;
	unsigned int _crc32;
	int ret = 0, i = 0;

	if (!mem || !panel_param_mem.mem || !panel_param_mem.head->key_cnt)
		return -1;

	for (i = 0; i < panel_param_mem.head->key_cnt; i++) {
		key = &panel_param_mem.keys[i];
		if (strncmp(key->name, name, sizeof(key->name)) == 0) {
			if (len < key->size) {
				memset(panel_param_mem.key_mem + key->mem_pos, 0, key->size);
				memcpy(panel_param_mem.key_mem + key->mem_pos, mem, len);
				key->size = len;
			} else {
				//once for all, we don't care about this memory
				memset(panel_param_mem.key_mem + key->mem_pos, 0, key->size);
				memset(key, 0, PANEL_PARAM_KEY_SIZE);
				ret = panel_param_mem_put(mem, name, len);
			}
			if (ret == 0) {
				_crc32 = cal_CRC32(0, panel_param_mem.mem + 4,
						   panel_param_mem.head->size - 4);
				panel_param_mem.head->_crc32 = _crc32;
			}
			return ret;
		}
	}

	ret = panel_param_mem_put(mem, name, len);
	if (ret == 0) {
		_crc32 = cal_CRC32(0, panel_param_mem.mem + 4, panel_param_mem.head->size - 4);
		panel_param_mem.head->_crc32 = _crc32;
	}
	return ret;
}

unsigned char get_lcd_panel_file_type(int index)
{
	return index < 3 ? glcd_panel_file_type[index] : 0;
}

void set_lcd_panel_file_type(int index, unsigned char type)
{
	glcd_panel_file_type[index] = type;
}

unsigned char *read_file_to_buffer(const char *filename, int *size);

unsigned char *get_panel_file(int index, int *len)
{
	if (len)
		*len = glcd_panel_file_size[index];

	return glcd_panel_file[index];
}

int read_panel_file(int index, const char *filename)
{
	unsigned char *fil;
	int size;

	fil = read_file_to_buffer(filename, &size);
	if (fil && size > 0) {
		glcd_panel_file[index] = fil;
		glcd_panel_file_size[index] = size;
		return 0;
	}
	ALOGE("read panel file:%s fail", filename);
	return -1;
}

void rm_panel_file(int index)
{
	if (glcd_panel_file[index])
		free(glcd_panel_file[index]);
	glcd_panel_file[index] = NULL;
	glcd_panel_file_size[index] = 0;
}

int check_param_valid(int mode, int parse_len, unsigned char parse_buf[],
		      int ori_len, unsigned char ori_buf[])
{
	unsigned int ori_cal_crc32 = 0, parse_cal_crc32 = 0;
	struct lcd_header_s head, head2;
	unsigned char *p;

	if (mode == 0) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_hex_data_have_header_valid(&parse_cal_crc32, CC_MAX_DATA_SIZE, parse_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_hex_data_have_header_valid(&ori_cal_crc32, CC_MAX_DATA_SIZE, ori_len, ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	} else if (mode == 1) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_hex_data_no_header_valid(&parse_cal_crc32, CC_MAX_DATA_SIZE, parse_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_hex_data_no_header_valid(&ori_cal_crc32, CC_MAX_DATA_SIZE, ori_len, ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	} else if (mode == 2) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		memcpy((void *)&head, (void *)parse_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&parse_cal_crc32, CC_MAX_DATA_SIZE,
		    head.data_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		memcpy((void *)&head2, (void *)ori_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&ori_cal_crc32, CC_MAX_DATA_SIZE, head2.data_len,
		    ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32)
		//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n",
			//__func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		p = parse_buf + head.data_len;
		memcpy((void *)&head, (void *)p, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&parse_cal_crc32, CC_MAX_DATA_SIZE, head.data_len, p) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		p = parse_buf + head2.data_len;
		memcpy((void *)&head2, (void *)ori_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&ori_cal_crc32, CC_MAX_DATA_SIZE, head2.data_len, p) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32)
		//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n",
			//__func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
	} else {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_string_data_have_header_valid(&parse_cal_crc32, (char *)parse_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_string_data_have_header_valid(&ori_cal_crc32, (char *)ori_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	}

	//ALOGD("%s, param check ok!\n", __func__);
	return CC_PARAM_CHECK_OK;
}

static int handle_integrity_flag(void)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("start", "start_tag", "null");
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s, start_tag is (%s)\n", __func__, ini_value);
	if (strncasecmp(ini_value, AML_START, strlen(AML_START))) {
		ALOGE("%s, start_tag (%s) is error!!!\n", __func__, ini_value);
		return -1;
	}

	ini_value = ini_get_string("end", "end_tag", "null");
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s, end_tag is (%s)\n", __func__, ini_value);
	if (strncasecmp(ini_value, AML_END, strlen(AML_END))) {
		ALOGE("%s, end_tag (%s) is error!!!\n", __func__, ini_value);
		return -1;
	}

	return 0;
}

int handle_read_bin_file(const char *file_name, unsigned long max_len)
{
	int size;

	bin_file_init();

	size = read_bin_file(file_name);
	if (size < 0) {
		ALOGE("%s, load bin file error!\n", __func__);
		bin_file_uninit();
		return 0;
	}

	if (size > max_len) {
		ALOGE("%s, bin file size out of support!\n", __func__);
		bin_file_uninit();
		return 0;
	}

	return size;
}

static int handle_lcd_basic(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int config_chk;
	unsigned int bits, cfmt;

	ini_value = ini_get_string("lcd_Attr", "model_name", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, model_name is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->basic.model_name, ini_value, CC_LCD_NAME_LEN_MAX - 1);
	p_attr->basic.model_name[CC_LCD_NAME_LEN_MAX - 1] = '\0';

	ini_value = ini_get_string("lcd_Attr", "interface", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, interface is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LCD_RGB") == 0)
		g_lcd_if = LCD_RGB;
	else if (strcmp(ini_value, "LCD_LVDS") == 0)
		g_lcd_if = LCD_LVDS;
	else if (strcmp(ini_value, "LCD_VBYONE") == 0)
		g_lcd_if = LCD_VBYONE;
	else if (strcmp(ini_value, "LCD_MIPI") == 0)
		g_lcd_if = LCD_MIPI;
	else if (strcmp(ini_value, "LCD_MLVDS") == 0)
		g_lcd_if = LCD_MLVDS;
	else if (strcmp(ini_value, "LCD_P2P") == 0)
		g_lcd_if = LCD_P2P;
	else if (strcmp(ini_value, "LCD_EDP") == 0)
		g_lcd_if = LCD_EDP;
	else if (strcmp(ini_value, "LCD_BT656") == 0)
		g_lcd_if = LCD_BT656;
	else if (strcmp(ini_value, "LCD_BT1120") == 0)
		g_lcd_if = LCD_BT1120;
	else
		g_lcd_if = LCD_TYPE_MAX;

	ini_value = ini_get_string("lcd_Attr", "config_check", "none");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, config_check is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		config_chk = 0;
	else
		config_chk = strtoul(ini_value, NULL, 0) ? 0x3 : 0x2;
	p_attr->basic.lcd_if_chk = (g_lcd_if & 0x3f) | ((config_chk & 0x3) << 6);

	ini_value = ini_get_string("lcd_Attr", "lcd_bits", "10");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, lcd_bits is (%s)\n", __func__, ini_value);
	bits = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "cmft_in", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, cmft_in is (%s)\n", __func__, ini_value);
	cfmt = strtoul(ini_value, NULL, 0);
	p_attr->basic.lcd_bits_cfmt = ((cfmt & 0x3) << 6) | (bits & 0x3f);

	ini_value = ini_get_string("lcd_Attr", "screen_width", "16");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, screen_width is (%s)\n", __func__, ini_value);
	p_attr->basic.screen_width = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "screen_height", "9");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, screen_height is (%s)\n", __func__, ini_value);
	p_attr->basic.screen_height = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_timming(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int width, pol;

	ini_value = ini_get_string("lcd_Attr", "h_active", "1920");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_active is (%s)\n", __func__, ini_value);
	p_attr->timming.h_active = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "v_active", "1080");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_active is (%s)\n", __func__, ini_value);
	p_attr->timming.v_active = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "h_period", "2200");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period is (%s)\n", __func__, ini_value);
	p_attr->timming.h_period = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "v_period", "1125");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period is (%s)\n", __func__, ini_value);
	p_attr->timming.v_period = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "hsync_width", "44");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_width is (%s)\n", __func__, ini_value);
	width = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "hsync_bp", "148");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_bp is (%s)\n", __func__, ini_value);
	p_attr->timming.hsync_bp = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "hsync_pol", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_pol is (%s)\n", __func__, ini_value);
	pol = strtoul(ini_value, NULL, 0);
	p_attr->timming.hsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

	ini_value = ini_get_string("lcd_Attr", "vsync_width", "5");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_width is (%s)\n", __func__, ini_value);
	width = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vsync_bp", "30");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_bp is (%s)\n", __func__, ini_value);
	p_attr->timming.vsync_bp = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vsync_pol", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_pol is (%s)\n", __func__, ini_value);
	pol = strtoul(ini_value, NULL, 0);
	p_attr->timming.vsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

	ini_value = ini_get_string("lcd_Attr", "pre_de_h", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pre_de_h is (%s)\n", __func__, ini_value);
	p_attr->timming.pre_de_h = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "pre_de_v", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pre_de_v is (%s)\n", __func__, ini_value);
	p_attr->timming.pre_de_v = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_customer(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned char clk_auto, clk_mode, ppc, custom_pinmux;

	ini_value = ini_get_string("lcd_Attr", "fr_adjust_type", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, fr_adjust_type is (%s)\n", __func__, ini_value);
	p_attr->customer.fr_adjust_type = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "ss_level", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, ss_level is (%s)\n", __func__, ini_value);
	p_attr->customer.ss_level = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "clk_auto_gen", "1");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, clk_auto_gen is (%s)\n", __func__, ini_value);
	clk_auto = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "clk_mode", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, clk_mode is (%s)\n", __func__, ini_value);
	clk_mode = strtoul(ini_value, NULL, 0);
	p_attr->customer.custom_val0 = ((clk_mode & 0xf) << 4) | (clk_auto & 0xf);

	ini_value = ini_get_string("lcd_Attr", "pixel_clk", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "h_period_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period_min is (%s)\n", __func__, ini_value);
	p_attr->customer.h_period_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "h_period_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period_max is (%s)\n", __func__, ini_value);
	p_attr->customer.h_period_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "v_period_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period_min is (%s)\n", __func__, ini_value);
	p_attr->customer.v_period_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "v_period_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period_max is (%s)\n", __func__, ini_value);
	p_attr->customer.v_period_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "pixel_clk_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk_min is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "pixel_clk_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk_max is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vlock_val_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_0 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vlock_val_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_1 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vlock_val_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_2 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "vlock_val_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_3 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "custom_pinmux", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, custom_pinmux is (%s)\n", __func__, ini_value);
	custom_pinmux = strtoul(ini_value, NULL, 0);
	if (custom_pinmux == 0) {
		ini_value = ini_get_string("lcd_Attr", "customer_value_9", "0");
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, customer_value_9 is (%s)\n", __func__, ini_value);
		custom_pinmux = strtoul(ini_value, NULL, 0);
	}

	ini_value = ini_get_string("lcd_Attr", "ppc_mode", "1");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, ppc_mode is (%s)\n", __func__, ini_value);
	ppc = strtoul(ini_value, NULL, 0);
	p_attr->customer.custom_val1 = ((ppc & 0xf) << 4) | (custom_pinmux & 0xf);

	ini_value = ini_get_string("lcd_Attr", "fr_auto_custom", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, fr_auto_custom is (%s)\n", __func__, ini_value);
	p_attr->customer.fr_auto_cus = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "frame_rate_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, frame_rate_min is (%s)\n", __func__, ini_value);
	p_attr->customer.frame_rate_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "frame_rate_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, frame_rate_max is (%s)\n", __func__, ini_value);
	p_attr->customer.frame_rate_max = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_interface(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("lcd_Attr", "if_attr_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_4", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_5", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_6", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_7", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_8", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "if_attr_9", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_9 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_pwr(struct lcd_attr_s *p_attr)
{
	int i = 0, tmp_cnt = 0, tmp_base_ind = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[1024];

	ini_value = ini_get_string("lcd_Attr", "power_on_step", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, power_on_step is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	g_lcd_pwr_on_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
	for (i = 0; i < g_lcd_pwr_on_seq_cnt; i++) {
		tmp_base_ind = i * CC_LCD_PWR_ITEM_CNT;
		p_attr->pwr[i].pwr_step_type = tmp_buf[tmp_base_ind + 0];
		p_attr->pwr[i].pwr_step_index = tmp_buf[tmp_base_ind + 1];
		p_attr->pwr[i].pwr_step_val = tmp_buf[tmp_base_ind + 2];
		p_attr->pwr[i].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
	}

	ini_value = ini_get_string("lcd_Attr", "power_off_step", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, power_off_step is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	g_lcd_pwr_off_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
	for (i = 0; i < g_lcd_pwr_off_seq_cnt; i++) {
		tmp_base_ind = i * CC_LCD_PWR_ITEM_CNT;
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type = tmp_buf[tmp_base_ind + 0];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index = tmp_buf[tmp_base_ind + 1];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val = tmp_buf[tmp_base_ind + 2];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
	}

	return 0;
}

static int handle_lcd_header(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int size = 0;

	size += sizeof(struct lcd_header_s);
	size += sizeof(struct lcd_basic_s);
	size += sizeof(struct lcd_timming_s);
	size += sizeof(struct lcd_customer_s);
	size += sizeof(struct lcd_interface_s);

	size += sizeof(struct lcd_pwr_s) * g_lcd_pwr_on_seq_cnt;
	size += sizeof(struct lcd_pwr_s) * g_lcd_pwr_off_seq_cnt;

	//p_attr->head.data_len = size;; //total data len will handle after all parameters parsed
	p_attr->head.block_cur_size = size;

	ini_value = ini_get_string("lcd_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	//p_attr->head.rev = 0;
	if (p_attr->head.version >= 2)
		p_attr->head.block_next_flag = 1;

	return 0;
}

static int handle_lcd_phy(unsigned char *p_attr)
{
	struct lcd_phy_s *phy;
	const char *ini_value = NULL;
	unsigned int temp_buf[72];
	unsigned int offset, lane_cnt = 0;
	int i, j, n;

	offset = sizeof(struct lcd_header_s);
	phy = (struct lcd_phy_s *)(p_attr + offset);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_flag", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_flag is (%s)\n", __func__, ini_value);
	phy->phy_attr_flag = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_0 is (%s)\n", __func__, ini_value);
	phy->phy_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_1 is (%s)\n", __func__, ini_value);
	phy->phy_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_2 is (%s)\n", __func__, ini_value);
	phy->phy_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_3 is (%s)\n", __func__, ini_value);
	phy->phy_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_4", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_4 is (%s)\n", __func__, ini_value);
	phy->phy_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_5", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_5 is (%s)\n", __func__, ini_value);
	phy->phy_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_6", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_6 is (%s)\n", __func__, ini_value);
	phy->phy_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_7", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_7 is (%s)\n", __func__, ini_value);
	phy->phy_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_8", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_8 is (%s)\n", __func__, ini_value);
	phy->phy_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_9", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_9 is (%s)\n", __func__, ini_value);
	phy->phy_attr_9 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_10", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_10 is (%s)\n", __func__, ini_value);
	phy->phy_attr_10 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_attr_11", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_11 is (%s)\n", __func__, ini_value);
	phy->phy_attr_11 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_Attr", "phy_lane_pn_swap", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_pn_swap is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null")) {
		lane_cnt = trans_buffer_data(ini_value, temp_buf);
		for (i = 0; i < lane_cnt; i++) {
			j = i / 8;
			n = i % 8;
			phy->phy_lane_pn_swap[j] = ((temp_buf[i] ? 1 : 0) << n);
		}
	}

	ini_value = ini_get_string("lcd_Attr", "phy_lane_ctrl", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_ctrl is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null")) {
		lane_cnt = trans_buffer_data(ini_value, temp_buf);
		for (i = 0; i < lane_cnt; i++) {
			phy->phy_lane_ctrl[i] = temp_buf[i];
			if (model_debug_flag & DEBUG_LCD) {
				ALOGD("%s, phy_lane_ctrl[%d] is (0x%x)\n", __func__,
				      i, phy->phy_lane_ctrl[i]);
			}
		}
	}

	ini_value = ini_get_string("lcd_Attr", "phy_lane_sel", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_sel is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null")) {
		lane_cnt = trans_buffer_data(ini_value, temp_buf);
		for (i = 0; i < lane_cnt; i++)
			phy->phy_lane_sel[i] = temp_buf[i];
	}

	if (model_debug_flag & DEBUG_LCD) {
		ALOGD("%s, phy_attr_flag: 0x%x\n", __func__, phy->phy_attr_flag);
		ALOGD("%s, vswing:0x%x, vcm:0x%x, ref_bias:0x%x, odt:0x%x, cv_mode:0x%x\n",
		      __func__, phy->phy_attr_0, phy->phy_attr_1,
		      phy->phy_attr_2, phy->phy_attr_3, phy->phy_attr_4);
		for (i = 0; i < lane_cnt; i++) {
			ALOGD("%s, lane[%d]: ctrl:0x%x, sel:0x%x\n",
			      __func__, i, phy->phy_lane_ctrl[i], phy->phy_lane_sel[i]);
		}
	}

	return 0;
}

static void handle_lcd_v2_header(struct lcd_header_s *header)
{
	unsigned int data_cnt;

	if (!header)
		return;

	data_cnt = 0;
	data_cnt += sizeof(struct lcd_header_s);
	data_cnt += sizeof(struct lcd_phy_s);
	data_cnt += glcd_cus_ctrl_cnt;

	header->crc32 = 0xffffffff;
	header->data_len = 0;
	header->version = 2;
	header->block_next_flag = 0;
	header->block_cur_size = data_cnt;
}

static void handle_lcd_v3_header(struct lcd_header_s *header)
{
	unsigned int data_cnt;

	if (!header)
		return;

	data_cnt = 0;
	data_cnt += sizeof(struct lcd_header_s);
	data_cnt += glcd_cus_ctrl_cnt;

	header->crc32 = 0xffffffff;
	header->data_len = 0;
	header->version = 3;
	header->block_next_flag = 0;
	header->block_cur_size = data_cnt;
}

void *handle_lcd_ext_buf_get(void)
{
	return (void *)lcd_ext_attr;
}

struct dccd_info_s *get_dccd_info(void)
{
	return &dccd_info;
}

unsigned int get_dccd_crc(void)
{
	return dccd_info.checksum;
}

unsigned int is_support_dccd(void)
{
	return dccd_info.is_dccd;
}

unsigned int dccd_has_tcon_file(void)
{
	return dccd_info.has_tcon_file;
}

static int handle_lcd_ext_basic(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("lcd_ext_Attr", "ext_name", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_name is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->basic.ext_name, ini_value, CC_LCD_EXT_NAME_LEN_MAX - 1);
	p_attr->basic.ext_name[CC_LCD_EXT_NAME_LEN_MAX - 1] = '\0';

	ini_value = ini_get_string("lcd_ext_Attr", "ext_index", "0xff");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_index is (%s)\n", __func__, ini_value);
	p_attr->basic.ext_index = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "ext_type", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_type is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LCD_EXTERN_I2C") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_I2C;
	else if (strcmp(ini_value, "LCD_EXTERN_SPI") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_SPI;
	else if (strcmp(ini_value, "LCD_EXTERN_MIPI") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_MIPI;
	else if (strcmp(ini_value, "LCD_EXTERN_SIMPLE") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_SIMPLE;
	else
		p_attr->basic.ext_type = LCD_EXTERN_MAX;

	ini_value = ini_get_string("lcd_ext_Attr", "ext_status", "0");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_status is (%s)\n", __func__, ini_value);
	p_attr->basic.ext_status = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_ext_type(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("lcd_ext_Attr", "value_0", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_0 is (%s)\n", __func__, ini_value);
	p_attr->type.value_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_1", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_1 is (%s)\n", __func__, ini_value);
	p_attr->type.value_1 = strtoul(ini_value, NULL, 0);

	if (p_attr->basic.ext_type == LCD_EXTERN_I2C)
		p_attr->type.value_2 = LCD_EXTERN_I2C_BUS_INVALID;
	else {
		ini_value = ini_get_string("lcd_ext_Attr", "value_2", "null");
		if (model_debug_flag & DEBUG_LCD_EXTERN)
			ALOGD("%s, value_2 is (%s)\n", __func__, ini_value);
		p_attr->type.value_2 = strtoul(ini_value, NULL, 0);
	}

	ini_value = ini_get_string("lcd_ext_Attr", "value_3", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_3 is (%s)\n", __func__, ini_value);
	p_attr->type.value_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_4", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_4 is (%s)\n", __func__, ini_value);
	p_attr->type.value_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_5", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_5 is (%s)\n", __func__, ini_value);
	p_attr->type.value_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_6", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_6 is (%s)\n", __func__, ini_value);
	p_attr->type.value_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_7", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_7 is (%s)\n", __func__, ini_value);
	p_attr->type.value_7 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_8", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_8 is (%s)\n", __func__, ini_value);
	p_attr->type.value_8 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_ext_Attr", "value_9", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_9 is (%s)\n", __func__, ini_value);
	p_attr->type.value_9 = strtoul(ini_value, NULL, 0);

	if (p_attr->basic.ext_type == LCD_EXTERN_I2C)
		glcd_ext_cmd_size = p_attr->type.value_3;
	else if (p_attr->basic.ext_type == LCD_EXTERN_SPI)
		glcd_ext_cmd_size = p_attr->type.value_6;
	else
		glcd_ext_cmd_size = p_attr->type.value_9;

	return 0;
}

#ifdef CONFIG_AML_LCD_TCON
static int handle_tcon_ext_bin_data(int index, int type, unsigned char *buf,
				    unsigned int offset, unsigned int data_len,
				    int multi_flag, int multi_id)
{
	char *file_name, str[2][50];
	unsigned int data_size = 0, bin_index, i, file_find = 0;
	unsigned char *bin_buf = NULL;

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}
	buf[0] = 0; /* init invalid data */
	i = 0;

	if (index >= 4) {
		ALOGE("%s, invalid index %d\n", __func__, index);
		return -1;
	}

	if (multi_flag) {
		sprintf(str[0], "model_tcon_ext_b%d_%d_spi", index, multi_id);
		sprintf(str[1], "model_tcon_ext_b%d_%d", index, multi_id);
	} else {
		sprintf(str[0], "model_tcon_ext_b%d_spi", index);
		sprintf(str[1], "model_tcon_ext_b%d", index);
	}

	while (i < 2) {
		file_name = env_get(str[i]);
		if (!file_name) {
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGD("%s: no %s path\n", __func__, str[i]);
		} else {
			if (ini_is_file_exist(file_name)) {
				if (model_debug_flag & DEBUG_NORMAL)
					ALOGD("%s: %s: %s\n", __func__, str[i], file_name);
				file_find = 1;
				bin_index = i;
				break;
			}
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGE("%s: %s: \"%s\" not exist.\n", __func__, str[i], file_name);
		}
		i++;
	}
	if (file_find == 0)
		return -1;

	data_size = handle_read_bin_file(file_name, LCD_EXTERN_INIT_ON_MAX);
	if (data_size == 0) {
		ALOGE("%s, %s data_size %d error!\n", __func__, str[bin_index], data_size);
		return -1;
	}
	if (data_size > LCD_EXTERN_INIT_ON_MAX) {
		ALOGE("%s, %s data_size %d out of support(max %d)!\n",
		      __func__, str[bin_index], data_size, LCD_EXTERN_INIT_ON_MAX);
		bin_file_uninit();
		return -1;
	}

	switch (type) {
	case LCD_EXT_CMD_TYPE_CMD_BIN_DATA: /* all data replace, reg_addr nonexistent */
		buf[0] = data_size;
		get_bin_data(&buf[1], data_size);
		break;
	case LCD_EXT_CMD_TYPE_CMD_BIN: /* data with reg_addr auto fill 0x0 */
		buf[0] = (data_size + 1); /* data size include reg_addr */
		buf[1] = 0x00;            /* reg_addr */
		get_bin_data(&buf[2], data_size);
		break;
	case LCD_EXT_CMD_TYPE_CMD_BIN2: /* data with reg_addr, only replace i2c_data */
		if (data_size < (offset + data_len - 1)) {
			ALOGE("%s, %s suspend size %d out of data_size(%d)!\n",
			      __func__, str[bin_index], (offset + data_len - 1), data_size);
			bin_file_uninit();
			return -1;
		}

		bin_buf = (unsigned char *)malloc(data_size);
		if (!bin_buf) {
			ALOGE("%s, malloc buffer memory error!!!\n", __func__);
			bin_file_uninit();
			return -1;
		}
		buf[0] = data_len;
		buf[1] = offset;
		get_bin_data(bin_buf, data_size);
		memcpy(&buf[2], &bin_buf[offset], data_len - 1);
		free(bin_buf);
		break;
	default:
		break;
	}

	if (model_debug_flag & DEBUG_LCD_EXTERN) {
		ALOGD("%s: %s:\n", __func__, str[bin_index]);
		for (i = 0; i < (buf[0] + 1); i++)
			printf(" 0x%02x", buf[i]);
		printf("\n");
	}

	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s %s finish\n", __func__, str[bin_index]);

	bin_file_uninit();

	return 0;
}

static int handle_lcd_ext_cmd_bin_load(unsigned char type,
				       unsigned int *raw_buf, int raw_size,
				       unsigned char *dest_buf, int dest_size)
{
	unsigned char next_type, multi_flag;
	unsigned int index, multi_id;
	unsigned int offset = 0, data_len = 0;
	unsigned char *data_buf;
	int ret = -1;

	if (type == LCD_EXT_CMD_TYPE_MULTI_CMD ||
	    type == LCD_EXT_CMD_TYPE_MULTI_DFT_CMD) {
		multi_flag = 1;
		next_type = raw_buf[1];
	} else {
		if (type == LCD_EXT_CMD_TYPE_CMD_MULTI ||
		    type == LCD_EXT_CMD_TYPE_CMD2_MULTI ||
		    type == LCD_EXT_CMD_TYPE_CMD3_MULTI ||
		    type == LCD_EXT_CMD_TYPE_CMD4_MULTI) {
			multi_flag = 1;
			next_type = ((raw_buf[1] << 4) | (type & 0xf));
		} else {
			multi_flag = 0;
			next_type = type;
		}
	}

	switch (next_type & 0xf0) {
	case LCD_EXT_CMD_TYPE_CMD_BIN2:
	case LCD_EXT_CMD_TYPE_CMD_BIN:
	case LCD_EXT_CMD_TYPE_CMD_BIN_DATA:
		break;
	case LCD_EXT_CMD_TYPE_CMD:
	default:
		return -1;
	}

	index = next_type & 0xf;
	if (multi_flag) {
		data_len = raw_size - 2; //id,next_cmd
		offset = raw_buf[2];
		multi_id = raw_buf[0];
	} else {
		data_len = raw_size;
		offset = raw_buf[0];
		multi_id = 0xff;
	}
	if (data_len > dest_size) {
		ALOGE("%s, data_len %d over size!\n", __func__, data_len);
		return -1;
	}
	memset(dest_buf, 0, dest_size);

	//save multi_flag for cmd data different replace method
	dest_buf[0] = multi_flag;
	data_buf = &dest_buf[1];

	ret = handle_tcon_ext_bin_data(index, next_type, data_buf, offset, data_len,
				       multi_flag, multi_id);
	if (ret || data_buf[0] == 0) /* bin data size invalid */
		return -1;

	return 0;
}
#endif

static int handle_lcd_ext_special_parse(unsigned char type,
					unsigned int *raw_buf, int raw_size,
					unsigned char *dest_buf, int dest_size)
{
	unsigned char *data_buf;
	unsigned int temp_val = 0;
	int i = 0, j = 0;
	int ret = -1;

	if (!raw_buf || !dest_buf)
		return -1;

	switch (type) {
	case LCD_EXT_CMD_TYPE_MULTI_LIST_UFR: //2byte frame rate
		if (raw_size < 3)
			return -1;
		memset(dest_buf, 0, dest_size);
		data_buf = &dest_buf[1];
		for (i = 0; i < raw_size; i += 3) {
			if ((j + 5) > dest_size) {
				ALOGE("%s: data over size\n", __func__);
				break;
			}
			data_buf[j] = raw_buf[i]; //index
			//frame rate min
			data_buf[j + 1] = raw_buf[i + 1] & 0xff;
			data_buf[j + 2] = (raw_buf[i + 1] >> 8) & 0xff;
			//frame rate max
			data_buf[j + 3] = raw_buf[i + 2] & 0xff;
			data_buf[j + 4] = (raw_buf[i + 2] >> 8) & 0xff;
			j += 5;
		}
		dest_buf[0] = j; //data_size
		ret = 0;
		break;
	case LCD_EXT_CMD_TYPE_DELAY: //maybe 2byte delay
		memset(dest_buf, 0, dest_size);
		for (i = 0; i < raw_size; i++)
			temp_val += raw_buf[i];
		if (temp_val > 0xff) {
			dest_buf[0] = 2; //data_size
			dest_buf[1] = temp_val & 0xff;
			dest_buf[2] = (temp_val >> 4) & 0xff;
		} else {
			dest_buf[0] = 1; //data_size
			dest_buf[1] = temp_val;
		}
		ret = 0;
		break;
	case LCD_EXT_CMD_TYPE_WAIT_GPIO: //maybe 2byte delay
	case LCD_EXT_CMD_TYPE_GPIO: //maybe 2byte delay
		if (raw_size < 2)
			return -1;
		memset(dest_buf, 0, dest_size);
		dest_buf[1] = raw_buf[0]; //gpio index
		dest_buf[2] = raw_buf[1]; //gpio value
		if (raw_size >= 3) {
			temp_val = raw_buf[2];
			if (temp_val > 0xff) {
				dest_buf[0] = 4; //data_size
				dest_buf[3] = temp_val & 0xff;
				dest_buf[4] = (temp_val >> 4) & 0xff;
			} else {
				dest_buf[0] = 3; //data_size
				dest_buf[3] = temp_val;
			}
		} else {
			dest_buf[0] = 2; //data_size
		}
		ret = 0;
		break;
	default:
		break;
	}

	return ret;
}

static int handle_lcd_ext_cmd_data(struct lcd_ext_attr_s *p_attr)
{
	int i = 0, j = 0, k, tmp_cnt = 0, tmp_off = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[2048];
	unsigned char type, raw_size, data_size;
	unsigned char *data_buf = NULL, data_len;
#ifdef CONFIG_AML_LCD_TCON
	unsigned char multi_flag;
#endif
	int ret;

	/* original data in ini */
	ini_value = ini_get_string("lcd_ext_Attr", "init_on", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, init_on is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);

	data_buf = (unsigned char *)malloc(LCD_EXTERN_INIT_ON_MAX);
	if (!data_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(data_buf, 0, LCD_EXTERN_INIT_ON_MAX);

	/* init on */
	if (tmp_cnt > LCD_EXTERN_INIT_ON_MAX) {
		ALOGE("%s: invalid init_on data\n", __func__);
		p_attr->cmd_data[0] = LCD_EXT_CMD_TYPE_END;
		p_attr->cmd_data[1] = 0;
		glcd_ext_init_on_cnt = 2;
		goto handle_lcd_ext_cmd_data_init_off;
	}

	if (glcd_ext_cmd_size == 0xff) {
		i = 0;
		j = 0;
		while (i < tmp_cnt) {
			type = tmp_buf[i];
			raw_size = tmp_buf[i + 1];
			data_size = raw_size;
			p_attr->cmd_data[j] = type;
			if (type == LCD_EXT_CMD_TYPE_END) {
				p_attr->cmd_data[j + 1] = 0;
				j += 2;
				break;
			}

			//special parse
			ret = handle_lcd_ext_special_parse(type, &tmp_buf[i + 2], data_size,
							   data_buf, LCD_EXTERN_INIT_ON_MAX);
			if (ret == 0) {
				//buf[0]:data_len
				//buf[1..]:data
				data_len = data_buf[0];
				data_size = data_len;
				p_attr->cmd_data[j + 1] = data_size;
				memcpy(&p_attr->cmd_data[j + 2], &data_buf[1], data_len);
				goto handle_lcd_ext_cmd_data_next;
			}

#ifdef CONFIG_AML_LCD_TCON
			ret = handle_lcd_ext_cmd_bin_load(type, &tmp_buf[i + 2], data_size,
							  data_buf, LCD_EXTERN_INIT_ON_MAX);
			if (ret == 0) {
				//buf[0]:multi_flag
				//buf[1]:data_len
				//buf[2..]:data
				multi_flag = data_buf[0];
				data_len = data_buf[1];
				if (multi_flag) {
					data_size = data_len + 2;
					p_attr->cmd_data[j + 1] = data_size;
					p_attr->cmd_data[j + 2] = tmp_buf[i + 2];
					p_attr->cmd_data[j + 3] = tmp_buf[i + 3];
					memcpy(&p_attr->cmd_data[j + 4], &data_buf[2], data_len);
				} else {
					data_size = data_len;
					p_attr->cmd_data[j + 1] = data_size;
					memcpy(&p_attr->cmd_data[j + 2], &data_buf[2], data_len);
				}
				goto handle_lcd_ext_cmd_data_next;
			}
#endif

			/* original ini data */
			p_attr->cmd_data[j + 1] = data_size;
			for (k = 0; k < data_size; k++)
				p_attr->cmd_data[j + 2 + k] = (unsigned char)tmp_buf[i + 2 + k];

handle_lcd_ext_cmd_data_next:
			j += data_size + 2;
			i += raw_size + 2; /* raw data */
		}
		glcd_ext_init_on_cnt = j;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->cmd_data[i] = tmp_buf[i];
		glcd_ext_init_on_cnt = tmp_cnt;
	}

handle_lcd_ext_cmd_data_init_off:
	/* init off */
	tmp_off = glcd_ext_init_on_cnt;
	ini_value = ini_get_string("lcd_ext_Attr", "init_off", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, init_off is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	if (tmp_cnt > LCD_EXTERN_INIT_OFF_MAX) {
		ALOGE("%s: invalid init_off data\n", __func__);
		p_attr->cmd_data[tmp_off + 0] = LCD_EXT_CMD_TYPE_END;
		p_attr->cmd_data[tmp_off + 1] = 0;
		glcd_ext_init_on_cnt = 2;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->cmd_data[tmp_off + i] = tmp_buf[i];
		glcd_ext_init_off_cnt = tmp_cnt;
	}

	if (model_debug_flag & DEBUG_LCD_EXTERN) {
		ALOGD("%s, init_on_data:\n", __func__);
		for (i = 0; i < glcd_ext_init_on_cnt; i++)
			printf("  [%d] = 0x%02x\n", i, p_attr->cmd_data[i]);

		ALOGD("%s, init_off_data:\n", __func__);
		for (i = 0; i < glcd_ext_init_off_cnt; i++)
			ALOGD("  [%d] = 0x%02x\n", i, p_attr->cmd_data[tmp_off + i]);
	}

	memset(data_buf, 0, LCD_EXTERN_INIT_ON_MAX);
	free(data_buf);
	data_buf = NULL;

	return 0;
}

static int lcd_ext_data_to_buf(unsigned char tmp_buf[], struct lcd_ext_attr_s *p_attr)
{
	int i = 0;
	int tmp_len = 0, tmp_off = 0;

	tmp_off = 0;

	tmp_len = sizeof(struct lcd_header_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->head), tmp_len);
	tmp_off += tmp_len;

	tmp_len = sizeof(struct lcd_ext_basic_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->basic), tmp_len);
	tmp_off += tmp_len;

	tmp_len = sizeof(struct lcd_ext_type_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->type), tmp_len);
	tmp_off += tmp_len;

	tmp_len = glcd_ext_init_on_cnt;
	for (i = 0; i < glcd_ext_init_on_cnt; i++)
		tmp_buf[tmp_off + i] = p_attr->cmd_data[i];
	tmp_off += tmp_len;

	for (i = 0; i < glcd_ext_init_off_cnt; i++)
		tmp_buf[tmp_off + i] = p_attr->cmd_data[tmp_len+i];

	return 0;
}

static int handle_lcd_ext_header(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned char *tmp_buf = NULL;

	tmp_buf = (unsigned char *) malloc(CC_MAX_TEMP_BUF_SIZE);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	glcd_ext_dcnt = 0;
	glcd_ext_dcnt += sizeof(struct lcd_header_s);
	glcd_ext_dcnt += sizeof(struct lcd_ext_basic_s);
	glcd_ext_dcnt += sizeof(struct lcd_ext_type_s);

	glcd_ext_dcnt += glcd_ext_init_on_cnt;
	glcd_ext_dcnt += glcd_ext_init_off_cnt;

	p_attr->head.data_len = glcd_ext_dcnt;

	ini_value = ini_get_string("lcd_ext_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = glcd_ext_dcnt;

	memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
	lcd_ext_data_to_buf(tmp_buf, p_attr);
	p_attr->head.crc32 = cal_CRC32(0, (tmp_buf + 4), glcd_ext_dcnt - 4);

	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, glcd_ext_dcnt = %d\n", __func__, glcd_ext_dcnt);

	free(tmp_buf);
	tmp_buf = NULL;

	return 0;
}

static int handle_bl_basic(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "bl_name", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_name is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->basic.bl_name, ini_value, CC_BL_NAME_LEN_MAX - 1);
	p_attr->basic.bl_name[CC_BL_NAME_LEN_MAX - 1] = '\0';

	return 0;
}

static int handle_bl_level(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "bl_level_uboot", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_uboot is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_uboot = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_level_kernel", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_kernel is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_kernel = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_max is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_min is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_level_mid", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_mid is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_mid = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_level_mid_mapping", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_mid_mapping is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_mid_mapping = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_method(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "bl_method", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_method is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "BL_CTRL_GPIO") == 0)
		p_attr->method.bl_method = BL_CTRL_GPIO;
	else if (strcmp(ini_value, "BL_CTRL_PWM") == 0)
		p_attr->method.bl_method = BL_CTRL_PWM;
	else if (strcmp(ini_value, "BL_CTRL_PWM_COMBO") == 0)
		p_attr->method.bl_method = BL_CTRL_PWM_COMBO;
	else if (strcmp(ini_value, "BL_CTRL_LOCAL_DIMING") == 0)
		p_attr->method.bl_method = BL_CTRL_LOCAL_DIMMING;
	else if (strcmp(ini_value, "BL_CTRL_LOCAL_DIMMING") == 0)
		p_attr->method.bl_method = BL_CTRL_LOCAL_DIMMING;
	else if (strcmp(ini_value, "BL_CTRL_EXTERN") == 0)
		p_attr->method.bl_method = BL_CTRL_EXTERN;
	else
		p_attr->method.bl_method = BL_CTRL_MAX;

	ini_value = ini_get_string("Backlight_Attr", "bl_en_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_en_gpio_on", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio_on is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio_on = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_en_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_on_delay is (%s)\n", __func__, ini_value);
	p_attr->method.bl_on_delay = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_off_delay is (%s)\n", __func__, ini_value);
	p_attr->method.bl_off_delay = strtoul(ini_value, NULL, 0);

	return 0;
}

static int get_pwm_method(const char *ini_value, int def_val)
{
	if (strcmp(ini_value, "BL_PWM_NEGATIVE") == 0)
		return BL_PWM_NEGATIVE;
	else if (strcmp(ini_value, "BL_PWM_POSITIVE") == 0)
		return BL_PWM_POSITIVE;
	else
		return def_val;
}

static char *bl_pwm_name[] = {
	"BL_PWM_A",
	"BL_PWM_B",
	"BL_PWM_C",
	"BL_PWM_D",
	"BL_PWM_E",
	"BL_PWM_F",
	"BL_PWM_G",
	"BL_PWM_H",
	"BL_PWM_I",
	"BL_PWM_J"
};

static char *bl_pwm_ao_name[] = {
	"BL_PWM_AO_A",
	"BL_PWM_AO_B",
	"BL_PWM_AO_C",
	"BL_PWM_AO_D",
	"BL_PWM_AO_E",
	"BL_PWM_AO_F",
	"BL_PWM_AO_G",
	"BL_PWM_AO_H"
};

static char bl_pwm_vs_name[] = {"BL_PWM_VS"};

static unsigned int get_pwm_port_index(const char *str)
{
	enum bl_pwm_port_e pwm_port = BL_PWM_MAX;
	int i, cnt;

	cnt = ARRAY_SIZE(bl_pwm_name);
	for (i = 0; i < cnt; i++) {
		if (strcmp(str, bl_pwm_name[i]) == 0) {
			pwm_port = i + BL_PWM_A;
			return pwm_port;
		}
	}

	cnt = ARRAY_SIZE(bl_pwm_ao_name);
	for (i = 0; i < cnt; i++) {
		if (strcmp(str, bl_pwm_ao_name[i]) == 0) {
			pwm_port = i + BL_PWM_AO_A;
			return pwm_port;
		}
	}

	if (strcmp(str, bl_pwm_vs_name) == 0) {
		pwm_port = BL_PWM_VS;
		return pwm_port;
	}

	return BL_PWM_MAX;
}

static int handle_bl_pwm(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "pwm_method", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_method is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_method = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = ini_get_string("Backlight_Attr", "pwm_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_port = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Backlight_Attr", "pwm_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_duty_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_duty_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_duty_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_duty_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_duty_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_duty_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_gpio", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_gpio is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_gpio = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_method", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_method is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_method = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_port = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_duty_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_duty_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_duty_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_duty_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_duty_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_duty_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_gpio", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_gpio is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_gpio = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_on_delay is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_on_delay = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_off_delay is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_off_delay = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_level_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_level_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_level_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_level_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_level_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_level_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "pwm2_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_level_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_level_min = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_ldim(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "bl_ldim_row", "1");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_row is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_row = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_ldim_col", "1");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_col is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_col = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_ldim_mode", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_mode is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LDIM_LR_SIDE") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_LR_SIDE;
	else if (strcmp(ini_value, "LDIM_TB_SIDE") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_TB_SIDE;
	else if (strcmp(ini_value, "LDIM_DIRECT") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_DIRECT;
	else
		p_attr->ldim.ldim_mode = LDIM_MODE_TB_SIDE;

	ini_value = ini_get_string("Backlight_Attr", "bl_ldim_dev_index", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_dev_index is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_dev_index = strtoul(ini_value, NULL, 0);

	p_attr->ldim.ldim_attr_4 = 0;
	p_attr->ldim.ldim_attr_5 = 0;
	p_attr->ldim.ldim_attr_6 = 0;
	p_attr->ldim.ldim_attr_7 = 0;
	p_attr->ldim.ldim_attr_8 = 0;
	p_attr->ldim.ldim_attr_9 = 0;

	return 0;
}

static int handle_bl_custome(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "bl_custome_val_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_0 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_custome_val_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_1 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_1 = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Backlight_Attr", "bl_custome_val_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_2 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_custome_val_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_3 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Backlight_Attr", "bl_custome_val_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_4 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_4 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_header(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Backlight_Attr", "version", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	gbl_dcnt = 0;
	gbl_dcnt += sizeof(struct lcd_header_s);
	gbl_dcnt += sizeof(struct bl_basic_s);
	gbl_dcnt += sizeof(struct bl_level_s);
	gbl_dcnt += sizeof(struct bl_method_s);
	gbl_dcnt += sizeof(struct bl_pwm_s);
	if (p_attr->head.version == 2) {
		gbl_dcnt += sizeof(struct bl_ldim_s);
		gbl_dcnt += sizeof(struct bl_custome_s);
	}
	p_attr->head.data_len = gbl_dcnt;

	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = gbl_dcnt;
	p_attr->head.crc32 = cal_CRC32(0, (((unsigned char *)p_attr) + 4), gbl_dcnt - 4);

	return 0;
}

#ifdef CONFIG_AML_LCD_BL_LDIM
static int handle_ldim_dev_basic(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "dev_name", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dev_name is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->basic.dev_name, ini_value, CC_LDIM_DEV_NAME_LEN_MAX - 1);
	p_attr->basic.dev_name[CC_LDIM_DEV_NAME_LEN_MAX - 1] = '\0';

	return 0;
}

static int handle_ldim_dev_if(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "if_type", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_type is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LDIM_DEV_I2C") == 0)
		p_attr->interface.type = LDIM_DEV_TYPE_I2C;
	else if (strcmp(ini_value, "LDIM_DEV_SPI") == 0)
		p_attr->interface.type = LDIM_DEV_TYPE_SPI;
	else
		p_attr->interface.type = LCD_EXTERN_MAX;

	ini_value = ini_get_string("Ldim_dev_Attr", "if_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_freq is (%s)\n", __func__, ini_value);
	p_attr->interface.freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_8", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "if_attr_9", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_9 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_ldim_dev_pwm(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_port = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_duty = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_vs_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_port = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_duty = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_hs_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_port = get_pwm_port_index(ini_value);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_freq = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_duty = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pwm_adj_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "pinmux_sel", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pinmux_sel is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->pwm.pinmux_sel, ini_value, 29);

	return 0;
}

static int handle_ldim_dev_ctrl(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "en_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "en_gpio_on", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio_on is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio_on = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "en_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, on_delay is (%s)\n", __func__, ini_value);
	p_attr->ctrl.on_delay = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, off_delay is (%s)\n", __func__, ini_value);
	p_attr->ctrl.off_delay = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "err_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, err_gpio is (%s)\n", __func__, ini_value);
	p_attr->ctrl.err_gpio = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "write_check", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, write_check is (%s)\n", __func__, ini_value);
	p_attr->ctrl.write_check = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "dim_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dim_max is (%s)\n", __func__, ini_value);
	p_attr->ctrl.dim_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "dim_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dim_min is (%s)\n", __func__, ini_value);
	p_attr->ctrl.dim_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "chip_count", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, chip_count is (%s)\n", __func__, ini_value);
	p_attr->ctrl.chip_cnt = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "zone_mapping_path", "null");
	if (strcmp(ini_value, "null") != 0) {
		env_set("bl_mapping_path", ini_value);
		strlcpy(p_attr->ctrl.zone_map_path, ini_value, 255); //if no path_k
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGE("%s, zone_mapping_path is (%s)\n", __func__, ini_value);
	}

	ini_value = ini_get_string("Ldim_dev_Attr", "zone_mapping_path_k", "null");
	if (strcmp(ini_value, "null") != 0) {
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, zone_mapping_path_k is (%s)\n", __func__, ini_value);
		strlcpy(p_attr->ctrl.zone_map_path, ini_value, 255);
	} else {
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, zone_mapping_path_k is NOT FOUND\n", __func__);
	}

	return 0;
}

static int handle_ldim_dev_profile(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_mode", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_mode is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_mode = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_path", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_path is (%s)\n", __func__, ini_value);
	strlcpy(p_attr->profile.profile_path, ini_value, 255);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "profile_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_7 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_ldim_dev_custom(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int tmp_buf[32];
	int i = 0, tmp_cnt = 0;

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_8", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "custome_attr_9", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_9 = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("Ldim_dev_Attr", "param_data", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, param_data is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		p_attr->custome.custome_param_size = 0;
		printf("%s, panel.ini no param_data(%s)\n", __func__, ini_value);
	} else {
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, param_data is (%d)\n", __func__, tmp_cnt);
		/* data check and copy */
		if (tmp_cnt > LDIM_PARAM_MAX) {
			printf("%s: invalid param data\n", __func__);
		} else {
			p_attr->custome.custome_param_size = tmp_cnt;
			i = 0;
			while (i < tmp_cnt) {
				p_attr->custome.custome_param[i] = tmp_buf[i];
				if (model_debug_flag & DEBUG_BACKLIGHT)
					ALOGD("param_data[%d] is (%d)\n", i, tmp_buf[i]);
				i++;
			}
		}
	}

	return 0;
}

static int handle_ldim_dev_init(struct ldim_dev_attr_s *p_attr)
{
	int i = 0, j = 0, k, tmp_cnt = 0, tmp_off = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[2048];
	unsigned int data_size = 0;

	ini_value = ini_get_string("Ldim_dev_Attr", "cmd_size", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, cmd_size is (%s)\n", __func__, ini_value);
	p_attr->init.cmd_size = strtoul(ini_value, NULL, 0);

	if (p_attr->init.cmd_size != 0xff) {
		ALOGE("%s: invalid cmd_size 0x%x\n", __func__, p_attr->init.cmd_size);
		p_attr->init.cmd_data[0] = 0xff;
		p_attr->init.cmd_data[1] = 0;
		g_ldim_dev_init_on_cnt = 2;
		p_attr->init.cmd_data[2] = 0xff;
		p_attr->init.cmd_data[3] = 0;
		g_ldim_dev_init_off_cnt = 2;
		return 0;
	}

	ini_value = ini_get_string("Ldim_dev_Attr", "init_on", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, init_on is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);

	/* data check and copy */
	if (tmp_cnt > LDIM_INIT_ON_MAX) {
		ALOGE("%s: invalid init_on data\n", __func__);
		p_attr->init.cmd_data[0] = 0xff;
		p_attr->init.cmd_data[1] = 0;
		g_ldim_dev_init_on_cnt = 2;
	} else {
		i = 0;
		j = 0;
		while (i < tmp_cnt) {
			p_attr->init.cmd_data[j] = tmp_buf[i];
			if (p_attr->init.cmd_data[j] == 0xff) {
				p_attr->init.cmd_data[j + 1] = 0;
				j += 2;
				break;
			}

			data_size = tmp_buf[i + 1];
			p_attr->init.cmd_data[j + 1] = data_size;
			for (k = 0; k < data_size; k++) {
				p_attr->init.cmd_data[j + 2 + k] =
					(unsigned char)tmp_buf[i + 2 + k];
			}

			j += data_size + 2;
			i += tmp_buf[i + 1] + 2; /* raw data */
		}
		g_ldim_dev_init_on_cnt = j;
	}

	tmp_off = g_ldim_dev_init_on_cnt;
	ini_value = ini_get_string("Ldim_dev_Attr", "init_off", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, init_off is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	if (tmp_cnt > LDIM_INIT_OFF_MAX) {
		ALOGE("%s: invalid init_off data\n", __func__);
		p_attr->init.cmd_data[tmp_off + 0] = 0xff;
		p_attr->init.cmd_data[tmp_off + 1] = 0;
		g_ldim_dev_init_off_cnt = 2;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->init.cmd_data[tmp_off + i] = tmp_buf[i];
		g_ldim_dev_init_off_cnt = tmp_cnt;
	}

	if (model_debug_flag & DEBUG_BACKLIGHT) {
		ALOGD("%s, init_on_data:\n", __func__);
		for (i = 0; i < g_ldim_dev_init_on_cnt; i++)
			printf("  [%d] = 0x%02x\n", i, p_attr->init.cmd_data[i]);

		ALOGD("%s, init_off_data:\n", __func__);
		for (i = 0; i < g_ldim_dev_init_off_cnt; i++)
			ALOGD("  [%d] = 0x%02x\n", i, p_attr->init.cmd_data[tmp_off + i]);
	}

	return 0;
}

static int handle_ldim_dev_header(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("Ldim_dev_Attr", "version", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	gldim_dev_dcnt = 0;
	gldim_dev_dcnt += sizeof(struct lcd_header_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_basic_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_if_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_pwm_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_ctrl_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_profile_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_custom_s);
	gldim_dev_dcnt += 1; //cmd_size
	gldim_dev_dcnt += g_ldim_dev_init_on_cnt;
	gldim_dev_dcnt += g_ldim_dev_init_off_cnt;
	p_attr->head.data_len = gldim_dev_dcnt;

	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = gldim_dev_dcnt;
	p_attr->head.crc32 = cal_CRC32(0, (((unsigned char *)p_attr) + 4), gldim_dev_dcnt - 4);

	return 0;
}
#endif

static int handle_panel_misc(struct panel_misc_s *p_misc)
{
	int tmp_val = 0;
	const char *ini_value = NULL;
	const char *display_layer = NULL;
	char *rev_ctrl = NULL;
	char *ret = NULL;
	char buf[64] = {0};
	unsigned char connector_idx;

	ini_value = ini_get_string("panel_misc", "panel_misc_version", "null");
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, panel_misc_version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		strcpy(p_misc->version, "V001");
	} else {
		tmp_val = strtol(ini_value, NULL, 0);
		if (tmp_val < 1)
			tmp_val = 1;

		sprintf(p_misc->version, "V%03d", tmp_val);
	}

	tmp_val = env_get_ulong("model_outputmode_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_outputmode_bypass\n");
		goto handle_panel_misc_next;
	}
	ini_value = ini_get_string("panel_misc", "outputmode2", "null");
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, outputmode2 is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		ini_value = ini_get_string("panel_misc", "outputmode", "null");
		if (model_debug_flag & DEBUG_MISC)
			ALOGD("%s, outputmode is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null")) {
			strlcpy(p_misc->outputmode, ini_value,
				sizeof(p_misc->outputmode) - 1);
			p_misc->outputmode[sizeof(p_misc->outputmode) - 1]
				= '\0';
			snprintf(buf, 63, "setenv outputmode %s", p_misc->outputmode);
			run_command(buf, 0);
		}
	} else {
		strlcpy(p_misc->outputmode, ini_value, 63);
		snprintf(buf, 63, "setenv outputmode2 %s", p_misc->outputmode);
		run_command(buf, 0);
	}

handle_panel_misc_next:
	tmp_val = env_get_ulong("model_connector_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_connector_bypass\n");
		goto handle_panel_misc_next2;
	}

	ini_value = ini_get_string("panel_misc", "connector2_type", "null");
	if (!strcmp(ini_value, "null")) {
		connector_idx = 2;
		goto handle_panel_misc_set_connector;
	}

	ini_value = ini_get_string("panel_misc", "connector1_type", "null");
	if (!strcmp(ini_value, "null")) {
		connector_idx = 1;
		goto handle_panel_misc_set_connector;
	}

	ini_value = ini_get_string("panel_misc", "connector0_type", "null");
	if (!strcmp(ini_value, "null")) {
		connector_idx = 0;
		goto handle_panel_misc_set_connector;
	} else {
		ini_value = ini_get_string("panel_misc", "connector_type", "null");
		if (!strcmp(ini_value, "null")) {
			connector_idx = 0;
			goto handle_panel_misc_set_connector;
		}
	}

	ALOGD("%s: connector not assigned\n", __func__);
	goto handle_panel_misc_next2;

handle_panel_misc_set_connector:
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, connector%u_type is (%s)\n", __func__, connector_idx, ini_value);
	strncpy(p_misc->connector_type, ini_value, sizeof(p_misc->connector_type) - 1);
	p_misc->connector_type[sizeof(p_misc->connector_type) - 1] = '\0';
	ret = strstr(p_misc->connector_type, "_");
	if (ret)
		p_misc->connector_type[ret - p_misc->connector_type] = '-';
	snprintf(buf, 63, "setenv connector%u_type %s", connector_idx, p_misc->connector_type);
	run_command(buf, 0);

handle_panel_misc_next2:
	rev_ctrl = env_get("reverse_ctrl");
	if (!rev_ctrl || strcmp(rev_ctrl, "0") == 0) {
		ini_value = ini_get_string("panel_misc", "panel_reverse", "null");
		if (model_debug_flag & DEBUG_MISC)
			ALOGD("%s, panel_reverse is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0 || strcmp(ini_value, "0") == 0 ||
			strcmp(ini_value, "false") == 0 || strcmp(ini_value, "no_rev") == 0) {
			p_misc->panel_reverse = 0;
		} else if (strcmp(ini_value, "true") == 0 || strcmp(ini_value, "1") == 0 ||
			strcmp(ini_value, "have_rev") == 0) {
			p_misc->panel_reverse = 1;
		} else if (strcmp(ini_value, "x_rev") == 0 || strcmp(ini_value, "2") == 0) {
			p_misc->panel_reverse = 2;
		} else if (strcmp(ini_value, "y_rev") == 0 || strcmp(ini_value, "3") == 0) {
			p_misc->panel_reverse = 3;
		} else {
			p_misc->panel_reverse = 0;
		}
		if (p_misc->panel_reverse) {
			display_layer = ini_get_string("panel_misc", "display_layer", "null");
			if (!display_layer)
				p_misc->display_layer = 4;
			else if (strcmp(display_layer, "osd0") == 0 ||
					strcmp(display_layer, "0") == 0)
				p_misc->display_layer = 0;
			else if (strcmp(display_layer, "osd1") == 0 ||
					strcmp(display_layer, "1") == 0)
				p_misc->display_layer = 1;
			else
				p_misc->display_layer = 4;
		}
		switch (p_misc->panel_reverse) {
		case 1:
			run_command("setenv panel_reverse 1", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,true", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,true", 0);
				break;
			default:
				run_command("setenv osd_reverse all,true", 0);
				break;
			}
			run_command("setenv video_reverse 1", 0);
			break;
		case 2:
			run_command("setenv panel_reverse 2", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,x_rev", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,x_rev", 0);
				break;
			default:
				run_command("setenv osd_reverse all,x_rev", 0);
				break;
			}
			run_command("setenv video_reverse 2", 0);
			break;
		case 3:
			run_command("setenv panel_reverse 3", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,y_rev", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,y_rev", 0);
				break;
			default:
				run_command("setenv osd_reverse all,y_rev", 0);
				break;
			}
			run_command("setenv video_reverse 3", 0);
			break;
		default:
			run_command("setenv panel_reverse 0", 0);
			run_command("setenv osd_reverse n", 0);
			run_command("setenv video_reverse 0", 0);
			break;
		}
	}
	return 0;
}

static int handle_lcd_optical_attr(struct lcd_optical_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = ini_get_string("lcd_optical_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		glcd_optical_dcnt = 0;
		return -1;
	}
	p_attr->head.version = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "hdr_support", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, hdr_support is (%s)\n", __func__, ini_value);
	p_attr->hdr_support = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "features", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, features is (%s)\n", __func__, ini_value);
	p_attr->features = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_r_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_r_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_r_x = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_r_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_r_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_r_y = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_g_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_g_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_g_x = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_g_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_g_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_g_y = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_b_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_b_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_b_x = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "primaries_b_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_b_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_b_y = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "white_point_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, white_point_x is (%s)\n", __func__, ini_value);
	p_attr->white_point_x = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "white_point_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, white_point_y is (%s)\n", __func__, ini_value);
	p_attr->white_point_y = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "luma_max", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_max is (%s)\n", __func__, ini_value);
	p_attr->luma_max = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "luma_min", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_min is (%s)\n", __func__, ini_value);
	p_attr->luma_min = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "luma_avg", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_avg is (%s)\n", __func__, ini_value);
	p_attr->luma_avg = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "ldim_support", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, ldim_support is (%s)\n", __func__, ini_value);
	p_attr->ldim_support = strtoul(ini_value, NULL, 0);

	ini_value = ini_get_string("lcd_optical_Attr", "luma_peak", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_peak is (%s)\n", __func__, ini_value);
	p_attr->luma_peak = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_optical_header(struct lcd_optical_attr_s *p_attr)
{
	unsigned char *tmp_buf = NULL;

	glcd_optical_dcnt = sizeof(struct lcd_optical_attr_s);

	tmp_buf = (unsigned char *)malloc(glcd_optical_dcnt);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset((void *)tmp_buf, 0, glcd_optical_dcnt);

	p_attr->head.data_len = glcd_optical_dcnt;

	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = glcd_optical_dcnt;

	memcpy(tmp_buf, p_attr, glcd_optical_dcnt);
	p_attr->head.crc32 = cal_CRC32(0, (tmp_buf + 4), glcd_optical_dcnt - 4);

	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, glcd_optical_dcnt = %d\n", __func__, glcd_optical_dcnt);

	free(tmp_buf);
	tmp_buf = NULL;

	return 0;
}

static void dccd_check_update(struct dccd_info_s *dccd)
{
	struct dccd_base_info_s *basic = NULL;
	int bufidx = 0, i = 0;

	if (!dccd || !dccd->data_buf || dccd->data_size <= 0)
		return;

	basic = (struct dccd_base_info_s *)dccd->data_buf;

	//check if it's dccd bin
	if (basic->dccd != 0x0d0c0c0d) {
		ALOGE("It's not dccd bin(%#x)\n", basic->dccd);
		return;
	}

	//check dccd checksum
	bufidx = sizeof(*basic) + basic->len - 1;
	if (bufidx >= dccd->data_size) {
		ALOGE("dccd len not match\n");
		return;
	}
	dccd->checksum = dccd->data_buf[bufidx];

	//check crc, calculate skip crc/checksum
	for (i = 0, dccd->calc_chksum = 0; i <= bufidx; i++)
		dccd->calc_chksum += dccd->data_buf[i];

	dccd->is_dccd = !(dccd->calc_chksum & 0xff);
	if (model_debug_flag & DEBUG_LCD) {
		ALOGD("dccd raw checksum=%#x, calc checksum=%#x, %s\n",
			dccd->checksum, dccd->calc_chksum,
			dccd->is_dccd ? "matched" : "miss-matched");
	}
}

static int dccd_update_lcd_attr(struct lcd_attr_s *p_attr, unsigned char *dccd_buf)
{
	int block_size = 0, data_start = 0;
	int data = 0;
	unsigned int hs_fp = 0, h_blk = 0, vs_fp = 0, v_blk = 0;
	unsigned int pol = 0, width = 0;
	unsigned int lane_num = 0;

	//0x1f: save product Name length
	block_size = dccd_buf[0x1f];
	data_start =  0x1f + block_size + 1;

	//0x80: panel timing data block
	if (dccd_buf[data_start] == 0x80) {
	/* offset 0x1: [7:4]:version, [3:0]:reserved
	 * 0x2: data_length
	 * 0x3: h_active[7:0]
	 * 0x4: h_active[15:8]
	 * 0x5: h_blank[7:0]
	 * 0x6: h_blank[15:8]
	 * 0x7: hsync_fp[7:0]
	 * 0x8: hsync_fp[15:8]
	 * 0x9: hsync_width[7:0]
	 * 0xa: bit7: hsync_pol, [6:0]:hsync_width[14:8]
	 * 0xb: v_active[7:0]
	 * 0xc: v_active[15:8]
	 * 0xf: vsync_fp[7:0]
	 * 0x10: vsync_fp[15:8]
	 * 0x11: vsync_width[7:0]
	 * 0x12: bit7:vsync_pol, [6:0]: vsync_width[14:8]
	 * 0x13~0x16: pixel_clk? [32:0]
	 * 0x17: [3: 0]: lcd_bits(0:6bit, 1:8bit, 2:10bit, 3:12bit..)
	 */
		p_attr->head.version = dccd_buf[data_start + 0x1];
		if (model_debug_flag & DEBUG_LCD_OPTICAL)
			ALOGD("%s, version is (%d)\n", __func__, p_attr->head.version);
		if (p_attr->head.version >= 2)
			p_attr->head.block_next_flag = 1;

		//lcd_timing
		p_attr->timming.h_active = dccd_buf[data_start + 0x3] |
					dccd_buf[data_start + 0x4] << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, h_active is (%d)\n", __func__, p_attr->timming.h_active);

		h_blk = dccd_buf[data_start + 0x5] |
					dccd_buf[data_start + 0x6] << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, h_blank is (%d)\n", __func__, h_blk);

		hs_fp = dccd_buf[data_start + 0x7] |
					dccd_buf[data_start + 0x8] << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, hsync_fp is (%d)\n", __func__, hs_fp);

		width = dccd_buf[data_start + 0x9] |
					(dccd_buf[data_start + 0xa] & 0x7f) << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, hsync_width is (%d)\n", __func__, width)

		p_attr->timming.hsync_bp = h_blk - hs_fp - width;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, hsync_bp is (%d)\n", __func__, p_attr->timming.hsync_bp);

		pol = (dccd_buf[data_start + 0xa] & 0x80) >> 7;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, hsync_pol is (%d)\n", __func__, pol);

		p_attr->timming.hsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

		p_attr->timming.v_active = dccd_buf[data_start + 0xb] |
					dccd_buf[data_start + 0xc] << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, v_active is (%d)\n", __func__, p_attr->timming.v_active);

		v_blk = dccd_buf[data_start + 0xd] | dccd_buf[data_start + 0xe] << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, v_blank is (%d)\n", __func__, v_blk);

		vs_fp = dccd_buf[data_start + 0xf] | dccd_buf[data_start + 0x10] << 8;
		if (vs_fp < 18)
			vs_fp = 18;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, vsync_fp is (%d)\n", __func__, vs_fp);

		width = dccd_buf[data_start + 0x11] |
					(dccd_buf[data_start + 0x12] & 0x7f) << 8;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, vsync_width is (%d)\n", __func__, width);

		p_attr->timming.vsync_bp = v_blk - vs_fp - width;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, vsync_bp is (%d)\n", __func__, p_attr->timming.vsync_bp);

		pol = (dccd_buf[data_start + 0x12] & 0x80) >> 7;
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, vsync_pol is (%d)\n", __func__, pol);

		p_attr->timming.vsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

		//lcd_basic
		data = dccd_buf[data_start + 0x17] & 0xf;
		switch (data) {
		case 0:
			p_attr->basic.lcd_bits_cfmt = 6;
			break;
		case 1:
			p_attr->basic.lcd_bits_cfmt = 8;
			break;
		case 3:
			p_attr->basic.lcd_bits_cfmt = 12;
			break;
		default:
			p_attr->basic.lcd_bits_cfmt = 10;
			break;
		}
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, lcd_bits is (%d)\n", __func__, p_attr->basic.lcd_bits_cfmt);
		data_start = 3 + data_start + dccd_buf[data_start + 0x2];
	}

	//0x81: panel timing data block2
	if (dccd_buf[data_start] == 0x81) {
		/* offset 0x1: [7:4]:version, [3:0]:reserved
		 * 0x2: data_length
		 * 0x3: screen_width[7:0] (cm)
		 * 0x4: screen_height[7:0] (cm)
		 * 0x5: [7:4]: screen_width[11:8], [3:0]: screen_height[11:8]
		 * 0x6: lcd_interface(0:vb1, 1:lvds, 2:p2p, 3: mlvds)
		 *   SS: don't support ss positive and negative, defalut use ss_level+ value
		 * 0x7: ss max freq (KHz)
		 * 0x8: ss_level+
		 * 0x9: ss_level-
		 * 0xc: lane_num
		 */
		g_lcd_if = dccd_buf[data_start + 0x6];
		switch (g_lcd_if) {
		case 0:
			p_attr->interface.if_attr_0 = dccd_buf[data_start + 0xc];
			lane_num = p_attr->interface.if_attr_0;
			break;
		case 1:
			p_attr->interface.if_attr_8 = dccd_buf[data_start + 0xc];
			lane_num = p_attr->interface.if_attr_8;
			break;
		case 2:
			//p_attr->interface.if_attr_1 = dccd_buf[data_start + 0xc];
			lane_num = p_attr->interface.if_attr_1;
			break;
		case 3:
			p_attr->interface.if_attr_0 = dccd_buf[data_start + 0xc];
			lane_num = p_attr->interface.if_attr_0;
			break;
		default:
			break;
		}
		p_attr->basic.lcd_if_chk &= ~0x3f;
		p_attr->basic.lcd_if_chk |= (g_lcd_if & 0x3f);
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, lcd_type is (%d), lane_num is (%d)\n",
			      __func__, g_lcd_if, lane_num);

		data_start = 3 + data_start + dccd_buf[data_start + 0x2];
	}
	return 0;
}

static int update_dccd_load(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned char *tmp_buf = NULL;
	unsigned int size = 0, temp = 0;

	ini_value = ini_get_string("lcd_Attr", "dccd_flag", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, dccd_flag is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	if (!temp)
		return 0;

	ini_value = ini_get_string("lcd_Attr", "dccd_timing", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, dccd_timing is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	if (!temp)
		return 0;

	//check dccd bin
	ini_value = ini_get_string("tcon_Path", "DCCD_BIN_PATH", "null");
	if (!strcmp(ini_value, "null")) {
		ALOGE("%s, no dccd_bin file error!\n", __func__);
		return -1;
	}
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s: dccd_path: %s\n", __func__, ini_value);
	if (!ini_is_file_exist(ini_value)) {
		ALOGE("%s, file \"%s\" not exist.\n", __func__, ini_value);
		return -1;
	}

	size = handle_read_bin_file(ini_value, CC_MAX_TCON_BIN_SIZE);
	if (!size)
		return -1;

	tmp_buf = (unsigned char *)malloc(size);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer error!\n", __func__);
		return -1;
	}

	get_bin_data(tmp_buf, size);

	dccd_info.data_buf = tmp_buf;
	dccd_info.data_size = size;
	dccd_info.is_dccd = 0;
	dccd_check_update(&dccd_info);

	if (dccd_info.is_dccd)
		dccd_update_lcd_attr(p_attr, tmp_buf);
	bin_file_uninit();

	return 0;
}

static int parse_panel_ini(const char *file_name, unsigned char *lcd_buf,
			   struct lcd_ext_attr_s *ext_attr,
			   struct bl_attr_s *bl_attr,
			   struct ldim_dev_attr_s *ldim_dev_attr,
			   struct panel_misc_s *misc_attr,
			   struct lcd_optical_attr_s *optical_attr)
{
	struct lcd_attr_s *lcd_attr;
	unsigned char *lcd_next_attr;
	struct lcd_header_s *header, *next_header = NULL;
	int ret;

	ini_parser_init();

	if (ini_parse_file(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		ini_parser_uninit();
		return -1;
	}

	// handle integrity flag
	if (handle_integrity_flag() < 0) {
		ALOGE("%s, handle_integrity_flag error!\n", __func__);
		ini_parser_uninit();
		return -1;
	}

	/* handle lcd attr */
	lcd_attr = (struct lcd_attr_s *)lcd_buf;
	handle_lcd_basic(lcd_attr);
	handle_lcd_timming(lcd_attr);
	handle_lcd_customer(lcd_attr);
	handle_lcd_interface(lcd_attr);
	handle_lcd_pwr(lcd_attr);
	update_dccd_load(lcd_attr);
	handle_lcd_header(lcd_attr);

	header = &lcd_attr->head;
	lcd_next_attr = lcd_buf + header->block_cur_size;
	next_header = (struct lcd_header_s *)lcd_next_attr;
	if (header->version == 2) {
		handle_lcd_phy(lcd_next_attr);
		handle_lcd_cus_ctrl(lcd_next_attr, header->version);
		handle_lcd_v2_header(next_header);
	} else if (header->version == 3) {
		handle_lcd_cus_ctrl(lcd_next_attr, header->version);
		handle_lcd_v3_header(next_header);
	}

	glcd_dcnt = header->block_cur_size + next_header->block_cur_size;
	header->data_len = glcd_dcnt;
	header->crc32 = cal_CRC32(0, (lcd_buf + 4), glcd_dcnt - 4);
	if (model_debug_flag & DEBUG_LCD) {
		ALOGD("%s: version=%d, data_len=%d, glcd_dcnt=%d, block1_size=%d, block2_size=%d\n",
		      __func__, header->version, header->data_len, glcd_dcnt,
		      header->block_cur_size, next_header->block_cur_size);
	}

	if (g_lcd_if == LCD_MLVDS ||
	    g_lcd_if == LCD_P2P)
		g_lcd_tcon_valid = 1;
	else
		g_lcd_tcon_valid = 0;

#ifdef CONFIG_AML_LCD_TCON
	/*should ready tcon path here, for lcd_ext usage */
	if (g_lcd_tcon_valid)
		handle_tcon_path();
#endif

	// handle lcd extern attr
	handle_lcd_ext_basic(ext_attr);
	handle_lcd_ext_type(ext_attr);
	handle_lcd_ext_cmd_data(ext_attr);
	handle_lcd_ext_header(ext_attr);

	// handle bl attr
	handle_bl_basic(bl_attr);
	handle_bl_level(bl_attr);
	handle_bl_method(bl_attr);
	handle_bl_pwm(bl_attr);
	handle_bl_ldim(bl_attr);
	handle_bl_custome(bl_attr);
	handle_bl_header(bl_attr);

#ifdef CONFIG_AML_LCD_BL_LDIM
	if (bl_attr->method.bl_method == BL_CTRL_LOCAL_DIMMING)
		g_ldim_dev_valid = 1;
	else
		g_ldim_dev_valid = 0;

	// handle ldim_dev attr
	if (g_ldim_dev_valid) {
		handle_ldim_dev_basic(ldim_dev_attr);
		handle_ldim_dev_if(ldim_dev_attr);
		handle_ldim_dev_pwm(ldim_dev_attr);
		handle_ldim_dev_ctrl(ldim_dev_attr);
		handle_ldim_dev_profile(ldim_dev_attr);
		handle_ldim_dev_custom(ldim_dev_attr);
		handle_ldim_dev_init(ldim_dev_attr);
		handle_ldim_dev_header(ldim_dev_attr);
	}
#endif

	handle_panel_misc(misc_attr);

	// handle lcd optical attr
	ret = handle_lcd_optical_attr(optical_attr);
	if (ret == 0)
		handle_lcd_optical_header(optical_attr);

	ini_parser_uninit();

	return 0;
}

#ifdef CONFIG_AML_LCD_BL_LDIM
int handle_ldim_dev_zone_mapping_get(unsigned char *buf, unsigned int size,
				     const char *path)
{
	unsigned int bin_size = 0;

	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s: %s\n", __func__, path);

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}

	bin_size = handle_read_bin_file(path, CC_MAX_LDIM_DEV_ZONE_MAP_SIZE);
	if (bin_size == 0)
		return -1;
	if (bin_size > size) {
		ALOGE("%s, file \"%s\" size 0x%x bigger than buf size 0x%x\n",
		      __func__, path, bin_size, size);
		return -1;
	}

	get_bin_data(buf, size);
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s: load ldim zone_mapping bin\n", __func__);

	return 0;
}
#endif

int handle_panel_ini(int index)
{
	int tmp_len = 0;
	unsigned char *tmp_buf = NULL;
	unsigned char *lcd_buf = NULL;
	struct bl_attr_s *bl_attr = NULL;
	struct ldim_dev_attr_s *ldim_dev_attr = NULL;
	struct panel_misc_s misc_attr;
	struct lcd_optical_attr_s *optical_attr = NULL;
	char *file_name;
	int print_flag;
	char str[15];
	char key_name[PANEL_PARAM_KEY_NAME_SIZE];

	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	print_flag = env_get_ulong("model_debug_print", 16, 0xffff);
	if (print_flag != 0xffff) {
		model_debug_flag = print_flag;
		ALOGD("model_debug_flag: 0x%x\n", model_debug_flag);
	}

	file_name = env_get(str);
	if (!file_name) {
		ALOGE("%s, %s path error!!!\n", __func__, str);
		return -1;
	}

	tmp_buf = (unsigned char *)malloc(CC_MAX_DATA_SIZE);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);

	lcd_buf = (unsigned char *)malloc(CC_MAX_DATA_SIZE);
	if (!lcd_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err0;
	}
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);

	if (!lcd_ext_attr) {
		lcd_ext_attr = (struct lcd_ext_attr_s *)malloc(sizeof(struct lcd_ext_attr_s));
		if (!lcd_ext_attr) {
			ALOGE("%s, malloc buffer memory error!!!\n", __func__);
			goto handle_panel_ini_err1;
		}
	}
	memset((void *)lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));

	bl_attr = (struct bl_attr_s *)malloc(sizeof(struct bl_attr_s));
	if (!bl_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err1;
	}
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));

#ifdef CONFIG_AML_LCD_BL_LDIM
	ldim_dev_attr = (struct ldim_dev_attr_s *)malloc(sizeof(struct ldim_dev_attr_s));
	if (!ldim_dev_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err2;
	}
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
#endif

	optical_attr = (struct lcd_optical_attr_s *)malloc(sizeof(struct lcd_optical_attr_s));
	if (!optical_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err3;
	}
	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));

	//init misc attr as default
	memset((void *)&misc_attr, 0, sizeof(struct panel_misc_s));
	strcpy(misc_attr.version, "V001");
	strcpy(misc_attr.outputmode, "1080p60hz");
	misc_attr.panel_reverse = 0;

	// start handle panel ini name
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: %s: %s\n", __func__, str, file_name);
	if (parse_panel_ini(file_name, lcd_buf, lcd_ext_attr,
		bl_attr, ldim_dev_attr, &misc_attr, optical_attr) < 0) {
		ALOGE("%s, parse_panel_ini file name \"%s\" fail.\n",
		      __func__, file_name);
		goto handle_panel_ini_err4;
	}

	// start handle lcd param
	if (index)
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd%d", index);
	else
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd");
	if (panel_param_mem_put((u8 *)lcd_buf, key_name, glcd_dcnt) == 0)
		panel_param_mem_set_ukey_flag();

	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_lcd_param(index, tmp_buf);
	if (check_param_valid(0, glcd_dcnt, lcd_buf, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check lcd param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_lcd_param(index, glcd_dcnt, lcd_buf);
	}
	// end handle lcd param

	// start handle lcd extern param
	if (index)
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd%d_extern", index);
	else
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd_extern");
	panel_param_mem_put((u8 *)lcd_ext_attr, key_name, glcd_ext_dcnt);

	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_lcd_extern_param(index, tmp_buf);
	//ALOGD("%s, start check lcd extern param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(0, glcd_ext_dcnt, (unsigned char *)lcd_ext_attr, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check lcd extern param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_lcd_extern_param(index, glcd_ext_dcnt, (unsigned char *)lcd_ext_attr);
	}

	// start handle backlight param
	if (index)
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "backlight%d", index);
	else
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "backlight");
	panel_param_mem_put((u8 *)bl_attr, key_name, gbl_dcnt);

	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_backlight_param(index, tmp_buf);
	if (check_param_valid(0, gbl_dcnt, (unsigned char *)bl_attr, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check backlight param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_backlight_param(index, gbl_dcnt, (unsigned char *)bl_attr);
	}

#ifdef CONFIG_AML_LCD_BL_LDIM
	// start handle ldim_dev param
	if (g_ldim_dev_valid) {
		snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "ldim_dev");
		panel_param_mem_put((u8 *)ldim_dev_attr, key_name, gldim_dev_dcnt);

		memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
		tmp_len = read_ldim_dev_param(tmp_buf);
		//ALOGD("%s, start check ldim_dev param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, gldim_dev_dcnt, (unsigned char *)ldim_dev_attr,
			tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check ldim_dev param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			save_ldim_dev_param(gldim_dev_dcnt, (unsigned char *)ldim_dev_attr);
		}
	}
	// end handle ldim_dev param
#endif

	// start handle lcd_optical param
	if (glcd_optical_dcnt) {
		if (index)
			snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd%d_optical", index);
		else
			snprintf(key_name, PANEL_PARAM_KEY_NAME_SIZE - 1, "lcd_optical");
		panel_param_mem_put((u8 *)optical_attr, key_name, glcd_optical_dcnt);

		memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
		tmp_len = read_lcd_optical_param(index, tmp_buf);
		//ALOGD("%s, start check lcd_tcon_spi param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, glcd_optical_dcnt, (unsigned char *)optical_attr,
			tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check lcd_optical param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			save_lcd_optical_param(index, glcd_optical_dcnt,
				(unsigned char *)optical_attr);
		}
	}
	// end handle lcd_optical param

	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));
	free(optical_attr);
#ifdef CONFIG_AML_LCD_BL_LDIM
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
	free(ldim_dev_attr);
#endif
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));
	free(bl_attr);
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);
	free(lcd_buf);
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	free(tmp_buf);

#ifdef CONFIG_AML_LCD_TCON
	if (g_lcd_tcon_valid)
		handle_tcon_bin();
#endif

	return 0;

handle_panel_ini_err4:
	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));
	free(optical_attr);
handle_panel_ini_err3:
#ifdef CONFIG_AML_LCD_BL_LDIM
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
	free(ldim_dev_attr);
handle_panel_ini_err2:
#endif
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));
	free(bl_attr);
handle_panel_ini_err1:
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);
	free(lcd_buf);
handle_panel_ini_err0:
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	free(tmp_buf);

	return -1;
}

static void model_list_panel_path(int index)
{
	char *path_str, str[15];

	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	path_str = env_get(str);
	if (path_str)
		printf("current %s: %s\n", str, path_str);
}
#endif

int parse_model_sum(int index, const char *file_name, char *model_name)
{
	const char *ini_value = NULL;
#ifdef CONFIG_AML_LCD
	char str[15];
#endif

	ini_parser_init();

	if (ini_parse_file(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		ini_parser_uninit();
		return -1;
	}

#ifdef CONFIG_AML_LCD
	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	ini_value = ini_get_string(model_name, "PANELINI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set(str, ini_value);
	else
		ALOGE("%s, invalid PANELINI_PATH!!!\n", __func__);
#endif

	ini_value = ini_get_string(model_name, "EDID_14_FILE_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_edid", ini_value);
	else
		ALOGD("%s, invalid EDID_14_FILE_PATH!!!\n", __func__);
	/*
	ini_value = ini_get_string(model_name, "PQINI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_pq", ini_value);

	ini_value = ini_get_string(model_name, "AMLOGIC_AUDIO_EFFECT_INI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_audio", ini_value);
	*/
	ini_parser_uninit();

	return 0;
}

const char *get_model_sum_path(int index)
{
	char *model_path, str[15];

	if (index == 0)
		sprintf(str, "model_path");
	else
		sprintf(str, "model%d_path", index);

	model_path = env_get(str);
	if (model_path == NULL) {
		if (dynamic_partition) {
			if (index == 2)
				return DEFAULT_MODEL2_SUM_PATH2;
			else if (index == 1)
				return DEFAULT_MODEL1_SUM_PATH2;
			else
				return DEFAULT_MODEL_SUM_PATH2;
		} else {
			if (index == 2)
				return DEFAULT_MODEL2_SUM_PATH1;
			else if (index == 1)
				return DEFAULT_MODEL1_SUM_PATH1;
			else
				return DEFAULT_MODEL_SUM_PATH1;
		}
	}

	printf("%s: %s\n", __func__, model_path);
	return model_path;
}

int handle_model_list_panel_key(void)
{
	char *path_path;

	path_path = env_get("model_panel");
	if (!path_path)
		return -1;

	ini_parser_init();

	if (ini_parse_file(path_path) < 0) {
		ALOGE("%s: ini load file error!\n", __func__);
		ini_parser_uninit();
		return -1;
	}

	ini_print_all();

	ini_parser_uninit();

	return 0;
}

int handle_model_list(void)
{
	char *model, str[15], model_val[50];
	int i;

	for (i = 0; i < 3; i++) {
		memset(model_val, 0, sizeof(model_val));
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		if (read_model_name_param(i, model_val) <= 0) {
			model = env_get(str);
			if (!model) {
				if (model_debug_flag & DEBUG_NORMAL)
					ALOGD("%s, no %s\n", __func__, str);
				continue;
			}
		} else {
			model = model_val;
		}
		printf("current %s: %s\n", str, model);
#ifdef CONFIG_AML_LCD
		model_list_panel_path(i);
#endif

		ini_parser_init();

		if (ini_parse_file(get_model_sum_path(i)) < 0) {
			ALOGE("%s, ini load file error!\n", __func__);
			ini_parser_uninit();
			return -1;
		}

		printf("%s list:\n", str);
		ini_list_section();
		printf("\n");

		ini_parser_uninit();
	}

	return 0;
}

unsigned char *read_file_to_buffer(const char *filename, int *size)
{
	int rd_cnt = 0, file_size = 0;
	unsigned char *buf = NULL;

	file_size = ini_get_file_size(filename);
	if (file_size <= 0)
		return NULL;

	buf = (unsigned char *)malloc(file_size * 2);
	if (buf) {
		memset((void *)buf, '\0', (file_size * 2) * sizeof(char));
		rd_cnt = ini_read_file_to_buffer(filename, 0, file_size, buf);
		if (rd_cnt > 0) {
			*size = rd_cnt;
			return buf;
		}
		free(buf);
		buf = NULL;
	}

	return NULL;
}

int handle_model_get(const char *model, char buf[])
{
	const char *model_name = model;
	char *str = NULL;
	int index = 0, ret = -1;

	if (!model || !buf)
		goto __model_get_exit;

	if (!model_name || !strcmp(model, "model_name") ||
			!strcmp(model, "0")) {
		index = 0;
		model_name = "model_name";
	} else if (!strcmp(model, "model1_name") ||
			!strcmp(model, "1")) {
		index = 1;
		model_name = "model1_name";
	} else if (!strcmp(model, "model2_name") ||
			!strcmp(model, "2")) {
		index = 2;
		model_name = "model2_name";
	}

	ret = read_model_name_param(index, buf);
	if (ret <= 0) {
		str = env_get(model_name);
		if (!str) {
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGD("%s, no %s\n", __func__, model);
			goto __model_get_exit;
		}
		strcpy(buf, str);
	}

	ret = 0;

__model_get_exit:
	return ret;
}

int handle_model_set(const char *model, const char *val)
{
	const char *name = model;
	int index = 0, ukey_ok = 0, env_ok = 0;

	if (!val)
		return -1;

	if (!name || !strcmp(name, "model_name") ||
		    !strcmp(name, "0")) {
		index = 0;
		name = "model_name";
	} else if (!strcmp(name, "model1_name") ||
			!strcmp(name, "1")) {
		index = 1;
		name = "model1_name";
	} else if (!strcmp(name, "model2_name") ||
			!strcmp(name, "2")) {
		index = 2;
		name = "model2_name";
	}

	if (save_model_name_param(index, strlen(val) + 1,
		    (char *)val) > 0) {
		ukey_ok = 1;
	}
	env_set(name, val);
	env_ok = !env_save();

	if (!ukey_ok || !env_ok) {
		ALOGE("%s, [%d](%s): set %s=%s fail\n", __func__, index,
			(ukey_ok ? (env_ok ? "unknown" : "env") : "ukey"),
			name, val);
	} else {
		if (model_debug_flag & DEBUG_NORMAL) {
			ALOGD("%s, [%d]: set %s=%s ok\n", __func__, index,
				name, val);
		}
	}
	return 0;
}

int handle_model_sum(void)
{
	char *model, str[15], model_val[50];
#ifdef CONFIG_AML_LCD
	char *file_name, *p;
#endif
	int i, ret;

	for (i = 0; i < 3; i++) {
		memset(model_val, 0, sizeof(model_val));
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		if (read_model_name_param(i, model_val) <= 0) {
			model = env_get(str);
			if (!model) {
				if (model_debug_flag & DEBUG_NORMAL)
					ALOGD("%s, no %s\n", __func__, str);
				continue;
			}
		} else {
			model = model_val;
		}
		ret = parse_model_sum(i, get_model_sum_path(i), model);
		if (ret < 0)
			continue;
#ifdef CONFIG_AML_LCD
		if (i == 0)
			sprintf(str, "model_panel");
		else
			sprintf(str, "model%d_panel", i);
		file_name = env_get(str);
		if (!file_name) {
			ALOGE("%s, %s path error!!!\n", __func__, str);
			return -1;
		}
		p = strrchr(file_name, '.');
		if (p && (!strncmp(p + 1, "ini", 3) || !strncmp(p + 1, "INI", 3))) {
			ret = handle_panel_ini(i);
#ifdef CONFIG_CMD_AML_MODEL
			//ret = read_panel_file(i, file_name);
			//set_lcd_panel_file_type(i, PANEL_FILE_INI); // maybe support later
#endif
		} else { //regard as json file, will be parse later
			ret = read_panel_file(i, file_name);
			set_lcd_panel_file_type(i, PANEL_FILE_JSON);
		}
#endif
	}

	return 0;
}

