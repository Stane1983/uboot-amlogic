// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
// #include <asm/arch/io.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_common.h"
#include "lcd_tcon.h"

static struct lcd_tcon_spi_s tcon_spi = {
	.block_cnt = 0,
	.init_flag = 0,

	.spi_block = NULL,
	.ext_buf = NULL,

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
static int lcd_tcon_spi_ext_update(struct lcd_extern_dev_s *ext_dev)
{
	struct lcd_unifykey_header_s *header;
	unsigned char *buf;
	unsigned int size, crc;

	if (!ext_dev) {
		LCDERR("%s: ext_dev is null\n", __func__);
		return -1;
	}

	buf = tcon_spi.ext_buf;

	/* write lcd_extern unifykey and driver init_on_table */
	memcpy(ext_dev->config.table_init_on,
	       &buf[LCD_UKEY_EXT_INIT],
	       tcon_spi.ext_init_on_cnt);
	ext_dev->config.table_init_on_cnt = tcon_spi.ext_init_on_cnt;

	header = (struct lcd_unifykey_header_s *)buf;

	/* update size & crc, then write to unifykey */
	size = LCD_UKEY_EXT_INIT + tcon_spi.ext_init_on_cnt +
		tcon_spi.ext_init_off_cnt;
	header->data_len = size;

	crc = (unsigned int)(lcd_crc32(0, &buf[4], (size - 4)));
	header->crc32 = crc;

	lcd_unifykey_write("lcd_extern", buf, size);
	if (is_ukey_in_param_mem())
		update_panel_param_to_kernel();

	return 0;
}

static int lcd_tcon_spi_ext_update_panel_param(struct lcd_extern_dev_s *ext_dev, int dev_index)
{
	unsigned char *vaddr, *p;
	unsigned int size;
	char name[32];

	size = tcon_spi.ext_init_on_cnt + tcon_spi.ext_init_off_cnt;
	sprintf(name, "panel%d_ext%d_init_table", 0, dev_index);
	vaddr = (unsigned char *)malloc(size + 8);
	if (vaddr) {
		p = vaddr;
		*(u32 *)(p + 0) = tcon_spi.ext_init_on_cnt;
		*(u32 *)(p + 4) = tcon_spi.ext_init_off_cnt;
		p += 8;
		memcpy(p, &tcon_spi.ext_buf[LCD_UKEY_EXT_INIT], size);
		panel_param_mem_modify(vaddr, name, size + 8);
		update_panel_param_to_kernel();
		memset(vaddr, 0, size + 8);
		free(vaddr);
		vaddr = NULL;
	}

	return 0;
}

static int lcd_tcon_spi_update_data(struct lcd_tcon_spi_block_s *spi_block,
				    unsigned char *cmp_buf, int flag, int i,
				    unsigned int offset, unsigned int data_len)
{
	if (flag) {
		if (spi_block->data_new_size > offset + data_len) {
			memcpy(&cmp_buf[i + 2], &spi_block->new_buf[offset],
			       data_len - 1);
			spi_block->new_buf[0] = data_len;
		}
	} else {
		memcpy(&cmp_buf[i + 1], spi_block->new_buf,
		       spi_block->data_new_size);
	}

	return 0;
}

/* for ext_data, need update cmd table when compare */
static int lcd_tcon_spi_ext_cmp(unsigned char index,
				struct lcd_tcon_spi_block_s *spi_block)
{
	unsigned char *cmp_buf, *tmp_buf;
	unsigned int cmp_buf_size, ext_size;
	unsigned char type, cnt;
	int i = 0, j = 0, k;
	unsigned int offset = 0, data_len = 0;
	int flag = 0;

	if (!spi_block->new_buf) {
		LCDERR("%s: new_buf is null\n", __func__);
		return -1;
	}
	if (!tcon_spi.ext_buf) {
		LCDERR("%s: ext_buf is null\n", __func__);
		return -1;
	}

	ext_size = LCD_EXTERN_INIT_ON_MAX + LCD_EXTERN_INIT_OFF_MAX;
	tmp_buf = (unsigned char *)malloc((ext_size * sizeof(unsigned char)));
	if (!tmp_buf) {
		LCDERR("%s: failed to alloc tmp_buf\n", __func__);
		return -1;
	}
	memset(tmp_buf, 0, (ext_size * sizeof(unsigned char)));

	cmp_buf = &tcon_spi.ext_buf[LCD_UKEY_EXT_INIT];
	cmp_buf_size = tcon_spi.ext_init_on_cnt;
	while ((i + 1) < cmp_buf_size) {
		type = cmp_buf[i];
		cnt = cmp_buf[i + 1];
		if (type == 0xff)
			break;
		if ((i + 2 + cnt) > cmp_buf_size)
			break;
		if ((((type >> 4) & 0xf) == 0xb) ||
		    (((type >> 4) & 0xf) == 0xd) ||
		    (((type >> 4) & 0xf) == 0xa)) {
			if (index != (type & 0xf))
				goto lcd_tcon_spi_ext_cmp_next;
			if (((type >> 4) & 0xf) == 0xa) {
				flag = 1;
				data_len = cmp_buf[i + 1];
				offset = cmp_buf[i + 2];
			} else {
				j = i + cnt + 2;
			}

			k = cmp_buf_size - j + tcon_spi.ext_init_off_cnt;
			if (cnt == 0) {
				if (spi_block->new_buf[0]) { /* new cnt */
					/* save data behind */
					memcpy(tmp_buf, &cmp_buf[j], k);
					/* update current data */
					lcd_tcon_spi_update_data(spi_block,
								 cmp_buf, flag,
								 i, offset,
								 data_len);
					/* recover data behind */
					j = i + spi_block->new_buf[0] + 2;
					memcpy(&cmp_buf[j], tmp_buf, k);
					tcon_spi.ext_init_on_cnt +=
						spi_block->new_buf[0];
					goto lcd_tcon_spi_ext_cmp_diff;
				}
				goto lcd_tcon_spi_ext_cmp_no_diff;
			}
			if (memcmp(&cmp_buf[i + 2], spi_block->new_buf,
				   spi_block->data_new_size)) {
				/* save data behind */
				memcpy(tmp_buf, &cmp_buf[j], k);
				/* update current data */
				lcd_tcon_spi_update_data(spi_block,
							 cmp_buf, flag,
							 i, offset,
							 data_len);
				/* recover data behind */
				j = i + spi_block->new_buf[0] + 2;
				memcpy(&cmp_buf[j], tmp_buf, k);
				tcon_spi.ext_init_on_cnt =
					tcon_spi.ext_init_on_cnt - cnt +
					spi_block->new_buf[0];
				goto lcd_tcon_spi_ext_cmp_diff;
			}
		}
lcd_tcon_spi_ext_cmp_next:
		i += (cnt + 2);
	}

lcd_tcon_spi_ext_cmp_no_diff:
	free(tmp_buf);
	return 0;

lcd_tcon_spi_ext_cmp_diff:
	free(tmp_buf);
	return -1;
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

static int lcd_tcon_spi_data_load(void)
{
	struct tcon_mem_map_table_s *mm_table = get_lcd_tcon_mm_table();
#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_dev_s *ext_dev = NULL;
	unsigned int ext_index;
	unsigned int ext_need_update = 0;
	unsigned char *p;
#endif
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
			if (!tcon_spi.ext_buf)
				break;
			ext_index = (tcon_spi.spi_block[i]->data_index >> 8) & 0xff;
			if (ext_index >= LCD_EXTERN_DEV_MAX) {
				LCDERR("%s: invalid ext device index %d for tcon data\n",
				       __func__, ext_index);
				break;
			}
			if (ext_dev) {
				if (ext_dev->config.index != ext_index) {
					LCDERR("%s: don't support multi ext device for tcon data\n",
					       __func__);
					continue;
				}
			} else {
				ext_dev = lcd_extern_get_dev(0, ext_index);
				if (!ext_dev)
					break;
			}
			if (get_lcd_panel_file_type(0) == PANEL_FILE_JSON) {
				p = tcon_spi.ext_buf + LCD_UKEY_EXT_INIT;
				memcpy(p, ext_dev->config.table_init_on,
				       ext_dev->config.table_init_on_cnt);
				p += ext_dev->config.table_init_on_cnt;
				memcpy(p, ext_dev->config.table_init_off,
				       ext_dev->config.table_init_off_cnt);
			}
			tcon_spi.ext_init_on_cnt = ext_dev->config.table_init_on_cnt;
			tcon_spi.ext_init_off_cnt = ext_dev->config.table_init_off_cnt;

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
			ret = lcd_tcon_spi_ext_cmp(j, tcon_spi.spi_block[i]);
			if (ret)
				ext_need_update = 1;
#endif
			break;
		default:
			break;
		}
	}
#ifdef CONFIG_AML_LCD_EXTERN
	if (ext_need_update) {
		if (get_lcd_panel_file_type(0) == PANEL_FILE_JSON)
			lcd_tcon_spi_ext_update_panel_param(ext_dev, ext_index);
		else
			lcd_tcon_spi_ext_update(ext_dev);
	}
#endif

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

#ifdef CONFIG_AML_LCD_JSON
static int lcd_tcon_spi_data_parse_json(void)
{
#ifdef CONFIG_AML_LCD_EXTERN
	unsigned int ext_size;
#endif
	unsigned int i, j,  block_size;
	unsigned int size;
	struct lcd_tcon_spi_block_s *blk;
	__maybe_unused struct json_s *parent, *child, *child2 = NULL;
	struct json_parse_s *jsp;

	jsp = get_panel_jsp(0);
	if (jsp->status != JSON_STATUS_OK) {
		LCDPR("panel 0 json not ready\n");
		return -1;
	}

	LCDPR("tcon spi parse from json\n");

	if (tcon_spi.init_flag) /* already parsed */
		return 0;

	parent = json_path_to_node(jsp, jsp->root, "tcon/tcon_spi");
	if (!parent) {
		LCDPR("can't find /tcon/tcon_spi\n");
		return 0;
	}

	tcon_spi.version = json_get_obj_u32(jsp, parent, "version", 1);

	parent = json_get_object_child(jsp, parent, "block");
	if (!parent)
		return 0;
	tcon_spi.block_cnt = json_get_array_size(jsp, parent);
	if (tcon_spi.block_cnt <= 0) {
		tcon_spi.block_cnt = 0;
		LCDERR("%s: block_cnt 0, exit\n", __func__);
		return 0;
	}

	if (tcon_spi.block_cnt > LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX) {
		LCDERR("%s: lcd_tcon_spi block_cnt %d out of support(max %d), limit to %d\n",
		       __func__, tcon_spi.block_cnt,
		       LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX,
		       LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX);
		tcon_spi.block_cnt = LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX;
	}

	size = tcon_spi.block_cnt * sizeof(struct lcd_tcon_spi_block_s *);
	tcon_spi.spi_block = (struct lcd_tcon_spi_block_s **)malloc(size);
	if (!tcon_spi.spi_block) {
		LCDERR("failed to alloc tcon_spi\n");
		goto lcd_tcon_spi_data_parse_err0;
	}
	memset(tcon_spi.spi_block, 0, size);

	block_size = sizeof(struct lcd_tcon_spi_block_s);
	for (i = 0; i < tcon_spi.block_cnt; i++) {
		child = json_get_array_child(jsp, parent, i);
		if (!child)
			return 0;

		blk = (struct lcd_tcon_spi_block_s *)malloc(block_size);
		if (!blk) {
			LCDERR("failed to alloc tcon_spi_block\n");
			for (j = 0; j < i; j++) {
				free(tcon_spi.spi_block[j]);
				tcon_spi.spi_block[j] = NULL;
			}
			goto lcd_tcon_spi_data_parse_err1;
		}
		tcon_spi.spi_block[i] = blk;

		memset(blk, 0, block_size);

		blk->data_type = json_get_obj_u32(jsp, child, "type", 0xff);
		blk->data_index = json_get_obj_u32(jsp, child, "index", 0xff);
		blk->data_flag = json_get_obj_u32(jsp, child, "flag", 0xff);
		blk->spi_offset = json_get_obj_u32(jsp, child, "offset", 0xff);
		blk->spi_size = json_get_obj_u32(jsp, child, "size", 0x0);
		blk->param_cnt = 0;
		child2 = json_get_object_child(jsp, child, "param");
		if (child2)
			blk->param_cnt = json_get_array_size(jsp, child2);

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("lcd_tcon_spi block %d:\n", i);
			LCDPR("data_type         = 0x%02x\n", blk->data_type);
			LCDPR("data_index        = %d\n", blk->data_index);
			LCDPR("data_flag         = %d\n", blk->data_flag);
			LCDPR("spi_offset        = 0x%08x\n", blk->spi_offset);
			LCDPR("spi_size          = 0x%08x\n", blk->spi_size);
			LCDPR("param_cnt         = %d\n", blk->param_cnt);
		}

		if (blk->param_cnt > 0) {
			blk->param = (u32 *)malloc(blk->param_cnt * sizeof(u32));
			if (!blk->param) {
				LCDERR("failed to alloc spi_block[%d] param\n", i);
				for (j = 0; j <= i; j++) {
					free(tcon_spi.spi_block[j]);
					tcon_spi.spi_block[j] = NULL;
				}
				goto lcd_tcon_spi_data_parse_err1;
			}
			memset(blk->param, 0, blk->param_cnt * sizeof(u32));
			for (j = 0; j < blk->param_cnt; j++)
				blk->param[j] = json_get_arr_u32(jsp, child2, i, 0);
		}

#ifdef CONFIG_AML_LCD_EXTERN
		ext_size = LCD_UKEY_EXT_INIT + LCD_EXTERN_INIT_ON_MAX + LCD_EXTERN_INIT_OFF_MAX;
		if (blk->data_type == LCD_TCON_DATA_BLOCK_TYPE_EXT && !tcon_spi.ext_buf) {
			tcon_spi.ext_buf = (unsigned char *)malloc(ext_size);
			if (!tcon_spi.ext_buf) {
				LCDERR("failed to alloc ext_buf\n");
				for (j = 0; j <= i; j++) {
					free(tcon_spi.spi_block[j]->raw_buf);
					tcon_spi.spi_block[j]->raw_buf = NULL;
					if (tcon_spi.spi_block[j]->param) {
						free(tcon_spi.spi_block[j]->param);
						tcon_spi.spi_block[j]->param = NULL;
					}
					free(tcon_spi.spi_block[j]);
					tcon_spi.spi_block[j] = NULL;
				}
				goto lcd_tcon_spi_data_parse_err1;
			}
			memset(tcon_spi.ext_buf, 0, ext_size);
		}
#endif
	}

	tcon_spi.init_flag = 1;

	return 0;

lcd_tcon_spi_data_parse_err1:
	free(tcon_spi.spi_block);
	tcon_spi.spi_block = NULL;
lcd_tcon_spi_data_parse_err0:
	return -1;
}
#else
static inline int lcd_tcon_spi_data_parse_json(void)
{
	return -1;
}
#endif

static int lcd_tcon_spi_data_parse(void)
{
	unsigned char *para, *p;
#ifdef CONFIG_AML_LCD_EXTERN
	unsigned int ext_size;
#ifdef CONFIG_CMD_INI
	unsigned char *data_buf = (unsigned char *)handle_lcd_ext_buf_get();
	struct lcd_unifykey_header_s *header;
#endif
#endif
	struct lcd_tcon_spi_unifykey_header_s *spi_header;
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
	spi_header = (struct lcd_tcon_spi_unifykey_header_s *)para;
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
		free(para);
		return 0;
	}
	if (tcon_spi.block_cnt == 0) {
		LCDERR("%s: block_cnt 0, exit\n", __func__);
		free(para);
		return 0;
	}
	if (tcon_spi.block_cnt > LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX) {
		LCDERR("%s: lcd_tcon_spi block_cnt %d out of support(max %d), limit to %d\n",
		       __func__, tcon_spi.block_cnt,
		       LCD_UKEY_TCON_SPI_BLOCK_CNT_MAX,
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
#ifdef CONFIG_AML_LCD_EXTERN
	ext_size = LCD_UKEY_EXT_INIT + LCD_EXTERN_INIT_ON_MAX +
			LCD_EXTERN_INIT_OFF_MAX;
#endif
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

#ifdef CONFIG_AML_LCD_EXTERN
		if (tcon_spi.spi_block[i]->data_type == LCD_TCON_DATA_BLOCK_TYPE_EXT &&
		    !tcon_spi.ext_buf) {
			tcon_spi.ext_buf = (unsigned char *)malloc
				((ext_size * sizeof(unsigned char)));
			if (!tcon_spi.ext_buf) {
				LCDERR("failed to alloc ext_buf\n");
				for (j = 0; j <= i; j++) {
					free(tcon_spi.spi_block[j]->raw_buf);
					tcon_spi.spi_block[j]->raw_buf = NULL;
					if (tcon_spi.spi_block[j]->param) {
						free(tcon_spi.spi_block[j]->param);
						tcon_spi.spi_block[j]->param = NULL;
					}
					free(tcon_spi.spi_block[j]);
					tcon_spi.spi_block[j] = NULL;
				}
				goto lcd_tcon_spi_data_parse_err1;
			}
			memset(tcon_spi.ext_buf, 0, (ext_size * sizeof(unsigned char)));
#ifdef CONFIG_CMD_INI
			if (data_buf) {
				header = (struct lcd_unifykey_header_s *)data_buf;
				if (header->data_len > ext_size) {
					LCDERR("%s: [%d] data_size %d out of support\n",
					       __func__, i, header->data_len);
				} else {
					memcpy(tcon_spi.ext_buf, data_buf, header->data_len);
				}
			}
#endif
		}
#endif
		p += block_size;
	}

	tcon_spi.init_flag = 1;

	free(para);
	return 0;

lcd_tcon_spi_data_parse_err1:
	free(tcon_spi.spi_block);
	tcon_spi.spi_block = NULL;
lcd_tcon_spi_data_parse_err0:
	free(para);
	return -1;
}

int lcd_tcon_spi_data_probe(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	if (pdrv->config_load == LCD_CONFIG_FILE) {
		if (get_lcd_panel_file_type(pdrv->index) == PANEL_FILE_JSON)
			ret = lcd_tcon_spi_data_parse_json();
		else
			ret = -1;//PANEL_FILE_INI todo
	} else {
		ret = lcd_tcon_spi_data_parse();
	}
	if (ret)
		return -1;

	pdrv->tcon_spi_print = lcd_tcon_spi_print;
	pdrv->tcon_spi_data_load = lcd_tcon_spi_data_load;

	return 0;
}
