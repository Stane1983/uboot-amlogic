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
#include "../../lcd_unifykey.h"
#include "../../lcd_common.h"
#include "../lcd_bl.h"
#include "ldim_drv.h"
#include "ldim_dev_drv.h"
#include "env.h"

static int ldim_pinmux_load_from_bsp(struct aml_ldim_driver_s *ldim_drv, const char *str,
				     struct bl_pwm_config_s *bl_pwm)
{
	unsigned int i, j;
	int set_cnt = 0, clr_cnt = 0;
	struct lcd_pinmux_ctrl_s *pinmux;

	pinmux = ldim_drv->data->dft_conf[0]->ldim_pinmux;
	if (!pinmux) {
		LDIMERR("%s: ldim_pinmux is null\n", __func__);
		return -1;
	}
	for (i = 0; i < LCD_PINMX_MAX; i++) {
		if (strncmp(pinmux->name, "invalid", 7) == 0)
			break;
		if (strncmp(pinmux->name, str, strlen(str)) == 0) {
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				bl_pwm->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
				bl_pwm->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				bl_pwm->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
				bl_pwm->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		pinmux++;
	}
	if (set_cnt < LCD_PINMUX_NUM) {
		bl_pwm->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		bl_pwm->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		bl_pwm->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		bl_pwm->pinmux_clr[clr_cnt][1] = 0x0;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			LDIMPR("%s set: %d, 0x%08x\n",
			       str,
			       bl_pwm->pinmux_set[i][0],
			       bl_pwm->pinmux_set[i][1]);
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (bl_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			LDIMPR("%s clr: %d, 0x%08x\n",
			       str,
			       bl_pwm->pinmux_clr[i][0],
			       bl_pwm->pinmux_clr[i][1]);
			i++;
		}
	}

	return 0;
}

static char *ldim_pinmux_str[] = {
	"ldim_pwm_pin",        /* 0 */
	"ldim_pwm_vs_pin",     /* 1 */
	"analog_pwm_pin",      /* 2 */
	"none",
};

static int ldim_pinmux_load(char *dt_addr, struct aml_ldim_driver_s *ldim_drv)
{
	struct bl_pwm_config_s *bl_pwm;
	char *str;
	int ret = 0;

	if (!ldim_drv->dev_drv)
		return -1;

	/* ldim_pwm */
	bl_pwm = &ldim_drv->dev_drv->ldim_pwm_config;
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		if (bl_pwm->pwm_port == BL_PWM_VS)
			str = ldim_pinmux_str[1];
		else
			str = ldim_pinmux_str[0];
		ret = ldim_pinmux_load_from_bsp(ldim_drv, str, bl_pwm);
		if (ret)
			return ret;
	}

	/* analog_pwm */
	bl_pwm = &ldim_drv->dev_drv->analog_pwm_config;
	if (bl_pwm->pwm_port < BL_PWM_VS) {
		str = ldim_pinmux_str[2];
		ret = ldim_pinmux_load_from_bsp(ldim_drv, str, bl_pwm);
	}

	return ret;
}

static int ldim_dev_zone_mapping_load(struct ldim_dev_driver_s *dev_drv, const char *path)
{
	int bin_size = 0, map_size;
	unsigned char *buf;
	int i, j;

	/* 2byte per zone */
	map_size = dev_drv->zone_num * 2;

	//ret = handle_ldim_dev_zone_mapping_get(buf, size, path);
	buf = model_read_file_to_buffer(path, &bin_size);
	if (!buf) {
		LDIMERR("%s: load bin: %s error\n", __func__, path);
		return -1;
	}
	if (bin_size > map_size) {
		memset(buf, 0, bin_size);
		free(buf);
		return -1;
	}

	for (i = 0; i < dev_drv->zone_num; i++) {
		j = 2 * i;
		dev_drv->bl_mapping[i] = buf[j] | (buf[j + 1] << 8);
	}

	LDIMPR("%s: load bin: %s finish\n", __func__, path);
	memset(buf, 0, bin_size);
	free(buf);
	return 0;
}

#ifdef CONFIG_OF_LIBFDT
static int ldim_dev_init_table_handle_dts(char *dtaddr, int nodeoffset,
					  struct ldim_dev_driver_s *dev_drv)
{
	int len_on, len_off, init_len, init_buf_size;
	unsigned int *init_buf;
	char *init_on, *init_off;
	unsigned char *table;
	int i = 0;

	init_on = (char *)fdt_getprop(dtaddr, nodeoffset, "init_on", &len_on);
	if (!init_on) {
		LDIMERR("%s: get init_on failed\n", dev_drv->name);
		return -1;
	}
	init_off = (char *)fdt_getprop(dtaddr, nodeoffset, "init_off", &len_off);
	if (!init_off) {
		LDIMERR("%s: get init_off failed\n", dev_drv->name);
		return -1;
	}
	len_on /= 4;
	len_off /= 4;
	init_len = len_on >= len_off ? len_on : len_off;
	if (init_len <= 0)
		return 0;

	init_buf_size = init_len * sizeof(unsigned int);
	init_buf = (unsigned int *)malloc(init_buf_size);
	if (!init_buf) {
		LDIMERR("%s: alloc memory error\n", __func__);
		return -1;
	}

	//init_on
	for (i = 0; i < len_on; i++)
		init_buf[i] = be32_to_cpup((((u32 *)init_on) + i));
	table = lcd_init_table_load_array("ldim_dev_init_on", LCD_EXT_CMD_SIZE_DYNAMIC,
				init_buf, len_on, LDIM_INIT_ON_MAX, &init_len);
	if (!table)
		goto ldim_dev_init_table_handle_dts_err;
	dev_drv->init_on = table;
	dev_drv->init_on_cnt = init_len;

	//init_on
	for (i = 0; i < len_off; i++)
		init_buf[i] = be32_to_cpup((((u32 *)init_off) + i));
	table = lcd_init_table_load_array("ldim_dev_init_off", LCD_EXT_CMD_SIZE_DYNAMIC,
				init_buf, len_off, LDIM_INIT_OFF_MAX, &init_len);
	if (!table)
		goto ldim_dev_init_table_handle_dts_err;
	dev_drv->init_off = table;
	dev_drv->init_off_cnt = init_len;

	dev_drv->init_loaded = 1;

	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return 0;

ldim_dev_init_table_handle_dts_err:
	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return -1;
}

static int ldim_dev_get_config_from_dts(struct ldim_dev_driver_s *dev_drv,
					char *dt_addr, int index)
{
	int child_offset;
	char *propname, *propdata;
	const char *str;
	int temp;
	struct bl_pwm_config_s *bl_pwm;
	char dbg_str[160];
	int i, dbg_str_len = 0, ret = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("load ldim_dev config from dts\n");

	/* get device config */
	propname = (char *)malloc(50);
	if (!propname) {
		LDIMERR("%s: propname malloc failed\n", __func__);
		return -1;
	}
	memset(propname, 0, 50);
	sprintf(propname, "/local_dimming_device/ldim_dev_%d", dev_drv->index);
	child_offset = fdt_path_offset(dt_addr, propname);
	if (child_offset < 0) {
		LDIMERR("not find %s node: %s\n", propname, fdt_strerror(child_offset));
		free(propname);
		return -1;
	}
	free(propname);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_dev_name", NULL);
	if (!propdata)
		LDIMERR("failed to get ldim_dev_name\n");
	else
		strlcpy(dev_drv->name, propdata, LDIM_DEV_NAME_MAX);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "type", NULL);
	if (!propdata) {
		LDIMERR("failed to get type\n");
		return -1;
	}
	dev_drv->type = be32_to_cpup((u32 *)propdata);
	if (dev_drv->type >= LDIM_DEV_TYPE_MAX) {
		LDIMERR("invalid type %d\n", dev_drv->type);
		return -1;
	}

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		/* get spi config */
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_bus_num", NULL);
		if (!propdata) {
			LDIMERR("failed to get spi_bus_num\n");
		} else {
			temp = be32_to_cpup((u32 *)propdata);
			dev_drv->spi_info.bus_num[0] = temp & 0xf;
			dev_drv->spi_info.bus_num[1] = (temp >> 4) & 0xf;
			if (dev_drv->spi_info.bus_num[1])
				dev_drv->spi_info.spi_dev_num = 2;
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_chip_select", NULL);
		if (!propdata) {
			LDIMERR("failed to get spi_chip_select\n");
		} else {
			temp = be32_to_cpup((u32 *)propdata);
			dev_drv->spi_info.chip_select[0] = temp & 0xf;
			dev_drv->spi_info.chip_select[1] = (temp >> 4) & 0xf;
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_max_frequency", NULL);
		if (!propdata)
			LDIMERR("failed to get spi_max_frequency\n");
		else
			dev_drv->spi_info.max_speed_hz = be32_to_cpup((u32 *)propdata);

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_mode", NULL);
		if (!propdata)
			LDIMERR("failed to get spi_mode\n");
		else
			dev_drv->spi_info.mode = be32_to_cpup((u32 *)propdata);

		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("spi bus_num=%d:%d, chip_select=%d:%d",
				dev_drv->spi_info.bus_num[0], dev_drv->spi_info.bus_num[1],
				dev_drv->spi_info.chip_select[0], dev_drv->spi_info.chip_select[1]);
			LDIMPR("max_frequency=%d, mode=%d, spi_dev_num:%d\n",
			       dev_drv->spi_info.max_speed_hz,
			       dev_drv->spi_info.mode, dev_drv->spi_info.spi_dev_num);
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_cs_delay", NULL);
		if (!propdata) {
			LDIMERR("failed to get spi_cs_delay\n");
		} else {
			dev_drv->cs_hold_delay = be32_to_cpup((u32 *)propdata);
			dev_drv->cs_clk_delay = be32_to_cpup((((u32 *)propdata) + 1));
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("cs_hold_delay=%dus, cs_clk_delay=%dus\n",
			       dev_drv->cs_hold_delay,
			       dev_drv->cs_clk_delay);
		}
		break;
	default:
		break;
	}

	/* ldim pwm */
	bl_pwm = &dev_drv->ldim_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_port", NULL);
	if (!propdata) {
		LDIMERR("failed to get ldim_pwm_port\n");
		bl_pwm->pwm_port = BL_PWM_MAX;
	} else {
		bl_pwm->pwm_port = bl_pwm_str_to_num(propdata);
		LDIMPR("ldim_pwm_port: %s(0x%x)\n", propdata, bl_pwm->pwm_port);
	}
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_attr", NULL);
		if (!propdata) {
			LDIMERR("failed to get ldim_pwm_attr\n");
			bl_pwm->pwm_method = BL_PWM_POSITIVE;
			if (bl_pwm->pwm_port == BL_PWM_VS)
				bl_pwm->pwm_freq = 1;
			else
				bl_pwm->pwm_freq = 60;
			bl_pwm->pwm_duty = 50;
			bl_pwm->pwm_phase = 0;
		} else {
			bl_pwm->pwm_method = be32_to_cpup((u32 *)propdata);
			temp = be32_to_cpup((((u32 *)propdata) + 1));
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				bl_pwm->pwm_freq = temp & 0xff;
				bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;
			} else {
				bl_pwm->pwm_freq = temp;
				bl_pwm->pwm_phase = 0;
			}
			bl_pwm->pwm_duty = be32_to_cpup((((u32 *)propdata) + 2));
		}
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			if (bl_pwm->pwm_freq > 8) {
				LDIMERR("pwm_vs wrong freq %d\n", bl_pwm->pwm_freq);
				bl_pwm->pwm_freq = BL_FREQ_VS_DEFAULT;
			}
		} else {
			if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
				bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		}
		LDIMPR("get ldim_pwm pol = %d, freq = %d, default duty = %d%%, phase=%d\n",
		       bl_pwm->pwm_method, bl_pwm->pwm_freq, bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	/* analog pwm */
	bl_pwm = &dev_drv->analog_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "analog_pwm_port", NULL);
	if (!propdata)
		bl_pwm->pwm_port = BL_PWM_MAX;
	else
		bl_pwm->pwm_port = bl_pwm_str_to_num(propdata);
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		LDIMPR("find analog_pwm_port: %s(0x%x)\n", propdata, bl_pwm->pwm_port);
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "analog_pwm_attr", NULL);
		if (!propdata) {
			LDIMERR("failed to get analog_pwm_attr\n");
			bl_pwm->pwm_method = BL_PWM_POSITIVE;
			if (bl_pwm->pwm_port == BL_PWM_VS)
				bl_pwm->pwm_freq = 1;
			else
				bl_pwm->pwm_freq = 60;
			bl_pwm->pwm_duty_max = 100;
			bl_pwm->pwm_duty_min = 20;
			bl_pwm->pwm_duty = 50;
			bl_pwm->pwm_phase = 0;
		} else {
			bl_pwm->pwm_method = be32_to_cpup((u32 *)propdata);
			temp = be32_to_cpup((((u32 *)propdata) + 1));
			if (bl_pwm->pwm_port == BL_PWM_VS) {
				bl_pwm->pwm_freq = temp & 0xff;
				bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;
			} else {
				bl_pwm->pwm_freq = temp;
				bl_pwm->pwm_phase = 0;
			}
			bl_pwm->pwm_duty_max = be32_to_cpup((((u32 *)propdata) + 2));
			bl_pwm->pwm_duty_min = be32_to_cpup((((u32 *)propdata) + 3));
			bl_pwm->pwm_duty = be32_to_cpup((((u32 *)propdata) + 4));
		}
		if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
			bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		LDIMPR("get analog_pwm pol = %d, freq = %d, duty_max = %d%%\n",
		       bl_pwm->pwm_method, bl_pwm->pwm_freq, bl_pwm->pwm_duty_max);
		LDIMPR("duty_min = %d%%, default duty = %d%%, phase=%d\n",
		       bl_pwm->pwm_duty_min, bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_pinmux_sel", NULL);
	if (propdata) {
		LDIMPR("find custom ldim_pwm_pinmux_sel: %s\n", propdata);
		strlcpy(dev_drv->pinmux_name, propdata, LDIM_DEV_NAME_MAX);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "en_gpio_on_off", NULL);
	if (!propdata) {
		LDIMERR("failed to get en_gpio_on_off\n");
	} else {
		dev_drv->en_gpio = be32_to_cpup((u32 *)propdata);
		dev_drv->en_gpio_on = be32_to_cpup((((u32 *)propdata) + 1));
		dev_drv->en_gpio_off = be32_to_cpup((((u32 *)propdata) + 2));
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		LDIMPR("en_gpio=%s(%d), en_gpio_on=%d, en_gpio_off=%d\n",
		       dev_drv->gpio_name[dev_drv->en_gpio],
		       dev_drv->en_gpio, dev_drv->en_gpio_on,
		       dev_drv->en_gpio_off);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "write_check", NULL);
	if (!propdata)
		dev_drv->write_check = 0;
	else
		dev_drv->write_check = (unsigned char)(be32_to_cpup((u32 *)propdata));
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("write_check=%d\n", dev_drv->write_check);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "dim_max_min", NULL);
	if (!propdata) {
		LDIMERR("failed to get dim_max_min\n");
		return -1;
	}
	dev_drv->dim_max = be32_to_cpup((u32 *)propdata);
	dev_drv->dim_min = be32_to_cpup((((u32 *)propdata) + 1));
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
		LDIMPR("dim_max=0x%03x, dim_min=0x%03x\n",
		       dev_drv->dim_max, dev_drv->dim_min);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "mcu_header", NULL);
	if (!propdata) {
		LDIMERR("failed to get mcu_header\n");
		dev_drv->mcu_header = 0;
	} else {
		dev_drv->mcu_header = (unsigned int)(be32_to_cpup((u32 *)propdata));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "mcu_dim", NULL);
	if (!propdata) {
		LDIMERR("failed to get mcu_dim\n");
		dev_drv->mcu_dim = 0;
	} else {
		dev_drv->mcu_dim = (unsigned int)(be32_to_cpup((u32 *)propdata));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "chip_count", NULL);
	if (!propdata)
		dev_drv->chip_cnt = 1;
	else
		dev_drv->chip_cnt = be32_to_cpup((u32 *)propdata);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_zone_mapping_path", NULL);
	if (propdata) {
		LDIMPR("%s:find custom ldim_zone_mapping_path\n", __func__);
		str = propdata;
		ret = ldim_dev_zone_mapping_load(dev_drv, str);
		if (ret) {
			for (i = 0; i < dev_drv->zone_num; i++)
				dev_drv->bl_mapping[i] = (unsigned short)i;
		}
		goto ldim_dev_get_config_from_dts_next;
	}
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_zone_mapping", NULL);
	if (!propdata) {
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_region_mapping", NULL);
		if (!propdata) {
			for (i = 0; i < dev_drv->zone_num; i++)
				dev_drv->bl_mapping[i] = (unsigned short)i;
			goto ldim_dev_get_config_from_dts_next;
		}
	}
	LDIMPR("%s:find custom ldim_zone_mapping\n", __func__);
	for (i = 0; i < dev_drv->zone_num; i++)
		dev_drv->bl_mapping[i] = (unsigned short)be32_to_cpup((((u32 *)propdata) + i));

ldim_dev_get_config_from_dts_next:
	dbg_str_len += sprintf(dbg_str + dbg_str_len, "mcu_header=0x%08x, mcu_dim=0x%08x, ",
		dev_drv->mcu_header, dev_drv->mcu_dim);
	sprintf(dbg_str + dbg_str_len, "chip_cnt:%d, cus pwm_pinmux_sel:%s",
		dev_drv->chip_cnt, dev_drv->pinmux_name);
	LDIMPR("load dts config: %s[%d]: type:%d, %s\n",
	       dev_drv->name, dev_drv->index, dev_drv->type, dbg_str);

	/* get init_cmd */
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "cmd_size", NULL);
	if (!propdata) {
		LDIMPR("no cmd_size\n");
	} else {
		temp = be32_to_cpup((u32 *)propdata);
		dev_drv->cmd_size = (unsigned char)temp;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("cmd_size=%d\n", dev_drv->cmd_size);
	if (dev_drv->cmd_size != LCD_EXT_CMD_SIZE_DYNAMIC)
		goto ldim_dev_get_config_from_dts_end;

	ret = ldim_dev_init_table_handle_dts(dt_addr, child_offset, dev_drv);

ldim_dev_get_config_from_dts_end:
	return ret;
}

static int ldim_dev_init_table_handle_ukey(struct ldim_dev_driver_s *dev_drv,
					   unsigned char *p, int len)
{
	unsigned int *init_buf;
	unsigned char *table;
	int init_buf_size, init_offset, init_len, data_cnt;
	int i;

	init_offset = LCD_UKEY_LDIM_DEV_INIT;
	data_cnt = len - LCD_UKEY_LDIM_DEV_INIT;
	if (data_cnt <= 0)
		return 0;

	init_buf_size = data_cnt * sizeof(unsigned int);
	init_buf = (unsigned int *)malloc(init_buf_size);
	if (!init_buf) {
		LDIMERR("%s: alloc memory error\n", __func__);
		return -1;
	}
	for (i = 0; i < data_cnt; i++)
		init_buf[i] = *(p + init_offset + i);
	table = lcd_init_table_load_array("ldim_dev_init_on", LCD_EXT_CMD_SIZE_DYNAMIC,
				init_buf, data_cnt, LDIM_INIT_ON_MAX, &init_len);
	if (!table)
		goto ldim_dev_init_table_handle_ukey_err;
	dev_drv->init_on = table;
	dev_drv->init_on_cnt = init_len;

	init_offset += dev_drv->init_on_cnt;
	data_cnt -= dev_drv->init_on_cnt;
	if (data_cnt > 0) {
		for (i = 0; i < data_cnt; i++)
			init_buf[i] = *(p + init_offset + i);
		table = lcd_init_table_load_array("ldim_dev_init_off", LCD_EXT_CMD_SIZE_DYNAMIC,
					init_buf, data_cnt, LDIM_INIT_OFF_MAX, &init_len);
		if (!table)
			goto ldim_dev_init_table_handle_ukey_err;
		dev_drv->init_off = table;
		dev_drv->init_off_cnt = init_len;
	} else {
		dev_drv->init_off_cnt = 0;
	}

	dev_drv->init_loaded = 1;

	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return 0;

ldim_dev_init_table_handle_ukey_err:
	memset(init_buf, 0, init_buf_size);
	free(init_buf);
	return -1;
}

static int ldim_dev_get_config_from_ukey(struct ldim_dev_driver_s *dev_drv)
{
	unsigned char *para, *p;
	int key_len, len;
	const char *str;
	unsigned int temp;
	struct bl_pwm_config_s *bl_pwm;
	char dbg_str[160];
	int i, dbg_str_len = 0, ret = 0;

	ret = lcd_unifykey_get_size("ldim_dev", &key_len);
	if (ret)
		return -1;
	para = (unsigned char *)malloc(key_len);
	if (!para)
		return -1;
	memset(para, 0, key_len);

	ret = lcd_unifykey_get("ldim_dev", para, key_len);
	if (ret < 0)
		goto ldim_dev_get_config_from_ukey_err;

	/* step 1: check header */
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		lcd_unifykey_header_print(para);

	/* step 2: check backlight parameters */
	len = 65; //10+30+25
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret < 0) {
		LDIMERR("unifykey length is incorrect\n");
		goto ldim_dev_get_config_from_ukey_err;
	}

	/* basic: 30byte */
	p = para;
	str = (const char *)(p + LCD_UKEY_HEAD_SIZE);
	strlcpy(dev_drv->name, str, LDIM_DEV_NAME_MAX);

	dev_drv->index = 0;

	/* interface (25Byte) */
	dev_drv->type = *(p + LCD_UKEY_LDIM_DEV_IF_TYPE);
	if (dev_drv->type >= LDIM_DEV_TYPE_MAX) {
		LDIMERR("invalid type %d\n", dev_drv->type);
		goto ldim_dev_get_config_from_ukey_err;
	}

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		temp = *(p + LCD_UKEY_LDIM_DEV_IF_ATTR_0);
		dev_drv->spi_info.bus_num[0] = temp & 0xf;
		dev_drv->spi_info.bus_num[1] = (temp >> 4) & 0xf;
		if (dev_drv->spi_info.bus_num[1])
			dev_drv->spi_info.spi_dev_num = 2;
		temp = *(p + LCD_UKEY_LDIM_DEV_IF_ATTR_1);
		dev_drv->spi_info.chip_select[0] = temp & 0xf;
		dev_drv->spi_info.chip_select[1] = (temp >> 4) & 0xf;
		dev_drv->spi_info.max_speed_hz =
			(*(p + LCD_UKEY_LDIM_DEV_IF_FREQ) |
			((*(p + LCD_UKEY_LDIM_DEV_IF_FREQ + 1)) << 8) |
			((*(p + LCD_UKEY_LDIM_DEV_IF_FREQ + 2)) << 16) |
			((*(p + LCD_UKEY_LDIM_DEV_IF_FREQ + 3)) << 24));
		dev_drv->spi_info.mode = *(p + LCD_UKEY_LDIM_DEV_IF_ATTR_2);
		dev_drv->cs_hold_delay =
			(*(p + LCD_UKEY_LDIM_DEV_IF_ATTR_4) |
			((*(p + LCD_UKEY_LDIM_DEV_IF_ATTR_4 + 1)) << 8));
		dev_drv->cs_clk_delay =
			(*(p + LCD_UKEY_LDIM_DEV_IF_ATTR_5) |
			((*(p + LCD_UKEY_LDIM_DEV_IF_ATTR_5 + 1)) << 8));
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("spi bus_num: %d:%d, chip_select: %d:%d, max_speed_hz: %d\n",
				dev_drv->spi_info.bus_num[0], dev_drv->spi_info.bus_num[1],
				dev_drv->spi_info.chip_select[0], dev_drv->spi_info.chip_select[1],
				dev_drv->spi_info.max_speed_hz);
			LDIMPR("spi_dev_num:%d, spi mode:%d, cs_hold_delay=%d, cs_clk_delay=%d\n",
			       dev_drv->spi_info.spi_dev_num, dev_drv->spi_info.mode,
				   dev_drv->cs_hold_delay, dev_drv->cs_clk_delay);
		}
		break;
	default:
		break;
	}

	/* pwm (48Byte) */
	bl_pwm = &dev_drv->ldim_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	bl_pwm->pwm_port = *(p + LCD_UKEY_LDIM_DEV_PWM_VS_PORT);
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		bl_pwm->pwm_method = *(p + LCD_UKEY_LDIM_DEV_PWM_VS_POL);
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			temp = (*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 1)) << 8) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 2)) << 16) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 3)) << 24));

			bl_pwm->pwm_freq = (temp & 0xff);
			bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;

		} else {
			bl_pwm->pwm_freq =
				(*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 1)) << 8) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 2)) << 16) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_FREQ + 3)) << 24));
			bl_pwm->pwm_phase = 0;
		}
		bl_pwm->pwm_duty =
			(*(p + LCD_UKEY_LDIM_DEV_PWM_VS_DUTY) |
			((*(p + LCD_UKEY_LDIM_DEV_PWM_VS_DUTY + 1)) << 8));

		if (bl_pwm->pwm_port == BL_PWM_VS) {
			if (bl_pwm->pwm_freq > 4) {
				LDIMERR("pwm_vs wrong freq %d\n", bl_pwm->pwm_freq);
				bl_pwm->pwm_freq = BL_FREQ_VS_DEFAULT;
			}
		} else {
			if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
				bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		}
		LDIMPR("get ldim_pwm pol = %d, freq = %d, dft duty = %d%%, phase=%d\n",
		       bl_pwm->pwm_method, bl_pwm->pwm_freq, bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	bl_pwm = &dev_drv->analog_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	bl_pwm->pwm_port = *(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_PORT);
	if (bl_pwm->pwm_port < BL_PWM_VS) {
		bl_pwm->pwm_method = *(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_POL);
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			temp = (*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 1)) << 8) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 2)) << 16) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 3)) << 24));
			bl_pwm->pwm_freq = (temp & 0xff);
			bl_pwm->pwm_phase = (temp >> 8) & 0xffffff;
		} else {
			bl_pwm->pwm_freq =
				(*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 1)) << 8) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 2)) << 16) |
				((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_FREQ + 3)) << 24));
			bl_pwm->pwm_phase = 0;
		}
		bl_pwm->pwm_duty =
			(*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_DUTY) |
			((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_DUTY + 1)) << 8));
		bl_pwm->pwm_duty_max =
			(*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_ATTR_0) |
			((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_ATTR_0 + 1)) << 8));
		bl_pwm->pwm_duty_min =
			(*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_ATTR_1) |
			((*(p + LCD_UKEY_LDIM_DEV_PWM_ADJ_ATTR_1 + 1)) << 8));

		if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
			bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		LDIMPR("get analog_pwm pol = %d, freq = %d\n",
		       bl_pwm->pwm_method, bl_pwm->pwm_freq);
		LDIMPR("duty max = %d%%, min = %d%%, default = %d%%, phase=%d\n",
		       bl_pwm->pwm_duty_max, bl_pwm->pwm_duty_min,
		       bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	str = (const char *)(p + LCD_UKEY_LDIM_DEV_PINMUX_SEL);
	if (strlen(str) == 0)
		strcpy(dev_drv->pinmux_name, "invalid");
	else
		strlcpy(dev_drv->pinmux_name, str, LDIM_DEV_NAME_MAX);

	/* ctrl (271Byte) */
	temp = *(p + LCD_UKEY_LDIM_DEV_EN_GPIO);
	if (temp >= BL_GPIO_NUM_MAX)
		dev_drv->en_gpio = LCD_GPIO_MAX;
	else
		dev_drv->en_gpio = temp;
	dev_drv->en_gpio_on = *(p + LCD_UKEY_LDIM_DEV_EN_GPIO_ON);
	dev_drv->en_gpio_off = *(p + LCD_UKEY_LDIM_DEV_EN_GPIO_OFF);

	temp = *(p + LCD_UKEY_LDIM_DEV_ERR_GPIO);
	if (temp >= BL_GPIO_NUM_MAX) {
		dev_drv->lamp_err_gpio = LCD_GPIO_MAX;
		dev_drv->fault_check = 0;
	} else {
		dev_drv->lamp_err_gpio = temp;
		dev_drv->fault_check = 1;
		ldim_gpio_set(dev_drv, dev_drv->lamp_err_gpio, LCD_GPIO_INPUT);
	}

	dev_drv->write_check = *(p + LCD_UKEY_LDIM_DEV_WRITE_CHECK);

	dev_drv->dim_max =
		(*(p + LCD_UKEY_LDIM_DEV_DIM_MAX) |
		((*(p + LCD_UKEY_LDIM_DEV_DIM_MAX + 1)) << 8));
	dev_drv->dim_min =
		(*(p + LCD_UKEY_LDIM_DEV_DIM_MIN) |
		((*(p + LCD_UKEY_LDIM_DEV_DIM_MIN + 1)) << 8));

	dev_drv->mcu_header =
		(*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_0) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_0 + 1)) << 8) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_0 + 2)) << 16) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_0 + 3)) << 24));
	dev_drv->mcu_dim =
		(*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_1) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_1 + 1)) << 8) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_1 + 2)) << 16) |
		((*(p + LCD_UKEY_LDIM_DEV_CUST_ATTR_1 + 3)) << 24));

	dev_drv->chip_cnt =
		(*(p + LCD_UKEY_LDIM_DEV_CHIP_COUNT) |
		((*(p + LCD_UKEY_LDIM_DEV_CHIP_COUNT + 1)) << 8));

	str = (const char *)(p + LCD_UKEY_LDIM_DEV_ZONE_MAP_PATH);
	if (strlen(str) == 0) {
		for (i = 0; i < dev_drv->zone_num; i++)
			dev_drv->bl_mapping[i] = (unsigned short)i;
	} else {
		str = env_get("bl_mapping_path");
		LDIMPR("find custom zone_mapping: %s\n", str);
		ret = ldim_dev_zone_mapping_load(dev_drv, str);
		if (ret) {
			for (i = 0; i < dev_drv->zone_num; i++)
				dev_drv->bl_mapping[i] = (unsigned short)i;
		}
	}

	dbg_str_len += sprintf(dbg_str + dbg_str_len, "mcu_header=0x%08x, mcu_dim=0x%08x, ",
		dev_drv->mcu_header, dev_drv->mcu_dim);
	sprintf(dbg_str + dbg_str_len, "chip_cnt:%d, cus pwm_pinmux_sel:%s",
		dev_drv->chip_cnt, dev_drv->pinmux_name);
	LDIMPR("load ukey config: %s: type:%d, %s\n", dev_drv->name, dev_drv->type, dbg_str);

	dev_drv->cmd_size = *(p + LCD_UKEY_LDIM_DEV_CMD_SIZE);
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("%s: cmd_size = %d\n", dev_drv->name, dev_drv->cmd_size);
	if (dev_drv->cmd_size != LCD_EXT_CMD_SIZE_DYNAMIC)
		goto ldim_dev_get_config_from_ukey_end;

	ret = ldim_dev_init_table_handle_ukey(dev_drv, p, key_len);
	if (ret)
		goto ldim_dev_get_config_from_ukey_err;

ldim_dev_get_config_from_ukey_end:
	memset(para, 0, key_len);
	free(para);
	return 0;

ldim_dev_get_config_from_ukey_err:
	memset(para, 0, key_len);
	free(para);
	return -1;
}

#if defined(CONFIG_AML_LCD_JSON) || defined(CONFIG_CMD_AML_MODEL)
static struct num_str_s ldim_dev_type_match[] = {
	{LDIM_DEV_TYPE_NORMAL, "NORMAL"},
	{LDIM_DEV_TYPE_SPI,    "SPI"},
	{LDIM_DEV_TYPE_I2C,    "I2C"},
	{LDIM_DEV_TYPE_MAX,    "MAX"}
};
#endif

/* config from json =============================================================================*/

#ifdef CONFIG_AML_LCD_JSON
static int ldim_gpio_name_to_index(struct ldim_dev_driver_s *drv, char *name)
{
	int i = 0;

	if (!drv || !name)
		return LCD_GPIO_MAX;

	for (i = 0; i < BL_GPIO_NUM_MAX; i++)
		if (!strcmp(drv->gpio_name[i], name))
			return i;
	return LCD_GPIO_MAX;
}

int ldim_dev_get_config_from_json(struct ldim_dev_driver_s *dev_drv)
{
	struct json_parse_s *jsp = get_panel_jsp(0);
	struct json_s *parent, *child, *child2, *child3;
	int i = 0, cnt, cnt_max, nums_size, data_cnt;
	const char *str = NULL;
	struct ldim_spi_dev_info_s *spi_info;
	struct bl_pwm_config_s *bl_pwm, *pwms[3] = {NULL, NULL, NULL};
	unsigned int *nums = NULL;
	unsigned char *table;
	int ret = 0;

	if (!json_parse_ok(jsp)) {
		LDIMERR("panel0 jsp not ok\n");
		return -1;
	}

	parent = json_path_to_node(jsp, jsp->root, "backlight/ldim_dev");
	if (!parent) {
		LDIMERR("failed find /backlight/ldim_dev\n");
		return -1;
	}

//basic_info
	child = json_get_object_child(jsp, parent, "basic_info");
	if (!child) {
		LDIMERR("fail to get basic_info\n");
		return -1;
	}

	str = json_get_obj_str(jsp, child, "name", NULL);
	strncpy(dev_drv->name, str, str ? LDIM_DEV_NAME_MAX - 1 : 0);
	dev_drv->index    = 0;
	dev_drv->chip_cnt = json_get_obj_u32(jsp, child, "chip_count", 1);
	dev_drv->dim_min  = json_get_obj_u32(jsp, child, "dim_min", 0);
	dev_drv->dim_max  = json_get_obj_u32(jsp, child, "dim_max", 4095);

//interface
	child = json_get_object_child(jsp, parent, "interface");
	if (!child) {
		LDIMERR("fail to get interface\n");
		return -1;
	}

	str = json_get_obj_str(jsp, child, "type", NULL);
	dev_drv->type = strnum_get_num(str, ldim_dev_type_match,
				       ARRAY_SIZE(ldim_dev_type_match), LDIM_DEV_TYPE_MAX);
	if (dev_drv->type == LDIM_DEV_TYPE_MAX) {
		LDIMERR("invalid type:%d\n", dev_drv->type);
		return -1;
	}

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		spi_info = &dev_drv->spi_info;
		spi_info->bus_num[0] = json_get_obj_u32(jsp, child, "bus_number", 2);
		spi_info->bus_num[1] = json_get_obj_u32(jsp, child, "bus_number1", 0);
		spi_info->chip_select[0] = json_get_obj_u32(jsp, child, "chip_select", 0);
		spi_info->chip_select[1] = json_get_obj_u32(jsp, child, "chip_select1", 0);
		spi_info->max_speed_hz = json_get_obj_u32(jsp, child, "max_frequency_hz", 3000000);
		spi_info->mode = json_get_obj_u32(jsp, child, "spi_mode", 0);
		dev_drv->cs_hold_delay = json_get_obj_u32(jsp, child, "cs_hold_delay_ms", 0);
		dev_drv->cs_clk_delay = json_get_obj_u32(jsp, child, "cs_clk_delay_ms", 0);
		if (spi_info->bus_num[1])
			spi_info->spi_dev_num = 2;
		else
			spi_info->spi_dev_num = 1;

		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("spi_dev_num:%d, spi bus: %d:%d, cs:%d:%d, max_freq:%d, mode: %d\n"
			       "cs_hold_dly:%dms, cs_clk_dly:%dms\n",
			       spi_info->spi_dev_num, spi_info->bus_num[0], spi_info->bus_num[1],
				   spi_info->chip_select[0],  spi_info->chip_select[1],
				   spi_info->max_speed_hz,
			       spi_info->mode, dev_drv->cs_hold_delay, dev_drv->cs_clk_delay);
		}
		break;
	default:
		break;
	}

//pwms
	child = json_get_object_child(jsp, parent, "pwms");
	if (child) {
		cnt = json_get_array_size(jsp, child);
		cnt = lcd_s32_constraint(cnt, 0, 2);
		pwms[0] = &dev_drv->ldim_pwm_config;
		pwms[1] = &dev_drv->analog_pwm_config;
		for (i = 0; i < cnt; i++) {
			child2 = json_get_array_child(jsp, child, i);
			if (!child2) {
				BLPR("fail find pwm[%d]\n", i);
				break;
			}

			bl_pwm = pwms[i];
			bl_pwm->drv_index = 0;
			str = json_get_obj_str(jsp, child2, "port", NULL);
			bl_pwm->pwm_port = bl_pwm_str_to_num(str ? str : "Invalid");
			if (bl_pwm->pwm_port >= BL_PWM_MAX ||
			    (i == 1 && bl_pwm->pwm_port >= BL_PWM_VS))
				continue;

			bl_pwm->pwm_method = json_get_obj_u32(jsp, child2, "polarity", 1);
			bl_pwm->pwm_phase  = json_get_obj_u32(jsp, child2, "phase", 0);
			bl_pwm->pwm_freq   = json_get_obj_u32(jsp, child2, "freq", 300);
			if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
				bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;

			child3 = json_get_object_child(jsp, child2, "duty_range");
			if (child3) {
				bl_pwm->pwm_duty_min = json_get_arr_u32(jsp, child3, 0, 0);
				bl_pwm->pwm_duty_max = json_get_arr_u32(jsp, child3, 1, 4095);
			}
			bl_pwm->pwm_duty = json_get_obj_u32(jsp, child2, "duty",
							    bl_pwm->pwm_duty_min);

			bl_pwm_config_init(bl_pwm);

			LDIMPR("get pwm[%d] pol = %d, freq = %d, phase = %d, duty:%d(%d ~ %d)\n",
				i, bl_pwm->pwm_method, bl_pwm->pwm_freq, bl_pwm->pwm_phase,
				bl_pwm->pwm_duty, bl_pwm->pwm_duty_min, bl_pwm->pwm_duty_max);
		}
	}

//ctrl
	child = json_get_object_child(jsp, parent, "ctrl");
	if (child) {
		str = json_get_obj_str(jsp, child, "pinmux_name", NULL);
		strncpy(dev_drv->pinmux_name, str ? str : "invalid", (LDIM_DEV_NAME_MAX - 1));

		str = json_get_obj_str(jsp, child, "err_gpio", NULL);
		dev_drv->lamp_err_gpio = ldim_gpio_name_to_index(dev_drv, (char *)str);
		str = json_get_obj_str(jsp, child, "en_gpio", NULL);
		dev_drv->en_gpio = ldim_gpio_name_to_index(dev_drv, (char *)str);
		dev_drv->en_gpio_on = json_get_obj_u32(jsp, child, "en_gpio_on", 1);
		dev_drv->en_gpio_off = json_get_obj_u32(jsp, child, "en_gpio_off", 0);

		if (dev_drv->lamp_err_gpio < BL_GPIO_NUM_MAX)
			dev_drv->fault_check = 1;

		dev_drv->write_check = json_get_obj_u32(jsp, child, "write_check", 0);
	}

//packet_info
	child = json_get_object_child(jsp, parent, "packet_info");
	if (child) {
		dev_drv->mcu_header = json_get_obj_u32(jsp, child, "header", 0x0);
		dev_drv->mcu_dim = json_get_obj_u32(jsp, child, "mcu_dim", 0x0);
	}

//boost

//profile & zone mapping
	str = json_get_obj_str(jsp, parent, "zone_mapping_path", NULL);
	if (str) {
		LDIMPR("find custom ldim_zone_mapping_path:%s\n", str);
		ret = ldim_dev_zone_mapping_load(dev_drv, str);
		if (ret) {
			for (i = 0; i < dev_drv->zone_num; i++)
				dev_drv->bl_mapping[i] = (unsigned short)i;
		}
	} else {
		for (i = 0; i < dev_drv->zone_num; i++)
			dev_drv->bl_mapping[i] = (unsigned short)i;
	}

//custom_params

//commands
	child = json_get_object_child(jsp, parent, "commands");
	if (child) {
		dev_drv->cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC;

		str = json_get_obj_str(jsp, child, "init_on", NULL);
		if (str) {
			cnt_max = lcd_get_str_array_cnt(str);
			nums_size = cnt_max * sizeof(unsigned int);
			nums = malloc(nums_size);
			if (!nums) {
				LDIMERR("init_on: no memory to save nums\n");
				goto parse_ldim_init_off;
			}

			memset(nums, 0, nums_size);
			cnt = lcd_trans_str_array(str, nums, cnt_max);
			table = lcd_init_table_load_array("ldim_dev_init_on",
							  LCD_EXT_CMD_SIZE_DYNAMIC,
							  nums, cnt_max, LDIM_INIT_ON_MAX,
							  &data_cnt);
			if (!table) {
				free(nums);
				goto ldim_dev_get_config_from_json_end;
			}
			dev_drv->init_on = table;
			dev_drv->init_on_cnt = data_cnt;
			memset(nums, 0, nums_size);
			free(nums);
		}
parse_ldim_init_off:
		str = json_get_obj_str(jsp, child, "init_off", NULL);
		if (str) {
			cnt_max = lcd_get_str_array_cnt(str);
			nums_size = cnt_max * sizeof(unsigned int);
			nums = malloc(nums_size);
			if (!nums) {
				LDIMERR("init_off: no memory to save nums\n");
				goto ldim_dev_get_config_from_json_end;
			}

			memset(nums, 0, nums_size);
			cnt = lcd_trans_str_array(str, nums, cnt_max);
			table = lcd_init_table_load_array("ldim_dev_init_off",
							  LCD_EXT_CMD_SIZE_DYNAMIC,
							  nums, cnt_max, LDIM_INIT_OFF_MAX,
							  &data_cnt);
			if (!table) {
				free(nums);
				goto ldim_dev_get_config_from_json_end;
			}
			dev_drv->init_off = table;
			dev_drv->init_off_cnt = data_cnt;
			free(nums);
		}
		dev_drv->init_loaded = 1;
	}

ldim_dev_get_config_from_json_end:
	return 0;
}

#else
int ldim_dev_get_config_from_json(struct ldim_dev_driver_s *dev_drv)
{
	return -1;
}
#endif

#if defined(CONFIG_CMD_AML_MODEL)
static inline int ldim_dev_type_str2num(const char *str)
{
	const char *start;

	start = strchr(str, 'V');
	if (start)
		start += 2;
	else
		start = str;

	return strnum_get_num(start, ldim_dev_type_match, ARRAY_SIZE(ldim_dev_type_match),
			      LDIM_DEV_TYPE_MAX);
}

static int ldim_dev_pwm_port_str2num(const char *str)
{
	char *start;

	start = strchr(str, 'P');
	if (!start)
		return BL_PWM_MAX;

	return bl_pwm_str_to_num(start);
}

int ldim_dev_get_config_from_ini(struct ldim_dev_driver_s *dev_drv)
{
	void *inip, *psec;
	const char *str = NULL;
	struct bl_pwm_config_s *bl_pwm;
	unsigned int val, *init_buf;
	unsigned char *table;
	int init_len, data_cnt, buf_size;
	char dbg_str[160];
	int i, dbg_str_len = 0, ret = 0;

	inip = get_lcd_ini_parse_mem(0);
	if (!inip)
		return -1;

	psec = lcd_ini_get_section(inip, "Ldim_dev_Attr");
	if (!psec) {
		LDIMERR("%s: not find Ldim_dev_Attr\n", __func__);
		return -1;
	}

	dev_drv->index = 0;
	str = lcd_ini_get_str(inip, psec, "dev_name", "null");
	strlcpy(dev_drv->name, str, LDIM_DEV_NAME_MAX);

	str = lcd_ini_get_str(inip, psec, "if_type", "null");
	dev_drv->type = ldim_dev_type_str2num(str);
	if (dev_drv->type == LDIM_DEV_TYPE_MAX)
		return -1;

	switch (dev_drv->type) {
	case LDIM_DEV_TYPE_SPI:
		dev_drv->spi_info.max_speed_hz = lcd_ini_get_val(inip, psec, "if_freq", 0);
		val = lcd_ini_get_val(inip, psec, "if_attr_0", 0);
		dev_drv->spi_info.bus_num[0] = val & 0xf;
		dev_drv->spi_info.bus_num[1] = (val >> 4) & 0xf;
		if (dev_drv->spi_info.bus_num[1])
			dev_drv->spi_info.spi_dev_num = 2;

		val = lcd_ini_get_val(inip, psec, "if_attr_1", 0);
		dev_drv->spi_info.chip_select[0] = val & 0xf;
		dev_drv->spi_info.chip_select[1] = (val >> 4) & 0xf;

		dev_drv->spi_info.mode = lcd_ini_get_val(inip, psec, "if_attr_2", 0);
		dev_drv->cs_hold_delay = lcd_ini_get_val(inip, psec, "if_attr_4", 0);
		dev_drv->cs_clk_delay = lcd_ini_get_val(inip, psec, "if_attr_5", 0);
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("spi bus_num: %d:%d, chip_select: %d:%d, max_speed_hz: %d\n",
				dev_drv->spi_info.bus_num[0], dev_drv->spi_info.bus_num[1],
				dev_drv->spi_info.chip_select[0], dev_drv->spi_info.chip_select[1],
				dev_drv->spi_info.max_speed_hz);
			LDIMPR("spi_dev_num:%d, spi mode:%d, cs_hold_delay=%d, cs_clk_delay=%d\n",
			       dev_drv->spi_info.spi_dev_num, dev_drv->spi_info.mode,
				   dev_drv->cs_hold_delay, dev_drv->cs_clk_delay);
		}
		break;
	default:
		break;
	}

	/* pwm (48Byte) */
	bl_pwm = &dev_drv->ldim_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	str = lcd_ini_get_str(inip, psec, "pwm_vs_port", "null");
	bl_pwm->pwm_port = ldim_dev_pwm_port_str2num(str);
	if (bl_pwm->pwm_port < BL_PWM_MAX) {
		str = lcd_ini_get_str(inip, psec, "pwm_vs_pol", "null");
		bl_pwm->pwm_method = bl_str_to_pwm_method(str, BL_PWM_POSITIVE);
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			val = lcd_ini_get_val(inip, psec, "pwm_vs_freq", 0);
			bl_pwm->pwm_freq = (val & 0xff);
			bl_pwm->pwm_phase = (val >> 8) & 0xffffff;

		} else {
			bl_pwm->pwm_freq = lcd_ini_get_val(inip, psec, "pwm_vs_freq", 0);
			bl_pwm->pwm_phase = 0;
		}
		bl_pwm->pwm_duty = lcd_ini_get_val(inip, psec, "pwm_vs_duty", 0);

		if (bl_pwm->pwm_port == BL_PWM_VS) {
			if (bl_pwm->pwm_freq > 4) {
				LDIMERR("pwm_vs wrong freq %d\n", bl_pwm->pwm_freq);
				bl_pwm->pwm_freq = BL_FREQ_VS_DEFAULT;
			}
		} else {
			if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
				bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		}
		LDIMPR("get ldim_pwm pol = %d, freq = %d, dft duty = %d%%, phase=%d\n",
		       bl_pwm->pwm_method, bl_pwm->pwm_freq, bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	bl_pwm = &dev_drv->analog_pwm_config;
	bl_pwm->drv_index = 0; /* only venc0 support ldim */
	str = lcd_ini_get_str(inip, psec, "pwm_adj_port", "null");
	bl_pwm->pwm_port = bl_pwm_str_to_num(str);
	if (bl_pwm->pwm_port < BL_PWM_VS) {
		str = lcd_ini_get_str(inip, psec, "pwm_adj_pol", "BL_PWM_POSITIVE");
		bl_pwm->pwm_method = bl_str_to_pwm_method(str, BL_PWM_POSITIVE);
		if (bl_pwm->pwm_port == BL_PWM_VS) {
			val = lcd_ini_get_val(inip, psec, "pwm_adj_freq", 0);
			bl_pwm->pwm_freq = (val & 0xff);
			bl_pwm->pwm_phase = (val >> 8) & 0xffffff;
		} else {
			bl_pwm->pwm_freq = lcd_ini_get_val(inip, psec, "pwm_adj_freq", 0);
			bl_pwm->pwm_phase = 0;
		}
		bl_pwm->pwm_duty = lcd_ini_get_val(inip, psec, "pwm_adj_duty", 0);
		bl_pwm->pwm_duty_max = lcd_ini_get_val(inip, psec, "pwm_adj_attr_0", 0);
		bl_pwm->pwm_duty_min = lcd_ini_get_val(inip, psec, "pwm_adj_attr_1", 0);

		if (bl_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
			bl_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		LDIMPR("get analog_pwm pol = %d, freq = %d\n",
			bl_pwm->pwm_method, bl_pwm->pwm_freq);
		LDIMPR("duty max = %d%%, min = %d%%, default = %d%%, phase=%d\n",
			bl_pwm->pwm_duty_max,
			bl_pwm->pwm_duty_min, bl_pwm->pwm_duty, bl_pwm->pwm_phase);
	}

	str = lcd_ini_get_str(inip, psec, "pinmux_sel", "invalid");
	strlcpy(dev_drv->pinmux_name, str, LDIM_DEV_NAME_MAX);

	/* ctrl (271Byte) */
	dev_drv->en_gpio = lcd_ini_get_val(inip, psec, "en_gpio", LCD_GPIO_MAX);
	dev_drv->en_gpio_on = lcd_ini_get_val(inip, psec, "en_gpio_on", 0);
	dev_drv->en_gpio_off = lcd_ini_get_val(inip, psec, "en_gpio_off", 0);

	dev_drv->lamp_err_gpio = lcd_ini_get_val(inip, psec, "err_gpio", LCD_GPIO_MAX);
	if (dev_drv->lamp_err_gpio >= BL_GPIO_NUM_MAX) {
		dev_drv->fault_check = 0;
	} else {
		dev_drv->fault_check = 1;
		ldim_gpio_set(dev_drv, dev_drv->lamp_err_gpio, LCD_GPIO_INPUT);
	}

	dev_drv->write_check = lcd_ini_get_val(inip, psec, "write_check", 0);

	dev_drv->dim_max = lcd_ini_get_val(inip, psec, "dim_max", 0);
	dev_drv->dim_min = lcd_ini_get_val(inip, psec, "dim_min", 0);

	dev_drv->mcu_header = lcd_ini_get_val(inip, psec, "custome_attr_0", 0);
	dev_drv->mcu_dim = lcd_ini_get_val(inip, psec, "custome_attr_1", 0);

	dev_drv->chip_cnt = lcd_ini_get_val(inip, psec, "chip_count", 0);

	str = lcd_ini_get_str(inip, psec, "zone_mapping_path", NULL);
	if (!str) {
		for (i = 0; i < dev_drv->zone_num; i++)
			dev_drv->bl_mapping[i] = (unsigned short)i;
	} else {
		LDIMPR("find custom zone_mapping: %s\n", str);
		ret = ldim_dev_zone_mapping_load(dev_drv, str);
		if (ret) {
			for (i = 0; i < dev_drv->zone_num; i++)
				dev_drv->bl_mapping[i] = (unsigned short)i;
		}
	}

	dbg_str_len += sprintf(dbg_str + dbg_str_len, "mcu_header=0x%08x, mcu_dim=0x%08x, ",
		dev_drv->mcu_header, dev_drv->mcu_dim);
	sprintf(dbg_str + dbg_str_len, "chip_cnt:%d, cus pwm_pinmux_sel:%s",
		dev_drv->chip_cnt, dev_drv->pinmux_name);
	LDIMPR("load ukey config: %s: type:%d, %s\n", dev_drv->name, dev_drv->type, dbg_str);

	dev_drv->cmd_size = lcd_ini_get_val(inip, psec, "cmd_size", 0);
	if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
		LDIMPR("%s: cmd_size = %d\n", dev_drv->name, dev_drv->cmd_size);
	if (dev_drv->cmd_size != LCD_EXT_CMD_SIZE_DYNAMIC)
		return -1;

	init_len = lcd_ini_get_array_cnt(inip, psec, "init_on");
	if (init_len < 0) {
		LDIMPR("%s: not find init_on\n", dev_drv->name);
		return 0;
	}
	buf_size = init_len * sizeof(unsigned int);
	init_buf = malloc(buf_size);
	if (!init_buf) {
		LDIMERR("malloc init_on buf failed\n");
		return -1;
	}
	memset(init_buf, 0, buf_size);
	data_cnt = lcd_ini_get_array(inip, psec, "init_on", init_buf, init_len);
	table = lcd_init_table_load_array("ldim_dev_init_on", LCD_EXT_CMD_SIZE_DYNAMIC,
					  init_buf, data_cnt, LDIM_INIT_ON_MAX, &init_len);
	if (!table)
		goto ldim_dev_get_config_from_ini_err;
	dev_drv->init_on = table;
	dev_drv->init_on_cnt = init_len;
	memset(init_buf, 0, buf_size);
	free(init_buf);

	init_len = lcd_ini_get_array_cnt(inip, psec, "init_off");
	if (init_len < 0) {
		LDIMERR("%s: not find init_on\n", dev_drv->name);
		return -1;
	}
	buf_size = init_len * sizeof(unsigned int);
	init_buf = malloc(buf_size);
	if (!init_buf) {
		LDIMERR("malloc init_off buf failed\n");
		return -1;
	}
	memset(init_buf, 0, buf_size);
	data_cnt = lcd_ini_get_array(inip, psec, "init_off", init_buf, init_len);
	table = lcd_init_table_load_array("ldim_dev_init_off", LCD_EXT_CMD_SIZE_DYNAMIC,
					  init_buf, data_cnt, LDIM_INIT_OFF_MAX, &init_len);
	if (!table)
		goto ldim_dev_get_config_from_ini_err;
	dev_drv->init_off = table;
	dev_drv->init_off_cnt = init_len;
	memset(init_buf, 0, buf_size);
	free(init_buf);

	dev_drv->init_loaded = 1;

	return 0;

ldim_dev_get_config_from_ini_err:
	memset(init_buf, 0, buf_size);
	free(init_buf);
	return -1;
}

#else
int ldim_dev_get_config_from_ini(struct ldim_dev_driver_s *dev_drv)
{
	return -1;
}
#endif

static int ldim_dt_valid(char *dt_addr)
{
#ifdef CONFIG_OF_LIBFDT

	int parent_offset;
	char *propdata;

	parent_offset = fdt_path_offset(dt_addr, "/local_dimming_device");
	if (parent_offset < 0) {
		parent_offset = fdt_path_offset(dt_addr, "/local_diming_device");
		if (parent_offset < 0) {
			LDIMERR("not find /local_dimming_device node: %s\n",
				fdt_strerror(parent_offset));
			return -1;
		}
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (propdata && strncmp(propdata, "okay", 2) == 0)
		return 1;

	LDIMPR("local_dimming_device status disabled\n");
	return 0;
#else
	return 0;
#endif
}

static int ldim_check_config_load(struct ldim_dev_driver_s *dev_drv)
{
	int ret = 0, dt_sta;

	dt_sta = ldim_dt_valid(lcd_get_dt_addr());
	dev_drv->config_load =
		lcd_panel_config_load_detect(0, dt_sta, dev_drv->key_valid, __func__);
	if (dev_drv->config_load == LCD_CONFIG_NONE || dev_drv->config_load == LCD_CONFIG_ERR)
		return -1;

	return ret;
}

int ldim_dev_get_config(char *dt_addr, struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;
	int parent_offset;
	char *propdata;
	char *p;
	const char *str;
	unsigned int val;
	int i, j, ret = 0;
	unsigned char file_type = PANEL_FILE_INVILD;

	if (!dt_addr) {
		LDIMERR("%s: dt_addr is NULL\n", __func__);
		return -1;
	}
	if (!dev_drv) {
		LDIMERR("%s: dev_drv is NULL\n", __func__);
		return -1;
	}

	parent_offset = fdt_path_offset(dt_addr, "/local_dimming_device");
	if (parent_offset < 0) {
		parent_offset = fdt_path_offset(dt_addr, "/local_diming_device");
		if (parent_offset < 0) {
			LDIMERR("not find /local_dimming_device node: %s\n",
				fdt_strerror(parent_offset));
			return -1;
		}
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (!propdata) {
		LDIMERR("not find local_dimming_device status, default to disabled\n");
		return -1;
	}
	if (strncmp(propdata, "okay", 2)) {
		LDIMPR("local_dimming_device status disabled\n");
		return -1;
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "key_valid", NULL);
	if (!propdata) {
		LDIMERR("failed to get key_valid\n");
		val = 0;
	} else {
		val = be32_to_cpup((u32 *)propdata);
	}
	dev_drv->key_valid = val;
	//LDIMPR("key_valid: %d\n", dev_drv->key_valid);

	/* init gpio */
	i = 0;
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "ldim_dev_gpio_names", NULL);
	if (!propdata) {
		LDIMERR("failed to get ldim_dev_gpio_names\n");
	} else {
		p = propdata;
		while (i < BL_GPIO_NUM_MAX) {
			if (i > 0)
				p += strlen(p) + 1;
			str = p;
			if (strlen(str) == 0)
				break;
			strlcpy(dev_drv->gpio_name[i], str, LCD_CPU_GPIO_NAME_MAX);
			if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL)
				LDIMPR("i=%d, gpio=%s\n", i, dev_drv->gpio_name[i]);
			i++;
		}
	}
	for (j = i; j < BL_GPIO_NUM_MAX; j++)
		strcpy(dev_drv->gpio_name[j], "invalid");

	ret = ldim_check_config_load(dev_drv);
	if (ret)
		return -1;

	switch (dev_drv->config_load) {
	case LCD_CONFIG_FILE:
		file_type = get_lcd_panel_file_type(0);
		if (file_type == PANEL_FILE_JSON)
			ret = ldim_dev_get_config_from_json(dev_drv);
		else if (file_type == PANEL_FILE_INI)
			ret = ldim_dev_get_config_from_ini(dev_drv);
		break;
	case LCD_CONFIG_UKEY:
		ret = ldim_dev_get_config_from_ukey(dev_drv);
		break;
	case LCD_CONFIG_DTS:
		ret = ldim_dev_get_config_from_dts(dev_drv, dt_addr, dev_drv->index);
		break;

	default:
		ret = -1;
		break;
	}

	ret = ldim_pinmux_load(dt_addr, ldim_drv);

	return ret;
}
#endif
