// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_extern.h>
#include "../lcd_extern.h"
#include "../../lcd_common.h"

#define MIPI_DSI_LANE  4
#define eDP_LANE       2

static inline void i2c_write_reg(struct lcd_extern_driver_s *edrv, u8 reg, u8 val)
{
	unsigned char _data[2] = {reg, val};

	aml_lcd_i2c_write(edrv->i2c_bus, edrv->dev[0]->i2c_addr[0], _data, 2);
}

static inline u8 i2c_read_reg(struct lcd_extern_driver_s *edrv, u8 reg)
{
	unsigned char _data = reg;

	aml_lcd_i2c_read(edrv->i2c_bus, edrv->dev[0]->i2c_addr[0], &_data, 1);

	return _data;
}

static void get_LT8911_chip_ID(struct lcd_extern_driver_s *edrv)
{
	unsigned char _data[3] = {0x00, 0x00, 0x00};

	i2c_write_reg(edrv, 0xff, 0x81);
	i2c_write_reg(edrv, 0x08, 0x7f);

	_data[0] = i2c_read_reg(edrv, 0x00);
	_data[1] = i2c_read_reg(edrv, 0x01);
	_data[2] = i2c_read_reg(edrv, 0x02);

	printf("%s: 0x%02x, 0x%02x, 0x%02x\n", __func__, _data[0], _data[1], _data[2]);
}

void LT8911_read_edid(struct lcd_extern_driver_s *edrv)
{
	u8 reg, i, j;
	u8 EDID_DATA[128];

	i2c_write_reg(edrv, 0xff, 0xac);
	i2c_write_reg(edrv, 0x00, 0x20); //Soft Link train
	i2c_write_reg(edrv, 0xff, 0xa6);
	i2c_write_reg(edrv, 0x2a, 0x01);

	/*set edid offset addr*/
	i2c_write_reg(edrv, 0x2b, 0x40); //CMD
	i2c_write_reg(edrv, 0x2b, 0x00); //addr[15:8]
	i2c_write_reg(edrv, 0x2b, 0x50); //addr[7:0]
	i2c_write_reg(edrv, 0x2b, 0x00); //data length
	i2c_write_reg(edrv, 0x2b, 0x00); //data length
	i2c_write_reg(edrv, 0x2c, 0x00); //start Aux read edid

	mdelay(10); //more than 10ms
	reg = (u8)i2c_read_reg(edrv, 0x25);
	printf("reg = %02x\n", reg);
	if ((reg & 0x0f) == 0x0c) {
		for (j = 0; j < 8; j++) {
			i2c_write_reg(edrv, 0x2b, j == 7 ? 0x10 : 0x50); //MOT

			i2c_write_reg(edrv, 0x2b, 0x00);
			i2c_write_reg(edrv, 0x2b, 0x50);
			i2c_write_reg(edrv, 0x2b, 0x0f);
			i2c_write_reg(edrv, 0x2c, 0x00); //start Aux read edid
			mdelay(10); //by tx 50 //more than 50ms
			if ((u8)i2c_read_reg(edrv, 0x39) == 0x31) {
				i2c_read_reg(edrv, 0x2b);
				for (i = 0; i < 16; i++)
					EDID_DATA[j * 16 + i] = (u8)i2c_read_reg(edrv, 0x2b);
			} else {
				printf("Read edid error: no_reply\n");
				return;
			}
		}
		for (i = 0; i < 128; i++)
			printf("0x%2x %s", EDID_DATA[i], ((i % 16) == 0) ? "\n" : "");

	} else if ((reg & 0x0f) == 0x0a) {
		printf("Read edid error: reply_nack\n");
	} else if ((reg & 0x0f) == 0x09) {
		printf("Read edid error: reply_nack\n");
	} else {
		printf("Read edid error: no_reply\n");
	}
}

/* mipi should be ready before configuring below video check setting*/
void LT8911_video_check(struct lcd_extern_driver_s *edrv)
{
	u32 reg = 0x00;
	u16 h_act, v_act;

	/* mipi byte clk check*/
	i2c_write_reg(edrv, 0xff, 0x85);
	i2c_write_reg(edrv, 0x1d, 0x00);   //FM select byte clk
	i2c_write_reg(edrv, 0x40, 0xf7);
	i2c_write_reg(edrv, 0x41, 0x30);
	i2c_write_reg(edrv, 0xa1, 0x82); // eDP scramble mode;//video from mipi
	// i2c_write_reg(edrv, 0x17, 0xf0 ); // 0xf0:Close scramble; 0xD0 : Open scramble
	i2c_write_reg(edrv, 0xff, 0x81);   //video check rst
	i2c_write_reg(edrv, 0x09, 0x7d);
	i2c_write_reg(edrv, 0x09, 0xfd);

	i2c_write_reg(edrv, 0xff, 0x85);
	mdelay(30);

	if ((u8)i2c_read_reg(edrv, 0x50) == 0x03) {
		reg = (u8)i2c_read_reg(edrv, 0x4d);
		reg = reg * 256 + (u8)i2c_read_reg(edrv, 0x4e);
		reg = reg * 256 + (u8)i2c_read_reg(edrv, 0x4f);
		printf("%s: mipi clk = %u\n", __func__, reg);
	} else {
		printf("%s: mipi clk unstable\n", __func__);
	}

	/* mipi vtotal check*/
	reg = (u8)i2c_read_reg(edrv, 0x76);
	reg = reg * 256 + (u8)i2c_read_reg(edrv, 0x77);
	printf("%s: Vtotal = %u\n", __func__, reg);

	/* mipi word count check*/
	i2c_write_reg(edrv, 0xff, 0xd0);
	reg = (u8)i2c_read_reg(edrv, 0x82);
	reg = reg * 256 + (u8)i2c_read_reg(edrv, 0x83);
	h_act = reg / 3;
	printf("%s: Hactive = %u\n", __func__, h_act);

	/* mipi Vact check*/
	reg = (u8)i2c_read_reg(edrv, 0x85);
	v_act = reg * 256 + (u8)i2c_read_reg(edrv, 0x86);
	printf("%s: Vactive = %u\n", __func__, v_act);

	//if ((lcd_diff(act_timing->h_active, h_act) < 5) &&
	//    (lcd_diff(act_timing->v_active, v_act) < 5)) {
	//	printf("%s: Vactive = %u\n", __func__, v_act);
	//} else {
	//	printf("%s: Vactive = %u\n", __func__, v_act)
	//}
}

void LT8911_link_train_result(struct lcd_extern_driver_s *edrv)
{
	u8 i, reg;

	i2c_write_reg(edrv, 0xff, 0xac);
	for (i = 0; i < 10; i++) {
		reg = (u8)i2c_read_reg(edrv, 0x82);
		if (reg & 0x20) {
			if ((reg & 0x1f) == 0x1e)
				printf("Link train success, 0x82 = 0x%02hx\n", reg);
			else
				printf("Link train fail, 0x82 = 0x%02hx\n", reg);

			printf("panel link rate: 0x%02x\n", i2c_read_reg(edrv, 0x83));
			printf("panel link count: %u\n", i2c_read_reg(edrv, 0x84));
			return;
		}
		mdelay(100);
	}
}

void LT8911_MIPI_Video_Timing(struct lcd_extern_driver_s *edrv)
{
	struct aml_lcd_drv_s *pdrv = aml_lcd_get_driver(edrv->index);
	struct lcd_detail_timing_s *act_timing = &pdrv->config.timing.act_timing;

	i2c_write_reg(edrv, 0xff, 0xd0);
	i2c_write_reg(edrv, 0x0d, (u8)(act_timing->v_period / 256));
	i2c_write_reg(edrv, 0x0e, (u8)(act_timing->v_period % 256)); //vtotal
	i2c_write_reg(edrv, 0x0f, (u8)(act_timing->v_active / 256));
	i2c_write_reg(edrv, 0x10, (u8)(act_timing->v_active % 256)); //vactive
	i2c_write_reg(edrv, 0x11, (u8)(act_timing->h_period / 256));
	i2c_write_reg(edrv, 0x12, (u8)(act_timing->h_period % 256)); //htotal
	i2c_write_reg(edrv, 0x13, (u8)(act_timing->h_active / 256));
	i2c_write_reg(edrv, 0x14, (u8)(act_timing->h_active % 256)); //hactive
	i2c_write_reg(edrv, 0x15, (u8)(act_timing->vsync_width % 256)); //vsa
	i2c_write_reg(edrv, 0x16, (u8)(act_timing->hsync_width % 256)); //hsa
	i2c_write_reg(edrv, 0x17, (u8)(act_timing->vsync_fp / 256));
	i2c_write_reg(edrv, 0x18, (u8)(act_timing->vsync_fp % 256)); //vfp
	i2c_write_reg(edrv, 0x19, (u8)(act_timing->hsync_fp / 256));
	i2c_write_reg(edrv, 0x1a, (u8)(act_timing->hsync_fp % 256)); //hfp
}

void LT8911_eDP_Video_Timing(struct lcd_extern_driver_s *edrv)
{
	struct aml_lcd_drv_s *pdrv = aml_lcd_get_driver(edrv->index);
	struct lcd_detail_timing_s *act_timing = &pdrv->config.timing.act_timing;

	i2c_write_reg(edrv, 0xff, 0xa8);
	i2c_write_reg(edrv, 0x2d, 0x88); // MSA from register
	i2c_write_reg(edrv, 0x05, (u8)(act_timing->h_period / 256));
	i2c_write_reg(edrv, 0x06, (u8)(act_timing->h_period % 256)); //htotal
	i2c_write_reg(edrv, 0x07,
		      (u8)((act_timing->hsync_width + act_timing->hsync_bp) / 256));
	i2c_write_reg(edrv, 0x08,
		      (u8)((act_timing->hsync_width + act_timing->hsync_bp) % 256)); //h_start
	i2c_write_reg(edrv, 0x09, (u8)(act_timing->hsync_width / 256));
	i2c_write_reg(edrv, 0x0a, (u8)(act_timing->hsync_width % 256)); //hsa
	i2c_write_reg(edrv, 0x0b, (u8)(act_timing->h_active / 256));
	i2c_write_reg(edrv, 0x0c, (u8)(act_timing->h_active % 256)); //hactive
	i2c_write_reg(edrv, 0x0d, (u8)(act_timing->v_period / 256));
	i2c_write_reg(edrv, 0x0e, (u8)(act_timing->v_period % 256)); //vtotal
	i2c_write_reg(edrv, 0x11,
		      (u8)((act_timing->vsync_width + act_timing->vsync_bp) / 256));
	i2c_write_reg(edrv, 0x12,
		      (u8)((act_timing->vsync_width + act_timing->vsync_bp) % 256)); //v_start
	i2c_write_reg(edrv, 0x14, (u8)(act_timing->vsync_width % 256)); //vsa
	i2c_write_reg(edrv, 0x15, (u8)(act_timing->v_active / 256));
	i2c_write_reg(edrv, 0x16, (u8)(act_timing->v_active % 256)); //vactive
}

void LT8911_init(struct lcd_extern_driver_s *edrv)
{
	u8 i;
	u8 pcr_pll_postdiv;
	u8 pcr_m;

	struct aml_lcd_drv_s *pdrv = aml_lcd_get_driver(edrv->index);
	struct lcd_detail_timing_s *act_timing = &pdrv->config.timing.act_timing;

	i2c_write_reg(edrv, 0xff, 0x81);
	i2c_write_reg(edrv, 0x49, 0xff); //enable clock

	i2c_write_reg(edrv, 0xff, 0x82); //GPIO test output
	i2c_write_reg(edrv, 0x5a, 0x0e);

	/* mipi Rx analog */
	i2c_write_reg(edrv, 0xff, 0x82);
	i2c_write_reg(edrv, 0x32, 0x51);
	i2c_write_reg(edrv, 0x35, 0x22); // EQ current
	i2c_write_reg(edrv, 0x4c, 0x0c);
	i2c_write_reg(edrv, 0x4d, 0x00);

	i2c_write_reg(edrv, 0x3a, 0x77); // 11 // 22 / 33 / 55 / 66 / 77; default 00
	i2c_write_reg(edrv, 0x3b, 0x77); // 11 // 22 / 33 / 55 / 66 / 77; default 00

	/* de ssc_pcr  pll analog */
	i2c_write_reg(edrv, 0xff, 0x82);
	i2c_write_reg(edrv, 0x6a, 0x40); //final setting: 0x40
	i2c_write_reg(edrv, 0x6b, 0x40); //0x40:pre-div = 1,

	if (act_timing->pixel_clk < 88000000) {
		i2c_write_reg(edrv, 0x6e, 0x82); //0x44:pre-div = 2 ,pixel_clk=44~ 88MHz
		pcr_pll_postdiv = 0x08;
	} else {
		i2c_write_reg(edrv, 0x6e, 0x81); //0x40:pre-div = 1, pixel_clk =88~176MHz
		pcr_pll_postdiv = 0x04;
	}

	pcr_m = (u8)(((act_timing->pixel_clk / 10000) * pcr_pll_postdiv) / 25 / 100);

	/* de ssc pll digital */
	i2c_write_reg(edrv, 0xff, 0x85);
	i2c_write_reg(edrv, 0xa9, 0x31);
	i2c_write_reg(edrv, 0xaa, 0x17);
	i2c_write_reg(edrv, 0xab, 0xba);
	i2c_write_reg(edrv, 0xac, 0xe1);
	i2c_write_reg(edrv, 0xad, 0x47);
	i2c_write_reg(edrv, 0xae, 0x01);
	i2c_write_reg(edrv, 0xae, 0x11);

	/* Digital Top */
	i2c_write_reg(edrv, 0xff, 0x85);
	i2c_write_reg(edrv, 0xc0, 0x01); //select mipi Rx

	// i2c_write_reg(edrv, 0xb0, 0xd0 ); // 8bit to 6 bit enable dither
	i2c_write_reg(edrv, 0xb0, 0x00); // disable dither

	/* mipi Rx Digital */
	i2c_write_reg(edrv, 0xff, 0xd0);
	i2c_write_reg(edrv, 0x00, MIPI_DSI_LANE % 4); // 0=4 Lane;/1/2/3 lane
	i2c_write_reg(edrv, 0x02, 0x08);
	i2c_write_reg(edrv, 0x08, 0x00);
	i2c_write_reg(edrv, 0x0a, 0x12); //pcr mode
	i2c_write_reg(edrv, 0x0c, 0x40); //vsync_wr_dly
	// lt8911exb_write(pdata, 0x24, 0x71); //pcr mode()
	i2c_write_reg(edrv, 0x1c, 0x3a);
	// i2c_write_reg(edrv, 0x2d,0x19); //M up limit
	i2c_write_reg(edrv, 0x31, 0x0a); //M down limit

	i2c_write_reg(edrv, 0x21, 0x4f);
	i2c_write_reg(edrv, 0x22, 0xff);
	i2c_write_reg(edrv, 0x2a, 0x08);

	i2c_write_reg(edrv, 0x3f, 0x10);
	i2c_write_reg(edrv, 0x40, 0x20);
	i2c_write_reg(edrv, 0x41, 0x30);

	// i2c_write_reg(edrv, 0x26, ( pcr_m | 0x80)); //test Pattern color
	i2c_write_reg(edrv, 0x26, pcr_m);

	i2c_write_reg(edrv, 0xff, 0x81); //PCR reset
	i2c_write_reg(edrv, 0x03, 0x7b);
	i2c_write_reg(edrv, 0x03, 0xff);

	/* Txpll 2.7G*/
	i2c_write_reg(edrv, 0xff, 0x87);
	i2c_write_reg(edrv, 0x19, 0x31);
	i2c_write_reg(edrv, 0xff, 0x82);
	i2c_write_reg(edrv, 0x02, 0x42);
	i2c_write_reg(edrv, 0x03, 0x00);
	i2c_write_reg(edrv, 0x03, 0x01);
	i2c_write_reg(edrv, 0xff, 0x81);
	i2c_write_reg(edrv, 0x09, 0xfc);
	i2c_write_reg(edrv, 0x09, 0xfd);
	i2c_write_reg(edrv, 0xff, 0x87);
	i2c_write_reg(edrv, 0x0c, 0x11);

	for (i = 0; i < 5; i++) { //Check Tx PLL
		mdelay(5);
		if ((u8)i2c_read_reg(edrv, 0x37) & 0x02) {
			printf("LT8911EXB tx pll locked\n");
			break;
		}
		printf("LT8911EXB tx pll unlocked\n");
		i2c_write_reg(edrv, 0xff, 0x81);
		i2c_write_reg(edrv, 0x09, 0xfc);
		i2c_write_reg(edrv, 0x09, 0xfd);
		i2c_write_reg(edrv, 0xff, 0x87);
		i2c_write_reg(edrv, 0x0c, 0x10);
		i2c_write_reg(edrv, 0x0c, 0x11);
	}
	/* tx phy */
	i2c_write_reg(edrv, 0xff, 0x82);
	i2c_write_reg(edrv, 0x11, 0x00);
	i2c_write_reg(edrv, 0x13, 0x10);
	i2c_write_reg(edrv, 0x14, 0x0c);
	i2c_write_reg(edrv, 0x14, 0x08);
	i2c_write_reg(edrv, 0x13, 0x20);
	i2c_write_reg(edrv, 0xff, 0x82);
	i2c_write_reg(edrv, 0x0e, 0x25);
	i2c_write_reg(edrv, 0x12, 0xff);
	// i2c_write_reg(edrv, 0xff, 0x80);
	// i2c_write_reg(edrv, 0x40, 0x22);

	/*eDP Tx Digital */
	i2c_write_reg(edrv, 0xff, 0xa8);

	// i2c_write_reg(edrv, 0x24, 0x50); // bit2 ~ bit 0 : test panttern image mode
	// i2c_write_reg(edrv, 0x25, 0x70); // bit6 ~ bit 4 : test Pattern color
	// i2c_write_reg(edrv, 0x27, 0x50); //0x50:Pattern; 0x10:mipi video
	i2c_write_reg(edrv, 0x27, 0x10); //0x50:Pattern; 0x10:mipi video

	// 6bit_
	// i2c_write_reg(edrv, 0x17, 0x00);
	// i2c_write_reg(edrv, 0x18, 0x00);
	i2c_write_reg(edrv, 0x17, 0x10);
	i2c_write_reg(edrv, 0x18, 0x20);

	i2c_write_reg(edrv, 0xff, 0xa0);
	i2c_write_reg(edrv, 0x00, 0x08);
	i2c_write_reg(edrv, 0x01, 0x00);
}

void LT8911_DPCD_write(struct lcd_extern_driver_s *edrv, u32 Address, u8 Data)
{
	u8 reg;
	u8 addr_H = 0x0f & (Address >> 16);
	u8 addr_M = 0xff & (Address >> 8);
	u8 addr_L = 0xff &  Address;

	i2c_write_reg(edrv, 0xff, 0xa6);
	i2c_write_reg(edrv, 0x2b, (0x80 | addr_H)); //CMD
	i2c_write_reg(edrv, 0x2b, addr_M); //addr[15:8]
	i2c_write_reg(edrv, 0x2b, addr_L); //addr[7:0]
	i2c_write_reg(edrv, 0x2b, 0x00); //data length
	i2c_write_reg(edrv, 0x2b, Data); //data
	i2c_write_reg(edrv, 0x2c, 0x00); //start Aux read edid

	mdelay(20); //more than 10ms
	reg = (u8)i2c_read_reg(edrv, 0x25);

	if ((reg & 0x0f) == 0x0c)
		return;

	//else if ((reg & 0x0f) == 0x0a)
	//	goto reply_nack;
	//else if ((reg & 0x0f) == 0x09)
	//	goto reply_defer;
	//else
	//	goto no_reply;
}

//u8 LT8911_DPCD_read(u32 Address)
//{
//	u8 val;
//	u8 DPCD_val = 0x00;
//	u8 addr_H = 0x0f & (Address >> 16);
//	u8 addr_M = 0xff & (Address >> 8);
//	u8 addr_L = 0xff &  Address    ;
//	u8 reg;
//
//	i2c_write_reg(edrv, 0xff, 0xa6);
//	i2c_write_reg(edrv, 0x2b, (0x80 | addr_H)); //CMD
//	i2c_write_reg(edrv, 0x2b, addr_M); //addr[15:8]
//	i2c_write_reg(edrv, 0x2b, addr_L); //addr[7:0]
//	i2c_write_reg(edrv, 0x2b, 0x00); //data length
//	i2c_write_reg(edrv, 0x2b, Data); //data
//	i2c_write_reg(edrv, 0x2c, 0x00); //start Aux read edid
//
//	mdelay( 20 ); //more than 10ms
//	reg = (u8)HDMI_ReadI2C_Byte( 0x25,&val );
//
//	if ((reg & 0x0f) == 0x0c)
//		return;
//
//	else if ((reg & 0x0f) == 0x0a)
//		goto reply_nack;
//	else if ((reg & 0x0f) == 0x09)
//		goto reply_defer;
//	else
//		goto no_reply;
//}

void LT8911_link_training(struct lcd_extern_driver_s *edrv)
{
	i2c_write_reg(edrv, 0xff, 0x85);
	i2c_write_reg(edrv, 0xa1, 0x82); // eDP scramble mode;//video from mipi

	// i2c_write_reg(edrv, 0x17, 0xf0); // 0xf0:Close scramble; 0xD0 : Open scramble
	i2c_write_reg(edrv, 0x17, 0xD0); // 0xf0:Close scramble; 0xD0 : Open scramble

	/* Aux operater init */
	i2c_write_reg(edrv, 0xff, 0xac);
	i2c_write_reg(edrv, 0x00, 0x20);	//Soft Link train
	i2c_write_reg(edrv, 0xff, 0xa6);
	i2c_write_reg(edrv, 0x2a, 0x01);

	LT8911_DPCD_write(edrv, 0x010a, 0x01);
	mdelay(10);
	LT8911_DPCD_write(edrv, 0x0102, 0x00);
	mdelay(10);
	LT8911_DPCD_write(edrv, 0x010a, 0x01);
	mdelay(200);

	/* Aux setup */
	i2c_write_reg(edrv, 0xff, 0xac);
	i2c_write_reg(edrv, 0x00, 0x60); //Soft Link train
	i2c_write_reg(edrv, 0xff, 0xa6);
	i2c_write_reg(edrv, 0x2a, 0x00);

	i2c_write_reg(edrv, 0xff, 0x81);
	i2c_write_reg(edrv, 0x07, 0xfe);
	i2c_write_reg(edrv, 0x07, 0xff);
	i2c_write_reg(edrv, 0x0a, 0xfc);
	i2c_write_reg(edrv, 0x0a, 0xfe);

	/* link train */
	i2c_write_reg(edrv, 0xff, 0x85);
	i2c_write_reg(edrv, 0x1a, eDP_LANE);
	// i2c_write_reg(edrv, 0x13,0xd1);
	i2c_write_reg(edrv, 0xff, 0xac);
	i2c_write_reg(edrv, 0x00, 0x64);
	i2c_write_reg(edrv, 0x01, 0x0a);
	i2c_write_reg(edrv, 0x0c, 0x85);
	i2c_write_reg(edrv, 0x0c, 0xc5);
}

static int lcd_LT8911EXB_power_on(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	// i2c_write_reg(edrv, 0xff, 0x81); // register bank
	// i2c_write_reg(edrv, 0x08, 0x7f);

	get_LT8911_chip_ID(edrv);

	LT8911_MIPI_Video_Timing(edrv);
	LT8911_eDP_Video_Timing(edrv);

	LT8911_init(edrv);

	LT8911_read_edid(edrv);

	LT8911_video_check(edrv);

	LT8911_link_training(edrv);
	LT8911_link_train_result(edrv);

	return 0;
}

static int lcd_LT8911EXB_power_off(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	return 0;
}

int aml_lcd_extern_LT8911EXB_probe(struct lcd_extern_driver_s *edrv, struct lcd_extern_dev_s *edev)
{
	if (!edrv) {
		printf("driver is null");
		return -1;
	}

	edev->power_on = lcd_LT8911EXB_power_on;
	edev->power_off = lcd_LT8911EXB_power_off;

	printf("%s: i2c_addr = %02x", __func__, edev->config.i2c_addr);

	return 0;
}
