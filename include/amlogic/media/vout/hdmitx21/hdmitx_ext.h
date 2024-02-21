/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX21_EXT_H__
#define __HDMITX21_EXT_H__

#include "../hdmitx_common.h"

/* global used by hdmitx21/20, and extern module */
/* -----------------Source Physical Address--------------- */
struct vsdb_phyaddr {
	unsigned char a:4;
	unsigned char b:4;
	unsigned char c:4;
	unsigned char d:4;
	unsigned char valid;
};

/* Sampling Freq Fs:
 * 0 - Refer to Stream Header;
 * 1 - 32KHz;
 * 2 - 44.1KHz;
 * 3 - 48KHz;
 * 4 - 88.2KHz...
 */
enum hdmi_audio_fs {
	FS_REFER_TO_STREAM = 0,
	FS_32K = 1,
	FS_44K1 = 2,
	FS_48K = 3,
	FS_88K2 = 4,
	FS_96K = 5,
	FS_176K4 = 6,
	FS_192K = 7,
	FS_768K = 8,
	FS_MAX,
};

/* HDMI Audio Parmeters */
/* Refer to CEA-861-D Page 88 */
#define DTS_HD_TYPE_MASK 0xff00
#define DTS_HD_MA  (0X1 << 8)
enum hdmi_audio_type {
	CT_REFER_TO_STREAM = 0,
	CT_PCM,
	CT_AC_3, /* DD */
	CT_MPEG1,
	CT_MP3,
	CT_MPEG2,
	CT_AAC,
	CT_DTS,
	CT_ATRAC,
	CT_ONE_BIT_AUDIO,
	CT_DD_P, /* DD+ */
	CT_DTS_HD,
	CT_MAT, /* TrueHD */
	CT_DST,
	CT_WMA,
	CT_CXT = 0xf, /* Audio Coding Extension Type */
	CT_DTS_HD_MA = CT_DTS_HD + (DTS_HD_MA),
	CT_MAX,
	CT_PREPARE, /* prepare for audio mode switching */
};

#define CT_DOLBY_D CT_DD_P

enum hdmi_audio_chnnum {
	CC_REFER_TO_STREAM = 0,
	CC_2CH,
	CC_3CH,
	CC_4CH,
	CC_5CH,
	CC_6CH,
	CC_7CH,
	CC_8CH,
	CC_MAX_CH
};

enum hdmi_audio_format {
	AF_SPDIF = 0, AF_I2S, AF_DSD, AF_HBR, AT_MAX
};

enum hdmi_audio_sampsize {
	SS_REFER_TO_STREAM = 0, SS_16BITS, SS_20BITS, SS_24BITS, SS_MAX
};

enum hdmi_audio_source_if {
	AUD_SRC_IF_SPDIF = 0,
	AUD_SRC_IF_I2S,
	AUD_SRC_IF_TDM, /* for T7 only */
};

/* should sync with sound/soc */
struct aud_para {
	bool prepare; /* when prepare is true, mute tx audio sample */

	/* below parameters will be compared with the previous setting
	 * if different, then call audio HW setting
	 */
	enum hdmi_audio_type type;
	enum hdmi_audio_fs rate;
	enum hdmi_audio_sampsize size;
	enum hdmi_audio_chnnum chs;
	u8 i2s_ch_mask;
	enum hdmi_audio_source_if aud_src_if; /* 0: spdif 1: i2s */

	unsigned char status[24]; /* AES/IEC958 channel status bits */
	/* aud_output_i2s_ch: bit[3:0] ch_msk  bit[7:4] ch_num
	 * configure for I2S: 8ch in, 2ch out
	 * 0: default setting  1:ch0/1  2:ch2/3  3:ch4/5  4:ch6/7
	 */
	u8 aud_output_i2s_ch;
	bool fifo_rst;
	bool aud_output_en; /* 0, off; 1, on */
	bool aud_notify_update;
};

void hdmitx21_init(void);
void hdmitx21_pxp_init(bool pxp_mode);
void hdmitx21_chip_type_init(enum amhdmitx_chip_e type);
//hpd state used by external module
bool hdmitx_get_hpd_state_ext(void);

#endif
