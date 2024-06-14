// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <amlogic/cpu_id.h>

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

#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
static unsigned int vpp_viu1_cm_state = VPP_CM_INVALID;
#endif

struct cntor_name2val_s {
	char *name;
	unsigned short val;
};

#define CONNECTOR_HEAD_LCD      0x100
#define CONNECTOR_HEAD_RESERVED 0x200
#define CONNECTOR_HEAD_HDMI     0x300
#define CONNECTOR_HEAD_CVBS     0x400
struct cntor_name2val_s vout_supported_cnt_list[] = {
	{.name = "LVDS-A",   .val = 0x100},
	{.name = "LVDS-B",   .val = 0x101},
	{.name = "LVDS-C",   .val = 0x102},
	{.name = "VBYONE-A", .val = 0x110},
	{.name = "VBYONE-B", .val = 0x111},
	{.name = "MIPI-A",   .val = 0x120},
	{.name = "MIPI-B",   .val = 0x121},
	{.name = "EDP-A",    .val = 0x130},
	{.name = "EDP-B",    .val = 0x131},
	{.name = "HDMI-A-A", .val = 0x300},
	{.name = "HDMI-A-B", .val = 0x301},
	{.name = "HDMI-A-C", .val = 0x302},
	{.name = "CVBS",     .val = 0x400},
};

static unsigned short vout_connector_check(unsigned char vout_index)
{
	char *cntor;
	char cnt_name[20] = "connectorX_type";
	unsigned char i;

	cnt_name[9] = '0' + vout_index;
	cntor = env_get(cnt_name);
	if (!cntor)
		return 0xffff;

	for (i = 0; i < ARRAY_SIZE(vout_supported_cnt_list); i++) {
		if (strcmp(cntor, vout_supported_cnt_list[i].name) == 0)
			return vout_supported_cnt_list[i].val;
	}

	return 0xffff;
}

static void pr_connector_and_vmode(void)
{
	char *cntor, *opt_vmode;
	char cnt_name[20] = "connectorX_type";
	char opt_mode_name[20] = "outputmode\0\0";
	unsigned char idx;

	printf("VOUT: connector & outputmode info:\n");
	for (idx = 0; idx < 3; idx++) {
		cnt_name[9] = '0' + idx;
		if (idx)
			opt_mode_name[10] = '1' + idx;

		cntor = env_get(cnt_name);
		opt_vmode = env_get(opt_mode_name);
		printf("  VOUT%c: %s: %-9s | outputmode%c: %s\n", idx ? '1' + idx : ' ',
			cnt_name, cntor, idx ? '1' + idx : ' ', opt_vmode);
	}
}

// ! only to maintain compatibility for uboot 2019, NO sync to 2023
static void legacy_panel_mode_support(unsigned char vout_index)
{
#ifdef CONFIG_AML_LCD
	char *opt_vmode;
	char cnt_name[20] = "connectorX_type";
	char opt_mode_name[20] = "outputmode\0\0";
	char legacy_lcd_mode[3][8] = {"panel", "panel1", "panel2"};
	unsigned char i, legacy_mode_find = 0xff, name_idx, lcd_type;
	char *connector_name_list[4] = {"LVDS", "VBYONE", "MIPI", "EDP"};
	struct aml_lcd_drv_s *pdrv;
	char cnt_buf[12], cmd_buf[64];

	if (vout_index)
		opt_mode_name[10] = '1' + vout_index;

	opt_vmode = env_get(opt_mode_name);
	if (!opt_vmode)
		return;

	for (i = 0; i < 3; i++) {
		if (!strcmp(legacy_lcd_mode[i], opt_vmode))
			legacy_mode_find = i;
	}

	if (legacy_mode_find == 0xff)
		return;

	pdrv = aml_lcd_get_driver(legacy_mode_find);
	if (!pdrv)
		return;

	lcd_type = pdrv->config.basic.lcd_type;
	if (lcd_type == LCD_LVDS || lcd_type == LCD_MLVDS)
		name_idx = 0;
	else if (lcd_type == LCD_VBYONE || lcd_type == LCD_P2P)
		name_idx = 1;
	else if (lcd_type == LCD_MIPI)
		name_idx = 2;
	else if (lcd_type == LCD_EDP)
		name_idx = 3;
	else
		return;

	sprintf(cnt_buf, "%s-%c", connector_name_list[name_idx], 'A' + legacy_mode_find);

	cnt_name[9] = '0' + vout_index;

	sprintf(cmd_buf, "setenv %s %s", cnt_name, cnt_buf);
	printf("vout: lcd compatible (%s)\n", cmd_buf);

	run_command(cmd_buf, 0);
#endif
}

#ifdef CONFIG_AML_HDMITX
static int vout_hdmi_hpd(int hpd_st)
{
#ifdef CONFIG_AML_LCD
	unsigned int venc_sel = VIU_MUX_MAX;
	char *mode;
#endif
	char *hdmimode;
	char *cvbsmode;
	char *colorattribute;

#ifdef CONFIG_AML_LCD
	mode = env_get("outputmode");
	venc_sel = aml_lcd_driver_outputmode_check(0, mode);

	if (venc_sel == VIU_MUX_ENCL) {
		printf("%s: lcd no need hpd detect\n", __func__);
		return 0;
	}
#endif
	/*get hdmi mode and colorattribute from env */
	hdmimode = env_get("hdmimode");
	if (hdmimode)
		printf("%s: hdmimode=%s\n", __func__, hdmimode);

	colorattribute = env_get("colorattribute");
	if (colorattribute)
		printf("%s: colorattribute=%s\n", __func__, colorattribute);

	/* if hpd_st high, output mode will be saved on hdmi side */
	if (!hpd_st) {
		cvbsmode = env_get("cvbsmode");
		if (cvbsmode)
			env_set("outputmode", cvbsmode);
		env_set("hdmichecksum", "0x00000000");
		//run_command("saveenv", 0);
	} else {
		if (!strstr(env_get("outputmode"), "hz"))
			env_set("outputmode", "1080p60hz");
	}

	return 1;
}

static int vout2_hdmi_hpd(int hpd_st)
{
#ifdef CONFIG_AML_LCD
	unsigned int venc_sel = VIU_MUX_MAX;
	char *mode;
#endif
	char *hdmimode;
	char *cvbsmode;
	char *colorattribute;

#ifdef CONFIG_AML_LCD
	mode = env_get("outputmode2");
	venc_sel = aml_lcd_driver_outputmode_check(0, mode);
	if (venc_sel == VIU_MUX_ENCL) {
		return 0;
	}
#endif
	/*get hdmi mode and colorattribute from env */
	hdmimode = env_get("hdmimode");
	if (hdmimode)
		printf("%s: hdmimode=%s\n", __func__, hdmimode);
	colorattribute = env_get("colorattribute");
	if (colorattribute)
		printf("%s: colorattribute=%s\n", __func__, colorattribute);
	/* if hpd_st high, output mode will be saved on hdmi side */
	if (!hpd_st) {
		cvbsmode = env_get("cvbsmode");
		if (cvbsmode)
			env_set("outputmode2", cvbsmode);
		env_set("hdmichecksum", "0x00000000");
		//run_command("saveenv", 0);
	} else {
		if (!strstr(env_get("outputmode2"), "hz"))
			env_set("outputmode2", "1080p60hz");
	}

	return 0;
}

int do_hpd_detect(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
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
	unsigned short on_connector_dev = vout_connector_check(0);


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

	for (i = 0; i < 3; i++) {
		on_connector_dev = vout_connector_check(i);
		if ((on_connector_dev & 0xf00) == CONNECTOR_HEAD_HDMI) {
			if (i == 0) {
				vout_hdmi_hpd(hpd_st);
			} else if (i == 1) {
				vout2_hdmi_hpd(hpd_st);
			} else {
				printf("warning unregistered connectorX_type\n");
				vout_hdmi_hpd(hpd_st);
			}
		}
	}

	hdev->hpd_state = hpd_st;
	return hpd_st;
}
#endif

static int do_vout_list(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	pr_connector_and_vmode();

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

static int do_vout_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	legacy_panel_mode_support(0);

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

	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
		vpp_viu1_cm_state = VPP_CM_RGB;
		vpp_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_prepare(venc_index, mode);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_HEAD_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			if (fmt_mode == 1) {
				vpp_viu1_cm_state = VPP_CM_RGB;
				vpp_matrix_update(VPP_CM_RGB);
			} else {
				vpp_viu1_cm_state = VPP_CM_YUV;
				vpp_matrix_update(VPP_CM_YUV);
			}
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_HEAD_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			vpp_viu1_cm_state = VPP_CM_YUV;
			vpp_matrix_update(VPP_CM_YUV);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	default:
		break;
	}
	printf("VOUT: output prepare fail(0x%04x)\n", on_connector_dev);
	pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	legacy_panel_mode_support(0);

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

#ifdef CONFIG_AML_HDMITX
	if (on_connector_dev == 0xffff) {
		/* if current mode is HDMI mode, but lacks the environment
		 * "HDMI-A-A" for "connector0_type", then assign the default
		 * value for HDMI
		 */
		if (hdmi_outputmode_check(mode, 0) != VIU_MUX_MAX) {
			env_set("connector0_type", "HDMI-A-A");
			on_connector_dev = vout_connector_check(0);
			printf("add default connector for HDMI mode %s", mode);
		} else {
			printf("error: invalid mode %s for hdmi or panel\n", mode);
		}
	}
#endif
	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
		if (vpp_viu1_cm_state != VPP_CM_RGB)
			vpp_matrix_update(VPP_CM_RGB);
		aml_lcd_driver_enable(venc_index, mode);
		run_command("setenv vout_init enable", 0);
		return CMD_RET_SUCCESS;
#endif
		break;
	case CONNECTOR_HEAD_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			if (fmt_mode == 1) {
				if (vpp_viu1_cm_state != VPP_CM_RGB)
					vpp_matrix_update(VPP_CM_RGB);
			} else {
				if (vpp_viu1_cm_state != VPP_CM_YUV)
					vpp_matrix_update(VPP_CM_YUV);
			}
			memset(str, 0, sizeof(str));
			sprintf(str, "hdmitx output %s", mode);
			if (run_command(str, 0) == CMD_RET_SUCCESS)
				run_command("setenv vout_init enable", 0);
			return CMD_RET_SUCCESS;
		}
#endif
		break;
	case CONNECTOR_HEAD_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
			if (vpp_viu1_cm_state != VPP_CM_YUV)
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
	pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout2_list(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	pr_connector_and_vmode();

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
	legacy_panel_mode_support(1);

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

	if (get_cpu_id().family_id == MESON_CPU_MAJOR_ID_TXHD2)
		vpp_matrix_update(VPP_CM_RGB);

	if (argc != 2)
		return CMD_RET_FAILURE;

	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
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
	case CONNECTOR_HEAD_HDMI:
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
	case CONNECTOR_HEAD_CVBS:
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
	pr_connector_and_vmode();
	return CMD_RET_FAILURE;

}

static int do_vout2_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	legacy_panel_mode_support(1);

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

	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
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
	case CONNECTOR_HEAD_HDMI:
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
	case CONNECTOR_HEAD_CVBS:
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
	pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout3_list(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pr_connector_and_vmode();

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
	legacy_panel_mode_support(2);

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

	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
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
	case CONNECTOR_HEAD_HDMI:
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
	case CONNECTOR_HEAD_CVBS:
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
	pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout3_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	legacy_panel_mode_support(2);

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

	switch (on_connector_dev & 0xf00) {
	case CONNECTOR_HEAD_LCD:
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
	case CONNECTOR_HEAD_HDMI:
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
	case CONNECTOR_HEAD_CVBS:
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
	pr_connector_and_vmode();
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
