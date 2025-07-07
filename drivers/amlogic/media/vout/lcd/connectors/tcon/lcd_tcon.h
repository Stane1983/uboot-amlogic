/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_TCON_H
#define _AML_LCD_TCON_H
#include <amlogic/media/vout/lcd/aml_lcd.h>

#define REG_LCD_TCON_MAX    0xffff

struct lcd_tcon_init_setting_s {
	unsigned int reg;
	unsigned int val;
	unsigned int bit;
	unsigned int len;
};

struct lcd_tcon_axi_mem_cfg_s {
	unsigned int mem_type;
	unsigned int mem_size;
	unsigned int axi_reg;  //ddrif reg
	unsigned int mem_valid;
};

struct lcd_tcon_dma_ops_s {
	int status;
	struct  list_head *addr_list;
	int (*get_frame_cnt)(struct aml_lcd_drv_s *pdrv);
	void (*start)(struct aml_lcd_drv_s *pdrv);
	void (*stop)(struct aml_lcd_drv_s *pdrv);
	void (*mif_set)(struct aml_lcd_drv_s *pdrv, phys_addr_t paddr, unsigned int size);
	void (*init)(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
	void (*deinit)(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
	void (*init_trans)(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
};

struct lcd_tcon_reg_skip_s {
	unsigned int start_reg;
	unsigned int end_reg;
};

#define TCON_BIN_VER_LEN    9
struct tcon_core_reg_info_s {
	char bin_ver[TCON_BIN_VER_LEN];
	struct lcd_tcon_init_block_header_s *header;
	struct lcd_tcon_init_block_ext_header_s ext_header;
	unsigned int table_size;
	unsigned char *table;
	char *user_info;
};

struct lcd_tcon_config_s {
	unsigned char tcon_valid;

	unsigned int core_reg_ver;
	unsigned int core_reg_width;
	unsigned int reg_table_width;
	unsigned int reg_table_len;
	unsigned int core_reg_start;
	struct lcd_tcon_reg_skip_s *reg_skip_tbl;  //skip flush core reg range
	unsigned int reg_skip_tbl_len;

	unsigned int axi_bank;

	unsigned int rsv_mem_size;
	unsigned int axi_size;
	unsigned int bin_path_size;
	unsigned int secure_cfg_size;

	struct lcd_tcon_init_setting_s *init_pre_setting;
	struct lcd_tcon_init_setting_s *init_post_setting;
	unsigned int axi_tbl_len;
	struct lcd_tcon_axi_mem_cfg_s *axi_mem_cfg_tbl;
	struct lcd_tcon_dma_ops_s *lut_dma_ops;

	void (*tcon_axi_mem_config)(void);
	void (*tcon_init_table_pre_proc)(struct tcon_core_reg_info_s *core_reg_info);
	void (*tcon_init_table_post_proc)(struct aml_lcd_drv_s *pdrv);
	void (*tcon_lut_post_proc)(struct aml_lcd_drv_s *pdrv, unsigned int lut_type);
	void (*tcon_global_reset)(struct aml_lcd_drv_s *pdrv);
	int (*tcon_top_init)(struct aml_lcd_drv_s *pdrv);
	int (*tcon_enable)(struct aml_lcd_drv_s *pdrv);
	int (*tcon_disable)(struct aml_lcd_drv_s *pdrv);
	int (*tcon_forbidden_check)(void);
	int (*tcon_check)(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
		struct tcon_core_reg_info_s *core_reg_info, char *ferr_str, char *warn_str);
};

struct tcon_rmem_config_s {
	unsigned int mem_paddr;
	unsigned char *mem_vaddr;
	unsigned int mem_size;
};

struct tcon_sec_mem_config_s {
	unsigned int mem_paddr;
	unsigned char *mem_vaddr;
	unsigned int mem_size;
	unsigned int sec_handle;
	unsigned int sec_protect;
};

struct tcon_rmem_s {
	unsigned int flag;
	int use_lrm;
	unsigned int axi_bank;

	unsigned int rsv_mem_paddr;
	unsigned int axi_mem_paddr;
	unsigned int rsv_mem_size;
	unsigned int axi_mem_size;

	unsigned int *axi_reg;
	struct tcon_rmem_config_s *axi_rmem;
	struct tcon_rmem_config_s bin_path_rmem;
	struct tcon_rmem_config_s secure_cfg_rmem;
	struct tcon_sec_mem_config_s secure_axi_rmem;

	struct tcon_rmem_config_s vac_rmem;
	struct tcon_rmem_config_s demura_set_rmem;
	struct tcon_rmem_config_s demura_lut_rmem;
	struct tcon_rmem_config_s acc_lut_rmem;
};

struct tcon_mem_map_table_s {
	/*header*/
	unsigned int version;
	unsigned int data_load_level;
	unsigned int block_cnt;
	//unsigned char init_load;
	unsigned char data_complete;
	unsigned char bin_path_valid;

	unsigned int lut_valid_flag;
	unsigned char demura_cnt;
	unsigned int block_bit_flag;

	unsigned int *data_size;
	unsigned char **data_mem_vaddr;
};

#define MEM_FLAG_MAX
struct lcd_tcon_local_cfg_s {
	struct tcon_core_reg_info_s def_core_info;
	struct tcon_core_reg_info_s cur_core_info;
	unsigned char is_dccd_flow;
};

/* *** tcon config *** */
/* T5 */
#define LCD_TCON_CORE_REG_WIDTH_T5       32
#define LCD_TCON_TABLE_WIDTH_T5          32
#define LCD_TCON_TABLE_LEN_T5            0x18d4 /* 0x635*4 */
#define LCD_TCON_AXI_BANK_T5             2

#define TCON_CORE_REG_START_T5           0x0100

/* T5D */
#define LCD_TCON_CORE_REG_WIDTH_T5D       32
#define LCD_TCON_TABLE_WIDTH_T5D          32
#define LCD_TCON_TABLE_LEN_T5D            0x102c /* 0x40b*4 */
#define LCD_TCON_AXI_BANK_T5D             1

#define TCON_CORE_REG_START_T5D           0x0100

/* T3X */
#define LCD_TCON_TABLE_LEN_T3X            0x1478 /* 0x51e*4 */
#define LCD_TCON_AXI_BANK_T3X             3

/* TXHD2 */
#define LCD_TCON_TABLE_LEN_TXHD2          0x1670 /* 0x59c*4 */
#define LCD_TCON_AXI_BANK_TXHD2           2

/* T6D */
#define LCD_TCON_TABLE_LEN_T6D            0x1668 /* 0x59a*4 */
#define LCD_TCON_AXI_BANK_T6D             2

/* T6W */
#define TCON_CORE_REG_START_T6W           0x0
#define LCD_TCON_TABLE_LEN_T6W            0x57ac /* 0x15eb*4 */
#define LCD_TCON_AXI_BANK_T6W             2

#ifdef CONFIG_CMD_INI
void *handle_lcd_ext_buf_get(void);
#endif

#define TCON_VAC_SET_PARAM_NUM    3
#define TCON_VAC_LUT_PARAM_NUM    256

int tcon_lut_dma_get_frame_cnt(struct aml_lcd_drv_s *pdrv);
int tcon_lut_dma_get_frame_cnt_t6w(struct aml_lcd_drv_s *pdrv);
void tcon_lut_dma_start(struct aml_lcd_drv_s *pdrv);
void tcon_lut_dma_stop(struct aml_lcd_drv_s *pdrv);
void tcon_lut_dma_start_t6d(struct aml_lcd_drv_s *pdrv);
void tcon_lut_dma_stop_t6d(struct aml_lcd_drv_s *pdrv);
void tcon_lut_dma_mif_set(struct aml_lcd_drv_s *pdrv, phys_addr_t paddr, unsigned int size);
void tcon_lut_dma_mif_set_t6w(struct aml_lcd_drv_s *pdrv, phys_addr_t paddr, unsigned int size);
void tcon_lut_dma_init_t5m(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
void tcon_lut_dma_init_t3x(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
void tcon_lut_dma_init_t6w(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
void tcon_lut_dma_deinit(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);
void lcd_tcon_dma_data_init_trans(struct aml_lcd_drv_s *pdrv, struct lcd_tcon_dma_ops_s *ops);

void lcd_tcon_init_data_version_update(struct tcon_core_reg_info_s *core_info,
		char *data_buf);
int lcd_tcon_data_multi_init_check(struct aml_lcd_drv_s *pdrv,
				   struct lcd_tcon_data_part_ctrl_s *ctrl_part, unsigned char *p);
int lcd_tcon_spi_data_probe(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_debug_probe(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_ext_header_check(struct lcd_tcon_init_block_ext_header_s *ext_header);
int lcd_tcon_get_core_size(struct lcd_tcon_config_s *tcon_conf,
	struct lcd_tcon_init_block_header_s *header,
	struct lcd_tcon_init_block_ext_header_s *ext_header);

int lcd_tcon_valid_check(void);
struct lcd_tcon_config_s *get_lcd_tcon_config(void);
struct tcon_rmem_s *get_lcd_tcon_rmem(void);
struct tcon_mem_map_table_s *get_lcd_tcon_mm_table(void);
struct lcd_tcon_local_cfg_s *get_lcd_tcon_local_cfg(void);

void lcd_tcon_lut_post_proc(struct aml_lcd_drv_s *pdrv, unsigned int lut_type);
void lcd_tcon_global_reset_txhd2(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_global_reset_t5m(struct aml_lcd_drv_s *pdrv);
void lcd_tcon_global_reset_t3x(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_top_set_t5m(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_top_set_t6d(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_enable_t5m(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_enable_txhd2(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_disable_t5m(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_disable_t6d(struct aml_lcd_drv_s *pdrv);

int lcd_tcon_forbidden_check_4k(void);
int lcd_tcon_forbidden_check_fhd(void);

int lcd_tcon_init_setting_check(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
				struct tcon_core_reg_info_s *core_reg_info);
int lcd_tcon_setting_check_t5m(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
				struct tcon_core_reg_info_s *core_reg_info, char *ferr_str, char *warn_str);
int lcd_tcon_setting_check_txhd2(struct aml_lcd_drv_s *pdrv, struct lcd_detail_timing_s *ptiming,
				struct tcon_core_reg_info_s *core_reg_info, char *ferr_str, char *warn_str);

int lcd_tcon_mem_od_is_valid(void);
int lcd_tcon_mem_demura_is_valid(void);
int lcd_tcon_get_table32_reg(struct tcon_core_reg_info_s *core_reg_info,
		unsigned int reg, unsigned int *val);
int lcd_tcon_set_table32_reg(struct tcon_core_reg_info_s *core_reg_info,
		unsigned int reg, unsigned int val);
int lcd_tcon_setb_table32_reg(struct tcon_core_reg_info_s *core_reg_info,
		unsigned int reg, unsigned int val, unsigned int start, unsigned int len);
void lcd_tcon_core_info_print(struct aml_lcd_drv_s *pdrv,
		struct tcon_core_reg_info_s *core_reg_info);

#endif

