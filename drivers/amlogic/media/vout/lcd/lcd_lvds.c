// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"

void lcd_lvds_enable(struct aml_lcd_drv_s *pdrv)
{
	unsigned int bit_num, pn_swap;
	unsigned int dual_port, fifo_mode, lvds_repack, sync_pol_reverse, lsb_first = 0;
	unsigned int offset;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	offset = pdrv->data->offset_venc_if[pdrv->index];
	lvds_repack = pdrv->config.control.lvds_cfg.lvds_repack & 0x3;
	pn_swap   = pdrv->config.control.lvds_cfg.pn_swap;
	dual_port = pdrv->config.control.lvds_cfg.dual_port;
	fifo_mode = dual_port ? 0x3 : 0x1;

	// H V:  L_POL_CNTL_ADDR LVDS_PACK_CNTL_ADDR
	// 0 0:  h: 1  v: 1      1
	// 0 1:  h: 1  v: 0      1
	// 1 0:  h: 1  v: 0      0
	// 1 1:  h: 1  v: 1      0
	sync_pol_reverse = !pdrv->config.timing.act_timing.hsync_pol; // reserve both h & v

	if (pdrv->config.timing.act_timing.lcd_bits == 30) {
		bit_num = 0;
	} else if (pdrv->config.timing.act_timing.lcd_bits == 18) {
		bit_num = 2;
	} else { // 24bit
		bit_num = 1;
	}

	if (pdrv->data->chip_type == LCD_CHIP_T7 ||
	    pdrv->data->chip_type == LCD_CHIP_T3 ||
	    pdrv->data->chip_type == LCD_CHIP_T3X ||
	    pdrv->data->chip_type == LCD_CHIP_T5M)
		lcd_vcbus_write(LVDS_SER_EN + offset, 0xfff);

	if (pdrv->data->chip_type == LCD_CHIP_T6D)
		lsb_first = 1;

	lcd_vcbus_write(LVDS_PACK_CNTL_ADDR + offset,
			(lvds_repack << 0) | // repack //[1:0]
			(sync_pol_reverse << 3) | // reserve
			(lsb_first << 4) |		// lsb first
			(pn_swap << 5) |	// pn swap
			(dual_port << 6) |	// dual port
			(0 << 7) |		// use tcon control
			(bit_num << 8) |	// 0:10bits, 1:8bits, 2:6bits, 3:4bits.
			(0 << 10) |		//r_select  //0:R, 1:G, 2:B, 3:0
			(1 << 12) |		//g_select  //0:R, 1:G, 2:B, 3:0
			(2 << 14));		//b_select  //0:R, 1:G, 2:B, 3:0;

	if (pdrv->data->chip_type == LCD_CHIP_T5M ||
	    pdrv->data->chip_type == LCD_CHIP_T3 ||
	    pdrv->data->chip_type == LCD_CHIP_T7 ||
	    pdrv->data->chip_type == LCD_CHIP_T3X ||
	    pdrv->data->chip_type == LCD_CHIP_T6D) {
		lcd_vcbus_write(P2P_BIT_REV + offset, 2);
	}

	lcd_vcbus_setb(LVDS_GEN_CNTL + offset, 1, 4, 1);
	lcd_vcbus_setb(LVDS_GEN_CNTL + offset, fifo_mode, 0, 2);// fifo wr mode
	lcd_vcbus_setb(LVDS_GEN_CNTL + offset, 1, 3, 1);// fifo enable
}

void lcd_lvds_disable(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset = pdrv->data->offset_venc_if[pdrv->index];

	/* disable lvds fifo */
	lcd_vcbus_setb(LVDS_GEN_CNTL + offset, 0, 3, 1);
	lcd_vcbus_setb(LVDS_GEN_CNTL + offset, 0, 0, 2);
}
