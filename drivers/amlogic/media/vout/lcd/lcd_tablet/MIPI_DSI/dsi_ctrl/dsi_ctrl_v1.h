/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef DSI_LEGACY_H
#define DSI_LEGACY_H

/*  MIPI DSI/VENC Color Format Definitions */
#define MIPI_DSI_VENC_COLOR_30B   0x0
#define MIPI_DSI_VENC_COLOR_24B   0x1
#define MIPI_DSI_VENC_COLOR_18B   0x2
#define MIPI_DSI_VENC_COLOR_16B   0x3

#define COLOR_16BIT_CFG_1         0x0
#define COLOR_16BIT_CFG_2         0x1
#define COLOR_16BIT_CFG_3         0x2
#define COLOR_18BIT_CFG_1         0x3
#define COLOR_18BIT_CFG_2         0x4
#define COLOR_24BIT               0x5
#define COLOR_20BIT_LOOSE         0x6
#define COLOR_24_BIT_YCBCR        0x7
#define COLOR_16BIT_YCBCR         0x8
#define COLOR_30BIT               0x9
#define COLOR_36BIT               0xa
#define COLOR_12BIT               0xb
#define COLOR_RGB_111             0xc
#define COLOR_RGB_332             0xd
#define COLOR_RGB_444             0xe

/*  MIPI DSI Relative REGISTERs Definitions */
/* For MIPI_DSI_TOP_CNTL */
#define BIT_DPI_COLOR_MODE         20
#define BIT_IN_COLOR_MODE          16
#define BIT_CHROMA_SUBSAMPLE       14
#define BIT_COMP2_SEL              12
#define BIT_COMP1_SEL              10
#define BIT_COMP0_SEL               8
#define BIT_DE_POL                  6
#define BIT_HSYNC_POL               5
#define BIT_VSYNC_POL               4
#define BIT_DPICOLORM               3
#define BIT_DPISHUTDN               2
#define BIT_EDPITE_INTR_PULSE       1
#define BIT_ERR_INTR_PULSE          0

/* For MIPI_DSI_DWC_CLKMGR_CFG_OS */
#define BIT_TO_CLK_DIV              8
#define BIT_TX_ESC_CLK_DIV          0

/* For MIPI_DSI_DWC_PCKHDL_CFG_OS */
#define BIT_CRC_RX_EN               4
#define BIT_ECC_RX_EN               3
#define BIT_BTA_EN                  2
#define BIT_EOTP_RX_EN              1
#define BIT_EOTP_TX_EN              0

/* For MIPI_DSI_DWC_VID_MODE_CFG_OS */
#define BIT_LP_CMD_EN              15
#define BIT_FRAME_BTA_ACK_EN       14
#define BIT_LP_HFP_EN              13
#define BIT_LP_HBP_EN              12
#define BIT_LP_VACT_EN             11
#define BIT_LP_VFP_EN              10
#define BIT_LP_VBP_EN               9
#define BIT_LP_VSA_EN               8
#define BIT_VID_MODE_TYPE           0

/* For MIPI_DSI_DWC_PHY_STATUS_OS */
#define BIT_PHY_ULPSACTIVENOT3LANE 12
#define BIT_PHY_STOPSTATE3LANE     11
#define BIT_PHY_ULPSACTIVENOT2LANE 10
#define BIT_PHY_STOPSTATE2LANE      9
#define BIT_PHY_ULPSACTIVENOT1LANE  8
#define BIT_PHY_STOPSTATE1LANE      7
#define BIT_PHY_RXULPSESC0LANE      6
#define BIT_PHY_ULPSACTIVENOT0LANE  5
#define BIT_PHY_STOPSTATE0LANE      4
#define BIT_PHY_ULPSACTIVENOTCLK    3
#define BIT_PHY_STOPSTATECLKLANE    2
#define BIT_PHY_DIRECTION           1
#define BIT_PHY_LOCK                0

/* For MIPI_DSI_DWC_PHY_IF_CFG_OS */
#define BIT_PHY_STOP_WAIT_TIME      8
#define BIT_N_LANES                 0

/* For MIPI_DSI_DWC_DPI_COLOR_CODING_OS */
#define BIT_LOOSELY18_EN            8
#define BIT_DPI_COLOR_CODING        0

/* For MIPI_DSI_DWC_GEN_HDR_OS */
#define BIT_GEN_WC_MSBYTE          16
#define BIT_GEN_WC_LSBYTE           8
#define BIT_GEN_VC                  6
#define BIT_GEN_DT                  0

/* For MIPI_DSI_DWC_LPCLK_CTRL_OS */
#define BIT_AUTOCLKLANE_CTRL        1
#define BIT_TXREQUESTCLKHS          0

/* For MIPI_DSI_DWC_DPI_CFG_POL_OS */
#define BIT_COLORM_ACTIVE_LOW       4
#define BIT_SHUTD_ACTIVE_LOW        3
#define BIT_HSYNC_ACTIVE_LOW        2
#define BIT_VSYNC_ACTIVE_LOW        1
#define BIT_DATAEN_ACTIVE_LOW       0

/* For MIPI_DSI_DWC_CMD_MODE_CFG_OS */
#define BIT_MAX_RD_PKT_SIZE        24
#define BIT_DCS_LW_TX              19
#define BIT_DCS_SR_0P_TX           18
#define BIT_DCS_SW_1P_TX           17
#define BIT_DCS_SW_0P_TX           16
#define BIT_GEN_LW_TX              14
#define BIT_GEN_SR_2P_TX           13
#define BIT_GEN_SR_1P_TX           12
#define BIT_GEN_SR_0P_TX           11
#define BIT_GEN_SW_2P_TX           10
#define BIT_GEN_SW_1P_TX            9
#define BIT_GEN_SW_0P_TX            8
#define BIT_ACK_RQST_EN             1
#define BIT_TEAR_FX_EN              0

/* For MIPI_DSI_DWC_CMD_PKT_STATUS_OS */
/* For DBI no use full */
#define BIT_DBI_RD_CMD_BUSY        14
#define BIT_DBI_PLD_R_FULL         13
#define BIT_DBI_PLD_R_EMPTY        12
#define BIT_DBI_PLD_W_FULL         11
#define BIT_DBI_PLD_W_EMPTY        10
#define BIT_DBI_CMD_FULL            9
#define BIT_DBI_CMD_EMPTY           8
/* For Generic interface */
#define BIT_GEN_RD_CMD_BUSY         6
#define BIT_GEN_PLD_R_FULL          5
#define BIT_GEN_PLD_R_EMPTY         4
#define BIT_GEN_PLD_W_FULL          3
#define BIT_GEN_PLD_W_EMPTY         2
#define BIT_GEN_CMD_FULL            1
#define BIT_GEN_CMD_EMPTY           0

/* For MIPI_DSI_TOP_MEAS_CNTL */
#define BIT_CNTL_MEAS_VSYNC        10 //measure vsync control
#define BIT_EDPITE_MEAS_EN          9 //tear measure enable
#define BIT_EDPITE_ACCUM_MEAS_EN    8 //not clear the counter
#define BIT_EDPITE_VSYNC_SPAN       0

/* For MIPI_DSI_TOP_STAT */
#define BIT_STAT_EDPIHALT          31  //signal from halt
#define BIT_STAT_TE_LINE           16  //line number when edpite pulse
#define BIT_STAT_TE_PIXEL           0  //pixel number when edpite pulse

/* For MIPI_DSI_TOP_INTR_CNTL_STAT */
/* State/Clear for pic_eof */
#define BIT_STAT_CLR_DWC_PIC_EOF   21
/* State/Clear for de_fall */
#define BIT_STAT_CLR_DWC_DE_FALL   20
/* State/Clear for de_rise */
#define BIT_STAT_CLR_DWC_DE_RISE   19
/* State/Clear for vs_fall */
#define BIT_STAT_CLR_DWC_VS_FALL   18
/* State/Clear for vs_rise */
#define BIT_STAT_CLR_DWC_VS_RISE   17
/* State/Clear for edpite */
#define BIT_STAT_CLR_DWC_EDPITE    16
/* end of picture */
#define BIT_PIC_EOF                 5
/* data enable fall */
#define BIT_DE_FALL                 4
/* data enable rise */
#define BIT_DE_RISE                 3
/* vsync fall */
#define BIT_VS_FALL                 2
/* vsync rise */
#define BIT_VS_RISE                 1
/* edpite int enable */
#define BIT_EDPITE_INT_EN           0

/* For MIPI_DSI_TOP_MEAS_CNTL */
#define BIT_VSYNC_MEAS_EN          19  //sync measure enable
#define BIT_VSYNC_ACCUM_MEAS_EN    18  //vsync accumulate measure
#define BIT_VSYNC_SPAN             10  //vsync span
#define BIT_TE_MEAS_EN              9  //tearing measure enable
#define BIT_TE_ACCUM_MEAS_EN        8  //tearing accumulate measure
#define BIT_TE_SPAN                 0  //tearing span

/* For MIPI_DSI_DWC_INT_ST0_OS */
#define BIT_DPHY_ERR_4             20  //LP1 contention error from lane0
#define BIT_DPHY_ERR_3             19  //LP0 contention error from lane0
#define BIT_DPHY_ERR_2             18  //ErrControl error from lane0
#define BIT_DPHY_ERR_1             17  //ErrSyncEsc error from lane0
#define BIT_DPHY_ERR_0             16  //ErrEsc escape error lane0
#define BIT_ACK_ERR_15             15
#define BIT_ACK_ERR_14             14
#define BIT_ACK_ERR_13             13
#define BIT_ACK_ERR_12             12
#define BIT_ACK_ERR_11             11
#define BIT_ACK_ERR_10             10
#define BIT_ACK_ERR_9               9
#define BIT_ACK_ERR_8               8
#define BIT_ACK_ERR_7               7
#define BIT_ACK_ERR_6               6
#define BIT_ACK_ERR_5               5
#define BIT_ACK_ERR_4               4
#define BIT_ACK_ERR_3               3
#define BIT_ACK_ERR_2               2
#define BIT_ACK_ERR_1               1
#define BIT_ACK_ERR_0               0

/* operation mode */
#define MIPI_DSI_OPERATION_MODE_VIDEO      0
#define MIPI_DSI_OPERATION_MODE_COMMAND    1

/* Command transfer type in command mode */
#define DCS_TRANS_HS                0
#define DCS_TRANS_LP                1

/* DSI Tear Defines */
#define MIPI_DCS_SET_TEAR_ON_MODE_0 0
#define MIPI_DCS_SET_TEAR_ON_MODE_1 1
#define MIPI_DCS_ENABLE_TEAR        1
#define MIPI_DCS_DISABLE_TEAR       0

/* Pixel FIFO Depth */
#define PIXEL_FIFO_DEPTH                    1440

#define BYTE_PER_PIXEL_COLOR_16BIT_CFG_1    2
#define BYTE_PER_PIXEL_COLOR_16BIT_CFG_2    2
#define BYTE_PER_PIXEL_COLOR_16BIT_CFG_3    2
#define BYTE_PER_PIXEL_COLOR_18BIT_CFG_1    3
#define BYTE_PER_PIXEL_COLOR_18BIT_CFG_2    3
#define BYTE_PER_PIXEL_COLOR_24BIT          3
#define BYTE_PER_PIXEL_COLOR_20BIT_LOOSE    3
#define BYTE_PER_PIXEL_COLOR_24_BIT_YCBCR   3
#define BYTE_PER_PIXEL_COLOR_16BIT_YCBCR    2
#define BYTE_PER_PIXEL_COLOR_30BIT          4
#define BYTE_PER_PIXEL_COLOR_36BIT          5
/* in fact it should be 1.5(12bit) */
#define BYTE_PER_PIXEL_COLOR_12BIT          3

/* Tearing Interrupt Bit */
#define INT_TEARING                         6

/* DCS COMMAND LIST */
#define DCS_CMD_CODE_ENTER_IDLE_MODE      0x0
#define DCS_CMD_CODE_ENTER_INVERT_MODE    0x1
#define DCS_CMD_CODE_ENTER_NORMAL_MODE    0x2
#define DCS_CMD_CODE_ENTER_PARTIAL_MODE   0x3
#define DCS_CMD_CODE_ENTER_SLEEP_MODE     0x4
#define DCS_CMD_CODE_EXIT_IDLE_MODE       0x5
#define DCS_CMD_CODE_EXIT_INVERT_MODE     0x6
#define DCS_CMD_CODE_EXIT_SLEEP_MODE      0x7
#define DCS_CMD_CODE_NOP                  0x8
#define DCS_CMD_CODE_SET_DISPLAY_OFF      0x9
#define DCS_CMD_CODE_SET_DISPLAY_ON       0xa
#define DCS_CMD_CODE_SET_TEAR_OFF         0xb
#define DCS_CMD_CODE_SOFT_RESET           0xc

#endif
