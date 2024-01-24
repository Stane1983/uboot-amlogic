// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include "../../../lcd_common.h"
#include "../dsi_common.h"
#include "../dsi_ctrl/dsi_ctrl.h"
#include <amlogic/media/vout/lcd/aml_lcd.h>

#define DSI_PANEL_DET_CNT_MAX 8
#define DSI_DET_TABLE_MAX 100
#define DSI_DET_SEQ_MAX 20

static void dsi_panel_detect_envset(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	char type_str[35];

	if (pdrv->index)
		sprintf(type_str, "panel%d_type", pdrv->index);
	else
		sprintf(type_str, "panel_type");

	env_set(type_str, dconf->matched_panel);

	if (dconf->panel_det_attr & 0x02) {
		if (pdrv->index)
			sprintf(type_str, "update_env_part -p -f panel_type");
		else
			sprintf(type_str, "update_env_part -p -f panel%d_type", pdrv->index);
		run_command(type_str, 0);
	}

	LCDPR("[%d]: %s: %s %s, stored:%d\n", pdrv->index, __func__,
		(dconf->panel_det_attr & 0x08) ? "matched" : "fallback",
		dconf->matched_panel, dconf->panel_det_attr & 0x02 && 1);
}

static void dsi_panel_try_match_dts(struct aml_lcd_drv_s *pdrv)
{
	int offset, rd_len = 0;
	u8 t, k, match_len = 0;
	u8 *_init_table = NULL, *_match_seq = NULL, *_rd_seq = NULL;
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

	char table_name[18];
	char *table_propdata;
	char seq_name[18];
	char *seq_propdata;
	char type_name[10];
	char *type_propdata;

	offset = fdt_path_offset(dconf->dt_addr, dconf->dsi_detect_dtb_path);
	if (offset < 0)
		return;

	_init_table = (u8 *)malloc(sizeof(u8) * DSI_DET_TABLE_MAX);
	_rd_seq     = (u8 *)malloc(sizeof(u8) * DSI_DET_SEQ_MAX);
	_match_seq  = (u8 *)malloc(sizeof(u8) * DSI_DET_SEQ_MAX);
	if (!_init_table || !_rd_seq || !_init_table) {
		free(_init_table);
		free(_rd_seq);
		free(_match_seq);
		return;
	}

	for (t = 0; t < DSI_PANEL_DET_CNT_MAX; t++) {
		snprintf(table_name, 18, "det%u_init_table", t);
		snprintf(seq_name,   15, "det%u_match_seq", t);
		snprintf(type_name,  10, "det%u_type", t);

		table_propdata = (char *)fdt_getprop(dconf->dt_addr, offset, table_name, NULL);
		seq_propdata   = (char *)fdt_getprop(dconf->dt_addr, offset, seq_name, NULL);
		type_propdata  = (char *)fdt_getprop(dconf->dt_addr, offset, type_name, NULL);

		// no match_seq & type at same time, consider as end
		if (!seq_propdata && !type_propdata) {
			if (!t)
				LCDERR("%d: %s no det sequence or type\n", pdrv->index, __func__);
			break;
		}
		// match_seq & type & table0 always needed, allow table1+ non-exist
		if (!seq_propdata || !type_propdata || !(table_propdata || t)) {
			LCDERR("[%d]: %s lack %s %s %s\n", pdrv->index, __func__,
				seq_propdata ? "" : seq_name,
				type_propdata ? "" : type_name,
				(table_propdata || t) ? "" : table_propdata);
			break;
		}

		// copy out target panel and seq
		memset(dconf->matched_panel, 0, sizeof(unsigned char) * MOD_LEN_MAX);
		strncpy(dconf->matched_panel, type_propdata, MOD_LEN_MAX - 1);
		memset(_match_seq, 0, sizeof(u8) * DSI_DET_SEQ_MAX);
		match_len = (u8)(be32_to_cpup((u32 *)seq_propdata));
		match_len = (match_len > DSI_DET_SEQ_MAX) ? DSI_DET_SEQ_MAX : match_len;
		for (k = 0; k <= match_len; k++)
			_match_seq[k] = (u8)(be32_to_cpup((u32 *)seq_propdata + k + 1));

		if (table_propdata) { // if no table, keep last read result, only update _rd_seq
			memset(_init_table, 0, sizeof(u8) * DSI_DET_TABLE_MAX);
			memset(_rd_seq,     0, sizeof(u8) * DSI_DET_SEQ_MAX);

			dsi_table_load_dts(table_propdata, _init_table, DSI_DET_TABLE_MAX);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s match[%u]_table:\n", pdrv->index, __func__, t);
				dsi_table_print(_init_table, DSI_DET_TABLE_MAX);
			}

			rd_len = dsi_exec_init_table(pdrv,
				_init_table, DSI_DET_TABLE_MAX, _rd_seq, DSI_DET_SEQ_MAX);
			pr_info("read seq[%d]: (%d): ", t, rd_len);
			for (k = 0; k < rd_len; k++)
				pr_info("0x%02x ", _rd_seq[k]);
			pr_info("\n");
		}

		if (rd_len != match_len) { // compare req and read out length
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				pr_info("  match[%u] seq[%u], read[%u]\n", t, match_len, rd_len);
			continue;
		}

		for (k = 0; k < match_len; k++) { // compare each character
			if (_match_seq[k] != _rd_seq[k]) {
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					pr_info("  match[%d] seq=0x%02x rd=0x%02x\n",
						t, _match_seq[k], _rd_seq[k]);
				}
				break;
			}
		}

		if (k == match_len) { // finish compare, all chars are same
			LCDPR("[%d]: det [%u] -> %s\n", pdrv->index, t, dconf->matched_panel);
			dconf->panel_det_attr |= 0x8;
			break;
		}
	}

	if (!(dconf->panel_det_attr & 0x8)) {
		type_propdata = (char *)fdt_getprop(dconf->dt_addr, offset, "fallback_type", NULL);
		if (type_propdata) {
			strncpy(dconf->matched_panel, type_propdata, 19);
		} else {
			LCDERR("[%d]: failed to get fallback_type\n", pdrv->index);
			strncpy(dconf->matched_panel, "NULL", 5);
		}
	}

	free(_init_table);
	free(_match_seq);
	free(_rd_seq);
}

static void dsi_panel_try_match_bsp(struct aml_lcd_drv_s *pdrv)
{
	int rd_len = 0;
	u8 t, k, match_len = 0;
	u8 *_rd_seq = NULL;
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	struct dsi_panel_det_attr_s *det_attr = (struct dsi_panel_det_attr_s *)dconf->dt_addr;

	_rd_seq = (u8 *)malloc(sizeof(u8) * DSI_DET_SEQ_MAX);
	if (!_rd_seq) {
		free(_rd_seq);
		return;
	}

	for (t = 0; t < DSI_PANEL_DET_CNT_MAX; t++) {
		//no match_seq & type at same time, consider as end
		if (!det_attr->det_match_seq[t] && !det_attr->det_type[t]) {
			if (!t) // print error when t=0 (no det executed)
				LCDERR("%d: %s no det sequence or type\n", pdrv->index, __func__);
			break;
		}
		//match_seq & type & table0 always needed, if table1+ non-exist, use previous match
		if (!det_attr->det_match_seq[t] || !det_attr->det_type[t] ||
		    !(det_attr->det_init_table[t] || t)) {
			LCDERR("[%d]: %s %hhu lack %s %s %s\n", pdrv->index, __func__, t,
			!det_attr->det_match_seq[t] ? "" : "seq",
			!det_attr->det_type[t] ? "" : "type",
			!(det_attr->det_init_table[t] || t) ? "" : "table");
			break;
		}

		match_len = det_attr->det_match_seq[t][0];
		match_len = (match_len > DSI_DET_SEQ_MAX) ? DSI_DET_SEQ_MAX : match_len;

		if (det_attr->det_init_table[t]) { //if no table, keep last read result
			memset(_rd_seq, 0, sizeof(u8) * DSI_DET_SEQ_MAX);

			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s match[%u]_table:\n", pdrv->index, __func__, t);
				dsi_table_print(det_attr->det_init_table[t], DSI_DET_TABLE_MAX);
			}

			rd_len = dsi_exec_init_table(pdrv, det_attr->det_init_table[t],
					DSI_DET_TABLE_MAX, _rd_seq, DSI_DET_SEQ_MAX);
			pr_info("read seq[%d]: (%d): ", t, rd_len);
			for (k = 0; k < rd_len; k++)
				pr_info("0x%02x ", _rd_seq[k]);
			pr_info("\n");
		}

		if (rd_len != match_len) { // compare req and read out length
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				pr_info("  match[%u] seq[%u], read[%u]\n", t, match_len, rd_len);
			continue;
		}

		for (k = 0; k < match_len; k++) { // compare each character
			if (det_attr->det_match_seq[t][k + 1] != _rd_seq[k]) {
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					pr_info("  match[%d] seq=0x%02x rd=0x%02x\n",
						t, det_attr->det_match_seq[t][k + 1], _rd_seq[k]);
				}
				continue;
			}
		}

		if (k == match_len) { // finish compare, all chars are same
			LCDPR("[%d]: det [%u] -> %s\n", pdrv->index, t, dconf->matched_panel);
			dconf->panel_det_attr |= 0x8;
			break;
		}
	}

	if (dconf->panel_det_attr & 0x8 && t < DSI_PANEL_DET_CNT_MAX) {
		strncpy(dconf->matched_panel, det_attr->det_type[t], 19);
	} else {
		if (det_attr->fallback_type) {
			strncpy(dconf->matched_panel, det_attr->fallback_type, 19);
		} else {
			LCDERR("[%d]: failed to get fallback_type\n", pdrv->index);
			strncpy(dconf->matched_panel, "NULL", 5);
		}
	}

	free(_rd_seq);
}

static void dsi_panel_detect_drv_config_reload(struct aml_lcd_drv_s *pdrv)
{
	char fake_mode[7];
	u8 load_id_temp;

	sprintf(fake_mode, "panel");
	if (pdrv->index)
		sprintf(fake_mode + 5, "%hhu", pdrv->index);

	load_id_temp = pdrv->config.control.mipi_cfg.panel_det_attr & 0x4 ?  0x1 : 0;
	LCDPR("[%d]: rerun panel=%s\n", pdrv->index, pdrv->config.control.mipi_cfg.matched_panel);
	pdrv->config.control.mipi_cfg.panel_det_attr = 0;

	if (lcd_get_panel_config(lcd_get_dt_addr(), load_id_temp, pdrv)) {
		LCDERR("[%d]: %s: load panel failed\n", pdrv->index, __func__);
		return;
	}
	lcd_panel_config_load_to_drv(pdrv);

#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_probe_single(pdrv->index, load_id_temp);
#endif
	lcd_encl_on(pdrv);
	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0)
		LCDERR("[%d]: %s: encl_on failed!\n", pdrv->index, __func__);
}

void dsi_panel_detect(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.control.mipi_cfg.panel_det_attr & 0x04)
		dsi_panel_try_match_dts(pdrv);
	else
		dsi_panel_try_match_bsp(pdrv);

	dsi_panel_detect_envset(pdrv);

	dsi_panel_detect_drv_config_reload(pdrv);
}
