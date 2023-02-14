// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <generic-phy.h>
#include <usb.h>
#include <dwc3-uboot.h>
#include <dm/devres.h>

#include <usb/xhci.h>
#include <asm/io.h>
#include <linux/usb/dwc3.h>
#include <linux/usb/otg.h>
#include <asm/amlogic/arch/usb.h>

struct xhci_crg_platdata {
    struct phy_bulk usb_phys;
};

unsigned int m31_phys;

void crg_set_mode(struct xhci_hccr *hccr, u32 mode)
{
	u64 tmp;

	if (mode == USB_DR_MODE_HOST) {
		/* set controller host role*/
		tmp = readl((u64)hccr + 0x20FC) & ~0x1;
		writel(tmp, (u64)hccr + 0x20FC);
	}
}


#if CONFIG_IS_ENABLED(DM_USB)

void xhci_crg_phy_tuning_1(struct udevice *dev, int port)
{
	/* nothing */
}

static int xhci_crg_setup_phy(struct udevice *dev, struct phy_bulk *phys)
{
    int ret;

	ret = generic_phy_get_bulk(dev, phys);
	if (ret)
		return ret;

	ret = generic_phy_init_bulk(phys);
	if (ret)
		return ret;

	ret = generic_phy_power_on_bulk(phys);
	if (ret)
		generic_phy_exit_bulk(phys);

	return ret;
}

static int xhci_crg_probe(struct udevice *dev)
{
	struct xhci_hcor *hcor;
	struct xhci_hccr *hccr;
	enum usb_dr_mode dr_mode;
	struct xhci_crg_platdata *plat = dev_get_plat(dev);
	int ret;

#ifdef CONFIG_AMLOGIC_USB
	if (dev_read_prop(dev, "m31", NULL)) {
		printf("M31 PHY\n");
#ifdef AML_USB_M31_PHY_ONLY
		m31_phy_init(m31_phys);
#endif
		m31_phys++;
	}

	ret = xhci_crg_setup_phy(dev, &plat->usb_phys);
	if (ret)
		return ret;

	hccr = (struct xhci_hccr *)((uintptr_t)dev_read_addr(dev));
	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));
#else
	hccr = (struct xhci_hccr *)((uintptr_t)dev_read_addr(dev));
	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	ret = dwc3_setup_phy(dev, &plat->usb_phys);
		if (ret && (ret != -ENOTSUPP))
			return ret;
#endif

	dr_mode = usb_get_dr_mode(dev_ofnode(dev));
	if (dr_mode == USB_DR_MODE_UNKNOWN)
		/* by default set dual role mode to HOST */
		dr_mode = USB_DR_MODE_HOST;

	crg_set_mode(hccr, dr_mode);

    return xhci_register(dev, hccr, hcor);

/*#ifndef CONFIG_AMLOGIC_USB
	return xhci_register(dev, hccr, hcor);
#else
	return xhci_register(dev, hccr, hcor, plat->usbportnum);
#endif
*
*/
}

int crg_shutdown_phy(struct udevice *dev, struct phy_bulk *phys)
{
	int ret;

	ret = generic_phy_power_off_bulk(phys);
	ret |= generic_phy_exit_bulk(phys);
	return ret;
}

static int xhci_crg_remove(struct udevice *dev)
{
	struct xhci_crg_platdata *plat = dev_get_plat(dev);

	crg_shutdown_phy(dev, &plat->usb_phys);

	return xhci_deregister(dev);
}

static const struct udevice_id xhci_crg_ids[] = {
	{ .compatible = "crg-xhci" },
	{ }
};

U_BOOT_DRIVER(xhci_crg) = {
	.name = "xhci-crg",
	.id = UCLASS_USB,
	.of_match = xhci_crg_ids,
	.probe = xhci_crg_probe,
	.remove = xhci_crg_remove,
	.ops = &xhci_usb_ops,
	.priv_auto = sizeof(struct xhci_ctrl),
	.plat_auto = sizeof(struct xhci_crg_platdata),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
#endif
