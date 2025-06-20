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
	MESON_CPU_ID_S5,
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
const char *find_non_frac_preferred_mode(const char *mode);

/* audio api */
u32 hdmitx_hw_get_audio_n_paras(enum hdmi_audio_fs fs,
				  int cd,
				  u32 tmds_clk);
u32 hdmi21_get_frl_aud_n_paras(enum hdmi_audio_fs fs,
			       u32 frl_rate);

#endif
