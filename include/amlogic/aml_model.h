/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_MODEL_H
#define _AML_MODEL_H

struct panel_misc_s {
	char version;
	char disp_idx;
	char outputmode[64];
	char connector_type[64];
	char panel_reverse[16];
	char display_layer[16];
	unsigned int hmirror_val;
	unsigned int vmirror_val;
	unsigned int disp_layer_val;
};

#ifndef u64
#define u64 unsigned long long
#endif

#ifndef s64
#define s64 signed long long
#endif

#ifndef u32
#define u32 unsigned int
#endif

#ifndef s32
#define s32 signed int
#endif

#ifndef f32
#define f32 float
#endif

#ifndef f64
#define f64 double
#endif

#define PANEL_FILE_INVILD  0
#define PANEL_FILE_INI     1
#define PANEL_FILE_JSON    2

static inline unsigned int cal_CRC32(unsigned int crc, const unsigned char *ptr, int buf_len)
{
	static const unsigned int s_crc32[16] = {
		0, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};

	unsigned int crcu32 = crc;
	unsigned char b;

	if (buf_len <= 0)
		return 0;

	if (!ptr)
		return 0;

	crcu32 = ~crcu32;
	while (buf_len--) {
		b = *ptr++;
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
	}

	return ~crcu32;
}

#if defined(CONFIG_CMD_INI) || defined(CONFIG_CMD_AML_MODEL)
int model_set_panel_misc(struct panel_misc_s *misc_attr);

int model_file_is_exist(const char *file_path);
int model_get_file_size(const char *file_path);
int model_read_file(const char *file_path, unsigned char *data_buf, int buf_size);
unsigned char *model_read_file_to_buffer(const char *file_path, int *buf_size);

#else
static inline int model_set_panel_misc(struct panel_misc_s *misc_attr)
{
	return -1;
}

static inline int model_file_is_exist(const char *file_path)
{
	return 0;
}

static inline int model_get_file_size(const char *file_path)
{
	return -1;
}

static inline int model_read_file(const char *file_path, unsigned char *data_buf, int buf_size)
{
	return -1;
}

static inline unsigned char *model_read_file_to_buffer(const char *file_path, int *buf_size)
{
	return NULL;
}

#endif

#if defined(CONFIG_CMD_INI)
int handle_model_list_panel_key(int index);
int handle_model_list(void);
int handle_model_sum(void);
int handle_model_logofile(void);
int handle_model_oemkey(void);
#endif

#if defined(CONFIG_CMD_AML_MODEL)
int handle_ini_mem_check(void *inip);
void handle_ini_parser_uninit(void *inip);
void *handle_ini_file_parse(const char *filename, int mem_size);
int handle_ini_parser_mem_get_size(void *inip);
void *handle_ini_parser_dupmem(void *inip, int *buf_size);
void *handle_ini_get_section(void *inip, const char *section);
const char *handle_ini_get_str(void *inip, void *psec, const char *key, const char *def_val);
unsigned int handle_ini_get_val(void *inip, void *psec, const char *key, unsigned int def_val);
int handle_ini_key_exist(void *inip, void *psec, const char *key);
int handle_ini_set_exist_single_key(void *inip, void *psec, const char *key, const char *str);
int handle_ini_set_exist_keys(void *inip, void *psec,
			      const char *new_line, const char *new_value, int line_cnt);
void handle_ini_list_key_value(void *inip);
void handle_ini_list_section(void *inip);
#endif

#ifdef CONFIG_AML_LCD
void panel_param_mem_set_ukey_flag(void);
int panel_param_mem_put(unsigned char *mem, const char *name, u32 len);
unsigned char *panel_param_mem_get(const char *name, u32 *len);
int panel_param_mem_modify(unsigned char *mem, const char *name, u32 len);

int set_panel_file_path(int index, const char *str);
const char *get_panel_file_path(int index);
void rm_panel_file_path(int index);
void set_lcd_panel_file_type(int index, unsigned char type);
unsigned char get_lcd_panel_file_type(int index);

void aml_lcd_panel_dump(int index, const char *path);

#ifdef CONFIG_AML_LCD_TCON
unsigned char *get_tcon_bin_path_mem(void);
unsigned char *get_tcon_bin_path_k_mem(void);
#endif
#endif

#endif /*_AML_MODEL_H*/

