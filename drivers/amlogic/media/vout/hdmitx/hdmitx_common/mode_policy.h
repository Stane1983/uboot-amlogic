/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include "mode_util.h"
#ifndef __UBOOT__
#include "mode_private.h"
#endif

#ifndef MESON_DISPLAY_MODE_POLICY_H
#define MESON_DISPLAY_MODE_POLICY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum meson_mode_policy {
    MESON_POLICY_BEST       = 0,           /* mode auto to best policy is enable */
    MESON_POLICY_RESOLUTION = 1,           /* mode preferred resolution for hdr and sdr when auto to best policy is enable */
    MESON_POLICY_FRAMERATE  = 2,           /* mode preferred frame rate for hdr and sdr when auto to best policy is enable */
    MESON_POLICY_DV = 3,
    MESON_POLICY_INVALID    = 8,           /* mode auto to best policy is disable */
} meson_mode_policy_e;

typedef enum meson_mode_state {
    MESON_SCENE_STATE_INIT = 0,             /* boot */
    MESON_SCENE_STATE_POWER = 1,            /* hot plug or suspend/resume */
    MESON_SCENE_STATE_SWITCH = 2,           /* user switch the mode */
    MESON_SCENE_STATE_SWITCH_ADAPTER = 3,   /* video auto switch the mode */
    MESON_SCENE_STATE_RESERVE = 4,
    MESON_SCENE_STATE_ADAPTER_END = 5,      /* end hint video auto switch the mode */
} meson_mode_state_e;

typedef enum meson_sink_type {
    MESON_SINK_TYPE_NONE     = 0,            /* hdmi plug out,use cvbs */
    MESON_SINK_TYPE_SINK     = 1,            /* hdmi sink */
    MESON_SINK_TYPE_REPEATER = 2,            /* repeater sink */
    MESON_SINK_TYPE_RESERVE  = 3
} meson_sink_type_e;

/*
 * bit0-bit3 for hdr strategy1
 * 0 → original cap
 * 1 → disable dolby vision cap
 * 2 → disable dolby vision  and hdr cap
 * bit4-bit7 for hdr strategy2
 * bit4: 1 → disable dv, 0 → enable dv
 * bit5: 1 → disable hdr10/hdr10+, 0 → enable hdr10/hdr10+
 * bit6: 1 → disable hlg,  0 → enable hlg
 * bit7-bit27:reverse
 * bit28-bit31 choose strategy
 * 0：strategy1
 * 1：strategy2
*/
typedef enum meson_hdr_priority {
    MESON_DOLBY_VISION_PRIORITY = 0x0,
    MESON_HDR10_PRIORITY        = 0x1,
    MESON_SDR_PRIORITY          = 0x2,
    MESON_G_DV_HDR10_HLG        = 0x10000000,
    MESON_G_DV_HDR10            = 0x10000040,
    MESON_G_DV_HLG              = 0x10000020,
    MESON_G_HDR10_HLG           = 0x10000010,
    MESON_G_DV                  = 0x10000060,
    MESON_G_HDR10               = 0x10000050,
    MESON_G_HLG                 = 0x10000030,
    MESON_G_SDR                 = 0x10000070,
} meson_hdr_priority_e;

typedef enum meson_hdr_policy {
    MESON_HDR_POLICY_SINK   = 0,
    MESON_HDR_POLICY_SOURCE = 1,
    MESON_HDR_POLICY_FORCE  = 4,
} meson_hdr_policy_e;

typedef enum meson_hdr_preferred_policy {
    MESON_HDR_SYSTEM_PREFERRED = 0,
    MESON_HDR_MATCH_CONTENT    = 1,
    MESON_HDR_FORCE            = 2,
} meson_hdr_preferred_policy_e;

typedef enum meson_hdr_force_mode {
    MESON_HDR_FORCE_MODE_INVALID    = 0,
    MESON_HDR_FORCE_MODE_SDR        = 1,
    MESON_HDR_FORCE_MODE_DV         = 2,
    MESON_HDR_FORCE_MODE_HDR10      = 3,
    MESON_HDR_FORCE_MODE_HDR10PLUS  = 4,  //need to do
    MESON_HDR_FORCE_MODE_HLG        = 5,
} meson_hdr_force_mode_e;

typedef enum meson_connector_type {
    MESON_MODE_HDMI = 0,
    MESON_MODE_PANEL = 1,
    MESON_MODE_LVDS = 2,
    MESON_MODE_TV = 3,
    MESON_MODE_CON_MAX = 4,
} meson_connector_type_e;

typedef struct meson_mode_info {
    char name[MESON_MODE_LEN];
    uint32_t dpi_x, dpi_y;
    uint32_t pixel_w, pixel_h;
    float refresh_rate;
    int32_t group_id;
} meson_mode_info_t;

typedef struct meson_hdr_info {
    bool is_amdv_enable;                    /* dolby vision enable or not */
    bool is_tv_supportHDR;                  /* tv is support HDR or not */
    bool is_tv_supportDv;                   /* tv is support dolby vision or not*/
    bool is_hdr_resolution_priority;        /* Hdr Resolution Priority enable or not */
    bool is_lowpower_mode;                  /* low power feature enable or not */
    char ubootenv_dv_type[MESON_MODE_LEN];      /* uboot env of dolby vision type */
    char dv_cap[MESON_MAX_STR_LEN];         /* tv dolby vision cap */
    char dv_max_mode[MESON_MODE_LEN];            /* tv dolby vision max support resolution */
    char dv_deepcolor[MESON_DV_MODE_LEN];   /* tv dolby vision deepcolor */
    meson_hdr_priority_e hdr_priority;      /* dynamic range fromat preference,0:dolby vision,1:hdr,2:sdr */
    meson_hdr_policy_e hdr_policy;          /* dynamic range policy,0 :follow sink, 1: match content, 2:hdr force mode */
    meson_hdr_force_mode_e hdr_force_mode;  /* hdr force mode,1 :force sdr, 2: force dv, 3: force hdr10, 5:force hlg*/
    /*
     *below members used by uboot
     */
    bool support_DV_RGB_444_8BIT;
    bool support_LL_YCbCr_422_12BIT;
    bool support_LL_RGB_444_10BIT;
    bool support_LL_RGB_444_12BIT;
    bool sup_2160p60hz;                     /* if as 0, then support 2160p30hz */
    char amdv_parity;
} meson_hdr_info_t;

typedef struct meson_connector_info {
    bool is_bestcolorspace;                 /* hdmi best colorspace,false:disable true:enable */
    bool is_support4k;                      /* soc support 4k or not */
    bool is_support4k30HZ;                  /* soc max support 4k30hz or not */
    bool is_deepcolor;                      /* deepcolor feature enable or not */
    bool isframeratepriority;               /* frame priority feature enable or not, false:disable true:enable */
    enum meson_sink_type sink_type;         /* 0: not hdmi sink; 1: hdmi sink; 2: repeater sink; */
    char edid_parsing[MESON_MODE_LEN];      /* edid parse ok or not, ok:parse ok,ng:parse ng */
    char dc_cap[MESON_MAX_STR_LEN];         /* device colorspace cap */
//    char disp_cap[MESON_MAX_STR_LEN];       /* device resolution cap */
    char ubootenv_cvbsmode[MESON_MODE_LEN];     /* the env of cvbsmode */
    char ubootenv_hdmimode[MESON_MODE_LEN];     /* the env of hdmimode */
    char ubootenv_colorattr[MESON_MODE_LEN];    /* the env of colospace for UI change colorspace or boot */

    /* TODO: refactor disp cap */
    meson_mode_info_t *modes;               /* for display modes */
    int32_t modes_size;                     /* the number of modes */
    int32_t modes_capacity;                 /* the memory capacity */
} meson_connector_info_t;

typedef struct meson_policy_in {
    char cur_displaymode[MESON_MODE_LEN];   /* hdmi current output mode */
    meson_mode_state_e state;               /* scene state */
    meson_hdr_info_t hdr_info;              /* hdr/dv info */
    meson_connector_info_t con_info;        /* connector info */
} meson_policy_in_t;

typedef struct meson_policy_out {
    char displaymode[MESON_MODE_LEN];
    char deepcolor[MESON_MODE_LEN];
    int32_t amdv_type;
} meson_policy_out_t;

struct meson_policy {
    struct meson_policy_in input;
    struct meson_policy_out output;
    enum meson_mode_policy policy;
};

#ifndef __UBOOT__

/*
 * below data struct is uesd only by hwc and linux
 */

/*
 * save user preferred hdr policy
 * "0":System-preferred conversion
 * "1":Match content Dynamic range
 * "2":Force conversion
 */
static const char* MESON_HDR_PREFERRED_POLICY[] = {
    "0",
    "1",
    "2"
};

/*
 * save user hdr policy
 * "0":follow sink
 * "1":follow source
 * "2""3":hdr vivid
 * "4":force
 */
static const char* MESON_HDR_POLICY[] = {
    "0",
    "1",
    "2",
    "3",
    "4"
};

/* force mode type in uboot env
 * 0: invalid type
 * 1: force sdr
 * 2: force dv
 * 3: force hdr10
 * 4: force hdr10plus (need to do)
 * 5: force hlg
 * */
static const char* MESON_FORCE_MODE_TYPE[] = {
    "0",
    "1",
    "2",
    "3",
    "4",
    "5"
};
#endif
/*
 * Set the mode policy
 */
int32_t meson_mode_set_policy(int32_t connector, const meson_mode_policy_e policy);

/*
 * set mode policy input
 */
int32_t meson_mode_set_policy_input(int32_t connector, const struct meson_policy_in *input);

/*
 * get mode policy output
 */
int32_t meson_mode_get_policy_output(int32_t connector, struct meson_policy_out *out);

/*
 * check mode support or not under type
 */
int32_t meson_mode_support_mode(int32_t connector, int32_t type, const char *mode);

#ifndef __UBOOT__
/*
 * below api is uesd only by hwc and linux
 */

/*
 * get current mode support color
 */
int32_t meson_mode_get_support_color(int32_t connector, const char *mode, char* color);

/*
 * for parse dv mode type
 */
const char *meson_dvModeTypeToString(const char *dvmode);

/*
 * debug help function
 */
const char *meson_hdrPriorityToString(int32_t type);

const char *meson_hdrPolicyToString(int32_t type);

const char *meson_modePolicyToString(int32_t type);

/*
 * for uinit test mode
 */
void meson_mode_set_test_mode(const bool enable);

#endif

#ifdef __cplusplus
}
#endif

#endif /* MESON_DISPLAY_MODE_POLICY_H */
