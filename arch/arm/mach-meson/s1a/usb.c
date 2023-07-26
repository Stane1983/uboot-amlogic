// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <bitfield.h>
#include <dm.h>
#include <errno.h>
#include <generic-phy.h>
#include <regmap.h>
#include <power/regulator.h>
#include <clk.h>
#include <asm/amlogic/arch/usb.h>
#include <amlogic/cpu_id.h>

#include <linux/compat.h>
#include <linux/ioport.h>
#include <asm-generic/gpio.h>

#define PHY20_RESET_LEVEL_BIT	8
#define	USB_RESET_BIT			4

#define RESET_BASE 0xFE002000
#define RESET_LEVEL_BASE 0xFE002040

#define USB2_PHY_PLL_OFFSET_40	(0x09400414)
#define USB2_PHY_PLL_OFFSET_44	(0x927E0000)
#define USB2_PHY_PLL_OFFSET_48	(0xac5f69e5)

#define USB2_PHY_PLL_OFFSET_10	(0x80000fff)
#define USB2_PHY_PLL_OFFSET_34	(0x78000)
#define USB2_REVB_PHY_PLL_OFFSET_34	(0x70000)

#define USB2_PHY_PLL_OFFSET_38_CLEAR	(0)
#define USB2_PHY_PLL_OFFSET_38_SET	    (0xe0004)
#define USB2_PHY_PLL_OFFSET_50	(0xfe18)
#define USB2_PHY_PLL_OFFSET_54	(0x2a)

#define TUNING_DISCONNECT_THRESHOLD 0x3f
#define DISCONNECT_THRESHOLD_ENHANCE 0x2

#define PHY_20_BASE 0xfe31c000
#define PLL_REG32_4		(PHY_20_BASE + 0x10)
#define PLL_REG32_16	(PHY_20_BASE + 0x40)
#define PLL_REG32_17	(PHY_20_BASE + 0x44)
#define PLL_REG32_18	(PHY_20_BASE + 0x48)
#define USBPLL_LK_RESET_BIT	28
#define USBPLL_EN_BIT		11
#define USBPLL_RESET_BIT	18

static struct phy usb_phys[2];

static uint32_t get_time(void)
{
	return readl(SYSCTRL_TIMERE);
}

void usb_udelay(unsigned int us)
{
	uint32_t t0 = get_time();

	while (get_time() - t0 <= us)
		;
}

int get_usbphy_baseinfo(struct phy *usb_phys)
{
	struct udevice *bus;
	struct uclass *uc;
	int ret, i;
	int count;

	if (usb_phys[0].dev && usb_phys[1].dev)
		return 0;

	ret = uclass_get(UCLASS_USB, &uc);
	if (ret)
		return ret;
	uclass_foreach_dev(bus, uc) {
		debug("bus->name=%s, bus->driver->name =%s\n",
			bus->name, bus->driver->name);
		count = dev_count_phandle_with_args(bus, "phys", "#phy-cells", 1);
		debug("usb phy count=%u\n", count);
		if (count <= 0)
			return count;
		for (i = 0; i < count; i++) {
			ret = generic_phy_get_by_index(bus, i, &usb_phys[i]);
			if (ret && ret != -ENOENT) {
				pr_err("Failed to get USB PHY%d for %s\n",
				       i, bus->name);
				return ret;
			}
			ret = generic_phy_getinfo(&usb_phys[i]);
			if (ret)
				return ret;
		}
	}
	return 0;
}

void usb_aml_detect_operation(int argc, char * const argv[])
{
	struct phy_aml_usb2_priv *usb2_priv;
	struct phy_aml_usb3_priv *usb3_priv;
	int ret;

	ret = get_usbphy_baseinfo(usb_phys);
	if (ret) {
		printf("get usb dts failed\n");
		return;
	}
	usb2_priv = dev_get_priv(usb_phys[0].dev);
	usb3_priv = dev_get_priv(usb_phys[1].dev);

	if (usb3_priv) {
		printf("priv->usb3 port num = %d, config addr=0x%08x\n",
			usb3_priv->usb3_port_num, usb3_priv->base_addr);
	}
	if (usb2_priv) {
		printf("usb2 phy: config addr = 0x%08x, reset addr=0x%08x\n",
			usb2_priv->base_addr, usb2_priv->reset_addr);

		printf("usb2 phy: portnum=%d, phy-addr1= 0x%08x, phy-addr2= 0x%08x\n",
		usb2_priv->u2_port_num, usb2_priv->usb_phy2_pll_base_addr[0],
		usb2_priv->usb_phy2_pll_base_addr[1]);
		printf("dwc2_a base addr: 0x%08x\n", usb2_priv->dwc2_a_addr);
	}

}

static void usb_set_calibration_trim(unsigned int phy2_pll_base)
{
	uint32_t cali, value, i;
	uint8_t cali_en;

	cali = readl(SYSCTRL_SEC_STATUS_REG12);
	//printf("SYSCTRL_SEC_STATUS_REG12=0x%08x\n", cali);
	/*****if cali_en ==0, set 0x10 to the default value: 0x1700****/
	cali_en = (cali >> 12) & 0x1;
	cali = cali >> 8;
	if (cali_en) {
		cali =cali & 0xf;
		if (cali > 12)
			cali = 12;
	} else {
		cali = 0x7;
	}
	value = (*(volatile uint32_t *)(unsigned long)((unsigned long)phy2_pll_base + 0x10));
	value &= (~0xfff);
	for (i = 0; i < cali; i++)
		value |= (1 << i);

	(*(volatile uint32_t *)(unsigned long)((unsigned long)phy2_pll_base + 0x10))
		= value;
	printf("0x10 trim value=0x%08x\n", value);;
}

void usb_reset(unsigned int reset_addr, int bit){
	*(volatile unsigned int *)(unsigned long)reset_addr = (1 << bit);
}

static void usb_enable_phy_pll (void)
{
	*(volatile uint32_t *)(unsigned long)RESETCTRL_RESET0_LEVEL |= (1 << PHY20_RESET_LEVEL_BIT);
}

void set_usb_pll(uint32_t phy2_pll_base)
{
	uint32_t retry = 5;
	uint32_t pll_val0, pll_val1;

	pll_val0 = 0x040a06f4;
	pll_val1 = 0x00000022;

__retry:
	writel(pll_val0, PLL_REG32_16);
	writel(pll_val1, PLL_REG32_17);
	usb_udelay(5);
	writel(pll_val1 | (1 << USBPLL_RESET_BIT), PLL_REG32_17);
	writel((pll_val0 | (1 << USBPLL_LK_RESET_BIT) | (1 << USBPLL_EN_BIT)), PLL_REG32_16);
	usb_udelay(50);
	writel(pll_val1, PLL_REG32_17);
	usb_udelay(50);
	writel((pll_val0 | (1 << USBPLL_EN_BIT)), PLL_REG32_16);

	// wait for 200us
	usb_udelay(200);
	//check lock bit
	if (readl(PLL_REG32_16) >> 31)
		return;

	retry--;
	if (!retry)
		return;

	goto __retry;
}

int usb_save_phy_dev (unsigned int number, struct phy *phy)
{
	usb_phys[number].dev = phy->dev;
	usb_phys[number].id = phy->id;
	return 0;
}

int usb2_phy_init (struct phy *phy) {
	struct phy_aml_usb2_priv *priv = dev_get_priv(phy->dev);
	struct u2p_aml_regs *u2p_aml_reg;
	u2p_r0_t dev_u2p_r0;
	u2p_r1_t dev_u2p_r1;
	int i,cnt;

	usb_save_phy_dev(0, phy);
	usb_enable_phy_pll();

	*(volatile unsigned int *)(unsigned long)priv->reset_addr = (1 << USB_RESET_BIT);

	udelay(500);
	priv->usbphy_reset_bit[0] = PHY20_RESET_LEVEL_BIT;

	for (i = 0; i < priv->u2_port_num; i++) {
		u2p_aml_reg = (struct u2p_aml_regs *)((ulong)(priv->base_addr + i * PHY_REGISTER_SIZE));
		dev_u2p_r0.d32 = u2p_aml_reg->u2p_r0;
		dev_u2p_r0.b.host_device= 1;
		dev_u2p_r0.b.POR= 0;
		u2p_aml_reg->u2p_r0  = dev_u2p_r0.d32;
		udelay(10);
		*(volatile unsigned int *)(unsigned long)priv->reset_addr = (1 << priv->usbphy_reset_bit[i]);
		udelay(50);

		/* wait for phy ready */
		dev_u2p_r1.d32  = u2p_aml_reg->u2p_r1;
		cnt = 0;
		while (dev_u2p_r1.b.phy_rdy != 1) {
			dev_u2p_r1.d32 = u2p_aml_reg->u2p_r1;
			/*we wait phy ready max 1ms, common is 100us*/
			if (cnt > 200)
				break;
			else {
				cnt++;
				udelay(5);
			}
		}
	}

	for (i = 0; i < priv->u2_port_num; i++) {
		debug("------set usb pll\n");
		set_usb_pll(priv->usb_phy2_pll_base_addr[i]);
	}
	return 0;

}

int usb2_phy_tuning(uint32_t phy2_pll_base, int port)
{
	return 0;
}

/**************************************************************/
/*           device mode config                               */
/**************************************************************/
unsigned int usb_get_dwc_a_base_addr(void)
{
	struct phy_aml_usb2_priv *usb2_priv;
	int ret;

	if (!usb_phys[0].dev) {
		ret = get_usbphy_baseinfo(usb_phys);
		if (ret) {
			printf("get usb dts failed\n");
			return 0;
		}
	}
	usb2_priv = dev_get_priv(usb_phys[0].dev);
	return usb2_priv->dwc2_a_addr;
}

unsigned int usb_get_device_mode_phy_base(void)
{
	struct phy_aml_usb2_priv *usb2_priv;
	int ret;

	if (!usb_phys[0].dev) {
		ret = get_usbphy_baseinfo(usb_phys);
		if (ret) {
			printf("get usb dts failed\n");
			return 0;
		}
	}
	usb2_priv = dev_get_priv(usb_phys[0].dev);
	return usb2_priv->usb_phy2_pll_base_addr[1];
}

void usb_phy_tuning_reset(void)
{
	return ;
}

void usb_device_mode_init(int phy_num)
{
	u2p_r0_t dev_u2p_r0;
	u2p_r1_t dev_u2p_r1;

	usb_r0_t dev_usb_r0;
	usb_r4_t dev_usb_r4;
	int cnt, ret;
	u2p_aml_regs_t * u2p_aml_regs;
	usb_aml_regs_t *usb_aml_regs;
	struct phy_aml_usb2_priv *usb2_priv;
	struct phy_aml_usb3_priv *usb3_priv;
	unsigned int phy_base_addr, reset_addr;

	ret = get_usbphy_baseinfo(usb_phys);
	if (ret) {
		printf("get usb dts failed\n");
		return;
	}
	usb2_priv = dev_get_priv(usb_phys[0].dev);
	usb3_priv = dev_get_priv(usb_phys[1].dev);
	if (!usb2_priv || !usb3_priv) {
		printf("get usb phy address from dts failed\n");
		return;
	}

	u2p_aml_regs = (u2p_aml_regs_t * )((unsigned long)(usb2_priv->base_addr + PHY_REGISTER_SIZE));
	usb_aml_regs = (usb_aml_regs_t * )((ulong)usb3_priv->base_addr);
	phy_base_addr = usb2_priv->usb_phy2_pll_base_addr[0];
	reset_addr = usb2_priv->reset_addr;

	printf("PHY2=%p,phy-base=0x%08x\n", u2p_aml_regs, phy_base_addr);
	if ((*(volatile uint32_t *)(unsigned long)(phy_base_addr + 0x38)) != 0) {
		usb_phy_tuning_reset();
		mdelay(150);
	}

	writel((readl(RESET_LEVEL_BASE) & (~(0x1 << PHY20_RESET_LEVEL_BIT))), RESET_LEVEL_BASE);
	usb_udelay(500);
	writel((readl(RESET_LEVEL_BASE) | (0x1 << PHY20_RESET_LEVEL_BIT)), RESET_LEVEL_BASE);

	//step 1: usb controller reset
	usb_reset(reset_addr, USB_RESET_BIT);

	// step 3: enable usb INT internal USB
	dev_usb_r0.d32	 = usb_aml_regs->usb_r0;
	dev_usb_r0.b.u2d_ss_scaledown_mode = 0;
	dev_usb_r0.b.u2d_act			   = 1;
	usb_aml_regs->usb_r0 = dev_usb_r0.d32;

	// step 4: disable usb phy sleep
	dev_usb_r4.d32	 = usb_aml_regs->usb_r4;
	dev_usb_r4.b.p21_SLEEPM0   = 1;
	usb_aml_regs->usb_r4   = dev_usb_r4.d32;

	// step 5: config phy20 device mode
	dev_u2p_r0.d32	 = u2p_aml_regs->u2p_r0;
	dev_u2p_r0.b.host_device= 0;
	dev_u2p_r0.b.POR= 0;
	u2p_aml_regs->u2p_r0  = dev_u2p_r0.d32;

	usb_udelay(10);
	//step 6: phy20 reset
	usb_reset(reset_addr, PHY20_RESET_LEVEL_BIT);
	usb_udelay(50);

	usb_set_calibration_trim(phy_base_addr);
	usb_udelay(50);

	// step 6: wait for phy ready
	dev_u2p_r1.d32	= u2p_aml_regs->u2p_r1;
	cnt = 0;
	while ((dev_u2p_r1.d32 & 0x00000001) != 1) {
		dev_u2p_r1.d32 = u2p_aml_regs->u2p_r1;
		if (cnt > 200)
			break;
		else {
			cnt++;
			usb_udelay(5);
		}
	}

	set_usb_pll(phy_base_addr);
	//--------------------------------------------------

	// ------------- usb phy20 initial end ----------

	//--------------------------------------------------
}
