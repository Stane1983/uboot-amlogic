// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/math64.h>
#include <common.h>
#include <amlogic/media/vout/hdmitx21/hdmitx_module.h>
#include "hdmi_param.h"

const struct hdmi_timing *hdmitx21_get_timing_para0(void)
{
	return &hdmi_timing_all[0];
}

int hdmitx21_timing_size(void)
{
	return ARRAY_SIZE(hdmi_timing_all);
}

static struct parse_cd parse_cd_[] = {
	{COLORDEPTH_24B, "8bit",},
	{COLORDEPTH_30B, "10bit"},
	{COLORDEPTH_36B, "12bit"},
	{COLORDEPTH_48B, "16bit"},
};

static struct parse_cs parse_cs_[] = {
	{HDMI_COLORSPACE_RGB, "rgb",},
	{HDMI_COLORSPACE_YUV422, "422",},
	{HDMI_COLORSPACE_YUV444, "444",},
	{HDMI_COLORSPACE_YUV420, "420",},
};

static struct parse_cr parse_cr_[] = {
	{COLORRANGE_LIM, "limit",},
	{COLORRANGE_FUL, "full",},
};

/**
 * sync function drm_mode_vrefresh()
 */
static int hdmi_timing_vrefresh(const struct hdmi_timing *t)
{
	unsigned int num, den;

	if (t->h_total == 0 || t->v_total == 0)
		return 0;

	num = t->pixel_freq;
	den = t->h_total * t->v_total;

	/*interlace mode*/
	if (t->pi_mode == 0)
		num *= 2;

	return DIV_ROUND_CLOSEST_ULL(mul_u32_u32(num, 1000), den);
}

/* parse the name string to cs/cd/cr */
static void _parse_hdmi_attr(char const *name,
	enum hdmi_colorspace *cs,
	enum hdmi_color_depth *cd,
	enum hdmi_color_range *cr)
{
	int i;

	if (!cs || !cd || !cr)
		return;
	if (!name) {
		/* assign defalut value*/
		*cs = HDMI_COLORSPACE_RGB;
		*cd = COLORDEPTH_24B;
		*cr = COLORRANGE_FUL;
		return;
	}

	/* parse color depth */
	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (strstr(name, parse_cd_[i].name)) {
			*cd = parse_cd_[i].cd;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cd_) / sizeof(struct parse_cd))
		*cd = COLORDEPTH_24B;

	/* parse color space */
	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (strstr(name, parse_cs_[i].name)) {
			*cs = parse_cs_[i].cs;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cs_) / sizeof(struct parse_cs))
		*cs = HDMI_COLORSPACE_RGB;

	/* parse color range */
	for (i = 0; i < sizeof(parse_cr_) / sizeof(struct parse_cr); i++) {
		if (strstr(name, parse_cr_[i].name)) {
			*cr = parse_cr_[i].cr;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cr_) / sizeof(struct parse_cr))
		*cr = COLORRANGE_FUL;
}

static u32 _calc_tmds_clk(u32 pixel_freq, enum hdmi_colorspace cs,
	enum hdmi_color_depth cd)
{
	u32 tmds_clk = pixel_freq;

	if (cs == HDMI_COLORSPACE_YUV420)
		tmds_clk = tmds_clk / 2;
	if (cs != HDMI_COLORSPACE_YUV422) {
		switch (cd) {
		case COLORDEPTH_48B:
			tmds_clk *= 2;
			break;
		case COLORDEPTH_36B:
			tmds_clk = tmds_clk * 3 / 2;
			break;
		case COLORDEPTH_30B:
			tmds_clk = tmds_clk * 5 / 4;
			break;
		case COLORDEPTH_24B:
		default:
			break;
		}
	}

	return tmds_clk;
}

static bool _tst_fmt_name(struct hdmi_format_para *para,
	char const *name, char const *attr)
{
	int i;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	enum hdmi_vic prefer_vic = HDMI_0_UNKNOWN;
	char tmp_mode[MODE_LEN] = {0};

	if (!para || !name || !attr)
		return 0;

	if (is_mode_name_frac(name))
		convert_name_frac2int(name, tmp_mode);
	else
		strncpy(tmp_mode, name, MODE_LEN - 1);

	/* check sname first */
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->sname && (strncmp(tmp_mode, timing->sname,
			strlen(timing->sname)) == 0)) {
			para->timing = *timing;
			goto next;
		}
		timing++;
	}

	/* check name */
	timing = hdmitx21_get_timing_para0();
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (strncmp(tmp_mode, timing->name, strlen(timing->name)) == 0) {
			para->timing = *timing;
			break;
		}
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return 0;
next:
	prefer_vic = hdmitx21_get_prefer_vic(hdev, timing->vic);
	timing = hdmitx21_gettiming_from_vic(prefer_vic);
	if (!timing)
		return 0;
	para->timing = *timing;
	_parse_hdmi_attr(attr, &para->cs, &para->cd, &para->cr);

	para->tmds_clk = _calc_tmds_clk(timing->pixel_freq, para->cs, para->cd);

	return 1;
}

const struct hdmi_timing *hdmitx21_match_dtd_timing(struct dtd *t)
{
	int i;
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();

	if (!t)
		return INVALID_HDMI_TIMING;

	/* interlace mode, all vertical timing parameters
	 * are halved, while vactive/vtotal is doubled
	 * in timing table. need double vactive before compare
	 */
	if (t->flags >> 7 == 0x1)
		t->v_active = t->v_active * 2;
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if ((abs(timing->pixel_freq / 10 - t->pixel_clock) <=
			(t->pixel_clock + 1000) / 1000) &&
		    t->h_active == timing->h_active &&
		    t->h_blank == timing->h_blank &&
		    t->v_active == timing->v_active &&
		    t->v_blank == timing->v_blank &&
		    t->h_sync_offset == timing->h_front &&
		    t->h_sync == timing->h_sync &&
		    t->v_sync_offset == timing->v_front &&
		    t->v_sync == timing->v_sync)
			return timing;
		timing++;
	}
	return INVALID_HDMI_TIMING;
}

struct hdmi_format_para *hdmitx21_tst_fmt_name(const char *name,
	const char *attr)
{
	static struct hdmi_format_para para;

	if (!name)
		return NULL;

	if (!attr)
		attr = "rgb,8bit";

	memset(&para, 0, sizeof(para));
	if (_tst_fmt_name(&para, name, attr))
		return &para;
	else
		return NULL;
}

const struct hdmi_timing *hdmitx_mode_match_vesa_timing(struct vesa_standard_timing *t)
{
	int i;
	const struct hdmi_timing *timing;

	if (!t)
		return INVALID_HDMI_TIMING;

	for (i = 0; i < ARRAY_SIZE(vesa_modes); i++) {
		timing = &vesa_modes[i];

		if (t->hactive == timing->h_active &&
		    t->vactive == timing->v_active) {
			if (t->vsync) {
				unsigned int vsync = hdmi_timing_vrefresh(timing);

				if (t->vsync == vsync)
					return timing;
			}
			if (t->hblank &&
			    t->hblank == timing->h_blank &&
			    t->vblank &&
			    t->vblank == timing->v_blank &&
			    t->tmds_clk &&
			    t->tmds_clk == timing->pixel_freq / 10)
				return timing;
		}
	}

	return INVALID_HDMI_TIMING;
}

const struct hdmi_timing *hdmitx21_gettiming_from_vic(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	int i;

	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->vic == vic)
			break;
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return NULL;

	return timing;
}

const struct hdmi_timing *hdmitx21_gettiming_from_name(const char *name)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	int i;
	enum hdmi_vic prefer_vic = HDMI_UNKNOWN;

	/* check sname first */
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->sname && strstr(timing->sname, name))
			goto next;
		timing++;
	}

	timing = hdmitx21_get_timing_para0();
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (strncmp(timing->name, name, strlen(timing->name)) == 0)
			break;
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return NULL;
next:
	prefer_vic = hdmitx21_get_prefer_vic(hdev, timing->vic);
	timing = hdmitx21_gettiming_from_vic(prefer_vic);
	return timing;
}

/*
 * Parameter 'name' can should be full name as 1920x1080p60hz,
 * 3840x2160p60hz, etc
 * attr strings likes as '444,8bit'
 */
struct hdmi_format_para *hdmitx21_get_fmtpara(const char *mode,
	const char *attr)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing;
	struct hdmi_format_para *para = hdev->para;

	if (!mode || !attr)
		return NULL;

	timing = hdmitx21_gettiming_from_name(mode);
	if (!timing)
		return NULL;

	para->timing = *timing;
	_parse_hdmi_attr(attr, &para->cs, &para->cd, &para->cr);

	para->tmds_clk = _calc_tmds_clk(para->timing.pixel_freq,
		para->cs, para->cd);

	return hdev->para;
}

/* For check all format parameters only */
void check21_detail_fmt(void)
{
}

bool _is_hdmi14_4k(enum hdmi_vic vic)
{
	bool ret = 0;

	switch (vic) {
	case HDMI_93_3840x2160p24_16x9:
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_98_4096x2160p24_256x135:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

bool _is_hdmi4k_420(enum hdmi_vic vic)
{
	return 0;
}

/*
 * From CTA-861 spec:
 * Picture Aspect Ratio - ratio of width to height dimension
 * of the Picture as delivered across the uncompressed digital
 * interface, including any top, bottom, or side Bars. Only four
 * Picture Aspect Ratios are specified for this interface:
 * 4:3, 16:9, 64:27, and 256:135;
 *
 * for VESA modes with aspect ratio not in 4:3, 16:9, 64:27, 256:135,
 * return HDMI_PICTURE_ASPECT_NONE
 */
enum hdmi_picture_aspect hdmitx_mode_get_vic_aspect_ratio(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing = hdmitx21_gettiming_from_vic(vic);
	enum hdmi_picture_aspect picture_ar = HDMI_PICTURE_ASPECT_NONE;

	if (vic == HDMI_0_UNKNOWN || !timing)
		return HDMI_PICTURE_ASPECT_NONE;

	if (timing->h_pict == 4 && timing->v_pict == 3)
		picture_ar = HDMI_PICTURE_ASPECT_4_3;
	else if (timing->h_pict == 16 && timing->v_pict == 9)
		picture_ar = HDMI_PICTURE_ASPECT_16_9;
	else if (timing->h_pict == 64 && timing->v_pict == 27)
		picture_ar = HDMI_PICTURE_ASPECT_64_27;
	else if (timing->h_pict == 256 && timing->v_pict == 135)
		picture_ar = HDMI_PICTURE_ASPECT_256_135;
	else
		picture_ar = HDMI_PICTURE_ASPECT_NONE;

	return picture_ar;
}

