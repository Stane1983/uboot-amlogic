// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
// #include <asm/arch/io.h>
#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
#include <amlogic/tee_aml.h>
#endif
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_memory.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "lcd_tcon.h"
#include "env.h"
#include <linux/crc32.h>

enum {
	TCON_AXI_MEM_TYPE_OD = 0,
	TCON_AXI_MEM_TYPE_DEMURA,
};

#define TCON_IRQ_TIMEOUT_MAX    BIT(17)
static struct lcd_tcon_config_s *lcd_tcon_conf;
static struct tcon_rmem_s tcon_rmem;
static struct tcon_mem_map_table_s tcon_mm_table = {
	.version = 0xff,
	.block_cnt = 0,
	.data_complete = 0,
	.bin_path_valid = 0,

	.lut_valid_flag = 0,
};
static struct lcd_tcon_local_cfg_s tcon_local_cfg;

int lcd_tcon_valid_check(void)
{
	if (!lcd_tcon_conf) {
		LCDERR("invalid tcon data\n");
		return -1;
	}
	if (lcd_tcon_conf->tcon_valid == 0) {
		LCDERR("invalid tcon\n");
		return -1;
	}

	return 0;
}

struct lcd_tcon_config_s *get_lcd_tcon_config(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return lcd_tcon_conf;
}

struct tcon_rmem_s *get_lcd_tcon_rmem(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return &tcon_rmem;
}

struct tcon_mem_map_table_s *get_lcd_tcon_mm_table(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return &tcon_mm_table;
}

struct lcd_tcon_local_cfg_s *get_lcd_tcon_local_cfg(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return &tcon_local_cfg;
}

unsigned int lcd_tcon_data_size_align(unsigned int size)
{
	unsigned int new_size;

	/* ready for burst 128bit */
	new_size = ((size + 15) / 16) * 16;

	return new_size;
}

unsigned char lcd_tcon_checksum(unsigned char *buf, unsigned int len)
{
	unsigned int temp = 0;
	unsigned int i;

	if (!buf)
		return 0;
	if (len == 0)
		return 0;
	for (i = 0; i < len; i++)
		temp += buf[i];

	return (unsigned char)(temp & 0xff);
}

unsigned char lcd_tcon_lrc(unsigned char *buf, unsigned int len)
{
	unsigned char temp = 0;
	unsigned int i;

	if (!buf)
		return 0xff;
	if (len == 0)
		return 0xff;
	temp = buf[0];
	for (i = 1; i < len; i++)
		temp = temp ^ buf[i];

	return temp;
}

/* **********************************
 * tcon function api
 * **********************************
 */
//ret:  bit[0]:tcon: fatal error, block driver
//      bit[1]:tcon: warning, only print warning message
int lcd_tcon_init_setting_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
		unsigned char *core_reg_table)
{
	char *ferr_str, *warn_str;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	ferr_str = malloc(PR_BUF_MAX);
	if (!ferr_str) {
		LCDERR("tcon: setting_check fail for NOMEM\n");
		return 0;
	}
	memset(ferr_str, 0, PR_BUF_MAX);
	warn_str = malloc(PR_BUF_MAX);
	if (!warn_str) {
		LCDERR("tcon: setting_check fail for NOMEM\n");
		free(ferr_str);
		return 0;
	}
	memset(warn_str, 0, PR_BUF_MAX);

	if (lcd_tcon_conf->tcon_check)
		ret = lcd_tcon_conf->tcon_check(pdrv, ptiming, core_reg_table, ferr_str, warn_str);
	else
		ret = 0;

	if (ret) {
		printf("**************** lcd tcon setting check ****************\n");
		if (ret & 0x1) {
			printf("lcd: tcon: FATAL ERROR:\n"
				"%s\n", ferr_str);
		}
		if (ret & 0x2) {
			printf("lcd: tcon: WARNING:\n"
				"%s\n", warn_str);
		}
		printf("************** lcd tcon setting check end ****************\n");
	}

	memset(ferr_str, 0, PR_BUF_MAX);
	memset(warn_str, 0, PR_BUF_MAX);
	free(ferr_str);
	free(warn_str);

	return ret;
}

#ifdef CONFIG_CMD_INI
static int lcd_tcon_bin_path_resv_mem_set(void)
{
	unsigned char *buf, *mem_vaddr;
	unsigned int data_size, block_size, temp_crc, n, i;

	if (tcon_rmem.flag == 0)
		return 0;

	buf = handle_tcon_path_resv_mem_get(tcon_rmem.bin_path_rmem.mem_size);
	if (!buf) {
		LCDERR("%s: bin_path buf invalid\n", __func__);
		return -1;
	}

	data_size = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);

	if (tcon_mm_table.data_size) {
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			block_size = tcon_mm_table.data_size[i];
			if (block_size == 0)
				continue;
			n = 32 + (i * 256);
			buf[n] = block_size & 0xff;
			buf[n + 1] = (block_size >> 8) & 0xff;
			buf[n + 2] = (block_size >> 16) & 0xff;
			buf[n + 3] = (block_size >> 24) & 0xff;
		}

		/* update data check */
		temp_crc = crc32(0, &buf[4], (data_size - 4));
		buf[0] = temp_crc & 0xff;
		buf[1] = (temp_crc >> 8) & 0xff;
		buf[2] = (temp_crc >> 16) & 0xff;
		buf[3] = (temp_crc >> 24) & 0xff;
	}

	mem_vaddr = (unsigned char *)(unsigned long)(tcon_rmem.bin_path_rmem.mem_paddr);
	memcpy(mem_vaddr, buf, data_size);

	return 0;
}
#endif

int lcd_tcon_enable(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	if (lcd_tcon_conf->tcon_enable)
		lcd_tcon_conf->tcon_enable(pdrv);

	return 0;
}

void lcd_tcon_disable(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	LCDPR("%s\n", __func__);

	if (lcd_tcon_conf->tcon_disable)
		lcd_tcon_conf->tcon_disable(pdrv);
	if (lcd_tcon_conf->tcon_global_reset) {
		lcd_tcon_conf->tcon_global_reset(pdrv);
		LCDPR("reset tcon\n");
	}
}

void lcd_tcon_global_reset(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (lcd_tcon_conf->tcon_global_reset) {
		lcd_tcon_conf->tcon_global_reset(pdrv);
		LCDPR("reset tcon\n");
	}
}

int lcd_tcon_top_init(struct aml_lcd_drv_s *pdrv)
{
	int ret = lcd_tcon_valid_check();

	if (ret)
		return ret;

	ret = -1;
	if (lcd_tcon_conf->tcon_top_init)
		ret = lcd_tcon_conf->tcon_top_init(pdrv);

	return ret;
}

static int lcd_tcon_forbidden_check(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	if (lcd_tcon_conf->tcon_forbidden_check)
		ret = lcd_tcon_conf->tcon_forbidden_check();
	else
		ret = 0;

	return ret;
}

void lcd_tcon_dbg_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming)
{
	int ret;

	ret = lcd_tcon_init_setting_check(pdrv, ptiming, tcon_local_cfg.cur_core_reg_table);
	if (ret == 0)
		printf("lcd tcon setting check: PASS\n");
}

static int lcd_tcon_data_multi_match_policy_check(struct aml_lcd_drv_s *pdrv,
		struct lcd_tcon_data_part_ctrl_s *ctrl_part, unsigned char *p)
{
#ifdef CONFIG_AML_LCD_BACKLIGHT
	struct aml_bl_drv_s *bldrv;
	struct bl_pwm_config_s *bl_pwm = NULL;
#endif
	unsigned int data_byte, data_cnt, data, min, max;
	unsigned int temp, j, k;

	if (!ctrl_part)
		return -1;
	if (!(ctrl_part->ctrl_data_flag & LCD_TCON_DATA_CTRL_FLAG_MULTI))
		return -1;

	data_byte = ctrl_part->data_byte_width;
	data_cnt = ctrl_part->data_cnt;

	k = 0;
	data = 0;
	min = 0;
	max = 0;

	switch (ctrl_part->ctrl_method) {
	case LCD_TCON_DATA_CTRL_MULTI_VFREQ:
		if (data_cnt != 2)
			goto lcd_tcon_data_multi_match_check_err_data_cnt;
		temp = pdrv->config.timing.act_timing.frame_rate;

		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_match_check_exit;
		break;
	case LCD_TCON_DATA_CTRL_MULTI_BL_LEVEL:
#ifdef CONFIG_AML_LCD_BACKLIGHT
		bldrv = aml_bl_get_driver(pdrv->index);
		if (!bldrv)
			goto lcd_tcon_data_multi_match_check_err_type;
		temp = bldrv->level;

		if (data_cnt != 2)
			goto lcd_tcon_data_multi_match_check_err_data_cnt;
		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_match_check_exit;
#endif
		break;
	case LCD_TCON_DATA_CTRL_MULTI_BL_PWM_DUTY:
#ifdef CONFIG_AML_LCD_BACKLIGHT
		bldrv = aml_bl_get_driver(pdrv->index);
		if (!bldrv)
			goto lcd_tcon_data_multi_match_check_err_type;

		if (data_cnt != 3)
			goto lcd_tcon_data_multi_match_check_err_data_cnt;
		for (j = 0; j < data_byte; j++)
			data |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));

		switch (bldrv->config.method) {
		case BL_CTRL_PWM:
			bl_pwm = bldrv->config.bl_pwm;
			break;
		case BL_CTRL_PWM_COMBO:
			if (data == 0)
				bl_pwm = bldrv->config.bl_pwm_combo0;
			else
				bl_pwm = bldrv->config.bl_pwm_combo1;
			break;
		default:
			break;
		}
		if (!bl_pwm)
			goto lcd_tcon_data_multi_match_check_err_type;

		temp = bl_pwm->pwm_duty;
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_match_check_exit;
#endif
		break;
	case LCD_TCON_DATA_CTRL_DEFAULT:
		return 1;
	default:
		return -1;
	}

	return 0;

lcd_tcon_data_multi_match_check_exit:
	return -1;

lcd_tcon_data_multi_match_check_err_data_cnt:
	LCDERR("%s: ctrl_part %s data_cnt error\n", __func__, ctrl_part->name);
	return -1;

#ifdef CONFIG_AML_LCD_BACKLIGHT
lcd_tcon_data_multi_match_check_err_type:
	LCDERR("%s: ctrl_part %s type invalid\n", __func__, ctrl_part->name);
	return -1;
#endif
}

/* return:
 *    0: matched
 *    1: dft list
 *   -1: not match
 */
int lcd_tcon_data_multi_match_find(struct aml_lcd_drv_s *pdrv, unsigned char *data_buf)
{
	struct lcd_tcon_data_block_header_s *block_header;
	struct lcd_tcon_data_block_ext_header_s *ext_header;
	struct lcd_tcon_data_part_ctrl_s *ctrl_part;
	unsigned char *p, part_type;
	unsigned int size, data_offset, offset, i;
	unsigned short part_cnt;
	int ret;

	block_header = (struct lcd_tcon_data_block_header_s *)data_buf;
	p = data_buf + LCD_TCON_DATA_BLOCK_HEADER_SIZE;
	ext_header = (struct lcd_tcon_data_block_ext_header_s *)p;
	part_cnt = ext_header->part_cnt;

	data_offset = LCD_TCON_DATA_BLOCK_HEADER_SIZE + block_header->ext_header_size;
	size = 0;
	for (i = 0; i < part_cnt; i++) {
		p = data_buf + data_offset;
		part_type = p[LCD_TCON_DATA_PART_NAME_SIZE + 3];

		switch (part_type) {
		case LCD_TCON_DATA_PART_TYPE_CONTROL:
			offset = LCD_TCON_DATA_PART_CTRL_SIZE_PRE;
			ctrl_part = (struct lcd_tcon_data_part_ctrl_s *)p;
			size = offset + (ctrl_part->data_cnt *
					 ctrl_part->data_byte_width);
			if (!(ctrl_part->ctrl_data_flag & LCD_TCON_DATA_CTRL_FLAG_MULTI))
				break;
			ret = lcd_tcon_data_multi_match_policy_check(pdrv,
				ctrl_part, (p + offset));
			if (ret == 0)
				return 0;
			if (ret == 1)
				return 1;
			break;
		default:
			return -1;
		}
		data_offset += size;
	}

	return -1;
}

/* **********************************
 * tcon config
 * **********************************
 */
static int lcd_tcon_vac_load(void)
{
	unsigned char *buff = tcon_rmem.vac_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.vac_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_vac(buff, tcon_rmem.vac_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no vac data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: vac_data data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: vac_data checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: vac_data lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: vac_data pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 30; i++)
			LCDPR("vac_data[%d]: 0x%02x\n", i, buff[i * 1]);
	}
#endif
	return ret;
}

static int lcd_tcon_demura_set_load(void)
{
	unsigned char *buff = tcon_rmem.demura_set_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.demura_set_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_demura_set(buff, tcon_rmem.demura_set_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no demura_set data\n", __func__);
		return -1;
	}

	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: demura_set data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: demura_set checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: demura_set lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: demura_set pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("demura_set[%d]: 0x%x\n", i, buff[i]);
	}
#endif
	return ret;
}

static int lcd_tcon_demura_lut_load(void)
{
	unsigned char *buff = tcon_rmem.demura_lut_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.demura_lut_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_demura_lut(buff, tcon_rmem.demura_lut_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no demura_lut data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: demura_lut data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: demura_lut checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: demura_lut lrc error\n", __func__);
		return -1;
	}
	if ((buff[6] != 0x55) || (buff[7] != 0xaa)) {
		LCDERR("%s: demura_lut pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("demura_lut[%d]: 0x%02x\n", i, buff[i]);
	}
#endif
	return ret;
}

static int lcd_tcon_acc_lut_load(void)
{
	unsigned char *buff = tcon_rmem.acc_lut_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.acc_lut_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_acc_lut(buff, tcon_rmem.acc_lut_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no acc_lut data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: acc_lut data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: acc_lut checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: acc_lut lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: acc_lut pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("acc_lut[%d]: 0x%02x\n", i, buff[i]);
	}
#endif
	return ret;
}

static void lcd_tcon_data_complete_check(struct aml_lcd_drv_s *pdrv, int index)
{
	unsigned char *table = tcon_mm_table.core_reg_table;
	int i, n = 0;

	if (tcon_mm_table.data_complete)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: index %d\n", __func__, index);

	tcon_mm_table.block_bit_flag |= (1 << index);
	for (i = 0; i < tcon_mm_table.block_cnt; i++) {
		if (tcon_mm_table.block_bit_flag & (1 << i))
			n++;
	}
	if (n < tcon_mm_table.block_cnt)
		return;

	tcon_mm_table.data_complete = 1;
	LCDPR("%s: data_complete: %d\n", __func__, tcon_mm_table.data_complete);

	/* specially check demura setting */
	if (pdrv->data->chip_type == LCD_CHIP_TL1 ||
		pdrv->data->chip_type == LCD_CHIP_TM2) {
		if (tcon_mm_table.demura_cnt < 2) {
			tcon_mm_table.lut_valid_flag &= ~LCD_TCON_DATA_VALID_DEMURA;
			if (table) {
				/* disable demura */
				table[0x178] = 0x38;
				table[0x17c] = 0x20;
				table[0x181] = 0x00;
				table[0x23d] &= ~(1 << 0);
			}
		}
	}
}

void lcd_tcon_data_block_regen_crc(unsigned char *data)
{
	unsigned int raw_crc32 = 0, new_crc32 = 0;
	struct lcd_tcon_data_block_header_s *header;

	if (!data)
		return;
	header = (struct lcd_tcon_data_block_header_s *)data;

	raw_crc32 = (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	new_crc32 = crc32(0, data + 4, header->block_size - 4);
	if (raw_crc32 != new_crc32) {
		data[0] = (unsigned char)(new_crc32 & 0xff);
		data[1] = (unsigned char)((new_crc32 >> 8) & 0xff);
		data[2] = (unsigned char)((new_crc32 >> 16) & 0xff);
		data[3] = (unsigned char)((new_crc32 >> 24) & 0xff);
	}
}

static int lcd_tcon_data_load(struct aml_lcd_drv_s *pdrv, unsigned char *data_buf, int index)
{
	struct lcd_tcon_data_block_header_s *block_header;
	struct lcd_tcon_init_block_header_s *init_header;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct lcd_detail_timing_s match_timing;
	unsigned char *core_reg_table;

	if (!data_buf) {
		LCDERR("%s: data_buf is null\n", __func__);
		return -1;
	}
	if (!tcon_mm_table.data_size) {
		LCDERR("%s: data_size buf error\n", __func__);
		return -1;
	}

	block_header = (struct lcd_tcon_data_block_header_s *)data_buf;
	if (block_header->block_size < sizeof(struct lcd_tcon_data_block_header_s)) {
		LCDERR("%s: block[%d] size 0x%x error\n",
			__func__, index, block_header->block_size);
		return -1;
	}

	if (is_block_type_basic_init(block_header->block_type)) {
		if (!tcon_conf)
			return -1;

		init_header = (struct lcd_tcon_init_block_header_s *)data_buf;
		core_reg_table = data_buf + sizeof(struct lcd_tcon_data_block_header_s);

		if (tcon_conf->tcon_init_table_pre_proc)
			tcon_conf->tcon_init_table_pre_proc(core_reg_table);
		lcd_tcon_data_block_regen_crc(data_buf);

		match_timing.h_active = init_header->h_active;
		match_timing.v_active = init_header->v_active;
		lcd_tcon_init_setting_check(pdrv, &match_timing, core_reg_table);
	} else {
		tcon_mm_table.lut_valid_flag |= block_header->block_flag;
		if (block_header->block_flag == LCD_TCON_DATA_VALID_DEMURA)
			tcon_mm_table.demura_cnt++;
	}

	tcon_mm_table.data_size[index] = block_header->block_size;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s block[%d]: size=0x%x, type=0x%02x, name=%s\n",
			__func__, index,
			block_header->block_size,
			block_header->block_type,
			block_header->name);
	}

	lcd_tcon_data_complete_check(pdrv, index);

	return 0;
}

static int is_bin_type_dma(unsigned char *data_buf)
{
	struct lcd_tcon_data_block_header_s *block_header;

	if (!data_buf)
		return 0;

	block_header = (struct lcd_tcon_data_block_header_s *)data_buf;

	if (!is_block_type_basic_init(block_header->block_type) &&
		is_block_ctrl_dma(block_header->block_ctrl))
		return 1;

	return 0;
}

int get_lcd_tcon_data_size(unsigned char *data_buf)
{
	struct lcd_tcon_data_block_header_s *block_header;

	block_header = (struct lcd_tcon_data_block_header_s *)data_buf;

	if (block_header)
		return block_header->block_size;
	else
		return 0;
}

static int lcd_tcon_reserved_mem_data_load(struct aml_lcd_drv_s *pdrv)
{
	unsigned char *table;
#ifdef CONFIG_CMD_INI
	unsigned char *vaddr;
	unsigned int size = 0;
	int i, data_relocate = 0;
	char name[32];
#endif
	int ret;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: mm_table version: %d\n", __func__, tcon_mm_table.version);

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	if (tcon_mm_table.version == 0) {
		table = tcon_mm_table.core_reg_table;
		if (!table)
			return 0;

		if (pdrv->data->chip_type == LCD_CHIP_TL1 ||
		    pdrv->data->chip_type == LCD_CHIP_TM2) {
			ret = lcd_tcon_vac_load();
			if (ret == 0)
				tcon_mm_table.lut_valid_flag |= LCD_TCON_DATA_VALID_VAC;
			ret = lcd_tcon_demura_set_load();
			if (ret)  {
				table[0x178] = 0x38;
				table[0x17c] = 0x20;
				table[0x181] = 0x00;
				table[0x23d] &= ~(1 << 0);
			} else {
				ret = lcd_tcon_demura_lut_load();
				if (ret) {
					table[0x178] = 0x38;
					table[0x17c] = 0x20;
					table[0x181] = 0x00;
					table[0x23d] &= ~(1 << 0);
				} else {
					tcon_mm_table.lut_valid_flag |= LCD_TCON_DATA_VALID_DEMURA;
				}
			}
		}

		ret = lcd_tcon_acc_lut_load();
		if (ret == 0)
			tcon_mm_table.lut_valid_flag |= LCD_TCON_DATA_VALID_ACC;
		tcon_mm_table.data_complete = 1;
	} else {
		if (!tcon_mm_table.data_mem_vaddr) {
			LCDERR("%s: data_mem error\n", __func__);
			return -1;
		}
		if (!tcon_mm_table.data_size) {
			LCDERR("%s: data_size error\n", __func__);
			return -1;
		}
#ifdef CONFIG_CMD_INI
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			ret = handle_tcon_data_load(tcon_mm_table.data_mem_vaddr, i);
			if (ret)
				continue;

			if (!tcon_mm_table.data_mem_vaddr[i])
				continue;

			if (is_bin_type_dma(tcon_mm_table.data_mem_vaddr[i])) {
				size = get_lcd_tcon_data_size(tcon_mm_table.data_mem_vaddr[i]);
				if (lrm_exist()) {
					sprintf(name, "lcd_tcon_data%d", i);
					vaddr = (unsigned char *)lrm_phys_alloc_tail(size, name);
					data_relocate = 1;
					LCDPR("%s data relocate from rsvd\n", __func__);
				} else if ((unsigned long)tcon_mm_table.data_mem_vaddr[i] & 0xf) {
					vaddr = memalign(16, size);
					data_relocate = 1;
					LCDPR("%s data relocate from heap\n", __func__);
				} else {
					vaddr = NULL;
					data_relocate = 0;
					LCDPR("%s data no need relocate\n", __func__);
				}

				if (data_relocate == 1) {
					if (vaddr) {
						memcpy(vaddr,
							tcon_mm_table.data_mem_vaddr[i], size);
						free(tcon_mm_table.data_mem_vaddr[i]);
						tcon_mm_table.data_mem_vaddr[i] = vaddr;
					} else {
						free(tcon_mm_table.data_mem_vaddr[i]);
						tcon_mm_table.data_mem_vaddr[i] = NULL;
						continue;
					}
				}
			}
			lcd_tcon_data_load(pdrv, tcon_mm_table.data_mem_vaddr[i], i);
		}
#endif
	}

	return 0;
}

static int lcd_tcon_bin_path_update(unsigned int size)
{
#ifdef CONFIG_CMD_INI
	unsigned char *mem_vaddr;
	unsigned int data_size, block_cnt;
	unsigned int data_crc32, temp_crc32;

	/* notice: different with kernel flow: mem_vaddr is not mapping to mem_paddr */
	tcon_rmem.bin_path_rmem.mem_vaddr = handle_tcon_path_mem_get(size);
	if (!tcon_rmem.bin_path_rmem.mem_vaddr) {
		LCDERR("%s: get mem error\n", __func__);
		return -1;
	}
	mem_vaddr = tcon_rmem.bin_path_rmem.mem_vaddr;
	data_size = mem_vaddr[4] |
		(mem_vaddr[5] << 8) |
		(mem_vaddr[6] << 16) |
		(mem_vaddr[7] << 24);
	if (data_size < 32) { /* header size */
		LCDERR("%s: tcon_bin_path data_size error\n", __func__);
		return -1;
	}
	block_cnt = mem_vaddr[16] |
		(mem_vaddr[17] << 8) |
		(mem_vaddr[18] << 16) |
		(mem_vaddr[19] << 24);
	if (block_cnt > 32) {
		LCDERR("%s: tcon_bin_path block_cnt error\n", __func__);
		return -1;
	}
	data_crc32 = mem_vaddr[0] |
		(mem_vaddr[1] << 8) |
		(mem_vaddr[2] << 16) |
		(mem_vaddr[3] << 24);
	temp_crc32 = crc32(0, &mem_vaddr[4], (data_size - 4));
	if (data_crc32 != temp_crc32) {
		LCDERR("%s: tcon_bin_path data crc error\n", __func__);
		return -1;
	}

	tcon_mm_table.version = mem_vaddr[8] |
		(mem_vaddr[9] << 8) |
		(mem_vaddr[10] << 16) |
		(mem_vaddr[11] << 24);
	tcon_mm_table.data_load_level = mem_vaddr[12] |
		(mem_vaddr[13] << 8) |
		(mem_vaddr[14] << 16) |
		(mem_vaddr[15] << 24);
	tcon_mm_table.block_cnt = block_cnt;
	//tcon_mm_table.init_load = mem_vaddr[20];
	//LCDPR("%s: init_load: %d\n", __func__, tcon_mm_table.init_load);

	tcon_mm_table.bin_path_valid = 1;
#endif

	return 0;
}

static void lcd_tcon_axi_tbl_set_valid(unsigned int type, int valid)
{
	struct lcd_tcon_axi_mem_cfg_s *axi_cfg = NULL;
	int i = 0;

	if (lcd_tcon_conf->axi_tbl_len && lcd_tcon_conf->axi_mem_cfg_tbl) {
		for (i = 0; i < lcd_tcon_conf->axi_tbl_len; i++) {
			axi_cfg = &lcd_tcon_conf->axi_mem_cfg_tbl[i];
			if (type == axi_cfg->mem_type) {
				if (lcd_debug_print_flag) {
					LCDPR("%s [%d] type=%d, valid=%d\n", __func__,
						i, type, axi_cfg->mem_valid);
				}
				axi_cfg->mem_valid = valid;
			}
		}
	}
}

static int lcd_tcon_axi_tbl_check_valid(unsigned int type)
{
	struct lcd_tcon_axi_mem_cfg_s *axi_cfg = NULL;
	int i = 0;

	if (lcd_tcon_conf->axi_tbl_len && lcd_tcon_conf->axi_mem_cfg_tbl) {
		for (i = 0; i < lcd_tcon_conf->axi_tbl_len; i++) {
			axi_cfg = &lcd_tcon_conf->axi_mem_cfg_tbl[i];
			if (type == axi_cfg->mem_type)
				return axi_cfg->mem_valid;
		}
	} else {
		if (type == TCON_AXI_MEM_TYPE_OD)
			return (tcon_rmem.axi_bank > 0);
		else if (type == TCON_AXI_MEM_TYPE_DEMURA)
			return (tcon_rmem.axi_bank > 1);
	}

	return 0;
}

int lcd_tcon_mem_od_is_valid(void)
{
	return lcd_tcon_axi_tbl_check_valid(TCON_AXI_MEM_TYPE_OD);
}

int lcd_tcon_mem_demura_is_valid(void)
{
	return lcd_tcon_axi_tbl_check_valid(TCON_AXI_MEM_TYPE_DEMURA);
}

static int lcd_tcon_mm_table_config_v0(void)
{
	unsigned int max_size;

	/* reserved memory */
	max_size = lcd_tcon_conf->axi_size +
		lcd_tcon_conf->bin_path_size +
		lcd_tcon_conf->vac_size +
		lcd_tcon_conf->demura_set_size +
		lcd_tcon_conf->demura_lut_size +
		lcd_tcon_conf->acc_lut_size;
	if (tcon_rmem.rsv_mem_size < max_size) {
		LCDERR("%s: tcon mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.rsv_mem_size, max_size);
		return -1;
	}

	if (tcon_mm_table.block_cnt != 4) {
		LCDERR("%s: tcon data block_cnt %d invalid\n",
		       __func__, tcon_mm_table.block_cnt);
		return -1;
	}

	tcon_rmem.vac_rmem.mem_size = lcd_tcon_conf->vac_size;
	tcon_rmem.vac_rmem.mem_paddr =
		tcon_rmem.bin_path_rmem.mem_paddr +
		tcon_rmem.bin_path_rmem.mem_size;
	tcon_rmem.vac_rmem.mem_vaddr =
		(unsigned char *)(unsigned long)(tcon_rmem.vac_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.vac_rmem.mem_size > 0)
		LCDPR("tcon vac paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.vac_rmem.mem_paddr,
		      tcon_rmem.vac_rmem.mem_size);

	tcon_rmem.demura_set_rmem.mem_size = lcd_tcon_conf->demura_set_size;
	tcon_rmem.demura_set_rmem.mem_paddr =
		tcon_rmem.vac_rmem.mem_paddr + tcon_rmem.vac_rmem.mem_size;
	tcon_rmem.demura_set_rmem.mem_vaddr = (unsigned char *)
			(unsigned long)(tcon_rmem.demura_set_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.demura_set_rmem.mem_size > 0)
		LCDPR("tcon demura set_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.demura_set_rmem.mem_paddr,
		      tcon_rmem.demura_set_rmem.mem_size);

	tcon_rmem.demura_lut_rmem.mem_size = lcd_tcon_conf->demura_lut_size;
	tcon_rmem.demura_lut_rmem.mem_paddr =
		tcon_rmem.demura_set_rmem.mem_paddr +
		tcon_rmem.demura_set_rmem.mem_size;
	tcon_rmem.demura_lut_rmem.mem_vaddr = (unsigned char *)
		(unsigned long)(tcon_rmem.demura_lut_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.demura_lut_rmem.mem_size > 0)
		LCDPR("tcon demura lut_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.demura_lut_rmem.mem_paddr,
		      tcon_rmem.demura_lut_rmem.mem_size);

	tcon_rmem.acc_lut_rmem.mem_size = lcd_tcon_conf->acc_lut_size;
	tcon_rmem.acc_lut_rmem.mem_paddr =
		tcon_rmem.demura_lut_rmem.mem_paddr +
		tcon_rmem.demura_lut_rmem.mem_size;
	tcon_rmem.acc_lut_rmem.mem_vaddr = (unsigned char *)
		(unsigned long)(tcon_rmem.acc_lut_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.acc_lut_rmem.mem_size > 0)
		LCDPR("tcon acc lut_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.acc_lut_rmem.mem_paddr,
		      tcon_rmem.acc_lut_rmem.mem_size);

	return 0;
}

static int lcd_tcon_mm_table_config_v1(void)
{
	if (tcon_mm_table.block_cnt > 32) {
		LCDERR("%s: tcon data block_cnt %d invalid\n",
		       __func__, tcon_mm_table.block_cnt);
		return -1;
	}

	if (tcon_mm_table.data_mem_vaddr)
		return 0;
	if (tcon_mm_table.block_cnt == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: block_cnt is zero\n", __func__);
		return 0;
	}

	tcon_mm_table.data_mem_vaddr =
		(unsigned char **)malloc(tcon_mm_table.block_cnt * sizeof(unsigned char *));
	if (!tcon_mm_table.data_mem_vaddr) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(tcon_mm_table.data_mem_vaddr, 0,
	       tcon_mm_table.block_cnt * sizeof(unsigned char *));

	tcon_mm_table.data_size =
		(unsigned int *)malloc(tcon_mm_table.block_cnt * sizeof(unsigned int));
	if (!tcon_mm_table.data_size) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(tcon_mm_table.data_size, 0, tcon_mm_table.block_cnt * sizeof(unsigned int));

	return 0;
}

static void lcd_tcon_axi_mem_config_tl1(void)
{
	unsigned int size[3] = {4162560, 4162560, 1960440};
	unsigned int total_size = 0, temp_size = 0;
	int i;

	for (i = 0; i < tcon_rmem.axi_bank; i++)
		total_size += size[i];
	if (total_size > tcon_rmem.axi_mem_size) {
		LCDERR("%s: tcon axi_mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.axi_mem_size, total_size);
		return;
	}

	tcon_rmem.axi_rmem = (struct tcon_rmem_config_s *)
		malloc(tcon_rmem.axi_bank * sizeof(struct tcon_rmem_config_s));
	if (!tcon_rmem.axi_rmem)
		return;
	memset(tcon_rmem.axi_rmem, 0, tcon_rmem.axi_bank * sizeof(struct tcon_rmem_config_s));

	for (i = 0; i < tcon_rmem.axi_bank; i++) {
		tcon_rmem.axi_rmem[i].mem_paddr = tcon_rmem.axi_mem_paddr + temp_size;
		tcon_rmem.axi_rmem[i].mem_vaddr = (unsigned char *)
			(unsigned long)tcon_rmem.axi_rmem[i].mem_paddr;
		tcon_rmem.axi_rmem[i].mem_size = size[i];
		temp_size += size[i];
	}

	//tcon_rmem.secure_axi_rmem.mem_paddr = tcon_rmem.axi_rmem[0].mem_paddr;
	//tcon_rmem.secure_axi_rmem.mem_vaddr = tcon_rmem.axi_rmem[0].mem_vaddr;
	//tcon_rmem.secure_axi_rmem.mem_size = size[0];

}

#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
static int lcd_tcon_mem_tee_protect(int protect_en)
{
	int ret;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	unsigned char *secure_cfg_vaddr;
	unsigned int *data, sec_protect, sec_handle;

	if (!tcon_conf) {
		LCDERR("%s: tcon_conf is null\n", __func__);
		return -1;
	}
	if (tcon_rmem.flag == 0 || !tcon_rmem.axi_rmem) {
		LCDPR("%s: no axi_rmem\n", __func__);
		return 0;
	}
	if (tcon_rmem.secure_axi_rmem.mem_paddr == 0 ||
	    tcon_rmem.secure_axi_rmem.mem_size == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: no secure_axi_rmem\n", __func__);
		return 0;
	}

	secure_cfg_vaddr = tcon_rmem.secure_cfg_rmem.mem_vaddr;
	if (!secure_cfg_vaddr) {
		LCDERR("%s: secure_cfg_vaddr is null\n", __func__);
		return 0;
	}
	data = (unsigned int *)secure_cfg_vaddr;

	if (protect_en) {
		if (tcon_rmem.secure_axi_rmem.sec_protect)
			return 0;
		flush_dcache_range(tcon_rmem.secure_axi_rmem.mem_paddr,
		      tcon_rmem.secure_axi_rmem.mem_paddr + tcon_rmem.secure_axi_rmem.mem_size);
		ret = tee_protect_mem_by_type(TEE_MEM_TYPE_TCON,
					      tcon_rmem.secure_axi_rmem.mem_paddr,
					      tcon_rmem.secure_axi_rmem.mem_size,
					      &tcon_rmem.secure_axi_rmem.sec_handle);

		if (ret) {
			LCDERR("%s: protect failed! mem, start: 0x%08x, size: 0x%x, ret: %d\n",
			       __func__,
			       tcon_rmem.secure_axi_rmem.mem_paddr,
			       tcon_rmem.secure_axi_rmem.mem_size, ret);
			return -1;
		}
		tcon_rmem.secure_axi_rmem.sec_protect = 1;
		LCDPR("%s: protect OK. mem, start: 0x%08x, size: 0x%x\n",
			__func__,
			tcon_rmem.secure_axi_rmem.mem_paddr,
			tcon_rmem.secure_axi_rmem.mem_size);
	} else {
		if (!tcon_rmem.secure_axi_rmem.sec_protect)
			return 0;
		tee_unprotect_mem(tcon_rmem.secure_axi_rmem.sec_handle);
		tcon_rmem.secure_axi_rmem.sec_protect = 0;
		tcon_rmem.secure_axi_rmem.sec_handle = 0;
		LCDPR("%s: unprotect OK. mem, start: 0x%08x, size: 0x%x\n",
			__func__,
			tcon_rmem.secure_axi_rmem.mem_paddr,
			tcon_rmem.secure_axi_rmem.mem_size);
	}
	//update secure_cfg_vaddr
	data[0] = tcon_rmem.secure_axi_rmem.sec_protect;
	data[1] = tcon_rmem.secure_axi_rmem.sec_handle;
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		sec_protect = secure_cfg_vaddr[0] |
				(secure_cfg_vaddr[1] << 8) |
				(secure_cfg_vaddr[2] << 16) |
				(secure_cfg_vaddr[3] << 24);
		sec_handle = secure_cfg_vaddr[4] |
				(secure_cfg_vaddr[5] << 8) |
				(secure_cfg_vaddr[6] << 16) |
				(secure_cfg_vaddr[7] << 24);
		LCDPR("mem sec_handle: %d, secure_cfg_rmem protect: %d, handle: %d\n",
			tcon_rmem.secure_axi_rmem.sec_handle,
			sec_protect, sec_handle);
	}

	return 0;
}
#endif

static void lcd_tcon_axi_mem_config(void)
{
	struct lcd_tcon_axi_mem_cfg_s *axi_mem_cfg = NULL;
	unsigned int temp_size = 0;
	unsigned int mem_paddr = 0, od_mem_size = 0;
	unsigned char *mem_vaddr = NULL;
	int i;

	if (!lcd_tcon_conf->axi_tbl_len || !lcd_tcon_conf->axi_mem_cfg_tbl)
		return;

	temp_size = tcon_rmem.axi_bank * sizeof(struct tcon_rmem_config_s);
	tcon_rmem.axi_rmem = (struct tcon_rmem_config_s *)malloc(temp_size);
	if (!tcon_rmem.axi_rmem)
		return;
	memset(tcon_rmem.axi_rmem, 0, temp_size);

	temp_size = tcon_rmem.axi_bank * sizeof(unsigned int);
	tcon_rmem.axi_reg = (unsigned int *)malloc(temp_size);
	if (!tcon_rmem.axi_reg) {
		free(tcon_rmem.axi_rmem);
		return;
	}
	memset(tcon_rmem.axi_reg, 0, temp_size);

	temp_size = 0;
	for (i = 0; i < lcd_tcon_conf->axi_tbl_len; i++) {
		axi_mem_cfg = &lcd_tcon_conf->axi_mem_cfg_tbl[i];
		tcon_rmem.axi_reg[i] = axi_mem_cfg->axi_reg;
		if (!axi_mem_cfg->mem_valid)
			continue;
		tcon_rmem.axi_rmem[i].mem_paddr = tcon_rmem.axi_mem_paddr + temp_size;
		tcon_rmem.axi_rmem[i].mem_vaddr = (unsigned char *)
			(unsigned long)tcon_rmem.axi_rmem[i].mem_paddr;
		tcon_rmem.axi_rmem[i].mem_size = axi_mem_cfg->mem_size;
		temp_size += axi_mem_cfg->mem_size;

		if (axi_mem_cfg->mem_type == TCON_AXI_MEM_TYPE_OD) {
			if (!mem_paddr) {
				mem_paddr = tcon_rmem.axi_rmem[i].mem_paddr;
				mem_vaddr = tcon_rmem.axi_rmem[i].mem_vaddr;
			}
			od_mem_size += tcon_rmem.axi_rmem[i].mem_size;
		}
	}
	tcon_rmem.secure_axi_rmem.mem_paddr = mem_paddr;
	tcon_rmem.secure_axi_rmem.mem_vaddr = mem_vaddr;
	tcon_rmem.secure_axi_rmem.mem_size = od_mem_size;
}

static int lcd_tcon_mem_config(void)
{
	unsigned char *mem_vaddr;
	unsigned int mem_size = 0, mem_od_size = 0, mem_dmr_size = 0;
	unsigned int axi_mem_size = 0;
	int ret = -1, i = 0;
	struct lcd_tcon_axi_mem_cfg_s *axi_cfg = NULL;

	if (tcon_rmem.flag == 0)
		return -1;

	tcon_rmem.axi_bank = lcd_tcon_conf->axi_bank;

	/* check reserved memory */
	if (lcd_tcon_conf->axi_tbl_len && lcd_tcon_conf->axi_mem_cfg_tbl) {
		for (i = 0; i < lcd_tcon_conf->axi_tbl_len; i++) {
			axi_cfg = &lcd_tcon_conf->axi_mem_cfg_tbl[i];
			if (lcd_debug_print_flag) {
				LCDPR("axi mem type=%d, size=%#x, reg=%#x, valid=%d\n",
					axi_cfg->mem_type, axi_cfg->mem_size,
					axi_cfg->axi_reg, axi_cfg->mem_valid);
			}
			switch (axi_cfg->mem_type) {
			case TCON_AXI_MEM_TYPE_OD:
				mem_od_size += axi_cfg->mem_size;
				break;
			case TCON_AXI_MEM_TYPE_DEMURA:
				mem_dmr_size += axi_cfg->mem_size;
				break;
			default:
				LCDERR("Unsupport mem type=%d\n", axi_cfg->mem_type);
				break;
			}
		}

		/* check mem */
		axi_mem_size = mem_od_size + mem_dmr_size;
		mem_size = axi_mem_size + lcd_tcon_conf->bin_path_size +
			lcd_tcon_conf->secure_cfg_size;
		if (tcon_rmem.rsv_mem_size < mem_size) {
			axi_mem_size = mem_od_size;
			mem_size = axi_mem_size + lcd_tcon_conf->bin_path_size +
				lcd_tcon_conf->secure_cfg_size;
			if (tcon_rmem.rsv_mem_size < mem_size) {
				LCDERR("%s: tcon mem size 0x%x is not enough, need min 0x%x\n",
					__func__, tcon_rmem.rsv_mem_size, mem_size);
				return -1;
			}
			lcd_tcon_axi_tbl_set_valid(TCON_AXI_MEM_TYPE_OD, 1);
		} else {
			lcd_tcon_axi_tbl_set_valid(TCON_AXI_MEM_TYPE_OD, 1);
			lcd_tcon_axi_tbl_set_valid(TCON_AXI_MEM_TYPE_DEMURA, 1);
		}

		lcd_tcon_conf->axi_size = axi_mem_size;
	} else {
		mem_size = lcd_tcon_conf->axi_size + lcd_tcon_conf->bin_path_size
			+ lcd_tcon_conf->secure_cfg_size;
		if (tcon_rmem.rsv_mem_size < mem_size) {
			LCDERR("%s: tcon mem size 0x%x is not enough, need 0x%x\n",
				__func__, tcon_rmem.rsv_mem_size, mem_size);
			return -1;
		}
	}

	tcon_rmem.axi_mem_size = lcd_tcon_conf->axi_size;
	tcon_rmem.axi_mem_paddr = tcon_rmem.rsv_mem_paddr;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("tcon axi_mem paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.axi_mem_paddr, tcon_rmem.axi_mem_size);

	if (lcd_tcon_conf->tcon_axi_mem_config)
		lcd_tcon_conf->tcon_axi_mem_config();
	else
		lcd_tcon_axi_mem_config();

	tcon_rmem.bin_path_rmem.mem_size = lcd_tcon_conf->bin_path_size;
	tcon_rmem.bin_path_rmem.mem_paddr =
		tcon_rmem.axi_mem_paddr + tcon_rmem.axi_mem_size;
	/* don't set bin_path_rmem.mem_vaddr here */
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.bin_path_rmem.mem_size > 0)
		LCDPR("tcon bin_path paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.bin_path_rmem.mem_paddr,
		      tcon_rmem.bin_path_rmem.mem_size);

	tcon_rmem.secure_cfg_rmem.mem_size = lcd_tcon_conf->secure_cfg_size;
	tcon_rmem.secure_cfg_rmem.mem_paddr =
		tcon_rmem.axi_mem_paddr + tcon_rmem.axi_mem_size + lcd_tcon_conf->bin_path_size;
	if (tcon_rmem.secure_cfg_rmem.mem_size > 0) {
		tcon_rmem.secure_cfg_rmem.mem_vaddr =
			(unsigned char *)(unsigned long)(tcon_rmem.secure_cfg_rmem.mem_paddr);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("tcon secure_cfg_rmem paddr: 0x%08x, vaddr: 0x%p, size: 0x%x\n",
				tcon_rmem.secure_cfg_rmem.mem_paddr,
				tcon_rmem.secure_cfg_rmem.mem_vaddr,
				tcon_rmem.secure_cfg_rmem.mem_size);
		}
	} else {
		tcon_rmem.secure_cfg_rmem.mem_vaddr = NULL;
	}

	/* default clear tcon rmem */
	mem_vaddr = (unsigned char *)(unsigned long)(tcon_rmem.rsv_mem_paddr);
	memset(mem_vaddr, 0, tcon_rmem.rsv_mem_size);

	ret = lcd_tcon_bin_path_update(tcon_rmem.bin_path_rmem.mem_size);
	if (ret)
		return -1;

	/* allocated memory, memory map table config */
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("tcon mm_table version: %d\n", tcon_mm_table.version);
	if (tcon_mm_table.version == 0)
		ret = lcd_tcon_mm_table_config_v0();
	else
		ret = lcd_tcon_mm_table_config_v1();

	return ret;
}

static void lcd_tcon_reserved_memory_init_default(struct aml_lcd_drv_s *pdrv)
{
	tcon_rmem.rsv_mem_paddr = env_get_ulong("tcon_mem_addr", 16, 0);
	if (tcon_rmem.rsv_mem_paddr) {
		tcon_rmem.rsv_mem_size = lcd_tcon_conf->rsv_mem_size;
		LCDPR("get tcon rsv_mem addr 0x%x from env tcon_mem_addr\n",
			tcon_rmem.rsv_mem_paddr);
	} else {
		LCDERR("can't find env tcon_mem_addr\n");
	}
}

static int lcd_tcon_load_init_data_from_unifykey(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	int key_len, data_len, ret;

	data_len = tcon_mm_table.core_reg_table_size;
	ret = lcd_unifykey_get_size("lcd_tcon", &key_len);
	if (ret)
		return -1;
	if (key_len < data_len) {
		LCDERR("%s: key_len %d is not enough, need %d\n",
			__func__, key_len, data_len);
		return -1;
	}
	if (!tcon_mm_table.core_reg_table) {
		tcon_mm_table.core_reg_table = (unsigned char *)malloc
					(sizeof(unsigned char) * data_len);
		if (!tcon_mm_table.core_reg_table)
			return -1;
	}
	memset(tcon_mm_table.core_reg_table, 0, (sizeof(unsigned char) * data_len));
	ret = lcd_unifykey_get_no_header("lcd_tcon",
					 tcon_mm_table.core_reg_table,
					 key_len);
	if (ret)
		goto lcd_tcon_load_init_data_err;

	memset(tcon_local_cfg.bin_ver, 0, TCON_BIN_VER_LEN);
	if (tcon_conf && tcon_conf->tcon_init_table_pre_proc)
		tcon_conf->tcon_init_table_pre_proc(tcon_mm_table.core_reg_table);

	LCDPR("tcon: load init data len: %d\n", data_len);
	return 0;

lcd_tcon_load_init_data_err:
	free(tcon_mm_table.core_reg_table);
	tcon_mm_table.core_reg_table = NULL;
	LCDERR("%s: tcon unifykey load error!!!\n", __func__);
	return -1;
}

void lcd_tcon_init_data_version_update(char *data_buf)
{
	if (!data_buf)
		return;

	memcpy(tcon_local_cfg.bin_ver, data_buf, LCD_TCON_INIT_BIN_VERSION_SIZE);
	tcon_local_cfg.bin_ver[TCON_BIN_VER_LEN - 1] = '\0';
}

static int lcd_tcon_load_init_data_from_unifykey_new(struct aml_lcd_drv_s *pdrv)
{
	int key_len, data_len;
	unsigned char *buf, *p;
	struct lcd_tcon_init_block_header_s *data_header = NULL, *tmp_header;
	struct lcd_tcon_init_block_ext_header_s *data_ext_header = NULL;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	int ret;

	ret = lcd_unifykey_get_size("lcd_tcon", &key_len);
	if (ret)
		return -1;
	buf = (unsigned char *)calloc(1, key_len);
	if (!buf)
		return -1;

	ret = lcd_unifykey_get_tcon("lcd_tcon", buf, key_len);
	if (ret)
		goto lcd_tcon_load_init_data_new_err;

	tmp_header = (struct lcd_tcon_init_block_header_s *)buf;
	data_header = calloc(1, tmp_header->header_size);
	if (!data_header)
		goto lcd_tcon_load_init_data_new_err;
	memcpy(data_header, tmp_header, tmp_header->header_size);
	data_len = tcon_mm_table.core_reg_table_size + data_header->header_size
			+ data_header->ext_header_size;
	if (key_len < data_len || data_header->block_size < data_len) {
		LCDERR("%s: key_len(%d) or data block_size(%d) are not enough, need %d\n",
			__func__, key_len, data_header->block_size, data_len);
		goto lcd_tcon_load_init_data_new_err;
	}
	if (data_header->ext_header_size > 0) {
		data_ext_header = (struct lcd_tcon_init_block_ext_header_s *)
			calloc(1, data_header->ext_header_size);
		if (!data_ext_header)
			goto lcd_tcon_load_init_data_new_err;
		memcpy(data_ext_header, buf + data_header->header_size, sizeof(*data_ext_header));
		tcon_mm_table.core_reg_ext_header = data_ext_header;
	}
	tcon_mm_table.core_reg_header = data_header;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("unifykey header:\n");
		LCDPR("crc32             = 0x%08x\n", data_header->crc32);
		LCDPR("block_size        = %d\n", data_header->block_size);
		LCDPR("chipid            = %d\n", data_header->chipid);
		LCDPR("resolution        = %dx%d\n",
			data_header->h_active, data_header->v_active);
		LCDPR("block_ctrl        = 0x%x\n", data_header->block_ctrl);
		LCDPR("name              = %s\n", data_header->name);
		if (data_ext_header) {
			LCDPR("unifykey extern header:\n");
			LCDPR("framerate_range   = %d~%d\n", data_ext_header->framerate_min,
				data_ext_header->framerate_max);
		}
	}
	lcd_tcon_init_data_version_update(data_header->version);

	data_len = tcon_mm_table.core_reg_table_size;
	if (!tcon_mm_table.core_reg_table) {
		tcon_mm_table.core_reg_table = (unsigned char *)calloc(1, data_len);
		if (!tcon_mm_table.core_reg_table)
			goto lcd_tcon_load_init_data_new_err;
	}
	p = buf + data_header->header_size + data_header->ext_header_size;
	memcpy(tcon_mm_table.core_reg_table, p, data_len);
	if (tcon_conf && tcon_conf->tcon_init_table_pre_proc)
		tcon_conf->tcon_init_table_pre_proc(tcon_mm_table.core_reg_table);
	free(buf);

	tcon_local_cfg.cur_core_reg_table = tcon_mm_table.core_reg_table;
	lcd_tcon_init_setting_check(pdrv, &pdrv->config.timing.dft_timing,
			tcon_mm_table.core_reg_table);

	LCDPR("tcon: load init data len: %d, ver: %s\n",
	      data_len, tcon_local_cfg.bin_ver);
	return 0;

lcd_tcon_load_init_data_new_err:
	if (buf)
		free(buf);
	if (data_header)
		free(data_header);
	if (data_ext_header)
		free(data_ext_header);
	LCDERR("%s: tcon unifykey load error!!!\n", __func__);
	return -1;
}

static int lcd_tcon_reserved_memory_init_dts(char *dt_addr, struct aml_lcd_drv_s *pdrv)
{
	int parent_offset, cell_size;
	char *propdata;
	phys_addr_t paddr = 0;
	u32 size = 0;
	int ret = 0;
#ifdef CONFIG_AMLOGIC_TEE
	u32 align = 0x10000;
#else
	u32 align = PAGE_SIZE;
#endif

	tcon_rmem.use_lrm = 0;
	parent_offset = fdt_path_offset(dt_addr, "/reserved-memory");
	if (parent_offset < 0) {
		LCDERR("can't find node: /reserved-memory\n");
		goto tcon_rsvd_try_alloc_from_lrm;
	}
	cell_size = fdt_address_cells(dt_addr, parent_offset);
	parent_offset = fdt_path_offset(dt_addr, "/reserved-memory/linux,lcd_tcon");
	if (parent_offset < 0) {
		LCDERR("can't find node: /reserved-memory/linux,lcd_tcon\n");
		goto tcon_rsvd_try_alloc_from_lrm;
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "alloc-ranges", NULL);
	if (propdata) {
		if (cell_size == 2)
			tcon_rmem.rsv_mem_paddr = be32_to_cpup((((u32 *)propdata) + 1));
		else
			tcon_rmem.rsv_mem_paddr = be32_to_cpup(((u32 *)propdata));

		propdata = (char *)fdt_getprop(dt_addr, parent_offset, "size", NULL);
		if (!propdata) {
			LCDERR("failed to get tcon rsv_mem size from dts\n");
			goto tcon_rsvd_try_alloc_from_lrm;
		}
		if (cell_size == 2)
			tcon_rmem.rsv_mem_size = be32_to_cpup((((u32 *)propdata) + 1));
		else
			tcon_rmem.rsv_mem_size = be32_to_cpup(((u32 *)propdata));
	} else {
		propdata = (char *)fdt_getprop(dt_addr, parent_offset, "reg", NULL);
		if (!propdata) {
			LCDERR("failed to get lcd_tcon reserved-memory from dts\n");
			goto tcon_rsvd_try_alloc_from_lrm;
		}
		if (cell_size == 2) {
			tcon_rmem.rsv_mem_paddr = be32_to_cpup((((u32 *)propdata) + 1));
			tcon_rmem.rsv_mem_size = be32_to_cpup((((u32 *)propdata) + 3));
		} else {
			tcon_rmem.rsv_mem_paddr = be32_to_cpup(((u32 *)propdata));
			tcon_rmem.rsv_mem_size = be32_to_cpup((((u32 *)propdata) + 1));
		}
	}

tcon_rsvd_try_alloc_from_lrm:
	if (!tcon_rmem.rsv_mem_paddr || !tcon_rmem.rsv_mem_size) {
		ret = -1;
		size = lrm_get_tcon_rsvd_size();
		// tee protect require addr and size must be 0x10000 aligned
		paddr = lrm_phys_alloc_align(size, align, "lcd_tcon_rsvd");
		if (paddr && size) {
			tcon_rmem.rsv_mem_paddr = paddr;
			tcon_rmem.rsv_mem_size = size;
			tcon_rmem.use_lrm = 1;
			ret = 0;
		}
	}

	return ret;
}

static int lcd_tcon_get_config(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int ret;

	if (load_id & 0x1) {
		ret = lcd_tcon_reserved_memory_init_dts(dt_addr, pdrv);
		if (ret)
			lcd_tcon_reserved_memory_init_default(pdrv);
	} else {
		lcd_tcon_reserved_memory_init_default(pdrv);
	}

	if (tcon_rmem.rsv_mem_paddr) {
		tcon_rmem.flag = 1;
		LCDPR("tcon: rsv_mem addr:0x%x, size:0x%x\n",
		      tcon_rmem.rsv_mem_paddr, tcon_rmem.rsv_mem_size);
		lcd_tcon_mem_config();
	}

	tcon_mm_table.core_reg_table_size = lcd_tcon_conf->reg_table_len;
	if (lcd_tcon_conf->core_reg_ver)
		lcd_tcon_load_init_data_from_unifykey_new(pdrv);
	else
		lcd_tcon_load_init_data_from_unifykey(pdrv);

	lcd_tcon_reserved_mem_data_load(pdrv);

#ifdef CONFIG_CMD_INI
	lcd_tcon_bin_path_resv_mem_set();
#endif
#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
	lcd_tcon_mem_tee_protect(1);
#endif
	return 0;
}

static int lcd_tcon_core_flag(enum lcd_chip_e chip_type)
{
	int ret = 0;

	switch (chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
		ret = (readl(TCON_CORE_FLAG_LIC2) >> 17) & 0x1;
		break;
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		if (ret)
			LCDPR("%s: tcon invalid\n", __func__);
	}

	return ret;
}

/* **********************************
 * tcon match data
 * **********************************
 */
static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_t5[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00800000, 0x261, 0 },  /* 8M */
	{ TCON_AXI_MEM_TYPE_DEMURA, 0x00100000, 0x1a9, 0 },  /* 1M */
};

static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_t5d[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00400000, 0x261, 0 },  /* 4M */
};

static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_t3x[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00400000, 0x261, 0 },  /* 4M */
	{ TCON_AXI_MEM_TYPE_OD,     0x00400000, 0x26d, 0 },  /* 4M */
	{ TCON_AXI_MEM_TYPE_DEMURA, 0x00100000, 0x1a9, 0 },  /* 1M */
};

static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_txhd2[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00400000, 0x261, 0 },  /* 4M */
	{ TCON_AXI_MEM_TYPE_DEMURA, 0x00100000, 0x19b, 0 },  /* 1M */
};

static struct lcd_tcon_config_s tcon_data_tl1 = {
	.tcon_valid = 0,

	.core_reg_ver = 0,
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_TL1,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_TL1,
	.reg_table_len = LCD_TCON_TABLE_LEN_TL1,
	.core_reg_start = TCON_CORE_REG_START_TL1,

	.reg_top_ctrl = TCON_TOP_CTRL,
	.bit_en = BIT_TOP_EN_TL1,

	.reg_core_od = REG_CORE_OD_TL1,
	.bit_od_en = BIT_OD_EN_TL1,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_TL1,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_TL1,

	.axi_bank = LCD_TCON_AXI_BANK_TL1,

	.rsv_mem_size    = 0x00c00000, /* 12M */
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0x00002000, /* 8K */
	.demura_set_size = 0x00001000, /* 4K */
	.demura_lut_size = 0x00120000, /* 1152K */
	.acc_lut_size    = 0x00001000, /* 4K */

	.tcon_axi_mem_config = lcd_tcon_axi_mem_config_tl1,
	.tcon_init_table_pre_proc = NULL,
	.tcon_global_reset = NULL,
	.tcon_top_init = lcd_tcon_top_set_tl1,
	.tcon_enable = lcd_tcon_enable_tl1,
	.tcon_disable = lcd_tcon_disable_tl1,
	.tcon_forbidden_check = NULL,
	.tcon_check = NULL,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_t5 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5,
	.tcon_check = lcd_tcon_setting_check_t5,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_t5d = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5D,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5D,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5D,
	.core_reg_start = TCON_CORE_REG_START_T5D,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5D,

	.reg_core_od = REG_CORE_OD_T5D,
	.bit_od_en = BIT_OD_EN_T5D,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5D,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5D,

	.axi_bank = LCD_TCON_AXI_BANK_T5D,

	.rsv_mem_size    = 0x00402840,
	.axi_size        = 0x00400000, /* 4M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5d),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5d,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5d,
	.tcon_check = lcd_tcon_setting_check_t5d,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_t3 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t3,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5,
	.tcon_check = lcd_tcon_setting_check_t5,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_t5m = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t3,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5,
	.tcon_check = lcd_tcon_setting_check_t5,
	.lut_dma_data_init_trans = lcd_tcon_dma_data_init_trans,
	.lut_dma_mif_set = lcd_tcon_lut_dma_mif_set_t5m,
	.lut_dma_enable = lcd_tcon_lut_dma_enable_t5m,
	.lut_dma_disable = lcd_tcon_lut_dma_disable_t5m,
};

static struct lcd_tcon_config_s tcon_data_t5w = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t3,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5,
	.tcon_check = lcd_tcon_setting_check_t5,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_t3x = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T3X,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T3X,

	.rsv_mem_size    = 0x00a02840, /* 10M10k */
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t3x),
	.axi_mem_cfg_tbl = (axi_mem_cfg_tbl_t3x),

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t3x,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5,
	.tcon_check = lcd_tcon_setting_check_t5,
	.lut_dma_data_init_trans = NULL,
};

static struct lcd_tcon_config_s tcon_data_txhd2 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5D,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5D,
	.reg_table_len = LCD_TCON_TABLE_LEN_TXHD2,
	.core_reg_start = TCON_CORE_REG_START_T5D,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5D,

	.reg_core_od = REG_CORE_OD_T5D,
	.bit_od_en = BIT_OD_EN_T5D,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5D,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5D,

	.axi_bank = LCD_TCON_AXI_BANK_TXHD2,

	.rsv_mem_size    = 0x00502840,
	.axi_size        = 0x00500000, /* 5M*/
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_txhd2),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_txhd2,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5,
	.tcon_top_init = lcd_tcon_top_set_t5,
	.tcon_enable = lcd_tcon_enable_txhd2,
	.tcon_disable = lcd_tcon_disable_t5,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_t5d,
	.tcon_check = lcd_tcon_setting_check_t5d,
	.lut_dma_data_init_trans = NULL,
};

int lcd_tcon_probe(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int ret = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	lcd_tcon_conf = NULL;
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
		if (lcd_tcon_core_flag(pdrv->data->chip_type) == 0)
			lcd_tcon_conf = &tcon_data_tl1;
		break;
	case LCD_CHIP_T5:
		lcd_tcon_conf = &tcon_data_t5;
		break;
	case LCD_CHIP_T5D:
		lcd_tcon_conf = &tcon_data_t5d;
		break;
	case LCD_CHIP_T3:
		lcd_tcon_conf = &tcon_data_t3;
		break;
	case LCD_CHIP_T5M:
		lcd_tcon_conf = &tcon_data_t5m;
		break;
	case LCD_CHIP_T5W:
		lcd_tcon_conf = &tcon_data_t5w;
		break;
	case LCD_CHIP_T3X:
		lcd_tcon_conf = &tcon_data_t3x;
		break;
	case LCD_CHIP_TXHD2:
		lcd_tcon_conf = &tcon_data_txhd2;
		break;
	default:
		break;
	}
	if (!lcd_tcon_conf)
		return 0;

	lcd_tcon_conf->tcon_valid = 0;
	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		lcd_tcon_conf->tcon_valid = 1;
		break;
	case LCD_P2P:
		if (pdrv->data->chip_type == LCD_CHIP_T5D ||
		    pdrv->data->chip_type == LCD_CHIP_TXHD2)
			lcd_tcon_conf->tcon_valid = 0;
		else
			lcd_tcon_conf->tcon_valid = 1;
		break;
	default:
		break;
	}
	if (lcd_tcon_conf->tcon_valid == 0)
		return 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s\n", __func__);

	memset(&tcon_rmem, 0, sizeof(struct tcon_rmem_s));
	memset(&tcon_mm_table, 0, sizeof(struct tcon_mem_map_table_s));
	/*must before tcon_config, for memory alloc*/
	lcd_tcon_spi_data_probe(pdrv);
	ret = lcd_tcon_get_config(dt_addr, pdrv, load_id);

#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
	pdrv->tcon_mem_tee_protect = lcd_tcon_mem_tee_protect;
#endif
	pdrv->tcon_forbidden_check = lcd_tcon_forbidden_check;

	lcd_tcon_debug_probe(pdrv);

	return ret;
}

