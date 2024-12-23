// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "env.h"
#include <command.h>

static struct num_str_s lcd_type_match_table[] = {
	{LCD_RGB,      "rgb"},
	{LCD_LVDS,     "lvds"},
	{LCD_VBYONE,   "vbyone"},
	{LCD_MIPI,     "mipi"},
	{LCD_MLVDS,    "minilvds"},
	{LCD_P2P,      "p2p"},
	{LCD_EDP,      "edp"},
	{LCD_BT656,    "bt656"},
	{LCD_BT1120,   "bt1120"},
	{LCD_TYPE_MAX, "invalid"},
};

int lcd_type_str_to_type(const char *str)
{
	int type = LCD_TYPE_MAX;
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_type_match_table); i++) {
		if (!strcmp(str, lcd_type_match_table[i].str)) {
			type = lcd_type_match_table[i].num;
			break;
		}
	}
	return type;
}

char *lcd_type_type_to_str(int type)
{
	char *name = lcd_type_match_table[LCD_TYPE_MAX].str;
	int i;

	for (i = 0; i < ARRAY_SIZE(lcd_type_match_table); i++) {
		if (type == lcd_type_match_table[i].num) {
			name = lcd_type_match_table[i].str;
			break;
		}
	}
	return name;
}

static char *lcd_mode_table[] = {
	"tv",
	"tablet",
	"invalid",
};

int lcd_mode_str_to_mode(const char *str)
{
	int mode;

	for (mode = 0; mode < ARRAY_SIZE(lcd_mode_table); mode++) {
		if (!strcmp(str, lcd_mode_table[mode]))
			break;
	}
	return mode;
}

char *lcd_mode_mode_to_str(int mode)
{
	return lcd_mode_table[mode];
}

static void lcd_config_load_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_detail_timing_s *ptiming = pdrv->config.timing.dft_timing;
	struct phy_attr_s *phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct lcd_config_s *pconf = &pdrv->config;
	union lcd_ctrl_config_u *pctrl;
	int i = 0;

	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) == 0)
		return;

	LCDPR("[%d]: %s, %s\n",
	      pdrv->index, pconf->basic.model_name,
	      lcd_type_type_to_str(pconf->basic.lcd_type));

	for (i = 0; i < pdrv->config.timing.num_timings; i++) {
		ptiming = pdrv->config.timing.timings[i];
		printf("config timing[%d]:\n", i);
		lcd_detail_timing_print(pdrv, ptiming);
	}

	LCDPR("pll_flag = %d\n", pconf->timing.pll_flag);
	LCDPR("clk_mode = %d\n", pconf->timing.clk_mode);
	LCDPR("pixel_clk = %d\n", ptiming->pixel_clk);
	LCDPR("custom_pinmux = %d\n", pconf->custom_pinmux);

	printf("\nphy_config:\n");
	lcd_phy_cfg_print(phy_cfg);
	for (i = 0; i < phy_cfg->group_num; i++) {
		phy = pdrv->config.phy_cfg.phys[i];
		if (!phy)
			continue;
		printf("phy_attr[%d]:\n", i);
		lcd_phy_attr_print(phy, phy_cfg->lane_num);
	}

	pctrl = &pconf->control;
	if (pconf->basic.lcd_type == LCD_RGB) {
		LCDPR("type = %d\n", pctrl->rgb_cfg.type);
		LCDPR("clk_pol = %d\n", pctrl->rgb_cfg.clk_pol);
		LCDPR("de_valid = %d\n", pctrl->rgb_cfg.de_valid);
		LCDPR("sync_valid = %d\n", pctrl->rgb_cfg.sync_valid);
		LCDPR("rb_swap = %d\n", pctrl->rgb_cfg.rb_swap);
		LCDPR("bit_swap = %d\n", pctrl->rgb_cfg.bit_swap);
	} else if (pconf->basic.lcd_type == LCD_LVDS) {
		LCDPR("lvds_repack = %d\n", pctrl->lvds_cfg.lvds_repack);
		LCDPR("pn_swap = %d\n", pctrl->lvds_cfg.pn_swap);
		LCDPR("dual_port = %d\n", pctrl->lvds_cfg.dual_port);
		LCDPR("port_swap = %d\n", pctrl->lvds_cfg.port_swap);
		LCDPR("lane_reverse = %d\n", pctrl->lvds_cfg.lane_reverse);
		LCDPR("phy_vswing = 0x%x\n", pctrl->lvds_cfg.phy_vswing);
		LCDPR("phy_preem = 0x%x\n", pctrl->lvds_cfg.phy_preem);
	} else if (pconf->basic.lcd_type == LCD_VBYONE) {
		LCDPR("lane_count = %d\n", pctrl->vbyone_cfg.lane_count);
		LCDPR("byte_mode = %d\n", pctrl->vbyone_cfg.byte_mode);
		LCDPR("region_num = %d\n", pctrl->vbyone_cfg.region_num);
		LCDPR("color_fmt = %d\n", pctrl->vbyone_cfg.color_fmt);
		LCDPR("phy_vswing = 0x%x\n", pctrl->vbyone_cfg.phy_vswing);
		LCDPR("phy_preem = 0x%x\n", pctrl->vbyone_cfg.phy_preem);
	} else if (pconf->basic.lcd_type == LCD_MLVDS) {
		LCDPR("channel_num = %d\n", pctrl->mlvds_cfg.channel_num);
		LCDPR("channel_sel0 = 0x%x\n", pctrl->mlvds_cfg.channel_sel0);
		LCDPR("channel_sel1 = 0x%x\n", pctrl->mlvds_cfg.channel_sel1);
		LCDPR("clk_phase = 0x%x\n", pctrl->mlvds_cfg.clk_phase);
		LCDPR("phy_vswing = 0x%x\n", pctrl->mlvds_cfg.phy_vswing);
		LCDPR("phy_preem = 0x%x\n", pctrl->mlvds_cfg.phy_preem);
	} else if (pconf->basic.lcd_type == LCD_P2P) {
		LCDPR("p2p_type = %d\n", pctrl->p2p_cfg.p2p_type);
		LCDPR("lane_num = %d\n", pctrl->p2p_cfg.lane_num);
		LCDPR("channel_sel0 = 0x%x\n", pctrl->p2p_cfg.channel_sel0);
		LCDPR("channel_sel1 = 0x%x\n", pctrl->p2p_cfg.channel_sel1);
		LCDPR("phy_vswing = 0x%x\n", pctrl->p2p_cfg.phy_vswing);
		LCDPR("phy_preem = 0x%x\n", pctrl->p2p_cfg.phy_preem);
	} else if (pconf->basic.lcd_type == LCD_MIPI) {
		if (pctrl->mipi_cfg.check_en) {
			LCDPR("check_reg = 0x%02x\n", pctrl->mipi_cfg.check_reg);
			LCDPR("check_cnt = %d\n", pctrl->mipi_cfg.check_cnt);
		}
		LCDPR("lane_num = %d\n", pctrl->mipi_cfg.lane_num);
		LCDPR("bit_rate_max = %d\n", pctrl->mipi_cfg.bit_rate_max);
		// LCDPR("pclk_lanebyteclk_factor = %d\n", pctrl->mipi_cfg.factor_numerator);
		LCDPR("operation_mode_init = %d\n", pctrl->mipi_cfg.operation_mode_init);
		LCDPR("operation_mode_disp = %d\n", pctrl->mipi_cfg.operation_mode_display);
		LCDPR("video_mode_type = %d\n", pctrl->mipi_cfg.video_mode_type);
		LCDPR("clk_always_hs = %d\n", pctrl->mipi_cfg.clk_always_hs);
		// LCDPR("phy_switch = %d\n", pctrl->mipi_cfg.phy_switch);
		LCDPR("extern_init = %d\n", pctrl->mipi_cfg.extern_init);
	} else if (pconf->basic.lcd_type == LCD_EDP) {
		LCDPR("max_lane_count = %d\n", pctrl->edp_cfg.max_lane_count);
		LCDPR("max_link_rate  = %d\n", pctrl->edp_cfg.max_link_rate);
		LCDPR("training_mode  = %d\n", pctrl->edp_cfg.training_mode);
		LCDPR("edid_en        = %d\n", pctrl->edp_cfg.edid_en);
		// LCDPR("sync_clk_mode  = %d\n", pctrl->edp_cfg.sync_clk_mode);
		LCDPR("lane_count     = %d\n", pctrl->edp_cfg.lane_count);
		LCDPR("link_rate      = %d\n", pctrl->edp_cfg.link_rate);
		LCDPR("phy_vswing = 0x%x\n", pctrl->edp_cfg.phy_vswing_preset);
		LCDPR("phy_preem  = 0x%x\n", pctrl->edp_cfg.phy_preem_preset);
	}
}

int lcd_base_config_load_from_dts(char *dt_addr, struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_OF_LIBFDT
	struct lcd_config_s *pconf = &pdrv->config;
	int parent_offset;
	char *propdata, *p, snode[10];
	const char *str;
	unsigned int temp;
	char str_info[128];
	int str_info_len = 0, i;

	if (pdrv->index == 0)
		sprintf(snode, "/lcd");
	else
		sprintf(snode, "/lcd%d", pdrv->index);
	parent_offset = fdt_path_offset(dt_addr, snode);
	if (parent_offset < 0) {
		LCDERR("[%d]: not find %s node: %s\n",
		       pdrv->index, snode, fdt_strerror(parent_offset));
		return -1;
	}

	/* check lcd status enable or not */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get status, default disable\n", pdrv->index);
		return -1;
	}
	if (strcmp(propdata, "okay")) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%d]: status disabled, exit\n", pdrv->index);
		return -1;
	}

	/* check lcd_mode & lcd_key_valid */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "mode", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get mode\n", pdrv->index);
		return -1;
	}
	pdrv->mode = lcd_mode_str_to_mode(propdata);

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "key_valid", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get key_valid\n", pdrv->index);
		pdrv->key_valid = 0;
	} else {
		pdrv->key_valid = (unsigned char)(be32_to_cpup((u32 *)propdata));
	}

	/* check lcd_clk_path */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "clk_path", NULL);
	if (!propdata)
		pdrv->clk_path = 0;
	else
		pdrv->clk_path = (unsigned char)(be32_to_cpup((u32 *)propdata));

	temp = env_get_ulong("lcd_clk_path", 10, 0xffff);
	if (temp != 0xffff) {
		if (temp)
			pdrv->clk_path = 1;
		else
			pdrv->clk_path = 0;
		LCDPR("[%d]: lcd_clk_path env set clk_path: %d\n",
		      pdrv->index, pdrv->clk_path);
	}

	i = 0;
	propdata = (char *)fdt_getprop(dt_addr, parent_offset,
				       "lcd_cpu_gpio_names", NULL);
	if (!propdata) {
		LCDPR("[%d]: failed to get lcd_cpu_gpio_names\n", pdrv->index);
	} else {
		p = propdata;
		while (i < LCD_CPU_GPIO_NUM_MAX) {
			str = p;
			if (strlen(str) == 0)
				break;
			strlcpy(pconf->power.cpu_gpio[i], str, LCD_CPU_GPIO_NAME_MAX);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: i=%d, gpio=%s\n",
				      pdrv->index, i, pconf->power.cpu_gpio[i]);
			}
			p += strlen(p) + 1;
			i++;
		}
	}

	for (; i < LCD_CPU_GPIO_NUM_MAX; i++)
		strcpy(pconf->power.cpu_gpio[i], "invalid");

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "config_check_glb", NULL);
	if (!propdata)
		pdrv->config_check_glb = 0;
	else
		pdrv->config_check_glb = be32_to_cpup((u32 *)propdata);

	str_info_len += sprintf(str_info + str_info_len, "clk_path: %d, ", pdrv->clk_path);
	sprintf(str_info + str_info_len, "cfg_chk_glb: %d", pdrv->config_check_glb);
	LCDPR("[%d]: drv_ver: %s(%d-%s), lcd_mode: %s, key_valid: %d, %s\n",
	      pdrv->index, LCD_DRV_VERSION, pdrv->data->chip_type, pdrv->data->chip_name,
	      lcd_mode_mode_to_str(pdrv->mode), pdrv->key_valid, str_info);

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "display_timing_req_min", NULL);
	if (!propdata) {
		pdrv->disp_req.alert_level = 0;
		pdrv->disp_req.hswbp_vid = 0;
		pdrv->disp_req.hfp_vid = 0;
		pdrv->disp_req.vswbp_vid = 0;
		pdrv->disp_req.vfp_vid = 0;
	} else {
		pdrv->disp_req.alert_level = be32_to_cpup((u32 *)propdata);
		pdrv->disp_req.hswbp_vid   = be32_to_cpup((((u32 *)propdata) + 1));
		pdrv->disp_req.hfp_vid     = be32_to_cpup((((u32 *)propdata) + 2));
		pdrv->disp_req.vswbp_vid   = be32_to_cpup((((u32 *)propdata) + 3));
		pdrv->disp_req.vfp_vid     = be32_to_cpup((((u32 *)propdata) + 4));
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: find display_timing_req_min: alert_level:%d\n"
				"hswbp:%d, hfp:%d, vswbp:%d, vfp:%d\n",
				pdrv->index, pdrv->disp_req.alert_level,
				pdrv->disp_req.hswbp_vid, pdrv->disp_req.hfp_vid,
				pdrv->disp_req.vswbp_vid, pdrv->disp_req.vfp_vid);
		}
	}

#endif
	return 0;
}

int lcd_base_config_load_from_bsp(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_dft_config_s *dft_conf;
	unsigned int temp;
	char (*lcd_gpio)[LCD_CPU_GPIO_NAME_MAX];
	int i;

	dft_conf = pdrv->data->dft_conf[pdrv->index];
	if (!dft_conf) {
		LCDERR("%s: dft_conf is NULL\n", __func__);
		return -1;
	}

	pdrv->mode = dft_conf->mode;
	pdrv->key_valid = dft_conf->key_valid;
	pdrv->clk_path = dft_conf->clk_path;
	LCDPR("[%d]: detect mode: %s, key_valid: %d, clk_path: %d\n",
	      pdrv->index, lcd_mode_mode_to_str(pdrv->mode),
	      pdrv->key_valid, pdrv->clk_path);

	temp = env_get_ulong("lcd_clk_path", 10, 0xffff);
	if (temp != 0xffff) {
		if (temp)
			pdrv->clk_path = 1;
		else
			pdrv->clk_path = 0;
		LCDPR("[%d]: lcd_clk_path flag set clk_path: %d\n",
		      pdrv->index, pdrv->clk_path);
	}

	i = 0;
	lcd_gpio = pdrv->data->dft_conf[pdrv->index]->lcd_gpio;
	if (!lcd_gpio) {
		LCDERR("[%d]: %s lcd_gpio is null\n", pdrv->index, __func__);
		return -1;
	}
	while (i < LCD_CPU_GPIO_NUM_MAX) {
		if (strcmp(lcd_gpio[i], "invalid") == 0)
			break;
		strcpy(pdrv->config.power.cpu_gpio[i], lcd_gpio[i]);
		i++;
	}

	for (; i < LCD_CPU_GPIO_NUM_MAX; i++)
		strcpy(pdrv->config.power.cpu_gpio[i], "invalid");

	return 0;
}

static char *lcd_rgb_pinmux_str[] = {
	"lcd_rgb_on_pin",         /* 0 */
	"lcd_rgb_de_on_pin",      /* 1 */
	"lcd_rgb_sync_on_pin"     /* 2 */
};

static int lcd_pinmux_load_rgb(struct lcd_pinmux_ctrl_s *pinmux, struct lcd_config_s *pconf)
{
	char propname[30];
	int pinmux_index = 0, set_cnt = 0, clr_cnt = 0;
	unsigned int i, j;

	/* data */
	pinmux_index = 0;
	sprintf(propname, "%s", lcd_rgb_pinmux_str[pinmux_index]);
	for (i = 0; i < LCD_PINMX_MAX; i++) {
		if (!pinmux)
			break;
		if (!pinmux->name)
			break;
		if (strncmp(pinmux->name, "invalid", 7) == 0)
			break;
		if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
				pconf->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
				pconf->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		pinmux++;
	}

	/* DE */
	if (pconf->control.rgb_cfg.de_valid) {
		pinmux_index = 1;
		sprintf(propname, "%s", lcd_rgb_pinmux_str[pinmux_index]);
		for (i = 0; i < LCD_PINMX_MAX; i++) {
			if (!pinmux)
				break;
			if (!pinmux->name)
				break;
			if (strncmp(pinmux->name, "invalid", 7) == 0)
				break;
			if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
						break;
					pconf->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
					pconf->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
					set_cnt++;
				}
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
						break;
					pconf->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
					pconf->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
					clr_cnt++;
				}
				break;
			}
			pinmux++;
		}
	}

	/* sync */
	if (pconf->control.rgb_cfg.sync_valid) {
		pinmux_index = 2;
		sprintf(propname, "%s", lcd_rgb_pinmux_str[pinmux_index]);
		for (i = 0; i < LCD_PINMX_MAX; i++) {
			if (!pinmux)
				break;
			if (!pinmux->name)
				break;
			if (strncmp(pinmux->name, "invalid", 7) == 0)
				break;
			if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
						break;
					pconf->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
					pconf->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
					set_cnt++;
				}
				for (j = 0; j < LCD_PINMUX_NUM; j++) {
					if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
						break;
					pconf->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
					pconf->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
					clr_cnt++;
				}
				break;
			}
			pinmux++;
		}
	}

	if (set_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_clr[clr_cnt][1] = 0x0;
	}

	return 0;
}

static int lcd_custom_pinmux_load_config(struct lcd_pinmux_ctrl_s *pinmux,
					 struct lcd_config_s *pconf)
{
	char propname[35];
	int set_cnt = 0, clr_cnt = 0;
	int i, j;

	sprintf(propname, "%s", pconf->basic.model_name);
	for (i = 0; i < LCD_PINMX_MAX; i++) {
		if (!pinmux)
			break;
		if (!pinmux->name)
			break;
		if (strncmp(pinmux->name, "invalid", 7) == 0)
			break;
		if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
				pconf->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
				pconf->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		pinmux++;
	}

	if (set_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_clr[clr_cnt][1] = 0x0;
	}
	return 0;
}

static int lcd_pinmux_load_config(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_pinmux_ctrl_s *pinmux;
	struct lcd_config_s *pconf = &pdrv->config;
	char propname[30];
	int set_cnt = 0, clr_cnt = 0;
	unsigned int i, j;
	int ret = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	if (!pdrv->data->dft_conf[pdrv->index]) {
		LCDERR("[%d]: %s: dft_conf is NULL\n", pdrv->index, __func__);
		return -1;
	}
	pinmux = pdrv->data->dft_conf[pdrv->index]->lcd_pinmux;
	if (!pinmux) {
		LCDERR("[%d]: %s: lcd_pinmux is NULL\n", pdrv->index, __func__);
		return -1;
	}

	if (pconf->basic.lcd_type == LCD_RGB) {
		ret = lcd_pinmux_load_rgb(pinmux, pconf);
		if (ret)
			return -1;
		goto lcd_pinmux_load_config_next;
	}

	if (pconf->custom_pinmux) {
		ret = lcd_custom_pinmux_load_config(pinmux, pconf);
		if (ret)
			return -1;
		goto lcd_pinmux_load_config_next;
	}

	switch (pconf->basic.lcd_type) {
	case LCD_VBYONE:
		sprintf(propname, "lcd_vbyone_pin");
		break;
	case LCD_MLVDS:
		sprintf(propname, "lcd_minilvds_pin");
		break;
	case LCD_P2P:
		if (pconf->control.p2p_cfg.p2p_type == P2P_USIT)
			sprintf(propname, "lcd_p2p_usit_pin");
		else
			sprintf(propname, "lcd_p2p_pin");
		break;
	case LCD_EDP:
		sprintf(propname, "lcd_edp_pin");
		break;
	default:
		pconf->pinmux_set[0][0] = LCD_PINMUX_END;
		pconf->pinmux_set[0][1] = 0x0;
		pconf->pinmux_clr[0][0] = LCD_PINMUX_END;
		pconf->pinmux_clr[0][1] = 0x0;
		return 0;
	}
	for (i = 0; i < LCD_PINMX_MAX; i++) {
		if (!pinmux->name)
			break;
		if (strncmp(pinmux->name, "invalid", 7) == 0)
			break;
		if (strncmp(pinmux->name, propname, strlen(propname)) == 0) {
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_set[j][0] = pinmux->pinmux_set[j][0];
				pconf->pinmux_set[j][1] = pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++) {
				if (pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				pconf->pinmux_clr[j][0] = pinmux->pinmux_clr[j][0];
				pconf->pinmux_clr[j][1] = pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		pinmux++;
	}
	if (set_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		pconf->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		pconf->pinmux_clr[clr_cnt][1] = 0x0;
	}

lcd_pinmux_load_config_next:
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (pdrv->config.pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			LCDPR("pinmux_set: %d, 0x%08x\n",
			      pdrv->config.pinmux_set[i][0],
			      pdrv->config.pinmux_set[i][1]);
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (pdrv->config.pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			LCDPR("pinmux_clr: %d, 0x%08x\n",
			      pdrv->config.pinmux_clr[i][0],
			      pdrv->config.pinmux_clr[i][1]);
			i++;
		}
	}

	return 0;
}

static void lcd_ss_config_fix(struct aml_lcd_drv_s *pdrv)
{
	int i = 0;

	//fix ss in detail timing and phy_attr if not config
	for (i = 0; i < pdrv->config.phy_cfg.group_num; i++) {
		if (pdrv->config.phy_cfg.phys[i]->ss.freq == 255)
			pdrv->config.phy_cfg.phys[i]->ss.freq = pdrv->config.timing.ss_freq;
		if (pdrv->config.phy_cfg.phys[i]->ss.level == 255)
			pdrv->config.phy_cfg.phys[i]->ss.level = pdrv->config.timing.ss_level;
		if (pdrv->config.phy_cfg.phys[i]->ss.mode == 255)
			pdrv->config.phy_cfg.phys[i]->ss.mode = pdrv->config.timing.ss_mode;
	}

	for (i = 0; i < pdrv->config.timing.num_timings; i++) {
		if (pdrv->config.timing.timings[i]->ss_level == 255)
			pdrv->config.timing.timings[i]->ss_level = pdrv->config.timing.ss_level;
		if (pdrv->config.timing.timings[i]->ss_freq == 255)
			pdrv->config.timing.timings[i]->ss_freq = pdrv->config.timing.ss_freq;
		if (pdrv->config.timing.timings[i]->ss_mode == 255)
			pdrv->config.timing.timings[i]->ss_mode = pdrv->config.timing.ss_mode;
	}
}

int lcd_get_dts_panel_node_ofst(unsigned char drv_idx)
{
	int node_ofst;
	char *dt_addr = lcd_get_dt_addr();
	char parent_str[6], type_str[12], propname[30];
	char *panel_type;

	if (drv_idx == 0) {
		sprintf(parent_str, "/lcd");
		sprintf(type_str, "panel_type");
	} else {
		sprintf(parent_str, "/lcd%d", drv_idx);
		sprintf(type_str, "panel%d_type", drv_idx);
	}
	panel_type = env_get(type_str);
	if (!panel_type) {
		LCDERR("[%d]: %s: no env: %s\n", drv_idx, __func__, type_str);
		return -1;
	}
	snprintf(propname, 30, "%s/%s", parent_str, panel_type);

	node_ofst = fdt_path_offset(dt_addr, propname);
	if (node_ofst < 0) {
		LCDERR("[%d]: %s: not find %s node: %s\n",
		       drv_idx, __func__, propname, fdt_strerror(node_ofst));
		return -1;
	}
	return node_ofst;
}

static int lcd_power_load_from_dts(struct aml_lcd_drv_s *pdrv, char *dt_addr, int child_offset)
{
	struct lcd_power_step_s *pstep;
	char *propdata;
	unsigned int i, j, temp;

	pstep = pdrv->config.power.power_on_step;
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "power_on_step", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get power_on_step\n", pdrv->index);
		return 0;
	}
	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		j = 4 * i;
		temp = be32_to_cpup((((u32 *)propdata) + j));
		pstep[i].type = temp;
		if (temp == 0xff)
			break;
		temp = be32_to_cpup((((u32 *)propdata) + j + 1));
		pstep[i].index = temp;
		temp = be32_to_cpup((((u32 *)propdata) + j + 2));
		pstep[i].value = temp;
		temp = be32_to_cpup((((u32 *)propdata) + j + 3));
		pstep[i].delay = temp;
		if (pstep[i].type == LCD_POWER_TYPE_CLK_SS) {
			temp = pstep[i].value;
			pdrv->config.timing.ss_freq = temp & 0xf;
			pdrv->config.timing.ss_mode = (temp >> 4) & 0xf;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: clk_ss value=0x%x: ss_freq=%d, ss_mode=%d\n",
				      pdrv->index, temp,
				      pdrv->config.timing.ss_freq,
				      pdrv->config.timing.ss_mode);
			}
		} else if (pstep[i].type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pstep[i].index);
#endif
		} else if (pstep[i].type == LCD_POWER_TYPE_MUTE) {
			pdrv->status |= LCD_STATUS_PRE_MUTE;
		}
		i++;
	}

	pstep = pdrv->config.power.power_off_step;
	propdata = (char *)fdt_getprop(dt_addr, child_offset, "power_off_step", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get power_off_step\n", pdrv->index);
		return 0;
	}
	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		j = 4 * i;
		temp = be32_to_cpup((((u32 *)propdata) + j));
		pstep[i].type = temp;
		if (temp == 0xff)
			break;
		temp = be32_to_cpup((((u32 *)propdata) + j + 1));
		pstep[i].index = temp;
		temp = be32_to_cpup((((u32 *)propdata) + j + 2));
		pstep[i].value = temp;
		temp = be32_to_cpup((((u32 *)propdata) + j + 3));
		pstep[i].delay = temp;
		if (pstep[i].type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pstep[i].index);
#endif
		}
		i++;
	}

	return 0;
}

static int lcd_config_load_from_dts(char *dt_addr, struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_OF_LIBFDT
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_detail_timing_s *ptiming;
	union lcd_ctrl_config_u *pctrl = &pdrv->config.control;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = NULL;
	int child_offset;
	char type_str[20];
	char *propdata;
	char str_info[128];
	int i, str_info_len = 0, len;
	unsigned int temp, lcd_bits = 24;

	if (pdrv->index == 0)
		sprintf(type_str, "panel_type");
	else
		sprintf(type_str, "panel%d_type", pdrv->index);

	char *panel_type = env_get(type_str);

	child_offset = lcd_get_dts_panel_node_ofst(pdrv->index);
	if (child_offset < 0)
		return -1;

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "model_name", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get model_name\n", pdrv->index);
		strlcpy(pconf->basic.model_name, panel_type, sizeof(pconf->basic.model_name) - 1);
	} else {
		strlcpy(pconf->basic.model_name, propdata, sizeof(pconf->basic.model_name) - 1);
	}
	pconf->basic.model_name[sizeof(pconf->basic.model_name) - 1] = '\0';

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "interface", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get interface\n", pdrv->index);
		return -1;
	}
	pconf->basic.lcd_type = lcd_type_str_to_type(propdata);
	LCDPR("load dts config: %s, lcd_type: %s(%d)\n",
	      pconf->basic.model_name, propdata, pconf->basic.lcd_type);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "config_check", NULL);
	if (!propdata) {
		pconf->basic.config_check = 0; //follow config_check_glb
	} else {
		temp = be32_to_cpup((u32 *)propdata);
		pconf->basic.config_check = temp ? 0x3 : 0x2;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%d]: find config_check: %d\n", pdrv->index, temp);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "basic_setting", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get basic_setting\n", pdrv->index);
		return -1;
	}

	ptiming = lcd_timing_alloc(pdrv);
	if (!ptiming) {
		LCDERR("[%d]: failed to alloc timing memory\n", pdrv->index);
		return -1;
	}
	memset(ptiming, 0, sizeof(*ptiming));

	ptiming->h_active = be32_to_cpup((u32 *)propdata);
	ptiming->v_active = be32_to_cpup((((u32 *)propdata) + 1));
	ptiming->h_period = be32_to_cpup((((u32 *)propdata) + 2));
	ptiming->v_period = be32_to_cpup((((u32 *)propdata) + 3));
	lcd_bits = be32_to_cpup((((u32 *)propdata) + 4)) * 3;
	pconf->basic.screen_width = be32_to_cpup((((u32 *)propdata) + 5));
	pconf->basic.screen_height = be32_to_cpup((((u32 *)propdata) + 6));

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "range_setting", NULL);
	if (!propdata) {
		ptiming->h_period_min = ptiming->h_period;
		ptiming->h_period_max = ptiming->h_period;
		ptiming->v_period_min = ptiming->v_period;
		ptiming->v_period_max = ptiming->v_period;
		ptiming->pclk_min = 0;
		ptiming->pclk_max = 0;
	} else {
		ptiming->h_period_min = be32_to_cpup((u32 *)propdata);
		ptiming->h_period_max = be32_to_cpup((((u32 *)propdata) + 1));
		ptiming->v_period_min = be32_to_cpup((((u32 *)propdata) + 2));
		ptiming->v_period_max = be32_to_cpup((((u32 *)propdata) + 3));
		ptiming->pclk_min = be32_to_cpup((((u32 *)propdata) + 4));
		ptiming->pclk_max = be32_to_cpup((((u32 *)propdata) + 5));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "range_frame_rate", NULL);
	if (!propdata) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%d]: no range_frame_rate\n", pdrv->index);
		ptiming->frame_rate_min = 0;
		ptiming->frame_rate_max = 0;
	} else {
		ptiming->frame_rate_min = be32_to_cpup((u32 *)propdata);
		ptiming->frame_rate_max = be32_to_cpup((((u32 *)propdata) + 1));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ppc_mode", NULL);
	if (!propdata) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%d]: no ppc_mode, set dft 1\n", pdrv->index);
		pconf->timing.ppc = 1;
	} else {
		pconf->timing.ppc = (unsigned short)(be32_to_cpup((u32 *)propdata));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "lcd_timing", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get lcd_timing\n", pdrv->index);
		return -1;
	}
	ptiming->hsync_width = (unsigned short)(be32_to_cpup((u32 *)propdata));
	ptiming->hsync_bp    = (unsigned short)(be32_to_cpup((((u32 *)propdata) + 1)));
	ptiming->hsync_fp = ptiming->h_period - ptiming->h_active -
			ptiming->hsync_width - ptiming->hsync_bp;
	ptiming->hsync_pol   = (unsigned short)(be32_to_cpup((((u32 *)propdata) + 2)));
	ptiming->vsync_width = (unsigned short)(be32_to_cpup((((u32 *)propdata) + 3)));
	ptiming->vsync_bp    = (unsigned short)(be32_to_cpup((((u32 *)propdata) + 4)));
	ptiming->vsync_fp = ptiming->v_period - ptiming->v_active -
			ptiming->vsync_width - ptiming->vsync_bp;
	ptiming->vsync_pol   = (unsigned short)(be32_to_cpup((((u32 *)propdata) + 5)));

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "pre_de", NULL);
	if (!propdata) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDERR("failed to get pre_de\n");
		pconf->timing.pre_de_h = 0;
		pconf->timing.pre_de_v = 0;
	} else {
		pconf->timing.pre_de_h = (unsigned char)(be32_to_cpup((u32 *)propdata));
		pconf->timing.pre_de_v = (unsigned char)(be32_to_cpup((((u32 *)propdata) + 1)));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "clk_attr", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get clk_attr\n", pdrv->index);
		ptiming->fr_adjust_type = 0xff;
		pconf->timing.ss_level = 0;
		pconf->timing.pll_flag = 1;
		ptiming->pixel_clk = 60;
	} else {
		ptiming->fr_adjust_type = (unsigned char)(be32_to_cpup((u32 *)propdata));
		temp = be32_to_cpup((((u32 *)propdata) + 1));
		pconf->timing.ss_level = temp & 0xff;
		pconf->timing.ss_freq = (temp >> 8) & 0xf;
		pconf->timing.ss_mode = (temp >> 12) & 0xf;
		temp = (unsigned char)(be32_to_cpup((((u32 *)propdata) + 2)));
		pconf->timing.pll_flag = temp & 0xf;
		ptiming->pixel_clk = be32_to_cpup((((u32 *)propdata) + 3));
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "clk_mode", NULL);
	if (!propdata) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDERR("[%d]: no clk_mode\n", pdrv->index);
		pconf->timing.clk_mode = 0;
	} else {
		pconf->timing.clk_mode = (unsigned char)(be32_to_cpup((u32 *)propdata));
	}
	ptiming->lcd_bits = lcd_bits;
	ptiming->switch_type = LCD_VMODE_SWITCH_NONE;
	ptiming->ss_force = 0;
	ptiming->ss_freq = pconf->timing.ss_freq;
	ptiming->ss_level = pconf->timing.ss_freq;
	ptiming->ss_mode = pconf->timing.ss_mode;
	pconf->timing.dft_timing = ptiming;
	lcd_clk_frame_rate_init(ptiming);
	lcd_config_timing_check(pdrv, ptiming);
	lcd_default_to_basic_timing_init_config(pdrv);

	str_info_len += sprintf(str_info + str_info_len, "ppc:%d, ",
			pconf->timing.ppc);
	str_info_len += sprintf(str_info + str_info_len, "clk_mode:%d, ",
			pconf->timing.clk_mode);
	if (pconf->timing.pre_de_h || pconf->timing.pre_de_h) {
		str_info_len += sprintf(str_info + str_info_len, "pre_de:%d,%d, ",
				pconf->timing.pre_de_h, pconf->timing.pre_de_h);
	}
	str_info_len += sprintf(str_info + str_info_len, "cfg_chk:0x%x, ",
			pconf->basic.config_check);
	sprintf(str_info + str_info_len, "cus_pinmux:%d", pconf->custom_pinmux);
	LCDPR("[%d]: load dts config: %s, %s, %dbit, %dx%d, %s\n",
	      pdrv->index, pconf->basic.model_name,
	      lcd_type_type_to_str(pconf->basic.lcd_type),
	      lcd_bits, ptiming->h_active, ptiming->v_active,
	      str_info);

	switch (pconf->basic.lcd_type) {
	case LCD_LVDS:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "lvds_attr", &len);
		if (!propdata) {
			LCDERR("[%d]: failed to get lvds_attr\n", pdrv->index);
			return -1;
		}
		len = len / 4;
		if (len == 5) {
			pctrl->lvds_cfg.lvds_repack = be32_to_cpup((u32 *)propdata);
			pctrl->lvds_cfg.dual_port   = be32_to_cpup((((u32 *)propdata) + 1));
			pctrl->lvds_cfg.pn_swap     = be32_to_cpup((((u32 *)propdata) + 2));
			pctrl->lvds_cfg.port_swap   = be32_to_cpup((((u32 *)propdata) + 3));
			pctrl->lvds_cfg.lane_reverse = be32_to_cpup((((u32 *)propdata) + 4));
		} else if (len == 4) {
			pctrl->lvds_cfg.lvds_repack = be32_to_cpup((u32 *)propdata);
			pctrl->lvds_cfg.dual_port   = be32_to_cpup((((u32 *)propdata) + 1));
			pctrl->lvds_cfg.pn_swap     = be32_to_cpup((((u32 *)propdata) + 2));
			pctrl->lvds_cfg.port_swap   = be32_to_cpup((((u32 *)propdata) + 3));
			pctrl->lvds_cfg.lane_reverse = 0;
		} else {
			LCDERR("[%d]: invalid lvds_attr parameters cnt: %d\n",
			       pdrv->index, len);
			return -1;
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_attr", &len);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get phy_attr\n", pdrv->index);
			pctrl->lvds_cfg.phy_vswing = LVDS_PHY_VSWING_DFT;
			pctrl->lvds_cfg.phy_preem  = LVDS_PHY_PREEM_DFT;
		} else {
			pctrl->lvds_cfg.phy_vswing = be32_to_cpup((u32 *)propdata);
			pctrl->lvds_cfg.phy_preem  = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: set phy vswing_level=0x%x, preem_level=0x%x\n",
				      pdrv->index, pctrl->lvds_cfg.phy_vswing,
				      pctrl->lvds_cfg.phy_preem);
			}
		}

		phy_cfg->vswing_level = pctrl->lvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->lvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->lvds_cfg.phy_preem;
		break;
	case LCD_VBYONE:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "vbyone_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get vbyone_attr\n", pdrv->index);
			return -1;
		}
		pctrl->vbyone_cfg.lane_count = be32_to_cpup((u32 *)propdata);
		pctrl->vbyone_cfg.region_num = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->vbyone_cfg.byte_mode  = be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->vbyone_cfg.color_fmt  = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->vbyone_cfg.slice = pdrv->config.timing.ppc ? pdrv->config.timing.ppc : 1;

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_attr", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get phy_attr\n", pdrv->index);
			pctrl->vbyone_cfg.phy_vswing = VX1_PHY_VSWING_DFT;
			pctrl->vbyone_cfg.phy_preem  = VX1_PHY_PREEM_DFT;
		} else {
			pctrl->vbyone_cfg.phy_vswing = be32_to_cpup((u32 *)propdata);
			pctrl->vbyone_cfg.phy_preem  = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("set phy vswing_level=0x%x, preem_level=0x%x\n",
				      pctrl->vbyone_cfg.phy_vswing,
				      pctrl->vbyone_cfg.phy_preem);
			}
		}

		phy_cfg->vswing_level = pctrl->vbyone_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->vbyone_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->vbyone_cfg.phy_preem;

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "vbyone_ctrl_flag", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get vbyone_ctrl_flag\n", pdrv->index);
			pctrl->vbyone_cfg.ctrl_flag = 0;
			pctrl->vbyone_cfg.power_on_reset_delay = VX1_PWR_ON_RESET_DLY_DFT;
			pctrl->vbyone_cfg.hpd_data_delay = VX1_HPD_DATA_DELAY_DFT;
			pctrl->vbyone_cfg.cdr_training_hold = VX1_CDR_TRAINING_HOLD_DFT;
		} else {
			pctrl->vbyone_cfg.ctrl_flag = be32_to_cpup((u32 *)propdata);
			LCDPR("vbyone ctrl_flag=0x%x\n", pctrl->vbyone_cfg.ctrl_flag);
		}
		if (pctrl->vbyone_cfg.ctrl_flag & 0x7) {
			propdata = (char *)fdt_getprop(dt_addr, child_offset,
						"vbyone_ctrl_timing", NULL);
			if (!propdata) {
				LCDPR("[%d]: failed to get vbyone_ctrl_timing\n", pdrv->index);
				pctrl->vbyone_cfg.power_on_reset_delay = VX1_PWR_ON_RESET_DLY_DFT;
				pctrl->vbyone_cfg.hpd_data_delay = VX1_HPD_DATA_DELAY_DFT;
				pctrl->vbyone_cfg.cdr_training_hold = VX1_CDR_TRAINING_HOLD_DFT;
			} else {
				pctrl->vbyone_cfg.power_on_reset_delay =
					be32_to_cpup((u32 *)propdata);
				pctrl->vbyone_cfg.hpd_data_delay =
					be32_to_cpup((((u32 *)propdata) + 1));
				pctrl->vbyone_cfg.cdr_training_hold =
					be32_to_cpup((((u32 *)propdata) + 2));
			}
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: power_on_reset_delay: %d\n",
				      pdrv->index,
				      pctrl->vbyone_cfg.power_on_reset_delay);
				LCDPR("[%d]: hpd_data_delay: %d\n",
				      pdrv->index,
				      pctrl->vbyone_cfg.hpd_data_delay);
				LCDPR("[%d]: cdr_training_hold: %d\n",
				      pdrv->index,
				      pctrl->vbyone_cfg.cdr_training_hold);
			}
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "hw_filter", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get hw_filter\n", pdrv->index);
			pctrl->vbyone_cfg.hw_filter_time = 0;
			pctrl->vbyone_cfg.hw_filter_cnt = 0;
		} else {
			pctrl->vbyone_cfg.hw_filter_time = be32_to_cpup((u32 *)propdata);
			pctrl->vbyone_cfg.hw_filter_cnt = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: vbyone hw_filter=0x%x 0x%x\n",
				      pdrv->index, pctrl->vbyone_cfg.hw_filter_time,
				      pctrl->vbyone_cfg.hw_filter_cnt);
			}
		}
		break;
	case LCD_MLVDS:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "minilvds_attr", &len);
		if (!propdata) {
			LCDERR("[%d]: failed to get minilvds_attr\n", pdrv->index);
			return -1;
		}
		pctrl->mlvds_cfg.channel_num  = be32_to_cpup((u32 *)propdata);
		pctrl->mlvds_cfg.channel_sel0 = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->mlvds_cfg.channel_sel1 = be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->mlvds_cfg.clk_phase    = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->mlvds_cfg.pn_swap      = be32_to_cpup((((u32 *)propdata) + 4));
		pctrl->mlvds_cfg.bit_swap     = be32_to_cpup((((u32 *)propdata) + 5));

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_attr", &len);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get phy_attr\n", pdrv->index);
			pctrl->mlvds_cfg.phy_vswing = LVDS_PHY_VSWING_DFT;
			pctrl->mlvds_cfg.phy_preem  = LVDS_PHY_PREEM_DFT;
		} else {
			pctrl->mlvds_cfg.phy_vswing = be32_to_cpup((u32 *)propdata);
			pctrl->mlvds_cfg.phy_preem  = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: set phy vswing=0x%x, preem=0x%x\n",
				      pdrv->index,
				      pctrl->mlvds_cfg.phy_vswing,
				      pctrl->mlvds_cfg.phy_preem);
			}
		}

		phy_cfg->vswing_level = pctrl->mlvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->mlvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->mlvds_cfg.phy_preem;
		break;
	case LCD_P2P:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "p2p_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get p2p_attr\n", pdrv->index);
			return -1;
		}
		pctrl->p2p_cfg.p2p_type = be32_to_cpup((u32 *)propdata);
		pctrl->p2p_cfg.lane_num = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->p2p_cfg.channel_sel0  = be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->p2p_cfg.channel_sel1  = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->p2p_cfg.pn_swap  = be32_to_cpup((((u32 *)propdata) + 4));
		pctrl->p2p_cfg.bit_swap  = be32_to_cpup((((u32 *)propdata) + 5));

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_attr", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get phy_attr\n", pdrv->index);
			pctrl->p2p_cfg.phy_vswing = 0x5;
			pctrl->p2p_cfg.phy_preem  = 0x1;
		} else {
			pctrl->p2p_cfg.phy_vswing = be32_to_cpup((u32 *)propdata);
			pctrl->p2p_cfg.phy_preem  = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: set phy vswing=0x%x, preem=0x%x\n",
				      pdrv->index,
				      pctrl->p2p_cfg.phy_vswing,
				      pctrl->p2p_cfg.phy_preem);
			}
		}

		phy_cfg->vswing_level = pctrl->p2p_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->p2p_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->p2p_cfg.phy_preem;
		break;
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_RGB:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "rgb_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get rgb_attr\n", pdrv->index);
			return -1;
		}
		pctrl->rgb_cfg.type = be32_to_cpup((u32 *)propdata);
		pctrl->rgb_cfg.clk_pol = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->rgb_cfg.de_valid = be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->rgb_cfg.sync_valid = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->rgb_cfg.rb_swap = be32_to_cpup((((u32 *)propdata) + 4));
		pctrl->rgb_cfg.bit_swap = be32_to_cpup((((u32 *)propdata) + 5));
		break;
	case LCD_BT656:
	case LCD_BT1120:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "bt_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get bt_attr\n", pdrv->index);
			return -1;
		}
		pctrl->bt_cfg.clk_phase = be32_to_cpup((u32 *)propdata);
		pctrl->bt_cfg.field_type = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->bt_cfg.mode_422 = be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->bt_cfg.yc_swap = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->bt_cfg.cbcr_swap = be32_to_cpup((((u32 *)propdata) + 4));
		break;
	case LCD_MIPI:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "mipi_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get mipi_attr\n", pdrv->index);
			return -1;
		}
		pctrl->mipi_cfg.lane_num = be32_to_cpup((u32 *)propdata);
		pctrl->mipi_cfg.bit_rate_max = be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->mipi_cfg.operation_mode_init = be32_to_cpup((((u32 *)propdata) + 3));
		pctrl->mipi_cfg.operation_mode_display = be32_to_cpup((((u32 *)propdata) + 4));
		pctrl->mipi_cfg.video_mode_type = be32_to_cpup((((u32 *)propdata) + 5));
		pctrl->mipi_cfg.clk_always_hs = be32_to_cpup((((u32 *)propdata) + 6));
		pctrl->mipi_cfg.user_pkt_size = be32_to_cpup((((u32 *)propdata) + 7));

		pctrl->mipi_cfg.check_en = 0;
		pctrl->mipi_cfg.check_reg = 0xff;
		pctrl->mipi_cfg.check_cnt = 0;
		lcd_dsi_init_table_load_dts(dt_addr, child_offset, &pctrl->mipi_cfg);

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "extern_init", NULL);
		if (propdata) {
			pctrl->mipi_cfg.extern_init = be32_to_cpup((u32 *)propdata);
			if (pctrl->mipi_cfg.extern_init < 0xff) {
				LCDPR("[%d]: find extern_init: %d\n",
				      pdrv->index, pctrl->mipi_cfg.extern_init);
			}
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pctrl->mipi_cfg.extern_init);
#endif
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "dsi_detect_attr", NULL);
		if (propdata) {
			LCDPR("[%d]: load MIPI-DSI panel detect config\n", pdrv->index);
			pctrl->mipi_cfg.panel_det_attr = 0x5; // dsi_det_en || dts
			pctrl->mipi_cfg.panel_det_attr |=
				(be32_to_cpup(((u32 *)propdata) + 1) && 1) << 1; // store2env
			pctrl->mipi_cfg.dt_addr = dt_addr;
		}

		phy_cfg->vswing_level = 0;
		phy_cfg->preem_level = 0;
		break;
	case LCD_EDP:
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "edp_attr", NULL);
		if (!propdata) {
			LCDERR("[%d]: failed to get edp_attr\n", pdrv->index);
			return -1;
		}
		pctrl->edp_cfg.max_lane_count = (u8)be32_to_cpup((u32 *)propdata);
		pctrl->edp_cfg.max_link_rate = (u8)be32_to_cpup((((u32 *)propdata) + 1));
		pctrl->edp_cfg.max_link_rate =
			pctrl->edp_cfg.max_link_rate < 0x6 ? 0 : pctrl->edp_cfg.max_link_rate;
		pctrl->edp_cfg.training_mode = (u8)be32_to_cpup((((u32 *)propdata) + 2));
		pctrl->edp_cfg.edid_en = (u8)be32_to_cpup((((u32 *)propdata) + 3));

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_attr", NULL);
		if (!propdata) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: failed to get phy_attr\n", pdrv->index);
			pctrl->edp_cfg.phy_vswing_preset = 0x5;
			pctrl->edp_cfg.phy_preem_preset  = 0x1;
		} else {
			pctrl->edp_cfg.phy_vswing_preset = be32_to_cpup((u32 *)propdata);
			pctrl->edp_cfg.phy_preem_preset  = be32_to_cpup((((u32 *)propdata) + 1));
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: set phy vswing=0x%x, preem=0x%x\n",
				      pdrv->index,
				      pctrl->edp_cfg.phy_vswing_preset,
				      pctrl->edp_cfg.phy_preem_preset);
			}
		}
		phy_cfg->vswing_level = pctrl->edp_cfg.phy_vswing_preset & 0xf;
		phy_cfg->preem_level = pctrl->edp_cfg.phy_preem_preset;
		break;
#endif
	default:
		LCDERR("invalid lcd type\n");
		break;
	}

	phy = lcd_phy_alloc(pdrv);
	if (!phy) {
		LCDERR("[%d]: failed to alloc phy memory\n", pdrv->index);
		return -1;
	}
	memset(phy, 0, sizeof(*phy));
	phy_cfg->act_phy = phy_cfg->phys[0];
	lcd_phy_param_preset(pdrv);
	lcd_lane_map_preset(pdrv);
	phy->ss.freq = 255;
	phy->ss.level = 255;
	phy->ss.mode = 255;

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_adv_attr", NULL);
	if (propdata && phy_cfg->phys[0]) {
		phy_cfg->flag     = be32_to_cpup(((u32 *)propdata) + 0);
		phy->vswing   = be32_to_cpup(((u32 *)propdata) + 1);
		phy->vcm      = be32_to_cpup(((u32 *)propdata) + 2);
		phy->ref_bias = be32_to_cpup(((u32 *)propdata) + 3);
		phy->odt      = be32_to_cpup(((u32 *)propdata) + 4);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("%s: ctrl_flag=0x%x vsw=0x%08x vcm=0x%x, ref_bias=0x%x, odt=0x%x\n",
			      __func__, phy_cfg->flag, phy->vswing,
			      phy->vcm, phy->ref_bias, phy->odt);
		}
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "phy_lane_ctrl", &len);

		if (phy_cfg->flag & (0x3 << 12) && len > 0 && propdata) {
			for (i = 0; i < phy_cfg->lane_num; i++) {
				if (i >= (len / 4))
					break;

				if (phy_cfg->flag & (1 << 12))
					phy->lane[i].preem =
						be32_to_cpup(((u32 *)propdata) + i) & 0xffff;

				if (phy_cfg->flag & (1 << 13))
					phy->lane[i].amp =
						be32_to_cpup(((u32 *)propdata) + i) >> 16;

				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					if ((phy_cfg->flag >> 12 & 0x3) == 0x3) {
						LCDPR("%s: lane[%d]: preem=0x%x amp=0x%x\n",
						      __func__, i, phy->lane[i].preem,
						      phy->lane[i].amp);
					} else if ((phy_cfg->flag >> 12 & 0x3) == 0x1) {
						LCDPR("%s: lane[%d]: preem=0x%x\n",
						      __func__, i, phy->lane[i].preem);
					} else if ((phy_cfg->flag >> 12 & 0x3) == 0x2) {
						LCDPR("%s: lane[%d]: amp=0x%x\n",
						      __func__, i, phy->lane[i].amp);
					}
				}
			}
		}
	}

	/* check power_step */
	lcd_power_load_from_dts(pdrv, dt_addr, child_offset);

	lcd_cus_ctrl_load_from_dts(pdrv);

	//fix ss in detail timing and phy_attr if not config
	lcd_ss_config_fix(pdrv);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "backlight_index", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get backlight_index\n", pdrv->index);
		pconf->backlight_index = 0xff;
	} else {
		pconf->backlight_index = be32_to_cpup((u32 *)propdata);
#ifdef CONFIG_AML_LCD_BACKLIGHT
		aml_bl_index_add(pdrv->index, pconf->backlight_index);
#endif
	}
#endif

	return 0;
}

static int lcd_power_load_from_unifykey(struct aml_lcd_drv_s *pdrv,
					unsigned char *buf, int key_len, int len)
{
	struct lcd_power_step_s *pstep;
	int i, j, temp;
	unsigned char *p;
	int ret = 0;

	/* power: (5byte * n) */
	pstep = pdrv->config.power.power_on_step;
	p = buf + len;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: power_on step:\n", pdrv->index);
	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		len += 5;
		ret = lcd_unifykey_len_check(key_len, len);
		if (ret) {
			pstep[i].type = 0xff;
			pstep[i].index = 0;
			pstep[i].value = 0;
			pstep[i].delay = 0;
			LCDERR("unifykey power_on length is incorrect\n");
			return -1;
		}
		pstep[i].type = *(p + LCD_UKEY_PWR_TYPE + 5 * i);
		pstep[i].index = *(p + LCD_UKEY_PWR_INDEX + 5 * i);
		pstep[i].value = *(p + LCD_UKEY_PWR_VAL + 5 * i);
		pstep[i].delay = (*(p + LCD_UKEY_PWR_DELAY + 5 * i) |
				  ((*(p + LCD_UKEY_PWR_DELAY + 5 * i + 1)) << 8));

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("step %d: type=%d, index=%d, value=%d, delay=%d\n",
			      i, pstep[i].type, pstep[i].index,
			      pstep[i].value, pstep[i].delay);
		}
		if (pstep[i].type >= LCD_POWER_TYPE_MAX)
			break;

		if (pstep[i].type == LCD_POWER_TYPE_CLK_SS) {
			temp = pstep[i].value;
			pdrv->config.timing.ss_freq = temp & 0xf;
			pdrv->config.timing.ss_mode = (temp >> 4) & 0xf;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: clk_ss value=0x%x: ss_freq=%d, ss_mode=%d\n",
				      pdrv->index, temp,
				      pdrv->config.timing.ss_freq,
				      pdrv->config.timing.ss_mode);
			}
		} else if (pstep[i].type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pstep[i].index);
#endif
		} else if (pstep[i].type == LCD_POWER_TYPE_MUTE) {
			pdrv->status |= LCD_STATUS_PRE_MUTE;
		}
		i++;
	}

	pstep = pdrv->config.power.power_off_step;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: power_off step:\n", pdrv->index);
	p += (5 * (i + 1));
	j = 0;
	while (j < LCD_PWR_STEP_MAX) {
		len += 5;
		ret = lcd_unifykey_len_check(key_len, len);
		if (ret) {
			pstep[j].type = 0xff;
			pstep[j].index = 0;
			pstep[j].value = 0;
			pstep[j].delay = 0;
			LCDERR("unifykey power_off length is incorrect\n");
			return -1;
		}
		pstep[j].type = *(p + LCD_UKEY_PWR_TYPE + 5 * j);
		pstep[j].index = *(p + LCD_UKEY_PWR_INDEX + 5 * j);
		pstep[j].value = *(p + LCD_UKEY_PWR_VAL + 5 * j);
		pstep[j].delay = (*(p + LCD_UKEY_PWR_DELAY + 5 * j) |
				  ((*(p + LCD_UKEY_PWR_DELAY + 5 * j + 1)) << 8));

		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("step %d: type=%d, index=%d, value=%d, delay=%d\n",
			      j, pstep[j].type, pstep[j].index,
			      pstep[j].value, pstep[j].delay);
		}
		if (pstep[j].type >= LCD_POWER_TYPE_MAX)
			break;

		if (pstep[j].type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pstep[j].index);
#endif
		}
		j++;
	}

	return ret;
}

static int lcd_config_load_from_unifykey_v2(struct aml_lcd_drv_s *pdrv,
					    unsigned char *p,
					    unsigned int key_len,
					    unsigned int offset)
{
	struct lcd_unifykey_header_s *header;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy;
	unsigned int len, size;
	unsigned char version;
	int i, ret;

	phy = phy_cfg->phys[0];
	if (!phy)
		return -1;

	header = (struct lcd_unifykey_header_s *)p;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		lcd_unifykey_header_print(p);

	/* step 2: check lcd parameters */
	len = offset + header->block_cur_size;
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret < 0) {
		LCDERR("ukey parameters length is incorrect\n");
		return -1;
	}

	/*phy 356byte*/
	phy_cfg->flag = (*(p + LCD_UKEY_PHY_ATTR_FLAG) |
		((*(p + LCD_UKEY_PHY_ATTR_FLAG + 1)) << 8) |
		((*(p + LCD_UKEY_PHY_ATTR_FLAG + 2)) << 16) |
		((*(p + LCD_UKEY_PHY_ATTR_FLAG + 3)) << 24));
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: ctrl_flag=0x%x\n", __func__, phy_cfg->flag);

	if (phy_cfg->flag & PHY_BIT_VSWING) {
		phy->vswing = (*(p + LCD_UKEY_PHY_ATTR_0) |
				*(p + LCD_UKEY_PHY_ATTR_0 + 1) << 8);
	}
	if (phy_cfg->flag & PHY_BIT_VCM) {
		phy->vcm = (*(p + LCD_UKEY_PHY_ATTR_1) |
				*(p + LCD_UKEY_PHY_ATTR_1 + 1) << 8);
	}
	if (phy_cfg->flag & PHY_BIT_REF_BIAS) {
		phy->ref_bias = (*(p + LCD_UKEY_PHY_ATTR_2) |
				*(p + LCD_UKEY_PHY_ATTR_2 + 1) << 8);
	}
	if (phy_cfg->flag & PHY_BIT_ODT) {
		phy->odt = (*(p + LCD_UKEY_PHY_ATTR_3) |
				*(p + LCD_UKEY_PHY_ATTR_3 + 1) << 8);
	}
	if (phy_cfg->flag & PHY_BIT_CV_MODE) {
		phy->cv_mode = (*(p + LCD_UKEY_PHY_ATTR_4) |
				*(p + LCD_UKEY_PHY_ATTR_4 + 1) << 8);
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: vswing=0x%x, vcm=0x%x, ref_bias=0x%x, odt=0x%x, cv_mode=%d\n",
		      __func__, phy->vswing, phy->vcm, phy->ref_bias, phy->odt, phy->cv_mode);
	}

	if (phy_cfg->flag & PHY_BIT_LANE_PREEM) {
		for (i = 0; i < phy_cfg->lane_num; i++) {
			phy->lane[i].preem =
				*(p + LCD_UKEY_PHY_LANE_CTRL + 4 * i) |
				(*(p + LCD_UKEY_PHY_LANE_CTRL + 4 * i + 1) << 8);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: lane[%d]: preem=0x%x\n",
				      __func__, i, phy->lane[i].preem);
			}
		}
	}

	if (phy_cfg->flag & PHY_BIT_LANE_AMP) {
		for (i = 0; i < phy_cfg->lane_num; i++) {
			phy->lane[i].amp =
				*(p + LCD_UKEY_PHY_LANE_CTRL + 4 * i + 2) |
				(*(p + LCD_UKEY_PHY_LANE_CTRL + 4 * i + 3) << 8);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: lane[%d]: amp=0x%x\n",
				      __func__, i, phy->lane[i].amp);
			}
		}
	}

	if (phy_cfg->flag & PHY_BIT_LANE_SEL) {
		for (i = 0; i < phy_cfg->lane_num; i++) {
			phy_cfg->ch_ctrl[i].sel = *(p + LCD_UKEY_PHY_LANE_SEL + i);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s: lane[%d]: sel=0x%x\n",
				      __func__, i, phy_cfg->ch_ctrl[i].sel);
			}
		}
	}

	size = LCD_UKEY_CUS_CTRL_ATTR_FLAG;
	version = header->version;
	lcd_cus_ctrl_load_from_unifykey(pdrv, (p + size), (key_len - size), version);

	return 0;
}

static int lcd_config_load_from_unifykey_v3(struct aml_lcd_drv_s *pdrv,
					    unsigned char *p,
					    unsigned int key_len,
					    unsigned int offset)
{
	struct lcd_unifykey_header_s *header;
	unsigned int len, size;
	unsigned char version;
	int ret;

	header = (struct lcd_unifykey_header_s *)p;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		lcd_unifykey_header_print(p);

	/* step 2: check lcd parameters */
	len = offset + header->block_cur_size;
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret < 0) {
		LCDERR("ukey parameters length is incorrect\n");
		return -1;
	}

	size = LCD_UKEY_CUS_CTRL_ATTR_FLAG_V3;
	version = header->version;
	lcd_cus_ctrl_load_from_unifykey(pdrv, (p + size), (key_len - size), version);

	return 0;
}

static int lcd_config_load_from_unifykey(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_detail_timing_s *ptiming;
	union lcd_ctrl_config_u *pctrl = &pdrv->config.control;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = NULL;
	struct lcd_unifykey_header_s *lcd_header;
	unsigned char *para;
	char key_str[10];
	int key_len, len;
	unsigned char *p, val;
	const char *str;
	unsigned int temp, lcd_bits = 24;
	char str_info[128];
	int str_info_len = 0, ret = 0;

	if (pdrv->index == 0)
		sprintf(key_str, "lcd");
	else
		sprintf(key_str, "lcd%d", pdrv->index);

	ret = lcd_unifykey_get_size(key_str, &key_len);
	if (ret)
		return -1;

	para = (unsigned char *)malloc(key_len);
	if (!para) {
		LCDERR("[%d]: %s: Not enough memory\n", pdrv->index, __func__);
		return -1;
	}
	memset(para, 0, key_len);

	ret = lcd_unifykey_get(key_str, para, key_len);
	if (ret)
		goto load_from_unifykey_exit;

	/* step 1: check header */
	lcd_header = (struct lcd_unifykey_header_s *)para;
	len = LCD_UKEY_DATA_LEN_V1; /*10+36+18+31+20*/
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		lcd_unifykey_header_print(para);

	/* step 2: check lcd parameters */
	ret = lcd_unifykey_len_check(key_len, len);
	if (ret) {
		LCDERR("[%d]: ukey parameters length is incorrect\n", pdrv->index);
		goto load_from_unifykey_exit;
	}

	/* basic: 36byte */
	p = para;
	str = (const char *)(p + LCD_UKEY_HEAD_SIZE);
	strncpy(pconf->basic.model_name, str,
		sizeof(pconf->basic.model_name) - 1);
	pconf->basic.model_name[sizeof(pconf->basic.model_name) - 1] = '\0';
	temp = *(p + LCD_UKEY_INTERFACE);
	pconf->basic.lcd_type = temp & 0x3f;
	pconf->basic.config_check = (temp >> 6) & 0x3;
	temp = *(p + LCD_UKEY_LCD_BITS_CFMT);
	lcd_bits = (temp & 0x3f) * 3;
	pconf->basic.screen_width = (*(p + LCD_UKEY_SCREEN_WIDTH) |
		((*(p + LCD_UKEY_SCREEN_WIDTH + 1)) << 8));
	pconf->basic.screen_height = (*(p + LCD_UKEY_SCREEN_HEIGHT) |
		((*(p + LCD_UKEY_SCREEN_HEIGHT + 1)) << 8));

	ptiming = lcd_timing_alloc(pdrv);
	if (!ptiming) {
		ret = -1;
		goto load_from_unifykey_exit;
	}
	memset(ptiming, 0, sizeof(*ptiming));
	/* timing: 18byte */
	ptiming->h_active = (*(p + LCD_UKEY_H_ACTIVE) |
		((*(p + LCD_UKEY_H_ACTIVE + 1)) << 8));
	ptiming->v_active = (*(p + LCD_UKEY_V_ACTIVE)) |
		((*(p + LCD_UKEY_V_ACTIVE + 1)) << 8);
	ptiming->h_period = (*(p + LCD_UKEY_H_PERIOD)) |
		((*(p + LCD_UKEY_H_PERIOD + 1)) << 8);
	ptiming->v_period = (*(p + LCD_UKEY_V_PERIOD)) |
		((*(p + LCD_UKEY_V_PERIOD + 1)) << 8);
	temp = *(unsigned short *)(p + LCD_UKEY_HS_WIDTH_POL);
	ptiming->hsync_width = temp & 0xfff;
	ptiming->hsync_pol = (temp >> 12) & 0xf;
	ptiming->hsync_bp = (*(p + LCD_UKEY_HS_BP) |
		((*(p + LCD_UKEY_HS_BP + 1)) << 8));
	ptiming->hsync_fp = ptiming->h_period - ptiming->h_active -
			ptiming->hsync_width - ptiming->hsync_bp;
	temp = *(unsigned short *)(p + LCD_UKEY_VS_WIDTH_POL);
	ptiming->vsync_width = temp & 0xfff;
	ptiming->vsync_pol = (temp >> 12) & 0xf;
	ptiming->vsync_bp = (*(p + LCD_UKEY_VS_BP) |
		((*(p + LCD_UKEY_VS_BP + 1)) << 8));
	ptiming->vsync_fp = ptiming->v_period - ptiming->v_active -
			ptiming->vsync_width - ptiming->vsync_bp;
	pconf->timing.pre_de_h = *(p + LCD_UKEY_PRE_DE_H);
	pconf->timing.pre_de_v = *(p + LCD_UKEY_PRE_DE_V);

	/* customer: 31byte */
	ptiming->fr_adjust_type = *(p + LCD_UKEY_FR_ADJ_TYPE);
	pconf->timing.ss_level = *(p + LCD_UKEY_SS_LEVEL);
	val = *(p + LCD_UKEY_CUST_VAL0);
	pconf->timing.clk_mode = (val >> 4) & 0xf;
	pconf->timing.pll_flag = val & 0xf;
	ptiming->pixel_clk = (*(p + LCD_UKEY_PCLK) |
		((*(p + LCD_UKEY_PCLK + 1)) << 8) |
		((*(p + LCD_UKEY_PCLK + 2)) << 16) |
		((*(p + LCD_UKEY_PCLK + 3)) << 24));
	ptiming->h_period_min = (*(p + LCD_UKEY_H_PERIOD_MIN) |
		((*(p + LCD_UKEY_H_PERIOD_MIN + 1)) << 8));
	ptiming->h_period_max = (*(p + LCD_UKEY_H_PERIOD_MAX) |
		((*(p + LCD_UKEY_H_PERIOD_MAX + 1)) << 8));
	ptiming->v_period_min = (*(p + LCD_UKEY_V_PERIOD_MIN) |
		((*(p  + LCD_UKEY_V_PERIOD_MIN + 1)) << 8));
	ptiming->v_period_max = (*(p + LCD_UKEY_V_PERIOD_MAX) |
		((*(p + LCD_UKEY_V_PERIOD_MAX + 1)) << 8));
	ptiming->pclk_min = (*(p + LCD_UKEY_PCLK_MIN) |
		((*(p + LCD_UKEY_PCLK_MIN + 1)) << 8) |
		((*(p + LCD_UKEY_PCLK_MIN + 2)) << 16) |
		((*(p + LCD_UKEY_PCLK_MIN + 3)) << 24));
	ptiming->pclk_max = (*(p + LCD_UKEY_PCLK_MAX) |
		((*(p + LCD_UKEY_PCLK_MAX + 1)) << 8) |
		((*(p + LCD_UKEY_PCLK_MAX + 2)) << 16) |
		((*(p + LCD_UKEY_PCLK_MAX + 3)) << 24));
	ptiming->frame_rate_min = *(p + LCD_UKEY_FRAME_RATE_MIN);
	ptiming->frame_rate_max = *(p + LCD_UKEY_FRAME_RATE_MAX);

	val = *(p + LCD_UKEY_CUST_VAL1);
	pconf->timing.ppc = (val >> 4) & 0xf;
	pconf->custom_pinmux = val & 0xf;

	pconf->fr_auto_cus = *(p + LCD_UKEY_FR_AUTO_CUS);
	ptiming->switch_type = LCD_VMODE_SWITCH_NONE;
	ptiming->lcd_bits = lcd_bits;
	ptiming->ss_force = 0;
	ptiming->ss_freq = 255;
	ptiming->ss_level = pconf->timing.ss_level;
	ptiming->ss_mode = 255;

	pdrv->config.timing.dft_timing = pdrv->config.timing.timings[0];
	lcd_clk_frame_rate_init(ptiming);
	lcd_config_timing_check(pdrv, ptiming);
	lcd_default_to_basic_timing_init_config(pdrv);

	str_info_len += sprintf(str_info + str_info_len, "ppc:%d, ",
			pconf->timing.ppc);
	str_info_len += sprintf(str_info + str_info_len, "clk_mode:%d, ",
			pconf->timing.clk_mode);
	if (pconf->timing.pre_de_h || pconf->timing.pre_de_h) {
		str_info_len += sprintf(str_info + str_info_len, "pre_de:%d,%d, ",
				pconf->timing.pre_de_h, pconf->timing.pre_de_h);
	}
	str_info_len += sprintf(str_info + str_info_len, "cfg_chk:0x%x, ",
			pconf->basic.config_check);
	str_info_len += sprintf(str_info + str_info_len, "cus_pinmux:%d, ",
			pconf->custom_pinmux);
	sprintf(str_info + str_info_len, "afr_cus:0x%x", pconf->fr_auto_cus);
	LCDPR("[%d]: load ukey config: %s, %s, %dbit, %dx%d, %s\n",
	      pdrv->index, pconf->basic.model_name,
	      lcd_type_type_to_str(pconf->basic.lcd_type),
	      lcd_bits, ptiming->h_active, ptiming->v_active,
	      str_info);

	/* interface: 20byte */
	switch (pconf->basic.lcd_type) {
	case LCD_RGB:
		pctrl->rgb_cfg.type =
			(*(p + LCD_UKEY_IF_ATTR_0) |
			 ((*(p + LCD_UKEY_IF_ATTR_0 + 1)) << 8));
		pctrl->rgb_cfg.clk_pol =
			(*(p + LCD_UKEY_IF_ATTR_1) |
			 ((*(p + LCD_UKEY_IF_ATTR_1 + 1)) << 8));
		pctrl->rgb_cfg.de_valid =
			(*(p + LCD_UKEY_IF_ATTR_2) |
			 ((*(p + LCD_UKEY_IF_ATTR_2 + 1)) << 8));
		pctrl->rgb_cfg.sync_valid =
			(*(p + LCD_UKEY_IF_ATTR_3) |
			 ((*(p + LCD_UKEY_IF_ATTR_3 + 1)) << 8));
		pctrl->rgb_cfg.rb_swap =
			(*(p + LCD_UKEY_IF_ATTR_4) |
			 ((*(p + LCD_UKEY_IF_ATTR_4 + 1)) << 8));
		pctrl->rgb_cfg.bit_swap =
			(*(p + LCD_UKEY_IF_ATTR_5) |
			 ((*(p + LCD_UKEY_IF_ATTR_5 + 1)) << 8));
		break;
	case LCD_LVDS:
		pctrl->lvds_cfg.lvds_repack =
			*(p + LCD_UKEY_IF_ATTR_0) |
			((*(p + LCD_UKEY_IF_ATTR_0 + 1)) << 8);
		pctrl->lvds_cfg.dual_port =
			*(p + LCD_UKEY_IF_ATTR_1) |
			((*(p + LCD_UKEY_IF_ATTR_1 + 1)) << 8);
		pctrl->lvds_cfg.pn_swap =
			*(p + LCD_UKEY_IF_ATTR_2) |
			((*(p + LCD_UKEY_IF_ATTR_2 + 1)) << 8);
		pctrl->lvds_cfg.port_swap =
			*(p + LCD_UKEY_IF_ATTR_3) |
			((*(p + LCD_UKEY_IF_ATTR_3 + 1)) << 8);
		pctrl->lvds_cfg.phy_vswing =
			*(p + LCD_UKEY_IF_ATTR_4) |
			((*(p + LCD_UKEY_IF_ATTR_4 + 1)) << 8);
		pctrl->lvds_cfg.phy_preem =
			*(p + LCD_UKEY_IF_ATTR_5) |
			((*(p + LCD_UKEY_IF_ATTR_5 + 1)) << 8);
		pctrl->lvds_cfg.lane_reverse =
			*(p + LCD_UKEY_IF_ATTR_8) |
			((*(p + LCD_UKEY_IF_ATTR_8 + 1)) << 8);

		phy_cfg->vswing_level = pctrl->lvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->lvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->lvds_cfg.phy_preem;
		break;
	case LCD_VBYONE:
		pctrl->vbyone_cfg.lane_count =
			*(p + LCD_UKEY_IF_ATTR_0) |
			((*(p + LCD_UKEY_IF_ATTR_0 + 1)) << 8);
		pctrl->vbyone_cfg.region_num =
			*(p + LCD_UKEY_IF_ATTR_1) |
			((*(p + LCD_UKEY_IF_ATTR_1 + 1)) << 8);
		pctrl->vbyone_cfg.byte_mode  =
			*(p + LCD_UKEY_IF_ATTR_2) |
			((*(p + LCD_UKEY_IF_ATTR_2 + 1)) << 8);
		pctrl->vbyone_cfg.color_fmt  =
			*(p + LCD_UKEY_IF_ATTR_3) |
			((*(p + LCD_UKEY_IF_ATTR_3 + 1)) << 8);
		pctrl->vbyone_cfg.phy_vswing =
			*(p + LCD_UKEY_IF_ATTR_4) |
			((*(p + LCD_UKEY_IF_ATTR_4 + 1)) << 8);
		pctrl->vbyone_cfg.phy_preem =
			*(p + LCD_UKEY_IF_ATTR_5) |
			((*(p + LCD_UKEY_IF_ATTR_5 + 1)) << 8);
		pctrl->vbyone_cfg.hw_filter_time =
			*(p + LCD_UKEY_IF_ATTR_8) |
			((*(p + LCD_UKEY_IF_ATTR_8 + 1)) << 8);
		pctrl->vbyone_cfg.hw_filter_cnt =
			*(p + LCD_UKEY_IF_ATTR_9) |
			((*(p + LCD_UKEY_IF_ATTR_9 + 1)) << 8);
		pctrl->vbyone_cfg.ctrl_flag = 0;
		pctrl->vbyone_cfg.power_on_reset_delay = VX1_PWR_ON_RESET_DLY_DFT;
		pctrl->vbyone_cfg.hpd_data_delay = VX1_HPD_DATA_DELAY_DFT;
		pctrl->vbyone_cfg.cdr_training_hold = VX1_CDR_TRAINING_HOLD_DFT;
		pctrl->vbyone_cfg.slice = pdrv->config.timing.ppc ? pdrv->config.timing.ppc : 1;

		phy_cfg->vswing_level = pctrl->vbyone_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->vbyone_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->vbyone_cfg.phy_preem;
		break;
	case LCD_MLVDS:
		pctrl->mlvds_cfg.channel_num =
			*(p + LCD_UKEY_IF_ATTR_0) |
			((*(p + LCD_UKEY_IF_ATTR_0 + 1)) << 8);
		pctrl->mlvds_cfg.channel_sel0 =
			*(p + LCD_UKEY_IF_ATTR_1) |
			((*(p + LCD_UKEY_IF_ATTR_1 + 1)) << 8) |
			((*(p + LCD_UKEY_IF_ATTR_2)) << 16) |
			((*(p + LCD_UKEY_IF_ATTR_2 + 1)) << 24);
		pctrl->mlvds_cfg.channel_sel1 =
			*(p + LCD_UKEY_IF_ATTR_3) |
			((*(p + LCD_UKEY_IF_ATTR_3 + 1)) << 8) |
			((*(p + LCD_UKEY_IF_ATTR_4)) << 16) |
			((*(p + LCD_UKEY_IF_ATTR_4 + 1)) << 24);
		pctrl->mlvds_cfg.clk_phase =
			*(p + LCD_UKEY_IF_ATTR_5) |
			((*(p + LCD_UKEY_IF_ATTR_5 + 1)) << 8);
		pctrl->mlvds_cfg.pn_swap =
			*(p + LCD_UKEY_IF_ATTR_6) |
			((*(p + LCD_UKEY_IF_ATTR_6 + 1)) << 8);
		pctrl->mlvds_cfg.bit_swap =
			*(p + LCD_UKEY_IF_ATTR_7) |
			((*(p + LCD_UKEY_IF_ATTR_7 + 1)) << 8);
		pctrl->mlvds_cfg.phy_vswing =
			*(p + LCD_UKEY_IF_ATTR_8) |
			((*(p + LCD_UKEY_IF_ATTR_8 + 1)) << 8);
		pctrl->mlvds_cfg.phy_preem =
			*(p + LCD_UKEY_IF_ATTR_9) |
			((*(p + LCD_UKEY_IF_ATTR_9 + 1)) << 8);

		phy_cfg->vswing_level = pctrl->mlvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->mlvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->mlvds_cfg.phy_preem;
		break;
	case LCD_P2P:
		pctrl->p2p_cfg.p2p_type =
			*(p + LCD_UKEY_IF_ATTR_0) |
			((*(p + LCD_UKEY_IF_ATTR_0 + 1)) << 8);
		pctrl->p2p_cfg.lane_num =
			*(p + LCD_UKEY_IF_ATTR_1) |
			((*(p + LCD_UKEY_IF_ATTR_1 + 1)) << 8);
		pctrl->p2p_cfg.channel_sel0 =
			*(p + LCD_UKEY_IF_ATTR_2) |
			((*(p + LCD_UKEY_IF_ATTR_2 + 1)) << 8) |
			(*(p + LCD_UKEY_IF_ATTR_3) << 16) |
			((*(p + LCD_UKEY_IF_ATTR_3 + 1)) << 24);
		pctrl->p2p_cfg.channel_sel1 =
			*(p + LCD_UKEY_IF_ATTR_4) |
			((*(p + LCD_UKEY_IF_ATTR_4 + 1)) << 8) |
			(*(p + LCD_UKEY_IF_ATTR_5) << 16) |
			((*(p + LCD_UKEY_IF_ATTR_5 + 1)) << 24);
		pctrl->p2p_cfg.pn_swap =
			*(p + LCD_UKEY_IF_ATTR_6) |
			((*(p + LCD_UKEY_IF_ATTR_6 + 1)) << 8);
		pctrl->p2p_cfg.bit_swap =
			*(p + LCD_UKEY_IF_ATTR_7) |
			((*(p + LCD_UKEY_IF_ATTR_7 + 1)) << 8);
		pctrl->p2p_cfg.phy_vswing =
			*(p + LCD_UKEY_IF_ATTR_8) |
			((*(p + LCD_UKEY_IF_ATTR_8 + 1)) << 8);
		pctrl->p2p_cfg.phy_preem =
			*(p + LCD_UKEY_IF_ATTR_9) |
			((*(p + LCD_UKEY_IF_ATTR_9 + 1)) << 8);

		phy_cfg->vswing_level = pctrl->p2p_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->p2p_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->p2p_cfg.phy_preem;
		break;
	default:
		LCDERR("[%d]: unsupport lcd_type: %d\n",
		       pdrv->index, pconf->basic.lcd_type);
		break;
	}

	phy = lcd_phy_alloc(pdrv);
	if (!phy) {
		ret = -1;
		goto load_from_unifykey_exit;
	}
	memset(phy, 0, sizeof(*phy));
	phy_cfg->act_phy = phy_cfg->phys[0];
	lcd_phy_param_preset(pdrv);
	lcd_lane_map_preset(pdrv);
	phy->ss.freq = 255;
	phy->ss.level = 255;
	phy->ss.mode = 255;

	/* step 3: check power sequence */
	ret = lcd_power_load_from_unifykey(pdrv, para, key_len, len);
	if (ret < 0)
		goto load_from_unifykey_exit;

	p = para + lcd_header->block_cur_size;
	switch (lcd_header->version) {
	case 2:
		lcd_config_load_from_unifykey_v2(pdrv, p, key_len, lcd_header->block_cur_size);
		break;
	case 3:
		lcd_config_load_from_unifykey_v3(pdrv, p, key_len, lcd_header->block_cur_size);
		break;
	default:
		break;
	}

	//fix ss in detail timing and phy_attr if not config
	lcd_ss_config_fix(pdrv);

#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_index_add(pdrv->index, 0);
#endif
load_from_unifykey_exit:

	free(para);
	return ret;
}

static int lcd_config_load_from_bsp(struct aml_lcd_drv_s *pdrv)
{
	struct ext_lcd_config_s *ext_lcd;
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_detail_timing_s *ptiming;
	union lcd_ctrl_config_u *pctrl = &pdrv->config.control;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	struct phy_attr_s *phy = NULL;
	struct lcd_power_step_s *power_step;
	char *panel_type, str[15];
	unsigned int i, done;
	unsigned int temp, str_info_len = 0;
	char str_info[128];

	if (pdrv->index >= LCD_MAX_DRV) {
		LCDERR("[%d]: invalid drv index %d\n", pdrv->index, pdrv->index);
		return -1;
	}

	if (pdrv->index == 0)
		sprintf(str, "panel_type");
	else
		sprintf(str, "panel%d_type", pdrv->index);
	panel_type = env_get(str);
	if (!panel_type) {
		LCDERR("[%d]: no %s exist\n", pdrv->index, str);
		return -1;
	}

	if (!pdrv->data->dft_conf[pdrv->index]) {
		LCDERR("[%d]: %s: dft_conf is NULL\n", pdrv->index, __func__);
		return -1;
	}
	ext_lcd = pdrv->data->dft_conf[pdrv->index]->ext_lcd;
	if (!ext_lcd) {
		LCDERR("[%d]: %s: ext_lcd is NULL\n", pdrv->index, __func__);
		return -1;
	}
	done = 0;
	for (i = 0 ; i < LCD_NUM_MAX ; i++) {
		if (strcmp(ext_lcd->panel_type, panel_type) == 0) {
			done = 1;
			break;
		}
		if (strcmp(ext_lcd->panel_type, "invalid") == 0)
			break;
		ext_lcd++;
	}
	if (done == 0) {
		LCDERR("[%d]: can't find %s\n ", pdrv->index, panel_type);
		return -1;
	}
	LCDPR("[%d]: use default %s=%s\n", pdrv->index, str, panel_type);

	strlcpy(pconf->basic.model_name, panel_type, sizeof(pconf->basic.model_name));
	pconf->basic.model_name[sizeof(pconf->basic.model_name) - 1] = '\0';

	pconf->basic.lcd_type = ext_lcd->lcd_type;

	ptiming = lcd_timing_alloc(pdrv);
	if (!ptiming)
		return -1;

	memset(ptiming, 0, sizeof(*ptiming));

	ptiming->lcd_bits = ext_lcd->lcd_bits * 3;
	ptiming->h_active = ext_lcd->h_active;
	ptiming->v_active = ext_lcd->v_active;
	ptiming->h_period = ext_lcd->h_period;
	ptiming->v_period = ext_lcd->v_period;

	ptiming->h_period_min = ptiming->h_period;
	ptiming->h_period_max = ptiming->h_period;
	ptiming->v_period_min = ptiming->v_period;
	ptiming->v_period_max = ptiming->v_period;
	ptiming->pclk_min = 0;
	ptiming->pclk_max = 0;
	ptiming->frame_rate_min = 0;
	ptiming->frame_rate_max = 0;

	ptiming->hsync_width = ext_lcd->hsync_width;
	ptiming->hsync_bp = ext_lcd->hsync_bp;
	ptiming->hsync_fp = ptiming->h_period - ptiming->h_active -
			ptiming->hsync_width - ptiming->hsync_bp;
	ptiming->hsync_pol   = ext_lcd->hsync_pol;
	ptiming->vsync_width = ext_lcd->vsync_width;
	ptiming->vsync_bp = ext_lcd->vsync_bp;
	ptiming->vsync_fp = ptiming->v_period - ptiming->v_active -
			ptiming->vsync_width - ptiming->vsync_bp;
	ptiming->vsync_pol   = ext_lcd->vsync_pol;
	pconf->timing.pre_de_h    = 0;
	pconf->timing.pre_de_v    = 0;

	/* fr_adjust_type */
	temp = ext_lcd->customer_val_0;
	if (temp == Rsv_val)
		ptiming->fr_adjust_type = 0xff;
	else
		ptiming->fr_adjust_type = (unsigned char)temp;
	/* ss_level */
	temp = ext_lcd->customer_val_1;
	if (temp == Rsv_val)
		pconf->timing.ss_level = 0;
	else
		pconf->timing.ss_level = temp;
	/* clk_auto_generate */
	temp = ext_lcd->customer_val_2;
	if (temp == Rsv_val) {
		pconf->timing.pll_flag = 1;
		pconf->timing.clk_mode = 0;
	} else {
		pconf->timing.pll_flag = temp & 0xf;
		pconf->timing.clk_mode = (temp >> 4) & 0xf;
	}
	/* lcd_clk */
	temp = ext_lcd->customer_val_3;
	if (temp == Rsv_val)
		ptiming->pixel_clk = 60;
	else
		ptiming->pixel_clk = temp;
	/* ppc_mode */
	temp = ext_lcd->customer_val_4;
	if (temp == Rsv_val)
		pconf->timing.ppc = 1;
	else
		pconf->timing.ppc = temp;

	ptiming->switch_type = LCD_VMODE_SWITCH_NONE;
	ptiming->ss_force = 0;
	ptiming->ss_freq = 255;
	ptiming->ss_level = pconf->timing.ss_level;
	ptiming->ss_mode = 255;

	pdrv->config.timing.dft_timing = pdrv->config.timing.timings[0];
	lcd_clk_frame_rate_init(ptiming);
	lcd_config_timing_check(pdrv, ptiming);
	lcd_default_to_basic_timing_init_config(pdrv);

	str_info_len += sprintf(str_info + str_info_len, "ppc:%d, ",
			pconf->timing.ppc);
	str_info_len += sprintf(str_info + str_info_len, "clk_mode:%d, ",
			pconf->timing.clk_mode);
	if (pconf->timing.pre_de_h || pconf->timing.pre_de_h) {
		str_info_len += sprintf(str_info + str_info_len, "pre_de:%d,%d, ",
				pconf->timing.pre_de_h, pconf->timing.pre_de_h);
	}
	str_info_len += sprintf(str_info + str_info_len, "cfg_chk:0x%x, ",
			pconf->basic.config_check);
	sprintf(str_info + str_info_len, "cus_pinmux:%d", pconf->custom_pinmux);
	LCDPR("[%d]: load bsp config: %s, %s, %dbit, %dx%d, %s\n",
	      pdrv->index, pconf->basic.model_name,
	      lcd_type_type_to_str(pconf->basic.lcd_type),
	      ptiming->lcd_bits, ptiming->h_active, ptiming->v_active,
	      str_info);

	switch (pconf->basic.lcd_type) {
	case LCD_LVDS:
		pctrl->lvds_cfg.lvds_repack = ext_lcd->lcd_spc_val0;
		pctrl->lvds_cfg.dual_port   = ext_lcd->lcd_spc_val1;
		pctrl->lvds_cfg.pn_swap     = ext_lcd->lcd_spc_val2;
		pctrl->lvds_cfg.port_swap   = ext_lcd->lcd_spc_val3;
		pctrl->lvds_cfg.lane_reverse = ext_lcd->lcd_spc_val4;
		pctrl->lvds_cfg.phy_vswing = ext_lcd->lcd_spc_val5;
		pctrl->lvds_cfg.phy_preem  = ext_lcd->lcd_spc_val6;
		pctrl->lvds_cfg.phy_clk_vswing = ext_lcd->lcd_spc_val7;
		pctrl->lvds_cfg.phy_clk_preem  = ext_lcd->lcd_spc_val8;

		phy_cfg->vswing_level = pctrl->lvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->lvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->lvds_cfg.phy_preem;
		break;
	case LCD_VBYONE:
		pctrl->vbyone_cfg.lane_count = ext_lcd->lcd_spc_val0;
		pctrl->vbyone_cfg.region_num = ext_lcd->lcd_spc_val1;
		pctrl->vbyone_cfg.byte_mode  = ext_lcd->lcd_spc_val2;
		pctrl->vbyone_cfg.color_fmt  = ext_lcd->lcd_spc_val3;
		pctrl->vbyone_cfg.phy_vswing = ext_lcd->lcd_spc_val4;
		pctrl->vbyone_cfg.phy_preem  = ext_lcd->lcd_spc_val5;
		if (ext_lcd->lcd_spc_val8 == Rsv_val ||
		    ext_lcd->lcd_spc_val9 == Rsv_val) {
			pctrl->vbyone_cfg.hw_filter_time = 0;
			pctrl->vbyone_cfg.hw_filter_cnt = 0;
		} else {
			pctrl->vbyone_cfg.hw_filter_time = ext_lcd->lcd_spc_val8;
			pctrl->vbyone_cfg.hw_filter_cnt  = ext_lcd->lcd_spc_val9;
		}

		pctrl->vbyone_cfg.ctrl_flag = 0;
		pctrl->vbyone_cfg.power_on_reset_delay = VX1_PWR_ON_RESET_DLY_DFT;
		pctrl->vbyone_cfg.hpd_data_delay = VX1_HPD_DATA_DELAY_DFT;
		pctrl->vbyone_cfg.cdr_training_hold = VX1_CDR_TRAINING_HOLD_DFT;
		pctrl->vbyone_cfg.slice = pdrv->config.timing.ppc ? pdrv->config.timing.ppc : 1;

		phy_cfg->vswing_level = pctrl->vbyone_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->vbyone_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->vbyone_cfg.phy_preem;
		break;
	case LCD_MLVDS:
		pctrl->mlvds_cfg.channel_num = ext_lcd->lcd_spc_val0;
		pctrl->mlvds_cfg.channel_sel0 = ext_lcd->lcd_spc_val1;
		pctrl->mlvds_cfg.channel_sel1 = ext_lcd->lcd_spc_val2;
		pctrl->mlvds_cfg.clk_phase  = ext_lcd->lcd_spc_val3;
		pctrl->mlvds_cfg.pn_swap    = ext_lcd->lcd_spc_val4;
		pctrl->mlvds_cfg.bit_swap   = ext_lcd->lcd_spc_val5;
		pctrl->mlvds_cfg.phy_vswing = ext_lcd->lcd_spc_val6;
		pctrl->mlvds_cfg.phy_preem  = ext_lcd->lcd_spc_val7;

		phy_cfg->vswing_level = pctrl->mlvds_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->mlvds_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->mlvds_cfg.phy_preem;
		break;
	case LCD_P2P:
		pctrl->p2p_cfg.p2p_type = ext_lcd->lcd_spc_val0;
		pctrl->p2p_cfg.lane_num = ext_lcd->lcd_spc_val1;
		pctrl->p2p_cfg.channel_sel0 = ext_lcd->lcd_spc_val2;
		pctrl->p2p_cfg.channel_sel1 = ext_lcd->lcd_spc_val3;
		pctrl->p2p_cfg.pn_swap    = ext_lcd->lcd_spc_val4;
		pctrl->p2p_cfg.bit_swap   = ext_lcd->lcd_spc_val5;
		pctrl->p2p_cfg.phy_vswing = ext_lcd->lcd_spc_val6;
		pctrl->p2p_cfg.phy_preem  = ext_lcd->lcd_spc_val7;

		phy_cfg->vswing_level = pctrl->p2p_cfg.phy_vswing & 0xf;
		phy_cfg->ext_pullup = (pctrl->p2p_cfg.phy_vswing >> 4) & 0x3;
		phy_cfg->preem_level = pctrl->p2p_cfg.phy_preem;
		break;
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_RGB:
		pctrl->rgb_cfg.type = ext_lcd->lcd_spc_val0;
		pctrl->rgb_cfg.clk_pol = ext_lcd->lcd_spc_val1;
		pctrl->rgb_cfg.de_valid = ext_lcd->lcd_spc_val2;
		pctrl->rgb_cfg.sync_valid = ext_lcd->lcd_spc_val3;
		pctrl->rgb_cfg.rb_swap = ext_lcd->lcd_spc_val4;
		pctrl->rgb_cfg.bit_swap = ext_lcd->lcd_spc_val5;
		break;
	case LCD_MIPI:
		pctrl->mipi_cfg.lane_num = ext_lcd->lcd_spc_val0;
		pctrl->mipi_cfg.bit_rate_max   = ext_lcd->lcd_spc_val1;
		pctrl->mipi_cfg.operation_mode_init = ext_lcd->lcd_spc_val3;
		pctrl->mipi_cfg.operation_mode_display = ext_lcd->lcd_spc_val4;
		pctrl->mipi_cfg.video_mode_type = ext_lcd->lcd_spc_val5;
		pctrl->mipi_cfg.clk_always_hs = ext_lcd->lcd_spc_val6;

		pctrl->mipi_cfg.panel_det_attr =
			ext_lcd->lcd_spc_val8 == Rsv_val ? 0 : ext_lcd->lcd_spc_val8;
		pctrl->mipi_cfg.dt_addr = (char *)ext_lcd->init_on;

		pctrl->mipi_cfg.check_en = 0;
		pctrl->mipi_cfg.check_reg = 0xff;
		pctrl->mipi_cfg.check_cnt = 0;
		pctrl->mipi_cfg.dsi_init_on = ext_lcd->init_on;
		pctrl->mipi_cfg.dsi_init_off = ext_lcd->init_off;
		lcd_dsi_init_table_load_bsp(&pctrl->mipi_cfg);

		if (ext_lcd->lcd_spc_val9 == Rsv_val) {
			pctrl->mipi_cfg.extern_init = 0xff;
		} else {
			pctrl->mipi_cfg.extern_init = ext_lcd->lcd_spc_val9;
			LCDPR("[%d]: extern_init: %d\n", pdrv->index, pctrl->mipi_cfg.extern_init);
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pctrl->mipi_cfg.extern_init);
#endif
		}

		phy_cfg->vswing_level = 0;
		phy_cfg->preem_level = 0;
		break;
	case LCD_EDP:
		pctrl->edp_cfg.max_lane_count = ext_lcd->lcd_spc_val0;
		pctrl->edp_cfg.max_link_rate =
			ext_lcd->lcd_spc_val1 < 0x6 ? 0 : ext_lcd->lcd_spc_val1;
		pctrl->edp_cfg.training_mode = ext_lcd->lcd_spc_val2;
		pctrl->edp_cfg.edid_en = ext_lcd->lcd_spc_val3;

		phy_cfg->vswing_level = pctrl->edp_cfg.phy_vswing_preset;
		phy_cfg->preem_level = pctrl->edp_cfg.phy_preem_preset;
		break;
#endif
	default:
		break;
	}

	phy = lcd_phy_alloc(pdrv);
	if (!phy)
		return -1;
	memset(phy, 0, sizeof(*phy));
	phy_cfg->act_phy = phy_cfg->phys[0];
	lcd_phy_param_preset(pdrv);
	lcd_lane_map_preset(pdrv);
	phy->ss.freq = 255;
	phy->ss.level = 255;
	phy->ss.mode = 255;

	lcd_ss_config_fix(pdrv);

	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		power_step = &ext_lcd->power_on_step[i];
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("power_on: step %d: type=%d, index=%d, value=%d, delay=%d\n",
			      i, power_step->type, power_step->index,
			      power_step->value, power_step->delay);
		}
		pconf->power.power_on_step[i].type = power_step->type;
		pconf->power.power_on_step[i].index = power_step->index;
		pconf->power.power_on_step[i].value = power_step->value;
		pconf->power.power_on_step[i].delay = power_step->delay;
		if (power_step->type >= LCD_POWER_TYPE_MAX)
			break;
		if (power_step->type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pconf->power.power_on_step[i].index);
#endif
		}
		i++;
	}

	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		power_step = &ext_lcd->power_off_step[i];
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("power_off: step %d: type=%d, index=%d, value=%d, delay=%d\n",
			      i, power_step->type, power_step->index,
			      power_step->value, power_step->delay);
		}
		pconf->power.power_off_step[i].type = power_step->type;
		pconf->power.power_off_step[i].index = power_step->index;
		pconf->power.power_off_step[i].value = power_step->value;
		pconf->power.power_off_step[i].delay = power_step->delay;
		if (power_step->type >= LCD_POWER_TYPE_MAX)
			break;
		if (power_step->type == LCD_POWER_TYPE_EXTERN) {
#ifdef CONFIG_AML_LCD_EXTERN
			lcd_extern_drv_index_add(pdrv->index, pconf->power.power_off_step[i].index);
#endif
		}
		i++;
	}
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_index_add(pdrv->index, 0);
#endif

	return 0;
}

/*  json  =============================================================*/
#ifdef CONFIG_AML_LCD_JSON

struct json_parse_s panel_jsp[3];

static struct num_str_s p2p_type_name[] = {
	{P2P_CEDS, "CEDS"},
	{P2P_CMPI, "CMPI"},
	{P2P_ISP,  "ISP"},
	{P2P_EPI,  "EPI"},
	{P2P_CHPI, "CHPI"},
	{P2P_CSPI, "CSPI"},
	{P2P_USIT, "USIT"},
	{P2P_MAX,  "Invalid"}
};

static struct num_str_s vmode_switch_name[] = {
	{LCD_VMODE_SWITCH_NONE,  "NONE"},
	{LCD_VMODE_SWITCH_FULL,  "FULL"},
	{LCD_VMODE_SWITCH_LIMIT, "LIMIT"},
	{LCD_VMODE_SWITCH_MIN,   "MIN"},
};

struct color_fmt_info_s color_fmt_info[] = {
	{CFMT_RGB565,         16, "RGB565"},
	{CFMT_RGB_6bit,       18, "RGB_6bit"},
	{CFMT_RGB_8bit,       24, "RGB_8bit"},
	{CFMT_RGB_10bit,      30, "RGB_10bit"},
	{CFMT_RGB_12bit,      36, "RGB_12bit"},
	{CFMT_YCbCr422_8bit,  16, "YCbCr422_8bit"},
	{CFMT_YCbCr422_10bit, 20, "YCbCr422_10bit"},
	{CFMT_YCbCr422_12bit, 24, "YCbCr422_12bit"},
	{CFMT_YCbCr444_8bit,  24, "YCbCr444_8bit"},
	{CFMT_YCbCr444_10bit, 30, "YCbCr444_10bit"},
	{CFMT_YCbCr444_12bit, 36, "YCbCr444_12bit"},
	{CFMT_YCbCr420_8bit,  12, "YCbCr420_8bit"},
	{CFMT_YCbCr420_10bit, 15, "YCbCr420_10bit"},
	{CFMT_YCbCr420_12bit, 18, "YCbCr420_12bit"},
};

static int panel_str2fmt(const char *str, unsigned char *cfmt, unsigned char *bits)
{
	unsigned int i = 0;

	if (!str)
		return -1;
	for (i = 0; i < ARRAY_SIZE(color_fmt_info); i++) {
		if (strcmp(str, color_fmt_info[i].name) == 0) {
			*cfmt = color_fmt_info[i].cfmt;
			*bits = color_fmt_info[i].bits;
			return 0;
		}
	}

	return -1;
}

struct json_parse_s *get_panel_jsp(int index)
{
	return &panel_jsp[index];
}

static int lcd_panel_parse_basic(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
	struct json_s *json, *child;
	const char *str = NULL;
	struct lcd_basic_s *cfg = &pdrv->config.basic;

	json = json_path_to_node(jsp, jsp->root, "/basic");
	if (!json) {
		LCDERR("find /basic\n");
		return -1;
	}

	str = json_get_obj_str(jsp, json, "model_name", "invalid");
	sprintf(cfg->model_name, "%s", str ? str : "invalid");

	str = json_get_obj_str(jsp, json, "interface", "invalid");
	cfg->lcd_type = lcd_type_str_to_type(str);

	cfg->config_check = json_get_obj_u32(jsp, json, "config_check", 1);
	pdrv->config.custom_pinmux = json_get_obj_u32(jsp, json, "custom_pinmux", 0);

	child = json_get_object_child(jsp, json, "screen_size");
	cfg->screen_width = json_get_arr_u32(jsp, child, 0, 16);
	cfg->screen_height = json_get_arr_u32(jsp, child, 1, 9);

	return 0;
}

static int lcd_panel_parse_timing(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
	struct json_s *parent, *child, *child2;
	const char *str = NULL;
	char strtmp[64];
	int cnt = 1, i = 0, bits = 8;
	struct lcd_detail_timing_s *dt;
	struct lcd_timing_s *tims = &pdrv->config.timing;

	parent = json_path_to_node(jsp, jsp->root, "/timing");
	if (!parent) {
		LCDERR("find /timing\n");
		return -1;
	}
	tims->ppc      = json_get_obj_u32(jsp, parent, "ppc_mode", 1);
	tims->clk_mode = json_get_obj_u32(jsp, parent, "clk_mode", LCD_CLK_MODE_DEPENDENCE);
	tims->pll_flag = json_get_obj_u32(jsp, parent, "pll_flag", 1);

	parent         = json_get_object_child(jsp, parent, "pre_de");
	tims->pre_de_h = json_get_arr_u32(jsp, parent, 0, 0);
	tims->pre_de_v = json_get_arr_u32(jsp, parent, 1, 0);

	parent = json_path_to_node(jsp, jsp->root, "/timing/timing");
	cnt = json_get_array_size(jsp, parent);
	if (cnt <= 0) {
		LCDERR("/timing/timing error\n");
		return -1;
	}

	for (i = 0; i < cnt; i++) {
		if (tims->num_timings >= LCD_MAX_NUM_TIMINGS)
			break;
		child = json_get_array_child(jsp, parent, i);
		if (!child) {
			LCDPR("fail find  timing[%d]\n", i);
			break;
		}
		dt = lcd_timing_alloc(pdrv);
		if (!dt)
			break;

		if (dt != tims->timings[0])
			memcpy(dt, tims->timings[0], sizeof(*dt));
		else
			memset(dt, 0, sizeof(*dt));

		dt->fr_adjust_type = json_get_obj_u32(jsp, child, "fr_adj_type",
						      dt->fr_adjust_type);
		dt->lcd_bits = 24;
		dt->cfmt = CFMT_RGB_8bit;
		bits = json_get_obj_u32(jsp, child, "lcd_bits", 8);
		str = json_get_obj_str(jsp, child, "color_fmt", NULL);
		if (str) {
			if (strcmp(str, "RGB565"))
				snprintf(strtmp, 63, "%s_%dbit", str, bits);
			else
				snprintf(strtmp, 63, "%s", str);

			panel_str2fmt(strtmp, &dt->cfmt, &dt->lcd_bits);
		}
		str = json_get_obj_str(jsp, child, "mode_switch_type", NULL);
		dt->switch_type = strnum_get_num(str, vmode_switch_name,
						 ARRAY_SIZE(vmode_switch_name),
						 LCD_VMODE_SWITCH_NONE);

		child2 = json_get_object_child(jsp, child, "timing");
		if (!child2 && dt == tims->timings[0]) {
			LCDPR("fail find  timing[0]->timing\n");
			lcd_timing_free_last(pdrv);
			return -1;
		}
		if (!child2) {
			LCDPR("fail find  timing[%d]->timing\n", i);
			continue;
		}
		dt->h_period    = json_get_arr_u32(jsp, child2, 0, dt->h_period);
		dt->h_active    = json_get_arr_u32(jsp, child2, 1, dt->h_active);
		dt->hsync_width = json_get_arr_u32(jsp, child2, 2, dt->hsync_width);
		dt->hsync_bp    = json_get_arr_u32(jsp, child2, 3, dt->hsync_bp);
		dt->hsync_pol   = json_get_arr_u32(jsp, child2, 4, dt->hsync_pol);
		dt->v_period    = json_get_arr_u32(jsp, child2, 5, dt->v_period);
		dt->v_active    = json_get_arr_u32(jsp, child2, 6, dt->v_active);
		dt->vsync_width = json_get_arr_u32(jsp, child2, 7, dt->vsync_width);
		dt->vsync_bp    = json_get_arr_u32(jsp, child2, 8, dt->vsync_bp);
		dt->vsync_pol   = json_get_arr_u32(jsp, child2, 9, dt->vsync_pol);
		dt->hsync_fp = dt->h_period - dt->h_active - dt->hsync_width - dt->hsync_bp;
		dt->vsync_fp = dt->v_period - dt->v_active - dt->vsync_width - dt->vsync_bp;

		child2 = json_get_object_child(jsp, child, "period_range");
		if (child2) {
			dt->h_period_min = json_get_arr_u32(jsp, child2, 0, dt->h_period_min);
			dt->h_period_max = json_get_arr_u32(jsp, child2, 1, dt->h_period_max);
			dt->v_period_min = json_get_arr_u32(jsp, child2, 2, dt->v_period_min);
			dt->v_period_max = json_get_arr_u32(jsp, child2, 3, dt->v_period_max);
		}

		child2 = json_get_object_child(jsp, child, "pclk_range");
		if (child2) {
			dt->pclk_min  = json_get_arr_u32(jsp, child2, 0, dt->pclk_min);
			dt->pclk_max  = json_get_arr_u32(jsp, child2, 1, dt->pclk_max);
			dt->pixel_clk = json_get_arr_u32(jsp, child2, 2, dt->pixel_clk);
		}

		child2 = json_get_object_child(jsp, child, "fr_range");
		if (child2) {
			dt->frame_rate_min = json_get_arr_u32(jsp, child2, 0, dt->frame_rate_min);
			dt->frame_rate_max = json_get_arr_u32(jsp, child2, 1, dt->frame_rate_max);
		}

		child2 = json_get_object_child(jsp, child, "ssc");
		if (child2) {
			dt->ss_level = json_get_obj_u32(jsp, child2, "level", 0);
			dt->ss_freq  = json_get_obj_u32(jsp, child2, "freq", 0);
			dt->ss_mode  = json_get_obj_u32(jsp, child2, "mode", 0);
			dt->ss_force = json_get_obj_u32(jsp, child2, "force", 0);
		}

		lcd_clk_frame_rate_init(dt);
		lcd_config_timing_check(pdrv, dt);
	}
	tims->dft_timing = tims->timings[0];
	lcd_default_to_basic_timing_init_config(pdrv);

	return 0;
}

static int lcd_panel_parse_phy(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
	struct json_s *parent, *child, *child2;
	const char *str = NULL;
	int cnt = 1, cnt2, i = 0, k;
	struct phy_config_s *phy_cfg;
	struct phy_attr_s *phy;
	struct ss_config_s *ss;

	parent = json_get_object_child(jsp, jsp->root, "phy");
	if (!parent) {
		LCDERR("find /phy\n");
		return -1;
	}

	phy_cfg = &pdrv->config.phy_cfg;
	phy = lcd_phy_alloc(pdrv);
	if (!phy) { //phy_cfg->phys[0] default phy
		LCDERR("%s dft phy alloc failed\n", __func__);
		return -1;
	}
	memset(phy, 0, sizeof(*phy));
	phy_cfg->act_phy = phy_cfg->phys[0];
	lcd_phy_param_preset(pdrv);
	lcd_lane_map_preset(pdrv);

	phy_cfg->lane_num = json_get_obj_u32(jsp, parent, "lane_num", phy_cfg->lane_num);
	child = json_get_object_child(jsp, parent, "ch_sel");
	if (child) {
		cnt = json_get_array_size(jsp, child);
		cnt = lcd_s32_constraint(cnt, 0, phy_cfg->lane_num);
		for (i = 0; i < cnt; i++)
			phy_cfg->ch_ctrl[i].sel = json_get_arr_u32(jsp, child, i, i);
	}
	phy_cfg->bypass_resample = json_get_obj_u32(jsp, child, "bypass_resample", 1);
	child = json_get_object_child(jsp, parent, "pn_swap");
	if (child) {
		cnt = json_get_array_size(jsp, child);
		cnt = lcd_s32_constraint(cnt, 0, phy_cfg->lane_num);
		for (i = 0; i < cnt; i++)
			phy_cfg->ch_ctrl[i].pn_swap = json_get_arr_u32(jsp, child, i, 0);
	}

	child = json_get_object_child(jsp, parent, "phase_sel");
	if (child) {
		cnt = json_get_array_size(jsp, child);
		cnt = lcd_s32_constraint(cnt, 0, phy_cfg->lane_num);
		for (i = 0; i < cnt; i++)
			phy_cfg->ch_ctrl[i].phase_sel = json_get_arr_u32(jsp, child, i, 0xff);
	}

	parent = json_get_object_child(jsp, parent, "attr");
	cnt = json_get_array_size(jsp, parent);
	if (cnt <= 0) {
		LCDPR("not find phy attr, use dft\n");
		return 0;
	}

	for (i = 0; i < cnt; i++) {
		child = json_get_array_child(jsp, parent, i);
		if (!child) {
			LCDPR("fail to find attr[%d]\n", i);
			return 0;
		}
		if (i != 0) {
			phy = lcd_phy_alloc(pdrv);
			if (!phy) {
				LCDPR("%s phy[%d] alloc fail, ignore it\n", __func__, i);
				return 0;
			}
			memcpy(phy, phy_cfg->phys[0], sizeof(*phy));
		}

		str = json_get_obj_str(jsp, child, "mode", NULL);
		phy->cv_mode   = (str && (strcmp(str, "voltage") == 0)) ? PHY_VMODE : PHY_CMODE;
		phy->phy_clk   = json_get_obj_u32(jsp, child, "phy_clk", 0);
		phy->vcm       = json_get_obj_u32(jsp, child, "vcm", phy->vcm);
		phy->odt       = json_get_obj_u32(jsp, child, "odt", phy->odt);
		phy->ref_bias  = json_get_obj_u32(jsp, child, "bias", phy->ref_bias);
		phy->vswing    = json_get_obj_u32(jsp, child, "vswing", phy->vswing);
		phy->clk_phase = json_get_obj_u32(jsp, child, "clk_phase", phy->clk_phase);

		child2 = json_get_object_child(jsp, child, "ssc");
		if (child2) {
			ss = &phy->ss;
			ss->level = json_get_obj_u32(jsp, child2, "level", ss->level);
			ss->freq  = json_get_obj_u32(jsp, child2, "freq", ss->freq);
			ss->mode  = json_get_obj_u32(jsp, child2, "mode", ss->mode);
		}

		child2 = json_get_object_child(jsp, child, "ch_preem");
		if (child2) {
			cnt2 = json_get_array_size(jsp, child2);
			cnt2 = lcd_s32_constraint(cnt2, 0, phy_cfg->lane_num);
			for (k = 0; k < cnt2; k++)
				phy->lane[k].preem = json_get_arr_u32(jsp, child2, k,
								     phy->lane[k].preem);
		}

		child2 = json_get_object_child(jsp, child, "ch_amp");
		if (child2) {
			cnt2 = json_get_array_size(jsp, child2);
			cnt2 = lcd_s32_constraint(cnt2, 0, phy_cfg->lane_num);
			for (k = 0; k < cnt2; k++)
				phy->lane[k].amp = json_get_arr_u32(jsp, child2, k,
								     phy->lane[k].amp);
		}
	}

	return 0;
}

static int lcd_panel_parse_interface(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
	struct json_s *parent;
	struct lvds_config_s   *lvds;
	struct vbyone_config_s *vx1;
	struct dsi_config_s    *mipi;
	struct mlvds_config_s  *mlvds;
	struct p2p_config_s    *p2p;
	union lcd_ctrl_config_u *cfg;
	int type, lcd_bits = pdrv->config.timing.base_timing->lcd_bits;
	const char *str;
	unsigned int *nums = NULL, nums_size = 0, cnt = 0, i = 0;

	parent = json_get_object_child(jsp, jsp->root, "interface");
	if (!parent) {
		LCDERR("find /interface\n");
		return -1;
	}

	cfg = &pdrv->config.control;
	type = pdrv->config.basic.lcd_type;
	switch (type) {
	case LCD_LVDS:
		lvds = &cfg->lvds_cfg;
		str = json_get_obj_str(jsp, parent, "lvds_fmt", NULL);
		lvds->lvds_repack  = (str && strcmp(str, "VESA") == 0) ? 1 : 0;
		if (lvds->lvds_repack)
			lvds->lvds_repack = (lcd_bits == 30) ? 2 : (lcd_bits == 18) ? 0 : 1;
		lvds->dual_port    = json_get_obj_u32(jsp, parent, "dual_port", 1);
		lvds->pn_swap      = json_get_obj_u32(jsp, parent, "pn_swap", 0);
		break;
	case LCD_VBYONE:
		vx1 = &cfg->vbyone_cfg;
		vx1->lane_count  = json_get_obj_u32(jsp, parent, "lane_num", 8);
		vx1->region_num  = json_get_obj_u32(jsp, parent, "region", 2);
		vx1->color_fmt   = 4;
		vx1->byte_mode   = (lcd_bits + 7) >> 3;
		//vx1->vsync_isr   = json_get_obj_u32(jsp, parent, "vsync_isr", 1);
		//vx1->vx1_isr     = json_get_obj_u32(jsp, parent, "vx1_isr", 1);
		vx1->hw_filter_time = json_get_obj_u32(jsp, parent, "filter_time", 0);
		vx1->hw_filter_cnt  = json_get_obj_u32(jsp, parent, "filter_cnt", 0);
		break;
	case LCD_P2P:
		p2p = &cfg->p2p_cfg;
		p2p->lane_num = json_get_obj_u32(jsp, parent, "lane_num", 0);
		str = json_get_obj_str(jsp, parent, "protocol", "Invalid");
		p2p->p2p_type = strnum_get_num(str, p2p_type_name, ARRAY_SIZE(p2p_type_name),
					       P2P_MAX);
		break;
	case LCD_MLVDS:
		mlvds = &cfg->mlvds_cfg;
		mlvds->channel_num  = json_get_obj_u32(jsp, parent, "lane_num", 0);
		break;
	case LCD_MIPI:
		mipi = &cfg->mipi_cfg;
		mipi->lane_num = json_get_obj_u32(jsp, parent, "data_lane", 0);
		mipi->bit_rate_max = json_get_obj_u32(jsp, parent, "bit_rate_max", 0);
		mipi->operation_mode_init =
				json_get_obj_u32(jsp, parent, "operation_mode_init", 0);
		mipi->operation_mode_display =
				json_get_obj_u32(jsp, parent, "operation_mode_display", 0);
		mipi->video_mode_type = json_get_obj_u32(jsp, parent, "video_mode", 0);
		mipi->clk_always_hs = json_get_obj_u32(jsp, parent, "clk_always_HS", 0);
		mipi->check_en = 0;
		mipi->check_reg = 0xff;
		mipi->check_cnt = 0;
		free(mipi->dsi_init_on);
		free(mipi->dsi_init_off);
		mipi->dsi_init_on = NULL;
		mipi->dsi_init_off = NULL;

		str = json_get_obj_str(jsp, parent, "init_on", NULL);
		if (!str) {
			LCDERR("not find mipi init_on\n");
			return -1;
		}

		nums_size = (strlen(str) + 1) * sizeof(unsigned int);
		nums = (unsigned int *)malloc(nums_size);
		if (!nums) {
			LCDERR("no memory to save nums\n");
			return -1;
		}

		memset(nums, 0, nums_size);
		cnt = string_to_numbers(str, nums);
		mipi->dsi_init_on = (unsigned char *)malloc(cnt * sizeof(unsigned char));
		if (!mipi->dsi_init_on) {
			LCDERR("no memory to save init_on data\n");
			free(nums);
			return -1;
		}
		for (i = 0; i < cnt; i++)
			mipi->dsi_init_on[i] = nums[i];

		free(nums);
		nums = NULL;

		str = json_get_obj_str(jsp, parent, "init_off", NULL);
		if (!str) {
			LCDERR("not find mipi init_off\n");
			free(nums);
			free(mipi->dsi_init_on);
			mipi->dsi_init_on = NULL;
			return -1;
		}

		nums_size = (strlen(str) + 1) * sizeof(unsigned int);
		nums = (unsigned int *)malloc(nums_size);
		if (!nums) {
			LCDERR("no memory to save nums\n");
			return -1;
		}

		memset(nums, 0, nums_size);
		cnt = string_to_numbers(str, nums);
		mipi->dsi_init_off = (unsigned char *)malloc(cnt * sizeof(unsigned char));
		if (!mipi->dsi_init_off) {
			LCDERR("no memory to save init_off data\n");
			free(nums);
			return -1;
		}
		for (i = 0; i < cnt; i++)
			mipi->dsi_init_off[i] = nums[i];

		free(nums);
		nums = NULL;

		break;
	default:
		LCDERR("can't match valid interface\n");
		return -1;
	}

	return 0;
}

struct num_str_s power_type[] = {
	{LCD_POWER_TYPE_CPU,                "gpio"},
	{LCD_POWER_TYPE_PMU,                "pmu"},
	{LCD_POWER_TYPE_SIGNAL,             "interface"},
	{LCD_POWER_TYPE_EXTERN,             "extern"},
	{LCD_POWER_TYPE_WAIT_GPIO,          "wait_gpio"},
	{LCD_POWER_TYPE_TCON_SPI_DATA_LOAD, "tcon_spi"},
	{LCD_POWER_TYPE_BACKLIGHT,          "backlight"},
	{LCD_POWER_TYPE_MUTE,               "mute"}
};

static int lcd_gpio_name_to_index(struct aml_lcd_drv_s *pdrv, const char *name)
{
	int i = 0;

	if (!name)
		return LCD_CPU_GPIO_NUM_MAX;

	for (i = 0; i < LCD_CPU_GPIO_NUM_MAX; i++)
		if (!strcmp(pdrv->config.power.cpu_gpio[i], name))
			return i;

	return LCD_CPU_GPIO_NUM_MAX;
}

static int lcd_panel_parse_power(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
	struct json_s *parent, *child;
	int cnt = 1, i = 0;
	struct lcd_power_ctrl_s *cfg = &pdrv->config.power;
	struct lcd_power_step_s *step;
	const char *str;

	parent = json_path_to_node(jsp, jsp->root, "/power_sequence/on");
	cnt = json_get_array_size(jsp, parent);
	if (cnt <= 0) {
		LCDERR("invalid /power_sequence/on\n");
		return -1;
	}

	cnt = lcd_s32_constraint(cnt, 0, LCD_PWR_STEP_MAX - 1);
	for (i = 0; i < cnt; i++) {
		child = json_get_array_child(jsp, parent, i);
		if (!child)
			return -1;

		step = &cfg->power_on_step[i];

		step->delay = json_get_arr_u32(jsp, child, 3, 0);
		step->value = json_get_arr_u32(jsp, child, 2, 0);
		str         = json_get_arr_str(jsp, child, 0, NULL);
		step->type = strnum_get_num(str, power_type, ARRAY_SIZE(power_type),
					    LCD_POWER_TYPE_MAX);

		switch (step->type) {
		case LCD_POWER_TYPE_CPU:
			str = json_get_arr_str(jsp, child, 1, NULL);
			step->index = lcd_gpio_name_to_index(pdrv, str);
			break;
		case LCD_POWER_TYPE_EXTERN:
			str = json_get_arr_str(jsp, child, 1, NULL);
			if (str && !strncmp(str, "lcd_ext_dev", 11))
				step->index = (int)strtoul(str + 11, NULL, 10);
			else
				step->index = 0xff;
			if (step->index < 255) {
				LCDPR("drv[%d] add extern device:%d\n", pdrv->index, step->index);
				lcd_extern_drv_index_add(pdrv->index, step->index);
			}
			break;
		case LCD_POWER_TYPE_MUTE:
			pdrv->status |= LCD_STATUS_PRE_MUTE;
			break;
		default:
			break;
		}
	}
	cfg->power_on_step[i].type = 0xff;
	if (lcd_debug_print_flag) {
		LCDPR("init on:\n");
		for (i = 0; i < cnt; i++) {
			step = &cfg->power_on_step[i];
			LCDPR("step[%d]: type=%d, index=%d, value=%d, delay=%d\n",
				i, step->type, step->index, step->value, step->delay);
		}
	}

	parent = json_path_to_node(jsp, jsp->root, "/power_sequence/off");
	cnt = json_get_array_size(jsp, parent);
	if (cnt <= 0) {
		LCDERR("/power_sequence/off\n");
		return -1;
	}

	cnt = lcd_s32_constraint(cnt, 0, LCD_PWR_STEP_MAX - 1);
	for (i = 0; i < cnt; i++) {
		child = json_get_array_child(jsp, parent, i);
		if (!child)
			return -1;

		step = &cfg->power_off_step[i];
		step->delay = json_get_arr_u32(jsp, child, 3, 0);
		step->value = json_get_arr_u32(jsp, child, 2, 0);
		str	    = json_get_arr_str(jsp, child, 0, NULL);
		step->type = strnum_get_num(str, power_type, ARRAY_SIZE(power_type),
					    LCD_POWER_TYPE_MAX);

		switch (step->type) {
		case LCD_POWER_TYPE_CPU:
		case LCD_POWER_TYPE_WAIT_GPIO:
			str = json_get_arr_str(jsp, child, 1, NULL);
			step->index = lcd_gpio_name_to_index(pdrv, str);
			break;
		case LCD_POWER_TYPE_EXTERN:
			str = json_get_arr_str(jsp, child, 1, NULL);
			if (str && !strncmp(str, "lcd_ext_dev", 11))
				step->index = (int)strtoul(str + 11, NULL, 10);
			else
				step->index = 0xff;
			break;
		default:
			break;
		}
	}
	cfg->power_off_step[i].type = 0xff;

	if (lcd_debug_print_flag) {
		LCDPR("init off:\n");
		for (i = 0; i < cnt; i++) {
			step = &cfg->power_off_step[i];
			LCDPR("step[%d]: type=%d, index=%d, value=%d, delay=%d\n",
				i, step->type, step->index, step->value, step->delay);
		}
	}

	return 0;
}

static int lcd_panel_parse_misc(struct json_parse_s *jsp, struct aml_lcd_drv_s *pdrv)
{
#define MAX_STR_LEN 64
	struct json_s *parent;
	char tmpstr[MAX_STR_LEN - 1];
	const char *str;
	int h = 0, v = 0, l = 0, n = 0, i;
	const char *outputmode[LCD_MAX_DRV] = {"outputmode", "outputmode2", "outputmode3"};
	const char *connector[LCD_MAX_DRV] = {"connector0_type", "connector1_type",
					      "connector2_type"};

	parent = json_path_to_node(jsp, jsp->root, "/misc");
	if (!parent) {
		LCDERR("find /misc\n");
		return -1;
	}

	str = json_get_obj_str(jsp, parent, "connector_type", NULL);
	if (str) {
		snprintf(tmpstr, MAX_STR_LEN, "setenv connector0_type %s", str);
		run_command(tmpstr, 0);
	}

	for (i = 0; i < LCD_MAX_DRV; i++) {
		str = json_get_obj_str(jsp, parent, outputmode[i], NULL);
		if (str) {
			snprintf(tmpstr, MAX_STR_LEN, "setenv %s %s", outputmode[i], str);
			run_command(tmpstr, 0);
		}
		str = json_get_obj_str(jsp, parent, connector[i], NULL);
		if (str) {
			snprintf(tmpstr, MAX_STR_LEN, "setenv %s %s", connector[i], str);
			run_command(tmpstr, 0);
		}
	}

	h = json_get_obj_u32(jsp, parent, "hmirror", 0);
	v = json_get_obj_u32(jsp, parent, "vmirror", 0);
	l = json_get_obj_u32(jsp, parent, "layer", 4);

	n = h << 1 | v;
	snprintf(tmpstr, MAX_STR_LEN, "setenv osd_reverse %s%s",
		l == 0 ? "osd0," : l == 1 ? "osd1," : "all,",
		n == 0 ? "n" : n == 1 ? "y_rev" : n == 2 ? "x_rev" : "true");
	run_command(tmpstr, 0);

	snprintf(tmpstr, MAX_STR_LEN, "setenv video_reverse %c",
		n == 0 ? '0' : n == 1 ? '3' : n == 2 ? '2' : '1');
	run_command(tmpstr, 0);

	snprintf(tmpstr, MAX_STR_LEN, "setenv panel_reverse %c",
		n == 0 ? '0' : n == 1 ? '3' : n == 2 ? '2' : '1');
	run_command(tmpstr, 0);

	return 0;
#undef MAX_STR_LEN
}

int panel_json_parse(struct json_parse_s *jsp, unsigned char *input)
{
	if (!input) {
		LCDERR("%s panel file not ready\n", __func__);
		jsp->status = JSON_STATUS_NO_FILE;
		return -1;
	}

	if (json_init(jsp, JSON_STR_MAX, JSON_NODE_MAX) < 0) {
		json_deinit(jsp);
		jsp->status = JSON_STATUS_ERROR;
		LCDERR("%s jsp init failed\n", __func__);
		return -1;
	}

	if (!json_parse(jsp, (char *)input, JSON_STR_MAX)) {
		json_deinit(jsp);
		jsp->status = JSON_STATUS_ERROR;
		LCDERR("%s jsp parse failed\n", __func__);
		return -1;
	}

	jsp->status = JSON_STATUS_OK;
	return 0;
}

static int lcd_config_load_from_json(struct aml_lcd_drv_s *pdrv)
{
#define JSON_PANEL_HANDLE_HEAD_SIZE (32)
	int index = 0, ret = 0;
	unsigned char *p;//, *save;
	char name[64];
	struct json_parse_s *jsp;
	struct json_panel_handle_head_s {
		unsigned int size;
		unsigned int json_cnt;
		unsigned int js_len;
		unsigned int json_start;
		unsigned int js_start;
		unsigned char rsvd[JSON_PANEL_HANDLE_HEAD_SIZE - 20];
	} head; //for make memory handle to kernel

	index = pdrv->index;
	jsp = &panel_jsp[index];
	if (!json_parse_ok(jsp)) {
		ret = panel_json_parse(jsp, get_panel_file(index, NULL));
		if (ret) {
			rm_panel_file(index);
			return -1;
		}
	}

	/*parse basic*/
	if (lcd_panel_parse_basic(jsp, pdrv) < 0) {
		ret = -2;
		goto parse_panel_err_exit;
	}

	/*misc*/
	lcd_panel_parse_misc(jsp, pdrv);

	/*parse timing*/
	if (lcd_panel_parse_timing(jsp, pdrv) < 0) {
		ret = -3;
		goto parse_panel_err_exit;
	}

	/*parse phy*/
	if (lcd_panel_parse_phy(jsp, pdrv) < 0) {
		ret = -4;
		goto parse_panel_err_exit;
	}

	/*parse interface*/
	if (lcd_panel_parse_interface(jsp, pdrv) < 0) {
		ret = -5;
		goto parse_panel_err_exit;
	}

	/*parse vlock,   uboot no need*/

	/*parse sw_vlock,   uboot no need*/

	/*parse sw_pdf,   uboot no need*/

	/*parse sw_pol,   uboot no need*/

	/*parse hdr,   uboot no need*/

	/*parse power sequence*/
	if (lcd_panel_parse_power(jsp, pdrv) < 0) {
		ret = -6;
		goto parse_panel_err_exit;
	}

	//lcd_panel_parse_data(jsp, pdrv);

#ifdef CONFIG_AML_LCD_BACKLIGHT
		aml_bl_index_add(pdrv->index, 0);
#endif

/* save jsp to reserved memory for kernel use */
	sprintf(name, "panel%d_jsp", index);

	//|size(4)|json_cnt(4)|js_len(4)|json_start(4)|(js_start)
	head.json_cnt = jsp->json_cnt;
	head.js_len = jsp->js_len;
	head.json_start = JSON_PANEL_HANDLE_HEAD_SIZE;
	head.json_start = ALIGN(head.json_start, 16);
	head.js_start = head.json_start + head.json_cnt * sizeof(*jsp->root);
	head.js_start = ALIGN(head.js_start, 16);
	head.size = head.js_start + head.js_len;
	head.size = ALIGN(head.size, 16);
	p = (unsigned char *)malloc(head.size);
	if (p) {
		memcpy(p, &head, JSON_PANEL_HANDLE_HEAD_SIZE);
		memcpy(p + head.json_start, jsp->root, jsp->json_cnt * sizeof(*jsp->root));
		memcpy(p + head.js_start, jsp->js, jsp->js_len);
		panel_param_mem_put(p, name, head.size);
		free(p);
		p = NULL;
	}
parse_panel_err_exit:
		rm_panel_file(index);

	if (ret)
		LCDPR("%s fatal error ret = %d\n", __func__, ret);

	return ret;
}
#else
static inline int lcd_config_load_from_json(struct aml_lcd_drv_s *pdrv)
{
	return -1;
}
#endif

static unsigned int lcd_dt_valid(char *dt_addr, int index)
{
	int parent_offset, ret = 0;
	char str[16];
	char *propdata;

	if (index == 0)
		sprintf(str, "/lcd");
	else
		sprintf(str, "/lcd%d", index);

	parent_offset = fdt_path_offset(dt_addr, str);
	if (!parent_offset)
		return 0;
	/* check lcd status enable or not */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (propdata && strncmp(propdata, "okay", 2) == 0)
		ret = 1;
	else
		LCDERR("[%d]: lcd disabled\n", index);

	return ret;
}

unsigned char lcd_panel_config_load_detect(int index, int dt_valid, int key_valid)
{
	unsigned char load = LCD_CONFIG_NONE;
	unsigned char file_type = PANEL_FILE_INVILD;

	file_type = get_lcd_panel_file_type(index);
	load = lcd_get_dbg_source();
	if (load != LCD_CONFIG_NONE) {
		switch (load) {
		case LCD_CONFIG_DTS:
			if (!dt_valid)
				load = LCD_CONFIG_NONE;
			break;
		case LCD_CONFIG_UKEY:
			if (!key_valid)
				load = LCD_CONFIG_NONE;
			break;
		case LCD_CONFIG_BSP:
			break;
		case LCD_CONFIG_FILE:
			if (file_type != PANEL_FILE_JSON && file_type != PANEL_FILE_INI)
				load = LCD_CONFIG_NONE;
			break;
		default:
			load = LCD_CONFIG_NONE;
		}
		return load;
	}

	if (file_type == PANEL_FILE_INI || file_type == PANEL_FILE_JSON) {
		load = LCD_CONFIG_FILE;
	} else {
		if (key_valid)
			load = LCD_CONFIG_UKEY;
		else if (dt_valid)
			load = LCD_CONFIG_DTS;
		else
			load = LCD_CONFIG_BSP;
	}

	return load;
}

static int lcd_check_config_load(struct aml_lcd_drv_s *pdrv)
{
	int ret = 0, dt_sta;

	dt_sta = lcd_dt_valid(lcd_get_dt_addr(), pdrv->index);
	pdrv->config_load = lcd_panel_config_load_detect(pdrv->index, dt_sta, pdrv->key_valid);
	if (pdrv->config_load == LCD_CONFIG_NONE) {
		LCDERR("[%d] config_load_check error: config_load:%d, dt_status:%d, key:%d",
			pdrv->index, pdrv->config_load, dt_sta, pdrv->key_valid);
		return -1;
	}

	return ret;
}

static void lcd_config_load_init(struct aml_lcd_drv_s *pdrv)
{
	unsigned int dbg_chk;

	dbg_chk = env_get_ulong("lcd_debug_check", 10, 0xff);
	if (dbg_chk == 0xff) {
		if (pdrv->config.basic.config_check & 0x2)
			pdrv->config_check_en = pdrv->config.basic.config_check & 0x1;
		else
			pdrv->config_check_en = pdrv->config_check_glb;
	} else {
		LCDPR("lcd_debug_check: %d\n", dbg_chk);
		pdrv->config_check_en = dbg_chk;
	}

	if (pdrv->index)
		pdrv->config.timing.ppc = 1;
}

int lcd_get_panel_config(char *dt_addr, int load_id, struct aml_lcd_drv_s *pdrv)
{
	int ret = -1;
	unsigned char file_type = PANEL_FILE_INVILD;

	if (lcd_check_config_load(pdrv))
		return -1;
	load_id = pdrv->config_load;

	switch (load_id) {
	case LCD_CONFIG_FILE:
		file_type = get_lcd_panel_file_type(pdrv->index);
		if (file_type == PANEL_FILE_JSON)
			ret = lcd_config_load_from_json(pdrv);
		else if (file_type == PANEL_FILE_INI)
			ret = -1; //todo
		break;
	case LCD_CONFIG_UKEY:
		ret = lcd_config_load_from_unifykey(pdrv);
		break;
	case LCD_CONFIG_DTS:
		ret = lcd_config_load_from_dts(dt_addr, pdrv);
		break;
	case LCD_CONFIG_BSP:
		ret = lcd_config_load_from_bsp(pdrv);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret)
		return -1;

	lcd_lane_map_update(pdrv);

	lcd_config_load_init(pdrv);
	lcd_config_load_print(pdrv);
	lcd_pinmux_load_config(pdrv);

#ifdef CONFIG_AML_LCD_TCON
	lcd_tcon_probe(dt_addr, pdrv, load_id);
#endif
	return 0;
}
