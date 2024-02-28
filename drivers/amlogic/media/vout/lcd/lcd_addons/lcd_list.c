// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#include <asm/gpio.h>
#include <amlogic/media/vout/lcd/lcd_timing.h>
#include "../lcd_common.h"

#ifdef CONFIG_DTB_MEM_ADDR
char *dt_addr = (char *)CONFIG_DTB_MEM_ADDR;
#else
char *dt_addr = (char *)0x01000000;
#endif

unsigned char dtimg_info_add(char *c_buf, struct lcd_detail_timing_s *dtm, unsigned char c_bits)
{
	unsigned long long temp, temp2;
	//unsigned short ht, vt;
	unsigned char c_pos = 0;//, cnt;

	if (dtm->pixel_clk > 1000) {
		temp = dtm->pixel_clk; temp *= 100;
		temp = div_around(div_around(temp, dtm->h_period), dtm->v_period);
		c_pos += snprintf(c_buf, 128 - c_pos, "%-4hu*%-4hu@%3llu.%-2lluHz ",
			dtm->h_active, dtm->v_active, temp / 100, temp % 100);
	} else {
		c_pos += snprintf(c_buf, 128 - c_pos, "%-4u*%-4u@%3u.00Hz ",
			dtm->h_active, dtm->v_active, dtm->pixel_clk ? dtm->pixel_clk : 60);
	}
	c_pos += snprintf(c_buf + c_pos, 128 - c_pos, "%2ub ", c_bits);

	if (dtm->vfreq_vrr_min && dtm->vfreq_vrr_max) {
		c_pos += snprintf(c_buf + c_pos, 128 - c_pos, "(VRR:%2huu~%huHz) ",
			dtm->vfreq_vrr_min, dtm->vfreq_vrr_max);
	}

	if (dtm->h_period_min && dtm->h_period_max && dtm->v_period_min && dtm->v_period_max) {
		temp = dtm->pclk_min; temp *= 1000;
		temp = div_around(div_around(temp, dtm->h_period_max), dtm->v_period_max);
		temp2 = dtm->pclk_max; temp2 *= 1000;
		temp2 = div_around(div_around(temp2, dtm->h_period_min), dtm->v_period_min);
		c_pos += snprintf(c_buf + c_pos, 128 - c_pos, "(MediaSync:%llu.%2llu~%llu.%2lluHz)",
			temp / 1000, temp % 1000, temp2 / 1000, temp2 % 1000);
	}

	//if ((dtm->fixed_type != 0xff) && (dtm->fixed_val_set[0] != 0)) {
	//	c_pos += snprintf(c_buf + c_pos, 128 - c_pos, "(fr:");
	//	for (cnt = 0; cnt < 8; cnt++) {
	//		if (dtm->fixed_val_set[cnt] == 0)
	//			break;
	//		temp = (dtm->fixed_type == 0) ? dtm->fixed_val_set[cnt] : dtm->pixel_clk;
	//		ht   = (dtm->fixed_type == 1) ? dtm->fixed_val_set[cnt] : dtm->h_period;
	//		vt   = (dtm->fixed_type == 2) ? dtm->fixed_val_set[cnt] : dtm->v_period;
	//		temp = div_around(div_around(temp * 1000, vt), ht);
	//		c_pos += snprintf(c_buf + c_pos, 128 - c_pos, "%llu.%llu ",
	//			temp / 1000, temp % 1000);
	//	}
	//	c_pos += snprintf(c_buf + c_pos - 1, 128 - c_pos, ")") - 1;
	//}

	return c_pos;
}

#define IF_LEN 80
static unsigned char interface_info_add(char *str, int node_ofst, unsigned char port_type)
{
	char *propdata;
	unsigned char c_pos = 0;
	unsigned short tmp;

	switch (port_type) {
	case LCD_EDP:
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "edp_attr", NULL);
		if (!propdata)
			return 0;
		tmp = (unsigned char)be32_to_cpup((u32 *)propdata);
		if (tmp)
			c_pos = snprintf(str, IF_LEN, "lane:%1u, ", tmp);
		else
			c_pos = snprintf(str, IF_LEN, "lane:auto, ");

		tmp = (unsigned int)be32_to_cpup(((u32 *)propdata) + 1);
		if (tmp < 0x6)
			c_pos += snprintf(str + c_pos, IF_LEN - c_pos, "link:auto, ");
		else
			c_pos += snprintf(str + c_pos, IF_LEN - c_pos, "link:%u.%-2uG, ",
					tmp * 27 / 100, tmp * 27 % 100);

		tmp = (unsigned char)be32_to_cpup(((u32 *)propdata) + 2);
		if (tmp == 2)
			c_pos += snprintf(str + c_pos, IF_LEN - c_pos, "full");
		else
			c_pos += snprintf(str + c_pos, IF_LEN - c_pos, tmp ? "fast" : "auto");
		c_pos += snprintf(str + c_pos, IF_LEN - c_pos, " LT, ");

		tmp = (unsigned char)be32_to_cpup(((u32 *)propdata) + 3);
		c_pos += snprintf(str + c_pos, IF_LEN - c_pos, "EDID en:%d", tmp);
		break;
	case LCD_MIPI:
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "mipi_attr", NULL);
		if (!propdata)
			return 0;
		tmp = (unsigned char)be32_to_cpup((u32 *)propdata);
		c_pos = snprintf(str, IF_LEN, "%1u lane", tmp);

		tmp = (unsigned int)be32_to_cpup(((u32 *)propdata) + 1);
		c_pos += snprintf(str + c_pos, IF_LEN, ", bitrate:%3uMHz", tmp);

		tmp = (unsigned char)be32_to_cpup(((u32 *)propdata) + 5);
		c_pos += snprintf(str + c_pos, IF_LEN, ", %sburst", tmp == 2 ? "" : "non-");
		if (tmp != 2) {
			c_pos += snprintf(str + c_pos, IF_LEN, "(sync %s)",
				tmp == 1 ? "event" : "pulse");
		}
		tmp = (unsigned char)be32_to_cpup(((u32 *)propdata) + 6);
		c_pos += snprintf(str + c_pos, IF_LEN, ", clk_HS:%u", tmp);
		break;
	case LCD_VBYONE:
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "vbyone_attr", NULL);
		if (!propdata)
			return 0;
		tmp = (unsigned char)be32_to_cpup((u32 *)propdata);
		c_pos = snprintf(str, IF_LEN, "%u lane, ", tmp);

		tmp = (unsigned int)be32_to_cpup(((u32 *)propdata) + 1);
		c_pos += snprintf(str + c_pos, IF_LEN, "%1u Region", tmp);
		break;
	case LCD_LVDS:
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "lvds_attr", NULL);
		if (!propdata)
			return 0;
		tmp = (unsigned char)be32_to_cpup(((u32 *)propdata) + 1);
		c_pos = snprintf(str, IF_LEN, "%u port, ", tmp ? 2 : 1);

		tmp = (unsigned int)be32_to_cpup(((u32 *)propdata) + 3);
		if (tmp)
			c_pos += snprintf(str + c_pos, IF_LEN, "port swap,  ");

		tmp = (unsigned int)be32_to_cpup(((u32 *)propdata) + 4);
		if (tmp)
			c_pos += snprintf(str + c_pos, IF_LEN, "lane reverse");
		break;
	case LCD_RGB:
	default:
		break;
	}
	return c_pos;
}

#define LCD_EXT_TIMING_MAX 8
static void ext_timing_info_add(char *dt_addr, int node_ofst)
{
	unsigned char etmg_idx, c_bit;//, tmp;
	//int len = 0;
	char *propdata, snode[32];
	struct lcd_detail_timing_s dtm;
	char cstr[128];

	for (etmg_idx = 0; etmg_idx < LCD_EXT_TIMING_MAX; etmg_idx++) {
		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (!propdata)
			break;
		memset(cstr, 0, 128 * sizeof(char));
		memset(&dtm, 0, sizeof(struct lcd_detail_timing_s));
		memset(snode, 0, 32 * sizeof(char));

		dtm.h_period = be32_to_cpup(((u32 *)propdata) + 0);
		dtm.h_active = be32_to_cpup(((u32 *)propdata) + 1);
		dtm.v_period = be32_to_cpup(((u32 *)propdata) + 5);
		dtm.v_active = be32_to_cpup(((u32 *)propdata) + 6);
		dtm.pixel_clk = be32_to_cpup(((u32 *)propdata) + 13);
		//dtm.fixed_type = 0xff;

		c_bit = be32_to_cpup(((u32 *)propdata) + 10);

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_frame_rate", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			dtm.vfreq_vrr_min = be32_to_cpup(((u32 *)propdata) + 0);
			dtm.vfreq_vrr_max = be32_to_cpup(((u32 *)propdata) + 1);
		}

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_range_setting", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			dtm.h_period_min = be32_to_cpup(((u32 *)propdata) + 0);
			dtm.h_period_max = be32_to_cpup(((u32 *)propdata) + 1);
			dtm.v_period_min = be32_to_cpup(((u32 *)propdata) + 2);
			dtm.v_period_max = be32_to_cpup(((u32 *)propdata) + 3);
			dtm.pclk_min     = be32_to_cpup(((u32 *)propdata) + 4);
			dtm.pclk_max     = be32_to_cpup(((u32 *)propdata) + 5);
		}

		//memset(snode, 0, 32 * sizeof(char));
		//sprintf(snode, "extend_tmg_%hu_fixed", etmg_idx);
		//propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, &len);
		//if (propdata) {
		//	dtm.fixed_type = be32_to_cpup((u32 *)propdata);
		//	len = (len > 36) ? 8 : (len / 4 - 1);
		//	for (tmp = 0; tmp < len; tmp++)
		//		dtm.fixed_val_set[tmp] = be32_to_cpup(((u32 *)propdata) + tmp + 1);
		//}

		dtimg_info_add(cstr, &dtm, c_bit);
		printf("%102s%s\n", "", cstr);
	}
}

static void lcd_list_dts(unsigned char drv_idx, int parent_ofst)
{
	int node_ofst;
	char *propdata, snode[20], *node_name, *panel_type, c_buf[256], line_buf[256];
	unsigned char lcd_type, c_pos, c_bit;
	unsigned int tmp;
	struct lcd_detail_timing_s dtm;
	short curr_port;

	curr_port = -1;
	if (drv_idx == 0)
		sprintf(snode, "panel_type");
	else
		sprintf(snode, "panel%d_type", drv_idx);
	panel_type = env_get(snode);

	// printf("\n%s panel list:\n", snode);
	printf("\nlcd%u_type   | %-*s| port  | %-*s| resolution\n",
		drv_idx, 20, "name", 56, "interface info");

	fdt_for_each_subnode(node_ofst, dt_addr, parent_ofst) {
		c_pos = 0;
		memset(c_buf, 0, 256 * sizeof(char));
		memset(&dtm, 0, sizeof(struct lcd_detail_timing_s));

		node_name = (char *)fdt_get_name(dt_addr, node_ofst, NULL);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "model_name", NULL);
		if (!propdata)
			continue;

		c_pos = snprintf(c_buf, 256, "%s %-10s| %-20s|",
			strcmp(panel_type, node_name) ? " " : "*", node_name, propdata);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "interface", NULL);
		lcd_type = lcd_type_str_to_type(propdata);
		if (!strcmp(panel_type, node_name))
			curr_port = lcd_type;

		c_pos += snprintf(c_buf + c_pos, 256 - c_pos, " %-6s| ", propdata);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "basic_setting", NULL);
		dtm.h_active = be32_to_cpup(((u32 *)propdata) + 0);
		dtm.v_active = be32_to_cpup(((u32 *)propdata) + 1);
		dtm.h_period = be32_to_cpup(((u32 *)propdata) + 2);
		dtm.v_period = be32_to_cpup(((u32 *)propdata) + 3);
		c_bit = be32_to_cpup(((u32 *)propdata) + 4);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "clk_attr", NULL);
		tmp = be32_to_cpup((u32 *)propdata);
		dtm.pixel_clk = be32_to_cpup(((u32 *)propdata) + 3);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "range_setting", NULL);
		if (propdata && tmp != 0xff) {
			dtm.h_period_min = dtm.h_period; dtm.h_period_max = dtm.h_period;
			dtm.v_period_min = dtm.v_period; dtm.v_period_max = dtm.v_period;
			if (dtm.pixel_clk > 1000) {
				dtm.pclk_min = dtm.pixel_clk;
				dtm.pclk_max = dtm.pixel_clk;
			} else {
				dtm.pclk_min = dtm.v_period * dtm.h_period *
						(dtm.pixel_clk ? dtm.pixel_clk : 60);
				dtm.pclk_max = dtm.pclk_min;
			}
			if (tmp == 1 || tmp == 3) {
				dtm.h_period_min = be32_to_cpup(((u32 *)propdata) + 0);
				dtm.h_period_max = be32_to_cpup(((u32 *)propdata) + 1);
			}
			if (tmp == 2 || tmp == 3) {
				dtm.v_period_min = be32_to_cpup(((u32 *)propdata) + 2);
				dtm.v_period_max = be32_to_cpup(((u32 *)propdata) + 3);
			}
			if (tmp == 0 || tmp == 3) {
				dtm.v_period_min = be32_to_cpup(((u32 *)propdata) + 4);
				dtm.v_period_max = be32_to_cpup(((u32 *)propdata) + 5);
			}
		}

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "range_frame_rate", NULL);
		if (propdata) {
			dtm.frame_rate_min = be32_to_cpup(((u32 *)propdata) + 1);
			dtm.frame_rate_max = be32_to_cpup(((u32 *)propdata) + 0);
		}

		//dtm.fixed_type = 0xff;
		c_pos += interface_info_add(c_buf + c_pos, node_ofst, lcd_type);
		memset(line_buf, 0, 256 * sizeof(char));
		c_pos = snprintf(line_buf, 256, "%-100s| ", c_buf);

		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "ppc_mode", NULL);
		if (propdata) {
			c_pos += snprintf(line_buf + c_pos, 256 - c_pos, "%hu ppc ",
				(unsigned short)(be32_to_cpup((u32 *)propdata)));
		}

		dtimg_info_add(line_buf + c_pos, &dtm, c_bit);
		printf("%s\n", line_buf);

		ext_timing_info_add(dt_addr, node_ofst);
	}

	sprintf_lcd_connector(line_buf, drv_idx, curr_port);

	printf("lcd%u as connector %s\n", drv_idx, line_buf);
}

void aml_lcd_list(void)
{
#ifndef CONFIG_OF_LIBFDT
	printf("LIBFDT not available\n");
#else
	int parent_ofst;
	char *propdata, snode[20];
	unsigned char i, key_valid;

	for (i = 0; i < 3; i++) {
		if (i == 0)
			sprintf(snode, "/lcd");
		else
			sprintf(snode, "/lcd%d", i);
		parent_ofst = fdt_path_offset(dt_addr, snode);
		propdata = (char *)fdt_getprop(dt_addr, parent_ofst, "key_valid", NULL);
		if (!propdata)
			continue;

		key_valid = be32_to_cpup((u32 *)propdata);

		if (key_valid) {
			if (i == 0)
				sprintf(snode, "model_list");
			else
				sprintf(snode, "model%u_list", i);
			run_command(snode, 0);
		} else {
			lcd_list_dts(i, parent_ofst);
		}
	}
#endif
}

static unsigned char lcd_set_dts(uint8_t drv_idx, char *lcd_type_name)
{
	char snode[8] = "/lcd\0\0";
	char *propdata, *node_name, target_connector[12], *env_connector;
	int node_ofst;
	uint8_t i, lcd_type = LCD_TYPE_MAX;

	if (drv_idx)
		snode[4] = '0' + drv_idx;

	fdt_for_each_subnode(node_ofst, dt_addr, fdt_path_offset(dt_addr, snode)) {
		node_name = (char *)fdt_get_name(dt_addr, node_ofst, NULL);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, "interface", NULL);
		if (!propdata)
			break;

		if (strcmp(node_name, lcd_type_name))
			continue;
		lcd_type = lcd_type_str_to_type(propdata);
		break;
	}

	if (lcd_type == LCD_TYPE_MAX) {
		printf("type (%s) not found in dts node %s\n", lcd_type_name, snode);
		return -1;
	}
	sprintf_lcd_connector(target_connector, drv_idx, lcd_type);

	for (i = 0; i < 3; i++) {
		env_connector = get_current_env_connector(i);
		if (!env_connector)
			continue;

		if (!strcmp(target_connector, env_connector)) {
			printf("lcd%u set type %s, as %s, on connector[%u]\n",
				drv_idx, lcd_type_name, target_connector, i);
			return 0;
		}
	}

	printf(" * lcd%u set type %s, as %s, no connector match\n"
	       " * run \"setenv connector0_type/connector1_type/connector2_type %s\"\n\n",
		drv_idx, lcd_type_name, target_connector, target_connector);
	return 0;
}

void aml_lcd_set(uint8_t drv_idx, char *lcd_type_name)
{
#ifndef CONFIG_OF_LIBFDT
	printf("LIBFDT not available\n");
#else
	int parent_ofst;
	char *propdata, cmd[64], snode[16] = "/lcd\0\0";
	unsigned char key_valid;

	if (drv_idx)
		snode[4] = '0' + drv_idx;

	parent_ofst = fdt_path_offset(dt_addr, snode);
	propdata = (char *)fdt_getprop(dt_addr, parent_ofst, "key_valid", NULL);
	if (!propdata) {
		printf("lcd[%hu] <key_valid> not found in dts node %s\n", drv_idx, snode);
		return;
	}

	key_valid = be32_to_cpup((u32 *)propdata);

	if (key_valid) {
		if (drv_idx == 0)
			sprintf(snode, "model_name");
		else
			sprintf(snode, "model%u_name", drv_idx);
	} else {
		if (lcd_set_dts(drv_idx, lcd_type_name))
			return;

		if (drv_idx == 0)
			sprintf(snode, "panel_type");
		else
			sprintf(snode, "panel%d_type", drv_idx);
	}
	sprintf(cmd, "setenv %s %s", snode, lcd_type_name);
	run_command(cmd, 0);
	sprintf(cmd, "update_env_part -p -f %s", snode);
	run_command(cmd, 0);

#endif
}
