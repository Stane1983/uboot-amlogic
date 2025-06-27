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

enum ospb_head_e {
	HEAD_BROADCAST	= 0x7200,
	HEAD_ADDRESS	= 0x7400,
	HEAD_DIMMING	= 0x7100,
	HEAD_CONFIG		= 0x7300,
	HEAD_READBACK	= 0x7500,
	HEAD_MAX,
};

//8608/8108
enum dim_mode_e {
	PWM_10PWM_2DITHER = 0,
	DC_10DC_6DITHER,
	MIX_6DC_10PWM,
	MIX_10DC_6PWM,
	DC_10DC,
	DC_8DC_8DITHER,
	DIM_MODE_MAX
};

enum dos_mode_e {
	SINGLE = 0,
	CH_SICK_INT1,
	SHORT_INT,
	OPEN_INT,
	OTP_INT,
	CRC_INT,
	PWM_HIGH_TO_SMALL,
	CH_SICK_INT2,
	OH_FAULT,
	ID_FAULT,
	FAULT_CRC,
	ADDR_CHECK = 0xb,
	DIP_TIMEOUT,
	UART_COMM = 0xf,
};

#define FB_VALUE 0xF840
#define COMM_PARALLEL 0
#define COMM_SERIAL 1
#define UNLOCK_KEY 0xa5c9
#define LOCK_KEY 0x5ac3

struct abcon_hy_ospb_s {
	unsigned int dev_on_flag;
	unsigned short vsync_cnt;
	unsigned char bl_pwm_en;	/*0:default uboot have pwm, 1: uboot without pwm*/

	unsigned char act_lane_num;
	unsigned char dual_wire;
	unsigned char default_value;
	unsigned int fb_cnt;
	unsigned int fb_high_cnt_th;
	unsigned char sop_type;
	unsigned char comm_mode;//datalen[14], 0:parallel, 1:serial
	unsigned char dos_mode;
	unsigned char dimming_mode;
};

struct abcon_hy_ospb_s *bl_hy_ospb;

struct hy_ospb_pkt_s {
	unsigned int pream_header;
	unsigned int pream_data;

	unsigned int head_header;
	unsigned int head_data;

	unsigned int addr_header;
	unsigned int addr_data;

	unsigned int sub_addr_header;
	unsigned int sub_addr_data;

	unsigned int fb_header;
	unsigned int fb_data;

	unsigned int datalen_header;
	unsigned int datalen_data;

	unsigned int data_header;
	unsigned int data_data;

	unsigned int crc_header;
	unsigned int crc_data;
};

struct hy_ospb_pkt_s hy_ospb_pkt;

static void ldim_abcon_hy_ospb_write_pkt(void)
{
	unsigned int len = 0;
	unsigned int *buf;// = abcon_mem.wseg;
	unsigned char i = 0;
	unsigned char act_lane_num = abcon->act_lane;

	if (bl_hy_ospb->sop_type == sop_dimming)
		buf = abcon_mem.ldc_seg;
	else
		buf = abcon_mem.wseg;

	if (bl_hy_ospb->sop_type == sop_readback)
		act_lane_num = 1;

	if (!buf)
		return;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.pream_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.pream_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.head_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.head_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.addr_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.addr_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.sub_addr_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.sub_addr_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.fb_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.fb_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.datalen_header;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.datalen_data;

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.data_header;

	if (bl_hy_ospb->sop_type != sop_dimming) {
		for (i = 0; i < act_lane_num; i++)
			buf[len++] = hy_ospb_pkt.data_data;
	}

	for (i = 0; i < act_lane_num; i++)
		buf[len++] = hy_ospb_pkt.crc_header;

	//for (i = 0; i < act_lane_num; i++)
	//	buf[len++] = hy_ospb_pkt.crc_data;

	flush_dcache_range((unsigned long)buf,
		(unsigned long)(buf + len));
	//clr done flag
	ldim_abcon_swrst();

	if (bl_hy_ospb->sop_type == sop_dimming) {
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

static void ldim_abcon_hy_ospb_broadcast(unsigned int regaddr, unsigned int value, unsigned char comm_mode)
{
	unsigned int temp = 0;

	ABCONPR("broadcast reg:0x%x val:0x%08x, ln=%d, comm_mode=%d\n",
	regaddr, value, abcon->act_lane, comm_mode);
	bl_hy_ospb->sop_type = sop_broadcast;
	bl_hy_ospb->dos_mode = SINGLE;

	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
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

	hy_ospb_pkt.pream_header = 0x40002082;
	hy_ospb_pkt.pream_data = 0x5555;

	hy_ospb_pkt.head_header = 0x400020b0;
	hy_ospb_pkt.head_data = HEAD_BROADCAST |
		(bl_hy_ospb->dos_mode << 4) | (bl_hy_ospb->dimming_mode << 1);

	hy_ospb_pkt.addr_header = 0xc00020a0;
	hy_ospb_pkt.addr_data = 0xff00;//fix value

	hy_ospb_pkt.sub_addr_header = 0xc00020a0;
	hy_ospb_pkt.sub_addr_data = regaddr & 0xff;

	hy_ospb_pkt.fb_header = 0x400020a0;
	hy_ospb_pkt.fb_data = FB_VALUE;

	hy_ospb_pkt.datalen_header = 0xc00020a0;
	hy_ospb_pkt.datalen_data = comm_mode << 14;

	hy_ospb_pkt.data_header = 0x400020a0;
	hy_ospb_pkt.data_data = value;

	hy_ospb_pkt.crc_header = 0x4000208c;//hw_crc_en, without crc_data
	//hy_ospb_pkt.crc_data = 0x400020c4;

	ldim_abcon_hy_ospb_write_pkt();
	udelay(500);
}

static void ldim_abcon_hy_ospb_address(void)
{
	unsigned int temp = 0;

	ABCONPR("begin to send hyasic address!\n");
	bl_hy_ospb->sop_type = sop_address;
	bl_hy_ospb->comm_mode = COMM_SERIAL;//serial
	bl_hy_ospb->dos_mode = ADDR_CHECK;

	//clr done flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
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

	hy_ospb_pkt.pream_header = 0x40002082;
	hy_ospb_pkt.pream_data = 0x5555;

	hy_ospb_pkt.head_header = 0x400020b0;
	hy_ospb_pkt.head_data = HEAD_ADDRESS | (bl_hy_ospb->dos_mode << 4);

	hy_ospb_pkt.addr_header = 0xc00020a0;
	hy_ospb_pkt.addr_data = (abcon->max_lane_dim) << 8 | 0x01;

	hy_ospb_pkt.sub_addr_header = 0xc00020a0;
	hy_ospb_pkt.sub_addr_data = 0x0;

	hy_ospb_pkt.fb_header = 0x400020a0;
	hy_ospb_pkt.fb_data = FB_VALUE;

	hy_ospb_pkt.datalen_header = 0xc00020a0;
	hy_ospb_pkt.datalen_data = bl_hy_ospb->comm_mode << 14;

	hy_ospb_pkt.data_header = 0x400020a0;
	hy_ospb_pkt.data_data = 0x0;

	hy_ospb_pkt.crc_header = 0x4000208c;//hw_crc_en, without crc_data
	//hy_ospb_pkt.crc_data = 0x400020c4;

	ldim_abcon_hy_ospb_write_pkt();
	udelay(500);
}

static void ldim_abcon_hy_ospb_dimming(void)
{
	unsigned int temp = 0;

	ABCONPR("begin dimming data packet\n");
	bl_hy_ospb->sop_type = sop_dimming;
	bl_hy_ospb->comm_mode = COMM_PARALLEL;
	bl_hy_ospb->dos_mode = CH_SICK_INT1;

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
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

	hy_ospb_pkt.pream_header = 0x40002082;
	hy_ospb_pkt.pream_data = 0x5555;

	hy_ospb_pkt.head_header = 0x400020b0;
	hy_ospb_pkt.head_data = HEAD_DIMMING |
		(bl_hy_ospb->dos_mode << 4) | (bl_hy_ospb->dimming_mode << 1);

	hy_ospb_pkt.addr_header = 0xc00020a0;
	hy_ospb_pkt.addr_data = (abcon->max_lane_dim - 1) << 8 | 0x01;

	hy_ospb_pkt.sub_addr_header = 0xc00020a0;
	hy_ospb_pkt.sub_addr_data = 0x0;

	hy_ospb_pkt.fb_header = 0x400020a0;
	hy_ospb_pkt.fb_data = FB_VALUE;

	hy_ospb_pkt.datalen_header = 0xc00020a0;
	hy_ospb_pkt.datalen_data =(abcon->max_lane_ch - 1) | (bl_hy_ospb->comm_mode << 14);

	hy_ospb_pkt.data_header = 0x10a1 | (abcon->max_lane_ch << 14);
	//hy_ospb_pkt.data_data = 0x0;

	hy_ospb_pkt.crc_header = 0x4000208c;//hw_crc_en, without crc_data
	//hy_ospb_pkt.crc_data = 0x400020c4;


	ldim_abcon_hy_ospb_write_pkt();
}


static void ldim_abcon_hy_ospb_read_pkt(void)
{
	unsigned int len = 0;
	unsigned int *buf = abcon_mem.rseg;

	if (!buf)
		return;

	buf[len++] = hy_ospb_pkt.pream_header;
	buf[len++] = hy_ospb_pkt.pream_data;
	buf[len++] = hy_ospb_pkt.head_header;
	buf[len++] = hy_ospb_pkt.head_data;
	buf[len++] = hy_ospb_pkt.addr_header;
	buf[len++] = hy_ospb_pkt.addr_data;
	buf[len++] = hy_ospb_pkt.sub_addr_header;
	buf[len++] = hy_ospb_pkt.sub_addr_data;
	buf[len++] = hy_ospb_pkt.fb_header;
	buf[len++] = hy_ospb_pkt.fb_data;
	buf[len++] = hy_ospb_pkt.datalen_header;
	buf[len++] = hy_ospb_pkt.datalen_data;
	buf[len++] = hy_ospb_pkt.data_header;
	buf[len++] = hy_ospb_pkt.data_data;
	buf[len++] = hy_ospb_pkt.crc_header;

	abcon_wr_reg_bits(0x05, len, 16, 13);//reg_abcon_sw_rseg_max_num
}

unsigned int ldim_abcon_hy_ospb_readback(unsigned char reg,
	unsigned char chip, unsigned char lane)
{
	unsigned int readback_val = 0;

	bl_hy_ospb->sop_type = sop_readback;
	bl_hy_ospb->comm_mode = COMM_SERIAL;
	bl_hy_ospb->dos_mode = SINGLE;

	//reg_abcon_crc_value
	abcon_wr_reg_bits(0x10, 0x5559, 16, 16);

	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, 1, 10, 4);// fix 1 lane

	//reg_abcon_act_lane
	abcon_wr_reg_bits(0x2a, 1 << lane, 22, 4);//fix use lane 0

	//prepare read packet first
	hy_ospb_pkt.pream_header = 0x00002104;
	hy_ospb_pkt.pream_data = 0x5555;

	hy_ospb_pkt.head_header = 0x00002148;
	hy_ospb_pkt.head_data = HEAD_READBACK | (bl_hy_ospb->dos_mode << 4);

	hy_ospb_pkt.addr_header = 0x04002148;
	hy_ospb_pkt.addr_data = chip & 0xff;

	hy_ospb_pkt.sub_addr_header = 0x04002148;
	hy_ospb_pkt.sub_addr_data = reg & 0xff;

	hy_ospb_pkt.fb_header = 0x00002148;
	hy_ospb_pkt.fb_data = FB_VALUE;

	hy_ospb_pkt.datalen_header = 0x04002148;
	hy_ospb_pkt.datalen_data = bl_hy_ospb->comm_mode << 14;

	hy_ospb_pkt.data_header = 0x00002148;
	hy_ospb_pkt.data_data = 0x0;

	hy_ospb_pkt.crc_header = 0x00002150;//hw_crc_en, without crc_data
	//hy_ospb_pkt.crc_data = 0x400020c4;
	ldim_abcon_hy_ospb_read_pkt();

	//then set write packet
	hy_ospb_pkt.pream_header = 0x40002082;
	hy_ospb_pkt.pream_data = 0x5555;

	hy_ospb_pkt.head_header = 0x400020b0;
	hy_ospb_pkt.head_data = HEAD_READBACK | (bl_hy_ospb->dos_mode << 4);

	hy_ospb_pkt.addr_header = 0xc00020a0;
	hy_ospb_pkt.addr_data = chip & 0xff;

	hy_ospb_pkt.sub_addr_header = 0xc00020a0;
	hy_ospb_pkt.sub_addr_data = reg & 0xff;

	hy_ospb_pkt.fb_header = 0x400020a0;
	hy_ospb_pkt.fb_data = FB_VALUE;

	hy_ospb_pkt.datalen_header = 0xc00020a0;
	hy_ospb_pkt.datalen_data = bl_hy_ospb->comm_mode << 14;

	hy_ospb_pkt.data_header = 0x400020a0;
	hy_ospb_pkt.data_data = 0x0;

	hy_ospb_pkt.crc_header = 0x4000208c;//hw_crc_en, without crc_data
	//hy_ospb_pkt.crc_data = 0x400020c4;

	ldim_abcon_hy_ospb_write_pkt();
	udelay(1000);
	readback_val = abcon_rd_reg(0x52);
	ABCONPR("readback: reg=0x%x, chip=%d, lane=%d, val=0x%x\n",
	reg, chip, lane, readback_val);

	return readback_val;
}

static void ldim_abcon_hy_ospb_init_feedback(void)
{
	bl_hy_ospb->sop_type = sop_feedback;
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
	abcon_wr_reg_bits(0x39, bl_hy_ospb->fb_cnt, 16, 16);
}

static void ldim_abcon_init_registers(void)
{
	//reg_abcon_clr_done_flag
	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	//reg_abcon_wire_mode
	abcon_wr_reg_bits(0x22, 0, 30, 2);
	//reg_frc_crc_val_en
	abcon_wr_reg_bits(0x03, 0, 0, 1);//disable
	//reg_abcon_rx_scale_sel
	abcon_wr_reg_bits(0x03, 1, 1, 1);
	//reg_abcon_hw_crc
	abcon_wr_reg_bits(0x03, 1, 3, 1);
	//reg_abcon_dual_line_mode
	abcon_wr_reg_bits(0x03, bl_hy_ospb->dual_wire, 4, 1);
	//reg_abcon_dix_mode
	abcon_wr_reg_bits(0x03, 0, 5, 1);/*0:dis, 1:dip*/
	//reg_abcon_default_value
	abcon_wr_reg_bits(0x03, bl_hy_ospb->default_value, 8, 1);
	//reg_abcon_rx_preamble_num
	abcon_wr_reg_bits(0x03, 16, 24, 8);
	//reg_abcon_crc_poly
	abcon_wr_reg(0x0d, 0x00008005);
	//reg_abcon_crc_init
	abcon_wr_reg(0x0e, 0xffff);
	//reg_abcon_crc_xorout
	abcon_wr_reg(0x0f, 0);

	abcon_wr_reg(0x11, 0x70f081ff);

	abcon_wr_reg(0x1a, 0x7f000000);
	abcon_wr_reg(0x1b, 0x20c32940);//6dc+10pwm

	//init feedback registers
	ldim_abcon_hy_ospb_init_feedback();
}

void ldim_abcon_init_hy_ospb(void)
{
	char i = 0, loop_cnt = 1;

	//unlock
	for (i = 0; i < loop_cnt; i++) {
		ldim_abcon_hy_ospb_broadcast(0x4f, 2 << 13, COMM_PARALLEL);
		ldim_abcon_hy_ospb_broadcast(0x78, UNLOCK_KEY, COMM_PARALLEL);
		ldim_abcon_hy_ospb_broadcast(0x7c, UNLOCK_KEY, COMM_SERIAL);
	}

	//send address command
	for (i = 0; i < loop_cnt; i++)
		ldim_abcon_hy_ospb_address();

	for (i = 0; i < loop_cnt; i++) {
		ldim_abcon_hy_ospb_broadcast(0x41, 0x0004, COMM_PARALLEL); // tp as 10bit
		ldim_abcon_hy_ospb_broadcast(0x42, 0x2000, COMM_PARALLEL); // DOS rate fix 200K
		ldim_abcon_hy_ospb_broadcast(0x48, 0x8a34, COMM_PARALLEL); // burst 80mA debounce set 32
		ldim_abcon_hy_ospb_broadcast(0x4a, 0x0011, COMM_PARALLEL); // full range 60mA, SR:00
		ldim_abcon_hy_ospb_broadcast(0x4c, 0x0c28, COMM_PARALLEL); //	blanking time: 3*4 + 3
		ldim_abcon_hy_ospb_broadcast(0x4e, 0x9f01, COMM_PARALLEL); // 1:PWM 20.48k
		ldim_abcon_hy_ospb_broadcast(0x47, 0x0007, COMM_PARALLEL); // Headroom voltage set 650mV
	}

	for (i = 0; i < loop_cnt; i++) {
		ldim_abcon_hy_ospb_broadcast(0x40, 0x00ff, COMM_PARALLEL); // all IC channel enable
		ldim_abcon_hy_ospb_broadcast(0x46, 0x8842, COMM_PARALLEL); //
		ldim_abcon_hy_ospb_broadcast(0x60, 0x003f, COMM_PARALLEL); // fault clear
	}

	for (i = 0; i < loop_cnt; i++) {
		ldim_abcon_hy_ospb_broadcast(0x78, UNLOCK_KEY, COMM_PARALLEL);
		ldim_abcon_hy_ospb_broadcast(0x4E, 0x8d01, COMM_PARALLEL);
		ldim_abcon_hy_ospb_broadcast(0x46, 0x8040, COMM_PARALLEL);
		ldim_abcon_hy_ospb_broadcast(0x78, LOCK_KEY, COMM_PARALLEL);
	}

	ldim_abcon_hy_ospb_broadcast(0x2f, 0x0000, COMM_PARALLEL);
}

static int abcon_hy_ospb_hw_init_on(struct ldim_dev_driver_s *dev_drv)
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

	//init abcon register for hyasic ospb
	ldim_abcon_init_registers();

	//init hyasic registers
	ldim_abcon_init_hy_ospb();

	return 0;
}

static int abcon_hy_ospb_hw_init_off(struct ldim_dev_driver_s *dev_drv)
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
	ldim_abcon_hy_ospb_dimming();

	//sw reset
	ldim_abcon_swrst();

	return 0;
}

static int abcon_hy_ospb_smr(struct aml_ldim_driver_s *ldim_drv, unsigned short *buf, unsigned int len)
{
	int ret = 0;

	if (!bl_hy_ospb)
		return -1;

	if (bl_hy_ospb->dev_on_flag == 0) {
		ABCONPR("%s: on_flag=%d\n", __func__, bl_hy_ospb->dev_on_flag);
		return 0;
	}

	ldim_abcon_swduty_set(ldim_drv);

	//reg_abcon_duty_zone_src_sel
	abcon_wr_reg_bits(0x2c, 0, 29, 1);//sw control id

	abcon_wr_reg_bits(0x00, 1, 4, 1);
	abcon_wr_reg_bits(0x00, 0, 4, 1);

	ldim_abcon_hy_ospb_dimming();

	return ret;
}

static int abcon_hy_ospb_power_on(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_hy_ospb)
		return -1;
	if (bl_hy_ospb->dev_on_flag) {
		ABCONPR("%s: abcon_hyasic is already on, exit\n", __func__);
		return 0;
	}

	abcon_hy_ospb_hw_init_on(ldim_drv->dev_drv);
	bl_hy_ospb->dev_on_flag = 1;
	bl_hy_ospb->vsync_cnt = 0;

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_hy_ospb_power_off(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_hy_ospb)
		return -1;

	bl_hy_ospb->dev_on_flag = 0;
	abcon_hy_ospb_hw_init_off(ldim_drv->dev_drv);

	ABCONPR("%s: ok\n", __func__);
	return 0;
}

static int abcon_hy_ospb_ldim_dev_update(struct ldim_dev_driver_s *dev_drv)
{

	if (!dev_drv) {
		LDIMERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	dev_drv->power_on = abcon_hy_ospb_power_on;
	dev_drv->power_off = abcon_hy_ospb_power_off;
	dev_drv->dev_smr = abcon_hy_ospb_smr;

	dev_drv->reg_write = NULL;
	dev_drv->reg_read = NULL;
	return 0;
}

int ldim_dev_abcon_hy_ospb_probe(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;

	if (!dev_drv) {
		ABCONERR("%s: dev_drv is null\n", __func__);
		return -1;
	}

	bl_hy_ospb = (struct abcon_hy_ospb_s *)malloc(sizeof(struct abcon_hy_ospb_s));
	if (!bl_hy_ospb) {
		ABCONERR("%s: bl_hy_ospb is error\n", __func__);
		return -1;
	}

	bl_hy_ospb->dev_on_flag = 0;
	bl_hy_ospb->vsync_cnt = 0;

	bl_hy_ospb->fb_cnt = 10;
	bl_hy_ospb->fb_high_cnt_th = 8;

	bl_hy_ospb->dual_wire = abcon->conf.ctrl & 0x1;
	bl_hy_ospb->default_value = (abcon->conf.ctrl >> 1) & 0x1;
	bl_hy_ospb->dimming_mode = abcon->conf.dimming_mode;
	ABCONPR("ctrl=0x%x, dual_wire=%d, default_value=%d, dimming_mode=%d\n",
	abcon->conf.ctrl, bl_hy_ospb->dual_wire, bl_hy_ospb->default_value, bl_hy_ospb->dimming_mode);

	//init abcon register for hyasic
	ldim_abcon_init_registers();

	if (bl_hy_ospb->bl_pwm_en) {
		/* Generate external VSYNC to VSYNC/PWM pin */
		ldim_set_duty_pwm(&dev_drv->ldim_pwm_config);
		ldim_set_duty_pwm(&dev_drv->analog_pwm_config);
		dev_drv->pinmux_ctrl(dev_drv, 1);
	}

	abcon_hy_ospb_ldim_dev_update(dev_drv);

	ABCONPR("%s ok\n", __func__);
	return 0;
}

int ldim_dev_abcon_hy_ospb_remove(struct aml_ldim_driver_s *ldim_drv)
{
	if (!bl_hy_ospb)
		return 0;

	free(bl_hy_ospb);
	bl_hy_ospb = NULL;

	return 0;
}
