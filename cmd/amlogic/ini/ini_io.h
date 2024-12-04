/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INI_IO_H__
#define __INI_IO_H__
#include "ini_size_define.h"

#define CS_LCD_ITEM_NAME                          "lcd"
#define CS_LCD_EXT_ITEM_NAME                      "lcd_extern"
#define CS_BACKLIGHT_ITEM_NAME                    "backlight"
#define CS_LDIM_DEV_ITEM_NAME                     "ldim_dev"
#define CS_LCD_TCON_ITEM_NAME                     "lcd_tcon"
#define CS_LCD_TCON_SPI_ITEM_NAME                 "lcd_tcon_spi"
#define CS_LCD_OPTICAL_ITEM_NAME                  "lcd_optical"
#define CS_MODEL_NAME_ITEM_NAME                   "model_name"

#define CS_LCD1_ITEM_NAME                         "lcd1"
#define CS_LCD1_EXT_ITEM_NAME                     "lcd1_extern"
#define CS_BACKLIGHT1_ITEM_NAME                   "backlight1"
#define CS_LCD1_OPTICAL_ITEM_NAME                 "lcd1_optical"
#define CS_MODEL1_NAME_ITEM_NAME                  "model1_name"

#define CS_LCD2_ITEM_NAME                         "lcd2"
#define CS_LCD2_EXT_ITEM_NAME                     "lcd2_extern"
#define CS_BACKLIGHT2_ITEM_NAME                   "backlight2"
#define CS_LCD2_OPTICAL_ITEM_NAME                 "lcd2_optical"
#define CS_MODEL2_NAME_ITEM_NAME                  "model2_name"

#define CS_PANEL_INI_PATH_ITEM_NAME               "panel_ini_path"
#define CS_PANEL_PQ_PATH_ITEM_NAME                "panel_pq_path"
#define CS_PANEL_ALL_INFO_ITEM_NAME               "panel_all_info"
#define CS_PANEL_ALL_DATA_ITEM_NAME               "panel_all"

#define CC_HEAD_CHKSUM_LEN                       (9)
#define CC_VERSION_LEN                           (5)

#ifdef __cplusplus
extern "C" {
#endif

int read_lcd_param(int index, unsigned char data_buf[]);
int save_lcd_param(int index, int wr_size, unsigned char data_buf[]);
int read_lcd_extern_param(int index, unsigned char data_buf[]);
int save_lcd_extern_param(int index, int wr_size, unsigned char data_buf[]);
int read_backlight_param(int index, unsigned char data_buf[]);
int save_backlight_param(int index, int wr_size, unsigned char data_buf[]);
int read_ldim_dev_param(unsigned char data_buf[]);
int save_ldim_dev_param(int wr_size, unsigned char data_buf[]);
int read_model_name_param(int index, unsigned char data_buf[]);
int save_model_name_param(int index, int wr_size, unsigned char data_buf[]);
int read_tcon_spi_param(unsigned char data_buf[]);
int save_tcon_spi_param(int wr_size, unsigned char data_buf[]);
int read_lcd_optical_param(int index, unsigned char data_buf[]);
int save_lcd_optical_param(int index, int wr_size, unsigned char data_buf[]);
int read_tcon_bin_param(unsigned char data_buf[]);
int save_tcon_bin_param(int wr_size, unsigned char data_buf[]);
int read_panel_ini_name(char data_buf[]);
int save_panel_ini_name(char data_buf[]);
int read_panel_PQ_path(char data_buf[]);
int save_panel_PQ_path(char data_buf[]);
int read_panel_all_info_data(unsigned char data_buf[]);
int save_panel_all_info_data(int wr_size, unsigned char data_buf[]);
int read_panel_all_data(int sec_no, unsigned char data_buf[]);
int save_panel_all_data(int sec_no, int wr_size, unsigned char data_buf[]);

int check_hex_data_no_header_valid(unsigned int* tmp_crc32, int max_len, int buf_len, unsigned char data_buf[]);
int check_hex_data_have_header_valid(unsigned int* tmp_crc32, int max_len, int buf_len, unsigned char data_buf[]);
int check_string_data_have_header_valid(unsigned int* tmp_crc32, char *data_str, int chksum_head_len, int ver_len);
unsigned int cal_CRC32(unsigned int crc, const unsigned char *ptr, int buf_len);
void print_data_buf(int data_cnt, unsigned char data_buf[]);

#ifdef __cplusplus
}
#endif

#endif //__INI_IO_H__
