// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_tablet.h"

static void lcd_list_support_mode(struct aml_lcd_drv_s *pdrv)
{
	printf("panel\n");
}

static int lcd_outputmode_is_matched(struct aml_lcd_drv_s *pdrv, char *mode)
{
	char str[16];

	if (pdrv->index == 0)
		sprintf(str, "panel");
	else
		sprintf(str, "panel%d", pdrv->index);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: outputmode=%s, support mode=%s\n",
		      pdrv->index, __func__, mode, str);
	}

	if (strcmp(mode, str))
		return -1;

	return 0;
}

static int lcd_config_valid(struct aml_lcd_drv_s *pdrv, char *mode)
{
	if (lcd_outputmode_is_matched(pdrv, mode))
		return -1;

	return 0;
}

static void lcd_config_init(struct aml_lcd_drv_s *pdrv)
{
	lcd_enc_timing_init_config(pdrv);
	lcd_clk_generate_parameter(pdrv);
	pdrv->config.timing.clk_change = 0; /* clear clk_change flag */
}

int lcd_mode_tablet_init(struct aml_lcd_drv_s *pdrv)
{
	pdrv->list_support_mode = lcd_list_support_mode;
	pdrv->outputmode_check = lcd_outputmode_is_matched;
	pdrv->config_valid = lcd_config_valid;
	pdrv->driver_init_pre = lcd_tablet_driver_init_pre;
	pdrv->driver_init = lcd_tablet_driver_init;
	pdrv->driver_disable = lcd_tablet_driver_disable;

	lcd_config_init(pdrv);
	pdrv->probe_done = 1;

	return 0;
}


