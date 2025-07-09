// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "dptx_IP_ctrls.h"
#include "./dptx_IP_REG_T7.h"
#include "../dptx_common.h"

struct dptx_if_ctrl_s dptx_if_dp14 = {
	.aux_write = NULL,
	.aux_write_single = NULL,
	.aux_read = NULL,
	.aux_i2c_op = NULL,

	.transmit_pattern = NULL,
	.set_MSA = NULL,

	.path_reset = NULL,

	.lane_cfg_to_IP = NULL,
	.phy_cfg_to_IP = NULL,

	.transmitter_init = NULL,
	.transmitter_output = NULL,

	.get_hpd_level = NULL,
	.get_hpd_irq = NULL,
	.set_hpd_interrupt_mask = NULL,

	.scramble_reset_set = NULL,
};

struct dptx_if_ctrl_s *dptx_if_bind_DP14(struct dptx_drv_s *dptx)
{
	return &dptx_if_dp14;
}
