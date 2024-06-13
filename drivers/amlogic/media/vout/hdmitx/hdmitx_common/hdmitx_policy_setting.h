/*
 * Copyright (c) 2022 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef HDMI_POLICY_SETTING_H
#define HDMI_POLICY_SETTING_H

#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#else
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#endif
#include "mode_policy.h"

bool hdmi_sink_disp_mode_sup(struct hdmitx_dev *hdev, const char *disp_mode);
void get_hdmi_input(struct hdmitx_dev *hdev, struct meson_policy_in *input);
int hdmitx_get_policy_output(struct meson_policy_out *output);
void get_hdmi_input(struct hdmitx_dev *hdev, struct meson_policy_in *input);
void hdmitx_set_mode_policy(void);

#endif

