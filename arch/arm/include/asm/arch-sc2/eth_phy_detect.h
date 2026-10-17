/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * SC2 (S905X4 / S905C2 / ...) external vs internal Ethernet PHY detection
 */

#ifndef __ARCH_SC2_ETH_PHY_DETECT_H__
#define __ARCH_SC2_ETH_PHY_DETECT_H__

#include <linux/types.h>

/*
 * Probe the external MDIO pads for a PHY.
 *
 * Returns 1 if an external PHY answers, 0 otherwise (use internal PHY).
 * Result is cached; also sets env "eth_phy" to "external" or "internal".
 * Can be forced with: setenv eth_phy_force internal|external
 */
int sc2_has_external_phy(void);

/*
 * Details of the detected external PHY (valid only when
 * sc2_has_external_phy() returned 1). Returns 0 on success.
 */
int sc2_external_phy_info(u32 *phy_id, int *phy_addr);

/*
 * Board hook: name of the GPIO wired to the external PHY reset
 * (e.g. "GPIOZ_15"), or NULL to skip toggling reset during the probe.
 * Default (weak) implementation returns "GPIOZ_15", as in meson-sc2.dtsi.
 * Override it in your board file if that pin is used for something else.
 */
const char *sc2_eth_phy_reset_gpio(void);

#endif /* __ARCH_SC2_ETH_PHY_DETECT_H__ */
