// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_extern.h>
#include "lcd_extern.h"
#include "../lcd_common.h"
#include "../lcd_reg.h"

#define EXT_DEFAULT_NAME	"ext_default"

static int lcd_extern_reg_read(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			       unsigned char reg, unsigned char *buf)
{
	int ret = 0;

	if (!buf) {
		EXTERR("[%d]: %s: buf is null\n", edrv->index, __func__);
		return -1;
	}

	switch (edev->config.type) {
	case LCD_EXTERN_I2C:
		buf[0] = reg;
		ret = aml_lcd_i2c_read(edrv->i2c_bus, edev->i2c_addr[0], buf, 1);
		break;
	case LCD_EXTERN_SPI:
		ret = lcd_extern_spi_read(edrv, edev, reg, buf);
		break;
	default:
		EXTERR("[%d]: %s: %s(%d): extern_type %d is not support\n",
		       edrv->index, __func__, edev->config.name,
		       edev->config.index, edev->config.type);
		ret = -1;
		break;
	}
	if (ret)
		EXTERR("[%d]: %s: failed\n", edrv->index, __func__);

	return ret;
}

static int lcd_extern_reg_write(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
				unsigned char *buf, unsigned int len)
{
	int ret = 0;

	if (!buf) {
		EXTERR("[%d]: %s: buf is null\n", edrv->index, __func__);
		return -1;
	}

	if (!len) {
		EXTERR("[%d]: %s: invalid len\n", edrv->index, __func__);
		return -1;
	}

	switch (edev->config.type) {
	case LCD_EXTERN_I2C:
		ret = aml_lcd_i2c_write(edrv->i2c_bus, edev->i2c_addr[0], buf, len);
		break;
	case LCD_EXTERN_SPI:
		ret = lcd_extern_spi_write(edrv, edev, buf, len);
		break;
	default:
		EXTERR("[%d]: %s: %s(%d): extern_type %d is not support\n",
		       edrv->index, __func__, edev->config.name,
		       edev->config.index, edev->config.type);
		ret = -1;
		break;
	}
	if (ret)
		EXTERR("[%d]: %s: failed\n", edrv->index, __func__);

	return ret;
}

static int lcd_extern_power_ctrl(struct lcd_extern_driver_s *edrv,
				 struct lcd_extern_dev_s *edev, int flag)
{
	int ret = 0;

	if (edev->config.type == LCD_EXTERN_SPI)
		spi_gpio_init(edrv, edev);

	ret = lcd_extern_power_cmd(edrv, edev, flag);

	/* step 3: power finish */
	if (edev->config.type == LCD_EXTERN_SPI)
		spi_gpio_off(edrv, edev);

	EXTPR("[%d]: %s: %s(%d): %d\n",
	      edrv->index, __func__, edev->config.name, edev->dev_index, flag);
	return ret;
}

static int lcd_extern_power_on(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	int ret;

	lcd_extern_pinmux_set(edrv, 1);
	ret = lcd_extern_power_ctrl(edrv, edev, 1);
	edev->state = 1;
	return ret;
}

static int lcd_extern_power_off(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	int ret;

	edev->state = 0;
	ret = lcd_extern_power_ctrl(edrv, edev, 0);
	lcd_extern_pinmux_set(edrv, 0);

	return ret;
}

static int lcd_extern_driver_update(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	if (edev->config.table_init_loaded == 0) {
		EXTERR("%s(%d): tablet_init is invalid\n",
		       edev->config.name, edev->dev_index);
		return -1;
	}

	if (edev->config.type == LCD_EXTERN_SPI)
		edev->config.spi_delay_us = 1000 / edev->config.spi_clk_freq;

	edev->reg_read  = lcd_extern_reg_read;
	edev->reg_write = lcd_extern_reg_write;
	edev->power_on  = lcd_extern_power_on;
	edev->power_off = lcd_extern_power_off;

	return 0;
}

int lcd_extern_default_probe(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	int ret = 0;

	if (!edrv) {
		EXTERR("%s: %s ext_drv is null\n", __func__, EXT_DEFAULT_NAME);
		return -1;
	}
	if (!edev) {
		EXTERR("[%d]: %s: %s ext_dev is null\n", edrv->index, __func__, EXT_DEFAULT_NAME);
		return -1;
	}

	if (edev->config.cmd_size < 2) {
		EXTERR("[%d]: %s: %s(%d): cmd_size %d is invalid\n",
			edrv->index, __func__,
			edev->config.name,
			edev->dev_index,
			edev->config.cmd_size);
		return -1;
	}

	ret = lcd_extern_driver_update(edrv, edev);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		EXTPR("[%d]: %s: %d %s\n",
		      edrv->index, __func__, edev->dev_index,
		      (ret == 0) ? "ok" : "fail");
	}
	return ret;
}

