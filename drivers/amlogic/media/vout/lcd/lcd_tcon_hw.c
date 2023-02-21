// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#ifdef CONFIG_AMLOGIC_TEE
#include <amlogic/tee_aml.h>
#endif
#include "lcd_reg.h"
#include "lcd_common.h"
#include "lcd_tcon.h"

static void lcd_tcon_core_reg_set(struct lcd_tcon_config_s *tcon_conf,
				  struct tcon_mem_map_table_s *mm_table,
				  unsigned char *core_reg_table)
{
	unsigned char *table8;
	unsigned int *table32;
	unsigned int len, offset, reg, bit;
	int i;

	if (!tcon_conf)
		return;
	if (!mm_table || !core_reg_table) {
		LCDERR("%s: table is NULL\n", __func__);
		return;
	}

	len = mm_table->core_reg_table_size;
	offset = tcon_conf->core_reg_start;
	reg = tcon_conf->reg_core_od;
	bit = tcon_conf->bit_od_en;
	if (tcon_conf->core_reg_width == 8) {
		table8 = core_reg_table;
		if (mm_table->version) {
			//pre disable od, enable od in lut bin
			table8[reg] &= ~(1 << bit);
		}
		for (i = offset; i < len; i++)
			lcd_tcon_write_byte(i, table8[i]);
	} else {
		if (tcon_conf->reg_table_width == 32) {
			len /= 4;
			table32 = (unsigned int *)core_reg_table;
			if (mm_table->version) {
				//pre disable od, enable od in lut bin
				table32[reg] &= ~(1 << bit);
			}
			//only valid in 32bit core reg
			if (tcon_conf->tcon_axi_mem_update)
				tcon_conf->tcon_axi_mem_update(table32);
			for (i = offset; i < len; i++)
				lcd_tcon_write(i, table32[i]);
		} else {
			table8 = core_reg_table;
			if (mm_table->version) {
				//pre disable od, enable od in lut bin
				table8[reg] &= ~(1 << bit);
			}
			for (i = offset; i < len; i++)
				lcd_tcon_write(i, table8[i]);
		}
	}
	LCDPR("%s\n", __func__);
}

static void lcd_tcon_data_init_set(struct aml_lcd_drv_s *pdrv, unsigned char *data_buf)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_init_block_header_s *data_header;
	unsigned char *core_reg_table;

	if (!tcon_conf)
		return;
	if (!mm_table)
		return;
	if (!local_cfg)
		return;

	data_header = (struct lcd_tcon_init_block_header_s *)data_buf;
	core_reg_table = data_buf + LCD_TCON_DATA_BLOCK_HEADER_SIZE;
	switch (data_header->block_ctrl) {
	case LCD_TCON_DATA_CTRL_FLAG_DLG:
		if (pdrv->config.basic.h_active == data_header->h_active &&
		    pdrv->config.basic.v_active == data_header->v_active) {
			lcd_tcon_init_data_version_update(data_header->version);
			lcd_tcon_core_reg_set(tcon_conf, mm_table, core_reg_table);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: dlg %dx%d init, bin_ver:%s\n",
					__func__, data_header->h_active,
					data_header->v_active,
					local_cfg->bin_ver);
			}
		}
		break;
	default:
		break;
	}
}

void lcd_tcon_axi_rmem_lut_load(unsigned int index, unsigned char *buf,
				unsigned int size)
{
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();

	if (!tcon_rmem || !tcon_rmem->axi_rmem) {
		LCDERR("axi_rmem is NULL\n");
		return;
	}
	if (!tcon_conf)
		return;
	if (index > tcon_conf->axi_bank) {
		LCDERR("axi_rmem index %d invalid\n", index);
		return;
	}
	if (tcon_rmem->axi_rmem[index].mem_size < size) {
		LCDERR("axi_mem[%d] size 0x%x is not enough, need 0x%x\n",
		       index, tcon_rmem->axi_rmem[index].mem_size, size);
		return;
	}

	memcpy(tcon_rmem->axi_rmem[index].mem_vaddr, buf, size);
}

static int lcd_tcon_wr_n_data_write(struct lcd_tcon_data_part_wr_n_s *wr_n,
				    unsigned char *p,
				    unsigned int n,
				    unsigned int reg)
{
	unsigned int k, data, d;

	if (wr_n->reg_inc) {
		for (k = 0; k < wr_n->data_cnt; k++) {
			data = 0;
			for (d = 0; d < wr_n->reg_data_byte; d++)
				data |= (p[n + d] << (d * 8));
			if (wr_n->reg_data_byte == 1)
				lcd_tcon_write_byte((reg + k), data);
			else
				lcd_tcon_write((reg + k), data);
			if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
				LCDPR("%s: write reg 0x%x=0x%x\n",
				      __func__, (reg + k), data);
			}
			n += wr_n->reg_data_byte;
		}
	} else {
		for (k = 0; k < wr_n->data_cnt; k++) {
			data = 0;
			for (d = 0; d < wr_n->reg_data_byte; d++)
				data |= (p[n + d] << (d * 8));
			if (wr_n->reg_data_byte == 1)
				lcd_tcon_write_byte(reg, data);
			else
				lcd_tcon_write(reg, data);
			if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
				LCDPR("%s: write reg 0x%x=0x%x\n",
				      __func__, reg, data);
			}
			n += wr_n->reg_data_byte;
		}
	}

	return 0;
}

static int lcd_tcon_data_common_parse_set(unsigned char *data_buf)
{
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct lcd_tcon_data_block_header_s *block_header;
	struct lcd_tcon_data_block_ext_header_s *ext_header;
	union lcd_tcon_data_part_u data_part;
	unsigned char *p;
	unsigned short part_cnt;
	unsigned char part_type;
	unsigned int size, reg, data, mask, temp, reg_base = 0;
	unsigned int data_offset, offset, i, j, k, d, m, n, step = 0;
	unsigned int reg_cnt, reg_byte, data_cnt, data_byte;
	unsigned short block_ctrl_flag;
	int ret;

	if (tcon_conf)
		reg_base = tcon_conf->core_reg_start;

	block_header = (struct lcd_tcon_data_block_header_s *)data_buf;
	p = data_buf + LCD_TCON_DATA_BLOCK_HEADER_SIZE;
	ext_header = (struct lcd_tcon_data_block_ext_header_s *)p;
	part_cnt = ext_header->part_cnt;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %s, part_cnt: %d\n", __func__, block_header->name, part_cnt);

	block_ctrl_flag = block_header->block_ctrl;
	data_offset = LCD_TCON_DATA_BLOCK_HEADER_SIZE + block_header->ext_header_size;
	size = 0;
	for (i = 0; i < part_cnt; i++) {
		p = data_buf + data_offset;
		part_type = p[LCD_TCON_DATA_PART_NAME_SIZE + 3];
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
			LCDPR("%s: start step %d, %s, type=0x%02x\n",
			      __func__, step, p, part_type);
		}
		switch (part_type) {
		case LCD_TCON_DATA_PART_TYPE_CONTROL:
			block_ctrl_flag = 0;
			data_part.ctrl = (struct lcd_tcon_data_part_ctrl_s *)p;
			offset = LCD_TCON_DATA_PART_CTRL_SIZE_PRE;
			size = offset + (data_part.ctrl->data_cnt *
					 data_part.ctrl->data_byte_width);
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			if (block_header->block_ctrl == 0)
				break;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: block %s: ctrl data_flag=0x%x, ctrl_method=0x%x\n",
				      __func__, block_header->name,
				      data_part.ctrl->ctrl_data_flag,
				      data_part.ctrl->ctrl_method);
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_WR_N:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.wr_n = (struct lcd_tcon_data_part_wr_n_s *)p;
			offset = LCD_TCON_DATA_PART_WR_N_SIZE_PRE;
			size = offset +
		(data_part.wr_n->reg_cnt * data_part.wr_n->reg_addr_byte) +
		(data_part.wr_n->data_cnt * data_part.wr_n->reg_data_byte);
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			reg_cnt = data_part.wr_n->reg_cnt;
			reg_byte = data_part.wr_n->reg_addr_byte;
			m = offset; /* for reg */
			n = m + (reg_cnt * reg_byte); /* for data */
			for (j = 0; j < reg_cnt; j++) {
				reg = 0;
				for (d = 0; d < reg_byte; d++)
					reg |= (p[m + d] << (d * 8));
				if (reg < reg_base)
					goto lcd_tcon_data_common_parse_set_err_reg;
				lcd_tcon_wr_n_data_write(data_part.wr_n, p, n, reg);
				m += reg_byte;
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_WR_DDR:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.wr_ddr = (struct lcd_tcon_data_part_wr_ddr_s *)p;
			offset = LCD_TCON_DATA_PART_WR_DDR_SIZE_PRE;
			m = data_part.wr_ddr->data_cnt * data_part.wr_ddr->data_byte;
			size = offset + m;
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			n = data_part.wr_ddr->axi_buf_id;
			lcd_tcon_axi_rmem_lut_load(n, &p[offset], m);
			break;
		case LCD_TCON_DATA_PART_TYPE_WR_MASK:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.wr_mask = (struct lcd_tcon_data_part_wr_mask_s *)p;
			offset = LCD_TCON_DATA_PART_WR_MASK_SIZE_PRE;
			size = offset + data_part.wr_mask->reg_addr_byte +
				(2 * data_part.wr_mask->reg_data_byte);
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			reg_byte = data_part.wr_mask->reg_addr_byte;
			data_byte = data_part.wr_mask->reg_data_byte;
			m = offset; /* for reg */
			n = m + reg_byte; /* for data */
			reg = 0;
			for (d = 0; d < reg_byte; d++)
				reg |= (p[m + d] << (d * 8));
			if (reg < reg_base)
				goto lcd_tcon_data_common_parse_set_err_reg;
			mask = 0;
			for (d = 0; d < data_byte; d++)
				mask |= (p[n + d] << (d * 8));
			n += data_byte;
			data = 0;
			for (d = 0; d < data_byte; d++)
				data |= (p[n + d] << (d * 8));
			if (data_byte == 1)
				lcd_tcon_update_bits_byte(reg, mask, data);
			else
				lcd_tcon_update_bits(reg, mask, data);
			if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
				LCDPR("%s: write reg 0x%x, data=0x%x, mask=0x%x\n",
				      __func__, reg, mask, data);
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_RD_MASK:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.rd_mask = (struct lcd_tcon_data_part_rd_mask_s *)p;
			offset = LCD_TCON_DATA_PART_RD_MASK_SIZE_PRE;
			size = offset + data_part.rd_mask->reg_addr_byte +
				data_part.rd_mask->reg_data_byte;
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			reg_byte = data_part.rd_mask->reg_addr_byte;
			data_byte = data_part.rd_mask->reg_data_byte;
			m = offset; /* for reg */
			n = m + reg_byte; /* for data */
			reg = 0;
			for (d = 0; d < reg_byte; d++)
				reg |= (p[m + d] << (d * 8));
			if (reg < reg_base)
				goto lcd_tcon_data_common_parse_set_err_reg;
			mask = 0;
			for (d = 0; d < data_byte; d++)
				mask |= (p[n + d] << (d * 8));
			if (data_byte == 1) {
				data = lcd_tcon_read_byte(reg) & mask;
				if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
					LCDPR("%s: read reg 0x%04x = 0x%02x, mask = 0x%02x\n",
					      __func__, reg, data, mask);
				}
			} else {
				data = lcd_tcon_read(reg) & mask;
				if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
					LCDPR("%s: read reg 0x%04x = 0x%08x, mask = 0x%08x\n",
					      __func__, reg, data, mask);
				}
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_CHK_WR_MASK:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.chk_wr_mask = (struct lcd_tcon_data_part_chk_wr_mask_s *)p;
			offset = LCD_TCON_DATA_PART_CHK_WR_MASK_SIZE_PRE;
			//include mask
			size = offset + data_part.chk_wr_mask->reg_chk_addr_byte +
				data_part.chk_wr_mask->reg_chk_data_byte *
				(data_part.chk_wr_mask->data_chk_cnt + 1) +
				data_part.chk_wr_mask->reg_wr_addr_byte +
				data_part.chk_wr_mask->reg_wr_data_byte *
				(data_part.chk_wr_mask->data_chk_cnt + 2);
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			reg_byte = data_part.chk_wr_mask->reg_chk_addr_byte;
			data_cnt = data_part.chk_wr_mask->data_chk_cnt;
			data_byte = data_part.chk_wr_mask->reg_chk_data_byte;
			m = offset; /* for reg */
			n = m + reg_byte; /* for data */
			reg = 0;
			for (d = 0; d < reg_byte; d++)
				reg |= (p[m + d] << (d * 8));
			if (reg < reg_base)
				goto lcd_tcon_data_common_parse_set_err_reg;
			mask = 0;
			for (d = 0; d < data_byte; d++)
				mask |= (p[n + d] << (d * 8));
			if (data_byte == 1)
				temp = lcd_tcon_read_byte(reg) & mask;
			else
				temp = lcd_tcon_read(reg) & mask;
			if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
				LCDPR("%s: read chk reg 0x%04x = 0x%02x, mask = 0x%02x\n",
				      __func__, reg, temp, mask);
			}
			n += data_byte;
			for (j = 0; j < data_cnt; j++) {
				data = 0;
				for (d = 0; d < data_byte; d++)
					data |= (p[n + d] << (d * 8));
				if ((data & mask) == temp)
					break;
				n += data_byte;
			}
			k = j;

			/* for reg */
			m = offset + reg_byte + data_byte * (data_cnt + 1);
			/* for data */
			n = m + data_part.chk_wr_mask->reg_wr_addr_byte;
			reg_byte = data_part.chk_wr_mask->reg_wr_addr_byte;
			data_byte = data_part.chk_wr_mask->reg_wr_data_byte;
			reg = 0;
			for (d = 0; d < reg_byte; d++)
				reg |= (p[m + d] << (d * 8));
			if (reg < reg_base)
				goto lcd_tcon_data_common_parse_set_err_reg;
			mask = 0;
			for (d = 0; d < data_byte; d++)
				mask |= (p[n + d] << (d * 8));
			n += data_byte;
			n += data_byte * k;
			data = 0;
			for (d = 0; d < data_byte; d++)
				data |= (p[n + d] << (d * 8));
			if (data_byte == 1)
				lcd_tcon_update_bits_byte(reg, mask, data);
			else
				lcd_tcon_update_bits(reg, mask, data);
			if (lcd_debug_print_flag & LCD_DBG_PR_ADV2) {
				LCDPR("%s: write reg 0x%x, data=0x%x, mask=0x%x\n",
				      __func__, reg, mask, data);
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_CHK_EXIT:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.chk_exit = (struct lcd_tcon_data_part_chk_exit_s *)p;
			offset = LCD_TCON_DATA_PART_CHK_EXIT_SIZE_PRE;
			size = offset + data_part.chk_exit->reg_addr_byte +
				(2 * data_part.chk_exit->reg_data_byte);
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			reg_byte = data_part.chk_exit->reg_addr_byte;
			data_byte = data_part.chk_exit->reg_data_byte;
			m = offset; /* for reg */
			n = m + reg_byte; /* for data */
			reg = 0;
			for (d = 0; d < reg_byte; d++)
				reg |= (p[m + d] << (d * 8));
			if (reg < reg_base)
				goto lcd_tcon_data_common_parse_set_err_reg;
			mask = 0;
			for (d = 0; d < data_byte; d++)
				mask |= (p[n + d] << (d * 8));
			n += data_byte;
			data = 0;
			for (d = 0; d < data_byte; d++)
				data |= (p[n + d] << (d * 8));
			if (data_byte == 1)
				ret = lcd_tcon_check_bits_byte(reg, mask, data);
			else
				ret = lcd_tcon_check_bits(reg, mask, data);
			if (ret) {
				LCDPR("%s: block %s data_part %d check exit\n",
				      __func__, block_header->name, i);
				return 0;
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_DELAY:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.delay = (struct lcd_tcon_data_part_delay_s *)p;
			size = LCD_TCON_DATA_PART_DELAY_SIZE;
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			if (data_part.delay->delay_us > 1000) {
				m = data_part.delay->delay_us / 1000;
				n = data_part.delay->delay_us % 1000;
				mdelay(m);
				if (n)
					udelay(n);
			}
			break;
		case LCD_TCON_DATA_PART_TYPE_PARAM:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			data_part.param = (struct lcd_tcon_data_part_param_s *)p;
			offset = LCD_TCON_DATA_PART_PARAM_SIZE_PRE;
			size = offset + data_part.param->param_size;
			if ((size + data_offset) > block_header->block_size)
				goto lcd_tcon_data_common_parse_set_err_size;
			break;
		default:
			if (block_ctrl_flag)
				goto lcd_tcon_data_common_parse_set_ctrl_err;
			LCDERR("%s: unsupport part type 0x%02x\n",
			       __func__, part_type);
			break;
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
			LCDPR("%s: end step %d, %s, type=0x%02x, size=%d\n",
			      __func__, step, p, part_type, size);
		}
		data_offset += size;
		step++;
	}

	return 0;

lcd_tcon_data_common_parse_set_ctrl_err:
	LCDERR("%s: block %s need control part\n", __func__, block_header->name);
	return -1;

lcd_tcon_data_common_parse_set_err_reg:
	LCDERR("%s: block %s step %d reg 0x%04x error\n",
	       __func__, block_header->name, step, reg);
	return -1;

lcd_tcon_data_common_parse_set_err_size:
	LCDERR("%s: block %s step %d size error\n",
	       __func__, block_header->name, step);
	return -1;
}

#ifdef CONFIG_AMLOGIC_TEE
int lcd_tcon_mem_tee_protect(int mem_flag, int protect_en)
{
	int ret;
	struct tcon_rmem_s *tcon_rmem = get_lcd_tcon_rmem();
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	unsigned char *secure_cfg_vaddr;
	unsigned int *data, sec_protect, sec_handle;

	if (!tcon_conf) {
		LCDERR("%s: tcon_conf is null\n", __func__);
		return -1;
	}
	if (!tcon_rmem->axi_rmem) {
		LCDERR("%s: axi_rmem is null\n", __func__);
		return -1;
	}

	if (mem_flag > tcon_conf->axi_bank) {
		LCDPR("no need protect\n");
		return 0;
	}

	/* mem_flag: 0 = od secure mem(default secure)
	 * 1 = demura_lut mem(default unsecure)
	 */
	secure_cfg_vaddr = tcon_rmem->secure_cfg_rmem.mem_vaddr;
	if (!secure_cfg_vaddr) {
		LCDERR("%s: secure_cfg_vaddr is null\n", __func__);
		return 0;
	}
	data = (unsigned int *)secure_cfg_vaddr;

	if (protect_en) {
		if (tcon_rmem->axi_rmem[mem_flag].sec_protect)
			return 0;

		ret = tee_protect_mem_by_type(TEE_MEM_TYPE_TCON,
					      tcon_rmem->axi_rmem[mem_flag].mem_paddr,
					      tcon_rmem->axi_rmem[mem_flag].mem_size,
					      &tcon_rmem->axi_rmem[mem_flag].sec_handle);

		if (ret) {
			LCDERR("%s: protect failed! mem[%d], start: 0x%08x, size: 0x%x, ret: %d\n",
			       __func__, mem_flag,
			       tcon_rmem->axi_rmem[mem_flag].mem_paddr,
			       tcon_rmem->axi_rmem[mem_flag].mem_size, ret);
			return -1;
		}
		tcon_rmem->axi_rmem[mem_flag].sec_protect = 1;
		LCDPR("%s: protect OK. mem[%d], start: 0x%08x, size: 0x%x\n",
			__func__, mem_flag,
			tcon_rmem->axi_rmem[mem_flag].mem_paddr,
			tcon_rmem->axi_rmem[mem_flag].mem_size);
	} else {
		if (!tcon_rmem->axi_rmem[mem_flag].sec_protect)
			return 0;
		tee_unprotect_mem(tcon_rmem->axi_rmem[mem_flag].sec_handle);
		tcon_rmem->axi_rmem[mem_flag].sec_protect = 0;
		tcon_rmem->axi_rmem[mem_flag].sec_handle = 0;
		LCDPR("%s: unprotect OK. mem[%d], start: 0x%08x, size: 0x%x\n",
			__func__, mem_flag,
			tcon_rmem->axi_rmem[mem_flag].mem_paddr,
			tcon_rmem->axi_rmem[mem_flag].mem_size);
	}
	//update secure_cfg_vaddr
	*data = tcon_rmem->axi_rmem[mem_flag].sec_protect;
	*(data + 1) = tcon_rmem->axi_rmem[mem_flag].sec_handle;
	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		sec_protect = secure_cfg_vaddr[0 + mem_flag * 8] |
				(secure_cfg_vaddr[1 + mem_flag * 8] << 8) |
				(secure_cfg_vaddr[2 + mem_flag * 8] << 16) |
				(secure_cfg_vaddr[3 + mem_flag * 8] << 24);
		sec_handle = secure_cfg_vaddr[4 + mem_flag * 8] |
				(secure_cfg_vaddr[5 + mem_flag * 8] << 8) |
				(secure_cfg_vaddr[6 + mem_flag * 8] << 16) |
				(secure_cfg_vaddr[7 + mem_flag * 8] << 24);
		LCDPR("mem[%d] sec_handle: %d, secure_cfg_rmem protect: %d, handle: %d\n",
			mem_flag, tcon_rmem->axi_rmem[mem_flag].sec_handle,
			sec_protect, sec_handle);
	}

	return 0;
}
#endif

static int lcd_tcon_data_set(struct aml_lcd_drv_s *pdrv,
		struct tcon_mem_map_table_s *mm_table)
{
	struct lcd_tcon_data_block_header_s *block_header;
	unsigned char *data_buf;
	unsigned int temp_crc32, index;
	int i, ret;

	if (!mm_table || !mm_table->data_mem_vaddr) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: no data_mem, exit\n", __func__);
		return 0;
	}

	if (!mm_table->data_priority) {
		LCDERR("%s: data_priority is null\n", __func__);
		return -1;
	}

	for (i = 0; i < mm_table->block_cnt; i++) {
		index = mm_table->data_priority[i].index;
		if (index >= mm_table->block_cnt) {
			LCDERR("%s: data index %d is invalid\n",
			       __func__, index);
			return -1;
		}
		if (!mm_table->data_mem_vaddr[index]) {
			LCDERR("%s: data_mem_vaddr[%d] is null\n",
			       __func__, index);
			continue;
		}
		data_buf = mm_table->data_mem_vaddr[index];
		block_header = (struct lcd_tcon_data_block_header_s *)data_buf;
		if (block_header->block_size < sizeof(struct lcd_tcon_data_block_header_s)) {
			LCDERR("%s: block_size[%d] 0x%x is invalid\n",
			       __func__, index, block_header->block_size);
			continue;
		}
		temp_crc32 = cal_crc32(0, &data_buf[4], (block_header->block_size - 4));
		if (temp_crc32 != block_header->crc32) {
			LCDERR("%s: block %d, %s data crc 0x%x error (raw 0x%x)\n",
				__func__, index, block_header->name,
				temp_crc32, block_header->crc32);
			continue;
		}

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: block %d, %s, priority %d: size=0x%x, type=0x%02x, ctrl=0x%x\n",
				__func__, index,
				block_header->name,
				mm_table->data_priority[i].priority,
				block_header->block_size,
				block_header->block_type,
				block_header->block_ctrl);
		}

		/* apply data */
		if (block_header->block_type == LCD_TCON_DATA_BLOCK_TYPE_BASIC_INIT) {
			lcd_tcon_data_init_set(pdrv, data_buf);
			continue;
		}

		if (block_header->block_ctrl == LCD_TCON_DATA_CTRL_FLAG_MULTI) {
			ret = lcd_tcon_data_multi_match_find(pdrv, data_buf);
			if (ret == 0)
				lcd_tcon_data_common_parse_set(data_buf);
		} else {
			lcd_tcon_data_common_parse_set(data_buf);
		}
	}

	LCDPR("%s finish\n", __func__);
	return 0;
}

static int lcd_tcon_top_set_t5(struct lcd_config_s *pconf)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("lcd tcon top set\n");

	lcd_tcon_write(TCON_CLK_CTRL, 0x001f);
	if (pconf->basic.lcd_type == LCD_P2P) {
		switch (pconf->control.p2p_cfg.p2p_type) {
		case P2P_CHPI:
		case P2P_USIT:
			lcd_tcon_write(TCON_TOP_CTRL, 0x8399);
			break;
		default:
			lcd_tcon_write(TCON_TOP_CTRL, 0x8b99);
			break;
		}
	} else {
		lcd_tcon_write(TCON_TOP_CTRL, 0x8b99);
	}
	lcd_tcon_write(TCON_PLLLOCK_CNTL, 0x0037);
	//lcd_tcon_write(TCON_RST_CTRL, 0x003f);
	lcd_tcon_write(TCON_RST_CTRL, 0x0000);
	lcd_tcon_write(TCON_DDRIF_CTRL0, 0x33fff000);
	lcd_tcon_write(TCON_DDRIF_CTRL1, 0x300300);

	return 0;
}

int lcd_tcon_enable_t5(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct tcon_rmem_s *rmem = get_lcd_tcon_rmem();
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;
	if (!tcon_conf)
		return -1;
	if (!mm_table)
		return -1;

	lcd_vcbus_write(ENCL_VIDEO_EN, 0);

	/* step 1: tcon top */
	lcd_tcon_top_set_t5(pconf);

	/* step 2: tcon_core_reg_update */
	if (mm_table->core_reg_header) {
		if (mm_table->core_reg_header->block_ctrl == 0) {
			lcd_tcon_core_reg_set(tcon_conf, mm_table,
				mm_table->core_reg_table);
		}
	}

	/* step 3:  tcon data set */
	if (mm_table->version)
		lcd_tcon_data_set(pdrv, mm_table);

	/* step 4: tcon_top_output_set */
	lcd_tcon_write(TCON_OUT_CH_SEL0, 0x76543210);
	lcd_tcon_write(TCON_OUT_CH_SEL1, 0xba98);

	if (rmem)
		flush_cache(rmem->rsv_mem_paddr, rmem->rsv_mem_size);

	lcd_vcbus_write(ENCL_VIDEO_EN, 1);

	return 0;
}

int lcd_tcon_enable_t3(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_tcon_config_s *tcon_conf = get_lcd_tcon_config();
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	struct tcon_rmem_s *rmem = get_lcd_tcon_rmem();
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;
	if (!tcon_conf)
		return -1;
	if (!mm_table)
		return -1;

	lcd_vcbus_write(ENCL_VIDEO_EN, 0);

	/* step 1: tcon top */
	lcd_tcon_top_set_t5(pconf);

	/* step 2: tcon_core_reg_update */
	if (mm_table->core_reg_header) {
		if (mm_table->core_reg_header->block_ctrl == 0) {
			lcd_tcon_core_reg_set(tcon_conf, mm_table,
				mm_table->core_reg_table);
		}
	}

	/* step 3:  tcon data set */
	if (mm_table->version)
		lcd_tcon_data_set(pdrv, mm_table);

#ifdef CONFIG_AMLOGIC_TEE
	lcd_tcon_mem_tee_protect(0, 1);
#endif

	/* step 4: tcon_top_output_set */
	lcd_tcon_write(TCON_OUT_CH_SEL0, 0x76543210);
	lcd_tcon_write(TCON_OUT_CH_SEL1, 0xba98);

	if (rmem)
		flush_cache(rmem->rsv_mem_paddr, rmem->rsv_mem_size);

	lcd_vcbus_write(ENCL_VIDEO_EN, 1);

	return 0;
}

int lcd_tcon_disable_t5(struct aml_lcd_drv_s *pdrv)
{
	/* disable od ddr_if */
	lcd_tcon_setb(0x263, 0, 31, 1);
	mdelay(100);

	/* top reset */
	lcd_tcon_write(TCON_RST_CTRL, 0x003f);

	/* global reset tcon */
	lcd_reset_setb(RESET1_MASK, 0, 4, 1);
	lcd_reset_setb(RESET1_LEVEL, 0, 4, 1);
	udelay(1);
	lcd_reset_setb(RESET1_LEVEL, 1, 4, 1);
	udelay(2);
	LCDPR("reset tcon\n");

	return 0;
}

int lcd_tcon_disable_t3(struct aml_lcd_drv_s *pdrv)
{
	/* disable od ddr_if */
	lcd_tcon_setb(0x263, 0, 31, 1);
	mdelay(100);

	/* top reset */
	lcd_tcon_write(TCON_RST_CTRL, 0x003f);

	/* global reset tcon */
	lcd_reset_setb(RESETCTRL_RESET2_MASK, 0, 5, 1);
	lcd_reset_setb(RESETCTRL_RESET2_LEVEL, 0, 5, 1);
	udelay(1);
	lcd_reset_setb(RESETCTRL_RESET2_LEVEL, 1, 5, 1);
	udelay(2);
	LCDPR("reset tcon\n");

	return 0;
}
