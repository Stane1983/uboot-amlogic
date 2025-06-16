// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <command.h>
#include <env.h>
#include <malloc.h>
#include "../../lcd_common.h"
#include "lcd_tcon.h"
#include "lcd_tcon_dccd.h"

#ifndef CONFIG_CMD_INI

struct tcon_dccd_conf_s {
	unsigned short calc_chksum;  //calculated checksum, need bit[7:0] = 0x00
	unsigned char checksum;  //checksum byte inside dccd buffer
	unsigned char is_dccd;   //check support dccd

	unsigned char has_tcon_file;  //check TCON_BIN_PATH
	unsigned int is_dccd_flow;  //check need to run dccd flow
};

struct dccd_base_info_s {
	unsigned int dccd;
	unsigned char minor_ver:4;
	unsigned char major_ver:4;
	unsigned char port_idx:4;
	unsigned char reserved0:2;
	unsigned char port_type:2;
	unsigned char capability1:4;
	unsigned char dev_type:4;
	unsigned char capability2;
	unsigned char capability3;
	unsigned char capability4;
	unsigned short len;  //all others info len
} __packed;

static struct tcon_dccd_conf_s dccd_conf;

static void tcon_dccd_check_and_update(struct tcon_dccd_conf_s *dccd,
		unsigned char *dccd_buf, unsigned int buf_size)
{
	struct dccd_base_info_s *basic = NULL;
	int bufidx = 0, i = 0;

	if (!dccd || !dccd_buf || buf_size <= 0)
		return;

	basic = (struct dccd_base_info_s *)dccd_buf;

	//check if it's dccd bin
	if (basic->dccd != 0x0d0c0c0d) {
		LCDERR("%s: it's not dccd bin(%#x)\n", __func__, basic->dccd);
		return;
	}

	//check dccd checksum
	bufidx = sizeof(*basic) + basic->len - 1;
	if (bufidx >= buf_size) {
		LCDERR("%s: dccd len not match\n", __func__);
		return;
	}
	dccd->checksum = dccd_buf[bufidx];

	//check crc, calculate skip crc/checksum
	for (i = 0, dccd->calc_chksum = 0; i <= bufidx; i++)
		dccd->calc_chksum += dccd_buf[i];

	dccd->is_dccd = !(dccd->calc_chksum & 0xff);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s: dccd raw checksum=%#x, calc checksum=%#x, %s\n",
			__func__, dccd->checksum, dccd->calc_chksum,
			dccd->is_dccd ? "matched" : "miss-matched");
	}
}

#ifdef CONFIG_CMD_AML_MODEL
static const char *tcon_ini_get_val(void *inip,
		const char *sec_name, const char *item_name, const char *def_val)
{
	void *psec;
	const char *str;

	psec = lcd_ini_get_section(inip, sec_name);
	if (!psec)
		return def_val;

	str = lcd_ini_get_str(inip, psec, item_name, NULL);
	if (!str)
		return def_val;

	return str;
}
#endif

#ifdef CONFIG_AML_LCD_JSON
static const char *tcon_json_get_val(struct json_parse_s *jsp,
		const char *sec_name, const char *item_name, const char *def_val)
{
	struct json_s *parent;
	const char *str;

	parent = json_get_object_child(jsp, jsp->root, sec_name);
	if (!parent)
		return def_val;

	str = json_get_obj_str(jsp, parent, item_name, NULL);
	if (!str)
		return def_val;

	return str;
}
#endif

static int tcon_dccd_update_timing(struct aml_lcd_drv_s *pdrv, unsigned char *dccd_buf)
{
	struct lcd_detail_timing_s *dt;
	int block_size = 0, data_start = 0, data = 0;
	unsigned int h_blk = 0, v_blk = 0, lcd_if = 0, lane_num = 0;
	unsigned char ver = 0;

	if (!pdrv || !dccd_buf)
		return -1;

	dt = pdrv->config.timing.dft_timing;

	//0x1f: save product Name length
	block_size = dccd_buf[0x1f];
	data_start =  0x1f + block_size + 1;

	//0x80: panel timing data block
	if (dccd_buf[data_start] == 0x80) {
		/* offset 0x1: [7:4]:version, [3:0]:reserved
		 * 0x2: data_length
		 * 0x3: h_active[7:0]
		 * 0x4: h_active[15:8]
		 * 0x5: h_blank[7:0]
		 * 0x6: h_blank[15:8]
		 * 0x7: hsync_fp[7:0]
		 * 0x8: hsync_fp[15:8]
		 * 0x9: hsync_width[7:0]
		 * 0xa: bit7: hsync_pol, [6:0]:hsync_width[14:8]
		 * 0xb: v_active[7:0]
		 * 0xc: v_active[15:8]
		 * 0xf: vsync_fp[7:0]
		 * 0x10: vsync_fp[15:8]
		 * 0x11: vsync_width[7:0]
		 * 0x12: bit7:vsync_pol, [6:0]: vsync_width[14:8]
		 * 0x13~0x16: pixel_clk? [32:0]
		 * 0x17: [3: 0]: lcd_bits(0:6bit, 1:8bit, 2:10bit, 3:12bit..)
		 */
		ver = dccd_buf[data_start + 0x1];
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, version is (%d)\n", __func__, ver);

		//lcd_timing
		dt->h_active = dccd_buf[data_start + 0x3] |
					dccd_buf[data_start + 0x4] << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, h_active is (%d)\n", __func__, dt->h_active);

		h_blk = dccd_buf[data_start + 0x5] |
					dccd_buf[data_start + 0x6] << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, h_blank is (%d)\n", __func__, h_blk);
		dt->h_period = dt->h_active + h_blk;

		dt->hsync_fp = dccd_buf[data_start + 0x7] |
					dccd_buf[data_start + 0x8] << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, hsync_fp is (%d)\n", __func__, dt->hsync_fp);

		dt->hsync_width = dccd_buf[data_start + 0x9] |
					(dccd_buf[data_start + 0xa] & 0x7f) << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, hsync_width is (%d)\n", __func__, dt->hsync_width);

		dt->hsync_bp = h_blk - dt->hsync_fp - dt->hsync_width;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, hsync_bp is (%d)\n", __func__, dt->hsync_bp);

		dt->hsync_pol = (dccd_buf[data_start + 0xa] & 0x80) >> 7;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, hsync_pol is (%d)\n", __func__, dt->hsync_pol);

		dt->v_active = dccd_buf[data_start + 0xb] |
					dccd_buf[data_start + 0xc] << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, v_active is (%d)\n", __func__, dt->v_active);

		v_blk = dccd_buf[data_start + 0xd] | dccd_buf[data_start + 0xe] << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, v_blank is (%d)\n", __func__, v_blk);
		dt->v_period = dt->v_active + v_blk;

		dt->vsync_fp = dccd_buf[data_start + 0xf] | dccd_buf[data_start + 0x10] << 8;
		if (dt->vsync_fp < 18)
			dt->vsync_fp = 18;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, vsync_fp is (%d)\n", __func__, dt->vsync_fp);

		dt->vsync_width = dccd_buf[data_start + 0x11] |
					(dccd_buf[data_start + 0x12] & 0x7f) << 8;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, vsync_width is (%d)\n", __func__, dt->vsync_width);

		dt->vsync_bp = v_blk - dt->vsync_fp - dt->vsync_width;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, vsync_bp is (%d)\n", __func__, dt->vsync_bp);

		dt->vsync_pol = (dccd_buf[data_start + 0x12] & 0x80) >> 7;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, vsync_pol is (%d)\n", __func__, dt->vsync_pol);

		//lcd_basic
		data = dccd_buf[data_start + 0x17] & 0xf;
		switch (data) {
		case 0:
			dt->lcd_bits = 6;
			break;
		case 1:
			dt->lcd_bits = 8;
			break;
		case 3:
			dt->lcd_bits = 12;
			break;
		default:
			dt->lcd_bits = 10;
			break;
		}
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			LCDPR("%s, lcd_bits is (%d)\n", __func__, dt->lcd_bits);
		data_start = 3 + data_start + dccd_buf[data_start + 0x2];
	}

	//0x81: panel timing data block2
	if (dccd_buf[data_start] == 0x81) {
		/* offset 0x1: [7:4]:version, [3:0]:reserved
		 * 0x2: data_length
		 * 0x3: screen_width[7:0] (cm)
		 * 0x4: screen_height[7:0] (cm)
		 * 0x5: [7:4]: screen_width[11:8], [3:0]: screen_height[11:8]
		 * 0x6: lcd_interface(0:vb1, 1:lvds, 2:p2p, 3: mlvds)
		 *   SS: don't support ss positive and negative, defalut use ss_level+ value
		 * 0x7: ss max freq (KHz)
		 * 0x8: ss_level+
		 * 0x9: ss_level-
		 * 0xc: lane_num
		 */
		lcd_if = dccd_buf[data_start + 0x6];
		pdrv->config.basic.screen_width =
			((dccd_buf[data_start + 0x5] >> 4) * 256 +
			dccd_buf[data_start + 0x3]) * 10;
		pdrv->config.basic.screen_height =
			((dccd_buf[data_start + 0x5] & 0xf) * 256 +
			dccd_buf[data_start + 0x4]) * 10;
		pdrv->config.phy_cfg.lane_num = dccd_buf[data_start + 0xc];
		switch (lcd_if) {
		case 2:  //p2p
			pdrv->config.basic.lcd_type = LCD_P2P;
			pdrv->config.control.p2p_cfg.lane_num = lane_num;
			break;
		case 3:  //mlvds
			pdrv->config.basic.lcd_type = LCD_MLVDS;
			pdrv->config.control.mlvds_cfg.channel_num = lane_num;
			break;
		default: //vb1
			pdrv->config.basic.lcd_type = LCD_VBYONE;
			pdrv->config.control.vbyone_cfg.lane_count = lane_num;
			break;
		}
		pdrv->config.phy_cfg.lane_num = lane_num;
		if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
			LCDPR("%s, lcd_type is (%d), width=%dmm, height=%dmm, lane_num is (%d)\n",
				__func__, pdrv->config.basic.lcd_type,
				pdrv->config.basic.screen_width,
				pdrv->config.basic.screen_height,
				pdrv->config.phy_cfg.lane_num);
		}
		data_start = 3 + data_start + dccd_buf[data_start + 0x2];
	}

	return 0;
}

static int tcon_dccd_has_tcon_reg(void)
{
	unsigned char *buf = NULL;
	unsigned int len = 0;

	buf = panel_param_mem_get("tcon_core_reg", &len);

	return (buf && (len > 0));
}

unsigned int lcd_tcon_dccd_get_crc(void)
{
	return dccd_conf.checksum;
}

unsigned int lcd_tcon_is_support_dccd(void)
{
	return dccd_conf.is_dccd;
}

unsigned int lcd_tcon_dccd_has_tcon_file(void)
{
	return dccd_conf.has_tcon_file;
}

#define DCCD_TCON_BASE_PATH_MAX 8
int lcd_tcon_load_dccd(struct aml_lcd_drv_s *pdrv)
{
	int file_type = 0, i = 0, bpath_num = 0, dccd_timing = 0, dccd_size = 0;
	char base_path_k[DCCD_TCON_BASE_PATH_MAX][TCON_BIN_PATH_LEN];
	char item_name[64], *dccd_path_k = NULL;
	unsigned char *dccd_data = NULL;
	const char *str;
#ifdef CONFIG_CMD_AML_MODEL
	void *inip;
	unsigned int temp = 0;
#endif
#ifdef CONFIG_AML_LCD_JSON
	struct json_parse_s *jsp;
	struct json_s *parent, *child;
	const char *dir_uboot, *dir_kernel;
	char u_path[TCON_BIN_PATH_LEN], k_path[TCON_BIN_PATH_LEN];
	int cnt = 0;

	memset(u_path, 0, sizeof(u_path));
	memset(k_path, 0, sizeof(k_path));
#endif

	memset(item_name, 0, sizeof(item_name));
	memset(base_path_k, 0, sizeof(base_path_k));

	file_type = get_lcd_panel_file_type(pdrv->index);
	if (file_type == PANEL_FILE_INI) {
#ifdef CONFIG_CMD_AML_MODEL
		inip = get_lcd_ini_parse_mem(pdrv->index);
		if (!inip) {
			LCDERR("%s: parse_mem not ready\n", __func__);
			return -1;
		}
		str = tcon_ini_get_val(inip, "lcd_Attr", "dccd_flag", "0");
		temp = strtoul(str, NULL, 0);
		if (!temp)
			return 0;

		str = tcon_ini_get_val(inip, "lcd_Attr", "dccd_timing", "0");
		dccd_timing = strtoul(str, NULL, 0);

		str = tcon_ini_get_val(inip, "tcon_Path", "DCCD_BIN_PATH", NULL);
		if (!str) {
			LCDPR("%s: no DCCD_BIN_PATH\n", __func__);
			return 0;
		}

		dccd_data = model_read_file_to_buffer((const char *)str, &dccd_size);
		if (!dccd_data || dccd_size <= 0) {
			LCDERR("%s: read file error: %s\n", __func__, str);
			return -1;
		}

		//get kernel bin path
		str = tcon_ini_get_val(inip, "tcon_Path", "DCCD_BIN_PATH_K", NULL);
		if (!str) {
			LCDERR("%s: no DCCD_BIN_PATH\n", __func__);
			return 0;
		}
		dccd_path_k = (char *)str;

		for (i = 0; i < DCCD_TCON_BASE_PATH_MAX; i++) {
			sprintf(item_name, "TCON_BASE_BIN_%d_PATH_K", i);
			str = tcon_ini_get_val(inip, "tcon_Path", item_name, NULL);
			if (!str) {
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
					LCDERR("%s: get %s fail\n", __func__, item_name);
				continue;
			}
			strcpy(base_path_k[bpath_num++], str);
		}
#endif
	} else if (file_type == PANEL_FILE_JSON) {
#ifdef CONFIG_AML_LCD_JSON
		jsp = get_panel_jsp(pdrv->index);
		if (!json_parse_ok(jsp)) {
			LCDPR("%s: lcd%d json not ready\n", __func__, pdrv->index);
			return -1;
		}

		dir_uboot = tcon_json_get_val(jsp, "tcon", "panel_dir_uboot", NULL);
		if (!dir_uboot) {
			LCDERR("can't find /data/panel_dir_uboot\n");
			return 0;
		}

		dir_kernel = tcon_json_get_val(jsp, "tcon", "panel_dir_kernel", NULL);
		if (!dir_kernel) {
			LCDERR("can't find /data/panel_dir_kernel\n");
			return 0;
		}

		str = tcon_json_get_val(jsp, "tcon", "dccd_bin_path", NULL);
		if (!str) {
			LCDPR("%s: no dccd_bin_path\n", __func__);
			return 0;
		}

		if (path_name_compose(dir_uboot, str, u_path)) {
			LCDERR("%s: dccd_bin_path not right\n", __func__);
			return 0;
		}
		dccd_data = model_read_file_to_buffer((const char *)u_path, &dccd_size);

		//get kernel bin path
		if (path_name_compose(dir_kernel, str, k_path)) {
			LCDERR("%s: dccd_bin_path not right\n", __func__);
			return 0;
		}
		dccd_path_k = k_path;

		str = tcon_json_get_val(jsp, "timing", "dccd_timing", "0");
		dccd_timing = strtoul(str, NULL, 0);

		parent = json_get_object_child(jsp, jsp->root, "tcon");
		if (!parent) {
			LCDPR("%s: can't find /tcon\n", __func__);
			return 0;
		}
		child = json_get_object_child(jsp, parent, "dccd_tcon_base_path");
		cnt = json_get_array_size(jsp, child);
		if (cnt <= 0) {
			LCDERR("%s: dccd_tcon_base_path error\n", __func__);
			return 0;
		}
		for (i = 0; i < cnt && i < DCCD_TCON_BASE_PATH_MAX; i++) {
			str = json_get_arr_str(jsp, child, i, NULL);
			if (!str)
				continue;
			if (path_name_compose(dir_uboot, str, base_path_k[bpath_num])) {
				LCDERR("%s: get tcon_base_path[%d] fail\n", __func__, i);
				continue;
			}
			bpath_num++;
		}
#endif
	}

	if (!dccd_data) {
		LCDPR("No dccd data, not support\n");
		return 0;
	}

	dccd_conf.is_dccd = 0;
	dccd_conf.has_tcon_file = tcon_dccd_has_tcon_reg();

	tcon_dccd_check_and_update(&dccd_conf, dccd_data, dccd_size);

	if (dccd_conf.is_dccd) {
		if (dccd_timing) {
			LCDPR("%s: dccd update timing\n", __func__);
			tcon_dccd_update_timing(pdrv, dccd_data);
		}

		//save dccd kernel bin path to env
		env_set("DCCD_BIN_PATH_K", dccd_path_k);
		sprintf(item_name, "update_env_part -p -f DCCD_BIN_PATH_K");
		run_command(item_name, 0);

		for (i = 0; i < bpath_num; i++) {
			sprintf(item_name, "TCON_BASE_BIN_%d_PATH_K", i);
			env_set(item_name, base_path_k[i]);
			sprintf(item_name, "update_env_part -p -f TCON_BASE_BIN_%d_PATH_K", i);
			run_command(item_name, 0);
		}
	}

	if (dccd_data)
		free(dccd_data);

	return 0;
}

#else

int lcd_tcon_load_dccd(struct aml_lcd_drv_s *pdrv)
{
	return 0;
}

unsigned int lcd_tcon_dccd_get_crc(void)
{
	return 0;
}

unsigned int lcd_tcon_is_support_dccd(void)
{
	return 0;
}

unsigned int lcd_tcon_dccd_has_tcon_file(void)
{
	return 0;
}

#endif

