// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPTX_timing.h>
#include "DPTX_IP/dptx_IP_ops.h"
#include "dptx_common.h"

#define DPTX_EDID_READ_RETRY_MAX   5
#define DPTX_AUX_I2C_READ_BYTES    16

#define DPTX_EDID_SIZE             128

static const unsigned char base_block_header[8] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

// EDID Base Block Identification                     ****
#define EDID_BASE_BLOCK_ID_SN                         0xff
#define EDID_BASE_BLOCK_ID_ASCII_STR                  0xfe
#define EDID_BASE_BLOCK_ID_RANGE_TIMING               0xfd
#define EDID_BASE_BLOCK_ID_PRODUCT_NAME               0xfc
#define EDID_BASE_BLOCK_ID_DETAIL_TIMING              0x01
// EDID Extended Block Header                         ****
#define EXTENDED_HEADER_EDID_CEA_861                  0x02
#define EXTENDED_HEADER_DisplayID                     0x70
// DisplayID 1.3/2.3 Product Identification           ****
#define DisplayID_Display_Parameters_1                0x01
#define DisplayID_Color_Characteristics               0x02
#define DisplayID_Type_I_Timing                       0x03
#define DisplayID_Type_II_Timing                      0x04
#define DisplayID_Type_III_Timing                     0x05
#define DisplayID_Type_IV_Timing                      0x06
#define DisplayID_VESA_Timing_Standard                0x07
#define DisplayID_CEA_Timing_Standard                 0x08
#define DisplayID_Video_Timing_Range                  0x09
#define DisplayID_Product_SerialNumber                0x0A
#define DisplayID_General_Purpose_ASCII_String	      0x0B
#define DisplayID_Display_Device_Data                 0x0C
#define DisplayID_Interface_Power_Sequencing          0x0D
#define DisplayID_Transfer_Characteristics            0x0E
#define DisplayID_Display_Interface_Data              0x0F
#define DisplayID_Stereo_Display_Interface_1          0x10
#define DisplayID_Type_V_Timing                       0x11
#define DisplayID_Type_VI_Timing                      0x13
#define DisplayID_Vendor_Specific_1                   0x7F
#define DisplayID_Product_Identification              0x20
#define DisplayID_Display_Parameters_2                0x21
#define DisplayID_Type_VII_Detailed_Timing            0x22
#define DisplayID_Type_VIII_Enumerated_Timing_Code    0x23
#define DisplayID_Type_IX_Formula_Based_Timing        0x24
#define DisplayID_Dynamic_Video_Timing_Range_Limits   0x25
#define DisplayID_Display_Interface_Features          0x26
#define DisplayID_Stereo_Display_Interface_2          0x27
#define DisplayID_Tiled_Display_Topology              0x28
#define DisplayID_ContainerID                         0x29
#define DisplayID_Vendor_Specific_2                   0x7E
#define DisplayID_CTA_DisplayID                       0x81
// EDID CEA-861 Block Identification                  ****
#define EDID_CEA_861_TAG_AUDIO                        0x01
#define EDID_CEA_861_TAG_VIDEO                        0x02
#define EDID_CEA_861_TAG_VENDOR_SPEC                  0x03
#define EDID_CEA_861_TAG_SPEAKER_ALLOCATION           0x04
#define EDID_CEA_861_TAG_VESA_DSIP_TRANS              0x05
#define EDID_CEA_861_TAG_EXT_FLAG                     0x07

void dptx_edid_print_raw(unsigned char *_buf)
{
	unsigned short c;
	unsigned char i, j, n, edid_idx, edid_blocks;
	char edid_pr_buf[64];

	edid_blocks = 1 + _buf[126];

	for (edid_idx = 0; edid_idx < edid_blocks; edid_idx++) {
		printf("EDID block %u Raw data:\n", edid_idx);
		for (i = 0; i < 128; i += 16) {
			n = 0;
			memset(edid_pr_buf, 0, 64);
			for (j = 0; j < 16; j++) {
				c = edid_idx * 128 + i + j;
				n += sprintf(edid_pr_buf + n, " %02x", _buf[c]);
			}
			printf("%s\n", edid_pr_buf);
		}
	}
}

void dptx_edid_print_parsed(struct dptx_drv_s *dptx)
{
	unsigned char i;

	pr_info("Manufacturer: %s\n"
		"Product ID:   0x%04x\n"
		"Product SN:   0x%08x\n"
		"Product Time: week %d of %d\n"
		"EDID Version: %04x\n",
		dptx->edid_info.manufacturer_id,
		dptx->edid_info.product_id, dptx->edid_info.product_sn,
		dptx->edid_info.week, dptx->edid_info.year + 1990,
		dptx->edid_info.version);
	if (dptx->edid_info.name[0])
		pr_info("Monitor Name: %s\n", dptx->edid_info.name);
	if (dptx->edid_info.asc_string[0])
		pr_info("Monitor Text: %s\n", dptx->edid_info.asc_string);
	if (dptx->edid_info.serial_num[0])
		pr_info("Monitor SN:   %s\n", dptx->edid_info.serial_num);
	if (dptx->edid_info.range.vfreq[1]) {
		pr_info("Range Limit:\n"
			"  V freq:     %u - %u Hz  (Min blank: %u)\n"
			"  H freq:     %u - %u kHz (Min blank: %u)\n"
			"  Pixel Clk:  %u - %u kHz\n",
			dptx->edid_info.range.vfreq[0], dptx->edid_info.range.vfreq[1],
			dptx->edid_info.range.v_blank_min,
			dptx->edid_info.range.hfreq[0], dptx->edid_info.range.hfreq[1],
			dptx->edid_info.range.h_blank_min,
			dptx->edid_info.range.pclk[0] / 1000,
			dptx->edid_info.range.pclk[1] / 1000);
	}
	for (i = 0; i < dptx->edid_info.dtd_cnt; i++) {
		pr_info("Detail Timing Desctipter[%d]:\n"
			"  Pixel Clock: %u.%u MHz\n"
			"  H: Active:%4u Blank:%4u FP:%3u PW:%2u %s Size:%4umm Border:%u\n"
			"  V: Active:%4u Blank:%4u FP:%3u PW:%2u %s Size:%4umm Border:%u\n",
			i, dptx->edid_info.dtd_timing[i].pclk / 1000000,
			(dptx->edid_info.dtd_timing[i].pclk % 1000000) / 1000,
			dptx->edid_info.dtd_timing[i].h_act,
			dptx->edid_info.dtd_timing[i].h_blank,
			dptx->edid_info.dtd_timing[i].h_fp, dptx->edid_info.dtd_timing[i].h_pw,
			dptx->edid_info.dtd_timing[i].ctrl & 0x1 ? "P" : "N",
			dptx->edid_info.dtd_timing[i].h_size,
			// dptx->edid_info.dtd_timing[i].h_border,
			0,
			dptx->edid_info.dtd_timing[i].v_act,
			dptx->edid_info.dtd_timing[i].v_blank,
			dptx->edid_info.dtd_timing[i].v_fp, dptx->edid_info.dtd_timing[i].v_pw,
			dptx->edid_info.dtd_timing[i].ctrl & 0x2 ? "P" : "N",
			dptx->edid_info.dtd_timing[i].v_size,
			// dptx->edid_info.dtd_timing[i].v_border);
			0);
	}
}

static char dptx_edid_base_block_parse(struct dptx_drv_s *dptx, unsigned char *_buf)
{
	struct dptx_edid_range_limit_s *range = &dptx->edid_info.range;
	struct dptx_detail_timing_s *timing;
	u64 temp1;
	unsigned int temp;
	unsigned int checksum = 0;
	unsigned short i, j;
	char *ret;

	if (memcmp(_buf, base_block_header, 8)) {
		DPTXPR(dptx->idx, LOG_E, "%s: invalid EDID header", __func__);
		return -1;
	}

	for (i = 0; i < 128; i++)
		checksum += _buf[i];
	if ((checksum & 0xff)) {
		DPTXPR(dptx->idx, LOG_E, "%s: EDID checksum Wrong", __func__);
		// return -1;
	}

	memset(&dptx->edid_info, 0, sizeof(struct dptx_edid_info_s));

	temp = ((_buf[8] << 8) | _buf[9]);
	for (i = 0; i < 3; i++)
		dptx->edid_info.manufacturer_id[i] = (((temp >> ((2 - i) * 5)) & 0x1f) - 1) + 'A';

	dptx->edid_info.manufacturer_id[3] = '\0';
	temp = ((_buf[11] << 8) | _buf[10]);
	dptx->edid_info.product_id = temp;
	temp = ((_buf[12] << 24) | (_buf[13] << 16) | (_buf[14] << 8) | _buf[15]);
	dptx->edid_info.product_sn = temp;
	dptx->edid_info.week = _buf[16];
	dptx->edid_info.year = _buf[17];
	temp = ((_buf[18] << 8) | _buf[19]);
	dptx->edid_info.version = temp;

	if (!(_buf[20] & 0x80) || ((_buf[20] & 0xf) != 0x5))
		DPTXPR(dptx->idx, LOG_E, "non digital or not DP device [%2x]", _buf[20]);

	dptx->edid_info.cfmt_support = 1 << CFMT_RGB_6bit;
	// [20]BIT[4:6]: bpc: 000=undefined, 001=6, 010=8, 011=10, 100=12, 101=14, 110=16
	// [24]BIT[3:4]: RGB + BIT[0]=YUV444/BIT[1]=YUV422
	if (((_buf[20] >> 4) & 0x7) >= 0x4) {
		dptx->edid_info.cfmt_support |= 1 << CFMT_RGB_12bit;
		if (_buf[24] & 0x08)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr444_12bit;
		if (_buf[24] & 0x10)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr422_12bit;
	}
	if (((_buf[20] >> 4) & 0x7) >= 0x3) {
		dptx->edid_info.cfmt_support |= 1 << CFMT_RGB_10bit;
		if (_buf[24] & 0x08)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr444_10bit;
		if (_buf[24] & 0x10)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr422_10bit;
	}
	if (((_buf[20] >> 4) & 0x7) >= 0x2) {
		dptx->edid_info.cfmt_support |= 1 << CFMT_RGB_8bit;
		if (_buf[24] & 0x08)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr444_8bit;
		if (_buf[24] & 0x10)
			dptx->edid_info.cfmt_support |= 1 << CFMT_YCbCr422_8bit;
	}

	for (i = 0; i < 4; i++) {
		j = 54 + i * 18;
		if (_buf[j] || _buf[j + 1]) {//detail timing
			if (dptx->edid_info.dtd_cnt >= DPTX_DRV_TIMING_MAX - 1)
				continue;

			timing = &dptx->edid_info.dtd_timing[dptx->edid_info.dtd_cnt];

			timing->pclk = ((_buf[j + 1] << 8) | (_buf[j])) * 10000;
			timing->h_act   = (((_buf[j + 4] >> 4) & 0xf) << 8) | _buf[j + 2];
			timing->h_blank = (((_buf[j + 4] >> 0) & 0xf) << 8) | _buf[j + 3];
			timing->v_act   = (((_buf[j + 7] >> 4) & 0xf) << 8) | _buf[j + 5];
			timing->v_blank = (((_buf[j + 7] >> 0) & 0xf) << 8) | _buf[j + 6];
			timing->h_fp   = (((_buf[j + 11] >> 6) & 0x3) << 8) | _buf[j + 8];
			timing->h_pw   = (((_buf[j + 11] >> 4) & 0x3) << 8) | _buf[j + 9];
			timing->v_fp   = (((_buf[j + 11] >> 2) & 0x3) << 4) |
						((_buf[j + 10] >> 4) & 0xf);
			timing->v_pw   = (((_buf[j + 11] >> 0) & 0x3) << 4) |
						((_buf[j + 10] >> 0) & 0xf);
			timing->h_size = (((_buf[j + 14] >> 4) & 0xf) << 8) | _buf[j + 12];
			timing->v_size = (((_buf[j + 14] >> 0) & 0xf) << 8) | _buf[j + 13];
			// timing->h_border = _buf[j + 15];
			// timing->v_border = _buf[j + 16];
			timing->ctrl |= _buf[j + 17] & 0x1 ? CTRL_HSYNC_POS : 0;
			timing->ctrl |= _buf[j + 17] & 0x2 ? CTRL_VSYNC_POS : 0;

			timing->h_period = timing->h_act + timing->h_blank;
			timing->v_period = timing->v_act + timing->v_blank;
			timing->h_bp = timing->h_blank - timing->h_fp - timing->h_pw;
			timing->v_bp = timing->v_blank - timing->v_fp - timing->v_pw;

			temp1 = timing->pclk;
			temp1 *= 1000;
			temp  = timing->h_period * timing->v_period;
			timing->fr1000 = dptx_div_around(temp1, temp);

			dptx->edid_info.dtd_cnt++;
		}

		// some panel didn`t follow spec, keep compatibility
		// if (!(_buf[j] || _buf[j + 1] || _buf[j + 2] || _buf[j + 4])) {
		if (!(_buf[j] || _buf[j + 1] || _buf[j + 2])) {
			switch (_buf[j + 3]) {
			case EDID_BASE_BLOCK_ID_PRODUCT_NAME: //monitor name
				memcpy(dptx->edid_info.name, &_buf[j + 5], 13);
				ret = strstr(dptx->edid_info.name, "\n");
				if (ret)
					dptx->edid_info.name[ret - dptx->edid_info.name] = '\0';
				dptx->edid_info.name[13] = '\0';
				break;
			case EDID_BASE_BLOCK_ID_RANGE_TIMING: //monitor range limits
				range->vfreq[0] = _buf[j + 5] + (_buf[j + 4] & 0x1 ? 255 : 0);
				range->vfreq[1] = _buf[j + 6] + (_buf[j + 4] & 0x2 ? 255 : 0);
				range->hfreq[0] = _buf[j + 7] + (_buf[j + 4] & 0x8 ? 255 : 0);
				range->hfreq[1] = _buf[j + 8] + (_buf[j + 4] & 0x4 ? 255 : 0);
				range->pclk[1]  = _buf[j + 9] * 10 * 1000000; // Hz
				range->pclk[0]  = 0; // Hz
				break;
			case EDID_BASE_BLOCK_ID_ASCII_STR: //ascii string
				memcpy(dptx->edid_info.asc_string, &_buf[j + 5], 13);
				ret = strstr(dptx->edid_info.asc_string, "\n");
				if (ret)
					dptx->edid_info.asc_string
						[ret - dptx->edid_info.asc_string] = '\0';
				dptx->edid_info.asc_string[13] = '\0';
				break;
			case EDID_BASE_BLOCK_ID_SN: //monitor serial num
				memcpy(dptx->edid_info.serial_num, &_buf[j + 5], 13);
				ret = strstr(dptx->edid_info.serial_num, "\n");
				if (ret)
					dptx->edid_info.serial_num
						[ret - dptx->edid_info.serial_num] = '\0';
				dptx->edid_info.serial_num[13] = '\0';
				break;
			default:
				break;
			}
		}
	}
	return _buf[126];
}

static char dptx_edid_DisplayID_parse(struct dptx_drv_s *dptx, unsigned char *_buf)
{
	// struct dptx_edid_range_limit_s *range = &dptx->edid_info.range;
	struct dptx_detail_timing_s *timing;
	unsigned int checksum = 0, temp;
	u64 temp1;
	int i;
	unsigned char tag_ofst, d_pos, dtd_cnt;

	for (i = 0; i < 128; i++)
		checksum += _buf[i];
	if ((checksum & 0xff)) {
		DPTXPR(dptx->idx, LOG_E, "%s: DisplayID checksum Wrong\n", __func__);
		// return -1;
	}

	for (tag_ofst = 5; tag_ofst < 126;) {
		switch (_buf[tag_ofst]) {
		//DisplayID v1.x
		case DisplayID_Type_I_Timing: //03h
			dtd_cnt = _buf[tag_ofst + 2] / 20;

			for (i = 0; i < dtd_cnt; i++) { //Type I Detailed Timing Descriptor
				if (dptx->edid_info.dtd_cnt >= 7)
					break;

				d_pos = tag_ofst + 3 + 20 * i;
				timing = &dptx->edid_info.dtd_timing[dptx->edid_info.dtd_cnt];

				timing->pclk = ((_buf[d_pos + 2] << 16) | (_buf[d_pos + 1] << 8)  |
						(_buf[d_pos + 0] << 0)) * 10000;
				timing->ctrl |= _buf[d_pos + 3] & 0x80 ? CTRL_PREFERRED_TIMING : 0;
				timing->h_act   = 1 +  (_buf[d_pos + 4] | (_buf[d_pos + 5] << 8));
				timing->h_blank = 1 +  (_buf[d_pos + 6] | (_buf[d_pos + 7] << 8));
				timing->h_fp    = 1 +  (_buf[d_pos + 8] |
								((_buf[d_pos + 9] & 0x7f) << 8));
				timing->h_pw    = 1 + (_buf[d_pos + 10] | (_buf[d_pos + 11] << 8));
				timing->v_act   = 1 + (_buf[d_pos + 12] | (_buf[d_pos + 13] << 8));
				timing->v_blank = 1 + (_buf[d_pos + 14] | (_buf[d_pos + 15] << 8));
				timing->v_fp    = 1 + (_buf[d_pos + 16] |
								((_buf[d_pos + 17] & 0x7f) << 8));
				timing->v_pw    = 1 + (_buf[d_pos + 18] |
								((_buf[d_pos + 19] & 0x7f) << 8));
				timing->ctrl |=  (_buf[d_pos + 9] & 0x80) ? CTRL_HSYNC_POS : 0;
				timing->ctrl |= (_buf[d_pos + 17] & 0x80) ? CTRL_VSYNC_POS : 0;

				timing->h_size = timing->h_act;
				timing->v_size = timing->v_act;

				timing->h_period = timing->h_act + timing->h_blank;
				timing->v_period = timing->v_act + timing->v_blank;
				timing->h_bp = timing->h_blank - timing->h_fp - timing->h_pw;
				timing->v_bp = timing->v_blank - timing->v_fp - timing->v_pw;

				temp1 = timing->pclk;
				temp1 *= 1000;
				temp  = timing->h_period * timing->v_period;
				timing->fr1000 = dptx_div_around(temp1, temp);

				dptx->edid_info.dtd_cnt++;
			}
			break;
		case DisplayID_Video_Timing_Range: //09h
			//range->pclk[0] = ((_buf[tag_ofst + 5] << 16) |
			//		  (_buf[tag_ofst + 4] << 8)  |

			break;
		case DisplayID_Type_II_Timing:
		case DisplayID_Type_IV_Timing:
		case DisplayID_Type_III_Timing:
		case DisplayID_Display_Parameters_1:
		case DisplayID_Color_Characteristics:
		case DisplayID_VESA_Timing_Standard:
		case DisplayID_CEA_Timing_Standard:
		case DisplayID_Product_SerialNumber:
		case DisplayID_General_Purpose_ASCII_String:
		case DisplayID_Display_Device_Data:
		case DisplayID_Interface_Power_Sequencing:
		case DisplayID_Transfer_Characteristics:
		case DisplayID_Display_Interface_Data:
		case DisplayID_Stereo_Display_Interface_1:
		case DisplayID_Type_V_Timing:
		case DisplayID_Type_VI_Timing:
		case DisplayID_Vendor_Specific_1:
			break;
		//DisplayID v2.x
		case DisplayID_Type_VII_Detailed_Timing:
		case DisplayID_Display_Interface_Features:
			break;
		case DisplayID_Product_Identification:
		case DisplayID_Display_Parameters_2:
		case DisplayID_Type_VIII_Enumerated_Timing_Code:
		case DisplayID_Type_IX_Formula_Based_Timing:
		case DisplayID_Dynamic_Video_Timing_Range_Limits:
		case DisplayID_Stereo_Display_Interface_2:
		case DisplayID_Tiled_Display_Topology:
		case DisplayID_ContainerID:
		case DisplayID_Vendor_Specific_2:
		case DisplayID_CTA_DisplayID:
			break;
		default:
			tag_ofst = 126;
			break;
		}
		tag_ofst += 2 + _buf[tag_ofst + 2];
	}

	return 0;
}

static char dptx_edid_CEA_861_parse(struct dptx_drv_s *dptx, unsigned char *_buf)
{
	struct dptx_detail_timing_s *timing;
	u32 temp, checksum = 0;
	u64 temp1;
	u8 i, j;
	unsigned char dtd_ofst, tag, count;//, native_dtd_cnt;

	for (i = 0; i < 128; i++)
		checksum += _buf[i];
	if ((checksum & 0xff)) {
		DPTXPR(dptx->idx, LOG_E, "%s: EDID checksum Wrong\n", __func__);
		// return -1;
	}

	dtd_ofst = _buf[2];

	for (j = 4; j < dtd_ofst;) { // from HDMI code
		tag = _buf[j] >> 5;
		count = _buf[j] & 0x1f;
		switch (tag) {
		case EDID_CEA_861_TAG_VIDEO:
			// TODO
			j += count + 1;
			break;
		case EDID_CEA_861_TAG_AUDIO:
		case EDID_CEA_861_TAG_VENDOR_SPEC:
		case EDID_CEA_861_TAG_SPEAKER_ALLOCATION:
		case EDID_CEA_861_TAG_VESA_DSIP_TRANS:
		default:
			j += count + 1;
			break;
		}
	}

	for (i = 0; i < 8; i++) {
		j = dtd_ofst + 18 * i;

		if (j > 128 || dptx->edid_info.dtd_cnt >= 7 || !_buf[j])
			break;

		timing = &dptx->edid_info.dtd_timing[dptx->edid_info.dtd_cnt];

		timing->pclk    =                ((_buf[j + 1] << 8) | _buf[j]) * 10000;
		timing->h_act   =  (((_buf[j + 4] >> 4) & 0xf) << 8) | _buf[j + 2];
		timing->h_blank =  (((_buf[j + 4] >> 0) & 0xf) << 8) | _buf[j + 3];
		timing->v_act   =  (((_buf[j + 7] >> 4) & 0xf) << 8) | _buf[j + 5];
		timing->v_blank =  (((_buf[j + 7] >> 0) & 0xf) << 8) | _buf[j + 6];
		timing->h_fp    = (((_buf[j + 11] >> 6) & 0x3) << 8) | _buf[j + 8];
		timing->h_pw    = (((_buf[j + 11] >> 4) & 0x3) << 8) | _buf[j + 9];
		timing->v_fp    = (((_buf[j + 11] >> 2) & 0x3) << 4) | ((_buf[j + 10] >> 4) & 0xf);
		timing->v_pw    = (((_buf[j + 11] >> 0) & 0x3) << 4) | ((_buf[j + 10] >> 0) & 0xf);
		timing->h_size  = (((_buf[j + 14] >> 4) & 0xf) << 8) | _buf[j + 12];
		timing->v_size  = (((_buf[j + 14] >> 0) & 0xf) << 8) | _buf[j + 13];
		// timing->h_border = _buf[j + 15];
		// timing->v_border = _buf[j + 16];
		timing->ctrl = (_buf[j + 17] & 0x1 ? CTRL_HSYNC_POS : 0) |
				(_buf[j + 17] & 0x2 ? CTRL_VSYNC_POS : 0);

		timing->h_period = timing->h_act + timing->h_blank;
		timing->v_period = timing->v_act + timing->v_blank;
		timing->h_bp = timing->h_blank - timing->h_fp - timing->h_pw;
		timing->v_bp = timing->v_blank - timing->v_fp - timing->v_pw;

		temp1 = 1000ULL * timing->pclk;
		temp  = timing->h_blank * timing->v_blank;
		timing->fr1000 = dptx_div_around(temp1, temp);

		dptx->edid_info.dtd_cnt++;
	}
	return 0;
}

static void dptx_edid_ext_block_parse(struct dptx_drv_s *dptx, unsigned char *_buf)
{
	if (_buf[0] == EXTENDED_HEADER_DisplayID)
		dptx_edid_DisplayID_parse(dptx, _buf);
	else if (_buf[0] == EXTENDED_HEADER_EDID_CEA_861)
		dptx_edid_CEA_861_parse(dptx, _buf);
	else if (_buf[0] == 0x0)
		return;

	DPTXPR(dptx->idx, LOG_E, "%s: not a DisplayID or EDID CEA-861", __func__);
}

static char dptx_edid_parse(struct dptx_drv_s *dptx, unsigned char *_buf)
{
	int ret;
	unsigned char i;//, ext_block;

	memset(&dptx->edid_info, 0, sizeof(struct dptx_edid_info_s));

	ret = dptx_edid_base_block_parse(dptx, _buf);
	if (ret < 0)
		return -1;

	for (i = 1; i < 4; i++)
		dptx_edid_ext_block_parse(dptx, _buf + (128 * i));
	return 0;
}

static int dptx_read_edid(struct dptx_drv_s *dptx, unsigned char *edid_buf)
{
	int i, ret;
	unsigned char aux_data;
	unsigned char read_count = 128 / DPTX_AUX_I2C_READ_BYTES;
	unsigned char ext_block_cnt = 0;

	if (!edid_buf) {
		DPTXPR(dptx->idx, LOG_E, "%s: edid buf is null", __func__);
		return -1;
	}

	aux_data = 0x00;
	ret = dptx_aux_i2c_op(dptx, DPTX_AUX_CMD_I2C_WRITE_MOT, 0x50, 1, &aux_data);
	for (i = 0; i < read_count; i++) {
		ret = dptx_aux_i2c_op(dptx,
			(i == (read_count - 1)) ?
				DPTX_AUX_CMD_I2C_READ : DPTX_AUX_CMD_I2C_READ_MOT,
			0x50, 16, &edid_buf[i * 16]);
		if (ret)
			return ret;
	}
	if (!ret)
		ext_block_cnt = edid_buf[126];

	if (ext_block_cnt > 8)
		return -1;

	if (ext_block_cnt >= 1) {
		aux_data = 0x80; // read from 0x80
		ret = dptx_aux_i2c_op(dptx, DPTX_AUX_CMD_I2C_WRITE_MOT, 0x50, 1, &aux_data);
		for (i = 0; i < read_count; i++) {
			ret = dptx_aux_i2c_op(dptx,
				(i == (read_count - 1)) ?
					DPTX_AUX_CMD_I2C_READ : DPTX_AUX_CMD_I2C_READ_MOT,
				0x50, 16, &edid_buf[128 + i * 16]);
			if (ret)
				return ret;
		}
	}

	if (ext_block_cnt >= 2) {
		aux_data = 0x01; //switch, do as Intel do.
		ret = dptx_aux_i2c_op(dptx, DPTX_AUX_CMD_I2C_WRITE_MOT, 0x30, 1, &aux_data);

		aux_data = 0x00; // read from 0x80
		ret = dptx_aux_i2c_op(dptx, DPTX_AUX_CMD_I2C_WRITE_MOT, 0x50, 1, &aux_data);
		for (i = 0; i < read_count; i++) {
			ret = dptx_aux_i2c_op(dptx,
				(i == (read_count - 1)) ?
					DPTX_AUX_CMD_I2C_READ : DPTX_AUX_CMD_I2C_READ_MOT,
				0x50, 16, &edid_buf[256 + i * 16]);
			if (ret)
				return ret;
		}
	}

	if (ext_block_cnt >= 3) {
		aux_data = 0x80; // read from 0x80
		ret = dptx_aux_i2c_op(dptx, DPTX_AUX_CMD_I2C_WRITE_MOT, 0x50, 1, &aux_data);
		for (i = 0; i < read_count; i++) {
			ret = dptx_aux_i2c_op(dptx,
				(i == (read_count - 1)) ?
					DPTX_AUX_CMD_I2C_READ : DPTX_AUX_CMD_I2C_READ_MOT,
				0x50, 16, &edid_buf[384 + i * 16]);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void dptx_edid_crc_cal(struct dptx_drv_s *dptx)
{
	uint8_t i, crc = 0;

	for (i = 0; i < 14; i++)
		crc += dptx->edid_info.name[i];

	crc += dptx->edid_info.range.vfreq[0];
	crc += dptx->edid_info.range.vfreq[1];

	crc += dptx->edid_info.dtd_cnt;

	for (i = 0; i < dptx->edid_info.dtd_cnt; i++) {
		crc += dptx->edid_info.dtd_timing[i].h_period;
		crc += dptx->edid_info.dtd_timing[i].h_act;
		crc += dptx->edid_info.dtd_timing[i].v_period;
		crc += dptx->edid_info.dtd_timing[i].v_act;
		crc += dptx->edid_info.dtd_timing[i].pclk;
	}
	DPTXPR(dptx->idx, LOG_V, "sink EDID crc: 0x%2x", crc);
	dptx->edid_info.edid_crc = crc;
}

// read & parse EDID
int __dptx_EDID_probe(struct dptx_drv_s *dptx)
{
	unsigned char *edid_buf;
	int ret;
	unsigned char retry_cnt = 0;

	if (!dptx)
		return -1;

	edid_buf = malloc(512 * sizeof(unsigned char));
	if (!edid_buf) {
		DPTXPR(dptx->idx, LOG_E, "EDID buffer malloc failed");
		return -1;
	}

dptx_EDID_probe_retry:
	memset(edid_buf, 0, 512 * sizeof(unsigned char));

	ret = dptx_read_edid(dptx, edid_buf);
	if (ret) {
		if (retry_cnt++ > DPTX_EDID_READ_RETRY_MAX) {
			DPTXPR(dptx->idx, LOG_E, "%s: read fail @%d", __func__, retry_cnt);
			goto dptx_EDID_probe_err;
		}
		goto dptx_EDID_probe_retry;
	}

	ret = dptx_edid_parse(dptx, edid_buf);
	if (ret < 0) {
		if (retry_cnt++ > DPTX_EDID_READ_RETRY_MAX) {
			DPTXPR(dptx->idx, LOG_E, "%s: parse fail @%d", __func__, retry_cnt);
			goto dptx_EDID_probe_err;
		}
		goto dptx_EDID_probe_retry;
	}

	if (dptx_print_level >= LOG_V) {
		dptx_edid_print_raw(edid_buf);
		dptx_edid_print_parsed(dptx);
	}

	dptx_edid_crc_cal(dptx);
	// dptx_edid_store(dptx, edid_buf);

	DPTXPR(dptx->idx, LOG_I, "%s ok", __func__);
	free(edid_buf);
	return 0;

dptx_EDID_probe_err:
	DPTXPR(dptx->idx, LOG_E, "%s failed", __func__);
	free(edid_buf);
	return -1;
}
