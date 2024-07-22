// SPDX-License-Identifier: GPL-2.0
/*
 * Amlogic Meson Reset Controller driver
 *
 * Copyright (c) 2018 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <malloc.h>
#include <reset-uclass.h>
#include <regmap.h>
#include <linux/bitops.h>

#define REG_COUNT	8
#define BITS_PER_REG	32
#define LEVEL_OFFSET	0x7c

#if defined(CONFIG_AMLOGIC_MODIFY)
struct meson_reset_data {
	uint level_offset;
};
#endif

struct meson_reset_priv {
	struct regmap *regmap;
#if defined(CONFIG_AMLOGIC_MODIFY)
	struct meson_reset_data *data;
#endif
};

static int meson_reset_request(struct reset_ctl *reset_ctl)
{
	if (reset_ctl->id > (REG_COUNT * BITS_PER_REG))
		return -EINVAL;

	return 0;
}

static int meson_reset_level(struct reset_ctl *reset_ctl, bool assert)
{
	struct meson_reset_priv *priv = dev_get_priv(reset_ctl->dev);
	uint bank = reset_ctl->id / BITS_PER_REG;
	uint offset = reset_ctl->id % BITS_PER_REG;
	uint reg_offset = LEVEL_OFFSET + (bank << 2);
	uint val;

#if defined(CONFIG_AMLOGIC_MODIFY)
	if (priv->data)
		reg_offset = priv->data->level_offset + (bank << 2);
#endif

	regmap_read(priv->regmap, reg_offset, &val);
	if (assert)
		val &= ~BIT(offset);
	else
		val |= BIT(offset);
	regmap_write(priv->regmap, reg_offset, val);

	return 0;
}

static int meson_reset_assert(struct reset_ctl *reset_ctl)
{
	return meson_reset_level(reset_ctl, true);
}

static int meson_reset_deassert(struct reset_ctl *reset_ctl)
{
	return meson_reset_level(reset_ctl, false);
}

struct reset_ops meson_reset_ops = {
	.request = meson_reset_request,
	.rst_assert = meson_reset_assert,
	.rst_deassert = meson_reset_deassert,
};

#if defined(CONFIG_AMLOGIC_MODIFY)
struct meson_reset_data meson_reset_s7d_data = {
	.level_offset = 0x40,
};
#endif

static const struct udevice_id meson_reset_ids[] = {
	{ .compatible = "amlogic,meson-gxbb-reset" },
	{ .compatible = "amlogic,meson-axg-reset" },
#if defined(CONFIG_AMLOGIC_MODIFY)
	{
		.compatible = "amlogic,meson-s7d-reset",
		.data = (ulong)&meson_reset_s7d_data,
	},
#endif
	{ }
};

static int meson_reset_probe(struct udevice *dev)
{
	struct meson_reset_priv *priv = dev_get_priv(dev);

	return regmap_init_mem(dev_ofnode(dev), &priv->regmap);
}

#if defined(CONFIG_AMLOGIC_MODIFY)
int meson_reset_of_to_plat(struct udevice *dev)
{
	struct meson_reset_priv *priv = dev_get_priv(dev);

	priv->data = (struct meson_reset_data *)dev_get_driver_data(dev);

	return 0;
}
#endif

U_BOOT_DRIVER(meson_reset) = {
	.name = "meson_reset",
	.id = UCLASS_RESET,
	.of_match = meson_reset_ids,
	.probe = meson_reset_probe,
#if defined(CONFIG_AMLOGIC_MODIFY)
	.of_to_plat = meson_reset_of_to_plat,
#endif
	.ops = &meson_reset_ops,
	.priv_auto	= sizeof(struct meson_reset_priv),
};
