// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"

static void lcd_ch_swap_to_lane_sel(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int sel = 0xff;
	int i, n;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X:
		for (i = 0; i < phy->lane_num; i++) {
			if (i < 8) {
				n = i * 4;
				sel = (phy->ch_swap0 >> n) & 0xf;
			} else if (i < 16) {
				n = (i - 8) * 4;
				sel = (phy->ch_swap1 >> n) & 0xf;
			}
			phy->lane[i].sel = sel;
		}
		break;
	default:
		for (i = 0; i < phy->lane_num; i++) {
			if (i < 8) {
				n = i * 4;
				sel = (phy->ch_swap0 >> n) & 0xf;
			} else if (i < 16) {
				n = (i - 8) * 4;
				sel = (phy->ch_swap1 >> n) & 0xf;
			}
			if (sel == 0xf)
				phy->lane[i].sel = 0xff;
			else
				phy->lane[i].sel = sel;
		}
		break;
	}
}

static void lcd_lvds_lane_swap(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned char ch_reg_idx = 0;

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

	// 12-12 channel:
	// d0_a:0 d1_a:1 d2_a:2 clk_a:3 d3_a:4 d4_a:5 d0_b:6 d1_b:7 d2_b:8 clk_b:9 d3_b:a d4_b:b
	// DIF_CH0:d0_a DIF_CH1:d1_a DIF_CH2:d2_a DIF_CH3:clk_a DIF_CH4:d3_a
	// DIF_CH5:d0_b DIF_CH6:d1_b DIF_CH7:d2_b DIF_CH8:clk_b DIF_CH9:d3_b
	// DIF_CH10:d4_a/invalid  DIF_CH11:d4_b/invalid
	unsigned int ch_map_6_to_5lane[8] = {0x345789ab, 0x0612,  // 1, 1
					     0x210a9876, 0x5b43,  // 1, 0
					     0x9ab12345, 0x6078,  // 0, 1
					     0x87643210, 0xb5a9}; // 0, 0

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

	/* lvds swap */
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
	case LCD_CHIP_T5W:
	case LCD_CHIP_T5M:
	case LCD_CHIP_T3:
		phy->ch_swap0 = ch_map_6lane[ch_reg_idx];
		phy->ch_swap1 = ch_map_6lane[ch_reg_idx + 1];
		break;
	case LCD_CHIP_T5:
		phy->ch_swap0 = ch_map_6_to_5lane[ch_reg_idx];
		phy->ch_swap1 = ch_map_6_to_5lane[ch_reg_idx + 1];
		break;
	case LCD_CHIP_T5D:
	case LCD_CHIP_TXHD2:
	case LCD_CHIP_T6D:
		phy->ch_swap0 = ch_map_5lane[ch_reg_idx];
		phy->ch_swap1 = ch_map_5lane[ch_reg_idx + 1];
		break;
	case LCD_CHIP_T7:
		//don't support port_swap and lane_reverse when single port
		if (pdrv->index == 2 && pdrv->config.control.lvds_cfg.dual_port) {
			phy->ch_swap0 = ch_map_6_to_5lane[ch_reg_idx];
			phy->ch_swap1 = ch_map_6_to_5lane[ch_reg_idx + 1];
		} else if (pdrv->index == 1) {
			phy->ch_swap0 = 0xf43210ff;
			phy->ch_swap1 = 0xffff;
		} else { // (drv==2, single port) || drv==0
			phy->ch_swap0 = 0xfff43210;
			phy->ch_swap1 = 0xffff;
		}
		break;
	case LCD_CHIP_T3X: // second path not support lvds
		phy->ch_swap0 = ch_map_8_to_6lane[ch_reg_idx];
		phy->ch_swap1 = ch_map_8_to_6lane[ch_reg_idx + 1];
		break;
	default:
		break;
	}
}

static void lcd_mlvds_phy_ckdi_config(struct aml_lcd_drv_s *pdrv)
{
	unsigned int channel_sel0, channel_sel1, pi_clk_sel = 0;
	unsigned int i, temp;

	channel_sel0 = pdrv->config.phy_cfg.ch_swap0;
	channel_sel1 = pdrv->config.phy_cfg.ch_swap1;

	/* pi_clk select */
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
		/* mlvds channel:    //tx 12 channels
		 *    0: clk_a
		 *    1: d0_a
		 *    2: d1_a
		 *    3: d2_a
		 *    4: d3_a
		 *    5: d4_a
		 *    6: clk_b
		 *    7: d0_b
		 *    8: d1_b
		 *    9: d2_b
		 *   10: d3_b
		 *   11: d4_b
		 */
		for (i = 0; i < 8; i++) {
			temp = (channel_sel0 >> (i * 4)) & 0xf;
			if (temp == 0 || temp == 6)
				pi_clk_sel |= (1 << i);
		}
		for (i = 0; i < 4; i++) {
			temp = (channel_sel1 >> (i * 4)) & 0xf;
			if (temp == 0 || temp == 6)
				pi_clk_sel |= (1 << (i + 8));
		}
		break;
	case LCD_CHIP_T5:
	case LCD_CHIP_T5D:
	case LCD_CHIP_T3:
	case LCD_CHIP_T5W:
	case LCD_CHIP_T5M:
	case LCD_CHIP_TXHD2:
	case LCD_CHIP_T6D:
		/* mlvds channel:    //tx 8 channels
		 *    0: d0_a
		 *    1: d1_a
		 *    2: d2_a
		 *    3: clk_a
		 *    4: d0_b
		 *    5: d1_b
		 *    6: d2_b
		 *    7: clk_b
		 */
		for (i = 0; i < 8; i++) {
			temp = (channel_sel0 >> (i * 4)) & 0xf;
			if (temp == 3 || temp == 7)
				pi_clk_sel |= (1 << i);
		}
		for (i = 0; i < 4; i++) {
			temp = (channel_sel1 >> (i * 4)) & 0xf;
			if (temp == 3 || temp == 7)
				pi_clk_sel |= (1 << (i + 8));
		}
		break;
	default:
		break;
	}

	pdrv->config.control.mlvds_cfg.pi_clk_sel = pi_clk_sel;
	pdrv->config.phy_cfg.ckdi = pi_clk_sel;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: channel_sel0=0x%08x, channel_sel1=0x%08x, pi_clk_sel=0x%03x\n",
		      pdrv->index, channel_sel0, channel_sel1, pi_clk_sel);
	}
}

void lcd_lane_map_preset(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_lvds_lane_swap(pdrv);
		break;
	case LCD_VBYONE:
		if (pdrv->config.control.vbyone_cfg.lane_count <= 8 &&
		    pdrv->config.control.vbyone_cfg.slice == 2) {
			phy->ch_swap0 = 0xba983210;
			phy->ch_swap1 = 0xfedc7654;
		} else {
			phy->ch_swap0 = 0x76543210;
			phy->ch_swap1 = 0xfedcba98;
		}
		break;
	case LCD_MLVDS:
		phy->ch_swap0 = pdrv->config.control.mlvds_cfg.channel_sel0;
		phy->ch_swap1 = pdrv->config.control.mlvds_cfg.channel_sel1;
		break;
	case LCD_P2P:
		phy->ch_swap0 = pdrv->config.control.p2p_cfg.channel_sel0;
		phy->ch_swap1 = pdrv->config.control.p2p_cfg.channel_sel1;
		break;
	case LCD_MIPI:
		phy->ch_swap0 = 0x76543210;
		phy->ch_swap1 = 0xfedcba98;
		break;
	case LCD_EDP:
		phy->ch_swap0 = 0x76543210;
		phy->ch_swap1 = 0xfedcba98;
		break;
	default:
		break;
	}
	lcd_ch_swap_to_lane_sel(pdrv);
	if (pdrv->config.basic.lcd_type == LCD_MLVDS)
		lcd_mlvds_phy_ckdi_config(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ch_swap0: 0x%08x, ch_swap1: 0x%08x\n",
		      pdrv->index, __func__, phy->ch_swap0, phy->ch_swap1);
	}
}

void lcd_lane_map_update(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	int i, n;

	phy->ch_swap0 = 0;
	phy->ch_swap1 = 0;
	for (i = 0; i < phy->lane_num; i++) {
		if (i < 8) {
			n = i * 4;
			if (phy->lane[i].sel == 0xff)
				phy->ch_swap0 |= (0xf << n);
			else
				phy->ch_swap0 |= (phy->lane[i].sel << n);
			continue;
		}
		if (i < 16) {
			n = (i - 8) * 4;
			if (phy->lane[i].sel == 0xff)
				phy->ch_swap1 |= (0xf << n);
			else
				phy->ch_swap1 |= (phy->lane[i].sel << n);
		}
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: lane_nem: %d, ch_swap0: 0x%08x, ch_swap1: 0x%08x\n",
		      pdrv->index, __func__, phy->lane_num, phy->ch_swap0, phy->ch_swap1);
	}

	if (pdrv->config.basic.lcd_type == LCD_MLVDS)
		lcd_mlvds_phy_ckdi_config(pdrv);
}

static void lcd_lane_map_set(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];
	lcd_vcbus_write(P2P_CH_SWAP0 + offset, pdrv->config.phy_cfg.ch_swap0);
	lcd_vcbus_write(P2P_CH_SWAP1 + offset, pdrv->config.phy_cfg.ch_swap1);
}

static void lcd_lane_map_set_t7_lvds(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];
	//don't support port_swap and lane_reverse when single port
	if (pdrv->index == 2 && pdrv->config.control.lvds_cfg.dual_port) {
		lcd_vcbus_write(P2P_CH_SWAP0 + offset, pdrv->config.phy_cfg.ch_swap0);
		lcd_vcbus_write(P2P_CH_SWAP1 + offset, pdrv->config.phy_cfg.ch_swap1);
	} else if (pdrv->index == 1) {
		lcd_vcbus_write(P2P_CH_SWAP0 + offset, 0xf43210ff);
		lcd_vcbus_write(P2P_CH_SWAP1 + offset, 0xffff);
	} else { // (drv==2, single port) || drv==0
		lcd_vcbus_write(P2P_CH_SWAP0 + offset, 0xfff43210);
		lcd_vcbus_write(P2P_CH_SWAP1 + offset, 0xffff);
	}
}

void lcd_mipi_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int bit_lane_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
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
	case LCD_CHIP_T7:
		if (pdrv->index == 0) { /* lane0~lane4 */
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
			bit_data_in_lvds = 0;
			bit_data_in_edp = 1;
			bit_lane_sel = 0;
			// should only t3x drv0 has dual port
			val_lane_sel = dual_port ? 0x5550555 : 0x155;
			len_lane_sel = dual_port ? 32 : 10;
		} else if (pdrv->index == 1) { /* lane10~lane14 */
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
			bit_data_in_lvds = 2;
			bit_data_in_edp = 3;
			bit_lane_sel = 20;
			val_lane_sel = 0x155;
			len_lane_sel = 10;
		} else { // should only t7 has drv2
			reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL0;
			reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL1;
			bit_data_in_lvds = 4;
			bit_data_in_edp = 0xff;
			bit_lane_sel = 10;
			// dual_port ? lane5~lane14 : lane5~lane9
			val_lane_sel = dual_port ? 0xaaaaa : 0x2aa;
			len_lane_sel = dual_port ? 20 : 10;
		}
		break;
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
		break;
	case LCD_CHIP_T3:
	case LCD_CHIP_T5M:
	case LCD_CHIP_T6D:
		reg_dphy_tx_ctrl0 = ANACTRL_LVDS_TX_PHY_CNTL0;
		reg_dphy_tx_ctrl1 = ANACTRL_LVDS_TX_PHY_CNTL1;
		break;
	case LCD_CHIP_TXHD2:
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		break;
	default:
		reg_dphy_tx_ctrl0 = HHI_LVDS_TX_PHY_CNTL0;
		reg_dphy_tx_ctrl1 = HHI_LVDS_TX_PHY_CNTL1;
		break;
	}

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
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
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0x3ff, 16, 10);
			/* decoupling fifo enable, gated clock enable */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl1, (1 << 6) | (1 << 0));
			/* decoupling fifo write enable after fifo enable */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 1);

			lcd_lane_map_set_t7_lvds(pdrv);
		} else {
			/* disable fifo */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 6, 2);
			/* disable lane */
			lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 16);
		}
		break;
	case LCD_CHIP_T3X:
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
	case LCD_CHIP_T3:
	case LCD_CHIP_T5M:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(reg_dphy_tx_ctrl0, (1 << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(reg_dphy_tx_ctrl1, (1 << 30) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0, 16, 12);
		}
		break;
	case LCD_CHIP_T6D:
		lcd_ana_write(reg_dphy_tx_ctrl1, on_off ? 0xc3000000 : 0);
		lcd_lane_map_set(pdrv);
		break;
	case LCD_CHIP_TXHD2:
		if (on_off) {
			lcd_combo_dphy_write(COMBO_DPHY_CNTL0, 0x55555);
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
	default:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(reg_dphy_tx_ctrl0, (1 << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(reg_dphy_tx_ctrl1,
				(1 << 30) | ((phy_div - 1) << 25) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0, 16, 12);
		}
		break;
	}
}

void lcd_vbyone_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off)
{
	unsigned int div_sel, lane_num, lane_sel = 0;
	unsigned int reg_dphy_tx_ctrl0, reg_dphy_tx_ctrl1;
	unsigned int bit_data_in_lvds = 0, bit_data_in_edp = 0, bit_lane_sel = 0;
	unsigned int bit_fifo_clk = 0, cntl_ser_mask = 0, cntl_ser_bit = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lane_num = pdrv->config.control.vbyone_cfg.lane_count;

	if (pdrv->config.basic.lcd_bits == 6)
		div_sel = 0;
	else if (pdrv->config.basic.lcd_bits == 8)
		div_sel = 2;
	else // pdrv->config.basic.lcd_bits == 10
		div_sel = 3;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
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
		break;
	case LCD_CHIP_T3:
	case LCD_CHIP_T5M:
		if (pdrv->index == 0) {
			reg_dphy_tx_ctrl0 = ANACTRL_LVDS_TX_PHY_CNTL0;
			reg_dphy_tx_ctrl1 = ANACTRL_LVDS_TX_PHY_CNTL1;
			cntl_ser_bit = 12;
			cntl_ser_mask = 0xfff;
			bit_fifo_clk = 1;
		} else { // drv1
			reg_dphy_tx_ctrl0 = ANACTRL_LVDS_TX_PHY_CNTL2;
			reg_dphy_tx_ctrl1 = ANACTRL_LVDS_TX_PHY_CNTL3;
			cntl_ser_bit = 4;
			cntl_ser_mask = 0xf;
			bit_fifo_clk = 3;
		}
		break;
	default:
		reg_dphy_tx_ctrl0 = HHI_LVDS_TX_PHY_CNTL0;
		reg_dphy_tx_ctrl1 = HHI_LVDS_TX_PHY_CNTL1;
		break;
	}

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
	case LCD_CHIP_T3X:
		if (on_off) {
			// sel dphy data_in
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_edp, 1);
			lcd_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_lvds, 1);

			// sel dphy lane
			if (pdrv->data->chip_type == LCD_CHIP_T7)
				lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, 0x5555, bit_lane_sel, 16);
			else if (pdrv->index == 0 && lane_num > 8)  // T3X 16-lane
				lcd_combo_dphy_write(COMBO_DPHY_CNTL1, 0x55555555);
			else  // lane8~15 sel [1]: mux to phy0 lane8~15, [2]: mux to phy1 lane0~7
				lcd_combo_dphy_setb(COMBO_DPHY_CNTL1, lane_sel, bit_lane_sel, 16);

			/* set fifo_clk_sel: div 7 */
			lcd_combo_dphy_write(reg_dphy_tx_ctrl0, (div_sel << 5));
			/* set cntl_ser_en:  8-channel to 1 */
			if (pdrv->data->chip_type == LCD_CHIP_T7)
				lcd_combo_dphy_setb(reg_dphy_tx_ctrl0, 0xff, 16, 8);
			if (pdrv->data->chip_type == LCD_CHIP_T3X)
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
	case LCD_CHIP_T3:
	case LCD_CHIP_T5M:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(reg_dphy_tx_ctrl0, (div_sel << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(reg_dphy_tx_ctrl0, cntl_ser_mask, 16, cntl_ser_bit);
			/* pn swap */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(reg_dphy_tx_ctrl1, (1 << 30) | (bit_fifo_clk << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0, 16, 12);
		}
		break;
	default:
		if (on_off) {
			/* set fifo_clk_sel: div 7 */
			lcd_ana_write(reg_dphy_tx_ctrl0, (div_sel << 6));
			/* set cntl_ser_en:  8-channel to 1 */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0xfff, 16, 12);
			/* pn swap */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 1, 2, 1);
			/* decoupling fifo enable, gated clock enable */
			lcd_ana_write(reg_dphy_tx_ctrl1, (1 << 30) | (1 << 24));
			/* decoupling fifo write enable after fifo enable */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 1, 31, 1);

			lcd_lane_map_set(pdrv);
		} else {
			/* disable fifo */
			lcd_ana_setb(reg_dphy_tx_ctrl1, 0, 30, 2);
			/* disable lane */
			lcd_ana_setb(reg_dphy_tx_ctrl0, 0, 16, 12);
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
	if (pdrv->config.basic.lcd_bits == 6)
		div_sel = 0;
	else // lcd_bits == 8
		div_sel = 2;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3:
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
	case LCD_CHIP_T3:
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
