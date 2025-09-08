/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_TCON_INIT_H
#define _AML_LCD_TCON_INIT_H
#include "lcd_tcon.h"

static struct lcd_tcon_init_setting_s lcd_tcon_init_pre_setting_uhd[] = {
	//addr, val, bit, len
	{0x207, 0,    4,   1},  //pre_proc_clk disable
	{0x263, 0,   31,   1},  //od ddrif disable
	{0x1a3, 0,   31,   1},  //demura ddrif disable
	{0x222, 1,   14,   1},   //demo acc
	{0x222, 1,   15,   1},   //demo dither
	{0x240, 1,    1,   1},   //demo od
	{0x150, 1,    1,   1},   //demo lod
	{0x190, 1,   30,   1},   //demo demura
	{0x2a1, 1,   23,   1},   //demo vac
	{REG_LCD_TCON_MAX, 0, 0, 0}
};

static struct lcd_tcon_init_setting_s lcd_tcon_init_pre_setting_fhd[] = {
	//addr, val, bit, len
	{0x207, 0,    4,   1},  //pre_proc_clk disable
	{0x263, 0,   31,   1},  //od ddrif disable
	{0x198, 0,   31,   1},   //demura ddrif disable
	{0x222, 1,   14,   1},   //demo acc
	{0x222, 1,   15,   1},   //demo dither
	{0x240, 1,    1,   1},   //demo od
	{0x150, 1,    1,   1},   //demo lod
	{0x190, 1,   30,   1},   //demo demura
	{REG_LCD_TCON_MAX, 0, 0, 0}
};

static struct lcd_tcon_init_setting_s lcd_tcon_init_post_setting[] = {
	//addr, val, bit, len
	{0x207, 1,    4,   1},  //pre_proc_clk enable
	{REG_LCD_TCON_MAX, 0, 0, 0}
};

#endif

