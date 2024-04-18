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

static void set_lcd_csb(struct lcd_extern_driver_s *edrv,
			struct lcd_extern_dev_s *edev, unsigned int v)
{
	lcd_extern_gpio_set(edrv, edev->config.spi_gpio_cs, v);
	udelay(edev->config.spi_delay_us);
}

static void set_lcd_scl(struct lcd_extern_driver_s *edrv,
			struct lcd_extern_dev_s *edev, unsigned int v)
{
	lcd_extern_gpio_set(edrv, edev->config.spi_gpio_clk, v);
	udelay(edev->config.spi_delay_us);
}

static void set_lcd_sda(struct lcd_extern_driver_s *edrv,
			struct lcd_extern_dev_s *edev, unsigned int v)
{
	lcd_extern_gpio_set(edrv, edev->config.spi_gpio_data, v);
	udelay(edev->config.spi_delay_us);
}

void spi_gpio_init(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	set_lcd_csb(edrv, edev, 1);
	set_lcd_scl(edrv, edev, 1);
	set_lcd_sda(edrv, edev, 1);
}

void spi_gpio_off(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	set_lcd_sda(edrv, edev, 0);
	set_lcd_scl(edrv, edev, 0);
	set_lcd_csb(edrv, edev, 0);
}

static void spi_write_byte(struct lcd_extern_driver_s *edrv,
			   struct lcd_extern_dev_s *edev, unsigned char data)
{
	int i;

	for (i = 0; i < 8; i++) {
		set_lcd_scl(edrv, edev, 0);
		if (data & 0x80)
			set_lcd_sda(edrv, edev, 1);
		else
			set_lcd_sda(edrv, edev, 0);
		data <<= 1;
		set_lcd_scl(edrv, edev, 1);
	}
}

int lcd_extern_spi_read(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			unsigned char reg, unsigned char *buf)
{
	EXTERR("[%d]: %s: %s(%d): extern_type %d is not support\n",
	       edrv->index, __func__, edev->config.name,
	       edev->config.index, edev->config.type);

	return -1;
}

int lcd_extern_spi_write(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			 unsigned char *buf, int len)
{
	int i;

	if (len < 2) {
		EXTERR("[%d]: %s: len %d error\n", edrv->index, __func__, len);
		return -1;
	}

	set_lcd_csb(edrv, edev, 0);

	for (i = 0; i < len; i++)
		spi_write_byte(edrv, edev, buf[i]);

	set_lcd_csb(edrv, edev, 1);
	set_lcd_scl(edrv, edev, 1);
	set_lcd_sda(edrv, edev, 1);
	udelay(edev->config.spi_delay_us);

	return 0;
}

