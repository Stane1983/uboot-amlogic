// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "ini_config.h"

#define LOG_TAG "ini_handler"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_core.h"
#include "ini_handler.h"
#include "ini_platform.h"

static void trim(char *str, char ch);
static void trim_all(char *str);
static INI_SECTION *_get_section(const char *section, INI_HANDLER_DATA *pHandlerData);
static INI_LINE *get_key_line_at_sec(INI_SECTION *pSec, const char *key);
static int set_key_value(void *user, const char *section, const char *name,
			 const char *value, int set_mode);
static int handler(void *user, const char *section, const char *name, const char *value);
static INI_LINE *new_line(const char *name, const char *value);
static INI_SECTION *new_section(const char *section, INI_LINE *pLINE);

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
static void alloc_mem(const char *fun_name, const char *var_name, void *ptr);
static void free_mem(const char *fun_name, const char *var_name, void *ptr);
static void print_alloc_mem_nd(const char *fun_name);
static void print_free_mem_nd(const char *fun_name);
static void clear_mem_nd(void);
#endif

int _bin_file_read(const char *filename, unsigned char *file_buf)
{
	int tmp_ret = -1, rd_cnt = 0, file_size = 0;
	unsigned char *tmp_buf = NULL;

	file_size = ini_get_file_size(filename);
	if (file_size <= 0)
		return -1;

	tmp_buf = (unsigned char *)malloc(file_size * 2);
	if (tmp_buf != NULL) {
		rd_cnt = ini_read_file_to_buffer(filename, 0, file_size, tmp_buf);
		if (rd_cnt > 0) {
			if (file_size > CC_MAX_INI_FILE_SIZE) {
				ALOGE("%s: file \"%s\" size out of support!\n", __func__, filename);
				tmp_ret = -1;
			} else {
				memcpy(file_buf, tmp_buf, file_size);
				tmp_ret = file_size;
			}
		}

		free(tmp_buf);
		tmp_buf = NULL;
	}

	return tmp_ret;
}

int _ini_file_parse(const char *filename, INI_HANDLER_DATA *pHandlerData)
{
	int tmp_ret = -1, rd_cnt = 0, file_size = 0;
	unsigned char *tmp_buf = NULL;

	file_size = ini_get_file_size(filename);
	if (file_size <= 0)
		return -1;

	tmp_buf = (unsigned char *)malloc(file_size * 2);
	if (tmp_buf != NULL) {
		strncpy(pHandlerData->mpFileName, filename, CC_MAX_INI_FILE_NAME_LEN - 1);

		memset((void *)tmp_buf, '\0', (file_size * 2) * sizeof(char));
		rd_cnt = ini_read_file_to_buffer(filename, 0, file_size, tmp_buf);
		if (rd_cnt > 0)
			tmp_ret = ini_mem_parse(tmp_buf, pHandlerData);

		free(tmp_buf);
		tmp_buf = NULL;
	}

	return tmp_ret;
}

int ini_mem_parse(unsigned char *file_buf, INI_HANDLER_DATA *pHandlerData)
{
	// ALOGD("%s, entering...\n", __func__);
	return _ini_mem_parse((char *)file_buf, handler, (void *)pHandlerData);
}

int _ini_set_save_file_name(const char *filename, INI_HANDLER_DATA *pHandlerData)
{
	// ALOGD("%s, entering...\n", __func__);

	strncpy(pHandlerData->mpFileName, filename, CC_MAX_INI_FILE_NAME_LEN - 1);
	return 0;
}

static void _ini_line_free(INI_LINE *pline)
{
	if (pline->Value) {
		memset(pline->Value, 0, pline->value_size);
		free(pline->Value);
	}

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
	free_mem(__func__, "pLine", pline);
#endif

	memset(pline, 0, sizeof(INI_LINE));
	free(pline);
}

void _ini_free_mem(INI_HANDLER_DATA *pHandlerData)
{
	// ALOGD("%s, entering...\n", __func__);

	if (!pHandlerData)
		return;

	INI_SECTION *pNextSec = NULL;
	INI_SECTION *pSec = pHandlerData->mpFirstSection;
	INI_LINE *pNextLine = NULL;
	INI_LINE *pLine = NULL;

	while (pSec) {
		pNextSec = pSec->pNext;
		pLine = pSec->pLine;

		while (pLine) {
			pNextLine = pLine->pNext;
			_ini_line_free(pLine);
			pLine = pNextLine;
		}

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
		free_mem(__func__, "pSec", pSec);
#endif

		memset(pSec, 0, sizeof(INI_SECTION));
		free(pSec);
		pSec = pNextSec;
	}

	pHandlerData->mpFirstSection = NULL;
	pHandlerData->mpCurSection = NULL;

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
	print_alloc_mem_nd(__func__);
	print_free_mem_nd(__func__);
	clear_mem_nd();
#endif
}

static void trim(char *str, char ch)
{
	char *pStr;

	pStr = str;
	while (*pStr != '\0') {
		if (*pStr == ch) {
			char *pTmp = pStr;
			while (*pTmp != '\0') {
				*pTmp = *(pTmp + 1);
				pTmp++;
			}
		} else {
			pStr++;
		}
	}
}

static void trim_all(char *str)
{
	char *pStr = NULL;

	pStr = strchr(str, '\n');
	if (pStr != NULL)
		*pStr = 0;

	int Len = strlen(str);
	if (Len > 0) {
		if (str[Len - 1] == '\r')
			str[Len - 1] = '\0';
	}

	pStr = strchr(str, '#');
	if (pStr != NULL)
		*pStr = 0;

	pStr = strchr(str, ';');
	if (pStr != NULL)
		*pStr = 0;

	trim(str, ' ');
	trim(str, '{');
	trim(str, '\\');
	trim(str, '}');
	trim(str, '\"');
	return;
}

void _ini_print_all(INI_HANDLER_DATA *pHandlerData)
{
	INI_SECTION *pSec = NULL;
	INI_LINE *pLine = NULL;
	char *str;
	int i, j, n, m;

	str = (char *)malloc(512);
	if (!str) {
		printf("%s: malloc print memory error\n", __func__);
		return;
	}

	for (pSec = pHandlerData->mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
		printf("[%s]\n", pSec->Name);
		pLine = pSec->pLine;
		while (pLine) {
			if (pLine->value_size >= 510) {
				printf("  %s = ", pLine->Name);
				n = pLine->value_size / 510;
				m = pLine->value_size % 510;
				for (i = 0; i < n; i++) {
					j = i * 510;
					strncpy(str, pLine->Value + j, 510);
					str[510] = '\0';
					printf("%s\n", str);
				}
				if (m) {
					j = n * 510;
					strncpy(str, pLine->Value + j, m);
					str[m] = '\0';
					printf("%s\n", str);
				}
			} else {
				printf("  %s = %s\n", pLine->Name, pLine->Value);
			}

			pLine = pLine->pNext;
		}
		printf("\n");
	}

	memset(str, 0, 512);
	free(str);
}

void _ini_list_section(INI_HANDLER_DATA *pHandlerData)
{
	INI_SECTION *pSec = NULL;

	for (pSec = pHandlerData->mpFirstSection; pSec != NULL; pSec = pSec->pNext)
		printf("  %s\n", pSec->Name);
}

static INI_SECTION *_get_section(const char *section, INI_HANDLER_DATA *pHandlerData)
{
	INI_SECTION *pSec = NULL;

	for (pSec = pHandlerData->mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
		if (strcmp(pSec->Name, section) == 0)
			return pSec;
	}

	return NULL;
}

static INI_LINE *get_key_line_at_sec(INI_SECTION *pSec, const char *key)
{
	INI_LINE *pLine = NULL;

	for (pLine = pSec->pLine; pLine != NULL; pLine = pLine->pNext) {
		if (strcmp(pLine->Name, key) == 0)
			return pLine;
	}
	return NULL;
}

const char *_ini_get_string(const char *section, const char *key,
			    const char *def_value, INI_HANDLER_DATA *pHandlerData)
{
	INI_SECTION *pSec = _get_section(section, pHandlerData);

	if (!pSec) {
		// ALOGD("%s, section %s is NULL\n", __func__, section);
		return def_value;
	}

	INI_LINE *pLine = get_key_line_at_sec(pSec, key);
	if (pLine == NULL) {
		// ALOGD("%s, key \"%s\" is NULL\n", __func__, key);
		return def_value;
	}

	return pLine->Value;
}

int _ini_set_string(const char *section, const char *key,
		    const char *value, INI_HANDLER_DATA *pHandlerData)
{
	set_key_value(pHandlerData, section, key, value, 1);
	return 0;
}

int _ini_save_to_file(const char *filename, INI_HANDLER_DATA *pHandlerData)
{
#if (defined CC_COMPILE_IN_PC || defined CC_COMPILE_IN_ANDROID)
	const char *fname = NULL;
	FILE *fp = NULL;

	if (filename == NULL) {
		if (strlen(pHandlerData->mpFileName) == 0) {
			ALOGE("%s, save file name is NULL!!!\n", __func__);
			return -1;

		fname = pHandlerData->mpFileName;
	} else {
		fname = filename;
	}

	fp = fopen(fname, "wb")
	if (!fp) {
		ALOGE("%s, Open file \"%s\" ERROR (%s)!!!\n", __func__, fname, strerror(errno));
		return -1;
	}

	INI_SECTION *pSec = NULL;
	for (pSec = pHandlerData->mpFirstSection; pSec != NULL; pSec = pSec->pNext) {
		fprintf(fp, "[%s]\r\n", pSec->Name);
		INI_LINE *pLine = NULL;

		for (pLine = pSec->pLine; pLine != NULL; pLine = pLine->pNext)
			fprintf(fp, "%s = %s\r\n", pLine->Name, pLine->Value);
	}

	fflush(fp);
	fsync(fileno(fp));

	fclose(fp);
	fp = NULL;

	return 0;
#elif (defined CC_COMPILE_IN_UBOOT)
	return 0;
#endif
}

static int ini_set_line_exist_key_val(INI_LINE *pline, const char *value, unsigned int set_mode)
{
	char *pvalue = NULL;
	int pre_len, new_len, n;

	if (!pline || !value)
		return -1;

	pre_len = pline->value_size;
	new_len = strlen(value) + 1;

	if (set_mode == 1) {
		pvalue = (char *)malloc(new_len);
		if (!pvalue)
			return -1;
		memset(pvalue, 0, new_len);

		memset(pline->Value, 0, pre_len);
		free(pline->Value);
		pline->Value = pvalue;
		strcpy(pline->Value, value);
		pline->value_size = new_len;
		return 0;
	}

	pvalue = (char *)malloc(pre_len + new_len - 1);
	if (!pvalue)
		return -1;
	memset(pvalue, 0, (pre_len + new_len - 1));

	n = sprintf(pvalue, "%s", pline->Value);

	memset(pline->Value, 0, pre_len);
	free(pline->Value);

	pline->Value = pvalue;
	sprintf(pline->Value + n, "%s", value);
	pline->value_size = pre_len + new_len - 1;
	return 0;
}

static INI_LINE *new_line(const char *name, const char *value)
{
	INI_LINE *pLine = NULL;
	char *pvalue = NULL;
	unsigned int val_size;

	pLine = (INI_LINE *)malloc(sizeof(INI_LINE));
	if (pLine != NULL) {
		memset(pLine, 0, sizeof(INI_LINE));
		val_size = strlen(value) + 1;
		pvalue = (char *)malloc(val_size);
		if (!pvalue) {
			ALOGE("%s: malloc value error, size %d\n", __func__, val_size);
			free(pLine);
			return NULL;
		}
		memset(pvalue, 0, val_size);

		pLine->pNext = NULL;
		strncpy(pLine->Name, name, sizeof(pLine->Name) - 1);
		pLine->Name[sizeof(pLine->Name) - 1] = '\0';

		pLine->Value = pvalue;
		strcpy(pLine->Value, value);
		pLine->Value[val_size - 1] = '\0';
		pLine->value_size = val_size;

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
		alloc_mem(__func__, "pLine", pLine);
#endif
	}

	return pLine;
}

static INI_SECTION *new_section(const char *section, INI_LINE *pLine)
{
	INI_SECTION *pSec = NULL;

	pSec = (INI_SECTION *)malloc(sizeof(INI_SECTION));
	if (pSec != NULL) {
		memset(pSec, 0, sizeof(INI_SECTION));
		pSec->pLine = pLine;
		pSec->pNext = NULL;
		strncpy(pSec->Name, section, sizeof(pSec->Name) - 1);
		pSec->Name[sizeof(pSec->Name) - 1] = '\0';

#if CC_MEMORY_ALLOC_FREE_TRACE == 1
		alloc_mem(__func__, "pSec", pSec);
#endif
	}

	return pSec;
}

static int set_key_value(void *user, const char *section, const char *key,
			 const char *value, int set_mode)
{
	INI_LINE *pLine = NULL;
	INI_SECTION *pSec = NULL;
	INI_HANDLER_DATA *pHandlerData = (INI_HANDLER_DATA *)user;
	int ret;

	if (section == NULL || key == NULL || value == NULL)
		return 1;

	trim_all((char *)value);
	if (value[0] == '\0')
		return 1;

	if (strlen(key) > CC_MAX_INI_LINE_NAME_LEN) {
		ALOGE("key name is too long, limit %d.\n", CC_MAX_INI_LINE_NAME_LEN);
		return 1;
	}
	if (strlen(value) > CC_MAX_INI_FILE_LINE_LEN) {
		ALOGE("value is too long, limit %d.\n", CC_MAX_INI_FILE_LINE_LEN);
		return 1;
	}

	if (pHandlerData->mpFirstSection == NULL) {
		pLine = new_line(key, value);
		if (!pLine)
			goto ini_set_key_value_end;
		pSec = new_section(section, pLine);
		if (!pSec) {
			_ini_line_free(pLine);
			goto ini_set_key_value_end;
		}

		pHandlerData->mpFirstSection = pSec;
		pHandlerData->mpCurSection = pSec;
		pSec->pCurLine = pLine;
	} else {
		pSec = _get_section(section, pHandlerData);
		if (!pSec) {
			pLine = new_line(key, value);
			if (!pLine)
				goto ini_set_key_value_end;
			pSec = new_section(section, pLine);
			if (!pSec) {
				_ini_line_free(pLine);
				goto ini_set_key_value_end;
			}

			pHandlerData->mpCurSection->pNext = pSec;
			pHandlerData->mpCurSection = pSec;
			pSec->pCurLine = pLine;
		} else {
			pLine = get_key_line_at_sec(pSec, key);
			if (pLine == NULL) {
				pLine = new_line(key, value);
				if (!pLine)
					goto ini_set_key_value_end;

				pSec->pCurLine->pNext = pLine;
				pSec->pCurLine = pLine;
			} else {
				ret = ini_set_line_exist_key_val(pLine, value, set_mode);
				if (ret)
					goto ini_set_key_value_end;
			}
		}
	}

	return 0;

ini_set_key_value_end:
	printf("%s: section[%s]: key_name: %s, error\n", __func__, section, key);
	return 0;
}

static int handler(void *user, const char *section, const char *name, const char *value)
{
	// ALOGD("%s, section = %s, name = %s, value = %s\n", __func__, section, name, value);
	set_key_value(user, section, name, value, 0);
	return 1;
}

#if CC_MEMORY_ALLOC_FREE_TRACE == 1

#define CC_MEM_RECORD_CNT (1024)

typedef struct tag_memnd {
	char fun_name[50];
	char var_name[50];
	void *ptr;
} memnd;

static memnd gMemAllocItems[CC_MEM_RECORD_CNT];
static int gMemAllocInd = 0;

static memnd gMemFreeItems[CC_MEM_RECORD_CNT];
static int gMemFreeInd = 0;

static void alloc_mem(const char *fun_name, const char *var_name, void *ptr)
{
	strncpy(gMemAllocItems[gMemAllocInd].fun_name, fun_name,
		sizeof(gMemAllocItems[gMemAllocInd].fun_name) - 1);
	gMemAllocItems[gMemAllocInd].fun_name[sizeof(gMemAllocItems[gMemAllocInd].fun_name) - 1] = '\0';
	strncpy(gMemAllocItems[gMemAllocInd].var_name, var_name,
		sizeof(gMemAllocItems[gMemAllocInd].var_name) - 1);
	gMemAllocItems[gMemAllocInd].var_name[sizeof(gMemAllocItems[gMemAllocInd].var_name) - 1] = '\0';
	gMemAllocItems[gMemAllocInd].ptr = ptr;

	gMemAllocInd += 1;
}

static void free_mem(const char *fun_name, const char *var_name, void *ptr)
{
	strncpy(gMemFreeItems[gMemFreeInd].fun_name, fun_name,
		sizeof(gMemFreeItems[gMemFreeInd].fun_name) - 1);
	gMemFreeItems[gMemFreeInd].fun_name[sizeof(gMemFreeItems[gMemFreeInd].fun_name) - 1] = '\0';
	strncpy(gMemFreeItems[gMemFreeInd].var_name, var_name,
		sizeof(gMemFreeItems[gMemFreeInd].var_name) - 1);
	gMemFreeItems[gMemFreeInd].var_name[sizeof(gMemFreeItems[gMemFreeInd].var_name) - 1] = '\0';

	gMemFreeItems[gMemFreeInd].ptr = ptr;

	gMemFreeInd += 1;
}

static void printMemND(const char *fun_name, memnd *tmp_nd, int tmp_cnt)
{
#if CC_MEMORY_ALLOC_FREE_TRACE_PRINT_ALL == 1
	int i = 0;

	ALOGD("fun_name = %s, total_cnt = %d\n", fun_name, tmp_cnt);

	for (i = 0; i < tmp_cnt; i++)
		ALOGD("fun_name = %s, var_name = %s, ptr = %p\n", tmp_nd[i].fun_name, tmp_nd[i].var_name, tmp_nd[i].ptr);
#endif
}

static void print_free_mem_nd(const char *fun_name)
{
	printMemND(__func__, gMemFreeItems, gMemFreeInd);
}

static void print_alloc_mem_nd(const char *fun_name)
{
	printMemND(__func__, gMemAllocItems, gMemAllocInd);
}

static void clear_mem_nd(void)
{
	gMemAllocInd = 0;
	gMemFreeInd = 0;
	memset((void *)gMemAllocItems, 0, sizeof(memnd) * CC_MEM_RECORD_CNT);
	memset((void *)gMemFreeItems, 0, sizeof(memnd) * CC_MEM_RECORD_CNT);
}
#endif
