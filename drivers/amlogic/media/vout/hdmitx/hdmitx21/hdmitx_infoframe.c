// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include "hdmitx_drv.h"

/* now this interface should be not used, otherwise need
 * adjust as hdmi_vend_infoframe_rawset fistly
 */
void hdmi_vend_infoframe_set(struct hdmi_vendor_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
		return;
	}

	hdmi_vendor_infoframe_pack(info, body, sizeof(body));
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
}

/* refer to DV Consumer Decoder for Source Devices
 * System Development Guide Kit version chapter 4.4.8 Game
 * content signaling:
 * 1.if DV sink device that supports ALLM with
 * InfoFrame Data Block (IFDB), HF-VSIF with ALLM_Mode = 1
 * should comes after Dolby VSIF with L11_MD_Present = 1 and
 * Content_Type[3:0] = 0x2(case B1)
 * 2. DV sink device that supports ALLM without
 * InfoFrame Data Block (IFDB), Dolby VSIF with L11_MD_Present
 * = 1 and Content_Type[3:0] = 0x2 should comes after HF-VSIF
 * with  ALLM_Mode = 1(case B2), or should only send Dolby VSIF,
 * not send HF-VSIF(case A)
 */
/* only used for DV_VSIF / HDMI1.4b_VSIF / HDR10+ VSIF */
void hdmi_vend_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!hb || !pb) {
		if (!hdev->RXCap.ifdb_present) {
			hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
			printf("%s clear vendor2\n", __func__);
		} else {
			hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
			printf("%s clear vendor\n", __func__);
		}
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	if (hdev->RXCap.ifdb_present && hdev->RXCap.additional_vsif_num >= 1) {
		/* dolby cts case93 B1 */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
		printf("%s set vendor\n", __func__);
	} else if (!hdev->RXCap.ifdb_present) {
		/* dolby cts case92 B2 */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
		printf("%s set vendor2\n", __func__);
	} else {
		/* case89 ifdb_present but no additional_vsif, should not send HF-VSIF */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
		printf("%s clear vendor2, set vendor\n", __func__);
	}
}

/* only used for HF-VSIF */
void hdmi_vend_infoframe2_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	if (!hb || !pb) {
		if (!hdev->RXCap.ifdb_present) {
			hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, NULL);
			printf("%s clear vendor\n", __func__);
		} else {
			hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, NULL);
			printf("%s clear vendor2\n", __func__);
		}
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	if (hdev->RXCap.ifdb_present && hdev->RXCap.additional_vsif_num >= 1) {
		/* dolby cts case93 B1 */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
		printf("%s set vendor2, vsif_num >= 1\n", __func__);
	} else if (!hdev->RXCap.ifdb_present) {
		/* dolby cts case92 B2 */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR, body);
		printf("%s set vendor\n", __func__);
	} else {
		/* case89 ifdb_present but no additional_vsif, currently
		 * no DV-VSIF enabled, then send HF-VSIF
		 */
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_VENDOR2, body);
		printf("%s set vendor2\n", __func__);
	}
}

void hdmi_avi_infoframe_set(struct hdmi_avi_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, NULL);
		return;
	}

	hdmi_avi_infoframe_pack(info, body, sizeof(body));
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, body);
}

void hdmi_avi_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AVI, body);
}

void hdmi_avi_infoframe_config(enum avi_component_conf conf, u8 val)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	struct hdmi_avi_infoframe *info = &hdev->infoframes.avi.avi;
	struct hdmi_format_para *para = hdev->para;

	switch (conf) {
	case CONF_AVI_CS:
		info->colorspace = val;
		break;
	case CONF_AVI_BT2020:
		if (val == SET_AVI_BT2020) {
			info->colorimetry = HDMI_COLORIMETRY_EXTENDED;
			info->extended_colorimetry = HDMI_EXTENDED_COLORIMETRY_BT2020;
		}
		if (val == CLR_AVI_BT2020) {
			if (para->timing.v_total <= 576) {/* SD formats */
				info->colorimetry = HDMI_COLORIMETRY_ITU_601;
				info->extended_colorimetry = 0;
			} else {
				if (hdev->colormetry) {
					info->colorimetry = HDMI_COLORIMETRY_EXTENDED;
					info->extended_colorimetry =
						HDMI_EXTENDED_COLORIMETRY_BT2020;
				} else {
					info->colorimetry = HDMI_COLORIMETRY_ITU_709;
					info->extended_colorimetry = 0;
				}
			}
		}
		break;
	case CONF_AVI_Q01:
		info->quantization_range = val;
		break;
	case CONF_AVI_YQ01:
		info->ycc_quantization_range = val;
		break;
	case CONF_AVI_VIC:
		info->video_code = val & 0xff;
		break;
	default:
		break;
	}
	hdmi_avi_infoframe_set(info);
}

/* EMP packets is different as other packets
 * no checksum, the successive packets in a video frame...
 */
void hdmi_emp_infoframe_set(enum emp_type type, struct emp_packet_st *info)
{
	u8 body[31] = {0};
	u8 *md;
	u16 pkt_type;

	pkt_type = (HDMI_INFOFRAME_TYPE_EMP << 8) | type;
	if (!info) {
		hdmitx21_infoframe_send(pkt_type, NULL);
		return;
	}

	/* head body, 3bytes */
	body[0] = info->header.header;
	body[1] = info->header.first << 7 | info->header.last << 6;
	body[2] = info->header.seq_idx;
	/* packet body, 28bytes */
	body[3] = info->body.emp0.new << 7 |
		info->body.emp0.end << 6 |
		info->body.emp0.ds_type << 4 |
		info->body.emp0.afr << 3 |
		info->body.emp0.vfr << 2 |
		info->body.emp0.sync << 1;
	body[4] = 0; /* RSVD */
	body[5] = info->body.emp0.org_id;
	body[6] = info->body.emp0.ds_tag >> 8 & 0xff;
	body[7] = info->body.emp0.ds_tag & 0xff;
	body[8] = info->body.emp0.ds_length >> 8 & 0xff;
	body[9] = info->body.emp0.ds_length & 0xff;
	md = &body[10];
	switch (info->type) {
	case EMP_TYPE_VRR_GAME:
		md[0] = info->body.emp0.md.game_md.fva_factor_m1 << 4 |
			info->body.emp0.md.game_md.vrr_en << 0;
		md[1] = info->body.emp0.md.game_md.base_vfront;
		md[2] = info->body.emp0.md.game_md.brr_rate >> 8 & 3;
		md[3] = info->body.emp0.md.game_md.brr_rate & 0xff;
		break;
	case EMP_TYPE_VRR_QMS:
		md[0] = info->body.emp0.md.qms_md.qms_en << 2 |
			info->body.emp0.md.qms_md.m_const << 1;
		md[1] = info->body.emp0.md.qms_md.base_vfront;
		md[2] = info->body.emp0.md.qms_md.next_tfr << 3 |
			(info->body.emp0.md.qms_md.brr_rate >> 8 & 3);
		md[3] = info->body.emp0.md.qms_md.brr_rate & 0xff;
		break;
/*
 *	case EMP_TYPE_SBTM:
 *		md[0] = info->body.emp0.md.sbtm_md.sbtm_ver << 0;
 *		md[1] = info->body.emp0.md.sbtm_md.sbtm_mode << 0 |
 *			info->body.emp0.md.sbtm_md.sbtm_type << 2 |
 *			info->body.emp0.md.sbtm_md.grdm_min << 4 |
 *			info->body.emp0.md.sbtm_md.grdm_lum << 6;
 *		md[2] = (info->body.emp0.md.sbtm_md.frmpblimitint >> 8) & 0x3f;
 *		md[3] = info->body.emp0.md.sbtm_md.frmpblimitint & 0xff;
 *		break;
 */
	default:
		break;
	}
	hdmitx21_infoframe_send(pkt_type, body);
}

/* this is only configuring the EMP frame body, not send by HW
 * then call hdmi_emp_infoframe_set to send out
 */
void hdmi_emp_frame_set_member(struct emp_packet_st *info,
	enum emp_component_conf conf, u32 val)
{
	if (!info)
		return;

	switch (conf) {
	case CONF_HEADER_INIT:
		info->header.header = HDMI_INFOFRAME_TYPE_EMP; /* fixed value */
		break;
	case CONF_HEADER_LAST:
		info->header.last = !!val;
		break;
	case CONF_HEADER_FIRST:
		info->header.first = !!val;
		break;
	case CONF_HEADER_SEQ_INDEX:
		info->header.seq_idx = val;
		break;
	case CONF_SYNC:
		info->body.emp0.sync = !!val;
		break;
	case CONF_VFR:
		info->body.emp0.vfr = !!val;
		break;
	case CONF_AFR:
		info->body.emp0.afr = !!val;
		break;
	case CONF_DS_TYPE:
		info->body.emp0.ds_type = val & 3;
		break;
	case CONF_END:
		info->body.emp0.end = !!val;
		break;
	case CONF_NEW:
		info->body.emp0.new = !!val;
		break;
	case CONF_ORG_ID:
		info->body.emp0.org_id = val;
		break;
	case CONF_DATA_SET_TAG:
		info->body.emp0.ds_tag = val;
		break;
	case CONF_DATA_SET_LENGTH:
		info->body.emp0.ds_length = val;
		break;
	case CONF_VRR_EN:
		info->body.emp0.md.game_md.vrr_en = !!val;
		break;
	case CONF_FACTOR_M1:
		info->body.emp0.md.game_md.fva_factor_m1 = val;
		break;
	case CONF_QMS_EN:
		info->body.emp0.md.qms_md.qms_en = !!val;
		break;
	case CONF_M_CONST:
		info->body.emp0.md.qms_md.m_const = !!val;
		break;
	case CONF_BASE_VFRONT:
		info->body.emp0.md.qms_md.base_vfront = val;
		break;
	case CONF_NEXT_TFR:
		info->body.emp0.md.qms_md.next_tfr = val;
		break;
	case CONF_BASE_REFRESH_RATE:
		info->body.emp0.md.qms_md.brr_rate = val & 0x3ff;
		break;
/*	case CONF_SBTM_VER:
 *		info->body.emp0.md.sbtm_md.sbtm_ver = val & 0xf;
 *		break;
 *	case CONF_SBTM_MODE:
 *		info->body.emp0.md.sbtm_md.sbtm_mode = val & 0x3;
 *		break;
 *	case CONF_SBTM_TYPE:
 *		info->body.emp0.md.sbtm_md.sbtm_type = val & 0x3;
 *		break;
 *	case CONF_SBTM_GRDM_MIN:
 *		info->body.emp0.md.sbtm_md.grdm_min = val & 0x3;
 *		break;
 *	case CONF_SBTM_GRDM_LUM:
 *		info->body.emp0.md.sbtm_md.grdm_lum = val & 0x3;
 *		break;
 *	case CONF_SBTM_FRMPBLIMITINT:
 *		info->body.emp0.md.sbtm_md.frmpblimitint = val & 0x3fff;
 *		break;
 */
	default:
		break;
	}
}

void hdmi_spd_infoframe_set(struct hdmi_spd_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_SPD, NULL);
		return;
	}

	hdmi_spd_infoframe_pack(info, body, sizeof(body));
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_SPD, body);
}

void hdmi_audio_infoframe_set(struct hdmi_audio_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, NULL);
		return;
	}

	hdmi_audio_infoframe_pack(info, body, sizeof(body));
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, body);
}

void hdmi_audio_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_AUDIO, body);
}

void hdmi_drm_infoframe_set(struct hdmi_drm_infoframe *info)
{
	u8 body[31] = {0};

	if (!info) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, NULL);
		return;
	}

	hdmi_drm_infoframe_pack(info, body, sizeof(body));
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, body);
}

void hdmi_drm_infoframe_rawset(u8 *hb, u8 *pb)
{
	u8 body[31] = {0};

	if (!hb || !pb) {
		hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, NULL);
		return;
	}

	memcpy(body, hb, 3);
	memcpy(&body[3], pb, 28);
	hdmitx21_infoframe_send(HDMI_INFOFRAME_TYPE_DRM, body);
}

/* for only 8bit */
void hdmi_gcppkt_manual_set(bool en)
{
	u8 body[31] = {0};

	body[0] = HDMI_PACKET_TYPE_GCP;
	if (en)
		hdmitx21_infoframe_send(HDMI_PACKET_TYPE_GCP, body);
	else
		hdmitx21_infoframe_send(HDMI_PACKET_TYPE_GCP, NULL);
}

