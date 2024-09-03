// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/bl_ldim.h>
#include "../../lcd_common.h"
#include "ldim_drv.h"
#include "ldim_dev_drv.h"

#define BROADCAST_SAME   0x00
#define BROADCAST_DIFF   0x7F
#define IW7039_REG_MAX        0x100

struct iw7039_s {
	unsigned int dev_on_flag;

	unsigned int reg_buf_size;
	unsigned int tbuf_size;
	unsigned int rbuf_size;

	unsigned short *reg_buf; /* local dimming driver smr api usage */

	/* spi api internal used, don't use outside!!! */
	unsigned char *tbuf;
	unsigned char *rbuf;
};

static struct iw7039_s *bl_iw7039;

unsigned short init_00[32] = {
	0x0802, 0x03a2, 0x0000, 0x0ffe, 0x3ffb, 0x43a7, 0x4585, 0x8190,
	0x0100, 0xa140, 0x0000, 0x0834, 0x0044, 0x5840, 0x3737, 0xcc82,
	0x3fff, 0xffff, 0x01e1, 0x01e1, 0x0118, 0xc000, 0x0000, 0x0000,
	0xffff, 0xffff, 0x0000, 0x01ff, 0x4fff, 0x1fff, 0x3fff, 0xffff
};

unsigned short init_buf[32];

static int spi_device_number(struct spi_slave *spi, unsigned short chip_cnt)
{
	unsigned char *tbuf;
	int ret;
	unsigned short chip_id = 0x7E;

	if (!bl_iw7039 || !bl_iw7039->tbuf) {
		LDIMERR("%s: bl_iw7039 or tbuf is null\n", __func__);
		return -1;
	}
	tbuf = bl_iw7039->tbuf;

	tbuf[0] = 0x80 | chip_id;
	tbuf[1] = chip_cnt & 0x7f;
	tbuf[2] = 0;
	tbuf[3] = 0;
	tbuf[4] = 0;
	tbuf[5] = 0;

	ret = ldim_spi_write(spi, tbuf, 6);

	return ret;
}

/* write same data to all device */
static int spi_wregs_all(struct spi_slave *spi, unsigned int chip_cnt,
			 unsigned short reg, unsigned short *data_buf, int tlen)
{
	unsigned char *tbuf;
	int n, xlen, ret, i, j;

	if (!bl_iw7039 || !bl_iw7039->tbuf) {
		LDIMERR("%s: bl_iw7039 or tbuf is null\n", __func__);
		return -1;
	}
	tbuf = bl_iw7039->tbuf;

	if (tlen == 0) {
		LDIMERR("%s: tlen is 0\n", __func__);
		return -1;
	}
	n = 1 + (chip_cnt - 1) / 16;/*dammy count*/
	if (tlen == 1)
		xlen = 2 + 1 + n;
	else
		xlen = 2 + tlen + n;
	xlen = 2 * xlen;//16-->8bit
	if (bl_iw7039->tbuf_size < xlen) {
		LDIMERR("%s: tbuf_size %d is not enough\n", __func__, bl_iw7039->tbuf_size);
		return -1;
	}

	if (tlen == 1) {
		tbuf[0] = 0x80 | BROADCAST_SAME;
		tbuf[1] = 0x01;
		tbuf[2] = reg >> 8;
		tbuf[3] = reg & 0xff;
		tbuf[4] = data_buf[0] >> 8;
		tbuf[5] = data_buf[0] & 0xff;
		memset(&tbuf[6], 0, 2 * n);
	} else {
		tbuf[0] = 0x80 | BROADCAST_SAME;
		tbuf[1] = tlen;
		tbuf[2] = reg >> 8;
		tbuf[3] = reg & 0xff;
		j = 4;
		for (i = 0; i < tlen; i++) {
			tbuf[j++] = data_buf[i] >> 8;
			tbuf[j++] = data_buf[i] & 0xff;
		}
		memset(&tbuf[j], 0, 2 * n);
	}
	ret = ldim_spi_write(spi, tbuf, xlen);

	return ret;
}

/* write diff data to all device */
static int spi_wregs_duty(struct spi_slave *spi, unsigned int chip_cnt,
			  unsigned short reg, unsigned short *data_buf, int tlen)
{
	unsigned char *tbuf;
	int i, j, k, p, n, xlen, ret;

	if (!bl_iw7039 || !bl_iw7039->tbuf) {
		LDIMERR("%s: bl_iw7039 or tbuf is null\n", __func__);
		return -1;
	}
	tbuf = bl_iw7039->tbuf;

	if (tlen == 0) {
		LDIMERR("%s: tlen is 0\n", __func__);
		return -1;
	}
	n = 1 + (chip_cnt - 1) / 16;
	if (tlen == 1)
		xlen = 2 + chip_cnt + n;
	else
		xlen = 2 + tlen * chip_cnt + n;
	xlen = xlen * 2;
	if (bl_iw7039->tbuf_size < xlen) {
		LDIMERR("%s: tbuf_size %d is not enough\n", __func__, bl_iw7039->tbuf_size);
		return -1;
	}
	if (bl_iw7039->rbuf_size < xlen) {
		LDIMERR("%s: rbuf_size %d is not enough\n", __func__, bl_iw7039->rbuf_size);
		return -1;
	}

	if (tlen == 1) {
		tbuf[0] = 0x80 | BROADCAST_DIFF;
		tbuf[1] = 0x01;
		tbuf[2] = reg >> 8;
		tbuf[3] = reg & 0xff;
		j = 4;
		for (i = 0; i < chip_cnt; i++) {
			tbuf[j++] = data_buf[0] >> 8;
			tbuf[j++] = data_buf[0] & 0xff;
		}
		memset(&tbuf[j], 0, 2 * n);
	} else {
		tbuf[0] = 0x80 | BROADCAST_DIFF;
		tbuf[1] = tlen;
		tbuf[2] = reg >> 8;
		tbuf[3] = reg & 0xff;
		j = 4;
		p = 0;
		for (i = 0; i < chip_cnt; i++) {
			for (k = 0; k < tlen; k++) {
				tbuf[j++] = data_buf[p] >> 8;
				tbuf[j++] = data_buf[p] & 0xff;
				p++;
			}
		}
		memset(&tbuf[j], 0, 2 * n);
	}

	ret = ldim_spi_write(spi, tbuf,	xlen);

	return ret;
}

static int iw7039_reg_write_all(struct ldim_dev_driver_s *dev_drv,
unsigned short reg, unsigned short *buf, unsigned int len)
{
	int ret;

	ret = spi_wregs_all(dev_drv->spi_info.spi, dev_drv->chip_cnt, reg, buf, len);
	if (ret)
		LDIMERR("%s: reg 0x%x, len %d error\n", __func__, reg, len);

	return ret;
}

static int iw7039_reg_write_duty(struct ldim_dev_driver_s *dev_drv,
unsigned short reg, unsigned short *buf, unsigned int len)
{
	int ret;

	ret = spi_wregs_duty(dev_drv->spi_info.spi, dev_drv->chip_cnt, reg, buf, len);
	if (ret)
		LDIMERR("%s: reg 0x%x, len %d error\n", __func__, reg, len);

	return ret;
}

static int ldim_power_cmd_dynamic_size(struct ldim_dev_driver_s *dev_drv)
{
	unsigned char *table;
	int i = 0, j, step = 0, max_len = 0;
	unsigned char type, cmd_size;
	int delay_ms, ret = 0;
	unsigned short val = 0;

	table = dev_drv->init_on;
	max_len = dev_drv->init_on_cnt;

	while ((i + 1) < max_len) {
		type = table[i];
		if (type == LCD_EXT_CMD_TYPE_END)
			break;
		if (lcd_debug_print_flag & LCD_DBG_PR_BL_NORMAL) {
			LDIMPR("%s: step %d: type=0x%02x, cmd_size=%d\n",
			       __func__, step, type, table[i + 1]);
		}
		cmd_size = table[i + 1];
		if (cmd_size == 0)
			goto power_cmd_dynamic_next;
		if ((i + 2 + cmd_size) > max_len)
			break;

		if (type == LCD_EXT_CMD_TYPE_NONE) {
			/* do nothing */
		} else if (type == LCD_EXT_CMD_TYPE_DELAY) {
			delay_ms = 0;
			for (j = 0; j < cmd_size; j++)
				delay_ms += table[i + 2 + j];
			if (delay_ms > 0)
				mdelay(delay_ms);
		} else if (type == LCD_EXT_CMD_TYPE_CMD) {
			val = (table[i + 3] << 8) | table[i + 4];
			ret = iw7039_reg_write_all(dev_drv, table[i + 2], &val, 1);
			udelay(1);
		} else {
			LDIMERR("%s: type 0x%02x invalid\n", __func__, type);
		}
power_cmd_dynamic_next:
		i += (cmd_size + 2);
		step++;
	}

	return ret;
}

static int iw7039_power_on_init(struct ldim_dev_driver_s *dev_drv)
{
	int ret = 0;

	if (dev_drv->cmd_size < 2) {
		LDIMERR("%s: invalid cmd_size %d\n", __func__, dev_drv->cmd_size);
		return -1;
	}
	if (!dev_drv->init_on) {
		LDIMERR("%s: init_on is null\n", __func__);
		return -1;
	}

	if (dev_drv->cmd_size == LCD_EXT_CMD_SIZE_DYNAMIC)
		ret = ldim_power_cmd_dynamic_size(dev_drv);

	return ret;
}

static int iw7039_hw_init_on(struct ldim_dev_driver_s *dev_drv)
{
	unsigned short temp[2];
	int i;

	LDIMPR("start init on iw7039 hw\n");

	/* step 1: system power_on */
	ldim_gpio_set(dev_drv, dev_drv->en_gpio, dev_drv->en_gpio_on);

	/* step 2: delay for internal logic stable */
	mdelay(10);

	spi_device_number(dev_drv->spi_info.spi, dev_drv->chip_cnt);

	iw7039_reg_write_all(dev_drv, 0x00, init_00, 0x20);

	temp[0] = 0xa5ff;
	iw7039_reg_write_all(dev_drv, 0xa0, temp, 1);

	for (i = 0; i < 32; i++)
		init_buf[i] = 0x0174;
	iw7039_reg_write_all(dev_drv, 0x20, init_buf, 0x20);

	for (i = 0; i < 32; i++)
		init_buf[i] = 0xfff;
	iw7039_reg_write_all(dev_drv, 0x60, init_buf, 0x20);

	temp[0] = 0xa533;
	iw7039_reg_write_all(dev_drv, 0xa0, temp, 1);

	mdelay(50);

	temp[0] = 0x0803;
	iw7039_reg_write_all(dev_drv, 0x00, temp, 1);

	mdelay(500);

	if (i == 0xffff)
		iw7039_power_on_init(dev_drv);
	/* step 15: calibration done */
	LDIMPR("%s: calibration done\n", __func__);

	return 0;
}

static int iw7039_hw_init_off(struct ldim_dev_driver_s *dev_drv)
{
	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	ldim_gpio_set(dev_drv, dev_drv->en_gpio, dev_drv->en_gpio_off);
	dev_drv->pinmux_ctrl(dev_drv, 0);
	ldim_pwm_off(&dev_drv->ldim_pwm_config);
	ldim_pwm_off(&dev_drv->analog_pwm_config);

	return 0;
}

static inline void ldim_data_mapping(unsigned short *duty_buf, unsigned int max, unsigned int min,
				     unsigned int zone_num, unsigned short *mapping)
{
	unsigned int i, j, val, zone_max;

	zone_max = bl_iw7039->reg_buf_size;
	for (i = 0; i < zone_num; i++) {
		val = min + ((duty_buf[i] * (max - min)) / LD_DATA_MAX);
		j = mapping[i];
		if (j >= zone_max) {
			LDIMPR("%s: mapping[%d]=%d invalid, max %d\n", __func__, i, j, zone_max);
			return;
		}
		bl_iw7039->reg_buf[j] = val;
	}
}

static int iw7039_smr(struct aml_ldim_driver_s *ldim_drv, unsigned short *buf, unsigned int len)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}
	if (!bl_iw7039)
		return -1;

	if (bl_iw7039->dev_on_flag == 0) {
		LDIMPR("%s: on_flag=%d\n", __func__, bl_iw7039->dev_on_flag);
		return 0;
	}
	if (len != dev_drv->zone_num) {
		LDIMERR("%s: data len %d invalid\n", __func__, len);
		return -1;
	}
	if (!bl_iw7039->reg_buf) {
		LDIMERR("%s: reg_buf is null\n", __func__);
		return -1;
	}

	ldim_data_mapping(buf, dev_drv->dim_max, dev_drv->dim_min,
			  dev_drv->zone_num, dev_drv->bl_mapping);

	iw7039_reg_write_duty(dev_drv, 0x60, bl_iw7039->reg_buf, 0x20);

	return 0;
}

static int iw7039_power_on(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_iw7039)
		return -1;

	if (bl_iw7039->dev_on_flag) {
		LDIMPR("%s: iw7039 is already on, exit\n", __func__);
		return 0;
	}

	iw7039_hw_init_on(ldim_drv->dev_drv);
	bl_iw7039->dev_on_flag = 1;

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int iw7039_power_off(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_iw7039)
		return -1;

	bl_iw7039->dev_on_flag = 0;
	iw7039_hw_init_off(ldim_drv->dev_drv);

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int iw7039_ldim_driver_update(struct ldim_dev_driver_s *dev_drv)
{
	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	dev_drv->power_on = iw7039_power_on;
	dev_drv->power_off = iw7039_power_off;
	dev_drv->dev_smr = iw7039_smr;

	dev_drv->reg_write = NULL;
	dev_drv->reg_read = NULL;
	return 0;
}

int ldim_dev_iw7039_probe(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;
	unsigned int size;

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}
	if (!dev_drv->spi_info.spi) {
		LDIMERR("%s: spi is null\n", __func__);
		return -1;
	}

	bl_iw7039 = (struct iw7039_s *)malloc(sizeof(struct iw7039_s));
	if (!bl_iw7039) {
		LDIMERR("iw7039 malloc error\n");
		return -1;
	}
	memset(bl_iw7039, 0, sizeof(struct iw7039_s));

	bl_iw7039->dev_on_flag = 0;

	/* 16 each device, each zone 2 bytes */
	bl_iw7039->reg_buf_size = 32 * dev_drv->chip_cnt;
	bl_iw7039->reg_buf = (unsigned short *)malloc(bl_iw7039->reg_buf_size);
	if (!bl_iw7039->reg_buf)
		goto ldim_dev_iw7039_probe_err0;
	memset(bl_iw7039->reg_buf, 0, bl_iw7039->reg_buf_size);

	/* header + reg_max_cnt + chip_cnt */
	bl_iw7039->tbuf_size = IW7039_REG_MAX + bl_iw7039->reg_buf_size * 2;
	size = bl_iw7039->tbuf_size * sizeof(unsigned char);
	bl_iw7039->tbuf = (unsigned char *)malloc(size);
	if (!bl_iw7039->tbuf)
		goto ldim_dev_iw7039_probe_err1;
	memset(bl_iw7039->tbuf, 0, size);

	/* header + reg_max_cnt + chip_cnt + dev_id_max(=chip_cnt) */
	bl_iw7039->rbuf_size = bl_iw7039->tbuf_size;
	size = bl_iw7039->rbuf_size * sizeof(unsigned char);
	bl_iw7039->rbuf = (unsigned char *)malloc(size);
	if (!bl_iw7039->rbuf)
		goto ldim_dev_iw7039_probe_err2;
	memset(bl_iw7039->rbuf, 0, size);

	iw7039_ldim_driver_update(dev_drv);

	LDIMPR("%s ok\n", __func__);
	return 0;

ldim_dev_iw7039_probe_err2:
	free(bl_iw7039->tbuf);
	bl_iw7039->tbuf_size = 0;
ldim_dev_iw7039_probe_err1:
	free(bl_iw7039->reg_buf);
	bl_iw7039->reg_buf_size = 0;
ldim_dev_iw7039_probe_err0:
	free(bl_iw7039);
	bl_iw7039 = NULL;
	return -1;
}

int ldim_dev_iw7039_remove(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_iw7039)
		return 0;

	free(bl_iw7039->rbuf);
	bl_iw7039->rbuf_size = 0;
	free(bl_iw7039->tbuf);
	bl_iw7039->tbuf_size = 0;
	free(bl_iw7039->reg_buf);
	bl_iw7039->reg_buf_size = 0;
	free(bl_iw7039);
	bl_iw7039 = NULL;

	return 0;
}

