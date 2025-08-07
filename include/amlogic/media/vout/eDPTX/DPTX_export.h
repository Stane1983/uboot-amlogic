/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AMLOGIC_DisplayPort_TX_H
#define _AMLOGIC_DisplayPort_TX_H

#include <amlogic/media/vout/eDPTX/DPTX.h>

// struct aml_lcd_data_s *aml_lcd_get_data(void);
// struct aml_lcd_drv_s *aml_lcd_get_driver(int index);

// char *lcd_get_dt_addr(void);
int dptx_probe(void);

/* global api for cmd */
unsigned int dptx_driver_outputmode_check(u8 index, char *mode);
void dptx_driver_vmode_list(void);

void dptx_driver_probe(void);
uint8_t dptx_driver_prepare(u8 index, char *mode);
uint8_t dptx_driver_enable(u8 index, char *mode);
void dptx_driver_disable(u8 index);
void dptx_driver_info(u8 index);
void dptx_driver_reg_print(u8 index);
void dptx_driver_test(u8 index, u8 num);
void dptx_driver_list_vmode(u8 index);
void dptx_driver_set_vmode(u8 index, u8 num);
void dptx_driver_reset(u8 index, u8 num);
void dptx_driver_PSR1_en(u8 index, u8 num);
void dptx_driver_PSR2_en(u8 index, u8 num);

// void dptx_driver_clk_info(u8 index);
// void dptx_driver_debug_print(u8 index, unsigned int val);
// void dptx_driver_reg_info(u8 index);

// void dptx_driver_list_support_mode(void);
// int aml_lcd_edp_debug(int index, char *str, int num);

// int aml_lcd_driver_prbs(int index, unsigned int s, unsigned int mode_flag);

// int aml_lcd_driver_suspend(void *pm_ops);
// int aml_lcd_driver_resume(void *pm_ops);
// int aml_lcd_driver_poweroff(void *pm_ops);
// void aml_lcd_set_poweron_suspend_sta(int state);

#endif /* INC_AML_LCD_VOUT_H */
