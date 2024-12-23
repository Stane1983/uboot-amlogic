// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_extern.h>
#include "lcd_extern.h"
#include "../lcd_common.h"

struct lcd_extern_pmu_bin_s {
	char *name;
	char *path;
	int cnt;
	int name_len;
	int path_len;
};

static struct lcd_extern_pmu_bin_s ext_pmu_bins = {
	.name_len = 64,
	.path_len = 256,
};

#ifdef CONFIG_OF_LIBFDT
int lcd_extern_get_dts_child(char *dtaddr, char *snode, int index)
{
	int nodeoffset;
	char child_node[30];
	char *propdata;

	sprintf(child_node, "%s/extern_%d", snode, index);
	nodeoffset = fdt_path_offset(dtaddr, child_node);
	if (nodeoffset < 0) {
		EXTERR("dts: not find  node %s\n", child_node);
		return nodeoffset;
	}

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "index", NULL);
	if (!propdata) {
		EXTERR("get index failed, exit\n");
		return -1;
	}
	if (be32_to_cpup((u32 *)propdata) != index) {
		EXTERR("index not match, exit\n");
		return -1;
	}

	return nodeoffset;
}

static int lcd_extern_get_init_dts(char *dtaddr, struct lcd_extern_driver_s *edrv)
{
	int parent_offset;
	char *propdata, *p;
	const char *str;
	char snode[15];
	int i;

	if (edrv->index == 0)
		sprintf(snode, "/lcd_extern");
	else
		sprintf(snode, "/lcd%d_extern", edrv->index);

	parent_offset = fdt_path_offset(dtaddr, snode);
	if (parent_offset < 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTERR("not find %s node: %s\n",
			       snode, fdt_strerror(parent_offset));
		}
		return -1;
	}

	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "key_valid", NULL);
	if (!propdata) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("[%d]: failed to get key_valid\n", edrv->index);
		edrv->key_valid = 0;
	} else {
		edrv->key_valid = (unsigned char)(be32_to_cpup((u32 *)propdata));
	}

	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "i2c_bus", NULL);
	if (!propdata)
		edrv->i2c_bus = LCD_EXTERN_I2C_BUS_MAX;
	else
		edrv->i2c_bus = aml_lcd_i2c_bus_get_str(propdata);

	i = 0;
	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "extern_gpio_names", NULL);
	if (propdata) {
		EXTPR("[%d]: find extern_gpio_names\n", edrv->index);
		p = propdata;
		while (i < LCD_EXTERN_GPIO_NUM_MAX) {
			if (i > 0)
				p += strlen(p) + 1;
			str = p;
			if (strlen(str) == 0)
				break;
			strlcpy(edrv->gpio_name[i], str, LCD_CPU_GPIO_NAME_MAX);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				EXTPR("[%d]: gpio[%d]=%s\n",
				      edrv->index, i, edrv->gpio_name[i]);
			}
			i++;
		}
	}
	if (i < LCD_EXTERN_GPIO_NUM_MAX)
		strcpy(edrv->gpio_name[i], "invalid");

	propdata = (char *)fdt_getprop(dtaddr, parent_offset, "i2c_gpio_off", NULL);
	if (!propdata) {
		edrv->i2c_sck_gpio = LCD_EXT_GPIO_INVALID;
		edrv->i2c_sck_gpio_off = 2;
		edrv->i2c_sda_gpio = LCD_EXT_GPIO_INVALID;
		edrv->i2c_sda_gpio_off = 2;
	} else {
		edrv->i2c_sck_gpio = be32_to_cpup((u32 *)propdata);
		edrv->i2c_sck_gpio_off = be32_to_cpup((((u32 *)propdata) + 1));
		edrv->i2c_sda_gpio = be32_to_cpup((((u32 *)propdata) + 2));
		edrv->i2c_sda_gpio_off = be32_to_cpup((((u32 *)propdata) + 3));
	}

	return 0;
}
#endif

static int lcd_extern_get_init_bsp(struct lcd_extern_driver_s *edrv)
{
	struct lcd_extern_common_s *ext_common;
	char (*ext_gpio)[LCD_CPU_GPIO_NAME_MAX];
	int i, j;

	ext_common = edrv->data->dft_conf[edrv->index]->ext_common;
	edrv->key_valid = ext_common->key_valid;
	edrv->i2c_bus = ext_common->i2c_bus;

	i = 0;
	ext_gpio = edrv->data->dft_conf[edrv->index]->ext_common->ext_gpio;
	if (!ext_gpio) {
		EXTERR("[%d]: %s: ext_gpio is null\n", edrv->index, __func__);
		return -1;
	}
	while (i < LCD_EXTERN_GPIO_NUM_MAX) {
		if (strcmp(ext_gpio[i], "invalid") == 0)
			break;
		strlcpy(edrv->gpio_name[i], ext_gpio[i], LCD_CPU_GPIO_NAME_MAX);
		i++;
	}
	for (j = i; j < LCD_EXTERN_GPIO_NUM_MAX; j++)
		strcpy(edrv->gpio_name[j], "invalid");

	edrv->i2c_sck_gpio = ext_common->i2c_sck_gpio;
	edrv->i2c_sck_gpio_off = ext_common->i2c_sck_gpio_off;
	edrv->i2c_sda_gpio = ext_common->i2c_sda_gpio;
	edrv->i2c_sda_gpio_off = ext_common->i2c_sda_gpio_off;

	return 0;
}

static int lcd_extern_pinmux_load_from_bsp(struct lcd_extern_driver_s *edrv)
{
	struct lcd_pinmux_ctrl_s *pinmux;
	char propname[20];
	unsigned int i, j;
	int set_cnt = 0, clr_cnt = 0;

	if (!edrv->data)
		return -1;
	if (!edrv->data->dft_conf[edrv->index]) {
		EXTERR("[%d]: %s: dft_conf is NULL\n", edrv->index, __func__);
		return -1;
	}
	pinmux = edrv->data->dft_conf[edrv->index]->ext_common->ext_pinmux;
	if (!pinmux) {
		EXTERR("[%d]: %s: ext_pinmux is NULL\n", edrv->index, __func__);
		return -1;
	}

	sprintf(propname, "extern_on");
	for (i = 0; i < LCD_PINMX_MAX; i++) {
		if (!pinmux->name)
			break;
		if (strcmp(pinmux->name, "invalid") == 0)
			break;
		if (strcmp(pinmux->name, propname)) {
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				edrv->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
				edrv->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				edrv->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
				edrv->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		pinmux++;
	}
	if (set_cnt < LCD_PINMUX_NUM) {
		edrv->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		edrv->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		edrv->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		edrv->pinmux_clr[clr_cnt][1] = 0x0;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (edrv->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			EXTPR("pinmux set: %d, 0x%08x\n",
			      edrv->pinmux_set[i][0],
			      edrv->pinmux_set[i][1]);
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (edrv->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			EXTPR("pinmux clr: %d, 0x%08x\n",
			      edrv->pinmux_clr[i][0],
			      edrv->pinmux_clr[i][1]);
			i++;
		}
	}

	return 0;
}

static int lcd_extern_init_table_save(struct lcd_extern_config_s *extconf, int flag,
				      unsigned char *table)
{
	if (!extconf || !table) {
		EXTERR("%s: resource error\n", __func__);
		return -1;
	}
	if (flag) {
		if (extconf->table_init_on) {
			free(extconf->table_init_on);
			extconf->table_init_on = NULL;
		}
		extconf->table_init_on = (unsigned char *)malloc(extconf->table_init_on_cnt);
		if (!extconf->table_init_on) {
			EXTERR("%s: Not enough memory\n", __func__);
			return -1;
		}
		memcpy(extconf->table_init_on, table, extconf->table_init_on_cnt);
	} else {
		if (extconf->table_init_off) {
			free(extconf->table_init_off);
			extconf->table_init_off = NULL;
		}
		extconf->table_init_off = (unsigned char *)malloc(extconf->table_init_off_cnt);
		if (!extconf->table_init_off) {
			EXTERR("%s: Not enough memory\n", __func__);
			return -1;
		}
		memcpy(extconf->table_init_off, table, extconf->table_init_off_cnt);
	}

	return 0;
}

static int lcd_extern_init_dynamic_load_array(struct lcd_extern_driver_s *edrv,
					      struct lcd_extern_dev_s *edev,
					      unsigned int *buf, int max_len, int flag)
{
	struct lcd_extern_config_s *extconf = &edev->config;
	int i = 0, j, tbl_max, step = 0, ret = 0;
	unsigned char *table, type, size;
	char propname[20];

	if (flag) {
		extconf->table_init_on_cnt = 0;
		tbl_max = LCD_EXTERN_INIT_ON_MAX;
		sprintf(propname, "init_on");
	} else {
		extconf->table_init_off_cnt = 0;
		tbl_max = LCD_EXTERN_INIT_OFF_MAX;
		sprintf(propname, "init_off");
	}
	if (max_len == 0)
		return 0;

	table = (unsigned char *)malloc(tbl_max);
	if (!table) {
		EXTERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(table, 0, tbl_max);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		EXTPR("%s: %d: max_len=%d, tbl_max=%d\n",
		      __func__, flag, max_len, tbl_max);
	}

	switch (extconf->type) {
	case LCD_EXTERN_I2C:
	case LCD_EXTERN_SPI:
	case LCD_EXTERN_SIMPLE:
		while (1) {
			if ((i + 2) > max_len) {
				EXTERR("%s: %s: %s: no ending error\n",
				       __func__, extconf->name, propname);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			if ((i + 2) > tbl_max) {
				EXTERR("%s: %s: %s: size out of support (max %d)\n",
				       __func__, extconf->name, propname, tbl_max);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			table[i] = buf[i];
			table[i + 1] = buf[i + 1];
			type = table[i];
			size = table[i + 1];
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				EXTPR("[%d]: %s: dev[%d]: %s: step[%d]: type=0x%x, size=%d, i=%d\n",
				      edrv->index, __func__, edev->dev_index, propname,
				      step, type, size, i);
			}
			i += 2;

			if (type == LCD_EXT_CMD_TYPE_END)
				break;
			if (size == 0)
				goto init_dynamic_i2c_spi_array_next;
			if ((i + size) > max_len) {
				EXTERR("%s: %s size out of support (max_len %d)\n",
				       extconf->name, propname, max_len);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			if ((i + size) > tbl_max) {
				EXTERR("%s: %s: %s: size out of support (max %d)\n",
				       __func__, extconf->name, propname, tbl_max);
				goto lcd_extern_init_dynamic_load_array_err;
			}

			/* data */
			for (j = 0; j < size; j++)
				table[i + j] = buf[i + j];
			i += size;

init_dynamic_i2c_spi_array_next:
			step++;
		}
		if (flag)
			extconf->table_init_on_cnt = i;
		else
			extconf->table_init_off_cnt = i;
		break;
	case LCD_EXTERN_MIPI:
		while (1) {
			if ((i + 2) > max_len) {
				EXTERR("%s: get %s array: no ending error\n",
				       extconf->name, propname);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			if ((i + 2) > tbl_max) {
				EXTERR("%s: %s: %s: size out of support (max %d)\n",
				       __func__, extconf->name, propname, tbl_max);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			table[i] = buf[i];
			table[i + 1] = buf[i + 1];
			type = table[i];
			size = table[i + 1];
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				EXTPR("[%d]: %s(%d): dev[%d] step[%d]: type=0x%x, size=%d, i=%d\n",
				      edrv->index, __func__, flag, edev->dev_index,
				      step, type, size, i);
			}
			i += 2;

			if (type == LCD_EXT_CMD_TYPE_END) {
				if (size == 0xff || size == 0)
					break;
				size = 0;
			}
			if (size == 0)
				goto init_dynamic_mipi_array_next;
			if ((i + size) > max_len) {
				EXTERR("%s: %s size out of support (max_len %d)\n",
				       extconf->name, propname, max_len);
				goto lcd_extern_init_dynamic_load_array_err;
			}
			if ((i + size) > tbl_max) {
				EXTERR("%s: %s: %s: size out of support (max %d)\n",
				       __func__, extconf->name, propname, tbl_max);
				goto lcd_extern_init_dynamic_load_array_err;
			}

			for (j = 0; j < size; j++)
				table[i + j] = buf[i + j];
			i += size;

init_dynamic_mipi_array_next:
			step++;
		}
		if (flag)
			extconf->table_init_on_cnt = i;
		else
			extconf->table_init_off_cnt = i;
		break;
	default:
		if (flag)
			extconf->table_init_on_cnt = 0;
		else
			extconf->table_init_off_cnt = 0;
		goto lcd_extern_init_dynamic_load_array_err;
	}

	ret = lcd_extern_init_table_save(extconf, flag, table);
	if (ret)
		goto lcd_extern_init_dynamic_load_array_err;
	memset(table, 0, tbl_max);
	free(table);
	return 0;

lcd_extern_init_dynamic_load_array_err:
	memset(table, 0, tbl_max);
	free(table);
	return -1;
}

static int lcd_extern_init_fixed_load_array(struct lcd_extern_driver_s *edrv,
					    struct lcd_extern_dev_s *edev,
					    unsigned int *buf, int max_len, int flag)
{
	struct lcd_extern_config_s *extconf = &edev->config;
	unsigned char cmd_size;
	int i = 0, j, tbl_max, ret = 0;
	unsigned char *table;
	char propname[20];

	cmd_size = extconf->cmd_size;
	if (flag) {
		extconf->table_init_on_cnt = 0;
		tbl_max = LCD_EXTERN_INIT_ON_MAX;
		sprintf(propname, "init_on");
	} else {
		extconf->table_init_off_cnt = 0;
		tbl_max = LCD_EXTERN_INIT_OFF_MAX;
		sprintf(propname, "init_off");
	}
	if (max_len == 0)
		return 0;

	table = (unsigned char *)malloc(tbl_max);
	if (!table) {
		EXTERR("%s: Not enough memory\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		EXTPR("%s: %d: max_len=%d, tbl_max=%d\n",
		      __func__, flag, max_len, tbl_max);
	}

	while (1) {
		if ((i + cmd_size) > max_len) {
			EXTERR("%s: %s: no ending error\n", extconf->name, propname);
			goto lcd_extern_init_fixed_load_array_err;
		}
		if ((i + cmd_size) > tbl_max) {
			EXTERR("%s: %s: size out of support (max %d)\n",
			       extconf->name, propname, tbl_max);
			goto lcd_extern_init_fixed_load_array_err;
		}

		for (j = 0; j < cmd_size; j++)
			table[i + j] = buf[i + j];
		i += cmd_size;

		if (table[i] == LCD_EXT_CMD_TYPE_END)
			break;
	}

	if (flag)
		extconf->table_init_on_cnt = i;
	else
		extconf->table_init_off_cnt = i;
	ret = lcd_extern_init_table_save(extconf, flag, table);
	if (ret)
		goto lcd_extern_init_fixed_load_array_err;
	memset(table, 0, tbl_max);
	free(table);
	return 0;

lcd_extern_init_fixed_load_array_err:
	memset(table, 0, tbl_max);
	free(table);
	return -1;
}

__maybe_unused static unsigned char *lcd_ext_pmu_bin_get(char *tag_name, int *size)
{
	int i = 0;

	for (i = 0; i < ext_pmu_bins.cnt; i++) {
		if (strcmp(tag_name, ext_pmu_bins.name + i * ext_pmu_bins.name_len) == 0) {
			return read_file_to_buffer(ext_pmu_bins.path + i * ext_pmu_bins.path_len,
						   size);
		}
	}

	return NULL;
}

__maybe_unused static int lcd_extern_data_init_load(unsigned char *init_data, unsigned int *nums,
						    unsigned int num_cnt)
{
#define MAX_STR_LEN 64
	int n = 0, i = 0, k = 0, m = 0, offset_st = 0, bin_size = 0;
	int type, size, index;
	unsigned char next_type, multi_flag, multi_id;
	char path_tag[2][MAX_STR_LEN];
	unsigned char *bin = NULL;

	while (i < num_cnt) {
		type = nums[i];
		size = nums[i + 1];

		if (type == LCD_EXT_CMD_TYPE_END || (i + size >= LCD_EXTERN_INIT_ON_MAX)) {
			init_data[n] = LCD_EXT_CMD_TYPE_END;
			init_data[n + 1] = 0;
			n += 2;
			return n;
		}

		switch (type) {
		case LCD_EXT_CMD_TYPE_MULTI_CMD:
		case LCD_EXT_CMD_TYPE_MULTI_DFT_CMD:
			multi_flag = 1;
			multi_id = nums[i + 2];
			next_type = nums[i + 3];
			offset_st = 4;
			break;
		case LCD_EXT_CMD_TYPE_CMD_MULTI:
		case LCD_EXT_CMD_TYPE_CMD2_MULTI:
		case LCD_EXT_CMD_TYPE_CMD3_MULTI:
		case LCD_EXT_CMD_TYPE_CMD4_MULTI:
			multi_flag = 1;
			multi_id = nums[i + 2];
			next_type = ((nums[i + 3] << 4) | (type & 0xf));
			offset_st = 4;
			break;
		case LCD_EXT_CMD_TYPE_MULTI_LIST_UFR:
			init_data[n + 0] = nums[i + 0];
			m = 0;
			for (k = 0; k < size; k += 3) {
				// id
				init_data[n + m + 2] = nums[i + k + 2];
				// fr min
				init_data[n + m + 3] = (nums[i + k + 3] >> 0) & 0xff;
				init_data[n + m + 4] = (nums[i + k + 3] >> 8) & 0xff;
				//fr max
				init_data[n + m + 5] = (nums[i + k + 4] >> 0) & 0xff;
				init_data[n + m + 6] = (nums[i + k + 4] >> 8) & 0xff;
				m += 5;
			}
			init_data[n + 1] = m;//new size
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_MULTI_LIST_FR:
			init_data[n + 0] = nums[i + 0];
			m = 0;
			for (k = 0; k < size; k += 3) {
				// id
				init_data[n + m + 2] = nums[i + k + 2];
				// fr min
				init_data[n + m + 3] = nums[i + k + 3] & 0xff;
				//fr max
				init_data[n + m + 4] = nums[i + k + 4] & 0xff;
				m += 3;
			}
			init_data[n + 1] = m;//new size
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_DELAY:
			if (size != 1)
				return -1;
			init_data[n + 0] = nums[i + 0];
			init_data[n + 2] = (nums[i + 2] >> 0) & 0xff;
			init_data[n + 3] = (nums[i + 2] >> 8) & 0xff;
			init_data[n + 1] = 2;
			goto ext_bin_to_data_ok;
		case LCD_EXT_CMD_TYPE_WAIT_GPIO:
		case LCD_EXT_CMD_TYPE_GPIO:
			if (size < 3)
				return -1;
			init_data[n + 0] = nums[i + 0];//type
			init_data[n + 2] = nums[i + 2];//gpio id
			init_data[n + 3] = nums[i + 3];//gpio val
			init_data[n + 4] = (nums[i + 4] >> 0) & 0xff;//dly
			init_data[n + 5] = (nums[i + 4] >> 8) & 0xff;//dly
			init_data[n + 1] = 4;
			goto ext_bin_to_data_ok;
		default:
			multi_flag = 0;
			multi_id = 0xff;
			next_type = type;
			offset_st = 2;
			break;
		}

		if (multi_flag && size <= 3) {
			init_data[n] = LCD_EXT_CMD_TYPE_END;
			init_data[n + 1] = 0;
			n += 2;
			EXTPR("parse multi error size:%d\n", size);
			return n;
		}

		if ((next_type & 0xf0) == LCD_EXT_CMD_TYPE_CMD) {
			EXTPR("parse cmd no need replace\n");
			goto ext_origin_data;
		}

		index = next_type & 0xf;

		if (multi_flag) {
			snprintf(path_tag[0], MAX_STR_LEN, "TCON_EXT_B%d_%d_SPI_BIN_PATH",
				 index, multi_id);
			snprintf(path_tag[1], MAX_STR_LEN, "TCON_EXT_B%d_%d_BIN_PATH",
				 index, multi_id);
		} else {
			snprintf(path_tag[0], MAX_STR_LEN, "TCON_EXT_B%d_SPI_BIN_PATH", index);
			snprintf(path_tag[1], MAX_STR_LEN, "TCON_EXT_B%d_BIN_PATH", index);
		}

		for (k = 0; k < 2; k++) {
			bin = lcd_ext_pmu_bin_get(path_tag[k], &bin_size);
			if (bin)
				break;
		}

		if (!bin) {
			EXTPR("no pmu data bin find\n");
			goto ext_origin_data;
		}
						//normal  / multi
		init_data[n + 0] = nums[i + 0];	//type	  / type
		init_data[n + 1] = nums[i + 1];	//size	  / size
		if (size >= 1)
			init_data[n + 2] = nums[i + 2];//offset  / multi_id maybe
		if (size >= 2)
			init_data[n + 3] = nums[i + 3];//data	  / next_type maybe
		if (size >= 3)
			init_data[n + 4] = nums[i + 4];//data	  / offset maybe

		switch (next_type & 0xf0) {
		case LCD_EXT_CMD_TYPE_CMD_BIN_DATA:
			memcpy(&init_data[n + offset_st], bin, bin_size);
			if (multi_flag)
				init_data[n + 1] = bin_size + 2;
			else
				init_data[n + 1] = bin_size;
			break;
		case LCD_EXT_CMD_TYPE_CMD_BIN:
			memcpy(&init_data[n + offset_st + 1], bin, bin_size);
			if (multi_flag)
				init_data[n + 1] = bin_size + 2 + 1;//multi_id sub_type
			else
				init_data[n + 1] = bin_size + 1; //offset
			break;
		case LCD_EXT_CMD_TYPE_CMD_BIN2:
			if (multi_flag)
				memcpy(&init_data[n + 5], bin + nums[n + 4], size - 3);
			else
				memcpy(&init_data[n + 3], bin + nums[n + 2], size - 1);
			break;
		default:
			EXTPR("error type:%x\n", next_type);
			goto ext_origin_data;
		}
		if (bin) {
			memset(bin, 0, bin_size);
			free(bin);
			bin = NULL;
		}
		goto ext_bin_to_data_ok;

ext_origin_data:
		EXTPR("init_on bin to data failed, keep origin data\n");
		for (k = 0; k < size; k++)
			init_data[n + k] = nums[i + k];
ext_bin_to_data_ok:

		i += size + 2;
		n += init_data[n + 1] + 2;
	}
#undef MAX_STR_LEN
	return n;
}

#ifdef CONFIG_OF_LIBFDT
static int lcd_extern_init_table_handle_dts(struct lcd_extern_driver_s *edrv,
					    struct lcd_extern_dev_s *edev,
					    char *dtaddr, int nodeoffset)
{
	struct lcd_extern_config_s *extconf = &edev->config;
	int len_on, len_off, init_max, init_buf_size;
	unsigned int *init_buf;
	char *init_on, *init_off;
	int i = 0, ret;

	init_on = (char *)fdt_getprop(dtaddr, nodeoffset, "init_on", &len_on);
	if (!init_on) {
		EXTERR("%s: get init_on failed\n", extconf->name);
		return -1;
	}
	init_off = (char *)fdt_getprop(dtaddr, nodeoffset, "init_off", &len_off);
	if (!init_on) {
		EXTERR("%s: get init_off failed\n", extconf->name);
		return -1;
	}
	len_on /= 4;
	len_off /= 4;
	init_max = len_on >= len_off ? len_on : len_off;
	if (init_max <= 0)
		return 0;

	init_buf_size = init_max * sizeof(unsigned int);
	init_buf = (unsigned int *)malloc(init_buf_size);
	if (!init_buf) {
		EXTERR("%s: alloc memory error\n", __func__);
		return -1;
	}

	//init_on
	for (i = 0; i < len_on; i++)
		init_buf[i] = be32_to_cpup((((u32 *)init_on) + i));
	if (extconf->cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
		ret = lcd_extern_init_dynamic_load_array(edrv, edev, init_buf, len_on, 1);
	else
		ret = lcd_extern_init_fixed_load_array(edrv, edev, init_buf, len_on, 1);
	if (ret)
		goto lcd_extern_init_table_handle_dts_err;

	//init_off
	for (i = 0; i < len_off; i++)
		init_buf[i] = be32_to_cpup((((u32 *)init_off) + i));
	if (extconf->cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
		ret = lcd_extern_init_dynamic_load_array(edrv, edev, init_buf, len_off, 0);
	else
		ret = lcd_extern_init_fixed_load_array(edrv, edev, init_buf, len_off, 0);
	if (ret)
		goto lcd_extern_init_table_handle_dts_err;

	extconf->table_init_loaded = 1;

	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return 0;

lcd_extern_init_table_handle_dts_err:
	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return -1;
}

static int lcd_extern_get_config_dts(char *dtaddr, char *snode,
				     struct lcd_extern_driver_s *edrv,
				     struct lcd_extern_dev_s *edev)
{
	struct lcd_extern_config_s *extconf = &edev->config;
	int nodeoffset;
	char *propdata;
	const char *str;
	int ret = 0;

	extconf->table_init_loaded = 0;
	nodeoffset = lcd_extern_get_dts_child(dtaddr, snode, edev->dev_index);
	if (nodeoffset < 0)
		return -1;

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "index", NULL);
	if (!propdata) {
		extconf->index = LCD_EXTERN_INDEX_INVALID;
		EXTERR("get index failed, exit\n");
		return -1;
	}
	extconf->index = (unsigned char)(be32_to_cpup((u32 *)propdata));

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "extern_name", NULL);
	if (!propdata) {
		str = "invalid_name";
		strcpy(extconf->name, str);
		EXTERR("get extern_name failed\n");
	} else {
		memset(extconf->name, 0, LCD_EXTERN_NAME_LEN_MAX);
		strlcpy(extconf->name, propdata, LCD_EXTERN_NAME_LEN_MAX);
	}

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "status", NULL);
	if (!propdata) {
		EXTERR("get status failed, default to disabled\n");
		extconf->status = 0;
	} else {
		if (strncmp(propdata, "okay", 2) == 0)
			extconf->status = 1;
		else
			extconf->status = 0;
	}
	if (extconf->status == 0)
		return -1;

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "type", NULL);
	if (!propdata) {
		extconf->type = LCD_EXTERN_MAX;
		EXTERR("get type failed, exit\n");
		return -1;
	}
	extconf->type = be32_to_cpup((u32 *)propdata);

	EXTPR("[%d]: load dts config: dev[%d]: %s(%d), type: %d\n",
	      edrv->index, edev->dev_index, extconf->name, extconf->index, extconf->type);

	switch (extconf->type) {
	case LCD_EXTERN_I2C:
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "i2c_address", NULL);
		if (!propdata) {
			EXTERR("%s: get i2c_address failed, exit\n", extconf->name);
			extconf->i2c_addr = 0xff;
			return -1;
		}
		extconf->i2c_addr = (unsigned char)(be32_to_cpup((u32 *)propdata));

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: i2c_address=0x%02x\n", extconf->name, extconf->i2c_addr);

		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "i2c_address2", NULL);
		if (!propdata) {
			propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "i2c_second_address",
						       NULL);
			if (!propdata) {
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
					EXTPR("%s no i2c_address2 exist\n", extconf->name);
				extconf->i2c_addr2 = 0xff;
			} else {
				extconf->i2c_addr2 =
					(unsigned char)(be32_to_cpup((u32 *)propdata));
			}
		} else {
			extconf->i2c_addr2 = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: i2c_address2=0x%02x\n",
			      extconf->name, extconf->i2c_addr2);
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "i2c_address3", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				EXTPR("%s no i2c_address3 exist\n", extconf->name);
			extconf->i2c_addr3 = 0xff;
		} else {
			extconf->i2c_addr3 = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: i2c_address3=0x%02x\n", extconf->name, extconf->i2c_addr3);
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "i2c_address4", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				EXTPR("%s no i2c_address4 exist\n", extconf->name);
			extconf->i2c_addr4 = 0xff;
		} else {
			extconf->i2c_addr4 = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: i2c_address4=0x%02x\n", extconf->name, extconf->i2c_addr4);

		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "cmd_size", NULL);
		if (!propdata) {
			EXTPR("%s: no cmd_size\n", extconf->name);
			extconf->cmd_size = 0;
		} else {
			extconf->cmd_size = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: cmd_size=%d\n", extconf->name, extconf->cmd_size);
		if (extconf->cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_dts(edrv, edev, dtaddr, nodeoffset);
		break;
	case LCD_EXTERN_SPI:
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "gpio_spi_cs", NULL);
		if (!propdata) {
			EXTERR("%s: get gpio_spi_cs failed, exit\n", extconf->name);
			extconf->spi_gpio_cs = LCD_EXT_GPIO_INVALID;
			return -1;
		}
		extconf->spi_gpio_cs = (unsigned char)(be32_to_cpup((u32 *)propdata));

		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "gpio_spi_clk", NULL);
		if (!propdata) {
			EXTERR("%s: get gpio_spi_clk failed, exit\n", extconf->name);
			extconf->spi_gpio_clk = LCD_EXT_GPIO_INVALID;
			return -1;
		}
		extconf->spi_gpio_clk = (unsigned char)(be32_to_cpup((u32 *)propdata));

		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "gpio_spi_data", NULL);
		if (!propdata) {
			EXTERR("%s: get gpio_spi_data failed, exit\n", extconf->name);
			extconf->spi_gpio_data = LCD_EXT_GPIO_INVALID;
			return -1;
		}
		extconf->spi_gpio_data = (unsigned char)(be32_to_cpup((u32 *)propdata));

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("%s: gpio_spi cs=%d, clk=%d, data=%d\n",
			      extconf->name, extconf->spi_gpio_cs,
			      extconf->spi_gpio_clk, extconf->spi_gpio_data);
		}
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "spi_clk_freq", NULL);
		if (!propdata) {
			EXTERR("%s: get spi_clk_freq failed, default to %dKHz\n",
			       extconf->name, LCD_EXT_SPI_CLK_FREQ_DFT);
			extconf->spi_clk_freq = LCD_EXT_SPI_CLK_FREQ_DFT;
		} else {
			extconf->spi_clk_freq = be32_to_cpup((u32 *)propdata);
		}

		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "spi_clk_pol", NULL);
		if (!propdata) {
			EXTERR("%s: get spi_clk_pol failed, default to 1\n", extconf->name);
			extconf->spi_clk_pol = 1;
		} else {
			extconf->spi_clk_pol = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag) {
			EXTPR("%s: spi clk=%dKHz, clk_pol=%d\n",
			      extconf->name, extconf->spi_clk_freq,
			      extconf->spi_clk_pol);
		}
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "cmd_size", NULL);
		if (!propdata) {
			EXTPR("%s: no cmd_size\n", extconf->name);
			extconf->cmd_size = 0;
		} else {
			extconf->cmd_size = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag)
			EXTPR("%s: cmd_size=%d\n", extconf->name, extconf->cmd_size);
		if (extconf->cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_dts(edrv, edev, dtaddr, nodeoffset);
		break;
	case LCD_EXTERN_MIPI:
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "cmd_size", NULL);
		if (!propdata) {
			EXTPR("%s: no cmd_size\n", extconf->name);
			extconf->cmd_size = 0;
		} else {
			extconf->cmd_size = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: cmd_size=%d\n", extconf->name, extconf->cmd_size);
		if (extconf->cmd_size != LCD_EXT_CMD_SIZE_DYNAMIC)
			break;
		ret = lcd_extern_init_table_handle_dts(edrv, edev, dtaddr, nodeoffset);
		break;
	case LCD_EXTERN_SIMPLE:
		propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "cmd_size", NULL);
		if (!propdata) {
			EXTPR("%s: no cmd_size\n", extconf->name);
			extconf->cmd_size = 0;
		} else {
			extconf->cmd_size = (unsigned char)(be32_to_cpup((u32 *)propdata));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: cmd_size=%d\n", extconf->name, extconf->cmd_size);
		if (extconf->cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_dts(edrv, edev, dtaddr, nodeoffset);
		break;
	default:
		break;
	}

	return ret;
}
#endif

static int lcd_extern_init_table_handle_ukey(struct lcd_extern_driver_s *edrv,
					     struct lcd_extern_dev_s *edev,
					     unsigned char *p, int key_len)
{
	unsigned int *init_buf;
	int init_buf_size, init_offset, init_max;
	int i, ret;

	init_offset = LCD_UKEY_EXT_INIT;
	init_max = key_len - LCD_UKEY_EXT_INIT;
	if (init_max <= 0)
		return 0;

	init_buf_size = init_max * sizeof(unsigned int);
	init_buf = (unsigned int *)malloc(init_buf_size);
	if (!init_buf) {
		EXTERR("%s: alloc memory error\n", __func__);
		return -1;
	}
	for (i = 0; i < init_max; i++)
		init_buf[i] = *(p + init_offset + i);
	if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
		ret = lcd_extern_init_dynamic_load_array(edrv, edev, init_buf, init_max, 1);
	else
		ret = lcd_extern_init_fixed_load_array(edrv, edev, init_buf, init_max, 1);
	if (ret)
		goto lcd_extern_init_table_handle_ukey_err;

	init_offset += edev->config.table_init_on_cnt;
	init_max -= edev->config.table_init_on_cnt;
	if (init_max > 0) {
		for (i = 0; i < init_max; i++)
			init_buf[i] = *(p + init_offset + i);
		if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
			ret = lcd_extern_init_dynamic_load_array(edrv, edev, init_buf, init_max, 0);
		else
			ret = lcd_extern_init_fixed_load_array(edrv, edev, init_buf, init_max, 0);
		if (ret)
			goto lcd_extern_init_table_handle_ukey_err;
	} else {
		edev->config.table_init_off_cnt = 0;
	}

	edev->config.table_init_loaded = 1;

	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return 0;

lcd_extern_init_table_handle_ukey_err:
	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return -1;
}

static int lcd_extern_get_config_ukey(struct lcd_extern_driver_s *edrv,
				      struct lcd_extern_dev_s *edev, char *snode)
{
	unsigned char *para, *p;
	int key_len, len;
	const char *str;
	int ret;

	ret = lcd_unifykey_get_size(snode, &key_len);
	if (ret)
		return -1;
	para = (unsigned char *)malloc(key_len);
	if (!para) {
		EXTERR("[%d]: %s: Not enough memory\n", edrv->index, __func__);
		return -1;
	}
	memset(para, 0, key_len);
	ret = lcd_unifykey_get(snode, para, key_len);
	if (ret) {
		free(para);
		return -1;
	}

	/* check lcd_extern unifykey length */
	len = 10 + 33 + 10;
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret) {
		EXTERR("[%d]: unifykey length is not correct\n", edrv->index);
		free(para);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		lcd_unifykey_header_print(para);

	/* basic: 33byte */
	p = para;
	str = (const char *)(p + LCD_UKEY_HEAD_SIZE);
	strlcpy(edev->config.name, str, LCD_EXTERN_NAME_LEN_MAX);
	edev->config.index = *(p + LCD_UKEY_EXT_INDEX);
	edev->config.type = *(p + LCD_UKEY_EXT_TYPE);
	edev->config.status = *(p + LCD_UKEY_EXT_STATUS);

	if (edev->config.status == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("[%d]: dev[%d]: %s(%d) is disabled\n",
			      edrv->index, edev->dev_index,
			      edev->config.name, edev->config.index);
		}
		free(para);
		return -1;
	}

	EXTPR("[%d]: load ukey config: dev[%d]: %s(%d), type: %d\n",
	      edrv->index, edev->dev_index, edev->config.name,
	      edev->config.index, edev->config.type);

	/* type: 10byte */
	switch (edev->config.type) {
	case LCD_EXTERN_I2C:
		if (*(p + LCD_UKEY_EXT_TYPE_VAL_0))
			edev->config.i2c_addr = *(p + LCD_UKEY_EXT_TYPE_VAL_0);
		else
			edev->config.i2c_addr = LCD_EXT_I2C_ADDR_INVALID;
		if (*(p + LCD_UKEY_EXT_TYPE_VAL_1))
			edev->config.i2c_addr2 = *(p + LCD_UKEY_EXT_TYPE_VAL_1);
		else
			edev->config.i2c_addr2 = LCD_EXT_I2C_ADDR_INVALID;
		if (*(p + LCD_UKEY_EXT_TYPE_VAL_4))
			edev->config.i2c_addr3 = *(p + LCD_UKEY_EXT_TYPE_VAL_4);
		else
			edev->config.i2c_addr3 = LCD_EXT_I2C_ADDR_INVALID;
		if (*(p + LCD_UKEY_EXT_TYPE_VAL_5))
			edev->config.i2c_addr4 = *(p + LCD_UKEY_EXT_TYPE_VAL_5);
		else
			edev->config.i2c_addr4 = LCD_EXT_I2C_ADDR_INVALID;
		edev->config.cmd_size = *(p + LCD_UKEY_EXT_TYPE_VAL_3);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("[%d]: %s(%d): cmd_size = %d\n",
			      edrv->index, edev->config.name,
			      edev->dev_index, edev->config.cmd_size);
		}

		/* init */
		if (edev->config.cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_ukey(edrv, edev, p, key_len);
		break;
	case LCD_EXTERN_SPI:
		edev->config.spi_gpio_cs = *(p + LCD_UKEY_EXT_TYPE_VAL_0);
		edev->config.spi_gpio_clk = *(p + LCD_UKEY_EXT_TYPE_VAL_1);
		edev->config.spi_gpio_data = *(p + LCD_UKEY_EXT_TYPE_VAL_2);
		edev->config.spi_clk_freq = (*(p + LCD_UKEY_EXT_TYPE_VAL_3) |
			((*(p + LCD_UKEY_EXT_TYPE_VAL_4)) << 8));
		edev->config.spi_clk_pol = *(p + LCD_UKEY_EXT_TYPE_VAL_5);
		edev->config.cmd_size = *(p + LCD_UKEY_EXT_TYPE_VAL_6);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("[%d]: %s(%d): cmd_size = %d\n",
			      edrv->index, edev->config.name,
			      edev->dev_index, edev->config.cmd_size);
		}

		/* init */
		if (edev->config.cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_ukey(edrv, edev, p, key_len);
		break;
	case LCD_EXTERN_MIPI:
		edev->config.cmd_size = *(p + LCD_UKEY_EXT_TYPE_VAL_9);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("[%d]: %s(%d): cmd_size = %d\n",
			      edrv->index, edev->config.name,
			      edev->dev_index, edev->config.cmd_size);
		}

		if (edev->config.cmd_size != LCD_EXT_CMD_SIZE_DYNAMIC)
			break;
		ret = lcd_extern_init_table_handle_ukey(edrv, edev, p, key_len);
		break;
	case LCD_EXTERN_SIMPLE:
		edev->config.cmd_size = *(p + LCD_UKEY_EXT_TYPE_VAL_9);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			EXTPR("[%d]: %s(%d): cmd_size = %d\n",
			      edrv->index, edev->config.name,
			      edev->dev_index, edev->config.cmd_size);
		}

		/* init */
		if (edev->config.cmd_size == 0)
			break;
		ret = lcd_extern_init_table_handle_ukey(edrv, edev, p, key_len);
		break;
	default:
		break;
	}

	memset(para, 0, key_len);
	free(para);
	return ret;
}

static int lcd_extern_get_config_bsp(struct lcd_extern_driver_s *edrv,
					  struct lcd_extern_dev_s *edev)
{
	struct lcd_dft_config_s *dft_conf;
	struct lcd_extern_config_s *ext_conf;
	int ret = 0, dev_index = edev->dev_index;

	EXTPR("[%d]: load dev config %d from bsp\n", edrv->index, dev_index);
	dft_conf = edrv->data->dft_conf[edrv->index];
	if (dev_index >= dft_conf->ext_common->ext_num) {
		EXTERR("[%d]: %s: %d invalid\n", edrv->index, __func__, dev_index);
		ret = -1;
	} else {
		if (dft_conf->ext_conf) {
			ext_conf = dft_conf->ext_conf + dev_index;
			memcpy(&edev->config, ext_conf, sizeof(*ext_conf));
		}
	}
	return ret;
}

/* config from json =============================================================================*/
#ifdef CONFIG_AML_LCD_JSON
static struct num_str_s ext_type_name[] = {
	{LCD_EXTERN_I2C,    "LCD_EXTERN_I2C"},
	{LCD_EXTERN_SPI,    "LCD_EXTERN_SPI"},
	{LCD_EXTERN_MIPI,   "LCD_EXTERN_MIPI"},
	{LCD_EXTERN_MAX, "LCD_EXTERN_MAX"},
};

__maybe_unused static int lcd_extern_init_table_check(unsigned char *table, int len)
{
	int i = 0, type = 0, size = 0;

	for (i = 0; i < len; i += size) {
		type = table[i];
		size = table[i + 1] + 2;//type + size
		if (i + size > len)
			return -1;
		if (type == LCD_EXT_CMD_TYPE_END)
			return 0;
	}
	return -1;
}

int lcd_extern_get_config_json(struct lcd_extern_driver_s *edrv,
				      struct lcd_extern_dev_s *edev)
{
#define MAX_STR_LEN 64
	struct json_s *parent, *child, *data_json, *pmu_json, *json;
	const char *str = NULL, *dir_uboot;
	int cnt = 1, i = 0, n = 0, nums_size = 0, ret;
	unsigned int *nums = NULL;
	unsigned char *init_data = NULL;
	int size, index = edrv->index;
	char path[256], tag_name[MAX_STR_LEN];
	struct lcd_extern_config_s *cfg;
	unsigned char *vaddr, *p;
	struct json_parse_s *jsp = get_panel_jsp(edrv->index);

	if (!json_parse_ok(jsp)) {
		ret = panel_json_parse(jsp, get_panel_file(index, NULL));
		if (ret) {
			rm_panel_file(index);
			return -1;
		}
	}

	parent = json_path_to_node(jsp, jsp->root, "/lcd_ext_dev");
	if (!parent) {
		EXTERR("find /lcd_extern\n");
		return -1;
	}
	parent = json_get_array_child(jsp, parent, edev->dev_index);
	if (!parent) {
		EXTERR("find /lcd_ext_dev[%d]\n", edev->dev_index);
		return -1;
	}

	cfg = &edev->config;
	cfg->index = edev->dev_index;
	str = json_get_obj_str(jsp, parent, "name", "ext_default");
	strlcpy(cfg->name, str ? str : "ext_default", LCD_EXTERN_NAME_LEN_MAX);
	str = json_get_obj_str(jsp, parent, "type", NULL);
	cfg->type = strnum_get_num(str, ext_type_name, ARRAY_SIZE(ext_type_name), LCD_EXTERN_MAX);
	cfg->status = json_get_obj_u32(jsp, parent, "status", 0);

	switch (cfg->type) {
	case LCD_EXTERN_I2C:
		child = json_get_object_child(jsp, parent, "i2c_addr");
		cfg->i2c_addr = json_get_arr_u32(jsp, child, 0, LCD_EXT_I2C_ADDR_INVALID);
		cfg->i2c_addr2 = json_get_arr_u32(jsp, child, 1, LCD_EXT_I2C_ADDR_INVALID);
		cfg->i2c_addr3 = json_get_arr_u32(jsp, child, 2, LCD_EXT_I2C_ADDR_INVALID);
		cfg->i2c_addr4 = json_get_arr_u32(jsp, child, 3, LCD_EXT_I2C_ADDR_INVALID);
		cfg->cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC;
		if (lcd_debug_print_flag)
			EXTPR("i2c_addr=[%x, %x, %x, %x]\n", cfg->i2c_addr, cfg->i2c_addr2,
			      cfg->i2c_addr3, cfg->i2c_addr4);
		break;
	case LCD_EXTERN_SPI:
		cfg->spi_gpio_cs    = json_get_obj_u32(jsp, parent, "gpio_cs_id", 0);
		cfg->spi_gpio_clk   = json_get_obj_u32(jsp, parent, "gpio_clk_id", 0);
		cfg->spi_gpio_data  = json_get_obj_u32(jsp, parent, "gpio_data_id", 0);
		cfg->spi_clk_pol    = json_get_obj_u32(jsp, parent, "clk_pol", 0);
		cfg->spi_clk_freq   = json_get_obj_u32(jsp, parent, "clk_freq", 0);
		//cfg->spi_delay_us   = json_get_obj_u32(jsp, parent, "interval", 10);
		if (lcd_debug_print_flag)
			EXTPR("spi cs=%d, clk=%d data=%d, pol=%d, freq=%d\n",
			      cfg->spi_gpio_cs, cfg->spi_gpio_clk, cfg->spi_gpio_data,
			      cfg->spi_clk_pol, cfg->spi_clk_freq);
		break;
	default:
		EXTERR("invalid type\n");
		return -1;
	}

/*-----------------------------------------------------------------------------------------------*/
	data_json = json_get_object_child(jsp, jsp->root, "tcon");
	pmu_json  = json_get_object_child(jsp, data_json, "pmu_data");
	dir_uboot = json_get_obj_str(jsp, data_json, "panel_dir_uboot", NULL);

	cnt = json_get_object_size(jsp, pmu_json);
	ext_pmu_bins.name = (char *)malloc(cnt * ext_pmu_bins.name_len);
	ext_pmu_bins.path = (char *)malloc(cnt *  ext_pmu_bins.path_len);
	memset(ext_pmu_bins.name, 0, cnt * ext_pmu_bins.name_len);
	memset(ext_pmu_bins.path, 0, cnt * ext_pmu_bins.path_len);
	for (i = 0; i < cnt; i++) {
		json = json_get_object_child_by_id(jsp, pmu_json, i);
		if (!json)
			break;

		snprintf(tag_name, MAX_STR_LEN, "%s", json_get_key(jsp, json));
		str = json_get_str(jsp, json);
		ret = path_name_compose(dir_uboot, str, path);
		if (ret)
			continue;
		strcpy(ext_pmu_bins.name + ext_pmu_bins.cnt * ext_pmu_bins.name_len, tag_name);
		strcpy(ext_pmu_bins.path + ext_pmu_bins.cnt * ext_pmu_bins.path_len, path);
		ext_pmu_bins.cnt++;
	}

	str = json_get_obj_str(jsp, parent, "init_on", NULL);
	if (!str) {
		EXTPR("not find /lcd_extern[%d]/find init_on\n", edev->dev_index);
		goto parse_init_off;
	}

	nums_size = (strlen(str) + 1) * sizeof(unsigned int);
	nums = (unsigned int *)malloc(nums_size);
	if (!nums) {
		EXTPR("/lcd_extern[%d]/find init_on: no memory to save nums\n",
			edev->dev_index);
		goto parse_init_off;
	}

	memset(nums, 0, nums_size);
	cnt = string_to_numbers(str, nums);

	init_data = (unsigned char *)malloc(LCD_EXTERN_INIT_ON_MAX + LCD_EXTERN_INIT_OFF_MAX);
	if (!init_data) {
		EXTPR("no memory to save init_on data\n");
		goto parse_init_off;
	}
	memset(init_data, 0, LCD_EXTERN_INIT_ON_MAX + LCD_EXTERN_INIT_OFF_MAX);
	n = lcd_extern_data_init_load(init_data, nums, nums_size);
/*-----------------------------------------------------------------------------------------------*/

parse_init_off:
	if (n >= 2 && init_data) {
		cfg->table_init_on_cnt = n;
		cfg->table_init_on = (unsigned char *)malloc(n);
		memcpy(cfg->table_init_on, init_data, n);
	}

	n = 0;
	str = json_get_obj_str(jsp, parent, "init_off", NULL);
	if (!str)
		goto parse_init_off_fail;

	size = (strlen(str) + 1) * sizeof(unsigned int);
	if (size > nums_size) {
		if (nums) {
			memset(nums, 0, nums_size);
			free(nums);
		}
		nums_size = size;
		nums = NULL;
		nums = malloc(nums_size);
	}
	if (!nums)
		goto parse_init_off_fail;

	cnt = string_to_numbers(str, nums);

	if (cnt <= LCD_EXTERN_INIT_OFF_MAX) {
		for (i = 0; i < cnt; i++)
			init_data[n + i] = nums[i];
		n += cnt;
		goto parse_init_off_ok;
	}

parse_init_off_fail:
	init_data[n + 0] = LCD_EXT_CMD_TYPE_END;
	init_data[n + 1] = 0;
	n += 2;

parse_init_off_ok:
	if (n >= 2 && init_data) {
		cfg->table_init_off_cnt = n;
		cfg->table_init_off = (unsigned char *)malloc(n);
		memcpy(cfg->table_init_off, init_data, n);
	}

	cfg->table_init_loaded = 1;
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		EXTPR("init_on: (cnt=%d)\n", cfg->table_init_on_cnt);
		mem_dump(cfg->table_init_on, cfg->table_init_on_cnt);

		EXTPR("init off: (cnt=%d)\n", cfg->table_init_off_cnt);
		mem_dump(cfg->table_init_off, cfg->table_init_off_cnt);
	}

/* save for kernel use */
	size = cfg->table_init_on_cnt + cfg->table_init_off_cnt + 8;
	sprintf(tag_name, "panel%d_ext%d_init_table", edrv->index, edev->dev_index);
	vaddr = (unsigned char *)malloc(size);
	if (vaddr) {
		p = vaddr;
		*(u32 *)(p + 0) = cfg->table_init_on_cnt;
		*(u32 *)(p + 4) = cfg->table_init_off_cnt;
		p += 8;
		memcpy(p, cfg->table_init_on, cfg->table_init_on_cnt);
		p += cfg->table_init_on_cnt;
		memcpy(p, cfg->table_init_off, cfg->table_init_off_cnt);
		panel_param_mem_put(vaddr, tag_name, size);
		memset(vaddr, 0, size);
		free(vaddr);
		vaddr = NULL;
	}

	if (init_data)
		free(init_data);
	if (nums)
		free(nums);
	return 0;
#undef MAX_STR_LEN
}
#else
static inline int lcd_extern_get_config_json(struct lcd_extern_driver_s *edrv,
					     struct lcd_extern_dev_s *edev)
{
	return -1;
}
#endif

static unsigned int lcd_extern_dt_valid(char *dt_addr, int index)
{
#ifdef CONFIG_OF_LIBFDT
	int parent_offset;
	char str[16];
	char *propdata;

	if (index == 0)
		sprintf(str, "/lcd_extern");
	else
		sprintf(str, "/lcd_extern%d", index);

	parent_offset = fdt_path_offset(dt_addr, str);
	if (!parent_offset)
		return 0;
	/* check lcd status enable or not */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (propdata && strncmp(propdata, "okay", 2) == 0)
		return 1;

	LCDERR("[%d]: extern disabled\n", index);
#endif
	return 0;
}

static int lcd_ext_check_config_load(struct lcd_extern_driver_s *edrv)
{
	int ret = 0, dt_sta;

	dt_sta = lcd_extern_dt_valid(lcd_get_dt_addr(), edrv->index);
	edrv->config_load = lcd_panel_config_load_detect(edrv->index, dt_sta, edrv->key_valid);
	if (edrv->config_load == LCD_CONFIG_NONE) {
		LCDERR("[%d] config_load_check error: config_load:%d, dt_status:%d, key:%d",
			edrv->index, edrv->config_load, dt_sta, edrv->key_valid);
		return -1;
	}

	return ret;
}

static int lcd_extern_dev_probe(struct lcd_extern_driver_s *edrv, int n, int dev_index)
{
	struct lcd_extern_dev_s *edev;
	char skey[15], snode[15];
	int ret = -1;
	unsigned char file_type = PANEL_FILE_INVILD;

	if (!edrv->dev[n]) {
		edrv->dev[n] = (struct lcd_extern_dev_s *)malloc(sizeof(struct lcd_extern_dev_s));
		if (!edrv->dev[n]) {
			EXTERR("[%d]: %s: Not enough memory\n",
			       edrv->index, __func__);
			return -1;
		}
	}
	edev = edrv->dev[n];
	memset(edev, 0, sizeof(struct lcd_extern_dev_s));
	edev->dev_index = dev_index;

	if (edrv->index == 0) {
		sprintf(snode, "/lcd_extern");
		sprintf(skey, "lcd_extern");
	} else {
		sprintf(snode, "/lcd%d_extern", edrv->index);
		sprintf(skey, "lcd%d_extern", edrv->index);
	}

	switch (edrv->config_load) {
	case LCD_CONFIG_FILE:
		file_type = get_lcd_panel_file_type(edrv->index);
		if (file_type == PANEL_FILE_JSON)
			ret = lcd_extern_get_config_json(edrv, edev);
		else if (file_type == PANEL_FILE_INI)
			ret = -1; //todo
		break;
	case LCD_CONFIG_UKEY:
		ret = lcd_extern_get_config_ukey(edrv, edev, skey);
		break;
	case LCD_CONFIG_DTS:
		ret = lcd_extern_get_config_dts(lcd_get_dt_addr(), snode, edrv, edev);
		break;
	case LCD_CONFIG_BSP:
		ret = lcd_extern_get_config_bsp(edrv, edev);
		break;
	default:
		ret = -1;
		break;
	}

	EXTPR("[%d]: %s: %s(%d) ok\n",
	      edrv->index, __func__, edev->config.name, dev_index);
	return ret;
}

int lcd_extern_load_config(struct lcd_extern_driver_s *edrv, char *dtaddr, int load_id,
			   int *ext_index_lut)
{
	int dev_index;
	int ret = 0, i;

	if (load_id != LCD_CONFIG_BSP)
		ret = lcd_extern_get_init_dts(dtaddr, edrv);
	else
		ret = lcd_extern_get_init_bsp(edrv);
	if (ret)
		return -1;

	if (lcd_ext_check_config_load(edrv))
		return -1;

	if (0)
		lcd_extern_pinmux_load_from_bsp(edrv);

	for (i = 0; i < edrv->dev_cnt; i++) {
		dev_index = ext_index_lut[i];
		ret = lcd_extern_dev_probe(edrv, i, dev_index);
		if (ret)
			return -1;
	}

	return 0;
}
