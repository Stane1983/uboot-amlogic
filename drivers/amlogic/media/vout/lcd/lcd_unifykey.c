// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/keyunify.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"

#define LCDUKEY(fmt, args...)     printf("lcd: ukey: " fmt "", ## args)
#define LCDUKEYERR(fmt, args...)     printf("lcd: ukey err: " fmt "", ## args)

#ifdef CONFIG_UNIFY_KEY_MANAGE
int lcd_unifykey_len_check(int key_len, int len)
{
	if (key_len < len) {
		LCDUKEYERR("invalid unifykey length %d, need %d\n", key_len, len);
		return -1;
	}
	return 0;
}

int lcd_unifykey_check_exist(const char *key_name)
{
	int key_exist = 0;
	int ret;

	if (!key_name) {
		LCDUKEYERR("%s: key_name is null\n", __func__);
		return -1;
	}

	ret = key_unify_query_exist(key_name, &key_exist);
	if (ret) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDUKEYERR("%s query exist error\n", key_name);
		return -1;
	}
	if (key_exist == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDUKEYERR("%s is not exist\n", key_name);
		return -1;
	}

	return 0;
}

int lcd_unifykey_check(const char *key_name)
{
	int key_exist = 0, isSecure;
	int ret;

	ret = key_unify_query_exist(key_name, &key_exist);
	if (ret) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDUKEYERR("%s: %s query exist error\n", __func__, key_name);
		return -1;
	}
	if (key_exist == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDUKEYERR("%s: %s is not exist\n", __func__, key_name);
		return -1;
	}
	ret = key_unify_query_secure(key_name, &isSecure);
	if (ret) {
		LCDUKEYERR("%s: %s query secure error\n", __func__, key_name);
		return -1;
	}
	if (isSecure) {
		LCDUKEYERR("%s: %s is secure key\n", __func__, key_name);
		return -1;
	}

	return 0;
}

int lcd_unifykey_get_size(const char *key_name, int *len)
{
	ssize_t key_size = 0;
	int ret;

	ret = lcd_unifykey_check(key_name);
	if (ret)
		return -1;
	ret = key_unify_query_size(key_name, &key_size);
	if (ret < 0)
		return -1;
	if (key_size == 0) {
		LCDUKEYERR("%s: %s size 0!\n", __func__, key_name);
		return -1;
	}
	*len = (int)key_size;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDUKEY("%s: %s size: 0x%x\n", __func__, key_name, *len);

	return 0;
}

int lcd_unifykey_get(const char *key_name, unsigned char *buf, int len)
{
	struct lcd_unifykey_header_s *key_header;
	uint32_t key_crc;
	unsigned int retry_cnt = 0, key_crc32;
	int ret;

	ret = lcd_unifykey_check(key_name);
	if (ret)
		return -1;

lcd_unifykey_get_retry:
	ret = key_unify_read(key_name, buf, len);
	if (ret) {
		LCDUKEYERR("%s: %s error\n", __func__, key_name);
		return -1;
	}

	/* check header */
	if (len <= LCD_UKEY_HEAD_SIZE) {
		LCDUKEYERR("%s: %s key_len %d error\n", __func__, key_name, len);
		return -1;
	}
	key_header = (struct lcd_unifykey_header_s *)buf;
	if (len != key_header->data_len) {  //length check
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDUKEYERR("%s: %s data_len %d is not match key_len %d\n",
				__func__, key_name, key_header->data_len, len);
		}
		if (retry_cnt++ < LCD_UKEY_RETRY_CNT_MAX) {
			memset(buf, 0, len);
			goto lcd_unifykey_get_retry;
		}
		LCDUKEYERR("%s: %s failed\n", __func__, key_name);
		return -1;
	}
	key_crc = crc32(0, &buf[4], (len - 4)); //except crc32
	key_crc32 = (unsigned int)key_crc;
	if (key_crc32 != key_header->crc32) {  //crc32 check
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDUKEYERR("%s: %s header_crc32 0x%08x is not match 0x%08x\n",
				__func__, key_name, key_header->crc32, key_crc32);
		}
		if (retry_cnt++ < LCD_UKEY_RETRY_CNT_MAX) {
			memset(buf, 0, len);
			goto lcd_unifykey_get_retry;
		}
		LCDUKEYERR("%s: %s failed\n", __func__, key_name);
		return -1;
	}

	return 0;
}

int lcd_unifykey_get_tcon(const char *key_name, unsigned char *buf, int len)
{
#ifdef CONFIG_AML_LCD_TCON
	struct lcd_tcon_init_block_header_s *init_header;
	int retry_cnt = 0;
	unsigned int key_crc32;
	int ret;

	ret = lcd_unifykey_check(key_name);
	if (ret)
		return -1;

lcd_unifykey_get_tcon_retry:
	ret = key_unify_read(key_name, buf, len);
	if (ret) {
		LCDUKEYERR("%s: %s error\n", __func__, key_name);
		return -1;
	}

	/* check header */
	if (len <= LCD_TCON_DATA_BLOCK_HEADER_SIZE) {
		LCDUKEYERR("%s: %s key_len %d error\n", __func__, key_name, len);
		return -1;
	}
	init_header = (struct lcd_tcon_init_block_header_s *)buf;
	if (len != init_header->block_size) {  //length check
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDUKEYERR("%s: %s data_len %d is not match key_len %d\n",
				   __func__, key_name, init_header->block_size, len);
		}
		if (retry_cnt++ < LCD_UKEY_RETRY_CNT_MAX) {
			memset(buf, 0, len);
			goto lcd_unifykey_get_tcon_retry;
		}
		LCDUKEYERR("%s: %s failed\n", __func__, key_name);
		return -1;
	}
	key_crc32 = crc32(0, &buf[4], (len - 4)); //except crc32
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDUKEY("%s: %s crc32: 0x%08x, header_crc32: 0x%08x\n",
			__func__, key_name, key_crc32, init_header->crc32);
	}
	if (key_crc32 != init_header->crc32) {  //crc32 check
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDUKEYERR("%s: %s header_crc32 0x%08x is not match 0x%08x\n",
				   __func__, key_name, init_header->crc32, key_crc32);
		}
		if (retry_cnt++ < LCD_UKEY_RETRY_CNT_MAX) {
			memset(buf, 0, len);
			goto lcd_unifykey_get_tcon_retry;
		}
		LCDUKEYERR("%s: %s failed\n", __func__, key_name);
		return -1;
	}

	return 0;
#else
	LCDUKEYERR("Don't support tcon\n");
	return -1;
#endif
}

int lcd_unifykey_get_no_header(const char *key_name, unsigned char *buf, int len)
{
	int ret;

	ret = lcd_unifykey_check(key_name);
	if (ret)
		return -1;

	ret = key_unify_read(key_name, buf, len);
	if (ret) {
		LCDUKEYERR("%s: %s error\n", __func__, key_name);
		return -1;
	}
	return 0;
}

int lcd_unifykey_write(const char *key_name, unsigned char *buf, int len)
{
	key_unify_write(key_name, buf, len);
	return 0;
}

void lcd_unifykey_dump(int index, unsigned int flag)
{
	unsigned char *para;
	int key_len;
	char str[20];
	int ret, i;

#ifdef CONFIG_AML_LCD_TCON
	if ((flag & LCD_UKEY_DEBUG_NORMAL) == 0)
		goto lcd_unifykey_dump_tcon;
#else
	if ((flag & LCD_UKEY_DEBUG_NORMAL) == 0)
		return;
#endif

	/* dump unifykey: lcd */
	if (index > 0)
		sprintf(str, "lcd%d", index);
	else
		sprintf(str, "lcd");
	ret = lcd_unifykey_get_size(str, &key_len);
	if (ret)
		goto lcd_unifykey_dump_next1;
	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDUKEYERR("%s: Not enough memory\n", __func__);
		return;
	}
	memset(para, 0, (sizeof(unsigned char) * key_len));

	ret = lcd_unifykey_get(str, para, key_len);
	if (ret == 0) {
		printf("unifykey: %s:", str);
		for (i = 0; i < key_len; i++) {
			if ((i % 16) == 0)
				printf("\n%03x0:", (i / 16));
			printf(" %02x", para[i]);
		}
	}
	printf("\n");
	free(para);

lcd_unifykey_dump_next1:
	/* dump unifykey: lcd_extern */
	if (index > 0)
		sprintf(str, "lcd%d_extern", index);
	else
		sprintf(str, "lcd_extern");
	ret = lcd_unifykey_get_size(str, &key_len);
	if (ret)
		goto lcd_unifykey_dump_next2;
	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDUKEYERR("%s: Not enough memory\n", __func__);
		return;
	}
	memset(para, 0, (sizeof(unsigned char) * key_len));

	ret = lcd_unifykey_get(str, para, key_len);
	if (ret == 0) {
		printf("unifykey: %s:", str);
		for (i = 0; i < key_len; i++) {
			if ((i % 16) == 0)
				printf("\n%03x0:", (i / 16));
			printf(" %02x", para[i]);
		}
	}
	printf("\n");
	free(para);

lcd_unifykey_dump_next2:
	/* dump unifykey: backlight */
	if (index > 0)
		sprintf(str, "backlight%d", index);
	else
		sprintf(str, "backlight");
	ret = lcd_unifykey_get_size(str, &key_len);
	if (ret)
		return;
	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDUKEYERR("%s: Not enough memory\n", __func__);
		return;
	}
	memset(para, 0, (sizeof(unsigned char) * key_len));

	ret = lcd_unifykey_get(str, para, key_len);
	if (ret == 0) {
		printf("unifykey: %s:", str);
		for (i = 0; i < key_len; i++) {
			if ((i % 16) == 0)
				printf("\n%03x0:", (i / 16));
			printf(" %02x", para[i]);
		}
	}
	printf("\n");
	free(para);
	return;

#ifdef CONFIG_AML_LCD_TCON
lcd_unifykey_dump_tcon:
	/* dump unifykey: lcd_tcon */
	if ((flag & LCD_UKEY_DEBUG_TCON) == 0)
		return;
	if (index > 0)
		sprintf(str, "lcd%d_tcon", index);
	else
		sprintf(str, "lcd_tcon");
	ret = lcd_unifykey_get_size(str, &key_len);
	if (ret)
		return;
	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDUKEYERR("%s: Not enough memory\n", __func__);
		return;
	}
	memset(para, 0, (sizeof(unsigned char) * key_len));

	ret = lcd_unifykey_get_no_header(str, para, key_len);
	if (ret == 0) {
		printf("unifykey: %s:", str);
		for (i = 0; i < key_len; i++) {
			if ((i % 16) == 0)
				printf("\n%03x0:", (i / 16));
			printf(" %02x", para[i]);
		}
	}
	printf("\n");
	free(para);

	/* dump unifykey: lcd_tcon_spi */
	if (index > 0)
		sprintf(str, "lcd%d_tcon_spi", index);
	else
		sprintf(str, "lcd_tcon_spi");
	ret = lcd_unifykey_get_size(str, &key_len);
	if (ret)
		return;
	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDUKEYERR("%s: Not enough memory\n", __func__);
		return;
	}
	memset(para, 0, (sizeof(unsigned char) * key_len));

	ret = lcd_unifykey_get(str, para, key_len);
	if (ret == 0) {
		printf("unifykey: %s:", str);
		for (i = 0; i < key_len; i++) {
			if ((i % 16) == 0)
				printf("\n%03x0:", (i / 16));
			printf(" %02x", para[i]);
		}
	}
	printf("\n");
	free(para);
#endif
}

#else
/* dummy driver */
int lcd_unifykey_len_check(int key_len, int len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_header_check(unsigned char *buf,
			      struct lcd_unifykey_header_s *header)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_check(const char *key_name)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_get_size(const char *key_name, int *len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_get(const char *key_name, unsigned char *buf, int len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_get_tcon(const char *key_name, unsigned char *buf, int len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_get_no_header(const char *key_name, unsigned char *buf, int len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

int lcd_unifykey_write(const char *key_name, unsigned char *buf, int len)
{
	LCDUKEYERR("Don't support unifykey\n");
	return -1;
}

void lcd_unifykey_dump(int index, unsigned int flag)
{
	LCDUKEYERR("Don't support unifykey\n");
}

#endif


