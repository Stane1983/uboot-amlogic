/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LDIM_ABCON_H__
#define __LDIM_ABCON_H__

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/amlogic/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/aml_bl.h>
#include <amlogic/media/vout/lcd/bl_ldim.h>
#include "../../../lcd_common.h"
#include "../../../lcd_reg.h"
#include "../../lcd_bl.h"
#include "../ldim_drv.h"
#include "../ldim_dev_drv.h"
#include "env.h"

#define ABCON_CHIP_DEF 0
#define ABCON_CHIP_T6W 1
#define ABCON_CHIP_T6X 2

#define ABCON_CORE_NUM 4

/*#define LDIM_DEBUG_INFO*/
#define ABCONPR(fmt, args...)     printf("ldim: abcon: " fmt "", ## args)
#define ABCONERR(fmt, args...)    printf("ldim: abcon: error: " fmt "", ## args)
#define ABCONWARN(fmt, args...)    printf("ldim: abcon: warning: " fmt "", ## args)

/* abcon memory map*/
//#define ABCON_MEM_TOTAL 0xa000
#define CH_MAPPING_SIZE_T6W 0x800   //1024 * 2bytes
#define SW_DUTY_SIZE_T6W 0x8000 //0X2000 * 4, as ldc hw duty
#define WSEG_SIZE_T6W 0x400
#define RSEG_SIZE_T6W 0x400
#define LDC_SEG_SIZE_T6W 0x400  // for reg_abcon_ldc_seg_baddr

#define CH_MAPPING_OFFSET_T6W 0
#define LDC_SEG_OFFSET_T6W 0x800    //CH_MAPPING_OFFSET + CH_MAPPING_SIZE
#define SW_DUTY_OFFSET_T6W 0xc00    //LDC_SEG_OFFSET  + LDC_SEG_SIZE
#define WSEG_OFFSET_T6W 0x8c00      //SW_DUTY_OFFSET + SW_DUTY_SIZE
#define RSEG_OFFSET_T6W 0x9000      //WSEG_OFFSET + WSEG_SIZE
#define ABCON_OFFSET_END_T6W 0x9400 //RSEG_OFFSET + RSEG_SIZE

#define LDC_DUTY_SIZE_T6W 0x2000 //t6w 8k

//for t6x
#define CH_MAPPING_SIZE_T6X 0x1800   //4096 * 12 / 8 bytes
#define SW_DUTY_SIZE_T6X 0x10000 //0X4000 * 4, as ldc hw duty
#define WSEG_SIZE_T6X 0x800
#define RSEG_SIZE_T6X 0x800
#define LDC_SEG_SIZE_T6X 0x200	// for reg_abcon_ldc_seg_baddr

#define CH_MAPPING_OFFSET_T6X 0
#define LDC_SEG_OFFSET_T6X 0x1800	//CH_MAPPING_OFFSET + CH_MAPPING_SIZE
#define SW_DUTY_OFFSET_T6X 0x1A00    //LDC_SEG_OFFSET  + LDC_SEG_SIZE
#define WSEG_OFFSET_T6X 0x11A00   //SW_DUTY_OFFSET + SW_DUTY_SIZE
#define RSEG_OFFSET_T6X 0x12200  //WSEG_OFFSET + WSEG_SIZE
#define ABCON_OFFSET_END_T6X 0x12A00	//RSEG_OFFSET + RSEG_SIZE

#define LDC_DUTY_SIZE_T6X 0x4000 //t6x 16k

struct abcon_mem_s {
	unsigned int *base_vaddr;
	phys_addr_t base_paddr;
	unsigned int *wseg;
	phys_addr_t wseg_paddr;
	unsigned int *rseg;
	phys_addr_t rseg_paddr;
	unsigned int *swduty;
	phys_addr_t swduty_paddr;
	unsigned int *ch_mapping;
	phys_addr_t ch_mapping_paddr;
	unsigned int *ldc_seg;
	phys_addr_t ldc_seg_paddr;
	unsigned char swduty_fid;
};

struct abcon_s {
	struct abcon_conf_s conf;
	unsigned char chip_type;
	unsigned char seg_row;
	unsigned char seg_col;
	unsigned char act_lane_num;
	unsigned int act_lane;
	unsigned int lane_ch[12];
	unsigned int max_lane_dim;
	unsigned int max_lane_ch;
	unsigned int tot_ch_num;
};

extern struct abcon_mem_s abcon_mem;
extern struct abcon_s *abcon;

void ldim_abcon_swrst(void);
void ldim_abcon_init_common_registers(struct abcon_s *abcon);
void ldim_abcon_set_gpio(struct abcon_s *abcon);
void ldim_abcon_set_txrx_clk(unsigned int txclk, unsigned int rxclk);
void ldim_abcon_fb_pwm(struct aml_ldim_driver_s *ldim_drv);

void ldim_abcon_load_ch_mapping(struct ldim_dev_driver_s *dev_drv, unsigned int len);
void ldim_abcon_swduty_set(struct aml_ldim_driver_s *ldim_drv);

int ldim_abcon_debug_probe(struct ldim_dev_driver_s *dev_drv);
int ldim_abcon_debug_remove(struct ldim_dev_driver_s *dev_drv);

#endif//__LDIM_ABCON_H__
