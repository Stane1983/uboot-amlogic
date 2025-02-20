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

#endif
