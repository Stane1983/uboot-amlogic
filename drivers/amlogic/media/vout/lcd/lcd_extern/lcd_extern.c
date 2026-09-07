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
#include "../lcd_reg.h"

static int lcd_ext_dev_cnt[LCD_MAX_DRV];
static int lcd_ext_index_lut[LCD_MAX_DRV][LCD_EXTERN_DEV_MAX];
static struct lcd_extern_driver_s *ext_driver[LCD_MAX_DRV];

struct lcd_extern_driver_s *lcd_extern_get_driver(int drv_index)
{
	if (drv_index >= LCD_MAX_DRV)
		return NULL;

	return ext_driver[drv_index];
}

struct lcd_extern_dev_s *lcd_extern_get_dev(struct lcd_extern_driver_s *edrv, int dev_index)
{
	int i = 0;

	if (!edrv)
		return NULL;
	if (dev_index >= LCD_EXTERN_INDEX_INVALID)
		return NULL;

	for (i = 0; i < lcd_ext_dev_cnt[edrv->index]; i++) {
		if (edrv->dev[i] && edrv->dev[i]->dev_index == dev_index)
			return edrv->dev[i];
	}

	EXTERR("[%d]: invalid dev_index: %d\n", edrv->index, dev_index);
	return NULL;
}

static void lcd_extern_init_table_dynamic_print(struct lcd_extern_config_s *econf, int flag)
{
	int i, j, max_len;
	unsigned char type, size;
	unsigned char *table;

	if (flag) {
		printf("power on:\n");
		table = econf->table_init_on;
		max_len = econf->table_init_on_cnt;
	} else {
		printf("power off:\n");
		table = econf->table_init_off;
		max_len = econf->table_init_off_cnt;
	}
	if (!table) {
		EXTERR("init_table %d is NULL\n", flag);
		return;
	}
	i = 0;
	switch (econf->type) {
	case LCD_EXTERN_I2C:
	case LCD_EXTERN_SPI:
	case LCD_EXTERN_SIMPLE:
		while ((i + 1) < max_len) {
			type = table[i];
			size = table[i + 1];
			if (type == LCD_EXT_CMD_TYPE_END) {
				printf("  0x%02x,%d,\n", type, size);
				break;
			}
			printf("  0x%02x,%d,", type, size);
			if (size == 0)
				goto init_table_dynamic_print_i2c_spi_next;
			if (i + 2 + size > max_len) {
				printf("size out of support\n");
				break;
			}

			if (type == LCD_EXT_CMD_TYPE_GPIO ||
			    type == LCD_EXT_CMD_TYPE_DELAY) {
				for (j = 0; j < size; j++)
					printf("%d,", table[i + 2 + j]);
			} else if ((type == LCD_EXT_CMD_TYPE_CMD_DELAY) ||
				(type == LCD_EXT_CMD_TYPE_CMD2_DELAY)) {
				for (j = 0; j < (size - 1); j++)
					printf("0x%02x,", table[i + 2 + j]);
				printf("%d,", table[i + size + 1]);
			} else {
				for (j = 0; j < size; j++)
					printf("0x%02x,", table[i + 2 + j]);
			}
init_table_dynamic_print_i2c_spi_next:
			printf("\n");
			i += (size + 2);
		}
		break;
	default:
		break;
	}
}

static void lcd_extern_init_table_fixed_print(struct lcd_extern_config_s *econf, int flag)
{
	int i, j, max_len;
	unsigned char cmd_size;
	unsigned char *table;

	cmd_size = econf->cmd_size;
	if (flag) {
		printf("power on:\n");
		table = econf->table_init_on;
		max_len = econf->table_init_on_cnt;
	} else {
		printf("power off:\n");
		table = econf->table_init_off;
		max_len = econf->table_init_off_cnt;
	}
	if (!table) {
		EXTERR("init_table %d is NULL\n", flag);
		return;
	}

	i = 0;
	while ((i + cmd_size) <= max_len) {
		printf("  ");
		for (j = 0; j < cmd_size; j++)
			printf("0x%02x,", table[i + j]);
		printf("\n");

		if (table[i] == LCD_EXT_CMD_TYPE_END)
			break;
		i += cmd_size;
	}
}

static void lcd_extern_multi_list_print(struct lcd_extern_dev_s *edev)
{
	struct lcd_extern_multi_list_s *temp_list;
	unsigned char *buf;
	int i;

	if (!edev->multi_list_header) {
		printf("multi_list: NULL\n");
		return;
	}

	temp_list = edev->multi_list_header;
	while (temp_list) {
		printf("multi_list[%d]:\n", temp_list->index);
		printf("  type: 0x%x\n", temp_list->type);
		printf("  data:");
		buf = temp_list->data_buf;
		if (temp_list->type == LCD_EXT_CMD_TYPE_MULTI_LIST_UFR) {
			for (i = 0; i < temp_list->data_len; i += 2)
				printf(" %d", buf[i] | (buf[i + 1] << 8));
		} else {
			for (i = 0; i < temp_list->data_len; i++)
				printf(" %d", buf[i]);
		}
		printf("\n");
		temp_list = temp_list->next;
	}
}

static void lcd_extern_dev_info_print(struct lcd_extern_dev_s *edev)
{
	printf("lcd_extern device[%d] info:\n", edev->config.index);
	printf("name:             %s\n"
		"type:             %d\n"
		"status:           %d\n",
		edev->config.name,
		edev->config.type, edev->config.status);

	switch (edev->config.type) {
	case LCD_EXTERN_I2C:
		printf("i2c_addr:         0x%02x\n"
			"i2c_addr2:        0x%02x\n"
			"i2c_addr3:        0x%02x\n"
			"i2c_addr4:        0x%02x\n"
			"table_loaded:     %d\n",
			edev->config.i2c_addr, edev->config.i2c_addr2,
			edev->config.i2c_addr3, edev->config.i2c_addr4,
			edev->config.table_init_loaded);
		if (edev->config.cmd_size == 0)
			break;
		printf("init_loaded           = %d\n"
			"cmd_size              = %d\n"
			"table_init_on_cnt:    = %d\n"
			"table_init_off_cnt:   = %d\n",
			edev->config.table_init_loaded,
			edev->config.cmd_size,
			edev->config.table_init_on_cnt,
			edev->config.table_init_off_cnt);
		if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC) {
			lcd_extern_init_table_dynamic_print(&edev->config, 1);
			lcd_extern_init_table_dynamic_print(&edev->config, 0);
		} else {
			lcd_extern_init_table_fixed_print(&edev->config, 1);
			lcd_extern_init_table_fixed_print(&edev->config, 0);
		}
		lcd_extern_multi_list_print(edev);
		break;
	case LCD_EXTERN_SPI:
		printf("spi_gpio_cs:      %d\n"
			"spi_gpio_clk:     %d\n"
			"spi_gpio_data:    %d\n"
			"spi_clk_freq:     %d\n"
			"spi_clk_pol:      %d\n"
			"table_loaded:     %d\n",
			edev->config.spi_gpio_cs,
			edev->config.spi_gpio_clk,
			edev->config.spi_gpio_data,
			edev->config.spi_clk_freq,
			edev->config.spi_clk_pol,
			edev->config.table_init_loaded);
		if (edev->config.cmd_size == 0)
			break;
		printf("init_loaded           = %d\n"
			"cmd_size              = %d\n"
			"table_init_on_cnt:    = %d\n"
			"table_init_off_cnt:   = %d\n",
			edev->config.table_init_loaded,
			edev->config.cmd_size,
			edev->config.table_init_on_cnt,
			edev->config.table_init_off_cnt);
		if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC) {
			lcd_extern_init_table_dynamic_print(&edev->config, 1);
			lcd_extern_init_table_dynamic_print(&edev->config, 0);
		} else {
			lcd_extern_init_table_fixed_print(&edev->config, 1);
			lcd_extern_init_table_fixed_print(&edev->config, 0);
		}
		break;
	case LCD_EXTERN_SIMPLE:
		if (edev->config.cmd_size == 0)
			break;
		printf("init_loaded           = %d\n"
			"cmd_size              = %d\n"
			"table_init_on_cnt:    = %d\n"
			"table_init_off_cnt:   = %d\n",
			edev->config.table_init_loaded,
			edev->config.cmd_size,
			edev->config.table_init_on_cnt,
			edev->config.table_init_off_cnt);
		if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC) {
			lcd_extern_init_table_dynamic_print(&edev->config, 1);
			lcd_extern_init_table_dynamic_print(&edev->config, 0);
		} else {
			lcd_extern_init_table_fixed_print(&edev->config, 1);
			lcd_extern_init_table_fixed_print(&edev->config, 0);
		}
		lcd_extern_multi_list_print(edev);
		break;
	default:
		printf("not support extern_type\n");
		break;
	}
}

static void lcd_extern_info_print(struct lcd_extern_driver_s *edrv)
{
	struct lcd_extern_dev_s *edev;
	int i;

	EXTPR("[%d]: driver info:\n", edrv->index);
	printf("index:            %d\n"
		"key_valid:        %d\n"
		"dev_cnt:          %d\n"
		"i2c_bus:          %d\n",
		edrv->index, edrv->key_valid,
		edrv->dev_cnt, edrv->i2c_bus);
	if (edrv->i2c_sck_gpio < LCD_EXTERN_GPIO_NUM_MAX) {
		printf("i2c_sck_gpio:     %d\n"
			"i2c_sck_gpio_off: %d\n",
			edrv->i2c_sck_gpio, edrv->i2c_sck_gpio_off);
	}
	if (edrv->i2c_sda_gpio < LCD_EXTERN_GPIO_NUM_MAX) {
		printf("i2c_sda_gpio:     %d\n"
			"i2c_sda_gpio_off: %d\n",
			edrv->i2c_sda_gpio, edrv->i2c_sda_gpio_off);
	}
	i = 0;
	while (i < LCD_EXTERN_GPIO_NUM_MAX) {
		if (strcmp(edrv->gpio_name[i], "invalid") == 0)
			break;
		printf("gpio[%d]=%s\n", i, edrv->gpio_name[i]);
		i++;
	}

	for (i = 0; i < edrv->dev_cnt; i++) {
		edev = edrv->dev[i];
		if (!edev)
			continue;
		lcd_extern_dev_info_print(edev);
	}
}

static void lcd_extern_power_ctrl(struct lcd_extern_driver_s *edrv, int status)
{
	struct lcd_extern_dev_s *edev;
	int i;

	EXTPR("%s: %d\n", __func__, status);

	if (status) {
		for (i = 0; i < edrv->dev_cnt; i++) {
			edev = edrv->dev[i];
			if (edev && edev->power_on) {
				EXTPR("[%d]: %s: %d: dev[%d]:\n",
				      edrv->index, __func__, status, i);
				edev->power_on(edrv, edev);
			}
		}
	} else {
		for (i = 0; i < edrv->dev_cnt; i++) {
			edev = edrv->dev[i];
			if (edev && edev->power_off) {
				EXTPR("[%d]: %s: %d: dev[%d]:\n",
				      edrv->index, __func__, status, i);
				edev->power_off(edrv, edev);
			}
		}
	}
}

static void lcd_extern_multi_list_add(struct lcd_extern_dev_s *edev,
		unsigned int index, unsigned int type,
		unsigned char data_len, unsigned char *data_buf)
{
	struct lcd_extern_multi_list_s *temp_list;
	struct lcd_extern_multi_list_s *cur_list;

	/* creat list */
	cur_list = (struct lcd_extern_multi_list_s *)malloc(sizeof(struct lcd_extern_multi_list_s));
	if (!cur_list)
		return;
	memset(cur_list, 0, sizeof(struct lcd_extern_multi_list_s));
	cur_list->index = index;
	cur_list->type = type;
	cur_list->data_len = data_len;
	cur_list->data_buf = data_buf;

	if (!edev->multi_list_header) {
		edev->multi_list_header = cur_list;
	} else {
		temp_list = edev->multi_list_header;
		while (temp_list->next) {
			if (temp_list->index == cur_list->index) {
				EXTERR("%s: dev[%d]: index=%d(type=0x%x) already in list\n",
				       __func__, edev->dev_index,
				       cur_list->index, cur_list->type);
				free(cur_list);
				return;
			}
			temp_list = temp_list->next;
		}
		temp_list->next = cur_list;
	}

	EXTPR("%s: dev[%d]: index=%d, type=0x%x\n",
	      __func__, edev->dev_index, cur_list->index, cur_list->type);
}

static int lcd_extern_multi_list_remove(struct lcd_extern_dev_s *edev)
{
	struct lcd_extern_multi_list_s *cur_list;
	struct lcd_extern_multi_list_s *next_list;

	/* add to exist list */
	cur_list = edev->multi_list_header;
	while (cur_list) {
		next_list = cur_list->next;
		free(cur_list);
		cur_list = next_list;
	}
	edev->multi_list_header = NULL;

	return 0;
}

static void lcd_extern_config_update_dynamic_size(struct lcd_extern_dev_s *edev, int flag)
{
	unsigned char type, size, *table;
	unsigned int max_len = 0, i = 0, j, index;

	if (flag) {
		max_len = edev->config.table_init_on_cnt;
		table = edev->config.table_init_on;
	} else {
		max_len = edev->config.table_init_off_cnt;
		table = edev->config.table_init_off;
	}

	while ((i + 1) < max_len) {
		type = table[i];
		size = table[i + 1];
		if (type == LCD_EXT_CMD_TYPE_END)
			break;
		if (size == 0)
			goto lcd_extern_config_update_dynamic_size_next;
		if ((i + 2 + size) > max_len)
			break;

		if (type == LCD_EXT_CMD_TYPE_MULTI_LIST_FR) {
			for (j = 0; j < size; j += 3) {
				index = i + 2 + j;
				lcd_extern_multi_list_add(edev, table[index],
							  type, 2, &table[index + 1]);
			}
		} else if (type == LCD_EXT_CMD_TYPE_MULTI_LIST_UFR) {
			for (j = 0; j < size; j += 5) {
				index = i + 2 + j;
				lcd_extern_multi_list_add(edev, table[index],
							  type, 4, &table[index + 1]);
			}
		}
lcd_extern_config_update_dynamic_size_next:
		i += (size + 2);
	}
}

static void lcd_extern_config_update(struct lcd_extern_dev_s *edev)
{
	if (edev->config.type == LCD_EXTERN_I2C) {
		edev->i2c_addr[0] = edev->config.i2c_addr;
		edev->i2c_addr[1] = edev->config.i2c_addr2;
		edev->i2c_addr[2] = edev->config.i2c_addr3;
		edev->i2c_addr[3] = edev->config.i2c_addr4;
	}
	if (edev->config.cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC) {
		lcd_extern_config_update_dynamic_size(edev, 1);
		lcd_extern_config_update_dynamic_size(edev, 0);
	}
}

static int lcd_extern_add_dev(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	int ret = -1;

	if (edev->config.status == 0) {
		EXTERR("[%d]: %s: %s(%d) is disabled\n",
		       edrv->index, __func__,
		       edev->config.name, edev->dev_index);
		return -1;
	}

	if (strcmp(edev->config.name, "ext_default") == 0) {
		ret = lcd_extern_default_probe(edrv, edev);
#ifdef CONFIG_AML_LCD_EXTERN_I2C_RT6947
	} else if (strcmp(edev->config.name, "i2c_RT6947") == 0) {
		ret = lcd_extern_i2c_RT6947_probe(edrv, edev);
#endif
#ifdef CONFIG_AML_LCD_EXTERN_I2C_ANX6862_7911
	} else if (strcmp(edev->config.name, "i2c_ANX6862_7911") == 0) {
		ret = lcd_extern_i2c_ANX6862_7911_probe(edrv, edev);
#endif
#ifdef CONFIG_AML_LCD_EXTERN_I2C_OLED
	} else if (strcmp(edev->config.name, "i2c_oled") == 0) {
		ret = lcd_extern_i2c_oled_probe(edrv, edev);
#endif
	} else {
		EXTERR("[%d]: %s: invalid dev: %s(%d)\n", edrv->index, __func__,
		       edev->config.name, edev->dev_index);
	}
	if (ret) {
		EXTERR("[%d]: %s: %s(%d) failed\n", edrv->index, __func__,
		       edev->config.name, edev->dev_index);
		return -1;
	}

	return ret;
}

int lcd_extern_probe(struct udevice *dev)
{
	struct aml_lcd_data_s *pdata = aml_lcd_get_data();
	struct lcd_extern_driver_s *edrv;
	struct lcd_extern_dev_s *edev;
	int ret = 0;
	unsigned int index;

	ret = dev_read_u32(dev, "index", &index);
	if (ret) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			EXTPR("%s: no index exist, default to 0\n", __func__);
		index = 0;
	}
	if (!ext_driver[index]) {
		ext_driver[index] = (struct lcd_extern_driver_s *)
			malloc(sizeof(struct lcd_extern_driver_s));
		if (!ext_driver[index]) {
			EXTERR("[%d]: %s: Not enough memory\n", index, __func__);
			return -1;
		}
	}

	edrv = ext_driver[index];
	memset(edrv, 0, sizeof(struct lcd_extern_driver_s));
	edrv->data = pdata;
	edrv->index = index;
	edrv->dev_cnt = lcd_ext_dev_cnt[index];
	edrv->udev = dev;
	edrv->info_print = lcd_extern_info_print;
	edrv->power_ctrl = lcd_extern_power_ctrl;

	ret = lcd_extern_load_config(edrv, lcd_get_dt_addr(), lcd_ext_index_lut[edrv->index]);
	if (ret)
		return -1;

	for (index = 0; index < edrv->dev_cnt; index++) {
		edev = edrv->dev[index];
		if (!edev)
			continue;
		lcd_extern_config_update(edev);
		ret = lcd_extern_add_dev(edrv, edev);
		if (ret) {
			EXTERR("[%d]: %s: %d failed\n",
			       edrv->index, __func__, edev->dev_index);
			lcd_extern_multi_list_remove(edev);
			free(edev);
			edrv->dev[index] = NULL;
			return -1;
		}
	}

	return 0;
}

int lcd_extern_remove(void)
{
	int i, j;

	for (i = 0; i < LCD_MAX_DRV; i++) {
		for (j = 0; j < LCD_EXTERN_DEV_MAX; j++)
			lcd_ext_index_lut[i][j] = LCD_EXTERN_INDEX_INVALID;
		if (ext_driver[i])
			free(ext_driver[i]);
		ext_driver[i] = NULL;
		lcd_ext_dev_cnt[i] = 0;
	}

	return 0;
}

int lcd_extern_dev_index_add(int drv_index, int dev_index)
{
	int dev_cnt, i;

	if (drv_index >= LCD_MAX_DRV) {
		EXTERR("%s: invalid drv_index: %d\n", __func__, drv_index);
		return -1;
	}
	if (dev_index == 0xff)
		return 0;

	dev_cnt = lcd_ext_dev_cnt[drv_index];
	if (dev_cnt >= LCD_EXTERN_DEV_MAX) {
		EXTERR("[%d]: %s: out off dev_cnt support\n", drv_index, __func__);
		return -1;
	}

	for (i = 0; i < LCD_EXTERN_DEV_MAX; i++) {
		if (lcd_ext_index_lut[drv_index][i] == dev_index) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				EXTPR("[%d]: %s: dev_index %d already exist\n",
				      drv_index, __func__, dev_index);
			}
			return 0;
		}
	}

	lcd_ext_index_lut[drv_index][dev_cnt] = dev_index;
	lcd_ext_dev_cnt[drv_index]++;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		EXTPR("[%d]: %s: dev_index: %d, dev_cnt: %d\n",
		      drv_index, __func__, dev_index,
		      lcd_ext_dev_cnt[drv_index]);
	}
	return 0;
}

int lcd_extern_dev_index_remove(int drv_index, int dev_index)
{
	int find, i;

	if (drv_index >= LCD_MAX_DRV) {
		BLERR("%s: invalid drv_index: %d\n", __func__, dev_index);
		return -1;
	}
	if (dev_index == 0xff)
		return 0;

	if (lcd_ext_dev_cnt[drv_index] == 0)
		return -1;

	find = 0xff;
	for (i = 0; i < LCD_EXTERN_DEV_MAX; i++) {
		if (lcd_ext_index_lut[drv_index][i] == dev_index)
			find = i;
	}
	if (find == 0xff)
		return 0;

	lcd_ext_index_lut[drv_index][find] = LCD_EXTERN_INDEX_INVALID;
	for (i = (find + 1); i < LCD_EXTERN_DEV_MAX; i++) {
		if (lcd_ext_index_lut[drv_index][i] == LCD_EXTERN_INDEX_INVALID)
			break;
		lcd_ext_index_lut[drv_index][i - 1] = lcd_ext_index_lut[drv_index][i];
		lcd_ext_index_lut[drv_index][i] = LCD_EXTERN_INDEX_INVALID;
	}
	if (lcd_ext_dev_cnt[drv_index])
		lcd_ext_dev_cnt[drv_index]--;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		EXTPR("[%d]: %s: dev_index: %d\n", drv_index, __func__, dev_index);
	return 0;
}

int lcd_extern_init(unsigned int index)
{
	int i;

	for (i = 0; i < LCD_EXTERN_DEV_MAX; i++)
		lcd_ext_index_lut[index][i] = LCD_EXTERN_INDEX_INVALID;
	if (ext_driver[index])
		free(ext_driver[index]);
	ext_driver[index] = NULL;
	lcd_ext_dev_cnt[index] = 0;

	return 0;
}

static const struct udevice_id lcd_extern_match_table[] = {
	{
		.compatible = "amlogic, lcd_extern",
	},
	{}
};

U_BOOT_DRIVER(lcd_extern) = {
	.name = "lcd_extern",
	.id = UCLASS_MISC,
	.of_match = of_match_ptr(lcd_extern_match_table),
	.probe = lcd_extern_probe,
};
