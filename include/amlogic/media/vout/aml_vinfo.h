/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_VINFO_H_
#define __AML_VINFO_H_

#define LATENCY_INVALID_UNKNOWN	0
#define LATENCY_NOT_SUPPORT		0xffff
struct rx_av_latency {
	unsigned int vLatency;
	unsigned int aLatency;
	unsigned int i_vLatency;
	unsigned int i_aLatency;
};

struct vinfo_s {
	ushort width;  /* Number of columns (i.e. 160) */
	ushort height; /* Number of rows (i.e. 100) */
	ushort field_height; /* for interlace */
	u_char vl_bpix; /* Bits per pixel, 0 = 1 */

	void *vd_base; /* Start of framebuffer memory */

	void *vd_console_address; /* Start of console buffer */
	short console_col;
	short console_row;

	int vd_color_fg;
	int vd_color_bg;

	ushort *cmap; /* Pointer to the colormap */
	void *priv; /* Pointer to driver-specific data */
	/* new parameters for s5 or later
	 * if current output is FRL or DSC mode,
	 * then there may use 2 or 4 slices pixel per clock.
	 * the default value is 0 or 1.
	 */
	u_char cur_enc_ppc;
	/* 0: yuv, 1: rgb */
	u8 vpp_post_out_color_fmt;
};

/*
 *hdr_dynamic_type
 * 0x0001: type_1_hdr_metadata_version
 * 0x0002: ts_103_433_spec_version
 * 0x0003: itu_t_h265_spec_version
 * 0x0004: type_4_hdr_metadata_version
 */
struct hdr_dynamic {
	unsigned int type;
	unsigned char support_flags;
	unsigned int of_len;   /*optional_fields length*/
	unsigned char optional_fields[28];
};

struct hdr10_plus_info {
	u32 ieeeoui;
	u8 length;
	u8 application_version;
};

struct cuva_info {
	u32 cuva_support;
	u8 rawdata[15];
	u8 length;
	u32 ieeeoui;
	u8 system_start_code;
	u8 version_code;
	u32 display_max_lum;
	u16 display_min_lum;
	u8 monitor_mode_sup;
	u8 rx_mode_sup;
};

/* SBTM EDID capabilities */
struct sbtm_info {
	unsigned char sbtm_support: 1;
	unsigned char max_sbtm_ver: 4;
	unsigned char grdm_support: 2;
	unsigned char drdm_ind: 1;
	unsigned char hgig_cat_drdm_sel: 3;
	unsigned char: 1;
	unsigned char use_hgig_drdm: 1;
	unsigned char maxrgb: 1;
	unsigned char gamut: 2;
	unsigned short red_x;
	unsigned short red_y;
	unsigned short green_x;
	unsigned short green_y;
	unsigned short blue_x;
	unsigned short blue_y;
	unsigned short white_x;
	unsigned short white_y;
	unsigned char min_bright_10;
	unsigned char peak_bright_100;
	unsigned char p0_exp: 2;
	unsigned char p0_mant: 6;
	unsigned char peak_bright_p0;
	unsigned char p1_exp: 2;
	unsigned char p1_mant: 6;
	unsigned char peak_bright_p1;
	unsigned char p2_exp: 2;
	unsigned char p2_mant: 6;
	unsigned char peak_bright_p2;
	unsigned char p3_exp: 2;
	unsigned char p3_mant: 6;
	unsigned char peak_bright_p3;
};

#define HDR_SUP_EOTF_SDR			BIT(0)
#define HDR_SUP_EOTF_HDR			BIT(1)
#define HDR_SUP_EOTF_SMPTE_ST_2084	BIT(2)
#define HDR_SUP_EOTF_HLG			BIT(3)

struct hdr_info {
/* RX EDID hdr support types */
	/* hdr_support: bit0/SDR bit1/HDR bit2/SMPTE2084 bit3/HLG */
	u32 hdr_support;
	/* In CTA-861-I Table 52, static_metadata_type1 use only 1 bit */
	unsigned char static_metadata_type1;
	unsigned char rawdata[7];
/*
 *dynamic_info[0] expresses type1's parameters certainly
 *dynamic_info[1] expresses type2's parameters certainly
 *dynamic_info[2] expresses type3's parameters certainly
 *dynamic_info[3] expresses type4's parameters certainly
 *if some types don't exist, the corresponding dynamic_info
 *is zero instead of inexistence
 */
	struct hdr_dynamic dynamic_info[4];
	struct hdr10_plus_info hdr10plus_info;
/*bit7:BT2020RGB    bit6:BT2020YCC bit5:BT2020cYCC bit4:adobeRGB*/
/*bit3:adobeYCC601 bit2:sYCC601     bit1:xvYCC709    bit0:xvYCC601*/
	u8 colorimetry_support; /* RX EDID colorimetry support types */
	u32 lumi_max; /* RX EDID Lumi Max value */
	u32 lumi_avg; /* RX EDID Lumi Avg value */
	u32 lumi_min; /* RX EDID Lumi Min value */
	u32 lumi_peak; /* RX EDID Lumi Peak value */
	u32 ldim_support; /* RX EDID Local Dimming Support */
	struct cuva_info cuva_info;
	struct sbtm_info sbtm_info; /* TV SBTM EDID capabilities */
};

/************************************************************************/
/* ** BITMAP DISPLAY SUPPORT						*/
/************************************************************************/
#if defined(CONFIG_CMD_BMP) || defined(CONFIG_SPLASH_SCREEN)
# include <bmp_layout.h>
# include <asm/byteorder.h>
#endif

/*
 *  Information about displays we are using. This is for configuring
 *  the LCD controller and memory allocation. Someone has to know what
 *  is connected, as we can't autodetect anything.
 */
#define CONFIG_SYS_HIGH	0	/* Pins are active high			*/
#define CONFIG_SYS_LOW		1	/* Pins are active low			*/

/* Calculate nr. of bits per pixel  and nr. of colors */
#define NBITS(bit_code)		(bit_code)
#define NCOLORS(bit_code)	(1 << NBITS(bit_code))

/************************************************************************/
/* ** CONSOLE CONSTANTS							*/
/************************************************************************/
#if LCD_BPP == LCD_MONOCHROME

/*
 * Simple black/white definitions
 */
# define CONSOLE_COLOR_BLACK	0
# define CONSOLE_COLOR_WHITE	1	/* Must remain last / highest	*/

#elif LCD_BPP == LCD_COLOR8

/*
 * 8bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	0
# define CONSOLE_COLOR_RED	1
# define CONSOLE_COLOR_GREEN	2
# define CONSOLE_COLOR_YELLOW	3
# define CONSOLE_COLOR_BLUE	4
# define CONSOLE_COLOR_MAGENTA	5
# define CONSOLE_COLOR_CYAN	6
# define CONSOLE_COLOR_GREY	14
# define CONSOLE_COLOR_WHITE	15	/* Must remain last / highest	*/

#elif LCD_BPP == LCD_COLOR24
/*
 * 24bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	 0
# define CONSOLE_COLOR_RED 	0x0000ff
# define CONSOLE_COLOR_GREEN	0x00ff00
# define CONSOLE_COLOR_YELLOW	0x00ffff
# define CONSOLE_COLOR_BLUE	0xff0000
# define CONSOLE_COLOR_MAGENTA	0xff00ff
# define CONSOLE_COLOR_CYAN	0xffff00
# define CONSOLE_COLOR_GREY	0x808080
# define CONSOLE_COLOR_WHITE	0xffffff	/* Must remain last / highest	*/

#else

/*
 * 16bpp color definitions
 */
# define CONSOLE_COLOR_BLACK	0x0000
# define CONSOLE_COLOR_RED 		0xf800
# define CONSOLE_COLOR_GREEN	0x07e0
# define CONSOLE_COLOR_YELLOW	0xffe0
# define CONSOLE_COLOR_BLUE		0x001f
# define CONSOLE_COLOR_MAGENTA	0xf81f
# define CONSOLE_COLOR_CYAN		0x07ff
# define CONSOLE_COLOR_WHITE	0xffff	/* Must remain last / highest	*/

#endif /* color definitions */

/************************************************************************/
#ifndef PAGE_SIZE
# define PAGE_SIZE	4096
#endif

/************************************************************************/
/* ** CONSOLE DEFINITIONS & FUNCTIONS					*/
/************************************************************************/
#if defined(CONFIG_LCD_LOGO) && !defined(CONFIG_LCD_INFO_BELOW_LOGO)
# define CONSOLE_ROWS		((info->vl_row-BMP_LOGO_HEIGHT) \
					/ VIDEO_FONT_HEIGHT)
#else
# define CONSOLE_ROWS		(info->vl_row / VIDEO_FONT_HEIGHT)
#endif

#define CONSOLE_COLS		(panel_info.vl_col / VIDEO_FONT_WIDTH)
#define CONSOLE_ROW_SIZE	(VIDEO_FONT_HEIGHT * lcd_line_length)
#define CONSOLE_ROW_FIRST	(info->vd_console_address)
#define CONSOLE_ROW_SECOND	(info->vd_console_address + CONSOLE_ROW_SIZE)
#define CONSOLE_ROW_LAST	(info->vd_console_address + CONSOLE_SIZE \
					- CONSOLE_ROW_SIZE)
#define CONSOLE_SIZE		(CONSOLE_ROW_SIZE * CONSOLE_ROWS)
#define CONSOLE_SCROLL_SIZE	(CONSOLE_SIZE - CONSOLE_ROW_SIZE)

# define COLOR_MASK(c)		(c)

/************************************************************************/

#endif
