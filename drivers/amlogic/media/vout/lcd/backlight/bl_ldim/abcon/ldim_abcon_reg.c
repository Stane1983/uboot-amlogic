// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/aml_bl.h>
#include <amlogic/media/vout/lcd/bl_ldim.h>
#include "../../../lcd_common.h"
#include "../../../lcd_reg.h"
#include "../../lcd_bl.h"
#include "../ldim_drv.h"
#include "../ldim_dev_drv.h"
#include "env.h"
#include "ldim_abcon.h"
#include "ldim_abcon_reg.h"

void abcon_wr_reg(unsigned int addr, unsigned int val)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_REG)
		printf("%s: [0x%08x]=0x%08x\n", __func__, addr, val);
	(*(volatile unsigned int *)ABCON_REG_ADDR_VCBUS(addr)) = (val);
};

unsigned int abcon_rd_reg(unsigned int addr)
{
	unsigned int val;

	val = (*(volatile unsigned int *)ABCON_REG_ADDR_VCBUS(addr));
	if (lcd_debug_print_flag & LCD_DBG_PR_REG)
		printf("%s: [0x%08x]=0x%08x\n", __func__, addr, val);

	return val;
};

void abcon_wr_reg_bits(unsigned int addr, unsigned int val,
			unsigned int start, unsigned int len)
{
	unsigned int temp = 0;

	temp = abcon_rd_reg(addr);
	temp = (temp & (~(((1L << len) - 1) << start))) |
		((val & ((1L << len) - 1)) << start);
	abcon_wr_reg(addr, temp);
};

unsigned int abcon_rd_reg_bits(unsigned int addr,
				unsigned int start, unsigned int len)
{
	unsigned int temp = 0;

	temp = abcon_rd_reg(addr);
	temp = (temp >> start) & ((1L << len) - 1);

	return temp;
};

