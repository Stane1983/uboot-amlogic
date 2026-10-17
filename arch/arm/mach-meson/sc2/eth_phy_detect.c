// SPDX-License-Identifier: GPL-2.0+
/*
 * SC2 external vs internal Ethernet PHY detection
 *
 * The U-Boot ethernet driver (drivers/net/designware.c) does not detect
 * anything; it trusts "internal_phy" from the built-in U-Boot DTB. This
 * probes the hardware directly, so board code (e.g. checkhw()) can choose
 * the kernel DTB before the ethernet driver runs:
 *
 *   1. power up the ETH domain (same SMC call the driver uses)
 *   2. route the MAC's MDIO to the external pads
 *      (ETH_PHY_CNTL2 = 0, as setup_external_phy() does)
 *   3. mux GPIOZ_0/GPIOZ_1 to eth_mdio/eth_mdc (function 1)
 *   4. pulse the external PHY reset (board-overridable GPIO)
 *   5. read PHY ID regs 2/3 on MDIO addresses 0..31 via the
 *      DesignWare GMAC MII address/data registers
 *   6. restore everything that was touched
 *
 * With CNTL2 = 0 the internal PHY (0181:4400 @ addr 8) is not on the bus,
 * so anything that answers is a real external chip.
 */

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <asm/arch/pwr_ctrl.h>
#include <asm/arch/register.h>
#include <asm/arch/eth_phy_detect.h>

/* arch/arm/dts/meson-sc2.dtsi: &ethmac reg = eth_base, eth_top, eth_cfg */
#define SC2_ETH_BASE		0xfdc00000UL
#define SC2_ETH_TOP		0xfe024000UL	/* ETHTOP_CNTL0 (mc_val) */
#define SC2_ETH_CFG		0xfe028000UL
#define SC2_ETH_PHY_CNTL2	(SC2_ETH_CFG + 0x88)

/* DesignWare GMAC MII registers (drivers/net/designware.h) */
#define SC2_GMAC_MIIADDR	(SC2_ETH_BASE + 0x10)
#define SC2_GMAC_MIIDATA	(SC2_ETH_BASE + 0x14)
#define SC2_MII_BUSY		(1 << 0)
#define SC2_MII_CLK_150_250M	(0x10)		/* same CR the driver uses */

/* pinctrl-meson-sc2.c: BANK_PMX("Z", GPIOZ_0..15, reg 0x6), 4 bits/pin */
#define SC2_PMX_Z_0_7		PADCTRL_PIN_MUX_REG6
#define SC2_PMX_MDIO_MDC_MASK	0xff		/* GPIOZ_0, GPIOZ_1 */
#define SC2_PMX_MDIO_MDC_ETH	0x11		/* function 1 = eth */

#define SC2_INT_PHY_ID1		0x0181
#define SC2_INT_PHY_ID2		0x4400

static int sc2_ext_phy = -1;	/* -1 = not probed yet */
static u32 sc2_ext_phy_id;
static int sc2_ext_phy_addr = -1;

__weak const char *sc2_eth_phy_reset_gpio(void)
{
	return "GPIOZ_15";
}

static int sc2_mdio_read(int addr, int reg, u16 *val)
{
	int timeout = 10000;

	writel(((addr & 0x1f) << 11) | ((reg & 0x1f) << 6) |
	       SC2_MII_CLK_150_250M | SC2_MII_BUSY,
	       (void *)SC2_GMAC_MIIADDR);

	while (readl((void *)SC2_GMAC_MIIADDR) & SC2_MII_BUSY) {
		if (--timeout == 0)
			return -ETIMEDOUT;
		udelay(1);
	}
	*val = readl((void *)SC2_GMAC_MIIDATA) & 0xffff;
	return 0;
}

static void sc2_ext_phy_reset(void)
{
	const char *name = sc2_eth_phy_reset_gpio();
	unsigned int gpio;
	int ret;

	if (!name)
		return;

	ret = gpio_lookup_name(name, NULL, NULL, &gpio);
	if (ret) {
		printf("sc2 phy-detect: can't find %s (%d)\n", name, ret);
		return;
	}
	if (gpio_request(gpio, "sc2_eth_phy_rst"))
		return;

	gpio_direction_output(gpio, 0);	/* assert reset (nRESET low) */
	mdelay(20);
	gpio_direction_output(gpio, 1);	/* release */
	gpio_free(gpio);

	/* e.g. RTL8211F needs ~50ms after reset before MDIO is reliable */
	mdelay(100);
}

static int sc2_probe_external_phy(u32 *out_id, int *out_addr)
{
	u32 saved_cntl2, saved_top, saved_pmx;
	int addr, found = 0;
	u16 id1, id2;

	pwr_ctrl_psci_smc(PM_ETH, 1);
	udelay(100);

	saved_cntl2 = readl((void *)SC2_ETH_PHY_CNTL2);
	saved_top   = readl((void *)SC2_ETH_TOP);
	saved_pmx   = readl((void *)SC2_PMX_Z_0_7);

	/* MDIO/MDC -> external pads */
	writel(0x0, (void *)SC2_ETH_PHY_CNTL2);

	/* GPIOZ_0 = eth_mdio, GPIOZ_1 = eth_mdc */
	writel((saved_pmx & ~SC2_PMX_MDIO_MDC_MASK) | SC2_PMX_MDIO_MDC_ETH,
	       (void *)SC2_PMX_Z_0_7);

	sc2_ext_phy_reset();

	for (addr = 0; addr < 32; addr++) {
		if (sc2_mdio_read(addr, 2, &id1) ||
		    sc2_mdio_read(addr, 3, &id2)) {
			printf("sc2 phy-detect: MDIO timeout\n");
			break;
		}

		if ((id1 == 0xffff && id2 == 0xffff) ||
		    (id1 == 0x0000 && id2 == 0x0000))
			continue;

		/* never count the Amlogic internal PHY */
		if (id1 == SC2_INT_PHY_ID1 &&
		    (id2 & 0xfff0) == SC2_INT_PHY_ID2)
			continue;

		*out_id = ((u32)id1 << 16) | id2;
		*out_addr = addr;
		found = 1;
		break;
	}

	/* restore so the real driver starts from a clean state */
	writel(saved_pmx,   (void *)SC2_PMX_Z_0_7);
	writel(saved_top,   (void *)SC2_ETH_TOP);
	writel(saved_cntl2, (void *)SC2_ETH_PHY_CNTL2);

	return found;
}

int sc2_has_external_phy(void)
{
	char *force;

	if (sc2_ext_phy >= 0)
		return sc2_ext_phy;

	force = env_get("eth_phy_force");
	if (force && !strcmp(force, "external")) {
		sc2_ext_phy = 1;
	} else if (force && !strcmp(force, "internal")) {
		sc2_ext_phy = 0;
	} else {
		sc2_ext_phy = sc2_probe_external_phy(&sc2_ext_phy_id,
						     &sc2_ext_phy_addr);
		if (sc2_ext_phy)
			printf("eth: external PHY id %08x at addr %d\n",
			       sc2_ext_phy_id, sc2_ext_phy_addr);
		else
			printf("eth: no external PHY, using internal\n");
	}

	env_set("eth_phy", sc2_ext_phy ? "external" : "internal");
	return sc2_ext_phy;
}

int sc2_external_phy_info(u32 *phy_id, int *phy_addr)
{
	if (sc2_has_external_phy() != 1 || sc2_ext_phy_addr < 0)
		return -ENODEV;

	if (phy_id)
		*phy_id = sc2_ext_phy_id;
	if (phy_addr)
		*phy_addr = sc2_ext_phy_addr;
	return 0;
}
