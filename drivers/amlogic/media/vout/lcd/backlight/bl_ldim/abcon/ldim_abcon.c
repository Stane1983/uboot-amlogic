// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <linux/dma-mapping.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_memory.h>
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

struct abcon_mem_s abcon_mem;
struct abcon_s *abcon;

void ldim_abcon_swrst(void)
{
	abcon_wr_reg(0x00, 0x30);
	udelay(1);
	abcon_wr_reg(0x00, 0x00);
	udelay(1);
}

void ldim_abcon_hw_reset(void)
{
	lcd_reset_setb(RESETCTRL_RESET2_MASK, 0, 6, 1);
	lcd_reset_setb(RESETCTRL_RESET2_LEVEL, 0, 6, 1);
	udelay(1);
	lcd_reset_setb(RESETCTRL_RESET2_LEVEL, 1, 6, 1);
	udelay(2);
}

//todo: consider boost mode, glb cur or local cur
void ldim_abcon_swduty_set(struct aml_ldim_driver_s *ldim_drv)
{
	struct ldim_dev_driver_s *dev_drv = ldim_drv->dev_drv;
	unsigned short *bl_matrix = ldim_drv->bl_matrix;
	unsigned char *buf, *p;
	unsigned int fid;
	unsigned int row, col, n, zone_num, index;
	unsigned int temp = 0;
	int i, j, k, m, ext;
	unsigned int rowbuf;

	if (!dev_drv) {
		ABCONERR("%s: dev_drv buf is null\n", __func__);
		return;
	}

	if (!bl_matrix) {
		ABCONERR("%s: bl_matrix buf is null\n", __func__);
		return;
	}
	if (!abcon_mem.swduty) {
		ABCONERR("%s: swduty_vaddr is null\n", __func__);
		return;
	}

	abcon_mem.swduty_fid = 0;//always set 0 for test
	fid = abcon_mem.swduty_fid;
	buf = (unsigned char *)(abcon_mem.swduty + (LDC_DUTY_SIZE * fid >> 2));

	row = dev_drv->bl_row;
	col = dev_drv->bl_col;
	zone_num = row * col;
	rowbuf = 0x100; //256 Bytes(16*128bit) per row as t3x

	n = 3; /* 3bytes 2 seg */
	for (i = 0; i < row; i++) {
		p = buf + (i * rowbuf);
		m = 0;
		for (j = 0; j < col; j += 2) {
			ext = j / 10;
			index = i * col + j;
			temp = bl_matrix[index] & 0xfff;
			if ((index + 1) < zone_num)
				temp |= ((bl_matrix[index + 1] & 0xfff) << 12);

			for (k = 0; k < n; k++)
				p[m * n + k + ext] = (temp >> (k * 8)) & 0xff;
			m++;
		}
	}

	//reg_abcon_ldc_gen_duty_zone
	abcon_wr_reg_bits(0x2c, fid, 30, 2);
	//invalidate_dcache_range((unsigned long)buf,
	//	(unsigned long)(buf + LDC_DUTY_SIZE));
	//flush_cache((unsigned long)abcon_mem.swduty, LDC_DUTY_SIZE);
	flush_dcache_range((unsigned long)abcon_mem.swduty,
		(unsigned long)(abcon_mem.swduty + LDC_DUTY_SIZE));

	temp = abcon->conf.init_dc & 0x3ff;
	//reg_abcon_dc_ldc_glb_apl
	abcon_wr_reg_bits(0x11, temp, 0, 10);
}

int ldim_abcon_mem_init(void)
{
	unsigned int size = ABCON_OFFSET_END;
	unsigned int *vaddr;
	phys_addr_t paddr;

	//vaddr = dma_alloc_coherent(size, (unsigned long *)&paddr);
	vaddr = (unsigned int *)malloc(size);
	paddr = (phys_addr_t)vaddr;
	if (!vaddr) {
		ABCONERR("no rsvd mem to save abcon_base\n");
		return -1;
	} else {
		ABCONPR("malloc buf, paddr=0x%x, vaddr=0x%p, size = 0x%x\n", (unsigned int)paddr, vaddr, size);
	}

	abcon_mem.base_paddr = paddr;
	abcon_mem.base_vaddr = vaddr;

	abcon_mem.ch_mapping_paddr = abcon_mem.base_paddr + CH_MAPPING_OFFSET;
	abcon_mem.ldc_seg_paddr = abcon_mem.base_paddr + LDC_SEG_OFFSET;
	abcon_mem.swduty_paddr = abcon_mem.base_paddr + SW_DUTY_OFFSET;
	abcon_mem.wseg_paddr = abcon_mem.base_paddr + WSEG_OFFSET;
	abcon_mem.rseg_paddr = abcon_mem.base_paddr + RSEG_OFFSET;

	abcon_mem.ch_mapping = abcon_mem.base_vaddr;
	abcon_mem.ldc_seg = abcon_mem.base_vaddr + LDC_SEG_OFFSET / 4;
	abcon_mem.swduty = abcon_mem.base_vaddr + SW_DUTY_OFFSET / 4;
	abcon_mem.wseg = abcon_mem.base_vaddr + WSEG_OFFSET / 4;
	abcon_mem.rseg = abcon_mem.base_vaddr + RSEG_OFFSET / 4;
	abcon_mem.swduty_fid = 0;

	ABCONPR("abcon_base paddr=0x%x, total size=0x%x\n"
		"ch_mapping_paddr=0x%x, ch_mapping=0x%px\n"
		"ldc_seg_paddr=0x%x, ldc_seg=0x%px\n"
		"swduty_paddr=0x%x, swduty=0x%px\n"
		"wseg_paddr=0x%x, wseg=0x%px\n"
		"rseg_paddr=0x%x, rseg=0x%px\n",
		(unsigned int)paddr, size,
		(unsigned int)abcon_mem.ch_mapping_paddr, abcon_mem.ch_mapping,
		(unsigned int)abcon_mem.ldc_seg_paddr, abcon_mem.ldc_seg,
		(unsigned int)abcon_mem.swduty_paddr, abcon_mem.swduty,
		(unsigned int)abcon_mem.wseg_paddr, abcon_mem.wseg,
		(unsigned int)abcon_mem.rseg_paddr, abcon_mem.rseg);

	return 0;
}

void ldim_abcon_set_txrx_clk(unsigned int txclk, unsigned int rxclk)
{
	unsigned char bmc_enc;
	unsigned char enc_bit_num;
	unsigned int txscale, rxscale;

	switch (abcon->conf.dev_type) {
	case 0://hyasic
		bmc_enc = 1;
		enc_bit_num = 2;
		break;
	case 1:
	case 2://xsignal
	case 3://xianxin
		bmc_enc = 0;
		enc_bit_num = 4;
		break;
	default:
		bmc_enc = 1;
		enc_bit_num = 2;
		break;
	}

	if (txclk == 0)
		txclk = 1000; //default 1Mhz
	if (rxclk == 0)
		rxclk = 1000; //default 1Mhz
	txscale = 1000000 / (txclk * enc_bit_num * 6);
	rxscale = 1000000 / (rxclk * enc_bit_num * 6);

	abcon_wr_reg_bits(0x05, bmc_enc, 31, 1);//reg_abcon_bmc_enc
	abcon_wr_reg_bits(0x05, bmc_enc, 13, 1);//reg_abcon_bmc_dec

	abcon_wr_reg_bits(0x16, enc_bit_num, 13, 3);//reg_abcon_enc_bit_num

	//reg_abcon_tx_sck_set
	abcon_wr_reg_bits(0x02, ((txscale / 2) - 1), 0, 8);

	abcon_wr_reg_bits(0x01, txscale, 0, 10);//reg_abcon_tx_scale
	abcon_wr_reg_bits(0x01, txscale, 20, 10);//reg_abcon_tx_scale0
	abcon_wr_reg_bits(0x17, txscale, 0, 10);//reg_abcon_tx_scale1
	abcon_wr_reg_bits(0x17, txscale, 10, 10);//reg_abcon_tx_scale2
	abcon_wr_reg_bits(0x17, txscale, 20, 10);//reg_abcon_tx_scale3

	abcon_wr_reg_bits(0x01, rxscale, 10, 10);//reg_abcon_rx_scale
	abcon_wr_reg_bits(0x18, rxscale, 11, 10);//reg_abcon_rx_scale0
	abcon_wr_reg_bits(0x18, rxscale, 21, 10);//reg_abcon_rx_scale1
	abcon_wr_reg_bits(0x19, rxscale, 0, 10);//reg_abcon_rx_scale2
	abcon_wr_reg_bits(0x19, rxscale, 10, 10);//reg_abcon_rx_scale3
}

void ldim_abcon_set_gpio(struct abcon_s *abcon)
{
	if (abcon->chip_type == ABCON_CHIP_T6W) {
		//reg_abcon_gpio_o_sel0
		abcon_wr_reg_bits(0x25, abcon->conf.gpio_o[0], 0, 32);
		//reg_abcon_gpio_o_sel1
		abcon_wr_reg_bits(0x26, abcon->conf.gpio_o[1], 0, 16);

		//reg_abcon_gpio_i_sel0
		abcon_wr_reg_bits(0x23, abcon->conf.gpio_i[0], 0, 32);
		//reg_abcon_gpio_i_sel1
		abcon_wr_reg_bits(0x24, abcon->conf.gpio_i[1], 0, 16);
	} else {
		//reg_abcon_gpio_o_sel0
		abcon_wr_reg_bits(0x25, abcon->conf.gpio_o[0], 0, 29);
		//reg_abcon_gpio_o_sel1
		abcon_wr_reg_bits(0x26, abcon->conf.gpio_o[1], 0, 29);
		//reg_abcon_gpio_o_sel2
		abcon_wr_reg_bits(0x4a, abcon->conf.gpio_o[2], 0, 29);

		//reg_abcon_gpio_i_sel0
		abcon_wr_reg_bits(0x23, abcon->conf.gpio_i[0], 0, 29);
		//reg_abcon_gpio_i_sel1
		abcon_wr_reg_bits(0x49, abcon->conf.gpio_i[1], 0, 29);
		//reg_abcon_gpio_i_sel2
		abcon_wr_reg_bits(0x4b, abcon->conf.gpio_i[2], 0, 29);
	}
}

void ldim_abcon_set_channel(struct abcon_s *abcon)
{
	unsigned char temp = 0;
	
	if (abcon->chip_type == ABCON_CHIP_T6W) {
		//reg_abcon_abcon_total_chnl_num
		abcon_wr_reg_bits(0x05, abcon->tot_ch_num, 0, 13);

		//reg_abcon_lane0_chnl_num
		abcon_wr_reg_bits(0x12, abcon->lane_ch[0], 0, 11);
		//reg_abcon_lane1_chnl_num
		abcon_wr_reg_bits(0x12, abcon->lane_ch[1], 16, 11);
		//reg_abcon_lane2_chnl_num
		abcon_wr_reg_bits(0x13, abcon->lane_ch[2], 0, 11);
		//reg_abcon_lane3_chnl_num
		abcon_wr_reg_bits(0x13, abcon->lane_ch[3], 16, 11);

		//reg_abcon_max_lane_chnl_num
		if (abcon->conf.dev_type == 3)
			abcon_wr_reg_bits(0x18, abcon->conf.ch_num, 0, 11);//xianxin special
		else
			abcon_wr_reg_bits(0x18, abcon->max_lane_ch, 0, 11);
	} else {
		temp = 0;
		//reg_abcon_lane0_chnl_num
		abcon_wr_reg_bits(0x12, temp, 0, 13);
		temp = abcon->lane_ch[0];
		//reg_abcon_lane1_chnl_num
		abcon_wr_reg_bits(0x12, temp, 16, 13);
		temp += abcon->lane_ch[1];
		//reg_abcon_lane2_chnl_num
		abcon_wr_reg_bits(0x13, temp, 0, 13);
		temp += abcon->lane_ch[2];
		//reg_abcon_lane3_chnl_num
		abcon_wr_reg_bits(0x13, temp, 16, 13);
		temp += abcon->lane_ch[3];
		//reg_abcon_lane4_chnl_num
		abcon_wr_reg_bits(0x14, temp, 0, 13);
		temp += abcon->lane_ch[4];
		//reg_abcon_lane5_chnl_num
		abcon_wr_reg_bits(0x14, temp, 16, 13);
		temp += abcon->lane_ch[5];
		//reg_abcon_lane6_chnl_num
		abcon_wr_reg_bits(0x30, temp, 0, 13);
		temp += abcon->lane_ch[6];
		//reg_abcon_lane7_chnl_num
		abcon_wr_reg_bits(0x30, temp, 16, 13);
		temp += abcon->lane_ch[7];
		//reg_abcon_lane8_chnl_num
		abcon_wr_reg_bits(0x31, temp, 0, 13);
		temp += abcon->lane_ch[8];
		//reg_abcon_lane9_chnl_num
		abcon_wr_reg_bits(0x31, temp, 16, 13);
		temp += abcon->lane_ch[9];
		//reg_abcon_lane10_chnl_num
		abcon_wr_reg_bits(0x32, temp, 0, 13);
		temp += abcon->lane_ch[10];
		//reg_abcon_lane11_chnl_num
		abcon_wr_reg_bits(0x32, temp, 16, 13);

		//reg_abcon_max_lane_chnl_num
		if (abcon->conf.dev_type == 3)
			abcon_wr_reg_bits(0x05, abcon->conf.ch_num, 0, 13);//xianxin special
		else
			abcon_wr_reg_bits(0x05, abcon->max_lane_ch, 0, 13);
	}
}

void ldim_abcon_init_common_registers(struct abcon_s *abcon)
{
	ABCONPR("begin init bcon common registers\n");
	//rdmif little endian
	abcon_wr_reg(0x20, 0x00000040);

	//reg_abcon_manu_num
	abcon_wr_reg_bits(0x03, abcon->conf.dev_type, 14, 4);

	//set txrx clk
	ldim_abcon_set_txrx_clk(abcon->conf.tx_clk, abcon->conf.rx_clk);

	//reg_abcon_mapping_table_baddr
	abcon_wr_reg(0x0a, abcon_mem.ch_mapping_paddr / 4);

	//reg_abcon_duty_baddr
	abcon_wr_reg(0x07, abcon_mem.swduty_paddr / 4);

	//reg_abcon_sw_wseg_baddr
	abcon_wr_reg(0x08, abcon_mem.wseg_paddr / 4);
	//reg_abcon_ldc_seg_baddr
	abcon_wr_reg(0x09, abcon_mem.ldc_seg_paddr / 4);

	//reg_abcon_sw_rseg_baddr
	abcon_wr_reg(0x15, abcon_mem.rseg_paddr / 4);

	//reg_abcon_auto_trans_baddr
	abcon_wr_reg(0x28, abcon_mem.wseg_paddr / 4);

	//reg_abcon_act_lane_num
	abcon_wr_reg_bits(0x03, abcon->act_lane_num, 10, 4);

	//set gpio
	ldim_abcon_set_gpio(abcon);

	//set channel
	ldim_abcon_set_channel(abcon);

	//reg_abcon_duty_zone_ofst_sft
	abcon_wr_reg_bits(0x35, 9, 20, 4);//0x800 = 1 << 9
	//reg_abcon_duty_stride
	abcon_wr_reg_bits(0x0b, 64, 16, 13);

	//reg_abcon_sw_wseg_stride
	abcon_wr_reg_bits(0x0b, 0x1ffff, 0, 13);
	//reg_abcon_ldc_seg_stride
	abcon_wr_reg_bits(0x0c, 0x1ffff, 0, 13);
	//reg_abcon_mapping_stride
	abcon_wr_reg_bits(0x0c, 0x1ffff, 16, 13);
	//reg_abcon_sw_rseg_stride
	abcon_wr_reg_bits(0x16, 0x1ffff, 0, 13);

	ABCONPR("init bcon common registers end\n");
}

void ldim_abcon_load_ch_mapping(struct ldim_dev_driver_s *dev_drv, unsigned int len)
{
	if (dev_drv->bl_mapping && abcon_mem.ch_mapping) {
		//copy mapping data to ch_mapping buf
		memcpy(abcon_mem.ch_mapping, dev_drv->bl_mapping, len);
		//invalidate_dcache_range((unsigned long)abcon_mem.ch_mapping,
		//	(unsigned long)(abcon_mem.ch_mapping + len));
		//flush_cache((unsigned long)abcon_mem.ch_mapping, len);
		flush_dcache_range((unsigned long)abcon_mem.ch_mapping,
			(unsigned long)(abcon_mem.ch_mapping + len));

		udelay(1);

		//reg_abcon_load_mapping_start
		abcon_wr_reg_bits(0x00, 1, 2, 1);
		abcon_wr_reg_bits(0x00, 0, 2, 1);
		ABCONPR("abcon load ch mapping done, total size=0x%x\n", len);
	}
}

int ldim_abcon_dev_probe(struct ldim_dev_driver_s *dev_drv)
{
	int ret = 0;
	unsigned int temp = 0;
	unsigned int i = 0;

	abcon = malloc(sizeof(struct abcon_s));
	if (!abcon) {
		LDIMERR("%s: abcon malloc failed\n", __func__);
		return -1;
	}

	memcpy(&abcon->conf, &dev_drv->abcon_conf, sizeof(struct abcon_conf_s));
	abcon->tot_ch_num = 0;
	abcon->act_lane = 0;
	abcon->act_lane_num = 0;
	abcon->max_lane_ch = 0;
	for (i = 0; i < 12; i++) {
		abcon->lane_ch[i] = abcon->conf.ch_num * abcon->conf.chip_num[i];
		abcon->tot_ch_num += abcon->lane_ch[i];
		if (abcon->lane_ch[i] > abcon->max_lane_ch)
			abcon->max_lane_ch = abcon->lane_ch[i];
		if (abcon->lane_ch[i]) {
			abcon->act_lane_num += 1;
			abcon->act_lane |= 1 << i;
		}
	}
	abcon->max_lane_dim = abcon->max_lane_ch / abcon->conf.ch_num;

	ABCONPR("lane_ch:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d\n"
		"tot_ch_num:%d, act_lane:0x%x,act_lane_num:%d,max_lane_ch:%d,max_lane_dim=%d\n",
		abcon->lane_ch[0], abcon->lane_ch[1],
		abcon->lane_ch[2], abcon->lane_ch[3],
		abcon->lane_ch[4], abcon->lane_ch[5],
		abcon->lane_ch[6], abcon->lane_ch[7],
		abcon->lane_ch[8], abcon->lane_ch[9],
		abcon->lane_ch[10], abcon->lane_ch[11],
		abcon->tot_ch_num, abcon->act_lane, abcon->act_lane_num, abcon->max_lane_ch, abcon->max_lane_dim);

	ldim_abcon_hw_reset();
	udelay(5);

	//set abcon clk ctrl
	temp = (1 << 7) |	/*bcon clk_sel*/
			(1 << 6) |	/*mux0 en*/
			(0 << 0);	/*bcon clk_div*/
	abcon_wr_reg(CLKCTRL_BCON_CLK_CNTL, temp);
	i = abcon_rd_reg(CLKCTRL_BCON_CLK_CNTL);
	ABCONPR("CLKCTRL_BCON_CLK_CNTL:0x%08x\n", i);

	ldim_abcon_swrst();

	//memory init
	ret =  ldim_abcon_mem_init();
	if (ret)
		return -1;

	/*init register*/
	ldim_abcon_init_common_registers(abcon);

	/*load ch mapping*/
	temp = abcon->tot_ch_num * 2;
	ldim_abcon_load_ch_mapping(dev_drv, temp);

	i = abcon_rd_reg(0x50);
	ABCONPR("reg 0x50:0x%08x\n", i);
	i = abcon_rd_reg(0x51);
	ABCONPR("reg 0x51:0x%08x\n", i);

	return ret;
}

int ldim_abcon_dev_remove(struct aml_ldim_driver_s *ldim_drv)
{
	return 0;
}
