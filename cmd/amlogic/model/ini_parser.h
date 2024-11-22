/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "model_log.h"

#define INI_PARSER_MEM_SIZE_DFT         102400 //200k
#define INI_LINE_LEN_MAX                512
#define INI_SECTION_NAME_LEN_MAX        48
#define INI_KEY_NAME_LEN_MAX            64

#define INI_MULTI_LINE_LEN_MAX          0x80000

#define INI_SET_KEY_VAL_MODE_APPEND     0
#define INI_SET_KEY_VAL_MODE_OVERWRITE  1
#define INI_SET_KEY_VAL_MODE            INI_SET_KEY_VAL_MODE_APPEND

struct ini_multi_line_s {
	char *sec_name;
	char *key_name;
	int valid;
	int max_len;
	int cur_pos;
	char *buf;
};

struct ini_line_s {
	int local_size;
	int next;      //global offset
	int name_pos;  //global offset
	int value_pos; //global offset
};

struct ini_section_s {
	int local_size; //secton own size, not include lines
	int first_line; //struct ini_line_s *first_line;
	int cur_line;   //struct ini_line_s *cur_line;
	int next;       //global offset
	int name_pos;   //global offset
};

#define INI_MEM_DATA_OFFSET             16
#define INI_IDENTIFIER_STR              "ini parse mem"

struct ini_s {
	unsigned int crc32;
	int total_size;
	int mem_size;
	int first_section; //global offset
	int cur_section;   //global offset
	int mem_start_pos; //offset by total buffer start
	int mem_cur_pos;   //offset by mem start
	unsigned char *mem;  //include magic_num
};

#define INI_PARSER_MEM_TRACE        0
#define INI_MEM_ALL        0
#define INI_MEM_SECTION    1
#define INI_MEM_KEY        2

int ini_parse_mem(struct ini_s *ini_buf, const char *tmp_buf);
struct ini_section_s *ini_get_section(struct ini_s *ini_buf, const char *section);
struct ini_line_s *ini_get_key_line_at_sec(struct ini_s *ini_buf,
			struct ini_section_s *psec, const char *key);
int ini_set_line_exist_key_val(struct ini_s *ini_buf, struct ini_section_s *psec,
			struct ini_line_s *pline, const char *new_value, unsigned int set_mode);
int ini_set_line_exist_keys(struct ini_s *ini_buf, struct ini_section_s *psec,
			const char *new_line, const char *new_value, int line_cnt);
const char *ini_get_string(struct ini_s *ini_buf, struct ini_section_s *psec,
			const char *key, const char *def_value);
void ini_list_key_value(struct ini_s *ini_buf);
void ini_list_section(struct ini_s *ini_buf);
void ini_handler_mem_free(struct ini_s *ini_buf);

