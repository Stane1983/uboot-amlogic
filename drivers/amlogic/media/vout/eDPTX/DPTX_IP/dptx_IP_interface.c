// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/media/vout/eDPTX/DPTX.h>
#include "dptx_IP_ctrls.h"
#include "../dptx_common.h"

uint8_t dptx_if_aux_write(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_write))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_write
			(dptx, port, addr, len, buf);
	return 0;
}

uint8_t dptx_if_aux_write_single(struct dptx_drv_s *dptx, uint8_t port, uint32_t addr, uint8_t val)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_write_single))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_write_single
			(dptx, port, addr, val);
	return 0;
}

uint8_t dptx_if_aux_read(struct dptx_drv_s *dptx, uint8_t port,
			uint32_t addr, int len, uint8_t *buf)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_read))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_read
			(dptx, port, addr, len, buf);
	return 0;
}

uint8_t dptx_if_aux_i2c_op(struct dptx_drv_s *dptx, uint8_t port,
				uint8_t cmd_type, uint32_t dev_addr, uint8_t len, uint8_t *data)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_i2c_op))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->aux_i2c_op
			(dptx, port, cmd_type, dev_addr, len, data);
	return 0;
}

void dptx_if_transmit_pattern(struct dptx_drv_s *dptx, uint8_t port, uint8_t pattern, uint8_t lane)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmit_pattern))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmit_pattern
			(dptx, port, pattern, lane);
}

void dptx_if_set_MSA(struct dptx_drv_s *dptx, uint8_t port)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->set_MSA))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->set_MSA(dptx, port);
}

void dptx_if_path_reset(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->path_reset))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->path_reset(dptx, port, mask);
}

void dptx_if_set_lane_cfg(struct dptx_drv_s *dptx, uint8_t port)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->lane_cfg_to_IP))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->lane_cfg_to_IP(dptx, port);
}

void dptx_if_set_phy_cfg(struct dptx_drv_s *dptx, uint8_t port, uint8_t lane_mask)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->phy_cfg_to_IP))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->phy_cfg_to_IP(dptx, port, lane_mask);
}

void dptx_if_transmitter_init(struct dptx_drv_s *dptx, uint8_t port)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmitter_init))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmitter_init(dptx, port);
}

void dptx_if_transmitter_output(struct dptx_drv_s *dptx, uint8_t port, uint8_t en)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmitter_output))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->transmitter_output(dptx, port, en);
}

uint8_t dptx_if_get_hpd_level(struct dptx_drv_s *dptx, uint8_t port)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->get_hpd_level))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->get_hpd_level(dptx, port);
	return 0;
}

uint16_t dptx_if_get_hpd_irq(struct dptx_drv_s *dptx, uint8_t port)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->get_hpd_irq))
		return ((struct dptx_if_ctrl_s *)dptx->if_ctrls)->get_hpd_irq(dptx, port);
	return 0;
}

void dptx_if_set_hpd_interrupt_mask(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->set_hpd_interrupt_mask))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->set_hpd_interrupt_mask(dptx, port, mask);
}

void dptx_if_scramble_reset_set(struct dptx_drv_s *dptx, uint8_t port, uint8_t sr_type)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->scramble_reset_set))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->scramble_reset_set(dptx, port, sr_type);
}

void dptx_if_PSR1_ctrl(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->PSR1_SDP_ctrl))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->PSR1_SDP_ctrl(dptx, port, flag);
}

void dptx_if_PSR2_ctrl(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag)
{
	if (likely(((struct dptx_if_ctrl_s *)dptx->if_ctrls)->PSR2_SDP_en))
		((struct dptx_if_ctrl_s *)dptx->if_ctrls)->PSR2_SDP_en(dptx, port, flag);
}

void dptx_if_IP_probe(struct dptx_drv_s *dptx)
{
	switch (dptx->data->chip_type) {
	case DPTX_CHIP_T7:
		dptx->if_ctrls = dptx_if_bind_t7(dptx);
		break;
	default:
		dptx->if_ctrls = NULL;
		DPTX_ERR(dptx, "%s: invalid chip type", __func__);
		return;
	}
}
