// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <malloc.h>
#include <asm/byteorder.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#else
#define VPP_CM_RGB     0
#define VPP_CM_YUV     2
#define VPP_CM_INVALID 0xff
__weak void vpp_matrix_update(int cfmt) {}
__weak void vpp_viu2_matrix_update(int cfmt) {}
__weak void vpp_viu3_matrix_update(int cfmt) {}
#endif

#include <amlogic/media/vout/aml_vout.h>
#ifdef CONFIG_AML_HDMITX
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#else
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#endif
#endif
#ifdef CONFIG_AML_CVBS
#include <amlogic/media/vout/aml_cvbs.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/aml_lcd.h>
#endif

#ifdef CONFIG_AML_HDMITX
static int vout_hdmi_hpd(struct hdmitx_dev *hdev, int hpd_st)
{
	char *hdmimode;
	char *cvbsmode;
	char *colorattribute;
	char *connector0_type;
	int hdmi_enc_idx;

	/* get hdmi mode and colorattribute from env */
	hdmimode = env_get("hdmimode");
	if (hdmimode)
		printf("%s: hdmimode=%s\n", __func__, hdmimode);

	colorattribute = env_get("colorattribute");
	if (colorattribute)
		printf("%s: colorattribute=%s\n", __func__, colorattribute);

	connector0_type = env_get("connector0_type");
	if (!connector0_type || !strcmp(connector0_type, "none")) {
		env_set("connector0_type", "HDMI-A-A");
		connector0_type = env_get("connector0_type");
		printf("add default connector for HDMI-A-A\n");
	} else {
		printf("%s: connector0_type: %s\n", __func__, connector0_type);
	}

	/* check whether the current connector is HDMI */
	hdmi_enc_idx = is_valid_hdmi(connector0_type);

	/*
	 * Check whether the current connector is HDMI or CVBS,
	 * and set a new value for the outputmode based on the HPD status.
	 * In this way, the LCD will not be processed.
	 * if hpd_st high, outputmode and connector0_type will be saved on hdmi side
	 * if hpd_st low, outputmode and connector0_type will be saved on cvbs side
	 */
	if (hdmi_enc_idx || strcmp(connector0_type, "TV-1") == 0) {
		if (!hpd_st) {
			cvbsmode = env_get("cvbsmode");
			if (cvbsmode)
				env_set("outputmode", cvbsmode);
			env_set("hdmichecksum", "0x00000000");
			env_set("connector0_type", "TV-1");
		} else {
			if (!strstr(env_get("outputmode"), "hz"))
				env_set("outputmode", "1080p60hz");
			switch (hdev->enc_idx) {
			case 0:
				env_set("connector0_type", "HDMI-A-A");
				break;
			case 1:
				env_set("connector0_type", "HDMI-A-B");
				break;
			case 2:
				env_set("connector0_type", "HDMI-A-C");
				break;
			}
		}
	}

	return 1;
}

static int vout2_hdmi_hpd(struct hdmitx_dev *hdev, int hpd_st)
{
	char *hdmimode;
	char *cvbsmode;
	char *colorattribute;
	char *connector1_type;
	int hdmi_enc_idx;

	/* get hdmi mode and colorattribute from env */
	hdmimode = env_get("hdmimode");
	if (hdmimode)
		printf("%s: hdmimode=%s\n", __func__, hdmimode);
	colorattribute = env_get("colorattribute");
	if (colorattribute)
		printf("%s: colorattribute=%s\n", __func__, colorattribute);

	connector1_type = env_get("connector1_type");
	if (!connector1_type || !strcmp(connector1_type, "none")) {
		env_set("connector1_type", "HDMI-A-A");
		connector1_type = env_get("connector1_type");
		printf("add default connector for HDMI-A-A\n");
	} else {
		printf("%s: connector1_type: %s\n", __func__, connector1_type);
	}

	/* check whether the current connector is HDMI */
	hdmi_enc_idx = is_valid_hdmi(connector1_type);

	/*
	 * Check whether the current connector is HDMI or CVBS,
	 * and set a new value for the outputmode2 based on the HPD status.
	 * In this way, the LCD will not be processed.
	 * if hpd_st high, outputmode2 and connector1_type will be saved on hdmi side
	 * if hpd_st low, outputmode2 and connector1_type will be saved on cvbs side
	 */
	if (hdmi_enc_idx || strcmp(connector1_type, "TV-1") == 0) {
		if (!hpd_st) {
			cvbsmode = env_get("cvbsmode");
			if (cvbsmode)
				env_set("outputmode2", cvbsmode);
			env_set("hdmichecksum", "0x00000000");
		} else {
			if (!strstr(env_get("outputmode2"), "hz"))
				env_set("outputmode2", "1080p60hz");
			switch (hdev->enc_idx) {
			case 0:
				env_set("connector1_type", "HDMI-A-A");
				break;
			case 1:
				env_set("connector1_type", "HDMI-A-B");
				break;
			case 2:
				env_set("connector1_type", "HDMI-A-C");
				break;
			}
		}
	}

	return 0;
}

int do_hpd_detect(cmd_tbl_t *cmdtp, int flag, int argc,
		  char *const argv[])
{
	char *st;
	int hpd_st = 0;
	unsigned long i = 0;
	/* some TV sets pull hpd high 1.3S after detect pwr5v high */
	unsigned long hdmitx_hpd_wait_cnt = 15;
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdev = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdev = get_hdmitx21_device();
#endif
	int ret = 0;

	st = env_get("hdmitx_hpd_bypass");
	if (st && (strcmp((const char *)(uintptr_t)st[0], "1") == 0)) {
		printf("hdmitx_hpd_bypass detect\n");
		return 0;
	}
	st = env_get("hdmitx_hpd_wait_cnt");
	if (st)
		hdmitx_hpd_wait_cnt = simple_strtoul(st, NULL, 10);
	hpd_st = hdev->hwop.get_hpd_state();

	if (!hpd_st) {
		/* For some TV, they cost extra time to pullup HPD after 5V */
		for (i = 0; i < hdmitx_hpd_wait_cnt; i++) {
			mdelay(100);
			hpd_st = hdev->hwop.get_hpd_state();
			if (hpd_st) {
				printf("hpd delay %lu ms\n", (i + 1) * 100);
				break;
			}
		}
	}
	printf("%s, hpd_state=%d\n", __func__, hpd_st);

	ret = vout_hdmi_hpd(hdev, hpd_st);
	if (!ret)
		vout2_hdmi_hpd(hdev, hpd_st);

	hdev->hpd_state = hpd_st;
	return hpd_st;
}
#endif

int aml_vout_prepare(uint8_t vout_idx, char *mode)
{
	unsigned short on_connector_dev = vout_connector_check(vout_idx);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;
	unsigned char viu_sel = vout_idx + 1; //VOUT_VIU1_SEL/VOUT_VIU2_SEL/VOUT_VIU3_SEL
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#ifdef CONFIG_AML_HDMITX
	struct vinfo_s *vinfo  = vout_get_current_vinfo();
	unsigned int fmt_mode = vinfo->vpp_post_out_color_fmt;
#endif

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(viu_sel, VIU_MUX_ENCL | venc_index << 4);
		vpp_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_prepare(venc_index, mode);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(viu_sel, mux_sel);
			if (fmt_mode == 1)
				vpp_matrix_update(VPP_CM_RGB);
			else
				vpp_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(viu_sel, mux_sel);
			vpp_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT: output prepare fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

int aml_vout_output(uint8_t vout_idx, char *mode)
{
	unsigned short on_connector_dev = vout_connector_check(vout_idx);
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;
#endif
	int ret = -1;

	if (!mode)
		return -1;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		aml_lcd_driver_enable(venc_index, mode);
		ret = 0;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel >= VIU_MUX_MAX)
			break;
		// ret = aml_hdmitx_output(mode);
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCI)
			break;
		ret = cvbs_set_vmode(mode);
#endif
		break;
	default:
		break;
	}

	if (ret == 0) {
		if (vout_idx == 0)
			run_command("setenv vout_init enable", 0);
		else if (vout_idx == 1)
			run_command("setenv vout2_init enable", 0);
		else if (vout_idx == 2)
			run_command("setenv vout3_init enable", 0);
	}

	return ret;
}

static int do_vout_list(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	vout_pr_connector_and_vmode();

#ifdef CONFIG_AML_HDMITX
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdmitx_device = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdmitx_device = get_hdmitx21_device();
#endif
#endif

#ifdef CONFIG_AML_HDMITX
	if (!hdmitx_device) {
		printf("\nerror: hdmitx device is null\n");
	} else {
		printf("\nvalid hdmi mode:\n");
		hdmitx_device->hwop.list_support_modes();
	}
#endif

#ifdef CONFIG_AML_CVBS
	printf("\nvalid cvbs mode:\n");
	cvbs_show_valid_vmode();
#endif

#ifdef CONFIG_AML_LCD
	printf("\nvalid lcd mode:\n");
	aml_lcd_driver_list_support_mode();
#endif

	return CMD_RET_SUCCESS;
}

static int do_vout_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(0);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, 64);
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#ifdef CONFIG_AML_HDMITX
	struct vinfo_s *vinfo  = vout_get_current_vinfo();
	unsigned int fmt_mode = vinfo->vpp_post_out_color_fmt;
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
		vpp_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_prepare(venc_index, mode);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			if (fmt_mode == 1)
				vpp_matrix_update(VPP_CM_RGB);
			else
				vpp_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			vpp_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT: output prepare fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(0);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, 64);
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#ifdef CONFIG_AML_HDMITX
	struct vinfo_s *vinfo = vout_get_current_vinfo();
	unsigned int fmt_mode = vinfo->vpp_post_out_color_fmt;
	char str[64];
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
		vpp_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_enable(venc_index, mode);
		run_command("setenv vout_init enable", 0);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			if (fmt_mode == 1)
				vpp_matrix_update(VPP_CM_RGB);
			else
				vpp_matrix_update(VPP_CM_YUV);
			memset(str, 0, sizeof(str));
			sprintf(str, "hdmitx output %s", mode);
			if (run_command(str, 0) == CMD_RET_SUCCESS)
				run_command("setenv vout_init enable", 0);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			vpp_matrix_update(VPP_CM_YUV);
			if (cvbs_set_vmode(mode) == 0) {
				run_command("setenv vout_init enable", 0);
				return CMD_RET_SUCCESS;
			}
		}
#endif
		break;
	default:
		break;
	}

	printf("VOUT: output fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout2_list(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	vout_pr_connector_and_vmode();

#ifdef CONFIG_AML_HDMITX
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdmitx_device = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdmitx_device = get_hdmitx21_device();
#endif

	if (!hdmitx_device) {
		printf("\nerror: hdmitx device is null\n");
	} else {
		printf("\nvalid hdmi mode:\n");
		hdmitx_device->hwop.list_support_modes();
	}
#endif

#ifdef CONFIG_AML_CVBS
	printf("\nvalid cvbs mode:\n");
	cvbs_show_valid_vmode();
#endif

#ifdef CONFIG_AML_LCD
	printf("\nvalid lcd mode:\n");
	aml_lcd_driver_list_support_mode();
#endif

	return CMD_RET_SUCCESS;
}

static int do_vout2_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(1);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, 64);
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#ifdef CONFIG_AML_HDMITX
	char str[64];
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		aml_lcd_driver_enable(venc_index, mode);
		run_command("setenv vout2_init enable", 0);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			memset(str, 0, sizeof(str));
			sprintf(str, "hdmitx output %s", mode);
			run_command(str, 0);
			run_command("setenv vout2_init enable", 0);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			if (cvbs_set_vmode(mode) == 0) {
				run_command("setenv vout2_init enable", 0);
				return CMD_RET_SUCCESS;
			}
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT2: output fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;

}

static int do_vout2_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(1);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, 64);
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU2_SEL, VIU_MUX_ENCL | venc_index << 4);
		vpp_viu2_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_prepare(venc_index, mode);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU2_SEL, mux_sel);
			vpp_viu2_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU2_SEL, mux_sel);
			vpp_viu2_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT2: output prepare fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout3_list(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	vout_pr_connector_and_vmode();

#ifdef CONFIG_AML_HDMITX
#ifdef CONFIG_AML_HDMITX20
	struct hdmitx_dev *hdmitx_device = hdmitx_get_hdev();
#else
	struct hdmitx_dev *hdmitx_device = get_hdmitx21_device();
#endif

	if (!hdmitx_device) {
		printf("\nerror: hdmitx device is null\n");
	} else {
		printf("\nvalid hdmi mode:\n");
		hdmitx_device->hwop.list_support_modes();
	}
#endif

#ifdef CONFIG_AML_CVBS
	printf("\nvalid cvbs mode:\n");
	cvbs_show_valid_vmode();
#endif

#ifdef CONFIG_AML_LCD
	printf("\nvalid lcd mode:\n");
	aml_lcd_driver_list_support_mode();
#endif

	return CMD_RET_SUCCESS;
}

static int do_vout3_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(2);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, 64);
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_HDMITX
	char str[64];
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		aml_lcd_driver_enable(venc_index, mode);
		run_command("setenv vout3_init enable", 0);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			memset(str, 0, sizeof(str));
			sprintf(str, "hdmitx output %s", mode);
			run_command(str, 0);
			run_command("setenv vout3_init enable", 0);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			if (cvbs_set_vmode(mode) == 0) {
				run_command("setenv vout3_init enable", 0);
				return CMD_RET_SUCCESS;
			}
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT3: output fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout3_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned short on_connector_dev = vout_connector_check(2);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	char mode[64]; //use stack instead of heap for smp
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;

	memset(mode, 0, (sizeof(char) * 64));
	sprintf(mode, "%s", argv[1]);
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU3_SEL, VIU_MUX_ENCL | venc_index << 4);
		vpp_viu3_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_prepare(venc_index, mode);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU3_SEL, mux_sel);
			vpp_viu3_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU3_SEL, mux_sel);
			vpp_viu3_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT3: output prepare fail(0x%04x)\n", on_connector_dev);
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout_info(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	vout_vinfo_dump();

	return CMD_RET_SUCCESS;
}

#define VOUT_HELPER_STRING \
	"vout/vout2/vout3 [list | output format | info]\n" \
	"    list    : list for valid video mode names\n" \
	"    prepare : prepare\n" \
	"    format  : perfered output video mode\n" \
	"    info    : dump vinfo\n"

static cmd_tbl_t cmd_vout_sub[] = {
	U_BOOT_CMD_MKENT(list,    1, 1, do_vout_list,    "", ""),
	U_BOOT_CMD_MKENT(prepare, 3, 1, do_vout_prepare, "", ""),
	U_BOOT_CMD_MKENT(output,  3, 1, do_vout_output,  "", ""),
	U_BOOT_CMD_MKENT(info,    1, 1, do_vout_info,    "", ""),
};

static int do_vout(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vout_sub[0], ARRAY_SIZE(cmd_vout_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vout, CONFIG_SYS_MAXARGS, 1, do_vout, "VOUT sub-system", VOUT_HELPER_STRING);

static cmd_tbl_t cmd_vout2_sub[] = {
	U_BOOT_CMD_MKENT(list,    1, 1, do_vout2_list,    "", ""),
	U_BOOT_CMD_MKENT(prepare, 3, 1, do_vout2_prepare, "", ""),
	U_BOOT_CMD_MKENT(output,  3, 1, do_vout2_output,  "", ""),
	U_BOOT_CMD_MKENT(info,    1, 1, do_vout_info,     "", ""),
};

static int do_vout2(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vout2_sub[0], ARRAY_SIZE(cmd_vout2_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vout2, CONFIG_SYS_MAXARGS, 1, do_vout2, "VOUT2 sub-system", VOUT_HELPER_STRING);

static cmd_tbl_t cmd_vout3_sub[] = {
	U_BOOT_CMD_MKENT(list,    1, 1, do_vout3_list,    "", ""),
	U_BOOT_CMD_MKENT(prepare, 3, 1, do_vout3_prepare, "", ""),
	U_BOOT_CMD_MKENT(output,  3, 1, do_vout3_output,  "", ""),
	U_BOOT_CMD_MKENT(info,    1, 1, do_vout_info,     "", ""),
};

static int do_vout3(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vout3_sub[0], ARRAY_SIZE(cmd_vout3_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vout3, CONFIG_SYS_MAXARGS, 1, do_vout3, "VOUT3 sub-system", VOUT_HELPER_STRING);
