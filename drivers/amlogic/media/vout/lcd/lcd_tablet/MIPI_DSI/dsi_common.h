/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef DSI_COMMON_H
#define DSI_COMMON_H

/* DCS Command List */
#define DCS_ENTER_IDLE_MODE          0x39
#define DCS_ENTER_INVERT_MODE        0x21
#define DCS_ENTER_NORMAL_MODE        0x13
#define DCS_ENTER_PARTIAL_MODE       0x12
#define DCS_ENTER_SLEEP_MODE         0x10
#define DCS_EXIT_IDLE_MODE           0x38
#define DCS_EXIT_INVERT_MODE         0x20
#define DCS_EXIT_SLEEP_MODE          0x11
#define DCS_GET_3D_CONTROL           0x3f
#define DCS_GET_ADDRESS_MODE         0x0b
#define DCS_GET_BLUE_CHANNEL         0x08
#define DCS_GET_DIAGNOSTIC_RESULT    0x0f
#define DCS_GET_DISPLAY_MODE         0x0d
#define DCS_GET_GREEN_CHANNEL        0x07
#define DCS_GET_PIXEL_FORMAT         0x0c
#define DCS_GET_POWER_MODE           0x0a
#define DCS_GET_RED_CHANNEL          0x06
#define DCS_GET_SCANLINE             0x45
#define DCS_GET_SIGNAL_MODE          0x0e
#define DCS_NOP                      0x00
#define DCS_READ_DDB_CONTINUE        0xa8
#define DCS_READ_DDB_START           0xa1
#define DCS_READ_MEMORY_CONTINUE     0x3e
#define DCS_READ_MEMORY_START        0x2e
#define DCS_SET_3D_CONTROL           0x3d
#define DCS_SET_ADDRESS_MODE         0x36
#define DCS_SET_COLUMN_ADDRESS       0x2a
#define DCS_SET_DISPLAY_OFF          0x28
#define DCS_SET_DISPLAY_ON           0x29
#define DCS_SET_GAMMA_CURVE          0x26
#define DCS_SET_PAGE_ADDRESS         0x2b
#define DCS_SET_PARTIAL_COLUMNS      0x31
#define DCS_SET_PARTIAL_ROWS         0x30
#define DCS_SET_PIXEL_FORMAT         0x3a
#define DCS_SET_SCROLL_AREA          0x33
#define DCS_SET_SCROLL_START         0x37
#define DCS_SET_TEAR_OFF             0x34
#define DCS_SET_TEAR_ON              0x35
#define DCS_SET_TEAR_SCANLINE        0x44
#define DCS_SET_VSYNC_TIMING         0x40
#define DCS_SOFT_RESET               0x01
#define DCS_WRITE_LUT                0x2d
#define DCS_WRITE_MEMORY_CONTINUE    0x3c
#define DCS_WRITE_MEMORY_START       0x2c

/*  MIPI DCS Pixel-to-Byte Format */
#define DCS_PF_RSVD                  0x0
#define DCS_PF_3BIT                  0x1
#define DCS_PF_8BIT                  0x2
#define DCS_PF_12BIT                 0x3
#define DCS_PF_16BIT                 0x5
#define DCS_PF_18BIT                 0x6
#define DCS_PF_24BIT                 0x7

/* ********************************************************
 * MIPI DSI Data Type/ MIPI DCS Command Type Definitions
 * Peripheral to Host
 */
enum mipi_dsi_data_type_host_e {
	DT_VSS                  = 0x01,
	DT_VSE                  = 0x11,
	DT_HSS                  = 0x21,
	DT_HSE                  = 0x31,
	DT_EOTP                 = 0x08,
	DT_CMOFF                = 0x02,
	DT_CMON                 = 0x12,
	DT_SHUT_DOWN            = 0x22,
	DT_TURN_ON              = 0x32,
	DT_GEN_SHORT_WR_0       = 0x03,
	DT_GEN_SHORT_WR_1       = 0x13,
	DT_GEN_SHORT_WR_2       = 0x23,
	DT_GEN_RD_0             = 0x04,
	DT_GEN_RD_1             = 0x14,
	DT_GEN_RD_2             = 0x24,
	DT_DCS_SHORT_WR_0       = 0x05,
	DT_DCS_SHORT_WR_1       = 0x15,
	DT_DCS_RD_0             = 0x06,
	DT_SET_MAX_RET_PKT_SIZE = 0x37,
	DT_NULL_PKT             = 0x09,
	DT_BLANK_PKT            = 0x19,
	DT_GEN_LONG_WR          = 0x29,
	DT_DCS_LONG_WR          = 0x39,
	DT_20BIT_LOOSE_YCBCR    = 0x0c,
	DT_24BIT_YCBCR          = 0x1c,
	DT_16BIT_YCBCR          = 0x2c,
	DT_30BIT_RGB_101010     = 0x0d,
	DT_36BIT_RGB_121212     = 0x1d,
	DT_12BIT_YCBCR          = 0x3d,
	DT_16BIT_RGB_565        = 0x0e,
	DT_18BIT_RGB_666        = 0x1e,
	DT_18BIT_LOOSE_RGB_666  = 0x2e,
	DT_24BIT_RGB_888        = 0x3e
};

/* Peripheral to Host
 * normal: 0x87(LPDT), data_type, 0, 0, ecc.  (write or tearing-effect)
 * error:  0x87(LPDT), 0x02, error_code[15:0], ecc.
 * short read: 0x87, data_type, data0, data1, ecc
 * long read:  0x87, data_type, word_cnt[15:0], ecc, data0, ... data(N-1), checksum(or 0)[15:0].
 */
enum mipi_dsi_data_type_peripheral_e {
	DT_RESP_TE             = 0xba,
	DT_RESP_ACK            = 0x84,
	DT_RESP_ACK_ERR        = 0x02,
	DT_RESP_EOTP           = 0x08,
	DT_RESP_GEN_READ_1     = 0x11,
	DT_RESP_GEN_READ_2     = 0x12,
	DT_RESP_GEN_READ_LONG  = 0x1a,
	DT_RESP_DCS_READ_LONG  = 0x1c,
	DT_RESP_DCS_READ_1     = 0x21,
	DT_RESP_DCS_READ_2     = 0x22,
};

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

/* mipi-dsi config */
/* Operation mode parameters */
#define OPERATION_VIDEO_MODE     0
#define OPERATION_COMMAND_MODE   1

#define SYNC_PULSE               0x0
#define SYNC_EVENT               0x1
#define BURST_MODE               0x2

extern char *dsi_op_mode_table[];

extern char *dsi_video_mode_type_table[];
enum dsi_vid_type_e {
	DSI_VID_SYNC_PULSE,
	DSI_VID_SYNC_EVENT,
	DSI_VID_BURST
};

/* **** DPHY timing parameter     Value (unit: 0.01ns) **** */
/* >100ns (4M) */
#define DPHY_TIME_LP_TESC(ui)     (250 * 100)
/* >50ns */
#define DPHY_TIME_LP_LPX(ui)      (100 * 100)
/* (lpx, 2*lpx) */
#define DPHY_TIME_LP_TA_SURE(ui)  DPHY_TIME_LP_LPX(ui)
/* 4*lpx */
#define DPHY_TIME_LP_TA_GO(ui)    (4 * DPHY_TIME_LP_LPX(ui))
/* 5*lpx */
#define DPHY_TIME_LP_TA_GETX(ui)  (5 * DPHY_TIME_LP_LPX(ui))
/* >100ns */
#define DPHY_TIME_HS_EXIT(ui)     (110 * 100)
/* max(8*ui, 60+4*ui), (test)<105+12*ui */
static inline u32 DPHY_TIME_HS_TRAIL(u32 ui)
{
	return ui > (60 * 100 / 4) ? (8 * ui) : ((60 * 100) + 4 * ui);
}

/* (40+4*ui, 85+6*ui) */
#define DPHY_TIME_HS_PREPARE(ui)  (50 * 100 + 4 * (ui))
/* hs_prepare+hs_zero >145+10*ui */
static inline u32 DPHY_TIME_HS_ZERO(u32 ui)
{
	return 160 * 100 + 10 * ui - DPHY_TIME_HS_PREPARE(ui);
}

/* >60ns, (test)<105+12*ui */
#define DPHY_TIME_CLK_TRAIL(ui)   (70 * 100)
/* >60+52*ui */
#define DPHY_TIME_CLK_POST(ui)    (2 * (60 * 100 + 52 * (ui)))
/* (38, 95) */
#define DPHY_TIME_CLK_PREPARE(ui) (50 * 100)
/* clk_prepare+clk_zero > 300 */
#define DPHY_TIME_CLK_ZERO(ui)    (320 * 100 - DPHY_TIME_CLK_PREPARE(ui))
/* >8*ui */
#define DPHY_TIME_CLK_PRE(ui)     (10 * (ui))
/* >100us */
#define DPHY_TIME_INIT(ui)        (110 * 1000 * 100)
/* >1ms */
#define DPHY_TIME_WAKEUP(ui)      (1020 * 1000 * 100)

void dsi_init_table_print(struct dsi_config_s *dconf);
int dsi_run_oneline_cmd(struct aml_lcd_drv_s *pdrv, u8 *payload, u8 *rd_back, u32 rd_back_len);
int dsi_read(struct aml_lcd_drv_s *pdrv, u8 *payload, u8 *rd_data, u8 rd_byte_len);

/* DSI setting */
// DSI escape mode commman ack control field
#define MIPI_DSI_DCS_NO_ACK  0
#define MIPI_DSI_DCS_REQ_ACK 1

#define ACK_CTRL_GEN_SHORT_WR         MIPI_DSI_DCS_NO_ACK
#define ACK_CTRL_DCS_SHORT_WR         MIPI_DSI_DCS_NO_ACK
#define ACK_CTRL_GEN_LONG_WR          MIPI_DSI_DCS_NO_ACK
#define ACK_CTRL_DCS_LONG_WR          MIPI_DSI_DCS_NO_ACK
#define ACK_CTRL_SET_MAX_RET_PKT_SIZE MIPI_DSI_DCS_NO_ACK
#define ACK_CTRL_GEN_RD               MIPI_DSI_DCS_REQ_ACK
#define ACK_CTRL_DCS_RD               MIPI_DSI_DCS_REQ_ACK

// DSI phy LP-HS switch control field
#define PHY_SWITCH_FAST 0
#define PHY_SWITCH_SLOW 1

#define STOP_STATE_TO_HS_WAIT_TIME    PHY_SWITCH_FAST

/* Range [0,3] */
#define MIPI_DSI_VIRTUAL_CHAN_ID        0
/* Define DSI command transfer type: high speed or low power */
#define MIPI_DSI_CMD_TRANS_TYPE         DCS_TRANS_LP

#define DSI_RD_MAX 4

#define MIPI_DSI_COLOR_18BIT            COLOR_18BIT_CFG_2//COLOR_18BIT_CFG_1
#define MIPI_DSI_COLOR_24BIT            COLOR_24BIT

#endif
