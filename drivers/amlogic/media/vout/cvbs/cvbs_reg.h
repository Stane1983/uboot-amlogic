/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __CVBS_REG_H__
#define __CVBS_REG_H__
#include "cvbs.h"
#include "cvbs_dummy_reg.h"

// encoder registers
#define BUS_TYPE_CBUS	0
#define BUS_TYPE_HIU	1
#define BUS_TYPE_VCBUS	2

#define DAC0_CLK_SEL            28
#define DAC1_CLK_SEL            24
#define DAC2_CLK_SEL            20
#define VCLK2_XD_RST            17
#define VCLK2_XD_EN             16
#define ENCL_CLK_SEL            12
#define VCLK2_XD                 0
#define VCLK_XD0                 0
#define VCLK_CLK_IN_SEL         16
#define VCLK_EN0                19
#define VCLK_XD_EN              16
#define VCLK_DIV1_EN             0
#define VCLK_SOFT_RST           15

#define VCLK2_EN                19
#define VCLK2_CLK_IN_SEL        16
#define VCLK2_SOFT_RST          15
#define VCLK2_DIV12_EN           4
#define VCLK2_DIV6_EN            3
#define VCLK2_DIV4_EN            2
#define VCLK2_DIV2_EN            1
#define VCLK2_DIV1_EN            0

/* ********************************
 * register define
 * ********************************* */

#define CVBS_REG_OFFSET_VCBUS(reg)    (((reg) << 2))

#define CVBS_REG_ADDR(reg)            (reg + 0L)
#define CVBS_REG_ADDR_HIU(reg)        (reg + 0L)
#define CVBS_REG_ADDR_VCBUS(base, reg)      (base + CVBS_REG_OFFSET_VCBUS(reg))

/*********************************
 * ENCI START
 **********************************/
#define ENCI_VIDEO_MODE                            0x1b00
#define ENCI_VIDEO_MODE_ADV                        0x1b01
#define ENCI_VIDEO_FSC_ADJ                         0x1b02
#define ENCI_VIDEO_BRIGHT                          0x1b03
#define ENCI_VIDEO_CONT                            0x1b04
#define ENCI_VIDEO_SAT                             0x1b05
#define ENCI_VIDEO_HUE                             0x1b06
#define ENCI_VIDEO_SCH                             0x1b07
#define ENCI_SYNC_MODE                             0x1b08
#define ENCI_SYNC_CTRL                             0x1b09
#define ENCI_SYNC_HSO_BEGIN                        0x1b0a
#define ENCI_SYNC_HSO_END                          0x1b0b
#define ENCI_SYNC_VSO_EVN                          5
#define ENCI_SYNC_VSO_ODD                          0x1b0d
#define ENCI_SYNC_VSO_EVNLN                        0x1b0e
#define ENCI_SYNC_VSO_ODDLN                        0x1b0f
#define ENCI_SYNC_HOFFST                           0x1b10
#define ENCI_SYNC_VOFFST                           0x1b11
#define ENCI_SYNC_ADJ                              0x1b12
#define ENCI_RGB_SETTING                           0x1b13

#define ENCI_DE_H_BEGIN                            0x1b16
#define ENCI_DE_H_END                              0x1b17
#define ENCI_DE_V_BEGIN_EVEN                       0x1b18
#define ENCI_DE_V_END_EVEN                         0x1b19
#define ENCI_DE_V_BEGIN_ODD                        0x1b1a
#define ENCI_DE_V_END_ODD                          0x1b1b
#define ENCI_VBI_SETTING                           0x1b20
#define ENCI_VBI_CCDT_EVN                          0x1b21
#define ENCI_VBI_CCDT_ODD                          0x1b22
#define ENCI_VBI_CC525_LN                          0x1b23
#define ENCI_VBI_CC625_LN                          0x1b24
#define ENCI_VBI_WSSDT                             0x1b25
#define ENCI_VBI_WSS_LN                            0x1b26
#define ENCI_VBI_CGMSDT_L                          0x1b27
#define ENCI_VBI_CGMSDT_H                          0x1b28
#define ENCI_VBI_CGMS_LN                           0x1b29
#define ENCI_VBI_TTX_HTIME                         0x1b2a
#define ENCI_VBI_TTX_LN                            0x1b2b
#define ENCI_VBI_TTXDT0                            0x1b2c
#define ENCI_VBI_TTXDT1                            0x1b2d
#define ENCI_VBI_TTXDT2                            0x1b2e
#define ENCI_VBI_TTXDT3                            0x1b2f
#define ENCI_MACV_N0                               0x1b30
#define ENCI_MACV_N1                               0x1b31
#define ENCI_MACV_N2                               0x1b32
#define ENCI_MACV_N3                               0x1b33
#define ENCI_MACV_N4                               0x1b34
#define ENCI_MACV_N5                               0x1b35
#define ENCI_MACV_N6                               0x1b36
#define ENCI_MACV_N7                               0x1b37
#define ENCI_MACV_N8                               0x1b38
#define ENCI_MACV_N9                               0x1b39
#define ENCI_MACV_N10                              0x1b3a
#define ENCI_MACV_N11                              0x1b3b
#define ENCI_MACV_N12                              0x1b3c
#define ENCI_MACV_N13                              0x1b3d
#define ENCI_MACV_N14                              0x1b3e
#define ENCI_MACV_N15                              0x1b3f
#define ENCI_MACV_N16                              0x1b40
#define ENCI_MACV_N17                              0x1b41
#define ENCI_MACV_N18                              0x1b42
#define ENCI_MACV_N19                              0x1b43
#define ENCI_MACV_N20                              0x1b44
#define ENCI_MACV_N21                              0x1b45
#define ENCI_MACV_N22                              0x1b46

#define ENCI_DBG_PX_RST                            0x1b48
#define ENCI_DBG_FLDLN_RST                         0x1b49
#define ENCI_DBG_PX_INT                            0x1b4a
#define ENCI_DBG_FLDLN_INT                         0x1b4b
#define ENCI_DBG_MAXPX                             0x1b4c
#define ENCI_DBG_MAXLN                             0x1b4d
#define ENCI_MACV_MAX_AMP                          0x1b50
#define ENCI_MACV_PULSE_LO                         0x1b51
#define ENCI_MACV_PULSE_HI                         0x1b52
#define ENCI_MACV_BKP_MAX                          0x1b53
#define ENCI_CFILT_CTRL                            0x1b54
#define ENCI_CFILT7                                0x1b55
#define ENCI_YC_DELAY                              0x1b56
#define ENCI_VIDEO_EN                              0x1b57

#define ENCI_DVI_HSO_BEGIN                         0x1c00
#define ENCI_DVI_HSO_END                           0x1c01
#define ENCI_DVI_VSO_BLINE_EVN                     0x1c02
#define ENCI_DVI_VSO_BLINE_ODD                     0x1c03
#define ENCI_DVI_VSO_ELINE_EVN                     0x1c04
#define ENCI_DVI_VSO_ELINE_ODD                     0x1c05
#define ENCI_DVI_VSO_BEGIN_EVN                     0x1c06
#define ENCI_DVI_VSO_BEGIN_ODD                     0x1c07
#define ENCI_DVI_VSO_END_EVN                       0x1c08
#define ENCI_DVI_VSO_END_ODD                       0x1c09

#define ENCI_CFILT_CTRL2                           0x1c0a
#define ENCI_DACSEL_0                              0x1c0b
#define ENCI_DACSEL_1                              0x1c0c
#define ENCP_DACSEL_0                              0x1c0d
#define ENCP_DACSEL_1                              0x1c0e
#define ENCP_MAX_LINE_SWITCH_POINT                 0x1c0f
#define ENCI_TST_EN                                0x1c10
#define ENCI_TST_MDSEL                             0x1c11
#define ENCI_TST_Y                                 0x1c12
#define ENCI_TST_CB                                0x1c13
#define ENCI_TST_CR                                0x1c14
#define ENCI_TST_CLRBAR_STRT                       0x1c15
#define ENCI_TST_CLRBAR_WIDTH                      0x1c16
#define ENCI_TST_VDCNT_STSET                       0x1c17

#define ENCI_VFIFO2VD_CTL                          0x1c18

#define ENCI_VFIFO2VD_PIXEL_START                  0x1c19

#define ENCI_VFIFO2VD_PIXEL_END                    0x1c1a

#define ENCI_VFIFO2VD_LINE_TOP_START               0x1c1b

#define ENCI_VFIFO2VD_LINE_TOP_END                 0x1c1c

#define ENCI_VFIFO2VD_LINE_BOT_START               0x1c1d

#define ENCI_VFIFO2VD_LINE_BOT_END                 0x1c1e
#define ENCI_VFIFO2VD_CTL2                         0x1c1f

#define ENCP_DVI_HSO_BEGIN                         0x1c30
#define ENCP_DVI_HSO_END                           0x1c31
#define ENCP_DVI_VSO_BLINE_EVN                     0x1c32
#define ENCP_DVI_VSO_BLINE_ODD                     0x1c33
#define ENCP_DVI_VSO_ELINE_EVN                     0x1c34
#define ENCP_DVI_VSO_ELINE_ODD                     0x1c35
#define ENCP_DVI_VSO_BEGIN_EVN                     0x1c36
#define ENCP_DVI_VSO_BEGIN_ODD                     0x1c37
#define ENCP_DVI_VSO_END_EVN                       0x1c38
#define ENCP_DVI_VSO_END_ODD                       0x1c39
#define ENCP_DE_H_BEGIN                            0x1c3a
#define ENCP_DE_H_END                              0x1c3b
#define ENCP_DE_V_BEGIN_EVEN                       0x1c3c
#define ENCP_DE_V_END_EVEN                         0x1c3d
#define ENCP_DE_V_BEGIN_ODD                        0x1c3e
#define ENCP_DE_V_END_ODD                          0x1c3f

#define ENCI_SYNC_LINE_LENGTH                      0x1c40

#define ENCI_SYNC_PIXEL_EN                         0x1c41

#define ENCI_SYNC_TO_LINE_EN                       0x1c42

#define ENCI_SYNC_TO_PIXEL                         0x1c43

#define ENCP_SYNC_LINE_LENGTH                      0x1c44

#define ENCP_SYNC_PIXEL_EN                         0x1c45

#define ENCP_SYNC_TO_LINE_EN                       0x1c46

#define ENCP_SYNC_TO_PIXEL                         0x1c47

#define ENCT_SYNC_LINE_LENGTH                      0x1c48

#define ENCT_SYNC_PIXEL_EN                         0x1c49

#define ENCT_SYNC_TO_LINE_EN                       0x1c4a

#define ENCT_SYNC_TO_PIXEL                         0x1c4b

#define ENCL_SYNC_LINE_LENGTH                      0x1c4c

#define ENCL_SYNC_PIXEL_EN                         0x1c4d

#define ENCL_SYNC_TO_LINE_EN                       0x1c4e

#define ENCL_SYNC_TO_PIXEL                         0x1c4f

#define ENCP_VFIFO2VD_CTL2                         0x1c50

#define VENC_DVI_SETTING_MORE                      0x1c51
#define VENC_VDAC_DAC4_FILT_CTRL0                  0x1c54
#define VENC_VDAC_DAC4_FILT_CTRL1                  0x1c55
#define VENC_VDAC_DAC5_FILT_CTRL0                  0x1c56
#define VENC_VDAC_DAC5_FILT_CTRL1                  0x1c57

#define VENC_VDAC_DAC0_FILT_CTRL0                  0x1c58

#define VENC_VDAC_DAC0_FILT_CTRL1                  0x1c59

#define VENC_VDAC_DAC1_FILT_CTRL0                  0x1c5a

#define VENC_VDAC_DAC1_FILT_CTRL1                  0x1c5b

#define VENC_VDAC_DAC2_FILT_CTRL0                  0x1c5c

#define VENC_VDAC_DAC2_FILT_CTRL1                  0x1c5d

#define VENC_VDAC_DAC3_FILT_CTRL0                  0x1c5e

#define VENC_VDAC_DAC3_FILT_CTRL1                  0x1c5f

#define ENCP_VFIFO2VD_CTL                          0x1b58

#define ENCP_VFIFO2VD_PIXEL_START                  0x1b59

#define ENCP_VFIFO2VD_PIXEL_END                    0x1b5a

#define ENCP_VFIFO2VD_LINE_TOP_START               0x1b5b

#define ENCP_VFIFO2VD_LINE_TOP_END                 0x1b5c

#define ENCP_VFIFO2VD_LINE_BOT_START               0x1b5d

#define ENCP_VFIFO2VD_LINE_BOT_END                 0x1b5e

#define VENC_SYNC_ROUTE                            0x1b60
#define VENC_VIDEO_EXSRC                           0x1b61
#define VENC_DVI_SETTING                           0x1b62
#define VENC_C656_CTRL                             0x1b63
#define VENC_UPSAMPLE_CTRL0                        0x1b64
#define VENC_UPSAMPLE_CTRL1                        0x1b65
#define VENC_UPSAMPLE_CTRL2                        0x1b66

#define TCON_INVERT_CTL                            0x1b67
#define VENC_VIDEO_PROG_MODE                       0x1b68

#define VENC_ENCI_LINE                             0x1b69
#define VENC_ENCI_PIXEL                            0x1b6a
#define VENC_ENCP_LINE                             0x1b6b
#define VENC_ENCP_PIXEL                            0x1b6c

#define VENC_STATA                                 0x1b6d

#define VENC_INTCTRL                               0x1b6e
#define VENC_INTFLAG                               0x1b6f

#define VENC_VIDEO_TST_EN                          0x1b70
#define VENC_VIDEO_TST_MDSEL                       0x1b71
#define VENC_VIDEO_TST_Y                           0x1b72
#define VENC_VIDEO_TST_CB                          0x1b73
#define VENC_VIDEO_TST_CR                          0x1b74
#define VENC_VIDEO_TST_CLRBAR_STRT                 0x1b75
#define VENC_VIDEO_TST_CLRBAR_WIDTH                0x1b76
#define VENC_VIDEO_TST_VDCNT_STSET                 0x1b77

#define VENC_VDAC_DACSEL0                          0x1b78
#define VENC_VDAC_DACSEL1                          0x1b79
#define VENC_VDAC_DACSEL2                          0x1b7a
#define VENC_VDAC_DACSEL3                          0x1b7b
#define VENC_VDAC_DACSEL4                          0x1b7c
#define VENC_VDAC_DACSEL5                          0x1b7d
#define VENC_VDAC_SETTING                          0x1b7e
#define VENC_VDAC_TST_VAL                          0x1b7f
#define VENC_VDAC_DAC0_GAINCTRL                    0x1bf0
#define VENC_VDAC_DAC0_OFFSET                      0x1bf1
#define VENC_VDAC_DAC1_GAINCTRL                    0x1bf2
#define VENC_VDAC_DAC1_OFFSET                      0x1bf3
#define VENC_VDAC_DAC2_GAINCTRL                    0x1bf4
#define VENC_VDAC_DAC2_OFFSET                      0x1bf5
#define VENC_VDAC_DAC3_GAINCTRL                    0x1bf6
#define VENC_VDAC_DAC3_OFFSET                      0x1bf7
#define VENC_VDAC_DAC4_GAINCTRL                    0x1bf8
#define VENC_VDAC_DAC4_OFFSET                      0x1bf9
#define VENC_VDAC_DAC5_GAINCTRL                    0x1bfa
#define VENC_VDAC_DAC5_OFFSET                      0x1bfb
#define VENC_VDAC_FIFO_CTRL                        0x1bfc
#define ENCL_TCON_INVERT_CTL                       0x1bfd

/*********************************
 *ENCI END
 *********************************
 */

/* ********************************
 * register access api
 * ********************************* */
static inline int cvbs_write_hiu(unsigned int addr, unsigned int value)
{
	*(volatile unsigned int *)CVBS_REG_ADDR_HIU(addr) = (value);
	return 0;
}

static inline int cvbs_read_hiu(unsigned int addr)
{
	unsigned int val = 0;

	val = *(volatile unsigned int *)(CVBS_REG_ADDR_HIU(addr));
	return val;
}

static inline int cvbs_set_hiu_bits(unsigned int addr, unsigned int value, unsigned int start, unsigned int len)
{
	cvbs_write_hiu(addr, ((cvbs_read_hiu(addr) &
			~(((1L << (len))-1) << (start))) |
			(((value)&((1L<<(len))-1)) << (start))));
	return 0;
}

static inline int cvbs_get_hiu_bits(unsigned int addr, unsigned int start, unsigned int len)
{
	return (cvbs_read_hiu(addr) >> (start)) & ((1L << (len)) - 1);
}

static inline  unsigned int cvbs_read_vcbus(unsigned int addr_offset)
{
	unsigned int val = 0;
	struct cvbs_drv_s *cvbs_drv = get_cvbs_drv();

	if (addr_offset > 0x10000)
		val = *(volatile unsigned int *)(CVBS_REG_ADDR(addr_offset));
	else if (cvbs_drv->data->chip_type >= CVBS_CHIP_SC2)
		val = *(volatile unsigned int *)(CVBS_REG_ADDR_VCBUS(REG_SC2_BASE_VCBUS,
									addr_offset));
	else
		val = *(volatile unsigned int *)(CVBS_REG_ADDR_VCBUS(REG_BASE_VCBUS, addr_offset));
	return val;
}

static inline  int cvbs_write_vcbus(unsigned int addr_offset, unsigned int value)
{
	struct cvbs_drv_s *cvbs_drv = get_cvbs_drv();

	if (addr_offset > 0x10000)
		*(volatile unsigned int *)CVBS_REG_ADDR(addr_offset) = (value);
	else if (cvbs_drv->data->chip_type >= CVBS_CHIP_SC2)
		*(volatile unsigned int *)CVBS_REG_ADDR_VCBUS(REG_SC2_BASE_VCBUS, addr_offset) =
									(value);
	else
		*(volatile unsigned int *)CVBS_REG_ADDR_VCBUS(REG_BASE_VCBUS, addr_offset) =
									(value);
	return 0;
}

static inline  int cvbs_set_vcbus_bits(unsigned int addr_offset, unsigned int value, unsigned int start, unsigned int len)
{
	cvbs_write_vcbus(addr_offset, ((cvbs_read_vcbus(addr_offset) &
		~(((1L << (len))-1) << (start))) |
		(((value)&((1L<<(len))-1)) << (start))));
	return 0;
}

#endif
