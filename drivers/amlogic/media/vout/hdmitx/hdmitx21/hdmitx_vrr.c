// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <amlogic/media/vout/aml_vout.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include <amlogic/auge_sound.h>
#include <linux/arm-smccc.h>
#include "hdmitx_drv.h"
#include "../hdmitx_common/hdmitx_log.h"

static const u16 vsync_tfr_table[TFR_MAX] = {
	[TFR_QMSVRR_INACTIVE] = 0,
	[TFR_23P97] = 2397,
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

	if (!hdev->qms_en)
		return;
	timing = hdmitx21_gettiming_from_vic(hdev->brr_vic);
	if (!timing) {
		pr_info("hdmitx: can't find timing for BRR VIC %d\n", hdev->brr_vic);
		return;
	}
	pr_info("hdmitx: set qms parameters\n");
	brr_rate = timing->v_freq / 1000;

	memset(vrr_pkt, 0, sizeof(*vrr_pkt));
	vrr_pkt->type = EMP_TYPE_VRR_QMS; /* FIXED VALUE */
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

	brr_vfront = timing->v_front;
	hdmi_emp_frame_set_member(vrr_pkt, CONF_M_CONST, 0);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_QMS_EN, 1);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_NEXT_TFR, vsync_match_to_tfr(timing->v_freq / 10));
	hdmi_emp_frame_set_member(vrr_pkt, CONF_BASE_VFRONT, brr_vfront);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_BASE_REFRESH_RATE, brr_rate);
	hdmi_emp_frame_set_member(vrr_pkt, CONF_M_CONST, 1);
	hdmi_emp_infoframe_set(EMP_TYPE_VRR_QMS, vrr_pkt);
	hdmitx_vrr_set_maxlncnt(timing->v_total);
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

/* refer to HDMI 2.1 Sink Capability Indication for QMS/GAME VRR */
/* brr_vfreq unit: 100    23.976Hz -> 2397 */
static void calc_vrr_range(struct rx_cap *prxcap, struct drm_vrr_mode_group *group, u32 brr_vfreq)
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
		group->vrr_min = 48000 / 1001 * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x14:
		group->vrr_min = 48000 / 1001 * 100;
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x12:
		group->vrr_min = prxcap->vrr_min * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x16:
		group->vrr_min = prxcap->vrr_min * 100;
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x13:
		group->vrr_min = prxcap->vrr_min * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x17:
		group->vrr_min = prxcap->vrr_min * 100;
		group->vrr_max = prxcap->vrr_max * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x18:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x1c:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		break;
	case 0x1a:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x1e:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = brr_vfreq;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = brr_vfreq;
		break;
	case 0x1b:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = 60 * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	case 0x1f:
		group->vrr_min = 24000 / 1001 * 100;
		group->vrr_max = prxcap->vrr_max * 100;
		group->game_vrr_min = prxcap->vrr_min * 100;
		group->game_vrr_max = prxcap->vrr_max * 100;
		break;
	default:
		group->vrr_min = 0;
		group->vrr_max = 0;
		group->game_vrr_min = 0;
		group->game_vrr_max = 0;
		HDMITX_DEBUG("%s invalid VRR capability\n", __func__);
		HDMITX_DEBUG("qms %d qms_tfr_min/max %d %d vrr_min/max %d %d\n",
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

/* find current VIC's BRR VIC */
enum hdmi_vic hdmitx_find_brr_vic(enum hdmi_vic vic)
{
	int i;
	enum hdmi_vic brr_vic = HDMI_UNKNOWN;
	const struct hdmi_timing *vic_timing = NULL;
	const struct hdmi_timing *brr_timing = NULL;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct rx_cap *prxcap = &hdev->RXCap;
	struct drm_vrr_mode_group test_group = {0};

	vic_timing = hdmitx21_gettiming_from_vic(vic);
	if (!vic_timing) {
		pr_info("hdmitx: can't find timing for VIC %d\n", vic);
		return HDMI_UNKNOWN;
	}

	for (i = 0; i < ARRAY_SIZE(brr_list); i++) {
		if (vic == brr_list[i])
			brr_vic = vic;
		brr_timing = hdmitx21_gettiming_from_vic(brr_list[i]);
		if (!brr_timing)
			brr_vic = HDMI_UNKNOWN;
		else if (vic_timing->h_active == brr_timing->h_active &&
			vic_timing->v_active == brr_timing->v_active)
			brr_vic = brr_list[i];
	}

	if (!is_rx_supported_vic(brr_vic))
		brr_vic = HDMI_UNKNOWN;

	calc_vrr_range(prxcap, &test_group, get_tx_max_vfreq());
	// TODO for 120

	return brr_vic;
}
