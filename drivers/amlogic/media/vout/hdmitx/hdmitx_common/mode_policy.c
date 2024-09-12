/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "mode_policy.h"
#include "mode_private.h"
#include "mode_policy_parser.h"

static struct meson_policy g_in[MESON_MODE_CON_MAX];

#define GET_CURRENT_POLICY(connector) \
    struct meson_policy *mp = NULL; \
    if ((connector) < 0 || (connector) >= MESON_MODE_CON_MAX) \
        return -EINVAL; \
    mp = &g_in[connector];

static bool is_support_420_mode(const char *mode) {
    /*
     * check input param
     */
    if (!mode) {
        SYS_LOGI("%s mode is null\n", __FUNCTION__);
        return false;
    }

    if (!strcmp(mode, MODE_4K2K60HZ) ||
        !strcmp(mode, MODE_4K2K50HZ) ||
        !strcmp(mode, MODE_4K2KSMPTE60HZ) ||
        !strcmp(mode, MODE_4K2KSMPTE50HZ) ||
        !strcmp(mode, MODE_4K2K100HZ) ||
        !strcmp(mode, MODE_4K2K120HZ) ||
        !strcmp(mode, MODE_8K4K60HZ) ||
        !strcmp(mode, MODE_8K4K50HZ) ||
        !strcmp(mode, MODE_8K4K48HZ) ||
        !strcmp(mode, MODE_8K4K30HZ) ||
        !strcmp(mode, MODE_8K4K25HZ) ||
        !strcmp(mode, MODE_8K4K24HZ))
        return true;
    return false;
}

static bool is_dv_prefer(struct meson_policy_in *input) {
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGI("%s input is null\n", __FUNCTION__);
        return false;
    }

    struct meson_hdr_info *hdr_ptr = &input->hdr_info;

    /*
     * not dv priority
     */
    if (hdr_ptr->hdr_priority != MESON_DOLBY_VISION_PRIORITY
        && hdr_ptr->hdr_priority != MESON_G_DV_HDR10_HLG
        && hdr_ptr->hdr_priority != MESON_G_DV_HDR10
        && hdr_ptr->hdr_priority != MESON_G_DV_HLG
        && hdr_ptr->hdr_priority != MESON_G_DV) {
        SYS_LOGI("not prefer dv, hdr_priority:%x\n", hdr_ptr->hdr_priority);
        return false;
    }

    /*
     * not force dv
     */
    if (hdr_ptr->hdr_policy == MESON_HDR_POLICY_FORCE
        && hdr_ptr->hdr_force_mode != MESON_HDR_FORCE_MODE_DV) {
        SYS_LOGI("not force dv, hdr_policy:%d hdr_force_mode:%d\n", hdr_ptr->hdr_policy, hdr_ptr->hdr_force_mode);
        return false;
    }

    /*
     * dv is enable and tv also support it
     */
    if (hdr_ptr->is_amdv_enable && hdr_ptr->is_tv_supportDv)
        return true;

    return false;
}

static bool is_hdr_prefer(struct meson_policy_in *input) {
    /*
     * check input param
     */
    if (!input) {
        SYS_LOGI("%s input is null\n", __FUNCTION__);
        return false;
    }

    struct meson_hdr_info *hdr_ptr = &(input->hdr_info);

    /*
     * not prefer hdr
     */
    if (!hdr_ptr->is_hdr_resolution_priority) {
        SYS_LOGI("not prefer hdr is_hdr_resolution_priority:%d\n", hdr_ptr->is_hdr_resolution_priority);
        return false;
    }
    /*
     * not force hdr
     */
    if (hdr_ptr->hdr_policy == MESON_HDR_POLICY_FORCE
        && !(hdr_ptr->hdr_force_mode == MESON_HDR_FORCE_MODE_HDR10
        ||  hdr_ptr->hdr_force_mode == MESON_HDR_FORCE_MODE_HLG
        ||  hdr_ptr->hdr_force_mode == MESON_HDR_FORCE_MODE_HDR10PLUS)) {
        SYS_LOGI("not force hdr, hdr_policy:%d hdr_force_mode:%d\n", hdr_ptr->hdr_policy, hdr_ptr->hdr_force_mode);
        return false;
    }

    /*
     * policy is prefer dv or hdr and tv support hdr
     */
    if (hdr_ptr->is_tv_supportHDR &&
        ((hdr_ptr->hdr_priority != MESON_SDR_PRIORITY)
        && (hdr_ptr->hdr_priority != MESON_G_SDR)))
        return true;

    return false;
}

static int32_t update_dv_type(struct meson_hdr_info *info) {
    char type[MESON_MODE_LEN];
    /*
     * check input param
     */
    if (!info) {
        SYS_LOGE("%s info is null\n", __FUNCTION__);
        return false;
    }

    /*
     * 1. update input amdolby vision info
     * 1.1 update current amdolby vision mode
     */
    strlcpy(type, info->ubootenv_dv_type, sizeof(type));

    /*
     * 2. check tv support or not
     */
    if ((strstr(type, "1") != NULL) && support_DV_RGB_444_8BIT(info)) {
        return DOLBY_VISION_STD_ENABLE;
    } else if ((strstr(type, "2") != NULL) && support_LL_YCbCr_422_12BIT(info)) {
        return DOLBY_VISION_LL_YUV;
    } else if ((strstr(type, "3") != NULL) &&
            (support_LL_RGB_444_12BIT(info) ||
             support_LL_RGB_444_10BIT(info))) {
        return DOLBY_VISION_LL_RGB;
    } else if (strstr(type, "0") != NULL) {
        return DOLBY_VISION_DISABLE;
    }

    /*
     * 3. amdolby vision best policy:STD->LL_YUV->LL_RGB for netflix request
     * amdolby vision best policy:LL_YUV->STD->LL_RGB for amdolby vision request
     */
    if (support_DV_RGB_444_8BIT(info) ||
        support_LL_YCbCr_422_12BIT(info)) {
        if (support_DV_RGB_444_8BIT(info)) {
            return DOLBY_VISION_STD_ENABLE;
        } else if (support_LL_YCbCr_422_12BIT(info)) {
            return DOLBY_VISION_LL_YUV;
        }
    } else if (support_LL_RGB_444_12BIT(info) ||
            support_LL_RGB_444_10BIT(info)) {
        return DOLBY_VISION_LL_RGB;
    }

    return DOLBY_VISION_DISABLE;
}

/*
 * transfer amdolby vision mode to color format
 */
static void update_dv_attr(struct meson_hdr_info *info, int amdv_type, char *amdv_attr) {
    /*
     * check input param
     */
    if (!info || !amdv_attr) {
        SYS_LOGE("%s info or amdv_attr is null\n", __FUNCTION__);
        return;
    }

    switch (amdv_type) {
        case DOLBY_VISION_STD_ENABLE:
            strcpy(amdv_attr, "444,8bit");
            break;
        case DOLBY_VISION_LL_YUV:
            strcpy(amdv_attr, "422,12bit");
            break;
        case DOLBY_VISION_LL_RGB:
            if (support_LL_RGB_444_12BIT(info)) {
                strcpy(amdv_attr, "444,12bit");
            } else if (support_LL_RGB_444_10BIT(info)) {
                strcpy(amdv_attr, "444,10bit");
            }
            break;
        default:
            strcpy(amdv_attr, "444,8bit");
            break;
    }

    SYS_LOGI("amdv_type :%d amdv_attr:%s\n", amdv_type, amdv_attr);
}

/*
 * find the index of mode base the hdmi resolution priority table
 * TODO: refactor
 */
static int32_t find_resolution_index(const char *mode, int flag) {
    /*
     * check mode is or not valid mode
     */
    int32_t validMode = 0;
    /*
     * check input param
     */
    if (!mode) {
        SYS_LOGE("%s mode is null\n", __FUNCTION__);
        return -1;
    }

    if (strlen(mode) > 0) {
        for (int i = 0; i < ARRAY_SIZE(DISPLAY_MODE_LIST); i++) {
            if (strcmp(mode, DISPLAY_MODE_LIST[i]) == 0) {
                validMode = 1;
                break;
            }
        }
    }

    if (!validMode) {
        SYS_LOGI("the mode [%s] is not valid\n", mode);
        return -1;
    }

    /*
     * frame rate priority than resolution
     * ex:1080p60hz prefer to 2160p30hz
     */
    if (flag == MESON_POLICY_FRAMERATE) {
        for (int64_t index = 0; index < sizeof(MODE_FRAMERATE_FIRST)/sizeof(char *); index++) {
            if (strcmp(mode, MODE_FRAMERATE_FIRST[index]) == 0) {
                return index;
            }
        }
    } else {
        /*
         * resolution priority than frame rate
         * ex:2160p30hz prefer to 1080p60hz
         */
        for (int64_t index = 0; index < ARRAY_SIZE(DISPLAY_MODE_LIST); index++) {
            if (strcmp(mode, DISPLAY_MODE_LIST[index]) == 0) {
                return index;
            }
        }
    }

    return -1;
}

static bool is_dv_support_mode(struct meson_policy_in *input, const char *mode) {
    bool validMode = false;

    /*
     * check input param
     */
    if (!input || !mode) {
        SYS_LOGE("%s input or mode is null\n", __FUNCTION__);
        return false;
    }

    /*
     * check mode support or not
     */
    if (!is_support_hdmimode(input, mode)) {
        SYS_LOGI("%s could not find mode:%s\n", __func__, mode);
        return validMode;
    }

    /*
     * special resolution not support dv
     */
    if ((strstr(mode, "480p") != NULL) ||
        (strstr(mode, "576p") != NULL) ||
        (strstr(mode, "smpte") != NULL) ||
        (strstr(mode, "4096") != NULL) ||
        (strstr(mode, "i") != NULL)) {
        SYS_LOGI("%s mode:%s not support dv\n", __func__, mode);
        return validMode;
    }

    /*
     * need to check the flag of Parity for high frame rate
     */
    if (!strcmp(mode, MODE_1080P100HZ) ||
        !strcmp(mode, MODE_1080P120HZ) ||
        !strcmp(mode, MODE_720P100HZ) ||
        !strcmp(mode, MODE_720P120HZ)) {
        if (support_DV_VSVDB_PARITY(&input->hdr_info)) {
            validMode = true;
        }
    } else {
        /*
         * resolution larger than dv max resolution not support
         */
        if (find_resolution_index(mode, MESON_POLICY_RESOLUTION) <
            find_resolution_index(input->hdr_info.dv_max_mode, MESON_POLICY_RESOLUTION)) {
            validMode = false;
        } else {
            validMode = true;
        }
    }

    return validMode;
}

static int32_t amdv_update_mode(struct meson_policy_in *input,
                    char *cur_outputmode,
                    int amdv_type,
                    char *final_displaymode,
                    enum meson_mode_policy policy) {
    char dv_displaymode[MESON_MODE_LEN] = {0};
    int32_t ret = 0;

    /*
     * check input param
     */
    if (!input || !cur_outputmode || !final_displaymode) {
        SYS_LOGE("%s input or cur_outputmode or final_displaymode is null\n", __FUNCTION__);
        return ret;
    }

    /*
     * 1. update tv support amdolby vision resolution
     */
    for (int i = 0; i < ARRAY_SIZE(DISPLAY_MODE_LIST); i++) {
        if (strstr(input->hdr_info.dv_max_mode, DISPLAY_MODE_LIST[i]) != NULL) {
            strlcpy(dv_displaymode, DISPLAY_MODE_LIST[i], sizeof(dv_displaymode));
            break;
        }
    }

    /*
     * 2. find prefer amdolby vision resolution
     */
    /*
     * if current resolution is not support by new tv, run dv best policy
     */
    if (!is_support_hdmimode(input, cur_outputmode)) {
        policy = MESON_POLICY_BEST;
    }

    if (policy == MESON_POLICY_BEST
        || policy == MESON_POLICY_RESOLUTION
        || policy == MESON_POLICY_FRAMERATE) {
        /* 2.1 best policy enable case */
        if (!strcmp(dv_displaymode, MODE_4K2K60HZ)) {
            /* TV support amdolby vision 2160p60hz case */
            if (amdv_type == DOLBY_VISION_LL_RGB) {
                /* amdolby vision LL RGB(rgb 10/12bit) only support 1080p60hz */
                strcpy(final_displaymode, MODE_1080P);
            } else {
                /* other amdolby visin mode,use 2160p60hz */
                strcpy(final_displaymode, MODE_4K2K60HZ);
            }
        } else {
            /* TV support amdolby vision non 2160p60hz case */
            if (!strcmp(dv_displaymode, MODE_4K2K30HZ) ||
                    !strcmp(dv_displaymode,MODE_4K2K25HZ) ||
                    !strcmp(dv_displaymode, MODE_4K2K24HZ)) {
                /*
                 * TV support amdolby vision support 2160p30hz or 2160p25hz or 2160p24hz
                 * 1080p60hz prefer to 2160p30hz 2160p25hz 2160p24hz
                 */
                strcpy(final_displaymode, MODE_1080P);
            } else {
                /*
                 * TV support amdolby vision non 2160p30hz 2160p25hz 2160p24hz
                 * use tv support amdolby vision resolution
                 */
                strcpy(final_displaymode, dv_displaymode);
            }
        }
    } else {
        /*
         * 2.1 best policy disable case
         * smpte(3840x2160@XXhz) and i timing not support amdolby vision
         * hdmi output resolution need small than amdolby vision resolution
         * x:amdolby vision support 1080p60hz,only can output small 1080p60hz resolution
         */
        if (!is_dv_support_mode(input, cur_outputmode)) {
            ret = -1;
            SYS_LOGI("cur_outputmode:%s doesn't support dv\n", cur_outputmode);
        } else {
            strcpy(final_displaymode, cur_outputmode);
        }
    }

    SYS_LOGI("final_displaymode:%s, cur_outputmode:%s, dv_displaymode:%s\n", final_displaymode, cur_outputmode, dv_displaymode);
    return ret;
}

static int32_t dv_scene_process(struct meson_policy_in *input,
                                struct meson_policy_out *output,
                                enum meson_mode_policy policy) {
    int32_t ret = -1;
    int amdv_type = DOLBY_VISION_DISABLE;
    char amdv_attr[MESON_MODE_LEN] = {0};
    char final_displaymode[MESON_MODE_LEN] = {0};
    char cur_displaymode[MESON_MODE_LEN] = {0};

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return ret;
    }
    /*
     * 1. update amdolby vision output type
     */
    amdv_type = update_dv_type(&input->hdr_info);

    output->amdv_type = amdv_type;
    SYS_LOGI("amdv type:%d\n", output->amdv_type);

    /*
     * 2. update amdolby vision output output mode to color format
     * 2.1 update amdolby vision deepcolor
     */
    update_dv_attr(&input->hdr_info, amdv_type, amdv_attr);
    strlcpy(output->deepcolor, amdv_attr, sizeof(output->deepcolor));
    SYS_LOGI("dv final_deepcolor:%s\n", output->deepcolor);

    /*
     * 2.2 update amdolby vision output resolution
     */
    strlcpy(cur_displaymode, input->cur_displaymode, sizeof(cur_displaymode));

    ret = amdv_update_mode(input,
                   cur_displaymode,
                   amdv_type,
                   final_displaymode,
                   policy);
    strlcpy(output->displaymode, final_displaymode, sizeof(output->displaymode));
    SYS_LOGI("dv final_displaymode:%s\n", output->displaymode);

    return ret;
}

/*
 * check 4k50/4k60 hdr support or not base driver edid
 */
static bool is_support_4kHDR(struct meson_policy_in *input,
                             struct meson_policy_out *output) {
    const char **color_list = NULL;
    int color_list_length   = 0;

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return false;
    }

    /*
     * use 4k hdr color format table
     */
    color_list      = HDR_4K_COLOR_ATTRIBUTE_LIST;
    color_list_length = ARRAY_SIZE(HDR_4K_COLOR_ATTRIBUTE_LIST);

    /*
     * choose prefer color format and resolution for 4k hdr
     * modes_ptr:the list of TV support resolution from connector
     * dc_cap:the list of TV support color format from driver parse edid
     */
    for (int i = 0; i < color_list_length; i++) {
        if (is_support_color_format(input, color_list[i])) {
            const char **resolution_list = NULL;
            int resolution_list_length   = 0;
            /*
             * use 4k hdr resolution table
             */
            resolution_list = MODE_4K_LIST;
            resolution_list_length = ARRAY_SIZE(MODE_4K_LIST);
            for (int j = 0; j < resolution_list_length; j++) {
                if (is_support_hdmimode(input, resolution_list[j])) {
                    if (mode_support_check(resolution_list[j], color_list[i], input)) {
                       strlcpy(output->deepcolor, color_list[i], sizeof(output->deepcolor));
                       strlcpy(output->displaymode, resolution_list[j], sizeof(output->displaymode));
                       SYS_LOGI("%s mode:[%s], deep color:[%s]\n", __FUNCTION__, output->deepcolor, output->displaymode);
                       return true;
                    }
                }
            }
        }
    }

    SYS_LOGI("%s 4k hdr not support\n", __FUNCTION__);
    return false;
}

/*
 * check non 4k hdr support or not
 */
static bool is_support_non4kHDR(struct meson_policy_in *input,
                                struct meson_policy_out *output) {
    const char **color_list = NULL;
    int color_list_length = 0;

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return false;
    }

    /*
     * use non 4k hdr color format table
     */
    color_list        = HDR_NON4K_COLOR_ATTRIBUTE_LIST;
    color_list_length = ARRAY_SIZE(HDR_NON4K_COLOR_ATTRIBUTE_LIST);

    /*
     * choose prefer color format and resolution for non 4k hdr
     * modes_ptr:the list of TV support resolution from connector
     * dc_cap:the list of TV support color format from driver parse edid
     */
    for (int i = 0; i < color_list_length; i++) {
        if (is_support_color_format(input, color_list[i])) {
            const char **resolution_list = NULL;
            int resolution_list_length = 0;
            /*
             * use non 4k hdr resolution table
             */
            resolution_list = MODE_NON4K_LIST;
            resolution_list_length = ARRAY_SIZE(MODE_NON4K_LIST);
            for (int j = 0; j < resolution_list_length; j++) {
                if (is_support_hdmimode(input, resolution_list[j])) {
                    if (mode_support_check(resolution_list[j], color_list[i], input)) {
                       strlcpy(output->deepcolor, color_list[i], sizeof(output->deepcolor));
                       strlcpy(output->displaymode, resolution_list[j], sizeof(output->displaymode));
                       SYS_LOGI("%s mode:[%s], color format:[%s]\n", __FUNCTION__, output->displaymode, output->deepcolor);
                       return true;
                    }
                }
            }
        }
    }

    SYS_LOGI("%s non 4k hdr not support\n", __FUNCTION__);
    return false;
}

static bool find_hdr_prefer_mode(struct meson_policy_in *input,
                                 struct meson_policy_out *output) {
    bool find = false;

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return find;
    }

    /*
     * box can support 4k case
     * find prefer 4k hdr resolution and color format base driver edid
     */
    if (input->con_info.is_support4k == true) {
        find = is_support_4kHDR(input, output);
    }

    /*
     * not find 4k hdr mode and find non 4k case
     * find prefer non 4k hdr resolution and color format base driver edid
     */
    if (!find)
        find = is_support_non4kHDR(input, output);

    return find;
}

static void get_best_color_attr(struct meson_policy_in *input,
                               const char *outputmode, char* colorattribute) {
    int length = 0;
    const char **color_list = NULL;

    /*
     * check input param
     */
    if (!input || !outputmode || !colorattribute) {
        SYS_LOGE("%s input or outputmode or colorattribute is null\n", __FUNCTION__);
        return;
    }

    /*
     * if read /sys/class/amhdmitx/amhdmitx0/dc_cap is NULL
     * use default color format(444 8bit)
     */
    char brr_mode[MESON_MODE_LEN] = {0};
    find_brr_mode(outputmode, input, brr_mode);
    SYS_LOGI("get_best_color_attr current mode:[%s]  brr_mode:[%s]\n", outputmode, brr_mode);

    if (!is_hdmi_dc_cap_ok(input)) {
        if (is_support_420_mode(brr_mode)) {
            strcpy(colorattribute, MESON_DEFAULT_COLOR_FORMAT_4K);
        } else {
            strcpy(colorattribute, MESON_DEFAULT_COLOR_FORMAT);
        }

        SYS_LOGE("dc_cap is NULL\n");
        return;
    }

    /*
     * 1. select the color format table for different resolution or scene.
     */
    if (is_support_420_mode(brr_mode)) {
        /*
         * 2160p50hz 2160p60hz 3840x2160p60hz 3840x2160p50hz case
         */
        if (input->hdr_info.is_lowpower_mode) {
            color_list = COLOR_ATTRIBUTE_LIST3;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST3);
        } else {
            color_list = COLOR_ATTRIBUTE_LIST1;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
        }
    } else {
        /*
         * except 2160p60hz 2160p50hz 3840x2160p60hz 3840x2160p60hz case
         */
        if (input->hdr_info.is_lowpower_mode) {
            /*
             * 8bit prefer to 10bit for low power mode
             * detail priority as table
             */
            color_list = COLOR_ATTRIBUTE_LIST4;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST4);
        } else if (is_hdr_prefer(input)) {
            /*
             * hdr non 4k color format priority table
             * for user change resolution case
             * exp:connector 2160p60 420 8bit TV,when switch to 1080p60,
             * 10bit first for hdr,switch 2160p60,only 420 8bit.
             */
            color_list = COLOR_ATTRIBUTE_LIST2;
            length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
        } else {
            /*
             * sdr non 4k color format priority table
             */
            color_list = SDR_NON4K_COLOR_ATTRIBUTE_LIST;
            length = ARRAY_SIZE(SDR_NON4K_COLOR_ATTRIBUTE_LIST);
        }
    }

    /*
     * 2. select the preferred color format base resolution
     */
    for (int i = 0; i < length; i++) {
        if (is_support_color_format(input, color_list[i])) {
            /*
             * check resolution+color format support or not base driver edid
             */
            if (mode_support_check(outputmode, color_list[i], input)) {
                strcpy(colorattribute, color_list[i]);
                SYS_LOGI("support current mode:[%s], color format:[%s]\n", outputmode, colorattribute);
                break;
            }
        }
    }

    /*
     * TODO: what about not find
     */
}

int get_depth(char *color_attribute)
{
    int depth = 0;

    if (!color_attribute)
        return depth;
    if (strstr(color_attribute, "8bit"))
        depth  = 8;
    else if (strstr(color_attribute, "10bit"))
        depth = 10;
    else if (strstr(color_attribute, "12bit"))
        depth = 12;

    return depth;
}

static bool hdr_scene_process(struct meson_policy_in *input,
                              struct meson_policy_out *output,
                              enum meson_mode_policy policy) {
    bool find = false;

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return false;
    }

    if (strstr(input->con_info.ubootenv_colorattr, "8bit") != NULL) {
        input->con_info.is_bestcolorspace = true;
        SYS_LOGI("Do not output 8bit hdr. Change to auto color space");
    }

    SYS_LOGI("policy:%d is_bestcolorspace:%d state:%d\n", policy, input->con_info.is_bestcolorspace, input->state);

    /*
     * boot/hdmiplug/suspend/resume case
     */
    if ((input->state == MESON_SCENE_STATE_INIT) ||
        (input->state == MESON_SCENE_STATE_POWER)) {
        /*
         * if current resolution is not support by new tv, run dv best policy
         * ex:change to 2160p60hz and plug to FHD TV
         */
        if (!is_support_hdmimode(input, input->cur_displaymode)) {
            policy = MESON_POLICY_BEST;
        }

        /*
         * frame rate or resolution priority depend isframeratepriority
         * when auto to best is enable
         */
        if (policy == MESON_POLICY_BEST) {
            if (input->con_info.isframeratepriority)
                policy = MESON_POLICY_FRAMERATE;
            else
                policy = MESON_POLICY_RESOLUTION;
        }

        if (policy == MESON_POLICY_FRAMERATE && input->con_info.is_bestcolorspace) {
            /*
             * best resolution policy and best color space enable case
             */
            find = find_hdr_prefer_mode(input, output);
        } else if (policy == MESON_POLICY_RESOLUTION && input->con_info.is_bestcolorspace) {
            /*
             * best resolution policy and best color space enable case
             */
            const char **resolution_list = NULL;
            int resolution_list_length   = 0;
            bool first = false;

            resolution_list        = MODE_RESOLUTION_FIRST;
            resolution_list_length = ARRAY_SIZE(MODE_RESOLUTION_FIRST);

            for (int j = resolution_list_length - 1; j >= 0 ; j--) {
                char color_attribute[MESON_MODE_LEN] = { 0 };
                int depth = 0;

                if (is_support_hdmimode(input, resolution_list[j])) {
                    get_best_color_attr(input, resolution_list[j], color_attribute);
                    depth = get_depth(color_attribute);
                    if (depth != 0) {
                        find = true;
                        if (depth >= 10) {
                            strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
                            strlcpy(output->displaymode, resolution_list[j], sizeof(output->displaymode));
                            break;
                        }

                        if (!first) {
                            strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
                            strlcpy(output->displaymode, resolution_list[j], sizeof(output->displaymode));
                            first = true;
                        }
                    }
                }
            }
        } else if (policy == MESON_POLICY_RESOLUTION || policy == MESON_POLICY_FRAMERATE) {
            /*
             * best resolution policy enable and best color space disable case
             */
            const char **resolution_list = NULL;
            int resolution_list_length   = 0;
            /*
             * choose base table
             */
            if (policy == MESON_POLICY_FRAMERATE) {
                resolution_list        = MODE_FRAMERATE_FIRST;
                resolution_list_length = ARRAY_SIZE(MODE_FRAMERATE_FIRST);
            } else if (policy == MESON_POLICY_RESOLUTION) {
                resolution_list        = MODE_RESOLUTION_FIRST;
                resolution_list_length = ARRAY_SIZE(MODE_RESOLUTION_FIRST);
            }

            /*
             * choose prefer resolution for hdr
             */
            for (int j = resolution_list_length - 1; j >= 0 ; j--) {
                if (is_support_hdmimode(input, resolution_list[j])) {
                    if (meson_mode_support_mode(MESON_MODE_HDMI, MESON_HDR10_PRIORITY, resolution_list[j]) == 0) {
                        SYS_LOGI("%s mode:[%s] support hdr\n", __FUNCTION__, resolution_list[j]);
                        strlcpy(output->displaymode, resolution_list[j], sizeof(output->displaymode));
                        find = true;
                        break;
                    }
                }
            }

            if (find) {
                /*
                 * check color space base resolution support or not
                 * if not support, choose prefer color space base resolution
                 */
                if (mode_support_check(output->displaymode, input->con_info.ubootenv_colorattr, input)) {
                    SYS_LOGI("support current mode:[%s], deep color:[%s]\n", output->displaymode, input->con_info.ubootenv_colorattr);
                    strlcpy(output->deepcolor, input->con_info.ubootenv_colorattr, sizeof(output->deepcolor));
                } else {
                    char color_attribute[MESON_MODE_LEN] = {0};
                    get_best_color_attr(input, output->displaymode, color_attribute);
                    strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
                }
            } else {
                SYS_LOGE("%s not find mode support hdr\n", __FUNCTION__);
            }
        } else if (input->con_info.is_bestcolorspace) {
            /*
             * best resolution policy disable and best color space enable case
             */
            const char **color_list = NULL;
            int color_list_length   = 0;
            char brr_mode[MESON_MODE_LEN] = {0};
            find_brr_mode(input->cur_displaymode, input, brr_mode);

            if (is_support_420_mode(brr_mode)) {
                //2160p50hz 2160p60hz 3840x2160p60hz 3840x2160p50hz case
                //use 4k color format table
                color_list        = COLOR_ATTRIBUTE_LIST1;
                color_list_length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST1);
            } else {
                //except 2160p60hz 2160p50hz 3840x2160p60hz 3840x2160p60hz case
                //use non 4k color format table
                color_list        = COLOR_ATTRIBUTE_LIST2;
                color_list_length = ARRAY_SIZE(COLOR_ATTRIBUTE_LIST2);
            }

            for (int j = 0; j < color_list_length; j++) {
                if (is_support_color_format(input, color_list[j])) {
                    if (mode_support_check(input->cur_displaymode, color_list[j], input)) {
                        SYS_LOGI("%s mode:[%s], color format:[%s]\n", __FUNCTION__, input->cur_displaymode, color_list[j]);
                        strlcpy(output->deepcolor, color_list[j], sizeof(output->deepcolor));
                        strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
                        find = true;
                        break;
                    }
                }
            }
        } else {
            /*
             * best resolution policy disable and best color space disable case
             */
            /*
             * 1.check mode+color format support or not
             */
            if (mode_support_check(input->cur_displaymode, input->con_info.ubootenv_colorattr, input)) {
                SYS_LOGI("support current mode:[%s], deep color:[%s]\n", input->cur_displaymode, input->con_info.ubootenv_colorattr);
                strlcpy(output->deepcolor, input->con_info.ubootenv_colorattr, sizeof(output->deepcolor));
                strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
                find = true;
            } else if (is_support_hdmimode(input, input->cur_displaymode)) {
                SYS_LOGI("support current mode:[%s]\n", input->cur_displaymode);
                /*
                 * 2.check cur_displaymode support or not
                 * if displaymode support ,and find best color format base mode.
                 */
                char color_attribute[MESON_MODE_LEN] = {0};
                get_best_color_attr(input, input->cur_displaymode, color_attribute);
                strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
                strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
                find = true;
            }
        }

        /*
         * not find support mode and color format
         * try best policy
         */
        if (!find && !(policy == MESON_POLICY_FRAMERATE && input->con_info.is_bestcolorspace)) {
            find = find_hdr_prefer_mode(input, output);
        }
    } else {
        /*
         * switch case
         */
        /*
         * 1.check cur_displaymode + ubootenv.var.colorattribute support or not
         * except from third apk or framework set mode.
         */
        if (mode_support_check(input->cur_displaymode, input->con_info.ubootenv_colorattr, input)
            && !input->con_info.is_bestcolorspace) {
            SYS_LOGI("support current mode:[%s], deep color:[%s]\n", input->cur_displaymode, input->con_info.ubootenv_colorattr);
            strlcpy(output->deepcolor, input->con_info.ubootenv_colorattr, sizeof(output->deepcolor));
            strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
            find = true;
        } else if (is_support_hdmimode(input, input->cur_displaymode)) {
            /*
             * 2.check cur_displaymode support or not
             * if displaymode support ,and find best color format base mode.
             */
            char color_attribute[MESON_MODE_LEN] = {0};
            get_best_color_attr(input, input->cur_displaymode, color_attribute);
            strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
            strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
            find = true;
        } else {
            /*
             * 3.find best hdr prefer mode
             */
            find = find_hdr_prefer_mode(input, output);
        }
    }

    return find;
}


static void get_highest_mode_by_policy(struct meson_policy_in *input,
                                        char *mode, enum meson_mode_policy policy) {
    const char **resolution_list = NULL;
    int resolution_list_length   = 0;

    /*
     * check input param
     */
    if (!input || !mode) {
        SYS_LOGE("%s input or mode is null\n", __FUNCTION__);
        return;
    }

    /*
     * choose base table
     */
    if (policy == MESON_POLICY_BEST) {
        if (input->con_info.isframeratepriority) {
            resolution_list        = MODE_FRAMERATE_FIRST;
            resolution_list_length = ARRAY_SIZE(MODE_FRAMERATE_FIRST);
        } else {
            resolution_list        = MODE_RESOLUTION_FIRST;
            resolution_list_length = ARRAY_SIZE(MODE_RESOLUTION_FIRST);
        }
    } else  if (policy == MESON_POLICY_FRAMERATE) {
        resolution_list        = MODE_FRAMERATE_FIRST;
        resolution_list_length = ARRAY_SIZE(MODE_FRAMERATE_FIRST);
    } else if (policy == MESON_POLICY_RESOLUTION) {
        resolution_list        = MODE_RESOLUTION_FIRST;
        resolution_list_length = ARRAY_SIZE(MODE_RESOLUTION_FIRST);
    } else {
        resolution_list        = MODE_FRAMERATE_FIRST;
        resolution_list_length = ARRAY_SIZE(MODE_FRAMERATE_FIRST);
    }

    /*
     * find preferred mode
     */
    for (int i = resolution_list_length - 1; i >= 0 ; i--) {
        if (is_support_hdmimode(input, resolution_list[i])) {
            strcpy(mode, resolution_list[i]);
            SYS_LOGI("%s preferred mode:[%s]\n", __FUNCTION__, mode);
            break;
        }
    }
}

static void get_hdmi_outputmode(struct meson_policy_in *input,
                                char *mode, enum meson_mode_policy policy) {
    /*
     * check input param
     */
    if (!input || !mode) {
        SYS_LOGE("%s input or mode is null\n", __FUNCTION__);
        return;
    }

    /*
     * Fall back to 480p if EDID be parsed ng
     */
    if (!is_hdmi_edid_parserok(input)) {
        strcpy(mode, MESON_DEFAULT_HDMI_MODE);
        SYS_LOGE("EDID parsing error detected\n");
        return;
    }

    if (policy == MESON_POLICY_INVALID) {
        /*
         * if current mode support, use current mode
         */
        if (is_support_hdmimode(input, input->cur_displaymode))
            strcpy(mode, input->cur_displaymode);
        else
        /*
         * if current mode not support, find best prefer resolution base driver edid
         */
            get_highest_mode_by_policy(input, mode, MESON_POLICY_BEST);
    } else {
        get_highest_mode_by_policy(input, mode, policy);
    }

    SYS_LOGI("%s mode:%s\n", __FUNCTION__, mode);
}

static void get_hdmi_color_attr(struct meson_policy_in *input,
                                const char *outputmode,
                                char *color_attr) {
    char colorTemp[MESON_MODE_LEN] = {0};

    /*
     * check input param
     */
    if (!input || !outputmode || !color_attr) {
        SYS_LOGE("%s input or mode or color_attr is null\n", __FUNCTION__);
        return;
    }

    /*
     * if read /sys/class/amhdmitx/amhdmitx0/dc_cap is NULL.
     * use default color format
     */
    char brr_mode[MESON_MODE_LEN] = {0};
    find_brr_mode(outputmode, input, brr_mode);

    if (!is_hdmi_dc_cap_ok(input)) {
        if (is_support_420_mode(brr_mode)) {
            strcpy(color_attr, MESON_DEFAULT_COLOR_FORMAT_4K);
        } else {
            strcpy(color_attr, MESON_DEFAULT_COLOR_FORMAT);
        }

        SYS_LOGE("Error!!! Do not find sink color list, use default color attribute:%s\n", color_attr);
        return;
    }

    /*
     * use ubootenv.var.colorattribute when best color space policy is disable
     * will check resolution + color format be support TV EDID
     */
    if (!input->con_info.is_bestcolorspace) {
        strlcpy(colorTemp, input->con_info.ubootenv_colorattr, sizeof(colorTemp));
        if (mode_support_check(outputmode, colorTemp, input)) {
            strcpy(color_attr, input->con_info.ubootenv_colorattr);
        } else {
            get_best_color_attr(input, outputmode, color_attr);
        }
    } else {
        /*
         * best policy enable case
         * select the preferred color format base outputmode(resolution)
         */
        get_best_color_attr(input, outputmode, color_attr);
    }

    /*
     * if colorAttr is NULL above steps, will defines a initial value to it
     * edid_parsing ng, will defines a initial value to it
     */
    if (!strstr(color_attr, "bit")
        || !is_hdmi_edid_parserok(input)) {
        strcpy(color_attr, MESON_DEFAULT_COLOR_FORMAT);
    }

    SYS_LOGI("get hdmi color attribute : [%s], outputmode is: [%s]\n",
        color_attr, outputmode);
}

static void update_deepcolor(struct meson_policy_in *input,
                             const char* outputmode,
                             char* colorattribute) {
    /*
     * check input param
     */
    if (!input || !outputmode || !colorattribute) {
        SYS_LOGE("%s input or mode or colorattribute is null\n", __FUNCTION__);
        return;
    }

    if (input->con_info.is_deepcolor) {
        /*
         * deep color(10/12bit) enable case
         */
        get_hdmi_color_attr(input, outputmode, colorattribute);
    } else {
        /*
         * deep color disable case
         */
        strcpy(colorattribute, "default");
    }

    SYS_LOGI("colorattribute = %s\n", colorattribute);
}

static void sdr_scene_process(struct meson_policy_in *input,
                              struct meson_policy_out *output,
                              enum meson_mode_policy policy) {
    char outputmode[MESON_MODE_LEN] = {0};

    /*
     * check input param
     */
    if (!input || !output) {
        SYS_LOGE("%s input or output is null\n", __FUNCTION__);
        return;
    }

    /*
     * 1. choose resolution and frame rate
     */
    if ((input->state == MESON_SCENE_STATE_INIT) ||
        (input->state == MESON_SCENE_STATE_POWER)) {
        /*
         * boot/hot plug/suspend/resmue case
         * choose resolution, frame rate base driver edid
         */

        if (input->con_info.sink_type != MESON_SINK_TYPE_NONE) {
            /* hdmi connect */
            get_hdmi_outputmode(input, outputmode, policy);
        } else {
            /* hdmi not connect */
            strlcpy(outputmode, input->con_info.ubootenv_cvbsmode, sizeof(outputmode));
        }
        /* not find prefer resolution,use default resolution */
        if (strlen(outputmode) == 0) {
            strlcpy(outputmode, MESON_DEFAULT_HDMI_MODE, sizeof(outputmode));
        }

        strlcpy(output->displaymode, outputmode, sizeof(output->displaymode));
        SYS_LOGI("final_displaymode:%s\n", output->displaymode);
    } else if (input->state == MESON_SCENE_STATE_SWITCH) {
        /*
         * user/framework change scene
         * doesn't read hdmi info for ui switch scene
         * use user want to set resolution and frame rate
         */
        strlcpy(output->displaymode, input->cur_displaymode, sizeof(output->displaymode));
        SYS_LOGI("final_displaymode:%s\n", output->displaymode);
    }

    /*
     * 2. choose color format, bit-depth
     */
    char color_attribute[MESON_MODE_LEN] = {0};
    update_deepcolor(input, output->displaymode, color_attribute);
    strlcpy(output->deepcolor, color_attribute, sizeof(output->deepcolor));
    SYS_LOGI("final_deepcolor = %s\n", output->deepcolor);
}

/*
 * Set the mode policy
 *
 * @param connector         [in] connector type info
 * @param policy            [in] meson policy
 *
 * Return of a value other than 0 means an error has occurred:
 * -EINVAL - invalid connector type
 */
int32_t meson_mode_set_policy(int32_t connector, const meson_mode_policy_e policy) {
    GET_CURRENT_POLICY(connector);
    SYS_LOGI("connector:%d to policy:%d\n", connector, policy);
    mp->policy = policy;
    return 0;
}

/*
 * set mode policy input
 *
 * @param connector         [in] connector type info
 * @param input             [in] meson policy input, like hdr info, connector info
 *
 * Return of a value other than 0 means an error has occurred:
 * -EINVAL - invalid connector type or input
 */
int32_t meson_mode_set_policy_input(int32_t connector, const struct meson_policy_in *input) {
    GET_CURRENT_POLICY(connector);

    /*
     * check input param
     */
    if (!input) {
        SYS_LOGE("%s input is null\n", __FUNCTION__);
        return -EINVAL;
    }

    mp->input = *input;
    return 0;
}

/*
 * get mode policy output
 * @param connector         [in] connector type info
 * @param output            [out] meson policy output: mode info, colorspace
 *
 * Return of a value other than 0 means an error has occurred:
 * -EINVAL - invalid connector type or input
 */
int32_t meson_mode_get_policy_output(int32_t connector,
                                    struct meson_policy_out *output) {
    GET_CURRENT_POLICY(connector);
    struct meson_policy_in *input = &mp->input;
    enum meson_mode_policy policy = mp->policy;
    struct meson_policy_out  scene_output_info;
    int32_t dv_support = 0;

    /*
     * check input param
     */
    if (!output) {
        SYS_LOGE("%s output is null\n", __FUNCTION__);
        return -EINVAL;
    }

    memset(&scene_output_info, 0, sizeof(scene_output_info));

    /*
     * 1. amdolby vision scene process
     *    only for tv support dv and box enable dv
     */
    if (is_dv_prefer(input)) {
        dv_support = dv_scene_process(input, &scene_output_info, policy);
    } else if (input->hdr_info.is_amdv_enable) {
        /*
         * enable amdolby vision core when first boot connecting non dv tv
         */
        output->amdv_type = DOLBY_VISION_STD_ENABLE;
    } else {
        /*
         * UI disable amdolby vision core and boot keep the status
         */
        output->amdv_type = DOLBY_VISION_DISABLE;
    }

    /*
     * 2. hdr/sdr scene process
     *    and decide final display mode and deepcolor
     */
    if (is_dv_prefer(input) && dv_support == 0) {
        strcpy(output->displaymode, scene_output_info.displaymode);
        strcpy(output->deepcolor, scene_output_info.deepcolor);
        output->amdv_type = scene_output_info.amdv_type;
    } else if (is_hdr_prefer(input) || dv_support != 0) {
        hdr_scene_process(input, &scene_output_info, policy);
        strcpy(output->displaymode, scene_output_info.displaymode);
        strcpy(output->deepcolor, scene_output_info.deepcolor);
        if (input->hdr_info.is_amdv_enable)
            output->amdv_type = DOLBY_VISION_STD_ENABLE;
    } else {
        sdr_scene_process(input, &scene_output_info, policy);
        strcpy(output->displaymode, scene_output_info.displaymode);
        strcpy(output->deepcolor, scene_output_info.deepcolor);
        if (input->hdr_info.is_amdv_enable)
            output->amdv_type = DOLBY_VISION_STD_ENABLE;
    }

    /*
     * 3. not find outputmode and use default mode
     */
    if (strlen(output->displaymode) == 0) {
        strlcpy(output->displaymode, MESON_DEFAULT_HDMI_MODE, sizeof(output->displaymode));
    }

    /*
     * 4. not find color space and use default mode
     */
    if (!strstr(output->deepcolor, "bit")) {
        strlcpy(output->deepcolor, MESON_DEFAULT_COLOR_FORMAT, sizeof(output->deepcolor));
    }

    mp->output = *output;

    SYS_LOGI("final_displaymode:%s, final_deepcolor:%s, amdv_type:%d\n",
        output->displaymode, output->deepcolor, output->amdv_type);

    return 0;
}

/*
 * @function: check mode is support or not as the hdr type
 * @param type: dv,hdr,sdr
 */
int32_t meson_mode_support_mode(int32_t connector, int32_t type, const char *mode) {
    int32_t ret = -EINVAL;

    /*
     * check input param
     */
    if (!mode) {
        SYS_LOGE("%s mode is null\n", __FUNCTION__);
        return ret;
    }

    SYS_LOGI("%s type:%d mode:%s", __func__, type, mode);
    GET_CURRENT_POLICY(connector);
    struct meson_policy_in *input = &mp->input;

    /*
     * check mode support or not
     */
    if (!is_support_hdmimode(input, mode)) {
        SYS_LOGI("%s could not find mode:%s", __func__, mode);
        return ret;
    }

    if (type == MESON_HDR10_PRIORITY) {
        int length = 0;
        const char **color_list = NULL;
        /*
         * 1. select the color format table for different resolution
         */
        char brr_mode[MESON_MODE_LEN] = {0};
        find_brr_mode(mode, input, brr_mode);

        if (is_support_420_mode(brr_mode)) {
            /*
             * resolution support 420 case
             */
            color_list = HDR_4K_COLOR_ATTRIBUTE_LIST;
            length = ARRAY_SIZE(HDR_4K_COLOR_ATTRIBUTE_LIST);
        } else {
            color_list = HDR_NON4K_COLOR_ATTRIBUTE_LIST;
            length    = ARRAY_SIZE(HDR_NON4K_COLOR_ATTRIBUTE_LIST);
        }
        /*
         * 2. check support or not
         */
        for (int i = 0; i < length; i++) {
            if (is_support_color_format(input, color_list[i])) {
                /*
                 * check resolution+color format support or not base driver edid
                 */
                if (mode_support_check(mode, color_list[i], input)) {
                    SYS_LOGI("support current mode:[%s], color format:[%s]\n", mode, color_list[i]);
                    ret = 0;
                    break;
                }
            }
        }
    } else if (type == MESON_DOLBY_VISION_PRIORITY) {
        if (is_dv_support_mode(input, mode)) {
            SYS_LOGI("%s mode:%s support dv\n", __func__, mode);
            ret = 0;
        } else {
            SYS_LOGI("%s mode:%s not support dv\n", __func__, mode);
        }
    } else if (type == MESON_SDR_PRIORITY) {
        /*
         * no check for sdr
         */
        ret = 0;
    }

    return ret;
}

#ifndef __UBOOT__
/*
 * below api is uesd only by hwc and linux
 */

/*
 * @function: get color format list of mode
 */
int32_t meson_mode_get_support_color(int32_t connector, const char *mode, char* color) {
    /*
     * check input param
     */
    if (!mode || !color) {
        SYS_LOGE("%s mode or color is null\n", __FUNCTION__);
        return -EINVAL;
    }

    GET_CURRENT_POLICY(connector);
    struct meson_policy_in *input = &mp->input;
    char dc_cap[MESON_MAX_STR_LEN] = {0};
    strncpy(dc_cap, input->con_info.dc_cap, MESON_MAX_STR_LEN - 1);
    dc_cap[MESON_MAX_STR_LEN -1] = '\0';

    char *token = NULL;
    char *rest = dc_cap;
    for (token = strtok_r(rest, "\n", &rest); token != NULL; token = strtok_r(NULL, "\n", &rest)) {
        if (mode_support_check(mode, token, input)) {
            strcat(color, token);
            strcat(color, "\n");
        }
    }

    return 0;
}

const char *meson_hdrPriorityToString(int32_t type) {
    const char * typeStr;
    switch (type) {
        case MESON_DOLBY_VISION_PRIORITY:
            typeStr = "DV Priority";
            break;
        case MESON_HDR10_PRIORITY:
            typeStr = "HDR10 Priority";
            break;
        case MESON_SDR_PRIORITY:
            typeStr = "SDR priority";
            break;
        case MESON_G_DV_HDR10_HLG:
            typeStr = "DV_HDR10_HLG priority";
            break;
        case MESON_G_DV_HDR10:
            typeStr = "DV_HDR10 priority";
            break;
        case MESON_G_DV_HLG:
            typeStr = "DV_HLG priority";
            break;
        case MESON_G_HDR10_HLG:
            typeStr = "HDR10_HLG priority";
            break;
        case MESON_G_DV:
            typeStr = "DV priority";
            break;
        case MESON_G_HDR10:
            typeStr = "HDR10 priority";
            break;
        case MESON_G_HLG:
            typeStr = "HLG priority";
            break;
        case MESON_G_SDR:
            typeStr = "SDR priority";
            break;
        default :
            typeStr = "INVALID";
            break;
    }
    return typeStr;
}

const char *meson_hdrPolicyToString(int32_t type) {
    const char * typeStr;
    switch (type) {
        case MESON_HDR_POLICY_SINK:
            typeStr = "follow sink";
            break;
        case MESON_HDR_POLICY_SOURCE:
            typeStr = "follow source";
            break;
        case MESON_HDR_POLICY_FORCE:
            typeStr = "force";
            break;
        default:
            typeStr = "INVALID";
            break;
    }
    return typeStr;
}

const char *meson_dvModeTypeToString(const char *dvMode) {
    const char * typeStr;
    if (strstr(dvMode, "current amdv_mode = HDR10")) {
        typeStr = MESON_FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_HDR10];
    } else if (strstr(dvMode, "current amdv_mode = IPT_TUNNEL")) {
        typeStr = MESON_FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_DV];
    } else if (strstr(dvMode, "current amdv_mode = SDR8") ||
                  strstr(dvMode, "current amdv_mode = SDR10")) {
        typeStr = MESON_FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_SDR];
    } else {
        typeStr = MESON_FORCE_MODE_TYPE[MESON_HDR_FORCE_MODE_INVALID];
    }
    return typeStr;
}

const char *meson_modePolicyToString(int32_t type) {
    const char * typeStr;
    switch (type) {
        case MESON_POLICY_BEST:
            typeStr = "best";
            break;
        case MESON_POLICY_RESOLUTION:
            typeStr = "resolution";
            break;
        case MESON_POLICY_FRAMERATE:
            typeStr = "framerate";
            break;
        case MESON_POLICY_DV:
            typeStr = "dv policy";
            break;
        default:
            typeStr = "INVALID";
            break;
    }
    return typeStr;
}

/*
 * @param enable         [in] enable test mode or not
 *
 */
void meson_mode_set_test_mode(const bool enable) {
    meson_mode_set_test_mode_enable(enable);
}
#endif
