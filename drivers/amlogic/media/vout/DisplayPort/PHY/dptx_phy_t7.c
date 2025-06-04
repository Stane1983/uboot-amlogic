// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/types.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "dptx_phy_config.h"
#include "../dptx_common.h"
#include "../dptx_regs.h"

static void dptx_dphy_set(struct dptx_drv_s *dptx, u8 on_off)
{
	unsigned int reg_dphy_tx_ctrl0, reg_dphy_tx_ctrl1;
	unsigned int bit_data_in_lvds, bit_data_in_edp, bit_lane_sel;

	if (dptx->idx == 0) {
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		bit_data_in_lvds = 0;
		bit_data_in_edp = 1;
		bit_lane_sel = 0;
	} else { //dptx->idx == 1
		reg_dphy_tx_ctrl0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
		reg_dphy_tx_ctrl1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		bit_data_in_lvds = 2;
		bit_data_in_edp = 3;
		bit_lane_sel = 16;
	}

	if (on_off) {
		// sel dphy data_in
		dptx_combo_dphy_setb(COMBO_DPHY_CNTL0, 0, bit_data_in_lvds, 1);
		dptx_combo_dphy_setb(COMBO_DPHY_CNTL0, 1, bit_data_in_edp, 1);
		// sel dphy lane
		dptx_combo_dphy_setb(COMBO_DPHY_CNTL1, 0x155, bit_lane_sel, 10);
		// sel edp fifo clkdiv 20, enable lane
		dptx_combo_dphy_write(reg_dphy_tx_ctrl0, ((0x4 << 5) | (0x1f << 16)));
		dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 6, 10); // fifo enable
		dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 1, 7, 10); // fifo wr enable

		dptx_combo_dphy_write(reg_dphy_tx_ctrl1, 0x000000d1); // !temp
	} else {
		dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 7, 10); // fifo wr disable
		dptx_combo_dphy_setb(reg_dphy_tx_ctrl1, 0, 6, 10); // fifo disable
		dptx_combo_dphy_setb(reg_dphy_tx_ctrl0, 0, 16, 8); // lane disable
	}
}

static void dptx0_ana_phy_cntl_set(struct dptx_drv_s *dptx, uint8_t status)
{
	struct dptx_phy_cfg_s *phy = &dptx->phy_cfg;

	dptx_ana_write(ANACTRL_DIF_PHY_CNTL1, status ? 0x46770038 : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL2, status ? 0x0000ffff : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL3,
		status ? (0x16530028 | (phy->lane[0].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL4,
		status ? (0x16530028 | (phy->lane[1].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL5,
		status ? (0x16530028 | (phy->lane[2].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL6,
		status ? (0x16530028 | (phy->lane[3].preem << 28)) : 0);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL7, data_lane);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL8, data_lane);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL9, data_lane);
}

static void dptx1_ana_phy_cntl_set(struct dptx_drv_s *dptx, uint8_t status)
{
	struct dptx_phy_cfg_s *phy = &dptx->phy_cfg;

	dptx_ana_write(ANACTRL_DIF_PHY_CNTL10, status ? 0x46770038 : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL11, status ? 0x0000ffff : 0);

	dptx_ana_write(ANACTRL_DIF_PHY_CNTL12,
		status ? (0x16530028 | (phy->lane[0].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL13,
		status ? (0x16530028 | (phy->lane[1].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL14,
		status ? (0x16530028 | (phy->lane[2].preem << 28)) : 0);
	dptx_ana_write(ANACTRL_DIF_PHY_CNTL15,
		status ? (0x16530028 | (phy->lane[3].preem << 28)) : 0);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL16, data_lane);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL17, data_lane);
	// dptx_ana_write(ANACTRL_DIF_PHY_CNTL18, data_lane);
}

static void dptx_ana_phy_set(struct dptx_drv_s *dptx, u8 status)
{
	// uint32_t flag, data_lane0_aux, data_lane1_aux, data_lane;
	struct dptx_phy_cfg_s *phy = &dptx->phy_cfg;
	uint32_t vswing;

	DPTXPR(dptx->idx, LOG_I, "%s: %u", __func__, status);

	if (dptx->idx == 0)
		dptx0_ana_phy_cntl_set(dptx, status);
	else
		dptx1_ana_phy_cntl_set(dptx, status);

	if (status) {
		vswing = phy->vswing >= 0x3 ? phy->vswing : 0x3;
		dptx_ana_write(ANACTRL_DIF_PHY_CNTL19, 0x00406240 | vswing);
	}

	// DPTXPR(dptx->idx, LOG_I, "phy lane_lock: 0x%x\n", phy_ctrl_p->lane_lock);
}

static void dptx_phy_enable_t7(struct dptx_drv_s *dptx)
{
	dptx->phy_cfg.vswing = 0x3;
	dptx_ana_phy_set(dptx, 1);
	dptx_dphy_set(dptx, 1);
}

static void dptx_phy_disable_t7(struct dptx_drv_s *dptx)
{
	dptx_dphy_set(dptx, 0);
	dptx_ana_phy_set(dptx, 0);
}

static void dptx_phy_set_lane_t7(struct dptx_drv_s *dptx, uint8_t lane_mask)
{
	// !!!!!!!!!!!!!!!!!!!
	dptx_ana_phy_set(dptx, 1);
	dptx_dphy_set(dptx, 1);
}

static struct dptx_phy_ctrl_s dptx_phy_ctrl_t7 = {
	.lane_lock     = 0,
	.phy_enable    = dptx_phy_enable_t7,
	.phy_disable   = dptx_phy_disable_t7,
	.phy_set_lane  = dptx_phy_set_lane_t7,
};

struct dptx_phy_ctrl_s *dptx_phy_config_init_t7(void)
{
	return &dptx_phy_ctrl_t7;
}
