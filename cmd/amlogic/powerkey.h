/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INI_POWERKEY_H__
#define __INI_POWERKEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_KEY_SUPPORT 8

#define PARSE_TAB_TYPE_REMOTE             0x01
#define PARSE_TAB_TYPE_KEYPAD             0x02

enum {
	CONFIG_LEVEL,
	KEYMAP_LEVEL,
	WAKEUP_LEVEL,
};

int tab_mem_parse(unsigned char *file_buf);
int keymap_mem_parse(char *file_buf);
int parse_tab_file(const char *file_path, unsigned char type);
int parse_key_map(const char *file_path);

#ifdef __cplusplus
}
#endif

#endif //__INI_POWERKEY_H__

