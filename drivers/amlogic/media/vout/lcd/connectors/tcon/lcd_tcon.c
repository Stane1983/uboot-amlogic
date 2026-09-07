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
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "../lcd_connector.h"
#include "lcd_tcon.h"
#include "lcd_tcon_init.h"
#include "lcd_tcon_dccd.h"
#include "env.h"

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
static unsigned char *g_tcon_bin_path, *g_tcon_bin_path_k;

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

static unsigned char *init_tcon_bin_path_mem(void)
{
	struct lcd_tcon_bin_path_header_s *header;

	if (!g_tcon_bin_path) {
		g_tcon_bin_path = (unsigned char *)malloc(TCON_BIN_PATH_MAX_SIZE);
		if (!g_tcon_bin_path) {
			LCDERR("%s: malloc memory error!\n", __func__);
			return NULL;
		}
	}
	memset(g_tcon_bin_path, 0, TCON_BIN_PATH_MAX_SIZE);

	header = (struct lcd_tcon_bin_path_header_s *)g_tcon_bin_path;
	header->mem_total_size = TCON_BIN_PATH_MAX_SIZE;

	return g_tcon_bin_path;
}

static unsigned char *init_tcon_bin_path_k_mem(void)
{
	struct lcd_tcon_bin_path_header_s *header;

	if (!g_tcon_bin_path_k) {
		g_tcon_bin_path_k = (unsigned char *)malloc(TCON_BIN_PATH_MAX_SIZE);
		if (!g_tcon_bin_path_k) {
			LCDERR("%s: malloc memory error!\n", __func__);
			return NULL;
		}
	}
	memset(g_tcon_bin_path_k, 0, TCON_BIN_PATH_MAX_SIZE);

	header = (struct lcd_tcon_bin_path_header_s *)g_tcon_bin_path_k;
	header->mem_total_size = TCON_BIN_PATH_MAX_SIZE;

	return g_tcon_bin_path_k;
}

/* for uboot use */
unsigned char *get_tcon_bin_path_mem(void)
{
	if (!g_tcon_bin_path) {
		g_tcon_bin_path = init_tcon_bin_path_mem();
		if (!g_tcon_bin_path)
			return NULL;
	}

	return g_tcon_bin_path;
}

/* save to reserved memory for kernel */
unsigned char *get_tcon_bin_path_k_mem(void)
{
	if (!g_tcon_bin_path_k) {
		g_tcon_bin_path_k = init_tcon_bin_path_k_mem();
		if (!g_tcon_bin_path_k)
			return NULL;
	}

	return g_tcon_bin_path_k;
}

static int get_tcon_data_bin_by_id(unsigned char **buf, int id)
{
	unsigned char *bin_buf;
	char *path;
	int offset, bin_size = 0;
	unsigned int data_size, raw_crc32, temp_crc32;

	if (!buf)
		return -1;
	if (buf[id]) //for reload
		free(buf[id]);

	offset = 32 + 256 * id + 4;
	if (!g_tcon_bin_path || offset > TCON_BIN_PATH_MAX_SIZE - 256)
		return -1;

	path = (char *)(g_tcon_bin_path + offset);
	bin_buf = model_read_file_to_buffer((const char *)path, &bin_size);
	if (!bin_buf || bin_size <= 0) {
		LCDPR("%s: read %s error, size:%d\n", __func__, path, bin_size);
		return -1;
	}

	data_size = bin_buf[8] |
		(bin_buf[9] << 8) |
		(bin_buf[10] << 16) |
		(bin_buf[11] << 24);
	if (data_size > bin_size) {
		memset(bin_buf, 0, bin_size);
		free(bin_buf);
		return -1;
	}
	raw_crc32 = bin_buf[0] |
		(bin_buf[1] << 8) |
		(bin_buf[2] << 16) |
		(bin_buf[3] << 24);
	temp_crc32 = cal_CRC32(0, &bin_buf[4], (data_size - 4));
	if (raw_crc32 != temp_crc32) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDERR("%s: bin[%d] crc32 check error (raw=0x%08x, cal=0x%02x)\n",
			       __func__, id, raw_crc32, temp_crc32);
		} else {
			LCDERR("%s: bin[%d] crc32 check error\n", __func__, id);
		}
		memset(bin_buf, 0, bin_size);
		free(bin_buf);
		return -1;
	}

	buf[id] = bin_buf;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: bin[%d] finish, bin_size = %d\n", __func__, id, bin_size);

	return 0;
}

static int lcd_tcon_bin_path_resv_mem_set(void)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *buf, *mem_vaddr;
	unsigned int block_size, temp_crc, n, i;

	if (tcon_rmem.flag == 0)
		return 0;

	buf = get_tcon_bin_path_k_mem();
	if (!buf) {
		LCDERR("%s: bin_path buf invalid\n", __func__);
		return -1;
	}
	header = (struct lcd_tcon_bin_path_header_s *)buf;
	if (header->data_size > tcon_rmem.bin_path_rmem.mem_size ||
	    header->data_size < sizeof(struct lcd_tcon_bin_path_header_s)) {
		LCDERR("%s: bin_path data_size error\n", __func__);
		return -1;
	}
	if (header->block_cnt > 32) {
		LCDERR("%s: bin_path block_cnt error\n", __func__);
		return -1;
	}
	temp_crc = cal_CRC32(0, &buf[4], (header->data_size - 4));
	if (header->crc32 != temp_crc) {
		LCDERR("%s: bin_path data crc error\n", __func__);
		return -1;
	}

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
		header->crc32 = cal_CRC32(0, &buf[4], (header->data_size - 4));
	}

	mem_vaddr = (unsigned char *)(unsigned long)(tcon_rmem.bin_path_rmem.mem_paddr);
	memcpy(mem_vaddr, buf, header->data_size);

	return 0;
}

/* **********************************
 * tcon function api
 * **********************************
 */
//ret:  bit[0]:tcon: fatal error, block driver
//      bit[1]:tcon: warning, only print warning message
int lcd_tcon_init_setting_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
		struct tcon_core_reg_info_s *core_reg_info)
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
		ret = lcd_tcon_conf->tcon_check(pdrv, ptiming, core_reg_info, ferr_str, warn_str);
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

	ret = lcd_tcon_init_setting_check(pdrv, ptiming, &tcon_local_cfg.cur_core_info);
	if (ret == 0)
		printf("lcd tcon setting check: PASS\n");
}

int lcd_tcon_data_multi_init_check(struct aml_lcd_drv_s *pdrv,
				   struct lcd_tcon_data_part_ctrl_s *ctrl_part,
				   unsigned char *p)
{
#ifdef CONFIG_AML_LCD_BACKLIGHT
	struct aml_bl_drv_s *bldrv;
	struct bl_pwm_config_s *bl_pwm = NULL;
	unsigned int data = 0;
#endif
	unsigned int data_byte, data_cnt, min = 0, max = 0, hsize = 0, vsize = 0;
	unsigned int temp, j, k = 0, frame_rate;

	if (!ctrl_part)
		return -1;

	data_byte = ctrl_part->data_byte_width;
	data_cnt = ctrl_part->data_cnt;

	switch (ctrl_part->ctrl_method) {
	case LCD_TCON_DATA_CTRL_MULTI_VFREQ:
		if (data_cnt != 2)
			goto lcd_tcon_data_multi_init_check_err_data_cnt;
		temp = pdrv->config.timing.act_timing.frame_rate;

		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_init_check_exit;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: vfreq %d-%d hit: %d\n", __func__, min, max, temp);
		break;
	case LCD_TCON_DATA_CTRL_MULTI_RESOLUTION:
		if (data_cnt < 2)
			goto lcd_tcon_data_multi_init_check_err_data_cnt;

		for (j = 0; j < data_byte; j++)
			hsize |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			vsize |= (p[k + j] << (j * 8));

		if (pdrv->config.timing.base_timing->h_active != hsize ||
		    pdrv->config.timing.base_timing->v_active != vsize)
			goto lcd_tcon_data_multi_init_check_exit;

		if (data_cnt == 2) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("%s: resolution %dx%d hit\n", __func__, hsize, vsize);
			break;
		}
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		if (data_cnt == 3) { //display mode (base bas_timing) eg:3840x2160p120hz
			if (min != pdrv->config.timing.base_timing->frame_rate)
				goto lcd_tcon_data_multi_init_check_exit;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("%s: disp_mode %dx%dp%dhz hit\n",
					__func__, hsize, vsize, min);
			break;
		}

		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));

		frame_rate = pdrv->config.timing.base_timing->frame_rate;
		if (frame_rate < min || frame_rate > max)
			goto lcd_tcon_data_multi_init_check_exit;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: disp_mode %dx%dp[%d~%d]hz hit\n",
					__func__, hsize, vsize, min, max);

		break;
	case LCD_TCON_DATA_CTRL_MULTI_BL_LEVEL:
#ifdef CONFIG_AML_LCD_BACKLIGHT
		bldrv = aml_bl_get_driver(pdrv->index);
		if (!bldrv)
			goto lcd_tcon_data_multi_init_check_err_type;
		temp = bldrv->level;

		if (data_cnt != 2)
			goto lcd_tcon_data_multi_init_check_err_data_cnt;
		for (j = 0; j < data_byte; j++)
			min |= (p[k + j] << (j * 8));
		k += data_byte;
		for (j = 0; j < data_byte; j++)
			max |= (p[k + j] << (j * 8));
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_init_check_exit;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: bl_level %d-%d hit: %d\n", __func__, min, max, temp);
#endif
		break;
	case LCD_TCON_DATA_CTRL_MULTI_BL_PWM_DUTY:
#ifdef CONFIG_AML_LCD_BACKLIGHT
		bldrv = aml_bl_get_driver(pdrv->index);
		if (!bldrv)
			goto lcd_tcon_data_multi_init_check_err_type;

		if (data_cnt != 3)
			goto lcd_tcon_data_multi_init_check_err_data_cnt;
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
		case BL_CTRL_PWM_ARRAY:
			if (data < 4)
				bl_pwm = bldrv->config.bl_pwm_array[data];
			break;
		default:
			break;
		}
		if (!bl_pwm)
			goto lcd_tcon_data_multi_init_check_err_type;

		temp = bl_pwm->pwm_duty;
		if (temp < min || temp > max)
			goto lcd_tcon_data_multi_init_check_exit;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: bl_pwm[%d] duty %d-%d hit: %d\n",
			      __func__, bl_pwm->index, min, max, temp);
		}
#endif
		break;
	case LCD_TCON_DATA_CTRL_DEFAULT:
		return 1;
	default:
		return -1;
	}

	return 0;

lcd_tcon_data_multi_init_check_exit:
	return -1;

lcd_tcon_data_multi_init_check_err_data_cnt:
	LCDERR("%s: ctrl_part %s data_cnt error\n", __func__, ctrl_part->name);
	return -1;

#ifdef CONFIG_AML_LCD_BACKLIGHT
lcd_tcon_data_multi_init_check_err_type:
	LCDERR("%s: ctrl_part %s type invalid\n", __func__, ctrl_part->name);
	return -1;
#endif
}

/* **********************************
 * tcon config
 * **********************************
 */
static void lcd_tcon_data_complete_check(struct aml_lcd_drv_s *pdrv, int index)
{
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
}

static void lcd_tcon_init_table_pre_proc(struct tcon_core_reg_info_s *core_reg_info)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	unsigned int reg = 0, val, bit, len, paddr, i = 0;

	if (!core_reg_info || !tcon_rmem)
		return;

	if (tcon_conf && tcon_conf->init_pre_setting) {
		while (1) {
			reg = tcon_conf->init_pre_setting[i].reg;
			if (reg == REG_LCD_TCON_MAX)
				break;
			val = tcon_conf->init_pre_setting[i].val;
			bit = tcon_conf->init_pre_setting[i].bit;
			len = tcon_conf->init_pre_setting[i].len;
			lcd_tcon_setb_table32_reg(core_reg_info, reg, val, bit, len);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: reg: 0x%08x[%d:%d] = 0x%x\n",
				      __func__, reg, (bit + len - 1), bit, val);
			}
			i++;
		}
	}

	//update axi paddr
	if (tcon_rmem->flag == 0 || !tcon_rmem->axi_rmem || !tcon_rmem->axi_reg) {
		LCDPR("%s: invalid axi_rmem\n", __func__);
		return;
	}
	for (i = 0; i < tcon_rmem->axi_bank; i++) {
		reg = tcon_rmem->axi_reg[i];
		paddr = tcon_rmem->axi_rmem[i].mem_paddr;
		lcd_tcon_set_table32_reg(core_reg_info, reg, paddr);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: axi[%d] reg: 0x%08x, paddr: 0x%08x\n",
			      __func__, i, reg, paddr);
		}
	}
}

static void lcd_tcon_init_table_post_proc(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	unsigned int reg = 0, val, bit, len, i = 0;

	if (tcon_conf && tcon_conf->init_post_setting) {
		while (1) {
			reg = tcon_conf->init_post_setting[i].reg;
			if (reg == REG_LCD_TCON_MAX)
				break;
			val = tcon_conf->init_post_setting[i].val;
			bit = tcon_conf->init_post_setting[i].bit;
			len = tcon_conf->init_post_setting[i].len;
			lcd_tcon_setb(reg, val, bit, len);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: reg: 0x%08x[%d:%d] = 0x%x\n",
				      __func__, reg, (bit + len - 1), bit, val);
			}
			i++;
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
	new_crc32 = cal_CRC32(0, data + 4, header->block_size - 4);
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
	struct tcon_core_reg_info_s core_reg_info;
	struct lcd_tcon_init_block_ext_header_s ext_header;

	if (!data_buf) {
		LCDERR("%s: data_buf is null\n", __func__);
		return -1;
	}
	if (!tcon_mm_table.data_size) {
		LCDERR("%s: data_size buf error\n", __func__);
		return -1;
	}

	memset(&core_reg_info, 0, sizeof(core_reg_info));
	memset(&ext_header, 0, sizeof(ext_header));

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
		if (init_header->ext_header_size) {
			memcpy(&ext_header, data_buf + init_header->header_size,
				sizeof(ext_header));
			if (ext_header.reg_blk_num) {
				ext_header.reg_blk_info = (struct lcd_tcon_reg_block_info_s *)
					(data_buf + TCON_EXT_BLK_INFO_PRE_OFFSET);
			}
			lcd_tcon_ext_header_check(&ext_header);
		}
		core_reg_info.header = init_header;
		core_reg_info.ext_header = ext_header;
		core_reg_info.table = data_buf + block_header->header_size
				+ block_header->ext_header_size;

		if (tcon_conf->tcon_init_table_pre_proc)
			tcon_conf->tcon_init_table_pre_proc(&core_reg_info);

		lcd_tcon_data_block_regen_crc(data_buf);

		match_timing.h_active = init_header->h_active;
		match_timing.v_active = init_header->v_active;
		lcd_tcon_init_setting_check(pdrv, &match_timing, &core_reg_info);
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

#if defined(CONFIG_AML_LCD_JSON) || defined(CONFIG_CMD_AML_MODEL)
static void lcd_tcon_data_path_head_add(unsigned char *p, u32 cnt)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned int temp;

	header = (struct lcd_tcon_bin_path_header_s *)p;
	header->version = 1;
	header->data_load_level = 1;
	header->init_load = 1;
	header->ready = 1;
	header->block_cnt = cnt;

	temp = 32 + cnt * 256;
	header->data_size = temp;

	temp = cal_CRC32(0, &p[4], (header->data_size - 4));
	header->crc32 = temp;
}
#endif

#ifdef CONFIG_AML_LCD_JSON
static int lcd_panel_parse_tcon_from_json(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_init_block_header_s *header;
	unsigned char *bin_path, *bin_path_k;
	const char *str, *dir_uboot, *dir_kernel;
	unsigned char *data = NULL;
	char path[TCON_BIN_PATH_LEN], *p0, *p1;
	int ret = 0, size, cnt = 0, i, index;
	unsigned int temp_crc32;
	int tcon_bin_cnt = 0, tcon_bin_cnt_rsvd = 0;
	__maybe_unused struct json_s *parent, *child;
	struct json_parse_s *jsp;
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_ip27_s *ip27;

	index = pdrv->index;
	jsp = get_panel_jsp(index);
	if (!json_parse_ok(jsp)) {
		LCDPR("%s: lcd%d json not ready\n", __func__, index);
		return -1;
	}

	LCDPR("tcon parse from json\n");

	parent = json_get_object_child(jsp, jsp->root, "tcon");
	if (!parent) {
		LCDPR("can't find /tcon\n");
		return 0;
	}

	/*ip27*/
	child = json_get_object_child(jsp, parent, "csot_ip27");
	if (child) {
		ip27 = &local_cfg->ip27;
		ip27->cfg_valid = json_get_obj_u32(jsp, child, "valid", 0);
		ip27->trig_mode = json_get_obj_u32(jsp, child, "trig_mode", 0);
		ip27->trig_line = json_get_obj_u32(jsp, child, "trig_line", 0);
		ip27->i2c_addr  = json_get_obj_u32(jsp, child, "i2c_addr", 0x33);
		ip27->i2c_speed = json_get_obj_u32(jsp, child, "i2c_speed", 400000);//400k
		//ip27->i2c_bytes = json_get_obj_u32(jsp, child, "i2c_byte", 4);//3byte mode
		ip27->debug     = json_get_obj_u32(jsp, child, "debug", 0);//
		ip27->data_mode = json_get_obj_u32(jsp, child, "data_mode", 0);//
		ip27->vcom_order = json_get_obj_u32(jsp, child, "vcom_order", 0);//
		ip27->i2c_reg_offset = json_get_obj_u32(jsp, child, "i2c_reg_offset", 0x25);//
		ip27->trig_line_dbg = ip27->trig_line;
		ip27->i2c_bytes = ip27->data_mode == 0 ? 24 + 1 : 3 + 1;
	}
	dir_uboot = json_get_obj_str(jsp, parent, "panel_dir_uboot", NULL);
	if (!dir_uboot) {
		LCDPR("can't find /data/panel_dir_uboot\n");
		return 0;
	}

	dir_kernel = json_get_obj_str(jsp, parent, "panel_dir_kernel", NULL);
	if (!dir_kernel)
		dir_kernel = dir_uboot;

	str = json_get_obj_str(jsp, parent, "tcon_reg_path", NULL);
	if (!str) {
		LCDERR("can't find /data/tcon_reg_path\n");
		return -1;
	}

	ret = path_name_compose(dir_uboot, str, path);
	if (ret) {
		LCDPR("tcon_reg_path not right\n");
		return -1;
	}
	data = model_read_file_to_buffer((const char *)path, &size);
	if (!data || size <= 0) {
		LCDERR("can't open file:%s\n", path);
		return -1;
	}
	header = (struct lcd_tcon_init_block_header_s *)data;
	temp_crc32 = cal_CRC32(0, &data[4], (header->block_size - 4));
	if (temp_crc32 != header->crc32) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDERR("%s: tcon_bin crc32 check error (raw=0x%08x, cal=0x%02x)\n",
			       __func__, header->crc32, temp_crc32);
		} else {
			LCDERR("%s: tcon_bin crc32 check error\n", __func__);
		}
		return -1;
	}
	if (lcd_debug_print_flag)
		LCDPR("tcon_reg size:%d path:%s\n", size, path);
	panel_param_mem_put(data, "tcon_core_reg", header->block_size);
	memset(data, 0, size);
	free(data);
	data = NULL;

	child = json_get_object_child(jsp, parent, "tcon_data_path");
	cnt = json_get_array_size(jsp, child);
	if (cnt <= 0) {
		LCDPR("tcon_data_path error\n");
		return 0;
	}

	bin_path = get_tcon_bin_path_mem();
	bin_path_k = get_tcon_bin_path_k_mem();
	if (!bin_path || !bin_path_k) {
		LCDERR("no memory to save tcon data path\n");
		return -1;
	}
	p0 = (char *)(bin_path + sizeof(struct lcd_tcon_bin_path_header_s));
	p1 = (char *)(bin_path_k + sizeof(struct lcd_tcon_bin_path_header_s));

	for (i = 0; i < cnt; i++) {
		str = json_get_arr_str(jsp, child, i, NULL);
		if (!str)
			continue;
		if (path_name_compose(dir_uboot, str, path) == 0) {
			strlcpy(p0 + 4, path, TCON_BIN_PATH_LEN - 4);
			tcon_bin_cnt++;
			p0 += TCON_BIN_PATH_LEN;
		}
		if (path_name_compose(dir_kernel, str, path) == 0) {
			strlcpy(p1 + 4, path, TCON_BIN_PATH_LEN - 4);
			tcon_bin_cnt_rsvd++;
			p1 += TCON_BIN_PATH_LEN;
		}
	}
	lcd_tcon_data_path_head_add((u8 *)bin_path, tcon_bin_cnt);
	lcd_tcon_data_path_head_add((u8 *)bin_path_k, tcon_bin_cnt_rsvd);

	return 0;
}
#else
static inline int lcd_panel_parse_tcon_from_json(struct aml_lcd_drv_s *pdrv)
{
	return -1;
}
#endif

#ifdef CONFIG_CMD_AML_MODEL
static int lcd_panel_parse_tcon_from_ini(struct aml_lcd_drv_s *pdrv)
{
	void *inip, *psec;
	const char *str;
	struct lcd_tcon_init_block_header_s *init_header;
	struct lcd_tcon_bin_path_header_s *path_header;
	unsigned char *data = NULL, *bin_path, *bin_path_k;
	char path[32], *p0, *p1;
	int size, i;
	unsigned int temp_crc32;
	int tcon_bin_cnt = 0, tcon_bin_cnt_rsvd = 0;

	inip = get_lcd_ini_parse_mem(pdrv->index);
	if (!inip) {
		LCDERR("%s: parse_mem not ready\n", __func__);
		return -1;
	}

	psec = lcd_ini_get_section(inip, "tcon_Path");
	if (!psec) {
		LCDERR("%s: not find tcon_Path\n", __func__);
		return -1;
	}

	str = lcd_ini_get_str(inip, psec, "TCON_BIN_PATH", NULL);
	if (!str) {
		LCDERR("%s: can't find TCON_BIN_PATH\n", __func__);
		return -1;
	}

	data = model_read_file_to_buffer((const char *)str, &size);
	if (!data || size <= 0) {
		LCDERR("%s: read file error: %s\n", __func__, str);
		return -1;
	}
	init_header = (struct lcd_tcon_init_block_header_s *)data;
	temp_crc32 = cal_CRC32(0, &data[4], (init_header->block_size - 4));
	if (temp_crc32 != init_header->crc32) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDERR("%s: tcon_bin crc32 check error (raw=0x%08x, cal=0x%02x)\n",
			       __func__, init_header->crc32, temp_crc32);
		} else {
			LCDERR("%s: tcon_bin crc32 check error\n", __func__);
		}
		memset(data, 0, size);
		free(data);
		return -1;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: read tcon_reg bin ok:%s, size:%d\n", __func__, str, size);
	panel_param_mem_put(data, "tcon_core_reg", init_header->block_size);
	memset(data, 0, size);
	free(data);
	data = NULL;

	bin_path = get_tcon_bin_path_mem();
	bin_path_k = get_tcon_bin_path_k_mem();
	if (!bin_path || !bin_path_k) {
		LCDERR("no memory to save tcon data path\n");
		return -1;
	}
	p0 = (char *)(bin_path + sizeof(struct lcd_tcon_bin_path_header_s));
	p1 = (char *)(bin_path_k + sizeof(struct lcd_tcon_bin_path_header_s));

	for (i = 0; i < 32; i++) {
		snprintf(path, 31, "TCON_DATA_%d_BIN_PATH", i);
		str = lcd_ini_get_str(inip, psec, path, NULL);
		if (!str)
			break;
		strlcpy(p0 + 4, str, TCON_BIN_PATH_LEN - 4);
		tcon_bin_cnt++;
		p0 += TCON_BIN_PATH_LEN;
	}
	lcd_tcon_data_path_head_add((u8 *)bin_path, tcon_bin_cnt);

	str = lcd_ini_get_str(inip, psec, "TCON_BIN_PATH_K", NULL);
	if (str) {
		for (i = 0; i < 32; i++) {
			snprintf(path, 31, "TCON_DATA_%d_BIN_PATH_K", i);
			str = lcd_ini_get_str(inip, psec, path, NULL);
			if (!str)
				break;
			strlcpy(p1 + 4, str, TCON_BIN_PATH_LEN - 4);
			tcon_bin_cnt_rsvd++;
			p1 += TCON_BIN_PATH_LEN;
		}
		lcd_tcon_data_path_head_add((u8 *)bin_path_k, tcon_bin_cnt_rsvd);
	} else { //no tcon_bin_path_k
		path_header = (struct lcd_tcon_bin_path_header_s *)bin_path;
		memcpy(bin_path_k, bin_path, path_header->data_size);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: tcon bin_path_k same as uboot\n", __func__);
	}

	return 0;
}
#else
static inline int lcd_panel_parse_tcon_from_ini(struct aml_lcd_drv_s *pdrv)
{
	return -1;
}
#endif

static int lcd_tcon_reserved_mem_data_load(struct aml_lcd_drv_s *pdrv)
{
	unsigned char *vaddr;
	unsigned int size = 0;
	int i, data_relocate = 0;
	char name[32];
	int ret;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: mm_table version: %d\n", __func__, tcon_mm_table.version);

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	if (tcon_mm_table.version == 0)
		return -1;

	if (!tcon_mm_table.data_mem_vaddr) {
		LCDERR("%s: data_mem error\n", __func__);
		return -1;
	}
	if (!tcon_mm_table.data_size) {
		LCDERR("%s: data_size error\n", __func__);
		return -1;
	}
	if (pdrv->config_load != LCD_CONFIG_FILE && pdrv->config_load != LCD_CONFIG_UKEY)
		return -1;

	for (i = 0; i < tcon_mm_table.block_cnt; i++) {
		ret = get_tcon_data_bin_by_id(tcon_mm_table.data_mem_vaddr, i);
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
					memcpy(vaddr, tcon_mm_table.data_mem_vaddr[i], size);
					free(tcon_mm_table.data_mem_vaddr[i]);
					tcon_mm_table.data_mem_vaddr[i] = vaddr;

					flush_cache((unsigned long)vaddr, size);
				} else {
					free(tcon_mm_table.data_mem_vaddr[i]);
					tcon_mm_table.data_mem_vaddr[i] = NULL;
					continue;
				}
			}
		}
		lcd_tcon_data_load(pdrv, tcon_mm_table.data_mem_vaddr[i], i);
	}

	return 0;
}

static int lcd_tcon_bin_path_update(unsigned int size)
{
	struct lcd_tcon_bin_path_header_s *header;
	unsigned char *mem_vaddr;
	unsigned int  temp_crc32;

	/* notice: different with kernel flow: mem_vaddr is not mapping to mem_paddr */
	mem_vaddr = get_tcon_bin_path_mem();
	if (!mem_vaddr) {
		LCDERR("%s: get mem error\n", __func__);
		return -1;
	}

	header = (struct lcd_tcon_bin_path_header_s *)mem_vaddr;
	if (header->data_size > size ||
	    header->data_size < sizeof(struct lcd_tcon_bin_path_header_s)) {
		LCDERR("%s: bin_path data_size error\n", __func__);
		return -1;
	}
	if (header->block_cnt > 32) {
		LCDERR("%s: bin_path block_cnt error\n", __func__);
		return -1;
	}

	temp_crc32 = cal_CRC32(0, &mem_vaddr[4], (header->data_size - 4));
	if (header->crc32 != temp_crc32) {
		LCDERR("%s: bin_path data crc error\n", __func__);
		return -1;
	}

	tcon_rmem.bin_path_rmem.mem_vaddr = mem_vaddr;
	tcon_mm_table.version = header->version;
	tcon_mm_table.data_load_level = header->data_load_level;
	tcon_mm_table.block_cnt = header->block_cnt;
	//tcon_mm_table.init_load = header->init_load;
	//LCDPR("%s: init_load: %d\n", __func__, tcon_mm_table.init_load);

	tcon_mm_table.bin_path_valid = 1;

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
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
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

static int lcd_tcon_mm_table_config(void)
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
		ret = tee_protect_mem(TEE_MEM_TYPE_TCON, 0,
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
				LCDPR("axi[%d] mem type=%d, size=%#x, reg=%#x, valid=%d\n",
				      i, axi_cfg->mem_type, axi_cfg->mem_size,
				      axi_cfg->axi_reg, axi_cfg->mem_valid);
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
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("tcon axi_mem paddr: 0x%08x, size: 0x%x, bank=%d\n",
		      tcon_rmem.axi_mem_paddr, tcon_rmem.axi_mem_size,
		      tcon_rmem.axi_bank);
	}

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
	ret = lcd_tcon_mm_table_config();

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

void lcd_tcon_init_data_version_update(struct tcon_core_reg_info_s *core_info,
		char *data_buf)
{
	if (!core_info || !data_buf)
		return;

	memcpy(core_info->bin_ver, data_buf, LCD_TCON_INIT_BIN_VERSION_SIZE);
	core_info->bin_ver[TCON_BIN_VER_LEN - 1] = '\0';
}

void lcd_tcon_ext_header_check(struct lcd_tcon_init_block_ext_header_s *ext_header)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct lcd_tcon_reg_block_info_s *reg_block = NULL;
	struct lcd_tcon_reg_skip_s *reg_skip_blk = NULL;
	int i, j, reg_start, reg_end, valid = 0;
	unsigned int pre_reg_end = 0, max_len = 0, tbl_width;

	if (!ext_header || !tcon_conf)
		return;

	if (!ext_header->reg_blk_info)
		goto __ext_header_chk_exit;

	tbl_width = lcd_do_div(tcon_conf->reg_table_width, 8);
	for (i = 0; i < ext_header->reg_blk_num; i++) {
		reg_block = &ext_header->reg_blk_info[i];
		reg_start = reg_block->start;
		reg_end = reg_block->start + reg_block->len - 1;
		max_len += reg_block->len * tbl_width;

		if (reg_block->len <= 0)
			goto __ext_header_chk_exit;

		//check current block reg > pre block reg
		if (pre_reg_end && (reg_start <= pre_reg_end || reg_end <= pre_reg_end))
			goto __ext_header_chk_exit;

		//check skip range
		if (tcon_conf->reg_skip_tbl_len) {
			for (j = 0; j < tcon_conf->reg_skip_tbl_len; j++) {
				reg_skip_blk = &tcon_conf->reg_skip_tbl[j];

				if (reg_start >= reg_skip_blk->start_reg &&
					reg_start <= reg_skip_blk->end_reg)
					goto __ext_header_chk_exit;

				if (reg_end >= reg_skip_blk->start_reg &&
					reg_end <= reg_skip_blk->end_reg)
					goto __ext_header_chk_exit;
			}
		}

		pre_reg_end = reg_end;
	}

	if (max_len > tcon_conf->reg_table_len)
		goto __ext_header_chk_exit;

	valid = 1;

__ext_header_chk_exit:
	if (!valid && ext_header->reg_blk_num) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDERR("%s: invalid reg block, change blk_num %d-->0\n",
				__func__, ext_header->reg_blk_num);
		}
		ext_header->reg_blk_num = 0;  //restore blk num to 0
	}
}

int lcd_tcon_get_core_size(struct lcd_tcon_config_s *tcon_conf,
	struct lcd_tcon_init_block_header_s *header,
	struct lcd_tcon_init_block_ext_header_s *ext_header)
{
	struct lcd_tcon_reg_block_info_s *reg_blk = NULL;
	int table_size = 0, i;

	if (!tcon_conf || !header)
		return 0;

	table_size = tcon_conf->reg_table_len;
	if (ext_header && ext_header->reg_blk_num) {
		table_size = 0;
		for (i = 0; i < ext_header->reg_blk_num; i++) {
			reg_blk = &ext_header->reg_blk_info[i];
			table_size += reg_blk->len * header->data_byte_width;
		}
	}

	return table_size;
}

void lcd_tcon_core_info_print(struct aml_lcd_drv_s *pdrv,
		struct tcon_core_reg_info_s *core_reg_info)
{
	struct lcd_tcon_init_block_header_s *header;
	struct lcd_tcon_init_block_ext_header_s *ext_header;
	struct lcd_tcon_reg_block_info_s *reg_blk_info;
	int i = 0;

	if (!pdrv || !core_reg_info) {
		LCDERR("%s: invalid parameter\n", __func__);
		return;
	}

	if (!core_reg_info->header) {
		LCDERR("%s: header is null\n", __func__);
		return;
	}

	header = core_reg_info->header;
	ext_header = &core_reg_info->ext_header;

	LCDPR("core info:\n");
	LCDPR("  table      = %#llx\n", (unsigned long long)core_reg_info->table);
	LCDPR("  table_size = %d\n", core_reg_info->table_size);
	LCDPR("header:\n");
	LCDPR("  crc32      = 0x%08x\n", header->crc32);
	LCDPR("  chipid     = %d\n", header->chipid);
	LCDPR("  block_size = %d\n", header->block_size);
	LCDPR("  block_type = 0x%x\n", header->block_type);
	LCDPR("  block_ctrl = 0x%x\n", header->block_ctrl);
	LCDPR("  byte_width = 0x%x\n", header->data_byte_width);
	LCDPR("  resolution = %dx%d\n", header->h_active, header->v_active);
	LCDPR("  name       = %s\n", header->name);
	LCDPR("  header_size = %d\n", header->header_size);
	LCDPR("  ext_header_size = %d\n", header->ext_header_size);
	if (ext_header) {
		LCDPR("extern header:\n");
		LCDPR("  framerate_range   = %d~%d\n",
			ext_header->framerate_min, ext_header->framerate_max);
		LCDPR("  reg_block_num     = %d\n", ext_header->reg_blk_num);
		for (i = 0; i < ext_header->reg_blk_num; i++) {
			reg_blk_info = &ext_header->reg_blk_info[i];
			LCDPR("  reg_block[%d]: start=%#-6x, len=%-4d(%#-5x)  (%#x~%#x)\n",
				i, reg_blk_info->start, reg_blk_info->len,
				reg_blk_info->len, reg_blk_info->start,
				(reg_blk_info->start + reg_blk_info->len - 1));
		}
	}
	if (core_reg_info->user_info)
		LCDPR("user info:\n%s\n", core_reg_info->user_info);
}

static int lcd_tcon_core_reg_check_load(struct aml_lcd_drv_s *pdrv, unsigned char *buf, int len)
{
	int data_len;
	unsigned char *p;
	struct lcd_tcon_init_block_header_s *header = NULL, *tmp_header;
	struct lcd_tcon_init_block_ext_header_s ext_header;
	struct tcon_core_reg_info_s *core_reg_info = NULL;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();

	if (!buf) {
		LCDERR("null tcon reg buf\n");
		return -1;
	}
	memset(&ext_header, 0, sizeof(ext_header));
	core_reg_info = &tcon_local_cfg.def_core_info;
	tmp_header = (struct lcd_tcon_init_block_header_s *)buf;
	header = calloc(1, tmp_header->header_size);
	if (!header)
		goto lcd_tcon_core_reg_check_load_err;

	memcpy(header, tmp_header, tmp_header->header_size);
	if (header->ext_header_size > 0) {
		memcpy(&ext_header, buf + header->header_size, sizeof(ext_header));
		ext_header.reg_blk_info = NULL;
		if (ext_header.reg_blk_num) {
			ext_header.reg_blk_info = (struct lcd_tcon_reg_block_info_s *)
				calloc(ext_header.reg_blk_num,
					sizeof(struct lcd_tcon_reg_block_info_s));
			if (!ext_header.reg_blk_info) {
				LCDERR("%s: alloc reg block fail\n", __func__);
				goto lcd_tcon_core_reg_check_load_err;
			}
			memcpy(ext_header.reg_blk_info,
				buf + TCON_EXT_BLK_INFO_PRE_OFFSET,
				ext_header.reg_blk_num *
				sizeof(struct lcd_tcon_reg_block_info_s));
		}
		lcd_tcon_ext_header_check(&ext_header);
		core_reg_info->table_size = lcd_tcon_get_core_size(tcon_conf,
			header, &ext_header);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: update core reg table size=%d (%#x)\n", __func__,
				core_reg_info->table_size, core_reg_info->table_size);
		}
		if (core_reg_info->table_size == 0)
			goto lcd_tcon_core_reg_check_load_err;
	}

	data_len = core_reg_info->table_size +
		header->header_size + header->ext_header_size;
	if (len < data_len || header->block_size < data_len) {
		LCDERR("%s: key_len(%d) or data block_size(%d) are not enough, need %d\n",
			__func__, len, header->block_size, data_len);
		goto lcd_tcon_core_reg_check_load_err;
	}
	if (header->block_size > data_len) {
		//user info
		core_reg_info->user_info = calloc(sizeof(char), header->block_size - data_len + 1);
		if (!core_reg_info->user_info)
			goto lcd_tcon_core_reg_check_load_err;

		memcpy(core_reg_info->user_info,
			buf + data_len, header->block_size - data_len);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("user info:\n%s\n", core_reg_info->user_info);
	}
	lcd_tcon_init_data_version_update(core_reg_info, header->version);
	memcpy(core_reg_info->bin_ver, tcon_local_cfg.cur_core_info.bin_ver,
		sizeof(tcon_local_cfg.cur_core_info.bin_ver));

	data_len = core_reg_info->table_size;
	if (!core_reg_info->table) {
		core_reg_info->table = (unsigned char *)calloc(1, data_len);
		if (!core_reg_info->table)
			goto lcd_tcon_core_reg_check_load_err;
	}
	p = buf + header->header_size + header->ext_header_size;
	memcpy(core_reg_info->table, p, data_len);
	core_reg_info->header = header;
	core_reg_info->ext_header = ext_header;

	if (tcon_conf && tcon_conf->tcon_init_table_pre_proc)
		tcon_conf->tcon_init_table_pre_proc(core_reg_info);

	memcpy(&tcon_local_cfg.cur_core_info,
		core_reg_info, sizeof(*core_reg_info));
	lcd_tcon_init_setting_check(pdrv, pdrv->config.timing.dft_timing,
		core_reg_info);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		lcd_tcon_core_info_print(pdrv, core_reg_info);
	LCDPR("tcon: load init data len: %d, ver: %s\n",
		data_len, tcon_local_cfg.cur_core_info.bin_ver);
	return 0;

lcd_tcon_core_reg_check_load_err:
	if (header)
		free(header);
	if (ext_header.reg_blk_info)
		free(ext_header.reg_blk_info);
	if (core_reg_info->user_info)
		free(core_reg_info->user_info);
	LCDERR("%s: tcon unifykey load error!!!\n", __func__);
	return -1;
}

static int lcd_tcon_load_init_data_from_unifykey_new(struct aml_lcd_drv_s *pdrv)
{
	int key_len;
	unsigned char *buf;
	int ret = 0;

	ret = lcd_unifykey_get_size("lcd_tcon", &key_len);
	if (ret)
		return -1;
	buf = (unsigned char *)calloc(1, key_len);
	if (!buf)
		return -1;

	ret = lcd_unifykey_get_tcon("lcd_tcon", buf, key_len);
	if (ret) {
		free(buf);
		return -1;
	}

	ret = lcd_tcon_core_reg_check_load(pdrv, buf, key_len);

	if (buf)
		free(buf);

	return ret;
}

/*
 * json and ini can use panel_param_mem_put to put tcon_core_reg to param_mem before tcon probe
 * and all this function to get
 */
static int lcd_tcon_load_init_data_from_param_mem(struct aml_lcd_drv_s *pdrv)
{
	unsigned int size = 0;
	int ret = 0;
	unsigned char *buf;

	if (pdrv->index != 0)
		return 0;

	buf = panel_param_mem_get("tcon_core_reg", &size);
	if (!buf) {
		LCDPR("%s error\n", __func__);
		return -1;
	}

	ret = lcd_tcon_core_reg_check_load(pdrv, buf, size);

	return ret;
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
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("can't find: rmem linux,lcd_tcon, try tcon mem from lrm\n");
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

int lcd_tcon_is_dccd_flow(void)
{
	struct lcd_tcon_local_cfg_s *tcon_local = get_lcd_tcon_local_cfg();

	if (!tcon_local)
		return 0;
	return tcon_local->is_dccd_flow;
}

unsigned int lcd_tcon_get_vfp_tail(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();

	if (!pdrv || !tcon_conf)
		return 0;

	return tcon_conf->vfp_tail;
}

static void lcd_tcon_update_dccd_flow(void)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct lcd_tcon_local_cfg_s *tcon_local = get_lcd_tcon_local_cfg();
	struct lcd_tcon_init_block_header_s *header;
	unsigned int crc = 0, support_dccd = 0, has_tcon_file = 0;
	unsigned char *data_buf;
	int i, is_dccd_flow = 0;

#ifdef CONFIG_CMD_INI
	support_dccd = is_support_dccd();
	has_tcon_file = dccd_has_tcon_file();
	crc = get_dccd_crc();
#else
	support_dccd = lcd_tcon_is_support_dccd();
	has_tcon_file = lcd_tcon_dccd_has_tcon_file();
	crc = lcd_tcon_dccd_get_crc();
#endif
	if (!tcon_local || !mm_table || !tcon_local->def_core_info.header ||
	    mm_table->version == 0)
		return;

	if (!support_dccd)
		goto __is_dccd_flow_exit;

	if (!has_tcon_file) {
		is_dccd_flow = 1;
		goto __is_dccd_flow_exit;
	}

	//check tcon bin
	if (!tcon_local->def_core_info.header->dccd_flag ||
			tcon_local->def_core_info.header->dccd_crc != crc) {
		is_dccd_flow = 1;
		goto __is_dccd_flow_exit;
	}

	//check multi bin
	for (i = 0; i < mm_table->block_cnt; i++) {
		if (!mm_table->data_mem_vaddr[i])
			continue;
		data_buf = mm_table->data_mem_vaddr[i];
		header = (struct lcd_tcon_init_block_header_s *)data_buf;
		if (is_block_type_basic_init(header->block_type) &&
				is_block_ctrl_ufr(header->block_ctrl)) {
			if (!header->dccd_flag || header->dccd_crc != crc) {
				is_dccd_flow = 1;
				break;
			}
		}
	}

__is_dccd_flow_exit:
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: dccd flow flag=%d\n", __func__, is_dccd_flow);
	tcon_local->is_dccd_flow = is_dccd_flow;
}

static int lcd_tcon_get_config(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int ret;

	if (load_id == LCD_CONFIG_BSP)
		lcd_tcon_reserved_memory_init_default(pdrv);
	else
		lcd_tcon_reserved_memory_init_dts(dt_addr, pdrv);

	if (load_id == LCD_CONFIG_FILE) {
		if (get_lcd_panel_file_type(pdrv->index) == PANEL_FILE_JSON)
			lcd_panel_parse_tcon_from_json(pdrv);
		else if (get_lcd_panel_file_type(pdrv->index) == PANEL_FILE_INI)
			lcd_panel_parse_tcon_from_ini(pdrv);
		lcd_tcon_load_dccd(pdrv);
	}

	if (tcon_rmem.rsv_mem_paddr) {
		tcon_rmem.flag = 1;
		LCDPR("tcon: rsv_mem addr:0x%x, size:0x%x\n",
		      tcon_rmem.rsv_mem_paddr, tcon_rmem.rsv_mem_size);
		lcd_tcon_mem_config();
	}

	tcon_local_cfg.def_core_info.table_size = lcd_tcon_conf->reg_table_len;

	ret = -1;
	if (load_id == LCD_CONFIG_FILE)
		ret = lcd_tcon_load_init_data_from_param_mem(pdrv);
	else if (lcd_tcon_conf->core_reg_ver)
		ret = lcd_tcon_load_init_data_from_unifykey_new(pdrv);
	if (ret)
		return -1;

	lcd_tcon_reserved_mem_data_load(pdrv);

	lcd_tcon_update_dccd_flow();
	lcd_tcon_bin_path_resv_mem_set();

#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
	lcd_tcon_mem_tee_protect(1);
#endif
	return 0;
}

/* **********************************
 * tcon match data
 * **********************************
 */
static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_t5m[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00800000, 0x261, 0 },  /* 8M */
	{ TCON_AXI_MEM_TYPE_DEMURA, 0x00100000, 0x1a9, 0 },  /* 1M */
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

static struct lcd_tcon_axi_mem_cfg_s axi_mem_cfg_tbl_t6d[] = {
	{ TCON_AXI_MEM_TYPE_OD,     0x00200000, 0x261, 0 },  /* 2M */
	{ TCON_AXI_MEM_TYPE_DEMURA, 0x00100000, 0x19b, 0 },  /* 1M */
};

static struct lcd_tcon_reg_skip_s tcon_skip_reg_tbl_t6w[] = {
	{ 0x0040, 0x00ff },
	{ 0x1200, 0x12ff },
};

static struct lcd_tcon_dma_ops_s lcd_tcon_dma_ops_t5m = {
	.status = 0,
	.addr_list = NULL,
	.get_frame_cnt = tcon_lut_dma_get_frame_cnt,
	.start = tcon_lut_dma_start,
	.stop = tcon_lut_dma_stop,
	.mif_set = tcon_lut_dma_mif_set,
	.init = tcon_lut_dma_init_t5m,
	.deinit = tcon_lut_dma_deinit,
	.init_trans = lcd_tcon_dma_data_init_trans,
	.is_done = NULL,
	.clr_done = NULL,
	.wait_done = NULL,
	.sw_mif_set = NULL,
	.sw_trigger = NULL,
	.sw_is_done = NULL,
	.sw_wait_done = NULL,
	.sw_clr_done = NULL,
};

static struct lcd_tcon_dma_ops_s lcd_tcon_dma_ops_t3x = {
	.status = 0,
	.addr_list = NULL,
	.get_frame_cnt = tcon_lut_dma_get_frame_cnt,
	.start = tcon_lut_dma_start,
	.stop = tcon_lut_dma_stop,
	.mif_set = tcon_lut_dma_mif_set,
	.init = tcon_lut_dma_init_t3x,
	.deinit = tcon_lut_dma_deinit,
	.init_trans = lcd_tcon_dma_data_init_trans,
	.is_done = NULL,
	.clr_done = NULL,
	.wait_done = NULL,
	.sw_mif_set = NULL,
	.sw_trigger = NULL,
	.sw_is_done = NULL,
	.sw_wait_done = NULL,
	.sw_clr_done = NULL,
};

static struct lcd_tcon_dma_ops_s lcd_tcon_dma_ops_t6d = {
	.status = 0,
	.addr_list = NULL,
	.get_frame_cnt = tcon_lut_dma_get_frame_cnt,
	.start = tcon_lut_dma_start_t6d,
	.stop = tcon_lut_dma_stop_t6d,
	.mif_set = tcon_lut_dma_mif_set,
	.init = tcon_lut_dma_init_t5m,
	.deinit = tcon_lut_dma_deinit,
	.init_trans = lcd_tcon_dma_data_init_trans,
	.is_done = NULL,
	.clr_done = NULL,
	.wait_done = NULL,
	.sw_mif_set = NULL,
	.sw_trigger = NULL,
	.sw_is_done = NULL,
	.sw_wait_done = NULL,
	.sw_clr_done = NULL,
};

static struct lcd_tcon_dma_ops_s lcd_tcon_dma_ops_t6w = {
	.status = 0,
	.addr_list = NULL,
	.get_frame_cnt = tcon_lut_dma_get_frame_cnt_t6w,
	.start = tcon_lut_dma_start,
	.stop = tcon_lut_dma_stop,
	.mif_set = tcon_lut_dma_mif_set_t6w,
	.init = tcon_lut_dma_init_t6w,
	.deinit = tcon_lut_dma_deinit,
	.init_trans = lcd_tcon_dma_data_init_trans,
	.is_done = NULL,
	.clr_done = NULL,
	.wait_done = NULL,
	.sw_mif_set = NULL,
	.sw_trigger = NULL,
	.sw_is_done = NULL,
	.sw_wait_done = NULL,
	.sw_clr_done = NULL,
};

static struct lcd_tcon_dma_ops_s lcd_tcon_dma_ops_t6x = {
	.status = 0,
	.addr_list = NULL,
	.get_frame_cnt = tcon_lut_dma_get_frame_cnt_t6w,
	.start = tcon_lut_dma_start,
	.stop = tcon_lut_dma_stop,
	.mif_set = tcon_lut_dma_mif_set_t6w,
	.init = tcon_lut_dma_init_t6w,
	.deinit = tcon_lut_dma_deinit,
	.init_trans = lcd_tcon_dma_data_init_trans_t6x,
	.is_done = tcon_aptu_dma_done,
	.clr_done = tcon_clr_dma_done,
	.wait_done = wait_tcon_aptu_dma_done,

	.sw_mif_set = tcon_sw_dma_mif_set,
	.sw_trigger = tcon_sw_dma_trig,
	.sw_is_done = tcon_sw_dma_done,
	.sw_clr_done = tcon_clr_dma_done,
	.sw_wait_done = wait_tcon_sw_dma_done,
};

static struct lcd_tcon_config_s tcon_data_t5m = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 5,

	.init_pre_setting = lcd_tcon_init_pre_setting_uhd,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5m),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5m,
	.lut_dma_ops = &lcd_tcon_dma_ops_t5m,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5m,
	.tcon_top_init = lcd_tcon_top_set_t5m,
	.tcon_enable = lcd_tcon_enable_t5m,
	.tcon_disable = lcd_tcon_disable_t5m,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_4k,
	.tcon_check = lcd_tcon_setting_check_t5m,
};

static struct lcd_tcon_config_s tcon_data_t3x = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T3X,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T3X,

	.rsv_mem_size    = 0x00a02840, /* 10M10k */
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 5,

	.init_pre_setting = lcd_tcon_init_pre_setting_uhd,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t3x),
	.axi_mem_cfg_tbl = (axi_mem_cfg_tbl_t3x),
	.lut_dma_ops = &lcd_tcon_dma_ops_t3x,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t3x,
	.tcon_top_init = lcd_tcon_top_set_t5m,
	.tcon_enable = lcd_tcon_enable_t5m,
	.tcon_disable = lcd_tcon_disable_t5m,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_4k,
	.tcon_check = lcd_tcon_setting_check_t5m,
};

static struct lcd_tcon_config_s tcon_data_txhd2 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5D,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5D,
	.reg_table_len = LCD_TCON_TABLE_LEN_TXHD2,
	.core_reg_start = TCON_CORE_REG_START_T5D,

	.axi_bank = LCD_TCON_AXI_BANK_TXHD2,

	.rsv_mem_size    = 0x00502840,
	.axi_size        = 0x00500000, /* 5M*/
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 3,

	.init_pre_setting = lcd_tcon_init_pre_setting_uhd,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_txhd2),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_txhd2,
	.lut_dma_ops = NULL,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_txhd2,
	.tcon_top_init = lcd_tcon_top_set_t5m,
	.tcon_enable = lcd_tcon_enable_txhd2,
	.tcon_disable = lcd_tcon_disable_t6d,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_fhd,
	.tcon_check = lcd_tcon_setting_check_txhd2,
};

static struct lcd_tcon_config_s tcon_data_t6d = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5D,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5D,
	.reg_table_len = LCD_TCON_TABLE_LEN_T6D,
	.core_reg_start = TCON_CORE_REG_START_T5D,

	.axi_bank = LCD_TCON_AXI_BANK_T6D,

	.rsv_mem_size    = 0x00302840,
	.axi_size        = 0x00300000, /* 3M*/
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 3,

	.init_pre_setting = lcd_tcon_init_pre_setting_fhd,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t6d),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t6d,
	.lut_dma_ops = &lcd_tcon_dma_ops_t6d,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5m,
	.tcon_top_init = lcd_tcon_top_set_t6d,
	.tcon_enable = lcd_tcon_enable_t5m,
	.tcon_disable = lcd_tcon_disable_t6d,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_fhd,
	.tcon_check = lcd_tcon_setting_check_txhd2,
};

static struct lcd_tcon_config_s tcon_data_t6w = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T6W,
	.core_reg_start = TCON_CORE_REG_START_T6W,
	.reg_skip_tbl = tcon_skip_reg_tbl_t6w,
	.reg_skip_tbl_len = ARRAY_SIZE(tcon_skip_reg_tbl_t6w),

	.axi_bank = LCD_TCON_AXI_BANK_T6W,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 5,

	.init_pre_setting = lcd_tcon_init_pre_setting_t6w,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t5m),
	.axi_mem_cfg_tbl = axi_mem_cfg_tbl_t5m,
	.lut_dma_ops = &lcd_tcon_dma_ops_t6w,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5m,
	.tcon_top_init = lcd_tcon_top_set_t6d,
	.tcon_enable = lcd_tcon_enable_t5m,
	.tcon_disable = lcd_tcon_disable_t5m,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_4k,
	.tcon_check = lcd_tcon_setting_check_t5m,
};

static struct lcd_tcon_config_s tcon_data_t6x = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T6W,
	.core_reg_start = TCON_CORE_REG_START_T6W,
	.reg_skip_tbl = tcon_skip_reg_tbl_t6x,
	.reg_skip_tbl_len = ARRAY_SIZE(tcon_skip_reg_tbl_t6x),

	.axi_bank = LCD_TCON_AXI_BANK_T3X,

	.rsv_mem_size    = 0x00a02840,
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.secure_cfg_size = 0x00000040, /* 64byte */

	.vfp_tail = 5,

	.init_pre_setting = lcd_tcon_init_pre_setting_t6x,
	.init_post_setting = lcd_tcon_init_post_setting,
	.axi_tbl_len = ARRAY_SIZE(axi_mem_cfg_tbl_t3x),
	.axi_mem_cfg_tbl = (axi_mem_cfg_tbl_t3x),
	.lut_dma_ops = &lcd_tcon_dma_ops_t6x,

	.tcon_init_table_pre_proc = lcd_tcon_init_table_pre_proc,
	.tcon_init_table_post_proc = lcd_tcon_init_table_post_proc,
	.tcon_lut_post_proc = lcd_tcon_lut_post_proc,
	.tcon_global_reset = lcd_tcon_global_reset_t5m,
	.tcon_top_init = lcd_tcon_top_set_t6d,
	.tcon_enable = lcd_tcon_enable_t5m,
	.tcon_disable = lcd_tcon_disable_t5m,
	.tcon_forbidden_check = lcd_tcon_forbidden_check_4k,
	.tcon_check = lcd_tcon_setting_check_t5m,
};

int lcd_tcon_probe(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int ret = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	lcd_tcon_conf = NULL;
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
		lcd_tcon_conf = &tcon_data_t5m;
		break;
	case LCD_CHIP_T3X:
		lcd_tcon_conf = &tcon_data_t3x;
		break;
	case LCD_CHIP_TXHD2:
		lcd_tcon_conf = &tcon_data_txhd2;
		break;
	case LCD_CHIP_T6D:
		lcd_tcon_conf = &tcon_data_t6d;
		break;
	case LCD_CHIP_T6W:
		lcd_tcon_conf = &tcon_data_t6w;
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
		if (pdrv->data->chip_type == LCD_CHIP_TXHD2 ||
		    pdrv->data->chip_type == LCD_CHIP_T6D)
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

