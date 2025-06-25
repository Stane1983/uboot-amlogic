// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#ifdef CONFIG_SECURE_POWER_CONTROL
#include <asm/amlogic/arch/pwr_ctrl.h>
#endif
#include "../../lcd_reg.h"
#include "../../lcd_common.h"
#include "../lcd_connector.h"
#include "./dsi_common.h"
#include "./dsi_ctrl/dsi_ctrl.h"
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/media/vout/lcd/lcd_extern.h>
#endif

char *dsi_op_mode_table[] = {"Video", "Command"};

char *dsi_video_mode_type_table[] = {
	"Non-Burst (sync pulse)",
	"Non-Burst (sync event)",
	"Burst",
};

char *dsi_video_data_type_table[] = {
	"16bit CFG-1",
	"16bit CFG-2",
	"16bit CFG-3",
	"18bit CFG-1",
	"18bit CFG-2",
	"24bit",
	"20bit YCbCr 4:2:2 (loosely)",
	"24bit YCbCr 4:2:2",
	"16bit YCbCr 4:2:2",
	"30bit",
	"36bit",
	"12bit YCbCr 4:2:0",
	"un-support type",
};

void dsi_config_print_helper(struct lcd_config_s *pconf, u8 pr_flag)
{
	struct dsi_config_s *dconf = &pconf->control.mipi_cfg;
	u32 esc_clk;

	if (pr_flag & DSI_HOST_CFG_PR_VID_TIMING) {
		printf("MIPI DSI Config:\n"
			"  %u-port, %u-lane\n"
			"  bit rate = %lluHz (limit < %uMHz)\n"
			"  lane_byte_clk:    %uhz\n"
			"  operation mode:   init:%s(%u), display:%s(%u)\n"
			"  video mode type:  %s(%d)\n"
			"  clk always hs:    %u\n"
			"  data format:      %s\n",
			dconf->multi_port_cfg ? 2 : 1, dconf->lane_num,
			pconf->timing.bit_rate, dconf->bit_rate_max,
			dconf->lane_byte_clk,
			dsi_op_mode_table[dconf->operation_mode_init],
			dconf->operation_mode_init,
			dsi_op_mode_table[dconf->operation_mode_display],
			dconf->operation_mode_display,
			dsi_video_mode_type_table[dconf->video_mode_type],
			dconf->video_mode_type,
			dconf->clk_always_hs,
			dsi_video_data_type_table[dconf->dpi_data_format]);
	}

	if (pr_flag & DSI_HOST_CFG_PR_VID_TIMING) {
		printf("MIPI DSI video timing:\n"
			"  HLINE       = %hu\n"
			"  HSA         = %hu\n"
			"  HBP         = %hu\n"
			"  VSA         = %hu\n"
			"  VBP         = %hu\n"
			"  VFP         = %hu\n"
			"  VACT        = %hu\n",
			dconf->hline, dconf->hsa, dconf->hbp,
			pconf->timing.act_timing.vsync_width,
			pconf->timing.act_timing.vsync_bp,
			pconf->timing.act_timing.vsync_fp,
			pconf->timing.act_timing.v_active);
	}

	if (pr_flag & DSI_HOST_CFG_PR_CLK) {
		printf("MIPI DSI clk:\n"
		      "  Pixel CLK   = %10uHz\n"
		      "  bit_rate    = %10lluHz\n"
		      "  lanebyteclk = %10uHz\n"
		      "  PCLK_period/lanebyteclk_period = (num=%u/den=%u)\n",
		      pconf->timing.act_timing.pixel_clk,
		      pconf->timing.bit_rate,
		      dconf->lane_byte_clk, dconf->factor_numerator, dconf->factor_denominator);
	}

	if (pr_flag & DSI_HOST_CFG_PR_DPHY_TIM) {
		esc_clk = div_around(1000000, dconf->dphy.lp_tesc[1]);

		printf("MIPI DSI DPHY setting:\n"
			"  t-UI        = %u.%02uns\n"
			"  LP-ESC clk  = %d.%03dMHz\n"
			"  LP-tesc     = 0x%02x (%uns)\n"
			"  LP-lpx      = 0x%02x (%uns)\n"
			"  LP-ta_sure  = 0x%02x (%uns)\n"
			"  LP-ta_go    = 0x%02x (%uns)\n"
			"  LP-ta_get   = 0x%02x (%uns)\n"
			"  HS-exit     = 0x%02x (%uns)\n"
			"  HS-trail    = 0x%02x (%uns)\n"
			"  HS-zero     = 0x%02x (%uns)\n"
			"  HS-prepare  = 0x%02x (%uns)\n"
			"  CLK-trail   = 0x%02x (%uns)\n"
			"  CLK-post    = 0x%02x (%uns)\n"
			"  CLK-zero    = 0x%02x (%uns)\n"
			"  CLK-prepare = 0x%02x (%uns)\n"
			"  CLK-pre     = 0x%02x (%uns)\n"
			"  init        = 0x%02x\n"
			"  wakeup      = 0x%02x\n",
			dconf->dphy.t_ui / 100,     dconf->dphy.t_ui % 100,
			(esc_clk / 1000),           (esc_clk % 1000),
			dconf->dphy.lp_tesc[0],     dconf->dphy.lp_tesc[1],
			dconf->dphy.lp_lpx[0],      dconf->dphy.lp_lpx[1],
			dconf->dphy.lp_ta_sure[0],  dconf->dphy.lp_ta_sure[1],
			dconf->dphy.lp_ta_go[0],    dconf->dphy.lp_ta_go[1],
			dconf->dphy.lp_ta_get[0],   dconf->dphy.lp_ta_get[1],
			dconf->dphy.hs_exit[0],     dconf->dphy.hs_exit[1],
			dconf->dphy.hs_trail[0],    dconf->dphy.hs_trail[1],
			dconf->dphy.hs_zero[0],     dconf->dphy.hs_zero[1],
			dconf->dphy.hs_prepare[0],  dconf->dphy.hs_prepare[1],
			dconf->dphy.clk_trail[0],   dconf->dphy.clk_trail[1],
			dconf->dphy.clk_post[0],    dconf->dphy.clk_post[1],
			dconf->dphy.clk_zero[0],    dconf->dphy.clk_zero[1],
			dconf->dphy.clk_prepare[0], dconf->dphy.clk_prepare[1],
			dconf->dphy.clk_pre[0],     dconf->dphy.clk_pre[1],
			dconf->dphy.init, dconf->dphy.wakeup);

		pr_info("MIPI DSI HOST setting:\n"
			"  phy_hs2lp   = 0x%02x (%uns)\n"
			"  phy_hs2lp   = 0x%02x (%uns)\n"
			"  max_rd_time = 0x%02x (%uns)\n",
			dconf->dphy.phy_hs2lp_time[0], dconf->dphy.phy_hs2lp_time[1],
			dconf->dphy.phy_lp2hs_time[0], dconf->dphy.phy_lp2hs_time[1],
			dconf->dphy.max_rd_time[0], dconf->dphy.max_rd_time[1]);
	}

	if (pr_flag & DSI_HOST_CFG_PR_N_BURST_ST) {
		if (dconf->video_mode_type != DSI_VID_BURST) {
			printf("MIPI DSI NON-BURST setting:\n"
				"  vid_num_chunks: %d\n"
				"  vid_pkt_size:   %d\n"
				"  vid_null_size:  %d\n\n",
				dconf->vid_num_chunks,
				dconf->vid_pkt_size,
				dconf->vid_null_size);
		}
	}
}

u16 dsi_table_load_dts(char *propdata, u8 *table, u16 max_len)
{
	u8 cmd_size, type;
	u16 i = 0, j;

	table[0] = LCD_EXT_CMD_TYPE_END;
	table[1] = 0;

	while ((i + 1) < max_len) {
		table[i] = (u8)(be32_to_cpup((((u32 *)propdata) + i)));
		table[i + 1] = (u8)(be32_to_cpup((((u32 *)propdata) + i + 1)));
		type = table[i];
		cmd_size = table[i + 1];

		if (type == LCD_EXT_CMD_TYPE_END) {
			if (cmd_size == 0xff || cmd_size == 0) {
				i += 2;
				return i;
			}
			cmd_size = 0;
		}
		if ((i + 2 + cmd_size) > max_len) {
			LCDERR("(%d): cmd_size out of limit\n", i);
			table[i] = LCD_EXT_CMD_TYPE_END;
			table[i + 1] = 0;
			i += 2;
			return i;
		}

		for (j = 0; j < cmd_size; j++) {
			table[i + 2 + j] =
				(u8)(be32_to_cpup((((u32 *)propdata) + i + 2 + j)));
		}
		i += (cmd_size + 2);
	}
	return i;
}

static u16 dsi_table_load_bsp(u8 *table, u16 max_len)
{
	u8 cmd_size, type;
	u16 i = 0;

	while ((i + 1) < max_len) {
		type = table[i];
		cmd_size = table[i + 1];
		if (type == LCD_EXT_CMD_TYPE_END) {
			if (cmd_size == 0xff || cmd_size == 0)
				break;
			cmd_size = 0;
		}
		if (cmd_size == 0) {
			i += 2;
			continue;
		}
		if ((i + 2 + cmd_size) > max_len) {
			LCDERR("cmd_size out of support\n");
			table[i] = LCD_EXT_CMD_TYPE_END;
			table[i + 1] = 0;
			break;
		}
		i += (cmd_size + 2);
	}
	return i;
}

void lcd_dsi_init_table_load_dts(char *dtaddr, int nodeoffset, struct dsi_config_s *dconf)
{
	char *propdata;

	free(dconf->dsi_init_on);
	free(dconf->dsi_init_off);
	dconf->dsi_init_on = NULL;
	dconf->dsi_init_off = NULL;

	dconf->dsi_init_on = (u8 *)malloc(sizeof(u8) * DSI_INIT_ON_MAX);
	dconf->dsi_init_off = (u8 *)malloc(sizeof(u8) * DSI_INIT_OFF_MAX);
	if (!dconf->dsi_init_on || !dconf->dsi_init_off) {
		LCDERR("[*]: %s: table malloc failed\n", __func__);
		free(dconf->dsi_init_on);
		free(dconf->dsi_init_off);
		dconf->dsi_init_on = NULL;
		dconf->dsi_init_off = NULL;
		return;
	}

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "dsi_init_on", NULL);
	if (!propdata) {
		LCDERR("[*]: %s: fdt get dsi_init_on failed\n", __func__);
		dconf->dsi_init_on[0] = 0xff;
		dconf->dsi_init_on[1] = 0;
	} else {
		dsi_table_load_dts(propdata, dconf->dsi_init_on, DSI_INIT_ON_MAX);
	}

	propdata = (char *)fdt_getprop(dtaddr, nodeoffset, "dsi_init_off", NULL);
	if (!propdata) {
		LCDERR("[*]: %s: fdt get dsi_init_off failed\n", __func__);
		dconf->dsi_init_off[0] = 0xff;
		dconf->dsi_init_off[1] = 0;
	} else {
		dsi_table_load_dts(propdata, dconf->dsi_init_off, DSI_INIT_OFF_MAX);
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		dsi_init_table_print(dconf);
}

void lcd_dsi_init_table_load_bsp(struct dsi_config_s *dconf)
{
	if (!dconf->dsi_init_on || !dconf->dsi_init_off) {
		LCDERR("%s: init table is not exist\n", __func__);
		return;
	}

	dsi_table_load_bsp(dconf->dsi_init_on,  DSI_INIT_ON_MAX);
	dsi_table_load_bsp(dconf->dsi_init_off, DSI_INIT_OFF_MAX);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		dsi_init_table_print(dconf);
}

static void dsi_req_print(int ret, struct dsi_cmd_req_s *req)
{
	char *_str;
	u32 n, k;

	u8 is_DCS =
		req->data_type == DT_DCS_LONG_WR ||
		req->data_type == DT_DCS_SHORT_WR_0 ||
		req->data_type == DT_DCS_SHORT_WR_1 ||
		req->data_type == DT_DCS_RD_0;
	u8 is_read =
		req->data_type == DT_GEN_RD_0 ||
		req->data_type == DT_GEN_RD_1 ||
		req->data_type == DT_GEN_RD_2 ||
		req->data_type == DT_DCS_RD_0;
	u8 is_long =
		req->data_type == DT_DCS_LONG_WR ||
		req->data_type == DT_GEN_LONG_WR;
	//DCS_LONG_WR:0x29, GEN_LONG_WR:0x39
	u8 num = (req->data_type & 0xf) == 0x9 ? 3 : (req->data_type >> 4) & 0xf;

	_str = malloc(PR_BUF_MAX * sizeof(char));
	if (!_str) {
		LCDERR("%s: buf malloc error\n", __func__);
		return;
	}
	memset(_str, 0, PR_BUF_MAX);

	n = snprintf(_str, PR_BUF_MAX, "DSI %s %s %s%u%s: ",
		is_DCS  ? "DCS"          : "generic",
		is_read ? "RD"           : "WR",
		is_long ? "long("        : "",
		is_long ? req->pld_count : num,
		is_long ? ")"            : "");

	for (k = 0; k < req->pld_count; k++)
		n += snprintf(_str + n, PR_BUF_MAX - n, "0x%02x ", req->payload[k]);

	if (ret) {
		snprintf(_str + n, PR_BUF_MAX - n, " failed");
	} else {
		if (req->rd_out_len)
			n += snprintf(_str + n, PR_BUF_MAX - n, ": ");

		for (k = 0; k < req->rd_out_len; k++)
			n += snprintf(_str + n, PR_BUF_MAX - n, "0x%02x ", req->rd_data[k]);
	}
	printf("%s\n", _str);
	free(_str);
}

/* helper func to check req->pld_count satisfied req->data_type and return req->payload[pld_idx] */
static u32 cmd_pld_cnt_check(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	u8 pld_req = 0;

	switch (req->data_type) {
	case DT_GEN_SHORT_WR_2:
	case DT_GEN_RD_2:
	case DT_SET_MAX_RET_PKT_SIZE:
	case DT_DCS_SHORT_WR_1:
		pld_req = 2;
		break;
	case DT_GEN_SHORT_WR_1:
	case DT_GEN_RD_1:
	case DT_DCS_SHORT_WR_0:
	case DT_DCS_RD_0:
		pld_req = 1;
		break;
	case DT_GEN_RD_0:
	case DT_GEN_SHORT_WR_0:
		pld_req = 0;
		break;
	case DT_DCS_LONG_WR:
	case DT_GEN_LONG_WR:
	default:
		pld_req = req->pld_count;
		break;
	}

	if (req->pld_count < pld_req) {
		LCDERR("[%d]: payload count %d insufficient for 0x%02x (req:%d)\n",
			pdrv->index, req->pld_count, req->data_type, pld_req);
		return -1;
	}
	return 0;
}

/* Function: dsi_run_oneline_cmd
 * Supported Data Type:
 *	DT_GEN_SHORT_WR_0, DT_GEN_SHORT_WR_1, DT_GEN_SHORT_WR_2,
 *	DT_DCS_SHORT_WR_0, DT_DCS_SHORT_WR_1,
 *	DT_GEN_LONG_WR, DT_DCS_LONG_WR,
 *	DT_SET_MAX_RET_PKT_SIZE
 *	DT_GEN_RD_0, DT_GEN_RD_1, DT_GEN_RD_2,
 *	DT_DCS_RD_0
 * @payload: one line of dsi_init_on/off
 * @rd_back_len: max this rd_back can contain
 * Return: -1:error; 0: Wr done; 0/>0: number of rdback
 */
int dsi_run_oneline_cmd(struct aml_lcd_drv_s *pdrv, uint8_t port,
			uint8_t *payload, uint8_t *rd_back, uint32_t rd_back_len)
{
	struct dsi_cmd_req_s dsi_cmd_req;
	u8 is_rd = 0;
	u16 dsi_rd_len = 0;
	int ret = 0;

	/* @payload:
	 * [0]: data_type
	 * [1]: pld_count
	 * [2++]: pld context
	 */
	dsi_cmd_req.data_type = payload[0];
	dsi_cmd_req.pld_count = payload[1];
	dsi_cmd_req.payload = &payload[2];
	dsi_cmd_req.vc_id = MIPI_DSI_VIRTUAL_CHAN_ID;
	dsi_cmd_req.rd_out_len = 0;

	ret = cmd_pld_cnt_check(pdrv, &dsi_cmd_req);
	if (ret)
		return -1;

	switch (dsi_cmd_req.data_type) {
	case DT_GEN_SHORT_WR_0:
	case DT_GEN_SHORT_WR_1:
	case DT_GEN_SHORT_WR_2:
		dsi_cmd_req.req_ack = ACK_CTRL_GEN_SHORT_WR;
		ret = dsi_DT_generic_short_write(pdrv, port, &dsi_cmd_req);
		break;
	case DT_DCS_SHORT_WR_0:
	case DT_DCS_SHORT_WR_1:
		dsi_cmd_req.req_ack = ACK_CTRL_DCS_SHORT_WR;
		ret = dsi_DT_DCS_short_write(pdrv, port, &dsi_cmd_req);
		break;
	case DT_DCS_LONG_WR:
		dsi_cmd_req.req_ack = ACK_CTRL_GEN_LONG_WR;
		ret = dsi_DT_DCS_long_write(pdrv, port, &dsi_cmd_req);
		break;
	case DT_GEN_LONG_WR:
		dsi_cmd_req.req_ack = ACK_CTRL_DCS_LONG_WR;
		ret = dsi_DT_generic_long_write(pdrv, port, &dsi_cmd_req);
		break;
	case DT_TURN_ON:
		dsi_DT_sink_turn_on(pdrv, port);
		break;
	case DT_SHUT_DOWN:
		dsi_DT_sink_shut_down(pdrv, port);
		break;
	case DT_SET_MAX_RET_PKT_SIZE:
		dsi_cmd_req.req_ack = ACK_CTRL_SET_MAX_RET_PKT_SIZE;
		dsi_rd_len  = dsi_cmd_req.payload[0];
		dsi_rd_len |= (u16)dsi_cmd_req.payload[1] << 8;
		pdrv->config.control.mipi_cfg.dsi_rd_n = dsi_rd_len;
		// DSI_RD_MAX default size is 4, if any read will return size over 4,
		// enlarge DSI_RD_MAX, each req will malloc such mem.
		if (dsi_rd_len > DSI_RD_MAX)
			LCDPR("[%d]: set max rx %d bytes, need to enlarge DSI_RD_MAX (%d)\n",
				pdrv->index, dsi_rd_len, DSI_RD_MAX);
		ret = dsi_DT_set_max_return_pkt_size(pdrv, port, &dsi_cmd_req);
		return ret;
	case DT_GEN_RD_0:
	case DT_GEN_RD_1:
	case DT_GEN_RD_2:
		is_rd = 1;
		dsi_cmd_req.req_ack = ACK_CTRL_GEN_RD; //need BTA ack
		ret = dsi_DT_generic_read(pdrv, port, &dsi_cmd_req);
		break;
	case DT_DCS_RD_0:
		is_rd = 1;
		dsi_cmd_req.req_ack = ACK_CTRL_DCS_RD; //need BTA ack
		ret = dsi_DT_DCS_read(pdrv, port, &dsi_cmd_req);
		break;
	default:
		LCDERR("[%d]: unsupport DSI cmd: 0x%02x\n", pdrv->index, dsi_cmd_req.data_type);
		return -1;
	}

	if (ret || lcd_debug_print_flag & LCD_DBG_PR_ADV)
		dsi_req_print(ret, &dsi_cmd_req);

	if (!ret && is_rd && rd_back_len && rd_back) {
		if (dsi_cmd_req.rd_out_len > rd_back_len) {
			LCDERR("[%d]: %s rd out %d, back limit %d\n", pdrv->index, __func__,
				dsi_cmd_req.rd_out_len, rd_back_len);
			return 0;
		}
		memcpy(rd_back, dsi_cmd_req.rd_data, sizeof(char) * dsi_cmd_req.rd_out_len);
		return dsi_cmd_req.rd_out_len;
	}
	return ret;
}

int dsi_exec_init_table(struct aml_lcd_drv_s *pdrv,
			u8 *payload, u32 pld_limit, u8 *rd_back, u32 rd_back_max)
{
	u16 i = 0, j = 0, step = 0;
	u8 cmd_size, k;
	char *str;
	u32 gpio, delay_ms;
	u8 rd_cnt = 0, ret, curr_port = 0x01;

	/* mipi command(payload)
	 * format:  data_type, cmd_size, data....
	 *	data_type=0xff,
	 *		cmd_size<0xff means delay ms,
	 *		cmd_size=0xff or 0 means ending.
	 *	data_type=0xf0, for gpio control
	 *		data0=gpio_index, data1=gpio_value.
	 *		data0=gpio_index, data1=gpio_value, data2=delay.
	 *	data_type=0xfd, for delay ms
	 *		data0=delay, data_1=delay, ..., data_n=delay.
	 */
	while ((i + 1) < pld_limit) {
		cmd_size = payload[i + 1];
		if (payload[i] == LCD_EXT_CMD_TYPE_END) {
			if (cmd_size == 0xff || cmd_size == 0)
				return rd_cnt;
			cmd_size = 0;
			mdelay(payload[i + 1]);
		}

		if (cmd_size == 0) {
			i += (cmd_size + 2);
			continue;
		}
		if (i + 2 + cmd_size > pld_limit) {
			LCDERR("[%d]: step %d: cmd_size out of support\n", pdrv->index, step);
			break;
		}

		switch (payload[i]) {
		case LCD_EXT_CMD_TYPE_DELAY:
			delay_ms = 0;
			for (j = 0; j < cmd_size; j++)
				delay_ms += payload[i + 2 + j];
			mdelay(delay_ms);
			break;
		case LCD_EXT_CMD_TYPE_GPIO:
			if (cmd_size < 2) {
				LCDERR("[%d]: step %d: invalid size %d for gpio\n",
					pdrv->index, step, cmd_size);
				break;
			}
			str = pdrv->config.power.cpu_gpio[payload[i + 2]];
			gpio = lcd_gpio_name_map_num(str);
			lcd_gpio_set(gpio, payload[i + 3]);
			if (cmd_size > 2)
				mdelay(payload[i + 4]);
			break;
		case LCD_EXT_CMD_TYPE_CHECK:
			if (cmd_size < 3) {
				LCDERR("[%d]: step %d: invalid size %d for check\n",
					pdrv->index, step, cmd_size);
				break;
			}
			mipi_dsi_check_state(pdrv, payload[i + 2], payload[i + 3]);
			break;
		case LCD_EXT_CMD_TYPE_SWITCH_PORT:
			if (cmd_size != 1) {
				LCDERR("[%d]: step %d: invalid size %d for switch port\n",
					pdrv->index, step, cmd_size);
				break;
			}
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("[%d]: step %d: following cmd will be on port-%c%c\n",
					pdrv->index, step,
					payload[i + 2] & 0x1 ? 'A' : ' ',
					payload[i + 2] & 0x2 ? 'B' : ' ');
			curr_port = payload[i + 2];
			break;
		case DT_GEN_SHORT_WR_0:
		case DT_GEN_SHORT_WR_1:
		case DT_GEN_SHORT_WR_2:
		case DT_DCS_SHORT_WR_0:
		case DT_DCS_SHORT_WR_1:
		case DT_DCS_LONG_WR:
		case DT_GEN_LONG_WR:
		case DT_TURN_ON:
		case DT_SHUT_DOWN:
		case DT_SET_MAX_RET_PKT_SIZE:
			for (k = 0; k < 4; k++) {
				if (curr_port & (1 << k))
					dsi_run_oneline_cmd(pdrv, k, &payload[i], NULL, 0);
			}			break;
		case DT_GEN_RD_0:
		case DT_GEN_RD_1:
		case DT_GEN_RD_2:
		case DT_DCS_RD_0:
			// ! read support on port A
			if (!rd_back) {
				dsi_run_oneline_cmd(pdrv, 0, &payload[i], NULL, 0);
			} else {
				ret = dsi_run_oneline_cmd(pdrv, 0, &payload[i],
					&rd_back[rd_cnt], rd_back_max - rd_cnt);
				if (ret > 0) //bypass wr or error
					rd_cnt += ret;
			}
			break;
		default:
			break;
		}
		i += (cmd_size + 2);
		step++;
	}
	return rd_cnt;
}

/* Function: dsi_read
 * payload struct: data_type, data_cnt, parameters...
 * Supported Data Type:
 * DT_DCS_RD_0, DT_GEN_RD_0, DT_GEN_RD_1, DT_GEN_RD_2,
 * Return: data count, -1 for error
 */
int dsi_read(struct aml_lcd_drv_s *pdrv, u8 *payload, u8 *rd_data, u8 len)
{
	int ret = -1;
	u8 rd_out;
	struct dsi_cmd_req_s dsi_cmd_req;

	dsi_cmd_req.data_type = payload[0];
	dsi_cmd_req.pld_count = payload[1];
	dsi_cmd_req.payload = &payload[2];
	dsi_cmd_req.rd_out_len = 0;
	dsi_cmd_req.vc_id = MIPI_DSI_VIRTUAL_CHAN_ID;

	switch (dsi_cmd_req.data_type) {
	case DT_GEN_RD_0:
	case DT_GEN_RD_1:
	case DT_GEN_RD_2:
		dsi_cmd_req.req_ack = ACK_CTRL_GEN_RD; /* need BTA ack */
		ret = dsi_DT_generic_read(pdrv, 0, &dsi_cmd_req);
		break;
	case DT_DCS_RD_0:
		dsi_cmd_req.req_ack = ACK_CTRL_DCS_RD; /* need BTA ack */
		ret = dsi_DT_DCS_read(pdrv, 0, &dsi_cmd_req);
		break;
	default:
		LCDPR("[%d]: unsupported data_type: 0x%02x\n", pdrv->index, dsi_cmd_req.data_type);
		break;
	}

	if (ret < 0)
		return -1;
	rd_out = dsi_cmd_req.rd_out_len > len ? len : dsi_cmd_req.rd_out_len;
	memcpy(rd_data, dsi_cmd_req.rd_data, rd_out);
	return rd_out;
}

static void dsi_panel_init(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
	struct lcd_extern_dev_s *edev;

	if (dconf->extern_init == LCD_EXTERN_INDEX_INVALID) {
		LCDPR("[%d]: %s extern [%d] invalid\n", pdrv->index, __func__, dconf->extern_init);
		goto dsi_panel_init_main;
	}
	edrv = lcd_extern_get_driver(pdrv->index);
	edev = lcd_extern_get_dev(edrv, dconf->extern_init);
	if (!edrv || !edev) {
		LCDPR("[%d]: no lcd_extern dev\n", pdrv->index);
		goto dsi_panel_init_main;
	}
	// remove support on dsi cmd on extern driver
	if (edev->config.table_init_on && edev->power_on) {
		edev->power_on(edrv, edev);
		LCDPR("[%d]: [extern]%s dsi init on\n", pdrv->index, edev->config.name);
	}
dsi_panel_init_main:
#endif
	if (dconf->dsi_init_on) {
		dsi_exec_init_table(pdrv, dconf->dsi_init_on, DSI_INIT_ON_MAX, NULL, 0);
		LCDPR("[%d]: %s table\n", pdrv->index, __func__);
	}
}

static void dsi_panel_deinit(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

	if (dconf->dsi_init_off) {
		dsi_exec_init_table(pdrv, dconf->dsi_init_off, DSI_INIT_OFF_MAX, NULL, 0);
		LCDPR("[%d]: %s table\n", pdrv->index, __func__);
	}

#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
	struct lcd_extern_dev_s *edev;

	if (dconf->extern_init == LCD_EXTERN_INDEX_INVALID) {
		LCDPR("[%d]: %s extern [%d] invalid\n", pdrv->index, __func__, dconf->extern_init);
		return;
	}
	edrv = lcd_extern_get_driver(pdrv->index);
	edev = lcd_extern_get_dev(edrv, dconf->extern_init);
	if (!edrv || !edev) {
		LCDPR("[%d]: no lcd_extern dev\n", pdrv->index);
		return;
	}
	if (edev->config.table_init_off && edev->power_off) {
		edev->power_off(edrv, edev);
		LCDPR("[%d]: [extern]%s dsi init off\n", pdrv->index, edev->config.name);
	}
#endif
}

void lcd_dsi_tx_ctrl(struct aml_lcd_drv_s *pdrv, u8 en)
{
	if (en) {
		lcd_dsi_if_bind(pdrv);
		dsi_tx_ready(pdrv);
		if (pdrv->config.control.mipi_cfg.panel_det_attr & 0x01) {
			dsi_panel_detect(pdrv);
			dsi_tx_close(pdrv);
			return;
		}
		dsi_panel_init(pdrv);
		dsi_disp_on(pdrv);
	} else {
		dsi_disp_off(pdrv);
		dsi_panel_deinit(pdrv);
		dsi_tx_close(pdrv);
	}
}

u64 lcd_dsi_get_min_bitrate(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	u64 bit_rate_min, band_width;
	u8 port_cnt = dconf->multi_port_cfg & BIT(0) ? 2 : 1;

	/* unit in kHz for calculation */
	band_width = pdrv->config.timing.dft_timing->pixel_clk;
	if (dconf->operation_mode_display == OPERATION_VIDEO_MODE) {
		if (dconf->video_mode_type != BURST_MODE)
			//band_width = band_width * 4 * dconf->data_bits;
			band_width = band_width * pdrv->config.timing.dft_timing->lcd_bits;
		else
			band_width = band_width * pdrv->config.timing.dft_timing->lcd_bits;
	} else {
		LCDERR("[%d]: %s: Only VIDEO mode need HS bitrate\n", pdrv->index, __func__);
		return 0;
	}

	bit_rate_min = lcd_do_div(band_width, port_cnt * dconf->lane_num);

	return bit_rate_min;
}
