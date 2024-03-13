/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef INC_AML_LCD_TIMING_H
#define INC_AML_LCD_TIMING_H

struct lcd_detail_timing_s {
	short h_active;    /* Horizontal display area */
	short v_active;    /* Vertical display area */
	short h_period;    /* Horizontal total period time */
	short v_period;    /* Vertical total period time */

	short hsync_width;
	short hsync_bp;
	short hsync_fp;
	short vsync_width;
	short vsync_bp;
	short vsync_fp;
	unsigned char hsync_pol;
	unsigned char vsync_pol;
	unsigned char fr_adjust_type; /* 0=clock, 1=htotal, 2=vtotal */
	unsigned int pixel_clk;

	unsigned short h_period_min;
	unsigned short h_period_max;
	unsigned short v_period_min;
	unsigned short v_period_max;
	unsigned short frame_rate_min; //for vmode management
	unsigned short frame_rate_max; //for vmode management
	unsigned int pclk_min;
	unsigned int pclk_max;

	unsigned short vfreq_vrr_min; //driver calculate for vrr range
	unsigned short vfreq_vrr_max; //driver calculate for vrr range
	unsigned short frame_rate;  //driver calculate for internal usage
	unsigned short frac;
	unsigned int sync_duration_num;  //driver calculate for internal usage
	unsigned int sync_duration_den;  //driver calculate for internal usage
};

#endif
