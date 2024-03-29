// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/cpu_id.h>
#include <fdtdec.h>
#include <amlogic/media/vpu/vpu.h>
#include "vpu_reg.h"
#include "vpu.h"

struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level1_tables;
struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level2_tables;
struct vpu_arb_table_s *vpu_wrarb_vpu0_tables;
struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu0_2_level2_tables;
struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu0_tables;

#ifndef AML_C3_DISPLAY
static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level1_t7[] = {
		/* vpu module,        reg,             bit, len, bind_port,        name */
	{VPU_ARB_OSD1,        VPP_RDARB_MODE,  20,  1,   VPU_ARB_VPP_ARB0,  "osd1",
		/*slv_reg,             bit,        len*/
		VPP_RDARB_REQEN_SLV,   0,          2},
	{VPU_ARB_OSD2,        VPP_RDARB_MODE,  21,  1,   VPU_ARB_VPP_ARB1,  "osd2",
		VPP_RDARB_REQEN_SLV,   2,          2},
	{VPU_ARB_VD1,         VPP_RDARB_MODE,  22,  1,   VPU_ARB_VPP_ARB0,  "vd1",
		VPP_RDARB_REQEN_SLV,   4,          2},
	{VPU_ARB_VD2,         VPP_RDARB_MODE,  23,  1,   VPU_ARB_VPP_ARB1,  "vd2",
		VPP_RDARB_REQEN_SLV,   6,          2},
	{VPU_ARB_OSD3,        VPP_RDARB_MODE,  24,  1,   VPU_ARB_VPP_ARB0,  "osd3",
		VPP_RDARB_REQEN_SLV,   8,          2},
	{VPU_ARB_OSD4,        VPP_RDARB_MODE,  25,  1,   VPU_ARB_VPP_ARB1,  "osd4",
		VPP_RDARB_REQEN_SLV,   10,          2},
	{VPU_ARB_AMDOLBY0,    VPP_RDARB_MODE,  26,  1,   VPU_ARB_VPP_ARB0,  "amdolby0",
		VPP_RDARB_REQEN_SLV,   12,          2},
	{VPU_ARB_MALI_AFBCD,  VPP_RDARB_MODE,  27,  1,   VPU_ARB_VPP_ARB1,  "mali_afbc",
		VPP_RDARB_REQEN_SLV,   14,          2},
	{VPU_ARB_VD3,         VPP_RDARB_MODE,  28,  1,   VPU_ARB_VPP_ARB0,  "vd3",
		VPP_RDARB_REQEN_SLV,   16,          2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level1_txhd2[] = {
	/* vpu module,        reg,             bit, len, bind_port,        name */
	{VPU_ARB_OSD1,        VPP_RDARB_MODE,  20,  1,   VPU_ARB_VPP_ARB0,  "osd1",
		/*slv_reg,             bit,        len*/
		VPP_RDARB_REQEN_SLV,   0,          2},
	{VPU_ARB_OSD2,        VPP_RDARB_MODE,  21,  1,   VPU_ARB_VPP_ARB1,  "osd2",
		VPP_RDARB_REQEN_SLV,   2,          2},
	{VPU_ARB_VD1_RDMIF,   VPP_RDARB_MODE,  22,  1,   VPU_ARB_VPP_ARB0,  "vd1_rdmif",
		VPP_RDARB_REQEN_SLV,   4,          2},
	{VPU_ARB_VD1_AFBCD,   VPP_RDARB_MODE,  23,  1,   VPU_ARB_VPP_ARB1,  "vd1_afbcd",
		VPP_RDARB_REQEN_SLV,   6,          2},
	{VPU_ARB_OSD3,        VPP_RDARB_MODE,  24,  1,   VPU_ARB_VPP_ARB0,  "osd3",
		VPP_RDARB_REQEN_SLV,   8,          2},
	{VPU_ARB_AMDOLBY0,    VPP_RDARB_MODE,  26,  1,   VPU_ARB_VPP_ARB0,  "amdolby0",
		VPP_RDARB_REQEN_SLV,   12,          2},
	{VPU_ARB_MALI_AFBCD,  VPP_RDARB_MODE,  27,  1,   VPU_ARB_VPP_ARB1,  "mali_afbc",
		VPP_RDARB_REQEN_SLV,   14,          2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level2_t7[] = {
	/* vpu module,			reg,			bit, len,   bind_port, name */
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_MODE_L2C1, 16, 1, VPU_READ0, "vpp_arb0",
		/*slv_reg,                 bit,       len*/
		VPU_RDARB_REQEN_SLV_L2C1,	0,			2},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_MODE_L2C1, 17, 1, VPU_READ0, "vpp_arb1",
		VPU_RDARB_REQEN_SLV_L2C1,	2,			2},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_MODE_L2C1, 18, 1, VPU_READ0, "rdma_read",
		VPU_RDARB_REQEN_SLV_L2C1,	4,			2},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_MODE_L2C1, 23, 1, VPU_READ2, "ldim_rd",
		VPU_RDARB_REQEN_SLV_L2C1,	14,			2},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_MODE_L2C1, 24, 1, VPU_READ0, "vdin_afbce_rd",
		VPU_RDARB_REQEN_SLV_L2C1,	16,			2},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_MODE_L2C1, 25, 1, VPU_READ0, "vpu_dma",
		VPU_RDARB_REQEN_SLV_L2C1,	18,			2},
		{}
};

static struct vpu_arb_table_s vpu_rdarb_vpu0_2_level2_txhd2[] = {
	/* vpu module,          reg,                  bit, len, bind_port,  name */
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_MODE_L2C1,  16,  1,   VPU_READ0,  "vpp_arb0",
		/*slv_reg,                 bit,        len*/
		VPU_RDARB_REQEN_SLV_L2C1,   0,          2},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_MODE_L2C1,  17,  1,   VPU_READ0,  "vpp_arb1",
		VPU_RDARB_REQEN_SLV_L2C1,   2,          2},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_MODE_L2C1,  18,  1,   VPU_READ0,  "rdma_read",
		VPU_RDARB_REQEN_SLV_L2C1,   4,          2},
	{VPU_ARB_TCON_P1,       VPU_RDARB_MODE_L2C1,  20,  1,   VPU_READ0,  "tcon_p1",
		VPU_RDARB_REQEN_SLV_L2C1,   8,          2},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_MODE_L2C1,  21,  1,   VPU_READ0,  "tvfe",
		VPU_RDARB_REQEN_SLV_L2C1,   10,          2},
	{VPU_ARB_TCON_P2,       VPU_RDARB_MODE_L2C1,  22,  1,   VPU_READ0,  "tcon_p2",
		VPU_RDARB_REQEN_SLV_L2C1,   12,          2},
		{}
};

static struct vpu_arb_table_s vpu_wrarb_vpu0_t7[] = {
	/* vpu module,          reg,                 bit, len, bind_port, name */
	{VPU_ARB_VDIN_WR,       VPU_WRARB_MODE_L2C1,  16, 1,  NO_PORT,   "vdin_wr",
		/*slv_reg,                 bit,        len*/
		VPU_WRARB_REQEN_SLV_L2C1,   0,          1},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_MODE_L2C1,  17, 1, VPU_WRITE0, "rdma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   1,          1},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_MODE_L2C1,  18, 1,  NO_PORT,   "tvfe_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   2,          1},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_MODE_L2C1,  19, 1,  NO_PORT,   "tcon1_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   3,          1},
	{VPU_ARB_TCON2_WR,      VPU_WRARB_MODE_L2C1,  20, 1,  NO_PORT,   "tcon2_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   4,          1},
	{VPU_ARB_TCON3_WR,      VPU_WRARB_MODE_L2C1,  21, 1,  NO_PORT,   "tcon3_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   5,          1},
	{VPU_ARB_VPU_DMA_WR,    VPU_WRARB_MODE_L2C1,  22, 1,  NO_PORT,   "vpu_dma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   6,          1},
		{}
};

static struct vpu_arb_table_s vpu_wrarb_vpu0_txhd2[] = {
	/* vpu module,          reg,                    bit, len, bind_port,  name */
	{VPU_ARB_VDIN_WR,       VPU_WRARB_MODE_L2C1,    16,  1,   NO_PORT,    "vdin_wr",
		/*slv_reg,                 bit,        len*/
		VPU_WRARB_REQEN_SLV_L2C1,   0,          1},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_MODE_L2C1,    17,  1,   VPU_WRITE0, "rdma_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   1,          1},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_MODE_L2C1,    18,  1,   NO_PORT,    "tvfe_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   2,          1},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_MODE_L2C1,    19,  1,   NO_PORT,    "tcon1_wr",
		VPU_WRARB_REQEN_SLV_L2C1,   3,          1},
	/*
	 *{VPU_ARB_DI_AXI1_WR,	VPU_WRARB_MODE_L2C1,	20,  1,  NO_PORT,	  "tcon1_wr",
	 *		VPU_WRARB_REQEN_SLV_L2C1,	4,			1},
	 */
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu0_2_level2_t7[] = {
	/*vpu module,           reg,                port,     val, start, len, name*/
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   0,    2,  "vpp_arb0"},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   2,    2,  "vpp_arb1"},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   4,    2,  "rdma_read"},
	{VPU_ARB_VIU2,          VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   6,    2,  "viu2(no use)"},
	{VPU_ARB_TCON_P1,        VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   8,    2,  "tcon1(no use)"},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   10,   2,  "tvfe_read(no use)"},
	{VPU_ARB_TCON_P2,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   12,   2,  "tcon_p2(no use)"},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_UGT_L2C1, VPU_READ2, 3,   14,   2,  "ldim_rd"},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   16,   2,  "vdin_afbce_rd"},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   18,   2,  "vpu_dma"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_rd_vpu0_2_level2_txhd2[] = {
	/*vpu module,           reg,                port,     val, start, len, name*/
	{VPU_ARB_VPP_ARB0,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   0,    2,  "vpp_arb0"},
	{VPU_ARB_VPP_ARB1,      VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   2,    2,  "vpp_arb1"},
	{VPU_ARB_RDMA_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   4,    2,  "rdma_read"},
	{VPU_ARB_VIU2,          VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   6,    2,  "viu2(no use)"},
	{VPU_ARB_TCON_P1,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   8,    2,  "tcon1"},
	{VPU_ARB_TVFE_READ,     VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   10,   2,  "tvfe_read"},
	{VPU_ARB_TCON_P2,       VPU_RDARB_UGT_L2C1, VPU_READ0, 3,   12,   2,  "tcon_p2"},
	{VPU_ARB_LDIM_RD,       VPU_RDARB_UGT_L2C1, VPU_READ2, 3,   14,   2,  "ldim_rd(no use)"},
	{VPU_ARB_VDIN_AFBCE_RD, VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   16,   2,  "vdin_afbce(no use)"},
	{VPU_ARB_VPU_DMA,       VPU_RDARB_UGT_L2C1, VPU_READ0, 0,   18,   2,  "vpu_dma(no use)"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu0_t7[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_VDIN_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   0,    2,    "vdin_wr"},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   2,    2,    "rdma_wr"},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   4,    2,    "tvfe_wr"},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   6,    2,    "tcon1_wr"},
	{VPU_ARB_TCON2_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   8,    2,    "tcon2_wr"},
	{VPU_ARB_TCON3_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   10,   2,    "tcon3_wr"},
	{VPU_ARB_VPU_DMA_WR,    VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   12,   2,    "vpu_dma_wr"},
		{}
};

static struct vpu_urgent_table_s vpu_urgent_table_wr_vpu0_txhd2[] = {
	/*vpu module,           reg,                 port,       val, start, len,  name*/
	{VPU_ARB_VDIN_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   0,    2,    "vdin_wr"},
	{VPU_ARB_RDMA_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   2,    2,    "rdma_wr"},
	{VPU_ARB_TVFE_WR,       VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   4,    2,    "tvfe_wr"},
	{VPU_ARB_TCON1_WR,      VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   6,    2,    "tcon1_wr"},
	{VPU_ARB_DI_AXI1_WR,    VPU_WRARB_UGT_L2C1,  VPU_WRITE0,  0,   8,    2,    "di_axi1_wr"},
		{}
};

#endif

int vpu_rdarb_bind_l1(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module)
{
	struct vpu_arb_table_s *vpu0_2_level1_module =
						vpu_rdarb_vpu0_2_level1_tables;

	if (!vpu0_2_level1_module ||
		level1_module >= VPU_ARB_VPP_ARB0) {
		VPUERR("%s table or module err\n", __func__);
		return -1;
	}

	while (vpu0_2_level1_module->vmod) {
		if (level1_module == vpu0_2_level1_module->vmod)
			break;
		vpu0_2_level1_module++;
	}
	if (!vpu0_2_level1_module) {
		VPUERR("level1_table no such module\n");
		return -1;
	}
	switch (level2_module) {
	case VPU_ARB_VPP_ARB0:
		vpu_vcbus_setb(vpu0_2_level1_module->reg, 0,
				vpu0_2_level1_module->bit,
				vpu0_2_level1_module->len);
		vpu0_2_level1_module->bind_port = VPU_ARB_VPP_ARB0;
		break;
	case VPU_ARB_VPP_ARB1:
		vpu_vcbus_setb(vpu0_2_level1_module->reg, 1,
				vpu0_2_level1_module->bit,
				vpu0_2_level1_module->len);
		vpu0_2_level1_module->bind_port = VPU_ARB_VPP_ARB1;
		break;
	default:
		break;
	}
	if (vpu0_2_level1_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu0_2_level1_module->reqen_slv_reg, 1,
			vpu0_2_level1_module->reqen_slv_bit,
			vpu0_2_level1_module->reqen_slv_len);
	else if (vpu0_2_level1_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu0_2_level1_module->reqen_slv_reg, 3,
			vpu0_2_level1_module->reqen_slv_bit,
			vpu0_2_level1_module->reqen_slv_len);
	return 0;
}

void print_bind1_change_info(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_level1_module =
							vpu_rdarb_vpu0_2_level1_tables;

	while (vpu0_2_level1_module->vmod) {
		if (level1_module == vpu0_2_level1_module->vmod)
			break;
		vpu0_2_level1_module++;
	}
	if (!vpu0_2_level1_module) {
		VPUERR("level1_table no such module\n");
		return;
	}
	switch (level2_module) {
	case VPU_ARB_VPP_ARB0:
		VPUPR("%s bind to vpp_arb0\n", vpu0_2_level1_module->name);
		break;
	case VPU_ARB_VPP_ARB1:
		VPUPR("%s bind to vpp_arb1\n", vpu0_2_level1_module->name);
		break;
	default:
		break;
	}
	val = vpu_vcbus_read(vpu0_2_level1_module->reg);
	VPUPR("VPU_RDARB_MOD[0x%x] = 0x%x\n",
		(u32)(vpu0_2_level1_module->reg - REG_BASE_VCBUS) >> 2, val);
}

int vpu_rdarb_bind_l2(enum vpu_arb_mod_e level2_module, u32 vpu_read_port)
{
	struct vpu_arb_table_s *vpu0_2_level2_module =
						vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_level2_module ||
		level2_module < VPU_ARB_VPP_ARB0) {
		VPUERR("%s table or module err\n", __func__);
		return -1;
	}
	while (vpu0_2_level2_module->vmod) {
		if (level2_module == vpu0_2_level2_module->vmod)
			break;
		vpu0_2_level2_module++;
	}
	if (!vpu0_2_level2_module) {
		VPUERR("level2_table no such module\n");
		return -1;
	}
	switch (vpu_read_port) {
	case VPU_READ0:
		vpu_vcbus_setb(vpu0_2_level2_module->reg, 0,
				vpu0_2_level2_module->bit,
				vpu0_2_level2_module->len);
		vpu0_2_level2_module->bind_port = VPU_READ0;
		break;
	case VPU_READ2:
		vpu_vcbus_setb(vpu0_2_level2_module->reg, 1,
				vpu0_2_level2_module->bit,
				vpu0_2_level2_module->len);
		vpu0_2_level2_module->bind_port = VPU_READ2;
		break;
	default:
		break;
	}
	if (vpu0_2_level2_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu0_2_level2_module->reqen_slv_reg, 1,
			vpu0_2_level2_module->reqen_slv_bit,
			vpu0_2_level2_module->reqen_slv_len);
	else if (vpu0_2_level2_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu0_2_level2_module->reqen_slv_reg, 3,
			vpu0_2_level2_module->reqen_slv_bit,
			vpu0_2_level2_module->reqen_slv_len);
	return 0;
}

void print_bind2_change_info(enum vpu_arb_mod_e level2_module, u32 vpu_read_port)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_level2_module = vpu_rdarb_vpu0_2_level2_tables;

	while (vpu0_2_level2_module->vmod) {
		if (level2_module == vpu0_2_level2_module->vmod)
			break;
		vpu0_2_level2_module++;
	}
	if (!vpu0_2_level2_module) {
		VPUERR("level2_table no such module\n");
		return;
	}
	switch (vpu_read_port) {
	case VPU_READ0:
		VPUPR("%s bind to vpu_read0\n", vpu0_2_level2_module->name);
		break;
	case VPU_READ2:
		VPUPR("%s bind to vpu_read2\n", vpu0_2_level2_module->name);
		break;
	default:
		break;
	}
	val = vpu_vcbus_read(vpu0_2_level2_module->reg);
	VPUPR("VPU_RDARB_MODE_L2C1[0x%x] = 0x%x\n",
		(u32)(vpu0_2_level2_module->reg - REG_BASE_VCBUS) >> 2, val);
}

int vpu_wrarb0_bind(enum vpu_arb_mod_e write_module, u32 vpu_write_port)
{
	struct vpu_arb_table_s *vpu_write0_module = vpu_wrarb_vpu0_tables;

	if (!vpu_write0_module ||
		write_module < VPU_ARB_VDIN_WR) {
		VPUERR("%s table or module err\n", __func__);
		return -1;
	}
	while (vpu_write0_module->vmod) {
		if (write_module == vpu_write0_module->vmod)
			break;
		vpu_write0_module++;
	}
	if (!vpu_write0_module) {
		VPUERR("write0 table no such module\n");
		return -1;
	}
	if (vpu_write0_module->reqen_slv_len == 1)
		vpu_vcbus_setb(vpu_write0_module->reqen_slv_reg, 1,
			vpu_write0_module->reqen_slv_bit,
			vpu_write0_module->reqen_slv_len);
	else if (vpu_write0_module->reqen_slv_len == 2)
		vpu_vcbus_setb(vpu_write0_module->reqen_slv_reg, 3,
			vpu_write0_module->reqen_slv_bit,
			vpu_write0_module->reqen_slv_len);
	vpu_vcbus_setb(vpu_write0_module->reg, 1,
					vpu_write0_module->bit,
					vpu_write0_module->len);
	vpu_write0_module->bind_port = VPU_WRITE0;
	return 0;
}

void get_arb_module_info(void)
{
	struct vpu_arb_table_s *vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_level2_tables ||
		!vpu0_2_level2_tables)
		return;
	VPUPR("===========level1 port_num===========\n");
	while (vpu0_2_level1_tables->vmod) {
		VPUPR("module_name:%-10s module_index:%d (bind_reg:[0x%x],bit%d)\n",
			vpu0_2_level1_tables->name,
			vpu0_2_level1_tables->vmod,
			(u32)(vpu0_2_level1_tables->reg - REG_BASE_VCBUS) >> 2,
			vpu0_2_level1_tables->bit);
		vpu0_2_level1_tables++;
	}
	VPUPR("===========level2 port_num===========\n");
	while (vpu0_2_level2_tables->vmod) {
		VPUPR("module_name:%-15s module_index:%d (bind_reg[0x%x],bit%d)\n",
			vpu0_2_level2_tables->name,
			vpu0_2_level2_tables->vmod,
			(u32)(vpu0_2_level2_tables->reg - REG_BASE_VCBUS) >> 2,
			vpu0_2_level2_tables->bit);
		vpu0_2_level2_tables++;
	}
	VPUPR("===========vpu read/write port_num===========\n");
	VPUPR("port_name:VPU_READ0  port_index:%d\n", VPU_READ0);
	VPUPR("port_name:VPU_READ2  port_index:%d\n", VPU_READ2);
	VPUPR("port_name:VPU_WRITE0 port_index:%d\n", VPU_WRITE0);
}

int vpu_read0_2_urgent_set(u32 vmod, u32 urgent_value)
{
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
					vpu_urgent_table_rd_vpu0_2_level2_tables;

	if (urgent_value > 3) {
		VPUERR("urgent value is error range:0-3\n");
		return -1;
	}
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		if (rd_vpu0_2_level2_urgent_tables->vmod == vmod)
			break;
		rd_vpu0_2_level2_urgent_tables++;
	}
	if (!rd_vpu0_2_level2_urgent_tables) {
		VPUERR("level2_urgent_table no such module\n");
		return -1;
	}
	rd_vpu0_2_level2_urgent_tables->val = urgent_value;
	vpu_vcbus_setb(rd_vpu0_2_level2_urgent_tables->reg, urgent_value,
			rd_vpu0_2_level2_urgent_tables->start_bit,
			rd_vpu0_2_level2_urgent_tables->len);
	return 0;
}

int vpu_write0_urgent_set(u32 vmod, u32 urgent_value)
{
	struct vpu_urgent_table_s *write0_urgent_tables =
					vpu_urgent_table_wr_vpu0_tables;

	while (write0_urgent_tables->vmod) {
		if (write0_urgent_tables->vmod == vmod)
			break;
		write0_urgent_tables++;
	}
	if (!write0_urgent_tables) {
		VPUERR("level2_urgent_table no such module\n");
		return -1;
	}
	write0_urgent_tables->val = urgent_value;
	vpu_vcbus_setb(write0_urgent_tables->reg, urgent_value,
			write0_urgent_tables->start_bit, write0_urgent_tables->len);
	return 0;
}

int vpu_urgent_set(u32 vmod, u32 urgent_value)
{
	int ret = -1;

	if (urgent_value > 3) {
		VPUERR("urgent value is error range:0-3\n");
		return ret;
	}
	if (vmod >= VPU_ARB_VPP_ARB0 &&
		vmod <= VPU_ARB_VPU_DMA)
		ret = vpu_read0_2_urgent_set(vmod, urgent_value);
	else if (vmod >= VPU_ARB_VDIN_WR &&
		vmod <= VPU_ARB_VPU_DMA_WR)
		ret = vpu_write0_urgent_set(vmod, urgent_value);
	else
		VPUERR("module error\n");
	return ret;
}

void print_urgent_change_info(u32 vmod, u32 urgent_value)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
					vpu_urgent_table_rd_vpu0_2_level2_tables;
	struct vpu_urgent_table_s *write0_urgent_tables =
					vpu_urgent_table_wr_vpu0_tables;
	struct vpu_urgent_table_s *set_urgent_module = NULL;

	if (urgent_value > 3)
		return;
	if (vmod >= VPU_ARB_VPP_ARB0 &&
		vmod <= VPU_ARB_VPU_DMA) {
		while (rd_vpu0_2_level2_urgent_tables->vmod) {
			if (rd_vpu0_2_level2_urgent_tables->vmod == vmod)
				set_urgent_module = rd_vpu0_2_level2_urgent_tables;
			rd_vpu0_2_level2_urgent_tables++;
		}
	} else if (vmod >= VPU_ARB_VDIN_WR &&
		vmod <= VPU_ARB_VPU_DMA_WR) {
		while (write0_urgent_tables->vmod) {
			if (write0_urgent_tables->vmod == vmod)
				set_urgent_module = write0_urgent_tables;
			write0_urgent_tables++;
		}
	}
	VPUPR("%s urgent set to %d\n", set_urgent_module->name, urgent_value);
	val = vpu_vcbus_read(set_urgent_module->reg);
	VPUPR("VPU_RDARB_UGT_L2C1[0x%x] = 0x%x\n",
		(u32)(set_urgent_module->reg - REG_BASE_VCBUS) >> 2, val);
}

static void sort_urgent_table(struct vpu_urgent_table_s *urgent_table)
{
	int i, j;
	int array_size = 0;
	struct vpu_urgent_table_s temp;
	struct vpu_urgent_table_s *urgent_module = urgent_table;

	while (urgent_module->vmod) {
		array_size++;
		urgent_module++;
	}
	urgent_module = urgent_table;
	for (i = 0; i < array_size - 1; i++) {
		for (j = 0; j < array_size - i - 1; j++) {
			if (urgent_module[j].val < urgent_module[j + 1].val) {
				temp = urgent_module[j];
				urgent_module[j] = urgent_module[j + 1];
				urgent_module[j + 1] = temp;
			}
		}
	}
	for (i = 0; i < array_size; i++)
		urgent_table[i] = urgent_module[i];
}

void vpu_urgent_get(void)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
						vpu_urgent_table_rd_vpu0_2_level2_tables;

	if (!rd_vpu0_2_level2_urgent_tables)
		VPUERR("vpu_read0_2 urgent table is error\n");
	sort_urgent_table(vpu_urgent_table_rd_vpu0_2_level2_tables);
	VPUPR("===========vpu urgent table===========\n");
	/*urgent value = 3*/
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		VPUPR("module name:%15s urgent_value: %d\n",
			rd_vpu0_2_level2_urgent_tables->name,
			rd_vpu0_2_level2_urgent_tables->val);
		rd_vpu0_2_level2_urgent_tables++;
	}
	rd_vpu0_2_level2_urgent_tables = vpu_urgent_table_rd_vpu0_2_level2_tables;
	val = vpu_vcbus_read(vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg);
	VPUPR("\nVPU_RDARB_UGT_L2C1[0x%x] = 0x%x\n",
		(u32)(vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg - REG_BASE_VCBUS) >> 2, val);
}

void dump_vpu_rdarb_table(void)
{
	u32 val;
	struct vpu_arb_table_s *vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	if (!vpu0_2_level1_tables || !vpu0_2_level2_tables) {
		VPUERR("rdarb table error!\n");
		return;
	}
	VPUPR("===========vpu rdarb table===========\n");
	/*arb->VPU_READ0*/
	while (vpu0_2_level2_tables->vmod) {
		if (vpu0_2_level2_tables->bind_port == VPU_READ0) {
			if (vpu0_2_level2_tables->vmod == VPU_ARB_VPP_ARB0) {
			/*VPU_VPP_ARB0*/
				while (vpu0_2_level1_tables->vmod) {
					if (vpu0_2_level1_tables->bind_port == VPU_ARB_VPP_ARB0) {
						VPUPR("%-10s ---> vpp_arb0 ---> vpu_read0\n",
							vpu0_2_level1_tables->name);
					}
					vpu0_2_level1_tables++;
				}
				vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			} else if (vpu0_2_level2_tables->vmod == VPU_ARB_VPP_ARB1) {
			/*VPU_VPP_ARB1*/
				while (vpu0_2_level1_tables->vmod) {
					if (vpu0_2_level1_tables->bind_port == VPU_ARB_VPP_ARB1) {
						VPUPR("%-10s ---> vpp_arb1 ---> vpu_read0\n",
							vpu0_2_level1_tables->name);
					}
					vpu0_2_level1_tables++;
				}
				vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			}
		}
		vpu0_2_level2_tables++;
	}
	vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;
	/*level2 other module-> VPU_READ0*/
	while (vpu0_2_level2_tables->vmod) {
		if (vpu0_2_level2_tables->bind_port == VPU_READ0) {
			if (vpu0_2_level2_tables->vmod != VPU_ARB_VPP_ARB0 &&
				vpu0_2_level2_tables->vmod != VPU_ARB_VPP_ARB1) {
				/*level2 other module*/
				VPUPR("%24s ---> vpu_read0\n",
					vpu0_2_level2_tables->name);
				}
			}
		vpu0_2_level2_tables++;
	}
	vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;

	/*reserve fo vpu_read1*/
	while (vpu0_2_level2_tables->vmod) {
		if (vpu0_2_level2_tables->bind_port != VPU_READ2 &&
			vpu0_2_level2_tables->bind_port != VPU_READ0) {
			VPUPR("%24s ---> vpu_read1\n",
				vpu0_2_level2_tables->name);
		}
		vpu0_2_level2_tables++;
	}
	vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;
	/*arb->VPU_READ2*/
	while (vpu0_2_level2_tables->vmod) {
		if (vpu0_2_level2_tables->bind_port == VPU_READ2) {
			if (vpu0_2_level2_tables->vmod == VPU_ARB_VPP_ARB0) {
			/*VPU_VPP_ARB0*/
				while (vpu0_2_level1_tables->vmod) {
					if (vpu0_2_level1_tables->bind_port == VPU_ARB_VPP_ARB0) {
						VPUPR("%-10s ---> vpp_arb0 ---> vpu_read2\n",
							vpu0_2_level1_tables->name);
					}
					vpu0_2_level1_tables++;
				}
				vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			} else if (vpu0_2_level2_tables->vmod == VPU_ARB_VPP_ARB1) {
			/*VPU_VPP_ARB1*/
				while (vpu0_2_level1_tables->vmod) {
					if (vpu0_2_level1_tables->bind_port == VPU_ARB_VPP_ARB1) {
						VPUPR("%-10s ---> vpp_arb1 ---> vpu_read2\n",
							vpu0_2_level1_tables->name);
					}
					vpu0_2_level1_tables++;
				}
				vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_tables;
			}
		}
		vpu0_2_level2_tables++;
	}
	vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;
	while (vpu0_2_level2_tables->vmod) {
		if (vpu0_2_level2_tables->bind_port == VPU_READ2) {
			if (vpu0_2_level2_tables->vmod != VPU_ARB_VPP_ARB0 &&
				vpu0_2_level2_tables->vmod != VPU_ARB_VPP_ARB1) {
				/*level2 other module*/
				VPUPR("%24s ---> vpu_read2\n",
					vpu0_2_level2_tables->name);
				}
			}
		vpu0_2_level2_tables++;
	}
	vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_tables;
	val = vpu_vcbus_read(vpu_rdarb_vpu0_2_level1_tables[0].reg);
	VPUPR("VPU_RDARB_MOD[0x%x] = 0x%x\n",
		(u32)(vpu_rdarb_vpu0_2_level1_tables[0].reg - REG_BASE_VCBUS) >> 2, val);
	val = vpu_vcbus_read(vpu_rdarb_vpu0_2_level2_tables[0].reg);
	VPUPR("VPU_RDARB_MODE_L2C1[0x%x] = 0x%x\n",
		(u32)(vpu_rdarb_vpu0_2_level2_tables[0].reg  - REG_BASE_VCBUS) >> 2, val);
}

void dump_vpu_urgent_table(void)
{
	u32 val;
	struct vpu_urgent_table_s *rd_vpu0_2_level2_urgent_tables =
						vpu_urgent_table_rd_vpu0_2_level2_tables;

	if (!rd_vpu0_2_level2_urgent_tables)
		VPUERR("vpu_read0_2 urgent table is error\n");
	sort_urgent_table(vpu_urgent_table_rd_vpu0_2_level2_tables);
	VPUPR("===========vpu urgent table===========\n");
	/*urgent value = 3*/
	while (rd_vpu0_2_level2_urgent_tables->vmod) {
		VPUPR("module name:%18s urgent_value: %d\n",
			rd_vpu0_2_level2_urgent_tables->name,
			rd_vpu0_2_level2_urgent_tables->val);
		rd_vpu0_2_level2_urgent_tables++;
	}
	rd_vpu0_2_level2_urgent_tables = vpu_urgent_table_rd_vpu0_2_level2_tables;
	val = vpu_vcbus_read(vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg);
	VPUPR("\nVPU_RDARB_UGT_L2C1[0x%x] = 0x%x\n",
		(u32)(vpu_urgent_table_rd_vpu0_2_level2_tables[0].reg - REG_BASE_VCBUS) >> 2, val);
}

void init_read0_2_bind(void)
{
	struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level1 =
							vpu_rdarb_vpu0_2_level1_tables;
	struct vpu_arb_table_s *vpu_rdarb_vpu0_2_level2 =
							vpu_rdarb_vpu0_2_level2_tables;

	while (vpu_rdarb_vpu0_2_level1->vmod) {
		vpu_rdarb_bind_l1(vpu_rdarb_vpu0_2_level1->vmod,
			vpu_rdarb_vpu0_2_level1->bind_port);
		vpu_rdarb_vpu0_2_level1++;
	}
	while (vpu_rdarb_vpu0_2_level2->vmod) {
		vpu_rdarb_bind_l2(vpu_rdarb_vpu0_2_level2->vmod,
			vpu_rdarb_vpu0_2_level2->bind_port);
		vpu_rdarb_vpu0_2_level2++;
	}
}

void init_write0_bind(void)
{
	struct vpu_arb_table_s *vpu_write0_arb = vpu_wrarb_vpu0_tables;

	vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x20000);
	while (vpu_write0_arb->vmod) {
		if (vpu_write0_arb->bind_port != NO_PORT)
			vpu_wrarb0_bind(vpu_write0_arb->vmod,
				vpu_write0_arb->bind_port);
		vpu_write0_arb++;
	}
}

void init_read0_2_write0_urgent(void)
{
	struct vpu_urgent_table_s *vpu_urgent_table_rd_vpu0_2 =
		vpu_urgent_table_rd_vpu0_2_level2_tables;
	struct vpu_urgent_table_s *vpu_urgent_table_wr_vpu1 =
		vpu_urgent_table_wr_vpu0_tables;

	while (vpu_urgent_table_rd_vpu0_2->vmod) {
		vpu_urgent_set(vpu_urgent_table_rd_vpu0_2->vmod,
			vpu_urgent_table_rd_vpu0_2->val);
		vpu_urgent_table_rd_vpu0_2++;
	}
	while (vpu_urgent_table_wr_vpu1->vmod) {
		vpu_urgent_set(vpu_urgent_table_wr_vpu1->vmod,
			vpu_urgent_table_wr_vpu1->val);
		vpu_urgent_table_wr_vpu1++;
	}
}

int init_arb_urgent_table(void)
{
#ifndef AML_C3_DISPLAY
	if (vpu_conf.data->chip_type == VPU_CHIP_T7) {
		vpu_rdarb_vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_t7;
		vpu_rdarb_vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_t7;
		vpu_wrarb_vpu0_tables = vpu_wrarb_vpu0_t7;
		vpu_urgent_table_rd_vpu0_2_level2_tables = vpu_urgent_table_rd_vpu0_2_level2_t7;
		vpu_urgent_table_wr_vpu0_tables = vpu_urgent_table_wr_vpu0_t7;
	} else if (vpu_conf.data->vpu_read_type == ONLY_READ0) {
		vpu_rdarb_vpu0_2_level1_tables = vpu_rdarb_vpu0_2_level1_txhd2;
		vpu_rdarb_vpu0_2_level2_tables = vpu_rdarb_vpu0_2_level2_txhd2;
		vpu_wrarb_vpu0_tables = vpu_wrarb_vpu0_txhd2;
		vpu_urgent_table_rd_vpu0_2_level2_tables = vpu_urgent_table_rd_vpu0_2_level2_txhd2;
		vpu_urgent_table_wr_vpu0_tables = vpu_urgent_table_wr_vpu0_txhd2;
	}
#endif
	init_read0_2_bind();
	init_write0_bind();
	init_read0_2_write0_urgent();

	return 0;
}
