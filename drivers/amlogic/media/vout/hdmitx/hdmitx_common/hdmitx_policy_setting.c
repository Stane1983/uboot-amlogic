/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <command.h>
#include <env.h>
#include <malloc.h>
#include <asm/byteorder.h>
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#else
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include "hdmitx_check_valid.h"
#endif
#include <amlogic/media/dv/dolby_vision.h>
#include <cpu.h>
#include <amlogic/cpu_id.h>
#include "mode_policy.h"

static bool is_best_policy(void) {
	char *hdmimode = env_get("hdmimode");
	char *is_bestmode = env_get("is.bestmode");

	if (!hdmimode || !strstr(hdmimode, "hz")) {
		//if hdmimode is empty or no resolution is saved to enable the auto to best strategy
		return true;
	}
	return !is_bestmode || (strcmp(is_bestmode, "true") == 0);
}

static bool is_best_color_space(void)
{
	char *user_cs = env_get("user_colorattribute");

	if (!user_cs)
		return true;
	else if (!strstr(user_cs, "bit"))
		return true;

	return false;
}

/* decide output dolby status by uboot dolby vision driver */
/* uboot hdmi driver will update this value follow below policy,
 * 1.hdr_policy = always
 * dv type = sink-led -> dolby_status = 1
 * dv type = source-led -> dolby_status = 2
 * dv type = dolby disable -> dolby_status = 0
 * 2.hdr_policy = adaptive -> dolby_status = 0

 * systemcontrol save current dv output status
 * dolby_status = 0 -> dolby vision disable
 * dolby_status = 1 -> std mode(sink-led)
 * dolby_status = 2 -> LL YUV(source-led)
 * dolby_status = 3 -> LL RGB
 */
int get_ubootenv_dv_status(void)
{
	char *dolby_status = NULL;

	dolby_status = env_get("dolby_status");

	if (!dolby_status) {
		printf("no ubootenv dolby_status\n");
		return DOLBY_VISION_DISABLE;
	}
	if (!strcmp(dolby_status, DOLBY_VISION_SET_STD))
		return DOLBY_VISION_STD_ENABLE;
	else if (!strcmp(dolby_status, DOLBY_VISION_SET_LL_YUV))
		return DOLBY_VISION_LL_YUV;
	else if (!strcmp(dolby_status, DOLBY_VISION_SET_LL_RGB))
		return DOLBY_VISION_LL_RGB;
	else
		return DOLBY_VISION_DISABLE;
}

/* user_prefer_dv_type is used to save user select dolby vision
 * output mode. Note: if the user has not set it, it will be empty or "none".
 * amdv_type = 0 ->dolby vision disable
 * amdv_type = 1 ->std mode(sink-led)
 * amdv_type = 2 ->LL YUV(source-led)
 * amdv_type = 3 ->LL RGB
 */
int get_ubootenv_dv_type(void)
{
	char *amdv_type = NULL;

	amdv_type = env_get("user_prefer_dv_type");

	if (!amdv_type) {
		printf("no ubootenv user_prefer_dv_type\n");
		return AMDV_NONE;
	}
	if (!strcmp(amdv_type, "none"))
		return AMDV_NONE;
	else if (!strcmp(amdv_type, DOLBY_VISION_SET_STD))
		return DOLBY_VISION_STD_ENABLE;
	else if (!strcmp(amdv_type, DOLBY_VISION_SET_LL_YUV))
		return DOLBY_VISION_LL_YUV;
	else if (!strcmp(amdv_type, DOLBY_VISION_SET_LL_RGB))
		return DOLBY_VISION_LL_RGB;
	else
		return DOLBY_VISION_DISABLE;
}

bool is_amdolby_enabled(void)
{
	if (get_ubootenv_dv_status() != DOLBY_VISION_DISABLE)
		return true;
	else
		return false;
}

bool is_tv_support_hdr(struct hdmitx_dev *hdev)
{
	struct hdr_info *hdr;
	struct hdr10_plus_info *hdr10p;

	if (!hdev)
		return false;
	hdr = &hdev->RXCap.hdr_info;
	hdr10p = &hdev->RXCap.hdr_info.hdr10plus_info;
	if (hdr->hdr_support & HDR_SUP_EOTF_SMPTE_ST_2084 || hdr->hdr_support & HDR_SUP_EOTF_HLG)
		return true;
	if (hdr10p->ieeeoui == HDR10_PLUS_IEEE_OUI &&
		hdr10p->application_version != 0xFF)
		return true;
	return false;
}

bool is_tv_support_dv(struct hdmitx_dev *hdev)
{
	/*todo*/
	struct dv_info *dv;

	if (!hdev)
		return false;
	dv = &(hdev->RXCap.dv_info);

	if ((dv->ieeeoui != DV_IEEE_OUI) || (dv->block_flag != CORRECT))
		return false;
	return true;
}

/* Hdr Resolution Priority enable or not, false:disable true:enable
 * note that the ubootenv name may be confused. the actual meaning is:
 * when connected to HDR TV which only support 4K60hz 420_8bit maximum,
 * if this ubootenv is true/null, then it will select 1080p deep_color
 * (thus to output HDR) as netflix request;
 * if this ubootenv is false, then it will select 4k with 8bit(SDR)
 * for special project usage.
 */
static bool is_hdr_resolution_priority(void)
{
	char *hdr_resolution_priority = env_get("hdr_resolution_priority");

	return !hdr_resolution_priority || (strcmp(hdr_resolution_priority, "true") == 0);
}

/* for application, the actually hdr_priority may be 0x10000030
 * and the string hdr_priority will be like 268435504
 * so here needs coverting such string to hex value
 */
int get_hdr_strategy_priority(void)
{
	unsigned int hdr_strategy_priority = 0;

	hdr_strategy_priority = env_get_ulong("hdr_priority", 10, ~0UL);
	return (int)hdr_strategy_priority;
}

int get_hdr_priority(void)
{
	unsigned int hdr_priority = get_hdr_strategy_priority();
	hdr_priority_e value = DOLBY_VISION_PRIORITY;

	if (hdr_priority != -1) {
		if (((hdr_priority >> 28) & 0xf) == 0) {
			unsigned int strategy1 = hdr_priority & 0xf;

			if (strategy1 == 2)
				value = SDR_PRIORITY;
			else if (strategy1 == 1)
				value = HDR10_PRIORITY;
			else
				value = DOLBY_VISION_PRIORITY;
		}
	} else {
		value = DOLBY_VISION_PRIORITY;
	}

	return (int)value;
}

int get_hdr_policy(void)
{
	char *hdr_policy = env_get("hdr_policy");
	hdr_policy_e value = HDR_POLICY_SINK;

	if (hdr_policy) {
		if (strcmp(hdr_policy, "0") == 0)
			value = HDR_POLICY_SINK;
		else if (strcmp(hdr_policy, "1") == 0)
			value = HDR_POLICY_SOURCE;
		else if (strcmp(hdr_policy, "4") == 0)
			value = HDR_POLICY_FORCE;
		else
			printf("error ubootenv value of hdr_policy\n");
	} else {
		value = HDR_POLICY_SINK;
	}

	return (int)value;
}

static int get_hdr_force_mode(void)
{
	char *hdr_force_mode = env_get("hdr_force_mode");
	int force_mode = 0;

	if (hdr_force_mode)
		force_mode = ustrtoul(hdr_force_mode, NULL, 10);
	else
		force_mode = MESON_HDR_FORCE_MODE_DV;

	return force_mode;
}

static bool is_low_powermode(void)
{
	return false;
}

#define DV_MODE_4K2K30HZ                "2160p30hz"
#define DV_MODE_4K2K60HZ                "2160p60hz"
static void get_amdv_max_mode(struct hdmitx_dev *hdev, char *amdv_max_mode)
{
	if (!hdev || !amdv_max_mode)
		return;
	if (hdev->RXCap.dv_info.sup_2160p60hz == 1)
		strcpy(amdv_max_mode, DV_MODE_4K2K60HZ);
	else
		strcpy(amdv_max_mode, DV_MODE_4K2K30HZ);
	return;
}

static bool is_support_deepcolor(void)
{
	return true;
}

static bool is_framerate_priority(void)
{
	char *framerate_priority = env_get("framerate_priority");

	return !framerate_priority || (strcmp(framerate_priority, "true") == 0);
}

/* import from kernel */
static inline bool package_id_is(unsigned int id)
{
	return get_cpu_id().package_id == id;
}

static inline bool is_meson_gxl_cpu(void)
{
	return get_cpu_id().family_id == MESON_CPU_MAJOR_ID_GXL;
}

static inline bool is_meson_s1a_cpu(void)
{
	return get_cpu_id().family_id == MESON_CPU_MAJOR_ID_S1A;
}

static inline bool is_meson_gxl_package_805X(void)
{
	return is_meson_gxl_cpu() && package_id_is(0x30);
}

static inline bool is_meson_gxl_package_805Y(void)
{
	return is_meson_gxl_cpu() && package_id_is(0xb0);
}

static inline bool is_meson_s1a_package_805C1(void)
{
	return is_meson_s1a_cpu();
}

/* below items has feature limited, may need extra judgement */
bool is_hdmitx_limited_1080p(void)
{
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdev = get_hdmitx21_device();
#endif

	if (is_meson_gxl_package_805X())
		return true;
	else if (is_meson_gxl_package_805Y())
		return true;
	else if (is_meson_s1a_package_805C1())
		return true;
	else if (hdev->tx_common.res_1080p == 1)
		return true;
	else
		return false;
}

bool is_support_4k(void)
{
	if (is_hdmitx_limited_1080p())
		return false;
	return true;
}

/* for some non-std TV, it declare 4k while MAX_TMDS_CLK
 * not match 4K format, so filter out mode list by
 * check if basic color space/depth is supported
 * or not under this resolution
 * note that disp_mode should not contain colorspace, such as 420
 */
bool hdmi_sink_disp_mode_sup(struct hdmitx_dev *hdev, const char *disp_mode)
{
	enum hdmi_vic vic = HDMI_0_UNKNOWN;

	if (!hdev || !disp_mode)
		return false;

	vic = hdmitx_edid_vic_tab_map_vic(disp_mode);

	if (hdmitx_mode_validate_y420_vic(vic)) {
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "420,8bit"))
			return true;
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "rgb,8bit"))
			return true;
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "444,8bit"))
			return true;
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "422,12bit"))
			return true;
	} else {
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "rgb,8bit"))
			return true;
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "444,8bit"))
			return true;
		if (hdmitx_chk_mode_attr_sup(hdev, disp_mode, "422,12bit"))
			return true;
	}
	return false;
}

void get_hdmi_input(struct hdmitx_dev *hdev, struct meson_policy_in *input)
{
	struct dv_info *dv_info = &hdev->RXCap.dv_info;
	char *amdv_type = NULL;
	char *hdmimode = NULL;
	char *colorattribute = NULL;

	if (!hdev || !input)
		return;

	input->state = MESON_SCENE_STATE_INIT;
	input->con_info.is_bestcolorspace = is_best_color_space();
	input->con_info.is_support4k = is_support_4k();
	input->con_info.is_deepcolor = is_support_deepcolor();
	input->con_info.isframeratepriority = is_framerate_priority();
	input->con_info.sink_type = MESON_SINK_TYPE_SINK;

	/* 1."hdmimode" is used to save user manually selected mode,
	 * note that if auto best mode is on, it means no user manual
	 * operation. then this env will be default "none" or NULL
	 * 2."user_colorattribute" is used to save user manually
	 * selected color space/depth.
	 * while
	 * 3."outputmode" is used to save the actual output hdmi mode
	 * 4."colorattribute" is used to save the actual output color space/depth
	 */
	hdmimode = env_get("hdmimode");
	colorattribute = env_get("user_colorattribute");
	/* the default value here is just an init value in
	 * case the env is null. if it's null/none, it will
	 * select the auto best mode/color by policy
	 */
	if (!hdmimode || !strcmp(hdmimode, "none"))
		hdmimode = DEFAULT_HDMI_MODE;
	if (!colorattribute || !strcmp(colorattribute, "none"))
		colorattribute = DEFAULT_COLOR_FORMAT;
	strcpy(input->con_info.ubootenv_hdmimode, hdmimode);
	strcpy(input->con_info.ubootenv_colorattr, colorattribute);
	strcpy(input->cur_displaymode, hdmimode);

	/* hdr input info */
	input->hdr_info.is_amdv_enable = is_amdolby_enabled();
	input->hdr_info.is_tv_supportDv = is_tv_support_dv(hdev);
	input->hdr_info.is_tv_supportHDR = is_tv_support_hdr(hdev);
	input->hdr_info.is_hdr_resolution_priority = is_hdr_resolution_priority();
	input->hdr_info.is_lowpower_mode = is_low_powermode();
	input->hdr_info.hdr_priority = get_hdr_strategy_priority();
	input->hdr_info.hdr_policy = get_hdr_policy();
	input->hdr_info.hdr_force_mode = get_hdr_force_mode();
	input->hdr_info.support_DV_RGB_444_8BIT = dv_info->support_DV_RGB_444_8BIT;
	input->hdr_info.support_LL_YCbCr_422_12BIT = dv_info->support_LL_YCbCr_422_12BIT;
	input->hdr_info.support_LL_RGB_444_10BIT = dv_info->support_LL_RGB_444_10BIT;
	input->hdr_info.support_LL_RGB_444_12BIT = dv_info->support_LL_RGB_444_12BIT;
	input->hdr_info.sup_2160p60hz = dv_info->sup_2160p60hz;
	amdv_type = env_get("user_prefer_dv_type");
	if (amdv_type)
		strlcpy(input->hdr_info.ubootenv_dv_type, amdv_type, sizeof(input->hdr_info.ubootenv_dv_type));
	get_amdv_max_mode(hdev, input->hdr_info.dv_max_mode);
	input->hdr_info.amdv_parity = dv_info->parity;

	printf("ubootenv user_prefer_dv_type: %s, dv_sts:%d, hdr_priority: %x, hdr_policy: %d\n",
		input->hdr_info.ubootenv_dv_type,
		get_ubootenv_dv_status(),
		input->hdr_info.hdr_priority,
		input->hdr_info.hdr_policy);
	printf("ubootenv best_policy: %d, best_color: %d, framerate_priority:%d, hdr_force_mode:%d\n",
		is_best_policy(),
		input->con_info.is_bestcolorspace,
		is_framerate_priority(),
		input->hdr_info.hdr_force_mode);

	meson_mode_set_policy_input(MESON_MODE_HDMI, input);
}

void hdmitx_set_mode_policy(void)
{
	enum meson_mode_policy policy = MESON_POLICY_INVALID;

	if (is_best_policy())
		policy = MESON_POLICY_BEST;
	meson_mode_set_policy(MESON_MODE_HDMI, policy);
}

int hdmitx_get_policy_output(struct meson_policy_out *output) {

	return meson_mode_get_policy_output(MESON_MODE_HDMI, output);
}

