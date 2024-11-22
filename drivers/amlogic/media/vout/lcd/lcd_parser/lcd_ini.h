/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LCD_INI__H
#define __LCD_INI__H

void *lcd_ini_get_section(void *inip, const char *section);
const char *lcd_ini_get_str(void *inip, void *psec, const char *key, const char *def_val);
unsigned int lcd_ini_get_val(void *inip, void *psec, const char *key, unsigned int def_val);
int lcd_ini_get_array_cnt(void *inip, void *psec, const char *key);
int lcd_ini_get_array(void *inip, void *psec, const char *key, unsigned int *buf, unsigned int cnt);
unsigned int lcd_ini_get_array_index(void *inip, void *psec, const char *key,
				     unsigned int index, unsigned int def_val);
int lcd_ini_set_exist_single_key(void *inip, void *psec, const char *key, const char *str);
int lcd_ini_set_exist_keys(void *inip, void *psec,
			   const char *new_line, const char *new_value, int line_cnt);
void *lcd_ini_file_parse(int index);
void *get_lcd_ini_parse_mem(int index);
void lcd_ini_mem_free(void *parse_mem);
int lcd_ini_param_mem_save(void *parse_mem, int index);
int lcd_ini_param_mem_save_update(void *parse_mem, int index);
void lcd_ini_list_key_value(int index);

#endif //__LCD_INI__H
