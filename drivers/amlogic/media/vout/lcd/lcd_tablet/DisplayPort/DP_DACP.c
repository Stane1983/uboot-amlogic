// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "DP_tx.h"
#include "../lcd_tablet.h"
#include "../../lcd_reg.h"
#include "../../lcd_common.h"

/* Display Authentication and Content Protection Support
 * Method 1 : HDCP
 * Method 2 : eDP panel limit
 * Method 3a: eDP Alternate Scrambler Seed Reset (ASSR)
 */

static void dptx_set_eDP_ASSR(struct aml_lcd_drv_s *pdrv)
{
	uint8_t aux_data = 0x01;

	dptx_reg_write(pdrv->index, EDP_TX_ALTERNATE_SCRAMBLER_RESET, 1);
	dptx_aux_write(pdrv, DPCD_eDP_CONFIGURATION_SET, 1, &aux_data);
}

void dptx_set_ContentProtection(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.control.edp_cfg.DACP_support & 0x4)
		dptx_set_eDP_ASSR(pdrv);
}
