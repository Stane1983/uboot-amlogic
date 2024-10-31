/* SPDX-License-Identifier: GPL-2.0+ */
/*
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef _LCD_CLK_UTILS_H
#define _LCD_CLK_UTILS_H

#include "lcd_clk_config.h"

extern char *lcd_clk_div_sel_table[];

/* **********************************
 * lcd controller operation
 * **********************************/
#define PLL_CLK_CHECK_MAX    2000000 /* Hz */
int lcd_clk_msr_check(int msr_id, unsigned int freq);
int lcd_pll_ss_level_generate(struct lcd_clk_config_s *cconf);
int lcd_pll_wait_lock(int id, unsigned int reg, unsigned int lock_bit);

/* ****************************************************
 * lcd clk parameters calculate
 * ****************************************************
 */
#define PLL_FVCO_ERR_MAX    2000 /* Hz */
unsigned long long clk_vid_pll_div_calc(unsigned long long clk, unsigned int div_sel, int dir);
int lcd_pll_get_frac(struct lcd_clk_config_s *cconf, unsigned long long pll_fvco);

/* ****************************************************
 * lcd clk chip default func
 * ****************************************************
 */
void lcd_clk_config_print_dft(struct aml_lcd_drv_s *pdrv);
void lcd_pll_frac_generate_dft(struct aml_lcd_drv_s *pdrv);
void lcd_clk_config_init_print_dft(struct aml_lcd_drv_s *pdrv);
void lcd_clk_generate_dft(struct aml_lcd_drv_s *pdrv);
void lcd_clk_generate_prbs_clk(struct aml_lcd_drv_s *pdrv,
			       unsigned int enc_clk, unsigned long long bit_rate);
int lcd_prbs_clk_check(unsigned int encl_clk, int encl_msr_id, unsigned int fifo_clk,
		       int fifo_msr_id, unsigned int c);
void lcd_set_vid_pll_div_dft(struct lcd_clk_config_s *cconf);
void lcd_set_vclk_crt_dft(struct aml_lcd_drv_s *pdrv);
#ifdef CONFIG_MESON_S6
unsigned char lcd_dsi_generate_DSI_PLL_s6_model(struct aml_lcd_drv_s *pdrv);
#endif

/* ****************************************************
 * lcd clk chip init help func
 * ****************************************************
 */
#ifdef CONFIG_MESON_T5M
void lcd_clk_config_chip_init_t5m(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif
#ifdef CONFIG_MESON_T3X
void lcd_clk_config_chip_init_t3x(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif
#ifdef CONFIG_MESON_A4
void lcd_clk_config_chip_init_a4(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif
#ifdef CONFIG_MESON_TXHD2
void lcd_clk_config_chip_init_txhd2(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif
#ifdef CONFIG_MESON_S6
void lcd_clk_config_chip_init_s6(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif
#ifdef CONFIG_MESON_T6D
void lcd_clk_config_chip_init_t6d(struct aml_lcd_drv_s *pdrv, struct lcd_clk_config_s *cconf);
#endif

unsigned long long lcd_abs(unsigned long long a, unsigned long long b);
#endif
