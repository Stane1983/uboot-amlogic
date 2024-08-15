// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "hdmitx_check_valid.h"

/* only check vic in edid */
bool hdmitx_edid_validate_mode(struct rx_cap *rxcap, u32 vic)
{
	int i = 0;
	bool edid_matched = false;

	if (!rxcap)
		return false;

	if (vic < HDMITX_VESA_OFFSET) {
		/* check cea cap */
		for (i = 0 ; i < rxcap->VIC_count; i++) {
			if (rxcap->VIC[i] == vic) {
				edid_matched = true;
				break;
			}
		}
	} else {
		enum hdmi_vic *vesa_t = &rxcap->vesa_timing[0];
		/* check vesa mode. */
		for (i = 0; i < VESA_MAX_TIMING && vesa_t[i]; i++) {
			if (vic == vesa_t[i]) {
				edid_matched = true;
				break;
			}
		}
	}

	return edid_matched;
}

/* check if vic is supported by SOC hdmitx */
int hdmitx_common_validate_vic(struct hdmitx_common *tx_comm, u32 vic)
{
	const struct hdmi_timing *timing = hdmitx_mode_vic_to_hdmi_timing(vic);

	if (!timing)
		return -EINVAL;

	/* soc level filter */
	/* filter 1080p max size. */
	if (tx_comm->res_1080p) {
		/*
		 * if the vic equals to HDMI_0_UNKNOWN or VESA,
		 * then create it as over limited
		 */
		if (vic == HDMI_0_UNKNOWN || vic >= HDMITX_VESA_OFFSET)
			return -ERANGE;
		/* check the resolution is over 1920x1080 or not */
		if (timing->h_active > 1920 || timing->v_active > 1080)
			return -ERANGE;

		/* check the fresh rate is over 60hz or not */
		if (timing->v_freq > 60000)
			return -ERANGE;

		/* test current vic is over 150MHz or not */
		if (timing->pixel_freq > 150000)
			return -ERANGE;
	}

	/* efuse ctrl all 4k mode */
	if (tx_comm->efuse_dis_output_4k)
		if (timing->v_active >= 2160)
			return -ERANGE;

	/* efuse ctrl 4k50, 4k60 */
	if (tx_comm->efuse_dis_hdmi_4k60)
		if (timing->v_active >= 2160 && timing->v_freq >= 5000)
			return -ERANGE;

	/* filter max refreshrate. */
	if (timing->v_freq > (tx_comm->max_refreshrate * 1000)) {
		/* HDMITX_INFO("validate refreshrate (%s)-(%d) fail\n", */
		/* timing->name, timing->v_freq); */
		return -EACCES;
	}

	/* ip level filter */
	if (hdmitx_hw_validate_mode(tx_comm->tx_hw, vic, tx_comm->max_refreshrate) != 0)
		return -EPERM;

	return 0;
}

#ifdef CONFIG_AMLOGIC_DSC
static bool hdmitx_check_dsc_support(struct tx_cap *hdmi_tx_cap,
				     struct rx_cap *rxcap, struct hdmi_format_para *para)
{
	enum dsc_encode_mode dsc_mode = DSC_ENCODE_MAX;
	u8 dsc_slice_num = 0;
	enum frl_rate_enum dsc_frl_rate = FRL_NONE;
	u32 bytes_target = 0;
	u8 dsc_policy;

	if (!hdmi_tx_cap || !rxcap || !para)
		return false;

	/* step1: check if DSC mode is supported by SOC driver & policy */
	if (!hdmi_tx_cap->dsc_capable) {
		HDMITX_DEBUG_EDID("tx not capable of dsc\n");
		return false;
	}
	dsc_policy = hdmi_tx_cap->dsc_policy;

	dsc_mode = dsc_enc_confirm_mode(para->timing.h_active,
					para->timing.v_active,
					para->timing.v_freq,
					para->cs);

	if (dsc_mode == DSC_ENCODE_MAX) {
		HDMITX_DEBUG_EDID("dsc mode not supported!\n");
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
		HDMITX_DEBUG_EDID("RX not support DSC\n");
		return false;
	}
	/* check dsc color depth cap */
	if (para->cd == COLORDEPTH_30B &&
		!rxcap->dsc_10bpc) {
		HDMITX_DEBUG_EDID("RX not support 10bpc DSC\n");
		return false;
	} else if (para->cd == COLORDEPTH_36B &&
		!rxcap->dsc_12bpc) {
		HDMITX_DEBUG_EDID("RX not support 12bpc DSC\n");
		return false;
	}
	/* check dsc color space cap */
	if (para->cs == HDMI_COLORSPACE_YUV420 &&
		!rxcap->dsc_native_420) {
		HDMITX_DEBUG_EDID("RX not support Y420 DSC\n");
		return false;
	}
	dsc_slice_num = dsc_get_slice_num(dsc_mode);
	/* slice num exceed rx cap */
	if (dsc_slice_num == 0 ||
		dsc_slice_num > dsc_max_slices_num[rxcap->dsc_max_slices]) {
		HDMITX_DEBUG_EDID("current slice num %d exceed rx cap %d\n",
			dsc_slice_num, dsc_max_slices_num[rxcap->dsc_max_slices]);
		return false;
	}
	/*
	 * note: pixel clock per slice not checked, assume
	 * it's always within rx cap
	 */
	/* check dsc frl rate within rx cap */
	dsc_frl_rate = get_dsc_frl_rate(dsc_mode);
	if (dsc_frl_rate == FRL_RATE_MAX ||
		dsc_frl_rate > rxcap->dsc_max_frl_rate ||
		dsc_frl_rate > rxcap->max_frl_rate) {
		HDMITX_DEBUG_EDID("current dsc frl rate %d exceed rx cap %d-%d\n",
			dsc_frl_rate, rxcap->dsc_max_frl_rate, rxcap->max_frl_rate);
		return false;
	}
	/*
	 * 2.1 spec table 6-56, if Bytes Target is greater than
	 * the value indicated by DSC_TotalChunkKBytes (see Sections
	 * 7.7.1 and 7.7.4.2), then the configuration is not
	 * supported with Compressed Video Transport.
	 */
	bytes_target = dsc_get_bytes_target_by_mode(dsc_mode);
	if (bytes_target > (rxcap->dsc_total_chunk_bytes + 1) * 1024) {
		HDMITX_DEBUG_EDID("bytes_target %d exceed DSC_TotalChunkKBytes %d\n",
			bytes_target, (rxcap->dsc_total_chunk_bytes + 1) * 1024);
		return false;
	}
	return true;
}
#endif

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
EXPORT_SYMBOL(hdmitx_mode_validate_y420_vic);

/* return true means support; false means not support */
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

/*
 * For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
static int hdmitx_edid_validate_format_para(struct tx_cap *hdmi_tx_cap,
					    struct rx_cap *prxcap, struct hdmi_format_para *para)
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
	u8 dsc_policy;

	if (!hdmi_tx_cap || !prxcap || !para)
		return -EPERM;

	dsc_policy = hdmi_tx_cap->dsc_policy;
	dv = &prxcap->dv_info;
	/* step1: check if mode + cs/cd is supported by TX */
	switch (para->timing.vic) {
	/*
	 * Note: below check for formats which should use FRL
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
			HDMITX_DEBUG_EDID("cs:%d, cd:%d not support by DVI sink\n",
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

	/* more > 4k60 must use frl mode */
	if (para->timing.h_active > 4096 || para->timing.v_active > 2160 ||
	    para->timing.v_freq == 48000 || calc_tmds_clk > 594 ||
	    para->timing.pixel_freq / 1000 > 600)
		must_frl_flag = true;

	if (hdmi_tx_cap->tx_max_frl_rate == FRL_NONE) {
		/* output format need FRL while SOC not support FRL */
		if (must_frl_flag) {
			HDMITX_DEBUG_EDID("output format need FRL, while tx not support\n");
			return -EPERM;
		}
		/* tmds clk of the output format exceed TX/RX cap */
		if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
			HDMITX_DEBUG_EDID("output tmds clk:%d exceed tx cap: %d\n",
				calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
			return -EPERM;
		}
		if (calc_tmds_clk > rx_max_tmds_clk) {
			HDMITX_DEBUG_EDID("output tmds clk:%d exceed rx cap: %d\n",
					  calc_tmds_clk, rx_max_tmds_clk);
			return -EPERM;
		}
	} else {
#ifdef CONFIG_AMLOGIC_DSC
		if (dsc_policy == 1) {
			/* force enable policy */
			if (hdmitx_check_dsc_support(hdmi_tx_cap, prxcap, para))
				return 0;
		} else if (dsc_policy == 2) {
			/* for debug test */
			return 0;
		}
#endif
		if (!must_frl_flag) {
			if (calc_tmds_clk > hdmi_tx_cap->tx_max_tmds_clk) {
				HDMITX_DEBUG_EDID("output tmds clk:%d exceed tx cap: %d\n",
					calc_tmds_clk, hdmi_tx_cap->tx_max_tmds_clk);
				return -EPERM;
			}
			if (calc_tmds_clk > rx_max_tmds_clk) {
				HDMITX_DEBUG_EDID("output tmds clk:%d exceed rx cap: %d\n",
						  calc_tmds_clk, rx_max_tmds_clk);
				return -EPERM;
			}
		} else {
			/* try to check if able to run under FRL mode */

			/*
			 * output format need FRL while RX not support FRL
			 * no need below check, it will be checked with rx_frl_bandwidth_cap
			 */
			if (prxcap->max_frl_rate == FRL_NONE) {
				HDMITX_DEBUG_EDID("output format need FRL, while rx not support\n");
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
					HDMITX_DEBUG_EDID("frl bandwitch:%d exceed tx_cap:%d\n",
							  tx_frl_bandwidth, tx_frl_bandwidth_cap);
					return -EPERM;
				}
				if (tx_frl_bandwidth > rx_frl_bandwidth_cap) {
					HDMITX_DEBUG_EDID("frl bandwitch:%d exceed rx_cap:%d\n",
							  tx_frl_bandwidth, rx_frl_bandwidth_cap);
					return -EPERM;
				}
			} else {
				if (tx_frl_bandwidth <= tx_frl_bandwidth_cap &&
				    tx_frl_bandwidth <= rx_frl_bandwidth_cap)
					; /* non-dsc bandwidth is within cap, continue check */
#ifdef CONFIG_AMLOGIC_DSC
				/* forcely filter out dsc mode output */
				else if (dsc_policy == 3)
					return -EPERM;
				else if (!hdmitx_check_dsc_support(hdmi_tx_cap, prxcap, para))
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

/*
 * check fmt para is supported or not by hdmitx/edid.
 * note that vic is not checked if supported by hdmitx/edid
 * return 0 means valid, other value means invalid
 */
int hdmitx_common_validate_format_para(struct hdmitx_common *tx_comm,
				       struct hdmi_format_para *para)
{
	int ret = 0;

	if (!tx_comm || !para)
		return -EPERM;

	if (para->vic == HDMI_0_UNKNOWN)
		return -EPERM;

	/* check if the format param is within capability of both TX/RX */
	ret = hdmitx_edid_validate_format_para(&tx_comm->tx_hw->hdmi_tx_cap,
					       &tx_comm->rxcap, para);

	return ret;
}
EXPORT_SYMBOL(hdmitx_common_validate_format_para);

/*
 * check VIC supported or not with basic cs/cd
 * compare with hdmitx_common_validate_mode_locked()
 * or valid_mode_store(), it doesn't check if vic
 * is supported by hdmitx/rx or not
 * return 0 means valid, other value means invalid
 */
int hdmitx_common_check_valid_para_of_vic(struct hdmitx_common *tx_comm, enum hdmi_vic vic)
{
	struct hdmi_format_para tst_para;
	/* cd8, cd10 or cd12 */
	enum hdmi_color_depth cd;
	/* 0/1/2/3: rgb/422/444/420 */
	enum hdmi_colorspace cs;
	/* default to full */
	const enum hdmi_quantization_range cr = HDMI_QUANTIZATION_RANGE_FULL;
	int i = 0;

	if (!tx_comm || vic == HDMI_0_UNKNOWN)
		return -EINVAL;

	if (hdmitx_mode_validate_y420_vic(vic)) {
		cs = HDMI_COLORSPACE_YUV420;
		cd = COLORDEPTH_24B;
		if (hdmitx_common_build_format_para(tx_comm,
						    &tst_para, vic, false, cs, cd, cr) == 0 &&
		    hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	struct {
		enum hdmi_colorspace cs;
		enum hdmi_color_depth cd;
	} test_color_attr[] = {
		{HDMI_COLORSPACE_RGB, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV444, COLORDEPTH_24B},
		{HDMI_COLORSPACE_YUV422, COLORDEPTH_36B},
	};

	for (i = 0; i < ARRAY_SIZE(test_color_attr); i++) {
		cs = test_color_attr[i].cs;
		cd = test_color_attr[i].cd;
		hdmitx_format_para_reset(&tst_para);
		if (hdmitx_common_build_format_para(tx_comm,
						    &tst_para, vic, false, cs, cd, cr) == 0 &&
		    hdmitx_common_validate_format_para(tx_comm, &tst_para) == 0) {
			return 0;
		}
	}

	return -EPERM;
}
EXPORT_SYMBOL(hdmitx_common_check_valid_para_of_vic);
