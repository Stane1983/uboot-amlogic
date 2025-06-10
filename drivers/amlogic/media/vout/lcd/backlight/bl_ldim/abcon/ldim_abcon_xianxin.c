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
#include "../../../lcd_common.h"
#include "../../../lcd_reg.h"
#include "../../lcd_bl.h"
#include "../ldim_drv.h"
#include "../ldim_dev_drv.h"
#include "env.h"
#include "ldim_abcon.h"
#include "ldim_abcon_reg.h"

enum sop_type_e {
	sop_cmd = 0,
	sop_dimming,
	sop_readback,
	sop_max,
};

struct regmap_s {
	unsigned char dim_addr;
	unsigned char sample_time;
	unsigned char duty_data;
	unsigned char data_format;
	unsigned char read_dim_addr;
	unsigned char read_reg_addr;
};

//for BX7D871
struct regmap_s regmap_conf_871 = {
	.dim_addr = 0x04,
	.sample_time = 0x01,
	.duty_data = 0x12,
	.data_format = 0x02,
	.read_dim_addr = 0x16,
	.read_reg_addr = 0x15,
};

//for BX7D852
struct regmap_s regmap_conf_852 = {
	.dim_addr = 0x01,
	.sample_time = 0x12,
	.duty_data = 0x0f,
	.data_format = 0x1d,
	.read_dim_addr = 0x0e,
	.read_reg_addr = 0x02,
};

struct abcon_xianxin_s {
	struct regmap_s *regmap;
	unsigned char dev_type;
	unsigned int dev_on_flag;
	unsigned short vsync_cnt;
	unsigned char bl_pwm_en;	/*0:default uboot have pwm, 1: uboot without pwm*/
	unsigned char init_sampletime;

	unsigned char act_lane_num;
	unsigned int fb_cnt;
	unsigned int fb_high_cnt_th;
	unsigned char sop_type;
	unsigned char dataformat;

	unsigned char samplepoint;
};

struct abcon_xianxin_s *bl_abcon_xianxin;

struct xianxin_packet_s {
	//unsigned int packet_header_len;
	//unsigned int packet_header[6];

	unsigned int cmd_header;
	unsigned int cmd_data;

	unsigned int addr_header;
	unsigned int addr_data;

	unsigned int data_header;
	unsigned int data_data;

	unsigned int eop_header;
	unsigned int eop_data;
};

struct xianxin_packet_s xianxin_packet;

void ldim_abcon_xianxin_write_packet(void)
{
	unsigned int len = 0;
	unsigned int *buf;
	unsigned char i = 0;
	unsigned char act_lane_num = abcon->act_lane;

	if (bl_abcon_xianxin->sop_type == sop_dimming) {
		buf = abcon_mem.ldc_seg;
		invalidate_dcache_range((unsigned long)buf,
			(unsigned long)(buf + LDC_SEG_SIZE));
	} else {
		buf = abcon_mem.wseg;
		invalidate_dcache_range((unsigned long)buf,
			(unsigned long)(buf + WSEG_SIZE));
	}

	if (!buf)
		return;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = xianxin_packet.cmd_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = xianxin_packet.cmd_data;

	if (bl_abcon_xianxin->sop_type == sop_dimming) {
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = xianxin_packet.addr_header;

		for (i = 0; i < act_lane_num; i++)
			buf[len++] = xianxin_packet.addr_data;

		for (i = 0; i < act_lane_num; i++)
			buf[len++] = xianxin_packet.data_header;

		//for (i = 0; i < act_lane_num; i++)
		//	buf[len++] = xianxin_packet.data_data;

		for (i = 0; i < act_lane_num; i++)
			buf[len++] = xianxin_packet.eop_header;

		for (i = 0; i < act_lane_num; i++)
			buf[len++] = xianxin_packet.eop_data;
	}

	flush_cache((unsigned long)buf, len);

	//clr done flag
	ldim_abcon_swrst();

	if (bl_abcon_xianxin->sop_type == sop_dimming) {
		abcon_wr_reg_bits(0x04, len, 16, 13);//reg_abcon_ldc_seg_max_num
		udelay(10);
		abcon_wr_reg_bits(0x00, 1, 1, 1);//reg_abcon_ldc_mod_start
		abcon_wr_reg_bits(0x00, 0, 1, 1);//reg_abcon_ldc_mod_start
		mdelay(15);
	} else {
		abcon_wr_reg_bits(0x04, len, 0, 13);//reg_abcon_sw_wseg_max_num
		udelay(10);
		abcon_wr_reg_bits(0x00, 1, 0, 1);//reg_abcon_sw_mod_start
		abcon_wr_reg_bits(0x00, 0, 0, 1);//reg_abcon_sw_mod_start
	}
}

unsigned char ldim_abcon_xianxin_readback(unsigned char reg, unsigned char dim, unsigned char lane)
{
	unsigned int readback_val = 0;
	unsigned int cmd_data = 0;
	unsigned char temp = 1;

	if (!abcon_mem.rseg || !abcon_mem.wseg) {
		ABCONERR("%s, rseg or wseg is null!", __func__);
		return -1;
	}

	bl_abcon_xianxin->sop_type = sop_readback;

	//step1, prepare read packet first
	abcon_mem.rseg[0] = 0x40000112;
	abcon_wr_reg_bits(0x05, 1, 16, 13);//reg_abcon_sw_rseg_max_num

	//step2, select read which lane
	abcon_wr_reg_bits(0x03, 1, 10, 4);//reg_abcon_act_lane_num
	switch (lane) {
	case 0:
		temp = 0x1;
		break;
	case 1:
		temp = 0x2;
		break;
	case 2:
		temp = 0x4;
		break;
	case 3:
		temp = 0x8;
		break;
	default:
		temp = 0x1;
		break;
	}
	abcon_wr_reg_bits(0x2a, temp, 22, 4);//reg_abcon_act_lane

	invalidate_dcache_range((unsigned long)abcon_mem.wseg,
		(unsigned long)(abcon_mem.wseg + 4));

	//step3, set write packet
	cmd_data = (5 << 14) | (bl_abcon_xianxin->regmap->read_reg_addr << 9) | (reg << 1);
	abcon_mem.wseg[0] = 0x44000090;
	abcon_mem.wseg[1] = cmd_data;

	cmd_data = (4 << 14) | (bl_abcon_xianxin->regmap->read_dim_addr << 9) | (dim << 1);
	abcon_mem.wseg[2] = 0x44000088;
	abcon_mem.wseg[3] = cmd_data;

	flush_cache((unsigned long)abcon_mem.wseg, 4);

	abcon_wr_reg_bits(0x04, 4, 0, 13);//reg_abcon_sw_wseg_max_num
	abcon_wr_reg_bits(0x00, 1, 0, 1);//reg_abcon_sw_mod_start
	abcon_wr_reg_bits(0x00, 0, 0, 1);//reg_abcon_sw_mod_start

	udelay(100);
	readback_val = abcon_rd_reg(0x52);

	return readback_val;
}

void ldim_abcon_write_xianxin_register(unsigned char regaddr, unsigned char value)
{
	unsigned int temp = 0;
	unsigned int cmd_data = 0;

	ABCONPR("write cmd reg:0x%x val:0x%08x, ln=%d\n", regaddr, value, abcon->act_lane);
	bl_abcon_xianxin->sop_type = sop_cmd;

	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane, 10, 4);
	switch (abcon->act_lane) {
	case 1:
		temp = 0x1;
		break;
	case 2:
		temp = 0x3;
		break;
	case 3:
		temp = 0x7;
		break;
	case 4:
		temp = 0xf;
		break;
	default:
		temp = 0x01;
		break;
	}
	//reg_abcon_act_lane
	abcon_wr_reg_bits(0x2a, temp, 22, 4);

	cmd_data = (5 << 14) | ((regaddr & 0x1f) << 9) | (value << 1);
	xianxin_packet.cmd_header = 0x44000088;
	xianxin_packet.cmd_data = cmd_data;

	ldim_abcon_xianxin_write_packet();
	udelay(10);
}

void ldim_abcon_xianxin_dimming(void)
{
	unsigned int temp = 0;

	ABCONPR("%s: set duty command, act_lane=%d\n", __func__, abcon->act_lane);
	bl_abcon_xianxin->sop_type = sop_dimming;

	//reg_abcon_pin_dir_del_start
	abcon_wr_reg_bits(0x00, 1, 8, 1);
	abcon_wr_reg_bits(0x00, 0, 8, 1);
	udelay(50);

	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane, 10, 4);
	switch (abcon->act_lane) {
	case 1:
		temp = 0x1;
		break;
	case 2:
		temp = 0x3;
		break;
	case 3:
		temp = 0x7;
		break;
	case 4:
		temp = 0xf;
		break;
	default:
		temp = 0x01;
		break;
	}
	//reg_abcon_act_lane
	abcon_wr_reg_bits(0x2a, temp, 22, 4);

	xianxin_packet.cmd_header = 0x20000080;
	xianxin_packet.cmd_data = 0xa0 | (bl_abcon_xianxin->regmap->duty_data & 0x1f);

	xianxin_packet.addr_header = 0x20000080;
	xianxin_packet.addr_data = 0x01;

	xianxin_packet.data_header = 0x1081 | (abcon->conf.ch_num << 14);
	//xianxin_packet.data_data = value;

	xianxin_packet.eop_header = 0x04000088;
	xianxin_packet.eop_data = 0x00000000;

	ldim_abcon_xianxin_write_packet();
}

void ldim_abcon_init_registers_for_xianxin(void)
{
	//reg_abcon_clr_done_flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_mul_ldc_duty_num
	abcon_wr_reg_bits(0x02, abcon->max_lane_dim, 24, 8);

	//reg_abcon_pin_dir_level
	abcon_wr_reg_bits(0x05, 1, 30, 1);
	//reg_abcon_wire_mode
	abcon_wr_reg_bits(0x22, 0, 30, 2);
	//reg_abcon_xianxin_dim_addr_inc
	abcon_wr_reg_bits(0x2a, 1, 11, 1);
	//reg_abcon_xianxin_dim_addr_init
	abcon_wr_reg_bits(0x2a, 0, 0, 11);

	//reg_abcon_dim_addr_seg_pos
	abcon_wr_reg_bits(0x2c, 1, 1, 11);
	//reg_abcon_dim_addr_auto_update
	abcon_wr_reg_bits(0x2c, 1, 0, 1);

	//reg_abcon_ldc_pkt_num_lane0
	abcon_wr_reg_bits(0x3b, abcon->conf.chip_num[0], 0, 8);
	//reg_abcon_ldc_pkt_num_lane1
	abcon_wr_reg_bits(0x3b, abcon->conf.chip_num[1], 8, 8);
	//reg_abcon_ldc_pkt_num_lane2
	abcon_wr_reg_bits(0x3b, abcon->conf.chip_num[2], 16, 8);
	//reg_abcon_ldc_pkt_num_lane3
	abcon_wr_reg_bits(0x3b, abcon->conf.chip_num[3], 24, 8);

	//reg_abcon_rx_wait_tx
	abcon_wr_reg_bits(0x40, 1, 30, 1);

	//reg_abcon_pin_dir_sel_time
	abcon_wr_reg_bits(0x43, abcon->max_lane_dim * 10000 / 6, 0, 20);//9000
	//reg_abcon_pin_dir_del_start
	abcon_wr_reg_bits(0x00, 1, 8, 1);
	abcon_wr_reg_bits(0x00, 0, 8, 1);

	abcon_wr_reg(0x1a, 0x7f000000);
	abcon_wr_reg(0x1b, 0x20c32940);//6dc+10pwm	//todo
}

void ldim_abcon_init_xianxin(void)
{
	char i;

	if (bl_abcon_xianxin->dev_type) {//871
		if (bl_abcon_xianxin->init_sampletime == 0) {
			//set txrx clk as 500Khz, due to dimmer default set is 500Khz
			ldim_abcon_set_txrx_clk(500, 500);
			//set sample point
			ldim_abcon_write_xianxin_register(0x01, 0x1b);
			bl_abcon_xianxin->init_sampletime = 1;
		}

		//set data format
		ldim_abcon_write_xianxin_register(0x02, 0xa4);

		//command set
		ldim_abcon_write_xianxin_register(0x03, 0x01);

		//send address command
		for (i = 0; i < abcon->max_lane_dim; i++)
			ldim_abcon_write_xianxin_register(bl_abcon_xianxin->regmap->dim_addr, 1 + i);

		//set sample point
		//ldim_abcon_write_xianxin_register(0x01, bl_abcon_xianxin->samplepoint);
		//set txrx clk
		//ldim_abcon_set_txrx_clk(abcon->conf.tx_clk, abcon->conf.rx_clk);
	} else {//852
		if (bl_abcon_xianxin->init_sampletime == 0) {
			//set txrx clk as 500Khz, due to dimmer default set is 500Khz
			ldim_abcon_set_txrx_clk(330, 330);
			//set sample point
			ldim_abcon_write_xianxin_register(0x12, 0x17);
			bl_abcon_xianxin->init_sampletime = 1;
		}

		//set data format
		ldim_abcon_write_xianxin_register(0x1d, 0xa5);

		//set pwm en
		ldim_abcon_write_xianxin_register(0x03, 0x0f);

		//command set
		ldim_abcon_write_xianxin_register(0x00, 0x01);

		//send address command
		for (i = 0; i < abcon->max_lane_dim; i++)
			ldim_abcon_write_xianxin_register(bl_abcon_xianxin->regmap->dim_addr, 1 + i);

		//set sample point
		//ldim_abcon_write_xianxin_register(0x12, bl_abcon_xianxin->samplepoint);
		//set txrx clk
		//ldim_abcon_set_txrx_clk(abcon->conf.tx_clk, abcon->conf.rx_clk);
	}
}

static int abcon_xianxin_hw_init_on(struct ldim_dev_driver_s *dev_drv)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	/* step 1: system power_on */
	ldim_gpio_set(dev_drv, dev_drv->en_gpio, dev_drv->en_gpio_on);

	/* step 2: delay for internal logic stable */
	mdelay(10);

	/* step 3: Generate external VSYNC to VSYNC/PWM pin */
	ldim_set_duty_pwm(&dev_drv->ldim_pwm_config);
	ldim_set_duty_pwm(&dev_drv->analog_pwm_config);
	dev_drv->pinmux_ctrl(dev_drv, 1);

	//init abcon common
	ldim_abcon_init_common_registers(abcon);

	//init abcon register for xianxin
	ldim_abcon_init_registers_for_xianxin();

	//init xianxin registers
	ldim_abcon_init_xianxin();

	mdelay(1);
	ldim_abcon_swrst();

	//set duty as 0
	memset(ldim_drv->bl_matrix, 0,
		(dev_drv->bl_col * dev_drv->bl_row * sizeof(unsigned short)));
	ldim_abcon_swduty_set(ldim_drv);

	//reg_abcon_duty_zone_src_sel
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id

	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	ldim_abcon_xianxin_dimming();

	return 0;
}

static int abcon_xianxin_hw_init_off(struct ldim_dev_driver_s *dev_drv)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_gpio_set(dev_drv, dev_drv->en_gpio, dev_drv->en_gpio_off);
	dev_drv->pinmux_ctrl(dev_drv, 0);
	ldim_pwm_off(&dev_drv->ldim_pwm_config);
	ldim_pwm_off(&dev_drv->analog_pwm_config);

	//sw reset
	ldim_abcon_swrst();

	//set duty as 0
	memset(ldim_drv->bl_matrix, 0,
		(dev_drv->bl_col * dev_drv->bl_row * sizeof(unsigned short)));
	ldim_abcon_swduty_set(ldim_drv);
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id
	ldim_abcon_xianxin_dimming();

	//sw reset
	ldim_abcon_swrst();

	return 0;
}

static int abcon_xianxin_smr(struct aml_ldim_driver_s *ldim_drv, unsigned short *buf, unsigned int len)
{
	int ret = 0;

	if (!bl_abcon_xianxin)
		return -1;

	if (bl_abcon_xianxin->dev_on_flag == 0) {
		ABCONPR("%s: on_flag=%d\n", __func__, bl_abcon_xianxin->dev_on_flag);
		return 0;
	}

	ldim_abcon_swduty_set(ldim_drv);

	//reg_abcon_duty_zone_src_sel
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id

	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	ldim_abcon_xianxin_dimming();

	return ret;
}

static int abcon_xianxin_power_on(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_abcon_xianxin)
		return -1;
	if (bl_abcon_xianxin->dev_on_flag) {
		ABCONPR("%s: abcon_xianxin is already on, exit\n", __func__);
		return 0;
	}

	abcon_xianxin_hw_init_on(ldim_drv->dev_drv);
	bl_abcon_xianxin->dev_on_flag = 1;
	bl_abcon_xianxin->vsync_cnt = 0;

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_xianxin_power_off(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_abcon_xianxin)
		return -1;

	bl_abcon_xianxin->dev_on_flag = 0;
	abcon_xianxin_hw_init_off(ldim_drv->dev_drv);

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_xianxin_ldim_dev_update(struct ldim_dev_driver_s *dev_drv)
{

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	dev_drv->power_on = abcon_xianxin_power_on;
	dev_drv->power_off = abcon_xianxin_power_off;
	dev_drv->dev_smr = abcon_xianxin_smr;

	dev_drv->reg_write = NULL;
	dev_drv->reg_read = NULL;
	return 0;
}

int ldim_dev_abcon_xianxin_probe(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		ABCONERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	bl_abcon_xianxin = (struct abcon_xianxin_s *)malloc(sizeof(struct abcon_xianxin_s));
	if (!bl_abcon_xianxin) {
		ABCONERR("%s: bl_abcon_xianxin is error\n", __func__);
		return -1;
	}

	bl_abcon_xianxin->dev_type = abcon->conf.ctrl & 0x1;

	if (bl_abcon_xianxin->dev_type) {
		bl_abcon_xianxin->regmap = &regmap_conf_871;
		//samplepoint is about ((1000000/txclk)/2)/31.79
		bl_abcon_xianxin->samplepoint = (1000000 / abcon->conf.tx_clk) >> 6;
	} else {
		bl_abcon_xianxin->regmap = &regmap_conf_852;
		//samplepoint is about ((1000000/txclk)/2)/63.581
		bl_abcon_xianxin->samplepoint = (1000000 / abcon->conf.tx_clk) >> 7;
	}

	bl_abcon_xianxin->dev_on_flag = 0;
	bl_abcon_xianxin->vsync_cnt = 0;
	bl_abcon_xianxin->dataformat = 0;//to do
	bl_abcon_xianxin->init_sampletime = 0;

	ABCONPR("samplepoint=%d\n", bl_abcon_xianxin->samplepoint);

	//init abcon register for xianxin
	ldim_abcon_init_registers_for_xianxin();

	if (bl_abcon_xianxin->bl_pwm_en) {
		/* Generate external VSYNC to VSYNC/PWM pin */
		ldim_set_duty_pwm(&dev_drv->ldim_pwm_config);
		ldim_set_duty_pwm(&dev_drv->analog_pwm_config);
		dev_drv->pinmux_ctrl(dev_drv, 1);
	}

	abcon_xianxin_ldim_dev_update(dev_drv);

	ABCONPR("%s ok\n", __func__);
	return 0;
}

int ldim_dev_abcon_xianxin_remove(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_abcon_xianxin)
		return 0;

	free(bl_abcon_xianxin);
	bl_abcon_xianxin = NULL;

	return 0;
}
