/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_CMN_H__
#define __HDMITX_CMN_H__

/* the default max_tmds_clk is 165MHz/5 in H14b Table 8-16 */
#define DEFAULT_MAX_TMDS_CLK    33

#define GET_OUI_BYTE0(oui)      ((oui) & 0xff) /* Little Endian */
#define GET_OUI_BYTE1(oui)      (((oui) >> 8) & 0xff)
#define GET_OUI_BYTE2(oui)      (((oui) >> 16) & 0xff)

int hdmitx_likely_frac_rate_mode(char *m);

enum amhdmitx_chip_e {
	MESON_CPU_ID_SC2,
	MESON_CPU_ID_T7,
	MESON_CPU_ID_S1A,
	MESON_CPU_ID_S5,
	MESON_CPU_ID_S7,
	MESON_CPU_ID_S7D,
	MESON_CPU_ID_S6,
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

/* HDMI Audio Parameters */
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

#define AUDIO_PARA_MAX_NUM       14
struct hdmi_audio_fs_ncts {
	struct {
		u32 tmds_clk;
		unsigned int n; /* 24 bit */
		unsigned int cts; /* 24 bit */
		unsigned int n_30bit; /* 30 bit */
		unsigned int cts_30bit; /* 30bit */
		u32 n_36bit;
		u32 cts_36bit;
		u32 n_48bit;
		u32 cts_48bit;
	} array[AUDIO_PARA_MAX_NUM];
	u32 def_n;
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

/* audio api */
u32 hdmitx_hw_get_audio_n_paras(enum hdmi_audio_fs fs,
				int cd,
				u32 tmds_clk);
u32 hdmi21_get_frl_aud_n_paras(enum hdmi_audio_fs fs,
			       u32 frl_rate);

int get_hdr_strategy_priority(void);
/*
 * old processing of 1080p59 and 1080p60
 *                modename      frac_rate_policy
 * 1080p59.94hz   1080p60hz     1
 * 1080p60hz      1080p60hz     0
 *
 * new processing of 1080p59 and 1080p60
 *                modename      frac_rate_policy
 * 1080p59.94hz   1080p59hz     X(ignore)
 * 1080p60hz      1080p60hz     0
 * when do_edid, call hdmi_mode_frac_preprocess() firstly
 * when do_output, call hdmi_mode_frac_postprocess() lastly
 */
void hdmi_mode_frac_preprocess(void);
void hdmi_mode_frac_postprocess(void);
void convert_name_frac2int(const char *name, char *conv_name);
bool is_mode_name_frac(const char *name);
bool get_kernel_dtb_info(void);
/*
 * sync with uboot 2019 follow SWPL-166617
 * When updating outputmodeX env, check whether the connectorX is HDMI
 * to avoid affecting the MIPI screen
 */
int is_valid_hdmi(const char *input);

#endif
