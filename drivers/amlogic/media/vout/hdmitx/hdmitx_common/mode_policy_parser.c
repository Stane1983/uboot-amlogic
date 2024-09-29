/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "mode_util.h"
#include "mode_policy.h"
#include "mode_policy_parser.h"

static bool test_mode = false;

#ifndef __UBOOT__
/*
 * linux and android code
 */
#include "mode_private.h"

bool support_DV_RGB_444_8BIT(struct meson_hdr_info *info) {
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(info->dv_deepcolor, "DV_RGB_444_8BIT"))
        return true;
    return false;
}

bool support_LL_YCbCr_422_12BIT(struct meson_hdr_info *info) {
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(info->dv_deepcolor, "LL_YCbCr_422_12BIT"))
        return true;
    return false;
}

bool support_LL_RGB_444_10BIT(struct meson_hdr_info *info) {
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(info->dv_deepcolor, "LL_RGB_444_10BIT"))
        return true;
    return false;
}

bool support_LL_RGB_444_12BIT(struct meson_hdr_info *info) {
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(info->dv_deepcolor, "LL_RGB_444_12BIT"))
        return true;
    return false;
}

bool support_DV_VSVDB_PARITY(struct meson_hdr_info *info) {
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(info->dv_cap, DV_VSVDB_PARITY) != NULL)
        return true;
    return false;
}

/* TODO: need refactor */
/* check if the edid support current hdmi mode */
bool is_support_hdmimode(struct meson_policy_in *input, const char* mode) {
    /*
     * check input param
     */
    if (!input || !mode) {
        SYS_LOGE("%s input or mode is null\n", __FUNCTION__);
        return false;
    }

    /*
     * check current resolution support or not base connector mode list
     */
    meson_mode_info_t *modes_ptr = input->con_info.modes;
    for (int i = 0; i < input->con_info.modes_size; i ++) {
        meson_mode_info_t *it = &modes_ptr[i];
        if (!strcmp(it->name, mode)) {
            SYS_LOGI("%s mode: %s\n", __FUNCTION__, mode);
            return true;
        }
    }

    SYS_LOGI("%s mode: %s not support\n", __FUNCTION__, mode);

    return false;
}

bool is_support_color_format(struct meson_policy_in *input, const char* color_format) {
    bool ret = false;
    /*
     * check input param
     */
    if (!input || !color_format) {
        SYS_LOGE("%s input or color_format is null\n", __FUNCTION__);
        return ret;
    }

    if (strstr(input->con_info.dc_cap, color_format) != NULL) {
        ret = true;
    }

    return ret;
}

bool is_hdmi_edid_parserok(struct meson_policy_in *input) {
    bool ret = false;
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGE("%s input is null\n", __FUNCTION__);
        return ret;
    }

    if (!strcmp(input->con_info.edid_parsing, "ok")) {
        ret = true;
    }

    return ret;
}

bool is_hdmi_dc_cap_ok(struct meson_policy_in *input) {
    bool ret = false;
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGE("%s input is null\n", __FUNCTION__);
        return ret;
    }

    if (strlen(input->con_info.dc_cap)) {
        ret = true;
    }

    return ret;
}

/*
 * we need find the brr mode and check it with cs/cd.
 */
bool find_brr_mode(const char *mode, struct meson_policy_in *input, char* outputmode) {
    /*
     * check input param
     */
    if (!mode || !input || !outputmode) {
        SYS_LOGE("%s input or mode or outputmode is null\n", __FUNCTION__);
        return false;
    }

    strcpy(outputmode, mode);

    meson_mode_info_t *config_ptr = NULL;
    meson_mode_info_t *modes_ptr = input->con_info.modes;
    /*
     * first loop find the meson_mode_info
     */
    for (int i = 0; i < input->con_info.modes_size; i ++) {
        meson_mode_info_t *it = &modes_ptr[i];
        if (!strcmp(it->name, mode)) {
            config_ptr = it;
            break;
        }
    }

    if (!config_ptr) {
        SYS_LOGI("%s mode: %s not support\n", __FUNCTION__, mode);
        return false;
    }

    /*
     * second loop find the brr mode
     */
    for (int i = 0; i < input->con_info.modes_size; i ++) {
        meson_mode_info_t *it = &modes_ptr[i];

        if (it->group_id == config_ptr->group_id) {
            if (it->refresh_rate > config_ptr->refresh_rate) {
                config_ptr = it;
            }
        }
    }

    strcpy(outputmode, config_ptr->name);

    return true;
}

/*
 * check resolution and color format support or not
 */
bool mode_support_check(const char *mode, const char * color, struct meson_policy_in *input) {
    char outputmode[MESON_MODE_LEN] = {0};
    bool validmode = false;

    /*
     * check input param
     */
    if (!mode || !input || !color) {
        SYS_LOGE("%s input or mode or color is null\n", __FUNCTION__);
        return false;
    }

    /*
     * find brr mode base mode
     */
    find_brr_mode(mode, input, outputmode);
    strlcat(outputmode, color, sizeof(outputmode));

    /*
     * check mode and color format support or not
     */
    validmode = meson_write_valid_mode_sys(DISPLAY_HDMI_VALID_MODE, outputmode);

    if (test_mode)
        return true;
    else
        return validmode;
}

#else
/*
 * uboot code
 */
#include "hdmitx_policy_setting.h"

bool support_DV_RGB_444_8BIT(struct meson_hdr_info *info) {
    if (!info)
        return false;

    if (info->support_DV_RGB_444_8BIT)
        return true;
    return false;
}


bool support_LL_YCbCr_422_12BIT(struct meson_hdr_info *info) {
    if (!info)
        return false;

    if (info->support_LL_YCbCr_422_12BIT)
        return true;
    return false;
}

bool support_LL_RGB_444_10BIT(struct meson_hdr_info *info) {
    if (!info)
        return false;

    if (info->support_LL_RGB_444_10BIT)
        return true;
    return false;
}

bool support_LL_RGB_444_12BIT(struct meson_hdr_info *info) {
    if (!info)
        return false;

    if (info->support_LL_RGB_444_12BIT)
        return true;
    return false;
}

bool support_DV_VSVDB_PARITY(struct meson_hdr_info *info) {
    if (!info)
        return false;

    if (info->amdv_parity)
        return true;
    return false;
}

/* check if the edid support current hdmi mode */
bool is_support_hdmimode(struct meson_policy_in *input, const char *mode)
{
#ifdef CONFIG_AML_HDMITX20
    struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
    struct hdmitx_dev *hdev = get_hdmitx21_device();
#endif

    if (!input || !mode)
        return false;

    if (!hdmi_sink_disp_mode_sup(hdev, mode))
        return false;
    return true;
}


bool is_support_color_format(struct meson_policy_in *input, const char* color_format) {
    bool ret = false;
    /*
     * check input param
     */
    if (!input || !color_format) {
        SYS_LOGE("%s input or color_format is null\n", __FUNCTION__);
        return ret;
    }

    ret = true;
    return ret;
}

bool is_hdmi_edid_parserok(struct meson_policy_in *input) {
#ifdef CONFIG_AML_HDMITX20
    struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
    struct hdmitx_dev *hdev = get_hdmitx21_device();
#endif
    bool ret = false;
    int edid_check = 0;
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGE("%s input is null\n", __FUNCTION__);
        return ret;
    }

    if (hdmitx_edid_check_data_valid(edid_check, hdev->rawedid)) {
        ret = true;
    }

    return ret;
}

bool is_hdmi_dc_cap_ok(struct meson_policy_in *input) {
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGE("%s input is null\n", __FUNCTION__);
        return false;
    }

    return true;
}

/*
 * uboot cannot be executed temporarily, it is just to synchronize the code
 * compilation pass, and then make modifications when it is used later.
 */
bool find_brr_mode(const char *mode, struct meson_policy_in *input, char* outputmode) {
#ifdef CONFIG_AML_HDMITX21
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	bool rx_qms_cap = 0;
	bool env_qms_en = 0;
	bool progressive_mode = 1;
	const char *i_modes[3] = {
		"480i", "576i", "1080i",
	};
	const struct hdmi_timing *tfr_timing = NULL;
	const struct hdmi_timing *brr_timing = NULL;
	const char *brr_mode = NULL;
	int i;
	enum hdmi_vic qms_brr_vic = HDMI_UNKNOWN;
	const struct hdmi_timing *hdmitx21_gettiming_from_name(const char *name);
#endif

	if (!mode || !input || !outputmode) {
		SYS_LOGE("%s input or mode or outputmode is null\n", __func__);
		return false;
	}
/* QMS is not applied for hdmi20 devices */
#ifdef CONFIG_AML_HDMITX20
	strcpy(outputmode, mode);
	return false;
#endif
#ifdef CONFIG_AML_HDMITX21
	rx_qms_cap = hdev->RXCap.qms;
	if (env_get("qms_en") && (env_get_ulong("qms_en", 10, 0) == 1))
		env_qms_en = 1;

	/* if current mode is interlaced mode, then skip QMS */
	for (i = 0; i < 3; i++) {
		if (strstr(mode, i_modes[i])) {
			progressive_mode = 0;
			break;
		}
	}

	if (!(env_qms_en && progressive_mode && rx_qms_cap)) {
		strcpy(outputmode, mode);
		SYS_LOGE("hdmitx: qms: env %d mode %d rx_qms %d\n",
			env_qms_en, progressive_mode, rx_qms_cap);
		return false;
	}
	tfr_timing = hdmitx21_gettiming_from_name(mode);
	if (!tfr_timing) {
		strcpy(outputmode, mode);
		SYS_LOGE("hdmitx: qms: not find timing of %s\n", mode);
		return false;
	}
	qms_brr_vic = hdmitx_find_brr_vic(tfr_timing->vic);
	brr_timing = hdmitx21_gettiming_from_vic(qms_brr_vic);
	brr_mode = brr_timing->sname ? brr_timing->sname : brr_timing->name;
	if (brr_timing->v_freq < tfr_timing->v_freq) {
		strcpy(outputmode, mode);
		SYS_LOGE("hdmitx: qms: tfr %s larger than brr %s\n",
			tfr_timing->sname ? tfr_timing->sname : tfr_timing->name, brr_mode);
		return false;
	}
	SYS_LOGE("hdmitx: qms: the brr mode of %s is %s\n", mode, brr_mode);
	strcpy(outputmode, brr_mode);
	return true;
#endif
	return false;
}

/*
 * check resolution and color format support or not
 */
bool mode_support_check(const char *mode, const char * color, struct meson_policy_in *input) {
struct hdmi_format_para *para = NULL;
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	char brr_mode[32] = {0};
#endif

	if (!mode || !color || !input)
		return false;

#ifdef CONFIG_AML_HDMITX20
	para = hdmi_tst_fmt_name(mode, color);
	return hdmitx_edid_check_valid_mode(hdev, para);
#else
	if (find_brr_mode(mode, input, brr_mode))
		mode = &brr_mode[0];
	para = hdmitx21_tst_fmt_name(mode, color);
	return hdmitx21_validate_mode(hdev, para);
#endif
}

#endif

/*
 * @param enable         [in] enable test mode or not
 *
 */
void meson_mode_set_test_mode_enable(const bool enable) {
    test_mode = enable;
}

