/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SD_EMMC_H__
#define __SD_EMMC_H__

#include <mmc.h>

#define SDIO_PORT_A			0
#define SDIO_PORT_B			1
#define SDIO_PORT_C			2

#define SD_EMMC_CLKSRC_24M		24000000	/* 24 MHz */
#define SD_EMMC_CLKSRC_DIV2		1000000000	/* 1 GHz */

#define MESON_SD_EMMC_CLOCK		0x00
#define CLK_MAX_DIV			GENMASK(5, 0)
#define CLK_MAX_SRC			GENMASK(7, 6)
#define CFG_DIV				0
#define CFG_SRC				6
#define CFG_CO_PHASE			8
#define	CFG_TX_PHASE			10
#define	CFG_RX_PHASE			12
#define	CFG_SRAM_PD			14
#define	CFG_TX_DELAY			16
#define	CFG_RX_DELAY			22
#define	CFG_ALWAYS_ON			28
#define	CFG_IRQ_SDIO_SLEEP		29
#define CFG_IRQ_SDIO_SLEEP_DS		30

#define MESON_SD_EMMC_DELAY1	0x4
#define DLY_D0_MASK	GENMASK(5, 0)
#define DLY_D1_MASK	GENMASK(11, 6)
#define DLY_D2_MASK	GENMASK(17, 12)
#define DLY_D3_MASK	GENMASK(23, 18)
#define DLY_D4_MASK	GENMASK(31, 24)
#define DLY_D0	0
#define DLY_D1	6
#define DLY_D2	12
#define DLY_D3	18
#define DLY_D4	24

#define MESON_SD_EMMC_DELAY2	0x8
#define DLY_D5_MASK	GENMASK(5, 0)
#define DLY_D6_MASK	GENMASK(11, 6)
#define DLY_D7_MASK	GENMASK(17, 12)
#define DLY_D8_MASK	GENMASK(23, 18)
#define DLY_D9_MASK	GENMASK(31, 24)
#define DLY_d5	0
#define DLY_d6	6
#define DLY_d7	12
#define DLY_d8	18
#define DLY_d9	24

#define MESON_SD_EMMC_ADJUST	0xC
#define CALI_SEL_MASK		GENMASK(11, 8)
#define ADJ_DLY_MASK		GENMASK(21, 16)
#define CFG_CALI_SEL		8
#define CFG_CALI_EN		12
#define CFG_ADJ_EN		13
#define CFG_CALI_RISE		14
#define CFG_DS_EN		15
#define CFG_ADJ_DLY		16
#define CFG_ADJ_AUTO		22
#define CFG_ADJ_INIT		23

#define MESON_SD_EMMC_START		0x40
#define   CFG_DESC_INIT			BIT(0)
#define   CFG_DESC_BUSY			BIT(1)
#define   CFG_DESC_ADDR			2

#define MESON_SD_EMMC_CFG		0x44
#define   CFG_BUS_WIDTH_MASK		GENMASK(1, 0)
#define   CFG_BUS_WIDTH_1		0
#define   CFG_BUS_WIDTH_4		1
#define   CFG_BUS_WIDTH_8		2
#define   CFG_DDR			BIT(2)
#define   CFG_BL_LEN_MASK		GENMASK(7, 4)
#define   CFG_BL_LEN_SHIFT		4
#define   CFG_BL_LEN_512		(9 << 4)
#define   CFG_RESP_TIMEOUT_MASK		GENMASK(11, 8)
#define   CFG_RESP_TIMEOUT_256		(8 << 8)
#define   CFG_RC_CC_MASK		GENMASK(15, 12)
#define   CFG_RC_CC_16			(4 << 12)
#define   CFG_SDCLK_ALWAYS_ON		BIT(18)
#define   CFG_AUTO_CLK			BIT(23)

#define MESON_SD_EMMC_STATUS		0x48
#define   STATUS_MASK			GENMASK(15, 0)
#define   STATUS_ERR_MASK		GENMASK(12, 0)
#define   STATUS_RXD_ERR_MASK		GENMASK(7, 0)
#define   STATUS_TXD_ERR		BIT(8)
#define   STATUS_DESC_ERR		BIT(9)
#define   STATUS_RESP_ERR		BIT(10)
#define   STATUS_RESP_TIMEOUT		BIT(11)
#define   STATUS_DESC_TIMEOUT		BIT(12)
#define   STATUS_END_OF_CHAIN		BIT(13)
#define   STATUS_DATA_0			BIT(16)

#define MESON_SD_EMMC_IRQ_EN		0x4c
#define   CFG_CMD_SETUP			BIT(17)

#define MESON_SD_EMMC_CMD_CFG		0x50
#define   CMD_CFG_LENGTH_MASK		GENMASK(8, 0)
#define   CMD_CFG_BLOCK_MODE		BIT(9)
#define   CMD_CFG_R1B			BIT(10)
#define   CMD_CFG_END_OF_CHAIN		BIT(11)
#define   CMD_CFG_TIMEOUT_4S		(12 << 12)
#define   CMD_CFG_NO_RESP		BIT(16)
#define   CMD_CFG_NO_CMD		BIT(17)
#define   CMD_CFG_DATA_IO		BIT(18)
#define   CMD_CFG_DATA_WR		BIT(19)
#define   CMD_CFG_RESP_NOCRC		BIT(20)
#define   CMD_CFG_RESP_128		BIT(21)
#define   CMD_CFG_RESP_NUM		BIT(22)
#define   CMD_CFG_DATA_NUM		BIT(23)
#define   CMD_CFG_CMD_INDEX_MASK	GENMASK(29, 24)
#define   CMD_CFG_CMD_INDEX_SHIFT	24
#define   CMD_CFG_ERR			BIT(30)
#define   CMD_CFG_OWNER			BIT(31)

#define MESON_SD_EMMC_CMD_ARG		0x54
#define MESON_SD_EMMC_CMD_DAT		0x58
#define MESON_SD_EMMC_CMD_RSP		0x5c
#define MESON_SD_EMMC_CMD_RSP1		0x60
#define MESON_SD_EMMC_CMD_RSP2		0x64
#define MESON_SD_EMMC_CMD_RSP3		0x68
#define MESON_SD_EMMC_CURR_CFG		0x70
#define MESON_SD_EMMC_NEXT_CFG		0x80

#define CQHCI_RMEM			0x150

#define SD_EMMC_RXD_ERROR               (1 << 0)
#define SD_EMMC_TXD_ERROR               (1 << 1)
#define SD_EMMC_DESC_ERROR              (1 << 2)
#define SD_EMMC_RESP_CRC_ERROR          (1 << 3)
#define SD_EMMC_RESP_TIMEOUT_ERROR      (1 << 4)
#define SD_EMMC_DESC_TIMEOUT_ERROR      (1 << 5)

//boot from sd or emmc
/*this macro of SEC_AO_SEC_GP_CFG0 was defined in ./arch/arm/include/asm/arch-s4/soc_def.h */
//#define SEC_AO_SEC_GP_CFG0      (SYSCTRL_SEC_STATUS_REG4)
#endif
