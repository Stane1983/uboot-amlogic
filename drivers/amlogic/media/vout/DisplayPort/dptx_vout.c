// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/cpu_id.h>
#include <fdtdec.h>
#ifdef CONFIG_SECURE_POWER_CONTROL
#include <asm/arch/pwr_ctrl.h>
#endif
#include <string.h>
#include <amlogic/pm.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPTX_export.h>
#include "dptx_common.h"

uint8_t dptx_print_level;
static unsigned int dptx_debug_test_flag;

struct dptx_chip_data_s *dptx_dev_data;
struct dptx_drv_s *dptx_drivers[DPTX_MAX_DRV];

static char *g_dt_addr = (char *)0x01000000;
char *dptx_pm_name[DPTX_MAX_DRV] = {"dptx_0_pm", "dptx_1_pm"};

static struct dptx_chip_data_s dptx_data_t7 = {
	.chip_type = DPTX_CHIP_T7,
	.chip_name = "T7",
	.drv_max = 2,
	.offset_venc      = {0x0, (0x600 << 2)},
	.offset_venc_if   = {0x0, (0x500 << 2)},
	.offset_venc_data = {0x0, (0x100 << 2)},

	.link_rate    = DP_LINK_RATE_HBR,
	.TPS_support  = BIT(0),
	.DACP_support = BIT(2),
	.venc_clk_msr_id = {222, 221},
};

static void dptx_chip_detect(void)
{
	uint32_t cpu_type;
	// uint32_t rev_type;

	cpu_type = get_cpu_id().family_id;
	// rev_type = get_cpu_id().chip_rev;
	switch (cpu_type) {
	case MESON_CPU_MAJOR_ID_T7:
		dptx_dev_data = &dptx_data_t7;
		break;
	default:
		dptx_dev_data = NULL;
		return;
	}

	DPTXPR(0, LOG_I, "chip: %u %s", dptx_dev_data->chip_type, dptx_dev_data->chip_name);
}

static struct dptx_drv_s *dptx_driver_check_valid(u8 index)
{
	if (index >= DPTX_MAX_DRV)
		return NULL;

	if (!dptx_drivers[index] || !(dptx_drivers[index]->status & DPTX_STA_PROBE_DONE)) {
		DPTXPR(0, LOG_E, "invalid DPTX%d config", index);
		return NULL;
	}
	return dptx_drivers[index];
}

static char *get_current_env_connector(unsigned char cnt_idx)
{
	char cnt_name[20];

	sprintf(cnt_name, "connector%hu_type", cnt_idx);

	return env_get(cnt_name);
}

int dptx_connector_check(struct dptx_drv_s *dptx)
{
	char dptx_cnt_name[8];
	uint8_t i;
	char *cnt_type_name;

	sprintf(dptx_cnt_name, "DP-%c", 'A' + dptx->idx);
	for (i = 0; i < 3; i++) {
		cnt_type_name = get_current_env_connector(i);
		if (!cnt_type_name)
			continue;

		if (strcmp(dptx_cnt_name, cnt_type_name))
			continue;

		dptx->viu_sel = i + 1;
		DPTXPR(dptx->idx, LOG_I, "%s: connector[%hu] - (%s)\n", __func__, i, cnt_type_name);
		return 0;
	}

	DPTXPR(dptx->idx, LOG_E, "%s: no connector to DisplayPort TX: %s", __func__, dptx_cnt_name);
	return -1;
}

int dptx_outputmode_check(struct dptx_drv_s *dptx, char *mode)
{
	char vmd_name[24];
	uint8_t i;

	for (i = 0; i < DPTX_DRV_VMODE_MAX; i++) {
		if (!(dptx->vmode_mgr.vmodes[i].flag & VMODE_FLAG_VALID))
			continue;
		memset(vmd_name, 0, 24 * sizeof(char));
		__str_add_vmode(dptx, vmd_name, i, 1);
		// printf("%s\n", vmd_name);
		if (strcmp(mode, vmd_name)) {
			dptx->vmode_mgr.vmode_sel_idx = i;
			DPTXPR(dptx->idx, LOG_I, "%s: timing: %u, %s", __func__, i, mode);
			return 0;
		}
	}

	dptx->vmode_mgr.vmode_sel_idx = U8_MAX;
	DPTXPR(dptx->idx, LOG_I, "%s: timing: %s (not match)", __func__, mode);
	return 0;
}

static void dptx_update_outputmode(struct dptx_drv_s *dptx)
{
	char cnt_type_str[16] = "connectorX_type";
	char mode_env_name[16] = "outputmode\0\0";
	char temp_str[20] = "EDP-X";
	uint8_t i;

	if (!(dptx->status & DPTX_STA_LINK_ON)) {
		DPTXPR(dptx->idx, LOG_E, "curr link not on");
		dptx->vmode_mgr.vmode_sel_idx = U8_MAX;
	}

	temp_str[4] = 'A' + dptx->idx;

	for (i = 0; i < DPTX_MAX_VOUT; i++) {
		cnt_type_str[9] = '0' + i;
		if (env_get(cnt_type_str) && strcmp(temp_str, env_get(cnt_type_str)) == 0) {
			dptx->viu_sel = 1 + i;
			if (i)
				mode_env_name[11] = '1' + i;
			break;
		}
	}
	if (!dptx->viu_sel)
		return;

	memset(temp_str, 0, 20 * sizeof(char));
	__str_add_vmode(dptx, temp_str, dptx->vmode_mgr.vmode_sel_idx, 1);

	env_set(mode_env_name, temp_str);

	DPTXPR(dptx->idx, LOG_I, "%s=%s: set %s=%s",
		cnt_type_str, (char *)env_get(cnt_type_str), mode_env_name, temp_str);
}

/***** DISPLAYPORT DRIVER PREPARE / ENABLE / REMOVE ********/
static uint8_t dptx_module_prepare(struct dptx_drv_s *dptx, char *mode)
{
	if ((dptx->status & DPTX_STA_PROBE_DONE) == 0)
		return 0;

	if ((dptx->status & DPTX_STA_DRV_READY) == 0)
		dptx_driver_ready(dptx);

	dptx_drv_check_HPD(dptx);

	if (dptx->status & DPTX_STA_HPD_HIGH)
		dptx_drv_start(dptx);

	dptx_update_outputmode(dptx);

	if (dptx->status & DPTX_STA_LINK_ON) {
		switch (dptx->vmode_mgr.vmode_cfmt_sel) {
		case DPTX_CFMT_YCbCr422_8bit:
		case DPTX_CFMT_YCbCr422_10bit:
		case DPTX_CFMT_YCbCr422_12bit:
		case DPTX_CFMT_YCbCr444_8bit:
		case DPTX_CFMT_YCbCr444_10bit:
		case DPTX_CFMT_YCbCr444_12bit:
		case DPTX_CFMT_YCbCr420_8bit:
		case DPTX_CFMT_YCbCr420_10bit:
		case DPTX_CFMT_YCbCr420_12bit:
		case DPTX_CFMT_Y_only_8bit:
		case DPTX_CFMT_Y_only_10bit:
		case DPTX_CFMT_Y_only_12bit:
			return 2; // YUV
		case DPTX_CFMT_RGB_6bit:
		case DPTX_CFMT_RGB_8bit:
		case DPTX_CFMT_RGB_10bit:
		case DPTX_CFMT_RGB_12bit:
		default:
			return 0; // RGB
		}
	}
	return 0;
}

static uint8_t dptx_module_enable(struct dptx_drv_s *dptx, char *mode)
{
	if ((dptx->status & DPTX_STA_PROBE_DONE) == 0)
		return 0;

	if ((dptx->status & DPTX_STA_DRV_READY) == 0)
		dptx_driver_ready(dptx);

	dptx_drv_check_HPD(dptx);

	if (dptx->status & DPTX_STA_HPD_HIGH)
		dptx_drv_start(dptx);

	dptx_update_outputmode(dptx);

	if (dptx->status & DPTX_STA_LINK_ON) {
		dptx_drv_disp_on(dptx);

		switch (dptx->vmode_mgr.vmode_cfmt_sel) {
		case DPTX_CFMT_YCbCr422_8bit:
		case DPTX_CFMT_YCbCr422_10bit:
		case DPTX_CFMT_YCbCr422_12bit:
		case DPTX_CFMT_YCbCr444_8bit:
		case DPTX_CFMT_YCbCr444_10bit:
		case DPTX_CFMT_YCbCr444_12bit:
		case DPTX_CFMT_YCbCr420_8bit:
		case DPTX_CFMT_YCbCr420_10bit:
		case DPTX_CFMT_YCbCr420_12bit:
		case DPTX_CFMT_Y_only_8bit:
		case DPTX_CFMT_Y_only_10bit:
		case DPTX_CFMT_Y_only_12bit:
			return 2; // YUV
		case DPTX_CFMT_RGB_6bit:
		case DPTX_CFMT_RGB_8bit:
		case DPTX_CFMT_RGB_10bit:
		case DPTX_CFMT_RGB_12bit:
		default:
			return 0; // RGB
		}
	}
	return 0;
}

static void dptx_module_disable(struct dptx_drv_s *dptx)
{
	if (dptx->status & DPTX_STA_DISP_ON)
		dptx_drv_disp_off(dptx);

	if (dptx->status & DPTX_STA_LINK_ON)
		dptx_driver_close(dptx);
	//lcd_update_outputmode(pdrv);
}

/***** DISPLAYPORT DRIVER PREPARE / ENABLE / REMOVE END  ********/

/***** DISPLAYPORT DRIVER PROBE & REMOVE ********/
static struct dptx_drv_s *dptx_driver_add(u8 index)
{
	struct dptx_drv_s *dptx;
	int init_once = 0;

	if (index >= dptx_dev_data->drv_max) {
		DPTXPR(0, LOG_E, "%s: invalid index: %d\n", __func__, index);
		return NULL;
	}

	if (!dptx_drivers[index]) {
		dptx_drivers[index] = (struct dptx_drv_s *)malloc(sizeof(struct dptx_drv_s));
		if (!dptx_drivers[index]) {
			DPTXPR(index, LOG_E, "%s: Not enough memory", __func__);
			return NULL;
		}
		init_once = 1;
	}

	dptx = dptx_drivers[index];
	memset(dptx, 0, sizeof(struct dptx_drv_s));
	dptx->idx = index;

	if (init_once) {
		//dptx->power_on_suspend = 1;
		//dptx->dev_pm_ops = dev_register_pm(lcd_pm_name[index],
		//	 &dptx_drivers_suspend, &dptx_drivers_resume, &dptx_driver_poweroff);
	}

	/* default config */
	dptx->data = dptx_dev_data;

	return dptx;
}

static uint8_t dptx_config_load_from_dts(char *dt_addr, struct dptx_drv_s *dptx)
{
#ifdef CONFIG_OF_LIBFDT
	int parent_offset;
	char *propdata, snode[20];
	uint8_t i;

	sprintf(snode, "/DisplayPort_TX_%u", dptx->idx);
	parent_offset = fdt_path_offset(dt_addr, snode);
	if (parent_offset < 0) {
		DPTXPR(dptx->idx, LOG_E, "load %s node: %s", snode, fdt_strerror(parent_offset));
		return -1;
	}

	DPTXPR(dptx->idx, LOG_V, "load %s node", snode);

	/* check lcd status enable or not */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (!propdata) {
		DPTXPR(dptx->idx, LOG_E, "failed to get status, default disable");
		return -1;
	}
	if (strcmp(propdata, "okay")) {
		DPTXPR(dptx->idx, LOG_I, "status disabled, exit");
		return -1;
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "device_name", NULL);
	if (propdata && (strlen(propdata) != 0)) {
		strncpy(dptx->dev_name, propdata, 31);
		DPTXPR(dptx->idx, LOG_I, "device name: %s", dptx->dev_name);
	} else {
		strcpy(dptx->dev_name, "Amlogic DisplayPort TX");
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "HPD-ignore", NULL);
	if (propdata) {
		dptx->hpd_ignore = (u16)be32_to_cpup((u32 *)propdata);
		DPTXPR(dptx->idx, LOG_I, "HPD ignore: %ums", dptx->hpd_ignore);
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "dptx-gpio-PWR-name", NULL);
	if (propdata && (strlen(propdata) != 0)) {
		strncpy(dptx->PWR_gpio_name, propdata, (DPTX_GPIO_NAME_MAX - 1));
		DPTXPR(dptx->idx, LOG_I, "PWR GPIO: %s", dptx->PWR_gpio_name);
	} else {
		strcpy(dptx->PWR_gpio_name, "invalid");
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "dptx-gpio-CFG1-name", NULL);
	if (propdata && (strlen(propdata) != 0)) {
		strncpy(dptx->CFG1_gpio_name, propdata, (DPTX_GPIO_NAME_MAX - 1));
		DPTXPR(dptx->idx, LOG_I, "CFG1 GPIO: %s", dptx->CFG1_gpio_name);
	} else {
		strcpy(dptx->CFG1_gpio_name, "invalid");
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "dptx-gpio-CFG2-name", NULL);
	if (propdata && (strlen(propdata) != 0)) {
		strncpy(dptx->CFG2_gpio_name, propdata, (DPTX_GPIO_NAME_MAX - 1));
		DPTXPR(dptx->idx, LOG_I, "CFG2 GPIO: %s", dptx->CFG2_gpio_name);
	} else {
		strcpy(dptx->CFG2_gpio_name, "invalid");
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "drive-strength-lut", NULL);
	if (propdata) {
		for (i = 0; i < 30; i++) {
			dptx->phy_cfg.level_to_phy_lut[i / 10][i % 3] =
				(u16)be32_to_cpup(((u32 *)propdata) + i);
		}
	} else {
		DPTXPR(dptx->idx, LOG_E, "DPCD level to PHY LUT not set");
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "assigned-link-rate", NULL);
	if (propdata) {
		dptx->user_link_rate = (u16)be32_to_cpup((u32 *)propdata);
		DPTXPR(dptx->idx, LOG_I, "assigned link rate: %u", dptx->user_link_rate);
	} else {
		dptx->user_link_rate = 0;
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "assigned-lane-count", NULL);
	if (propdata) {
		dptx->user_lane_count = (u16)be32_to_cpup((u32 *)propdata);
		DPTXPR(dptx->idx, LOG_I, "assigned lane count: %u", dptx->user_lane_count);
	} else {
		dptx->user_lane_count = 0xff;
	}

	dptx->user_color_format = 0xff;
#endif
	return 0;
}

static void dptx_user_env_load(struct dptx_drv_s *dptx)
{
	char env_str[20];
	uint32_t temp;
	char *_str;

	sprintf(env_str, "dptx%u_link_rate", dptx->idx);
	temp = env_get_ulong(env_str, 10, 0);
	if (temp) {
		dptx->user_link_rate = temp;
		DPTXPR(dptx->idx, LOG_I, "user env: %s: %u", env_str, temp);
	}

	sprintf(env_str, "dptx%u_lane_count", dptx->idx);
	temp = env_get_ulong(env_str, 10, 0);
	if (temp) {
		dptx->user_lane_count = temp;
		DPTXPR(dptx->idx, LOG_I, "user env %s: %u", env_str, temp);
	}

	sprintf(env_str, "dptx%u_hpd_ignore", dptx->idx);
	temp = env_get_ulong(env_str, 10, 0);
	if (temp) {
		dptx->hpd_ignore = temp;
		DPTXPR(dptx->idx, LOG_I, "user env: %s: %u", env_str, temp);
	}

	sprintf(env_str, "dptx%u_color_format", dptx->idx);
	_str = env_get(env_str);
	if (_str) {
		for (temp = 0;; temp++) {
			if (dptx_cfmt_table[temp].cfmt_id == DPTX_CFMT_invalid)
				break;
			if (strcmp(_str, dptx_cfmt_table[temp].name) == 0) {
				dptx->user_color_format = temp;
				DPTXPR(dptx->idx, LOG_I, "user env: %s: %s", env_str, _str);
				break;
			}
		}
	}
}

static int dptx_driver_remove(u8 index)
{
	if (index >= dptx_dev_data->drv_max)
		return 0;

	if (!dptx_drivers[index])
		return 0;

	if (dptx_drivers[index]->dev_pm_ops)
		dev_unregister_pm(dptx_drivers[index]->dev_pm_ops);

	free(dptx_drivers[index]);
	dptx_drivers[index] = NULL;

	return 0;
}

static int dptx_config_probe(void)
{
	struct dptx_drv_s *dptx;
	unsigned char i;
	int ret;
	char ctrl_name[] = "dptx0_ctrl";

#ifdef CONFIG_DTB_MEM_ADDR
	g_dt_addr = (char *)CONFIG_DTB_MEM_ADDR;
#endif
//#ifdef CONFIG_AML_LCD_PXP
//	g_dt_addr = (char *)0x06000000;
//#endif

#ifdef CONFIG_OF_LIBFDT
	if (fdt_check_header(g_dt_addr) < 0) {
		DPTXPR(0, LOG_E, "check dts: %s", fdt_strerror(fdt_check_header(g_dt_addr)));
		return -1;
	}
#endif

	for (i = 0; i < dptx_dev_data->drv_max; i++) {
		dptx = dptx_driver_add(i);
		if (!dptx)
			continue;

		ret = dptx_config_load_from_dts(g_dt_addr, dptx);
		if (ret)
			dptx_driver_remove(i);

		dptx_user_env_load(dptx);
		dptx_clk_config_probe(dptx);
		dptx_if_IP_probe(dptx);
		ctrl_name[4] = '0' + dptx->idx;
		env_set(ctrl_name, NULL);
		dptx->status = DPTX_STA_PROBE_DONE;
	}

	return 0;
}

static void dptx_power_domain_off(struct dptx_chip_data_s *dptx_data_p)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	switch (dptx_dev_data->chip_type) {
	case DPTX_CHIP_T7:
		pwr_ctrl_psci_smc(PM_EDP0, 0);
		pwr_ctrl_psci_smc(PM_EDP1, 0);
		break;
	default:
		return;
	}
	if (dptx_print_level >= LOG_I)
		DPTXPR(0, LOG_I, "power domain off\n");
#endif
}

int dptx_probe(void)
{
	dptx_print_level = env_get_ulong("dptx_debug", 16, 1);
	DPTXPR(0, LOG_I, "DPTX print level: %u", dptx_print_level);

	dptx_debug_test_flag = env_get_ulong("dptx_test", 10, 0);

	dptx_chip_detect();
	if (!dptx_dev_data) {
		DPTXPR(0, LOG_E, "%s: invalid dptx chip data\n", __func__);
		return -1;
	}

	dptx_phy_probe(dptx_dev_data);
	dptx_venc_probe(dptx_dev_data);
	// dptx_debug_probe(dptx_dev_data);
	if (dptx_config_probe())
		return -1;

	dptx_power_domain_off(dptx_dev_data);

	return 0;
}

int dptx_remove(void)
{
	u8 i;

	if (!dptx_dev_data)
		return 0;

	for (i = 0; i < DPTX_MAX_DRV; i++)
		dptx_driver_remove(i);

	return 0;
}

void dptx_driver_probe(void)
{
	dptx_probe();
}

uint8_t dptx_driver_prepare(u8 index, char *mode)
{
	struct dptx_drv_s *dptx;

	if (!mode) {
		DPTXPR(index, LOG_E, "%s: mode is NULL", __func__);
		return 0;
	}

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return 0;

	if (dptx->status & DPTX_STA_DRV_READY) {
		DPTXPR(dptx->idx, LOG_I, "already prepared");
		return 0;
	}

	return dptx_module_prepare(dptx, mode);
}

uint8_t dptx_driver_enable(u8 index, char *mode)
{
	struct dptx_drv_s *dptx;

	if (!mode) {
		DPTXPR(index, LOG_E, "%s: mode is NULL", __func__);
		return 0;
	}

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return 0;

	if (dptx->status & DPTX_STA_DISP_ON) {
		DPTXPR(dptx->idx, LOG_I, "already enabled");
		return 0;
	}

	return dptx_module_enable(dptx, mode);
}

void dptx_driver_disable(u8 index)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	if ((dptx->status & DPTX_STA_LINK_ON) == 0) {
		DPTXPR(dptx->idx, LOG_I, "already disabled");
		return;
	}

	dptx_module_disable(dptx);
}

void dptx_driver_info(u8 index)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	dptx_info_print(dptx);
}

void dptx_driver_reg_print(u8 index)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	dptx_reg_print(dptx);
}

void dptx_driver_test(u8 index, u8 num)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	dptx_debug_test(dptx, num);
}

unsigned int dptx_driver_outputmode_check(u8 index, char *mode)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return 0;

	// dptx_connector_mode_check(dptx, mode);

	return 0;
}

void dptx_driver_list_vmode(u8 index)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	dptx_list_support_vmode(dptx);
}

void dptx_driver_set_vmode(u8 index, u8 num)
{
	struct dptx_drv_s *dptx;

	dptx = dptx_driver_check_valid(index);
	if (!dptx)
		return;

	dptx_user_set_vmode(dptx, num);
}
