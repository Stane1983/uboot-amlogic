// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "dptx_phy_config.h"
#include "../dptx_common.h"

static struct dptx_phy_ctrl_s *dptx_phy_ctrl;

void dptx_phy_enable(struct dptx_drv_s *dptx, uint8_t port)
{
	if (!dptx_phy_ctrl->phy_enable) {
		DPTX_P_PR(dptx, port, "%s: phy_enable is null", __func__);
		return;
	}

	DPTX_P_DBG(dptx, port, "%s", __func__);
	dptx_phy_ctrl->phy_enable(dptx, port);
}

void dptx_phy_disable(struct dptx_drv_s *dptx, uint8_t port)
{
	if (!dptx_phy_ctrl->phy_disable) {
		DPTX_P_PR(dptx, port, "%s: phy_disable is null", __func__);
		return;
	}

	DPTX_P_DBG(dptx, port, "%s", __func__);
	dptx_phy_ctrl->phy_disable(dptx, port);
}

void dptx_phy_set_lane(struct dptx_drv_s *dptx, uint8_t port, uint8_t lane_mask)
{
	if (!dptx_phy_ctrl->phy_disable) {
		DPTX_P_PR(dptx, port, "%s: phy_disable is null", __func__);
		return;
	}

	DPTX_P_DBG(dptx, port, "%s", __func__);
	dptx_phy_ctrl->phy_set_lane(dptx, port, lane_mask);
}

void dptx_phy_probe(struct dptx_chip_data_s *dptx_chip_data)
{
	dptx_phy_ctrl = NULL;

	switch (dptx_chip_data->chip_type) {
	case DPTX_CHIP_T7:
		dptx_phy_ctrl = dptx_phy_config_init_t7();
		break;
	default:
		break;
	}
}
