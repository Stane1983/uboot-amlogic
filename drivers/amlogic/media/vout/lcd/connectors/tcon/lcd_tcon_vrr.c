// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "lcd_tcon.h"

/*
 * mode: 0-normal mode, 1-fix mode
 */
void tcon_fr_detect_config(struct aml_lcd_drv_s *pdrv, unsigned int mode,
			unsigned int fr_levels[], unsigned int num_level,
			unsigned int step_counter)
{
	unsigned int h_blank = 0, i = 0, nvfp = 0, pclk, ht, vt = 0;
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int levels[FR_DETECT_LEVEL_MAX];

	if (mode == 0 && num_level == 0)
		return;
	if (num_level > 20)
		num_level = 20;

	pclk = pdrv->config.timing.base_timing->pixel_clk;
	ht = pdrv->config.timing.base_timing->h_period;

	h_blank = pconf->timing.base_timing->h_period - pconf->timing.base_timing->h_active;
	nvfp = pconf->timing.base_timing->v_active + pconf->timing.base_timing->vsync_bp +
		pconf->timing.base_timing->vsync_width;

	for (i = 0; i < num_level; i++) {
		vt = pclk / ht / fr_levels[i];
		levels[i] = vt - nvfp - 1; //vfp
	}

	lcd_tcon_setb(0x232, mode, 14, 1);//0=normal mode, calculate frame rate and trig intr

	lcd_tcon_setb(0x232, h_blank - 1, 15, 10);// ht - hact - 1
	if (mode == 1) { //fix mode only set step_counter
		lcd_tcon_setb(0x233, step_counter,  0, 13);
		return;
	}

	for (i = 0; i < num_level / 2; i++) {
		// frame rate level0[12:0] / level1[28:16]
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 0],  0, 13);
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 1], 16, 13);
	}

	if (num_level & 0x1)
		lcd_tcon_setb(0x233 + i, levels[i * 2 + 0],  0, 13);
}

void tcon_fr_detect_enable(struct aml_lcd_drv_s *pdrv, int enable)
{
	if (enable)
		lcd_tcon_setb(0x232, 1,  13, 1);
	else
		lcd_tcon_setb(0x232, 0,  13, 1);
}

void lcd_tcon_data_parse_vrr(struct lcd_tcon_vrr_data_s *vrr,
				unsigned char *p, unsigned int data_cnt, unsigned int data_width)
{
	unsigned int d = 0, m = 0, temp = 0, k;

	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_h_active = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_v_active = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_frame_rate_min = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->disp_frame_rate_max = temp;

	m += data_width;
	for (d = 0, temp = 0;  d < data_width; d++)
		temp |= (p[m + d] << (d * 8));
	vrr->part = temp;

	if (vrr->part >= FR_DETECT_LEVEL_MAX) {
		vrr->part = 0;
		return;
	}
	for (k = 0; k < vrr->part; k++) {
		m += data_width;
		for (d = 0, temp = 0;  d < data_width; d++)
			temp |= (p[m + d] << (d * 8));
		vrr->fr_level[k] = temp;
	}
	vrr->fr_count = vrr->part;
	for (k = vrr->fr_count; k < 20; k++)
		vrr->fr_level[k] = 0x1fff;
	vrr->support = 1;
}

int lcd_tcon_vrr_fr_sw_match(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_vrr_data_s *vrr)
{
	int i = 0, fr;

	fr = pdrv->config.timing.act_timing.frame_rate;

	for (i = 0; i < vrr->fr_count; i++)
		if (fr >= vrr->fr_level[i])
			break;

	return (i >= vrr->part ? 0 : i);
}

/*===============================================================================================*/
/* slave_addr: must be 8bit addr (7bit addr << 1)
 * bytes: data bytes + 1byte reg offset
 * speed: i2c speed
 */
void tcon_i2c_init(struct aml_lcd_drv_s *pdrv, u32 slave_addr, u32 bytes, u32 speed)
{
	const unsigned int base_clk = 167000000;//168Mhz
	unsigned int i2c_clk_div = 1670;

	speed = lcd_s32_constraint(speed, 50000, 1000000);//50k~1M
	i2c_clk_div = base_clk / speed;

	/* i2c controller set */
	lcd_periphs_write(0XFE068000 + 0, 0);

	lcd_periphs_write(0XFE068000 + 0xc, 0x21000 | (i2c_clk_div & 0xfff));

	//irq enable, bit0:rd trans err, bit1;wr trans err, bit2: nack err, bit3:trans done
	lcd_periphs_write(0XFE068000 + 0x34, 0x0);// disable all irq
	lcd_periphs_write(0XFE068000 + 0x04, 0x14);//bit4:1=dma 0=normal, bit3:1=negative edge
	lcd_periphs_write(0XFE068000 + 0x10, 0x0);
	lcd_periphs_write(0XFE068000 + 0x14, 0x0);
	lcd_periphs_write(0XFE068000 + 0x18, 0x0);
	lcd_periphs_write(0XFE068000 + 0x1c, 0x0);
	lcd_periphs_write(0XFE068000 + 0x08, (bytes << 12) | (slave_addr << 1));
}

unsigned char tcon_ip27_valid(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_ip27_s *ip27)
{
	unsigned int temp;//, temp2;

	if (!pdrv->data || pdrv->data->chip_type != LCD_CHIP_T6X || !ip27->cfg_valid)
		return 0;

	temp = lcd_tcon_read(0x1425);//cfcom must exist
	if (temp || ip27->debug)
		return 1;

	return 0;
}

unsigned int tcon_ip27_calc_trig_line(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_ip27_s *ip27)
{
	unsigned int trig_line = 0;
	unsigned int trans_time_us, trans_bits, line_us, trans_line, bk_line;

	/* calculate best trigger line */
	trans_bits = (ip27->i2c_bytes + 1) * 9; //1 slave addr, ignore start/stop

	trans_time_us = 1000000 * trans_bits / ip27->i2c_speed;

	line_us = 1000000 / (pdrv->config.timing.base_timing->vfreq_vrr_max *
			     pdrv->config.timing.base_timing->v_period_min);
	trans_line = trans_time_us / line_us + 1;

	bk_line = pdrv->config.timing.base_timing->v_period_min -
		  pdrv->config.timing.base_timing->v_active - 16; // for pre_de

	if (bk_line >= trans_line)
		trig_line = pdrv->config.timing.vend;
	else
		trig_line = pdrv->config.timing.vend - (trans_line - bk_line + 1);

	LCDPR("ip27 trig line:%d\n", trig_line);

	return trig_line;
}

// trig_mode: 0-ip27-self, 1-aptu-int
void tcon_ip27_init(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_ip27_s *ip27)
{
	// disable ip27 fifo clock
	lcd_tcon_setb(0x207, 0, 3, 1);//reg_pre_gam_clk disabled for ip27 in pre

	lcd_tcon_setb(0x14f1, ip27->vcom_order, 6, 1); //reg_ip27_vcom_data_order
	lcd_tcon_setb(0x14f1, ip27->i2c_bytes - 1, 1, 5);//reg_ip27_i2c_data_num
	lcd_tcon_setb(0x14f1, 0x7f801 | (ip27->i2c_reg_offset << 1), 12, 20);

	switch (ip27->data_mode) {
	case 0: //24 byte mode
		lcd_tcon_write(0x14f2, 0x07654321);//reg_ip27_i2c_dout_order31_0
		lcd_tcon_setb(0x14f1, 0x0, 7, 4);//reg_ip27_i2c_dout_order35_32
		break;
	case 1://acom+cfcom mode use data1/2
		lcd_tcon_write(0x14f2, 0x0);//reg_ip27_i2c_dout_order31_0
		lcd_tcon_setb(0x14f1, 0x0, 7, 4);//reg_ip27_i2c_dout_order35_32
		break;
	case 2://acom+cfcom mode use data17/18
		lcd_tcon_write(0x14f2, 0x8);//reg_ip27_i2c_dout_order31_0
		lcd_tcon_setb(0x14f1, 0x0, 7, 4);//reg_ip27_i2c_dout_order35_32
		break;
	default:
		break;
	}

	lcd_tcon_setb(0x1400, 1, 31, 1); // ip27_bypass
	lcd_tcon_setb(0x367, 0, 16, 1); // aptu trig en
	lcd_tcon_setb(0x200e, 1, 7, 1); //bit7:apb_clk_gate_ctrl to clear ip27 fifo
	if (ip27->trig_mode == IP27_TRIG_APTU) {
		lcd_tcon_setb(0x367, ip27->trig_line, 0, 14); // trigger delay
		lcd_tcon_setb(0x2000, 1, 23, 1); // use aptu int trigger
	} else {
		lcd_tcon_setb(0x2000, 0, 23, 1); // use ip27 trigger
	}

	lcd_tcon_setb(0x207, 1, 29, 1); // reg_pre_ip27_clk_en
	// release init data
	lcd_tcon_setb(0x1400, 0, 31, 1); // ip27_bypass disable
	lcd_wait_vsync(pdrv);
	lcd_tcon_setb(0x1400, 1, 31, 1); // ip27_bypass

	// enable ip27 fifo clock
	lcd_tcon_setb(0x207, 1, 3, 1);//reg_pre_gam_clk disabled for ip27 in pre
}

void tcon_ip27_setup(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_ip27_s *ip27;

	if (!pdrv || !local_cfg)
		return;

	ip27 = &local_cfg->ip27;
	ip27->valid = tcon_ip27_valid(pdrv, ip27);
	if (ip27->valid) {
		LCDPR("ip27 valid\n");
		if (ip27->trig_line_dbg)
			ip27->trig_line = ip27->trig_line_dbg;
		else
			ip27->trig_line = tcon_ip27_calc_trig_line(pdrv, ip27);
		tcon_ip27_init(pdrv, ip27);
		tcon_i2c_init(pdrv, ip27->i2c_addr, ip27->i2c_bytes,
			      ip27->i2c_speed);
		ip27->step = 1;
		ip27->init = 1;
	}
}

void tcon_ip27_release(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_tcon_local_cfg_s *local_cfg = get_lcd_tcon_local_cfg();
	struct lcd_tcon_ip27_s *ip27;

	if (!pdrv || !local_cfg)
		return;

	ip27 = &local_cfg->ip27;
	if (ip27->valid) {
		lcd_tcon_setb(0x1400, 1, 31, 1); //ip27_bypass_en
		lcd_tcon_setb(0x2000, 0, 23, 1); // use ip27 trigger
		lcd_tcon_setb(0x207, 0, 29, 1); // reg_pre_ip27_clk_en
		ip27->step = 0;
		ip27->init = 0;
		ip27->valid = 0;
	}
}
