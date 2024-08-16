/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef MESON_MODE_POLICY_PARSER_H
#define MESON_MODE_POLICY_PARSER_H

#include "mode_policy.h"

bool support_DV_RGB_444_8BIT(struct meson_hdr_info *info);
bool support_LL_YCbCr_422_12BIT(struct meson_hdr_info *info);
bool support_LL_RGB_444_10BIT(struct meson_hdr_info *info);
bool support_LL_RGB_444_12BIT(struct meson_hdr_info *info);
bool support_DV_VSVDB_PARITY(struct meson_hdr_info *info);
bool is_support_hdmimode(struct meson_policy_in *input, const char* mode);
bool is_support_color_format(struct meson_policy_in *input, const char* color_format);
bool is_hdmi_edid_parserok(struct meson_policy_in *input);
bool is_hdmi_dc_cap_ok(struct meson_policy_in *input);
bool mode_support_check(const char *mode, const char * color, struct meson_policy_in *input);
bool find_brr_mode(const char *mode, struct meson_policy_in *input, char* outputmode);
/*
 * for unit test mode
 */
void meson_mode_set_test_mode_enable(const bool enable);

#ifndef __UBOOT__
/*
 * only android and linux need api
 */

#else
/*
 * only uboot need api
 */

#endif

#endif

