// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/stddef.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include "../hdmitx21/hdmitx_drv.h"
#include <amlogic/media/vout/dsc.h>

#define EDID_MAX_BLOCK 8

int hdmitx_edid_VIC_support(enum hdmi_vic vic)
{
	int i;
	int size = hdmitx21_timing_size();
	const struct hdmi_timing *t = hdmitx21_get_timing_para0();

	for (i = 0; i < size; i++) {
		if (vic == t->vic)
			return 1;
		t++;
	}

	return 0;
}

enum hdmi_vic hdmitx_edid_vic_tab_map_vic(const char *disp_mode)
{
	int i;
	enum hdmi_vic vic = HDMI_UNKNOWN;
	int size = hdmitx21_timing_size();
	const struct hdmi_timing *t = hdmitx21_get_timing_para0();

	for (i = 0; i < size; i++) {
		if (t->sname && strncmp(disp_mode, t->sname, strlen(t->sname)) == 0) {
			vic = t->vic;
			break;
		}
		if (strncmp(disp_mode, t->name, strlen(t->name)) == 0) {
			vic = t->vic;
			break;
		}
		t++;
	}

	if (vic == HDMI_UNKNOWN)
		printf("not find mapped vic\n");

	return vic;
}

const char *hdmitx_edid_vic_tab_map_string(enum hdmi_vic vic)
{
	int i;
	const char *disp_str = NULL;
	int size = hdmitx21_timing_size();
	const struct hdmi_timing *t = hdmitx21_get_timing_para0();

	for (i = 0; i < size; i++) {
		if (vic == t->vic) {
			disp_str = t->sname;
			if (!disp_str)
				disp_str = t->name;
			break;
		}
		t++;
	}

	return disp_str;
}

const char *hdmitx_edid_vic_to_string(enum hdmi_vic vic)
{
	return hdmitx_edid_vic_tab_map_string(vic);
}

bool _is_y420_vic(enum hdmi_vic vic)
{
	int i;
	enum hdmi_vic y420_vic[] = {
		HDMI_96_3840x2160p50_16x9,
		HDMI_97_3840x2160p60_16x9,
		HDMI_101_4096x2160p50_256x135,
		HDMI_102_4096x2160p60_256x135,
		HDMI_106_3840x2160p50_64x27,
		HDMI_107_3840x2160p60_64x27,
	};
	const struct hdmi_timing *timing;

	for (i = 0; i < ARRAY_SIZE(y420_vic); i++) {
		if (vic == y420_vic[i])
			return 1;
	}

	/* In Spec2.1 Table 7-36, greater than 2160p30hz will support y420 */
	timing = hdmitx21_gettiming_from_vic(vic);
	if (!timing)
		return 0;

	if (timing->v_active >= 2160 && timing->v_freq > 30000)
		return 1;
	if (timing->v_active >= 4320)
		return 1;

	return 0;
}

static bool is_vic_support_y420(struct hdmitx_dev *hdev, enum hdmi_vic vic)
{
	unsigned int i = 0;
	struct rx_cap *prxcap = &hdev->RXCap;
	bool ret = false;
	const struct hdmi_timing *timing = hdmitx21_gettiming_from_vic(vic);

	if (!timing)
		return ret;

	/* In Spec2.1 Table 7-34, greater than 2160p30hz will support y420 */
	if ((timing->v_active >= 2160 && timing->v_freq > 30000) ||
		timing->v_active >= 4320) {
		for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
			if (prxcap->y420_vic[i]) {
				if (prxcap->y420_vic[i] == vic) {
					ret = true;
					break;
				}
			} else {
				ret = false;
				break;
			}
		}
	}
	return ret;
}

bool hdmitx_mode_validate_y420_vic(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing;

	/* In Spec2.1 Table 7-34, greater than 2160p30hz will support y420 */
	timing = hdmitx_mode_vic_to_hdmi_timing(vic);
	if (!timing)
		return false;
	if (timing->v_active >= 2160 && timing->v_freq > 30000)
		return true;
	if (timing->v_active >= 4320)
		return true;
	return false;
}

static int is_4k_fmt(char *mode)
{
	int i;
	static char const *hdmi4k[] = {
		"2160p",
		"smpte",
		NULL
	};

	for (i = 0; hdmi4k[i]; i++) {
		if (strstr(mode, hdmi4k[i]))
			return 1;
	}
	return 0;
}

static bool is_over_60hz(const struct hdmi_timing *timing)
{
	if (!timing)
		return 1;

	if (timing->v_freq > 60000)
		return 1;

	return 0;
}

/* check the resolution is over 1920x1080 or not */
static bool is_over_1080p(const struct hdmi_timing *timing)
{
	if (!timing)
		return 1;

	if (timing->h_active > 1920 || timing->v_active > 1080)
		return 1;

	return 0;
}

/* test current vic is over 150MHz or not */
static bool is_over_pixel_150mhz(const struct hdmi_timing *timing)
{
	if (!timing)
		return 1;

	if (timing->pixel_freq > 150000)
		return 1;

	return 0;
}

bool is_vic_over_limited_1080p(enum hdmi_vic vic)
{
	const struct hdmi_timing *tp = hdmitx21_gettiming_from_vic(vic);

	if (!tp)
		return 1;

	if (is_over_1080p(tp) || is_over_60hz(tp) ||
		is_over_pixel_150mhz(tp)) {
		pr_err("over limited vic: %d\n", vic);
		return 1;
	}
	return 0;
}

static bool hdmitx_check_4x3_16x9_mode(struct hdmitx_dev *hdev,
		enum hdmi_vic vic)
{
	bool flag = 0;
	int j;
	struct rx_cap *prxcap = NULL;

	prxcap = &hdev->RXCap;
	if (vic == HDMI_2_720x480p60_4x3 ||
		vic == HDMI_6_720x480i60_4x3 ||
		vic == HDMI_17_720x576p50_4x3 ||
		vic == HDMI_21_720x576i50_4x3) {
		for (j = 0; (j < prxcap->VIC_count) && (j < VIC_MAX_NUM); j++) {
			if ((vic + 1) == (prxcap->VIC[j] & 0xff)) {
				flag = 1;
				break;
			}
		}
	} else if (vic == HDMI_3_720x480p60_16x9 ||
			vic == HDMI_7_720x480i60_16x9 ||
			vic == HDMI_18_720x576p50_16x9 ||
			vic == HDMI_22_720x576i50_16x9) {
		for (j = 0; (j < prxcap->VIC_count) && (j < VIC_MAX_NUM); j++) {
			if ((vic - 1) == (prxcap->VIC[j] & 0xff)) {
				flag = 1;
				break;
			}
		}
	}
	return flag;
}

/* For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
bool hdmitx_edid_check_valid_mode(struct hdmitx_dev *hdev,
	struct hdmi_format_para *para)
{
	bool valid = 0;
	struct rx_cap *prxcap = NULL;
	struct dv_info *dv = &hdev->RXCap.dv_info;
	unsigned int rx_max_tmds_clk = 0;
	unsigned int calc_tmds_clk = 0;
	int i = 0;
	int svd_flag = 0;
	int must_frl_flag = 0;
	/* Default max color depth is 24 bit */
	enum hdmi_color_depth rx_y444_max_dc = COLORDEPTH_24B;
	enum hdmi_color_depth rx_rgb_max_dc = COLORDEPTH_24B;
	u32 rx_frl_bandwidth = 0;
	u32 tx_frl_bandwidth = 0;
	/* maximum supported bandwidth of soc */
	u32 tx_bandwidth_cap = 0;
	const struct hdmi_timing *timing;

	if (!hdev || !para)
		return 0;

	prxcap = &hdev->RXCap;

	if (para->sname && strcmp(para->sname, "invalid") == 0)
		return 0;
	/* if current limits to 1080p, here will check the freshrate and
	 * 4k resolution
	 */
	if (is_hdmitx_limited_1080p()) {
		if (is_vic_over_limited_1080p(para->timing.vic)) {
			printf("over limited vic%d in %s\n", para->timing.vic, __func__);
			return 0;
		}
	}
	/* add efuse ctrl */
	if (hdev->efuse_dis_output_4k)
		if (para->timing.v_active >= 2160)
			return false;
	if (hdev->efuse_dis_hdmi_4k60)
		if (para->timing.v_active >= 2160 && para->timing.v_freq >= 50000)
			return false;

	if (!is_support_4k() && para->sname && is_4k_fmt(para->sname))
		return false;
	/* exclude such as: 2160p60hz YCbCr444 10bit */
	switch (para->timing.vic) {
	case HDMI_96_3840x2160p50_16x9:
	case HDMI_97_3840x2160p60_16x9:
	case HDMI_101_4096x2160p50_256x135:
	case HDMI_102_4096x2160p60_256x135:
	case HDMI_106_3840x2160p50_64x27:
	case HDMI_107_3840x2160p60_64x27:
		if (para->cs == HDMI_COLORSPACE_RGB ||
		    para->cs == HDMI_COLORSPACE_YUV444)
			if (para->cd != COLORDEPTH_24B &&
				(prxcap->max_frl_rate == FRL_NONE ||
				hdev->tx_max_frl_rate == FRL_NONE))
				return 0;
		break;
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
		if (para->cs == HDMI_COLORSPACE_YUV422)
			return 0;
		break;
	default:
		break;
	}

	/* DVI case, only rgb,8bit */
	if (prxcap->ieeeoui != HDMI_IEEE_OUI) {
		if (para->cd != COLORDEPTH_24B || para->cs != HDMI_COLORSPACE_RGB)
			return 0;
	}
	/* target mode is not contained at RX SVD */
	for (i = 0; (i < prxcap->VIC_count) && (i < VIC_MAX_NUM); i++) {
		if ((para->timing.vic & 0xff) == (prxcap->VIC[i] & 0xff)) {
			svd_flag = 1;
			break;
		} else if (hdmitx_check_4x3_16x9_mode(hdev, para->timing.vic & 0xff)) {
			svd_flag = 1;
			break;
		}
	}
	if (svd_flag == 0)
		return 0;

	/* Get RX Max_TMDS_Clock */
	if (prxcap->Max_TMDS_Clock2) {
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
	} else {
		/* Default min is 74.25 / 5 */
		if (prxcap->Max_TMDS_Clock1 < 0xf)
			prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
	}

	/* if current status already limited to 1080p, so here also needs to
	 * limit the rx_max_tmds_clk as 150 * 1.5 = 225 to make the valid mode
	 * checking works
	 */
	if (is_hdmitx_limited_1080p()) {
		if (rx_max_tmds_clk > 225)
			rx_max_tmds_clk = 225;
	}
	calc_tmds_clk = para->tmds_clk / 1000;
	rx_frl_bandwidth = get_frl_bandwidth(prxcap->max_frl_rate);
	timing = hdmitx21_gettiming_from_vic(para->timing.vic);
	if (!timing)
		return 0;

	/* more than 4k60 must use frl mode */
	if (timing->h_active > 4096 || timing->v_active > 2160 ||
	timing->v_freq == 48000 || calc_tmds_clk > 594 ||
	timing->pixel_freq / 1000 > 600)
		must_frl_flag = 1;

	if (prxcap->max_frl_rate == FRL_NONE) {
		if (must_frl_flag)
			return 0;
	}
	if (hdev->tx_max_frl_rate == FRL_NONE) {
		if (must_frl_flag)
			return 0;
		/* Used for S1A to judge whether it exceeds the chip support */
		/* if (hdev->data.chip_type == MESON_CPU_ID_S1A) { */
		/* tx_bandwidth_cap = 225; */
		/* if (calc_tmds_clk > tx_bandwidth_cap) */
		/*	return 0; */
		/* } */
		if (calc_tmds_clk > rx_max_tmds_clk)
			return 0;
	} else {
#ifdef CONFIG_AML_DSC_ENC
		if (hdev->dsc_policy == 1) {
			if (edid_check_dsc_support(&hdev->txcap, prxcap, para, hdev->dsc_policy))
				return 1;
		} else if (hdev->dsc_policy == 2) {
			/* for debug test */
			return 1;
		}
#endif
		if (!must_frl_flag) {
			/* used TMDS, calc_tmds_clk must less than 594, not*/
			/* need repeat judgment(calc_tmds_clk < tx_bandwidth_cap)*/
			if (calc_tmds_clk > rx_max_tmds_clk)
				return 0;
		} else {
			/* try to check if able to run under FRL mode */
			/* tx_frl_bandwidth = timing->pixel_freq / 1000 * 24 * 1.122 */
			tx_frl_bandwidth = calc_frl_bandwidth(timing->pixel_freq / 1000,
				para->cs, para->cd);
			tx_bandwidth_cap = get_frl_bandwidth(hdev->tx_max_frl_rate);
			if (prxcap->dsc_1p2 == 0) {
				if (tx_frl_bandwidth > tx_bandwidth_cap)
					return 0;
				else if (tx_frl_bandwidth > rx_frl_bandwidth)
					return 0;
			} else {
				if (tx_frl_bandwidth <= tx_bandwidth_cap &&
				    tx_frl_bandwidth <= rx_frl_bandwidth)
					; // non-dsc bandwidth is within cap, continue check
#ifdef CONFIG_AML_DSC_ENC
				else if (hdev->dsc_policy == 3) //forcely filter out dsc mode output
					return 0;
				else if (!edid_check_dsc_support(&hdev->txcap, prxcap, para, hdev->dsc_policy))
					return 0;
#endif
			}
			valid = 1;
		}
	}

	if (para->cs == HDMI_COLORSPACE_YUV444) {
		/* Rx may not support Y444 */
		if (!(prxcap->native_Mode & (1 << 5)))
			return 0;
		if ((prxcap->dc_y444 && prxcap->dc_30bit) ||
		    dv->sup_10b_12b_444 == 0x1)
			rx_y444_max_dc = COLORDEPTH_30B;
		if ((prxcap->dc_y444 && prxcap->dc_36bit) ||
		    dv->sup_10b_12b_444 == 0x2)
			rx_y444_max_dc = COLORDEPTH_36B;
		if (para->cd <= rx_y444_max_dc)
			valid = 1;
		else
			valid = 0;
		return valid;
	}
	if (para->cs == HDMI_COLORSPACE_YUV422) {
		/* Rx may not support Y422 */
		if (!(prxcap->native_Mode & (1 << 4)))
			return 0;
		return 1;
	}
	if (para->cs == HDMI_COLORSPACE_RGB) {
		/* Always assume RX supports RGB444 */
		if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1)
			rx_rgb_max_dc = COLORDEPTH_30B;
		if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2)
			rx_rgb_max_dc = COLORDEPTH_36B;
		if (para->cd <= rx_rgb_max_dc)
			valid = 1;
		else
			valid = 0;
		return valid;
	}
	if (para->cs == HDMI_COLORSPACE_YUV420) {
		if (!is_vic_support_y420(hdev, para->timing.vic))
			return 0;
		if (!prxcap->dc_30bit_420)
			if (para->cd == COLORDEPTH_30B)
				return 0;
		if (!prxcap->dc_36bit_420)
			if (para->cd == COLORDEPTH_36B)
				return 0;
		valid = 1;
	}

	return valid;
}

static bool pre_process_str(char *name)
{
	int i;
	unsigned int flag = 0;
	char *color_format[4] = {"444", "422", "420", "rgb"};

	for (i = 0 ; i < 4 ; i++) {
		if (strstr(name, color_format[i]))
			flag++;
	}
	if (flag >= 2)
		return false;
	else
		return true;
}

bool is_supported_mode_attr(hdmi_data_t *hdmi_data, char *mode_attr)
{
	struct hdmi_format_para *para = NULL;
	struct hdmitx_dev *hdev = NULL;

	if (!hdmi_data || !mode_attr)
		return false;
	hdev = container_of(hdmi_data->prxcap,
			struct hdmitx_dev, RXCap);

	if (mode_attr[0]) {
		if (!pre_process_str(mode_attr))
			return false;
		para = hdmitx21_tst_fmt_name(mode_attr, mode_attr);
	}
#if(0)
	if (para) {
		printf("sname = %s\n", para->sname);
		printf("char_clk = %d\n", para->tmds_clk);
		printf("cd = %d\n", para->cd);
		printf("cs = %d\n", para->cs);
	}
#endif
	return hdmitx_edid_check_valid_mode(hdev, para);
}

bool hdmitx_chk_mode_attr_sup(hdmi_data_t *hdmi_data, char *mode, char *attr)
{
	struct hdmi_format_para *para = NULL;
	struct hdmitx_dev *hdev = NULL;

	if (!hdmi_data || !mode || !attr)
		return false;
	hdev = container_of(hdmi_data->prxcap,
			struct hdmitx_dev, RXCap);

	if (attr[0]) {
		if (!pre_process_str(attr))
			return false;
		para = hdmitx21_tst_fmt_name(mode, attr);
	}
	/* if (para) { */
		/* printf("sname = %s\n", para->sname); */
		/* printf("char_clk = %d\n", para->tmds_clk); */
		/* printf("cd = %d\n", para->cd); */
		/* printf("cs = %d\n", para->cs); */
	/* } */

	return hdmitx_edid_check_valid_mode(hdev, para);
}

/* force_flag: 0 means check with RX's edid */
/* 1 means no check with RX's edid */
enum hdmi_vic hdmitx_edid_get_VIC(struct hdmitx_dev *hdev,
	const char *disp_mode, char force_flag)
{
	struct rx_cap *prxcap = &hdev->RXCap;
	int j;
	enum hdmi_vic vic = hdmitx_edid_vic_tab_map_vic(disp_mode);

	if (vic != HDMI_UNKNOWN) {
		if (force_flag == 0) {
			for (j = 0 ; j < prxcap->VIC_count ; j++) {
				if (prxcap->VIC[j] == vic)
					break;
			}
			if (j >= prxcap->VIC_count)
				vic = HDMI_UNKNOWN;
		}
	}
	return vic;
}


static bool hdmitx_edid_notify_ng(unsigned char *buf)
{
	if (!buf)
		return true;
	return hdmitx_edid_check_data_valid(0, buf) == 0;
	/* notify EDID NG to systemcontrol */
	/* if (hdmitx_check_edid_all_zeros(buf)) { */
		/* printf("ERR: edid all zero\n"); */
		/* return true; */
	/* } else if ((buf[0x7e] > 3) && */
		/* hdmitx_edid_header_invalid(buf)) { */
		/* printf("ERR: edid header invalid\n"); */
		/* return true; */
	/* } */
	/* may extend NG case here */

	/* return false; */
}

bool edid_parsing_ok(struct hdmitx_dev *hdev)
{
	if (!hdev)
		return false;

	if (hdmitx_edid_notify_ng(hdev->rawedid))
		return false;
	return true;
}

bool hdmitx_edid_check_y420_support(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	unsigned int i = 0;
	bool ret = false;
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing || !prxcap)
		return false;

	if (hdmitx_mode_validate_y420_vic(vic)) {
		for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
			if (prxcap->y420_vic[i]) {
				if (prxcap->y420_vic[i] == vic) {
					ret = true;
					break;
				}
			} else {
				ret = false;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_AML_DSC_ENC

/* get the needed frl rate, refer to 2.1 spec table 7-37/38,
 * actually it may also need to check bpp
 */
enum frl_rate_enum get_dsc_frl_rate(enum dsc_encode_mode dsc_mode)
{
	enum frl_rate_enum frl_rate = FRL_RATE_MAX;

	switch (dsc_mode) {
	case DSC_RGB_3840X2160_60HZ:
	case DSC_YUV444_3840X2160_60HZ:
	case DSC_YUV422_3840X2160_60HZ:
	case DSC_YUV420_3840X2160_60HZ:
	case DSC_RGB_3840X2160_50HZ:
	case DSC_YUV444_3840X2160_50HZ:
	case DSC_YUV422_3840X2160_50HZ:
	case DSC_YUV420_3840X2160_50HZ:
		frl_rate = FRL_3G3L;
		break;
	case DSC_RGB_3840X2160_120HZ:
	case DSC_YUV444_3840X2160_120HZ:
	case DSC_RGB_3840X2160_100HZ:
	case DSC_YUV444_3840X2160_100HZ:
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_3840X2160_120HZ:
	case DSC_YUV420_3840X2160_120HZ:
	case DSC_YUV422_3840X2160_100HZ:
	case DSC_YUV420_3840X2160_100HZ:
		frl_rate = FRL_3G3L;
		break;

	case DSC_RGB_7680X4320_60HZ:
	case DSC_YUV444_7680X4320_60HZ:
		/* 6G4L is spec recommended, but actually it can't
		 * work on board, need to work under 8G4L
		 */
		frl_rate = FRL_6G4L;
		break;
	case DSC_YUV422_7680X4320_60HZ:
	case DSC_YUV420_7680X4320_60HZ:
		/* 6G3L is spec recommended, but actually it can't
		 * work on board, need to work under 6G4L
		 */
		frl_rate = FRL_6G3L;
		break;

	case DSC_RGB_7680X4320_50HZ:
	case DSC_YUV444_7680X4320_50HZ:
		frl_rate = FRL_6G4L;
		break;
	case DSC_YUV422_7680X4320_50HZ:
	case DSC_YUV420_7680X4320_50HZ:
		frl_rate = FRL_6G3L;
		break;

	case DSC_YUV444_7680X4320_30HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_30HZ: /* bpp = 12 */
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_7680X4320_30HZ: /* bpp = 7.375 */
	case DSC_YUV420_7680X4320_30HZ: /* bpp = 7.375 */
		/* 3G3L is spec recommended, but actually it can't
		 * work on board, need to work under 6G3L
		 */
		frl_rate = FRL_3G3L;
		break;

	case DSC_YUV444_7680X4320_25HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_25HZ: /* bpp = 12 */
	case DSC_YUV444_7680X4320_24HZ: /* bpp = 12 */
	case DSC_RGB_7680X4320_24HZ: /* bpp = 12 */
		frl_rate = FRL_6G3L;
		break;
	case DSC_YUV422_7680X4320_25HZ: /* bpp = 7.6875 */
	case DSC_YUV420_7680X4320_25HZ: /* bpp = 7.6875 */
	case DSC_YUV422_7680X4320_24HZ: /* bpp = 7.6875 */
	case DSC_YUV420_7680X4320_24HZ: /* bpp = 7.6875 */
		frl_rate = FRL_3G3L;
		break;
	case DSC_ENCODE_MAX:
	default:
		frl_rate = FRL_RATE_MAX;
		break;
	}
	return frl_rate;
}

bool edid_check_dsc_support(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *rxcap, struct hdmi_format_para *para, u8 dsc_policy)
{
	enum dsc_encode_mode dsc_mode = DSC_ENCODE_MAX;
	u8 dsc_slice_num = 0;
	enum frl_rate_enum dsc_frl_rate = FRL_NONE;
	u32 bytes_target = 0;

	if (!hdmi_tx_cap || !rxcap || !para)
		return false;

	/* step1: check if DSC mode is supported by SOC driver & policy */
	if (!hdmi_tx_cap->dsc_capable) {
		pr_info("tx not capable of dsc\n");
		return false;
	}

	dsc_mode = dsc_enc_confirm_mode(para->timing.h_active,
		para->timing.v_active, para->timing.v_freq, para->cs);

	if (dsc_mode == DSC_ENCODE_MAX) {
		pr_info("dsc mode not supported!\n");
		return false;
	}
	if (dsc_policy == 0) {
		/* force not support below 12bit format temporarily */
		switch (dsc_mode) {
		/* 4k120hz */
		case DSC_RGB_3840X2160_120HZ:
		case DSC_YUV444_3840X2160_120HZ:
		/* 4k100hz */
		case DSC_RGB_3840X2160_100HZ:
		case DSC_YUV444_3840X2160_100HZ:
		/* 8k60hz */
		case DSC_RGB_7680X4320_60HZ:
		case DSC_YUV444_7680X4320_60HZ:
		/* 8k50hz */
		case DSC_RGB_7680X4320_50HZ:
		case DSC_YUV444_7680X4320_50HZ:
		/* 8k24hz */
		case DSC_RGB_7680X4320_24HZ:
		case DSC_YUV444_7680X4320_24HZ:
		/* 8k25hz */
		case DSC_RGB_7680X4320_25HZ:
		case DSC_YUV444_7680X4320_25HZ:
		/* 8k30hz */
		case DSC_RGB_7680X4320_30HZ:
		case DSC_YUV444_7680X4320_30HZ:
			if (para->cd == COLORDEPTH_36B)
				return false;
			break;
		default:
			break;
		}
	}

	/* step2: check if DSC mode is supported by RX */
	if (rxcap->dsc_1p2 == 0) {
		pr_info("RX not support DSC\n");
		return false;
	}
	/* check dsc color depth cap */
	if (para->cd == COLORDEPTH_30B &&
		!rxcap->dsc_10bpc) {
		pr_info("RX not support 10bpc DSC\n");
		return false;
	} else if (para->cd == COLORDEPTH_36B &&
		!rxcap->dsc_12bpc) {
		pr_info("RX not support 12bpc DSC\n");
		return false;
	}
	/* check dsc color space cap */
	if (para->cs == HDMI_COLORSPACE_YUV420 &&
		!rxcap->dsc_native_420) {
		pr_info("RX not support Y420 DSC\n");
		return false;
	}
	dsc_slice_num = dsc_get_slice_num(dsc_mode);
	/* slice num exceed rx cap */
	if (dsc_slice_num == 0 ||
		dsc_slice_num > dsc_max_slices_num[rxcap->dsc_max_slices]) {
		pr_info("current slice num %d exceed rx cap %d\n",
			dsc_slice_num, dsc_max_slices_num[rxcap->dsc_max_slices]);
		return false;
	}
	/* note: pixel clock per slice not checked, assume
	 * it's always within rx cap
	 */
	/* check dsc frl rate within rx cap */
	dsc_frl_rate = get_dsc_frl_rate(dsc_mode);
	if (dsc_frl_rate == FRL_RATE_MAX ||
		dsc_frl_rate > rxcap->dsc_max_frl_rate ||
		dsc_frl_rate > rxcap->max_frl_rate) {
		pr_info("current dsc frl rate %d exceed rx cap %d-%d\n",
			dsc_frl_rate, rxcap->dsc_max_frl_rate, rxcap->max_frl_rate);
		return false;
	}
	/* 2.1 spec table 6-56, if Bytes Target is greater than
	 * the value indicated by DSC_TotalChunkKBytes (see Sections
	 * 7.7.1 and 7.7.4.2), then the configuration is not
	 * supported with Compressed Video Transport.
	 */
	bytes_target = dsc_get_bytes_target_by_mode(dsc_mode);
	if (bytes_target > (rxcap->dsc_total_chunk_bytes + 1) * 1024) {
		pr_info("bytes_target %d exceed DSC_TotalChunkKBytes %d\n",
			bytes_target, (rxcap->dsc_total_chunk_bytes + 1) * 1024);
		return false;
	}
	return true;
}
#endif

/* only check vic in edid */
bool hdmitx_edid_validate_mode(struct rx_cap *rxcap, u32 vic)
{
	int i = 0;
	bool edid_matched = false;

	if (!rxcap)
		return false;

	if (vic < HDMITX_VESA_OFFSET) {
		/*check cea cap*/
		for (i = 0 ; i < rxcap->VIC_count; i++) {
			if (rxcap->VIC[i] == vic) {
				edid_matched = true;
				break;
			}
		}
	} else {
		enum hdmi_vic *vesa_t = &rxcap->vesa_timing[0];
		/*check vesa mode.*/
		for (i = 0; i < VESA_MAX_TIMING && vesa_t[i]; i++) {
			if (vic == vesa_t[i]) {
				edid_matched = true;
				break;
			}
		}
	}

	return edid_matched;
}

#if(0)
/* For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
int hdmitx_edid_validate_format_para(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *prxcap, struct hdmi_format_para *para, u8 dsc_policy)
{
	const struct dv_info *dv;
	/* needed tmds clk bandwidth for current format */
	unsigned int calc_tmds_clk = 0;
	/* max tmds clk supported by RX */
	unsigned int rx_max_tmds_clk = 0;
	/* bandwidth needed for current FRL mode */
	u32 tx_frl_bandwidth = 0;
	/* maximum supported frl bandwidth of RX */
	u32 rx_frl_bandwidth_cap = 0;
	/* maximum supported frl bandwidth of soc */
	u32 tx_frl_bandwidth_cap = 0;
	bool must_frl_flag = 0;
	int ret = 0;

	if (!hdmi_tx_cap || !prxcap || !para)
		return -EPERM;

	dv = &prxcap->dv_info;
	/* step1: check if mode + cs/cd is supported by TX */
	switch (para->timing.vic) {
	/* Note: below check for formats which should use FRL
	 * is also checked in step3, so remove
	 */
	/* case HDMI_96_3840x2160p50_16x9: */
	/* case HDMI_97_3840x2160p60_16x9: */
	/* case HDMI_101_4096x2160p50_256x135: */
	/* case HDMI_102_4096x2160p60_256x135: */
	/* case HDMI_106_3840x2160p50_64x27: */
	/* case HDMI_107_3840x2160p60_64x27: */
		/* if (para->cs == HDMI_COLORSPACE_RGB || */
		    /* para->cs == HDMI_COLORSPACE_YUV444) */
			/* if (para->cd != COLORDEPTH_24B && */
				/* (prxcap->max_frl_rate == FRL_NONE || */
				/* hdmi_tx_cap->tx_max_frl_rate == FRL_NONE)) */
				/* return -EPERM; */
		/* break; */
	case HDMI_6_720x480i60_4x3:
	case HDMI_7_720x480i60_16x9:
	case HDMI_21_720x576i50_4x3:
	case HDMI_22_720x576i50_16x9:
		if (para->cs == HDMI_COLORSPACE_YUV422)
			return -EPERM;
		break;
	/* don't support 640x480p60 */
	case HDMI_1_640x480p60_4x3:
		return -EPERM;
	default:
		break;
	}

	/* step2: DVI case, only rgb,8bit */
	if (prxcap->ieeeoui != HDMI_IEEE_OUI) {
		if (para->cd != COLORDEPTH_24B || para->cs != HDMI_COLORSPACE_RGB) {
			pr_info("cs:%d, cd:%d not support by DVI sink\n",
				para->cs, para->cd);
			return -EPERM;
		}
	}

	/* step3: check TMDS/FRL bandwidth is within TX/RX cap */
	if (prxcap->Max_TMDS_Clock2) {
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
	} else {
		/* Default min is 74.25 / 5 */
		if (prxcap->Max_TMDS_Clock1 < 0xf)
			prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
	}
	calc_tmds_clk = para->tmds_clk / 1000;

	/* TODO move to SOC HW check*/
	/* more > 4k60 must use frl mode */
	if (para->timing.h_active > 4096 || para->timing.v_active > 2160 ||
		para->timing.v_freq == 48000 || calc_tmds_clk > 594 ||
		para->timing.pixel_freq / 1000 > 600)
		must_frl_flag = true;

	if (hdmi_tx_cap->tx_max_frl_rate == FRL_NONE) {
		/* output format need FRL while SOC not support FRL */
		if (must_frl_flag) {
			pr_info("output format need FRL, while tx not support\n");
			return -EPERM;
		}
		/* tmds clk of the output format exceed TX/RX cap */
		if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
			pr_info("output tmds clk:%d exceed tx cap: %d\n",
				calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
			return -EPERM;
		}
		if (calc_tmds_clk > rx_max_tmds_clk) {
			pr_info("output tmds clk:%d exceed rx cap: %d\n",
				calc_tmds_clk, rx_max_tmds_clk);
			return -EPERM;
		}
	} else {
#ifdef CONFIG_AML_DSC_ENC
		if (dsc_policy == 1) {
			/* force enable policy */
			if (edid_check_dsc_support(hdmi_tx_cap, prxcap, para, dsc_policy))
				return 0;
		} else if (dsc_policy == 2) {
			/* for debug test */
			return 0;
		}
#endif
		if (!must_frl_flag) {
			if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
				pr_info("output tmds clk:%d exceed tx cap: %d\n",
					calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
				return -EPERM;
			}
			if (calc_tmds_clk > rx_max_tmds_clk) {
				pr_info("output tmds clk:%d exceed rx cap: %d\n",
					calc_tmds_clk, rx_max_tmds_clk);
				return -EPERM;
			}
		} else {
			/* try to check if able to run under FRL mode */

			/* output format need FRL while RX not support FRL
			 * no need below check, it will be checked with rx_frl_bandwidth_cap
			 */
			if (prxcap->max_frl_rate == FRL_NONE) {
				pr_info("output format need FRL, while rx not support\n");
				return -EPERM;
			}
			/* tx_frl_bandwidth = timing->pixel_freq / 1000 * 24 * 1.122 */
			tx_frl_bandwidth = hdmitx_calc_frl_bandwidth(para->timing.pixel_freq / 1000,
				para->cs, para->cd);
			tx_frl_bandwidth_cap =
				hdmitx_get_frl_bandwidth(hdmi_tx_cap->tx_max_frl_rate);
			rx_frl_bandwidth_cap = hdmitx_get_frl_bandwidth(prxcap->max_frl_rate);

			if (prxcap->dsc_1p2 == 0) {
				/* RX not support DSC */
				if (tx_frl_bandwidth > tx_frl_bandwidth_cap) {
					pr_info("frl bandwitch:%d exceed tx_cap:%d\n",
						tx_frl_bandwidth, tx_frl_bandwidth_cap);
					return -EPERM;
				}
				if (tx_frl_bandwidth > rx_frl_bandwidth_cap) {
					pr_info("frl bandwitch:%d exceed rx_cap:%d\n",
						tx_frl_bandwidth, rx_frl_bandwidth_cap);
					return -EPERM;
				}
			} else {
				if (tx_frl_bandwidth <= tx_frl_bandwidth_cap &&
					tx_frl_bandwidth <= rx_frl_bandwidth_cap)
					; // non-dsc bandwidth is within cap, continue check
#ifdef CONFIG_AML_DSC_ENC
				else if (dsc_policy == 3) //forcely filter out dsc mode output
					return -EPERM;
				else if (!edid_check_dsc_support(hdmi_tx_cap, prxcap,
					para, dsc_policy))
					return -EPERM;
#else
				else
					return -EPERM;
#endif
			}
		}
	}

	/* step4: check color space/depth is within RX cap */
	if (para->cs == HDMI_COLORSPACE_YUV444) {
		enum hdmi_color_depth rx_y444_max_dc = COLORDEPTH_24B;
		/* Rx may not support Y444 */
		if (!(prxcap->native_Mode & (1 << 5)))
			return -EACCES;
		if (prxcap->dc_y444 && (prxcap->dc_30bit ||
					dv->sup_10b_12b_444 == 0x1))
			rx_y444_max_dc = COLORDEPTH_30B;
		if (prxcap->dc_y444 && (prxcap->dc_36bit ||
					dv->sup_10b_12b_444 == 0x2))
			rx_y444_max_dc = COLORDEPTH_36B;

		if (para->cd <= rx_y444_max_dc)
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_YUV422) {
		/* Rx may not support Y422 */
		if (prxcap->native_Mode & (1 << 4))
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_RGB) {
		enum hdmi_color_depth rx_rgb_max_dc = COLORDEPTH_24B;
		/* Always assume RX supports RGB444 */
		if (prxcap->dc_30bit || dv->sup_10b_12b_444 == 0x1)
			rx_rgb_max_dc = COLORDEPTH_30B;
		if (prxcap->dc_36bit || dv->sup_10b_12b_444 == 0x2)
			rx_rgb_max_dc = COLORDEPTH_36B;

		if (para->cd <= rx_rgb_max_dc)
			ret = 0;
		else
			ret = -EACCES;

		return ret;
	}

	if (para->cs == HDMI_COLORSPACE_YUV420) {
		ret = 0;
		if (!hdmitx_edid_check_y420_support(prxcap, para->vic))
			ret = -EACCES;
		else if (!prxcap->dc_30bit_420 && para->cd == COLORDEPTH_30B)
			ret = -EACCES;
		else if (!prxcap->dc_36bit_420 && para->cd == COLORDEPTH_36B)
			ret = -EACCES;

		return ret;
	}

	return -EACCES;
}
#endif

bool hdmitx_edid_only_support_sd(struct rx_cap *prxcap)
{
	enum hdmi_vic vic;
	u32 i, j;
	bool only_support_sd = true;
	/* EDID of SL8800 equipment only support below formats */
	static enum hdmi_vic sd_fmt[] = {
		1, 3, 4, 17, 18
	};

	if (!prxcap)
		return false;

	for (i = 0; i < prxcap->VIC_count; i++) {
		vic = prxcap->VIC[i];
		for (j = 0; j < ARRAY_SIZE(sd_fmt); j++) {
			if (vic == sd_fmt[j])
				break;
		}
		if (j == ARRAY_SIZE(sd_fmt)) {
			only_support_sd = false;
			break;
		}
	}

	return only_support_sd;
}

