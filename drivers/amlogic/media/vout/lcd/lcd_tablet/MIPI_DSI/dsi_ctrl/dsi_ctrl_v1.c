// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#ifdef CONFIG_SECURE_POWER_CONTROL
#include <asm/arch/pwr_ctrl.h>
#endif
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../../../lcd_reg.h"
#include "../../../lcd_common.h"
#include "../../lcd_tablet.h"
#include "./dsi_ctrl.h"
#include "./dsi_ctrl_v1.h"

#define MIPI_DSI_TEAR_SWITCH            MIPI_DCS_DISABLE_TEAR
#define CMD_TIMEOUT_CNT                 20000
/* ************************************************************* */

static void host_config_print(struct lcd_config_s *pconf)
{
	if (pconf->control.mipi_cfg.video_mode_type == BURST_MODE)
		return;

	pr_info("MIPI DSI NON-BURST setting:\n"
		" multi_pkt_en:     %d\n"
		" vid_num_chunks:   %d\n"
		" pixel_per_chunk:  %d\n"
		" byte_per_chunk:   %d\n"
		" vid_null_size:    %d\n\n",
		pconf->control.mipi_cfg.multi_pkt_en, pconf->control.mipi_cfg.vid_num_chunks,
		pconf->control.mipi_cfg.pixel_per_chunk, pconf->control.mipi_cfg.byte_per_chunk,
		pconf->control.mipi_cfg.vid_null_size);
}

/* *************************************************************
 * Function: mipi_dcs_set
 * Configure relative registers in command mode
 * Parameters:   int trans_type, // 0: high speed, 1: low power
 *               int req_ack,    // 1: request ack, 0: do not need ack
 *               int tear_en     // 1: enable tear ack, 0: disable tear ack
 */
static void mipi_dcs_set(struct aml_lcd_drv_s *pdrv, int trans_type, int req_ack, int tear_en)
{
	dsi_host_write(pdrv, MIPI_DSI_DWC_CMD_MODE_CFG_OS,
		(trans_type << BIT_MAX_RD_PKT_SIZE) |
		(trans_type << BIT_DCS_LW_TX)    |
		(trans_type << BIT_DCS_SR_0P_TX) |
		(trans_type << BIT_DCS_SW_1P_TX) |
		(trans_type << BIT_DCS_SW_0P_TX) |
		(trans_type << BIT_GEN_LW_TX)    |
		(trans_type << BIT_GEN_SR_2P_TX) |
		(trans_type << BIT_GEN_SR_1P_TX) |
		(trans_type << BIT_GEN_SR_0P_TX) |
		(trans_type << BIT_GEN_SW_2P_TX) |
		(trans_type << BIT_GEN_SW_1P_TX) |
		(trans_type << BIT_GEN_SW_0P_TX) |
		(req_ack << BIT_ACK_RQST_EN)     |
		(tear_en << BIT_TEAR_FX_EN));

	if (tear_en == MIPI_DCS_ENABLE_TEAR) {
		/* Enable Tear Interrupt if tear_en is valid */
		dsi_host_set_mask(pdrv, MIPI_DSI_TOP_INTR_CNTL_STAT, (0x1 << BIT_EDPITE_INT_EN));
		/* Enable Measure Vsync */
		dsi_host_set_mask(pdrv, MIPI_DSI_TOP_MEAS_CNTL,
			(0x1 << BIT_VSYNC_MEAS_EN) | (0x1 << BIT_TE_MEAS_EN));
	}

	/* Packet header settings */
	dsi_host_write(pdrv, MIPI_DSI_DWC_PCKHDL_CFG_OS,
		(1 << BIT_CRC_RX_EN)    |
		(1 << BIT_ECC_RX_EN)    |
		(req_ack << BIT_BTA_EN) |
		(0 << BIT_EOTP_RX_EN)   |
		(0 << BIT_EOTP_TX_EN));
}

/* Function: check_phy_status
 * Check the status of the dphy: phylock and stopstateclklane,
 *  to decide if the DPHY is ready
 */
#define DPHY_TIMEOUT    20000
static void check_phy_status(struct aml_lcd_drv_s *pdrv)
{
	int i = 0;

	while (dsi_host_getb(pdrv, MIPI_DSI_DWC_PHY_STATUS_OS, BIT_PHY_LOCK, 1) == 0) {
		if (i++ >= DPHY_TIMEOUT) {
			LCDERR("[%d]: %s: phy_lock timeout\n", pdrv->index, __func__);
			break;
		}
		udelay(5);
	}

	i = 0;
	udelay(10);
	while (dsi_host_getb(pdrv, MIPI_DSI_DWC_PHY_STATUS_OS, BIT_PHY_STOPSTATECLKLANE, 1) == 0) {
		if (i == 0)
			LCDPR("[%d]: Waiting STOP STATE LANE\n", pdrv->index);
		if (i++ >= DPHY_TIMEOUT) {
			LCDERR("[%d]: %s: lane_state timeout\n", pdrv->index, __func__);
			break;
		}
		udelay(5);
	}
}

static void dsi_phy_init(struct aml_lcd_drv_s *pdrv, struct dsi_dphy_s *dphy)
{
	u8 lane_n  = pdrv->config.control.mipi_cfg.lane_num;

	/* enable phy clock. */
	dsi_phy_write(pdrv, MIPI_DSI_PHY_CTRL,  0x1); /* enable DSI top clock. */
	dsi_phy_write(pdrv, MIPI_DSI_PHY_CTRL,
		(1 << 0)  | /* enable the DSI PLL clock . */
		(1 << 7)  | /* enable pll clock which connected to DDR clock path */
		(1 << 8)  | /* enable the clock divider counter */
		(0 << 9)  | /* enable the divider clock out */
		(0 << 10) | /* clock divider. 1: freq/4, 0: freq/2 */
		(0 << 11) | /* 1: select the mipi DDRCLKHS from clock divider, 0: from PLL clock */
		(0 << 12)); /* enable the byte clock generateion. */
	/* enable the divider clock out */
	dsi_phy_setb(pdrv, MIPI_DSI_PHY_CTRL,  1, 9, 1);
	/* enable the byte clock generateion. */
	dsi_phy_setb(pdrv, MIPI_DSI_PHY_CTRL,  1, 12, 1);
	dsi_phy_setb(pdrv, MIPI_DSI_PHY_CTRL,  1, 31, 1);
	dsi_phy_setb(pdrv, MIPI_DSI_PHY_CTRL,  0, 31, 1);

	/* 0x05210f08);//0x03211c08 */
	dsi_phy_write(pdrv, MIPI_DSI_CLK_TIM,
		      (dphy->clk_trail |
		      ((dphy->clk_post + dphy->hs_trail) << 8) |
		      (dphy->clk_zero << 16) |
		      (dphy->clk_prepare << 24)));
	dsi_phy_write(pdrv, MIPI_DSI_CLK_TIM1, dphy->clk_pre); /* ?? */
	/* 0x050f090d */
	if (pdrv->config.timing.bit_rate > 500000000) { /*MAX than 500MHZ*/
		dsi_phy_write(pdrv, MIPI_DSI_HS_TIM,
			      (dphy->hs_exit |
			      (dphy->hs_trail << 8) |
			      (dphy->hs_zero << 16) |
			      (dphy->hs_prepare << 24)));
	} else {
		LCDPR("[%d]: %s: bit_rate: %lldhz\n",
		      pdrv->index, __func__, pdrv->config.timing.bit_rate);
		dsi_phy_write(pdrv, MIPI_DSI_HS_TIM,
			      (dphy->hs_exit |
			      ((dphy->hs_trail / 2) << 8) |
			      (dphy->hs_zero << 16) |
			      (dphy->hs_prepare << 24)));
	}
	/* 0x4a370e0e */
	dsi_phy_write(pdrv, MIPI_DSI_LP_TIM,
		(dphy->lp_lpx | (dphy->lp_ta_sure << 8) |
		(dphy->lp_ta_go << 16) | (dphy->lp_ta_get << 24)));
	/* ?? //some number to reduce sim time. */
	dsi_phy_write(pdrv, MIPI_DSI_ANA_UP_TIM, 0x0100);
	/* 0xe20   //30d4 -> d4 to reduce sim time. */
	dsi_phy_write(pdrv, MIPI_DSI_INIT_TIM, dphy->init);
	/* 0x8d40  //1E848-> 48 to reduct sim time. */
	dsi_phy_write(pdrv, MIPI_DSI_WAKEUP_TIM, dphy->wakeup);
	/* wait for the LP analog ready. */
	dsi_phy_write(pdrv, MIPI_DSI_LPOK_TIM, 0x7C);
	/* 1/3 of the tWAKEUP. */
	dsi_phy_write(pdrv, MIPI_DSI_ULPS_CHECK, 0x927C);
	/* phy TURN watch dog. */
	dsi_phy_write(pdrv, MIPI_DSI_LP_WCHDOG, 0x1000);
	/* phy ESC command watch dog. */
	dsi_phy_write(pdrv, MIPI_DSI_TURN_WCHDOG, 0x1000);

	/* Powerup the analog circuit. */
	dsi_phy_write(pdrv, MIPI_DSI_CHAN_CTRL, (0xf0 >> (4 - lane_n)) & 0xf);
}

static void set_dsi_phy_config(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	/* Digital */
	/* Power up DSI */
	dsi_host_write(pdrv, MIPI_DSI_DWC_PWR_UP_OS, 1);

	/* Setup Parameters of DPHY */
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL1_OS, 0x00010044);/*testcode*/
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL0_OS, 0x2);
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL0_OS, 0x0);
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL1_OS, 0x00000074);/*testwrite*/
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL0_OS, 0x2);
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TST_CTRL0_OS, 0x0);

	/* Power up D-PHY */
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_RSTZ_OS, 0xf);

	/* Analog */
	dsi_phy_init(pdrv, &dconf->dphy);

	/* Check the phylock/stopstateclklane to decide if the DPHY is ready */
	check_phy_status(pdrv);

	/* Trigger a sync active for esc_clk */
	dsi_phy_set_mask(pdrv, MIPI_DSI_PHY_CTRL, (1 << 1));
}

static void startup_mipi_dsi_host(struct aml_lcd_drv_s *pdrv)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	/* Enable dwc mipi_dsi_host's clock */
	dsi_host_set_mask(pdrv, MIPI_DSI_TOP_CNTL, ((1 << 4) | (1 << 5) | (0 << 6)));
	/* mipi_dsi_host's reset */
	dsi_host_set_mask(pdrv, MIPI_DSI_TOP_SW_RESET, 0xf);
	/* Release mipi_dsi_host's reset */
	dsi_host_clr_mask(pdrv, MIPI_DSI_TOP_SW_RESET, 0xf);
	/* Enable dwc mipi_dsi_host's clock */
	dsi_host_set_mask(pdrv, MIPI_DSI_TOP_CLK_CNTL, 0x3);

	dsi_host_write(pdrv, MIPI_DSI_TOP_MEM_PD, 0);

	mdelay(10);
}

static void mipi_dsi_lpclk_ctrl(struct aml_lcd_drv_s *pdrv)
{
	/* when lpclk = 1, enable clk lp state */
	u32 lpclk = (pdrv->config.control.mipi_cfg.clk_always_hs) ? 0 : 1;

	dsi_host_write(pdrv, MIPI_DSI_DWC_LPCLK_CTRL_OS,
		(lpclk << BIT_AUTOCLKLANE_CTRL) | (0x1 << BIT_TXREQUESTCLKHS));
}

/* Function: set_mipi_dsi_host
 * Parameters: vcid, // virtual id
 *		chroma_subsample, // chroma_subsample for YUV422 or YUV420 only
 *		operation_mode,   // video mode/command mode
 *		p,                //lcd config
 */
static void set_mipi_dsi_host(struct aml_lcd_drv_s *pdrv,
		u8 vcid, u8 chroma_subsample, u8 operation_mode)
{
	u32 dpi_data_format, venc_data_width;
	u32 lane_num, vid_mode_type;
	u32 h_act, v_act, v_sync, v_bp, v_fp;
	u32 temp;
	struct dsi_config_s *dconf;

	dconf = &pdrv->config.control.mipi_cfg;
	venc_data_width = dconf->venc_data_width;
	dpi_data_format = dconf->dpi_data_format;
	lane_num        = (u32)(dconf->lane_num);
	vid_mode_type   = (u32)(dconf->video_mode_type);
	h_act           = pdrv->config.timing.act_timing.h_active;
	v_act           = pdrv->config.timing.act_timing.v_active;
	v_sync          = pdrv->config.timing.act_timing.vsync_width;
	v_bp            = pdrv->config.timing.act_timing.vsync_bp;
	v_fp            = pdrv->config.timing.act_timing.vsync_fp;

	/* ----------------------------------------------------- */
	/* Standard Configuration for Video Mode Operation */
	/* ----------------------------------------------------- */
	/* 1,    Configure Lane number and phy stop wait time */
	if (STOP_STATE_TO_HS_WAIT_TIME) {
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_IF_CFG_OS,
			(0x28 << BIT_PHY_STOP_WAIT_TIME) | ((lane_num - 1) << BIT_N_LANES));
	} else {
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_IF_CFG_OS,
			(1 << BIT_PHY_STOP_WAIT_TIME) | ((lane_num - 1) << BIT_N_LANES));
	}

	/* 2.1,  Configure Virtual channel settings */
	dsi_host_write(pdrv, MIPI_DSI_DWC_DPI_VCID_OS, vcid);
	/* 2.2,  Configure Color format */
	dsi_host_write(pdrv, MIPI_DSI_DWC_DPI_COLOR_CODING_OS,
		(((dpi_data_format == COLOR_18BIT_CFG_2) ? 1 : 0) << BIT_LOOSELY18_EN) |
		(dpi_data_format << BIT_DPI_COLOR_CODING));
	/* 2.2.1 Configure Set color format for DPI register */
	temp = (dsi_host_read(pdrv, MIPI_DSI_TOP_CNTL) &
		~(0xf << BIT_DPI_COLOR_MODE) &
		~(0x7 << BIT_IN_COLOR_MODE) &
		~(0x3 << BIT_CHROMA_SUBSAMPLE));
	dsi_host_write(pdrv, MIPI_DSI_TOP_CNTL,
		(temp |
		(dpi_data_format  << BIT_DPI_COLOR_MODE)  |
		(venc_data_width  << BIT_IN_COLOR_MODE)   |
		(chroma_subsample << BIT_CHROMA_SUBSAMPLE)));
	/* 2.3   Configure Signal polarity */
	dsi_host_write(pdrv, MIPI_DSI_DWC_DPI_CFG_POL_OS,
		(0x0 << BIT_COLORM_ACTIVE_LOW) |
		(0x0 << BIT_SHUTD_ACTIVE_LOW)  |
		(0 << BIT_HSYNC_ACTIVE_LOW)    |
		(0 << BIT_VSYNC_ACTIVE_LOW)    |
		(0x0 << BIT_DATAEN_ACTIVE_LOW));

	if (operation_mode == OPERATION_VIDEO_MODE) {
		/* 3.1   Configure Low power and video mode type settings */
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_MODE_CFG_OS,
			// (1 << BIT_LP_CMD_EN) |
			(0 << BIT_FRAME_BTA_ACK_EN) | /* enable BTA after one frame, need check */
			(1 << BIT_LP_HFP_EN)  | /* enable lp */
			(1 << BIT_LP_HBP_EN)  | /* enable lp */
			(1 << BIT_LP_VACT_EN) | /* enable lp */
			(1 << BIT_LP_VFP_EN)  | /* enable lp */
			(1 << BIT_LP_VBP_EN)  | /* enable lp */
			(1 << BIT_LP_VSA_EN)  | /* enable lp */
			(vid_mode_type << BIT_VID_MODE_TYPE)); /* burst non burst mode */
		/* [23:16]outvact, [7:0]invact */
		dsi_host_write(pdrv, MIPI_DSI_DWC_DPI_LP_CMD_TIM_OS, (4 << 16) | (4 << 0));

		/* 3.2 Configure video packet size settings */
		/* 3.3 Configure number of chunks and null packet size for one line */
		if (vid_mode_type == BURST_MODE) {
			dsi_host_write(pdrv, MIPI_DSI_DWC_VID_PKT_SIZE_OS, h_act);
		} else { // NON_BURST_SYNC_PULSE || NON_BURST_SYNC_EVENT
			dsi_host_write(pdrv, MIPI_DSI_DWC_VID_PKT_SIZE_OS, dconf->pixel_per_chunk);
			dsi_host_write(pdrv, MIPI_DSI_DWC_VID_NUM_CHUNKS_OS, dconf->vid_num_chunks);
			dsi_host_write(pdrv, MIPI_DSI_DWC_VID_NULL_SIZE_OS, dconf->vid_null_size);
		}

		/* 4 Configure the video relative parameters according to the output type */
		/*   include horizontal timing and vertical line */
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_HLINE_TIME_OS, dconf->hline);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_HSA_TIME_OS, dconf->hsa);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_HBP_TIME_OS, dconf->hbp);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_VSA_LINES_OS, v_sync);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_VBP_LINES_OS, v_bp);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_VFP_LINES_OS, v_fp);
		dsi_host_write(pdrv, MIPI_DSI_DWC_VID_VACTIVE_LINES_OS, v_act);
	}  /* operation_mode == OPERATION_VIDEO_MODE */

	/* ----------------------------------------------------- */
	/* Finish Configuration */
	/* ----------------------------------------------------- */

	/* Inner clock divider settings */
	dsi_host_write(pdrv, MIPI_DSI_DWC_CLKMGR_CFG_OS,
		(0x1 << BIT_TO_CLK_DIV) |
		(dconf->dphy.lp_tesc << BIT_TX_ESC_CLK_DIV));
	/* Packet header settings  //move to mipi_dcs_set */
	/* dsi_host_write(pdrv, MIPI_DSI_DWC_PCKHDL_CFG_OS,
	 *	(1 << BIT_CRC_RX_EN) |
	 *	(1 << BIT_ECC_RX_EN) |
	 *	(0 << BIT_dsi_bta_control_EN) |
	 *	(0 << BIT_EOTP_RX_EN) |
	 *	(0 << BIT_EOTP_TX_EN) );
	 */
	/* operation mode setting: video/command mode */
	dsi_host_write(pdrv, MIPI_DSI_DWC_MODE_CFG_OS, operation_mode);

	/* Phy Timer */
	if (STOP_STATE_TO_HS_WAIT_TIME) {
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TMR_CFG_OS, 0x03320000);
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TMR_LPCLK_CFG_OS, 0x870025);
	} else {
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TMR_CFG_OS, 0x090f0000);
		dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_TMR_LPCLK_CFG_OS, 0x260017);
	}
}

/* mipi dsi command support
 */
static inline void print_mipi_cmd_status(struct aml_lcd_drv_s *pdrv, int cnt, u32 status)
{
	if (cnt == 0) {
		LCDPR("[%d]: cmd error: status=0x%04x, int0=0x%06x, int1=0x%06x\n",
			pdrv->index, status,
			dsi_host_read(pdrv, MIPI_DSI_DWC_INT_ST0_OS),
			dsi_host_read(pdrv, MIPI_DSI_DWC_INT_ST1_OS));
	}
}

static void dsi_bta_control(struct aml_lcd_drv_s *pdrv, int en)
{
	u8 ack_req = en ? MIPI_DSI_DCS_REQ_ACK : MIPI_DSI_DCS_NO_ACK;

	dsi_host_setb(pdrv, MIPI_DSI_DWC_CMD_MODE_CFG_OS, ack_req, BIT_ACK_RQST_EN, 1);
	dsi_host_setb(pdrv, MIPI_DSI_DWC_PCKHDL_CFG_OS, ack_req, BIT_BTA_EN, 1);
}

/* Function: wait_bta_ack
 * Poll to check if the BTA ack is finished
 */
static int wait_bta_ack(struct aml_lcd_drv_s *pdrv)
{
	u32 phy_status;
	int i;

	/* Check if phydirection is RX */
	i = CMD_TIMEOUT_CNT;
	do {
		phy_status = dsi_host_read(pdrv, MIPI_DSI_DWC_PHY_STATUS_OS);
		udelay(1);
		i--;
	} while ((((phy_status & 0x2) >> BIT_PHY_DIRECTION) == 0x0) && (i > 0));
	if (i == 0) {
		LCDERR("[%d]: phy direction error: RX\n", pdrv->index);
		return -1;
	}

	/* Check if phydirection is return to TX */
	i = CMD_TIMEOUT_CNT;
	do {
		phy_status = dsi_host_read(pdrv, MIPI_DSI_DWC_PHY_STATUS_OS);
		udelay(1);
		i--;
	} while ((((phy_status & 0x2) >> BIT_PHY_DIRECTION) == 0x1) && (i > 0));
	if (i == 0) {
		LCDERR("[%d]: phy direction error: TX\n", pdrv->index);
		return -1;
	}

	return 0;
}

/* Function: wait_cmd_fifo_empty
 * Poll to check if the generic command fifo is empty
 */
static int wait_cmd_fifo_empty(struct aml_lcd_drv_s *pdrv)
{
	u32 cmd_status;
	int i = CMD_TIMEOUT_CNT;

	do {
		udelay(10);
		i--;
		cmd_status = dsi_host_getb(pdrv,
			MIPI_DSI_DWC_CMD_PKT_STATUS_OS, BIT_GEN_CMD_EMPTY, 1);
	} while ((cmd_status != 0x1) && (i > 0));

	if (cmd_status == 0) {
		cmd_status = dsi_host_read(pdrv, MIPI_DSI_DWC_CMD_PKT_STATUS_OS);
		print_mipi_cmd_status(pdrv, i, cmd_status);
		return -1;
	}

	return 0;
}

/* ***** DSI command related ***** */

/* Set Maximum Return Packet Size, Data Type = 11 0111 (0x37)
 * two-byte value for maximum return packet size.
 * Note that the two-byte value is transmitted with LS byte first.
 */
static int dsi_set_max_return_pkt_size(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	int ret = 0;

	dsi_bta_control(pdrv, req->req_ack == MIPI_DSI_DCS_REQ_ACK);
	dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_HDR_OS,
		((u32)req->payload[1] << BIT_GEN_WC_MSBYTE |
		 (u32)req->payload[0] << BIT_GEN_WC_LSBYTE |
		 (u32)req->vc_id      << BIT_GEN_VC |
		 (u32)req->data_type  << BIT_GEN_DT));
	if (req->req_ack == MIPI_DSI_DCS_REQ_ACK)
		ret = wait_bta_ack(pdrv);
	else
		ret = wait_cmd_fifo_empty(pdrv);
	return ret;
}

static int dsi_generic_read_packet(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	u32 rd_data, rd_fifo_empty, pld_non0_idx;
	u32 i = 0, j;
	int ret = 0;
	// struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;
	u32 d_para[2] = {0, 0};

	if (pdrv->data->chip_type == LCD_CHIP_T7)
		dsi_phy_setb(pdrv, MIPI_DSI_CHAN_CTRL, 0x3, 20, 2);

	if (req->data_type == DT_GEN_RD_2 ||
	    req->data_type == DT_GEN_RD_1 ||
	    req->data_type == DT_DCS_RD_0)
		d_para[0] = req->payload[0];
	if (req->data_type == DT_GEN_RD_2)
		d_para[1] = req->payload[1];

	dsi_bta_control(pdrv, 1);

	dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_HDR_OS,
		(d_para[1]      << BIT_GEN_WC_MSBYTE |
		 d_para[0]      << BIT_GEN_WC_LSBYTE |
		 req->vc_id     << BIT_GEN_VC |
		 req->data_type << BIT_GEN_DT));
	ret = wait_bta_ack(pdrv);
	if (ret)
		goto dsi_generic_read_packet_done;

	rd_fifo_empty = dsi_host_getb(pdrv, MIPI_DSI_DWC_CMD_PKT_STATUS_OS,
		BIT_GEN_PLD_R_EMPTY, 1);

	while (!rd_fifo_empty) {
		rd_data = dsi_host_read(pdrv, MIPI_DSI_DWC_GEN_PLD_DATA_OS);

		rd_fifo_empty = dsi_host_getb(pdrv, MIPI_DSI_DWC_CMD_PKT_STATUS_OS,
			BIT_GEN_PLD_R_EMPTY, 1);

		pld_non0_idx = 4;
		if (rd_fifo_empty) { //fifo empty, last rd
			for (j = 0; j < 4; j++) //count num of non_0 data
				pld_non0_idx = pld_non0_idx - !(rd_data & (0xff000000 >> (8 * j)));
		}
		for (j = 0; j < pld_non0_idx; j++) {
			if (i >= DSI_RD_MAX) {
				LCDPR("need enlarge DSI_RD_MAX (rd_raw: 0x%08x)\n", rd_data);
				goto dsi_generic_read_packet_done;
			}
			req->rd_data[i++] = (u8)((rd_data >> (j * 8)) & 0xff);
		}
	}
	req->rd_out_len = i;

dsi_generic_read_packet_done:
	if (pdrv->data->chip_type == LCD_CHIP_T7)
		dsi_phy_setb(pdrv, MIPI_DSI_CHAN_CTRL, 0x3, 20, 2);
	if (ret)
		return -1;
	return 0;
}

/* Function: generic_write_short_packet
 * Generic Write Short Packet with Generic Interface
 * Supported Data Type: DT_GEN_SHORT_WR_0, DT_GEN_SHORT_WR_1, DT_GEN_SHORT_WR_2,
 */
static int dsi_generic_write_short_packet(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	int ret = 0;
	u32 d_para[2] = {0, 0};

	if (req->data_type == DT_GEN_SHORT_WR_2 || req->data_type == DT_GEN_SHORT_WR_1)
		d_para[0] = req->payload[0];
	if (req->data_type == DT_GEN_SHORT_WR_2)
		d_para[1] = req->payload[1];

	dsi_bta_control(pdrv, req->req_ack == MIPI_DSI_DCS_REQ_ACK);

	dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_HDR_OS,
		(d_para[1]      << BIT_GEN_WC_MSBYTE |
		 d_para[0]      << BIT_GEN_WC_LSBYTE |
		 req->vc_id     << BIT_GEN_VC |
		 req->data_type << BIT_GEN_DT));
	if (req->req_ack == MIPI_DSI_DCS_REQ_ACK)
		ret = wait_bta_ack(pdrv);
	else
		ret = wait_cmd_fifo_empty(pdrv);

	return ret;
}

/* Function: dcs_write_short_packet
 * DCS Write Short Packet with Generic Interface
 * Supported Data Type: DT_DCS_SHORT_WR_0, DT_DCS_SHORT_WR_1,
 */
static int dsi_dcs_write_short_packet(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	int ret = 0;
	u32 d_command, d_para = 0;

	d_command = req->payload[0];
	if (req->data_type == DT_DCS_SHORT_WR_1)
		d_para = req->payload[1];

	dsi_bta_control(pdrv, req->req_ack == MIPI_DSI_DCS_REQ_ACK);
	dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_HDR_OS,
		(d_para         << BIT_GEN_WC_MSBYTE |
		 d_command      << BIT_GEN_WC_LSBYTE |
		 req->vc_id     << BIT_GEN_VC |
		 req->data_type << BIT_GEN_DT));
	if (req->req_ack == MIPI_DSI_DCS_REQ_ACK)
		ret = wait_bta_ack(pdrv);
	else
		ret = wait_cmd_fifo_empty(pdrv);

	return ret;
}

/* Function: dsi_write_long_packet
 * Write Long Packet with Generic Interface
 * Supported Data Type: DT_GEN_LONG_WR, DT_DCS_LONG_WR
 */
static int dsi_write_long_packet(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req)
{
	u32 cmd_status, payload_data;
	u32 i, j, n;
	int ret = 0;

	dsi_bta_control(pdrv, req->req_ack == MIPI_DSI_DCS_REQ_ACK);
	/* Write Payload Register First */
	n = (req->pld_count + 3) / 4;
	for (i = 0; i < n; i++) {
		payload_data = 0;
		for (j = 0; j < 4; j++)
			payload_data |= (i * 4 + j >= req->pld_count) ?
				0 : (u32)req->payload[i * 4 + j] << j * 8;

		//Check the pld fifo status before write to it, do not need check every word
		if ((i + 1) % 4 == 0 || i == n - 1) { //every four write or last one
			j = CMD_TIMEOUT_CNT;
			do {
				udelay(10);
				j--;
				cmd_status = dsi_host_read(pdrv, MIPI_DSI_DWC_CMD_PKT_STATUS_OS);
			} while ((((cmd_status >> BIT_GEN_PLD_W_FULL) & 0x1) == 0x1) && (j > 0));
			print_mipi_cmd_status(pdrv, j, cmd_status);
		}
		dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_PLD_DATA_OS, payload_data);
	}

	/* Check cmd fifo status before write to it */
	j = CMD_TIMEOUT_CNT;
	do {
		udelay(10);
		j--;
		cmd_status = dsi_host_read(pdrv, MIPI_DSI_DWC_CMD_PKT_STATUS_OS);
	} while ((((cmd_status >> BIT_GEN_CMD_FULL) & 0x1) == 0x1) && (j > 0));
	print_mipi_cmd_status(pdrv, j, cmd_status);
	/* Write Header Register */
	/* include command */
	dsi_host_write(pdrv, MIPI_DSI_DWC_GEN_HDR_OS,
			(req->pld_count << BIT_GEN_WC_LSBYTE |
			 req->vc_id     << BIT_GEN_VC |
			 req->data_type << BIT_GEN_DT));
	if (req->req_ack == MIPI_DSI_DCS_REQ_ACK)
		ret = wait_bta_ack(pdrv);
	else
		ret = wait_cmd_fifo_empty(pdrv);

	return ret;
}

static void dsi0_DT_sink_shut_down(struct aml_lcd_drv_s *pdrv)
{
	dsi_host_setb(pdrv, MIPI_DSI_TOP_CNTL, 1, 2, 1);
	mdelay(20); /* wait for vsync trigger */
}

static void dsi0_DT_sink_turn_on(struct aml_lcd_drv_s *pdrv)
{
	dsi_host_setb(pdrv, MIPI_DSI_TOP_CNTL, 1, 2, 1);
	mdelay(20); /* wait for vsync trigger */
	dsi_host_setb(pdrv, MIPI_DSI_TOP_CNTL, 0, 2, 1);
	mdelay(20); /* wait for vsync trigger */
}

static void mipi_dsi_non_burst_packet_config(struct lcd_config_s *pconf)
{
	struct dsi_config_s *dconf = &pconf->control.mipi_cfg;
	u32 lane_num, hactive, multi_pkt_en;
	u64 bit_rate_required;
	u32 pixel_per_chunk = 0, vid_num_chunks = 0;
	u32 byte_per_chunk = 0, vid_pkt_byte_per_chunk = 0;
	u32 total_bytes_per_chunk = 0;
	u32 chunk_overhead = 0, vid_null_size = 0;
	int i, done = 0;

	lane_num = (int)(dconf->lane_num);
	hactive = pconf->timing.act_timing.h_active;
	bit_rate_required = pconf->timing.act_timing.pixel_clk;
	bit_rate_required = bit_rate_required * 3 * pconf->basic.lcd_bits;
	bit_rate_required = lcd_do_div(bit_rate_required, lane_num);
	if (pconf->timing.bit_rate > bit_rate_required)
		multi_pkt_en = 1;
	else
		multi_pkt_en = 0;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("non-burst: bit_rate_required=%lld, bit_rate=%lld, multi_pkt_en=%d\n",
		      bit_rate_required, pconf->timing.bit_rate, multi_pkt_en);
	}

	if (multi_pkt_en == 0) {
		pixel_per_chunk = hactive;
		if (dconf->dpi_data_format == COLOR_18BIT_CFG_1) {
			/* 18bit (4*18/8=9byte) */
			byte_per_chunk = pixel_per_chunk * (9 / 4);
		} else {
			/* 24bit or 18bit-loosely */
			byte_per_chunk = pixel_per_chunk * 3;
		}
		vid_pkt_byte_per_chunk = 4 + byte_per_chunk + 2;
		total_bytes_per_chunk = lane_num * pixel_per_chunk * dconf->factor_denominator;
		total_bytes_per_chunk = total_bytes_per_chunk / (8 * dconf->factor_numerator);

		vid_num_chunks = 0;
		vid_null_size = 0;
	} else {
		i = 2;
		while ((i < hactive) && (done == 0)) {
			vid_num_chunks = i;
			pixel_per_chunk = hactive / vid_num_chunks;

			if (dconf->dpi_data_format == COLOR_18BIT_CFG_1) {
				if ((pixel_per_chunk % 4) > 0)
					continue;
				/* 18bit (4*18/8=9byte) */
				byte_per_chunk = pixel_per_chunk * (9 / 4);
			} else {
				/* 24bit or 18bit-loosely */
				byte_per_chunk = pixel_per_chunk * 3;
			}
			vid_pkt_byte_per_chunk = 4 + byte_per_chunk + 2;
			total_bytes_per_chunk = lane_num * pixel_per_chunk *
				dconf->factor_denominator;
			total_bytes_per_chunk = total_bytes_per_chunk /
				(8 * dconf->factor_numerator);

			chunk_overhead = total_bytes_per_chunk - vid_pkt_byte_per_chunk;
			if (chunk_overhead >= 12) {
				vid_null_size = chunk_overhead - 12;
				done = 1;
			} else if (chunk_overhead >= 6) {
				vid_null_size = 0;
				done = 1;
			}
			i += 2;
		}
		if (done == 0) {
			LCDERR("Packet no room for chunk_overhead\n");
			//pixel_per_chunk = hactive;
			//vid_num_chunks = 0;
			//vid_null_size = 0;
		}
	}

	dconf->pixel_per_chunk = pixel_per_chunk;
	dconf->vid_num_chunks = vid_num_chunks;
	dconf->vid_null_size = vid_null_size;
	dconf->byte_per_chunk = byte_per_chunk;
	dconf->multi_pkt_en = multi_pkt_en;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("MIPI DSI NON-BURST setting:\n"
			"  multi_pkt_en             = %d\n"
			"  vid_num_chunks           = %d\n"
			"  pixel_per_chunk          = %d\n"
			"  byte_per_chunk           = %d\n"
			"  vid_pkt_byte_per_chunk   = %d\n"
			"  total_bytes_per_chunk    = %d\n"
			"  chunk_overhead           = %d\n"
			"  vid_null_size            = %d\n\n",
			multi_pkt_en, vid_num_chunks,
			pixel_per_chunk, byte_per_chunk,
			vid_pkt_byte_per_chunk, total_bytes_per_chunk,
			chunk_overhead, vid_null_size);
	}
}

static void mipi_dsi_vid_mode_config(struct lcd_config_s *pconf)
{
	u64 h_period, hs_width, hs_bp;
	u16 vfp;
	u32 den, num;
	struct dsi_config_s *dconf = &pconf->control.mipi_cfg;

	h_period = pconf->timing.act_timing.h_period;
	hs_width = pconf->timing.act_timing.hsync_width;
	hs_bp = pconf->timing.act_timing.hsync_bp;
	den = pconf->control.mipi_cfg.factor_denominator;
	num = pconf->control.mipi_cfg.factor_numerator;

	dconf->hline = (u16)div_around(h_period * num, den);
	dconf->hsa   = (u16)div_around(hs_width * num, den);
	dconf->hbp   = (u16)div_around(hs_bp    * num, den);

	vfp = pconf->timing.act_timing.v_period
		- pconf->timing.act_timing.vsync_width
		- pconf->timing.act_timing.vsync_bp
		- pconf->timing.act_timing.v_active;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("MIPI DSI video timing:\n"
			"  HLINE       = %hu\n"
			"  HSA         = %hu\n"
			"  HBP         = %hu\n"
			"  VSA         = %hu\n"
			"  VBP         = %hu\n"
			"  VFP         = %hu\n"
			"  VACT        = %hu\n",
			dconf->hline, dconf->hsa, dconf->hbp,
			pconf->timing.act_timing.vsync_width,
			pconf->timing.act_timing.vsync_bp,
			vfp, pconf->timing.act_timing.v_active);
	}

	if (pconf->control.mipi_cfg.video_mode_type == BURST_MODE) {
		dconf->pixel_per_chunk = pconf->timing.act_timing.h_active;
		dconf->vid_num_chunks = 0;
		dconf->vid_null_size = 0;
	} else {
		mipi_dsi_non_burst_packet_config(pconf);
	}
}

/* ********************** DSI tx host/phy control ****************************/
static void mipi_dsi_phy_config(struct dsi_dphy_s *dphy, u32 dsi_ui)
{
	u32 temp, t_ui, t_req_min, t_req_max;
	u32 val;

	if (dsi_ui == 0) {
		LCDERR("%s: DSI dphy t_UI is 0\n", __func__);
		return;
	}

	t_ui = lcd_do_div(dsi_ui, 1000);
	t_ui = (1000000 * 100) / t_ui; /*100*ns */
	temp = t_ui * 8; /* lane_byte cycle time */

	dphy->lp_tesc = ((DPHY_TIME_LP_TESC(t_ui) + temp - 1) / temp) & 0xff;
	dphy->lp_lpx = ((DPHY_TIME_LP_LPX(t_ui) + temp - 1) / temp) & 0xff;
	dphy->lp_ta_sure = ((DPHY_TIME_LP_TA_SURE(t_ui) + temp - 1) / temp) & 0xff;
	dphy->lp_ta_go = ((DPHY_TIME_LP_TA_GO(t_ui) + temp - 1) / temp) & 0xff;
	dphy->lp_ta_get = ((DPHY_TIME_LP_TA_GETX(t_ui) + temp - 1) / temp) & 0xff;
	dphy->init = (DPHY_TIME_INIT(t_ui) + temp - 1) / temp;
	dphy->wakeup = (DPHY_TIME_WAKEUP(t_ui) + temp - 1) / temp;
	dphy->clk_pre = ((DPHY_TIME_CLK_PRE(t_ui) + temp - 1) / temp) & 0xff;

	t_req_max = 95 * 100;
	t_req_min = 38 * 100;
	val = (t_req_max + t_req_min) / 2;
	dphy->clk_prepare = val / temp;
	if ((dphy->clk_prepare * temp) < t_req_min)
		dphy->clk_prepare += 1;

	t_req_min = 300 * 100 - dphy->clk_prepare * temp + 10 * 100;
	dphy->clk_zero = t_req_min / temp;
	if ((dphy->clk_zero * temp) < t_req_min)
		dphy->clk_zero += 1;

	//t_req_max = 105 + (12 * t_ui) / 100;
	t_req_min = 60 * 100;
	//val = (t_req_max + t_req_min) / 2;
	dphy->clk_trail = t_req_min / temp;
	if ((dphy->clk_trail * temp) < t_req_min)
		dphy->clk_trail += 1;

	t_req_min = 60 * 100 + 52 * t_ui + 30 * 100;
	dphy->clk_post = t_req_min / temp;
	if ((dphy->clk_post * temp) < t_req_min)
		dphy->clk_post += 1;

	t_req_min = 100 * 100;
	dphy->hs_exit = t_req_min / temp;
	if ((dphy->hs_exit * temp) < t_req_min)
		dphy->hs_exit += 1;

	//t_req_max = 105 + (12 * t_ui) / 100;
	t_req_min = ((8 * t_ui) > (60 * 100 + 4 * t_ui)) ? (8 * t_ui) : (60 * 100 + 4 * t_ui);
	//val = (t_req_max + t_req_min) / 2;
	dphy->hs_trail = t_req_min / temp;
	if ((dphy->hs_trail * temp) < t_req_min)
		dphy->hs_trail += 1;

	t_req_min = 40 * 100 + 4 * t_ui;
	t_req_max = 85 * 100 + 6 * t_ui;
	val = (t_req_max + t_req_min) / 2;
	dphy->hs_prepare = val / temp;
	if ((dphy->hs_prepare * temp) < t_req_min)
		dphy->hs_prepare += 1;

	t_req_min = 145 * 100 + 10 * t_ui - dphy->hs_prepare * temp;
	dphy->hs_zero = t_req_min / temp;
	if ((dphy->hs_zero * temp) < t_req_min)
		dphy->hs_zero += 1;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s:\n"
			"  lp_tesc     = 0x%02x\n"
			"  lp_lpx      = 0x%02x\n"
			"  lp_ta_sure  = 0x%02x\n"
			"  lp_ta_go    = 0x%02x\n"
			"  lp_ta_get   = 0x%02x\n"
			"  hs_exit     = 0x%02x\n"
			"  hs_trail    = 0x%02x\n"
			"  hs_zero     = 0x%02x\n"
			"  hs_prepare  = 0x%02x\n"
			"  clk_trail   = 0x%02x\n"
			"  clk_post    = 0x%02x\n"
			"  clk_zero    = 0x%02x\n"
			"  clk_prepare = 0x%02x\n"
			"  clk_pre     = 0x%02x\n"
			"  init        = 0x%02x\n"
			"  wakeup      = 0x%02x\n",
			__func__,
			dphy->lp_tesc, dphy->lp_lpx, dphy->lp_ta_sure,
			dphy->lp_ta_go, dphy->lp_ta_get, dphy->hs_exit,
			dphy->hs_trail, dphy->hs_zero, dphy->hs_prepare,
			dphy->clk_trail, dphy->clk_post,
			dphy->clk_zero, dphy->clk_prepare, dphy->clk_pre,
			dphy->init, dphy->wakeup);
	}
}

/* bit_rate is confirm by clk_genrate, so internal clk config must after that */
static void mipi_dsi_config_post(struct lcd_config_s *pconf)
{
	struct dsi_config_s *dconf = &pconf->control.mipi_cfg;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("%s:\n"
			"  Pixel CLK   = %10uHz\n"
			"  bit_rate    = %10lluHz\n"
			"  lanebyteclk = %10uHz\n"
			"  PCLK_period/lanebyteclk_period = (num=%u/den=%u)\n",
			__func__, pconf->timing.act_timing.pixel_clk, pconf->timing.bit_rate,
			dconf->lane_byte_clk, dconf->factor_numerator, dconf->factor_denominator);
	}

	if (dconf->operation_mode_display == OPERATION_VIDEO_MODE)
		mipi_dsi_vid_mode_config(pconf);

	/* phy config */
	mipi_dsi_phy_config(&dconf->dphy, pconf->timing.bit_rate);
}

static void mipi_dsi_init_t7(u32 drv_idx)
{
	u8 bit;

	//host reset
	bit = drv_idx == 0 ? 30 : 31;
	lcd_reset_setb(RESETCTRL_RESET1_MASK, 0, bit, 1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit, 1);
	udelay(1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit, 1);
	udelay(1);

	//phy reset
	bit = drv_idx == 0 ? 29 : 28;
	lcd_reset_setb(RESETCTRL_RESET1_MASK, 0, bit, 1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit, 1);
	udelay(1);
	lcd_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit, 1);
	udelay(1);
}

static void mipi_dsi_color_format_config(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

	if (pdrv->config.basic.lcd_bits == 6) {
		dconf->venc_data_width = MIPI_DSI_VENC_COLOR_18B;
		dconf->dpi_data_format = MIPI_DSI_COLOR_18BIT;
	} else {
		dconf->venc_data_width = MIPI_DSI_VENC_COLOR_24B;
		dconf->dpi_data_format  = MIPI_DSI_COLOR_24BIT;
	}
}

static void dsi_host_on_pre(struct aml_lcd_drv_s *pdrv)
{
	u8 op_mode_init;

	if (pdrv->data->chip_type == LCD_CHIP_T7) {
		mipi_dsi_init_t7(pdrv->index);
#ifdef CONFIG_SECURE_POWER_CONTROL
		if (pdrv->index == 0)
			pwr_ctrl_psci_smc(PM_MIPI_DSI0, 1);
		else
			pwr_ctrl_psci_smc(PM_MIPI_DSI1, 1);
#endif
	}

	mipi_dsi_color_format_config(pdrv);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	lcd_venc_enable(pdrv, 0);
	udelay(100);

	mipi_dsi_config_post(&pdrv->config);

	startup_mipi_dsi_host(pdrv);

	set_dsi_phy_config(pdrv);

	op_mode_init = pdrv->config.control.mipi_cfg.operation_mode_init;
	mipi_dcs_set(pdrv,
		MIPI_DSI_CMD_TRANS_TYPE, //0: high speed, 1: low power
		MIPI_DSI_DCS_REQ_ACK, //if need bta ack check
		MIPI_DSI_TEAR_SWITCH); //enable tear ack

	set_mipi_dsi_host(pdrv,
		MIPI_DSI_VIRTUAL_CHAN_ID, //Virtual channel id
		0, //Chroma sub sample, only for YUV 422 or 420, even or odd
		op_mode_init); //DSI operation mode, video or command

	/* Startup transfer */
	mipi_dsi_lpclk_ctrl(pdrv);

	//bit[3]: ULPS exit on data
	//bit[2]: ULPS request on data
	//bit[1]: ULPS exit on clk
	//bit[0]: ULPS request on clk
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_ULPS_CTRL_OS, 0xa);
}

static void dsi_host_on_post(struct aml_lcd_drv_s *pdrv)
{
	u8 op_mode_disp = pdrv->config.control.mipi_cfg.operation_mode_display;
	u8 op_mode_init = pdrv->config.control.mipi_cfg.operation_mode_init;

	if (op_mode_disp != op_mode_init) {
		set_mipi_dsi_host(pdrv, MIPI_DSI_VIRTUAL_CHAN_ID,
			0, //Chroma sub sample, only for YUV 422 or 420, even or odd
			op_mode_disp); //DSI operation mode, video or command
	}
	if (op_mode_disp == MIPI_DSI_OPERATION_MODE_VIDEO)
		lcd_venc_enable(pdrv, 1);
}

static void dsi_host_off_pre(struct aml_lcd_drv_s *pdrv)
{
	struct dsi_config_s *dconf = &pdrv->config.control.mipi_cfg;

	lcd_venc_enable(pdrv, 0);

	if (dconf->operation_mode_init != dconf->operation_mode_display) {
		set_mipi_dsi_host(pdrv, MIPI_DSI_VIRTUAL_CHAN_ID,
			0, //Chroma sub sample, only for YUV 422 or 420, even or odd
			dconf->operation_mode_init); //DSI operation mode, video or command
	}
}

static void dsi_switch_operation_mode(struct aml_lcd_drv_s *pdrv, u8 op_mode)
{
	set_mipi_dsi_host(pdrv, MIPI_DSI_VIRTUAL_CHAN_ID, 0, op_mode);
}

static void dsi_host_off_post(struct aml_lcd_drv_s *pdrv)
{
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s\n", pdrv->index, __func__);

	//bit[3]: ULPS exit on data
	//bit[2]: ULPS request on data
	//bit[1]: ULPS exit on clk
	//bit[0]: ULPS request on clk
	dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_ULPS_CTRL_OS, 0x5);

	/* Power down DSI */
	dsi_host_write(pdrv, MIPI_DSI_DWC_PWR_UP_OS, 0);

	/* Power down D-PHY, do not have to close dphy */
	/* dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_RSTZ_OS,
	 *	(dsi_host_read(pdrv, MIPI_DSI_DWC_PHY_RSTZ_OS ) & 0xc));
	 */
	/* dsi_host_write(pdrv, MIPI_DSI_DWC_PHY_RSTZ_OS, 0xc); */

	dsi_phy_write(pdrv, MIPI_DSI_CHAN_CTRL, 0x1f);
	//LCDPR("MIPI_DSI_PHY_CTRL=0x%x\n", dsi_phy_read(index, MIPI_DSI_PHY_CTRL));
	dsi_phy_setb(pdrv, MIPI_DSI_PHY_CTRL, 0, 7, 1);
}

struct dsi_ctrl_s dsi_ctrl_v1 = {
	.tx_ready = dsi_host_on_pre,
	.disp_on  = dsi_host_on_post,
	.disp_off = dsi_host_off_pre,
	.tx_close = dsi_host_off_post,

	.fr_change_pre = NULL,
	.fr_change_post = NULL,

	.host_config_print = host_config_print,

	.DT_generic_short_write = dsi_generic_write_short_packet,
	.DT_generic_read = dsi_generic_read_packet,
	.DT_DCS_short_write = dsi_dcs_write_short_packet,
	.DT_DCS_read = dsi_generic_read_packet,
	.DT_set_max_return_pkt_size = dsi_set_max_return_pkt_size,
	.DT_generic_long_write = dsi_write_long_packet,
	.DT_DCS_long_write = dsi_write_long_packet,
	.DT_sink_shut_down = dsi0_DT_sink_shut_down,
	.DT_sink_turn_on = dsi0_DT_sink_turn_on,

	.op_mode_switch = dsi_switch_operation_mode,
	.dphy_reset = NULL,
	.host_reset = NULL,
};

struct dsi_ctrl_s *dsi_bind_v1(struct aml_lcd_drv_s *pdrv)
{
	return &dsi_ctrl_v1;
}
