/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_COMMON_H
#define _AML_LCD_COMMON_H
#include <div64.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "./lcd_clk/lcd_clk_config.h"
#include "lcd_unifykey.h"
#include "lcd_reg.h"
#include "lcd_parser/json_parse.h"

/* 20240314: sync from uboot2019 (3fb59b45dc4) + 405906/5 + 407129/6 */
/* 20240318: optimize tcon reserved memory */
/* 20240412: lcd_extern support more option for tcon pmic usage */
/* 20240607: lcd tcon support extern header */
/* 20240618: lcd tcon new ctrl_type(resolution) for demura multi lut */
/* 20240620: optimize tcon multi data set */
/* 20240704: lcd tcon support user info */
/* 20240710: add support for S6 */
/* 20240712: lcd tcon lut dma flow optimize */
/* 20240806: support phy tuning function */
/* 20240815: sync lcd multi-timing from 2019 */
/* 20240909: update phy tuning: get real state from register */
/* 20240923: support reserved memory to transmit panel parameter to kernel */
/* 20241108: optimize config load flow */
/* 20241127: add lcd config json parse driver */
#define LCD_DRV_VERSION    "20241127"

#define CFMT_RGB565          0x05
#define CFMT_RGB_6bit        0x06
#define CFMT_RGB_8bit        0x08
#define CFMT_RGB_10bit       0x0a
#define CFMT_RGB_12bit       0x0c
#define CFMT_YCbCr422_8bit   0x18
#define CFMT_YCbCr422_10bit  0x1a
#define CFMT_YCbCr422_12bit  0x1c
#define CFMT_YCbCr444_8bit   0x28
#define CFMT_YCbCr444_10bit  0x2a
#define CFMT_YCbCr444_12bit  0x2c
#define CFMT_YCbCr420_8bit   0x38
#define CFMT_YCbCr420_10bit  0x3a
#define CFMT_YCbCr420_12bit  0x3c

struct color_fmt_info_s {
	unsigned int cfmt;
	unsigned char bits;
	char name[32];
};

void mdelay(unsigned long n);

static inline unsigned long long lcd_do_div(unsigned long long num, unsigned int den)
{
	unsigned long long ret = num;

	do_div(ret, den);

	return ret;
}

static inline unsigned long long div_around(unsigned long long num, unsigned int den)
{
	unsigned long long ret = num + den / 2;

	if (den == 1)
		return num;

	do_div(ret, den);

	return ret;
}

static inline unsigned long long lcd_diff(unsigned long long a, unsigned long long b)
{
	return (a >= b) ? (a - b) : (b - a);
}

static inline int lcd_s32_constraint(int v, int min, int max)
{
	return v > max ? max : v < min ? min : v;
}

extern unsigned int lcd_prbs_freq, lcd_prbs_performed, lcd_prbs_err;

struct num_str_s {
	int  num;
	char str[32];
};

void lcd_display_init_test(struct aml_lcd_drv_s *pdrv);
void lcd_display_init_reg_dump(struct aml_lcd_drv_s *pdrv);

#define LCD_CMA_PAGE_SIZE_1K (1 * 1024)
#define LCD_CMA_PAGE_SIZE_2K (2 * 1024)
#define LCD_CMA_PAGE_SIZE_4K (4 * 1024)
#define LCD_CMA_PAGE_SIZE_8K (8 * 1024)

/* lcd common */
int strnum_get_num(const char *str, struct num_str_s *arr, int size_arr, int dft);
char *strnum_get_str(int num, struct num_str_s *arr, int size_arr, char *dft);
int path_name_compose(const char *path, const char *name, char *path_name);
void mem_dump(unsigned char *addr, int size);
int string_to_numbers(const char *str, unsigned int nums[]);

int lcd_base_config_load_from_dts(char *dt_addr, struct aml_lcd_drv_s *pdrv);
int lcd_base_config_load_from_bsp(struct aml_lcd_drv_s *pdrv);
void lcd_panel_config_load_to_drv(struct aml_lcd_drv_s *pdrv);
int lcd_get_panel_config(char *dt_addr, int load_id, struct aml_lcd_drv_s *pdrv);

unsigned int str_add_vmode(char *buf, struct lcd_vmode_info_s *vm_info, unsigned short framerate);
void lcd_cma_pool_init(struct aml_lcd_cma_mem *cma,
		phys_addr_t pa, unsigned long size, unsigned int page_size);
void *lcd_cma_pool_simple_alloc(struct aml_lcd_cma_mem *cma, unsigned long size);
void *lcd_alloc_dma_buffer(struct aml_lcd_drv_s *pdrv, unsigned long size);

int lcd_type_str_to_type(const char *str);
char *lcd_type_type_to_str(int type);
int lcd_mode_str_to_mode(const char *str);
char *lcd_mode_mode_to_str(int mode);
int lcd_get_dts_panel_node_ofst(unsigned char drv_idx);
unsigned char dtimg_info_add(char *c_buf, struct lcd_detail_timing_s *dtm, unsigned char c_bits);

void lcd_encl_on(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_config_timing_check(struct aml_lcd_drv_s *pdrv,
				     struct lcd_detail_timing_s *ptiming);

void update_panel_param_to_kernel(void);
unsigned char lcd_get_dbg_source(void);
unsigned char lcd_panel_config_load_detect(int index, int dt_valid, int key_valid);
int panel_json_parse(struct json_parse_s *jsp, unsigned char *input);

void lcd_clk_frame_rate_init(struct lcd_detail_timing_s *ptiming);
void lcd_default_to_basic_timing_init_config(struct aml_lcd_drv_s *pdrv);
void lcd_enc_timing_init_config(struct aml_lcd_drv_s *pdrv);

int lcd_fr_is_frac(struct aml_lcd_drv_s *pdrv, unsigned int frame_rate);
int lcd_vmode_frac_is_support(struct aml_lcd_drv_s *pdrv, unsigned int frame_rate);
int lcd_frame_rate_change(struct aml_lcd_drv_s *pdrv);
void lcd_pinmux_set(struct aml_lcd_drv_s *pdrv, int status);
void lcd_vbyone_bit_rate_config(struct aml_lcd_drv_s *pdrv);
void lcd_mlvds_bit_rate_config(struct aml_lcd_drv_s *pdrv);
void lcd_p2p_bit_rate_config(struct aml_lcd_drv_s *pdrv);
void lcd_mipi_dsi_bit_rate_config(struct aml_lcd_drv_s *pdrv);
void lcd_edp_bit_rate_config(struct aml_lcd_drv_s *pdrv);
struct lcd_detail_timing_s *lcd_timing_alloc(struct aml_lcd_drv_s *pdrv);
void lcd_timing_free_last(struct aml_lcd_drv_s *pdrv);
struct phy_attr_s *lcd_phy_alloc(struct aml_lcd_drv_s *pdrv);
void lcd_phy_free_last(struct aml_lcd_drv_s *pdrv);

void lcd_detail_timing_print(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *dt);
void lcd_phy_cfg_print(struct phy_config_s *cfg);
void lcd_phy_attr_print(struct phy_attr_s *phy, u32 lane_num);

/* lcd cus_ctrl */
void lcd_cus_ctrl_dump_raw_data(struct aml_lcd_drv_s *pdrv);
void lcd_cus_ctrl_dump_info(struct aml_lcd_drv_s *pdrv);
int lcd_cus_ctrl_load_from_dts(struct aml_lcd_drv_s *pdrv);
int lcd_cus_ctrl_load_from_unifykey(struct aml_lcd_drv_s *pdrv, unsigned char *buf,
		unsigned int max_size, unsigned char version);
void lcd_cus_ctrl_config_remove(struct aml_lcd_drv_s *pdrv);
int lcd_cus_ctrl_config_update(struct aml_lcd_drv_s *pdrv, void *param, unsigned int mask_sel);
void lcd_cus_ctrl_state_clear(struct aml_lcd_drv_s *pdrv, unsigned int mask_sel);
int lcd_cus_ctrl_timing_is_valid(struct aml_lcd_drv_s *pdrv);
int lcd_cus_ctrl_timing_is_activated(struct aml_lcd_drv_s *pdrv);
struct lcd_detail_timing_s **lcd_cus_ctrl_timing_match_get(struct aml_lcd_drv_s *pdrv);

/* lcd venc */
void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_get_encl_line_cnt(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_get_max_line_cnt(struct aml_lcd_drv_s *pdrv);
void lcd_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num);
void lcd_set_venc_timing(struct aml_lcd_drv_s *pdrv);
void lcd_set_venc(struct aml_lcd_drv_s *pdrv);
void lcd_venc_enable(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_mute_set(struct aml_lcd_drv_s *pdrv,  unsigned char flag);
void lcd_venc_reg_print(struct aml_lcd_drv_s *pdrv);
void lcd_venc_save_bootctrl_to_regs(struct aml_lcd_drv_s *pdrv);
int lcd_venc_probe(struct aml_lcd_data_s *pdata);

/* lcd clk*/
struct lcd_clk_config_s *get_lcd_clk_config(struct aml_lcd_drv_s *pdrv);
void lcd_clk_config_print(struct aml_lcd_drv_s *pdrv);
void lcd_clk_reg_print(struct aml_lcd_drv_s *pdrv);
void lcd_get_ss(struct aml_lcd_drv_s *pdrv);
int lcd_set_ss(struct aml_lcd_drv_s *pdrv, unsigned int level,
	       unsigned int freq, unsigned int mode);
void lcd_update_clk_frac(struct aml_lcd_drv_s *pdrv);
void lcd_set_clk(struct aml_lcd_drv_s *pdrv);
void lcd_disable_clk(struct aml_lcd_drv_s *pdrv);
void lcd_clk_generate_parameter(struct aml_lcd_drv_s *pdrv);
void lcd_clk_config_probe(struct aml_lcd_drv_s *pdrv);
int aml_lcd_prbs_test(struct aml_lcd_drv_s *pdrv, unsigned int ms, unsigned int mode_flag);

/* lcd phy */
unsigned int lcd_phy_check_lane_phase_sel(struct aml_lcd_drv_s *pdrv);
int lcd_phy_param_preset(struct aml_lcd_drv_s *pdrv);
int lcd_phy_param_get(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg,
		      struct phy_attr_s *phy);
void lcd_phy_param_print(struct aml_lcd_drv_s *pdrv);
void lcd_phy_analog_reg_print(struct aml_lcd_drv_s *pdrv);
void lcd_phy_set(struct aml_lcd_drv_s *pdrv, int status);
int lcd_phy_probe(struct aml_lcd_drv_s *pdrv);
int lcd_phy_config_init(struct aml_lcd_data_s *pdata);

/* lcd dphy */
void lcd_lane_map_preset(struct aml_lcd_drv_s *pdrv);
void lcd_lane_map_update(struct aml_lcd_drv_s *pdrv);
int lcd_lane_sel_get(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg);
void lcd_mipi_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
void lcd_edp_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
void lcd_lvds_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
void lcd_vbyone_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
#ifdef CONFIG_AML_LCD_TCON
void lcd_mlvds_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
void lcd_p2p_dphy_set(struct aml_lcd_drv_s *pdrv, unsigned char on_off);
#endif
void lcd_dphy_reg_print(struct aml_lcd_drv_s *pdrv);

/* lcd lvds*/
void lcd_lvds_enable(struct aml_lcd_drv_s *pdrv);
void lcd_lvds_disable(struct aml_lcd_drv_s *pdrv);

/* lcd vbyone*/
void lcd_vbyone_enable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_disable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_sw_reset(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_wait_timing_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_cdr_training_hold(struct aml_lcd_drv_s *pdrv, int flag);
void lcd_vbyone_wait_hpd(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_wait_stable(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_hw_filter(struct aml_lcd_drv_s *pdrv, int flag);

/* lcd tcon */
#ifdef CONFIG_AML_LCD_TCON
void lcd_tcon_info_print(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_top_init(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_enable(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_disable(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_global_reset(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_dbg_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming);
int lcd_tcon_probe(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id);
int lcd_tcon_is_dccd_flow(void);
#endif

/* lcd gpio */
int lcd_gpio_name_map_num(const char *name);
int lcd_gpio_set(int gpio, int value);
unsigned int lcd_gpio_input_get(int gpio);

/* lcd debug */
int lcd_debug_info_len(int num);
void lcd_info_print(struct aml_lcd_drv_s *pdrv);
void lcd_reg_print(struct aml_lcd_drv_s *pdrv);
void lcd_vbyone_rst(struct aml_lcd_drv_s *pdrv);
int lcd_vbyone_cdr(struct aml_lcd_drv_s *pdrv);
int lcd_vbyone_lock(struct aml_lcd_drv_s *pdrv);
void lcd_debug_probe(struct aml_lcd_drv_s *pdrv);

char *get_current_env_connector(unsigned char cnt_idx);
void sprintf_lcd_connector(char *buf, unsigned char lcd_idx, unsigned char lcd_type);

/* lcd driver */
#ifdef CONFIG_AML_LCD_TV
int lcd_mode_tv_init(struct aml_lcd_drv_s *pdrv);
#endif

#ifdef CONFIG_AML_LCD_TABLET
int lcd_mode_tablet_init(struct aml_lcd_drv_s *pdrv);
/* @dsi_common.c */
void lcd_dsi_init_table_load_dts(char *dtaddr, int offset, struct dsi_config_s *dconf);
void lcd_dsi_init_table_load_bsp(struct dsi_config_s *dconf);
void lcd_dsi_tx_ctrl(struct aml_lcd_drv_s *pdrv, unsigned char en);
unsigned long long lcd_dsi_get_min_bitrate(struct aml_lcd_drv_s *pdrv);
/* @dsi_debug.c */
void lcd_dsi_info_print(struct lcd_config_s *pconf);
void lcd_dsi_set_operation_mode(struct aml_lcd_drv_s *pdrv, unsigned char op_mode);
void lcd_dsi_dphy_test(struct aml_lcd_drv_s *pdrv, unsigned char test_item);
void lcd_dsi_write_cmd(struct aml_lcd_drv_s *pdrv, unsigned char *payload);
unsigned char lcd_dsi_read(struct aml_lcd_drv_s *pdrv,
			unsigned char *payload, unsigned char *rd_data, unsigned char rd_byte_len);
/* @dsi_addons/dsi_check_panel.c */
int mipi_dsi_check_state(struct aml_lcd_drv_s *pdrv, unsigned char reg, unsigned char cnt);

/* @lcd_eDP.c */
void dptx_DPCD_dump(struct aml_lcd_drv_s *pdrv);
int eDP_debug_test(struct aml_lcd_drv_s *pdrv, char *str, int num);
void edp_tx_ctrl(struct aml_lcd_drv_s *pdrv, int flag);
#endif

void lcd_wait_vsync(struct aml_lcd_drv_s *pdrv);
#if IS_ENABLED(CONFIG_CMD_INI)
unsigned int is_support_dccd(void);
unsigned int dccd_has_tcon_file(void);
unsigned int get_dccd_crc(void);
#endif

/* aml_bl driver */
void aml_bl_probe_single(unsigned char index, int load_id);
void aml_bl_remove_all(void);
int aml_bl_index_add(int drv_index, int conf_index);
int aml_bl_index_remove(int drv_index);
int aml_bl_init(void);
void aml_bl_driver_enable(int index);
void aml_bl_driver_disable(int index);
void aml_bl_set_level(int index, unsigned int level);
unsigned int aml_bl_get_level(int index);
void aml_bl_config_print(int index);
int aml_bl_pwm_reg_config_init(struct aml_lcd_data_s *pdata);

unsigned int lcd_crc32(unsigned int seed, const unsigned char *ptr, int buf_len);
#ifdef CONFIG_AML_LCD_JSON
struct json_parse_s *get_panel_jsp(int index);
#endif

#endif

