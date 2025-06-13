// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/libfdt.h>
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#else
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#endif
#include <amlogic/media/vout/hdmitx_common.h>

int is_valid_hdmi(const char *input)
{
	static const char * const valid_hdmi_modes[] = {
			"HDMI-A-A", /* venc0 */
			"HDMI-A-B", /* venc1 */
			"HDMI-A-C"  /* venc2 */
	};

	int num_modes = ARRAY_SIZE(valid_hdmi_modes);
	int i;

	for (i = 0; i < num_modes; i++) {
		/* Found a match */
		if (strcmp(input, valid_hdmi_modes[i]) == 0)
			return 1;
	}
	/* No match found */
	return 0;
}

int hdmitx_likely_frac_rate_mode(char *m)
{
	if (strstr(m, "24hz") || strstr(m, "30hz") || strstr(m, "48hz") ||
		strstr(m, "60hz") || strstr(m, "120hz") || strstr(m, "240hz"))
		return 1;
	else
		return 0;
}

bool hdmitx_get_hpd_state_ext(void)
{
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdev = get_hdmitx21_device();
#endif

	return hdev->hpd_state;
}

/* process the 1080p59hz mode name */

struct hdmi_mode_name {
	char *integer_name;
	char *frac_name;
};

static const struct hdmi_mode_name names[] = {
	{"24hz", "23hz"},
	{"30hz", "29hz"},
	{"48hz", "47hz"},
	{"60hz", "59hz"},
	{"120hz", "119hz"},
	{"240hz", "239hz"},
};

bool is_mode_name_frac(const char *name)
{
	int i;

	if (!name)
		return false;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		if (strstr(name, names[i].frac_name))
			return true;
	}
	return false;
}

void convert_name_frac2int(const char *name, char *conv_name)
{
	int i;
	bool match = false;

	if (!name || !conv_name)
		return;

	for (i = 0; i < ARRAY_SIZE(names); i++) {
		if (strstr(name, names[i].frac_name)) {
			match = true;
			break;
		}
	}
	if (match) {
		char *tmp = strstr(name, names[i].frac_name);
		int len = tmp - name + 1;

		strlcpy(conv_name, name, len);
		strcat(conv_name, names[i].integer_name);
	}
}

/* kernel 6.12 or later, /amhdmitx/frac_enable should be set as default 1 */
static bool _get_kernel_dtb_info(void)
{
	char *dt_addr = NULL;
	int nodeoffset;
	char *propdata;

	dt_addr = (char *)env_get_ulong("dtb_mem_addr", 16, 0x1000000);
	if (!dt_addr)
		return 0;

	if (fdt_check_header(dt_addr) != 0)
		return 0;
	nodeoffset = fdt_path_offset(dt_addr, "/amhdmitx");
	if (nodeoffset < 0)
		return 0;
	propdata = (char *)fdt_getprop((const void *)dt_addr, nodeoffset, "frac_enable", NULL);
	if (propdata)
		return !!be32_to_cpup((u32 *)propdata);
	return 0;
}

bool get_kernel_dtb_info(void)
{
	bool ret = _get_kernel_dtb_info();

	printf("get kernel frac_enable as %d\n", ret);
	return ret;
}

/*
 * old processing of 1080p59 and 1080p60
 *                modename      frac_rate_policy
 * 1080p59.94hz   1080p60hz     1
 * 1080p60hz      1080p60hz     0
 *
 * new processing of 1080p59 and 1080p60
 *                modename      frac_rate_policy
 * 1080p59.94hz   1080p59hz     NULL
 * 1080p60hz      1080p60hz     NULL
 */
#define MAX_NAME_LENGTH 32
static char tmp_hdmimode[MAX_NAME_LENGTH];
static char tmp_outputmode[MAX_NAME_LENGTH];
static char tmp_outputmode2[MAX_NAME_LENGTH];
static char tmp_outputmode3[MAX_NAME_LENGTH];

void hdmi_mode_frac_preprocess(void)
{
	char *hdmimode = env_get("hdmimode");
	char *outputmode = env_get("outputmode");
	char *outputmode2 = env_get("outputmode2");
	char *outputmode3 = env_get("outputmode3");
	char conv_hdmimode[MAX_NAME_LENGTH] = {0};
	char conv_outputmode[MAX_NAME_LENGTH] = {0};
	char conv_outputmode2[MAX_NAME_LENGTH] = {0};
	char conv_outputmode3[MAX_NAME_LENGTH] = {0};

	/* kernel should have /amhdmitx/frac_enable node as 1; otherwise return */
	if (!get_kernel_dtb_info())
		return;

	/* if frac_rate_policy is NULL, or hdmimode/outputmode is frac name, backup the names */
	if ((hdmimode && is_mode_name_frac(hdmimode)) ||
		(outputmode && is_mode_name_frac(outputmode)) ||
		(outputmode2 && is_mode_name_frac(outputmode2)) ||
		(outputmode3 && is_mode_name_frac(outputmode3))) {
		if (hdmimode && is_mode_name_frac(hdmimode))
			strncpy(tmp_hdmimode, hdmimode, sizeof(tmp_hdmimode));

		/* The outputmode must be saved based on the value of connectorX_type */
		if (env_get("connector0_type") &&
		    is_valid_hdmi(env_get("connector0_type"))) {
			if (outputmode && is_mode_name_frac(outputmode))
				strncpy(tmp_outputmode, outputmode, sizeof(tmp_outputmode));
		} else if (env_get("connector1_type") &&
			   is_valid_hdmi(env_get("connector1_type"))) {
			if (outputmode2 && is_mode_name_frac(outputmode2))
				strncpy(tmp_outputmode2, outputmode2, sizeof(tmp_outputmode2));
		} else if (env_get("connector2_type") &&
			   is_valid_hdmi(env_get("connector2_type"))) {
			if (outputmode3 && is_mode_name_frac(outputmode3))
				strncpy(tmp_outputmode3, outputmode3, sizeof(tmp_outputmode3));
		} else {
			pr_info("no config connectorX_type, no check frac\n");
		}
	}

	if (!tmp_hdmimode[0] && !tmp_outputmode[0] && !tmp_outputmode2[0] &&
	    !tmp_outputmode3[0]) {
		/* no frac name and return*/
		return;
	}

	/* rename 59hz to 60hz for compatible old name */
	memset(conv_hdmimode, 0, sizeof(conv_hdmimode));
	memset(conv_outputmode, 0, sizeof(conv_outputmode));
	memset(conv_outputmode2, 0, sizeof(conv_outputmode2));
	memset(conv_outputmode3, 0, sizeof(conv_outputmode3));
	if (tmp_hdmimode[0]) {
		if (hdmimode && is_mode_name_frac(hdmimode)) {
			convert_name_frac2int(tmp_hdmimode, conv_hdmimode);
			env_set("hdmimode", conv_hdmimode);
		} else {
			env_set("hdmimode", hdmimode);
		}
	}
	if (tmp_outputmode[0]) {
		if (outputmode && is_mode_name_frac(outputmode)) {
			convert_name_frac2int(tmp_outputmode, conv_outputmode);
			env_set("outputmode", conv_outputmode);
		} else {
			env_set("outputmode", outputmode);
		}
	}
	if (tmp_outputmode2[0]) {
		if (outputmode2 && is_mode_name_frac(outputmode2)) {
			convert_name_frac2int(tmp_outputmode2, conv_outputmode2);
			env_set("outputmode2", conv_outputmode2);
		} else {
			env_set("outputmode2", outputmode2);
		}
	}
	if (tmp_outputmode3[0]) {
		if (outputmode3 && is_mode_name_frac(outputmode3)) {
			convert_name_frac2int(tmp_outputmode3, conv_outputmode3);
			env_set("outputmode3", conv_outputmode3);
		} else {
			env_set("outputmode3", outputmode3);
		}
	}
}

static void _hdmi_mode_frac_postprocess(void)
{
	if (!tmp_hdmimode[0] && !tmp_outputmode[0] && !tmp_outputmode2[0] &&
	    !tmp_outputmode3[0]) {
		/* no frac name and return*/
		return;
	}

	if (!get_kernel_dtb_info())
		return;

	/* restore backup name to new name */
	if (tmp_hdmimode[0])
		env_set("hdmimode", tmp_hdmimode);
	if (tmp_outputmode[0])
		env_set("outputmode", tmp_outputmode);
	if (tmp_outputmode2[0])
		env_set("outputmode2", tmp_outputmode2);
	if (tmp_outputmode3[0])
		env_set("outputmode3", tmp_outputmode3);
}

void hdmi_mode_frac_postprocess(void)
{
	_hdmi_mode_frac_postprocess();
	printf("hdmi_tx_set: mode: %s, attr: %s, save hdmichecksum: %s\n",
	       env_get("outputmode"), env_get("colorattribute"), env_get("hdmichecksum"));
	run_command("saveenv", 0);
}
