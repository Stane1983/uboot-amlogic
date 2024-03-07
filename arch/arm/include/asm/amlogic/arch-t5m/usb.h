/*
 * arch/arm/include/asm/arch-txl/usb-new.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __ARCH_ARM_MESON_USB_H_U_BOOT__
#define __ARCH_ARM_MESON_USB_H_U_BOOT__

#include <common.h>
#include <asm/types.h>
#include <asm/amlogic/arch/io.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <generic-phy.h>
#include <asm-generic/gpio.h>

#define USB_PHY2_ENABLE			0x10000000
#define USB_PHY2_RESET			0x20000000

/* XHCI PHY register structure */
#define PHY_REGISTER_SIZE	0x20

#define USB_POC7(poc)       ((poc) >> 7 & 1)
#define POC_USB_CHANNEL_C(poc)  (!USB_POC7(poc))

#define CEG_UDC_1_BASE        (u64)((POC_USB_CHANNEL_C(*(u32 *)\
	((0x0060  << 2) + 0xfe010000)) ? 0xfe350000 : 0xfe310000))

struct phy_aml_usb2_priv {
	unsigned int base_addr;
	unsigned int reset_addr;
	unsigned int dwc2_a_addr;
	unsigned int u2_port_num;
	unsigned int usbphy_reset_bit[8];
	unsigned int usb_phy2_pll_base_addr[4];
};

struct phy_aml_usb3_priv {
	unsigned int base_addr;
	unsigned int usb3_port_num;
	struct gpio_desc desc;
};

/* Register definitions */
typedef struct u2p_aml_regs {
	volatile uint32_t u2p_r0;
	volatile uint32_t u2p_r1;
} u2p_aml_regs_t;

typedef union u2p_r0 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned host_device:1;
		unsigned power_ok:1;
		unsigned hast_mode:1;
		unsigned POR:1;
		unsigned IDPULLUP0:1;
		unsigned DRVVBUS0:1;
		unsigned reserved:26;
    } b;
} u2p_r0_t;

typedef union u2p_r1 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned phy_rdy:1;
		unsigned IDDIG0:1;
		unsigned OTGSESSVLD0:1;
		unsigned VBUSVALID0:1;
		unsigned reserved:28;
	} b;
} u2p_r1_t;


typedef struct usb_aml_regs {
	volatile uint32_t usb_r0;
	volatile uint32_t usb_r1;
	volatile uint32_t usb_r2;
	volatile uint32_t usb_r3;
	volatile uint32_t usb_r4;
	volatile uint32_t usb_r5;
} usb_aml_regs_t;

typedef union usb_r0 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned reserved:17;
		unsigned p30_lane0_tx2rx_loopback:1;
		unsigned p30_lane0_ext_pclk_reg:1;
		unsigned p30_pcs_rx_los_mask_val:10;
		unsigned u2d_ss_scaledown_mode:2;
		unsigned u2d_act:1;
    } b;
} usb_r0_t;

typedef union usb_r1 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned u3h_bigendian_gs:1;
		unsigned u3h_pme_en:1;
		unsigned u3h_hub_port_overcurrent:3;
		unsigned reserved_1:2;
		unsigned u3h_hub_port_perm_attach:3;
		unsigned reserved_2:2;
		unsigned u3h_host_u2_port_disable:2;
		unsigned reserved_3:2;
		unsigned u3h_host_u3_port_disable:1;
		unsigned u3h_host_port_power_control_present:1;
		unsigned u3h_host_msi_enable:1;
		unsigned u3h_fladj_30mhz_reg:6;
		unsigned p30_pcs_tx_swing_full:7;
	} b;
} usb_r1_t;

typedef union usb_r2 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned reserved:20;
		unsigned p30_pcs_tx_deemph_3p5db:6;
		unsigned p30_pcs_tx_deemph_6db:6;
	} b;
} usb_r2_t;

typedef union usb_r3 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p30_ssc_en:1;
		unsigned p30_ssc_range:3;
		unsigned p30_ssc_ref_clk_sel:9;
		unsigned p30_ref_ssp_en:1;
		unsigned reserved:18;
	} b;
} usb_r3_t;

typedef union usb_r4 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned p21_PORTRESET0:1;
		unsigned p21_SLEEPM0:1;
		unsigned mem_pd:2;
		unsigned p21_only:1;
		unsigned reserved:27;
	} b;
} usb_r4_t;

typedef union usb_r5 {
	/** raw register data */
	uint32_t d32;
	/** register bits */
	struct {
		unsigned iddig_sync:1;
		unsigned iddig_reg:1;
		unsigned iddig_cfg:2;
		unsigned iddig_en0:1;
		unsigned iddig_en1:1;
		unsigned iddig_curr:1;
		unsigned usb_iddig_irq:1;
		unsigned iddig_th:8;
		unsigned iddig_cnt:8;
		unsigned reserved:8;
    } b;
} usb_r5_t;

int usb2_phy_tuning(uint32_t phy2_pll_base, int port);
void set_usb_pll(uint32_t phy2_pll_base);
int usb_save_phy_dev (unsigned int number, struct phy *phy);
int usb2_phy_init (struct phy *phy);
void usb_device_mode_init(int phy_num);
#endif
