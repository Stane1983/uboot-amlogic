/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _LCD_EXTERN_H_
#define _LCD_EXTERN_H_
#include <amlogic/media/vout/lcd/lcd_extern.h>
#include <amlogic/media/vout/lcd/lcd_i2c_dev.h>

#define EXTPR(fmt, args...)     printf("lcd extern: "fmt"", ## args)
#define EXTERR(fmt, args...)    printf("lcd extern: error: "fmt"", ## args)

#define LCD_EXTERN_DRIVER		"lcd_extern"

void udelay(unsigned long usec);
void mdelay(unsigned long msec);

/* common API */
#ifdef CONFIG_OF_LIBFDT
char *lcd_extern_get_dts_prop(int nodeoffset, char *propname);
int lcd_extern_get_dts_child(char *dtaddr, char *snode, int index);
#endif
int lcd_extern_load_config(struct lcd_extern_driver_s *edrv, char *dtaddr, int load_id,
			   int *ext_index_lut);

void spi_gpio_init(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev);
void spi_gpio_off(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev);
int lcd_extern_spi_read(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			unsigned char reg, unsigned char *buf);
int lcd_extern_spi_write(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			 unsigned char *buf, int len);

int lcd_extern_gpio_get(struct lcd_extern_driver_s *ext_drv, unsigned char index);
int lcd_extern_gpio_set(struct lcd_extern_driver_s *ext_drv, unsigned char index, int value);
void lcd_extern_pinmux_set(struct lcd_extern_driver_s *ext_drv, int status);

void lcd_extern_check_add(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			  int cmd_step, unsigned char *data_buf, unsigned char data_len);
void lcd_extern_check_handler(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			      unsigned char i2c_bus, unsigned char i2c_addr, unsigned char cmd_type,
			      unsigned char *raw_table, unsigned char data_len);
int lcd_extern_cmd_multi_id(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			    unsigned char multi_id);
int lcd_extern_cmd_gpio(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			int cmd_step, unsigned char *data_buf, unsigned char data_len);
int lcd_extern_cmd_wait_gpio(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			     int cmd_step, unsigned char *data_buf, unsigned char data_len);
int lcd_extern_cmd_delay(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
			 int cmd_step, unsigned char *data_buf, unsigned char data_len);
int lcd_extern_cmd_i2c(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev,
		       int cmd_step, unsigned char cmd_type,
		       unsigned char *data_buf, unsigned char data_len);
int lcd_extern_power_cmd(struct lcd_extern_driver_s *edrv,
			 struct lcd_extern_dev_s *edev, int flag);

/* specific API */
int lcd_extern_default_probe(struct lcd_extern_driver_s *edrv,
			     struct lcd_extern_dev_s *ext_dev);
int lcd_extern_mipi_default_probe(struct lcd_extern_driver_s *edrv,
				  struct lcd_extern_dev_s *ext_dev);

#ifdef CONFIG_AML_LCD_EXTERN_I2C_RT6947
int lcd_extern_i2c_RT6947_probe(struct lcd_extern_driver_s *edrv,
				struct lcd_extern_dev_s *ext_dev);
#endif
#ifdef CONFIG_AML_LCD_EXTERN_I2C_ANX6862_7911
int lcd_extern_i2c_ANX6862_7911_probe(struct lcd_extern_driver_s *edrv,
				      struct lcd_extern_dev_s *ext_dev);
#endif
#ifdef CONFIG_AML_LCD_EXTERN_I2C_OLED
int lcd_extern_i2c_oled_probe(struct lcd_extern_driver_s *edrv,
			      struct lcd_extern_dev_s *ext_dev);
#endif

#endif

