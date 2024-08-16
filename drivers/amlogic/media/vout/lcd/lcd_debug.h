/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_DEBUG_H
#define _AML_LCD_DEBUG_H
#include <amlogic/media/vout/lcd/lcd_vout.h>
#include "lcd_reg.h"

#define LCD_DEBUG_REG_CNT_MAX    30
#define LCD_DEBUG_REG_END        0xffffffff

struct lcd_debug_info_s {
	unsigned int *reg_pinmux_table;

	void (*reg_dump_lvds)(struct aml_lcd_drv_s *pdrv);
	void (*reg_dump_vbyone)(struct aml_lcd_drv_s *pdrv);
#ifdef CONFIG_AML_LCD_TABLET
	void (*reg_dump_mipi)(struct aml_lcd_drv_s *pdrv);
	void (*reg_dump_edp)(struct aml_lcd_drv_s *pdrv);
#endif
#ifdef CONFIG_AML_LCD_TCON
	void (*reg_dump_mlvds)(struct aml_lcd_drv_s *pdrv);
	void (*reg_dump_p2p)(struct aml_lcd_drv_s *pdrv);
#endif

	void (*interface_print)(struct aml_lcd_drv_s *pdrv);
	void (*reg_dump_interface)(struct aml_lcd_drv_s *pdrv);
};

struct reg_info_t {
	char *name;
	unsigned int reg_offst;
};

#define MK_REG_INFO(REG_NAME)({.name = #REG_NAME, .reg_offst = REG_NAME})

static unsigned int lcd_reg_dump_pinmux_txdh2[] = {
	PERIPHS_PIN_MUX_5,
	PERIPHS_PIN_MUX_6,
	LCD_DEBUG_REG_END
};

static unsigned int lcd_reg_dump_pinmux_t3[] = {
	PADCTRL_PIN_MUX_REG7,
	PADCTRL_PIN_MUX_REG8,
	LCD_DEBUG_REG_END
};

static unsigned int lcd_reg_dump_pinmux_c3[] = {
	PADCTRL_PIN_MUX_REG0,
	PADCTRL_PIN_MUX_REG1,
	PADCTRL_PIN_MUX_REG3,
	PADCTRL_PIN_MUX_REG4,
	PADCTRL_PIN_MUX_REGB,
	PADCTRL_PIN_MUX_REGJ,
	PADCTRL_PIN_MUX_REGK,
	LCD_DEBUG_REG_END
};

#endif

