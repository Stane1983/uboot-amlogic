/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_MODEL_H
#define _AML_MODEL_H

#ifndef u32
#define u32 unsigned int
#endif

#define PANEL_FILE_INVILD  0
#define PANEL_FILE_INI     1
#define PANEL_FILE_JSON    2

#ifdef CONFIG_CMD_INI
int is_panel_param_mem_ok(void);
int is_ukey_in_param_mem(void);
void panel_param_mem_dump(void);
unsigned char *get_panel_param_mem(void);
int panel_param_mem_put(unsigned char *mem, const char *name, u32 len);
unsigned char *panel_param_mem_get(const char *name, u32 *len);
int panel_param_mem_modify(unsigned char *mem, const char *name, u32 len);
unsigned char *get_panel_file(int index, int *len);
void rm_panel_file(int index);
unsigned char *read_file_to_buffer(const char *filename, int *size);
unsigned char get_lcd_panel_file_type(int index);

#else
static inline int is_panel_param_mem_ok(void)
{
	return 0;
}

static inline int is_ukey_in_param_mem(void)
{
	return 0;
}

static inline void panel_param_mem_dump(void)
{
}

static inline unsigned char *get_panel_param_mem(void)
{
	return NULL;
}

static inline unsigned char *panel_param_mem_get(const char *name, u32 *len)
{
	return NULL;
}

static inline int panel_param_mem_modify(unsigned char *mem, const char *name, u32 len)
{
	return -1;
}

static inline unsigned char *get_panel_file(int index, int *len)
{
	return NULL;
}

static inline void rm_panel_file(int index)
{
}

static inline unsigned char *read_file_to_buffer(const char *filename, int *size)
{
	return NULL;
}

static inline unsigned char get_lcd_panel_file_type(int index)
{
	return PANEL_FILE_INVILD;
}

#endif

#endif/*_AML_MODEL_H*/

