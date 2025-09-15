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
#include <amlogic/media/vout/hdmitx21/hdmitx_ext.h>
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
#ifdef CONFIG_ARMV8_MULTIENTRY
#include <asm/arch-meson/smp.h>
#endif

static int do_vout_output_job(unsigned char vout_idx, char *mode_str);

#ifdef CONFIG_ARMV8_MULTIENTRY
 //param bit[0:7]:curr vout idx; bit[8]:smp;
static void aml_display_on_post_job(unsigned long param)
{
	if ((param & 0xf) == 0)
		aml_vout_output(0, env_get("outputmode"));
	else if ((param & 0xf) == 1)
		aml_vout_output(1, env_get("outputmode2"));
	else if ((param & 0xf) == 2)
		aml_vout_output(2, env_get("outputmode3"));

	printf("display[%lu] on post done\n", param);

	if (param & 0x100)
		secondary_off();
}
#endif

#ifndef VOUT_HDMITX_HPD_CHECK_CMD
#define VOUT_HDMITX_HPD_CHECK_CMD ""
#endif
#ifndef VOUT_OSD_LOGO_CMD
#define VOUT_OSD_LOGO_CMD         ""
#endif
#ifndef VOUT_HDMITX_HDMI_CMD2
#define VOUT_HDMITX_HDMI_CMD2     ""
#endif

int aml_display_process(unsigned char vout_idx)
{
	unsigned short on_connector_dev = vout_connector_check(vout_idx);
	char *cntor;
	char cnt_str[64], outputmode[16] = "outputmode\0\0\0";
	char vout_cmd[64];
	char vout_init_str[64];

	if (vout_idx >= VOUT_MAX_CNT) {
		printf("VOUT: %s [%u] not supported\n", __func__, vout_idx);
		return 0;
	}

	sprintf(cnt_str, "connector%u_type", vout_idx);
	cntor = env_get(cnt_str);
	if (!cntor) {
		// run_command(cnt_str, 0);
		return 0;
	}

#ifdef CONFIG_AML_LCD
	unsigned int smp_flow = 0;
#ifdef CONFIG_ARMV8_MULTIENTRY
	int smp_ret;
	unsigned char cpu_id = 1;

	smp_flow = (unsigned char)env_get_ulong("display_on_smp", 10, 0);
#endif
#endif
#ifdef CONFIG_AML_HDMITX
	int st = 0;
#endif

	if (vout_idx == 0) {
		run_command("setenv display_layer " CONNECTOR0_OSD ";", 0);
		// if (smp_flow)
		//	run_command("vout prepare ${outputmode};", 0);
		sprintf(vout_cmd, "vout output ${outputmode};");
		sprintf(vout_init_str, "setenv vout_init enable;");
	} else if (vout_idx == 1) {
		run_command("setenv display_layer " CONNECTOR1_OSD ";", 0);
		// run_command("vout2 prepare ${outputmode2};", 0);
		sprintf(vout_cmd, "vout2 output ${outputmode2};");
		sprintf(vout_init_str, "setenv vout2_init enable;");
		outputmode[10] = '2';
	} else if (vout_idx == 2) {
		run_command("setenv display_layer " CONNECTOR2_OSD ";", 0);
		// run_command("vout3 prepare ${outputmode3};", 0);
		sprintf(vout_cmd, "vout3 output ${outputmode3};");
		sprintf(vout_init_str, "setenv vout3_init enable;");
		outputmode[10] = '3';
	}

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
#ifdef CONFIG_AML_HDMITX
	case CONNECTOR_DEV_HDMI:
	case CONNECTOR_DEV_CVBS:
		run_command(VOUT_HDMITX_HPD_CHECK_CMD, 0);
		st = hdmitx_get_hpd_state_ext();
		printf("osd: hpd_state=%c\n", st ? '1' : '0');
		if (!st)
			break;
		run_command(vout_cmd, 0);
		run_command(VOUT_OSD_LOGO_CMD, 0);
		run_command(VOUT_HDMITX_HDMI_CMD2, 0);
		break;
#endif
#ifdef CONFIG_AML_LCD
	case CONNECTOR_DEV_LCD:
		if (smp_flow) {
#ifdef CONFIG_ARMV8_MULTIENTRY
			if (vout_idx == 0)
				run_command("vout prepare ${outputmode};", 0);

			run_command(VOUT_OSD_LOGO_CMD, 0);

			if (env_get("bootup_display") &&
			    strcmp(env_get("bootup_display"), "off") == 0) {
				printf("%s: bootup_display=%s, stop\n", __func__,
					env_get("bootup_display"));
				break;
			}
			smp_ret = run_smp_function(cpu_id + vout_idx,
				&aml_display_on_post_job, 0x100 + vout_idx);
			if (smp_ret)
				printf("display smp failed\n");
			else
				run_command(vout_init_str, 0);
#endif
		} else {
			run_command(VOUT_OSD_LOGO_CMD, 0);
			do_vout_output_job(vout_idx, env_get(outputmode));
		}
		break;
#endif
	default:
		break;
	}

	printf("display[%u]=0x%x -> [%s,%s]\n",
	       vout_idx, on_connector_dev, cntor, env_get(outputmode));

	//run_command(cnt_str, 0);
	return 0;
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
		printf("\nhdmi not support smp on yet\n");
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

	return ret;
}

static int do_vout_output_job(unsigned char vout_idx, char *mode_str)
{
	unsigned short on_connector_dev = vout_connector_check(vout_idx);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;
	char vout_init_str[64];
#endif
#ifdef CONFIG_AML_HDMITX
	struct vinfo_s *vinfo = vout_get_current_vinfo();
	unsigned int fmt_mode = vinfo->vpp_post_out_color_fmt;
	char str[64];
#endif
#if defined(CONFIG_AML_LCD)
	unsigned int venc_index = on_connector_dev & 0xf;
#endif

#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	if (vout_idx)
		sprintf(vout_init_str, "setenv vout%u_init enable", vout_idx);
	else
		sprintf(vout_init_str, "setenv vout_init enable");
#endif
	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode_str);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		if (vout_idx == 0) {
			vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_matrix_update(VPP_CM_RGB);
		} else if (vout_idx == 1) {
			vout_viu_mux(VOUT_VIU2_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_viu2_matrix_update(VPP_CM_RGB);
		} else if (vout_idx == 2) {
			vout_viu_mux(VOUT_VIU3_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_viu3_matrix_update(VPP_CM_RGB);
		}
		aml_lcd_driver_enable(venc_index, mode_str);
		run_command(vout_init_str, 0);
		return 0;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode_str, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			if (vout_idx == 0) {
				vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
				if (fmt_mode == 1)
					vpp_matrix_update(VPP_CM_RGB);
				else
					vpp_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 1) {
				vout_viu_mux(VOUT_VIU2_SEL, mux_sel);
				if (fmt_mode == 1)
					vpp_viu2_matrix_update(VPP_CM_RGB);
				else
					vpp_viu2_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 2) {
				vout_viu_mux(VOUT_VIU3_SEL, mux_sel);
				if (fmt_mode == 1)
					vpp_viu3_matrix_update(VPP_CM_RGB);
				else
					vpp_viu3_matrix_update(VPP_CM_YUV);
			}
			memset(str, 0, sizeof(str));
			sprintf(str, "hdmitx output %s", mode_str);
			run_command(str, 0);
			run_command(vout_init_str, 0);
			return 0;
		}
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode_str);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			if (cvbs_set_vmode(mode_str) == 0) {
				if (vout_idx == 0) {
					vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
					vpp_matrix_update(VPP_CM_YUV);
				}
				run_command(vout_init_str, 0);
				return 0;
			}
		}
#endif
		break;
	default:
		break;
	}
	return 1;
}

static int do_vout_prepare_job(unsigned char vout_idx, char *mode_str)
{
	unsigned short on_connector_dev = vout_connector_check(vout_idx);
#if defined(CONFIG_AML_CVBS) || defined(CONFIG_AML_HDMITX) || defined(CONFIG_AML_LCD)
	unsigned int mux_sel = VIU_MUX_MAX, venc_sel = VIU_MUX_MAX;
#endif
#ifdef CONFIG_AML_LCD
	unsigned int venc_index = on_connector_dev & 0xf;
#endif
#ifdef CONFIG_AML_HDMITX
	struct vinfo_s *vinfo  = vout_get_current_vinfo();
	unsigned int fmt_mode = vinfo->vpp_post_out_color_fmt;
	int st;
	char *connector1_type;
#endif

	switch (on_connector_dev & CONNECTOR_DEV_MASK) {
	case CONNECTOR_DEV_LCD:
#ifdef CONFIG_AML_LCD
		mux_sel = aml_lcd_driver_outputmode_check(venc_index, mode_str);
		venc_sel = mux_sel & 0xf;
		if (venc_sel != VIU_MUX_ENCL)
			break;
		if (vout_idx == 0) {
			vout_viu_mux(VOUT_VIU1_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_matrix_update(VPP_CM_RGB);
		} else if (vout_idx == 1) {
			vout_viu_mux(VOUT_VIU2_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_viu2_matrix_update(VPP_CM_RGB);
		} else if (vout_idx == 2) {
			vout_viu_mux(VOUT_VIU3_SEL, VIU_MUX_ENCL | venc_index << 4);
			vpp_viu3_matrix_update(VPP_CM_RGB);
		}
		aml_lcd_driver_prepare(venc_index, mode_str);
		return 0;
#endif
		break;
	case CONNECTOR_DEV_HDMI:
#ifdef CONFIG_AML_HDMITX
		mux_sel = hdmi_outputmode_check(mode_str, 0);
		venc_sel = mux_sel & 0xf;
		if (venc_sel < VIU_MUX_MAX) {
			if (vout_idx == 0) {
				vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
				if (fmt_mode == 1)
					vpp_matrix_update(VPP_CM_RGB);
				else
					vpp_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 1) {
				vout_viu_mux(VOUT_VIU2_SEL, mux_sel);
				vpp_viu2_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 2) {
				vout_viu_mux(VOUT_VIU3_SEL, mux_sel);
				vpp_viu3_matrix_update(VPP_CM_YUV);
			}
		} else {
			/*
			 * for dual display, hdmitx in viu2, the clkc should always be enabled,
			 * whether the hpd of hdmitx is high or low.
			 */
			st = hdmitx_get_hpd_state_ext();
			connector1_type = env_get("connector1_type");
			if (!st && !strcmp(connector1_type, "HDMI-A-A"))
				vout_set_clkc(VOUT_VIU2_SEL);
		}
		return 0;
#endif
		break;
	case CONNECTOR_DEV_CVBS:
#ifdef CONFIG_AML_CVBS
		mux_sel = cvbs_outputmode_check(mode_str);
		venc_sel = mux_sel & 0xf;
		if (venc_sel == VIU_MUX_ENCI) {
			if (vout_idx == 0) {
				vout_viu_mux(VOUT_VIU1_SEL, mux_sel);
				vpp_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 1) {
				vout_viu_mux(VOUT_VIU2_SEL, mux_sel);
				vpp_viu2_matrix_update(VPP_CM_YUV);
			} else if (vout_idx == 2) {
				vout_viu_mux(VOUT_VIU3_SEL, mux_sel);
				vpp_viu3_matrix_update(VPP_CM_YUV);
			}
			return 0;
		}
#endif
		break;
	default:
		break;
	}
	return 1;
}

static int do_vout_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	if (do_vout_prepare_job(0, mode)) {
		printf("VOUT: output prepare fail\n");
		vout_pr_connector_and_vmode();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int do_vout_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	if (do_vout_output_job(0, mode)) {
		printf("VOUT: output fail\n");
		vout_pr_connector_and_vmode();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int do_vout2_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	if (do_vout_output_job(1, mode)) {
		printf("VOUT2: output fail\n");
		vout_pr_connector_and_vmode();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int do_vout2_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	if (do_vout_prepare_job(1, mode)) {
		printf("VOUT2: output prepare fail\n");
		vout_pr_connector_and_vmode();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int do_vout3_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	do_vout_output_job(2, mode);

	printf("VOUT3: output fail\n");
	vout_pr_connector_and_vmode();
	return CMD_RET_FAILURE;
}

static int do_vout3_prepare(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char mode[32]; //use stack instead of heap for smp

	if (argc != 2)
		return CMD_RET_FAILURE;

	memset(mode, 0, 32);
	sprintf(mode, "%s", argv[1]);

	if (do_vout_prepare_job(2, mode)) {
		printf("VOUT3: output prepare fail\n");
		vout_pr_connector_and_vmode();
		return CMD_RET_FAILURE;
	}
	return CMD_RET_SUCCESS;
}

static int do_vout_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	vout_vinfo_dump();

	return CMD_RET_SUCCESS;
}

static int do_vout_list(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
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

static int do_vout_display(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned short on_connector_dev;

	on_connector_dev = vout_connector_check(0);
	if (on_connector_dev < 0xffff) {
		run_command("setenv display_layer " CONNECTOR0_OSD ";", 0);
		aml_display_process(0);
	}

	on_connector_dev = vout_connector_check(1);
	if (on_connector_dev < 0xffff) {
		run_command("setenv display_layer " CONNECTOR1_OSD ";", 0);
		run_command("vout2 prepare ${outputmode2};", 0);
		aml_display_process(1);
	}

	on_connector_dev = vout_connector_check(2);
	if (on_connector_dev < 0xffff) {
		run_command("setenv display_layer " CONNECTOR2_OSD ";", 0);
		run_command("vout3 prepare ${outputmode3};", 0);
		aml_display_process(2);
	}

	return CMD_RET_SUCCESS;
}

#define VOUT_HELPER_STRING \
	"vout/vout2/vout3 [list | output format | info | boot_display]\n" \
	"    list    : list for valid video mode names\n" \
	"    prepare : prepare\n" \
	"    format  : perfered output video mode\n" \
	"    info    : dump vinfo\n"

static cmd_tbl_t cmd_vout_sub[] = {
	U_BOOT_CMD_MKENT(boot_display, 1, 1, do_vout_display, "", ""),
	U_BOOT_CMD_MKENT(list,         1, 1, do_vout_list,    "", ""),
	U_BOOT_CMD_MKENT(prepare,      3, 1, do_vout_prepare, "", ""),
	U_BOOT_CMD_MKENT(output,       3, 1, do_vout_output,  "", ""),
	U_BOOT_CMD_MKENT(info,         1, 1, do_vout_info,    "", ""),
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
