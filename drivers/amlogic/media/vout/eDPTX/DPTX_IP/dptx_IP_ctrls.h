/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DPTX_IP_CTRL_H_
#define _DPTX_IP_CTRL_H_

#include <amlogic/media/vout/eDPTX/DPTX.h>
#include "../dptx_common.h"

//Source: Table 2-58: uPacket TX AUX CH State and Event Descriptions
//#define DPTX_AUX_REPLY_WAIT_TIMEOUT 5   //times
#define DPTX_AUX_REPLY_WAIT_TIMEOUT 120   //times
#define DPTX_AUX_REPLY_WAIT_TIMER   5   //us (protocol: 400us, check for each 50us, check 10 times)
#define DPTX_AUX_NO_REPLY_TIMEOUT   1   //ms
#define DPTX_AUX_NO_REPLY_RETRY     10   //times
//#define DPTX_AUX_NO_REPLY_RETRY     5   //times
#define AUX_STATUS_REPLY_ERROR         BIT(3)
#define AUX_STATUS_REQUEST_IN_PROGRESS BIT(2)
#define AUX_STATUS_REPLY_IN_PROGRESS   BIT(1)
#define AUX_STATUS_REPLY_RECEIVED      BIT(0)

#define AUX_REPLY_CODE_ACK       0x0
#define AUX_REPLY_CODE_AUX_NACK  0x1
#define AUX_REPLY_CODE_AUX_Defer 0x2
#define AUX_REPLY_CODE_I2C_NACK  0x4
#define AUX_REPLY_CODE_I2C_Defer 0x8

struct dptx_if_ctrl_s {
	uint8_t (*aux_write)(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf);
	uint8_t (*aux_write_single)(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, uint8_t val);
	uint8_t (*aux_read)(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf);
	uint8_t (*aux_i2c_op)(struct dptx_drv_s *dptx, uint8_t port,
				uint8_t cmd_type, uint32_t dev_addr, uint8_t len, uint8_t *data);

	void (*transmit_pattern)(struct dptx_drv_s *dptx, uint8_t port,
				uint8_t pattern, uint8_t lane_mask);
	void (*set_MSA)(struct dptx_drv_s *dptx, uint8_t port);

	void (*path_reset)(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask);

	void (*lane_cfg_to_IP)(struct dptx_drv_s *dptx, uint8_t port);
	void (*phy_cfg_to_IP)(struct dptx_drv_s *dptx, uint8_t port, uint8_t lane_mask);

	void (*transmitter_init)(struct dptx_drv_s *dptx, uint8_t port);
	// use port_mask to reduce port enabling interval
	void (*transmitter_output)(struct dptx_drv_s *dptx, uint8_t port, uint8_t en);

	uint8_t (*get_hpd_level)(struct dptx_drv_s *dptx, uint8_t port);
	uint16_t (*get_hpd_irq)(struct dptx_drv_s *dptx, uint8_t port);
	void (*set_hpd_interrupt_mask)(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask);

	void (*scramble_reset_set)(struct dptx_drv_s *dptx, uint8_t port, uint8_t sr_type);

	void (*PSR1_SDP_ctrl)(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);
	void (*PSR2_SDP_en)(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);
};

struct dptx_if_ctrl_s *dptx_if_bind_t7(struct dptx_drv_s *dptx);
struct dptx_if_ctrl_s *dptx_if_bind_DP14(struct dptx_drv_s *dptx);

#endif
