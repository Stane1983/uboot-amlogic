// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPCD_REG.h>
#include "DPTX_IP/dptx_IP_ops.h"
#include "dptx_common.h"

#define DP_FULL_LINK_TRAINING_ATTEMPTS 3
#define DP_TRAINING_LINKRATE_ATTEMPTS  3
#define DP_CLOCK_REC_TIMEOUT    150
#define DP_TRAINING_CR_ATTEMPTS 5
#define DP_TRAINING_EQ_ATTEMPTS 5

enum DP_training_status_e {
	DP_TRAINING_CLOCK_REC,
	DP_TRAINING_CHANNEL_EQ,
	DP_TRAINING_SUCCESS,
	DP_TRAINING_FAILED,
	DP_TRAINING_ADJ_SPD_CR_FAIL,
	DP_TRAINING_ADJ_SPD_CR_FAIL_IN_EQ,
	DP_TRAINING_ADJ_SPD_EQ_FAIL_OVERTIME,
};

struct DPTX_test_pat_s DP_test_pat[] = {
	{"TPS_DISABLE",        0x00, 0, 1}, // TPS_DISABLE
	{"TPS1",               0x01, 1, 1}, // TPS1
	{"TPS2",               0x02, 1, 1}, // TPS2
	{"TPS3",               0x03, 1, 1}, // TPS3
	{"TPS4",               0x07, 1, 1}, // TPS4
	{"QUAL_PAT_DISABLE",   0x00, 0, 1}, // QUAL_PAT_DISABLE
	{"D10.2(unscrambled)", 0x01, 1, 1}, // D10.2
	{"SYMBOL_ERROR_MSR",   0x02, 0, 1}, // SYMBOL_ERROR_MSR
	{"PRBS7",              0x03, 1, 0}, // PRBS7
	{"80BIT_CUSTOM",       0x04, 1, 0}, // 80BIT_CUSTOM
	{"HBR2_EYE",           0x05, 0, 1}, // HBR2_EYE
};

static void dptx_training_status_print(struct dptx_drv_s *dptx)
{
	unsigned char aux_data[3];
	int ret;

	ret = __dptx_aux_read(dptx, DPCD_LANE0_1_STATUS, 3, aux_data);
	if (ret == 0) {
		DPTXPR(dptx->idx, LOG_V, "%s: aux_data [0]=0x%x, [1]=0x%x, [2]=0x%x",
			__func__, aux_data[0], aux_data[1], aux_data[2]);
	}
}

/* @return:
 * 0: link rate reduced
 * 1: link rate already lowest RBR (1.62G)
 * 2: link rate not cover resolution (eDP)
 */
static int dptx_link_rate_config_reduce(struct dptx_drv_s *dptx)
{
	unsigned char link_rate, lane_cnt;

	link_rate = dptx->link_cfg.link_rate;
	lane_cnt = dptx->link_cfg.lane_count;

	if (link_rate > DP_LINK_RATE_HBR2) {
		link_rate = DP_LINK_RATE_HBR2;
	} else if (link_rate > DP_LINK_RATE_HBR) {
		link_rate = DP_LINK_RATE_HBR;
	} else if (link_rate > DP_LINK_RATE_RBR) {
		link_rate = DP_LINK_RATE_RBR;
	} else {
		DPTXPR(dptx->idx, LOG_E, "%s: already lowest link rate", __func__);
		return 1;
	}

	DPTXPR(dptx->idx, LOG_I, "%s: link_rate: 0x%x, lane: %d", __func__, link_rate, lane_cnt);
	dptx->link_cfg.link_rate = link_rate;
	dptx->link_cfg.link_rate_update = 1;

	return 0;
}

/* @brief:
 * 1. read DPCD_ADJUST_REQUEST_LANE0_1 2_3
 * 2. store to edp_cfg: [adj_req_preem, adj_req_vswing]
 * 3. compare between [curr*, adj_reg*], set phy_update tag
 */
static int dptx_training_phy_adj_req_process(struct dptx_drv_s *dptx)
{
	struct dptx_link_cfg_s *link_s = &dptx->link_cfg;
	unsigned char adj_req[2], i;

	if (__dptx_aux_read(dptx, DPCD_ADJUST_REQUEST_LANE0_1, 2, adj_req)) {
		DPTXPR(dptx->idx, LOG_E, "%s: aux read DPCD_ADJUST_REQUEST failed", __func__);
		dptx->link_cfg.phy_update = 0;
		return 0;
	}
	link_s->adj_req_ds[0] = dptx_v_p_to_ds(((adj_req[0] >> 0) & 0x3), (adj_req[0] >> 2) & 0x3);
	link_s->adj_req_ds[1] = dptx_v_p_to_ds(((adj_req[0] >> 4) & 0x3), (adj_req[0] >> 6) & 0x3);
	link_s->adj_req_ds[2] = dptx_v_p_to_ds(((adj_req[1] >> 0) & 0x3), (adj_req[1] >> 2) & 0x3);
	link_s->adj_req_ds[3] = dptx_v_p_to_ds(((adj_req[1] >> 4) & 0x3), (adj_req[1] >> 6) & 0x3);

	//determine vswing & preem change on any lane
	link_s->phy_update = 0;
	for (i = 0; i < 4; i++)
		link_s->phy_update |= (link_s->adj_req_ds[i] != link_s->curr_ds[i]) << i;

	DPTXPR(dptx->idx, LOG_I, "%s: phy update: 0x%x", __func__, link_s->phy_update);

	if (!(dptx_print_level >= LOG_V && link_s->phy_update))
		return 0;
	pr_info(" ----- 0:Vsw/Pre - 1:Vsw/Pre - 2:Vsw/Pre - 3:Vsw/Pre --\n"
		" adj_req:  %d %d   |   %d %d   |   %d %d   |   %d %d\n"
		" current:  %d %d   |   %d %d   |   %d %d   |   %d %d\n",
		dptx_ds_to_vswing(link_s->adj_req_ds[0]), dptx_ds_to_preem(link_s->adj_req_ds[0]),
		dptx_ds_to_vswing(link_s->adj_req_ds[1]), dptx_ds_to_preem(link_s->adj_req_ds[1]),
		dptx_ds_to_vswing(link_s->adj_req_ds[2]), dptx_ds_to_preem(link_s->adj_req_ds[2]),
		dptx_ds_to_vswing(link_s->adj_req_ds[3]), dptx_ds_to_preem(link_s->adj_req_ds[3]),
		dptx_ds_to_vswing(link_s->curr_ds[0]),    dptx_ds_to_preem(link_s->curr_ds[0]),
		dptx_ds_to_vswing(link_s->curr_ds[1]),    dptx_ds_to_preem(link_s->curr_ds[1]),
		dptx_ds_to_vswing(link_s->curr_ds[2]),    dptx_ds_to_preem(link_s->curr_ds[2]),
		dptx_ds_to_vswing(link_s->curr_ds[3]),    dptx_ds_to_preem(link_s->curr_ds[3]));
	return 0;
}

static void dptx_training_phy_update_apply(struct dptx_drv_s *dptx)
{
	struct dptx_link_cfg_s *cfg = &dptx->link_cfg;
	uint8_t i;

	if (cfg->phy_update == 0)
		return;

	for (i = 0; i < 4; i++)
		cfg->curr_ds[i] = cfg->adj_req_ds[i];

	__dptx_set_phy_config(dptx, 0);

	cfg->phy_update = 0;
}

/* *************** full link training ****************** */
int dptx_set_pattern(struct dptx_drv_s *dptx, unsigned char pattern)
{
	unsigned char _data;//, scrambling_en, encoding_en;
	int ret = 0;

	/* DPCD reg:
	 * TRAINING_PATTERN_SET
	 *   bit[1:0]:TRAINING_PATTERN_SELECT
	 *     00:off, 01:TPS1, 10:TPS2, 11:TPS3(DPCD1.2+)
	 *   bit[3:2]:LINK_QUAL_PATTERN_SET                            ! DPCD1.1
	 *     00:off, 01:D10.2, 10:Symbol-Error-Rate-msr, 11:PRBS7
	 *   bit[3:2]:ALWAYS 00  ! DPCD1.2 & 1.3 (per-lane control in LINK_QUAL_LANEn_SET)
	 *   bit[3:0]:0x0:off, 0x1:TPS1, 0x2:TPS2, 0x3:TPS3, 0x7:TPS4  ! DPCD1.4 (8b/10b)
	 *   bit[3:0]:0x0:off, 0x1:128b/132b_TPS1, 0x2:128b/132b_TPS2  ! DPCD1.4 (128b/132b)
	 *   bit[4]  :RECOVERED_CLOCK_OUT_EN (not available in 128b/132b)
	 *   bit[5]  :SCRAMBLING_DISABLE
	 *   bit[7:6]:SYMBOL ERROR COUNT SEL (not available in 128b/132b)
	 */

	switch (pattern) {
	case DPTX_TPS_DISABLE:
	case DPTX_TPS1:
	case DPTX_TPS2:
	case DPTX_TPS3:
		dptx_transmit_pattern(dptx, pattern, 0xf);
		_data = (DP_test_pat[pattern].data & 0x3) |
			(DP_test_pat[pattern].SR_disable << 5) |
			(0x3 << 6);
		ret = __dptx_aux_write_single(dptx, DPCD_TRAINING_PATTERN_SET, _data);
		if (dptx_print_level >= LOG_I || ret)
			DPTXPR(dptx->idx, LOG_I, "%s: %s %s", __func__,
				DP_test_pat[pattern].name, ret ? "failed" : "");
		break;
	case DPTX_QUAL_PAT_DISABLE:
	case DPTX_D10P2:
	case DPTX_SYMBOL_ERROR_MSR:
	case DPTX_PRBS7:
	case DPTX_HBR2_EYE:
		dptx_transmit_pattern(dptx, pattern, 0xf);
		_data = (DP_test_pat[pattern].SR_disable << 5) | (0x3 << 6);
		ret = __dptx_aux_write_single(dptx, DPCD_TRAINING_PATTERN_SET, _data);
		// ! DPCD1.0/1.1 should use DPCD_TRAINING_PATTERN_SET[2:3]
		// in case of year 2024, no device using DPCD1.0/1.1? ignore
		_data = (DP_test_pat[pattern].data & 0x7);
		ret = __dptx_aux_write_single(dptx, DPCD_LINK_QUAL_LANE0_SET, _data);
		ret = __dptx_aux_write_single(dptx, DPCD_LINK_QUAL_LANE1_SET, _data);
		ret = __dptx_aux_write_single(dptx, DPCD_LINK_QUAL_LANE2_SET, _data);
		ret = __dptx_aux_write_single(dptx, DPCD_LINK_QUAL_LANE3_SET, _data);
		if (dptx_print_level >= LOG_I || ret)
			DPTXPR(dptx->idx, LOG_I, "%s: %s %s", __func__,
				DP_test_pat[pattern].name, ret ? "failed" : "");
		break;
	case DPTX_80BIT_CUSTOM:
	case DPTX_TPS4:
		DPTXPR(dptx->idx, LOG_E, "%s: %s unsupported", __func__, DP_test_pat[pattern].name);
		break;
	default:
		DPTXPR(dptx->idx, LOG_E, "%s: (%u) invalid", __func__, pattern);
		break;
	}

	return ret;
}

// Lane status register constants
#define BIT_DPCD_LANE_02_STATUS_CLK_REC_DONE  0
#define BIT_DPCD_LANE_02_STATUS_CHAN_EQ_DONE  1
#define BIT_DPCD_LANE_02_STATUS_SYM_LOCK_DONE 2
#define BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE  4
#define BIT_DPCD_LANE_13_STATUS_CHAN_EQ_DONE  5
#define BIT_DPCD_LANE_13_STATUS_SYM_LOCK_DONE 6
#define BIT_DPCD_LANE_ALIGNMENT_DONE          0

static int dptx_check_channel_equalization(struct dptx_drv_s *dptx)
{
	unsigned int cr_done, channel_eq_done, symbol_lock_done, lane_align_done;
	unsigned char clk_rec[4], chan_eq[4], sym_lock[4], aux_data[3];
	unsigned char lane_cnt;
	int ret;

	lane_cnt = dptx->link_cfg.lane_count;

	ret = __dptx_aux_read(dptx, DPCD_LANE0_1_STATUS, 3, aux_data);
	if (ret) {
		DPTXPR(dptx->idx, LOG_E, "%s: aux read DPCD_LANE0_1_STATUS failed\n", __func__);
		return -1;
	}

	clk_rec[0]  = ((aux_data[0] >> BIT_DPCD_LANE_02_STATUS_CLK_REC_DONE)  & 0x01);
	clk_rec[1]  = ((aux_data[0] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE)  & 0x01);
	clk_rec[2]  = ((aux_data[1] >> BIT_DPCD_LANE_02_STATUS_CLK_REC_DONE)  & 0x01);
	clk_rec[3]  = ((aux_data[1] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE)  & 0x01);
	chan_eq[0]  = ((aux_data[0] >> BIT_DPCD_LANE_02_STATUS_CHAN_EQ_DONE)  & 0x01);
	chan_eq[1]  = ((aux_data[0] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE)  & 0x01);
	chan_eq[2]  = ((aux_data[1] >> BIT_DPCD_LANE_02_STATUS_CHAN_EQ_DONE)  & 0x01);
	chan_eq[3]  = ((aux_data[1] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE)  & 0x01);
	sym_lock[0] = ((aux_data[0] >> BIT_DPCD_LANE_02_STATUS_SYM_LOCK_DONE) & 0x01);
	sym_lock[1] = ((aux_data[0] >> BIT_DPCD_LANE_13_STATUS_SYM_LOCK_DONE) & 0x01);
	sym_lock[2] = ((aux_data[1] >> BIT_DPCD_LANE_02_STATUS_SYM_LOCK_DONE) & 0x01);
	sym_lock[3] = ((aux_data[1] >> BIT_DPCD_LANE_13_STATUS_SYM_LOCK_DONE) & 0x01);

	cr_done          =  clk_rec[0] +  clk_rec[1] +  clk_rec[2] +  clk_rec[3];
	channel_eq_done  =  chan_eq[0] +  chan_eq[1] +  chan_eq[2] +  chan_eq[3];
	symbol_lock_done = sym_lock[0] + sym_lock[1] + sym_lock[2] + sym_lock[3];

	lane_align_done = ((aux_data[2] >> BIT_DPCD_LANE_ALIGNMENT_DONE) & 0x01);

	DPTXPR(dptx->idx, LOG_V,
		"%s: CR:[%d %d %d %d] - EQ:[%d %d %d %d] - SymLock:[%d %d %d %d] - Align:%d",
		__func__, clk_rec[0],  clk_rec[1],  clk_rec[2],  clk_rec[3],
		chan_eq[0],  chan_eq[1],  chan_eq[2],  chan_eq[3],
		sym_lock[0], sym_lock[1], sym_lock[2], sym_lock[3], lane_align_done);

	if (cr_done != lane_cnt)
		return 1;
	if (channel_eq_done == lane_cnt && symbol_lock_done == lane_cnt && lane_align_done)
		return 0;

	return 2;
}

static int dptx_run_channel_equalization_loop(struct dptx_drv_s *dptx)
{
	int i = 0, ret = -1;

	if (dptx->link_cfg.TPS_support & BIT(1))
		dptx_set_pattern(dptx, DPTX_TPS4);
	else if (dptx->link_cfg.TPS_support & BIT(0))
		dptx_set_pattern(dptx, DPTX_TPS3);
	else
		dptx_set_pattern(dptx, DPTX_TPS2);

	//channel equalization & symbol lock loop
	while (i++ < DP_TRAINING_EQ_ATTEMPTS) {
		dptx_training_phy_update_apply(dptx);

		udelay(dptx_training_rd_interval[dptx->link_cfg.train_aux_rd_interval]);
		ret = dptx_check_channel_equalization(dptx);
		if (ret == 0) //success
			break;
		if (ret < 0) //aux error
			continue;

		//if ((dptx->config.control.edp_cfg.curr_vswing[0] & 0x03) != VAL_DP_std_LEVEL_3)
		//!!!!! judged by 4 lane !!!!!!!
		//five times?
		dptx_training_phy_adj_req_process(dptx);
	}

	return ret;
}

static int dptx_check_clk_recovery(struct dptx_drv_s *dptx)
{
	unsigned char cr_done[4], aux_data[2];
	unsigned char lane_cnt, cr_all_done = 0;
	int ret;

	lane_cnt = dptx->link_cfg.lane_count;

	ret = __dptx_aux_read(dptx, DPCD_LANE0_1_STATUS, 2, aux_data);
	if (ret) { //clear cr_done flags on failure of AUX transaction
		DPTXPR(dptx->idx, LOG_E, "%s: aux read failed", __func__);
		return -1;
	}

	cr_done[0] = ((aux_data[0] >> BIT_DPCD_LANE_02_STATUS_CLK_REC_DONE) & 0x01);
	cr_done[1] = ((aux_data[0] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE) & 0x01);
	cr_done[2] = ((aux_data[1] >> BIT_DPCD_LANE_02_STATUS_CLK_REC_DONE) & 0x01);
	cr_done[3] = ((aux_data[1] >> BIT_DPCD_LANE_13_STATUS_CLK_REC_DONE) & 0x01);

	cr_all_done = cr_done[0] + cr_done[1] + cr_done[2] + cr_done[3];

	DPTXPR(dptx->idx, LOG_I, "%s: CR result: [%d, %d, %d, %d]", __func__,
		cr_done[0], cr_done[1], cr_done[2], cr_done[3]);

	if (cr_all_done == lane_cnt)
		return 0;

	return 1;
}

static int dptx_run_clk_recovery_loop(struct dptx_drv_s *dptx)
{
	int i = 0, ret = 0;

	dptx_set_pattern(dptx, DPTX_TPS1);

	while (i++ < DP_TRAINING_CR_ATTEMPTS) {
		dptx_training_phy_update_apply(dptx);

		mdelay(DP_CLOCK_REC_TIMEOUT); //wait for clock recovery
		ret = dptx_check_clk_recovery(dptx);
		if (ret == 0) //success
			break;
		if (ret < 0) //aux error
			break;

		// TODO: check for max vswing level
		//if ((dptx->config.control.edp_cfg.preset_vswing_rx[0] & 0x07) != 0x07)
		//max Vod or same  5 times?
		dptx_training_phy_adj_req_process(dptx);
	}

	return ret;
}

static int dptx_link_config_update(struct dptx_drv_s *dptx)
{
	if (dptx->link_cfg.link_rate_update == 0)
		return 0;

	dptx_clk_set_link_clk(dptx, dptx->link_cfg.link_rate);

	__dptx_set_lane_config(dptx);

	return 0;
}

static int dptx_run_training_loop(struct dptx_drv_s *dptx, unsigned int retry_cnt)
{
	unsigned int bit_rate_adaptive = 1, link_rate_retry_cnt = 0;
	unsigned int state = DP_TRAINING_CLOCK_REC;
	int ret;
	int i;

	for (i = 0; i < 4; i++)	{
		dptx->link_cfg.curr_ds[i]    = 0;
		dptx->link_cfg.adj_req_ds[i] = 0;
	}

	//enter training loop
	while (!(state == DP_TRAINING_SUCCESS || state == DP_TRAINING_FAILED)) {
		if (retry_cnt++ > 5) {
			state = DP_TRAINING_FAILED;
			break;
		}
		switch (state) {
		case DP_TRAINING_CLOCK_REC:
		/* *************************************
		 * the initial state preforms clock recovery.
		 * if successful, the training sequence moves on to symbol lock.
		 * if clock recovery fails, the training loop exits.
		 */
			DPTXPR(dptx->idx, LOG_V, "---- CLOCK_REC @ %d ----", retry_cnt);
			ret = dptx_run_clk_recovery_loop(dptx);
			if (ret == 0) {
				state = DP_TRAINING_CHANNEL_EQ;
				break;
			} else if (ret < 0) {
				state = DP_TRAINING_FAILED;
				break;
			}
			DPTXPR(dptx->idx, LOG_I, "%s: CR failed", __func__);
			if (bit_rate_adaptive && ret == 1)
				state = DP_TRAINING_ADJ_SPD_CR_FAIL;
			else
				state = DP_TRAINING_FAILED;
			break;
		case DP_TRAINING_CHANNEL_EQ:
		/* *************************************
		 * once clock recovery is complete, perform symbol lock and channel equalization.
		 * if this state fails, then we can adjust the link rate.
		 */
			DPTXPR(dptx->idx, LOG_V, "---- CHANNEL_EQ @ %d ----", retry_cnt);
			ret = dptx_run_channel_equalization_loop(dptx);
			if (ret == 0) {
				state = DP_TRAINING_SUCCESS;
				break;
			}
			if (ret < 0) {
				state = DP_TRAINING_FAILED;
				break;
			}
			DPTXPR(dptx->idx, LOG_I, "%s: channel EQ failed", __func__);
			if (bit_rate_adaptive && ret == 1)
				state = DP_TRAINING_ADJ_SPD_CR_FAIL_IN_EQ;
			else if (bit_rate_adaptive && ret == 2)
				state = DP_TRAINING_ADJ_SPD_EQ_FAIL_OVERTIME;
			else
				state = DP_TRAINING_FAILED;
			break;
		case DP_TRAINING_ADJ_SPD_CR_FAIL:
		case DP_TRAINING_ADJ_SPD_CR_FAIL_IN_EQ:
		case DP_TRAINING_ADJ_SPD_EQ_FAIL_OVERTIME:
		/* *************************************
		 * when allowed by the function parameter, adjust the link speed and
		 *   attempt to retrain the link starting with clock recovery.
		 * the state of the state variable should not be changed in this state
		 *   allowing a failure condition to report the proper state.
		 */
			DPTXPR(dptx->idx, LOG_V, "---- ADJ_SPD @ %d ----", retry_cnt);
			ret = dptx_link_rate_config_reduce(dptx);
			dptx_link_config_update(dptx);
			if (ret)
				link_rate_retry_cnt++;
			//dptx_phy_config_reinit(dptx);
			if (link_rate_retry_cnt == DP_TRAINING_LINKRATE_ATTEMPTS) {
				state = DP_TRAINING_FAILED;
				break;
			}
			state = DP_TRAINING_CLOCK_REC;
		}
	}

	//training pattern off
	dptx_set_pattern(dptx, DPTX_TPS_DISABLE);

	if (dptx_print_level >= LOG_I)
		dptx_training_status_print(dptx);
	if (state == DP_TRAINING_SUCCESS)
		return 0;

	return -1;
}

int __dptx_full_link_training(struct dptx_drv_s *dptx)
{
	unsigned int training_done = 0, retry_cnt = 0;
	int ret;
	unsigned char aux_data[4];

	DPTXPR(dptx->idx, LOG_I, "%s", __func__);

	while (training_done == 0) {
		if (retry_cnt > DP_FULL_LINK_TRAINING_ATTEMPTS)
			break;
		ret = dptx_run_training_loop(dptx, retry_cnt);
		if (ret == 0)
			training_done = 1;
		retry_cnt++;
	}

	ret = __dptx_aux_read(dptx, DPCD_TRAINING_SCORE_LANE0, 4, aux_data);
	DPTXPR(dptx->idx, LOG_I, "%s: result: [0x%02x, 0x%02x, 0x%02x, 0x%02x]", __func__,
		aux_data[0], aux_data[1], aux_data[2], aux_data[3]);

	if (training_done) {
		DPTXPR(dptx->idx, LOG_I, "%s: ok", __func__);
		return 0;
	}

	DPTXPR(dptx->idx, LOG_E, "%s: failed", __func__);
	return -1;
}

/* *************** fast link training ****************** */
int __dptx_fast_link_training(struct dptx_drv_s *dptx)
{
	int ret;

	// set training pattern for CR
	dptx_set_pattern(dptx, DPTX_TPS1);
	mdelay(1);

	// set training pattern for EQ
	if (dptx->link_cfg.TPS_support & BIT(1))
		dptx_set_pattern(dptx, DPTX_TPS4);
	else if (dptx->link_cfg.TPS_support & BIT(0))
		dptx_set_pattern(dptx, DPTX_TPS3);
	else
		dptx_set_pattern(dptx, DPTX_TPS2);
	mdelay(1);

	// disable the training pattern
	dptx_set_pattern(dptx, DPTX_TPS_DISABLE);

	ret = dptx_check_channel_equalization(dptx);

	return ret;
}

/* ******************* link training ********************* */
int __dptx_link_training(struct dptx_drv_s *dptx)
{
	int ret = -1;

	dptx_clk_set_link_clk(dptx, dptx->link_cfg.link_rate);

	switch (dptx->link_cfg.training_mode) {
	case DPTX_FAST_LINK_TRAINING:
		ret = __dptx_fast_link_training(dptx);
		break;
	case DPTX_FULL_LINK_TRAINING:
		ret = __dptx_full_link_training(dptx);
		break;
	case DPTX_LINK_TRAINING_AUTO:
	default:
		ret = __dptx_fast_link_training(dptx);
		if (ret)
			ret = __dptx_full_link_training(dptx);
		break;
	}

	return ret;
}
