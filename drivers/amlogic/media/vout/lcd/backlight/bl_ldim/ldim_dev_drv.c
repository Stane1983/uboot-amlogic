// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/aml_bl.h>
#include <amlogic/media/vout/lcd/bl_ldim.h>
#include "../../lcd_common.h"
#include "../../lcd_reg.h"
#include "../lcd_bl.h"
#include "ldim_drv.h"
#include "ldim_dev_drv.h"
#include "env.h"

void ldim_gpio_set(struct ldim_dev_driver_s *dev_drv, int index, int value)
{
	int gpio;
	char *str;

	if (index >= BL_GPIO_NUM_MAX) {
		LDIMERR("%s: invalid index %d\n", __func__, index);
		return;
	}
	str = dev_drv->gpio_name[index];
	gpio = lcd_gpio_name_map_num(str);
	switch (value) {
	case LCD_GPIO_OUTPUT_LOW:
	case LCD_GPIO_OUTPUT_HIGH:
		lcd_gpio_set(gpio, value);
		break;
	case LCD_GPIO_INPUT:
	default:
		value = LCD_GPIO_INPUT;
		lcd_gpio_set(gpio, value);
		break;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("set gpio %s[%d] value: %d\n", str, index, value);
}

int ldim_gpio_get(struct ldim_dev_driver_s *dev_drv, int index)
{
	int gpio;
	char *str;
	int value;

	if (index >= BL_GPIO_NUM_MAX) {
		LDIMERR("%s: invalid index %d\n", __func__, index);
		return -1;
	}
	str = dev_drv->gpio_name[index];
	gpio = lcd_gpio_name_map_num(str);
	value = lcd_gpio_input_get(gpio);
	return value;
}

/* *************************************** */

void ldim_set_duty_pwm(struct bl_pwm_config_s *bl_pwm)
{
	if (bl_pwm->pwm_port >= BL_PWM_MAX)
		return;
	if (bl_pwm->pwm_duty_max == 0)
		return;

	bl_pwm_duty_to_pwm_level(bl_pwm);

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		LDIMPR("pwm_port 0x%x: duty=%d, duty_max=%d, duty_min=%d\n",
		       bl_pwm->pwm_port, bl_pwm->pwm_duty,
		       bl_pwm->pwm_duty_max, bl_pwm->pwm_duty_min);
	}

	bl_set_pwm(bl_pwm);
}

void ldim_pwm_off(struct bl_pwm_config_s *bl_pwm)
{
	if (bl_pwm->pwm_port >= BL_PWM_MAX)
		return;

	bl_pwm_en(bl_pwm, 0);
}

static int ldim_set_pinmux_pwm(int status, struct bl_pwm_config_s *bl_pwm)
{
	int i;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("%s: %d\n", __func__, status);

	if (status) {
		bl_pwm_en(bl_pwm, 1);
		/* set pinmux */
		bl_pwm->pinmux_flag = 1;
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_clr_mask(bl_pwm->pinmux_clr[i][0],
					    bl_pwm->pinmux_clr[i][1]);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
				LDIMPR("%s: port=0x%x, pinmux_clr=%d,0x%08x\n",
				       __func__, bl_pwm->pwm_port,
				       bl_pwm->pinmux_clr[i][0],
				       bl_pwm->pinmux_clr[i][1]);
			}
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_set_mask(bl_pwm->pinmux_set[i][0],
					    bl_pwm->pinmux_set[i][1]);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
				LDIMPR("%s: port=0x%x, pinmux_set=%d,0x%08x\n",
				       __func__, bl_pwm->pwm_port,
				       bl_pwm->pinmux_set[i][0],
				       bl_pwm->pinmux_set[i][1]);
			}
			i++;
		}
	} else {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_clr_mask(bl_pwm->pinmux_set[i][0],
					    bl_pwm->pinmux_set[i][1]);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
				LDIMPR("%s: port=0x%x, pinmux_clr=%d,0x%08x\n",
				       __func__, bl_pwm->pwm_port,
				       bl_pwm->pinmux_set[i][0],
				       bl_pwm->pinmux_set[i][1]);
			}
			i++;
		}
		bl_pwm->pinmux_flag = 0;

		bl_pwm_en(bl_pwm, 0);
	}

	return 0;
}

static int ldim_pwm_pinmux_ctrl(struct ldim_dev_driver_s *dev_drv, int status)
{
	if (dev_drv->ldim_pwm_config.pwm_port >= BL_PWM_MAX)
		return 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("%s: %d\n", __func__, status);

	ldim_set_pinmux_pwm(status, &dev_drv->ldim_pwm_config);
	if (dev_drv->analog_pwm_config.pwm_port < BL_PWM_VS)
		ldim_set_pinmux_pwm(status, &dev_drv->analog_pwm_config);

	return 0;
}

static void ldim_dev_init_dynamic_print(struct ldim_dev_driver_s *dev_drv, int flag)
{
	int i, j, max_len;
	unsigned char cmd_size;
	unsigned char *table;

	if (flag) {
		printf("power on:\n");
		table = dev_drv->init_on;
		max_len = dev_drv->init_on_cnt;
	} else {
		printf("power off:\n");
		table = dev_drv->init_off;
		max_len = dev_drv->init_off_cnt;
	}
	if (!table) {
		LDIMERR("init_table %d is NULL\n", flag);
		return;
	}

	i = 0;
	while ((i + 1) < max_len) {
		if (table[i] == LCD_EXT_CMD_TYPE_END) {
			printf("  0x%02x,%d,\n", table[i], table[i + 1]);
			break;
		}
		cmd_size = table[i + 1];
		printf("  0x%02x,%d,", table[i], cmd_size);
		if (cmd_size == 0)
			goto init_table_dynamic_print_next;
		if (i + 2 + cmd_size > max_len) {
			printf("cmd_size out of support\n");
			break;
		}

		if (table[i] == LCD_EXT_CMD_TYPE_DELAY) {
			for (j = 0; j < cmd_size; j++)
				printf("%d,", table[i + 2 + j]);
		} else if (table[i] == LCD_EXT_CMD_TYPE_CMD) {
			for (j = 0; j < cmd_size; j++)
				printf("0x%02x,", table[i + 2 + j]);
		} else if (table[i] == LCD_EXT_CMD_TYPE_CMD_DELAY) {
			for (j = 0; j < (cmd_size - 1); j++)
				printf("0x%02x,", table[i + 2 + j]);
			printf("%d,", table[i + cmd_size + 1]);
		} else {
			for (j = 0; j < cmd_size; j++)
				printf("0x%02x,", table[i + 2 + j]);
		}
init_table_dynamic_print_next:
		printf("\n");
		i += (cmd_size + 2);
	}
}

static void ldim_device_config_print(struct ldim_dev_driver_s *dev_drv)
{
	struct bl_pwm_config_s *bl_pwm;
	int i;

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return;
	}

	printf("\ndev_name               = %s\n"
		"key_valid              = %d\n"
		"index                  = %d\n"
		"dim_min                = 0x%03x\n"
		"dim_max                = 0x%03x\n"
		"bl_zone                = %d\n\n"
		"sub_cnt                = %d\n"
		"en_gpio                = %d\n"
		"en_gpio_on             = %d\n"
		"en_gpio_off            = %d\n\n",
		dev_drv->name,
		dev_drv->key_valid,
		dev_drv->index,
		dev_drv->dim_min,
		dev_drv->dim_max,
		dev_drv->zone_num,
		dev_drv->chip_cnt,
		dev_drv->en_gpio,
		dev_drv->en_gpio_on,
		dev_drv->en_gpio_off);

	printf("zone_mapping:\n");
	for (i = 0; i < dev_drv->zone_num; i++)
		printf("%d,", dev_drv->bl_mapping[i]);
	printf("\n\n");

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		printf("spi_dev                = 0x%p\n"
			"spi_modalias           = %s\n"
			"spi_mode               = %d\n"
			"spi_max_speed_hz       = %d\n"
			"spi_bus_num            = %d\n"
			"spi_chip_select        = %d\n"
			"cs_hold_delay          = %d\n"
			"cs_clk_delay           = %d\n"
			"write_check            = %d\n\n",
			dev_drv->spi_info.spi,
			dev_drv->spi_info.modalias,
			dev_drv->spi_info.mode,
			dev_drv->spi_info.max_speed_hz,
			dev_drv->spi_info.bus_num,
			dev_drv->spi_info.chip_select,
			dev_drv->cs_hold_delay,
			dev_drv->cs_clk_delay,
			dev_drv->write_check);
		break;
	default:
		break;
	}

	bl_pwm = &dev_drv->ldim_pwm_config;
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		printf("ldim_pwm_port          = 0x%x\n"
			"ldim_pwm_pol           = %d\n"
			"ldim_pwm_freq          = %d\n"
			"ldim_pwm_duty          = %d%%\n"
			"ldim_pwm_pinmux_flag   = %d\n\n",
			bl_pwm->pwm_port, bl_pwm->pwm_method,
			bl_pwm->pwm_freq, bl_pwm->pwm_duty,
			bl_pwm->pinmux_flag);
	}

	bl_pwm = &dev_drv->analog_pwm_config;
	if (bl_pwm->pwm_port < BL_PWM_VS) {
		printf("analog_pwm_port        = 0x%x\n"
			"analog_pwm_pol         = %d\n"
			"analog_pwm_freq        = %d\n"
			"analog_pwm_duty        = %d%%\n"
			"analog_pwm_duty_max    = %d%%\n"
			"analog_pwm_duty_min    = %d%%\n"
			"analog_pwm_pinmux_flag = %d\n\n",
			bl_pwm->pwm_port, bl_pwm->pwm_method,
			bl_pwm->pwm_freq, bl_pwm->pwm_duty,
			bl_pwm->pwm_duty_max, bl_pwm->pwm_duty_min,
			bl_pwm->pinmux_flag);
	}

	if (dev_drv->cmd_size > 0) {
		printf("init_loaded            = %d\n"
			"cmd_size               = %d\n"
			"init_on_cnt            = %d\n"
			"init_off_cnt           = %d\n",
			dev_drv->init_loaded,
			dev_drv->cmd_size,
			dev_drv->init_on_cnt,
			dev_drv->init_off_cnt);
		if (dev_drv->cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC) {
			ldim_dev_init_dynamic_print(dev_drv, 1);
			ldim_dev_init_dynamic_print(dev_drv, 0);
		}
	}
}

static int ldim_dev_add_driver(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;
	int ret = 0;

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		ret = ldim_spi_driver_add(dev_drv);
		break;
	default:
		break;
	}
	if (ret)
		return -1;

	ret = -1;
	if (strcmp(dev_drv->name, "iw7027") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_IW7027
		ret = ldim_dev_iw7027_probe(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "blmcu") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_BLMCU
		ret = ldim_dev_blmcu_probe(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "ob3350") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_OB3350
		ret = ldim_dev_ob3350_probe(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "global") == 0) {
		ret = ldim_dev_global_probe(ldim_drv);
	} else {
		LDIMERR("invalid device name: %s\n", dev_drv->name);
		ret = -1;
	}

	if (ret) {
		LDIMERR("add device failed: %s[%d]\n", dev_drv->name, dev_drv->index);
	} else {
		dev_drv->probe_flag = 1;
		LDIMPR("add device: %s[%d]\n", dev_drv->name, dev_drv->index);
	}

	return ret;
}

static int ldim_dev_remove_driver(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	if (dev_drv->probe_flag == 0)
		return 0;

	dev_drv->probe_flag = 0;
	if (strcmp(dev_drv->name, "iw7027") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_IW7027
		ldim_dev_iw7027_remove(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "blmcu") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_BLMCU
		ldim_dev_blmcu_remove(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "ob3350") == 0) {
#ifdef CONFIG_AML_LCD_BL_LDIM_OB3350
		ldim_dev_ob3350_remove(ldim_drv);
#endif
	} else if (strcmp(dev_drv->name, "global") == 0) {
		ldim_dev_global_remove(ldim_drv);
	} else {
		LDIMERR("invalid device name: %s\n", dev_drv->name);
	}

	LDIMPR("remove device: %s[%d]\n", dev_drv->name, dev_drv->index);

	return 0;
}

int aml_ldim_device_probe(char *dt_addr, struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv;
	int ret = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("%s\n", __func__);

	dev_drv = malloc(sizeof(struct ldim_dev_driver_s));
	if (!dev_drv) {
		LDIMERR("%s: dev_drv malloc failed\n", __func__);
		return -1;
	}
	memset(dev_drv, 0, sizeof(struct ldim_dev_driver_s));
	ldim_drv->dev_drv = dev_drv;

	/* device config default */
	dev_drv->probe_flag = 0;
	strcpy(dev_drv->name, "ldim_dev");
	dev_drv->type = LDIM_DEV_TYPE_MAX;
	dev_drv->chip_cnt = 1;
	dev_drv->en_gpio = 0xff;
	dev_drv->en_gpio_on = 1;
	dev_drv->en_gpio_off = 0;
	dev_drv->ldim_pwm_config.pwm_port = BL_PWM_MAX;
	dev_drv->analog_pwm_config.pwm_port = BL_PWM_MAX;
	dev_drv->ldim_pwm_config.pwm_duty_max = 4095;
	dev_drv->analog_pwm_config.pwm_duty_max = 4095;
	strcpy(dev_drv->pinmux_name, "invalid");

	strcpy(dev_drv->spi_info.modalias, "ldim_dev");
	strcpy(dev_drv->spi_info.spi_name, "none");
	dev_drv->spi_info.mode = SPI_MODE_0;
	dev_drv->spi_info.max_speed_hz = 1000000; /* 1MHz */
	dev_drv->spi_info.bus_num = 0; /* SPI bus No. */
	dev_drv->spi_info.chip_select = 0; /* the device index on the spi bus */
	dev_drv->spi_info.wordlen = 8;
	dev_drv->spi_info.spi = NULL;

	dev_drv->index = ldim_drv->config.dev_index;
	dev_drv->bl_row = ldim_drv->config.row;
	dev_drv->bl_col = ldim_drv->config.col;
	dev_drv->zone_num = dev_drv->bl_row * dev_drv->bl_col;
	dev_drv->bl_mapping = malloc(dev_drv->zone_num * sizeof(unsigned short));
	if (!dev_drv->bl_mapping)
		goto ldim_dev_probe_func_fail0;
	memset(dev_drv->bl_mapping, 0, dev_drv->zone_num * sizeof(unsigned short));

#ifdef CONFIG_OF_LIBFDT
	if (!dt_addr) {
		LDIMERR("%s: dt_addr is null\n", __func__);
		goto ldim_dev_probe_func_fail1;
	}
	ret = ldim_dev_get_config(dt_addr, ldim_drv);
	if (ret)
		goto ldim_dev_probe_func_fail1;
#endif

	/* get configs */
	dev_drv->pinmux_ctrl = ldim_pwm_pinmux_ctrl;
	dev_drv->config_print = ldim_device_config_print;

	/* add device driver */
	ret = ldim_dev_add_driver(ldim_drv);

	return ret;

#ifdef CONFIG_OF_LIBFDT
ldim_dev_probe_func_fail1:
	free(dev_drv->bl_mapping);
#endif
ldim_dev_probe_func_fail0:
	free(dev_drv);
	ldim_drv->dev_drv = NULL;
	pr_info("%s: failed\n", __func__);
	return -1;
}

int aml_ldim_device_remove(struct aml_ldim_driver_s *ldim_drv)
{
	ldim_dev_remove_driver(ldim_drv);

	return 0;
}
