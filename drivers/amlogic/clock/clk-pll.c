// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define LOG_CATEGORY UCLASS_CLK

#include <common.h>
#include <log.h>
#include <clk-uclass.h>
#include <malloc.h>
#include <asm/io.h>
#include <dm/device.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/math64.h>

#include <amlogic/clk-pll.h>

#define UBOOT_DM_CLK_PLL_AML "clk_pll_aml"

#define to_clk_pll(_clk) container_of(_clk, struct aml_pll, clk)

static bool clk_pll_is_enabled(struct clk *clk)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct pll_parm *pen, *pl;

	pen = &pll->en;
	pl = &pll->l;
	/* Enable and lock bit equal 1, it locks */
	if (readl(aml_pll->addr + pen->reg_off) & BIT(pen->shift) &&
	    readl(aml_pll->addr + pl->reg_off) & BIT(pl->shift))
		return true;

	return false;
}

static int clk_pll_init(struct clk *clk)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct reg_sequence *init_regs;
	int i;

	if (!clk)
		return -EFAULT;

	if (pll->init_count == 0)
		return 0;

	if ((pll->flags & CLK_AML_PLL_NOINIT_ENABLED) &&
	    clk_pll_is_enabled(clk)) {
		log_notice("ignore %s clock init\n", clk->dev->name);
		return 0;
	}

	if (pll->flags & CLK_AML_PLL_RSTN)
		clrbits_32(aml_pll->addr + pll->rst.reg_off, BIT(pll->rst.shift));
	else
		setbits_32(aml_pll->addr + pll->rst.reg_off, BIT(pll->rst.shift));

	init_regs = pll->init_regs;
	for (i = 0; i < pll->init_count; i++, init_regs++) {
		writel(init_regs->def, aml_pll->addr + init_regs->reg_off);
		if (init_regs->delay_us)
			udelay(init_regs->delay_us);
	}

	if (pll->flags & CLK_AML_PLL_RSTN)
		setbits_32(aml_pll->addr + pll->rst.reg_off, BIT(pll->rst.shift));
	else
		clrbits_32(aml_pll->addr + pll->rst.reg_off, BIT(pll->rst.shift));

	return 0;
}

#define PLL_WAIT_LOCK_US_MAX		200
#define PLL_LOCK_RETRY_CNT_MAX		10

static int clk_pll_wait_lock(void __iomem *addr, const struct pll_parm *pl)
{
	unsigned int val;
	int delay = PLL_WAIT_LOCK_US_MAX;

	do {
		/* Is the clock locked now ? */
		val = readl(addr + pl->reg_off);
		if (val & BIT(pl->shift))
			return 0;

		udelay(1);
	} while (delay--);

	return -ETIMEDOUT;
}

static int clk_pll_enable(struct clk *clk)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct pll_parm *pen = &pll->en;
	const struct pll_parm *prst = &pll->rst;
	const struct pll_parm *plrst = &pll->l_rst;
	const struct pll_parm *pl = &pll->l;
	int retry = 0;

	if (!clk)
		return -EFAULT;

	/* do nothing if the PLL is already enabled */
	if (clk_pll_is_enabled(clk))
		return 0;

	do {
		/* Make sure the pll is in reset */
		if (pll->flags & CLK_AML_PLL_RSTN)
			clrbits_32(aml_pll->addr + prst->reg_off,
				   BIT(prst->shift));
		else
			setbits_32(aml_pll->addr + prst->reg_off,
				   BIT(prst->shift));

		/* Enable the pll */
		setbits_32(aml_pll->addr + pen->reg_off, BIT(pen->shift));

		/* Make sure the pll is in lock reset */
		if (plrst->width) {
			if (pll->flags & CLK_AML_PLL_RSTN)
				clrbits_32(aml_pll->addr + plrst->reg_off,
					   BIT(plrst->shift));
			else
				setbits_32(aml_pll->addr + plrst->reg_off,
					   BIT(plrst->shift));
		}

		udelay(20);

		/* Take the pll out reset */
		if (pll->flags & CLK_AML_PLL_RSTN)
			setbits_32(aml_pll->addr + prst->reg_off,
				   BIT(prst->shift));
		else
			clrbits_32(aml_pll->addr + prst->reg_off,
				   BIT(prst->shift));

		/* Take the pll out lock reset */
		if (plrst->width) {
			udelay(20);
			if (pll->flags & CLK_AML_PLL_RSTN)
				setbits_32(aml_pll->addr + plrst->reg_off,
					   BIT(plrst->shift));
			else
				clrbits_32(aml_pll->addr + plrst->reg_off,
					   BIT(plrst->shift));
		}

		if (!clk_pll_wait_lock(aml_pll->addr, pl))
			return 0;

		retry++;
	} while (retry < PLL_LOCK_RETRY_CNT_MAX);

	log_err("%s: pll lock failed!!!\n", clk->dev->name);
	return -EIO;
}

static int clk_pll_disable(struct clk *clk)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct pll_parm *pen = &pll->en;
	const struct pll_parm *prst = &pll->rst;

	if (!clk)
		return -EINVAL;

	/* Put the pll is in reset */
	if (pll->flags & CLK_AML_PLL_RSTN)
		clrbits_32(aml_pll->addr + prst->reg_off, BIT(prst->shift));
	else
		setbits_32(aml_pll->addr + prst->reg_off, BIT(prst->shift));

	/* Disable the pll */
	clrbits_32(aml_pll->addr + pen->reg_off, BIT(pen->shift));

	return 0;
}

static unsigned long __pll_params_to_rate(unsigned long parent_rate,
					  unsigned int m, unsigned int n,
					  unsigned int frac,
					  const struct aml_pll_data *pll)
{
	u64 rate, frac_rate, frac_base;

	if (pll->flags & CLK_AML_PLL_FIXED_EN0P5)
		parent_rate = parent_rate >> 1;

	rate = (u64)parent_rate * m;
	/*
	 * FIXME: frac is currently only available on fixpll, and other frac
	 * features are not yet implemented.
	 */
	if (frac && pll->frac.width) {
		frac_rate = (u64)parent_rate * frac;
		if (pll->flags & CLK_AML_PLL_FIXED_FRAC_WEIGHT_PRECISION)
			frac_base = 100000;
		else
			frac_base = ((uint32_t)1 << pll->frac.width);

		rate += DIV_ROUND_UP_ULL(frac_rate, frac_base);
	}

	if (pll->flags & CLK_AML_PLL_POWER_OF_TWO)
		n = 1 << n;

	if (n == 0)
		return 0;

	return DIV_ROUND_UP_ULL(rate, n);
}

static const struct pll_params_table *clk_get_pll_settings(unsigned long rate,
							   const struct aml_pll_data *data)
{
	int i;

	if (!data || !data->table)
		return NULL;

	for (i = 0; i < data->table_count; i++) {
		if (rate == data->table[i].rate ||
		    ((i + 1 < data->table_count) && rate < data->table[i + 1].rate))
			return &data->table[i];
	}

	return &data->table[data->table_count - 1];
}

#define SETPMASK(width, shift)		GENMASK((shift) + (width) - 1, (shift))

#define PARM_GET(width, shift, reg)			\
	(((reg) & SETPMASK(width, shift)) >> (shift))

static unsigned long clk_pll_set_rate(struct clk *clk, unsigned long rate)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct pll_params_table *table;
	const struct pll_parm *pm, *pn, *pod, *pfrac;
	bool enabled;

	if (!clk)
		return -EFAULT;

	table = clk_get_pll_settings(rate, pll);
	if (!table)
		return -EFAULT;

	pm = &pll->m;
	pn = &pll->n;
	pod = &pll->od;
	pfrac = &pll->frac;
	enabled = clk_pll_is_enabled(clk);
	if (enabled)
		clk_pll_disable(clk);

	clrsetbits_32(aml_pll->addr + pm->reg_off, SETPMASK(pm->width, pm->shift),
		      table->m << pm->shift);
	clrsetbits_32(aml_pll->addr + pn->reg_off, SETPMASK(pn->width, pn->shift),
		      table->n << pn->shift);
	if (pod->width)
		clrsetbits_32(aml_pll->addr + pod->reg_off, SETPMASK(pod->width,
								     pod->shift),
			      table->od << pod->shift);

	if (pfrac->width)
		clrsetbits_32(aml_pll->addr + pfrac->reg_off, SETPMASK(pfrac->width,
								       pfrac->shift),
			      table->frac << pfrac->shift);

	/* If the pll is stopped, bail out now */
	if (!enabled)
		return 0;

	if (!clk_pll_enable(clk))
		return 0;

	return -EIO;
}

static unsigned long clk_pll_get_rate(struct clk *clk)
{
	const struct aml_pll *aml_pll = to_clk_pll(clk);
	const struct aml_pll_data *pll = aml_pll->data;
	const struct pll_parm *pm, *pn, *pod, *pfrac;
	unsigned int m, n, frac, od;
	unsigned long prate = clk_get_parent_rate(clk);

	if (!clk)
		return -EFAULT;

	pm = &pll->m;
	pn = &pll->n;
	pod = &pll->od;
	pfrac = &pll->frac;
	m = PARM_GET(pm->width, pm->shift, readl(aml_pll->addr + pm->reg_off));
	n = PARM_GET(pn->width, pn->shift, readl(aml_pll->addr + pn->reg_off));
	frac = pfrac->width ?
		PARM_GET(pfrac->width, pfrac->shift,
			 readl(aml_pll->addr + pfrac->reg_off)) : 0;
	od = pod->width ?
		PARM_GET(pod->width, pod->shift,
			 readl(aml_pll->addr + pod->reg_off)) : 0;

	return __pll_params_to_rate(prate, m, n, frac, pll) >> od;
}

const struct clk_ops clk_pll_ops = {
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.get_rate = clk_pll_get_rate,
	.set_rate = clk_pll_set_rate,
};

struct clk *aml_clk_register_pll(const char *name, const char *parent_name,
				 unsigned long flags, void __iomem *addr,
				 const struct aml_pll_data *data)
{
	struct aml_pll *pll;
	struct clk *clk;
	int ret;

	/* allocate the gate */
	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	/* struct clk_gate assignments */
	pll->data = data;
	pll->addr = addr;

	clk = &pll->clk;
	clk->flags = flags;

	ret = clk_register(clk, UBOOT_DM_CLK_PLL_AML, name, parent_name);
	if (ret) {
		kfree(pll);
		return ERR_PTR(ret);
	}

	clk_pll_init(clk);

	return clk;
}

U_BOOT_DRIVER(clk_pll_aml) = {
	.name	= UBOOT_DM_CLK_PLL_AML,
	.id	= UCLASS_CLK,
	.ops	= &clk_pll_ops,
	.flags = DM_FLAG_PRE_RELOC,
};

