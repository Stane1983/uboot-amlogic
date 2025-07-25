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
	sop_broadcast = 0,
	sop_config,
	sop_dimming,
	sop_readback,
	sop_max,
};

struct abcon_owc_s {
	unsigned int dev_on_flag;
	unsigned short vsync_cnt;
	unsigned char bl_pwm_en;	/*0:default uboot have pwm, 1: uboot without pwm*/

	unsigned char act_lane_num;
	unsigned char sop_type;
	unsigned char datalen;
	unsigned char dc_num;//per dim
	unsigned char pwm_num;//per dim
	unsigned char tot_dc_pwm_num;
};

struct abcon_owc_s *bl_owc;

struct owc_pkt_s {
	unsigned int pream_header;
	unsigned int pream_data;

	unsigned int addr_header;
	unsigned int addr_data;

	unsigned int status_header;
	unsigned int status_data;

	unsigned int reg_addr_header;
	unsigned int reg_addr_data;

	unsigned int datalen_header;
	unsigned int datalen_data;

	unsigned int data_header;
	unsigned int data_data;

	unsigned int end_header;
	unsigned int end_data;
};

struct owc_pkt_s owc_pkt;

static void ldim_abcon_owc_write_pkt(unsigned char lanenum, unsigned char *data)
{
	unsigned int len = 0;
	unsigned int *buf;// = abcon_mem.wseg;
	unsigned char i = 0, j = 0;
	unsigned char act_lane_num = lanenum;
	unsigned char temp = 0;
	unsigned short valid = 0;

	if (bl_owc->sop_type == sop_dimming)
		buf = abcon_mem.ldc_seg;
	else
		buf = abcon_mem.wseg;

	if (bl_owc->sop_type == sop_readback)
		act_lane_num = 1;

	if (!buf)
		return;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.pream_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.pream_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.addr_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.addr_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.status_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.status_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.reg_addr_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.reg_addr_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.datalen_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.datalen_data;

	if (bl_owc->sop_type == sop_dimming) {
		for (i = 0; i < act_lane_num; i++) {
			valid = abcon->act_lane & (1 << i);
			if (valid == 0)
				j = j + 1;

			buf[len++] = 0xc81 |
			((abcon->conf.chip_num[j] * bl_owc->tot_dc_pwm_num) << 14);
			j++;
		}
	} else {
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = owc_pkt.data_header;
	}

	if (bl_owc->sop_type != sop_dimming) {
		for (j = 0; j <= owc_pkt.datalen_data; j++)
			for (i = 0; i < act_lane_num; i++)
				buf[len++] = data[j];
	}

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = owc_pkt.end_header;

	if (abcon->conf.dev_type == 2) {
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = owc_pkt.end_data;
	} else {
		temp = (abcon->max_lane_dim / 32) + 1;
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = owc_pkt.end_data;
	}

	flush_dcache_range((unsigned long)buf,
		(unsigned long)(buf + len));
	//clr done flag
	ldim_abcon_swrst();

	if (bl_owc->sop_type == sop_dimming) {
		abcon_wr_reg_bits(0x04, len, 16, 13);//reg_abcon_ldc_seg_max_num
		abcon_wr_reg_bits(0x00, 1, 1, 1);//reg_abcon_ldc_mod_start
		abcon_wr_reg_bits(0x00, 0, 1, 1);//reg_abcon_ldc_mod_start
		mdelay(20);
	} else {
		abcon_wr_reg_bits(0x04, len, 0, 13);//reg_abcon_sw_wseg_max_num
		abcon_wr_reg_bits(0x00, 1, 0, 1);//reg_abcon_sw_mod_start
		abcon_wr_reg_bits(0x00, 0, 0, 1);//reg_abcon_sw_mod_start
	}
}

static void ldim_abcon_owc_broadcast(unsigned char reg_addr, unsigned char data)
{
	bl_owc->sop_type = sop_broadcast;

	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//reg_abcon_act_lane
	if (abcon->chip_type == ABCON_CHIP_T6W)
		abcon_wr_reg_bits(0x2a, abcon->act_lane, 22, 4);
	else
		abcon_wr_reg_bits(0x29, abcon->act_lane, 0, 12);

	if (abcon->conf.dev_type == 2) {
		//xp7104, 2 *bit1
		owc_pkt.pream_header = 0x08002082;
		owc_pkt.pream_data = 0x3;
	} else {
		//xp7008, 1 * bit1
		owc_pkt.pream_header = 0x04002082;
		owc_pkt.pream_data = 0x1;
	}

	owc_pkt.addr_header = 0x20002080;
	owc_pkt.addr_data = 0xff;//write same regs with same data

	owc_pkt.status_header = 0x60002080;
	owc_pkt.status_data = 0x00;

	owc_pkt.reg_addr_header = 0x20002080;
	owc_pkt.reg_addr_data = reg_addr & 0x7f;

	owc_pkt.datalen_header = 0x20002080;
	owc_pkt.datalen_data = 0;

	owc_pkt.data_header = 0x4880;// | (abcon->act_lane_num) << 14;
	//owc_pkt.data_data = value;

	if (abcon->conf.dev_type == 2) {
		//xp7104, 8 * bit0
		owc_pkt.end_header = 0x20002088;
		owc_pkt.end_data = 0x0;
	} else {
		//xp7008, 1 * bit1
		owc_pkt.end_header = 0x2088 | (abcon->max_lane_dim << 26);
		owc_pkt.end_data = 0x0;
	}

	ldim_abcon_owc_write_pkt(abcon->act_lane_num, &data);
	udelay(500);
}

static void ldim_abcon_owc_dimming(void)
{
	unsigned char data = 0;

	bl_owc->sop_type = sop_dimming;

	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//reg_abcon_act_lane
	if (abcon->chip_type == ABCON_CHIP_T6W)
		abcon_wr_reg_bits(0x2a, abcon->act_lane, 22, 4);
	else
		abcon_wr_reg_bits(0x29, abcon->act_lane, 0, 12);

	if (abcon->conf.dev_type == 2) {
		//xp7104, 2 *bit1
		owc_pkt.pream_header = 0x08002082;
		owc_pkt.pream_data = 0x3;
	} else {
		//xp7008, 1 * bit1
		owc_pkt.pream_header = 0x04002082;
		owc_pkt.pream_data = 0x1;
	}

	owc_pkt.addr_header = 0x20002080;
	owc_pkt.addr_data = 0x01;

	owc_pkt.status_header = 0x60002080;
	owc_pkt.status_data = 0x00;

	owc_pkt.reg_addr_header = 0x20002080;
	owc_pkt.reg_addr_data = 0;

	owc_pkt.datalen_header = 0x20002080;
	owc_pkt.datalen_data = (bl_owc->datalen - 1) & 0x3f;

	//owc_pkt.data_header = 0x881 | (24 << 14);
	//((abcon->act_lane_num * datalen * abcon->max_lane_dim) << 14);
	//owc_pkt.data_data = value;

	if (abcon->conf.dev_type == 2) {
		//xp7104, 8 * bit0
		owc_pkt.end_header = 0x20002088;
		owc_pkt.end_data = 0x0;
	} else {
		//xp7008, 1 * bit1
		owc_pkt.pream_header = 0x2088 | (abcon->max_lane_dim << 26);
		owc_pkt.pream_data = 0x0;
	}

	ldim_abcon_owc_write_pkt(abcon->act_lane_num, &data);
	udelay(500);
}

static void ldim_abcon_init_registers(void)
{
	//swrst
	ldim_abcon_swrst();

	//reg_abcon_wire_mode
	abcon_wr_reg_bits(0x22, 0, 30, 2);
	//reg_abcon_rx_scale_sel
	abcon_wr_reg_bits(0x03, 1, 1, 1);
	//reg_abcon_dual_line_mode
	abcon_wr_reg_bits(0x03, 0, 4, 1);

	//only 12bit pwm
	//abcon_wr_reg(0x1a, 0x7b000000);
	//abcon_wr_reg(0x1b, 0x00c00180);

	//12bit pwm + 8 dc
	abcon_wr_reg(0x1a, 0x7e000000);
	abcon_wr_reg(0x1b, 0x20c40180);

	//reg_abcon_pwm_num_per_dim
	abcon_wr_reg_bits(0x40, bl_owc->pwm_num, 0, 8);
	//reg_abcon_dc_num_per_dim
	abcon_wr_reg_bits(0x40, bl_owc->dc_num, 8, 8);

	//init feedback registers
	//ldim_abcon_owc_init_feedback();
}

void ldim_abcon_init_owc(void)
{
	ldim_abcon_owc_broadcast(0x6f, 0xb0);
	ldim_abcon_owc_broadcast(0x69, 0xff);//set pwm mode

	ldim_abcon_owc_broadcast(0x0c, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x0d, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x0e, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x0f, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x10, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x11, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x12, 0x5e);//22ma max60ma
	ldim_abcon_owc_broadcast(0x13, 0x5e);//22ma max60ma
}

static int abcon_owc_hw_init_on(struct ldim_dev_driver_s *dev_drv)
{
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

	//init abcon register for xsignal
	ldim_abcon_init_registers();

	//init xsignal registers
	ldim_abcon_init_owc();

	return 0;
}

static int abcon_owc_hw_init_off(struct ldim_dev_driver_s *dev_drv)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_gpio_set(dev_drv, dev_drv->en_gpio, dev_drv->en_gpio_off);
	dev_drv->pinmux_ctrl(dev_drv, 0);
	ldim_pwm_off(&dev_drv->ldim_pwm_config);
	ldim_pwm_off(&dev_drv->analog_pwm_config);

	//set duty as 0
	memset(ldim_drv->bl_matrix, 0,
		(dev_drv->bl_col * dev_drv->bl_row * sizeof(unsigned short)));
	ldim_abcon_swduty_set(ldim_drv);
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id
	ldim_abcon_owc_dimming();

	//sw reset
	ldim_abcon_swrst();

	return 0;
}

static int abcon_owc_smr(struct aml_ldim_driver_s *ldim_drv, unsigned short *buf, unsigned int len)
{
	int ret = 0;

	if (!bl_owc)
		return -1;

	if (bl_owc->dev_on_flag == 0) {
		ABCONPR("%s: on_flag=%d\n", __func__, bl_owc->dev_on_flag);
		return 0;
	}

	ldim_abcon_swduty_set(ldim_drv);

	//reg_abcon_duty_zone_src_sel
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id

	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	ldim_abcon_owc_dimming();

	return ret;
}

static int abcon_owc_power_on(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_owc)
		return -1;
	if (bl_owc->dev_on_flag) {
		ABCONPR("%s: abcon_owc is already on, exit\n", __func__);
		return 0;
	}

	abcon_owc_hw_init_on(ldim_drv->dev_drv);
	bl_owc->dev_on_flag = 1;
	bl_owc->vsync_cnt = 0;

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_owc_power_off(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_owc)
		return -1;

	bl_owc->dev_on_flag = 0;
	abcon_owc_hw_init_off(ldim_drv->dev_drv);

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_owc_dev_update(struct ldim_dev_driver_s *dev_drv)
{

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	dev_drv->power_on = abcon_owc_power_on;
	dev_drv->power_off = abcon_owc_power_off;
	dev_drv->dev_smr = abcon_owc_smr;

	dev_drv->reg_write = NULL;
	dev_drv->reg_read = NULL;
	return 0;
}

int ldim_dev_abcon_owc_probe(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		ABCONERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	bl_owc = (struct abcon_owc_s *)malloc(sizeof(struct abcon_owc_s));
	if (!bl_owc) {
		ABCONERR("%s: bl_owc is error\n", __func__);
		return -1;
	}

	bl_owc->dev_on_flag = 0;
	bl_owc->vsync_cnt = 0;

	bl_owc->datalen = abcon->conf.ctrl & 0xff;
	bl_owc->dc_num = (abcon->conf.ctrl >> 8) & 0xf;
	bl_owc->pwm_num = (abcon->conf.ctrl >> 12) & 0xf;
	bl_owc->tot_dc_pwm_num = bl_owc->dc_num + bl_owc->pwm_num;
	ABCONPR("ctrl=0x%x, datalen=%d, dc_num=%d, pwm_num=%d\n",
	abcon->conf.ctrl, bl_owc->datalen, bl_owc->dc_num, bl_owc->pwm_num);

	//init abcon register for xsignal
	ldim_abcon_init_registers();

	if (bl_owc->bl_pwm_en) {
		/* Generate external VSYNC to VSYNC/PWM pin */
		ldim_set_duty_pwm(&dev_drv->ldim_pwm_config);
		ldim_set_duty_pwm(&dev_drv->analog_pwm_config);
		dev_drv->pinmux_ctrl(dev_drv, 1);
	}

	abcon_owc_dev_update(dev_drv);

	ABCONPR("%s ok\n", __func__);
	return 0;
}

int ldim_dev_abcon_owc_remove(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_owc)
		return 0;

	free(bl_owc);
	bl_owc = NULL;

	return 0;
}
