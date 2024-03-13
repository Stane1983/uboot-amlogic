/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INI_PROXY_H__
#define __INI_PROXY_H__

#ifdef __cplusplus
extern "C" {
#endif

void bin_file_init(void);
void bin_file_uninit(void);
int read_bin_file(const char *filename);
int get_bin_data(unsigned char *file_buf, unsigned int file_size);

void ini_parser_init(void);
void ini_parser_uninit(void);
int ini_parse_file(const char *filename);
int ini_parse_mem(unsigned char *file_buf);
int ini_set_save_file_name(const char *filename);
void ini_parser_free(void);
void ini_print_all(void);
void ini_list_section(void);
const char *ini_get_string(const char *section, const char *key, const char *def_value);
int ini_set_string(const char *section, const char *key, const char *value);
int ini_save_to_file(const char *filename);

#ifdef __cplusplus
}
#endif

#endif //__INI_PROXY_H__
