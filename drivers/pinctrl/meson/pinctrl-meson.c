// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016 - Beniamino Galvani <b.galvani@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <malloc.h>
#include <asm/global_data.h>
#include <dm/device-internal.h>
#include <dm/device_compat.h>
#include <dm/lists.h>
#include <dm/pinctrl.h>
#include <fdt_support.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/libfdt.h>
#include <linux/sizes.h>
#include <asm/gpio.h>

#include "pinctrl-meson.h"
#if defined(CONFIG_AMLOGIC_MODIFY)
#include "pinctrl-meson-axg.h"
#endif

#ifdef CONFIG_ARMV8_MULTIENTRY
#define pinctrl_smp_lock() {		\
	if (gd->flags & GD_FLG_SMP)	\
		spin_lock(&priv->lock);	\
	}

#define pinctrl_smp_unlock() {		\
	if (gd->flags & GD_FLG_SMP)	\
		spin_unlock(&priv->lock);\
	}
#else
#define pinctrl_smp_lock() {}
#define pinctrl_smp_unlock() {}
#endif

DECLARE_GLOBAL_DATA_PTR;

static const char *meson_pinctrl_dummy_name = "_dummy";

static char pin_name[PINNAME_SIZE];

int meson_pinctrl_get_groups_count(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->num_groups;
}

const char *meson_pinctrl_get_group_name(struct udevice *dev,
					 unsigned int selector)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	if (!priv->data->groups[selector].name)
		return meson_pinctrl_dummy_name;

	return priv->data->groups[selector].name;
}

int meson_pinctrl_get_pins_count(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->num_pins;
}

const char *meson_pinctrl_get_pin_name(struct udevice *dev,
				       unsigned int selector)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
#if defined(CONFIG_AMLOGIC_MODIFY)
	int i;
	struct meson_axg_pmx_data *pmx = priv->data->pmx_data;
	struct meson_pmx_bank *bank = NULL;
	unsigned int pin;

	pin = selector + priv->data->pin_base;

	for (i = 0; i < pmx->num_pmx_banks; i++) {
		if (pin >= pmx->pmx_banks[i].first &&
		    pin <= pmx->pmx_banks[i].last) {
			bank = &pmx->pmx_banks[i];
			break;
		}
	}

	if (i == pmx->num_pmx_banks)
		snprintf(pin_name, PINNAME_SIZE, "Error");
	else if (bank)
		snprintf(pin_name, PINNAME_SIZE, "GPIO%s_%d",
			 bank->name, pin - bank->first + bank->first_num);
#else
	if (selector > priv->data->num_pins ||
	    selector > priv->data->funcs[0].num_groups)
		snprintf(pin_name, PINNAME_SIZE, "Error");
	else
		snprintf(pin_name, PINNAME_SIZE, "%s",
			 priv->data->funcs[0].groups[selector]);
#endif

	return pin_name;
}

int meson_pinmux_get_functions_count(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->num_funcs;
}

const char *meson_pinmux_get_function_name(struct udevice *dev,
					   unsigned int selector)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);

	return priv->data->funcs[selector].name;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static int meson_pinconf_calc_reg_and_bit(struct udevice *dev, unsigned int offset,
					  enum meson_reg_type reg_type,
					  unsigned int *reg, unsigned int *bit)
#else
static int meson_gpio_calc_reg_and_bit(struct udevice *dev, unsigned int offset,
				       enum meson_reg_type reg_type,
				       unsigned int *reg, unsigned int *bit)
#endif
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct meson_bank *bank = NULL;
	struct meson_reg_desc *desc;
	unsigned int pin;
	int i;

	pin = priv->data->pin_base + offset;

	for (i = 0; i < priv->data->num_banks; i++) {
		if (pin >= priv->data->banks[i].first &&
		    pin <= priv->data->banks[i].last) {
			bank = &priv->data->banks[i];
			break;
		}
	}

	if (!bank)
		return -EINVAL;

	desc = &bank->regs[reg_type];
	*reg = desc->reg * 4;
	*bit = desc->bit + pin - bank->first;

	return 0;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static int meson_gpio_calc_reg_and_bit(struct udevice *dev, unsigned int offset,
				       enum meson_reg_type reg_type,
				       unsigned int *reg, unsigned int *bit)
{
	struct meson_bank *bank = dev_get_priv(dev);
	struct meson_reg_desc *desc = &bank->regs[reg_type];

	*reg = desc->reg << 2;
	*bit = desc->bit + offset;

	return 0;
}
#endif

int meson_gpio_get(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	unsigned int reg, bit;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_IN, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_IN, &reg,
					  &bit);
#endif
	if (ret)
		return ret;

	pinctrl_smp_lock();
	ret =  !!(readl(priv->reg_gpio + reg) & BIT(bit));
	pinctrl_smp_unlock();

	return ret;
}

int meson_gpio_set(struct udevice *dev, unsigned int offset, int value)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	unsigned int reg, bit;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_OUT, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_OUT, &reg,
					  &bit);
#endif
	if (ret)
		return ret;

	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), value ? BIT(bit) : 0);
	pinctrl_smp_unlock();

	return 0;
}

int meson_gpio_get_direction(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	unsigned int reg, bit, val;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_DIR, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_DIR, &reg,
					  &bit);
#endif
	if (ret)
		return ret;

	pinctrl_smp_lock();
	val = readl(priv->reg_gpio + reg);
	pinctrl_smp_unlock();

	return (val & BIT(bit)) ? GPIOF_INPUT : GPIOF_OUTPUT;
}

int meson_gpio_direction_input(struct udevice *dev, unsigned int offset)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	unsigned int reg, bit;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_DIR, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_DIR, &reg,
					  &bit);
#endif
	if (ret)
		return ret;

	pinctrl_smp_lock();
	setbits_le32(priv->reg_gpio + reg, BIT(bit));
	pinctrl_smp_unlock();

	return 0;
}

int meson_gpio_direction_output(struct udevice *dev,
				unsigned int offset, int value)
{
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
	unsigned int reg, bit;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_OUT, &reg, &bit);
	if (ret)
		return ret;

	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), value ? BIT(bit) : 0);
	pinctrl_smp_unlock();

	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_DIR, &reg, &bit);
	if (ret)
		return ret;

	pinctrl_smp_lock();
	clrbits_le32(priv->reg_gpio + reg, BIT(bit));
	pinctrl_smp_unlock();
#else
	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_DIR, &reg,
					  &bit);
	if (ret)
		return ret;

	clrbits_le32(priv->reg_gpio + reg, BIT(bit));

	ret = meson_gpio_calc_reg_and_bit(dev->parent, offset, REG_OUT, &reg,
					  &bit);
	if (ret)
		return ret;

	clrsetbits_le32(priv->reg_gpio + reg, BIT(bit), value ? BIT(bit) : 0);
#endif
	return 0;
}

static int meson_pinconf_bias_set(struct udevice *dev, unsigned int pin,
				  unsigned int param)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	unsigned int offset = pin - priv->data->pin_base;
	unsigned int reg, bit;
	int ret;

#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_pinconf_calc_reg_and_bit(dev, offset, REG_PULLEN, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_PULLEN, &reg, &bit);
#endif
	if (ret)
		return ret;

	if (param == PIN_CONFIG_BIAS_DISABLE) {
		pinctrl_smp_lock();
		clrsetbits_le32(priv->reg_pullen + reg, BIT(bit), 0);
		pinctrl_smp_unlock();
		return 0;
	}

	/* othewise, enable the bias and select level */
	clrsetbits_le32(priv->reg_pullen + reg, BIT(bit), BIT(bit));
#if defined(CONFIG_AMLOGIC_MODIFY)
	ret = meson_pinconf_calc_reg_and_bit(dev, offset, REG_PULL, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_PULL, &reg, &bit);
#endif
	if (ret)
		return ret;
	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_pull + reg, BIT(bit),
			(param == PIN_CONFIG_BIAS_PULL_UP ? BIT(bit) : 0));
	pinctrl_smp_unlock();

	return 0;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static struct meson_bank *meson_pinconf_find_bank(struct meson_pinctrl *priv,
						  unsigned int pin)
{
	struct meson_bank *bank = NULL;
	int i;

	for (i = 0; i < priv->data->num_banks; i++) {
		if (pin >= priv->data->banks[i].first &&
		    pin <= priv->data->banks[i].last) {
			bank = &priv->data->banks[i];
			break;
		}
	}

	return bank;
}
#endif

static int meson_pinconf_drive_strength_set(struct udevice *dev,
					    unsigned int pin,
					    unsigned int drive_strength_ua)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	unsigned int offset = pin - priv->data->pin_base;
	unsigned int reg, bit;
	unsigned int ds_val;
	int ret;
#if defined(CONFIG_AMLOGIC_MODIFY)
	enum meson_reg_type reg_type;
	struct meson_bank *bank = meson_pinconf_find_bank(priv, pin);
#endif

	if (!priv->reg_ds) {
		dev_err(dev, "drive-strength-microamp not supported\n");
		return -ENOTSUPP;
	}

#if defined(CONFIG_AMLOGIC_MODIFY)
	if (bank && bank->ds_div > 0 && pin >= bank->ds_div) {
		reg_type = REG_DS_EX;
		offset -= bank->ds_div - bank->first;
		debug("use ds1 register, offset: %u\n", offset);
	} else {
		reg_type = REG_DS;
		debug("use ds0 register, offset: %u\n", offset);
	}
	ret = meson_pinconf_calc_reg_and_bit(dev, offset, reg_type, &reg, &bit);
#else
	ret = meson_gpio_calc_reg_and_bit(dev, offset, REG_DS, &reg, &bit);
#endif
	if (ret)
		return ret;

	bit = bit << 1;

#if defined(CONFIG_AMLOGIC_MODIFY)
	/* Consider cases with bits greater than 32 */
	reg += (bit >> 5) << 2;
	bit &= 0x1f;
#endif

	if (drive_strength_ua <= 500) {
		ds_val = MESON_PINCONF_DRV_500UA;
	} else if (drive_strength_ua <= 2500) {
		ds_val = MESON_PINCONF_DRV_2500UA;
	} else if (drive_strength_ua <= 3000) {
		ds_val = MESON_PINCONF_DRV_3000UA;
	} else if (drive_strength_ua <= 4000) {
		ds_val = MESON_PINCONF_DRV_4000UA;
	} else {
		dev_warn(dev,
			 "pin %u: invalid drive-strength-microamp : %d , default to 4mA\n",
			 pin, drive_strength_ua);
		ds_val = MESON_PINCONF_DRV_4000UA;
	}

	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_ds + reg, 0x3 << bit, ds_val << bit);
	pinctrl_smp_unlock();

	return 0;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static int meson_pinconf_input_enable(struct udevice *dev, unsigned int pin,
				      unsigned int param)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	unsigned int offset = pin - priv->data->pin_base;
	unsigned int reg, bit;
	int ret;

	debug("pin %u: %s input\n", offset, param ? "enable" : "disable");

	ret = meson_pinconf_calc_reg_and_bit(dev, offset, REG_DIR, &reg, &bit);
	if (ret)
		return ret;

	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_gpio + reg, 0x1 << bit, param << bit);
	pinctrl_smp_unlock();

	return 0;
}

static int meson_pinconf_output_set(struct udevice *dev, unsigned int pin,
				    unsigned int param)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	unsigned int offset = pin - priv->data->pin_base;
	unsigned int reg, bit;
	int ret;

	debug("pin %u: output %s\n", offset, param ? "high" : "low");

	ret = meson_pinconf_calc_reg_and_bit(dev, offset, REG_OUT, &reg, &bit);
	if (ret)
		return ret;
	pinctrl_smp_lock();
	clrsetbits_le32(priv->reg_gpio + reg, 0x1 << bit, param << bit);
	pinctrl_smp_unlock();

	return meson_pinconf_input_enable(dev, pin, 0);
}
#endif

int meson_pinconf_set(struct udevice *dev, unsigned int pin,
		      unsigned int param, unsigned int arg)
{
	int ret;

	switch (param) {
	case PIN_CONFIG_BIAS_DISABLE:
	case PIN_CONFIG_BIAS_PULL_UP:
	case PIN_CONFIG_BIAS_PULL_DOWN:
		ret = meson_pinconf_bias_set(dev, pin, param);
		break;
#if defined(CONFIG_AMLOGIC_MODIFY)
	case PIN_CONFIG_INPUT_ENABLE:
		ret = meson_pinconf_input_enable(dev, pin, arg);
		break;
	case PIN_CONFIG_OUTPUT:
		ret = meson_pinconf_output_set(dev, pin, arg);
		break;
#endif
	case PIN_CONFIG_DRIVE_STRENGTH_UA:
		ret = meson_pinconf_drive_strength_set(dev, pin, arg);
		break;
	default:
		dev_err(dev, "unsupported configuration parameter %u\n", param);
		return -EINVAL;
	}

	return ret;
}

int meson_pinconf_group_set(struct udevice *dev,
			    unsigned int group_selector,
			    unsigned int param, unsigned int arg)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct meson_pmx_group *grp = &priv->data->groups[group_selector];
	int i, ret;

	for (i = 0; i < grp->num_pins; i++) {
		ret = meson_pinconf_set(dev, grp->pins[i], param, arg);
		if (ret)
			return ret;
	}

	return 0;
}

int meson_gpio_probe(struct udevice *dev)
{
#if defined(CONFIG_AMLOGIC_MODIFY)
	struct meson_bank *bank = dev_get_priv(dev);
	char *name = calloc(1, 16);
#else
	struct meson_pinctrl *priv = dev_get_priv(dev->parent);
#endif
	struct gpio_dev_priv *uc_priv;

	uc_priv = dev_get_uclass_priv(dev);
#if defined(CONFIG_AMLOGIC_MODIFY)
	sprintf(name, "GPIO%s_", bank->name);
	uc_priv->bank_name = name;
	uc_priv->gpio_count = bank->last - bank->first + 1;
	debug("%s bank:%8s, first:%2d, last:%2d, count:%3d\n", __func__,
	      name, bank->first, bank->last, uc_priv->gpio_count);
#else
	uc_priv->bank_name = priv->data->name;
	uc_priv->gpio_count = priv->data->num_pins;
#endif
	return 0;
}

#if defined(CONFIG_AMLOGIC_MODIFY)
static int meson_gpio_reset_gpio_desc(struct meson_pinctrl_data *data,
				      unsigned int offset, struct gpio_desc *desc)
{
	struct udevice *dev;
	struct meson_bank *bank = NULL;
	unsigned int pin;
	int i;

	/* Find the corresponding bank based on the offset */
	pin = data->pin_base + offset;
	for (i = 0; i < data->num_banks; i++) {
		if (pin >= data->banks[i].first &&
		    pin <= data->banks[i].last) {
			bank = &data->banks[i];
			break;
		}
	}

	if (!bank)
		return -EINVAL;

	/* Find the corresponding device based on the bank */
	for (uclass_first_device(UCLASS_GPIO, &dev);
	     dev;
	     uclass_next_device(&dev)) {
		if (dev_get_priv(dev) == bank) {
			desc->dev = dev;
			desc->offset = pin - bank->first;
			desc->flags = 0;
			debug("%s redirect bank:%s offset:%u\n", __func__,
			      bank->name, desc->offset);
			return 0;
		}
	}

	/* No such GPIO */
	return -ENOENT;
}

int meson_gpio_get_xlate(struct udevice *dev, struct gpio_desc *desc,
			 struct ofnode_phandle_args *args) {
	struct gpio_dev_priv *uc_priv;
	struct meson_bank *priv = dev_get_priv(dev);
	struct meson_pinctrl *ppriv = dev_get_priv(dev->parent);
	int ret;

	if (args->args_count < 1)
		return -EINVAL;

	debug("%s self priv: %s, parent priv: %s\n", __func__,
	      priv->name, ppriv->data->name);

	/*
	 * Since the device_bind() binding used in 'pinctr-meson.c'
	 * was passed the same ofnode,  the gpio_request_by_name()
	 * always finds the first device, so we have to reset here.
	 */
	ret = meson_gpio_reset_gpio_desc(ppriv->data, args->args[0], desc);
	if (ret)
		return ret;

	dev = desc->dev;
	uc_priv = dev_get_uclass_priv(dev);

	if (desc->offset >= uc_priv->gpio_count)
		return -EINVAL;

	if (args->args_count < 2)
		return 0;

	desc->flags = gpio_flags_xlate(args->args[1]);

	return 0;
}
#endif

static fdt_addr_t parse_address(int offset, const char *name, int na, int ns)
{
	int index, len = 0;
	const fdt32_t *reg;

	index = fdt_stringlist_search(gd->fdt_blob, offset, "reg-names", name);
	if (index < 0)
		return FDT_ADDR_T_NONE;

	reg = fdt_getprop(gd->fdt_blob, offset, "reg", &len);
	if (!reg || (len <= (index * sizeof(fdt32_t) * (na + ns))))
		return FDT_ADDR_T_NONE;

	reg += index * (na + ns);

	return fdt_translate_address((void *)gd->fdt_blob, offset, reg);
}

int meson_pinctrl_probe(struct udevice *dev)
{
	struct meson_pinctrl *priv = dev_get_priv(dev);
	struct uclass_driver *drv;
	struct udevice *gpio_dev;
	fdt_addr_t addr;
	int node, gpio = -1, len;
	int na, ns;
	char *name;
#if defined(CONFIG_AMLOGIC_MODIFY)
	int index;
	int ret;
	ofnode dummy_node;
#endif

	/* FIXME: Should use livetree */
	na = fdt_address_cells(gd->fdt_blob, dev_of_offset(dev->parent));
	if (na < 1) {
		debug("bad #address-cells\n");
		return -EINVAL;
	}

	ns = fdt_size_cells(gd->fdt_blob, dev_of_offset(dev->parent));
	if (ns < 1) {
		debug("bad #size-cells\n");
		return -EINVAL;
	}

	fdt_for_each_subnode(node, gd->fdt_blob, dev_of_offset(dev)) {
		if (fdt_getprop(gd->fdt_blob, node, "gpio-controller", &len)) {
			gpio = node;
			break;
		}
	}

	if (!gpio) {
		debug("gpio node not found\n");
		return -EINVAL;
	}

	addr = parse_address(gpio, "mux", na, ns);
	if (addr == FDT_ADDR_T_NONE) {
		debug("mux address not found\n");
		return -EINVAL;
	}
	priv->reg_mux = (void __iomem *)addr;

	addr = parse_address(gpio, "gpio", na, ns);
	if (addr == FDT_ADDR_T_NONE) {
		debug("gpio address not found\n");
		return -EINVAL;
	}
	priv->reg_gpio = (void __iomem *)addr;

	addr = parse_address(gpio, "pull", na, ns);
	/* Use gpio region if pull one is not present */
	if (addr == FDT_ADDR_T_NONE)
		priv->reg_pull = priv->reg_gpio;
	else
		priv->reg_pull = (void __iomem *)addr;

	addr = parse_address(gpio, "pull-enable", na, ns);
	/* Use pull region if pull-enable one is not present */
	if (addr == FDT_ADDR_T_NONE)
		priv->reg_pullen = priv->reg_pull;
	else
		priv->reg_pullen = (void __iomem *)addr;

	addr = parse_address(gpio, "ds", na, ns);
	/* Drive strength region is optional */
	if (addr == FDT_ADDR_T_NONE)
		priv->reg_ds = NULL;
	else
		priv->reg_ds = (void __iomem *)addr;

	priv->data = (struct meson_pinctrl_data *)dev_get_driver_data(dev);

#if defined(CONFIG_AMLOGIC_MODIFY)
	/* Additional Configuration */
	if (priv->data->parse_dt)
		priv->data->parse_dt(priv);
#endif

	/* Lookup GPIO driver */
	drv = lists_uclass_lookup(UCLASS_GPIO);
	if (!drv) {
		puts("Cannot find GPIO driver\n");
		return -ENOENT;
	}

	name = calloc(1, 32);
	sprintf(name, "meson-gpio");

#if defined(CONFIG_AMLOGIC_MODIFY)
	memset(&dummy_node, 0, sizeof(dummy_node));
	/* Search for every single pin */
	for (index = 0; index < priv->data->num_banks; index++) {
		if (!priv->data->banks[index].name)
			continue;

		/* Create child device UCLASS_GPIO and bind it */
		ret = device_bind(dev, priv->data->gpio_driver, name, NULL,
				  index ? dummy_node : offset_to_ofnode(gpio), &gpio_dev);

		if (ret == 0)
			dev_set_priv(gpio_dev, &priv->data->banks[index]);
	}
#else
	/* Create child device UCLASS_GPIO and bind it */
	device_bind(dev, priv->data->gpio_driver, name, NULL,
		    offset_to_ofnode(gpio), &gpio_dev);
#endif

#ifdef CONFIG_ARMV8_MULTIENTRY
	spin_lock_init(&priv->lock);
#endif
	return 0;
}
