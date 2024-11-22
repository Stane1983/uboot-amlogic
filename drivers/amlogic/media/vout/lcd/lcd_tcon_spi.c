// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
// #include <asm/arch/io.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_extern.h>
#include "lcd_common.h"
#include "lcd_tcon.h"

#define LCD_TCON_SPI_SIZE             (0x610)

static struct lcd_tcon_spi_s tcon_spi = {
	.block_cnt = 0,
	.init_flag = 0,

	.spi_block = NULL,

	.data_read = NULL,
	.data_conv = NULL,
};

struct lcd_tcon_spi_s *lcd_tcon_spi_get(void)
{
	return &tcon_spi;
}

static void lcd_tcon_spi_print(void)
{
	struct lcd_tcon_spi_block_s *spi_block;
	int i, j;

	if (tcon_spi.version == 0) {
		LCDPR("tcon_spi invalid for version 0\n");
		return;
	}

	printf("lcd_tcon_spi info:\n");
	printf("version           = %d\n", tcon_spi.version);
	printf("block_cnt         = %d\n", tcon_spi.block_cnt);
	printf("init_flag         = 0x%x\n", tcon_spi.init_flag);
	if (tcon_spi.init_flag == 0)
		return;
	for (i = 0; i < tcon_spi.block_cnt; i++) {
		spi_block = tcon_spi.spi_block[i];
		printf("spi_block %d:\n"
			"data_type       0x%02x\n"
			"data_index      %d\n"
			"data_flag       0x%08x\n"
			"spi_offset      0x%08x\n"
			"spi_size        0x%08x\n"
			"param_cnt       0x%08x\n",
			i, spi_block->data_type,
			spi_block->data_index, spi_block->data_flag,
			spi_block->spi_offset, spi_block->spi_size,
			spi_block->param_cnt);
		for (j = 0; j < spi_block->param_cnt; j++) {
			printf("param_%d         0x%08x\n",
			       j, spi_block->param[j]);
		}
	}
	printf("\n");
}

#ifdef CONFIG_AML_LCD_EXTERN
static int lcd_tcon_spi_ext_update_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_extern_dev_s *ext_dev)
{
	struct lcd_unifykey_header_s *header;
	unsigned char *buf = NULL, *p;
	unsigned int size, crc;

	if (!ext_dev) {
		LCDERR("%s: ext_dev is null\n", __func__);
		return -1;
	}

#ifdef CONFIG_CMD_INI
	buf = (unsigned char *)handle_lcd_ext_buf_get();
#endif
	if (!buf)
		return -1;

	/* write lcd_extern unifykey and driver init_on_table */
	p = buf + LCD_UKEY_EXT_INIT;
	memcpy(p, ext_dev->config.table_init_on, ext_dev->config.table_init_on_cnt);
	p += ext_dev->config.table_init_on_cnt;
	memcpy(p, ext_dev->config.table_init_off, ext_dev->config.table_init_off_cnt);

	header = (struct lcd_unifykey_header_s *)buf;

	/* update size & crc, then write to unifykey */
	size = LCD_UKEY_EXT_INIT + ext_dev->config.table_init_on_cnt +
		ext_dev->config.table_init_off_cnt;
	header->data_len = size;

	crc = (unsigned int)(cal_CRC32(0, &buf[4], (size - 4)));
	header->crc32 = crc;

	lcd_unifykey_write("lcd_extern", buf, size);
	if (is_ukey_in_param_mem())
		update_panel_param_to_kernel();

	return 0;
}

static int lcd_tcon_spi_ext_update_panel_param(struct aml_lcd_drv_s *pdrv,
		struct lcd_extern_dev_s *ext_dev)
{
	unsigned char *vaddr, *p;
	unsigned int size;
	char name[32];

	size = ext_dev->config.table_init_on_cnt + ext_dev->config.table_init_off_cnt + 8;
	sprintf(name, "panel%d_ext%d_init_table", 0, ext_dev->dev_index);
	vaddr = (unsigned char *)malloc(size);
	if (vaddr) {
		p = vaddr;
		*(u32 *)(p + 0) = ext_dev->config.table_init_on_cnt;
		*(u32 *)(p + 4) = ext_dev->config.table_init_off_cnt;
		p += 8;
		memcpy(p, ext_dev->config.table_init_on, ext_dev->config.table_init_on_cnt);
		p += ext_dev->config.table_init_on_cnt;
		memcpy(p, ext_dev->config.table_init_off, ext_dev->config.table_init_off_cnt);
		panel_param_mem_modify(vaddr, name, size);
		update_panel_param_to_kernel();
		memset(vaddr, 0, size);
		free(vaddr);
		vaddr = NULL;
	}

	return 0;
}

static int lcd_tcon_spi_ext_update_ini_param(struct aml_lcd_drv_s *pdrv,
		struct lcd_extern_dev_s *ext_dev)
{
#ifdef CONFIG_CMD_AML_MODEL
	void *inip, *psec;
	char *new_value;
	int buf_size, i, j, ret;

	inip = get_lcd_ini_parse_mem(pdrv->index);
	if (!inip)
		return -1;

	psec = lcd_ini_get_section(inip, "lcd_ext_Attr");
	if (!psec) {
		LCDERR("%s: not find lcd_ext_Attr\n", __func__);
		return -1;
	}

	buf_size = ext_dev->config.table_init_on_cnt * 5 + 16;
	new_value = (char *)malloc(buf_size);
	if (new_value) {
		memset(new_value, 0, buf_size);
		j = 0;
		for (i = 0; i < ext_dev->config.table_init_on_cnt; i++) {
			j += snprintf(new_value + j, (buf_size - j - 1),
					"0x%02x,", ext_dev->config.table_init_on[i]);
		}
		ret = lcd_ini_set_exist_single_key(inip, psec, "init_on", new_value);
		if (ret) {
			LCDERR("dev[%d]: init_on update ini parser failed\n",
				ext_dev->dev_index);
		}
		memset(new_value, 0, buf_size);
		free(new_value);

		lcd_ini_param_mem_save_update(inip, pdrv->index);
	}

	return 0;
#else
	return -1;
#endif
}

/* for ext_data, need update cmd table when compare */
static int lcd_tcon_spi_ext_cmp(struct aml_lcd_drv_s *pdrv, unsigned char index,
		struct lcd_tcon_spi_block_s *spi_block)
{
	struct lcd_extern_driver_s *edrv;
	struct lcd_extern_dev_s *edev;
	int dev_id, data_id;
	unsigned char *ori_table, *init_data;
	int n = 0, i = 0, k = 0, m = 0, offset_st = 0, ori_table_size = 0;
	int type, size;
	unsigned char next_type, multi_flag;
	unsigned char file_type = PANEL_FILE_INVILD;
	unsigned int temp;

	if (!spi_block->new_buf) {
		LCDERR("%s: new_buf is null\n", __func__);
		return -1;
	}

	dev_id = (index >> 8) & 0xff;
	data_id = index & 0xff;
	edrv = lcd_extern_get_driver(pdrv->index);
	edev = lcd_extern_get_dev(edrv, dev_id);
	if (!edrv || !edev)
		return 0;

	init_data = (unsigned char *)malloc(LCD_EXTERN_INIT_ON_MAX);
	if (!init_data) {
		LCDERR("%s: malloc init_data buf failed\n", __func__);
		return -1;
	}
	memset(init_data, 0, LCD_EXTERN_INIT_ON_MAX);

	ori_table = edev->config.table_init_on;
	ori_table_size = edev->config.table_init_on_cnt;
	while ((i + 1) < ori_table_size) {
		type = ori_table[i];
		size = ori_table[i + 1];
		if (type == 0xff)
			break;
		if ((i + 2 + size) > ori_table_size)
			break;
		if (type == LCD_EXT_CMD_TYPE_END)
			break;

		switch (type) {
		case LCD_EXT_CMD_TYPE_MULTI_CMD:
		case LCD_EXT_CMD_TYPE_MULTI_DFT_CMD:
			multi_flag = 1;
			next_type = ori_table[i + 3];
			offset_st = 4;
			break;
		case LCD_EXT_CMD_TYPE_CMD_MULTI:
		case LCD_EXT_CMD_TYPE_CMD2_MULTI:
		case LCD_EXT_CMD_TYPE_CMD3_MULTI:
		case LCD_EXT_CMD_TYPE_CMD4_MULTI:
			multi_flag = 1;
			next_type = ((ori_table[i + 3] << 4) | (type & 0xf));
			offset_st = 4;
			break;
		case LCD_EXT_CMD_TYPE_MULTI_LIST_UFR: //2byte frame rate
			if (size < 3) {
				LCDERR("%s: init_data multi_list ufr err\n", __func__);
				goto lcd_extern_data_cmp_end;
			}
			init_data[n + 0] = ori_table[i + 0];
			m = 0;
			for (k = 0; k < size; k += 3) {
				// id
				init_data[n + m + 2] = ori_table[i + k + 2];
				// fr min
				init_data[n + m + 3] = (ori_table[i + k + 3] >> 0) & 0xff;
				init_data[n + m + 4] = (ori_table[i + k + 3] >> 8) & 0xff;
				//fr max
				init_data[n + m + 5] = (ori_table[i + k + 4] >> 0) & 0xff;
				init_data[n + m + 6] = (ori_table[i + k + 4] >> 8) & 0xff;
				m += 5;
			}
			init_data[n + 1] = m;//new size
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_MULTI_LIST_FR:
			init_data[n + 0] = ori_table[i + 0];
			m = 0;
			for (k = 0; k < size; k += 3) {
				// id
				init_data[n + m + 2] = ori_table[i + k + 2];
				// fr min
				init_data[n + m + 3] = ori_table[i + k + 3] & 0xff;
				//fr max
				init_data[n + m + 4] = ori_table[i + k + 4] & 0xff;
				m += 3;
			}
			init_data[n + 1] = m;//new size
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_DELAY:
			temp = 0;
			for (k = 0; k < size; k++)
				temp += ori_table[i + 2 + k];
			init_data[n + 0] = ori_table[i + 0];
			init_data[n + 2] = (temp >> 0) & 0xff;
			init_data[n + 3] = (temp >> 8) & 0xff;
			init_data[n + 1] = 2;
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_WAIT_GPIO:
		case LCD_EXT_CMD_TYPE_GPIO:
			if (size < 3) {
				LCDERR("%s: init_data gpio err\n", __func__);
				goto lcd_extern_data_cmp_end;
			}
			init_data[n + 0] = ori_table[i + 0];//type
			init_data[n + 2] = ori_table[i + 2];//gpio id
			init_data[n + 3] = ori_table[i + 3];//gpio val
			init_data[n + 4] = (ori_table[i + 4] >> 0) & 0xff;//dly
			init_data[n + 5] = (ori_table[i + 4] >> 8) & 0xff;//dly
			init_data[n + 1] = 4;
			goto ext_bin_to_data_ok;
		default:
			multi_flag = 0;
			next_type = type;
			offset_st = 2;
			break;
		}

		if (multi_flag && size <= 3) {
			LCDPR("%s: parse multi error size:%d\n", __func__, size);
			goto lcd_extern_data_cmp_end;
		}

		//case LCD_EXT_CMD_TYPE_CMD_BIN2:
		//case LCD_EXT_CMD_TYPE_CMD_BIN:
		//case LCD_EXT_CMD_TYPE_CMD_BIN_DATA:
		if ((next_type & 0xf0) == LCD_EXT_CMD_TYPE_CMD)
			goto ext_origin_data;

		if (data_id != (next_type & 0xf))
			goto ext_origin_data;

							//normal  / multi
		init_data[n + 0] = ori_table[i + 0];	//type	  / type
		init_data[n + 1] = ori_table[i + 1];	//size	  / size
		if (size >= 1)
			init_data[n + 2] = ori_table[i + 2];//offset  / multi_id maybe
		if (size >= 2)
			init_data[n + 3] = ori_table[i + 3];//data  / next_type maybe
		if (size >= 3)
			init_data[n + 4] = ori_table[i + 4];//data  / offset maybe

		switch (next_type & 0xf0) {
		case LCD_EXT_CMD_TYPE_CMD_BIN_DATA: /* all data replace, reg_addr nonexistent */
			memcpy(&init_data[n + offset_st], spi_block->new_buf,
				spi_block->data_new_size);
			if (multi_flag)
				init_data[n + 1] = spi_block->data_new_size + 2;
			else
				init_data[n + 1] = spi_block->data_new_size;
			break;
		case LCD_EXT_CMD_TYPE_CMD_BIN: /* data with reg_addr auto fill 0x0 */
			memcpy(&init_data[n + offset_st + 1], spi_block->new_buf,
				spi_block->data_new_size);
			if (multi_flag) //multi_id sub_type
				init_data[n + 1] = spi_block->data_new_size + 2 + 1;
			else
				init_data[n + 1] = spi_block->data_new_size + 1; //offset
			break;
		case LCD_EXT_CMD_TYPE_CMD_BIN2: /* data with reg_addr, only replace i2c_data */
			if (multi_flag) {
				memcpy(&init_data[n + 5], spi_block->new_buf + ori_table[i + 4],
					spi_block->data_new_size - 3);
			} else {
				memcpy(&init_data[n + 3], spi_block->new_buf + ori_table[i + 2],
					spi_block->data_new_size - 1);
			}
			break;
		default:
			LCDPR("%s: error type:%x\n", __func__, next_type);
			goto ext_origin_data;
		}
		goto ext_bin_to_data_ok;

ext_origin_data:
		for (k = 0; k < size; k++)
			init_data[n + k] = ori_table[i + k];

ext_bin_to_data_ok:
		i += size + 2;
		n += init_data[n + 1] + 2;
	}

	if (ori_table_size == n) {
		if (memcmp(ori_table, init_data, ori_table_size) == 0)
			goto lcd_extern_data_cmp_end; //same data
	}

	memset(edev->config.table_init_on, 0, edev->config.table_init_on_cnt);
	free(edev->config.table_init_on);
	edev->config.table_init_on = (unsigned char *)malloc(n);
	if (!edev->config.table_init_on) {
		LCDERR("%s: malloc init_data buf failed\n", __func__);
		memset(init_data, 0, LCD_EXTERN_INIT_ON_MAX);
		free(init_data);
		return -1;
	}
	memcpy(edev->config.table_init_on, init_data, n);
	edev->config.table_init_on_cnt = n;
	memset(init_data, 0, LCD_EXTERN_INIT_ON_MAX);
	free(init_data);

	if (pdrv->config_load == LCD_CONFIG_FILE) {
		file_type = get_lcd_panel_file_type(pdrv->index);
		if (file_type == PANEL_FILE_JSON)
			lcd_tcon_spi_ext_update_panel_param(pdrv, edev);
		else if (file_type == PANEL_FILE_INI)
			lcd_tcon_spi_ext_update_ini_param(pdrv, edev);
	} else if (pdrv->config_load == LCD_CONFIG_UKEY) {
		lcd_tcon_spi_ext_update_ukey(pdrv, edev);
	}

	return 1; //diff data

lcd_extern_data_cmp_end:
	memset(init_data, 0, LCD_EXTERN_INIT_ON_MAX);
	free(init_data);
	return 0; //same data
}
#endif

static int lcd_tcon_spi_data_cmp(struct lcd_tcon_spi_block_s *spi_block,
				 unsigned char *cmp_buf)
{
	unsigned int raw_data_check;

	raw_data_check = cmp_buf[4] | (cmp_buf[5] << 8) |
			 (cmp_buf[6] << 16) | (cmp_buf[7] << 24);
	if (raw_data_check != spi_block->data_raw_check)
		return -1;

	return 0;
}

static int lcd_tcon_spi_data_load(struct aml_lcd_drv_s *pdrv)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
	unsigned int i, j, size, new_size;
	int ret;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s start\n", __func__);
	if (tcon_spi.version == 0)
		return 0;

	if (!mm_table)
		return -2;
	if (mm_table->version == 0)
		return 0;

	if (!tcon_spi.spi_block) {
		LCDERR("%s: spi_block buf is null\n", __func__);
		return -1;
	}

	if (!tcon_spi.data_read) {
		LCDERR("%s: data_read is null\n", __func__);
		return -1;
	}
	if (!tcon_spi.data_conv) {
		LCDERR("%s: data_conv is null\n", __func__);
		return -1;
	}

	for (i = 0; i < tcon_spi.block_cnt; i++) {
		switch (tcon_spi.spi_block[i]->data_type) {
		case LCD_TCON_DATA_BLOCK_TYPE_DEMURA_LUT:
		case LCD_TCON_DATA_BLOCK_TYPE_ACC_LUT:
			if (!mm_table->data_mem_vaddr) {
				LCDERR("%s %d: data_mem error\n", __func__, i);
				continue;
			}
			ret = tcon_spi.data_read(tcon_spi.spi_block[i]);
			if (ret)
				continue;

			j = tcon_spi.spi_block[i]->data_index;

			/* update tcon data buf */
			if (!mm_table->data_mem_vaddr[j]) {
				/* no default bin file exist */
				ret = tcon_spi.data_conv(tcon_spi.spi_block[i]);
				if (ret)
					continue;
				if (!tcon_spi.spi_block[i]->new_buf) {
					LCDERR("%s: spi_block[%d] new_buf is null\n",
					       __func__, i);
					continue;
				}
				/* note: all the tcon data buf size must align to 32byte */
				new_size = lcd_tcon_data_size_align(tcon_spi.spi_block[i]->data_new_size);
				mm_table->data_mem_vaddr[j] = (unsigned char *)malloc(new_size);
				if (!mm_table->data_mem_vaddr[j]) {
					LCDERR("%s: Not enough memory\n",
					       __func__);
					continue;
				}
				memset(mm_table->data_mem_vaddr[j], 0, new_size);
				memcpy(mm_table->data_mem_vaddr[j],
				       tcon_spi.spi_block[i]->new_buf,
				       tcon_spi.spi_block[i]->data_new_size);
			} else {
				ret = lcd_tcon_spi_data_cmp(tcon_spi.spi_block[i],
							    mm_table->data_mem_vaddr[j]);
				if (ret == 0)
					continue;

				ret = tcon_spi.data_conv(tcon_spi.spi_block[i]);
				if (ret) {
					free(mm_table->data_mem_vaddr[j]);
					mm_table->data_mem_vaddr[j] = NULL;
					LCDERR("%s: block_data[%d] disabled\n",
						__func__, i);
					continue;
				}
				if (!tcon_spi.spi_block[i]->new_buf) {
					LCDERR("%s: spi_block[%d] new_buf is null\n",
					       __func__, i);
					continue;
				}
				size = mm_table->data_mem_vaddr[j][8] |
				       (mm_table->data_mem_vaddr[j][9] << 8) |
				       (mm_table->data_mem_vaddr[j][10] << 16) |
				       (mm_table->data_mem_vaddr[j][11] << 24);
				if (tcon_spi.spi_block[i]->data_new_size > size) {
					LCDERR("%s: block_data[%d] size is not match\n",
					       __func__, i);
					continue;
				}
				new_size = lcd_tcon_data_size_align(size);
				memset(mm_table->data_mem_vaddr[j], 0, new_size);
				memcpy(mm_table->data_mem_vaddr[j],
				       tcon_spi.spi_block[i]->new_buf,
				       tcon_spi.spi_block[i]->data_new_size);
			}
			break;
		case LCD_TCON_DATA_BLOCK_TYPE_EXT: /* pmu */
#ifdef CONFIG_AML_LCD_EXTERN
			j = tcon_spi.spi_block[i]->data_index & 0xff;
			ret = tcon_spi.data_read(tcon_spi.spi_block[i]);
			if (ret)
				continue;
			ret = tcon_spi.data_conv(tcon_spi.spi_block[i]);
			if (ret)
				continue;
			if (!tcon_spi.spi_block[i]->new_buf) {
				LCDERR("%s: spi_block[%d] new_buf is null\n",
				       __func__, i);
				continue;
			}
			lcd_tcon_spi_ext_cmp(pdrv, j, tcon_spi.spi_block[i]);
#endif
			break;
		default:
			break;
		}
	}

	for (i = 0; i < tcon_spi.block_cnt; i++) {
		if (tcon_spi.spi_block[i]->param) {
			free(tcon_spi.spi_block[i]->param);
			tcon_spi.spi_block[i]->param = NULL;
		}
		if (tcon_spi.spi_block[i]->raw_buf) {
			free(tcon_spi.spi_block[i]->raw_buf);
			tcon_spi.spi_block[i]->raw_buf = NULL;
		}
		if (tcon_spi.spi_block[i]->temp_buf) {
			free(tcon_spi.spi_block[i]->temp_buf);
			tcon_spi.spi_block[i]->temp_buf = NULL;
		}
		if (tcon_spi.spi_block[i]->new_buf) {
			free(tcon_spi.spi_block[i]->new_buf);
			tcon_spi.spi_block[i]->new_buf = NULL;
		}
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s ok\n", __func__);
	return 0;
}

#if defined(CONFIG_AML_LCD_JSON) || defined(CONFIG_CMD_AML_MODEL)
static int lcd_tcon_spi_data_save(unsigned char *buf)
{
	unsigned int raw_crc32, temp_crc32;
	int data_size, key_len = 0;
	unsigned char *ukey_buf = NULL;
	int ret;

	ret = lcd_unifykey_get_size("lcd_tcon_spi", &key_len);
	if (ret)
		goto lcd_tcon_spi_data_save_next;
	ukey_buf = (unsigned char *)malloc(key_len);
	if (!ukey_buf) {
		LCDERR("%s: malloc ukey_buf failed\n", __func__);
		return -1;
	}
	memset(ukey_buf, 0, key_len);

	ret = lcd_unifykey_get("lcd_tcon_spi", ukey_buf, key_len);
	if (ret)
		goto lcd_tcon_spi_data_save_next;

	raw_crc32 = ukey_buf[0] | (ukey_buf[1] << 8) | (ukey_buf[2] << 16) | (ukey_buf[3] << 24);
	temp_crc32 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	if (raw_crc32 == temp_crc32) {
		memset(ukey_buf, 0, key_len);
		free(ukey_buf);
		return 0;
	}

lcd_tcon_spi_data_save_next:
	data_size = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
	lcd_unifykey_write_force("lcd_tcon_spi", buf, data_size);
	if (ukey_buf) {
		memset(ukey_buf, 0, key_len);
		free(ukey_buf);
	}
	return 0;
}
#endif

#ifdef CONFIG_AML_LCD_JSON
static int lcd_tcon_spi_data_load_json(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_spi_header_s *header;
	unsigned char *tcon_spi, *p;
	unsigned int val, data_size, block_cnt, param_cnt;
	__maybe_unused struct json_s *parent, *child, *child2 = NULL;
	struct json_parse_s *jsp;
	unsigned int i, j, k, n;

	jsp = get_panel_jsp(0);
	if (!json_parse_ok(jsp)) {
		LCDPR("%s: lcd0 json not ready\n", __func__);
		return -1;
	}

	parent = json_path_to_node(jsp, jsp->root, "tcon/tcon_spi");
	if (!parent) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: not find /tcon/tcon_spi\n", __func__);
		return 0;
	}

	tcon_spi = (unsigned char *)malloc(LCD_TCON_SPI_SIZE);
	if (!tcon_spi) {
		LCDERR("%s: malloc memory error!\n", __func__);
		return -1;
	}
	memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
	header = (struct lcd_tcon_spi_header_s *)tcon_spi;

	header->version = json_get_obj_u32(jsp, parent, "version", 1);

	parent = json_get_object_child(jsp, parent, "block");
	if (!parent)
		return 0;
	block_cnt = json_get_array_size(jsp, parent);
	if (block_cnt <= 0) {
		memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
		free(tcon_spi);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: block_cnt 0, exit\n", __func__);
		return 0;
	}
	header->block_cnt = block_cnt;

	p = &tcon_spi[16];
	n = 0;
	for (i = 0; i < block_cnt; i++) {
		child = json_get_array_child(jsp, parent, i);
		if (!child) {
			LCDERR("%s: block_cnt[%d] error\n", __func__, i);
			memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
			free(tcon_spi);
			return -1;
		}

		val = json_get_obj_u32(jsp, child, "type", 0xff);
		p[n] = val & 0xff;
		p[n + 1] = (val >> 8) & 0xff;

		val = json_get_obj_u32(jsp, child, "index", 0xff);
		p[n + 2] = val & 0xff;
		p[n + 3] = (val >> 8) & 0xff;

		val = json_get_obj_u32(jsp, child, "flag", 0xff);
		p[n + 4] = val & 0xff;
		p[n + 5] = (val >> 8) & 0xff;
		p[n + 6] = (val >> 16) & 0xff;
		p[n + 7] = (val >> 24) & 0xff;

		val = json_get_obj_u32(jsp, child, "offset", 0xff);
		p[n + 8] = val & 0xff;
		p[n + 9] = (val >> 8) & 0xff;
		p[n + 10] = (val >> 16) & 0xff;
		p[n + 11] = (val >> 24) & 0xff;

		val = json_get_obj_u32(jsp, child, "size", 0x0);
		p[n + 12] = val & 0xff;
		p[n + 13] = (val >> 8) & 0xff;
		p[n + 14] = (val >> 16) & 0xff;
		p[n + 15] = (val >> 24) & 0xff;

		param_cnt = 0;
		child2 = json_get_object_child(jsp, child, "param");
		if (child2)
			param_cnt = json_get_array_size(jsp, child2);

		/* conversion parameters */
		k = n + 20;
		for (j = 0; j < param_cnt; j++) {
			val = json_get_arr_u32(jsp, child2, j, 0);
			p[k] = val & 0xff;
			p[k + 1] = (val >> 8) & 0xff;
			p[k + 2] = (val >> 16) & 0xff;
			p[k + 3] = (val >> 24) & 0xff;
			k += 4;
		}
		n += (20 + param_cnt * 4);
	}

	/* data size */
	data_size = 16 + n;
	header->data_size = data_size;

	/* crc */
	header->crc32 = cal_CRC32(0, (tcon_spi + 4), data_size - 4);

	lcd_tcon_spi_data_save(tcon_spi);
	memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
	free(tcon_spi);

	return 0;
}
#else
static inline int lcd_tcon_spi_data_load_json(struct aml_lcd_drv_s *pdrv)
{
	return -1;
}
#endif

#ifdef CONFIG_CMD_AML_MODEL
static int handle_tcon_spi_ini_v0(void *inip, void *psec, unsigned char *buff)
{
	struct lcd_tcon_spi_header_s *header;
	char key_name[32];
	unsigned int null_cnt = 0, block_cnt = 4, data_size = 0;
	unsigned int val, i, j, n;

	if (!inip || !psec || !buff)
		return -1;

	/* header */
	/* version 0, default */

	/* block 0: demura_lut */
	n = 16;
	val = lcd_ini_get_val(inip, psec, "demura_lut_offset", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	val = lcd_ini_get_val(inip, psec, "demura_lut_size", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(key_name, "block0_param_%d", j);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (val >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_1:
	/* block 1: p_gamma */
	val = lcd_ini_get_val(inip, psec, "p_gamma_offset", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	val = lcd_ini_get_val(inip, psec, "p_gamma_size", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(key_name, "block1_param_%d", j);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (val >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_2:
	/* block 2: acc_lut */
	val = lcd_ini_get_val(inip, psec, "acc_lut_offset", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	val = lcd_ini_get_val(inip, psec, "acc_lut_size", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(key_name, "block2_param_%d", j);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (val >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_3:
	/* block 3: auto_flicker */
	val = lcd_ini_get_val(inip, psec, "auto_flicker_offset", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	val = lcd_ini_get_val(inip, psec, "auto_flicker_size", 0);
	if (val == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	for (i = 0; i < 4; i++)
		buff[n + i] = (val >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(key_name, "block3_param_%d", j);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (val >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_next:
	if (null_cnt >= 4) {
		block_cnt = 0;
		data_size = 0;
	} else {
		block_cnt = 4;
		data_size = (16 + 32 * block_cnt);
	}

	header = (struct lcd_tcon_spi_header_s *)buff;
	header->block_cnt = block_cnt;
	header->data_size = data_size;
	/* crc */
	header->crc32 = cal_CRC32(0, (buff + 4), data_size - 4);

	return lcd_tcon_spi_data_save(buff);
}

static int lcd_tcon_spi_data_load_ini(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_spi_header_s *header;
	void *inip, *psec;
	unsigned char *tcon_spi, *p;
	char key_name[32];
	unsigned int data_size, block_cnt, param_cnt;
	unsigned int val, i, j, k, n;
	int ret;

	inip = get_lcd_ini_parse_mem(pdrv->index);
	if (!inip)
		return -1;

	psec = lcd_ini_get_section(inip, "tcon_spi_Attr");
	if (!psec)
		return 0;
	LCDPR("%s: find tcon_spi_Attr\n", __func__);

	tcon_spi = (unsigned char *)malloc(LCD_TCON_SPI_SIZE);
	if (!tcon_spi) {
		LCDERR("%s: malloc memory error!\n", __func__);
		return -1;
	}
	memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
	header = (struct lcd_tcon_spi_header_s *)tcon_spi;

	/* header */
	/* version */
	header->version = lcd_ini_get_val(inip, psec, "version", 0);
	if (header->version == 0) {
		ret = handle_tcon_spi_ini_v0(inip, psec, tcon_spi);
		memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
		free(tcon_spi);
		return ret;
	}

	/* new data format */
	/* block cnt */
	block_cnt = lcd_ini_get_val(inip, psec, "block_cnt", 0);
	header->block_cnt = block_cnt;

	p = &tcon_spi[16];
	n = 0;
	for (i = 0; i < block_cnt; i++) {
		snprintf(key_name, 31, "block%d_data_type", i);
		val = lcd_ini_get_val(inip, psec, key_name, 0xff);
		p[n] = val & 0xff;
		p[n + 1] = (val >> 8) & 0xff;

		snprintf(key_name, 31, "block%d_data_index", i);
		val = lcd_ini_get_val(inip, psec, key_name, 0xff);
		p[n + 2] = val & 0xff;
		p[n + 3] = (val >> 8) & 0xff;

		snprintf(key_name, 31, "block%d_data_flag", i);
		val = lcd_ini_get_val(inip, psec, key_name, 0xff);
		p[n + 4] = val & 0xff;
		p[n + 5] = (val >> 8) & 0xff;
		p[n + 6] = (val >> 16) & 0xff;
		p[n + 7] = (val >> 24) & 0xff;

		snprintf(key_name, 31, "block%d_spi_data_offset", i);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		p[n + 8] = val & 0xff;
		p[n + 9] = (val >> 8) & 0xff;
		p[n + 10] = (val >> 16) & 0xff;
		p[n + 11] = (val >> 24) & 0xff;

		snprintf(key_name, 31, "block%d_spi_data_size", i);
		val = lcd_ini_get_val(inip, psec, key_name, 0);
		p[n + 12] = val & 0xff;
		p[n + 13] = (val >> 8) & 0xff;
		p[n + 14] = (val >> 16) & 0xff;
		p[n + 15] = (val >> 24) & 0xff;

		snprintf(key_name, 31, "block%d_param_cnt", i);
		param_cnt = lcd_ini_get_val(inip, psec, key_name, 0);
		p[n + 16] = param_cnt & 0xff;
		p[n + 17] = (param_cnt >> 8) & 0xff;
		p[n + 18] = (param_cnt >> 16) & 0xff;
		p[n + 19] = (param_cnt >> 24) & 0xff;

		/* conversion parameters */
		k = n + 20;
		for (j = 0; j < param_cnt; j++) {
			snprintf(key_name, 31, "block%d_param_%d", i, j);
			val = lcd_ini_get_val(inip, psec, key_name, 0);
			p[k] = val & 0xff;
			p[k + 1] = (val >> 8) & 0xff;
			p[k + 2] = (val >> 16) & 0xff;
			p[k + 3] = (val >> 24) & 0xff;
			k += 4;
		}
		n += (20 + param_cnt * 4);
	}

	/* data size */
	data_size = 16 + n;
	header->block_cnt = data_size;

	/* crc */
	header->crc32 = cal_CRC32(0, (tcon_spi + 4), data_size - 4);

	lcd_tcon_spi_data_save(tcon_spi);
	memset(tcon_spi, 0, LCD_TCON_SPI_SIZE);
	free(tcon_spi);

	return 0;
}
#else
static inline int lcd_tcon_spi_data_load_ini(struct aml_lcd_drv_s *pdrv)
{
	return -1;
}
#endif

static int lcd_tcon_spi_data_parse(void)
{
	unsigned char *para, *p;
	struct lcd_tcon_spi_header_s *spi_header;
	unsigned int i, j, n, block_size;
	int key_len, len, ret;

	if (tcon_spi.init_flag) /* already parsed */
		return 0;

	ret = lcd_unifykey_check_exist("lcd_tcon_spi");
	if (ret)
		return -1;
	ret = lcd_unifykey_get_size("lcd_tcon_spi", &key_len);
	if (ret)
		return -1;

	para = (unsigned char *)malloc(sizeof(unsigned char) * key_len);
	if (!para) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}

	memset(para, 0, (sizeof(unsigned char) * key_len));
	ret = lcd_unifykey_get("lcd_tcon_spi", para, key_len);
	if (ret)
		goto lcd_tcon_spi_data_parse_err0;

	/* check lcd_tcon_spi unifykey length */
	len = 16;
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret) {
		LCDERR("lcd_tcon_spi unifykey length is not correct\n");
		goto lcd_tcon_spi_data_parse_err0;
	}

	/* header: 16byte */
	spi_header = (struct lcd_tcon_spi_header_s *)para;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("lcd_tcon_spi unifykey header:\n");
		LCDPR("crc32             = 0x%08x\n", spi_header->crc32);
		LCDPR("data_size         = %d\n", spi_header->data_size);
		LCDPR("version           = %d\n", spi_header->version);
		LCDPR("block_cnt         = %d\n", spi_header->block_cnt);
	}
	tcon_spi.version = spi_header->version;
	tcon_spi.block_cnt = spi_header->block_cnt;
	if (tcon_spi.version == 0) {
		LCDPR("%s: version 0, exit\n", __func__);
		memset(para, 0, key_len);
		free(para);
		return 0;
	}
	if (tcon_spi.block_cnt == 0) {
		LCDERR("%s: block_cnt 0, exit\n", __func__);
		memset(para, 0, key_len);
		free(para);
		return 0;
	}
	if (tcon_spi.block_cnt > LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX) {
		LCDERR("%s: lcd_tcon_spi block_cnt %d out of support(max %d), limit to max\n",
		       __func__, tcon_spi.block_cnt,
		       LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX);
		tcon_spi.block_cnt = LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX;
	}

	len = LCD_UKEY_TCON_SPI_HEAD_SIZE + LCD_UKEY_TCON_SPI_BLOCK_SIZE_PRE;
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret) {
		LCDERR("lcd_tcon_spi unifykey length is not correct\n");
		goto lcd_tcon_spi_data_parse_err0;
	}

	tcon_spi.spi_block = (struct lcd_tcon_spi_block_s **)malloc
		(tcon_spi.block_cnt * sizeof(struct lcd_tcon_spi_block_s *));
	if (!tcon_spi.spi_block) {
		LCDERR("failed to alloc tcon_spi\n");
		goto lcd_tcon_spi_data_parse_err0;
	}
	memset(tcon_spi.spi_block, 0,
	       (tcon_spi.block_cnt * sizeof(struct lcd_tcon_spi_block_s *)));

	len = LCD_UKEY_TCON_SPI_HEAD_SIZE;
	p = para + len;
	for (i = 0; i < tcon_spi.block_cnt; i++) {
		tcon_spi.spi_block[i] = (struct lcd_tcon_spi_block_s *)malloc
			(sizeof(struct lcd_tcon_spi_block_s));
		if (!tcon_spi.spi_block[i]) {
			LCDERR("failed to alloc tcon_spi_block\n");
			for (j = 0; j < i; j++) {
				free(tcon_spi.spi_block[j]);
				tcon_spi.spi_block[j] = NULL;
			}
			goto lcd_tcon_spi_data_parse_err1;
		}
		memset(tcon_spi.spi_block[i], 0, sizeof(struct lcd_tcon_spi_block_s));
		memcpy(tcon_spi.spi_block[i], p, LCD_UKEY_TCON_SPI_BLOCK_SIZE_PRE);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("lcd_tcon_spi block %d:\n", i);
			LCDPR("  data_type         = 0x%02x\n",
			      tcon_spi.spi_block[i]->data_type);
			LCDPR("  data_index        = %d\n",
			      tcon_spi.spi_block[i]->data_index);
			LCDPR("  data_flag         = %d\n",
			      tcon_spi.spi_block[i]->data_flag);
			LCDPR("  spi_offset        = 0x%08x\n",
			      tcon_spi.spi_block[i]->spi_offset);
			LCDPR("  spi_size          = 0x%08x\n",
			      tcon_spi.spi_block[i]->spi_size);
			LCDPR("  param_cnt         = %d\n",
			      tcon_spi.spi_block[i]->param_cnt);
		}

		block_size = LCD_UKEY_TCON_SPI_BLOCK_SIZE_PRE +
			     tcon_spi.spi_block[i]->param_cnt * 4;
		len += block_size;
		ret = lcd_unifykey_len_check(key_len, len);
		if (ret) {
			LCDERR("lcd_tcon_spi unifykey length is incorrect\n");
			goto lcd_tcon_spi_data_parse_err0;
		}

		if (tcon_spi.spi_block[i]->param_cnt > 0) {
			tcon_spi.spi_block[i]->param = (unsigned int *)malloc
				(tcon_spi.spi_block[i]->param_cnt * sizeof(unsigned int));
			if (!tcon_spi.spi_block[i]->param) {
				LCDERR("failed to alloc spi_block[%d] param\n", i);
				for (j = 0; j <= i; j++) {
					free(tcon_spi.spi_block[j]);
					tcon_spi.spi_block[j] = NULL;
				}
				goto lcd_tcon_spi_data_parse_err1;
			}
			memset(tcon_spi.spi_block[i]->param, 0,
			       tcon_spi.spi_block[i]->param_cnt * sizeof(unsigned int));
			n = LCD_UKEY_TCON_SPI_BLOCK_SIZE_PRE;
			for (j = 0; j < tcon_spi.spi_block[i]->param_cnt; j++) {
				tcon_spi.spi_block[i]->param[j] = p[n] |
							(p[n + 1] << 8) |
							(p[n + 2] << 16) |
							(p[n + 3] << 24);
				n += 4;
			}
		}
		p += block_size;
	}

	tcon_spi.init_flag = 1;

	memset(para, 0, key_len);
	free(para);
	return 0;

lcd_tcon_spi_data_parse_err1:
	free(tcon_spi.spi_block);
	tcon_spi.spi_block = NULL;
lcd_tcon_spi_data_parse_err0:
	memset(para, 0, key_len);
	free(para);
	return -1;
}

int lcd_tcon_spi_data_probe(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	if (pdrv->config_load == LCD_CONFIG_FILE) {
		if (get_lcd_panel_file_type(pdrv->index) == PANEL_FILE_JSON)
			lcd_tcon_spi_data_load_json(pdrv);
		else if (get_lcd_panel_file_type(pdrv->index) == PANEL_FILE_INI)
			lcd_tcon_spi_data_load_ini(pdrv);
	}
	ret = lcd_tcon_spi_data_parse();
	if (ret)
		return -1;

	pdrv->tcon_spi_print = lcd_tcon_spi_print;
	pdrv->tcon_spi_data_load = lcd_tcon_spi_data_load;

	return 0;
}
