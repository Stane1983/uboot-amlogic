// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include "hdmitx_edid_parse_header.h"

#define CEA_DATA_BLOCK_COLLECTION_ADDR_1STP 0x04
#define VIDEO_TAG 0x40
#define AUDIO_TAG 0x20
#define VENDOR_TAG 0x60
#define SPEAKER_TAG 0x80
#define EXTENSION_IFDB_TAG	0x20

#define HDMI_EDID_BLOCK_TYPE_RESERVED	 0
#define HDMI_EDID_BLOCK_TYPE_AUDIO		1
#define HDMI_EDID_BLOCK_TYPE_VIDEO		2
#define HDMI_EDID_BLOCK_TYPE_VENDER	        3
#define HDMI_EDID_BLOCK_TYPE_SPEAKER	        4
#define HDMI_EDID_BLOCK_TYPE_VESA		5
#define HDMI_EDID_BLOCK_TYPE_RESERVED2	        6
#define HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG       7

#define EXTENSION_VENDOR_SPECIFIC 0x1
#define EXTENSION_COLORMETRY_TAG 0x5
/* DRM stands for "Dynamic Range and Mastering " */
#define EXTENSION_DRM_STATIC_TAG    0x6
#define EXTENSION_DRM_DYNAMIC_TAG   0x7
   #define TYPE_1_HDR_METADATA_TYPE    0x0001
   #define TS_103_433_SPEC_TYPE        0x0002
   #define ITU_T_H265_SPEC_TYPE        0x0003
   #define TYPE_4_HDR_METADATA_TYPE    0x0004
/* Video Format Preference Data block */
#define EXTENSION_VFPDB_TAG	0xd
#define EXTENSION_Y420_VDB_TAG	0xe
#define EXTENSION_Y420_CMDB_TAG	0xf
#define EXTENSION_DOLBY_VSADB	0x11
#define EXTENSION_SCDB_EXT_TAG	0x79 /*HDMI Forum Sink Capability Data Block*/
#define EXTENSION_SBTM_EXT_TAG	0x7a /* 122 */

#define EDID_DETAILED_TIMING_DES_BLOCK0_POS 0x36
#define EDID_DETAILED_TIMING_DES_BLOCK1_POS 0x48
#define EDID_DETAILED_TIMING_DES_BLOCK2_POS 0x5A
#define EDID_DETAILED_TIMING_DES_BLOCK3_POS 0x6C

/* EDID Descriptor Tag */
#define TAG_PRODUCT_SERIAL_NUMBER 0xFF
#define TAG_ALPHA_DATA_STRING 0xFE
#define TAG_RANGE_LIMITS 0xFD
#define TAG_DISPLAY_PRODUCT_NAME_STRING 0xFC /* MONITOR NAME */
#define TAG_COLOR_POINT_DATA 0xFB
#define TAG_STANDARD_TIMINGS 0xFA
#define TAG_DISPLAY_COLOR_MANAGEMENT 0xF9
#define TAG_CVT_TIMING_CODES 0xF8
#define TAG_ESTABLISHED_TIMING_III 0xF7
#define TAG_DUMMY_DES 0x10

#define GET_BITS_FILED(val, start, len) \
	(((val) >> (start)) & ((1 << (len)) - 1))

const struct hdmi_timing *hdmitx_mode_match_timing_name(const char *name);
static void edid_dtd_parsing(struct rx_cap *prxcap, unsigned char *data);
static void hdmitx_edid_set_default_aud(struct rx_cap *prxcap);

/* Base Block, Vendor/Product Information, byte[8]~[18] */
struct edid_venddat_t {
	u8 data[10];
};

static struct edid_venddat_t vendor_id[] = {
{ {0x41, 0x0C, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x14} },
/* { {0x05, 0xAC, 0x30, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x19} }, */
/* Add new vendor data here */
};

static void phy_addr_clear(struct vsdb_phyaddr *vsdb_phy_addr)
{
	if (!vsdb_phy_addr)
		return;

	vsdb_phy_addr->a = 0;
	vsdb_phy_addr->b = 0;
	vsdb_phy_addr->c = 0;
	vsdb_phy_addr->d = 0;
	vsdb_phy_addr->valid = 0;
}

static bool hdmitx_edid_header_invalid(u8 edid_check, const u8 *buf)
{
	int i = 0;

	if (!buf)
		return true;

	if (edid_check & 0x01)
		return false;

	if (buf[0] != 0 || buf[7] != 0)
		return true;
	for (i = 1; i < 7; i++) {
		if (buf[i] != 0xff)
			return true;
	}

	return false;
}

/* return the blocks of extension CTA */
static unsigned char hdmitx_edid_get_cta_block_count(const u8 *edid_buf)
{
	unsigned char cta_block_count = 0;

	if (!edid_buf)
		return 0;

	cta_block_count = edid_buf[0x7E];
	/* HFR-EEODB */
	if (cta_block_count && edid_buf[128 + 4] == EXTENSION_EEODB_EXT_TAG &&
		edid_buf[128 + 5] == EXTENSION_EEODB_EXT_CODE)
		cta_block_count = edid_buf[128 + 6];
	/* limit cta_block_count to EDID_MAX_BLOCK - 1 */
	if (cta_block_count > EDID_MAX_BLOCK - 1)
		cta_block_count = EDID_MAX_BLOCK - 1;

	return cta_block_count;
}

/* return all the blocks of the EDID */
static unsigned char hdmitx_edid_get_block_count(const u8 *edid_buf)
{
	if (!edid_buf)
		return 0;

	return hdmitx_edid_get_cta_block_count(edid_buf) + 1;
}

bool hdmitx_edid_is_all_zeros(u8 *rawedid)
{
	unsigned int i = 0, j = 0;
	unsigned int chksum = 0;
	unsigned char block_count = 1;

	if (!rawedid)
		return false;

	block_count = hdmitx_edid_get_block_count(rawedid);
	for (j = 0; j < block_count; j++) {
		chksum = 0;
		for (i = 0; i < 128; i++)
			chksum += rawedid[i + j * 128];
		if (chksum != 0)
			return false;
	}
	return true;
}

/* check the checksum for each sub block */
static bool _check_edid_blk_chksum(u8 edid_check, unsigned char *block)
{
	unsigned int chksum = 0;
	unsigned int i = 0;

	if (!block)
		return false;

	if (edid_check & 0x02)
		return true;

	for (chksum = 0, i = 0; i < 0x80; i++)
		chksum += block[i];
	if ((chksum & 0xff) != 0)
		return false;
	else
		return true;
}

/* check the first edid block */
static bool _check_base_structure(u8 edid_check, unsigned char *buf)
{
	unsigned int i = 0;

	if (!buf)
		return false;

	if (!(edid_check & 0x01)) {
		/* check block 0 first 8 bytes */
		if (buf[0] != 0 || buf[7] != 0)
			return false;

		for (i = 1; i < 7; i++) {
			if (buf[i] != 0xff)
				return false;
		}
	}

	if (_check_edid_blk_chksum(edid_check, buf) == false)
		return false;

	return true;
}

/*
 * check the EDID validity
 * base structure: header, checksum
 * extension: the first non-zero byte, checksum
 */
bool hdmitx_edid_check_data_valid(u8 edid_check, unsigned char *buf)
{
	int i;
	int blk_cnt = 1;

	if (!buf)
		return false;

	blk_cnt = hdmitx_edid_get_block_count(buf);

	/* check block 0 */
	if (_check_base_structure(edid_check, &buf[0]) == 0)
		return false;

	if (blk_cnt == 1)
		return true;
	/* check block 1 extension tag */
	if (!(edid_check & 0x01)) {
		if (!(buf[0x80] == 0x2 || buf[0x80] == 0xf0))
			return false;
	}
	/* check extension block 1 and more */
	for (i = 1; i < blk_cnt; i++) {
		if (!(edid_check & 0x01)) {
			if (buf[i * 0x80] == 0)
				return false;
		}
		if (_check_edid_blk_chksum(edid_check, &buf[i * 0x80]) == false)
			return false;
	}

	return true;
}

static void _show_pcm_ch(struct rx_cap *prxcap, int i,
			 int *ppos, char *buf)
{
	const char * const aud_sample_size[] = {"ReferToStreamHeader",
		"16", "20", "24", NULL};
	int j = 0;

	for (j = 0; j < 3; j++) {
		if (prxcap->RxAudioCap[i].cc3 & (1 << j))
			*ppos += snprintf(buf + *ppos, PAGE_SIZE, "%s/",
				aud_sample_size[j + 1]);
	}
	*ppos += snprintf(buf + *ppos - 1, PAGE_SIZE, " bit\n") - 1;
}

/**/
ssize_t _show_aud_cap(struct rx_cap *prxcap, char *buf)
{
	int i, pos = 0, j;
	struct dolby_vsadb_cap *cap = &prxcap->dolby_vsadb_cap;

	static const char * const aud_ct[] =  {
		"ReferToStreamHeader", "PCM", "AC-3", "MPEG1", "MP3",
		"MPEG2", "AAC", "DTS", "ATRAC",	"OneBitAudio",
		"Dolby_Digital+", "DTS-HD", "MAT", "DST", "WMA_Pro",
		"Reserved", NULL};
	static const char * const aud_sampling_frequency[] = {
		"ReferToStreamHeader", "32", "44.1", "48", "88.2", "96",
		"176.4", "192", NULL};

	pos += snprintf(buf + pos, PAGE_SIZE,
		"CodingType MaxChannels SamplingFreq SampleSize\n");
	for (i = 0; i < prxcap->AUD_count; i++) {
		if (prxcap->RxAudioCap[i].audio_format_code == CT_CXT) {
			if ((prxcap->RxAudioCap[i].cc3 >> 3) == 0xb) {
				pos += snprintf(buf + pos, PAGE_SIZE, "MPEG-H, 8ch, ");
				for (j = 0; j < 7; j++) {
					if (prxcap->RxAudioCap[i].freq_cc & (1 << j))
						pos += snprintf(buf + pos, PAGE_SIZE, "%s/",
							aud_sampling_frequency[j + 1]);
				}
				pos += snprintf(buf + pos - 1, PAGE_SIZE, " kHz\n");
			}
			continue;
		}
		pos += snprintf(buf + pos, PAGE_SIZE, "%s",
			aud_ct[prxcap->RxAudioCap[i].audio_format_code]);
		if (prxcap->RxAudioCap[i].audio_format_code == CT_DD_P &&
		    (prxcap->RxAudioCap[i].cc3 & 1))
			pos += snprintf(buf + pos, PAGE_SIZE, "/ATMOS");
		if (prxcap->RxAudioCap[i].audio_format_code != CT_CXT)
			pos += snprintf(buf + pos, PAGE_SIZE, ", %d ch, ",
				prxcap->RxAudioCap[i].channel_num_max + 1);
		for (j = 0; j < 7; j++) {
			if (prxcap->RxAudioCap[i].freq_cc & (1 << j))
				pos += snprintf(buf + pos, PAGE_SIZE, "%s/",
					aud_sampling_frequency[j + 1]);
		}
		pos += snprintf(buf + pos - 1, PAGE_SIZE, " kHz, ") - 1;
		switch (prxcap->RxAudioCap[i].audio_format_code) {
		case CT_PCM:
			_show_pcm_ch(prxcap, i, &pos, buf);
			break;
		case CT_AC_3:
		case CT_MPEG1:
		case CT_MP3:
		case CT_MPEG2:
		case CT_AAC:
		case CT_DTS:
		case CT_ATRAC:
		case CT_ONE_BIT_AUDIO:
			pos += snprintf(buf + pos, PAGE_SIZE,
				"MaxBitRate %dkHz\n",
				prxcap->RxAudioCap[i].cc3 * 8);
			break;
		case CT_DD_P:
		case CT_DTS_HD:
		case CT_MAT:
		case CT_DST:
			pos += snprintf(buf + pos, PAGE_SIZE, "DepValue 0x%x\n",
				prxcap->RxAudioCap[i].cc3);
			break;
		case CT_WMA:
		default:
			break;
		}
	}

	if (cap->ieeeoui == DOVI_IEEEOUI) {
		/*
		 *Dolby Vendor Specific:
		 *  headphone_playback_only:0,
		 *  center_speaker:1,
		 *  surround_speaker:1,
		 *  height_speaker:1,
		 *  Ver:1.0,
		 *  MAT_PCM_48kHz_only:1,
		 *  e61146d0007001,
		 */
		pos += snprintf(buf + pos, PAGE_SIZE,
				"Dolby Vendor Specific:\n");
		if (cap->dolby_vsadb_ver == 0)
			pos += snprintf(buf + pos, PAGE_SIZE, "  Ver:1.0,\n");
		else
			pos += snprintf(buf + pos, PAGE_SIZE,
				"  Ver:Reversed,\n");
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  center_speaker:%d,\n", cap->spk_center);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  surround_speaker:%d,\n", cap->spk_surround);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  height_speaker:%d,\n", cap->spk_height);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  headphone_playback_only:%d,\n", cap->headphone_only);
		pos += snprintf(buf + pos, PAGE_SIZE,
			"  MAT_PCM_48kHz_only:%d,\n", cap->mat_48k_pcm_only);

		pos += snprintf(buf + pos, PAGE_SIZE, "  ");
		for (i = 0; i < 7; i++)
			pos += snprintf(buf + pos, PAGE_SIZE, "%02x",
				cap->rawdata[i]);
		pos += snprintf(buf + pos, PAGE_SIZE, ",\n");
	}
	return pos;
}

/*index is hdmi 1.4 vic in vsif, value is hdmi2.0 vic*/
static const u32 hdmi14_4k_vics[] = {
/* 0 - dummy*/
	0,
/* 1 - 3840x2160@30Hz */
	95,
/* 2 - 3840x2160@25Hz */
	94,
/* 3 - 3840x2160@24Hz */
	93,
/* 4 - 4096x2160@24Hz (SMPTE) */
	98,
};

/* HDMI 1.4 introduces the 4 types of 4K resolutions
 * 3840x2160@30/25/24Hz and 4096x2160@24Hz
 * but no corresponding CEA-861-D VIC.
 * so it uses VSIF.hdmi_vic as 1/2/3/4 to represent 4 types
 * In CEA-861-F, here assigns the VIC 95/94/93/98 later
 * this function converts 861-F 4k vic to VSIF.hdmi_vic
 */
u32 hdmitx_edid_get_hdmi14_4k_vic(u32 vic)
{
	bool ret = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(hdmi14_4k_vics); i++) {
		if (vic == hdmi14_4k_vics[i]) {
			ret = i;
			break;
		}
	}

	return ret;
}

static void edid_parsing_id_manufacturer_name(struct rx_cap *prxcap,
					   u8 *data)
{
	int i;
	u8 uppercase[26] = { 0 };
	u8 brand[3];

	if (!prxcap || !data)
		return;

	/* Fill array uppercase with 'A' to 'Z' */
	for (i = 0; i < 26; i++)
		uppercase[i] = 'A' + i;

	brand[0] = data[0] >> 2;
	brand[1] = ((data[0] & 0x3) << 3) + (data[1] >> 5);
	brand[2] = data[1] & 0x1f;

	if (brand[0] > 26 || brand[0] == 0 ||
	    brand[1] > 26 || brand[1] == 0 ||
	    brand[2] > 26 || brand[2] == 0)
		return;
	for (i = 0; i < 3; i++)
		prxcap->IDManufacturerName[i] = uppercase[brand[i] - 1];
}

static void edid_parsing_id_product_code(struct rx_cap *prxcap,
				      u8 *data)
{
	if (!prxcap || !data)
		return;

	prxcap->IDProductCode[0] = data[1];
	prxcap->IDProductCode[1] = data[0];
}

static void edid_parsing_id_serial_number(struct rx_cap *prxcap,
				       u8 *data)
{
	int i;

	if (!prxcap || !data)
		return;

	for (i = 0; i < 4; i++)
		prxcap->IDSerialNumber[i] = data[3 - i];
}

/* store the idx of vesa_timing[32], which is 0
 * note: only save vesa mode, for compliance with uboot.
 * uboot not parse standard timing, or CVT block.
 * as disp_cap will check all mode in rx_cap->VIC[],
 * and all mode in vesa_timing[], if CEA mode is
 * stored in vesa_timing[], it will cause kernel
 * support more CEA mode than uboot.
 */
static void store_vesa_idx(struct rx_cap *prxcap, enum hdmi_vic vesa_timing)
{
	int i;

	if (!prxcap)
		return;

	for (i = 0; i < VESA_MAX_TIMING; i++) {
		if (!prxcap->vesa_timing[i]) {
			prxcap->vesa_timing[i] = vesa_timing;
			break;
		}

		if (prxcap->vesa_timing[i] == vesa_timing)
			break;
	}
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

static void store_y420_idx(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	int i;
	int already = 0;

	if (!prxcap)
		return;

	/* Y420 is claimed in Y420VDB, y420_vic[] will list in dc_cap */
	for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
		if (vic == prxcap->y420_vic[i]) {
			already = 1;
			break;
		}
	}
	if (!already) {
		for (i = 0; i < Y420_VIC_MAX_NUM; i++) {
			if (prxcap->y420_vic[i] == 0) {
				prxcap->y420_vic[i] = vic;
				break;
			}
		}
	}
}

static void edid_established_timings(struct rx_cap *prxcap, u8 *data)
{
	if (!prxcap || !data)
		return;

	if (data[0] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_800x600p60hz);
	if (data[1] & (1 << 3))
		store_vesa_idx(prxcap, HDMIV_1024x768p60hz);
}

static void edid_standard_timing_iii(struct rx_cap *prxcap, u8 *data)
{
	if (!prxcap || !data)
		return;

	if (data[0] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_1152x864p75hz);
	if (data[1] & (1 << 6))
		store_vesa_idx(prxcap, HDMIV_1280x768p60hz);
	if (data[1] & (1 << 2))
		store_vesa_idx(prxcap, HDMIV_1280x960p60hz);
	if (data[1] & (1 << 1))
		store_vesa_idx(prxcap, HDMIV_1280x1024p60hz);
	if (data[2] & (1 << 7))
		store_vesa_idx(prxcap, HDMIV_1360x768p60hz);
	if (data[2] & (1 << 1))
		store_vesa_idx(prxcap, HDMIV_1400x1050p60hz);
	if (data[3] & (1 << 5))
		store_vesa_idx(prxcap, HDMIV_1680x1050p60hz);
	if (data[3] & (1 << 2))
		store_vesa_idx(prxcap, HDMIV_1600x1200p60hz);
	if (data[4] & (1 << 0))
		store_vesa_idx(prxcap, HDMIV_1920x1200p60hz);
}

static void calc_timing(struct rx_cap *prxcap, u8 *data, struct vesa_standard_timing *t)
{
	const struct hdmi_timing *standard_timing = NULL;

	if (!prxcap || !data || !t)
		return;
	if (data[0] < 2 && data[1] < 2)
		return;

	t->hactive = (data[0] + 31) * 8;
	switch ((data[1] >> 6) & 0x3) {
	case 0:
		t->vactive = t->hactive * 5 / 8;
		break;
	case 1:
		t->vactive = t->hactive * 3 / 4;
		break;
	case 2:
		t->vactive = t->hactive * 4 / 5;
		break;
	case 3:
	default:
		t->vactive = t->hactive * 9 / 16;
		break;
	}
	t->vsync = (data[1] & 0x3f) + 60;
	standard_timing = hdmitx_mode_match_vesa_timing(t);
	if (standard_timing) {
		/* prefer 16x9 mode */
		if (standard_timing->vic == HDMI_6_720x480i60_4x3 ||
			standard_timing->vic == HDMI_2_720x480p60_4x3 ||
			standard_timing->vic == HDMI_21_720x576i50_4x3 ||
			standard_timing->vic == HDMI_17_720x576p50_4x3)
			t->vesa_timing = standard_timing->vic + 1;
		else
			t->vesa_timing = standard_timing->vic;

		if (t->vesa_timing < HDMITX_VESA_OFFSET) {
			/* for compliance with uboot, don't
			 * save CEA mode in standard_timing block.
			 * uboot don't parse standard_timing block
			 */
			/* store_cea_idx(prxcap, t->vesa_timing); */
		} else {
			store_vesa_idx(prxcap, t->vesa_timing);
		}
	}
}

static void edid_standardtiming(struct rx_cap *prxcap, u8 *data,
				int max_num)
{
	int i;
	struct vesa_standard_timing timing;

	if (!prxcap || !data)
		return;
	for (i = 0; i < max_num; i++) {
		memset(&timing, 0, sizeof(struct vesa_standard_timing));
		calc_timing(prxcap, &data[i * 2], &timing);
	}
}

static void edid_receiverproductnameparse(struct rx_cap *prxcap,
					  u8 *data)
{
	int i = 0;

	if (!prxcap || !data)
		return;
	/* some Display Product name end with 0x20, not 0x0a
	 */
	while ((data[i] != 0x0a) && (data[i] != 0x20) && (i < 13)) {
		prxcap->ReceiverProductName[i] = data[i];
		i++;
	}
	prxcap->ReceiverProductName[i] = '\0';
}

/* ----------------------------------------------------------- */
static void edid_parseceatiming(struct rx_cap *prxcap,
	unsigned char *buff)
{
	int i;
	unsigned char *dtd_base = buff;

	if (!prxcap || !buff)
		return;

	for (i = 0; i < 4; i++) {
		edid_dtd_parsing(prxcap, dtd_base);
		dtd_base += 0x12;
	}
}

static struct vsdb_phyaddr vsdb_local = {0};
int get_vsdb_phy_addr(struct vsdb_phyaddr *vsdb)
{
	if (!vsdb)
		return -1;

	vsdb = &vsdb_local;
	return vsdb->valid;
}

static void set_vsdb_phy_addr(struct rx_cap *prxcap,
	unsigned char *edid_offset)
{
	int phy_addr;
	struct vsdb_phyaddr *vsdb;

	if (!prxcap || !edid_offset)
		return;
	vsdb = &prxcap->vsdb_phy_addr;
	vsdb->a = (edid_offset[0] >> 4) & 0xf;
	vsdb->b = (edid_offset[0] >> 0) & 0xf;
	vsdb->c = (edid_offset[1] >> 4) & 0xf;
	vsdb->d = (edid_offset[1] >> 0) & 0xf;
	vsdb_local = *vsdb;
	vsdb->valid = 1;

	phy_addr = ((vsdb->a & 0xf) << 12) |
		   ((vsdb->b & 0xf) <<  8) |
		   ((vsdb->c & 0xf) <<  4) |
		   ((vsdb->d & 0xf) <<  0);
	prxcap->physical_addr = phy_addr;
}

static void set_vsdb_dc_cap(struct rx_cap *prxcap)
{
	if (!prxcap)
		return;

	prxcap->dc_y444 = !!(prxcap->ColorDeepSupport & (1 << 3));
	prxcap->dc_30bit = !!(prxcap->ColorDeepSupport & (1 << 4));
	prxcap->dc_36bit = !!(prxcap->ColorDeepSupport & (1 << 5));
	prxcap->dc_48bit = !!(prxcap->ColorDeepSupport & (1 << 6));
}

static void set_vsdb_dc_420_cap(struct rx_cap *prxcap,
				u8 *edid_offset)
{
	if (!prxcap || !edid_offset)
		return;

	prxcap->dc_30bit_420 = !!(edid_offset[6] & (1 << 0));
	prxcap->dc_36bit_420 = !!(edid_offset[6] & (1 << 1));
	prxcap->dc_48bit_420 = !!(edid_offset[6] & (1 << 2));
}

static void _edid_parsingvendspec(struct dv_info *dv,
				  struct hdr10_plus_info *hdr10_plus,
				  struct cuva_info *cuva,
				 u8 *buf)
{
	u8 *dat = buf;
	u8 *cuva_dat = buf;
	u8 pos = 0;
	u32 ieeeoui = 0;
	u8 length = 0;

	if (!dv || !hdr10_plus || !cuva || !buf)
		return;

	length = dat[pos] & 0x1f;
	pos++;

	if (dat[pos] != 1) {
		HDMITX_ERROR("edid: parsing fail %s[%d]\n", __func__,
			__LINE__);
		return;
	}

	pos++;
	ieeeoui = dat[pos++];
	ieeeoui += dat[pos++] << 8;
	ieeeoui += dat[pos++] << 16;
	HDMITX_DEBUG_EDID("Edid_ParsingVendSpec:ieeeoui=0x%x,len=%u\n", ieeeoui, length);

/*HDR10+ use vsvdb*/
	if (ieeeoui == HDR10_PLUS_IEEE_OUI) {
		memset(hdr10_plus, 0, sizeof(struct hdr10_plus_info));
		hdr10_plus->length = length;
		hdr10_plus->ieeeoui = ieeeoui;
		hdr10_plus->application_version = dat[pos] & 0x3;
		pos++;
		return;
	}
	if (ieeeoui == CUVA_IEEEOUI) {
		memcpy(cuva->rawdata, cuva_dat, 15); /* 15, fixed length */
		cuva->length = cuva_dat[0] & 0x1f;
		cuva->ieeeoui = cuva_dat[2] |
				(cuva_dat[3] << 8) |
				(cuva_dat[4] << 16);
		cuva->system_start_code = cuva_dat[5];
		cuva->version_code = cuva_dat[6] >> 4;
		cuva->display_max_lum = cuva_dat[7] |
					(cuva_dat[8] << 8) |
					(cuva_dat[9] << 16) |
					(cuva_dat[10] << 24);
		cuva->display_min_lum = cuva_dat[11] | (cuva_dat[12] << 8);
		cuva->rx_mode_sup = (cuva_dat[13] >> 6) & 0x1;
		cuva->monitor_mode_sup = (cuva_dat[13] >> 7) & 0x1;
		return;
	}

	if (ieeeoui == DV_IEEE_OUI) {
		/* it is a Dovi block*/
		memset(dv, 0, sizeof(struct dv_info));
		dv->block_flag = CORRECT;
		dv->length = length;
		memcpy(dv->rawdata, dat, dv->length + 1);
		dv->ieeeoui = ieeeoui;
		dv->ver = (dat[pos] >> 5) & 0x7;
		if (dv->ver > 2) {
			dv->block_flag = ERROR_VER;
			return;
		}
		/* Refer to DV 2.9 Page 27 */
		if (dv->ver == 0) {
			if (dv->length == 0x19) {
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				pos++;
				dv->Rx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Ry =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Gx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Gy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Bx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->By =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Wx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Wy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->tminPQ =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->tmaxPQ =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->dm_major_ver = dat[pos] >> 4;
				dv->dm_minor_ver = dat[pos] & 0xf;
				pos++;
				HDMITX_DEBUG_EDID("v0 VSVDB: len=%d, sup_2160p60hz=%d\n",
					dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (dv->ver == 1) {
			if (dv->length == 0x0B) {/* Refer to DV 2.9 Page 33 */
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmax_lum = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tmin_lum = dat[pos] >> 1;
				pos++;
				dv->low_latency = dat[pos] & 0x3;
				dv->Bx = 0x20 | ((dat[pos] >> 5) & 0x7);
				dv->By = 0x08 | ((dat[pos] >> 2) & 0x7);
				pos++;
				dv->Gx = 0x00 | (dat[pos] >> 1);
				dv->Ry = 0x40 | ((dat[pos] & 0x1) |
					((dat[pos + 1] & 0x1) << 1) |
					((dat[pos + 2] & 0x3) << 2));
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				pos++;
				HDMITX_DEBUG_EDID("v1 VSVDB: len=%d, sup_2160p60hz=%d, ll=%d\n",
					dv->length, dv->sup_2160p60hz, dv->low_latency);
			} else if (dv->length == 0x0E) {
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmax_lum = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tmin_lum = dat[pos] >> 1;
				pos += 2; /* byte8 is reserved as 0 */
				dv->Rx = dat[pos++];
				dv->Ry = dat[pos++];
				dv->Gx = dat[pos++];
				dv->Gy = dat[pos++];
				dv->Bx = dat[pos++];
				dv->By = dat[pos++];
				HDMITX_DEBUG_EDID("v1 VSVDB: len=%d, sup_2160p60hz=%d\n",
					dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}
		if (dv->ver == 2) {
			/* v2 VSVDB length could be greater than 0xB
			 * and should not be treated as unrecognized
			 * block. Instead, we should parse it as a regular
			 * v2 VSVDB using just the remaining 11 bytes here
			 */
			if (dv->length >= 0x0B) {
				dv->sup_2160p60hz = 0x1;/*default*/
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_backlight_control = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				dv->backlt_min_luma = dat[pos] & 0x3;
				dv->tminPQ = dat[pos] >> 3;
				pos++;
				dv->Interface = dat[pos] & 0x3;
				dv->parity = (dat[pos] >> 2) & 0x1;
				/* if parity = 0, then not support > 60hz nor 8k */
				dv->sup_1080p120hz = dv->parity;
				dv->tmaxPQ = dat[pos] >> 3;
				pos++;
				dv->sup_10b_12b_444 = ((dat[pos] & 0x1) << 1) |
					(dat[pos + 1] & 0x1);
				dv->Gx = 0x00 | (dat[pos] >> 1);
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				dv->Bx = 0x20 | (dat[pos] & 0x7);
				pos++;
				dv->Ry = 0x40  | (dat[pos] >> 3);
				dv->By = 0x08  | (dat[pos] & 0x7);
				pos++;
				HDMITX_DEBUG_EDID("v2 VSVDB: len=%d, 2160p60hz=%d, Interface=%d\n",
					dv->length, dv->sup_2160p60hz, dv->Interface);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (pos > (dv->length + 1))
			HDMITX_DEBUG_EDID("hdmitx: edid: maybe invalid dv%d data\n", dv->ver);
		return;
	}
	/* future: other new VSVDB add here: */
}

static void edid_parsingvendspec(struct rx_cap *prxcap, u8 *buf)
{
	struct dv_info *dv = &prxcap->dv_info;
	struct dv_info *dv2 = &prxcap->dv_info2;
	struct hdr10_plus_info *hdr10_plus = &prxcap->hdr_info.hdr10plus_info;
	struct hdr10_plus_info *hdr10_plus2 = &prxcap->hdr_info2.hdr10plus_info;
	struct cuva_info *cuva = &prxcap->hdr_info.cuva_info;
	struct cuva_info *cuva2 = &prxcap->hdr_info2.cuva_info;

	u8 pos = 0;
	u32 ieeeoui = 0;

	if (!prxcap || !buf)
		return;

	pos++;

	if (buf[pos] != 1) {
		HDMITX_INFO("edid: parsing fail %s[%d]\n", __func__,
			__LINE__);
		return;
	}

	pos++;
	ieeeoui = buf[pos++];
	ieeeoui += buf[pos++] << 8;
	ieeeoui += buf[pos++] << 16;

	_edid_parsingvendspec(dv, hdr10_plus, cuva, buf);
	_edid_parsingvendspec(dv2, hdr10_plus2, cuva2, buf);
}

/* ----------------------------------------------------------- */
static int edid_parsingy420vdb(struct rx_cap *prxcap, u8 *buf)
{
	u8 tag = 0, ext_tag = 0, data_end = 0;
	u32 pos = 0;

	if (!prxcap || !buf)
		return -1;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f) + 1;
	pos++;
	ext_tag = buf[pos];

	if (tag != 0x7 || ext_tag != 0xe)
		goto INVALID_Y420VDB;

	pos++;
	while (pos < data_end) {
		if (prxcap->VIC_count < VIC_MAX_NUM) {
			if (hdmitx_mode_validate_y420_vic(buf[pos])) {
				store_cea_idx(prxcap, buf[pos]);
				store_y420_idx(prxcap, buf[pos]);
			}
		}
		pos++;
	}

	return 0;

INVALID_Y420VDB:
	HDMITX_ERROR("[%s] it's not a valid y420vdb!\n", __func__);
	return -1;
}

static int _edid_parsedrmsb(struct hdr_info *info, u8 *buf)
{
	u8 tag = 0, ext_tag = 0, data_end = 0;
	u32 pos = 0;

	if (!info || !buf)
		return -1;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f);
	memset(info->rawdata, 0, 7);
	memcpy(info->rawdata, buf, data_end + 1);
	pos++;
	ext_tag = buf[pos];
	if (tag != HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG ||
	    ext_tag != EXTENSION_DRM_STATIC_TAG)
		goto INVALID_DRM_STATIC;
	pos++;
	info->hdr_support = buf[pos];
	pos++;
	info->static_metadata_type1 = buf[pos];
	pos++;
	if (data_end == 3)
		return 0;
	if (data_end == 4) {
		info->lumi_max = buf[pos];
		return 0;
	}
	if (data_end == 5) {
		info->lumi_max = buf[pos];
		info->lumi_avg = buf[pos + 1];
		return 0;
	}
	if (data_end == 6) {
		info->lumi_max = buf[pos];
		info->lumi_avg = buf[pos + 1];
		info->lumi_min = buf[pos + 2];
		return 0;
	}
	return 0;
INVALID_DRM_STATIC:
	HDMITX_ERROR("[%s] it's not a valid DRM STATIC BLOCK\n", __func__);
	return -1;
}

static int edid_parsedrmsb(struct rx_cap *prxcap, u8 *buf)
{
	struct hdr_info *hdr;
	struct hdr_info *hdr2;

	if (!prxcap || !buf)
		return -1;

	hdr = &prxcap->hdr_info;
	hdr2 = &prxcap->hdr_info2;
	_edid_parsedrmsb(hdr, buf);
	_edid_parsedrmsb(hdr2, buf);
	return 0;
}

static int _edid_parsedrmdb(struct hdr_info *info, u8 *buf)
{
	u8 tag = 0, ext_tag = 0, data_end = 0;
	u32 pos = 0;
	u32 type;
	u32 type_length;
	u32 i;
	u32 num;

	if (!info || !buf)
		return -1;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f);
	pos++;
	ext_tag = buf[pos];
	if (tag != HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG ||
	    ext_tag != EXTENSION_DRM_DYNAMIC_TAG)
		goto INVALID_DRM_DYNAMIC;
	pos++;
	data_end--;/*extended tag code byte doesn't need*/

	while (data_end) {
		type_length = buf[pos];
		pos++;
		type = (buf[pos + 1] << 8) | buf[pos];
		pos += 2;
		switch (type) {
		case TS_103_433_SPEC_TYPE:
			num = 1;
			break;
		case ITU_T_H265_SPEC_TYPE:
			num = 2;
			break;
		case TYPE_4_HDR_METADATA_TYPE:
			num = 3;
			break;
		case TYPE_1_HDR_METADATA_TYPE:
		default:
			num = 0;
			break;
		}
		info->dynamic_info[num].of_len = type_length;
		info->dynamic_info[num].type = type;
		info->dynamic_info[num].support_flags = buf[pos];
		pos++;
		for (i = 0; i < type_length - 3; i++) {
			info->dynamic_info[num].optional_fields[i] =
			buf[pos];
			pos++;
		}
		data_end = data_end - (type_length + 1);
	}

	return 0;
INVALID_DRM_DYNAMIC:
	HDMITX_ERROR("[%s] it's not a valid DRM DYNAMIC BLOCK\n", __func__);
	return -1;
}

static int edid_parsedrmdb(struct rx_cap *prxcap, u8 *buf)
{
	struct hdr_info *hdr;
	struct hdr_info *hdr2;

	if (!prxcap || !buf)
		return -1;

	hdr = &prxcap->hdr_info;
	hdr2 = &prxcap->hdr_info2;
	_edid_parsedrmdb(hdr, buf);
	_edid_parsedrmdb(hdr2, buf);
	return 0;
}

static int edid_parsingvfpdb(struct rx_cap *prxcap, u8 *buf)
{
	u32 len;
	enum hdmi_vic svr = HDMI_0_UNKNOWN;

	if (!prxcap || !buf)
		return 0;

	len = buf[0] & 0x1f;
	if (buf[1] != EXTENSION_VFPDB_TAG)
		return 0;
	if (len < 2)
		return 0;

	svr = buf[2];
	if ((svr >= 1 && svr <= 127) ||
	    (svr >= 193 && svr <= 253)) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = svr;
		pr_debug("preferred mode 0 srv %d\n", prxcap->preferred_mode);
		return 1;
	}
	if (svr >= 129 && svr <= 144) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = prxcap->dtd[svr - 129].vic;
		pr_debug("preferred mode 0 dtd %d\n", prxcap->preferred_mode);
		return 1;
	}
	return 0;
}

/* ----------------------------------------------------------- */
static int edid_parsingy420cmdb(struct rx_cap *prxcap, u8 *buf)
{
	u8 tag = 0, ext_tag = 0, length = 0, data_end = 0;
	u32 pos = 0, i = 0;

	if (!prxcap || !buf)
		return -1;

	tag = (buf[pos] >> 5) & 0x7;
	length = buf[pos] & 0x1f;
	data_end = length + 1;
	pos++;
	ext_tag = buf[pos];

	if (tag != 0x7 || ext_tag != 0xf)
		goto INVALID_Y420CMDB;

	if (length == 1) {
		prxcap->y420_all_vic = 1;
		return 0;
	}

	prxcap->bitmap_length = 0;
	prxcap->bitmap_valid = 0;
	memset(prxcap->y420cmdb_bitmap, 0x00, Y420CMDB_MAX);

	pos++;
	if (pos < data_end) {
		prxcap->bitmap_length = data_end - pos;
		prxcap->bitmap_valid = 1;
	}
	while (pos < data_end) {
		if (i < Y420CMDB_MAX)
			prxcap->y420cmdb_bitmap[i] = buf[pos];
		pos++;
		i++;
	}

	return 0;

INVALID_Y420CMDB:
	HDMITX_ERROR("[%s] it's not a valid y420cmdb!\n", __func__);
	return -1;
}

static void edid_parsingdolbyvsadb(struct rx_cap *prxcap, unsigned char *buf)
{
	unsigned char length = 0;
	unsigned char pos = 0;
	unsigned int ieeeoui;
	struct dolby_vsadb_cap *cap;

	if (!prxcap || !buf)
		return;

	cap = &prxcap->dolby_vsadb_cap;
	memset(cap->rawdata, 0, sizeof(cap->rawdata));
	memcpy(cap->rawdata, buf, 7); /* fixed 7 bytes */

	pos = 0;
	length = buf[pos] & 0x1f;
	if (length != 0x06)
		pr_debug("%s[%d]: the length is %d, should be 6 bytes\n",
			__func__, __LINE__, length);

	cap->length = length;
	pos += 2;
	ieeeoui = buf[pos] + (buf[pos + 1] << 8) + (buf[pos + 2] << 16);
	if (ieeeoui != DOVI_IEEEOUI)
		pr_debug("%s[%d]: the ieeeoui is 0x%x, should be 0x%x\n",
			__func__, __LINE__, ieeeoui, DOVI_IEEEOUI);
	cap->ieeeoui = ieeeoui;

	pos += 3;
	cap->dolby_vsadb_ver = buf[pos] & 0x7;
	if (cap->dolby_vsadb_ver)
		pr_debug("%s[%d]: the version is 0x%x, should be 0x0\n",
			__func__, __LINE__, cap->dolby_vsadb_ver);

	cap->spk_center = (buf[pos] >> 4) & 1;
	cap->spk_surround = (buf[pos] >> 5) & 1;
	cap->spk_height = (buf[pos] >> 6) & 1;
	cap->headphone_only = (buf[pos] >> 7) & 1;

	pos++;
	cap->mat_48k_pcm_only = (buf[pos] >> 0) & 1;
}

static bool edid_y420cmdb_fill_all_vic(struct rx_cap *rxcap)
{
	u32 count;
	u32 a, b;

	if (!rxcap)
		return false;

	count = rxcap->VIC_count;

	if (rxcap->y420_all_vic != 1)
		return false;

	a = count / 8;
	a = (a >= Y420CMDB_MAX) ? Y420CMDB_MAX : a;
	b = count % 8;

	if (a > 0)
		memset(&rxcap->y420cmdb_bitmap[0], 0xff, a);

	if (b != 0 && a < Y420CMDB_MAX)
		rxcap->y420cmdb_bitmap[a] = (1 << b) - 1;

	rxcap->bitmap_length = (b == 0) ? a : (a + 1);
	rxcap->bitmap_valid = (rxcap->bitmap_length != 0) ? 1 : 0;

	return true;
}

static bool edid_y420cmdb_postprocess(struct rx_cap *rxcap)
{
	u32 i = 0, j = 0, valid = 0;
	u8 *p = NULL;
	enum hdmi_vic vic;

	if (!rxcap)
		return false;

	if (rxcap->y420_all_vic == 1)
		edid_y420cmdb_fill_all_vic(rxcap);

	if (rxcap->bitmap_valid == 0)
		goto PROCESS_END;

	for (i = 0; i < rxcap->bitmap_length; i++) {
		p = &rxcap->y420cmdb_bitmap[i];
		for (j = 0; j < 8; j++) {
			valid = ((*p >> j) & 0x1);
			vic = rxcap->SVD_VIC[i * 8 + j];
			if (valid != 0 &&
			    hdmitx_mode_validate_y420_vic(vic)) {
				store_y420_idx(rxcap, vic);
			}
		}
	}
	return true;
PROCESS_END:
	return false;
}

static void edid_y420cmdb_reset(struct rx_cap *prxcap)
{
	if (!prxcap)
		return;

	prxcap->bitmap_valid = 0;
	prxcap->bitmap_length = 0;
	prxcap->y420_all_vic = 0;
	memset(prxcap->y420cmdb_bitmap, 0x00, Y420CMDB_MAX);
}

/* ----------------------------------------------------------- */
static void hdmitx_3d_update(struct rx_cap *prxcap)
{
	int j = 0;

	if (!prxcap)
		return;

	for (j = 0; j < 16; j++) {
		if ((prxcap->threeD_MASK_15_0 >> j) & 0x1) {
			/* frame packing */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 0))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.frame_packing = 1;
			/* top and bottom */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 6))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.top_and_bottom = 1;
			/* top and bottom */
			if (prxcap->threeD_Structure_ALL_15_0
				& (1 << 8))
				prxcap->support_3d_format[prxcap->VIC[j]]
				.side_by_side = 1;
		}
	}
}

/* parse Sink 3D information */
static bool hdmitx_edid_3d_parse(struct rx_cap *prxcap, u8 *dat,
				u32 size)
{
	int j = 0;
	u32 base = 0;
	u32 pos = base + 1;
	u32 index = 0;

	if (!prxcap || !dat)
		return false;

	if (dat[base] & (1 << 7))
		pos += 2;
	if (dat[base] & (1 << 6))
		pos += 2;
	if (dat[base] & (1 << 5)) {
		prxcap->threeD_present = dat[pos] >> 7;
		prxcap->threeD_Multi_present = (dat[pos] >> 5) & 0x3;
		pos += 1;
		prxcap->hdmi_vic_LEN = dat[pos] >> 5;
		prxcap->HDMI_3D_LEN = dat[pos] & 0x1f;
		pos += prxcap->hdmi_vic_LEN + 1;

		if (prxcap->threeD_Multi_present == 0x01) {
			prxcap->threeD_Structure_ALL_15_0 =
				(dat[pos] << 8) + dat[pos + 1];
			prxcap->threeD_MASK_15_0 = 0;
			pos += 2;
		}
		if (prxcap->threeD_Multi_present == 0x02) {
			prxcap->threeD_Structure_ALL_15_0 =
				(dat[pos] << 8) + dat[pos + 1];
			pos += 2;
			prxcap->threeD_MASK_15_0 =
			(dat[pos] << 8) + dat[pos + 1];
			pos += 2;
		}
	}
	while (pos < size) {
		if ((dat[pos] & 0xf) < 0x8) {
			/* frame packing */
			if ((dat[pos] & 0xf) == T3D_FRAME_PACKING)
				prxcap->support_3d_format[prxcap->VIC[((dat[pos]
					& 0xf0) >> 4)]].frame_packing = 1;
			/* top and bottom */
			if ((dat[pos] & 0xf) == T3D_TAB)
				prxcap->support_3d_format[prxcap->VIC[((dat[pos]
					& 0xf0) >> 4)]].top_and_bottom = 1;
			pos += 1;
		} else {
			/* SidebySide */
			if ((dat[pos] & 0xf) == T3D_SBS_HALF &&
			    (dat[pos + 1] >> 4) < 0xb) {
				index = (dat[pos] & 0xf0) >> 4;
				prxcap->support_3d_format[prxcap->VIC[index]]
				.side_by_side = 1;
			}
			pos += 2;
		}
	}
	if (prxcap->threeD_MASK_15_0 == 0) {
		for (j = 0; (j < 16) && (j < prxcap->VIC_count); j++) {
			prxcap->support_3d_format[prxcap->VIC[j]]
			.frame_packing = 1;
			prxcap->support_3d_format[prxcap->VIC[j]]
			.top_and_bottom = 1;
			prxcap->support_3d_format[prxcap->VIC[j]]
			.side_by_side = 1;
		}
	} else {
		hdmitx_3d_update(prxcap);
	}
	return true;
}

/* parse Sink 4k2k information */
static void hdmitx_edid_4k2k_parse(struct rx_cap *prxcap, u8 *dat,
				   u32 size)
{
	if (!prxcap || !dat)
		return;

	if (size > 4 || size == 0) {
		pr_debug(EDID "4k2k in edid out of range, SIZE = %d\n", size);
		return;
	}
	while (size--) {
		if (*dat == 1)
			store_cea_idx(prxcap, HDMI_95_3840x2160p30_16x9);
		else if (*dat == 2)
			store_cea_idx(prxcap, HDMI_94_3840x2160p25_16x9);
		else if (*dat == 3)
			store_cea_idx(prxcap, HDMI_93_3840x2160p24_16x9);
		else if (*dat == 4)
			store_cea_idx(prxcap, HDMI_98_4096x2160p24_256x135);
		else
			;
		dat++;
	}
}

static void get_latency(struct rx_cap *prxcap, u8 *val)
{
	if (!prxcap || !val)
		return;

	if (val[0] == 0)
		prxcap->vLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[0] == 0xFF)
		prxcap->vLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->vLatency = (val[0] - 1) * 2;

	if (val[1] == 0)
		prxcap->aLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[1] == 0xFF)
		prxcap->aLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->aLatency = (val[1] - 1) * 2;
}

static void get_ilatency(struct rx_cap *prxcap, u8 *val)
{
	if (!prxcap || !val)
		return;

	if (val[0] == 0)
		prxcap->i_vLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[0] == 0xFF)
		prxcap->i_vLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->i_vLatency = val[0] * 2 - 1;

	if (val[1] == 0)
		prxcap->i_aLatency = LATENCY_INVALID_UNKNOWN;
	else if (val[1] == 0xFF)
		prxcap->i_aLatency = LATENCY_NOT_SUPPORT;
	else
		prxcap->i_aLatency = val[1] * 2 - 1;
}

static void hdmitx_edid_parse_hdmi14(struct rx_cap *prxcap,
	u8 offset, u8 *block_buf, u8 count)
{
	int idx = 0, tmp = 0;

	if (!prxcap || !block_buf)
		return;

	prxcap->ieeeoui = HDMI_IEEE_OUI;
	set_vsdb_phy_addr(prxcap, &block_buf[offset + 3]);

	prxcap->ColorDeepSupport = (count > 5) ? block_buf[offset + 5] : 0;
	set_vsdb_dc_cap(prxcap);
	prxcap->Max_TMDS_Clock1 =
		(count > 6) ? block_buf[offset + 6] : DEFAULT_MAX_TMDS_CLK;
	if (count > 7) {
		tmp = block_buf[offset + 7];
		idx = offset + 8;
		if (tmp & (1 << 6)) {
			u8 val[2];

			val[0] = block_buf[idx];
			val[1] = block_buf[idx + 1];
			get_latency(prxcap, val);
			idx += 2;
			val[0] = block_buf[idx];
			val[1] = block_buf[idx + 1];
			get_ilatency(prxcap, val);
			idx += 2;
		} else if (tmp & (1 << 7)) {
			unsigned char val[2];

			val[0] = block_buf[idx];
			val[1] = block_buf[idx + 1];
			get_latency(prxcap, val);
			idx += 2;
		}
		prxcap->cnc0 = (tmp >> 0) & 1;
		prxcap->cnc1 = (tmp >> 1) & 1;
		prxcap->cnc2 = (tmp >> 2) & 1;
		prxcap->cnc3 = (tmp >> 3) & 1;
		if (tmp & (1 << 5)) {
			idx += 1;
			/* valid 4k */
			if (block_buf[idx] & 0xe0) {
				hdmitx_edid_4k2k_parse(prxcap,
						       &block_buf[idx + 1],
				block_buf[idx] >> 5);
			}
			/* valid 3D */
			if (block_buf[idx - 1] & 0xe0) {
				hdmitx_edid_3d_parse(prxcap, &block_buf[offset + 7],
				count - 7);
			}
		}
	}
}

/* force_vsvdb */
/*  0: no force, use TV's */
/*  1~n: use preset vsvdb 0~n-1 */
/*  255: use current vsvdb_data */
/*       update by module param vsvdb_data */
static unsigned int force_vsvdb;
static unsigned int vsvdb_size = 12;
static unsigned char vsvdb_data[32] = {
	0xeb, 0x01, 0x46, 0xd0, 0x00, 0x45, 0x0b, 0x90,
	0x86, 0x60, 0x76, 0x8f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define PRESET_VSVDB_COUNT 4
static unsigned char tv_vsvdb[PRESET_VSVDB_COUNT][32] = {
	/* source-led */
	{
		0xeb, 0x01, 0x46, 0xd0,
		0x00, 0x45, 0x0b, 0x90,
		0x86, 0x60, 0x76, 0x8f
	},
	/* sink-led */
	{
		0xee, 0x01, 0x46, 0xd0,
		0x00, 0x24, 0x0f, 0x8b,
		0xa8, 0x53, 0x4b, 0x9d,
		0x27, 0x0b, 0x00
	},
	/* sink-led & source-led */
	{
		0xeb, 0x01, 0x46, 0xd0,
		0x00, 0x44, 0x4f, 0x42,
		0x8c, 0x46, 0x56, 0x8e
	},
	/* hdr10+ */
	{
		0xe5, 0x01, 0x8b, 0x84,
		0x90, 0x01
	}
};

module_param(force_vsvdb, uint, 0664);
MODULE_PARM_DESC(force_vsvdb, "\n force_vsvdb\n");
module_param_array(vsvdb_data, byte, &vsvdb_size, 0664);
MODULE_PARM_DESC(vsvdb_data, "\n vsvdb data\n");

/* force_hdr */
/*  0: no force, use TV's */
/*  1~n: use preset drm 0~n-1 */
/*  255: use current drm_data */
/*       update by module param drm_data */
static unsigned int force_hdr;
static unsigned int drm_size = 4;
static unsigned char drm_data[8] = {
	0xe3, 0x06, 0x0d, 0x01, 0x00, 0x00, 0x00, 0x00
};

#define PRESET_DRM_COUNT 3
static unsigned char tv_drm[PRESET_DRM_COUNT][32] = {
	/* hdr10 + hlg */
	{
		0xe3, 0x06, 0x0d, 0x01
	},
	/* hdr10 */
	{
		0xe3, 0x06, 0x05, 0x01
	},
	/* hlg */
	{
		0xe3, 0x06, 0x09, 0x01
	}
};

module_param(force_hdr, uint, 0664);
MODULE_PARM_DESC(force_hdr, "\n force_drm\n");
module_param_array(drm_data, byte, &drm_size, 0664);
MODULE_PARM_DESC(drm_data, "\n drm data\n");

static void hdmitx_edid_parse_ifdb(struct rx_cap *prxcap, u8 *blockbuf)
{
	u8 payload_len;
	u8 len;
	u8 sum_len = 0;

	if (!prxcap || !blockbuf)
		return;

	payload_len = blockbuf[0] & 0x1f;
	/* no additional bytes after extended tag code */
	if (payload_len <= 1)
		return;

	/* begin with an InfoFrame Processing Descriptor */
	if ((blockbuf[2] & 0x1f) != 0)
		HDMITX_ERROR("ERR: IFDB not begin with InfoFrame Processing Descriptor\n");
	sum_len = 1; /* Extended Tag Code len */

	len = (blockbuf[2] >> 5) & 0x7;
	sum_len += (len + 2);
	if (payload_len < sum_len) {
		HDMITX_ERROR("ERR: IFDB length abnormal: %d exceed playload len %d\n",
			sum_len, payload_len);
		return;
	}

	prxcap->additional_vsif_num = blockbuf[3];
	if (payload_len == sum_len)
		return;

	while (sum_len < payload_len) {
		if ((blockbuf[sum_len + 1] & 0x1f) == 1) {
			/* Short Vendor-Specific InfoFrame Descriptor */
			/* HDMITX_INFO(EDID "InfoFrame Type Code: 0x1, len: %d, IEEE: %x\n", */
				/* len, blockbuf[sum_len + 4] << 16 | */
				/* blockbuf[sum_len + 3] << 8 | blockbuf[sum_len + 2]); */
			/* number of additional bytes following the 3-byte OUI */
			len = (blockbuf[sum_len + 1] >> 5) & 0x7;
			sum_len += (len + 1 + 3);
		} else {
			/* Short InfoFrame Descriptor */
			/* HDMITX_INFO(EDID "InfoFrame Type Code: %x, len: %d\n", */
				/* blockbuf[sum_len + 1] & 0x1f, len); */
			len = (blockbuf[sum_len + 1] >> 5) & 0x7;
			sum_len += (len + 1);
		}
	}
}

static void hdmitx_edid_parse_hfscdb(struct rx_cap *prxcap,
	u8 offset, u8 *blockbuf, u8 count)
{
	if (!prxcap || !blockbuf)
		return;

	prxcap->hf_ieeeoui = HDMI_FORUM_IEEE_OUI;
	prxcap->Max_TMDS_Clock2 = blockbuf[offset + 4];
	prxcap->scdc_present = !!(blockbuf[offset + 5] & (1 << 7));
	prxcap->scdc_rr_capable = !!(blockbuf[offset + 5] & (1 << 6));
	prxcap->lte_340mcsc_scramble = !!(blockbuf[offset + 5] & (1 << 3));
	prxcap->max_frl_rate = (blockbuf[offset + 6] & 0xf0) >> 4;
	set_vsdb_dc_420_cap(prxcap, &blockbuf[offset]);

	if (count < 8)
		return;
	prxcap->allm = !!(blockbuf[offset + 7] & (1 << 1));
	prxcap->fva = !!(blockbuf[offset + 7] & (1 << 2));
	prxcap->neg_mvrr = !!(blockbuf[offset + 7] & (1 << 3));
	prxcap->mdelta = !!(blockbuf[offset + 7] & (1 << 5));
	prxcap->qms = !!(blockbuf[offset + 7] & (1 << 6));
	prxcap->fapa_end_extended = !!(blockbuf[offset + 7] & (1 << 7));

	if (count < 10)
		return;
	prxcap->vrr_max = (((blockbuf[offset + 8] & 0xc0) >> 6) << 8) +
				blockbuf[offset + 9];
	prxcap->vrr_min = (blockbuf[offset + 8] & 0x3f);
	prxcap->fapa_start_loc = !!(blockbuf[offset + 7] & (1 << 0));

	if (count < 11)
		return;
	prxcap->dsc_10bpc = !!(blockbuf[offset + 10] & (1 << 0));
	prxcap->dsc_12bpc = !!(blockbuf[offset + 10] & (1 << 1));
	prxcap->dsc_16bpc = !!(blockbuf[offset + 10] & (1 << 2));
	prxcap->dsc_all_bpp = !!(blockbuf[offset + 10] & (1 << 3));
	prxcap->qms_tfr_min = !!(blockbuf[offset + 10] & (1 << 4));
	prxcap->qms_tfr_max = !!(blockbuf[offset + 10] & (1 << 5));
	prxcap->dsc_native_420 = !!(blockbuf[offset + 10] & (1 << 6));
	prxcap->dsc_1p2 = !!(blockbuf[offset + 10] & (1 << 7));
	/* dsc_1p2 shall be cleared (=0) for devices that
	 * do not support FRL (i.e. Max_FRL_Rate=0).
	 */
	if (prxcap->max_frl_rate == 0)
		prxcap->dsc_1p2 = 0;
	if (prxcap->dsc_1p2) {
		if (count < 13) {
			pr_info(EDID "error: dsc_1p2 support, but dsc not complete\n");
			prxcap->dsc_1p2 = 0;
			return;
		}
		/* 3: up to 4 slices and up to (340 MHz/K SliceAdjust)
		 * pixel clock per slice
		 * 4: up to 8 slices and up to (340 MHz/K SliceAdjust)
		 * pixel clock per slice
		 */
		prxcap->dsc_max_slices = blockbuf[offset + 11] & 0xf;
		/* This is value shall be the same or lower than
		 * the physical maximum rate specified by the
		 * Max_FRL_Rate field.
		 */
		prxcap->dsc_max_frl_rate = (blockbuf[offset + 11] >> 4) & 0xf;
		/* The number of bytes is computed as:
		 * 1024 x (1+DSC_ TotalChunkKBytes)
		 */
		prxcap->dsc_total_chunk_bytes = blockbuf[offset + 12] & 0x3f;
	}
}

static inline unsigned short get_2_bytes(u8 *addr)
{
	if (!addr)
		return 0;
	return addr[0] | (addr[1] << 8);
}

static void hdmitx21_edid_parse_sbtmdb(struct rx_cap *prxcap,
	u8 offset, u8 *blockbuf, u8 len)
{
	struct sbtm_info *info;

	if (!prxcap || !blockbuf || !len)
		return;
	if (len < 2)
		return;
	blockbuf = blockbuf + offset;
	/* length should be 2, 3, 5, 7, ... or 29 */
	if (!(len == 2 || (len <= 29 && ((len % 2) == 1))))
		HDMITX_INFO("%s[%d] len is %d\n", __func__, __LINE__, len);
	info = &prxcap->hdr_info.sbtm_info;

	blockbuf++;
	len--;
	if (blockbuf[0])
		info->sbtm_support = 1;
	if (!info->sbtm_support)
		return;
	info->max_sbtm_ver = GET_BITS_FILED(blockbuf[0], 0, 4);
	info->grdm_support = GET_BITS_FILED(blockbuf[0], 5, 2);
	info->drdm_ind = GET_BITS_FILED(blockbuf[0], 7, 1);
	if (info->drdm_ind == 0)
		return;
	info->hgig_cat_drdm_sel = GET_BITS_FILED(blockbuf[1], 0, 3);
	info->use_hgig_drdm = GET_BITS_FILED(blockbuf[1], 4, 1);
	info->maxrgb = GET_BITS_FILED(blockbuf[1], 5, 1);
	info->gamut = GET_BITS_FILED(blockbuf[1], 6, 2);
	blockbuf += 2;
	len -= 2;
	if (info->drdm_ind && !info->gamut && len >= 16) {
		info->red_x = get_2_bytes(&blockbuf[0]);
		info->red_y = get_2_bytes(&blockbuf[2]);
		info->green_x = get_2_bytes(&blockbuf[4]);
		info->green_y = get_2_bytes(&blockbuf[6]);
		info->blue_x = get_2_bytes(&blockbuf[8]);
		info->blue_y = get_2_bytes(&blockbuf[10]);
		info->white_x = get_2_bytes(&blockbuf[12]);
		info->white_y = get_2_bytes(&blockbuf[14]);
		len -= 16;
		blockbuf += 16;
	}
	if (info->drdm_ind && !info->use_hgig_drdm && len >= 2) {
		info->min_bright_10 = blockbuf[0];
		info->peak_bright_100 = blockbuf[1];
		len -= 2;
		blockbuf += 2;
		if (len >= 2) {
			info->p0_exp = GET_BITS_FILED(blockbuf[0], 0, 2);
			info->p0_mant = GET_BITS_FILED(blockbuf[0], 2, 6);
			info->peak_bright_p0 = blockbuf[1];
			len -= 2;
			blockbuf += 2;
		}
		if (len >= 2) {
			info->p1_exp = GET_BITS_FILED(blockbuf[0], 0, 2);
			info->p1_mant = GET_BITS_FILED(blockbuf[0], 2, 6);
			info->peak_bright_p1 = blockbuf[1];
			len -= 2;
			blockbuf += 2;
		}
		if (len >= 2) {
			info->p2_exp = GET_BITS_FILED(blockbuf[0], 0, 2);
			info->p2_mant = GET_BITS_FILED(blockbuf[0], 2, 6);
			info->peak_bright_p2 = blockbuf[1];
			len -= 2;
			blockbuf += 2;
		}
		if (len >= 2) {
			info->p3_exp = GET_BITS_FILED(blockbuf[0], 0, 2);
			info->p3_mant = GET_BITS_FILED(blockbuf[0], 2, 6);
			info->peak_bright_p3 = blockbuf[1];
			/* end parsing */
		}
	}
}

/* refer to CEA-861-G 7.5.1 video data block */
static void _store_vics(struct rx_cap *prxcap, u8 vic_dat)
{
	u8 vic_bit6_0 = vic_dat & (~0x80);
	u8 vic_bit7 = !!(vic_dat & 0x80);

	if (!prxcap)
		return;

	if (vic_bit6_0 >= 1 && vic_bit6_0 <= 64) {
		prxcap->SVD_VIC[prxcap->SVD_VIC_count] = vic_bit6_0;
		prxcap->SVD_VIC_count++;
		/* don't support 640x480p60 */
		if (vic_bit6_0 > 1)
			store_cea_idx(prxcap, vic_bit6_0);
		if (vic_bit7) {
			if (prxcap->native_vic && !prxcap->native_vic2)
				prxcap->native_vic2 = vic_bit6_0;
			if (!prxcap->native_vic)
				prxcap->native_vic = vic_bit6_0;
		}
	} else {
		prxcap->SVD_VIC[prxcap->SVD_VIC_count] = vic_dat;
		prxcap->SVD_VIC_count++;
		store_cea_idx(prxcap, vic_dat);
	}
}

static int hdmitx_edid_cta_block_parse(struct rx_cap *prxcap, u8 *block_buf)
{
	u8 offset, end;
	u8 count;
	u8 tag;
	int i, tmp, idx;
	u8 *vfpdb_offset = NULL;
	u32 aud_flag = 0;

	if (!prxcap || !block_buf)
		return -1;

	end = block_buf[2]; /* CEA description. */
	prxcap->native_Mode = block_buf[1] >= 2 ? block_buf[3] : 0;
	prxcap->number_of_dtd += block_buf[1] >= 2 ? (block_buf[3] & 0xf) : 0;
	/* Initialize SVD_VIC used for SVD storage in the video data block */
	prxcap->SVD_VIC_count = 0;
	memset(prxcap->SVD_VIC, 0, sizeof(prxcap->SVD_VIC));
	/* do not reset anything during parsing as there could be
	 * more than one block. Below variable should be reset once
	 * before parsing and are already being reset before parse
	 *call
	 */
	/* prxcap->native_vic = 0;*/
	/* prxcap->native_vic2 = 0;*/
	/* prxcap->AUD_count = 0;*/

	edid_y420cmdb_reset(prxcap);
	if (end > 127)
		return 0;

	if (block_buf[1] <= 2) {
		/* skip below for loop */
		goto next;
	}
	/* this loop should be parsing when revision number is larger than 2 */
	for (offset = 4 ; offset < end ; ) {
		tag = block_buf[offset] >> 5;
		count = block_buf[offset] & 0x1f;
		switch (tag) {
		case HDMI_EDID_BLOCK_TYPE_AUDIO:
			aud_flag = 1;
			tmp = count / 3;
			idx = prxcap->AUD_count;
			prxcap->AUD_count += tmp;
			offset++;
			for (i = 0; i < tmp; i++) {
				prxcap->RxAudioCap[idx + i].audio_format_code =
					(block_buf[offset + i * 3] >> 3) & 0xf;
				prxcap->RxAudioCap[idx + i].channel_num_max =
					block_buf[offset + i * 3] & 0x7;
				prxcap->RxAudioCap[idx + i].freq_cc =
					block_buf[offset + i * 3 + 1] & 0x7f;
				prxcap->RxAudioCap[idx + i].cc3 =
					block_buf[offset + i * 3 + 2];
			}
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VIDEO:
			offset++;
			for (i = 0; i < count ; i++) {
				/* The SVD in the video data block is stored in SVD_VIC
				 * and mapped with 420 CMDB
				 */
				_store_vics(prxcap, block_buf[offset + i]);
			}
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VENDER:
			offset++;
			if (block_buf[offset] == 0x03 &&
			    block_buf[offset + 1] == 0x0c &&
			    block_buf[offset + 2] == 0x00) {
				hdmitx_edid_parse_hdmi14(prxcap, offset,
							 block_buf, count);
			} else if ((block_buf[offset] == 0xd8) &&
				(block_buf[offset + 1] == 0x5d) &&
				(block_buf[offset + 2] == 0xc4))
				hdmitx_edid_parse_hfscdb(prxcap, offset,
							 block_buf, count);
			offset += count; /* ignore the remains. */
			break;

		case HDMI_EDID_BLOCK_TYPE_SPEAKER:
			offset++;
			prxcap->RxSpeakerAllocation = block_buf[offset];
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VESA:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG:
			{
				u8 ext_tag = 0;

				ext_tag = block_buf[offset + 1];
				switch (ext_tag) {
				case EXTENSION_VENDOR_SPECIFIC:
					edid_parsingvendspec(prxcap, &block_buf[offset]);
					break;
				case EXTENSION_COLORMETRY_TAG:
					prxcap->colorimetry_data =
						block_buf[offset + 2];
					prxcap->colorimetry_data2 =
						block_buf[offset + 2];
					break;
				case EXTENSION_DRM_STATIC_TAG:
					edid_parsedrmsb(prxcap,
							&block_buf[offset]);
					rx_set_hdr_lumi(&block_buf[offset],
							(block_buf[offset] &
							 0x1f) + 1);
					break;
				case EXTENSION_DRM_DYNAMIC_TAG:
					edid_parsedrmdb(prxcap, &block_buf[offset]);
					break;
				case EXTENSION_VFPDB_TAG:
/* Just record VFPDB offset address, call Edid_ParsingVFPDB() after DTD
 * parsing, in case that
 * SVR >=129 and SVR <=144, Interpret as the Kth DTD in the EDID,
 * where K = SVR C 128 (for K=1 to 16)
 */
					vfpdb_offset = &block_buf[offset];
					break;
				case EXTENSION_Y420_VDB_TAG:
					edid_parsingy420vdb(prxcap, &block_buf[offset]);
					break;
				case EXTENSION_Y420_CMDB_TAG:
					edid_parsingy420cmdb(prxcap, &block_buf[offset]);
					break;
				case EXTENSION_SCDB_EXT_TAG:
					hdmitx_edid_parse_hfscdb(prxcap, offset + 1,
							 block_buf, count);
					break;
				case EXTENSION_SBTM_EXT_TAG:
					hdmitx21_edid_parse_sbtmdb(prxcap, offset + 1,
							 block_buf, count);
					break;
				case EXTENSION_DOLBY_VSADB:
					edid_parsingdolbyvsadb(prxcap, &block_buf[offset]);
					break;
				case EXTENSION_IFDB_TAG:
					prxcap->ifdb_present = true;
					hdmitx_edid_parse_ifdb(prxcap, &block_buf[offset]);
					break;
				default:
					break;
				}
			}
			offset += count + 1;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED2:
			offset++;
			offset += count;
			break;

		default:
			break;
		}
	}
next:
	if (force_vsvdb) {
		if (force_vsvdb <= PRESET_VSVDB_COUNT) {
			vsvdb_size = (tv_vsvdb[force_vsvdb - 1][0] & 0x1f) + 1;
			memcpy(vsvdb_data, tv_vsvdb[force_vsvdb - 1],
				vsvdb_size);
		}
		edid_parsingvendspec(prxcap, vsvdb_data);
	}
	if (force_hdr) {
		if (force_hdr <= PRESET_DRM_COUNT) {
			drm_size = (tv_drm[force_hdr - 1][0] & 0x1f) + 1;
			memcpy(drm_data, tv_drm[force_hdr - 1],
				drm_size);
		}
		edid_parsedrmsb(prxcap, drm_data);
	}

	if (aud_flag == 0)
		hdmitx_edid_set_default_aud(prxcap);

	edid_y420cmdb_postprocess(prxcap);

	/* dtds in extended blocks */
	i = 0;
	offset = block_buf[2] + i * 18;
	for ( ; (offset + 18) < 0x7f; i++) {
		edid_dtd_parsing(prxcap, &block_buf[offset]);
		offset += 18;
	}

	if (vfpdb_offset)
		edid_parsingvfpdb(prxcap, vfpdb_offset);

	return 0;
}

static void hdmitx_edid_set_default_aud(struct rx_cap *prxcap)
{
	/* if AUD_count not equal to 0, no need default value */
	if (!prxcap || prxcap->AUD_count)
		return;

	prxcap->AUD_count = 1;
	prxcap->RxAudioCap[0].audio_format_code = 1; /* PCM */
	prxcap->RxAudioCap[0].channel_num_max = 1; /* 2ch */
	prxcap->RxAudioCap[0].freq_cc = 7; /* 32/44.1/48 kHz */
	prxcap->RxAudioCap[0].cc3 = 1; /* 16bit */
}

/* for below cases:
 * for exception process: no CEA vic in parse result
 * DVI case(only one block), HDMI/HDCP CTS(TODO)
 */
static void hdmitx_edid_set_default_vic(struct rx_cap *prxcap)
{
	if (!prxcap)
		return;

	prxcap->VIC_count = 0x4;
	prxcap->VIC[0] = HDMI_3_720x480p60_16x9;
	prxcap->VIC[1] = HDMI_4_1280x720p60_16x9;
	prxcap->VIC[2] = HDMI_5_1920x1080i60_16x9;
	prxcap->VIC[3] = HDMI_16_1920x1080p60_16x9;
	prxcap->native_vic = HDMI_3_720x480p60_16x9;
	HDMITX_INFO("set default vic\n");
}

static bool hdmitx_edid_search_IEEEOUI(char *buf)
{
	int i;

	if (!buf)
		return false;

	for (i = 0; i < 0x180 - 2; i++) {
		if (buf[i] == 0x03 && buf[i + 1] == 0x0c &&
		    buf[i + 2] == 0x00)
			return true;
	}
	return false;
}

static void edid_manufacture_date_parse(struct rx_cap *prxcap,
				      u8 *data)
{
	if (!data)
		return;

	/* week:
	 *	0: not specified
	 *	0x1~0x36: valid week
	 *	0x37~0xfe: reserved
	 *	0xff: model year is specified
	 */
	if (data[0] == 0 || (data[0] >= 0x37 && data[0] <= 0xfe))
		prxcap->manufacture_week = 0;
	else
		prxcap->manufacture_week = data[0];

	/* year:
	 *	0x0~0xf: reserved
	 *	0x10~0xff: year of manufacture,
	 *		or model year(if specified by week=0xff)
	 */
	prxcap->manufacture_year =
		(data[1] <= 0xf) ? 0 : data[1];
}

static void edid_version_parse(struct rx_cap *prxcap,
			      u8 *data)
{
	if (!data)
		return;

	/*
	 *	0x1: edid version 1
	 *	0x0,0x2~0xff: reserved
	 */
	prxcap->edid_version = (data[0] == 0x1) ? 1 : 0;

	/*
	 *	0x0~0x4: revision number
	 *	0x5~0xff: reserved
	 */
	prxcap->edid_revision = (data[1] < 0x5) ? data[1] : 0;
}

static void edid_physical_size_parse(struct rx_cap *prxcap,
				   u8 *data)
{
	if (!prxcap || !data)
		return;

	if (data[0] != 0 && data[1] != 0) {
		/* Here the unit is cm, transfer to mm */
		prxcap->physical_width  = data[0] * 10;
		prxcap->physical_height = data[1] * 10;
	}
}

/* if edid block 0 are all zeros, then consider RX as HDMI device */
static bool edid_zero_data(u8 *buf)
{
	int sum = 0;
	int i = 0;

	if (!buf)
		return false;

	for (i = 0; i < 128; i++)
		sum += buf[i];

	if (sum == 0)
		return true;
	else
		return false;
}

static void dump_dtd_info(struct dtd *t)
{
	if (!t)
		return;

	if (0) {
		pr_debug(EDID "%s[%d]\n", __func__, __LINE__);
		pr_debug(EDID "pixel_clock: %d\n", t->pixel_clock);
		pr_debug(EDID "h_active: %d\n", t->h_active);
		pr_debug(EDID "v_active: %d\n", t->v_active);
		pr_debug(EDID "v_blank: %d\n", t->v_blank);
		pr_debug(EDID "h_sync_offset: %d\n", t->h_sync_offset);
		pr_debug(EDID "h_sync: %d\n", t->h_sync);
		pr_debug(EDID "v_sync_offset: %d\n", t->v_sync_offset);
		pr_debug(EDID "v_sync: %d\n", t->v_sync);
	}
}

static void edid_dtd_parsing(struct rx_cap *prxcap, u8 *data)
{
	const struct hdmi_timing *timing = NULL;
	struct dtd *t;

	if (!prxcap || !data)
		return;

	t = &prxcap->dtd[prxcap->dtd_idx];
	/* if data[0-2] are zeroes, no need parse, and skip*/
	if (data[0] == 0 && data[1] == 0 && data[2] == 0)
		return;
	memset(t, 0, sizeof(struct dtd));
	t->pixel_clock = data[0] + (data[1] << 8);
	t->h_active = (((data[4] >> 4) & 0xf) << 8) + data[2];
	t->h_blank = ((data[4] & 0xf) << 8) + data[3];
	t->v_active = (((data[7] >> 4) & 0xf) << 8) + data[5];
	t->v_blank = ((data[7] & 0xf) << 8) + data[6];
	t->h_sync_offset = (((data[11] >> 6) & 0x3) << 8) + data[8];
	t->h_sync = (((data[11] >> 4) & 0x3) << 8) + data[9];
	t->v_sync_offset = (((data[11] >> 2) & 0x3) << 4) +
		((data[10] >> 4) & 0xf);
	t->v_sync = (((data[11] >> 0) & 0x3) << 4) + ((data[10] >> 0) & 0xf);
	t->h_image_size = (((data[14] >> 4) & 0xf) << 8) + data[12];
	t->v_image_size = ((data[14] & 0xf) << 8) + data[13];
	t->flags = data[17];
/*
 * Special handling of 1080i60hz, 1080i50hz
 */
	if (t->pixel_clock == 7425 && t->h_active == 1920 &&
	    t->v_active == 1080) {
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * Special handling of 480i60hz, 576i50hz
 */
	if ((((t->flags >> 1) & 0x3) == 0) && t->h_active == 1440) {
		if (t->pixel_clock == 2700) /* 576i50hz */
			goto next;
		if ((t->pixel_clock - 2700) < 10) /* 480i60hz */
			t->pixel_clock = 2702;
next:
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * call hdmitx_mode_match_dtd_timing() to check t is matched with VIC
 */
	timing = hdmitx_mode_match_dtd_timing(t);
	if (timing->vic != HDMI_0_UNKNOWN) {
		/* diff 4x3 and 16x9 mode */
		if (timing->vic == HDMI_6_720x480i60_4x3 ||
			timing->vic == HDMI_2_720x480p60_4x3 ||
			timing->vic == HDMI_21_720x576i50_4x3 ||
			timing->vic == HDMI_17_720x576p50_4x3) {
			if (abs(t->v_image_size * 100 / t->h_image_size - 3 * 100 / 4) <= 2)
				t->vic = timing->vic;
			else
				t->vic = timing->vic + 1;
		} else {
			t->vic = timing->vic;
		}
		prxcap->preferred_mode = prxcap->dtd[0].vic; /* Select dtd0 */
		pr_debug(EDID "get dtd%d vic: %d\n",
			prxcap->dtd_idx, t->vic);
		prxcap->dtd_idx++;
		if (t->vic < HDMITX_VESA_OFFSET)
			store_cea_idx(prxcap, t->vic);
		else
			store_vesa_idx(prxcap, t->vic);
	} else {
		dump_dtd_info(t);
	}
}

static void edid_check_pcm_declare(struct rx_cap *prxcap)
{
	int idx_pcm = 0;
	int i;

	if (!prxcap || !prxcap->AUD_count)
		return;

	/* Try to find more than 1 PCMs, RxAudioCap[0] is always basic audio */
	for (i = 1; i < prxcap->AUD_count; i++) {
		if (prxcap->RxAudioCap[i].audio_format_code ==
			prxcap->RxAudioCap[0].audio_format_code) {
			idx_pcm = i;
			break;
		}
	}

	/* Remove basic audio */
	if (idx_pcm) {
		for (i = 0; i < prxcap->AUD_count - 1; i++)
			memcpy(&prxcap->RxAudioCap[i],
			       &prxcap->RxAudioCap[i + 1],
			       sizeof(struct rx_audiocap));
		/* Clear the last audio declaration */
		memset(&prxcap->RxAudioCap[i], 0, sizeof(struct rx_audiocap));
		prxcap->AUD_count--;
	}
}

static bool is_4k60_supported(struct rx_cap *prxcap)
{
	int i = 0;

	if (!prxcap)
		return false;

	for (i = 0; (i < prxcap->VIC_count) && (i < VIC_MAX_NUM); i++) {
		if (((prxcap->VIC[i] & 0xff) == HDMI_96_3840x2160p50_16x9) ||
		    ((prxcap->VIC[i] & 0xff) == HDMI_97_3840x2160p60_16x9)) {
			return true;
		}
	}
	return false;
}

bool is_support_y422(struct rx_cap *prxcap)
{
	if (!prxcap)
		return false;

	if (prxcap->native_Mode & (1 << 4))
		return true;

	return false;
}

static void edid_descriptor_pmt(struct rx_cap *prxcap,
				struct vesa_standard_timing *t,
				u8 *data)
{
	const struct hdmi_timing *timing = NULL;

	if (!prxcap || !t || !data)
		return;

	t->tmds_clk = data[0] + (data[1] << 8);
	t->hactive = data[2] + (((data[4] >> 4) & 0xf) << 8);
	t->hblank = data[3] + ((data[4] & 0xf) << 8);
	t->vactive = data[5] + (((data[7] >> 4) & 0xf) << 8);
	t->vblank = data[6] + ((data[7] & 0xf) << 8);

	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing && (timing->vic < (HDMI_107_3840x2160p60_64x27 + 1))) {
		prxcap->native_vic = timing->vic;
		pr_debug("hdmitx: get PMT vic: %d\n", timing->vic);
	}
	if (timing && timing->vic >= HDMITX_VESA_OFFSET)
		store_vesa_idx(prxcap, timing->vic);
}

static void edid_descriptor_pmt2(struct rx_cap *prxcap,
				 struct vesa_standard_timing *t,
				 u8 *data)
{
	const struct hdmi_timing *timing = NULL;

	if (!prxcap || !t || !data)
		return;

	t->tmds_clk = data[0] + (data[1] << 8);
	t->hactive = data[2] + (((data[4] >> 4) & 0xf) << 8);
	t->hblank = data[3] + ((data[4] & 0xf) << 8);
	t->vactive = data[5] + (((data[7] >> 4) & 0xf) << 8);
	t->vblank = data[6] + ((data[7] & 0xf) << 8);

	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing && timing->vic >= HDMITX_VESA_OFFSET)
		store_vesa_idx(prxcap, timing->vic);
}

static void edid_cvt_timing_3bytes(struct rx_cap *prxcap,
				   struct vesa_standard_timing *t,
				   const u8 *data)
{
	const struct hdmi_timing *timing = NULL;

	if (!prxcap || !t || !data)
		return;

	t->hactive = ((data[0] + (((data[1] >> 4) & 0xf) << 8)) + 1) * 2;
	switch ((data[1] >> 2) & 0x3) {
	case 0:
		t->vactive = t->hactive * 3 / 4;
		break;
	case 1:
		t->vactive = t->hactive * 9 / 16;
		break;
	case 2:
		t->vactive = t->hactive * 5 / 8;
		break;
	case 3:
	default:
		t->vactive = t->hactive * 3 / 5;
		break;
	}
	switch ((data[2] >> 5) & 0x3) {
	case 0:
		t->vsync = 50;
		break;
	case 1:
		t->vsync = 60;
		break;
	case 2:
		t->vsync = 75;
		break;
	case 3:
	default:
		t->vsync = 85;
		break;
	}
	timing = hdmitx_mode_match_vesa_timing(t);
	if (timing->vic != HDMI_0_UNKNOWN)
		t->vesa_timing = timing->vic;
}

static void edid_cvt_timing(struct rx_cap *prxcap, u8 *data)
{
	int i;
	struct vesa_standard_timing t;

	if (!prxcap || !data)
		return;

	for (i = 0; i < 4; i++) {
		memset(&t, 0, sizeof(struct vesa_standard_timing));
		edid_cvt_timing_3bytes(prxcap, &t, &data[i * 3]);
		if (t.vesa_timing) {
			if (t.vesa_timing >= HDMITX_VESA_OFFSET)
				store_vesa_idx(prxcap, t.vesa_timing);
		}
	}
}

static void check_dv_truly_support(struct rx_cap *prxcap, struct dv_info *dv)
{
	u32 max_tmds_clk = 0;

	if (!prxcap || !dv)
		return;

	if (dv->ieeeoui == DV_IEEE_OUI && dv->ver <= 2) {
		/* check max tmds rate to determine if 4k60 DV can truly be
		 * supported.
		 */
		if (prxcap->Max_TMDS_Clock2) {
			max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
		} else {
			/* Default min is 74.25 / 5 */
			if (prxcap->Max_TMDS_Clock1 < 0xf)
				prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
			max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
		}
		if (dv->ver == 0)
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if (dv->ver == 1 && dv->length == 0xB) {
			if (dv->low_latency == 0x00) {
				/*standard mode */
				dv->sup_2160p60hz = dv->sup_2160p60hz &&
							(max_tmds_clk >= 594);
			} else if (dv->low_latency == 0x01) {
				/* both standard and LL are supported. 4k60 LL
				 * DV support should/can be determined using
				 * video formats supported inthe E-EDID as flag
				 * sup_2160p60hz might not be set.
				 */
				if ((dv->sup_2160p60hz ||
				     is_4k60_supported(prxcap)) &&
				    max_tmds_clk >= 594)
					dv->sup_2160p60hz = 1;
				else
					dv->sup_2160p60hz = 0;
			}
		}

		if (dv->ver == 1 && dv->length == 0xE)
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if (dv->ver == 2) {
			/* 4k60 DV support should be determined using video
			 * formats supported in the EEDID as flag sup_2160p60hz
			 * is not applicable for VSVDB V2.
			 */
			if (is_4k60_supported(prxcap) && max_tmds_clk >= 594)
				dv->sup_2160p60hz = 1;
			else
				dv->sup_2160p60hz = 0;
		}
	}
}

bool hdmitx_find_philips(u8 *edid_buf)
{
	int j;
	int length = sizeof(vendor_id) / sizeof(struct edid_venddat_t);

	if (!edid_buf)
		return false;

	for (j = 0; j < length; j++) {
		if (memcmp(edid_buf, &vendor_id[j], sizeof(struct edid_venddat_t)) == 0)
			return true;
	}
	return false;
}

/*
 * if the EDID is invalid, then set the fallback mode
 * Resolution & RefreshRate:
 *   1920x1080p60hz 16:9
 *   1280x720p60hz 16:9 (default)
 *   720x480p 16:9
 * ColorSpace: RGB
 * ColorDepth: 8bit
 */
static void edid_set_fallback_mode(struct rx_cap *prxcap)
{
	struct vsdb_phyaddr *phyaddr;

	if (!prxcap)
		return;

	/* EDID extended blk chk error, set the 720p60, rgb,8bit */
	phyaddr = &prxcap->vsdb_phy_addr;
	prxcap->ieeeoui = HDMI_IEEE_OUI;

	/* set the default cec physical address as 0xffff */
	phyaddr->a = 0xf;
	phyaddr->b = 0xf;
	phyaddr->c = 0xf;
	phyaddr->d = 0xf;
	phyaddr->valid = 0;
	prxcap->physical_addr = 0xffff;

	prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK; /* 165MHZ / 5 */
	prxcap->native_Mode = 0; /* only RGB */
	prxcap->dc_y444 = 0; /* only 8bit */
	prxcap->VIC_count = 0x3;
	prxcap->VIC[0] = HDMI_16_1920x1080p60_16x9;
	prxcap->VIC[1] = HDMI_4_1280x720p60_16x9;
	prxcap->VIC[2] = HDMI_3_720x480p60_16x9;
	prxcap->native_vic = HDMI_4_1280x720p60_16x9;
	HDMITX_INFO("set default vic 720p60hz\n");

	hdmitx_edid_set_default_aud(prxcap);
}

static void _edid_parse_base_structure(struct rx_cap *prxcap, unsigned char *EDID_buf)
{
	unsigned char checksum;
	unsigned char zero_numbers;
	unsigned char cta_block_count;
	int i;

	if (!prxcap || !EDID_buf)
		return;

	edid_parsing_id_manufacturer_name(prxcap, &EDID_buf[8]);
	edid_parsing_id_product_code(prxcap, &EDID_buf[0x0A]);
	edid_parsing_id_serial_number(prxcap, &EDID_buf[0x0C]);

	edid_established_timings(prxcap, &EDID_buf[0x23]);

	edid_manufacture_date_parse(prxcap, &EDID_buf[16]);

	edid_version_parse(prxcap, &EDID_buf[18]);

	edid_physical_size_parse(prxcap, &EDID_buf[21]);
	prxcap->blk0_chksum = EDID_buf[0x7F];

	cta_block_count = hdmitx_edid_get_cta_block_count(EDID_buf);

	if (cta_block_count == 0) {
		HDMITX_INFO("EDID BlockCount=0\n");
		/* DVI case judgement: only contains one block and
		 * checksum valid
		 */
		checksum = 0;
		zero_numbers = 0;
		for (i = 0; i < 128; i++) {
			checksum += EDID_buf[i];
			if (EDID_buf[i] == 0)
				zero_numbers++;
		}
		HDMITX_DEBUG_EDID("edid blk0 checksum:%d ext_flag:%d\n",
			checksum, EDID_buf[0x7e]);
		if ((checksum & 0xff) == 0)
			prxcap->ieeeoui = 0;
		else
			prxcap->ieeeoui = HDMI_IEEE_OUI;
		if (zero_numbers > 120)
			prxcap->ieeeoui = HDMI_IEEE_OUI;
		hdmitx_edid_set_default_vic(prxcap);
	}
}

static int xtochar(u8 value, u8 *checksum)
{
	if (!checksum)
		return 0;
	if (((value  >> 4) & 0xf) <= 9)
		checksum[0] = ((value  >> 4) & 0xf) + '0';
	else
		checksum[0] = ((value  >> 4) & 0xf) - 10 + 'a';

	if ((value & 0xf) <= 9)
		checksum[1] = (value & 0xf) + '0';
	else
		checksum[1] = (value & 0xf) - 10 + 'a';

	return 0;
}

static int update_edid_chksum(struct rx_cap *prxcap, u8 *edid_buf)
{
	u32 i;
	unsigned char tmp_chksum[4] = {0};

	if (!prxcap || !edid_buf)
		return -EINVAL;

	memset(prxcap->hdmichecksum, 0, sizeof(prxcap->hdmichecksum));

	/* get the first 4 sub-blocks chksum */
	for (i = 0; i < 4; i++)
		tmp_chksum[i] = edid_buf[i * 128 + 127];

	prxcap->hdmichecksum[0] = '0';
	prxcap->hdmichecksum[1] = 'x';

	for (i = 0; i < 4; i++)
		xtochar(tmp_chksum[i], &prxcap->hdmichecksum[2 * i + 2]);

	return 0;
}

int hdmitx_edid_parse(struct rx_cap *prxcap, u8 *edid_buf)
{
	unsigned char cta_block_count;
	unsigned char block_count;
	int i;
	int idx[4];
	struct dv_info *dv;
	u8 edid_check = 0;

	if (!edid_buf || !prxcap)
		return -1;

	edid_check = prxcap->edid_check;
	cta_block_count = hdmitx_edid_get_cta_block_count(edid_buf);
	block_count = hdmitx_edid_get_block_count(edid_buf);
	dv = &prxcap->dv_info;
	prxcap->edid_parsing = hdmitx_edid_check_data_valid(edid_check, edid_buf);

	prxcap->head_err = hdmitx_edid_header_invalid(edid_check, &edid_buf[0]);
	for (i = 0; i < block_count; i++) {
		if (_check_edid_blk_chksum(edid_check, edid_buf + i * 128) == 0) {
			prxcap->chksum_err = 1;
			break;
		}
	}

	pr_debug(EDID "EDID Parser:\n");

	if (hdmitx_edid_check_data_valid(edid_check, edid_buf) == false) {
		edid_set_fallback_mode(prxcap);
		HDMITX_INFO("set fallback mode\n");
		return 0;
	}
	if (_check_base_structure(edid_check, edid_buf))
		_edid_parse_base_structure(prxcap, edid_buf);

	for (i = 1; i <= cta_block_count; i++) {
		if (edid_buf[i * 0x80] == 0x02 || edid_check & 0x01)
			hdmitx_edid_cta_block_parse(prxcap, &edid_buf[i * 0x80]);
	}

	edid_check_pcm_declare(prxcap);
	/* move parts that may contain cea timing parse behind
	 * VDB parse, so that to not affect VDB index which
	 * will be used in Y420CMDB map
	 */
	edid_standardtiming(prxcap, &edid_buf[0x26], 8);
	edid_parseceatiming(prxcap, &edid_buf[0x36]);
/*
 * Because DTDs are not able to represent some Video Formats, which can be
 * represented as SVDs and might be preferred by Sinks, the first DTD in the
 * base EDID data structure and the first SVD in the first CEA Extension can
 * differ. When the first DTD and SVD do not match and the total number of
 * DTDs defining Native Video Formats in the whole EDID is zero, the first
 * SVD shall take precedence.
 */
	if (!prxcap->flag_vfpdb &&
	    prxcap->preferred_mode != prxcap->VIC[0] &&
	    prxcap->number_of_dtd == 0) {
		pr_debug(EDID "change preferred_mode from %d to %d\n",
			prxcap->preferred_mode,	prxcap->VIC[0]);
		prxcap->preferred_mode = prxcap->VIC[0];
	}

	idx[0] = EDID_DETAILED_TIMING_DES_BLOCK0_POS;
	idx[1] = EDID_DETAILED_TIMING_DES_BLOCK1_POS;
	idx[2] = EDID_DETAILED_TIMING_DES_BLOCK2_POS;
	idx[3] = EDID_DETAILED_TIMING_DES_BLOCK3_POS;
	for (i = 0; i < 4; i++) {
		if ((edid_buf[idx[i]]) && (edid_buf[idx[i] + 1])) {
			struct vesa_standard_timing t;

			memset(&t, 0, sizeof(struct vesa_standard_timing));
			if (i == 0)
				edid_descriptor_pmt(prxcap, &t, &edid_buf[idx[i]]);
			if (i == 1)
				edid_descriptor_pmt2(prxcap, &t, &edid_buf[idx[i]]);
			continue;
		}
		switch (edid_buf[idx[i] + 3]) {
		case TAG_STANDARD_TIMINGS:
			edid_standardtiming(prxcap, &edid_buf[idx[i] + 5], 6);
			break;
		case TAG_CVT_TIMING_CODES:
			edid_cvt_timing(prxcap, &edid_buf[idx[i] + 6]);
			break;
		case TAG_ESTABLISHED_TIMING_III:
			edid_standard_timing_iii(prxcap, &edid_buf[idx[i] + 6]);
			break;
		case TAG_RANGE_LIMITS:
			break;
		case TAG_DISPLAY_PRODUCT_NAME_STRING:
			edid_receiverproductnameparse(prxcap, &edid_buf[idx[i] + 5]);
			break;
		default:
			break;
		}
	}

	if (hdmitx_edid_search_IEEEOUI(&edid_buf[128])) {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
		pr_debug(EDID "find IEEEOUT\n");
	} else {
		prxcap->ieeeoui = 0x0;
		pr_debug(EDID "not find IEEEOUT\n");
	}

	/* strictly DVI device judgement */
	/* valid EDID & no audio tag & no IEEEOUI */
	if (hdmitx_edid_check_data_valid(edid_check, &edid_buf[0]) &&
		!hdmitx_edid_search_IEEEOUI(&edid_buf[128])) {
		prxcap->ieeeoui = 0x0;
		pr_debug(EDID "sink is DVI device\n");
	} else {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
	}
	if (edid_zero_data(edid_buf))
		prxcap->ieeeoui = HDMI_IEEE_OUI;

	if (!prxcap->AUD_count && !prxcap->ieeeoui)
		hdmitx_edid_set_default_aud(prxcap);
	/* CEA-861F 7.5.2  If only Basic Audio is supported,
	 * no Short Audio Descriptors are necessary.
	 */
	if (!prxcap->AUD_count)
		hdmitx_edid_set_default_aud(prxcap);

	update_edid_chksum(prxcap, edid_buf);

	if (!hdmitx_edid_valid_block_num(&edid_buf[0])) {
		prxcap->ieeeoui = HDMI_IEEE_OUI;
		HDMITX_INFO("Invalid edid, consider RX as HDMI device\n");
	}
	/* EDID parsing complete - check if 4k60/50 DV can be truly supported */
	dv = &prxcap->dv_info;
	check_dv_truly_support(prxcap, dv);
	dv = &prxcap->dv_info2;
	check_dv_truly_support(prxcap, dv);
// TODO	hdmitx_device->vend_id_hit = hdmitx_find_philips(tx_comm);
	/* For some receivers, they don't claim the screen size
	 * and re-calculate it from the h/v image size from dtd
	 * the unit of screen size is cm, but the unit of image size is mm
	 */
	if (prxcap->physical_width == 0 || prxcap->physical_height == 0) {
		struct dtd *t = &prxcap->dtd[0];

		prxcap->physical_width  = t->h_image_size;
		prxcap->physical_height = t->v_image_size;
	}

	/* if edid are all zeroes, or no VIC, set default vic */
	if (edid_zero_data(edid_buf) || prxcap->VIC_count == 0)
		hdmitx_edid_set_default_vic(prxcap);

	if (prxcap->ieeeoui != HDMI_IEEE_OUI)
		prxcap->physical_addr = 0xffff;

	return 0;
}

void hdmitx_edid_buffer_clear(u8 *edid_buf, int size)
{
	if (!edid_buf)
		return;

	memset(edid_buf, 0, size);
}

/* Clear the Parse result of HDMI Sink's EDID. */
void hdmitx_edid_rxcap_clear(struct rx_cap *prxcap)
{
	char tmp[2] = {0};
	u8 ret = 0;

	if (!prxcap)
		return;

	ret = prxcap->edid_check;
	memset(prxcap, 0, sizeof(struct rx_cap));
	prxcap->edid_check = ret;

	/* Note: in most cases, we think that rx is tv and the default
	 * IEEEOUI is HDMI Identifier
	 */
	prxcap->ieeeoui = HDMI_IEEE_OUI;

	prxcap->edid_parsing = 0;
	hdmitx_edid_set_default_aud(prxcap);
	rx_set_hdr_lumi(&tmp[0], 2);
	//rx_set_receiver_edid(&tmp[0], 2);
	phy_addr_clear(&prxcap->vsdb_phy_addr);
}

/*
 * print one block data of edid
 */
#define TMP_EDID_BUF_SIZE	(256 + 8)
static void hdmitx_edid_blk_print(unsigned char *blk, unsigned int blk_idx)
{
	unsigned int i, pos;
	unsigned char *tmp_buf = NULL;

	if (!blk)
		return;

	tmp_buf = kmalloc(TMP_EDID_BUF_SIZE, GFP_KERNEL);
	if (!tmp_buf)
		return;

	memset(tmp_buf, 0, TMP_EDID_BUF_SIZE);
	HDMITX_INFO("edid: blk%d raw data\n", blk_idx);
	for (i = 0, pos = 0; i < 128; i++) {
		pos += sprintf(tmp_buf + pos, "%02x", blk[i]);
		if (((i + 1) & 0x3f) == 0)    /* print 64 bytes a line */
			pos += sprintf(tmp_buf + pos, "\n");
	}
	HDMITX_INFO("%s", tmp_buf);
	kfree(tmp_buf);
}

/*
 * check EDID buf contains valid block numbers
 */
unsigned int hdmitx_edid_valid_block_num(unsigned char *edid_buf)
{
	unsigned int valid_blk_no = 0;
	unsigned int i = 0, j = 0;
	unsigned int tmp_chksum = 0;

	if (!edid_buf)
		return 0;

	for (j = 0; j < EDID_MAX_BLOCK; j++) {
		for (i = 0; i < 128; i++)
			tmp_chksum += edid_buf[i + j * 128];
		if (tmp_chksum != 0) {
			valid_blk_no++;
			if ((tmp_chksum & 0xff) == 0)
				pr_debug(EDID "check sum valid\n");
			else
				pr_warn(EDID "check sum invalid\n");
		}
		tmp_chksum = 0;
	}
	/* EEODB case */
	if (edid_buf[128 + 4] == EXTENSION_EEODB_EXT_TAG &&
		edid_buf[128 + 5] == EXTENSION_EEODB_EXT_CODE)
		valid_blk_no = edid_buf[128 + 6] + 1;
	return valid_blk_no;
}

/*
 * print out EDID_buf
 */
void hdmitx_edid_print(u8 *edid_buf)
{
	u32 valid_blk_no = 0;
	u32 blk_idx = 0;

	if (!edid_buf)
		return;

	/* calculate valid edid block numbers */
	valid_blk_no = hdmitx_edid_valid_block_num(edid_buf);

	if (valid_blk_no == 0) {
		pr_debug(EDID "raw data are all zeroes\n");
	} else {
		for (blk_idx = 0; blk_idx < valid_blk_no; blk_idx++)
			hdmitx_edid_blk_print(&edid_buf[blk_idx * 128], blk_idx);
	}
}

int hdmitx_edid_print_sink_cap(const struct rx_cap *prxcap,
		char *buffer, int buffer_len)
{
	int i, pos = 0;

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Manufacturer Name: %s\n", prxcap->IDManufacturerName);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Product Code: %02x%02x\n",
		prxcap->IDProductCode[0], prxcap->IDProductCode[1]);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Serial Number: %02x%02x%02x%02x\n",
		prxcap->IDSerialNumber[0],
		prxcap->IDSerialNumber[1],
		prxcap->IDSerialNumber[2],
		prxcap->IDSerialNumber[3]);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Rx Product Name: %s\n", prxcap->ReceiverProductName);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Manufacture Week: %d\n", prxcap->manufacture_week);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Manufacture Year: %d\n", prxcap->manufacture_year + 1990);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"Physical size(mm): %d x %d\n",
		prxcap->physical_width, prxcap->physical_height);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"EDID Version: %d.%d\n",
		prxcap->edid_version, prxcap->edid_revision);

/*	pos += snprintf(buffer + pos, buffer_len - pos,
 *		"EDID block number: 0x%x\n", tx_comm->EDID_buf[0x7e]);
 *
 *
 *	pos += snprintf(buffer + pos, buffer_len - pos,
 *		"Source Physical Address[a.b.c.d]: %x.%x.%x.%x\n",
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.a,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.b,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.c,
 *		hdmitx_device->hdmi_info.vsdb_phy_addr.d);
 */

	// TODO native_vic2
	pos += snprintf(buffer + pos, buffer_len - pos,
		"native Mode %x, VIC (native %d):\n",
		prxcap->native_Mode, prxcap->native_vic);

	pos += snprintf(buffer + pos, buffer_len - pos,
		"ColorDeepSupport %x\n", prxcap->ColorDeepSupport);

	for (i = 0; i < prxcap->VIC_count ; i++) {
		pos += snprintf(buffer + pos, buffer_len - pos, "%d ",
		prxcap->VIC[i]);
	}
	pos += snprintf(buffer + pos, buffer_len - pos, "\n");
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Audio {format, channel, freq, cce}\n");
	for (i = 0; i < prxcap->AUD_count; i++) {
		pos += snprintf(buffer + pos, buffer_len - pos,
			"{%d, %d, %x, %x}\n",
			prxcap->RxAudioCap[i].audio_format_code,
			prxcap->RxAudioCap[i].channel_num_max,
			prxcap->RxAudioCap[i].freq_cc,
			prxcap->RxAudioCap[i].cc3);
	}
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Speaker Allocation: %x\n", prxcap->RxSpeakerAllocation);
	pos += snprintf(buffer + pos, buffer_len - pos,
		"Vendor: 0x%x ( %s device)\n",
		prxcap->ieeeoui, (prxcap->ieeeoui) ? "HDMI" : "DVI");

	pos += snprintf(buffer + pos, buffer_len - pos,
		"MaxTMDSClock1 %d MHz\n", prxcap->Max_TMDS_Clock1 * 5);

	if (prxcap->hf_ieeeoui) {
		pos += snprintf(buffer + pos, buffer_len - pos, "Vendor2: 0x%x\n",
			prxcap->hf_ieeeoui);
		pos += snprintf(buffer + pos, buffer_len - pos, "MaxTMDSClock2 %d MHz\n",
			prxcap->Max_TMDS_Clock2 * 5);
	}

	pos += snprintf(buffer + pos, buffer_len - pos, "MaxFRLRate: %d\n",
		prxcap->max_frl_rate);

	if (prxcap->allm)
		pos += snprintf(buffer + pos, buffer_len - pos, "ALLM: %x\n",
				prxcap->allm);

	if (prxcap->cnc3)
		pos += snprintf(buffer + pos, buffer_len - pos, "Game/CNC3: %x\n",
				prxcap->cnc3);

	pos += snprintf(buffer + pos, buffer_len - pos, "vLatency: ");
	if (prxcap->vLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->vLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos,
			" %d\n", prxcap->vLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "aLatency: ");
	if (prxcap->aLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->aLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->aLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "i_vLatency: ");
	if (prxcap->i_vLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->i_vLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->i_vLatency);

	pos += snprintf(buffer + pos, buffer_len - pos, "i_aLatency: ");
	if (prxcap->i_aLatency == LATENCY_INVALID_UNKNOWN)
		pos += snprintf(buffer + pos, buffer_len - pos,
				" Invalid/Unknown\n");
	else if (prxcap->i_aLatency == LATENCY_NOT_SUPPORT)
		pos += snprintf(buffer + pos, buffer_len - pos,
			" UnSupported\n");
	else
		pos += snprintf(buffer + pos, buffer_len - pos, " %d\n",
			prxcap->i_aLatency);

	if (prxcap->colorimetry_data)
		pos += snprintf(buffer + pos, buffer_len - pos,
			"ColorMetry: 0x%x\n", prxcap->colorimetry_data);

	pos += snprintf(buffer + pos, buffer_len - pos, "SCDC: %x\n",
		prxcap->scdc_present);

	pos += snprintf(buffer + pos, buffer_len - pos, "RR_Cap: %x\n",
		prxcap->scdc_rr_capable);
	pos += snprintf(buffer + pos, buffer_len - pos, "LTE_340M_Scramble: %x\n",
		prxcap->lte_340mcsc_scramble);

	/* dsc capability */
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_10bpc: %d\n",
		 prxcap->dsc_10bpc);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_12bpc: %d\n",
		 prxcap->dsc_12bpc);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_16bpc: %d\n",
		 prxcap->dsc_16bpc);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_all_bpp: %d\n",
		 prxcap->dsc_all_bpp);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_native_420: %d\n",
		 prxcap->dsc_native_420);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_1p2: %d\n",
		 prxcap->dsc_1p2);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_max_slices: 0x%x(%d slices)\n",
		 prxcap->dsc_max_slices, dsc_max_slices_num[prxcap->dsc_max_slices]);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_max_frl_rate: 0x%x\n",
		 prxcap->dsc_max_frl_rate);
	pos += snprintf(buffer + pos, buffer_len - pos, "dsc_total_chunk_bytes: 0x%x\n",
		 prxcap->dsc_total_chunk_bytes);

	if (prxcap->dv_info.ieeeoui == DOVI_IEEEOUI)
		pos += snprintf(buffer + pos, buffer_len - pos,
			"  DolbyVision%d", prxcap->dv_info.ver);

	if (prxcap->hdr_info2.hdr_support)
		pos += snprintf(buffer + pos, buffer_len - pos, "  HDR/%d",
			prxcap->hdr_info2.hdr_support);
	if (prxcap->hdr_info.sbtm_info.sbtm_support)
		pos += snprintf(buffer + pos, buffer_len - pos, "  SBTM");
	if (prxcap->dc_y444 || prxcap->dc_30bit || prxcap->dc_30bit_420)
		pos += snprintf(buffer + pos, buffer_len - pos, "  DeepColor");
	pos += snprintf(buffer + pos, buffer_len - pos, "\n");
	pos += snprintf(buffer + pos, buffer_len - pos, "additional_vsif_num: %d\n",
			prxcap->additional_vsif_num);
	pos += snprintf(buffer + pos, buffer_len - pos, "ifdb_present: %d\n",
			prxcap->ifdb_present);
	/* for checkvalue which maybe used by application to adjust
	 * whether edid is changed
	 */
	pos += snprintf(buffer + pos, buffer_len - pos,
			"checkvalue: %s\n", prxcap->hdmichecksum);

	return pos;
}

