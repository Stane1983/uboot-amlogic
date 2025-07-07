// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
// #include <asm/arch/io.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "../lcd_connector.h"
#include "lcd_tcon.h"
#include "env.h"

#define PR_LINE_BUF_MAX    200

static unsigned int lcd_tcon_reg_read(unsigned int addr, unsigned int flag)
{
	unsigned int val;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;

	if (flag)
		val = lcd_tcon_read_byte(addr);
	else
		val = lcd_tcon_read(addr);

	return val;
}

static void lcd_tcon_reg_write(unsigned int addr, unsigned int val, unsigned int flag)
{
	unsigned char temp;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (flag) {
		temp = (unsigned char)val;
		lcd_tcon_write_byte(addr, temp);
	} else {
		lcd_tcon_write(addr, val);
	}
}

static void lcd_tcon_reg_table_print(void)
{
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct tcon_core_reg_info_s *core_reg_info = NULL;
	struct lcd_tcon_reg_block_info_s *reg_blk = NULL;
	int i, j, cnt, blk, data_byte = 1, offset = 0;
	int ret, byte_start = 0, byte_len = 0;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;
	if (!local_cfg)
		return;

	core_reg_info = &local_cfg->cur_core_info;
	if (!core_reg_info->table) {
		LCDERR("%s: reg_table is null\n", __func__);
		return;
	}

	LCDPR("%s:\n", __func__);
	if (core_reg_info->header->ext_header_size &&
			core_reg_info->ext_header.reg_blk_num) {
		data_byte = core_reg_info->header->data_byte_width;
		for (blk = 0; blk < core_reg_info->ext_header.reg_blk_num; blk++) {
			reg_blk = &core_reg_info->ext_header.reg_blk_info[blk];
			byte_start = reg_blk->start * data_byte;
			byte_len = reg_blk->len * data_byte;
			printf("block[%d]: (start=%#x, len=%d)\n",
				blk, byte_start, byte_len);
			for (i = 0; i < byte_len; i += 16) {
				printf("%04x: ", byte_start + i);
				for (j = 0; j < 16; j++) {
					if ((i + j) >= byte_len)
						break;
					printf(" %02x",
						core_reg_info->table[offset++]);
				}
				printf("\n");
			}
		}
	} else {
		cnt = core_reg_info->table_size;
		for (i = 0; i < cnt; i += 16) {
			printf("%04x: ", i);
			for (j = 0; j < 16; j++) {
				if ((i + j) >= cnt)
					break;
				printf(" %02x", core_reg_info->table[i + j]);
			}
			printf("\n");
		}
	}
}

static void lcd_tcon_reg_readback_print(void)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct lcd_tcon_reg_skip_s *reg_skip = NULL;
	unsigned int i, j, k, cnt, offset;
	int ret, is_skip;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;
	if (!tcon_conf)
		return;

	LCDPR("%s:\n", __func__);
	cnt = tcon_conf->reg_table_len;
	offset = tcon_conf->core_reg_start;
	if (tcon_conf->core_reg_width == 8) {
		for (i = offset; i < cnt; i += 16) {
			printf("%04x: ", i);
			for (j = 0; j < 16; j++) {
				if ((i + j) >= cnt)
					break;
				printf(" %02x", lcd_tcon_read_byte(i + j));
			}
			printf("\n");
		}
	} else {
		if (tcon_conf->reg_table_width == 32) {
			cnt /= 4;
			for (i = offset; i < cnt; i += 4) {
				printf("%04x: ", i);
				for (j = 0; j < 4; j++) {
					if ((i + j) >= cnt)
						break;
					is_skip = 0;
					for (k = 0; k < tcon_conf->reg_skip_tbl_len; k++) {
						reg_skip = &tcon_conf->reg_skip_tbl[k];
						if ((i + j) >= reg_skip->start_reg &&
							(i + j) <= reg_skip->end_reg)
							is_skip = 1;
					}
					if (is_skip)
						printf(" xxxxxxxx");
					else
						printf(" %08x", lcd_tcon_read(i + j));
				}
				printf("\n");
			}
		} else {
			for (i = offset; i < cnt; i += 16) {
				printf("%04x: ", i);
				for (j = 0; j < 16; j++) {
					if ((i + j) >= cnt)
						break;
					is_skip = 0;
					for (k = 0; k < tcon_conf->reg_skip_tbl_len; k++) {
						reg_skip = &tcon_conf->reg_skip_tbl[k];
						if ((i + j) >= reg_skip->start_reg &&
							(i + j) <= reg_skip->end_reg)
							is_skip = 1;
					}
					if (is_skip)
						printf(" xx");
					else
						printf(" %02x", lcd_tcon_read(i + j));
				}
				printf("\n");
			}
		}
	}
}

static unsigned int lcd_tcon_table_read(unsigned int addr)
{
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_core_reg_info_s *core_reg_info = NULL;
	unsigned char *table8;
	unsigned int *table32, size = 0, val = 0;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;
	if (!tcon_conf || !local_cfg)
		return 0;

	core_reg_info = &local_cfg->cur_core_info;
	if (!core_reg_info || !core_reg_info->table) {
		LCDERR("tcon reg_table is null\n");
		return 0;
	}

	if (core_reg_info->header->ext_header_size &&
			core_reg_info->ext_header.reg_blk_num) {
		lcd_tcon_get_table32_reg(core_reg_info, addr, &val);
	} else {
		if (tcon_conf->core_reg_width == 8)
			size = core_reg_info->table_size;
		else
			size = core_reg_info->table_size / 4;
		if (addr >= size) {
			LCDERR("invalid tcon reg_table addr: 0x%04x\n", addr);
			return 0;
		}

		if (tcon_conf->core_reg_width == 8) {
			table8 = core_reg_info->table;
			val = table8[addr];
		} else {
			table32 = (unsigned int *)core_reg_info->table;
			val = table32[addr];
		}
	}

	return val;
}

static unsigned int lcd_tcon_table_write(unsigned int addr, unsigned int val)
{
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_core_reg_info_s *core_reg_info = NULL;
	unsigned char *table8;
	unsigned int *table32, size = 0, read_val = 0;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;
	if (!tcon_conf || !local_cfg)
		return 0;

	core_reg_info = &local_cfg->cur_core_info;
	if (!core_reg_info || !core_reg_info->table) {
		LCDERR("tcon reg_table is null\n");
		return 0;
	}

	if (core_reg_info->header->ext_header_size &&
			core_reg_info->ext_header.reg_blk_num) {
		lcd_tcon_set_table32_reg(core_reg_info, addr, val);
	} else {
		if (tcon_conf->core_reg_width == 8)
			size = core_reg_info->table_size;
		else
			size = core_reg_info->table_size / 4;
		if (addr >= size) {
			LCDERR("invalid tcon reg_table addr: 0x%04x\n", addr);
			return 0;
		}

		if (tcon_conf->core_reg_width == 8) {
			table8 = core_reg_info->table;
			table8[addr] = (unsigned char)(val & 0xff);
			read_val = table8[addr];
		} else {
			table32 = (unsigned int *)core_reg_info->table;
			table32[addr] = val;
			read_val = table32[addr];
		}
	}

	return read_val;
}

static void lcd_tcon_data_block_print(char *buf, unsigned char *data_mem, unsigned int size)
{
	int i, j, n, left;

	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_LINE_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			left = PR_LINE_BUF_MAX - n - 1;
			n += snprintf(buf + n, left, " %02x", data_mem[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}
}

static void lcd_tcon_data_print(unsigned char index)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	unsigned char *lut_buf = NULL;
	char *pr_buf = NULL;
	unsigned int size, i;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;
	if (!mm_table)
		return;

	if (mm_table->version == 0) {
		LCDERR("%s: mem map version invalid\n", __func__);
		return;
	}

	pr_buf = (char *)malloc(PR_LINE_BUF_MAX * sizeof(char));
	if (!pr_buf) {
		LCDERR("%s: pr_buf malloc error\n", __func__);
		return;
	}

	if (index == 0xff) {
		for (i = 0; i < mm_table->block_cnt; i++) {
			if (!mm_table->data_mem_vaddr[i])
				continue;

			printf("tcon data[%d] print:\n", i);
			lut_buf = mm_table->data_mem_vaddr[i];
			size = lut_buf[8] | (lut_buf[9] << 8) |
				(lut_buf[10] << 16) | (lut_buf[11] << 24);
			lcd_tcon_data_block_print(pr_buf, lut_buf, size);
		}
	} else {
		if (index >= mm_table->block_cnt) {
			LCDERR("%s: invalid index %d\n", __func__, index);
			free(pr_buf);
			return;
		}
		if (!mm_table->data_mem_vaddr[index]) {
			LCDERR("%s: invalid data_mem buf\n", __func__);
			free(pr_buf);
			return;
		}

		printf("tcon data[%d] print:\n", index);
		lut_buf = mm_table->data_mem_vaddr[index];
		size = lut_buf[8] | (lut_buf[9] << 8) |
			(lut_buf[10] << 16) | (lut_buf[11] << 24);
		lcd_tcon_data_block_print(pr_buf, lut_buf, size);
	}

	free(pr_buf);
}

static void lcd_tcon_axi_mem_print(struct tcon_rmem_s *tcon_rmem)
{
	int i;

	if (!tcon_rmem || !tcon_rmem->axi_rmem)
		return;

	if (tcon_rmem->secure_axi_rmem.sec_protect) {
		printf("axi secure mem paddr: 0x%lx, vaddr: 0x%p, size:0x%x\n",
		       (unsigned long)tcon_rmem->secure_axi_rmem.mem_paddr,
		       tcon_rmem->secure_axi_rmem.mem_vaddr,
		       tcon_rmem->secure_axi_rmem.mem_size);
	}
	for (i = 0; i < tcon_rmem->axi_bank; i++) {
		printf("axi_mem[%d] paddr: 0x%lx\n"
		       "axi_mem[%d] vaddr: 0x%p\n"
		       "axi_mem[%d] size:  0x%x\n",
		       i, (unsigned long)tcon_rmem->axi_rmem[i].mem_paddr,
		       i, tcon_rmem->axi_rmem[i].mem_vaddr,
		       i, tcon_rmem->axi_rmem[i].mem_size);
	}
}

static void lcd_tcon_mm_table_v1_print(struct tcon_mem_map_table_s *mm_table,
		struct tcon_rmem_s *tcon_rmem)
{
	unsigned char *mem_vaddr;
	struct lcd_tcon_data_block_header_s *bin_header;
	int i;

	if (mm_table->data_mem_vaddr && mm_table->data_size) {
		printf("data_mem block_cnt: %d\n"
			"data_mem list:\n",
			mm_table->block_cnt);
		for (i = 0; i < mm_table->block_cnt; i++) {
			mem_vaddr = mm_table->data_mem_vaddr[i];
			if (mem_vaddr) {
				bin_header = (struct lcd_tcon_data_block_header_s *)mem_vaddr;
				printf("  [%d]: vaddr: 0x%p, size: 0x%x, %s(0x%x)\n",
					i, mem_vaddr,
					bin_header->block_size,
					bin_header->name, bin_header->block_type);
			} else {
				printf("  [%d]: vaddr: NULL, size:  0\n", i);
			}
		}
	}
}

void lcd_tcon_info_print(struct aml_lcd_drv_s *pdrv)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_core_reg_info_s *core_info = NULL;
	struct lcd_tcon_reg_block_info_s *reg_blk_info = NULL;
	unsigned int size, cnt, file_size, n, sec_protect, sec_handle, *data;
	char *str;
	int i, ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (!mm_table || !tcon_rmem || !local_cfg || !tcon_conf)
		return;

	lcd_tcon_init_setting_check(pdrv, &pdrv->config.timing.act_timing,
			&local_cfg->cur_core_info);

	LCDPR("%s:\n", __func__);
	printf("core_reg_width:  %d\n"
		"reg_table_len:  %d\n"
		"tcon_bin_ver:   %s\n"
		"tcon_rmem_flag: %d\n"
		"rsv_mem paddr:  0x%08x\n"
		"rsv_mem size:   0x%08x\n",
		tcon_conf->core_reg_width,
		tcon_conf->reg_table_len,
		local_cfg->cur_core_info.bin_ver,
		tcon_rmem->flag,
		tcon_rmem->rsv_mem_paddr,
		tcon_rmem->rsv_mem_size);
	if (mm_table->bin_path_valid) {
		printf("bin_path_mem\n"
			"  paddr: 0x%08x\n"
			"  vaddr: 0x%p\n"
			"  size:  0x%08x\n",
			tcon_rmem->bin_path_rmem.mem_paddr,
			tcon_rmem->bin_path_rmem.mem_vaddr,
			tcon_rmem->bin_path_rmem.mem_size);
	}
	printf("\n");

	if (tcon_rmem->flag) {
		lcd_tcon_axi_mem_print(tcon_rmem);
		lcd_tcon_mm_table_v1_print(mm_table, tcon_rmem);

		if (tcon_rmem->bin_path_rmem.mem_vaddr) {
			size = *(unsigned int *)&tcon_rmem->bin_path_rmem.mem_vaddr[4];
			cnt = *(unsigned int *)&tcon_rmem->bin_path_rmem.mem_vaddr[16];
			if (size < (32 + 256 * cnt))
				return;
			if (cnt > 32)
				return;
			printf("\nbin_path:\n"
				"bin cnt:        %d\n"
				"data_complete:  %d\n"
				"bin_path_valid: %d\n",
				cnt, mm_table->data_complete, mm_table->bin_path_valid);
			for (i = 0; i < cnt; i++) {
				n = 32 + 256 * i;
				file_size = *(unsigned int *)&tcon_rmem->bin_path_rmem.mem_vaddr[n];
				str = (char *)&tcon_rmem->bin_path_rmem.mem_vaddr[n + 4];
				printf("bin[%d]: size: 0x%x, %s\n", i, file_size, str);
			}
		}
	}

	if (tcon_rmem->secure_cfg_rmem.mem_vaddr) {
		data = (unsigned int *)tcon_rmem->secure_cfg_rmem.mem_vaddr;
		cnt = tcon_conf->axi_bank;
		printf("\nsecure_cfg rsv_mem:\n"
			"  paddr: 0x%08x\n"
			"  vaddr: 0x%p\n"
			"  size:  0x%08x\n",
			tcon_rmem->secure_cfg_rmem.mem_paddr,
			tcon_rmem->secure_cfg_rmem.mem_vaddr,
			tcon_rmem->secure_cfg_rmem.mem_size);
		for (i = 0; i < cnt; i++) {
			n = 2 * i;
			sec_protect = *(data + n);
			sec_handle = *(data + n + 1);
			printf("  [%d]: protect: %d, handle: %d\n",
				i, sec_protect, sec_handle);
		}
	}

	printf("\nlut_valid:\n"
		"acc:    %d\n"
		"od:     %d\n"
		"lod:    %d\n"
		"vac:    %d\n"
		"demura: %d\n"
		"dither: %d\n",
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_ACC),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_OD),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_LOD),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_VAC),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_DEMURA),
		!!(mm_table->lut_valid_flag & LCD_TCON_DATA_VALID_DITHER));

	core_info = &local_cfg->cur_core_info;
	printf("\nbin info:\n");
	if (local_cfg->cur_core_info.header) {
		char user_version[TCON_BIN_VER_LEN];

		memset(user_version, 0, sizeof(user_version));
		memcpy(user_version, core_info->bin_ver,
			sizeof(core_info->bin_ver));
		printf("basic info:\n"
			"userVersion=%s\n"
			"userBinName=%s\n"
			"h_active=%d\n"
			"v_active=%d\n"
			"block_ctrl=%d\n",
			user_version,
			core_info->header->name,
			core_info->header->h_active,
			core_info->header->v_active,
			core_info->header->block_ctrl);
	}
	if (core_info->header->ext_header_size) {
		printf("framerate_min=%d\n"
			"framerate_max=%d\n"
			"reg_block_num=%d\n",
			core_info->ext_header.framerate_min,
			core_info->ext_header.framerate_max,
			core_info->ext_header.reg_blk_num);
		for (i = 0; i < core_info->ext_header.reg_blk_num; i++) {
			reg_blk_info = &core_info->ext_header.reg_blk_info[i];
			printf("  reg_block[%d]: start=%#x, len=%d(%#x)  (%#x~%#x)\n",
					i, reg_blk_info->start, reg_blk_info->len,
					reg_blk_info->len, reg_blk_info->start,
					(reg_blk_info->start + reg_blk_info->len - 1));
		}
	}
	if (core_info->user_info && strlen(core_info->user_info) > 0)
		printf("\nuser info:\n%s\n", core_info->user_info);

	printf("\n");
}

void lcd_tcon_debug_probe(struct aml_lcd_drv_s *pdrv)
{
	pdrv->tcon_reg_print = lcd_tcon_reg_readback_print;
	pdrv->tcon_table_print = lcd_tcon_reg_table_print;
	pdrv->tcon_data_print = lcd_tcon_data_print;
	pdrv->tcon_reg_read = lcd_tcon_reg_read;
	pdrv->tcon_reg_write = lcd_tcon_reg_write;
	pdrv->tcon_table_read = lcd_tcon_table_read;
	pdrv->tcon_table_write = lcd_tcon_table_write;
}
