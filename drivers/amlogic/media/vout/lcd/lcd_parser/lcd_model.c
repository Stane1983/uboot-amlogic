// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/aml_model.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_memory.h>
#include "../lcd_common.h"

struct panel_parse_mem_s {
	unsigned char type;
	int size;
	char file_path[256];
	void *mem;
};

static struct panel_parse_mem_s glcd_file_parse_mem[LCD_MAX_DRV] = {
	{PANEL_FILE_INVILD, 0, {0}, NULL},
	{PANEL_FILE_INVILD, 0, {0}, NULL},
	{PANEL_FILE_INVILD, 0, {0}, NULL}
};

#define PANEL_PARAM_MEM_RSVD_SIZE    (0x80000)
#define PANEL_PARAM_KEY_NUM_MAX      (64 - 1)
#define PANEL_PARAM_KEY_SIZE         (64)
#define PANEL_PARAM_HEAD_SIZE        PANEL_PARAM_KEY_SIZE
#define PANEL_PARAM_KEY_NAME_SIZE    (PANEL_PARAM_KEY_SIZE - 8)

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

static char *panel_config_load_strs[] = {
	[LCD_CONFIG_NONE] = "none",
	[LCD_CONFIG_DTS] = "dts",
	[LCD_CONFIG_UKEY] = "ukey",
	[LCD_CONFIG_FILE] = "file"
};

const char *get_lcd_config_load(unsigned char type)
{
	if (type > LCD_CONFIG_FILE)
		return panel_config_load_strs[LCD_CONFIG_NONE];

	return panel_config_load_strs[type];
}

int lcd_get_str_array_cnt(const char *data_str)
{
	const char *p;
	int cnt = 0;

	for (p = data_str; *p != '\0'; p++) {
		if (*p == ',')
			cnt++;
	}
	if (*(p - 1) != ',')
		cnt++;

	return cnt;
}

int lcd_trans_str_array(const char *data_str, unsigned int *data_buf, int cnt_max)
{
	int str_len, i = 0;
	char *token = NULL, *end;
	char *tmp_buf = NULL;

	if (!data_str || !data_buf)
		return -1;

	str_len = strlen(data_str) + 1;
	tmp_buf = (char *)malloc(str_len);
	if (!tmp_buf) {
		LCDERR("%s: malloc buffer (size %d) error!\n", __func__, str_len);
		return -1;
	}

	memset((void *)tmp_buf, 0, str_len);
	strlcpy(tmp_buf, data_str, str_len);
	token = tmp_buf;
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: cnt_max=%d, in_buf=%s\n", __func__, cnt_max, tmp_buf);
	while (*token != '\0') {
		if (i >= cnt_max)
			break;
		end = strchr(token, ',');
		if (end)
			*end = '\0';
		data_buf[i] = strtoul(token, NULL, 0);
		i++;
		if (!end)
			break;
		token = end + 1;
	}

	memset((void *)tmp_buf, 0, str_len);
	free(tmp_buf);

	return i;
}

unsigned int lcd_get_str_array_index(const char *data_str, unsigned int index, unsigned int def_val)
{
	int str_len, i = 0;
	char *token = NULL, *end;
	char *tmp_buf = NULL;
	unsigned int val = def_val;

	if (!data_str)
		return def_val;

	str_len = strlen(data_str) + 1;
	tmp_buf = (char *)malloc(str_len);
	if (!tmp_buf) {
		LCDERR("%s: malloc buffer (size %d) error!\n", __func__, str_len);
		return def_val;
	}

	memset((void *)tmp_buf, 0, str_len);
	strlcpy(tmp_buf, data_str, str_len);
	token = tmp_buf;
	while (*token != '\0') {
		end = strchr(token, ',');
		if (end)
			*end = '\0';
		if (i == index) {
			val = strtoul(token, NULL, 0);
			break;
		}
		i++;
		if (!end)
			break;
		token = end + 1;
	}

	memset((void *)tmp_buf, 0, str_len);
	free(tmp_buf);

	return val;
}

void mem_dump(unsigned char *addr, int size)
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
	strncpy(key->name, name, sizeof(key->name));
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

static void panel_param_mem_update(void)
{
	unsigned int size = 0, key_size = 0, key_mem_size = 0;
	phys_addr_t paddr;
	unsigned char *panel_rsvd;
	struct panel_param_head_s {
		unsigned int _crc32, size;
		unsigned short key_cnt, ukey_exist;
		unsigned char rsvd[PANEL_PARAM_HEAD_SIZE - 12];
	} *head;

	if (!panel_param_mem.mem || !panel_param_mem.head->key_cnt)
		return;

	head = (struct panel_param_head_s *)panel_param_mem.mem;
	if (!head->key_cnt) {
		LCDPR("error panel_param header\n");
		return;
	}
	key_size = head->key_cnt * PANEL_PARAM_KEY_SIZE + PANEL_PARAM_HEAD_SIZE;
	key_mem_size = head->size - PANEL_PARAM_KEY_MEM_OFST;
	size  = key_size + key_mem_size;

	panel_rsvd = (unsigned char *)lrm_alloc_tail(size, &paddr, "panel_config");
	if (!panel_rsvd) {
		LCDPR("no rsvd mem to save panel_config\n");
		return;
	}
	memcpy(panel_rsvd, panel_param_mem.mem, key_size);
	memcpy(panel_rsvd + key_size, panel_param_mem.mem + PANEL_PARAM_KEY_MEM_OFST, key_mem_size);

	head = (struct panel_param_head_s *)panel_rsvd;
	head->size = size;
	head->_crc32 = cal_CRC32(0, panel_rsvd + 4, size - 4);

	LCDPR("%s, paddr:0x%llx crc:0x%x, size:%d, key_cnt:%d, ukey_exist:%d\n", __func__,
		(u64)paddr, head->_crc32, head->size, head->key_cnt, head->ukey_exist);
}

void update_panel_param_to_kernel(void)
{
	u64 pa;
	u32 size;

	if (lrm_get_by_name("panel_config", &pa, &size))
		return;
	lrm_phys_free(pa);
	panel_param_mem_update();
}

int path_name_compose(const char *path, const char *name, char *path_name)
{
	char *p1;
	const char *p2;
	int len1, len2, len, back = 0, k;

	if (!path || !name || !path_name)
		return -1;

	p2 = name;
	len2 = strlen(name);
	if (name[0] == '/') {//absolute path, ignore path
		strcpy(path_name, name);
		path_name[len2 + 1] = '\0';
		return 0;
	} else if (name[0] == '.' && name[1] == '/') {
		back = 0;
		p2 += 2;
	} else if (p2[0] == '.' && p2[1] == '.' && p2[2] == '/') {
		while (len2 > 0 && p2[0] == '.' && p2[1] == '.' && p2[2] == '/') {
			p2 += 3;
			len2 -= 3;
			back++;
		}
	}

	if (len2 <= 0) {
		path_name[0] = '\0';
		return -1;
	}

	p1 = path_name;
	len1 = strlen(path);
	len = len1;
	memcpy(path_name, path, len);
	path_name[len] = '\0';
	if (path_name[len - 1] != '/') {
		path_name[len] = '/';
		len += 1;
		path_name[len] = '\0';
	}
	back += 1;

	for (k = len - 1; k > 0; k--) {
		if (p1[k] == '/')
			back--;
		if (back == 0) {
			memcpy(p1 + k + 1, p2, len2);
			len = k + len2 + 1;
			p1[len] = '\0';
			return 0;
		}
	}
	return -1;
}

unsigned char get_lcd_panel_file_type(int index)
{
	return index < LCD_MAX_DRV ? glcd_file_parse_mem[index].type : PANEL_FILE_INVILD;
}

void set_lcd_panel_file_type(int index, unsigned char type)
{
	if (index >= LCD_MAX_DRV) {
		LCDERR("%s: invalid index %d\n", __func__, index);
		return;
	}

	glcd_file_parse_mem[index].type = type;
}

int set_panel_file_path(int index, const char *str)
{
	if (!str)
		return -1;
	if (index >= LCD_MAX_DRV)
		return -1;

	strlcpy(glcd_file_parse_mem[index].file_path, str, 256);
	return 0;
}

const char *get_panel_file_path(int index)
{
	if (index >= LCD_MAX_DRV)
		return NULL;

	return glcd_file_parse_mem[index].file_path;
}

void rm_panel_file_path(int index)
{
	if (index >= LCD_MAX_DRV)
		return;

	*glcd_file_parse_mem[index].file_path = '\0';
}

int set_panel_file_parse_mem(int index, void *parse_mem, int size, unsigned char type)
{
	if (index >= LCD_MAX_DRV)
		return -1;
	if (glcd_file_parse_mem[index].type == PANEL_FILE_INVILD) {
		glcd_file_parse_mem[index].type = type;
	} else {
		if (type != glcd_file_parse_mem[index].type) {
			LCDERR("%s: panel%d file type not match\n", __func__, index);
			return -1;
		}
	}

	glcd_file_parse_mem[index].size = size;
	glcd_file_parse_mem[index].mem = parse_mem;
	return 0;
}

void *get_panel_file_parse_mem(int index)
{
	if (index >= LCD_MAX_DRV)
		return NULL;

	return glcd_file_parse_mem[index].mem;
}

void rm_panel_file_parse_mem(int index)
{
	if (index >= LCD_MAX_DRV)
		return;

	if (!glcd_file_parse_mem[index].mem)
		return;

	switch (glcd_file_parse_mem[index].type) {
#ifdef CONFIG_CMD_AML_MODEL
	case PANEL_FILE_JSON:
		panel_json_mem_free(glcd_file_parse_mem[index].mem);
		break;
#endif
#ifdef CONFIG_CMD_AML_MODEL
	case PANEL_FILE_INI:
		lcd_ini_mem_free(glcd_file_parse_mem[index].mem);
		break;
#endif
	default:
		memset(glcd_file_parse_mem[index].mem, 0, glcd_file_parse_mem[index].size);
		free(glcd_file_parse_mem[index].mem);
		break;
	}
	glcd_file_parse_mem[index].mem = NULL;
	glcd_file_parse_mem[index].size = 0;
	glcd_file_parse_mem[index].type = PANEL_FILE_INVILD;
}

int panel_file_parse_mem_save(void)
{
	void *parse_mem;
	int i, ret = -1;

	for (i = 0; i < LCD_MAX_DRV; i++) {
		parse_mem = glcd_file_parse_mem[i].mem;
		if (!parse_mem)
			continue;

		switch (glcd_file_parse_mem[i].type) {
#ifdef CONFIG_AML_LCD_JSON
		case PANEL_FILE_JSON:
			ret = panel_json_mem_save(parse_mem, i);
			break;
#endif
#ifdef CONFIG_CMD_AML_MODEL
		case PANEL_FILE_INI:
			ret = lcd_ini_param_mem_save(parse_mem, i);
			break;
#endif
		default:
			break;
		}
		if (ret)
			LCDERR("%s: parse_mem[%d] exit\n", __func__, i);
	}

	panel_param_mem_update();

	return 0;
}
