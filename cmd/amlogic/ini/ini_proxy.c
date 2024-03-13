// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "ini_config.h"

#define LOG_TAG "ini_proxy"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_handler.h"
#include "ini_proxy.h"

INI_HANDLER_DATA *_g_handler_data;
static unsigned char *_g_bin_data;

void bin_file_init(void)
{
	if (!_g_bin_data) {
		_g_bin_data = malloc(CC_MAX_INI_FILE_SIZE);
		if (_g_bin_data)
			memset(_g_bin_data, 0, CC_MAX_INI_FILE_SIZE);
	}
}

void bin_file_uninit(void)
{
	if (_g_bin_data) {
		free(_g_bin_data);
		_g_bin_data = NULL;
	}
}

int read_bin_file(const char *filename)
{
	if (!_g_bin_data)
		return -1;

	return _bin_file_read(filename, _g_bin_data);
}

int get_bin_data(unsigned char *file_buf, unsigned int file_size)
{
	if (!_g_bin_data)
		return -1;

	if (file_buf == NULL)
		return -1;

	memcpy(file_buf, _g_bin_data, file_size);
	return 0;
}

void ini_parser_init(void)
{
	if (!_g_handler_data) {
		_g_handler_data = (INI_HANDLER_DATA *)malloc(sizeof(INI_HANDLER_DATA));
		if (_g_handler_data)
			memset((void *)_g_handler_data, 0, sizeof(INI_HANDLER_DATA));
	}
}

void ini_parser_uninit(void)
{
	if (_g_handler_data) {
		ini_parser_free();
		free(_g_handler_data);
		_g_handler_data = NULL;
	}
}

int ini_parse_file(const char *filename)
{
	if (!_g_handler_data)
		return -1;

	return _ini_file_parse(filename, _g_handler_data);
}

int ini_parse_mem(unsigned char *file_buf)
{
	if (!_g_handler_data)
		return -1;

	return ini_mem_parse(file_buf, _g_handler_data);
}

int ini_set_save_file_name(const char *filename)
{
	if (!_g_handler_data)
		return -1;

	return _ini_set_save_file_name(filename, _g_handler_data);
}

void ini_parser_free(void)
{
	if (!_g_handler_data)
		return;

	return _ini_free_mem(_g_handler_data);
}

void ini_print_all(void)
{
	if (!_g_handler_data)
		return;

	return _ini_print_all(_g_handler_data);
}

void ini_list_section(void)
{
	if (!_g_handler_data) {
		ALOGE("%s, ini load file error!\n", __func__);
		return;
	}
	_ini_list_section(_g_handler_data);
}

const char *ini_get_string(const char *section, const char *key, const char *def_value)
{
	if (!_g_handler_data)
		return def_value;

	return _ini_get_string(section, key, def_value, _g_handler_data);
}

int ini_set_string(const char *section, const char *key, const char *value)
{
	if (!_g_handler_data)
		return -1;

	return _ini_set_string(section, key, value, _g_handler_data);
}

int ini_save_to_file(const char *filename)
{
	if (!_g_handler_data)
		return -1;

	return _ini_save_to_file(filename, _g_handler_data);
}
