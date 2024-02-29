// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 */

/*-----------------------------------------------------------------------
 * This helps initialize the SPI panel in U-Boot and display the logo.
 * It is applicable to drivers for ST7789V, ST7796S, and others.
 */
#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <asm-generic/gpio.h>
#include <lcd.h>
#include <mipi_display.h>

#define ONLY_CMD 0
#define NOT_ONLY_CMD 1
#define DATA 2
#define SPI_LCD_DEBUG 0
#ifndef CONFIG_DEFAULT_SPI_BUS
#define CONFIG_DEFAULT_SPI_BUS 2
#endif
#ifndef CONFIG_DEFAULT_SPI_MODE
#define CONFIG_DEFAULT_SPI_MODE SPI_MODE_0
#endif
#ifndef NUMARGS
#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
#endif
#ifndef write_reg
#define write_reg(...)                                            \
	(fb_write_register(NUMARGS(__VA_ARGS__), __VA_ARGS__))
#endif

/*
 * The maximum number of transfer commands.
 * Example usage: write_reg(par, 0xd0, 0xa4, 0xa1); // In this example, the count is 3.
 */
const int MAX_CMD_COUNT = 30;

vidinfo_t panel_info = {
	.vl_col = CONFIG_AML_SPI_LCD_COL,
	.vl_row = CONFIG_AML_SPI_LCD_ROW,
	.vl_bpix = LCD_BPP,
};

int osd_enabled = 1;
struct spi_slave *slave;
u8 *init_buf;
u8 *logo_buf;

#ifdef CONFIG_SPLASH_SCREEN
int splash_screen_prepare(void)
{
	env_set("logoPart", CONFIG_AML_SPI_LCD_LOGO_PART);
	env_set("logoPath", CONFIG_AML_SPI_LCD_LOGO_PATH);
	env_set_hex("logoLoadAddr", (ulong)CONFIG_AML_SPI_LCD_LOGO_LOAD_ADDR);
	env_set("logoLoadCmd", "rdext4pic ${logoPart} ${logoLoadAddr} ${logoPath}");
	return run_command("run logoLoadCmd", 0);
}
#endif

static int gpio_option(const char *gpiopin, int dir, bool enable)
{
	int ret;
	struct gpio_desc gpio_option_desc;

	ret = dm_gpio_lookup_name(gpiopin, &gpio_option_desc);
	if (ret) {
		printf("[LCD]%s: not found\n", gpiopin);
		return ret;
	}

	ret = dm_gpio_request(&gpio_option_desc, gpiopin);
	if (ret && ret != -EBUSY) {
		printf("[LCD]GPIO: requesting pin %s failed\n", gpiopin);
		return ret;
	}
	ret = dm_gpio_set_dir_flags(&gpio_option_desc, dir);
	if (ret) {
		printf("[LCD]Set direction failed\n");
		return ret;
	}

	dm_gpio_set_value(&gpio_option_desc, enable ? 1 : 0);

	return 0;
}

static int spi_xfer_write(u8 *buf, size_t len, int trans_flag)
{
	int ret = 0;

	if (trans_flag == ONLY_CMD) {
		gpio_option(CONFIG_AML_SPI_LCD_DC_GPIO, GPIOD_IS_OUT, 0);
		ret = spi_xfer(slave, 8, &buf[0], NULL, SPI_XFER_ONCE);
#if SPI_LCD_DEBUG
		printf("[LCD]Cmd is: 0x%02x\n", buf[0]);
#endif
	} else if (trans_flag == NOT_ONLY_CMD) {
		gpio_option(CONFIG_AML_SPI_LCD_DC_GPIO, GPIOD_IS_OUT, 0);
		ret = spi_xfer(slave, 8, &buf[0], NULL, SPI_XFER_BEGIN);
#if SPI_LCD_DEBUG
		printf("[LCD]Cmd is: 0x%02x\n", buf[0]);
#endif
	} else if (trans_flag == DATA) {
		gpio_option(CONFIG_AML_SPI_LCD_DC_GPIO, GPIOD_IS_OUT, 1);
		ret = spi_xfer(slave, (len - 1) * 8, &buf[0], NULL, SPI_XFER_BEGIN);
		ret = spi_xfer(slave, 8, &buf[len - 1], NULL, SPI_XFER_END);
#if SPI_LCD_DEBUG
		for (int i = 0; i < len; i++)
			printf("%02x ", buf[i]);
#endif
	} else {
		printf("[LCD]%s: Trans flag is invalid\n", __func__);
	}

	return ret;
}

void fb_write_register(int len, ...)
{
	va_list args;
	int i;

	va_start(args, len);
#if SPI_LCD_DEBUG
	printf("[LCD]The number of transfer commands: %d\n", len);
#endif
	if (len == 1) {
		/* Only write command */
		init_buf[0] = va_arg(args, unsigned int);
		/* Need keep DC low for all command bytes to transfer */
		spi_xfer_write(init_buf, 1, ONLY_CMD);
	} else if (len > 1) {
		/* Write command and data */
		for (i = 0; i < len; i++)
			init_buf[i] = va_arg(args, unsigned int);
		/* Need keep DC low for all command bytes to transfer */
		spi_xfer_write(init_buf, 1, NOT_ONLY_CMD);

		/* Need keep DC high for all data bytes to transfer */
		spi_xfer_write(init_buf + 1, len - 1, DATA);
	}
	va_end(args);
}

static void fb_set_addr_win(int xs, int ys, int xe, int ye)
{
	write_reg(MIPI_DCS_SET_COLUMN_ADDRESS,
		  (xs >> 8) & 0xFF, xs & 0xFF, (xe >> 8) & 0xFF, xe & 0xFF);

	write_reg(MIPI_DCS_SET_PAGE_ADDRESS,
		  (ys >> 8) & 0xFF, ys & 0xFF, (ye >> 8) & 0xFF, ye & 0xFF);

	write_reg(MIPI_DCS_WRITE_MEMORY_START);
}

static int fb_spi_init(unsigned int bus, unsigned int cs,
	unsigned int speed, unsigned int mode)
{
	int ret = 0;
	int wordlen = 8;
	char name[30], *str;
	struct udevice *dev;

	snprintf(name, sizeof(name), "spi_panel_%d:%d", bus, cs);
	str = strdup(name);
	if (!str)
		return -ENOMEM;

	ret = spi_get_bus_and_cs(bus, cs, speed, mode, "spi_generic_drv",
				 str, &dev, &slave);
	if (ret)
		return ret;
	slave->wordlen = wordlen;
	return 0;
}

static int fb_trans_init(void)
{
	int ret = 0;

	init_buf = malloc(MAX_CMD_COUNT * 8);
	ret = spi_claim_bus(slave);
	if (ret) {
		spi_release_bus(slave);
		return ret;
	}
	return 0;
}

static void fb_trans_release(void)
{
	free(init_buf);
	spi_release_bus(slave);
}

void fb_lcd_sync(void)
{
	int ret = 0;

	ret = fb_trans_init();
	if (ret) {
		printf("[LCD]Error: fb initialization failed\n");
		return;
	}

	fb_set_addr_win(0, 0, CONFIG_AML_SPI_LCD_COL - 1, CONFIG_AML_SPI_LCD_ROW - 1);
	ret = spi_xfer_write(logo_buf,
				CONFIG_AML_SPI_LCD_COL * CONFIG_AML_SPI_LCD_ROW * 2, DATA);
	if (ret)
		printf("[LCD]Error: Failed to send logo data via SPI\n");

	fb_trans_release();
}

extern void fb_display_init(void);
void lcd_ctrl_init(void *lcdbase)
{
	int ret = 0;
#ifdef CONFIG_SPLASH_SCREEN
	char addr_str[15];

	sprintf(addr_str, "0x%x", CONFIG_AML_SPI_LCD_LOGO_LOAD_ADDR);
	env_set("splashimage", addr_str);
#endif
	logo_buf = (u8 *)lcdbase;
	/* Reset */
	gpio_option(CONFIG_AML_SPI_LCD_RESET_GPIO, GPIOD_IS_OUT, 1);
	udelay(10);
	gpio_option(CONFIG_AML_SPI_LCD_RESET_GPIO, GPIOD_IS_OUT, 0);
	udelay(10);
	gpio_option(CONFIG_AML_SPI_LCD_RESET_GPIO, GPIOD_IS_OUT, 1);
	udelay(10);

	/* SPI init */
	ret = fb_spi_init(CONFIG_DEFAULT_SPI_BUS, CONFIG_DEFAULT_SPI_MODE,
					CONFIG_AML_SPI_LCD_MAX_CLOCK, SPI_MODE_0);
	if (ret) {
		printf("[LCD]Error: fb SPI initialization failed\n");
		return;
	}
	ret = fb_trans_init();
	if (ret) {
		printf("[LCD]Error: fb initialization failed\n");
		return;
	}

	/* LCD init */
	fb_display_init();
	fb_trans_release();
}

extern void fb_display_on(void);
void lcd_enable(void)
{
	int ret = 0;

	ret = fb_trans_init();
	if (ret) {
		printf("[LCD]Error: fb initialization failed\n");
		return;
	}
	/* Turn on the display */
	fb_display_on();
	/* Turn on backlight */
	gpio_option(CONFIG_AML_SPI_LCD_BACKLIGHT_GPIO, GPIOD_IS_OUT, 1);
	fb_trans_release();
}

void lcd_setcolreg(ushort regno, ushort red, ushort green, ushort blue) {}

