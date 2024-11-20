// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <amlogic/media/vout/aml_vout.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include <amlogic/auge_sound.h>
#include <linux/arm-smccc.h>
#include <malloc.h>
#include "hdmitx_drv.h"
#include <../hdmitx_common/hdmitx_check_valid.h>
#include "../hdmitx_common/hdmitx_log.h"

static const u16 vsync_tfr_table[TFR_MAX] = {
	[TFR_QMSVRR_INACTIVE] = 0,
	[TFR_23P97] = 2398,
	[TFR_24] = 2400,
	[TFR_25] = 2500,
	[TFR_29P97] = 2997,
	[TFR_30] = 3000,
	[TFR_47P95] = 4795,
	[TFR_48] = 4800,
	[TFR_50] = 5000,
	[TFR_59P94] = 5994,
	[TFR_60] = 6000,
	[TFR_100] = 10000,
	[TFR_119P88] = 11988,
	[TFR_120] = 12000,
};

static enum TARGET_FRAME_RATE vsync_match_to_tfr(const u16 duration)
{
	int i;

	for (i = 0; i < TFR_MAX; i++) {
		if (duration == vsync_tfr_table[i])
			break;
	}

	if (i == TFR_MAX)
		return TFR_QMSVRR_INACTIVE;
	return (enum TARGET_FRAME_RATE)i;
}

void vrr_init_qms_para(struct hdmitx_dev *hdev)
{
	struct emp_packet_st qms_pkt;
	struct emp_packet_st *vrr_pkt = &qms_pkt;
	u16 brr_rate = 60;
	u16 brr_vfront;
	const struct hdmi_timing *timing = NULL;
	const struct hdmi_timing *tfr_timing = NULL;
	u32 tfr_vtotal = 0;
	u32 tfr_vsync = 0;
	bool frac_rate = 0;
	char *mode = NULL;

	if (!hdev->qms_en)
		return;
	timing = hdmitx21_gettiming_from_vic(hdev->brr_vic);
	if (!timing) {
		pr_info("hdmitx: qms: can't find timing for BRR VIC %d\n", hdev->brr_vic);
		return;
	}
	mode = env_get("tfr_mode");
	if (mode)
		tfr_timing = hdmitx21_gettiming_from_name(mode);
	if (!tfr_timing) {
		pr_info("hdmitx: qms: failed to init para %s", mode);
		return;
	}
	frac_rate = env_get_ulong("frac_rate_policy", 10, 0);
	pr_info("hdmitx: qms: set tfr %s parameters\n", mode);
	brr_rate = timing->v_freq / 1000;

	brr_vfront = timing->v_front;
	tfr_vtotal = timing->v_total * timing->v_freq / tfr_timing->v_freq;
	tfr_vsync = tfr_timing->v_freq / 10;
	if (frac_rate && (tfr_timing->v_freq % 6 == 0)) {
		tfr_vtotal = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(tfr_vtotal, 1001), 1000);
		tfr_vsync = DIV_ROUND_CLOSEST_ULL(mul_u32_u32(tfr_vsync, 1000), 1001);
	}

	memset(vrr_pkt, 0, sizeof(*vrr_pkt));
	/* FIXED VALUE */
	vrr_pkt->type = EMP_TYPE_VRR_QMS;
	hdmi_emp_frame_set_member(vrr_pkt, CONF_HEADER_INIT,
				  HDMI_INFOFRAME_TYPE_EMP);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_HEADER_FIRST, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_HEADER_LAST, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_HEADER_SEQ_INDEX, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_DS_TYPE, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_SYNC, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_VFR, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_AFR, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_NEW, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_END, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_ORG_ID, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_DATA_SET_TAG, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_DATA_SET_LENGTH, 4);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_M_CONST, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_QMS_EN, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_NEXT_TFR, vsync_match_to_tfr(tfr_vsync));
	hdmi_emp_frame_set_member(vrr_pkt, CONF_BASE_VFRONT, brr_vfront);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_BASE_REFRESH_RATE, brr_rate);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_M_CONST, 1);
	hdmi_emp_infoframe_set(EMP_TYPE_VRR_QMS, vrr_pkt);
	hdmitx_vrr_set_maxlncnt(tfr_vtotal);
}

/*
 * if rate is a multiple of 6, then reduce 0.1%
 * A Video Timing with a vertical frequency that is an integer multiple of
 * 6.00 Hz (e.g., 24.00 or 120.00 Hz) is considered to be the same as a
 * Video Timing with the equivalent detailed timing information but where the
 * vertical frequency is adjusted by a factor of 1000/1001 (e.g., 24/1.001
 * or 120/1.001).
 */
static u32 reduce_0p1_percent(u32 value)
{
	/* the max value is 120000, so multiply with 1000 won't overflow */
	if (value % 6 == 0)
		return DIV_ROUND_CLOSEST_ULL(mul_u32_u32(value, 1000), 1001);
	return value;
}

/* refer to HDMI 2.1 Sink Capability Indication for QMS/GAME VRR */
/* brr_vfreq unit: 100    23.976Hz -> 2397 */
static void calc_vrr_range(struct rx_cap *prxcap, struct hdmitx_vrr_mode_group *group,
			   u32 brr_vfreq)
{
	bool qms;
	bool qms_tfr_min;
	bool qms_tfr_max;
	bool vrrmin;
	bool vrrmax;
	u8 data;

	if (!prxcap || !group)
		return;

	qms = !!prxcap->qms;
	qms_tfr_min = !!prxcap->qms_tfr_min;
	qms_tfr_max = !!prxcap->qms_tfr_max;
	vrrmin = !!prxcap->vrr_min;
	vrrmax = !!(prxcap->vrr_max >= 100);
	data = (qms << 4) | (qms_tfr_min << 3) | (qms_tfr_max << 2) | (vrrmin << 1) | vrrmax;

	switch (data) {
	case 0x00:
		group->vrr_min = 0;
		group->vrr_max = 0;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x02:
		group->vrr_min = 0;
		group->vrr_max = 0;
		group->game_vrr_min = prxcap->vrr_min;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x03:
		group->vrr_min = 0;
		group->vrr_max = 0;
		group->game_vrr_min = prxcap->vrr_min;
		group->game_vrr_max = prxcap->vrr_max;
		break;
	case 0x10:
		group->vrr_min = reduce_0p1_percent(4800);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x14:
		group->vrr_min = reduce_0p1_percent(4800);
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x12:
		group->vrr_min = reduce_0p1_percent(prxcap->vrr_min * 100);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x16:
		group->vrr_min = reduce_0p1_percent(prxcap->vrr_min * 100);
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x13:
		group->vrr_min = reduce_0p1_percent(prxcap->vrr_min * 100);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x17:
		group->vrr_min = reduce_0p1_percent(prxcap->vrr_min * 100);
		group->vrr_max = prxcap->vrr_max * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x18:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x1c:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x1a:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x1e:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x1b:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x1f:
		group->vrr_min = reduce_0p1_percent(2400);
		group->vrr_max = prxcap->vrr_max * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	default:
		group->vrr_min = 0;
		group->vrr_max = 0;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		HDMITX_DEBUG("qms: %s invalid VRR capability\n", __func__);
		HDMITX_DEBUG("qms: %d qms_tfr_min/max %d %d vrr_min/max %d %d\n",
			     qms, qms_tfr_min, qms_tfr_max, prxcap->vrr_min, prxcap->vrr_max);
		break;
	}
}

// TODO
int get_tx_max_vfreq(void)
{
	/* by default as 60 */
	return 60 * 100;
}

static const enum hdmi_vic brr_list[] = {
	HDMI_63_1920x1080p120_16x9,
	HDMI_16_1920x1080p60_16x9,
	HDMI_47_1280x720p120_16x9,
	HDMI_4_1280x720p60_16x9,
	HDMI_118_3840x2160p120_16x9,
	HDMI_97_3840x2160p60_16x9,
	HDMI_219_4096x2160p120_256x135,
	HDMI_102_4096x2160p60_256x135,
	HDMI_199_7680x4320p60_16x9,
};

/* find current VIC's BRR VIC */
enum hdmi_vic hdmitx_find_brr_vic(enum hdmi_vic vic)
{
	int i;
	enum hdmi_vic brr_vic = HDMI_UNKNOWN;
	const struct hdmi_timing *vic_timing = NULL;
	const struct hdmi_timing *brr_timing = NULL;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;

	vic_timing = hdmitx21_gettiming_from_vic(vic);
	if (!vic_timing) {
		pr_info("hdmitx: qms: can't find brr timing for VIC %d\n", vic);
		return HDMI_UNKNOWN;
	}

	for (i = 0; i < ARRAY_SIZE(brr_list); i++) {
		brr_timing = hdmitx21_gettiming_from_vic(brr_list[i]);
		if (!brr_timing)
			continue;
		/* if RX not support QMS tfr_max, then skip 120 */
		if (!prxcap->qms_tfr_max && brr_timing->v_freq == 120000)
			continue;
		/* if h/v is same, then find brr_vic */
		if (vic_timing->h_active == brr_timing->h_active &&
			vic_timing->v_active == brr_timing->v_active) {
			brr_vic = brr_list[i];
			/* check brr_vic is supported by both Tx and Rx */
			if (!hdmitx_edid_validate_mode(prxcap, brr_vic) ||
				hdmitx_common_validate_vic(&hdev->tx_common, brr_vic) < 0)
				brr_vic = HDMI_UNKNOWN;
			if (brr_vic) {
				struct hdmi_format_para *para;
				char *mode = NULL;
				char *color = NULL;

				mode = brr_timing->sname ? brr_timing->sname : brr_timing->name;
				color = env_get("colorattribute");
				para = hdmitx21_get_fmtpara(mode, color ? color : "");
				if (hdmitx_common_validate_format_para(&hdev->tx_common, para) >= 0)
					break;
			} else
				brr_vic = HDMI_UNKNOWN;
		}
	}

	return brr_vic;
}

static u32 drm_hdmitx_get_vrr_cap(void)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;
	u32 vrr_cap = 0;

	if (prxcap->qms || prxcap->vrr_max || prxcap->vrr_min) {
		vrr_cap |= prxcap->qms ? QMS_VRR_SUP : 0;
		vrr_cap |= prxcap->vrr_min ? GAMING_VRR_SUP : 0;
		return vrr_cap;
	}

	return false;
}

static bool is_rx_supported_vic(enum hdmi_vic brr_vic)
{
	int i;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;

	for (i = 0; i < prxcap->VIC_count; i++) {
		if (brr_vic == prxcap->VIC[i])
			return 1;
	}

	return 0;
}

static void add_brr_vic_lists(struct hdmitx_vrr_mode_group *group)
{
	int i = 0;
	enum hdmi_vic vic;
	const struct hdmi_timing *brr_timing;
	const struct hdmi_timing *vic_timing;
	int vsync;

	if (!group)
		return;

	brr_timing = hdmitx_mode_vic_to_hdmi_timing(group->brr_vic);
	if (!brr_timing)
		return;

	for (vic = HDMI_1_640x480p60_4x3; vic <= HDMI_219_4096x2160p120_256x135; vic++) {
		/* there is no VIC in 128 ~ 192 */
		if (vic == 128)
			vic = HDMI_193_5120x2160p120_64x27;

		vic_timing = hdmitx21_gettiming_from_vic(vic);
		if (!vic_timing)
			continue;
		if (!vic_timing->pi_mode) /* skip interlaced mode */
			continue;
		/* if vsync larger than the brr VIC, skip */
		if (vic_timing->v_freq > brr_timing->v_freq)
			continue;
		if (vic_timing->h_active != brr_timing->h_active)
			continue;
		if (vic_timing->v_active != brr_timing->v_active)
			continue;
		if (vic_timing->h_pict != brr_timing->h_pict)
			continue;
		if (vic_timing->v_pict != brr_timing->v_pict)
			continue;
		vsync = vic_timing->v_freq / 10;
		if (vsync >= reduce_0p1_percent(group->vrr_min) &&
		    (vic_timing->v_freq / 10) <= group->vrr_max) {
			if (i > MAX_QMS_GROUP_NUM) {
				pr_info("qms: vic list number over %d\n", MAX_QMS_GROUP_NUM);
				continue;
			}
			group->qms_vic_lists[i++] = vic_timing->vic;
		}
	}
}

static void add_vic_to_group(enum hdmi_vic vic, struct hdmitx_vrr_mode_group *group)
{
	const struct hdmi_timing *timing;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;
	char str_vics[64];
	int i;
	int len = 0;

	timing = hdmitx21_gettiming_from_vic(vic);
	if (!timing)
		return;
	group->brr_vic = vic;
	group->width = timing->h_active;
	group->height = timing->v_active;
	if (!prxcap->qms) {
		group->vrr_min = 0;
		group->vrr_max = 0;
	}
	if (!(prxcap->vrr_max || prxcap->vrr_min)) {
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
	}
	calc_vrr_range(prxcap, group, timing->v_freq / 10);
	add_brr_vic_lists(group);
	memset(str_vics, 0, sizeof(str_vics));
	for (i = 0; i < MAX_QMS_GROUP_NUM; i++)
		if (group->qms_vic_lists[i])
			len += snprintf(str_vics + len, sizeof(str_vics) - len, "%d ",
				group->qms_vic_lists[i]);
	if (prxcap->qms && str_vics[0])
		pr_info("qms: qms range group %s\n", str_vics);
}

static int drm_hdmitx_get_vrr_mode_group(struct hdmitx_vrr_mode_group *group, int max_group)
{
	int i = 0, j = 0;
	const struct hdmi_timing *timing;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;

	if (!group || max_group == 0)
		return 0;
	/* check RX VRR capabilities */
	if (!drm_hdmitx_get_vrr_cap())
		return 0;

	for (i = 0, j = 0; i < ARRAY_SIZE(brr_list) && j < max_group; i++) {
		timing = hdmitx21_gettiming_from_vic(brr_list[i]);
		if (!timing)
			continue;
		/* if RX not support QMS tfr_max, then skip 120 */
		if (!prxcap->qms_tfr_max && timing->v_freq == 120000)
			continue;
		/* check both TX and RX support current vic */
		if ((hdmitx_common_validate_vic(&hdev->tx_common, brr_list[i]) >= 0) &&
		    is_rx_supported_vic(brr_list[i])) {
			add_vic_to_group(brr_list[i], group + j);
			j++;
			/* if RX support tfr_max and BRR is 120, then skip 60 */
			if (prxcap->qms_tfr_max && timing->v_freq == 120000)
				i++;
		}
	}

	return j;
}

static void store_cea_idx(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	int i;
	int already = 0;

	if (!prxcap)
		return;

	for (i = 0; (i < VIC_MAX_NUM) && (i < prxcap->VIC_count); i++) {
		if (vic == prxcap->VIC[i]) {
			already = 1;
			break;
		}
	}
	if (!already) {
		prxcap->VIC[prxcap->VIC_count] = vic;
		prxcap->VIC_count++;
	}
}

static void store_qms_map_vics(struct rx_cap *prxcap, struct hdmitx_vrr_mode_group *groups)
{
	int i;
	int j;

	for (i = 0; i < MAX_VRR_MODE_GROUP; i++) {
		for (j = 0; j < MAX_QMS_GROUP_NUM; j++)
			if (groups[i].qms_vic_lists[j])
				store_cea_idx(prxcap, groups[i].qms_vic_lists[j]);
	}
}

/* call after EDID parsing */
void hdmitx_qms_map_vic(struct hdmitx_dev *hdev)
{
	struct hdmitx_vrr_mode_group *groups;

	groups = malloc(MAX_VRR_MODE_GROUP * sizeof(*groups));
	if (!groups) {
		pr_info("qms: %s alloc fail\n", __func__);
		return;
	}
	memset(groups, 0, MAX_VRR_MODE_GROUP * sizeof(*groups));
	drm_hdmitx_get_vrr_mode_group(groups, MAX_VRR_MODE_GROUP);
	store_qms_map_vics(&hdev->RXCap, groups);
	free(groups);
}
