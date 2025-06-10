/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LDIM_ABCON_REG_H__
#define __LDIM_ABCON_REG_H__

/* CLKCTRL_BCON_CLK_CNTL
 * [5:0] clk_div
 * [6] clk_en
 * [9:7] clk_sel
 */
#define CLKCTRL_BCON_CLK_CNTL ((0x0088  << 2) + 0xfe000000)	//0xfe000220

//gpio pinux
#define PIN_MUX_REG9 0xfe004024	//bcon 0-1 func7 GPIOH_22--23
#define PIN_MUX_REGA 0xfe004028	//bcon 2--7 func7 GPIOH_24--29
#define PIN_MUX_REG8 0xfe004020	//bcon 8 func7 GPIOH_13
#define PIN_MUX_REG4 0xfe004010	//bcon 9 func6 GPIOZ_1
#define PIN_MUX_REG6 0xfe004018	//bcon 10 func5 GPIOZ_19 bcon 11 func6 GPIOZ_18

#define GPIOH_PULL_EN 0xfe00412c	//set pull en
#define GPIOH_PULL_UP 0xfe004130	//set pull up

//bcon base addr
#define BCON_APB_BASE_ADDR 0xfe014000UL
#define BCON_APB_BASE_ADDR_END 0xfe015fffUL

#define ABCON_REG_OFFSET(reg)                   (((reg) << 2))
#define ABCON_REG_OFFSET_BYTE(reg)              ((reg))

#define ABCON_REG_ADDR_VCBUS(reg)              ((reg > 0x10000) ? (reg + 0L) : \
					(BCON_APB_BASE_ADDR + ABCON_REG_OFFSET(reg)))

void abcon_wr_reg(unsigned int addr, unsigned int val);
unsigned int abcon_rd_reg(unsigned int addr);
void abcon_wr_reg_bits(unsigned int addr, unsigned int val,
			unsigned int start, unsigned int len);
unsigned int abcon_rd_reg_bits(unsigned int addr,
				unsigned int start, unsigned int len);
#endif//__LDIM_ABCON_REG_H__

