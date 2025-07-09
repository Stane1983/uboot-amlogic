/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DPTX_VENC_H__
#define __DPTX_VENC_H__
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <linux/types.h>

#define DPTX_DUAL_PORT_L_R        0
#define DPTX_DUAL_PORT_R_L        1
#define DPTX_DUAL_PORT_O_E        2
#define DPTX_DUAL_PORT_E_O        3

#define DPTX_WAIT_VSYNC_TIMEOUT    50000

struct dptx_venc_op_s {
	void (*wait_vsync)(struct dptx_drv_s *dptx);
	u32  (*get_max_lcnt)(struct dptx_drv_s *dptx);
	int  (*venc_debug_test)(struct dptx_drv_s *dptx, u8 num);
	void (*venc_set_timing)(struct dptx_drv_s *dptx);
	void (*venc_set)(struct dptx_drv_s *dptx);
	void (*venc_switch)(struct dptx_drv_s *dptx, u8 flag);
	void (*mute_set)(struct dptx_drv_s *dptx, u8 flag);
	u32  (*get_encl_line_cnt)(struct dptx_drv_s *dptx);
};

void dptx_venc_op_init_t7(struct dptx_venc_op_s *venc_op);

#endif
