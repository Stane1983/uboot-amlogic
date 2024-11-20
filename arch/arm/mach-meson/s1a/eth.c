// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
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
#include <power/regulator.h>

#if defined(CONFIG_AMLOGIC_ETH)
#ifdef CONFIG_DM_ETH
#include <amlogic/pm.h>
#include <linux/io.h>
#include <command.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <linux/ioport.h>
#include <asm/amlogic/arch/pwr_ctrl.h>
#include <asm/amlogic/arch/register.h>
#include <asm/amlogic/arch/eth.h>
#include <dm/pinctrl.h>
#ifdef CONFIG_DM_GPIO
#include <asm/gpio.h>
#endif

#ifndef ANACTRL_PLL_GATE_DIS
#define ANACTRL_PLL_GATE_DIS 0xffffffff
#endif

#define AML_ETH_PLL_CTL0 0x44
#define AML_ETH_PLL_CTL1 0x48
#define AML_ETH_PLL_CTL2 0x4C
#define AML_ETH_PLL_CTL3 0x50
#define AML_ETH_PLL_CTL4 0x54
#define AML_ETH_PLL_CTL5 0x58
#define AML_ETH_PLL_CTL6 0x5C
#define AML_ETH_PLL_CTL7 0x60

#define AML_ETH_PHY_CNTL0 0x80
#define AML_ETH_PHY_CNTL1 0x84
#define AML_ETH_PHY_CNTL2 0x88


enum {
	/* chip num */
	ETH_PHY		= 0x0,
	ETH_PHY_C1	= 0x1,
	ETH_PHY_C2	= 0x2,
	ETH_PHY_SC2	= 0x3,
};

struct aml_phy_dev {
	void __iomem *phy_top;
	void __iomem *phy_cfg;
	struct dev_pm_ops *pm_ops;
};
struct aml_phy_dev aml_phy_dev;
struct aml_phy_dev *p_aml_phy_dev = &aml_phy_dev;

int aml_phy_suspend(void *pm_ops);
int aml_phy_resume(void *pm_ops);
int aml_phy_poweroff(void *pm_ops);

int internal_phy;
unsigned int setup_amp;
extern int soc_num;
extern struct phy_device *p_phydev;

void setup_tx_amp(struct udevice *dev)
{
	unsigned int tx_amp_src = 0;
	tx_amp_src = dev_read_u32_default(dev, "tx_amp_src", 0);
	if (0 == tx_amp_src) {
		printf("not set tx_amp_src\n");
	} else {
		setup_amp = readl((uintptr_t)tx_amp_src);
		printf("addr 0x%x  =  0x%x\n", tx_amp_src, readl((uintptr_t)tx_amp_src));
	}
}
static void setup_internal_phy(struct udevice *dev)
{
	int phy_cntl1 = 0;
	int mc_val = 0;
	int chip_num = 0;
	unsigned int pll_val[3] = {0};
	unsigned int analog_val[3] = {0};
	int rtn = 0;
	struct resource eth_top, eth_cfg;

	phy_cntl1 = dev_read_u32_default(dev, "phy_cntl1", 4);
	if (phy_cntl1 < 0) {
		printf("miss phy_cntl1\n");
	}

	mc_val = dev_read_u32_default(dev, "mc_val", 4);
	if (mc_val < 0) {
		printf("miss mc_val\n");
	}

	chip_num = dev_read_u32_default(dev, "chip_num", 4);
	if (chip_num < 0) {
		chip_num = 0;
		printf("use 0 as default chip num\n");
	}
	printf("chip num %d\n", chip_num);

	rtn = dev_read_u32_array(dev, "pll_val", pll_val, ARRAY_SIZE(pll_val));
	if (rtn < 0) {
		printf("miss pll_val\n");
	}

	dev_read_u32_array(dev, "analog_val", analog_val, ARRAY_SIZE(analog_val));
	if (rtn < 0) {
		printf("miss analog_val\n");
	}

	for (int i = 0;i <3; i++) {
		debug("pll_val 0x%08x\n", pll_val[i]);
	}

	for (int i = 0;i <3; i++) {
		debug("analog_val 0x%08x\n", analog_val[i]);
	}

	rtn = dev_read_resource_byname(dev, "eth_top", &eth_top);
	if (rtn) {
		printf("can't get eth_top resource(ret = %d)\n", rtn);
	}

	rtn = dev_read_resource_byname(dev, "eth_cfg", &eth_cfg);
	if (rtn) {
		printf("can't get eth_cfg resource(ret = %d)\n", rtn);
	}
//	printf("wzh eth_top 0x%x eth_cfg 0x%x \n", eth_top.start, eth_cfg.start);

	p_aml_phy_dev->phy_top = devm_ioremap(dev, eth_top.start, resource_size(&eth_top));
	p_aml_phy_dev->phy_cfg = devm_ioremap(dev, eth_cfg.start, resource_size(&eth_cfg));
	p_aml_phy_dev->pm_ops = dev_register_pm("eth ops",
				&aml_phy_suspend,
				&aml_phy_resume,
				&aml_phy_poweroff);

	setup_tx_amp(dev);
	/*top*/
//	setbits_le32(ETHTOP_CNTL0, mc_val);
	setbits_le32(eth_top.start, mc_val);
	/*pll*/
#if 0
	writel(pll_val[0] | 0x30000000, eth_cfg.start + AML_ETH_PLL_CTL0);
	writel(pll_val[1], eth_cfg.start + AML_ETH_PLL_CTL1);
	writel(pll_val[2], eth_cfg.start + AML_ETH_PLL_CTL2);
	writel(0x00000000, eth_cfg.start + AML_ETH_PLL_CTL3);
	udelay(200);
	writel(pll_val[0] | 0x10000000, eth_cfg.start + AML_ETH_PLL_CTL0);
#else
	pr_info("wzh 22nm\n");
	writel(0x608200a0, eth_cfg.start + AML_ETH_PLL_CTL0);
	writel(0xea002000, eth_cfg.start + AML_ETH_PLL_CTL1);
	writel(0x00000150, eth_cfg.start + AML_ETH_PLL_CTL2);
	writel(0x00000000, eth_cfg.start + AML_ETH_PLL_CTL3);
	writel(0x708200a0, eth_cfg.start + AML_ETH_PLL_CTL0);
	udelay(200);
	writel(0x508200a0, eth_cfg.start + AML_ETH_PLL_CTL0);
	writel(0x00000110, eth_cfg.start + AML_ETH_PLL_CTL2);
#endif

	/*analog*/
	writel(analog_val[0], eth_cfg.start + AML_ETH_PLL_CTL5);
	writel(analog_val[1], eth_cfg.start + AML_ETH_PLL_CTL6);
	writel(analog_val[2], eth_cfg.start + AML_ETH_PLL_CTL7);

	/*ctrl*/
	/*config phyid should between  a 0~0xffffffff*/
	/*please don't use 44000181, this has been used by internal phy*/
	writel(0x33000180, eth_cfg.start + AML_ETH_PHY_CNTL0);

	/*use_phy_smi | use_phy_ip | co_clkin from eth_phy_top*/
	writel(0x260, eth_cfg.start + AML_ETH_PHY_CNTL2);
	writel(phy_cntl1, eth_cfg.start + AML_ETH_PHY_CNTL1);
	writel(phy_cntl1 & (~0x40000), eth_cfg.start + AML_ETH_PHY_CNTL1);
	writel(phy_cntl1, eth_cfg.start + AML_ETH_PHY_CNTL1);
	udelay(200);

	if (chip_num != ETH_PHY_SC2) {
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 6));
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 7));
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 19));
	}
	/*s1a analog amp*/
	writel(0x00000023, eth_cfg.start + AML_ETH_PLL_CTL7);
	writel(0x0000c001, eth_cfg.start + AML_ETH_PLL_CTL6);
	writel(0x20220000, eth_cfg.start + AML_ETH_PLL_CTL5);
	writel(0xaa820000, eth_cfg.start + AML_ETH_PLL_CTL3);
}

static void setup_external_phy(struct udevice *dev)
{
	int mc_val = 0;
	int cali_val = 0;
	int analog_ver = 0;
	int chip_num = 0;
	int rtn = 0;
	struct resource eth_top, eth_cfg;
	/*reset phy*/
	struct gpio_desc desc;
	int ret;

	chip_num = dev_read_u32_default(dev, "chip_num", 4);
	if (chip_num < 0) {
		chip_num = 0;
		printf("use 0 as default chip num\n");
	}
	printf("chip num %d\n", chip_num);

	if (chip_num != ETH_PHY_SC2) {
		ret = gpio_request_by_name(dev, "reset-gpios", 0, &desc, GPIOD_IS_OUT);
		if (ret) {
			printf("request gpio failed!\n");
		//	return ret;
		}
		if (dm_gpio_is_valid(&desc)) {
			dm_gpio_set_value(&desc, 1);
			mdelay(100);
		}
		dm_gpio_free(dev, &desc);
	}

	mc_val = dev_read_u32_default(dev, "mc_val", 4);
	if (mc_val < 0) {
		printf("miss mc_val\n");
	}

	cali_val = dev_read_u32_default(dev, "cali_val", 4);
	if (mc_val < 0) {
		printf("miss cali_val\n");
	}

	/*set rmii pinmux*/
	if (mc_val & 0x4) {
		pinctrl_select_state(dev, "external_eth_rmii_pins");
		printf("set rmii\n");
	}
	/*set rgmii pinmux*/
	if (mc_val & 0x1) {
		pinctrl_select_state(dev, "external_eth_rgmii_pins");
		printf("set rgmii\n");
	}
	rtn = dev_read_resource_byname(dev, "eth_top", &eth_top);
	if (rtn) {
		printf("can't get eth_top resource(ret = %d)\n", rtn);
	}

	rtn = dev_read_resource_byname(dev, "eth_cfg", &eth_cfg);
	if (rtn) {
		printf("can't get eth_cfg resource(ret = %d)\n", rtn);
	}
//	printf("eth_top 0x%x eth_cfg 0x%x \n", eth_top.start, eth_cfg.start);

	setbits_le32(eth_top.start, mc_val);
	setbits_le32(eth_top.start + 4, cali_val);

	analog_ver = dev_read_u32_default(dev, "analog_ver", 4);
	if (mc_val < 0) {
		printf("miss analog_ver\n");
	}
	if (analog_ver != 2)
		writel(0x0, eth_cfg.start + AML_ETH_PHY_CNTL2);

	if (chip_num != ETH_PHY_SC2) {
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 6));
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 7));
		clrbits_le32(ANACTRL_PLL_GATE_DIS, (0x1 << 19));
	}
}

int aml_phy_suspend(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;
	unsigned int phy_setting;

	printf("disable analog %s\n", pm->name);
	writel(0x00000000, p_aml_phy_dev->phy_cfg + 0x0);
	writel(0x003e0000, p_aml_phy_dev->phy_cfg + 0x4);
	writel(0x12844008, p_aml_phy_dev->phy_cfg + 0x8);
	writel(0x0800a40c, p_aml_phy_dev->phy_cfg + 0xc);
	writel(0x00000000, p_aml_phy_dev->phy_cfg + 0x10);
	writel(0x031d161c, p_aml_phy_dev->phy_cfg + 0x14);
	writel(0x00001683, p_aml_phy_dev->phy_cfg + 0x18);
	writel(0x09c0040a, p_aml_phy_dev->phy_cfg + 0x44);

	phy_setting = phy_read(p_phydev, MDIO_DEVAD_NONE, 0);
	phy_write(p_phydev, MDIO_DEVAD_NONE, 0, phy_setting | 0x800);

	return 0;
}

int aml_phy_resume(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;
	unsigned int phy_setting;

	printf("recover analog %s\n", pm->name);
	writel(0x608200a0, p_aml_phy_dev->phy_cfg + 0x44);
	writel(0xea002000, p_aml_phy_dev->phy_cfg + 0x48);
	writel(0x00000150, p_aml_phy_dev->phy_cfg + 0x4c);
	writel(0x00000000, p_aml_phy_dev->phy_cfg + 0x50);
	writel(0x708200a0, p_aml_phy_dev->phy_cfg + 0x44);
	udelay(200);
	writel(0x508200a0, p_aml_phy_dev->phy_cfg + 0x44);
	writel(0x00000110, p_aml_phy_dev->phy_cfg + 0x4c);

	phy_setting = phy_read(p_phydev, MDIO_DEVAD_NONE, 0);
	phy_write(p_phydev, MDIO_DEVAD_NONE, 0, phy_setting & ~(0x800));

	return 0;
}

int aml_phy_poweroff(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("power off  %s\n", pm->name);
	aml_phy_suspend(pm_ops);
	return 0;
}

void __iomem *DM_network_interface_setup(struct udevice *dev)
{
	internal_phy = dev_read_u32_default(dev, "internal_phy", 1);
	if (internal_phy < 0) {
		debug("miss internal_phy item\n");
	}
	debug("internal_phy = 0x%x\n", internal_phy);
	if (internal_phy) {
		printf("in-phy\n");
		setup_internal_phy(dev);
	} else {
		printf("ex-phy\n");
		setup_external_phy(dev);
	}
	soc_num = dev_read_u32_default(dev, "soc_num", 0);
	printf("soc num %d\n", soc_num);
	udelay(1000);
	return 0;
}

static unsigned int return_write_val(struct phy_device *phy_dev, int rd_addr)
{
	int rd_data;
	int rd_data_hi;

	phy_write(phy_dev, MDIO_DEVAD_NONE, 20,
			((1 << 15) | (1 << 10) | ((rd_addr & 0x1f) << 5)));
	rd_data = phy_read(phy_dev, MDIO_DEVAD_NONE, 21);
	rd_data_hi = phy_read(phy_dev, MDIO_DEVAD_NONE, 22);
	rd_data = ((rd_data_hi & 0xffff) << 16) | rd_data;

	return rd_data;
}

static unsigned int phy_tst_write(struct phy_device *phy_dev, unsigned int wr_addr,
				unsigned int wr_data)
{
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0000);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0400);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0000);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0400);

	if (wr_addr <= 31) {
		phy_write(phy_dev, MDIO_DEVAD_NONE, 23, (wr_data & 0xffff));
		phy_write(phy_dev, MDIO_DEVAD_NONE, 20,
			((1 << 14) | (1 << 10) | ((wr_addr << 0) & 0x1f)));
		pr_info("write phy tstcntl [reg_%d] 0x%x, 0x%x\n",
			wr_addr, wr_data, return_write_val(phy_dev, wr_addr));
	} else {
		pr_info("Invalid parameter\n");
	}
	return 0;
}

#if 0
static unsigned int phy_tst_read(struct phy_device *phy_dev, unsigned int rd_addr)
{
	unsigned int rd_data_hi;
	unsigned int rd_data = 0;

	/*init*/
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0000);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0400);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0000);
	phy_write(phy_dev, MDIO_DEVAD_NONE, 20, 0x0400);

	if (rd_addr <= 31) {
		phy_write(phy_dev, MDIO_DEVAD_NONE, 20,
			((1 << 15) | (1 << 10) | ((rd_addr & 0x1f) << 5)));

		rd_data = phy_read(phy_dev, MDIO_DEVAD_NONE, 21);
		rd_data_hi = phy_read(phy_dev, MDIO_DEVAD_NONE, 22);
		rd_data = ((rd_data_hi & 0xffff) << 16) | rd_data;
		printf("read tstcntl phy [reg_%d] 0x%x\n", rd_addr, rd_data);
	} else {
		printf("Invalid parameter\n");
	}

	return rd_data;
}
#endif

void DM_network_interface_setup_final(struct phy_device *phydev)
{
	if (internal_phy) {
		phy_tst_write(phydev, 0x18, 0x8);
		phy_tst_write(phydev, 0x16, 0x8400);
		phy_tst_write(phydev, 0x15, 0x4408);
	}
}

void __iomem *DM_network_interface_remove(void)
{
	if (p_aml_phy_dev->pm_ops)
		dev_unregister_pm(p_aml_phy_dev->pm_ops);
	return 0;
}
#endif
#endif

