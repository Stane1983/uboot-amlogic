/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __ARCH_ARM_MESON_ETH_H_U_BOOT__
#define __ARCH_ARM_MESON_ETH_H_U_BOOT__

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <errno.h>
#include <log.h>
#include <miiphy.h>
#include <malloc.h>
#include <net.h>
#include <pci.h>
#include <reset.h>
#include <asm/cache.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <dm/devres.h>
#include <dm/lists.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <asm/io.h>


#if defined(CONFIG_AMLOGIC_ETH)
#ifdef CONFIG_DM_ETH
void __iomem *DM_network_interface_setup(struct udevice *dev);
void DM_network_interface_setup_final(struct phy_device *phydev);
void __iomem *DM_network_interface_remove(void);
#endif
#endif

#endif
