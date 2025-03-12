// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/aml_model.h>
#include "ini_parser.h"
#include "model_log.h"
#include "model.h"

#define LOG_TAG     "ini_handle"
#define LOG_NDEBUG  0

int handle_ini_mem_check(void *inip)
{
	struct ini_s *ini_buf;
	char *identifier;
	unsigned char *p;
	unsigned int crc32;
	unsigned char *local_ini_mem = inip;

	//check identifier
	p = (unsigned char *)inip;
	identifier = (char *)(p + sizeof(struct ini_s));
	if (strcmp(identifier, INI_IDENTIFIER_STR)) {
		//not ini mem
		return -1;
	}

	ini_buf = (struct ini_s *)inip;
	if (ini_buf->mem_start_pos != sizeof(struct ini_s)) {
		//mem start position not correct
		return -1;
	}

	crc32 = cal_CRC32(0, local_ini_mem +ini_buf->mem_start_pos, ini_buf->mem_cur_pos);
	if (ini_buf->crc32 != crc32) {
		//not sure ini mem
		return -1;
	}

	return 0;
}

//maybe other file type mem
void handle_ini_parser_uninit(void *inip)
{
	struct ini_s *ini_buf;
	int total_size;
	int ret;

	if (!inip)
		return;

	ret = handle_ini_mem_check(inip);
	if (ret) {
		//not sure mem_size, only free without clear
		free(inip);
		return;
	}

	ini_buf = (struct ini_s *)inip;
	ini_handler_mem_free(ini_buf);

	total_size = ini_buf->total_size;
	memset(inip, 0, total_size);
	free(inip);
}

static unsigned char *handle_ini_parser_init(int mem_size)
{
	unsigned char *local_ini_mem;
	struct ini_s *ini_buf;
	char *identifier;
	int total_size;

	if (mem_size)
		total_size = mem_size;
	else
		total_size = INI_PARSER_MEM_SIZE_DFT;

	local_ini_mem = malloc(total_size);
	if (!local_ini_mem) {
		ALOGE("%s: error\n", __func__);
		return NULL;
	}
	memset(local_ini_mem, 0, total_size);

	ini_buf = (struct ini_s *)local_ini_mem;
	ini_buf->total_size = total_size;
	ini_buf->mem_size = total_size - sizeof(struct ini_s);
	ini_buf->mem_start_pos = sizeof(struct ini_s);

	//init identifier:
	identifier = local_ini_mem + ini_buf->mem_start_pos;
	strlcpy(identifier, INI_IDENTIFIER_STR, INI_MEM_DATA_OFFSET);

	//init parser data offset
	ini_buf->mem_cur_pos = INI_MEM_DATA_OFFSET;

	return local_ini_mem;
}

void *handle_ini_file_parse(const char *filename, int mem_size)
{
	unsigned char *local_ini_mem = NULL;
	struct ini_s *ini_buf;
	unsigned char *tmp_buf = NULL;
	int file_size = 0;
	int ret = 0;

	ALOGD("%s: %s, mem_size:%d\n", __func__, filename, mem_size);

	tmp_buf = model_read_file_to_buffer(filename, &file_size);
	if (!tmp_buf) {
		ret = -1;
		goto handle_ini_file_parse_end;
	}

	local_ini_mem = handle_ini_parser_init(mem_size);
	if (!local_ini_mem)
		goto handle_ini_file_parse_end;

	ini_buf = (struct ini_s *)local_ini_mem;
	ret = ini_parse_mem(ini_buf, (const char *)tmp_buf);
	if (ret)
		handle_ini_parser_uninit(local_ini_mem);
	else
		ini_buf->crc32 = cal_CRC32(0, local_ini_mem + ini_buf->mem_start_pos, ini_buf->mem_cur_pos);

handle_ini_file_parse_end:
	memset((void *)tmp_buf, 0, file_size);
	free(tmp_buf);

	if (ret)
		return NULL;
	return (void *)local_ini_mem;
}

int handle_ini_parser_mem_get_size(void *inip)
{
	struct ini_s *ini_buf;
	int size, ret;

	if (!inip) {
		ALOGE("%s: error\n", __func__);
		return -1;
	}

	ret = handle_ini_mem_check(inip);
	if (ret)
		return -1;

	ini_buf = (struct ini_s *)inip;

	size = ini_buf->mem_start_pos + ini_buf->mem_cur_pos; //real mem size

	return size;
}

void *handle_ini_parser_dupmem(void *inip, int *buf_size)
{
	void *local_ini_mem;
	struct ini_s *ini_buf;
	unsigned char *p;
	int size;

	if (!inip) {
		ALOGE("%s: error\n", __func__);
		return NULL;
	}
	ini_buf = (struct ini_s *)inip;

	size = ini_buf->mem_start_pos + ini_buf->mem_cur_pos; //real mem size
	local_ini_mem = (void *)malloc(size);
	if (!local_ini_mem) {
		ALOGE("%s: error\n", __func__);
		return NULL;
	}
	memcpy(local_ini_mem, inip, size);

	//update dest_buf value
	ini_buf = (struct ini_s *)local_ini_mem;
	ini_buf->total_size = size;
	ini_buf->mem_size = ini_buf->mem_cur_pos;
	p = (unsigned char *)local_ini_mem;
	ini_buf->crc32 = cal_CRC32(0, local_ini_mem + ini_buf->mem_start_pos, ini_buf->mem_cur_pos);
	*buf_size = size;

	return local_ini_mem;
}

void *handle_ini_get_section(void *inip, const char *section)
{
	struct ini_s *ini_buf;
	struct ini_section_s *psec;

	if (!inip || !section)
		return NULL;
	ini_buf = (struct ini_s *)inip;

	psec = ini_get_section(ini_buf, section);
	return (void *)psec;
}

const char *handle_ini_get_str(void *inip, void *psec, const char *key, const char *def_val)
{
	struct ini_s *ini_buf;
	struct ini_section_s *ini_section;
	const char *value;

	if (!inip || !psec || !key)
		return def_val;

	ini_buf = (struct ini_s *)inip;
	ini_section = (struct ini_section_s *)psec;
	value = ini_get_string(ini_buf, ini_section, key, def_val);
	return value;
}

unsigned int handle_ini_get_val(void *inip, void *psec, const char *key, unsigned int def_val)
{
	struct ini_s *ini_buf;
	struct ini_section_s *ini_section;
	const char *str;

	if (!inip || !psec || !key)
		return def_val;
	ini_buf = (struct ini_s *)inip;

	ini_section = (struct ini_section_s *)psec;
	str = ini_get_string(ini_buf, ini_section, key, NULL);
	if (!str)
		return def_val;
	return (unsigned int)(simple_strtoul(str, NULL, 0));
}

int handle_ini_key_exist(void *inip, void *psec, const char *key)
{
	struct ini_s *ini_buf;
	struct ini_section_s *ini_section;
	const char *str;

	if (!inip || !psec || !key)
		return 0;
	ini_buf = (struct ini_s *)inip;

	ini_section = (struct ini_section_s *)psec;
	str = ini_get_string(ini_buf, ini_section, key, NULL);
	if (!str)
		return 0;
	return 1;
}

int handle_ini_set_exist_single_key(void *inip, void *psec, const char *key, const char *str)
{
	struct ini_s *ini_buf;
	struct ini_section_s *ini_section;
	struct ini_line_s *ini_line;
	int ret;

	if (!inip || !psec || !key)
		return -1;
	ini_buf = (struct ini_s *)inip;

	ini_section = (struct ini_section_s *)psec;
	ini_line = ini_get_key_line_at_sec(ini_buf, ini_section, key);

	ret = ini_set_line_exist_key_val(ini_buf, ini_section, ini_line, str,
					 INI_SET_KEY_VAL_MODE_OVERWRITE);
	return ret;
}

int handle_ini_set_exist_keys(void *inip, void *psec,
			      const char *new_line, const char *new_value, int line_cnt)
{
	struct ini_s *ini_buf;
	struct ini_section_s *ini_section;
	int ret;

	if (!inip || !psec || !new_line || !new_value)
		return -1;
	ini_buf = (struct ini_s *)inip;

	ini_section = (struct ini_section_s *)psec;

	ret = ini_set_line_exist_keys(ini_buf, ini_section, new_line, new_value, line_cnt);
	return ret;
}

void handle_ini_list_key_value(void *inip)
{
	struct ini_s *ini_buf;

	if (!inip)
		return;
	ini_buf = (struct ini_s *)inip;

	ini_list_key_value(ini_buf);
}

void handle_ini_list_section(void *inip)
{
	struct ini_s *ini_buf;

	if (!inip)
		return;
	ini_buf = (struct ini_s *)inip;

	ini_list_section(ini_buf);
}

