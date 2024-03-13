/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef DSI_CTRL_H
#define DSI_CTRL_H

#include "../dsi_common.h"

struct dsi_cmd_req_s {
	u32 data_type;
	u32 vc_id;
	u8 *payload;
	u32 pld_count;
	u32 req_ack;
	u8 rd_data[DSI_RD_MAX];
	u8 rd_out_len;
};

struct dsi_ctrl_s {
	/* DSI host&phy setup, if (init_on_table) enter op_mode_init*/
	void (*tx_ready)(struct aml_lcd_drv_s *pdrv);
	/* enter op_mode_disp, turn on encl output */
	void (*disp_on)(struct aml_lcd_drv_s *pdrv);

	/* turn off encl output, if (init_off_table) enter op_mode_init */
	void (*disp_off)(struct aml_lcd_drv_s *pdrv);
	/* DSI host&phy reset/powerdown */
	void (*tx_close)(struct aml_lcd_drv_s *pdrv);

	void (*fr_change_pre)(struct aml_lcd_drv_s *pdrv, u16 fr100);
	void (*fr_change_post)(struct aml_lcd_drv_s *pdrv);

	void (*host_config_print)(struct lcd_config_s *pconf);

	/* Processor to Peripheral Direction (Processor-Sourced) Packet Data Types */
	int (*DT_generic_short_write)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_generic_read)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_DCS_short_write)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_DCS_read)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_set_max_return_pkt_size)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_generic_long_write)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	int (*DT_DCS_long_write)(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
	void (*DT_sink_shut_down)(struct aml_lcd_drv_s *pdrv);
	void (*DT_sink_turn_on)(struct aml_lcd_drv_s *pdrv);

	/*debug purpose*/
	void (*op_mode_switch)(struct aml_lcd_drv_s *pdrv, u8 op_mode);
	void (*dphy_reset)(struct aml_lcd_drv_s *pdrv);
	void (*host_reset)(struct aml_lcd_drv_s *pdrv);
};

void dsi_tx_ready(struct aml_lcd_drv_s *pdrv);
void dsi_disp_on(struct aml_lcd_drv_s *pdrv);
void dsi_disp_off(struct aml_lcd_drv_s *pdrv);
void dsi_tx_close(struct aml_lcd_drv_s *pdrv);

void dsi_fr_change_pre(struct aml_lcd_drv_s *pdrv, u16 fr100);
void dsi_fr_change_post(struct aml_lcd_drv_s *pdrv);

void dsi_host_config_print(struct lcd_config_s *pconf);

int dsi_DT_generic_short_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_generic_read(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_DCS_short_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_DCS_read(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_set_max_return_pkt_size(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_generic_long_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
int dsi_DT_DCS_long_write(struct aml_lcd_drv_s *pdrv, struct dsi_cmd_req_s *req);
void dsi_DT_sink_shut_down(struct aml_lcd_drv_s *pdrv);
void dsi_DT_sink_turn_on(struct aml_lcd_drv_s *pdrv);

void dsi_op_mode_switch(struct aml_lcd_drv_s *pdrv, u8 op_mode);
void dsi_dphy_reset(struct aml_lcd_drv_s *pdrv);
void dsi_host_reset(struct aml_lcd_drv_s *pdrv);

struct dsi_ctrl_s *dsi_bind_v1(struct aml_lcd_drv_s *pdrv);
struct dsi_ctrl_s *dsi_bind_v2(struct aml_lcd_drv_s *pdrv);
void lcd_dsi_if_bind(struct aml_lcd_drv_s *pdrv);

#endif
