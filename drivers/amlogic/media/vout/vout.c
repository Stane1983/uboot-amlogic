// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <common.h>
//#include <asm/arch/io.h>
#include <asm/io.h>
//#include <asm/arch/secure_apb.h>
#include <asm/amlogic/arch/secure_apb.h>

#include <amlogic/cpu_id.h>
#include <amlogic/fb.h>
#include <amlogic/media/vpp/vpp.h>
#include <amlogic/media/vout/aml_vout.h>
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/aml_lcd.h>
#endif
#include "vout.h"
#include <env.h>

#define VOUT_LOG_DBG 0
#define VOUT_LOG_TAG "[VOUT]"
#define vout_log(fmt, ...) printf(VOUT_LOG_TAG fmt, ##__VA_ARGS__)
#define vout_logl() \
	do { \
		if (VOUT_LOG_DBG > 0) \
			vout_log("%s:%d\n", __func__, __LINE__); \
	} while (0)

static struct vout_conf_s *vout_conf;
static int vout_conf_check(void);
#include "vout_reg.h"
struct cntor_name2val_s {
	char *name;
	unsigned short val;
};

static struct cntor_name2val_s vout_supported_cnt_list[] = {
	{.name = "LVDS-A",    .val = 0x100},
	{.name = "LVDS-B",    .val = 0x101},
	{.name = "LVDS-C",    .val = 0x102},
	{.name = "VBYONE-A",  .val = 0x110},
	{.name = "VBYONE-B",  .val = 0x111},
	{.name = "MIPI-A",    .val = 0x120},
	{.name = "MIPI-B",    .val = 0x121},
	{.name = "EDP-A",     .val = 0x130},
	{.name = "EDP-B",     .val = 0x131},
	{.name = "LCD-A",     .val = 0x1f0},
	{.name = "HDMI-A-A",  .val = 0x300},
	{.name = "HDMI-A-B",  .val = 0x301},
	{.name = "HDMI-A-C",  .val = 0x302},
	/* Follow SWPL-177195, change CVBS's name to TV-1, matching the DRM framework. */
	{.name = "TV-1",      .val = 0x400},
};

unsigned short vout_connector_check(unsigned char vout_index)
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

void vout_pr_connector_and_vmode(void)
{
	char *cntor, *opt_vmode;
	char cnt_name[20] = "connectorX_type";
	char opt_mode_name[20] = "outputmode\0\0";
	unsigned char idx;

	printf("VOUT: connector & outputmode info:\n");
	for (idx = 0; idx < VOUT_MAX_CNT; idx++) {
		cnt_name[9] = '0' + idx;
		if (idx)
			opt_mode_name[10] = '1' + idx;

		cntor = env_get(cnt_name);
		opt_vmode = env_get(opt_mode_name);
		printf("  VOUT%c: %s: %-9s | outputmode%c: %s\n", idx ? '1' + idx : ' ',
		       cnt_name, cntor, idx ? '1' + idx : ' ', opt_vmode);
	}
}

static const struct vout_set_s vout_sets_dft[] = {
/*       name,      width, height, field_height */
	{"480i",    720,   480,    240},
	{"480cvbs", 720,   480,    240},
	{"ntsc_m",  720,   480,    240},
	{"pal_m",   720,   480,    240},
	{"pal_n",   720,   576,    288},
	{"480p",    720,   480,    480},
	{"576i",    720,   576,    288},
	{"576cvbs", 720,   576,    288},
	{"576p",    720,   576,    576},
	{"720p",   1280,   720,    720},
	{"768p",   1366,   768,    768},
	{"1080i",  1920,  1080,    540},
	{"1080p",  1920,  1080,   1080},
	{"2160p",  3840,  2160,   2160},
	{"smpte",  4096,  2160,   2160},
	{"vga",     640,   480,    480},
	{"svga",    800,   600,    600},
	{"xga",    1024,   768,    768},
	{"sxga",   1280,  1024,   1024},
	{"wsxga",  1440,   900,    900},
	{"fhdvga", 1920,  1080,   1080},
};

static struct vout_set_s vout_sets_full[VOUT_MAX_CNT] = {
	{"VOUT_VMODE",  640, 480, 480},
	{"VOUT2_VMODE", 640, 480, 480},
	{"VOUT3_VMODE", 640, 480, 480},
};

static struct vinfo_s vout_info = {
	.width  = 1920,              /* Number of columns (i.e. 160) */
	.height = 1080,               /* Number of rows (i.e. 100) */
	.field_height = 1080,

	.vl_bpix = 24,               /* Bits per pixel */
	.vd_base = NULL,             /* Start of framebuffer memory */
	.vd_console_address = NULL,  /* Start of console buffer	*/
	.console_col = 0,
	.console_row = 0,

	.vd_color_fg = 0xffff,
	.vd_color_bg = 0,
	.cmap = NULL,                /* Pointer to the colormap */
	.priv = NULL,                /* Pointer to driver-specific data */
};

static int vout_conf_check(void)
{
	if (vout_conf)
		return 0;

	vout_probe();
	if (!vout_conf) {
		vout_log("error: %s: no vout_conf\n", __func__);
		return -1;
	}

	return 0;
}

static int my_atoi(const char *str)
{
	int result = 0;
	int signal = 1;

	if ((*str >= '0' && *str <= '9') || *str == '-' || *str == '+') {
		if (*str == '-' || *str == '+') {
			if (*str == '-')
				signal = -1;
			str++;
		}
	} else {
		return 0;
	}

	while (*str >= '0' && *str <= '9')
		result = result * 10 + (*str++ - '0');

	return signal * result;
}

int parse_resolution(const char *str, unsigned int *width, unsigned int *height, unsigned int *fr)
{
	const char *ptr, *ptr2;

	ptr2 = str;
	ptr = strstr(str, "x");
	if (!ptr)
		return 0;
	*width = my_atoi(ptr2);

	ptr2 = ptr;
	ptr = strstr(ptr + 1, "p");
	if (!ptr)
		return 0;
	*height = my_atoi(ptr2 + 1);

	ptr2 = ptr;
	ptr = strstr(ptr + 1, "hz");
	if (!ptr)
		return 0;
	*fr = my_atoi(ptr2 + 1);

	return 1; // Success
}

static const struct vout_set_s *vout_find_mode_by_vout_idx(uint8_t vout_idx)
{
	char check_name[16] = "outputmode\0\0";
	char *outputmode;
	const struct vout_set_s *vset = NULL;
	unsigned int width, height, fr;
	int i = 0;

	if (vout_idx)
		check_name[10] = '1' + vout_idx;
	if (vout_idx >= VOUT_MAX_CNT)
		return NULL;

	outputmode = env_get(check_name);

	if (!outputmode)
		return NULL;

	if (parse_resolution(outputmode, &width, &height, &fr)) {
		vout_sets_full[vout_idx].width = width;
		vout_sets_full[vout_idx].height = height;
		vout_sets_full[vout_idx].field_height = height;
		vout_log("vout[%u] parse: W=%u, H=%u, FR=%uHz\n", vout_idx, width, height, fr);
		return &vout_sets_full[vout_idx];
	}

	vset = vout_sets_dft;
	for (i = 0; i < sizeof(vout_sets_dft) / sizeof(struct vout_set_s); i++) {
		if (strncmp(outputmode, vset->name, strlen(vset->name)) == 0) {
			vout_sets_full[vout_idx].width = vset->width;
			vout_sets_full[vout_idx].height = vset->height;
			vout_sets_full[vout_idx].field_height = vset->field_height;
			vout_log("vout[%u] match: W=%u, H=%u, %s\n", vout_idx, vset->width,
				 vset->height, vset->height == vset->field_height ? "P" : "I");
			return &vout_sets_full[vout_idx];
		}
		vset++;
	}

	vout_log("mode: %s not found\n", outputmode);
	return NULL;
}

static unsigned int vout_env2uint(const char *name, int base)
{
	return (unsigned int)env_get_ulong(name, base, 0);
}

static void vout_vinfo_init(ulong width, ulong height, ulong field_height)
{
	vout_info.width = width;
	vout_info.height = height;
	vout_info.field_height = field_height;
	vout_info.vd_base = (void *)get_fb_addr();
	vout_info.vl_bpix = (unsigned char)vout_env2uint("display_bpp", 10);
	vout_info.vd_color_fg = vout_env2uint("display_color_fg", 0);
	vout_info.vd_color_bg = vout_env2uint("display_color_bg", 0);
}

static void vout_axis_init(ulong w, ulong h)
{
	ulong width = w;
	ulong height = h;

	env_set_ulong("display_width", width);
	env_set_ulong("display_height", height);
}

static void vout_vmode_init(void)
{
	uint8_t check_connector_idx = 0;

	const struct vout_set_s *vset = NULL;
	ulong width = 0;
	ulong height = 0;
	ulong field_height = 0;
#ifdef CONFIG_AML_LCD
	struct aml_lcd_drv_s *pdrv;
	uint16_t connector;
	unsigned char venc_index = 0xff;
#endif
	uint index = 0;

	index = get_osd_layer();

	if (is_vpp0(index)) {
		if (is_keystone_enable_for_txhd2() ||
		    is_keystone_enable_for_t6d())
			check_connector_idx = 1;
		else
			check_connector_idx = 0;
	} else if (is_vpp1(index)) {
		check_connector_idx = 1;
	} else if (is_vpp2(index)) {
		check_connector_idx = 2;
	} else {
		vout_log("%s, layer%d is not supported\n", __func__, index);
	}

	vset = vout_find_mode_by_vout_idx(check_connector_idx);
	if (!vset)
		return;

#ifdef CONFIG_AML_LCD
	connector = vout_connector_check(check_connector_idx);
	if ((connector & CONNECTOR_DEV_MASK) == CONNECTOR_DEV_LCD)
		venc_index = connector & CONNECTOR_ENC_IDX_MASK;
	if (venc_index != 0xff) {
		pdrv = aml_lcd_get_driver(venc_index);
		if (pdrv)
			vout_info.cur_enc_ppc = pdrv->config.timing.ppc;
		printf("%s cur_enc_ppc = %d\n", __func__, vout_info.cur_enc_ppc);
	}
#endif

	width = vset->width;
	height = vset->height;
	field_height = vset->field_height;

	vout_axis_init(width, height);

	vout_vinfo_init(width, height, field_height);
}

static int getenv_int(char *env, int def)
{
	if (env_get(env) == NULL)
		return def;
	else
		return my_atoi(env_get(env));
}

static int get_window_axis(int *axis)
{
	int ret = 0;
	char *mode = NULL;
	int def_x, def_y, def_w, def_h;
	uint index = 0;

	index = get_osd_layer();
	if (is_vpp0(index))
		mode = env_get("outputmode");
	else if (is_vpp1(index))
		mode = env_get("outputmode2");
	else if (is_vpp2(index))
		mode = env_get("outputmode3");
	else
		vout_log("%s, osd%d is not supported\n", __func__, index);

	def_x = 0;
	def_y = 0;
	def_w = vout_info.width;
	def_h = vout_info.height;

	if (!mode) {
		axis[0] = def_x;
		axis[1] = def_y;
		axis[2] = def_w;
		axis[3] = def_h;
		vout_log("%s, osd%d cannot get env outputmode\n",
			 __func__, index);
		return -1;
	}
	/* adjust reproduction ratio */
	if (strncmp(mode, "480i", 4) == 0) {
		axis[0] = getenv_int("480i_x", def_x);
		axis[1] = getenv_int("480i_y", def_y);
		axis[2] = getenv_int("480i_w", def_w);
		axis[3] = getenv_int("480i_h", def_h);
	} else if (strcmp(mode, "480cvbs") == 0) {
		axis[0] = getenv_int("480cvbs_x", def_x);
		axis[1] = getenv_int("480cvbs_y", def_y);
		axis[2] = getenv_int("480cvbs_w", def_w);
		axis[3] = getenv_int("480cvbs_h", def_h);
	} else if (strncmp(mode, "480p", 4) == 0) {
		axis[0] = getenv_int("480p_x", def_x);
		axis[1] = getenv_int("480p_y", def_y);
		axis[2] = getenv_int("480p_w", def_w);
		axis[3] = getenv_int("480p_h", def_h);
	} else if (strncmp(mode, "640x480p60hz", 12) == 0) {
		axis[0] = getenv_int("640x480p60hz_x", def_x);
		axis[1] = getenv_int("640x480p60hz_y", def_y);
		axis[2] = getenv_int("640x480p60hz_w", def_w);
		axis[3] = getenv_int("640x480p60hz_h", def_h);
	} else if (strncmp(mode, "576i", 4) == 0) {
		axis[0] = getenv_int("576i_x", def_x);
		axis[1] = getenv_int("576i_y", def_y);
		axis[2] = getenv_int("576i_w", def_w);
		axis[3] = getenv_int("576i_h", def_h);
	} else if (strcmp(mode, "576cvbs") == 0) {
		axis[0] = getenv_int("576cvbs_x", def_x);
		axis[1] = getenv_int("576cvbs_y", def_y);
		axis[2] = getenv_int("576cvbs_w", def_w);
		axis[3] = getenv_int("576cvbs_h", def_h);
	} else if (strncmp(mode, "576p", 4) == 0) {
		axis[0] = getenv_int("576p_x", def_x);
		axis[1] = getenv_int("576p_y", def_y);
		axis[2] = getenv_int("576p_w", def_w);
		axis[3] = getenv_int("576p_h", def_h);
	} else if (strncmp(mode, "720p", 4) == 0) {
		axis[0] = getenv_int("720p_x", def_x);
		axis[1] = getenv_int("720p_y", def_y);
		axis[2] = getenv_int("720p_w", def_w);
		axis[3] = getenv_int("720p_h", def_h);
	} else if (strncmp(mode, "768p", 4) == 0) {
		axis[0] = getenv_int("768p_x", def_x);
		axis[1] = getenv_int("768p_y", def_y);
		axis[2] = getenv_int("768p_w", def_w);
		axis[3] = getenv_int("768p_h", def_h);
	} else if (strncmp(mode, "1080i", 5) == 0) {
		axis[0] = getenv_int("1080i_x", def_x);
		axis[1] = getenv_int("1080i_y", def_y);
		axis[2] = getenv_int("1080i_w", def_w);
		axis[3] = getenv_int("1080i_h", def_h);
	} else if (strncmp(mode, "1080p", 5) == 0) {
		axis[0] = getenv_int("1080p_x", def_x);
		axis[1] = getenv_int("1080p_y", def_y);
		axis[2] = getenv_int("1080p_w", def_w);
		axis[3] = getenv_int("1080p_h", def_h);
	} else if (strncmp(mode, "1920x1080p", 10) == 0) {
		axis[0] = getenv_int("1080p_x", def_x);
		axis[1] = getenv_int("1080p_y", def_y);
		axis[2] = getenv_int("1080p_w", def_w);
		axis[3] = getenv_int("1080p_h", def_h);
	} else if (strncmp(mode, "2160p", 5) == 0) {
		axis[0] = getenv_int("2160p_x", def_x);
		axis[1] = getenv_int("2160p_y", def_y);
		axis[2] = getenv_int("2160p_w", def_w);
		axis[3] = getenv_int("2160p_h", def_h);
	} else if (strncmp(mode, "smpte",5) == 0) {
		axis[0] = getenv_int("4k2ksmpte_x", def_x);
		axis[1] = getenv_int("4k2ksmpte_y", def_y);
		axis[2] = getenv_int("4k2ksmpte_w", def_w);
		axis[3] = getenv_int("4k2ksmpte_h", def_h);
	} else if (strncmp(mode, "3840x1080p", 10) == 0) {
		axis[0] = getenv_int("3840x1080p_x", def_x);
		axis[1] = getenv_int("3840x1080p_y", def_y);
		axis[2] = getenv_int("3840x1080p_w", def_w);
		axis[3] = getenv_int("3840x1080p_h", def_h);
	} else if (strncmp(mode, "3840x2160p", 10) == 0) {
		axis[0] = getenv_int("2160p_x", def_x);
		axis[1] = getenv_int("2160p_y", def_y);
		axis[2] = getenv_int("2160p_w", def_w);
		axis[3] = getenv_int("2160p_h", def_h);
	} else if (strncmp(mode, "7680x4320p", 10) == 0) {
		axis[0] = getenv_int("4320p_x", def_x);
		axis[1] = getenv_int("4320p_y", def_y);
		axis[2] = getenv_int("4320p_w", def_w);
		axis[3] = getenv_int("4320p_h", def_h);
	} else if (strncmp(mode, "panel",5) == 0) {
		axis[0] = getenv_int("panel_x", def_x);
		axis[1] = getenv_int("panel_y", def_y);
		axis[2] = getenv_int("panel_w", def_w);
		axis[3] = getenv_int("panel_h", def_h);
	} else {
		axis[0] = getenv_int("1080p_x", def_x);
		axis[1] = getenv_int("1080p_y", def_y);
		axis[2] = getenv_int("1080p_w", def_w);
		axis[3] = getenv_int("1080p_h", def_h);
	}

	return ret;
}

struct vinfo_s *vout_get_current_vinfo(void)
{
	struct vinfo_s *info = &vout_info;

	vout_logl();

	return info;
}

int vout_get_current_axis(int *axis)
{
	return get_window_axis(axis);
}

void vout_vinfo_dump(void)
{
	struct vinfo_s *info = NULL;

	vout_logl();
	info = vout_get_current_vinfo();
	vout_log("vinfo.vd_base: 0x%p\n", info->vd_base);
	vout_log("vinfo.width: %d\n", info->width);
	vout_log("vinfo.height: %d\n", info->height);
	vout_log("vinfo.field_height: %d\n", info->field_height);
	vout_log("vinfo.vl_bpix: %d\n", info->vl_bpix);
	vout_log("vinfo.vd_color_fg: %d\n", info->vd_color_fg);
	vout_log("vinfo.vd_color_bg: %d\n", info->vd_color_bg);

	if (vout_conf_check())
		return;
	if (vout_conf->reg_dump)
		vout_conf->reg_dump();
}

static void vout_reg_dump(void)
{
	unsigned int reg;

	if (vout_conf_check())
		return;

	reg = vout_conf->viu_mux_reg;
	if (reg == VOUT_REG_INVALID)
		return;

	vout_log("viu_mux: 0x%x = 0x%08x\n", reg, vout_reg_read(reg));
}

static unsigned int vout_viu1_mux = VIU_MUX_MAX;
static unsigned int vout_viu2_mux = VIU_MUX_MAX;
static void vout_viu_mux_default(int index, unsigned int mux_sel)
{
	unsigned int clk_bit = 0xff, clk_sel = 0;
	unsigned int vout_viu_sel = 0xf;
	unsigned int venc_sel = mux_sel;

	char *projector_mux = env_get("vout_projector_mux");
	int vout_projector_mux = 0;

	if (projector_mux) {
		if (strncmp(projector_mux, "en", 2) == 0)
			vout_projector_mux = 1;
	}

	switch (index) {
	case VOUT_VIU2_SEL:
		if (vout_conf->viu_valid[1]) {
			/* set cts_vpu_clkc to 200MHz*/
			vout_clk_setb(HHI_VPU_CLKC_CNTL, 2, 9, 3);
			vout_clk_setb(HHI_VPU_CLKC_CNTL, 1, 0, 1);
			vout_clk_setb(HHI_VPU_CLKC_CNTL, 1, 8, 3);
			clk_sel = 1;
		}
		if (venc_sel == vout_viu1_mux)
			vout_viu1_mux = VIU_MUX_MAX;
		vout_viu2_mux = venc_sel;
		break;
	case VOUT_VIU1_SEL:
		clk_sel = 0;
		if (venc_sel == vout_viu2_mux) {
			if (vout_conf->viu_valid[1])
				vout_clk_setb(HHI_VPU_CLKC_CNTL, 0, 8, 1);
			vout_viu2_mux = VIU_MUX_MAX;
		}
		vout_viu1_mux = venc_sel;
		break;
	default:
		break;
	}
	vout_viu_sel = (vout_viu1_mux | (vout_viu2_mux << 2));

	switch (venc_sel) {
	case VIU_MUX_ENCL:
		clk_bit = 1;
		break;
	case VIU_MUX_ENCI:
		clk_bit = 2;
		break;
	case VIU_MUX_ENCP:
		clk_bit = 0;
		break;
	default:
		break;
	}

	if (get_cpu_id().family_id == MESON_CPU_MAJOR_ID_T6D) {
		vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, 0, 0, 4);
		vout_reg_setb(VPU_VENCX_CLK_CTRL, 0, 0, 3);
		vout_log("T6D:%s\n", __func__);
	} else {
		vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, vout_viu_sel, 0, 4);
		if (vout_conf->viu_valid[1]) {
			if (clk_bit < 0xff)
				vout_reg_setb(VPU_VENCX_CLK_CTRL, clk_sel, clk_bit, 1);
		}
	}

	if (vout_projector_mux && get_cpu_id().family_id ==
		MESON_CPU_MAJOR_ID_T6D) {
		vout_reg_setb(VPP_MISC_T6D, 1, 27, 1);
		vout_reg_setb(OSD2_HDR2_MATRIXI_EN_CTRL, 0, 0, 1);
		vout_log("T6D: %s: vout_projector_mux %d\n", __func__, vout_projector_mux);
	}
}

static void vout_viu_mux_s6(int index, unsigned int mux_sel)
{
	unsigned int clk_bit = 0xff, clk_sel = 0;
	unsigned int vout_viu_sel = 0xf;
	unsigned int venc_sel = mux_sel;

	switch (index) {
	case VOUT_VIU2_SEL:
		if (vout_conf->viu_valid[1]) {
			/* set cts_vpu_clkc to 250MHz*/
			vout_clk_setb(CLKCTRL_VPU_CLKC_CTRL, 4, 9, 3);
			vout_clk_setb(CLKCTRL_VPU_CLKC_CTRL, 3, 0, 7);
			vout_clk_setb(CLKCTRL_VPU_CLKC_CTRL, 1, 8, 1);
			clk_sel = 1;
		}
		if (venc_sel == vout_viu1_mux)
			vout_viu1_mux = VIU_MUX_MAX;
		vout_viu2_mux = venc_sel;
		break;
	case VOUT_VIU1_SEL:
		clk_sel = 0;
		if (venc_sel == vout_viu2_mux) {
			if (vout_conf->viu_valid[1])
				vout_clk_setb(HHI_VPU_CLKC_CNTL, 0, 8, 1);
			vout_viu2_mux = VIU_MUX_MAX;
		}
		vout_viu1_mux = venc_sel;
		break;
	default:
		break;
	}
	vout_viu_sel = (vout_viu1_mux | (vout_viu2_mux << 2));

	switch (venc_sel) {
	case VIU_MUX_ENCL:
		clk_bit = 1;
		break;
	case VIU_MUX_ENCI:
		clk_bit = 2;
		break;
	case VIU_MUX_ENCP:
		clk_bit = 0;
		break;
	default:
		break;
	}

	vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, vout_viu_sel, 0, 4);
	if (vout_conf->viu_valid[1]) {
		if (clk_bit < 0xff)
			vout_reg_setb(VPU_VENCX_CLK_CTRL, clk_sel, clk_bit, 1);
	}
}

static void vout_viu_mux_t7(int index, unsigned int mux_sel)
{
	unsigned int viu_bit = 0xff, venc_idx;

	switch (index) {
	case VOUT_VIU1_SEL:
		viu_bit = 0;
		break;
	case VOUT_VIU2_SEL:
		viu_bit = 2;
		break;
	case VOUT_VIU3_SEL:
		viu_bit = 4;
		break;
	default:
		vout_log("error: %s: invalid index %d\n", __func__, index);
		return;
	}
	venc_idx = (mux_sel >> 4) & 0xf;

	/* viu_mux: viu0_sel: 0=venc0, 1=venc1, 2=venc2, 3=invalid */
	vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, venc_idx, viu_bit, 2);
}

static void vout_viu_mux_t3(int index, unsigned int mux_sel)
{
	unsigned int viu_bit = 0xff, venc_idx;

	switch (index) {
	case VOUT_VIU1_SEL:
		viu_bit = 0;
		break;
	case VOUT_VIU2_SEL:
		viu_bit = 2;
		break;
	default:
		vout_log("error: %s: invalid index %d\n", __func__, index);
		return;
	}
	venc_idx = (mux_sel >> 4) & 0xf;

	/* viu_mux: viu0_sel: 0=venc0, 1=venc1, 2=venc2, 3=invalid */
	vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, venc_idx, viu_bit, 2);
}

static void vout_viu_mux_s5(int index, unsigned int mux_sel)
{
	unsigned int viu_bit = 0xff, venc_idx;

	switch (index) {
	case VOUT_VIU1_SEL:
		viu_bit = 0;
		break;
	default:
		vout_log("error: %s: invalid index %d\n", __func__, index);
		return;
	}
	venc_idx = (mux_sel >> 4) & 0xf;

	/* viu_mux: viu0_sel: 0=venc0, 3=invalid */
	vout_reg_setb(VPU_VIU_VENC_MUX_CTRL, venc_idx, viu_bit, 2);
}

void vout_viu_mux(int index, unsigned int mux_sel)
{
	if (vout_conf_check())
		return;

	if (vout_conf->viu_mux)
		vout_conf->viu_mux(index, mux_sel);
}

void vout_init(void)
{
	vout_logl();
	vout_vmode_init();
}

/* **********************************
 * vout match data
 * **********************************
 */
static struct vout_conf_s vout_config_single = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 0,
	.viu_valid[2] = 0,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_default,
	.reg_dump = vout_reg_dump,
};

static struct vout_conf_s vout_config_dual = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 1,
	.viu_valid[2] = 0,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_default,
	.reg_dump = vout_reg_dump,
};

static struct vout_conf_s vout_config_triple = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 1,
	.viu_valid[2] = 1,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_t7,
	.reg_dump = vout_reg_dump,
};

static struct vout_conf_s vout_config_dual_t3 = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 1,
	.viu_valid[2] = 0,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_t3,
	.reg_dump = vout_reg_dump,
};

static struct vout_conf_s vout_config_c3 = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 0,
	.viu_valid[2] = 0,

	.viu_mux_reg = VOUT_REG_INVALID,

	.viu_mux = NULL,
	.reg_dump = NULL,
};

static struct vout_conf_s vout_config_single_s5 = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 0,
	.viu_valid[2] = 0,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_s5,
	.reg_dump = vout_reg_dump,
};

static struct vout_conf_s vout_config_dual_s6 = {
	.viu_valid[0] = 1,
	.viu_valid[1] = 1,
	.viu_valid[2] = 0,

	.viu_mux_reg = VPU_VIU_VENC_MUX_CTRL,

	.viu_mux = vout_viu_mux_s6,
	.reg_dump = vout_reg_dump,
};

void vout_probe(void)
{
	switch (get_cpu_id().family_id) {
	case MESON_CPU_MAJOR_ID_G12A:
	case MESON_CPU_MAJOR_ID_G12B:
	case MESON_CPU_MAJOR_ID_TL1:
	case MESON_CPU_MAJOR_ID_TM2:
	case MESON_CPU_MAJOR_ID_SM1:
	case MESON_CPU_MAJOR_ID_T5:
	case MESON_CPU_MAJOR_ID_T5D:
	case MESON_CPU_MAJOR_ID_T6D:
		vout_conf = &vout_config_dual;
		break;
	case MESON_CPU_MAJOR_ID_T7:
		vout_conf = &vout_config_triple;
		vout_reg_write(VPU_VIU_VENC_MUX_CTRL, 0x3f);
		break;
	case MESON_CPU_MAJOR_ID_T3:
	case MESON_CPU_MAJOR_ID_T5M:
		vout_conf = &vout_config_dual_t3;
		vout_reg_write(VPU_VIU_VENC_MUX_CTRL, 0x3f);
		break;
	case MESON_CPU_MAJOR_ID_C3:
		vout_conf = &vout_config_c3;
		break;
	case MESON_CPU_MAJOR_ID_S5:
		vout_conf = &vout_config_single_s5;
		vout_reg_write(VPU_VIU_VENC_MUX_CTRL, 0x3f);
		break;
	case MESON_CPU_MAJOR_ID_S6:
		vout_conf = &vout_config_dual_s6;
		break;
	default:
		vout_conf = &vout_config_single;
		break;
	}
}
