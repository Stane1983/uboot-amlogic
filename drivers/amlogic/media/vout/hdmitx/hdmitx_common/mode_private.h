
/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#ifndef MESON_DISPLAY_MODE_PRIVATE_H
#define MESON_DISPLAY_MODE_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * RX support deep color
 */
#define DISPLAY_HDMI_DEEP_COLOR         "/sys/class/amhdmitx/amhdmitx0/dc_cap"
/*
 * testing if tv support this displaymode and deepcolor combination
 * then if cat result is 1: support, 0: not
 */
#define DISPLAY_HDMI_VALID_MODE         "/sys/class/amhdmitx/amhdmitx0/valid_mode"

/*
 * default value
 */
#define MESON_DEFAULT_COLOR_FORMAT_4K       "420,8bit"
#define MESON_DEFAULT_COLOR_FORMAT          "rgb,8bit"
#define MESON_DEFAULT_HDMI_MODE             "720p60hz"

/*
 * check high frame rate support dv or not
 */
#define DV_VSVDB_PARITY                 "Parity: 1"

/*
 * default dv mode value
 */
#define DOLBY_VISION_LL_RGB             3
#define DOLBY_VISION_LL_YUV             2
#define DOLBY_VISION_STD_ENABLE         1
#define DOLBY_VISION_DISABLE            0

/*
 * define mode name
 */
#define MODE_480I                       "480i60hz"
#define MODE_480P                       "480p60hz"
#define MODE_640x480P                   "640x480p60hz"
#define MODE_576I                       "576i50hz"
#define MODE_576P                       "576p50hz"
#define MODE_720P24HZ                   "720p24hz"
#define MODE_720P25HZ                   "720p25hz"
#define MODE_720P30HZ                   "720p30hz"
#define MODE_720P48HZ                   "720p48hz"
#define MODE_720P50HZ                   "720p50hz"
#define MODE_720P                       "720p60hz"
#define MODE_720P100HZ                  "1280x720p100hz"
#define MODE_720P120HZ                  "1280x720p120hz"
#define MODE_1080P24HZ                  "1080p24hz"
#define MODE_1080P25HZ                  "1080p25hz"
#define MODE_1080P30HZ                  "1080p30hz"
#define MODE_1080P48HZ                  "1080p48hz"
#define MODE_1080I50HZ                  "1080i50hz"
#define MODE_1080P50HZ                  "1080p50hz"
#define MODE_1080I                      "1080i60hz"
#define MODE_1080P                      "1080p60hz"
#define MODE_1080P100HZ                 "1920x1080p100hz"
#define MODE_1080P120HZ                 "1920x1080p120hz"
#define MODE_1440P50HZ                  "2560x1440p50hz"
#define MODE_1440P60HZ                  "2560x1440p60hz"
#define MODE_1440P100HZ                 "2560x1440p100hz"
#define MODE_1440P120HZ                 "2560x1440p120hz"
#define MODE_4K2K24HZ                   "2160p24hz"
#define MODE_4K2K25HZ                   "2160p25hz"
#define MODE_4K2K30HZ                   "2160p30hz"
#define MODE_4K2K48HZ                   "2160p48hz"
#define MODE_4K2K50HZ                   "2160p50hz"
#define MODE_4K2K60HZ                   "2160p60hz"
#define MODE_4K2K100HZ                  "3840x2160p100hz"
#define MODE_4K2K120HZ                  "3840x2160p120hz"
#define MODE_4K2KSMPTE24HZ              "smpte24hz"
#define MODE_4K2KSMPTE30HZ              "smpte30hz"
#define MODE_4K2KSMPTE50HZ              "smpte50hz"
#define MODE_4K2KSMPTE60HZ              "smpte60hz"
#define MODE_4K2KSMPTE100HZ             "smpte100hz"
#define MODE_4K2KSMPTE120HZ             "smpte120hz"
#define MODE_8K4K24HZ                   "7680x4320p24hz"
#define MODE_8K4K25HZ                   "7680x4320p25hz"
#define MODE_8K4K30HZ                   "7680x4320p30hz"
#define MODE_8K4K48HZ                   "7680x4320p48hz"
#define MODE_8K4K50HZ                   "7680x4320p50hz"
#define MODE_8K4K60HZ                   "7680x4320p60hz"
/*
 * lcd mode
 */
#define MODE_PANEL                      "panel"
#define MODE_768P                       "768p60hz"

/*
 * cvbs mode
 */
#define MODE_480CVBS                    "480cvbs"
#define MODE_576CVBS                    "576cvbs"
#define MODE_PAL_M                      "pal_m"
#define MODE_PAL_N                      "pal_n"
#define MODE_NTSC_M                     "ntsc_m"

/*
 * define color format name
 */
#define COLOR_YCBCR444_12BIT             "444,12bit"
#define COLOR_YCBCR444_10BIT             "444,10bit"
#define COLOR_YCBCR444_8BIT              "444,8bit"
#define COLOR_YCBCR422_12BIT             "422,12bit"
#define COLOR_YCBCR422_10BIT             "422,10bit"
#define COLOR_YCBCR422_8BIT              "422,8bit"
#define COLOR_YCBCR420_12BIT             "420,12bit"
#define COLOR_YCBCR420_10BIT             "420,10bit"
#define COLOR_YCBCR420_8BIT              "420,8bit"
#define COLOR_RGB_12BIT                  "rgb,12bit"
#define COLOR_RGB_10BIT                  "rgb,10bit"
#define COLOR_RGB_8BIT                   "rgb,8bit"

/*
 * define mode list
 * must sort by resolution
 */
static const char* DISPLAY_MODE_LIST[] = {
    MODE_8K4K60HZ,
    MODE_8K4K50HZ,
    MODE_8K4K48HZ,
    MODE_8K4K30HZ,
    MODE_8K4K25HZ,
    MODE_8K4K24HZ,
    MODE_4K2KSMPTE120HZ,
    MODE_4K2KSMPTE100HZ,
    MODE_4K2KSMPTE60HZ,
    MODE_4K2KSMPTE50HZ,
    MODE_4K2KSMPTE30HZ,
    MODE_4K2KSMPTE24HZ,
    MODE_4K2K120HZ,
    MODE_4K2K100HZ,
    MODE_4K2K60HZ,
    MODE_4K2K50HZ,
    MODE_4K2K48HZ,
    MODE_4K2K30HZ,
    MODE_4K2K25HZ,
    MODE_4K2K24HZ,
    MODE_1440P120HZ,
    MODE_1440P100HZ,
    MODE_1440P60HZ,
    MODE_1440P50HZ,
    MODE_1080P120HZ,
    MODE_1080P100HZ,
    MODE_1080P,
    MODE_1080I,
    MODE_1080P50HZ,
    MODE_1080I50HZ,
    MODE_1080P48HZ,
    MODE_1080P30HZ,
    MODE_1080P25HZ,
    MODE_1080P24HZ,
    MODE_720P120HZ,
    MODE_720P100HZ,
    MODE_720P,
    MODE_720P50HZ,
    MODE_720P48HZ,
    MODE_720P30HZ,
    MODE_720P25HZ,
    MODE_720P24HZ,
    MODE_576P,
    MODE_480P,
    MODE_640x480P,
    MODE_576I,
    MODE_480I,
    /*
     * non hdmi solution
     */
    MODE_768P,
    MODE_PANEL,
    MODE_480CVBS,
    MODE_576CVBS,
    MODE_PAL_M,
    MODE_PAL_N,
    MODE_NTSC_M,
};

/*
 * mode for resolution priority
 * for HDR/SDR policy
 */
static const char* MODE_RESOLUTION_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_1080I50HZ,
    MODE_1080I,
    MODE_640x480P,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
    /*
     * for hdmi compatibility not choose 8k as preferred mode
     */
    /*
    MODE_4K2K100HZ,
    MODE_4K2K120HZ,
    MODE_8K4K24HZ,
    MODE_8K4K25HZ,
    MODE_8K4K30HZ,
    MODE_8K4K48HZ,
    MODE_8K4K50HZ,
    MODE_8K4K60HZ,
    */
};

/*
 * mode for frame rate priority
 * for HDR/SDR policy
 */
static const char* MODE_FRAMERATE_FIRST[] = {
    MODE_480I,
    MODE_576I,
    MODE_1080I50HZ,
    MODE_1080I,
    MODE_640x480P,
    MODE_480P,
    MODE_576P,
    MODE_720P50HZ,
    MODE_720P,
    MODE_4K2K24HZ,
    MODE_4K2K25HZ,
    MODE_4K2K30HZ,
    MODE_1080P50HZ,
    MODE_1080P,
    MODE_4K2K50HZ,
    MODE_4K2K60HZ,
    /*
     * for hdmi compatibility not choose 8k as preferred mode
     */
    /*
    MODE_4K2K100HZ,
    MODE_4K2K120HZ,
    MODE_8K4K24HZ,
    MODE_8K4K25HZ,
    MODE_8K4K30HZ,
    MODE_8K4K48HZ,
    MODE_8K4K50HZ,
    MODE_8K4K60HZ,
    */
};

/*
 * hdr 4k support or not
 */
static const char* MODE_4K_LIST[] = {
    MODE_4K2K60HZ,
    MODE_4K2K50HZ,
};

/*
 * hdr non-4k support or not
 */
static const char* MODE_NON4K_LIST[] = {
    MODE_1080P,
    MODE_1080P50HZ,
    MODE_720P,
    MODE_720P50HZ,
    MODE_576P,
    MODE_480P,
    MODE_1080I,
    MODE_1080I50HZ,
    MODE_576I,
    MODE_480I,
};

/*
 * this is prior selected list for sdr of 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
 * for user change resolution case
 */
static const char* COLOR_ATTRIBUTE_LIST1[] = {
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
};

/*
 * this is prior selected list for hdr and sdr  of non 4k display mode
 * for user change resolution case
 */
static const char* COLOR_ATTRIBUTE_LIST2[] = {
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
};

//this is prior selected list for sdr  of non 4k display mode
static const char* SDR_NON4K_COLOR_ATTRIBUTE_LIST[] = {
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_RGB_10BIT,
};

/*
 * this is prior selected list  of Low Power Mode 4k2k50hz, 4k2k60hz smpte50hz, smpte60hz
 */
static const char* COLOR_ATTRIBUTE_LIST3[] = {
    COLOR_YCBCR420_8BIT,
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_YCBCR444_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR420_12BIT,
    COLOR_YCBCR422_12BIT,
};

/*
 * this is prior selected list of Low Power Mode other display mode
 */
static const char* COLOR_ATTRIBUTE_LIST4[] = {
    COLOR_YCBCR444_8BIT,
    COLOR_YCBCR422_8BIT,
    COLOR_RGB_8BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_YCBCR422_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_12BIT,
    COLOR_YCBCR422_12BIT,
    COLOR_RGB_12BIT,
};

/*
 * this is prior selected list of HDR non 4k colorspace
 */
static const char* HDR_NON4K_COLOR_ATTRIBUTE_LIST[] = {
    COLOR_YCBCR422_12BIT,
    COLOR_YCBCR444_10BIT,
    COLOR_RGB_10BIT,
    COLOR_YCBCR444_12BIT,
    COLOR_RGB_12BIT,
};

/*
 * this is prior selected list of HDR 4k colorspace(2160p60hz/2160p50hz)
 */
static const char* HDR_4K_COLOR_ATTRIBUTE_LIST[] = {
    COLOR_YCBCR420_10BIT,
    COLOR_YCBCR422_12BIT,
};

#ifdef __cplusplus
}
#endif

#endif // MESON_DISPLAY_MODE_PRIVATE_H
