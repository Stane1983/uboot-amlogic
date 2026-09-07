// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <i2c.h>
#include <dm.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_i2c_dev.h>
#include "lcd_common.h"

#define LCDI2C_PR(fmt, args...)     printf("lcd_i2c: "fmt"", ## args)
#define LCDI2C_ERR(fmt, args...)    printf("lcd_i2c: error: "fmt"", ## args)

struct lcd_i2c_match_s {
	unsigned char bus_id;
	char *bus_str;
};

#ifdef CONFIG_DM_I2C
int aml_lcd_i2c_write(struct udevice *dev, unsigned int i2c_addr,
			 unsigned char *buff, unsigned int len)
{
	struct udevice *bus;
	struct udevice *i2c_dev;
	int i, ret = 0;
	unsigned char data[64] = {0};

	ret = uclass_get_device_by_phandle(UCLASS_I2C, dev, "i2c-bus", &bus);
	if (ret) {
		LCDI2C_ERR("no sys aml_i2c_bus find\n");
		return ret;
	}
	ret = i2c_get_chip(bus, i2c_addr, 1, &i2c_dev);
	if (ret) {
		LCDI2C_ERR("no i2c device addr %d find\n", i2c_addr);
		return ret;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		printf("%s:", __func__);
		for (i = 0; i < len; i++)
			printf(" 0x%02x", buff[i]);
		printf(" [addr 0x%02x]\n", i2c_addr);
	}

	if (len < 1) {
		LCDI2C_ERR("invalid len %d\n", len);
		return -1;
	}
	if (len == 1)
		ret = dm_i2c_write(i2c_dev, buff[0], data, 1);
	else if (len > 1)
		ret = dm_i2c_write(i2c_dev, buff[0], &buff[1], len - 1);

	if (ret) {
		LCDI2C_ERR("i2c write failed [addr 0x%02x]\n", i2c_addr);
		return ret;
	}

	return 0;
}

int aml_lcd_i2c_read(struct udevice *dev, unsigned int i2c_addr,
			unsigned char *buff, unsigned int len)
{
	struct udevice *bus;
	struct udevice *i2c_dev;
	int ret = 0, i;

	ret = uclass_get_device_by_phandle(UCLASS_I2C, dev, "i2c-bus", &bus);
	if (ret) {
		LCDI2C_ERR("no sys aml_i2c_bus find\n");
		return ret;
	}
	ret = i2c_get_chip(bus, i2c_addr, 1, &i2c_dev);
	if (ret) {
		LCDI2C_ERR("no i2c device addr %d find\n", i2c_addr);
		return ret;
	}

#if 0
	ret = i2c_write(i2c_dev, buff[0], &buff[1], 1);
	if (ret) {
		LCDI2C_ERR("i2c write failed [addr 0x%02x]\n", i2c_addr);
		return ret;
	}
#endif
	if (len < 1) {
		LCDI2C_ERR("invalid len %d\n", len);
		return -1;
	}

	ret = dm_i2c_read(i2c_dev, buff[0], buff, len);
	if (ret) {
		LCDI2C_ERR("i2c read failed [addr 0x%02x]\n", i2c_addr);
		return ret;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		printf("%s:", __func__);
		for (i = 0; i < len; i++)
			printf(" 0x%02x", buff[i]);
		printf(" [addr 0x%02x]\n", i2c_addr);
	}

	return 0;
}

#else
int aml_lcd_i2c_write(struct udevice *dev, unsigned int i2c_addr,
			 unsigned char *buff, unsigned int len)
{
	LCDI2C_ERR("no CONFIG_DM_I2C\n");
	return -1;
}

int aml_lcd_i2c_read(struct udevice *dev, unsigned int i2c_addr,
			unsigned char *buff, unsigned int len)
{
	LCDI2C_ERR("no CONFIG_DM_I2C\n");
	return -1;
}
#endif
