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
	sop_address = 0,
	sop_broadcast,
	sop_readback,
	sop_dimming,
	sop_feedback,
	sop_max,
};

struct abcon_spb_s {
	unsigned int dev_on_flag;
	unsigned short vsync_cnt;
	unsigned char bl_pwm_en;	/*0:default uboot have pwm, 1: uboot without pwm*/

	unsigned char act_lane_num;
	unsigned char dual_wire;
	unsigned char default_value;
	unsigned int fb_cnt;
	unsigned int fb_high_cnt_th;
	unsigned char sop_type;
};

struct abcon_spb_s *bl_spb;

struct spb_packet_s {
	//unsigned int packet_header_len;
	//unsigned int packet_header[6];

	unsigned int preamble_header;
	unsigned int preamble_data[2];

	unsigned int sop_header;
	unsigned int sop_data;

	unsigned int addr_header;
	unsigned int addr_data;

	unsigned int data_header;
	unsigned int data_data;

	unsigned int crc_header;
	unsigned int crc_data;

	unsigned int eop_header;
	unsigned int eop_data;
};

struct spb_packet_s spb_packet;

void ldim_abcon_spb_write_packet(void)
{
	unsigned int len = 0;
	unsigned int *buf;// = abcon_mem.wseg;
	unsigned char i = 0;
	unsigned char act_lane_num = abcon->act_lane_num;

	if (bl_spb->sop_type == sop_dimming)
		buf = abcon_mem.ldc_seg;
	else
		buf = abcon_mem.wseg;

	if (!buf)
		return;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.preamble_header;

	for (i = 0; i < act_lane_num; i++) {
		buf[len++] = spb_packet.preamble_data[0];
		buf[len++] = spb_packet.preamble_data[1];
	}

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.sop_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.sop_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.addr_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.addr_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.data_header;

	if (bl_spb->sop_type != sop_dimming) {
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = spb_packet.data_data;
	}

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.crc_header;

	//for (i = 0; i < act_lane_num; i++)
	//	buf[len++] = spb_packet.crc_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.eop_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = spb_packet.eop_data;

	flush_dcache_range((unsigned long)buf,
		(unsigned long)(buf + len));

	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	if (bl_spb->sop_type == sop_dimming) {
		abcon_wr_reg_bits(0x04, len, 16, 13);//reg_abcon_ldc_seg_max_num
		abcon_wr_reg_bits(0x00, 1, 1, 1);//reg_abcon_ldc_mod_start
		abcon_wr_reg_bits(0x00, 0, 1, 1);//reg_abcon_ldc_mod_start
		mdelay(15);
	} else {
		abcon_wr_reg_bits(0x04, len, 0, 13);//reg_abcon_sw_wseg_max_num
		abcon_wr_reg_bits(0x00, 1, 0, 1);//reg_abcon_sw_mod_start
		abcon_wr_reg_bits(0x00, 0, 0, 1);//reg_abcon_sw_mod_start
	}
}

void ldim_abcon_spb_read_packet(void)
{
	unsigned int len = 0;
	unsigned int *buf = abcon_mem.rseg;

	if (!buf)
		return;

	buf[len++] = spb_packet.preamble_header;

	buf[len++] = spb_packet.preamble_data[0];
	buf[len++] = spb_packet.preamble_data[1];

	buf[len++] = spb_packet.sop_header;
	buf[len++] = spb_packet.sop_data;

	buf[len++] = spb_packet.addr_header;
	buf[len++] = spb_packet.addr_data;

	buf[len++] = spb_packet.data_header;
	//buf[len++] = spb_packet.data_data;

	buf[len++] = spb_packet.crc_header;
//	buf[len++] = spb_packet.crc_data;

	buf[len++] = spb_packet.eop_header;
	buf[len++] = spb_packet.eop_data;

	flush_dcache_range((unsigned long)buf,
		(unsigned long)(buf + len));

	abcon_wr_reg_bits(0x05, len, 16, 13);//reg_abcon_sw_rseg_max_num
}

void ldim_abcon_spb_broadcast(unsigned int regaddr, unsigned int value)
{
	bl_spb->sop_type = sop_broadcast;

	//sw reset
	//abcon_wr_reg_bits(0x00, 1, 5, 1);
	//abcon_wr_reg_bits(0x00, 0, 5, 1);

	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 1, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//reg_abcon_act_lane
	if (abcon->chip_type == ABCON_CHIP_T6W)
		abcon_wr_reg_bits(0x2a, abcon->act_lane, 22, 4);
	else
		abcon_wr_reg_bits(0x29, abcon->act_lane, 0, 12);
	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x9a9a, 16, 16);

	spb_packet.preamble_header = 0x70006082;
	spb_packet.preamble_data[0] = 0x55555555;
	spb_packet.preamble_data[1] = 0x05555555;

	spb_packet.sop_header = 0x500020b0;
	spb_packet.sop_data = 0x000c1f07;

	spb_packet.addr_header = 0x400020e0;
	spb_packet.addr_data = (regaddr & 0x7f) << 8;

	spb_packet.data_header = 0x400020e0;
	spb_packet.data_data = value;

	spb_packet.crc_header = 0x400020c4;//hw_crc_en, without crc_data
	//spb_packet.crc_data = 0x400020c4;

	spb_packet.eop_header = 0x14002088;
	spb_packet.eop_data = 0x0000000d;

	ldim_abcon_spb_write_packet();
	udelay(1500);
}

void ldim_abcon_spb_address(void)
{
	bl_spb->sop_type = sop_address;

	//sw reset
	//abcon_wr_reg_bits(0x00, 1, 5, 1);
	//abcon_wr_reg_bits(0x00, 0, 5, 1);
	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//reg_abcon_act_lane
	if (abcon->chip_type == ABCON_CHIP_T6W)
		abcon_wr_reg_bits(0x2a, abcon->act_lane, 22, 4);
	else
		abcon_wr_reg_bits(0x29, abcon->act_lane, 0, 12);
	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x959f, 16, 16);

	spb_packet.preamble_header = 0x70006082;
	spb_packet.preamble_data[0] = 0x55555555;
	spb_packet.preamble_data[1] = 0x05555555;

	spb_packet.sop_header = 0x500020b0;
	spb_packet.sop_data = 0x000c6711;

	spb_packet.addr_header = 0x400020e0;
	spb_packet.addr_data = 0x00000001;

	spb_packet.data_header = 0x400020e0;
	spb_packet.data_data = 0;

	spb_packet.crc_header = 0x400020c4;//hw_crc_en, without crc_data
	//spb_packet.crc_data = 0x400020c4;

	spb_packet.eop_header = 0x14002088;
	spb_packet.eop_data = 0x0000000d;

	ldim_abcon_spb_write_packet();
	udelay(1500);
}

void ldim_abcon_spb_dimming(void)
{
	bl_spb->sop_type = sop_dimming;
	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 1, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//reg_abcon_act_lane
	if (abcon->chip_type == ABCON_CHIP_T6W)
		abcon_wr_reg_bits(0x2a, abcon->act_lane, 22, 4);
	else
		abcon_wr_reg_bits(0x29, abcon->act_lane, 0, 12);
	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x999f, 16, 16);

	spb_packet.preamble_header = 0x70006082;
	spb_packet.preamble_data[0] = 0x55555555;
	spb_packet.preamble_data[1] = 0x05555555;

	spb_packet.sop_header = 0x500020b0;
	spb_packet.sop_data = 0x000c6311;

	spb_packet.addr_header = 0x400020e0;
	spb_packet.addr_data = 0x00002001;

	spb_packet.data_header = 0x10e1 | ((abcon->max_lane_ch & 0xfff) << 14);
	//spb_packet.data_data = value;

	spb_packet.crc_header = 0x400020c4;//hw_crc_en, without crc_data
	//spb_packet.crc_data = 0x400020c4;

	spb_packet.eop_header = 0x14002088;
	spb_packet.eop_data = 0x0000000d;

	ldim_abcon_spb_write_packet();
}

unsigned int ldim_abcon_spb_readback(unsigned char reg, unsigned char chip)
{
	unsigned int readback_val = 0;

	bl_spb->sop_type = sop_readback;

	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x5559, 16, 16);

	//prepare read packet first
	spb_packet.preamble_header = 0xe000c105;
	spb_packet.preamble_data[0] = 0x55555555;
	spb_packet.preamble_data[1] = 0x05555555;

	spb_packet.sop_header = 0xa0004141;
	spb_packet.sop_data = 0x000c6711;

	spb_packet.addr_header = 0x800041c1;
	spb_packet.addr_data = ((reg & 0x7f) << 8) | (chip & 0xff);

	spb_packet.data_header = 0x800041c3;
	//spb_packet.data_data = data;

	spb_packet.crc_header = 0x400020c4;//hw_crc_en, without crc_data
	//spb_packet.crc_data = 0x400020c4;

	spb_packet.eop_header = 0x28004111;
	spb_packet.eop_data = 0x0000000d;
	ldim_abcon_spb_read_packet();

	//then set write packet
	spb_packet.preamble_header = 0x70006082;
	spb_packet.preamble_data[0] = 0x55555555;
	spb_packet.preamble_data[1] = 0x05555555;

	spb_packet.sop_header = 0x500020b0;
	spb_packet.sop_data = 0x000c6711;

	spb_packet.addr_header = 0x400020e0;
	spb_packet.addr_data = 0x00000001;

	spb_packet.data_header = 0x400020e0;
	spb_packet.data_data = 0;

	spb_packet.crc_header = 0x400020c4;//hw_crc_en, without crc_data
	//spb_packet.crc_data = 0x400020c4;

	spb_packet.eop_header = 0x14002088;
	spb_packet.eop_data = 0x0000000d;

	ldim_abcon_spb_write_packet();
	mdelay(1);
	readback_val = abcon_rd_reg(0x52);

	return readback_val;
}

void ldim_abcon_spb_init_feedback(void)
{
	bl_spb->sop_type = sop_feedback;
	//reg_abcon_fb_en
	abcon_wr_reg_bits(0x37, abcon->conf.fb_en, 20, 1);
	//reg_abcon_fb_sta_immed
	abcon_wr_reg_bits(0x37, 1, 16, 1);
	//reg_abcon_fb_tim_res, 100us sampling interval, 100000/6=16666
	abcon_wr_reg_bits(0x38, 16666, 16, 16);
	//reg_abcon_pre_fb_time
	abcon_wr_reg_bits(0x38, 1, 0, 16);
	//reg_abcon_post_fb_time
	abcon_wr_reg_bits(0x39, 1, 0, 16);
	//reg_abcon_fb_read_time, expect sampling count
	abcon_wr_reg_bits(0x39, bl_spb->fb_cnt, 16, 16);
}

void ldim_abcon_init_registers(void)
{
	//reg_abcon_clr_done_flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_frc_crc_val_en
	abcon_wr_reg_bits(0x03, 1, 0, 1);
	//reg_abcon_rx_scale_sel
	abcon_wr_reg_bits(0x03, 1, 1, 1);
	//reg_abcon_hw_crc
	abcon_wr_reg_bits(0x03, 1, 3, 1);
	//reg_abcon_dual_line_mode
	abcon_wr_reg_bits(0x03, bl_spb->dual_wire, 4, 1);
	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 1, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_default_value
	abcon_wr_reg_bits(0x03, bl_spb->default_value, 8, 1);
	//reg_abcon_rx_preamble_num
	abcon_wr_reg_bits(0x03, 60, 24, 8);
	//reg_abcon_crc_poly
	abcon_wr_reg(0x0d, 0x00008005);
	//reg_abcon_crc_init
	abcon_wr_reg(0x0e, 0xffffffff);

	abcon_wr_reg(0x11, 0x70f081ff);

	abcon_wr_reg(0x1a, 0x7f000000);
	abcon_wr_reg(0x1b, 0x20c32940);//6dc+10pwm

	//init feedback registers
	//ldim_abcon_spb_init_feedback();
}

void ldim_abcon_init_spb(void)
{
	char i;

	//set dos as communications
	ldim_abcon_spb_broadcast(0x44, 0x8000);

	//send address command
	for (i = 0; i < 3; i++)
		ldim_abcon_spb_address();

	//unlock
	for (i = 0; i < 3; i++) {
		ldim_abcon_spb_broadcast(0x78, 0xa5c9);
		ldim_abcon_spb_broadcast(0x7c, 0xa5c9);
	}

	//set iset range
	ldim_abcon_spb_broadcast(0x48, 0x6037);

	//dis dither
	ldim_abcon_spb_broadcast(0x4a, 0x0099);

	//en dos
	ldim_abcon_spb_broadcast(0x4c, 0x0029);

	//set pwm clk,
	ldim_abcon_spb_broadcast(0x4e, 0x9e07);

	//headroom
	ldim_abcon_spb_broadcast(0x5a, 0x0001);

	//dos clk
	ldim_abcon_spb_broadcast(0x5b, 0x0021);

	//ch enable
	ldim_abcon_spb_broadcast(0x40, 0x000f);

	//config done, sic ref
	ldim_abcon_spb_broadcast(0x46, 0x8842);

	//clr error flag
	ldim_abcon_spb_broadcast(0x60, 0x001f);

	//dos sick feedback, dimming mode
	ldim_abcon_spb_broadcast(0x44, 0xa100);
}

static int abcon_spb_hw_init_on(struct ldim_dev_driver_s *dev_drv)
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

	//init abcon register for spb
	ldim_abcon_init_registers();

	//init spb registers
	ldim_abcon_init_spb();

	return 0;
}

static int abcon_spb_hw_init_off(struct ldim_dev_driver_s *dev_drv)
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
	ldim_abcon_spb_dimming();

	//sw reset
	ldim_abcon_swrst();

	return 0;
}

static int abcon_spb_smr(struct aml_ldim_driver_s *ldim_drv, unsigned short *buf, unsigned int len)
{
	int ret = 0;

	if (!bl_spb)
		return -1;

	if (bl_spb->dev_on_flag == 0) {
		ABCONPR("%s: on_flag=%d\n", __func__, bl_spb->dev_on_flag);
		return 0;
	}

	ldim_abcon_swduty_set(ldim_drv);

	//reg_abcon_duty_zone_src_sel
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id

	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x999f, 16, 16);

	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	ldim_abcon_spb_dimming();

	return ret;
}

static int abcon_spb_power_on(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_spb)
		return -1;
	if (bl_spb->dev_on_flag) {
		ABCONPR("%s: abcon_spb is already on, exit\n", __func__);
		return 0;
	}

	abcon_spb_hw_init_on(ldim_drv->dev_drv);
	bl_spb->dev_on_flag = 1;
	bl_spb->vsync_cnt = 0;

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_spb_power_off(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_spb)
		return -1;

	bl_spb->dev_on_flag = 0;
	abcon_spb_hw_init_off(ldim_drv->dev_drv);

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_spb_dev_update(struct ldim_dev_driver_s *dev_drv)
{

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	dev_drv->power_on = abcon_spb_power_on;
	dev_drv->power_off = abcon_spb_power_off;
	dev_drv->dev_smr = abcon_spb_smr;

	dev_drv->reg_write = NULL;
	dev_drv->reg_read = NULL;
	return 0;
}

int ldim_dev_abcon_spb_probe(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		ABCONERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	bl_spb = (struct abcon_spb_s *)malloc(sizeof(struct abcon_spb_s));
	if (!bl_spb) {
		ABCONERR("%s: bl_spb is error\n", __func__);
		return -1;
	}

	bl_spb->dev_on_flag = 0;
	bl_spb->vsync_cnt = 0;

	bl_spb->fb_cnt = 10;
	bl_spb->fb_high_cnt_th = 8;

	bl_spb->dual_wire = abcon->conf.ctrl & 0x1;
	bl_spb->default_value = (abcon->conf.ctrl >> 1) & 0x1;
	ABCONPR("ctrl=0x%x, dual_wire=%d, default_value=%d\n",
	abcon->conf.ctrl, bl_spb->dual_wire, bl_spb->default_value);

	//init abcon register for spb
	ldim_abcon_init_registers();

	if (bl_spb->bl_pwm_en) {
		/* Generate external VSYNC to VSYNC/PWM pin */
		ldim_set_duty_pwm(&dev_drv->ldim_pwm_config);
		ldim_set_duty_pwm(&dev_drv->analog_pwm_config);
		dev_drv->pinmux_ctrl(dev_drv, 1);
	}

	abcon_spb_dev_update(dev_drv);

	ABCONPR("%s ok\n", __func__);
	return 0;
}

int ldim_dev_abcon_spb_remove(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_spb)
		return 0;

	free(bl_spb);
	bl_spb = NULL;

	return 0;
}
