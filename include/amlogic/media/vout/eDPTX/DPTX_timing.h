/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMLOGIC_DisplayPort_TX_TIMING_H
#define _AMLOGIC_DisplayPort_TX_TIMING_H

#define DPTX_CFMT_RGB_6bit             0
#define DPTX_CFMT_RGB_8bit             1
#define DPTX_CFMT_RGB_10bit            2
#define DPTX_CFMT_RGB_12bit            3
#define DPTX_CFMT_YCbCr422_8bit        4
#define DPTX_CFMT_YCbCr422_10bit       5
#define DPTX_CFMT_YCbCr422_12bit       6
#define DPTX_CFMT_YCbCr444_8bit        7
#define DPTX_CFMT_YCbCr444_10bit       8
#define DPTX_CFMT_YCbCr444_12bit       9
#define DPTX_CFMT_YCbCr420_8bit        10
#define DPTX_CFMT_YCbCr420_10bit       11
#define DPTX_CFMT_YCbCr420_12bit       12
#define DPTX_CFMT_Y_only_8bit          13
#define DPTX_CFMT_Y_only_10bit         14
#define DPTX_CFMT_Y_only_12bit         15
#define DPTX_CFMT_invalid              0xff

struct dptx_color_format_s {
	unsigned char cfmt_id;
	unsigned char bpp;
	char *name;
};

#define DPTX_COLOR_TYPE_MAX  17
extern struct dptx_color_format_s dptx_cfmt_table[DPTX_COLOR_TYPE_MAX];

#define CTRL_HSYNC_POS          BIT(0)
#define CTRL_VSYNC_POS          BIT(1)
#define CTRL_PREFERRED_TIMING   BIT(2)
#define CTRL_DUPLICATED_TIMING  BIT(7)

struct dptx_detail_timing_s {
	char name[32];

	uint16_t h_size;
	uint16_t v_size;

	uint16_t h_period;
	uint16_t h_act;
	uint16_t h_blank;
	uint16_t h_bp;
	uint16_t h_pw;
	uint16_t h_fp;

	uint16_t v_period;
	uint16_t v_act;
	uint16_t v_blank;
	uint16_t v_bp;
	uint16_t v_pw;
	uint16_t v_fp;

	// uint16_t h_border;
	// uint16_t v_border;

	uint16_t ctrl;

	uint32_t pclk;
	uint32_t fr1000;  //driver calculate for internal usage

	uint8_t cfmt;

	// unsigned short h_period_range[2];
	unsigned short v_period_range[2];
	unsigned short pclk_range[2];

	unsigned short frame_rate_range[2]; //for vmode management

	// unsigned short vfreq_vrr_range[2]; //driver calculate for vrr range

	unsigned int sync_duration_num;  //driver calculate for internal usage
	unsigned int sync_duration_den;  //driver calculate for internal usage
	unsigned short vmode_add_fr[8];
};

#endif /* AMLOGIC_DISPLAYPORT_TX_TIMING_H */
