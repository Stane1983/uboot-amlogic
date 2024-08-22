/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DPTX_IP_OPS_H_
#define _DPTX_IP_OPS_H_

#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include "../dptx_regs.h"
#include "../dptx_common.h"

//Source: Table 2-58: uPacket TX AUX CH State and Event Descriptions
#define DPTX_AUX_REPLY_WAIT_TIMEOUT 50   //times
#define DPTX_AUX_REPLY_WAIT_TIMER   10 //us (protocol: 400us, check for each 10us, check 50 times)
#define DPTX_AUX_NO_REPLY_TIMEOUT   1   //ms
#define DPTX_AUX_NO_REPLY_RETRY     5   //times
// #define DPTX_AUX_NO_REPLY_RETRY     5   //times
//AUX operation

#define AUX_STATUS_REPLY_ERROR         BIT(3)
#define AUX_STATUS_REQUEST_IN_PROGRESS BIT(2)
#define AUX_STATUS_REPLY_IN_PROGRESS   BIT(1)
#define AUX_STATUS_REPLY_RECEIVED      BIT(0)

#define AUX_REPLY_CODE_ACK       0x0
#define AUX_REPLY_CODE_AUX_NACK  0x1
#define AUX_REPLY_CODE_AUX_Defer 0x2
#define AUX_REPLY_CODE_I2C_NACK  0x4
#define AUX_REPLY_CODE_I2C_Defer 0x8

uint8_t __dptx_aux_write(struct dptx_drv_s *dptx, uint32_t addr, int len, uint8_t *buf);
uint8_t __dptx_aux_write_single(struct dptx_drv_s *dptx, uint32_t addr, uint8_t val);
uint8_t __dptx_aux_read(struct dptx_drv_s *dptx, uint32_t addr, int len, uint8_t *buf);
uint8_t dptx_aux_i2c_op(struct dptx_drv_s *dptx,
			uint8_t cmd_type, uint32_t dev_addr, uint8_t len, uint8_t *data);

void dptx_transmit_pattern(struct dptx_drv_s *dptx, uint8_t pattern, uint8_t lane);

void dptx_set_MSA(struct dptx_drv_s *dptx);

#define DPTX_RESET_COMBO_DPHY   0
#define DPTX_RESET_eDP_PIPE     1
#define DPTX_RESET_eDP_CTRL     2
void __dptx_reset_part(struct dptx_drv_s *dptx, u8 part);
void dptx_reset_all(struct dptx_drv_s *dptx);

void dptx_set_lane_config_to_IP(struct dptx_drv_s *dptx);
void dptx_set_phy_config_to_IP(struct dptx_drv_s *dptx, uint8_t use_preset);

void dptx_transmitter_init(struct dptx_drv_s *dptx);

void dptx_transmitter_shutdown(struct dptx_drv_s *dptx);
void dptx_main_stream_enable(struct dptx_drv_s *dptx);

unsigned short dptx_get_hpd_level(struct dptx_drv_s *dptx);

#define DPTX_SCRAMBLE_RESET_OFF              0
#define DPTX_SCRAMBLE_RESET_ON               1
#define DPTX_eDP_ALTERNATIVE_SCRAMBLE_RESRT  2
void dptx_set_scramble_reset(struct dptx_drv_s *dptx, uint8_t sr_type);

#endif
