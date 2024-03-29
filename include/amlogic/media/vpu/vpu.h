/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _VPU_INC_H_
#define _VPU_INC_H_

#define VPU_READ0   0
#define VPU_READ2   2
#define VPU_WRITE0  3
#define NO_PORT     5

enum vpu_arb_mod_e {
	VPU_ARB_OSD1 = 1,
	VPU_ARB_OSD2,
	VPU_ARB_VD1,
	VPU_ARB_VD1_RDMIF,
	VPU_ARB_VD1_AFBCD,
	VPU_ARB_VD2,
	VPU_ARB_OSD3,
	VPU_ARB_OSD4,
	VPU_ARB_AMDOLBY0,
	VPU_ARB_MALI_AFBCD,
	VPU_ARB_VD3,
	VPU_ARB_VPP_ARB0,
	VPU_ARB_VPP_ARB1,
	VPU_ARB_RDMA_READ,
	VPU_ARB_VIU2,
	VPU_ARB_TCON_P1,
	VPU_ARB_TVFE_READ,
	VPU_ARB_TCON_P2,
	VPU_ARB_LDIM_RD,
	VPU_ARB_VDIN_AFBCE_RD,
	VPU_ARB_VPU_DMA,

	VPU_ARB_VDIN_WR,
	VPU_ARB_RDMA_WR,
	VPU_ARB_TVFE_WR,
	VPU_ARB_TCON1_WR,
	VPU_ARB_DI_AXI1_WR,
	VPU_ARB_TCON2_WR,
	VPU_ARB_TCON3_WR,
	VPU_ARB_VPU_DMA_WR,
	ARB_MODULE_MAX,
};

extern void vcbus_test(void);
extern int vpu_probe(void);
extern int vpu_remove(void);
extern int vpu_clk_change(int level);
extern void vpu_clk_get(void);
extern void vpu_info_print(void);
void vpu_sec_debug(unsigned int flag);
void dump_vpu_rdarb_table(void);
void dump_vpu_urgent_table(void);
int vpu_rdarb_bind_l1(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module);
int vpu_rdarb_bind_l2(enum vpu_arb_mod_e level2_module, u32 vpu_read_port);
int vpu_urgent_set(u32 vmod, u32 urgent_value);
void print_bind1_change_info(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module);
void print_bind2_change_info(enum vpu_arb_mod_e level1_module, enum vpu_arb_mod_e level2_module);
void get_arb_module_info(void);
void print_urgent_change_info(u32 vmod, u32 urgent_value);
#endif
