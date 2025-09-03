// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/eDPTX/DPTX.h>
#include "dptx_IP_ctrls.h"
#include "./dptx_IP_REG_T7.h"
#include "../dptx_regs.h"
#include "../dptx_common.h"

static void dptx_aux_request(struct dptx_drv_s *dptx, uint8_t port, struct dptx_aux_req_s *req)
{
	unsigned int state, timeout = 0;
	int i = 0;

	timeout = 0;
	while (timeout++ < DPTX_AUX_NO_REPLY_RETRY) {
		state = dptx_reg_getb(dptx, port, EDP_TX_AUX_STATE, 1, 1);
		if (state == 0)
			break;
		mdelay(DPTX_AUX_NO_REPLY_TIMEOUT);
	};

	// 20-bit address for native AUX requests or 8-bit address for I2C over AUX requests.
	dptx_reg_write(dptx, port, EDP_TX_AUX_ADDRESS, req->address & 0xfffff);
	/*submit data only for write commands*/
	if (req->cmd_code == DPTX_AUX_CMD_I2C_WRITE ||
	    req->cmd_code == DPTX_AUX_CMD_I2C_WRITE_MOT ||
	    req->cmd_code == DPTX_AUX_CMD_I2C_WRITE_STATUS ||
	    req->cmd_code == DPTX_AUX_CMD_WRITE) {
		for (i = 0; i < req->byte_cnt; i++)
			dptx_reg_write(dptx, port, EDP_TX_AUX_WRITE_FIFO, req->data[i]);
	}
	/*submit the command and the data size*/
	dptx_reg_write(dptx, port, EDP_TX_AUX_COMMAND,
		((0 << 12) | //ADDRESS_ONLY
		(req->cmd_code << 8) | // COMMAND
		((req->byte_cnt - 1) & 0xf))); //number of bytes
}

static uint8_t dptx_aux_submit_cmd(struct dptx_drv_s *dptx, uint8_t port,
						struct dptx_aux_req_s *req)
{
	unsigned int status = 0, reply = 0, dc;
	unsigned int retry_cnt = 0, timeout = 0;//, irq_status;//, i2c_read_timeout_ignore = 0;
	char str[48];

	if (!dptx)
		return 1;
	if (!dptx_reg_read(dptx, port, EDP_TX_TRANSMITTER_OUTPUT_ENABLE)) {
		DPTX_P_ERR(dptx, port, "%s: DPtx is not enabled", __func__);
		return 1;
	}

	switch (req->cmd_code) {
	case DPTX_AUX_CMD_I2C_WRITE:
		sprintf(str, "Aux I2C Write: 0x%04x", req->address);
		break;
	case DPTX_AUX_CMD_I2C_WRITE_MOT:
		sprintf(str, "Aux I2C Write MOT: 0x%04x", req->address);
		break;
	case DPTX_AUX_CMD_I2C_WRITE_STATUS:
		sprintf(str, "Aux I2C Write Status: 0x%04x", req->address);
		break;
	case DPTX_AUX_CMD_I2C_READ:
		sprintf(str, "Aux I2C Read: 0x%04x", req->address);
		// i2c_read_timeout_ignore = 1;
		break;
	case DPTX_AUX_CMD_I2C_READ_MOT:
		sprintf(str, "Aux I2C Read MOT: 0x%04x", req->address);
		// i2c_read_timeout_ignore = 1;
		break;
	case DPTX_AUX_CMD_READ:
		sprintf(str, "Aux Native Read: 0x%04x", req->address);
		break;
	case DPTX_AUX_CMD_WRITE:
		sprintf(str, "Aux Native Write: 0x%04x", req->address);
		break;
	default:
		DPTX_ERR(dptx, "%s: unknown Aux cmd\n", __func__);
		return 1;
	}

	// clean up reg
	dptx_reg_read(dptx, port, EDP_TX_AUX_TRANSFER_STATUS);
	dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_CODE);

dptx_aux_submit_cmd_retry:
	dptx_aux_request(dptx, port, req);

	timeout = 0;
	while (timeout++ < DPTX_AUX_REPLY_WAIT_TIMEOUT) {
		udelay(DPTX_AUX_REPLY_WAIT_TIMER);

		// irq_status = dptx_reg_read(dptx->idx, EDP_TX_AUX_INTERRUPT_STATUS);
		//REPLY_RECEIVED: AUX ACK, may not finished
		// IP doc not asked to read
		status = dptx_reg_read(dptx, port, EDP_TX_AUX_TRANSFER_STATUS);
		reply = dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_CODE);

		if (status & AUX_STATUS_REQUEST_IN_PROGRESS ||
		    status & AUX_STATUS_REPLY_IN_PROGRESS)
			continue;

		DPTX_P_DBG2(dptx, port, "%s, status=0x%x, reply=0x%x", str, status, reply);

		if (status & AUX_STATUS_REPLY_ERROR || status & AUX_STATUS_REPLY_RECEIVED) {
			switch (req->cmd_code) {
			case DPTX_AUX_CMD_I2C_WRITE:
			case DPTX_AUX_CMD_I2C_WRITE_MOT:
			case DPTX_AUX_CMD_I2C_WRITE_STATUS:
			case DPTX_AUX_CMD_WRITE:
				if (status & AUX_STATUS_REPLY_RECEIVED)
					return 0;
				if (status & AUX_STATUS_REPLY_ERROR) {
					DPTX_P_ERR(dptx, port, "%s, status=0x%x, reply=0x%x",
						str, status, reply);
					return 0;
				}
				break;
			case DPTX_AUX_CMD_READ:
				if (status & AUX_STATUS_REPLY_ERROR) {
					timeout = DPTX_AUX_REPLY_WAIT_TIMEOUT;
					break;
				}
				if (!(status & AUX_STATUS_REPLY_RECEIVED))
					break; // not in any state, loop to wait

				if (reply & AUX_REPLY_CODE_AUX_Defer) {
					DPTX_P_DBG(dptx, port, "%s Defer", str);

					if (retry_cnt++ < DPTX_AUX_NO_REPLY_RETRY) {
						mdelay(DPTX_AUX_NO_REPLY_TIMEOUT);
						goto dptx_aux_submit_cmd_retry;
					}
					break;
				}
				// DPCD addr not supported by DPRX, or invalid request
				if (reply == AUX_REPLY_CODE_AUX_NACK) {
					DPTX_P_DBG(dptx, port, "%s NACK", str);
					retry_cnt = DPTX_AUX_NO_REPLY_RETRY;
					timeout = DPTX_AUX_REPLY_WAIT_TIMEOUT;
					break;
				}
				return 0;
			case DPTX_AUX_CMD_I2C_READ:
			case DPTX_AUX_CMD_I2C_READ_MOT:
				if (status & AUX_STATUS_REPLY_ERROR) {
					timeout = DPTX_AUX_REPLY_WAIT_TIMEOUT;
					break;
				}
				if (!(status & AUX_STATUS_REPLY_RECEIVED))
					break; // not in any state, loop to wait

				if (reply & (AUX_REPLY_CODE_AUX_Defer | AUX_REPLY_CODE_I2C_Defer)) {
					dc = dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_DATA_COUNT);
					DPTX_P_DBG(dptx, port, "%s %s Defer dc:%d", str,
						(reply & AUX_REPLY_CODE_I2C_Defer) ?
							"I2C" : "", dc);

					if (dc) // read with data error
						return 1;

					if (retry_cnt++ < DPTX_AUX_NO_REPLY_RETRY) {
						mdelay(DPTX_AUX_NO_REPLY_TIMEOUT);
						goto dptx_aux_submit_cmd_retry;
					}
					break;
				}
				// DPCD / I2C addr not supported by DPRX, invalid request
				if (reply == AUX_REPLY_CODE_AUX_NACK ||
					reply == AUX_REPLY_CODE_I2C_NACK) {
					DPTX_P_DBG(dptx, port, "%s %s NACK", str,
						(reply & AUX_REPLY_CODE_I2C_NACK) ?
							"I2C" : "");
					retry_cnt = DPTX_AUX_NO_REPLY_RETRY;
					timeout = DPTX_AUX_REPLY_WAIT_TIMEOUT;
					break;
				}
				return 0;
			default:
				return 0;
			}
		}
	}

	if (retry_cnt++ < DPTX_AUX_NO_REPLY_RETRY) {
		mdelay(DPTX_AUX_NO_REPLY_TIMEOUT);
		DPTX_P_PR(dptx, port, "%s timeout, retry %d", str, retry_cnt);
		goto dptx_aux_submit_cmd_retry;
	}

	DPTX_P_ERR(dptx, port, "%s failed", str);
	return 1;
}

static uint8_t _aux_write(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf)
{
	struct dptx_aux_req_s aux_req;

	if (!buf)
		return 1;

	aux_req.cmd_code = DPTX_AUX_CMD_WRITE;
	// aux_req.cmd_state = 0;
	aux_req.address = addr;
	aux_req.byte_cnt = len;
	aux_req.data = buf;

	return dptx_aux_submit_cmd(dptx, port, &aux_req);
}

static uint8_t _aux_write_single(struct dptx_drv_s *dptx, uint8_t port, uint32_t addr, uint8_t val)
{
	struct dptx_aux_req_s aux_req;
	uint8_t auxdata = val;
	uint8_t ret;

	aux_req.cmd_code = DPTX_AUX_CMD_WRITE;
	// aux_req.cmd_state = 0;
	aux_req.address = addr;
	aux_req.byte_cnt = 1;
	aux_req.data = &auxdata;

	ret = dptx_aux_submit_cmd(dptx, port, &aux_req);
	return ret;
}

static uint8_t _aux_read(struct dptx_drv_s *dptx, uint8_t port,
					uint32_t addr, int len, uint8_t *buf)
{
	struct dptx_aux_req_s aux_req;
	int i;//, reply_count;

	if (!buf)
		return 1;

	aux_req.cmd_code = DPTX_AUX_CMD_READ;
	// aux_req.cmd_state = 0;
	aux_req.address = addr;
	aux_req.byte_cnt = len;
	aux_req.data = buf;

	if (dptx_aux_submit_cmd(dptx, port, &aux_req))
		return 1;

	// reply_count = dptx_reg_read(dptx->idx, EDP_TX_AUX_REPLY_DATA_COUNT);

	for (i = 0; i < len; i++)
		buf[i] = (unsigned char)(dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_DATA));

	return 0;
}

static uint8_t _aux_i2c_op(struct dptx_drv_s *dptx, uint8_t port,
			uint8_t cmd_type, uint32_t dev_addr, uint8_t len, uint8_t *data)
{
	struct dptx_aux_req_s aux_req;
	// unsigned char aux_data[4];
	uint8_t n = 0, reply_count = 0;
	uint8_t i, ret = 0;

	aux_req.cmd_code = cmd_type;
	// aux_req.cmd_state = 0;
	aux_req.address = dev_addr;
	aux_req.byte_cnt = len;

	switch (cmd_type) {
	case DPTX_AUX_CMD_I2C_WRITE:
	case DPTX_AUX_CMD_I2C_WRITE_MOT:
	case DPTX_AUX_CMD_I2C_WRITE_STATUS:
		aux_req.data = data;
		ret = dptx_aux_submit_cmd(dptx, port, &aux_req);
		udelay(100);
		break;
	case DPTX_AUX_CMD_I2C_READ:
	case DPTX_AUX_CMD_I2C_READ_MOT:
		ret = dptx_aux_submit_cmd(dptx, port, &aux_req);
		udelay(100);
		reply_count = dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_DATA_COUNT);
		if (reply_count != len) {
			DPTX_P_ERR(dptx, port, "Aux I2C cmd reply %d", reply_count);
			return -1;
		}
		for (i = 0; i < reply_count; i++) {
			data[n] = dptx_reg_read(dptx, port, EDP_TX_AUX_REPLY_DATA);
			n++;
		}
		break;
	case DPTX_AUX_CMD_READ:
	case DPTX_AUX_CMD_WRITE:
		DPTX_P_ERR(dptx, port, "%s: not dptx Aux I2C cmd", __func__);
		break;
	default:
		DPTX_P_ERR(dptx, port, "%s: unknown dptx Aux cmd", __func__);
		break;
	}
	return ret;
}

static void dptx_transmit_pattern(struct dptx_drv_s *dptx, uint8_t port,
					uint8_t pattern, uint8_t lane_mask)
{
	unsigned char dptx_ip_pat_sets[11] = {
		0x00, 0x01, 0x02, 0x03, 0xff, //TRAINING_PATTERN_SET: off, TPS1, TPS2, TPS3, TPS4
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
	unsigned int reg_val;

	switch (pattern) {
	case DPTX_TPS_DISABLE:
	case DPTX_TPS1:
	case DPTX_TPS2:
	case DPTX_TPS3:
		reg_val = dptx_ip_pat_sets[pattern];
		dptx_reg_write(dptx, port,
			EDP_TX_SCRAMBLING_DISABLE, DP_test_pat[pattern].SR_disable);
		dptx_reg_write(dptx, port, EDP_TX_TRAINING_PATTERN_SET, reg_val);
		break;
	case DPTX_QUAL_PAT_DISABLE:
	case DPTX_D10P2:
	case DPTX_SYMBOL_ERROR_MSR:
	case DPTX_PRBS7:
	case DPTX_HBR2_EYE:
		reg_val = (lane_mask & BIT(0) ? dptx_ip_pat_sets[pattern] << 0  : 0) |
			  (lane_mask & BIT(1) ? dptx_ip_pat_sets[pattern] << 8  : 0) |
			  (lane_mask & BIT(2) ? dptx_ip_pat_sets[pattern] << 16 : 0) |
			  (lane_mask & BIT(3) ? dptx_ip_pat_sets[pattern] << 24 : 0);
		dptx_reg_write(dptx, port,
			EDP_TX_SCRAMBLING_DISABLE, DP_test_pat[pattern].SR_disable);
		dptx_reg_write(dptx, port, EDP_TX_LINK_QUAL_PATTERN_SET, reg_val);
	case DPTX_80BIT_CUSTOM:
	case DPTX_TPS4:
		DPTX_ERR(dptx, "%s: %s unsupported", __func__, DP_test_pat[pattern].name);
		break;
	default:
		DPTX_ERR(dptx, "%s: %d invalid", __func__, pattern);
		break;
	}
}

static void dptx_set_MSA(struct dptx_drv_s *dptx, uint8_t port)
{
	unsigned int hactive, vactive, htotal, vtotal, hsw, hbp, vsw, vbp;
	unsigned int bpp, data_per_lane, misc0_data, bit_depth, sync_mode;
	unsigned int m_vid; /*pclk/1000 */
	unsigned int n_vid; /*162000, 270000, 540000 */
	unsigned int ppc = 1; /* 1 pix per clock pix0 only */
	unsigned int cfmt;
	uint8_t port_cnt = dptx->sink.port_mask == 0x3 ? 2 : 1;

	hactive = dptx->act_timing.h_act    / port_cnt;
	htotal  = dptx->act_timing.h_period / port_cnt;
	hsw     = dptx->act_timing.h_pw     / port_cnt;
	hbp     = dptx->act_timing.h_bp     / port_cnt;
	vactive = dptx->act_timing.v_act;
	vtotal  = dptx->act_timing.v_period;
	vsw     = dptx->act_timing.v_pw;
	vbp     = dptx->act_timing.v_fp;

	// m_vid = dptx->act_timing.pclk / 1000;
	m_vid = (dptx->vid_clk.fout / port_cnt) / 1000;
	if (dptx->sink.link[port]->link_rate == DP_LINK_RATE_HBR2)
		n_vid = 540000;
	else if (dptx->sink.link[port]->link_rate == DP_LINK_RATE_HBR)
		n_vid = 270000;
	else
		n_vid = 162000;

	switch (dptx->act_timing.cfmt) {
	case DPTX_CFMT_RGB_8bit:
		bit_depth = 0x1; cfmt = 0; bpp = 24; break;
	case DPTX_CFMT_RGB_10bit:
		bit_depth = 0x2; cfmt = 0; bpp = 30; break;
	case DPTX_CFMT_RGB_12bit:
		bit_depth = 0x3; cfmt = 0; bpp = 36; break;
	case DPTX_CFMT_YCbCr422_8bit:
		bit_depth = 0x1; cfmt = 1; bpp = 16; break;
	case DPTX_CFMT_YCbCr422_10bit:
		bit_depth = 0x2; cfmt = 1; bpp = 20; break;
	case DPTX_CFMT_YCbCr422_12bit:
		bit_depth = 0x3; cfmt = 1; bpp = 24; break;
	case DPTX_CFMT_YCbCr444_8bit:
		bit_depth = 0x1; cfmt = 2; bpp = 24; break;
	case DPTX_CFMT_YCbCr444_10bit:
		bit_depth = 0x2; cfmt = 2; bpp = 30; break;
	case DPTX_CFMT_YCbCr444_12bit:
		bit_depth = 0x3; cfmt = 2; bpp = 36; break;
	case DPTX_CFMT_RGB_6bit:
	case DPTX_CFMT_invalid:
	default:
		bit_depth = 0x0; cfmt = 0; bpp = 18; break;
	}

	sync_mode = dptx->sink.link[port]->sync_clk_mode;
	data_per_lane = ((hactive * bpp) + 15) / 16 - 1;

	/*bit[0] sync mode (1=sync 0=async) */
	misc0_data = (bit_depth << 5) | (cfmt << 1) | (sync_mode << 0);

	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_HTOTAL, htotal);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_VTOTAL, vtotal);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_POLARITY, (0 << 1) | (0 << 0));
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_HSWIDTH, hsw);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_VSWIDTH, vsw);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_HRES, hactive);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_VRES, vactive);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_HSTART, (hsw + hbp));
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_VSTART, (vsw + vbp));
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_MISC0, misc0_data);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_MISC1, 0x00000000);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_M_VID, m_vid); /*unit: 1kHz */
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_N_VID, n_vid); /*unit: 10kHz */
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_TRANSFER_UNIT_SIZE, 48);
		/*Temporary change to 48 */
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_DATA_COUNT_PER_LANE, data_per_lane);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_USER_PIXEL_WIDTH, ppc);
}

static void dptx_reset_t7(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask)
{
	unsigned int bit;

	if (mask & DPTX_RESET_PHY) {
		dptx_reg_write(dptx, port, EDP_TX_PHY_RESET, 0xf); //reset the PHY
		mdelay(1);
	}

	if (mask & DPTX_RESET_COMBO_DPHY) {
		bit = (dptx->idx == 0 && port == 0) ? 19 : 20; //combo dphy
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 0, bit, 1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit, 1);
		udelay(1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit, 1);
		udelay(5);
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 1, bit, 1);
	}

	if (mask & DPTX_RESET_eDP_PIPE) {
		DPTX_PR(dptx, "edp_pipe = %u =  %u,", dptx->idx, port);

		bit = (dptx->idx == 0 && port == 0) ? 27 : 26; //eDP pipeline
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 0, bit, 1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit, 1);
		udelay(1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit, 1);
		udelay(5);
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 1, bit, 1);
	}

	if (mask & DPTX_RESET_eDP_CTRL) {
		bit = (dptx->idx == 0 && port == 0) ? 17 : 18; //eDP ctrl
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 0, bit, 1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 0, bit, 1);
		udelay(1);
		dptx_reset_setb(RESETCTRL_RESET1_LEVEL, 1, bit, 1);
		udelay(5);
		dptx_reset_setb(RESETCTRL_RESET1_MASK, 1, bit, 1);
	}

	if (mask & (DPTX_RESET_eDP_CTRL | DPTX_RESET_eDP_PIPE | DPTX_RESET_COMBO_DPHY))
		mdelay(1);

	if (mask & DPTX_RESET_AUX_CLK_DIVIDER) {
		//Set Aux channel clk-div: 24MHz
		dptx_reg_write(dptx, port, EDP_TX_AUX_CLOCK_DIVIDER, 24);
	}

	if (mask & DPTX_RESET_PHY) {
		//Enable the transmitter
		//remove the reset on the PHY
		dptx_reg_write(dptx, port, EDP_TX_PHY_RESET, 0);
	}

	if (mask & DPTX_RESET_VENC) {
		bit = (dptx->idx == 0) ? 24 : 27;
		dptx_reset_setb(RESETCTRL_RESET0_MASK, 0, bit, 1);
		dptx_reset_setb(RESETCTRL_RESET0_LEVEL, 0, bit, 1);
		udelay(1);
		dptx_reset_setb(RESETCTRL_RESET0_LEVEL, 1, bit, 1);
		udelay(5);
		dptx_reset_setb(RESETCTRL_RESET0_MASK, 1, bit, 1);
	}
}

static void dptx_set_lane_config_to_IP(struct dptx_drv_s *dptx, uint8_t port)
{
	// tx Link-rate and Lane_count
	dptx_reg_write(dptx, port, EDP_TX_LINK_BW_SET, dptx->sink.link[port]->link_rate);
	dptx_reg_write(dptx, port, EDP_TX_LINK_COUNT_SET, dptx->sink.link[port]->lane_count);
	dptx_reg_write(dptx, port, EDP_TX_ENHANCED_FRAME_EN, dptx->sink.link[port]->enh_frame_en);
	dptx_reg_write(dptx, port,
		EDP_TX_PHY_POWER_DOWN, (0xf << dptx->sink.link[port]->lane_count) & 0xf);
	dptx_reg_write(dptx, port,
		EDP_TX_DOWNSPREAD_CTRL, dptx->sink.link[port]->link_cap & BIT(5) ? 1 : 0);
}

static void dptx_set_phy_config_to_IP(struct dptx_drv_s *dptx, uint8_t port, uint8_t use_preset)
{
	uint8_t i, ds_val[4];

	for (i = 0; i < 4; i++) {
		ds_val[i] = use_preset ?
			dptx->sink.link[port]->preset_ds[i] : dptx->sink.link[port]->adj_req_ds[i];
	}

	dptx_reg_write(dptx, port, EDP_TX_PHY_VOLTAGE_DIFF_LANE_0, dptx_ds_to_vswing(ds_val[0]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_VOLTAGE_DIFF_LANE_1, dptx_ds_to_vswing(ds_val[1]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_VOLTAGE_DIFF_LANE_2, dptx_ds_to_vswing(ds_val[2]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_VOLTAGE_DIFF_LANE_3, dptx_ds_to_vswing(ds_val[3]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_PRE_EMPHASIS_LANE_0, dptx_ds_to_preem(ds_val[0]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_PRE_EMPHASIS_LANE_1, dptx_ds_to_preem(ds_val[1]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_PRE_EMPHASIS_LANE_2, dptx_ds_to_preem(ds_val[2]));
	dptx_reg_write(dptx, port, EDP_TX_PHY_PRE_EMPHASIS_LANE_3, dptx_ds_to_preem(ds_val[3]));
}

static int dptx_wait_phy_ready(struct dptx_drv_s *dptx, u8 port)
{
	uint32_t data = 0;
	uint8_t done = 0;

	do {
		data = dptx_reg_read(dptx, port, EDP_TX_PHY_STATUS);
		if (done > 20)
			DPTX_P_DBG(dptx, port, "wait phy ready: val=0x%x, cnt=%u", data, done);
		done++;
		udelay(100);
	} while (((data & 0x7f) != 0x7f) && (done < 100));

	if ((data & 0x7f) == 0x7f)
		return 0;

	DPTX_P_ERR(dptx, port, "phy init error!");
	return -1;
}

static void dptx_transmitter_init(struct dptx_drv_s *dptx, uint8_t port)
{
	dptx_reset_t7(dptx, port, DPTX_RESET_AUX_CLK_DIVIDER | DPTX_RESET_PHY);
	dptx_wait_phy_ready(dptx, port);
	mdelay(1);

	dptx_reg_write(dptx, port, EDP_TX_TRANSMITTER_OUTPUT_ENABLE, 0x1);
	//dptx_reg_write(dptx->idx, EDP_TX_AUX_INTERRUPT_MASK, 0); //turn off interrupt

	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_ENABLE, 0x0);
}

static void dptx_transmitter_output_set(struct dptx_drv_s *dptx, uint8_t port, uint8_t en)
{
	//if (en)
	//	dptx_reg_write(dptx, port, EDP_TX_TRANSMITTER_OUTPUT_ENABLE, 1);
	//else
	//	dptx_reg_write(dptx, port, EDP_TX_FORCE_SCRAMBLER_RESET, 0x1);
	dptx_reg_write(dptx, port, EDP_TX_MAIN_STREAM_ENABLE, en ? 0x1 : 0);

	if (!en)
		dptx_reg_write(dptx, port, EDP_TX_TRANSMITTER_OUTPUT_ENABLE, 0x0);
}

static uint8_t dptx_get_hpd_level(struct dptx_drv_s *dptx, uint8_t port)
{
	return dptx_reg_getb(dptx, port, EDP_TX_AUX_STATE, 0, 1);
}

static uint16_t dptx_get_hpd_irq(struct dptx_drv_s *dptx, uint8_t port)
{
/* INTERRUPT_STATUS
 * The transmitter core interrupt status register contains the cause of an interrupt asserted by
 * the core. The specific events that can cause an interrupt and the associated status bits are
 * shown below. A read from this register clears all values.
 * bit[3] – REPLY_TIMEOUT: a reply timeout has occurred when the sink has not sent a response
 * 400us after the transmitter has sent a request.
 * bit[2] – REPLY_RECEIEVED: an AUX reply transaction has been detected. This value may be used
 * to allow a system to process other events while waiting for a response from the sink device.
 * bit[1] – HPD_EVENT: the core has detected the presence of the HPD signal. This interrupt
 * asserts immediately after the detection of HPD and after the loss of HPD for 2 msec.
 * bit[0] – HPD_IRQ: an IRQ framed with the proper timing on the HPD signal has been detected.
 */
	return dptx_reg_read(dptx, port, EDP_TX_AUX_INTERRUPT_STATUS);
}

static void dptx_interrupt_mask_set(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask)
{
	//if (en) {
	//	dptx_reg_write(dptx->idx, EDP_TX_AUX_INTERRUPT_MASK, 0xc);
		// __dptx_reg_read(dptx, EDP_TX_AUX_INTERRUPT_STATUS);
	//} else {
	//	dptx_reg_write(dptx->idx, EDP_TX_AUX_INTERRUPT_MASK, 0xf);
	//}

	dptx_reg_write(dptx, port, EDP_TX_AUX_INTERRUPT_MASK, 0xf & ~(mask));

	if (mask)
		dptx_reg_read(dptx, port, EDP_TX_AUX_INTERRUPT_STATUS);
}

static void dptx_set_scramble_reset(struct dptx_drv_s *dptx, uint8_t port, uint8_t sr_type)
{
	dptx_reg_write(dptx, port, EDP_TX_SCRAMBLING_DISABLE,
		sr_type == DPTX_SCRAMBLE_RESET_OFF ? 0x01 : 0x00);

	dptx_reg_write(dptx, port, EDP_TX_ALTERNATE_SCRAMBLER_RESET,
		sr_type == DPTX_eDP_ALTERNATIVE_SCRAMBLE_RESET ? 0x01 : 0x00);

	dptx_reg_write(dptx, port, EDP_TX_FORCE_SCRAMBLER_RESET, 0x1);
}

void dptx_PSR1_ctrl_set(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag)
{
	dptx_reg_write(dptx, port, EDP_TX_PANEL_SELF_REFRESH, flag ? 0x01 : 0x00);
}

//only 16bit
static void dptx_reg_store_data(struct dptx_drv_s *dptx, uint8_t port, uint32_t d0, uint32_t d1)
{
	dptx_reg_write(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_0, d0 & 0xf);
	dptx_reg_write(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_1, (d0 >> 4) & 0xf);
	dptx_reg_write(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_2, (d0 >> 8) & 0xf);
	dptx_reg_write(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_3, (d0 >> 12) & 0xf);
}

static void dptx_reg_get_data(struct dptx_drv_s *dptx, uint8_t port, uint32_t *d0, uint32_t *d1)
{
	if (d0) {
		*d0 = 0;
		*d0 |= dptx_reg_read(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_0) & 0xf;
		*d0 |= (dptx_reg_read(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_1) & 0xf) << 4;
		*d0 |= (dptx_reg_read(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_2) & 0xf) << 8;
		*d0 |= (dptx_reg_read(dptx, port, EDP_TX_PHY_POST_EMPHASIS_LANE_3) & 0xf) << 12;
	}
	if (d1)
		*d1 = 0;
}

struct dptx_if_ctrl_s dptx_if_t7 = {
	.aux_write = _aux_write,
	.aux_write_single = _aux_write_single,
	.aux_read = _aux_read,
	.aux_i2c_op = _aux_i2c_op,

	.transmit_pattern = dptx_transmit_pattern,
	.set_MSA = dptx_set_MSA,

	.path_reset = dptx_reset_t7,

	.lane_cfg_to_IP = dptx_set_lane_config_to_IP,
	.phy_cfg_to_IP = dptx_set_phy_config_to_IP,

	.transmitter_init = dptx_transmitter_init,
	.transmitter_output = dptx_transmitter_output_set,

	.get_hpd_level = dptx_get_hpd_level,
	.get_hpd_irq = dptx_get_hpd_irq,
	.set_hpd_interrupt_mask = dptx_interrupt_mask_set,

	.scramble_reset_set = dptx_set_scramble_reset,

	.PSR1_SDP_ctrl = dptx_PSR1_ctrl_set,
	.PSR2_SDP_en = NULL,

	.reg_store = dptx_reg_store_data,
	.reg_store_get = dptx_reg_get_data,
};

struct dptx_if_ctrl_s *dptx_if_bind_t7(struct dptx_drv_s *dptx)
{
	return &dptx_if_t7;
}
