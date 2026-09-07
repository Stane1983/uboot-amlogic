// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2025 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <command.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <ext4fs.h>
#include <linux/stat.h>
#include <fs.h>
#include <amlogic/partition_table.h>
#include <amlogic/keyunify.h>
#include <amlogic/emmc_partitions.h>
#include "powerkey.h"

#define LOG_TAG "ini_powerkey"
#define LOG_NDEBUG 0

#define INI_MAX_LINE 2048
#define MAX_LINE_LEN 1024
#define DEFAULT_MODEL_SUM_PATH "/odm_ext/etc/tvconfig/model/model_sum.ini"
#define CS_BLOCK_DEV_INTERFACE "mmc"
#define CS_BLOCK_DEV_MARJOR_NUM "1"
#define DEFAULT_MODEL_PATH      "/odm_ext/etc/tvconfig/model"
#define DEFAULT_SECTION "GENERAL_CONFIG_FROM_MODEL_SUM"

unsigned int power_key[MAX_KEY_SUPPORT];
int power_key_num;
unsigned int adc_power_key[MAX_KEY_SUPPORT]  = {0};
unsigned int adc_power_key_tempbuf[128] = {0};
#define CC_MAX_INI_FILE_NAME_LEN         (512)
#define CMD_MAX_LEN                      (1024)

/* Strip whitespace chars off end of given string, in place. Return s. */
static char *rstrip(char *s)
{
	char *p = s + strlen(s);

	while (p > s && isspace((unsigned char)(*--p)))
		*p = '\0';
	return s;
}

/* Return pointer to first non-whitespace char in given string. */
static char *lskip(const char *s)
{
	while (*s && isspace((unsigned char)(*s)))
		s++;
	return (char *)s;
}

int tolower_ex(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	else
		return c;
}

int htoi(char s[])
{
	// Add null pointer check
	if (!s) {
		printf("%s: NULL pointer passed\n", __func__);
		return 0;
	}
	int i;
	int n = 0;

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		i = 2;
	else
		i = 0;

	for (; (s[i] >= '0' && s[i] <= '9') ||
		(s[i] >= 'a' && s[i] <= 'z') ||
		(s[i] >= 'A' && s[i] <= 'Z'); ++i) {
		if (tolower_ex(s[i]) > '9')
			n = 16 * n + (10 + tolower_ex(s[i]) - 'a');
		else
			n = 16 * n + (tolower_ex(s[i]) - '0');
	}
	return n;
}

char *skipspace(char *src)
{
	// Add input parameter check
	if (!src)
		return NULL;

	char *p = src;

	while (' ' == *p && '\0' != *p)
		p++;

	return p;
}

static void StrTrim(char **s)
{
	// Add input parameter check
	if (!s || !*s)
		return;

	int i;
	int len = strlen(*s);

	for (i = len - 1; i >= 0; i--) {
		if ((*s)[i] <= ' ')
			(*s)[i] = 0;
		else
			break;
	}

	while (**s) {
		if (**s <= ' ')
			(*s)++;
		else
			break;
	}
}

static void StrLineTrim(char *line, int length)
{
	char *tmp_ptr = line;

	if (!line)
		return;
	/*trim comment string*/
	while (*tmp_ptr && ((tmp_ptr - line) <= length)) {
		if (*tmp_ptr == '#' || *tmp_ptr == ';') {
			*tmp_ptr = '\0';
			break;
		}
		tmp_ptr++;
	}
}

static int StrEmpty(char *str)
{
	int len = strlen(str);

	return (len == 0);
}

int GetKeyValue(char *line, char **key, char **value)
{
	// Add input parameter check
	if (!line || !key || !value)
		return -1;

	char *p1, *p2;

	p1 = line;
	p2 = strchr(line, '=');
	if (p2) {
		*p2++ = 0;
		StrTrim(&p2);
	}
	if (!p2 || !*p2)
		return -1;
	if (!p1 || !*p1)
		return -1;
	StrTrim(&p1);
	*value = p2;
	*key = p1;
	return 0;
}

static int split_file_path(const char *file_path,
	char part_name[], char file_name[],
	const char *ext_name)
{
	// Add target buffer non-null check
	if (!part_name || !file_name) {
		printf("%s: NULL buffer pointer\n", __func__);
		return -1;
	}
	int i = 0;
	char *tmp_start_ptr = NULL;
	char *tmp_end_ptr = NULL;
	char *slot_name;

	if (!file_path) {
		printf("%s, file_path NULL!\n", __func__);
		return -1;
	}

	tmp_start_ptr = strchr((char *)file_path, '/');
	if (tmp_start_ptr != file_path) {
		printf("%s, we need one abstract file path!!!  %s.\n", __func__, file_path);
		return -1;
	}

	tmp_end_ptr = strchr(tmp_start_ptr + 1, '/');
	if (!tmp_end_ptr) {
		printf("%s, there is only partition name in the path!!!\n", __func__);
		return -1;
	}

	strlcpy(part_name, tmp_start_ptr + 1, tmp_end_ptr - tmp_start_ptr);
	//part_name[tmp_end_ptr - tmp_start_ptr - 1] = '\0';
	printf("%s, partition name is %s\n", __func__, part_name);

	if (has_boot_slot == 1) {
		slot_name = env_get("slot-suffixes");
		if (!slot_name) {
			run_command("get_valid_slot", 0);
			slot_name = env_get("slot-suffixes");
		}
		// Add safety check for string concatenation
		if (slot_name) {
			size_t current_len = strlen(part_name);

			if (strcmp(slot_name, "0") == 0 &&
				current_len + 3 < CC_MAX_INI_FILE_NAME_LEN)
				strcat(part_name, "_a");
			else if (strcmp(slot_name, "1") == 0 &&
				current_len + 3 < CC_MAX_INI_FILE_NAME_LEN)
				strcat(part_name, "_b");
		}
		//printf("%s, partition name is %s\n", __func__, part_name);
	}

	tmp_start_ptr = tmp_end_ptr;

	i = 0;
	while (*tmp_end_ptr && i < CC_MAX_INI_FILE_NAME_LEN) {
		tmp_end_ptr++;
		i++;
	}
	if (i >= CC_MAX_INI_FILE_NAME_LEN) {
		printf("%s, file path is too long (%d)!!!\n", __func__, i);
		return -1;
	}

	strlcpy(file_name, tmp_start_ptr, i + 1);
	//file_name[i] = '\0';
	printf("%s, file name is %s\n", __func__, file_name);

	if (ext_name) {
		if (strlen(ext_name) > 0) {
			while (tmp_end_ptr != tmp_start_ptr && *tmp_end_ptr != '.')
				tmp_end_ptr--;

			if (*tmp_end_ptr != '.')
				return -1;

			if (strcmp(tmp_end_ptr + 1, ext_name))
				return -1;
		}
	}

	return 0;
}

static int set_block_device(const char *part_name)
{
	int part_no = 0;
	char part_buf[128] = {0};
	char tmp_buf[128] = {0};

	// Add input parameter check
	if (!part_name) {
		printf("%s: NULL part_name\n", __func__);
		return -1;
	}

#ifdef CONFIG_MMC_MESON_GX
	part_no = get_partition_num_by_name((char *)part_name);
#endif

	//printf("%s, part_no is %d\n", __func__, part_no);
	if (part_no >= 0) {
		// Use safer string operation functions
		snprintf(part_buf, sizeof(part_buf), "%s:", CS_BLOCK_DEV_MARJOR_NUM);
		if (snprintf(tmp_buf, sizeof(tmp_buf), "%x", part_no) < sizeof(tmp_buf)) {
			// Ensure sufficient space for concatenation
			if (strlen(part_buf) + strlen(tmp_buf) < sizeof(part_buf)) {
				strcat(part_buf, tmp_buf);
				return fs_set_blk_dev(CS_BLOCK_DEV_INTERFACE,
					part_buf, FS_TYPE_EXT);
			}
		}
	}

	return -1;
}

int powerkey_is_file_exist(const char *file_path)
{
	char part_name[CC_MAX_INI_FILE_NAME_LEN];
	char file_name[CC_MAX_INI_FILE_NAME_LEN];

	memset((void *)part_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	memset((void *)file_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	if (split_file_path(file_path, part_name, file_name, NULL) < 0)
		return 0;

	if (set_block_device(part_name) < 0)
		return 0;

	return fs_exists(file_name);
}

int powerkey_get_file_size(const char *file_path)
{
	loff_t file_size = 0;
	char part_name[CC_MAX_INI_FILE_NAME_LEN];
	char file_name[CC_MAX_INI_FILE_NAME_LEN];

	memset((void *)part_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	memset((void *)file_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	if (split_file_path(file_path, part_name, file_name, NULL) < 0)
		return -1;

	if (set_block_device(part_name) < 0)
		return -1;

	if (fs_size(file_name, &file_size)) {
		printf("%s, file \"%s\" is not exist!\n", __func__, file_name);
		return -1;
	}
	if (file_size == 0)
		printf("%s, file \"%s\" size error!\n", __func__, file_name);

	return file_size;
}

int powerkey_read_file_to_buffer(const char *file_path,
	int offset, int rd_size, unsigned char data_buf[])
{
	int tmp_ret = -1;
	loff_t rd_cnt = 0;
	char part_name[CC_MAX_INI_FILE_NAME_LEN];
	char file_name[CC_MAX_INI_FILE_NAME_LEN];

	// Add input parameter check
	if (!file_path || !data_buf) {
		printf("%s: NULL pointer argument\n", __func__);
		return -1;
	}

	memset((void *)part_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	memset((void *)file_name, 0, CC_MAX_INI_FILE_NAME_LEN);
	if (split_file_path(file_path, part_name, file_name, NULL) < 0)
		return -1;

	if (set_block_device(part_name) < 0)
		return -1;

	tmp_ret = fs_read(file_name, (unsigned long)data_buf, 0, rd_size, &rd_cnt);
	if (tmp_ret < 0)
		return -1;

	flush_dcache_range((unsigned long)data_buf, (unsigned long)data_buf + rd_cnt);

	return rd_cnt;
}

int parse_tab_file(const char *file_path, unsigned char type)
{
	int tmp_ret = -1, rd_cnt = 0, file_size = 0;
	unsigned char *tmp_buf = NULL;
	char temp_file_path[120] = {0};

	// Add input parameter check
	if (!file_path) {
		printf("%s: NULL file_path\n", __func__);
		return -1;
	}

	memset(temp_file_path, 0, sizeof(temp_file_path));
	// Fix strlcpy size parameter, use actual array size instead of calculated value
	if (!strncmp("/mnt/vendor", file_path, 11))
		strlcpy(temp_file_path, file_path + 11, sizeof(temp_file_path));
	else
		strlcpy(temp_file_path, file_path, sizeof(temp_file_path));

	const char *dest_file_path = temp_file_path;

	if (!powerkey_is_file_exist(dest_file_path)) {
		printf("%s, file \"%s\" is not exist!\n", __func__, dest_file_path);
		return -1;
	}

	file_size = powerkey_get_file_size(dest_file_path);
	if (file_size <= 0) {
		printf("%s, file \"%s\" size error!\n", __func__, dest_file_path);
		return -1;
	}

	tmp_buf = (unsigned char *)malloc(file_size * 2);
	if (tmp_buf) {
		rd_cnt = powerkey_read_file_to_buffer(dest_file_path, 0, file_size, tmp_buf);
		//printf("%s, dest_file_path= %s\n", __func__, dest_file_path);
		if (rd_cnt == file_size) {
			if (type == PARSE_TAB_TYPE_REMOTE)
				tmp_ret = tab_mem_parse(tmp_buf, file_size);
			else if (type == PARSE_TAB_TYPE_KEYPAD)
				tmp_ret = keymap_mem_parse(tmp_buf, file_size);
		}

		free(tmp_buf);
		tmp_buf = NULL;
	}

	return tmp_ret;
}

int tab_parse_mem(const char *buf, int file_size)
{
	char *bufptr = (char *)buf;
	char *line;
	char *start;
	unsigned char parse_flag = CONFIG_LEVEL;
	bool is_mr = false;
	char *str_p = NULL;
	int ir_cnt = 1;
	int lineno = 0;

	// Add input parameter check
	if (!buf) {
		printf("%s: NULL buffer\n", __func__);
		return -2;
	}

	line = (char *)malloc(INI_MAX_LINE);
	if (!line)
		return -2;

	str_p = strstr(bufptr, "rc_begin");
	if (str_p) {
		if (strstr(str_p + 15, "rc_begin")) {
			printf("is multi ir protocol...\n");
			is_mr = true;
		}
	}

	while (lineno <= file_size) {
		int ncount = 0;

		while (*bufptr != '\0' && ncount < (INI_MAX_LINE - 2) && lineno < file_size) {
			if (*bufptr == '\r' || *bufptr == '\n')
				break;

			line[ncount] = *bufptr++;
			ncount++;
			lineno++;
		}
		while ((*bufptr == '\r' || *bufptr == '\n') && lineno < file_size) {
			bufptr++;
			lineno++;
		}
		line[ncount] = 0;

		if (ncount == 0)
			break;

		start = line;
		start = lskip(rstrip(start));

		if (*start == ';' || *start == '#')
			continue;

		switch (parse_flag) {
		case CONFIG_LEVEL:
			if (strstr(start, "rc_begin")) {
				parse_flag = KEYMAP_LEVEL;
				continue;
			}
			if (strcmp(start, "wakeup_begin") == 0) {
				parse_flag = WAKEUP_LEVEL;
				continue;
			}

			break;
		case KEYMAP_LEVEL:
			if (strstr(start, "rc_end")) {
				ir_cnt++;
				if (!is_mr)
					goto exit;
				else if (ir_cnt > 2 && is_mr)
					goto exit;
				parse_flag = CONFIG_LEVEL;
				break;
			}

			parse_key_map(start);
			break;
		case WAKEUP_LEVEL:
			if (strcmp(start, "wakeup_end") == 0) {
				parse_flag = CONFIG_LEVEL;
				break;//goto exit;
			}
			//printf("WAKEUP_LEVEL................, start = %s\n", start);

			if (power_key_num < MAX_KEY_SUPPORT) {
				power_key[power_key_num] = htoi(start);
				printf("power_key[%d] = %x\n", power_key_num,
					power_key[power_key_num]);
				power_key_num++;
			} else {
				printf("Warning: power_key is full, skip: %s\n", start);
			}
			break;
		}
	}
exit:
	if (line)
		free(line);
	return 0;
}

int parse_key_map(const char *file_path)
{
	int tmp_ret = -1, rd_cnt = 0, file_size = 0;
	unsigned char *tmp_buf = NULL;
	char temp_file_path[120] = {0};

	// Add input parameter check
	if (!file_path) {
		printf("%s: NULL file_path\n", __func__);
		return -1;
	}
	// Fix strlcpy size parameter, use actual array size instead of calculated value
	if (!strncmp("/mnt/vendor", file_path, 11))
		strlcpy(temp_file_path, file_path + 11, sizeof(temp_file_path));
	else
		strlcpy(temp_file_path, file_path, sizeof(temp_file_path));

	const char *dest_file_path = temp_file_path;

	if (!powerkey_is_file_exist(dest_file_path)) {
		printf("%s, file \"%s\" is not exist!\n", __func__, dest_file_path);
		return -1;
	}

	file_size = powerkey_get_file_size(dest_file_path);
	if (file_size <= 0) {
		printf("%s, file \"%s\" size error!\n", __func__, dest_file_path);
		return -1;
	}

	tmp_buf = (unsigned char *)malloc(file_size * 2);

	if (tmp_buf) {
		//printf("%s, dest_file_path= %s\n", __func__, dest_file_path);
		rd_cnt = powerkey_read_file_to_buffer(dest_file_path, 0, file_size, tmp_buf);
		if (rd_cnt == file_size)
			tmp_ret = tab_parse_mem(tmp_buf, file_size);

		free(tmp_buf);
		tmp_buf = NULL;
	}

	return tmp_ret;
}

int tab_mem_parse(unsigned char *file_buf, int file_size)
{
	int ret = -1;
	char *p = " ";
	char wakeup_cmd[256] = { 0 };

	//printf("%s, entering...\n", __func__);
	ret = tab_parse_mem((char *)file_buf, file_size);
	if (ret < 0) {
		printf("%s, tab_parse_mem failed.\n", __func__);
		return ret;
	}

	char **power_str = NULL;
	char power_cmd[256] = { 0 };

	memset(power_cmd, 0, sizeof(power_cmd));
	memset(wakeup_cmd, 0, sizeof(wakeup_cmd));

	int valid_key_num = (power_key_num <= MAX_KEY_SUPPORT) ? power_key_num : MAX_KEY_SUPPORT;

	if (valid_key_num > 0) {
		power_str = (char **)malloc(valid_key_num * sizeof(char *));
		if (!power_str) {
			printf("%s, malloc power_str failed\n", __func__);
			return -1;
		}

		for (int i = 0; i < valid_key_num; i++) {
			power_str[i] = (char *)malloc(64 * sizeof(char));
			if (!power_str[i]) {
				for (int j = 0; j < i; j++) {
					if (power_str[j])
						free(power_str[j]);
				}
				free(power_str);
				return -1;
			}
			snprintf(power_str[i], 64, "%s0x%x", p, power_key[i]);
		}

		const int BATCH_SIZE = 15;
		int total_batches = (valid_key_num + BATCH_SIZE - 1) / BATCH_SIZE;

		for (int batch = 0; batch < total_batches; batch++) {
			memset(power_cmd, 0, sizeof(power_cmd));
			memset(wakeup_cmd, 0, sizeof(wakeup_cmd));
			int batch_len = 0;

			int start_idx = batch * BATCH_SIZE;
			int end_idx = (batch + 1) * BATCH_SIZE;

			if (end_idx > valid_key_num)
				end_idx = valid_key_num;
			int current_batch_size = end_idx - start_idx;

			// Build power_cmd for this batch
			for (int i = start_idx; i < end_idx; i++) {
				if (batch_len + strlen(power_str[i]) < sizeof(power_cmd) - 1) {
					strcat(power_cmd, power_str[i]);
					batch_len += strlen(power_str[i]);
				} else {
					printf("%s, power_cmd buffer full for batch %d\n",
						__func__, batch);
					for (int j = 0; j < valid_key_num; j++) {
						if (power_str[j])
							free(power_str[j]);
					}
					free(power_str);
					return -1;
				}
			}

			// Generate wakeup_cmd for this batch
			snprintf(wakeup_cmd, sizeof(wakeup_cmd),
				"irkey_wakeup 0 %d 0x0%s", current_batch_size, power_cmd);

			// Execute the command for this batch
			printf("wakeup_cmd batch %d is %s\n", batch, wakeup_cmd);
			run_command(wakeup_cmd, 0);
		}

		for (int i = 0; i < valid_key_num; i++) {
			if (power_str[i])
				free(power_str[i]);
		}
		if (power_str)
			free(power_str);
	}

	return ret;
}

int keymap_mem_parse(char *file_buf, int file_size)
{
	char *bufptr = (char *)file_buf;
	char *line;
	char *start;
	char key_name[32], key_code[32], channel[32], value[32], tolerance[32];
	char keypap_str[128] = { 0 };
	int adc_power_key_num = 0;
	int lineno = 0;

	// Add input parameter check
	if (!file_buf) {
		printf("%s: NULL file_buf\n", __func__);
		return -2;
	}

	line = (char *)malloc(INI_MAX_LINE);
	if (!line)
		return -2;

	while (lineno <= file_size) {
		int ncount = 0;

		while (*bufptr != '\0' && ncount < (INI_MAX_LINE - 2) && lineno < file_size) {
			if (*bufptr == '\r' || *bufptr == '\n')
				break;

			line[ncount] = *bufptr++;
			ncount++;
			lineno++;
		}
		while ((*bufptr == '\r' || *bufptr == '\n') && lineno < file_size) {
			bufptr++;
			lineno++;
		}
		line[ncount] = 0;

		if (ncount == 0)
			break;

		start = line;
		start = lskip(rstrip(start));

		if (*start == ';' || *start == '#')
			continue;

		if ((strncmp(start, "power", 5)) == 0) {
			int j = 0;

			////parse key_name
			start = skipspace(start);
			memset(key_name, 0, 32);
			while (' ' != *start) {
				key_name[j] = *start;
				j++;
				start++;
			}
			key_name[j] = '\0';
			j = 0;
			//printf("key_name = %s\n", key_name);

			////parse key_code
			start = skipspace(start);
			memset(key_code, '\0', 32);
			while (' ' != *start && '\0' != *start) {
				key_code[j] = *start;
				j++;
				start++;
			}

			key_code[j] = '\0';
			j = 0;
			//printf("key_code = %s\n", key_code);

			////parse channel
			start = skipspace(start);
			memset(channel, '\0', 32);
			while (' ' != *start && '\0' != *start) {
				channel[j] = *start;
				j++;
				start++;
			}
			channel[j] = '\0';
			j = 0;
			//printf("channel = %s\n", channel);

			////parse adc value
			start = skipspace(start);
			memset(value, '\0', 32);
			while (' ' != *start && '\0' != *start) {
				value[j] = *start;
				j++;
				start++;
			}
			value[j] = '\0';
			j = 0;
			//printf("value = %s\n", value);

			////parse tolerance
			start = skipspace(start);
			memset(tolerance, '\0', 32);
			while (' ' != *start && '\0' != *start) {
				tolerance[j] = *start;
				j++;
				start++;
			}
			tolerance[j] = '\0';
			j = 0;
			//printf("tolerance = %s\n", tolerance);

			run_command("keypad wakeup remove", 0);
		    memset(keypap_str, 0, sizeof(keypap_str));
			// Use snprintf instead of sprintf to ensure string safety
			snprintf(keypap_str, sizeof(keypap_str), "keypad wakeup adc 520 %s %s 1",
				value, channel);
			printf("keypap_str is %s\n", keypap_str);
			run_command(keypap_str, 0);

			// Add boundary check for adc_power_key array
			if (adc_power_key_num < MAX_KEY_SUPPORT)
				adc_power_key_num++;
			else if (adc_power_key_num >= MAX_KEY_SUPPORT)
				printf("Warning: adc_power_key array is full\n");

			break;
		}
	}

	if (line)
		free(line);
	return 0;
}

int powerkey_mem_parse(const char *buf,
	int file_size, char *model,
	char *model_type, char *keymap_path)
{
	// Add input parameter check
	if (!buf || !model || !model_type || !keymap_path) {
		printf("%s: NULL pointer argument\n", __func__);
		return -1;
	}
	char *bufptr = (char *)buf;
	char *line;
	char *start;
	char model_name[128];
	unsigned char model_flag = 0;
	int lineno = 0;
	char *key, *value;

	line = (char *)malloc(INI_MAX_LINE);
	if (!line)
		return -1;

	sprintf(model_name, "[%s]", model);

	while (lineno <= file_size) {
		int ncount = 0;

		while (*bufptr != '\0' && ncount < (INI_MAX_LINE - 2) && lineno < file_size) {
			if (*bufptr == '\r' || *bufptr == '\n')
				break;

			line[ncount] = *bufptr++;
			ncount++;
			lineno++;
		}

		while ((*bufptr == '\r' || *bufptr == '\n') && lineno < file_size) {
			bufptr++;
			lineno++;
		}
		line[ncount] = 0;

		if (ncount == 0)
			break;

		/* Scan through file line by line */
		// while (fgets(line, INI_MAX_LINE, file) != NULL) {

		start = line;
		start = lskip(rstrip(start));

		StrLineTrim(line, strlen(line)); /*jump comment*/
		StrTrim(&line);

		if (model_flag) {
			if (StrEmpty(line))
				continue;
		} else {
			if ((strcasecmp(line, model_name)) == 0)
				model_flag = 1;
			continue;
		}

		if ((strncasecmp(line, model_type, strlen(model_type))) == 0)
			if (GetKeyValue(line, &key, &value) == 0) {
				strcpy(keymap_path, value);
				break;
			}
	}

	if (line)
		free(line);

	return 0;
}

int parse_model_file(const char *file_path, char *model, char *model_type, char *keymap_path)
{
	int tmp_ret = -1, rd_cnt = 0, file_size = 0;
	unsigned char *tmp_buf = NULL;

	// Add input parameter check
	if (!file_path || !model || !model_type || !keymap_path) {
		printf("%s: NULL pointer argument\n", __func__);
		return -1;
	}

	//printf("enter %s, file_path: %s\n", __func__, file_path);

	if (!powerkey_is_file_exist(file_path)) {
		printf("%s, file \"%s\" is not exist!\n", __func__, file_path);
		return -1;
	}

	file_size = powerkey_get_file_size(file_path);
	if (file_size <= 0) {
		printf("%s, file \"%s\" size error!\n", __func__, file_path);
		return -1;
	}

	tmp_buf = (unsigned char *)malloc(file_size * 2);

	if (tmp_buf) {
		rd_cnt = powerkey_read_file_to_buffer(file_path, 0, file_size, tmp_buf);
		if (rd_cnt == file_size)
			tmp_ret = powerkey_mem_parse(tmp_buf, file_size, model,
				model_type, keymap_path);

		free(tmp_buf);
		tmp_buf = NULL;
	}

	return tmp_ret;
}

int parse_multi_remote(const char *file_name, char *model_name)
{
	char remote_path[128] = {0};
	int ret = 1;

	// Add input parameter check
	if (!file_name || !model_name) {
		printf("%s: NULL pointer argument\n", __func__);
		return -1;
	}

	if (parse_model_file(file_name, model_name, "REMOTE_PATH", remote_path) < 0) {
		printf("parse mode remotefile %s error\n", file_name);
		return ret;
	}

	if (remote_path[0] != '\0') {
		printf("remote_path is %s\n", remote_path);
		parse_tab_file(remote_path, PARSE_TAB_TYPE_REMOTE);
		ret = 0;
	} else {
		printf("invalid REMOTE_PATH\n");
	}

	return ret;
}

int parse_multi_keypad(const char *file_name, char *model_name)
{
	char keypad_path[128] = {0};
	int ret = 1;

	// Add input parameter check
	if (!file_name || !model_name) {
		printf("%s: NULL pointer argument\n", __func__);
		return -1;
	}

	if (parse_model_file(file_name, model_name, "KEYPAD_PATH", keypad_path) < 0) {
		printf("parse mode remotefile %s error\n", file_name);
		return ret;
	}

	if (keypad_path[0] != '\0') {
		printf("keypad_path is %s\n", keypad_path);
		parse_tab_file(keypad_path, PARSE_TAB_TYPE_KEYPAD);
		ret = 0;
	} else {
		printf("invalid KEYPAD_PATH\n");
	}

	return ret;
}

int handle_multi_remote(void)
{
	char *model;
	int ret = 1;
	unsigned char modelname_buf[200] = {0x0};
	char model_path[128];

	power_key_num = 0;
	memset(power_key, 0, sizeof(power_key));

	if (key_unify_read("model_name", modelname_buf, sizeof(modelname_buf)) != 0) {
		printf("key_unify_read failed!!!\n");
		model = env_get("model_name");
	} else {
		model = modelname_buf;
	}

	if (model) {
		printf("model is %s\n", model);
		ret = parse_multi_remote(DEFAULT_MODEL_SUM_PATH, model);
		if (ret != 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
				DEFAULT_MODEL_PATH, model);
			printf("try model_path is %s\n", model_path);
			ret = parse_multi_remote(model_path, DEFAULT_SECTION);
		}
	}

	return ret;
}

int handle_multi_keypad(void)
{
	char *model;
	int ret = 1;
	unsigned char modelname_buf[200] = {0x0};
	char model_path[128];

	power_key_num = 0;
	memset(power_key, 0, sizeof(power_key));

	if (key_unify_read("model_name", modelname_buf, sizeof(modelname_buf)) != 0) {
		printf("key_unify_read failed!!!\n");
		model = env_get("model_name");
	} else {
		model = modelname_buf;
	}

	if (model) {
		printf("model is %s\n", model);
		ret = parse_multi_keypad(DEFAULT_MODEL_SUM_PATH, model);
		if (ret != 0) {
			snprintf(model_path, 127, "%s/Project_index_%s.ini",
				DEFAULT_MODEL_PATH, model);
			printf("try model_path is %s\n", model_path);
			ret = parse_multi_keypad(model_path, DEFAULT_SECTION);
		}
	}

	return ret;
}
