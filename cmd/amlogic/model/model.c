// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <command.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <amlogic/partition_table.h>
#include <amlogic/aml_model.h>
#include "model_log.h"
#include "model_size.h"
#include "model.h"

#define LOG_TAG     "model"
#define LOG_NDEBUG  0

#ifndef CONFIG_YOCTO
#define DEFAULT_MODEL_PATH1      "/odm/etc/tvconfig/model"
#define DEFAULT_MODEL_SUM_PATH1 "/odm/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/odm/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/odm/etc/tvconfig/model/model2_sum.ini"
#else
#define DEFAULT_MODEL_PATH1      "/vendor/etc/tvconfig/model"
#define DEFAULT_MODEL_SUM_PATH1 "/vendor/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/vendor/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/vendor/etc/tvconfig/model/model2_sum.ini"
#endif
#define DEFAULT_MODEL_PATH2      "/odm_ext/etc/tvconfig/model"
#define DEFAULT_MODEL_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model2_sum.ini"
#define DEFAULT_MODEL_NAME "DEFAULT"
#define DEFAULT_SECTION "GENERAL_CONFIG_FROM_MODEL_SUM"

/*********************************  common api  *************************************/
static int get_model_multi_max_cnt(void)
{
	int cnt_max = 1;

#ifdef CONFIG_VOUT_MULTI
	cnt_max = CONFIG_VOUT_MULTI;
#endif
	return cnt_max;
}

int model_set_panel_misc(struct panel_misc_s *misc_attr)
{
	int tmp_val = 0;
	char *rev_ctrl = NULL;
	char *ret = NULL;
	char param[32], buf[64] = {0};
	unsigned int panel_reverse = 0, display_layer = 0, n;

	if (!misc_attr)
		return -1;

	if (misc_attr->disp_idx == 0xff)
		return -1;

	tmp_val = env_get_ulong("model_outputmode_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_outputmode_bypass\n");
		goto model_set_panel_misc_next;
	}

	if (misc_attr->disp_idx == 0)
		sprintf(param, "outputmode");
	else
		sprintf(param, "outputmode%d", misc_attr->disp_idx + 1);
	ALOGD("%s: %s is (%s)\n", __func__, param, misc_attr->outputmode);
	snprintf(buf, 63, "setenv %s %s", param, misc_attr->outputmode);
	run_command(buf, 0);

model_set_panel_misc_next:
	tmp_val = env_get_ulong("model_connector_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_connector_bypass\n");
		goto model_set_panel_misc_next2;
	}

	sprintf(param, "connector%d_type", misc_attr->disp_idx);
	misc_attr->connector_type[sizeof(misc_attr->connector_type) - 1] = '\0';
	ret = strstr(misc_attr->connector_type, "_");
	if (ret)
		misc_attr->connector_type[ret - misc_attr->connector_type] = '-';
	ALOGD("%s: %s is (%s)\n", __func__, param, misc_attr->connector_type);
	snprintf(buf, 63, "setenv %s %s", param, misc_attr->connector_type);
	run_command(buf, 0);

model_set_panel_misc_next2:
	if (misc_attr->version >= 2)
		goto model_set_panel_misc_next3;
	rev_ctrl = env_get("reverse_ctrl");
	if (!rev_ctrl || strcmp(rev_ctrl, "0") == 0) {
		ALOGD("%s: panel_reverse is (%s)\n", __func__, misc_attr->panel_reverse);
		if (strcmp(misc_attr->panel_reverse, "null") == 0 ||
		    strcmp(misc_attr->panel_reverse, "0") == 0 ||
		    strcmp(misc_attr->panel_reverse, "false") == 0 ||
		    strcmp(misc_attr->panel_reverse, "no_rev") == 0) {
			panel_reverse = 0;
		} else if (strcmp(misc_attr->panel_reverse, "true") == 0 ||
			   strcmp(misc_attr->panel_reverse, "1") == 0 ||
			   strcmp(misc_attr->panel_reverse, "have_rev") == 0) {
			panel_reverse = 1;
		} else if (strcmp(misc_attr->panel_reverse, "x_rev") == 0 ||
			   strcmp(misc_attr->panel_reverse, "2") == 0) {
			panel_reverse = 2;
		} else if (strcmp(misc_attr->panel_reverse, "y_rev") == 0 ||
			   strcmp(misc_attr->panel_reverse, "3") == 0) {
			panel_reverse = 3;
		} else {
			panel_reverse = 0;
		}
		if (panel_reverse) {
			if (strcmp(misc_attr->display_layer, "osd0") == 0 ||
			    strcmp(misc_attr->display_layer, "0") == 0)
				display_layer = 0;
			else if (strcmp(misc_attr->display_layer, "osd1") == 0 ||
				 strcmp(misc_attr->display_layer, "1") == 0)
				display_layer = 1;
			else
				display_layer = 4;
		}
		switch (panel_reverse) {
		case 1:
			run_command("setenv panel_reverse 1", 0);
			switch (display_layer) {
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
			switch (display_layer) {
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
			switch (display_layer) {
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

model_set_panel_misc_next3:
	n = misc_attr->hmirror_val << 1 | misc_attr->vmirror_val;
	snprintf(buf, 64, "setenv osd_reverse %s%s",
		misc_attr->disp_layer_val == 0 ?
			"osd0," : misc_attr->disp_layer_val == 1 ? "osd1," : "all,",
		n == 0 ? "n" : n == 1 ? "y_rev" : n == 2 ? "x_rev" : "true");
	ALOGD("%s: %s\n", __func__, buf);
	run_command(buf, 0);

	snprintf(buf, 64, "setenv video_reverse %c",
		n == 0 ? '0' : n == 1 ? '3' : n == 2 ? '2' : '1');
	ALOGD("%s: %s\n", __func__, buf);
	run_command(buf, 0);

	snprintf(buf, 64, "setenv panel_reverse %c",
		n == 0 ? '0' : n == 1 ? '3' : n == 2 ? '2' : '1');
	ALOGD("%s: %s\n", __func__, buf);
	run_command(buf, 0);

	return 0;
}

static const char *get_model_path(int index)
{
	char *model_path, str[15];

	if (index == 0)
		sprintf(str, "model_path");
	else
		sprintf(str, "model%d_path", index);

	model_path = env_get(str);
	if (!model_path) {
		if (dynamic_partition)
			return DEFAULT_MODEL_PATH2;
		else
			return DEFAULT_MODEL_PATH1;
	}

	printf("%s: %s\n", __func__, model_path);
	return model_path;
}

/*********************************  cmd api *************************************/
static int parse_model_sum(int index, const char *file_name, char *model_name)
{
	void *local_ini_mem, *psec;
	const char *str = NULL;
#ifdef CONFIG_AML_LCD
	char model_panel[20], *p;
#endif

	local_ini_mem = handle_ini_file_parse(file_name, 0);
	if (!local_ini_mem) {
		ALOGE("%s: error: %s\n", __func__, file_name);
		return -1;
	}

	psec = handle_ini_get_section(local_ini_mem, model_name);
	if (!psec) {
		ALOGE("%s: section %s: error: %s\n", __func__, model_name, file_name);
		return -1;
	}

#ifdef CONFIG_AML_LCD
	str = handle_ini_get_str(local_ini_mem, psec, "PANELINI_PATH", "null");
	if (strcmp(str, "null") == 0)
		ALOGE("%s: %s: invalid PANELINI_PATH!\n", __func__, model_name);
	if (index == 0)
		sprintf(model_panel, "model_panel");
	else
		sprintf(model_panel, "model%d_panel", index);
	env_set(model_panel, str);
	set_panel_file_path(index, str);

	p = strrchr(str, '.');
	if (p && (!strncmp(p + 1, "ini", 3) || !strncmp(p + 1, "INI", 3)))
		set_lcd_panel_file_type(index, PANEL_FILE_INI);
	else //regard as json file, will be parse later
		set_lcd_panel_file_type(index, PANEL_FILE_JSON);

	str = handle_ini_get_str(local_ini_mem, psec, "BACKLIGHT_PATH", NULL);
	if (str) {
		if (index == 0)
			snprintf(model_panel, 19, "model_backlight");
		else
			snprintf(model_panel, 19, "model%d_backlight", index);
		env_set(model_panel, str);
		set_bl_file_path(index, str);

		p = strrchr(str, '.');
		if (p && (!strncmp(p + 1, "ini", 3) || !strncmp(p + 1, "INI", 3)))
			set_bl_panel_file_type(index, PANEL_FILE_INI);
		else //regard as json file, will be parse later
			set_bl_panel_file_type(index, PANEL_FILE_JSON);
	}
#endif

	str = handle_ini_get_str(local_ini_mem, psec, "EDID_14_FILE_PATH", "null");
	if (strcmp(str, "null") == 0)
		ALOGD("%s: %s: invalid EDID_14_FILE_PATH!\n", __func__, model_name);
	env_set("model_edid", str);

	/*
	 *str = handle_ini_get_str(local_ini_mem, psec, "PQINI_PATH", "null");
	 *env_set("model_pq", str);
	 *
	 *str = handle_ini_get_str(local_ini_mem, psec, "AMLOGIC_AUDIO_EFFECT_INI_PATH", "null");
	 *env_set("model_audio", str);
	 */

	handle_ini_parser_uninit(local_ini_mem);

	return 0;
}

bool validate_oemkey(const char *value)
{
	// Length check (11~32)
	int len = strlen(value);

	if (len < 11 || len > 32) {
		printf("Invalid length: %d (must be 11~32)\n", len);
		return false;
	}

	// 1.Check ATV prefix
	if (strncmp(value, "ATV", 3) != 0) {
		printf("Invalid prefix: must start with 'ATV'\n");
		return false;
	}

	// 2.Verify Schema version 00
	if (value[3] != '0' || value[4] != '0') {
		printf("Invalid schema version: must be '00'\n");
		return false;
	}

	// 3.Verify the voice remote control ID
	if (value[5] != '0' && value[5] != '1') {
		printf("Invalid voice flag: must be '0' or '1'\n");
		return false;
	}

	// 4.Verify PSZ panel size (must be 3 digits)
	for (int i = 6; i <= 8; ++i) {
		if (!isdigit(value[i])) {
			printf("Invalid PSZ: must be 3 digits\n");
			return false;
		}
	}

	// 5.Verify the year YR (must be 2 digits)
	for (int i = 9; i <= 10; ++i) {
		if (!isdigit(value[i])) {
			printf("Invalid YR: must be 2 digits\n");
			return false;
		}
	}
	return true;
}

int parse_model_oemkey(int index, const char *file_name, char *model_name)
{
	void *local_ini_mem, *psec;
	const char *oemkey_value = NULL;
	int ret = 1;

	local_ini_mem = handle_ini_file_parse(file_name, 0);

	if (!local_ini_mem) {
		ALOGE("%s: error: %s\n", __func__, file_name);
		return -1;
	}
	//read unkfykey first
	run_command("keyman read oemkey ${loadaddr} str", 0);
	char *oemkey = env_get("oemkey");

	psec =  handle_ini_get_section(local_ini_mem, model_name);
	if (!psec) {
		ALOGE("%s: section %s: error: %s\n", __func__, model_name, file_name);
		return -1;
	}

	oemkey_value = handle_ini_get_str(local_ini_mem, psec, "OEMKEY_NAME", "null");
	if (oemkey_value && strcmp(oemkey_value, "null") != 0) {
		if (validate_oemkey(oemkey_value)) {
			printf("oemkey_name is valid: %s\n", oemkey_value);
			env_set("oemkey_name", oemkey_value);
			ret = 0;
			if (oemkey && strncmp(oemkey, oemkey_value, strlen(oemkey)) != 0) {
				char result[65] = "keyman write oemkey str ";//size 23

				if (strlen(oemkey_value) > 32) {//str_value size 32
					printf("invalid oemkey\n");
					return ret;
				}
				strcat(result, oemkey_value);
				run_command(result, 0);
			}
		} else {
			printf("Invalid OEMKEY_NAME format\n");
			if (oemkey && strcmp(oemkey, "null") != 0) {
				env_set("oemkey_name", oemkey);
				ret = 0;
			} else {
				printf("no oemkey in unifykey\n");
			}
		}
	} else {
		printf("no oemkey in model_ini\n");
		if (oemkey && strcmp(oemkey, "null") != 0) {
			printf("use oemkey in unifykey\n");
			env_set("oemkey_name", oemkey);
			ret = 0;
		}
	}

	handle_ini_parser_uninit(local_ini_mem);
	return ret;
}

int split_file_path(const char *file_path, char part_name[], char file_name[], const char *ext_name)
{
	int i = 0;
	char *tmp_start_ptr = NULL;
	char *tmp_end_ptr = NULL;

	if (!file_path) {
		debug("%s, file_path is NULL!!!\n", __func__);
		return -1;
	}

	tmp_start_ptr = strchr((char *)file_path, '/');
	if (tmp_start_ptr != file_path) {
		debug("%s, we need one abstract file path!!!  %s.\n", __func__, file_path);
		return -1;
	}

	tmp_end_ptr = strchr(tmp_start_ptr + 1, '/');
	if (!tmp_end_ptr) {
		debug("%s, there is only partition name in the path!!!\n", __func__);
		return -1;
	}

	strncpy(part_name, tmp_start_ptr + 1, tmp_end_ptr - tmp_start_ptr - 1);
	part_name[tmp_end_ptr - tmp_start_ptr - 1] = '\0';

	tmp_start_ptr = tmp_end_ptr;

	i = 0;
	while (*tmp_end_ptr && i < 256) {
		tmp_end_ptr++;
		i++;
	}
	if (i >= 256) {
		debug("%s, file path is too long (%d)!!!\n", __func__, i);
		return -1;
	}

	strncpy(file_name, tmp_start_ptr, i);
	file_name[i] = '\0';

	if (ext_name) {
		if (strlen(ext_name) > 0) {
			while (tmp_end_ptr != tmp_start_ptr && *tmp_end_ptr != '.')
				tmp_end_ptr--;

			if (*tmp_end_ptr != '.') {
				debug("%s, the file path \"%s\" doesn't have ext name!!!\n",
					__func__, file_path);
				return -1;
			}
			if (strcmp(tmp_end_ptr + 1, ext_name)) {
				debug("%s, file path \"%s\" != ext name \"%s\"!!!\n",
					__func__, file_path, ext_name);
				return -1;
			}
		}
	}

	return 0;
}

int parse_model_logofile(int index, const char *file_name, char *model_name)
{
	void *local_ini_mem, *psec;
	const char *logo_value = NULL;
	char *str = NULL;
	char temp_file_path[256] = {0};
	char partition[128] = {0};
	char filename[256] = {0};
	int ret = 1;

	local_ini_mem = handle_ini_file_parse(file_name, 0);
	if (!local_ini_mem) {
		ALOGE("%s: error: %s\n", __func__, file_name);
		return -1;
	}

	psec = handle_ini_get_section(local_ini_mem, model_name);
	if (!psec) {
		ALOGE("%s: section %s: error: %s\n", __func__, model_name, file_name);
		return -1;
	}

	logo_value = handle_ini_get_str(local_ini_mem, psec, "LOGO_FILE_NAME", "null");
	//logo_value = "/logo_files/test_logo.jpg"; //for test
	if (strcmp(logo_value, "null") != 0) {
		env_set("logo_path", "exist");
		printf("logo_file_name is %s\n", logo_value);

		memset(temp_file_path, 0, sizeof(temp_file_path));
		if (!strncmp("/mnt/vendor", logo_value, 11)) {
			strlcpy(temp_file_path, logo_value + 11, (strlen(logo_value) - 10));

			if (split_file_path(temp_file_path, partition, filename, NULL) < 0)
				return 1;

			debug("filename is %s\n", filename);
			debug("partition is %s\n", partition);
			env_set("board_logo_part", partition);
		} else {
			strlcpy(filename, logo_value, (strlen(logo_value) + 1));
			printf("filename is %s\n", filename);
		}

		env_set("logo_file_name", filename);
	} else {
		printf("invalid LOGO_FILE_PATH\n");
	}

	str = env_get("fb_width");
	if (str && simple_strtoul(str, NULL, 10) == 3840)
		env_set("fb_for_4k2k", "1");
	else
		env_set("fb_for_4k2k", "0");
	ret = 0;

	handle_ini_parser_uninit(local_ini_mem);

	return ret;
}

static const char *get_model_sum_path(int index)
{
	char *model_path, str[15];

	if (index == 0)
		sprintf(str, "model_path");
	else
		sprintf(str, "model%d_path", index);

	model_path = env_get(str);
	if (!model_path) {
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

#ifdef CONFIG_AML_LCD
static char *model_list_panel_path(int index)
{
	char *path_str, str[15];

	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	path_str = env_get(str);
	if (path_str)
		printf("current %s: %s\n", str, path_str);

	return path_str;
}
#endif

static int read_model_name_param_from_ukey(int index, unsigned char *data_buf, int data_size)
{
	int rd_size = 0;
	const char *item_name = CS_MODEL_NAME_ITEM_NAME;
	unsigned char *read_buf;

	if (!data_buf)
		return -1;

	if (index == 1)
		item_name = CS_MODEL1_NAME_ITEM_NAME;
	else if (index == 2)
		item_name = CS_MODEL2_NAME_ITEM_NAME;

	read_buf = model_detect_ukey_data(item_name, &rd_size);
	if (!read_buf || rd_size <= 0)
		return -1;
	rd_size += 1; //add \0 for string
	if (data_size < rd_size) {
		ALOGE("%s: size error: %s\n", __func__, item_name);
		memset(read_buf, 0, rd_size);
		free(read_buf);
		return -1;
	}
	strlcpy(data_buf, read_buf, rd_size);
	memset(read_buf, 0, rd_size);
	free(read_buf);

	return rd_size;
}

static int save_model_name_param(int index, int wr_size, unsigned char *data_buf)
{
	int tmp_ret = 0;
	const char *item_name = CS_MODEL_NAME_ITEM_NAME;

	if (!data_buf)
		return -1;

	if (index == 1)
		item_name = CS_MODEL1_NAME_ITEM_NAME;
	else if (index == 2)
		item_name = CS_MODEL2_NAME_ITEM_NAME;

	tmp_ret = model_write_ukey_data(item_name, data_buf, wr_size);

	return tmp_ret;
}

static int get_panel_model_name_param(int index, char *model_buf, int buf_size,
				      char *key_buf, int key_size)
{
	char *model, env_str[64];
	int dbg_env_flag = 1;

	/****** factory mode *******/
	if (index == 0)
		snprintf(key_buf, key_size, "model_name_factory");
	else
		snprintf(key_buf, key_size, "model%d_name_factory", index);
	model = env_get(key_buf);
	if (model) {
		if (strcmp(model, "none")) {
			strlcpy(model_buf, model, buf_size);
			return 0;
		}
	}
	/***************************/

	if (index == 0)
		snprintf(key_buf, key_size, "model_name");
	else
		snprintf(key_buf, key_size, "model%d_name", index);
	if (read_model_name_param_from_ukey(index, model_buf, buf_size) > 0) {
		//find unifykey model_name
		dbg_env_flag = env_get_ulong("model_env_flag", 16, 0x1);
		if (dbg_env_flag) {
			env_set(key_buf, model_buf);
			sprintf(env_str, "update_env_part -p -f %s", key_buf);
			run_command(env_str, 0);
			ALOGD("%s: update env[%s]=%s\n", __func__, key_buf, model_buf);
		}
		return 0;
	}

	model = env_get(key_buf);
	if (model) {
		strlcpy(model_buf, model, buf_size);
		return 0;
	}

	ALOGD("%s: no %s\n", __func__, key_buf);
	return -1;
}

static int handle_model_list(void)
{
	void *local_ini_mem;
	const char *model_path;
	char str[32], model_val[64];
	int i, cnt_max, ret;

	cnt_max = get_model_multi_max_cnt();
	for (i = 0; i < cnt_max; i++) {
		ret = get_panel_model_name_param(i, model_val, 64, str, 32);
		if (ret)
			continue;

		printf("current %s: %s\n", str, model_val);
#ifdef CONFIG_AML_LCD
		model_list_panel_path(i);
#endif

		model_path = get_model_sum_path(i);
		local_ini_mem = handle_ini_file_parse(model_path, 0);
		if (!local_ini_mem) {
			ALOGE("%s: error: %s\n", __func__, model_path);
			return -1;
		}

		printf("%s list:\n", str);
		handle_ini_list_section(local_ini_mem);
		printf("\n");
		handle_ini_parser_uninit(local_ini_mem);
	}

	return 0;
}

int handle_model_oemkey(void)
{
	char *model, str[32], model_val[64];
	unsigned int print_flag;
	int i, cnt_max;
	int ret = 1;
	char model_path[128];

	print_flag = env_get_ulong("model_debug_print", 10, 0xffff);
	if (print_flag != 0xffff) {
		model_set_log_level(print_flag);
		printf("model_debug_flag: %d\n", print_flag);
	}

	cnt_max = get_model_multi_max_cnt();
	for (i = 0; i < cnt_max; i++) {
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		if (read_model_name_param_from_ukey(i, model_val, 64) <= 0) {
			model = env_get(str);
			if (!model) {
				ALOGD("%s: no %s\n", __func__, str);
				continue;
			}
		} else {
			model = model_val;
		}
		ALOGI("%s: %s: %s\n", __func__, str, model);
		ret = parse_model_oemkey(i, get_model_sum_path(i), model);
		if (ret != 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
					get_model_path(i), model);
			printf("oemkey try to read from model_path: %s\n", model_path);
			ret = parse_model_oemkey(i, model_path, DEFAULT_SECTION);
		}
		if (ret == 0)
			break;
	}

	return ret;
}

int handle_model_logofile(void)
{
	char *model, str[32], model_val[64];
	unsigned int print_flag;
	int i, cnt_max;
	int ret = 1;
	char model_path[128];

	print_flag = env_get_ulong("model_debug_print", 10, 0xffff);
	if (print_flag != 0xffff) {
		model_set_log_level(print_flag);
		printf("model_debug_flag: %d\n", print_flag);
	}

	cnt_max = get_model_multi_max_cnt();
	for (i = 0; i < cnt_max; i++) {
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		if (read_model_name_param_from_ukey(i, model_val, 64) <= 0) {
			model = env_get(str);
			if (!model) {
				ALOGD("%s: no %s\n", __func__, str);
				continue;
			}
		} else {
			model = model_val;
		}
		ALOGD("%s: %s: %s\n", __func__, str, model);
		ret = parse_model_logofile(i, get_model_sum_path(i), model);
		if (ret != 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
				get_model_path(i), model);
			printf("try to read from model_path: %s\n", model_path);
			ret = parse_model_logofile(i, model_path, DEFAULT_SECTION);
		}
		if (ret == 0)
			break;
	}

	return ret;
}

int handle_model_sum(void)
{
	char str[32], model_val[64];
	unsigned int print_flag;
	char model_path[128];
	int i, cnt_max, ret = 0;

	print_flag = env_get_ulong("model_debug_print", 10, 0xffff);
	if (print_flag != 0xffff) {
		model_set_log_level(print_flag);
		printf("model_debug_flag: %d\n", print_flag);
	}

	cnt_max = get_model_multi_max_cnt();
	for (i = 0; i < cnt_max; i++) {
		ret = get_panel_model_name_param(i, model_val, 64, str, 32);
		if (ret)
			continue;

		ALOGI("%s: %s: %s\n", __func__, str, model_val);
		ret = parse_model_sum(i, get_model_sum_path(i), model_val);
		if (ret < 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
				get_model_path(i), model_val);
			ret = parse_model_sum(i, model_path, DEFAULT_SECTION);
			debug("model_sum try to read from model_path: %s %s\n",
			      model_path, ret ? "fail" : "ok");
		}
	}

	return 0;
}

int handle_model_get(const char *model, char *buf, int buf_size)
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

	ret = read_model_name_param_from_ukey(index, buf, buf_size);
	if (ret <= 0) {
		str = env_get(model_name);
		if (!str) {
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

	if (!save_model_name_param(index, strlen(val) + 1, (char *)val))
		ukey_ok = 1;
	env_set(name, val);
	env_ok = !env_save();

	if (!ukey_ok || !env_ok) {
		ALOGE("%s, [%d](%s): set %s=%s fail\n", __func__, index,
		       (ukey_ok ? (env_ok ? "unknown" : "env") : "ukey"),
		       name, val);
	} else {
		ALOGD("%s, [%d]: set %s=%s ok\n", __func__, index,
			name, val);
	}
	return 0;
}

/*********************************  cmd  *************************************/
static int do_ini_model_list(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	if (argc == 1) {
		ret = handle_model_list();
		return ret;
	}
	if (strcmp(argv[1], "lcd") == 0 || strcmp(argv[1], "panel") == 0) {
		aml_lcd_panel_dump(0, NULL);
		return 0;
	} else if (strcmp(argv[1], "lcd1") == 0 || strcmp(argv[1], "panel1") == 0) {
		aml_lcd_panel_dump(1, NULL);
		return 0;
	} else if (strcmp(argv[1], "lcd2") == 0 || strcmp(argv[1], "panel2") == 0) {
		aml_lcd_panel_dump(2, NULL);
		return 0;
	}

	return CMD_RET_USAGE;
}

U_BOOT_CMD(model_list, 4, 0, do_ini_model_list,
	"list ini model name",
	" "
);

static int do_oemkey_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	if (argc > 1)
		return CMD_RET_USAGE;

	ret = handle_model_oemkey();
	return ret;
}

U_BOOT_CMD
(oemkey_read, 4, 0, do_oemkey_read,
	"list ini model name",
	" "
);

static int do_logo_display(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret;

	if (argc > 1)
		return CMD_RET_USAGE;

	ret = handle_model_logofile();
	return ret;
}

U_BOOT_CMD
(logo_display, 4, 0, do_logo_display,
	"list ini model name",
	" "
);

static int do_ini_model(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

	if (argc > 1)
		return CMD_RET_USAGE;

	ret = handle_model_sum();
	return ret;
}

U_BOOT_CMD(ini_model, 4, 0, do_ini_model,
	"parse ini file by env model_name",
	" "
);

int parse_model_powermode(int index, const char *file_name, char *model_name)
{
	void *local_ini_mem, *psec;
	const char *powermode_value = NULL;
	int ret = -1;

	local_ini_mem = handle_ini_file_parse(file_name, 0);
	if (!local_ini_mem) {
		ALOGE("%s: error: %s\n", __func__, file_name);
		return ret;
	}

	psec = handle_ini_get_section(local_ini_mem, model_name);
	if (!psec) {
		ALOGE("%s: section %s: error: %s\n", __func__, model_name, file_name);
		return ret;
	}

	powermode_value = handle_ini_get_str(local_ini_mem, psec, "POWER_ON_MODE", "null");
	if (strcmp(powermode_value, "null") != 0) {
		ALOGI("powermode_value is %s\n", powermode_value);
		if (!strcmp(powermode_value, "on") || !strcmp(powermode_value, "standby") ||
		    !strcmp(powermode_value, "last")) {
			env_set("powermode", powermode_value);
			env_set("powermode_ini", "1");
			env_save();
		}
		ret = 0;
	} else {
		env_set("powermode_ini", "2");
		env_save();
		ALOGD("not find POWER_ON_MODE in ini files !!!\n");
	}

	handle_ini_parser_uninit(local_ini_mem);

	return ret;
}

int handle_model_powermode(void)
{
	char *model, str[15];
	unsigned int print_flag;
	int i, cnt_max;
	int ret = -1;
	char model_path[128];

	print_flag = env_get_ulong("model_debug_print", 10, 0xffff);
	if (print_flag != 0xffff) {
		model_set_log_level(print_flag);
		ALOGI("model_debug_flag: %d\n", print_flag);
	}

	cnt_max = get_model_multi_max_cnt();
	for (i = 0; i < cnt_max; i++) {
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		model = env_get(str);
		if (!model) {
			ALOGD("%s: no %s\n", __func__, str);
			continue;
		}
		ALOGI("%s: %s: %s\n", __func__, str, model);

		ret = parse_model_powermode(i, get_model_sum_path(i), DEFAULT_MODEL_NAME);
		if (ret != 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
			        get_model_path(i), model);
			ALOGD("try to read from model_path: %s\n", model_path);
			ret = parse_model_powermode(i, model_path, DEFAULT_SECTION);
		}

		if (ret != 0)
			continue;
	}

	return ret;
}

static int do_ini_powermode(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = -1;

	if (argc > 1)
		return CMD_RET_USAGE;

	char str[] = "powermode_ini";
	char *val = NULL;

	val = env_get(str);
	if (!val) {
		ALOGI("powermode_ini is not set");
	} else if (strcmp(val, "0") == 0 || strcmp(val, "2") == 0) {
		ALOGI("powermode_ini is %s,not read from ini file\n", val);
		return 0;
	}

	ret = handle_model_powermode();
	return ret;
}

U_BOOT_CMD(ini_powermode, 4, 0, do_ini_powermode,
		"parse ini file to get powermode",
		" "
);

