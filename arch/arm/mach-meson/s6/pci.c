// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/types.h>
#include <asm/amlogic/arch/io.h>
#include <asm/amlogic/arch/secure_apb.h>

#define PCI_PHY_BASE				0xFE02A000
#define UPCTX_CONT0_REG2			(0x0009 << 2)
#define UPCTX_CONT1_REG2			(0x000e << 2)
#define UPCTX_DR_REG0				(0x0011 << 2)
#define UPCRX_DR_REG0				(0x0014 << 2)
#define UPCRX_DR_REG1				(0x0015 << 2)
#define REG_MISC_GEN1_REG			(0x001a << 2)
#define REG_DCHA_AFE_LPBK_REG			(0x001e << 2)
#define REG_DCHA_AFE_GEN1_REG			(0x001f << 2)
#define REG_DCHA_DFE_GEN1_REG			(0x0024 << 2)
#define REG_DCHD_CDR_GEN1_REG			(0x002e << 2)
#define REG_DCHD_EQ_GEN1_REG			(0x0033 << 2)
#define REG_DCHD_EQ_GEN2_REG			(0x0034 << 2)
#define REG_DCHD_EQ_GEN3_REG			(0x0035 << 2)

#define RESET_LEVEL_BASE			0xFE002044
void set_pci_power_on(void)
{
	writel(0xFF83FF, RESET_LEVEL_BASE);
	writel(0xFFFFFFFF, RESET_LEVEL_BASE);
}

void set_pci_power_off(void)
{
	writel(0x0, PCI_PHY_BASE + UPCTX_CONT0_REG2);
	writel(0x0, PCI_PHY_BASE + UPCTX_CONT1_REG2);
	writel(0x0, PCI_PHY_BASE + UPCTX_DR_REG0);
	writel(0x0, PCI_PHY_BASE + UPCRX_DR_REG0);
	writel(0x0, PCI_PHY_BASE + UPCRX_DR_REG1);
	writel(0x0, PCI_PHY_BASE + REG_MISC_GEN1_REG);
	writel(0x0, PCI_PHY_BASE + REG_DCHA_AFE_GEN1_REG);
	writel(0x0, PCI_PHY_BASE + REG_DCHA_DFE_GEN1_REG);
	writel(0x0, PCI_PHY_BASE + REG_DCHD_CDR_GEN1_REG);
	writel(0x0, PCI_PHY_BASE + REG_DCHD_EQ_GEN1_REG);
}
