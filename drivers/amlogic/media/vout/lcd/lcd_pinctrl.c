// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <errno.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <asm/gpio.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"

int lcd_gpio_name_map_num(const char *name)
{
	int gpio;
#if defined(CONFIG_DM_GPIO)
	int ret;
#endif

#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name(name, NULL, NULL, (unsigned int *)&gpio);
	if (ret) {
		LCDERR("gpio: wrong name %s\n", name);
		return LCD_GPIO_MAX;
	}
#else
	/* turn the gpio name into a gpio number */
	gpio = simple_strtoul(name, NULL, 10);
	if (gpio < 0) {
		LCDERR("gpio: wrong name %s\n", name);
		return LCD_GPIO_MAX;
	}
#endif
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("gpio: %s, %d\n", name, gpio);
	return gpio;
}

int lcd_gpio_set(int gpio, int value)
{
	int ret = 0;

	if (gpio >= LCD_GPIO_MAX)
		return -1;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("gpio: %d, value: %d\n", gpio, value);
	/* grab the pin before we tweak it */
	ret = gpio_request(gpio, "aml_lcd_gpio");
	if (ret && ret != -EBUSY) {
		LCDERR("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}

	/* finally, let's do it: set direction and exec command */
	switch (value) {
	case LCD_GPIO_OUTPUT_LOW:
	case LCD_GPIO_OUTPUT_HIGH:
		ret = gpio_direction_output(gpio, value);
		break;
	case LCD_GPIO_INPUT:
	default:
		ret = gpio_direction_input(gpio);
		break;
	}

	return 0;
}

unsigned int lcd_gpio_input_get(int gpio)
{
	unsigned int value;

	if (gpio >= LCD_GPIO_MAX)
		return 0;
	gpio_direction_input(gpio);
	value = gpio_get_value(gpio);
	return value;
}

static void lcd_custom_pinmux_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_config_s *pconf;
	unsigned int index = 0;
	char pinmux_str[CUS_PINMUX_NAME_MAX + 4];

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: custom_pinmux_set: %d\n", pdrv->index, status);

	pconf = &pdrv->config;

	memset(pinmux_str, 0, sizeof(pinmux_str));
	if (status) {
		index = 1;
		sprintf(pinmux_str, "%s", pdrv->config.cus_pinmux_name);
	} else {
		index = 2;
		sprintf(pinmux_str, "%s_off", pdrv->config.cus_pinmux_name);
	}

	if (index == 0) {
		LCDERR("[%d]: custom_pinmux: index %d error\n", pdrv->index, index);
		return;
	}
	if (pconf->pinmux_flag == index) {
		LCDPR("[%d]: custom_pinmux: %s is already selected\n", pdrv->index, pinmux_str);
		return;
	}

	if (pinctrl_select_state(pdrv->dev, pinmux_str)) {
		LCDERR("[%d]: custom_pinmux: %s(%d) error\n", pdrv->index, pinmux_str, index);
		return;
	}

	pconf->pinmux_flag = index;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: custom_pinmux: %s(%d)\n", pdrv->index, pinmux_str, index);
}

#if defined(CONFIG_AML_LCD_DPI)
static char *lcd_rgb_pinmux_str[] = {
	"none",           /* 0 */
	"rgb_sync_on",    /* 1 */
	"rgb_de_on",      /* 2 */
	"rgb_sync_de_on", /* 3 */
	"rgb_off"         /* 4 */
};

static char *lcd_bt_pinmux_str[] = {
	"none",      /* 0 */
	"bt656_on",  /* 1 */
	"bt656_off", /* 2 */
	"bt1120_on", /* 3 */
	"bt1120_off" /* 4 */
};
#endif

#if defined(CONFIG_AML_LCD_VBYONE)
static char *lcd_vbyone_pinmux_str[] = {
	"none",      /* 0 */
	"vbyone",    /* 1 */
	"vbyone_off" /* 2 */
};
#endif

#if defined(CONFIG_AML_LCD_TCON)
static char *lcd_tcon_pinmux_str[] = {
	"none"	         /* 0 */
	"tcon_p2p",      /* 1 */
	"tcon_p2p_usit", /* 2 */
	"tcon_p2p_off",  /* 3 */
	"tcon_mlvds",    /* 4 */
	"tcon_mlvds_off" /* 5 */
};
#endif

#if defined(CONFIG_AML_LCD_MIPI_DSI)
static char *lcd_mipi_pinmux_str[] = {
	"none",   /* 0 */
	"dsi_on", /* 1 */
	"dsi_off" /* 2 */
};
#endif

static char *lcd_pinmux_index_get(struct aml_lcd_drv_s *pdrv, int status, unsigned int *index)
{
	struct lcd_config_s *pconf;
	unsigned int pinmux_idx = 0;
#if defined(CONFIG_AML_LCD_TCON)
	unsigned int p2p_type;
#endif
	char *pinmux_str = NULL;

	pconf = &pdrv->config;
	switch (pconf->basic.lcd_type) {
#if defined(CONFIG_AML_LCD_DPI)
	case LCD_RGB:
		if (status) {
			if (pconf->control.rgb_cfg.sync_valid &&
			    pconf->control.rgb_cfg.de_valid) {
				pinmux_idx = 3;
			} else if (pconf->control.rgb_cfg.de_valid) {
				pinmux_idx = 2;
			} else if (pconf->control.rgb_cfg.sync_valid) {
				pinmux_idx = 1;
			} else {
				return NULL;
			}
		} else {
			pinmux_idx = 4;
		}
		pinmux_str = lcd_rgb_pinmux_str[pinmux_idx];
		break;
	case LCD_BT656:
	case LCD_BT1120:
		if (pconf->basic.lcd_type == LCD_BT656)
			pinmux_idx = status ? 1 : 2;
		else if (pconf->basic.lcd_type == LCD_BT1120)
			pinmux_idx = status ? 3 : 4;
		else
			return NULL;
		pinmux_str = lcd_bt_pinmux_str[pinmux_idx];
		break;
#endif
#if defined(CONFIG_AML_LCD_VBYONE)
	case LCD_VBYONE:
		pinmux_idx = status ? 1 : 2;
		pinmux_str = lcd_vbyone_pinmux_str[pinmux_idx];
		break;
#endif
#if defined(CONFIG_AML_LCD_TCON)
	case LCD_MLVDS:
		pinmux_idx = status ? 4 : 5;
		pinmux_str = lcd_tcon_pinmux_str[pinmux_idx];
		break;
	case LCD_P2P:
		p2p_type = pconf->control.p2p_cfg.p2p_type & 0x1f;
		if (p2p_type == P2P_USIT)
			pinmux_idx = status ? 2 : 3;
		else
			pinmux_idx = status ? 1 : 3;
		pinmux_str = lcd_tcon_pinmux_str[pinmux_idx];
		break;
#endif
#if defined(CONFIG_AML_LCD_MIPI_DSI)
	case LCD_MIPI:
		pinmux_idx = status ? 1 : 2;
		pinmux_str = lcd_mipi_pinmux_str[pinmux_idx];
		break;
#endif
	default:
		break;
	}

	*index = pinmux_idx;
	return pinmux_str;
}

void lcd_pinmux_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int index;
	char *pinmux_str = NULL;

	if (strcmp(pdrv->config.cus_pinmux_name, "null")) {
		lcd_custom_pinmux_set(pdrv, status);
		return;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: pinmux_set: %d\n", pdrv->index, status);

	pinmux_str = lcd_pinmux_index_get(pdrv, status, &index);
	if (!pinmux_str) {
		LCDERR("[%d]: pinmux_set: %s invalid\n",
		       pdrv->index, lcd_type_type_to_str(pdrv->config.basic.lcd_type));
		return;
	}
	if (index == 0)
		return;
	if (pdrv->config.pinmux_flag == index) {
		LCDPR("[%d]: pinmux_set: %s is already selected\n", pdrv->index, pinmux_str);
		return;
	}

	/* request pinmux */
	if (pinctrl_select_state(pdrv->dev, pinmux_str)) {
		LCDERR("[%d]: pinmux_set: %s(%d) error\n", pdrv->index, pinmux_str, index);
		return;
	}

	pdrv->config.pinmux_flag = index;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: pinmux_set: %s(%d)\n", pdrv->index, pinmux_str, index);
}
