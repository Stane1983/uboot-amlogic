// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"

static void lcd_lane_map_chip_init(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg)
{
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TXHD2:
	case LCD_CHIP_T6D:
		phy_cfg->lane_offset = 0;
		phy_cfg->lane_mask = 0x3ff;
		break;
	case LCD_CHIP_T3X:
		if (pdrv->index == 0) {
			phy_cfg->lane_offset = 0;
			phy_cfg->lane_mask = 0xffff;
		} else if (pdrv->index == 1) {
			phy_cfg->lane_offset = 8;
			phy_cfg->lane_mask = 0xff;
		}
		break;
	default: //common 12lane
		phy_cfg->lane_offset = 0;
		phy_cfg->lane_mask = 0xfff;
		break;
	}
}

static int lcd_lane_vbyone_ch_data_map(struct aml_lcd_drv_s *pdrv, unsigned char *data_map)
{
	unsigned char ch_slice[][8] = {
		{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7},
		{0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf}
	};
	unsigned int lane_cnt, slice_cnt, lane_pre_slice;
	int ch_idx, slice_idx, i;

	//generate ch slice data map table
	lane_cnt = pdrv->config.control.vbyone_cfg.lane_count;
	slice_cnt = pdrv->config.control.vbyone_cfg.slice;
	lane_pre_slice = (lane_cnt + slice_cnt - 1) / slice_cnt;
	if (lane_cnt >= CH_LANE_MAX) {
		LCDERR("[%d]: %s: vbyone lane_cnt %d err\n",
		       pdrv->index, __func__, lane_cnt);
		return -1;
	}

	for (i = 0; i < lane_cnt; i++) {
		slice_idx = i / lane_pre_slice;
		ch_idx = i % lane_pre_slice;
		if (slice_idx >= 2 || ch_idx >= 8) {
			LCDERR("[%d]: %s: slice_idx %d or ch_idx %d err\n",
			       pdrv->index, __func__, slice_idx, ch_idx);
			return -1;
		}
		data_map[i] = ch_slice[slice_idx][ch_idx];
	}
	for (; i < CH_LANE_MAX; i++)
		data_map[i] = 0xff;

	return 0;
}

//ch_swap for reg
static void lcd_lane_sel_to_ch_swap(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg)
{
	unsigned char data_map[CH_LANE_MAX];
	unsigned int data_sel, valid_flag = 0, i, bit;
	int ret;

	phy_cfg->ch_swap0 = 0xffffffff;
	phy_cfg->ch_swap1 = 0xffffffff;
	switch (pdrv->config.basic.lcd_type) {
	case LCD_VBYONE:
		ret = lcd_lane_vbyone_ch_data_map(pdrv, data_map);
		if (ret)
			return;
		for (i = 0; i < phy_cfg->lane_num; i++) {
			if (phy_cfg->ch_ctrl[i].sel >= CH_LANE_MAX)
				data_sel = 0xff;
			else
				data_sel = data_map[phy_cfg->ch_ctrl[i].sel];
			if (data_sel == 0xff)
				data_sel = 0xf;
			else
				valid_flag |= (1 << i);
			if (i < 8) {
				bit = i * 4;
				phy_cfg->ch_swap0 &= ~(0xf << bit);
				phy_cfg->ch_swap0 |= data_sel << bit;
			} else if (i < 16) {
				bit = (i - 8) * 4;
				phy_cfg->ch_swap1 &= ~(0xf << bit);
				phy_cfg->ch_swap1 |= data_sel << bit;
			}
		}
		break;
	default:
		for (i = 0; i < phy_cfg->lane_num; i++) {
			data_sel = phy_cfg->ch_ctrl[i].sel;
			if (data_sel == 0xff)
				data_sel = 0xf;
			else
				valid_flag |= (1 << i);
			if (i < 8) {
				bit = i * 4;
				phy_cfg->ch_swap0 &= ~(0xf << bit);
				phy_cfg->ch_swap0 |= data_sel << bit;
			} else if (i < 16) {
				bit = (i - 8) * 4;
				phy_cfg->ch_swap1 &= ~(0xf << bit);
				phy_cfg->ch_swap1 |= data_sel << bit;
			}
		}
		break;
	}

	phy_cfg->lane_valid = (valid_flag & phy_cfg->lane_mask) << phy_cfg->lane_offset;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: lane_num:%d, ch_swap:0x%08x,0x%08x, lane_valid:0x%x\n",
		      pdrv->index, __func__, phy_cfg->lane_num,
		      phy_cfg->ch_swap0, phy_cfg->ch_swap1, phy_cfg->lane_valid);
	}
}

static void lcd_ch_swap_to_lane_sel(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg)
{
	unsigned char data_map[CH_LANE_MAX];
	unsigned int sel = 0xff, valid_flag = 0;
	unsigned int i, j, bit;
	int ret;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_VBYONE:
		ret = lcd_lane_vbyone_ch_data_map(pdrv, data_map);
		if (ret)
			return;
		for (i = 0; i < phy_cfg->lane_num; i++) {
			if (i < 8) {
				bit = i * 4;
				sel = (phy_cfg->ch_swap0 >> bit) & 0xf;
			} else if (i < 16) {
				bit = (i - 8) * 4;
				sel = (phy_cfg->ch_swap1 >> bit) & 0xf;
			}

			phy_cfg->ch_ctrl[i].sel = 0xff;
			for (j = 0; j < CH_LANE_MAX; j++) {
				if (sel == data_map[j]) {
					phy_cfg->ch_ctrl[i].sel = j;
					valid_flag |= (1 << i);
					break;
				}
			}
		}
		break;
	default:
		for (i = 0; i < phy_cfg->lane_num; i++) {
			if (i < 8) {
				bit = i * 4;
				sel = (phy_cfg->ch_swap0 >> bit) & 0xf;
			} else if (i < 16) {
				bit = (i - 8) * 4;
				sel = (phy_cfg->ch_swap1 >> bit) & 0xf;
			}

			if (sel == 0xf) {
				phy_cfg->ch_ctrl[i].sel = 0xff;
			} else {
				phy_cfg->ch_ctrl[i].sel = sel;
				valid_flag |= (1 << i);
			}
		}
		break;
	}

	phy_cfg->lane_valid = (valid_flag & phy_cfg->lane_mask) << phy_cfg->lane_offset;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: lane_num:%d, ch_swap:0x%08x,0x%08x, lane_valid:0x%x\n",
		      pdrv->index, __func__, phy_cfg->lane_num,
		      phy_cfg->ch_swap0, phy_cfg->ch_swap1, phy_cfg->lane_valid);
	}
}

static void lcd_lvds_lane_swap(struct aml_lcd_drv_s *pdrv, unsigned int *map0, unsigned int *map1)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	unsigned char ch_reg_idx = 0, lcd_bits, temp, i, n;
	unsigned char valid_port0_s = 0xf, valid_port0_e = 0xf;
	unsigned char valid_port1_s = 0xf, valid_port1_e = 0xf;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	// 12-12 channel:
	// d0_a:0 d1_a:1 d2_a:2 clk_a:3 d3_a:4 d4_a:5 d0_b:6 d1_b:7 d2_b:8 clk_b:9 d3_b:a d4_b:b
	// DIF_CH0:d0_a DIF_CH1:d1_a DIF_CH2:d2_a DIF_CH3:clk_a DIF_CH4:d3_a DIF_CH5:d4_a
	// DIF_CH6:d0_b DIF_CH7:d1_b DIF_CH8:d2_b DIF_CH9:clk_b DIF_CH10:d3_b DIF_CH11:d4_b
						// port_swap, lane_reverse
	unsigned int ch_map_6lane[8] = {0x456789ab, 0x0123,  // 1, 1
					0x10ba9876, 0x5432,  // 1, 0
					0xab012345, 0x6789,  // 0, 1
					0x76543210, 0xba98}; // 0, 0

	// 10-12 channel: //2k chips
	// d0_a:0 d1_a:1 d2_a:2 clk_a:3 d3_a:4 d0_b:6 d1_b:7 d2_b:8 clk_b:9 d3_b:a
	// 5 & b: null
	// DIF_CH0:d0_a DIF_CH1:d1_a DIF_CH2:d2_a DIF_CH3:clk_a DIF_CH4:d3_a
	// DIF_CH5:d0_b DIF_CH6:d1_b DIF_CH7:d2_b DIF_CH8:clk_b DIF_CH9:d3_b
	unsigned int ch_map_5lane[8] = {0x2346789a, 0x5b01,  // 1, 1
					0x210a9876, 0x5b43,  // 1, 0
					0x89a01234, 0xb567,  // 0, 1
					0x87643210, 0xb5a9}; // 0, 0

	// 12-16 channel:
	// d0_a:0 d1_a:1 d2_a:2 clk_a:3 d3_a:4 d4_a:5 d0_b:8 d1_b:9 d2_b:a clk_b:b d3_b:c d4_b:d
	// DIF_CH0:d0_a DIF_CH1:d1_a DIF_CH2:d2_a DIF_CH3:clk_a DIF_CH4:d3_a DIF_CH5:d4_a
	// DIF_CH6:invalid DIF_CH7:invalid
	// DIF_CH8:d0_b DIF_CH9:d1_b DIF_CH10:d2_b DIF_CH11:clk_b DIF_CH12:d3_b DIF_CH13:d4_b
	// DIF_CH14:invalid DIF_CH15:invalid
	unsigned int ch_map_8_to_6lane[8] = {0x89abcdef, 0x01234567,   // 1, 1
					     0xfedcba98, 0x76543210,   // 1, 0
					     0x01234567, 0x89abcdef,   // 0, 1
					     0x76543210, 0xfedcba98};  // 0, 0

	ch_reg_idx  = pdrv->config.control.lvds_cfg.port_swap ? 0 : 4;
	ch_reg_idx += pdrv->config.control.lvds_cfg.lane_reverse ? 0 : 2;
	lcd_bits = pdrv->config.timing.base_timing->lcd_bits;
	if (lcd_bits == 30) {
		valid_port0_s = 0;
		valid_port0_e = 5;
		valid_port1_s = 6;
		valid_port1_e = 0xb;
	} else if (lcd_bits == 18) {
		valid_port0_s = 0;
		valid_port0_e = 3;
		valid_port1_s = 6;
		valid_port1_e = 9;
	} else { //24bit
		valid_port0_s = 0;
		valid_port0_e = 4;
		valid_port1_s = 6;
		valid_port1_e = 0xa;
	}

	/* lvds swap */
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
		*map0 = ch_map_6lane[ch_reg_idx];
		*map1 = ch_map_6lane[ch_reg_idx + 1];
		break;
	case LCD_CHIP_TXHD2:
	case LCD_CHIP_T6D:
		*map0 = ch_map_5lane[ch_reg_idx];
		*map1 = ch_map_5lane[ch_reg_idx + 1];
		break;
	case LCD_CHIP_T3X:
		if (lcd_bits == 30) {
			valid_port0_s = 0;
			valid_port0_e = 5;
			valid_port1_s = 8;
			valid_port1_e = 0xd;
		} else if (lcd_bits == 18) {
			valid_port0_s = 0;
			valid_port0_e = 3;
			valid_port1_s = 8;
			valid_port1_e = 0xb;
		} else { //24bit
			valid_port0_s = 0;
			valid_port0_e = 4;
			valid_port1_s = 8;
			valid_port1_e = 0xc;
		}

		*map0 = ch_map_8_to_6lane[ch_reg_idx];
		*map1 = ch_map_8_to_6lane[ch_reg_idx + 1];
		break;
	default:
		return;
	}

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if (i < 8) {
			n = i * 4;
			temp = (*map0 >> n) & 0xf;
			if (temp >= valid_port0_s && temp <= valid_port0_e)
				continue; //port0 valid lane
			if (pdrv->config.control.lvds_cfg.dual_port &&
			    (temp >= valid_port1_s && temp <= valid_port1_e)) {
				continue; //port1 valid lane
			}
			*map0 |= (0xf << n);
			continue;
		}
		if (i < 16) {
			n = (i - 8) * 4;
			temp = (*map1 >> n) & 0xf;
			if (temp >= valid_port0_s && temp <= valid_port0_e)
				continue; //port0 valid lane
			if (pdrv->config.control.lvds_cfg.dual_port &&
			    (temp >= valid_port1_s && temp <= valid_port1_e)) {
				continue; //port1 valid lane
			}
			*map1 |= (0xf << n);
		}
	}
}

static void lcd_mlvds_phy_ckdi_config(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	unsigned int clk_a_sel, clk_b_sel, pi_clk_sel = 0;
	int i;

	clk_a_sel = 0x3;
	clk_b_sel = 0x7;
	for (i = 0; i < phy_cfg->lane_num; i++) {
		if (phy_cfg->ch_ctrl[i].sel == clk_a_sel ||
		    phy_cfg->ch_ctrl[i].sel == clk_b_sel)
			pi_clk_sel |= (1 << i);
	}

	phy_cfg->ckdi = pi_clk_sel;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: mlvds ckdi=0x%03x\n", pdrv->index, phy_cfg->ckdi);
}

void lcd_lane_map_preset(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	unsigned int lane_map0 = 0xffffffff, lane_map1 = 0xffffffff;
	unsigned int bit, sel = 0xff;
	int i;

	lcd_lane_map_chip_init(pdrv, phy_cfg);

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_lvds_lane_swap(pdrv, &lane_map0, &lane_map1);
		break;
	case LCD_VBYONE:
		if (pdrv->config.control.vbyone_cfg.lane_count == 1) {
			lane_map0 = 0xfffffff0;
			lane_map1 = 0xffffffff;
		} else if (pdrv->config.control.vbyone_cfg.lane_count == 2) {
			lane_map0 = 0xffffff10;
			lane_map1 = 0xffffffff;
		} else if (pdrv->config.control.vbyone_cfg.lane_count == 4) {
			lane_map0 = 0xffff3210;
			lane_map1 = 0xffffffff;
		} else if (pdrv->config.control.vbyone_cfg.lane_count == 16) {
			lane_map0 = 0x76543210;
			lane_map1 = 0xfedcba98;
		} else { //8lane
			lane_map0 = 0x76543210;
			lane_map1 = 0xffffffff;
		}
		break;
	case LCD_MLVDS:
		lane_map0 = pdrv->config.control.mlvds_cfg.channel_sel0;
		lane_map1 = pdrv->config.control.mlvds_cfg.channel_sel1;
		break;
	case LCD_P2P:
		lane_map0 = pdrv->config.control.p2p_cfg.channel_sel0;
		lane_map1 = pdrv->config.control.p2p_cfg.channel_sel1;
		break;
	case LCD_MIPI:
		if (pdrv->config.control.mipi_cfg.lane_num == 1) {
			lane_map0 = 0xfffff2f0;
			lane_map1 = 0xffffffff;
		} else if (pdrv->config.control.mipi_cfg.lane_num == 2) {
			lane_map0 = 0xfffff210;
			lane_map1 = 0xffffffff;
		} else if (pdrv->config.control.mipi_cfg.lane_num == 3) {
			lane_map0 = 0xffff3210;
			lane_map1 = 0xffffffff;
		} else {
			lane_map0 = 0xfff43210;
			lane_map1 = 0xffffffff;
		}
		break;
	case LCD_EDP:
		lane_map0 = 0xfff43210;
		lane_map1 = 0xffffffff;
		break;
	default:
		break;
	}

	for (i = 0; i < phy_cfg->lane_num; i++) {
		if (i < 8) {
			bit = i * 4;
			sel = (lane_map0 >> bit) & 0xf;
		} else if (i < 16) {
			bit = (i - 8) * 4;
			sel = (lane_map1 >> bit) & 0xf;
		}

		if (pdrv->config.basic.lcd_type == LCD_VBYONE &&
		    pdrv->config.control.vbyone_cfg.lane_count == 16) {
			phy_cfg->ch_ctrl[i].sel = sel;
		} else {
			if (sel == 0xf)
				phy_cfg->ch_ctrl[i].sel = 0xff;
			else
				phy_cfg->ch_ctrl[i].sel = sel;
		}
	}

	lcd_lane_sel_to_ch_swap(pdrv, phy_cfg);
	if (pdrv->config.basic.lcd_type == LCD_MLVDS)
		lcd_mlvds_phy_ckdi_config(pdrv);
}

void lcd_lane_map_update(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;

	lcd_lane_sel_to_ch_swap(pdrv, phy_cfg);
	if (pdrv->config.basic.lcd_type == LCD_MLVDS)
		lcd_mlvds_phy_ckdi_config(pdrv);
}

int lcd_lane_sel_get(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg)
{
	unsigned int offset;

	if (!pdrv || !phy_cfg)
		return -1;

	offset = pdrv->data->offset_venc_if[pdrv->index];
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X: /* reg P2P_CH_SWAP can't readback, just use phy_cfg value */
		phy_cfg->ch_swap0 = pdrv->config.phy_cfg.ch_swap0;
		phy_cfg->ch_swap1 = pdrv->config.phy_cfg.ch_swap1;
		break;
	default:
		phy_cfg->ch_swap0 = lcd_vcbus_read(P2P_CH_SWAP0 + offset);
		phy_cfg->ch_swap1 = lcd_vcbus_read(P2P_CH_SWAP1 + offset);
		break;
	}
	lcd_ch_swap_to_lane_sel(pdrv, phy_cfg);

	return 0;
}

static void lcd_lane_map_set(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X: /* reg P2P_CH_SWAP can't readback, so print value when write reg */
		lcd_vcbus_write(P2P_CH_SWAP0 + offset, pdrv->config.phy_cfg.ch_swap0);
		lcd_vcbus_write(P2P_CH_SWAP1 + offset, pdrv->config.phy_cfg.ch_swap1);
		LCDPR("[%d]: %s: P2P_CH_SWAP0=0x%x, P2P_CH_SWAP1=0x%x\n",
		      pdrv->index, __func__, pdrv->config.phy_cfg.ch_swap0,
		      pdrv->config.phy_cfg.ch_swap1);
		break;
	default:
		lcd_vcbus_write(P2P_CH_SWAP0 + offset, pdrv->config.phy_cfg.ch_swap0);
		lcd_vcbus_write(P2P_CH_SWAP1 + offset, pdrv->config.phy_cfg.ch_swap1);
		break;
	}
}

void lcd_mipi_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int bit_lane_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TXHD2:
		if (pdrv->index == 0) {
			bit_lane_sel = 0;
		} else if (pdrv->index == 1) {
			bit_lane_sel = 16;
		} else {
			LCDERR("[%d]: %s: invalid drv_index\n", pdrv->index, __func__);
			return;
		}
		if (on_off) {
			// sel dphy lane
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, 0x0, bit_lane_sel, 10);
			lcd_lane_map_set(pdrv);
		}
		break;
	default:
		break;
	}
}

void lcd_edp_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int reg_dphy_tx_ctrl0, reg_dphy_tx_ctrl1;
	unsigned int bit_data_in_lvds, bit_data_in_edp, bit_lane_sel;

	if (pdrv->index == 0) {
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		bit_data_in_lvds = 0;
		bit_data_in_edp = 1;
		bit_lane_sel = 0;
	} else if (pdrv->index == 1) {
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		bit_data_in_lvds = 2;
		bit_data_in_edp = 3;
		bit_lane_sel = 16;
	} else {
		LCDERR("[%d]: %s: invalid drv_index\n", pdrv->index, __func__);
		return;
	}

	if (on_off) {
		// sel dphy data_in
		lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_lvds, 1);
		lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_edp, 1);
		// sel dphy lane
		lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, 0x155, bit_lane_sel, 10);
		// sel edp fifo clkdiv 20, enable lane
		lcd_combo_dphy_write(reg_dphy_tx_ctrl0, ((0x4 << 5) | (0x1f << 16)));
		// fifo enable
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 6, 10);
		// fifo wr enable
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 10);

		lcd_lane_map_set(pdrv);
	} else {
		// fifo wr disable
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 7, 10);
		// fifo disable
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 6, 10);
		// lane disable
		lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 8);
	}
}

void lcd_lvds_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int reg_dphy_tx_ctrl0, reg_dphy_tx_ctrl1;
	unsigned int bit_data_in_lvds = 0, bit_data_in_edp = 0, bit_lane_sel = 0;
	unsigned int val_lane_sel = 0, len_lane_sel = 0;
	unsigned int dual_port, phy_div;

	phy_div = pdrv->config.control.lvds_cfg.dual_port ? 2 : 1;
	dual_port = pdrv->config.control.lvds_cfg.dual_port & 0x1;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X:
		if (pdrv->index == 0) { /* lane0~lane4 */
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
			bit_data_in_lvds = 0;
			bit_data_in_edp = 1;
			bit_lane_sel = 0;
			// should only t3x drv0 has dual port
			val_lane_sel = dual_port ? 0x5550555 : 0x155;
			len_lane_sel = dual_port ? 32 : 10;
		} else { /* lane10~lane14 */
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
			bit_data_in_lvds = 2;
			bit_data_in_edp = 3;
			bit_lane_sel = 10;
			val_lane_sel = 0x155;
			len_lane_sel = 10;
		}

		if (on_off) {
			// sel dphy data_in
			if (bit_data_in_edp < 0xff)
				lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_edp, 1);
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_lvds, 1);
			// sel dphy lane
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, val_lane_sel,
				bit_lane_sel, len_lane_sel);
			/* set fifo_clk_sel: div 7 */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl0, (1 << 5));
			/* set cntl_ser_en:  8-channel */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0xffff, 16, 16);
			/* decoupling fifo enable, gated clock enable */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl1, (1 << 6) | (1 << 0));
			/* decoupling fifo write enable after fifo enable */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 1);
			/* t3x: pn swap */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 1, 2, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 6, 2);
			/* disable lane */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 16);
		}
		break;
	case LCD_CHIP_T5M:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL0, (1 << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	case LCD_CHIP_TXHD2:
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl0, (1 << 6));
			/* set cntl_ser_en:  8-channel */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0xfff, 16, 12);
			/* decoupling fifo enable, gated clock enable */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl1, 0xc1000000);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 30, 2);
			/* disable lane */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 16);
		}
		break;
	case LCD_CHIP_T6D:
		if (on_off) {
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, 0xc3000000);
			lcd_lane_map_set(pdrv);
		} else {
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, 0);
		}
		break;
	default:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL0, (1 << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL1,
				      (1 << 30) | ((phy_div - 1) << 25) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	}
}

void lcd_vbyone_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int div_sel, lane_num, lane_sel = 0;
	unsigned int reg_dphy_tx_ctrl0, reg_dphy_tx_ctrl1;
	unsigned int bit_data_in_lvds = 0, bit_data_in_edp = 0, bit_lane_sel = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lane_num = pdrv->config.control.vbyone_cfg.lane_count;

	div_sel = pdrv->config.timing.act_timing.lcd_bits;
	div_sel = div_sel == 18 ? 0 : div_sel == 24 ? 2 : 3;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X:
		if (pdrv->index == 0) {
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
			bit_data_in_lvds = 0;
			bit_data_in_edp = 1;
			bit_lane_sel = 0;
			lane_sel = 0x5555;
		} else { // drv1
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
			bit_data_in_lvds = 2;
			bit_data_in_edp = 3;
			bit_lane_sel = 16;
			lane_sel = 0xaaaa;
		}

		if (on_off) {
			// sel dphy data_in
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_edp, 1);
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_lvds, 1);

			// sel dphy lane
			if (pdrv->index == 0 && lane_num > 8)  // T3X 16-lane
				lcd_combo_dphy_write(COMBO_DPHY_CNTL1, 0x55555555);
			else  // lane8~15 sel [1]: mux to phy0 lane8~15, [2]: mux to phy1 lane0~7
				lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, lane_sel, bit_lane_sel, 16);

			/* set fifo_clk_sel: div 7 */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl0, (div_sel << 5));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0xffff, 16, 16);

			/* decoupling fifo enable, gated clock enable */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl1, (1 << 6) | (1 << 0));
			/* decoupling fifo write enable after fifo enable */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 6, 2);
			/* disable lane */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 16);
		}
		break;
	case LCD_CHIP_T5M:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL0, (div_sel << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	default:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL0, (div_sel << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	}
}

#ifdef CONFIG_AML_LCD_TCON
void lcd_mlvds_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int div_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	/* phy_div: 0=div6, 1=div 7, 2=div8, 3=div10 */
	if (pdrv->config.timing.act_timing.lcd_bits == 18)
		div_sel = 0;
	else // lcd_bits == 24
		div_sel = 2;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
		if (on_off) {
			/* fifo_clk_sel[7:6]: 0=div6, 1=div 7, 2=div8, 3=div10 */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL0, (div_sel << 6));
			/* serializer_en[27:16] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap[2] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* fifo enable[30], phy_clock gating[24] */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* fifo write enable[31] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	case LCD_CHIP_TXHD2:
		if (on_off) {
			/* fifo_clk_sel[7:6]: 0=div6, 1=div 7, 2=div8, 3=div10 */
			lcd_ana_write(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, (div_sel << 6));
			/* serializer_en[27:16] */
			lcd_ana_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, 0xfff, 16, 12);
			/* pn swap[2] */
			lcd_ana_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, 1, 2, 1);
			/* fifo enable[30], phy_clock gating[24] */
			lcd_ana_write(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, (1 << 30) | (1 << 24));
			/* fifo write enable[31] */
			lcd_ana_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, 0, 16, 12);
		}
		break;
	case LCD_CHIP_T6D:
		if (on_off) {
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, 0xc3000000);
			// lane_swap_set
			lcd_lane_map_set(pdrv);
		} else {
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, 0);
		}
		break;
	default:
		if (on_off) {
			/* fifo_clk_sel[7:6]: 0=div6, 1=div 7, 2=div8, 3=div10 */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL0, (div_sel << 6));
			/* serializer_en[27:16] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap[2] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 1, 2, 1);
			/* fifo enable[30], phy_clock gating[24] */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* fifo write enable[31] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	}
}

void lcd_p2p_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int p2p_type, phy_div, bit_data_in_lvds, bit_data_in_edp;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	/* phy_div: 0=div6, 1=div 7, 2=div8, 3=div10 */
	p2p_type = pdrv->config.control.p2p_cfg.p2p_type & 0x1f;
	switch (p2p_type) {
	case P2P_CHPI: /* 8/10 coding */
	case P2P_USIT:
		phy_div = 3;
		break;
	default:
		phy_div = 2;
		break;
	}

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
		if (on_off) {
			/* fifo_clk_sel[7:6]: 0=div6, 1=div 7, 2=div8, 3=div10 */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL0, (phy_div << 6));
			/* serializer_en[27:16] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap[2] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 1, 2, 1);

			/* fifo enable[30], phy_clock gating[24] */
			lcd_ana_write(ANACTRL_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* fifo write enable[31] */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(ANACTRL_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	case LCD_CHIP_T3X:
		if (on_off) {
			// sel dphy data_in
			bit_data_in_lvds = 0;
			bit_data_in_edp = 1;
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_edp, 1);
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_lvds, 1);
			/*
			 * sel dphy lane.
			 * For lane8~15 sel [1]: mux to phy0 lane8~15, [2]: mux to phy1 lane0~7
			 */
			if (pdrv->config.control.p2p_cfg.lane_num > 8)
				lcd_combo_dphy_write(COMBO_DPHY_CNTL1, 0x55555555);
			else
				lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, 0x5555, 0, 16);

			/* set fifo_clk_sel: div */
			lcd_combo_dphy_write(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, (phy_div << 5));
			/* set cntl_ser_en:  all-channel to 1 */
			lcd_combo_dphy_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, 0xffff, 16, 16);
			/* pn swap */
			lcd_combo_dphy_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, 1, 2, 1);

			/* decoupling fifo enable, gated clock enable */
			lcd_combo_dphy_write(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1,
					(1 << 6) | (1 << 0));
			/* decoupling fifo write enable after fifo enable */
			lcd_combo_dphy_setb(COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, 1, 7, 1);

			lcd_lane_map_set(pdrv);
		}
		break;
	default:
		if (on_off) {
			/* fifo_clk_sel[7:6]: 0=div6, 1=div 7, 2=div8, 3=div10 */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL0, (phy_div << 6));
			/* serializer_en[27:16] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0xfff, 16, 12);
			/* pn swap[2] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 1, 2, 1);

			/* fifo enable[30], phy_clock gating[24] */
			lcd_ana_write(HHI_LVDS_TX_PHY_CNTL1, (1 << 30) | (1 << 24));
			/* fifo write enable[31] */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 1, 31, 1);
		} else {
			/* disable fifo */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(HHI_LVDS_TX_PHY_CNTL0, 0, 16, 12);
		}
		break;
	}
}
#endif

void lcd_dphy_reg_print(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s *reg_dphy_table = NULL, *reg_ctrl_table = NULL, *reg_ch_swap = NULL;
	unsigned int size_dphy = 0, size_ctrl = 0, size_ch_swap = 0, reg_bus, reg_offset;
	struct reg_name_set_s ch_swap_reg_dft[] = {
		{P2P_CH_SWAP0, "P2P_CH_SWAP0"},
		{P2P_CH_SWAP1, "P2P_CH_SWAP1"}
	};
	struct reg_name_set_s dphy_ctrl_reg_t3x[] = {
		{COMBO_DPHY_CNTL0, "COMBO_DPHY_CNTL0"},
		{COMBO_DPHY_CNTL1, "COMBO_DPHY_CNTL1"}
	};
	struct reg_name_set_s dphy_reg_t3x[] = {
		{COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, "COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, "COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0, "COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1, "COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL0, "COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL0"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL1, "COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL1"}
	};
	struct reg_name_set_s dphy_reg_t5m[] = {
		{ANACTRL_LVDS_TX_PHY_CNTL0, "ANACTRL_LVDS_TX_PHY_CNTL0"},
		{ANACTRL_LVDS_TX_PHY_CNTL1, "ANACTRL_LVDS_TX_PHY_CNTL1"},
		{ANACTRL_LVDS_TX_PHY_CNTL2, "ANACTRL_LVDS_TX_PHY_CNTL2"},
		{ANACTRL_LVDS_TX_PHY_CNTL3, "ANACTRL_LVDS_TX_PHY_CNTL3"}
	};
	struct reg_name_set_s dphy_reg_txhd2[] = {
		{COMBO_DPHY_CNTL0,                  "COMBO_DPHY_CNTL0"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0, "COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0"},
		{COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1, "COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1"}
	};

	if (!pdrv)
		return;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
	case LCD_CHIP_T6D:
		reg_ch_swap = ch_swap_reg_dft;
		size_ch_swap = ARRAY_SIZE(ch_swap_reg_dft);
		reg_dphy_table = &dphy_reg_t5m[0];
		size_dphy = 2;
		reg_bus = LCD_REG_DBG_ANA_BUS;
		break;
	case LCD_CHIP_T3X:
		reg_ch_swap = ch_swap_reg_dft;
		size_ch_swap = ARRAY_SIZE(ch_swap_reg_dft);
		reg_ctrl_table = dphy_ctrl_reg_t3x;
		size_ctrl = ARRAY_SIZE(dphy_ctrl_reg_t3x);
		if (pdrv->index) {
			reg_dphy_table = &dphy_reg_t3x[2];
			size_dphy = 2;
		} else {
			reg_dphy_table = &dphy_reg_t3x[0];
			size_dphy = 2;
		}
		reg_bus = LCD_REG_DBG_COMBOPHY_BUS;
		break;
	case LCD_CHIP_TXHD2:
		reg_ch_swap = ch_swap_reg_dft;
		size_ch_swap = ARRAY_SIZE(ch_swap_reg_dft);
		reg_dphy_table = dphy_reg_txhd2;
		size_dphy = ARRAY_SIZE(dphy_reg_txhd2);
		reg_bus = LCD_REG_DBG_COMBOPHY_BUS;
		break;
	default:
		return;
	}

	if (reg_ch_swap) {
		reg_offset = pdrv->data->offset_venc_if[pdrv->index];
		str_add_reg_sets(pdrv, LCD_REG_DBG_VC_BUS, reg_offset, reg_ch_swap, size_ch_swap);
	}
	if (reg_ctrl_table)
		str_add_reg_sets(pdrv, reg_bus, 0, reg_ctrl_table, size_ctrl);
	str_add_reg_sets(pdrv, reg_bus, 0, reg_dphy_table, size_dphy);
}
