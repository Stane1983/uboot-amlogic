// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../../lcd_reg.h"
#include "../../../lcd_common.h"
#include "../../lcd_tablet.h"
#include "./dsi_ctrl.h"

struct dsi_ctrl_s dsi_ctrl_v2 = {};

struct dsi_ctrl_s *dsi_bind_v2(struct aml_lcd_drv_s *pdrv)
{
	return &dsi_ctrl_v2;
}
