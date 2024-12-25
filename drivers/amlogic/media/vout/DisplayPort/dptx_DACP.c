// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPCD_REG.h>
#include "dptx_common.h"

/* Display Authentication and Content Protection Support
 * Method 1 : HDCP
 * Method 2 : eDP panel limit
 * Method 3a: eDP Alternate Scrambler Seed Reset (ASSR)
 */

static void dptx_set_eDP_ASSR(struct dptx_drv_s *dptx)
{
	uint8_t aux_data = 0x01;

	dptx_if_scramble_reset_set(dptx, DPTX_eDP_ALTERNATIVE_SCRAMBLE_RESET);
	dptx_if_aux_write(dptx, DPCD_eDP_CONFIGURATION_SET, 1, &aux_data);

	DPTXPR(dptx->idx, LOG_I, "eDP ASSR en");
}

void dptx_set_content_protection(struct dptx_drv_s *dptx)
{
	if (dptx->link_cfg.DACP_support & 0x4)
		dptx_set_eDP_ASSR(dptx);
}
