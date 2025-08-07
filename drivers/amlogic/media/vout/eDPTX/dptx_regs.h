/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DPTX_REGS_H_
#define __DPTX_REGS_H_
#include <asm/arch/cpu.h>
#include <asm/arch/io.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/vpu_regs.h>
#include "./dptx_dummy_reg.h"

/* base & offset */
#define REG_OFFSET_CBUS(reg)  ((reg) << 2)
#define REG_OFFSET_VCBUS(reg) ((reg) << 2)

/* memory mapping */
#define REG_ADDR_AOBUS(reg)   ((reg) + 0L)
#define REG_ADDR_PERIPHS(reg) ((reg) + 0L)
#define REG_ADDR_CBUS(reg)    (((reg) > 0x10000) ? (reg0) : (REG_BASE_CBUS + REG_OFFSET_CBUS(reg)))
#define REG_ADDR_RESET(reg)   ((reg) + 0L)
#define REG_ADDR_HIU(reg)     ((reg) + 0L)
#define REG_ADDR_COMBO(reg)   ((reg) + 0L)
#define REG_ADDR_VCBUS(reg)   (((reg) > 0x10000) ? (reg) : (REG_BASE_VCBUS + REG_OFFSET_VCBUS(reg)))

#ifndef EDPTX0_A_BASE
#define EDPTX0_A_BASE          EDPTX0_BASE
#endif

#ifndef EDPTX0_B_BASE
#define EDPTX0_B_BASE          0
#endif

#ifndef EDPTX1_A_BASE
#define EDPTX1_A_BASE          EDPTX1_BASE
#endif

#ifndef EDPTX1_B_BASE
#define EDPTX1_B_BASE          0
#endif

//#define HHI_VIID_CLK_DIV     0x4a
#define DAC0_CLK_SEL           28
#define DAC1_CLK_SEL           24
#define DAC2_CLK_SEL           20
#define VCLK2_XD_RST           17
#define VCLK2_XD_EN            16
#define ENCL_CLK_SEL           12
#define VCLK2_XD                0

//#define HHI_VIID_CLK_CNTL    0x4b
#define VCLK2_EN               19
#define VCLK2_CLK_IN_SEL       16
#define VCLK2_SOFT_RST         15
#define VCLK2_DIV12_EN          4
#define VCLK2_DIV6_EN           3
#define VCLK2_DIV4_EN           2
#define VCLK2_DIV2_EN           1
#define VCLK2_DIV1_EN           0

//#define HHI_VIID_DIVIDER_CNTL 0x4c
#define DIV_CLK_IN_EN          16
#define DIV_CLK_SEL            15
#define DIV_POST_TCNT          12
#define DIV_LVDS_CLK_EN        11
#define DIV_LVDS_DIV2          10
#define DIV_POST_SEL            8
#define DIV_POST_SOFT_RST       7
#define DIV_PRE_SEL             4
#define DIV_PRE_SOFT_RST        3
#define DIV_POST_RST            1
#define DIV_PRE_RST             0

//#define HHI_VID_CLK_DIV        0x59
#define ENCI_CLK_SEL           28
#define ENCP_CLK_SEL           24
#define ENCT_CLK_SEL           20
#define VCLK_XD_RST            17
#define VCLK_XD_EN             16
#define ENCL_CLK_SEL           12
#define VCLK_XD1                8
#define VCLK_XD0                0

//#define HHI_VID_CLK_CNTL2        0x65
#define HDMI_TX_PIXEL_GATE_VCLK  5
#define VDAC_GATE_VCLK           4
#define ENCL_GATE_VCLK           3
#define ENCP_GATE_VCLK           2
#define ENCT_GATE_VCLK           1
#define ENCI_GATE_VCLK           0

static inline uint32_t dptx_combo_dphy_read(uint32_t reg)
{
	uint32_t val;

	val = *(volatile uint32_t *)(REG_ADDR_COMBO(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
};

static inline void dptx_combo_dphy_write(uint32_t reg, uint32_t val)
{
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	*(volatile uint32_t *)REG_ADDR_COMBO(reg) = (val);
};

static inline void dptx_combo_dphy_setb(uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_combo_dphy_write(reg, ((dptx_combo_dphy_read(reg) &
		~(((1L << (len)) - 1) << (start))) | (((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_combo_dphy_getb(uint32_t reg, uint8_t start, uint8_t len)
{
	return (dptx_combo_dphy_read(reg) >> (start)) & ((1L << (len)) - 1);
}

static inline uint32_t dptx_clk_read(uint32_t reg)
{
	uint32_t val;

	val = *(volatile uint32_t *)(REG_ADDR_HIU(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
};

static inline void dptx_clk_write(uint32_t reg, uint32_t val)
{
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	*(volatile uint32_t *)REG_ADDR_HIU(reg) = (val);
};

static inline void dptx_clk_setb(uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_clk_write(reg, ((dptx_clk_read(reg) &
		~(((1L << (len)) - 1) << (start))) | (((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_clk_getb(uint32_t reg, uint8_t start, uint8_t len)
{
	return (dptx_clk_read(reg) >> (start)) & ((1L << (len)) - 1);
}

static inline uint32_t dptx_ana_read(uint32_t reg)
{
	uint32_t val;

	val = *(volatile uint32_t *)(REG_ADDR_HIU(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
};

static inline void dptx_ana_write(uint32_t reg, uint32_t val)
{
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	*(volatile uint32_t *)REG_ADDR_HIU(reg) = (val);
};

static inline void dptx_ana_setb(uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_ana_write(reg, ((dptx_ana_read(reg) &
		~(((1L << (len)) - 1) << (start))) | (((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_ana_getb(uint32_t reg, uint8_t start, uint8_t len)
{
	return (dptx_ana_read(reg) >> (start)) & ((1L << (len)) - 1);
}

static inline uint32_t dptx_reset_read(uint32_t reg)
{
	uint32_t val;

	val = (*(volatile uint32_t *)REG_ADDR_RESET(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
};

static inline void dptx_reset_write(uint32_t reg, uint32_t val)
{
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	*(volatile uint32_t *)REG_ADDR_RESET(reg) = (val);
};

static inline void dptx_reset_setb(uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_reset_write(reg, ((dptx_reset_read(reg) &
		~(((1L << (len)) - 1) << (start))) | (((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_vcbus_read(uint32_t reg)
{
	uint32_t val;

	val = (*(volatile uint32_t *)REG_ADDR_VCBUS(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
}

static inline void dptx_vcbus_write(uint32_t reg, uint32_t val)
{
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	(*(volatile uint32_t *)REG_ADDR_VCBUS(reg)) = (val);
}

static inline void dptx_vcbus_setb(uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_vcbus_write(reg, ((dptx_vcbus_read(reg) &
		~(((1L << (len)) - 1) << (start))) | (((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_vcbus_getb(uint32_t reg, uint8_t start, uint8_t len)
{
	return (dptx_vcbus_read(reg) >> (start)) & ((1L << (len)) - 1);
}

static inline uint32_t dptx_reg_read(struct dptx_drv_s *dptx, uint8_t port, uint32_t reg)
{
	unsigned long paddr;
	unsigned int val;

	if (dptx->idx == 0 && port == 0)
		paddr = EDPTX0_A_BASE + reg;
	else if (dptx->idx == 0 && port == 1)
		paddr = EDPTX0_B_BASE + reg;
	else if (dptx->idx == 1 && port == 0)
		paddr = EDPTX1_A_BASE + reg;
	else if (dptx->idx == 1 && port == 1)
		paddr = EDPTX1_B_BASE + reg;
	else
		return 0;

	val = *(volatile unsigned int *)paddr;
	if (dptx_print_level >= LOG_A)
		printf("%s: [%d][0x%04x]=0x%08x\n", __func__, dptx->idx, reg, val);

	return val;
}

static inline void dptx_reg_write(struct dptx_drv_s *dptx, uint8_t port,
					uint32_t reg, uint32_t val)
{
	unsigned long paddr;

	if (dptx->idx == 0 && port == 0)
		paddr = EDPTX0_A_BASE + reg;
	else if (dptx->idx == 0 && port == 1)
		paddr = EDPTX0_B_BASE + reg;
	else if (dptx->idx == 1 && port == 0)
		paddr = EDPTX1_A_BASE + reg;
	else if (dptx->idx == 1 && port == 1)
		paddr = EDPTX1_B_BASE + reg;
	else
		return;

	*(volatile uint32_t *)paddr = (val);
	if (dptx_print_level >= LOG_A)
		printf("%s: [%d][0x%04x]=0x%08x\n", __func__, dptx->idx, reg, val);
}

static inline void dptx_reg_setb(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t reg, uint32_t val, uint8_t start, uint8_t len)
{
	dptx_reg_write(dptx, port, reg,
		((dptx_reg_read(dptx, port, reg) & ~(((1L << (len)) - 1) << (start))) |
		(((val) & ((1L << (len)) - 1)) << (start))));
}

static inline uint32_t dptx_reg_getb(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t reg, uint8_t start, uint8_t len)
{
	return (dptx_reg_read(dptx, port, reg) >> (start)) & ((1L << (len)) - 1);
}

static inline unsigned int dptx_periphs_read(unsigned int reg)
{
	unsigned int val;

	val = (*(volatile unsigned int *)REG_ADDR_PERIPHS(reg));
	if (dptx_print_level >= LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);

	return val;
};

static inline void dptx_periphs_write(unsigned int reg, unsigned int val)
{
	if (dptx_print_level > LOG_A)
		printf("%s: [0x%08x]=0x%08x\n", __func__, reg, val);
	*(volatile unsigned int *)REG_ADDR_PERIPHS(reg) = (val);
};

static inline void dptx_periphs_setb(unsigned int reg, unsigned int val,
		unsigned int start, unsigned int len)
{
	dptx_periphs_write(reg, ((dptx_periphs_read(reg) &
			~(((1L << (len)) - 1) << (start))) |
			(((val) & ((1L << (len)) - 1)) << (start))));
}

static inline unsigned int dptx_periphs_getb(unsigned int reg,
		unsigned int start, unsigned int len)
{
	return (dptx_periphs_read(reg) & (((1L << (len)) - 1) << (start)));
}

#endif
