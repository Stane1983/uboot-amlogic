/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DT_BINDINGS_CLOCK_S1A_H
#define __DT_BINDINGS_CLOCK_S1A_H
/*
 * CLKID index values
 */
#define CLKID_XTAL		0
#define CLKID_FIXED_PLL         1
#define CLKID_FCLK_DIV2         2
#define CLKID_FCLK_DIV3         3
#define CLKID_FCLK_DIV5         4
#define CLKID_FCLK_DIV7         5
#define CLKID_FCLK_DIV2P5       6
#define CLKID_FCLK_DIV4       	7
#define CLKID_SYS_CLK		8
#define CLKID_SYS_PLL		9
#define CLKID_GP0_PLL		10

#define CLKID_GATE_BASE		11
#define CLKID_SYS_SAR_ADC	(CLKID_GATE_BASE + 0)
#define CLKID_SAR_ADC_GATE	(CLKID_GATE_BASE + 1)
#define CLKID_SYS_SD_EMMC_C	(CLKID_GATE_BASE + 2)
#define CLKID_SD_EMMC_C_GATE	(CLKID_GATE_BASE + 3)
#define CLKID_SYS_ETH_PHY	(CLKID_GATE_BASE + 4)
#define CLKID_SYS_ETH_MAC	(CLKID_GATE_BASE + 5)
#define CLKID_ETH_125M_GATE	(CLKID_GATE_BASE + 6)
#define CLKID_ETH_RMII_GATE	(CLKID_GATE_BASE + 7)

#define CLKID_MUX_BASE		(CLKID_GATE_BASE + 8)
#define CLKID_SARADC_MUX	(CLKID_MUX_BASE + 0)
#define CLKID_SD_EMMC_C_MUX	(CLKID_MUX_BASE + 1)

#define CLKID_DIV_BASE		(CLKID_MUX_BASE + 2)
#define CLKID_SARADC_DIV	(CLKID_DIV_BASE + 1)
#define CLKID_SD_EMMC_C_DIV	(CLKID_DIV_BASE + 2)
#define CLKID_ETH_RMII_DIV	(CLKID_DIV_BASE + 3)

#define CLKID_UNREALIZED	100

#endif /* __S1A_CLKC_H */
