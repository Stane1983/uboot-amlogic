/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * arch/arm/include/asm/arch-t5/hdmirx_reg.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef HDMIRX_REG_H_
#define HDMIRX_REG_H_

/**
 * Bit field mask
 * @param m	width
 * @param n shift
 */
#define MSK(m, n)		(((1 << (m)) - 1) << (n))
/**
 * Bit mask
 * @param n shift
 */
#define _BIT(n)			MSK(1, (n))

#define HDMIRX_DWC_BASE_OFFSET	0xfe398000
#define HHI_BASE_ADDR	0xff63c000
#define CLK_CTRL_ADDR	0xfe000000

/* ------------------------------------- */
/* TOP-level wrapper registers addresses */
/* ------------------------------------- */

#define TOP_SW_RESET                     0x000
	#define HDCP22_TMDSCLK_EN		_BIT(3)
#define TOP_CLK_CNTL                     0x001
#define TOP_HPD_PWR5V                    0x002
#define TOP_PORT_SEL                     0x003
#define TOP_EDID_GEN_CNTL                0x004
#define TOP_EDID_ADDR_CEC                0x005
#define TOP_EDID_DATA_CEC_PORT01         0x006
#define TOP_EDID_DATA_CEC_PORT23         0x007
#define TOP_EDID_GEN_STAT                0x008
#define TOP_INTR_MASKN                   0x009
#define TOP_INTR_STAT                    0x00A
#define TOP_INTR_STAT_CLR                0x00B
#define TOP_VID_CNTL                     0x00C
#define TOP_VID_STAT                     0x00D
#define TOP_ACR_CNTL_STAT                0x00E
#define TOP_ACR_AUDFIFO                  0x00F
#define TOP_ARCTX_CNTL                   0x010
#define TOP_METER_HDMI_CNTL              0x011
#define TOP_METER_HDMI_STAT              0x012
#define TOP_VID_CNTL2                    0x013

/* hdmi2.0 new start */
#define TOP_MEM_PD                       0x014
#define TOP_EDID_RAM_OVR0                0x015
#define TOP_EDID_RAM_OVR0_DATA           0x016
#define TOP_EDID_RAM_OVR1                0x017
#define TOP_EDID_RAM_OVR1_DATA           0x018
#define TOP_EDID_RAM_OVR2                0x019
#define TOP_EDID_RAM_OVR2_DATA           0x01a
#define TOP_EDID_RAM_OVR3                0x01b
#define TOP_EDID_RAM_OVR3_DATA           0x01c
#define TOP_EDID_RAM_OVR4                0x01d
#define TOP_EDID_RAM_OVR4_DATA           0x01e
#define TOP_EDID_RAM_OVR5                0x01f
#define TOP_EDID_RAM_OVR5_DATA           0x020
#define TOP_EDID_RAM_OVR6                0x021
#define TOP_EDID_RAM_OVR6_DATA           0x022
#define TOP_EDID_RAM_OVR7                0x023
#define TOP_EDID_RAM_OVR7_DATA           0x024
#define TOP_EDID_GEN_STAT_B              0x025
#define TOP_EDID_GEN_STAT_C              0x026
#define TOP_EDID_GEN_STAT_D              0x027
/* tl1 */
#define TOP_CHAN_SWITCH_0				0x028
#define TOP_TMDS_ALIGN_CNTL0			0x029
#define TOP_TMDS_ALIGN_CNTL1			0x02a
#define TOP_TMDS_ALIGN_STAT				0x02b

/* GXTVBB/TXL/TXLX */
#define	TOP_ACR_CNTL2					 0x02a
/* Gxtvbb */
#define	TOP_INFILTER_GXTVBB				 0x02b
/* Gxtvbb */

#define	TOP_INFILTER_HDCP				 0x02C
#define	TOP_INFILTER_I2C0				 0x02D
#define	TOP_INFILTER_I2C1				 0x02E
#define	TOP_INFILTER_I2C2				 0x02F
#define	TOP_INFILTER_I2C3				 0x030
/* tl1 */
#define	TOP_PRBS_GEN					0x033
#define	TOP_PRBS_ANA_0					0x034
#define TOP_PRBS_ANA_1					0x035
#define	TOP_PRBS_ANA_STAT				0x036
#define	TOP_PRBS_ANA_BER_CH0			0x037
#define	TOP_PRBS_ANA_BER_CH1			0x038
#define	TOP_PRBS_ANA_BER_CH2			0x039
#define	TOP_METER_CABLE_CNTL			0x03a
#define	TOP_METER_CABLE_STAT			0x03b
#define	TOP_CHAN_SWITCH_1				0x03c
/* tl1 */
#define	TOP_AUDPLL_LOCK_FILTER			0x040

/* tl1 */
#define	TOP_CHAN01_ERRCNT				0x041
#define	TOP_CHAN2_ERRCNT				0x042
#define	TOP_TL1_ACR_CNTL2				0x043
#define	TOP_ACR_N_STAT					0x044
#define	TOP_ACR_CTS_STAT				0x045
#define	TOP_AUDMEAS_CTRL				0x046
#define	TOP_AUDMEAS_CYCLES_M1			0x047
#define	TOP_AUDMEAS_INTR_MASKN			0x048
#define	TOP_AUDMEAS_INTR_STAT			0x049
#define	TOP_AUDMEAS_REF_CYCLES_STAT0	0x04a
#define	TOP_AUDMEAS_REF_CYCLES_STAT1	0x04b
#define	TOP_HDCP22_BSOD					0x060

#define	TOP_SKP_CNTL_STAT				0x061
#define	TOP_NONCE_0						0x062
#define	TOP_NONCE_1						0x063
#define	TOP_NONCE_2						0x064
#define	TOP_NONCE_3						0x065
#define	TOP_PKF_0						0x066
#define	TOP_PKF_1						0x067
#define	TOP_PKF_2						0x068
#define	TOP_PKF_3						0x069
#define	TOP_DUK_0						0x06a
#define	TOP_DUK_1						0x06b
#define	TOP_DUK_2						0x06c
#define	TOP_DUK_3						0x06d
#define TOP_NSEC_SCRATCH				0x06e
#define	TOP_SEC_SCRATCH					0x06f
#define TOP_EDID_OFFSET					0x200

/* TL1 */
#define	TOP_EMP_DDR_START_A				0x070
#define	TOP_EMP_DDR_START_B				0x071
#define	TOP_EMP_DDR_FILTER				0x072
#define	TOP_EMP_CNTL_0					0x073
#define	TOP_EMP_CNTL_1					0x074
#define	TOP_EMP_CNTMAX					0x075
#define	TOP_EMP_ERR_STAT				0x076
#define	TOP_EMP_RCV_CNT_CUR				0x077
#define	TOP_EMP_RCV_CNT_BUF				0x078
#define	TOP_EMP_DDR_ADDR_CUR			0x079
#define	TOP_EMP_DDR_PTR_S_BUF			0x07a
#define	TOP_EMP_STAT_0					0x07b
#define	TOP_EMP_STAT_1					0x07c
#define TOP_AXI_CNTL_0					0x080
#define	TOP_AXI_ASYNC_HOLD_ESM			0x081
#define	TOP_AXI_ASYNC_HOLD_EMP			0x082
#define	TOP_AXI_STAT_0					0x083
#define	TOP_MISC_STAT0					0x084
#define TOP_EDID_ADDR_S					0x1000
#define TOP_EDID_ADDR_E					0x11ff

/* COR */
#define TOP_PHYIF_CNTL0					0x080

#define TOP_SECURE_INDEX                 0x0a0  /* 0x280 */
#define TOP_SECURE_DATA                  0x0a1  /* 0x284 */
#define TOP_SECURE_MODE                  0x0a2  /* 0x288 */

/* TM2 */
#define TOP_EDID_PORT2_ADDR_S			0x1200
#define TOP_EDID_PORT2_ADDR_E			0x13ff
#define TOP_EDID_PORT3_ADDR_S			0x1400
#define TOP_EDID_PORT3_ADDR_E			0x15ff

/* clk cntrl */
#define RX_CLK_CTRL			(0x4A << 2)
#define RX_CLK_CTRL1		(0x4B << 2)
#define RX_CLK_CTRL2		(0x4C << 2)
#define RX_CLK_CTRL3		(0x4D << 2)
#define CLKCTRL_SYS_CLK_EN0_REG2	(0x13 << 2)

#define TOP_BASE_OFFSET				0xfe398000

#define TOP_DONT_TOUCH0                  0x0fe
#define TOP_DONT_TOUCH1                  0x0ff

/** HPI Register */
#define HPI_REG_IRQ_STATUS				0x24
#define IRQ_STATUS_UPDATE_BIT			_BIT(3)
#define HPI_REG_EXCEPTION_STATUS		0x60
#define EXCEPTION_CODE					MSK(8, 1)
#define AUD_PLL_THRESHOLD	1000000

/* tl1 HIU related register */
#define HHI_HDMIRX_AXI_CLK_CNTL			(0xb8 << 2)

/* tl1 HIU apll register */
#define HHI_HDMIRX_APLL_CNTL0			(0xd2 << 2)/* 0x4C */
#define HHI_HDMIRX_APLL_CNTL1			(0xd3 << 2)/* 0x4D */
#define HHI_HDMIRX_APLL_CNTL2			(0xd4 << 2)/* 0x4E */
#define HHI_HDMIRX_APLL_CNTL3			(0xd5 << 2)/* 0x4F */
#define HHI_HDMIRX_APLL_CNTL4			(0xd6 << 2)/* 0x50 */

/* tl1 HIU PHY register */
#define HHI_HDMIRX_PHY_MISC_CNTL0		(0xd7 << 2)/*0x040*/
#define HHI_HDMIRX_PHY_MISC_CNTL1		(0xd8 << 2)/*0x041*/
#define HHI_HDMIRX_PHY_MISC_CNTL2		(0xe0 << 2)/*0x042*/
#define HHI_HDMIRX_PHY_MISC_CNTL3		(0xe1 << 2)/*0x043*/
#define HHI_HDMIRX_PHY_DCHA_CNTL0		(0xe2 << 2)/*0x045*/
#define HHI_HDMIRX_PHY_DCHA_CNTL1		(0xe3 << 2)/*0x046*/
#define HHI_HDMIRX_PHY_DCHA_CNTL2		(0xe4 << 2)/*0x047*/
#define HHI_HDMIRX_PHY_DCHD_CNTL0		(0xe5 << 2)/*0x048*/
#define HHI_HDMIRX_PHY_DCHD_CNTL1		(0xe6 << 2)/*0x049*/
#define HHI_HDMIRX_PHY_DCHD_CNTL2		(0xe7 << 2)/*0x04A*/
/*#define HHI_HDMIRX_PHY_MISC_STAT		(0xee << 2)*//*0x044*/
#define HHI_HDMIRX_PHY_DCHD_STAT		(0xef << 2)/*0x04B*/

#define TMDS_CLK_MIN			(24000UL)
#define TMDS_CLK_MAX			(340000UL)

/*t7/t3*/
#define PADCTRL_PIN_MUX_REGA                       ((0x000a  << 2) + 0xfe004000)
#define PADCTRL_PIN_MUX_REGB                       ((0x000b  << 2) + 0xfe004000)
#define PADCTRL_PIN_MUX_REGI                       ((0x0012  << 2) + 0xfe004000)
#define PADCTRL_PIN_MUX_REGJ                       ((0x0013  << 2) + 0xfe004000)
#endif
