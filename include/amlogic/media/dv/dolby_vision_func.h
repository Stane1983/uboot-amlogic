/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _DV_H_
#define _DV_H_

#include <linux/types.h>

#define EXT_MD_LEVEL_1   BIT(0)
#define EXT_MD_LEVEL_2   BIT(1)
#define EXT_MD_LEVEL_4   BIT(2)
#define EXT_MD_LEVEL_5   BIT(3)
#define EXT_MD_LEVEL_6   BIT(4)
#define EXT_MD_LEVEL_255 BIT(31)

#define NUM_IPCORE1 2
#define NUM_IPCORE2 1
#define IPCORE2_ID NUM_IPCORE1
#define OUTPUT_CONTROL_DATA_SIZE 0x1000

enum input_mode_enum {
	IN_MODE_OTT = 0,
	IN_MODE_HDMI = 1,
	IN_MODE_GRAPHICS = 2
};

enum signal_format_enum {
	FORMAT_INVALID = -1,
	FORMAT_DOVI = 0,
	FORMAT_HDR10 = 1,
	FORMAT_SDR = 2,
	FORMAT_DOVI_LL = 3
};

enum priority_mode_enum {
	V_PRIORITY = 0,
	G_PRIORITY = 1
};

enum cp_chroma_format_enum {
	CP_P420 = 0,
	CP_UYVY = 1,
	CP_P444 = 2,
	CP_I444 = 3
};

enum cp_signal_range_enum  {
	SIGNAL_RANGE_SMPTE = 0,
	SIGNAL_RANGE_FULL  = 1,
	SIGNAL_RANGE_SDI   = 2
};

enum cp_clr_enum {
	CP_YUV = 0,
	CP_RGB = 1,
	CP_IPT = 2
};

enum graphics_format_enum  {
	G_SDR_YUV = 0,
	G_SDR_RGB = 1,
	G_HDR_YUV = 2,
	G_HDR_RGB = 3
};

struct composer_reg_ipcore {
	/* offset 0xc8 */
	u32 composer_mode;
	u32 vdr_resolution;
	u32 bit_depth;
	u32 coefficient_log2_denominator;
	u32 bl_num_pivots_y;
	u32 bl_pivot[5];
	u32 bl_order;
	u32 bl_coefficient_y[8][3];
	u32 el_nlq_offset_y;
	u32 el_coefficient_y[3];
	u32 mapping_idc_u;
	u32 bl_num_pivots_u;
	u32 bl_pivot_u[3];
	u32 bl_order_u;
	u32 bl_coefficient_u[4][3];
	u32 mmr_coefficient_u[22][2];
	u32 mmr_order_u;
	u32 el_nlq_offset_u;
	u32 el_coefficient_u[3];
	u32 mapping_idc_v;
	u32 bl_num_pivots_v;
	u32 bl_pivot_v[3];
	u32 bl_order_v;
	u32 bl_coefficient_v[4][3];
	u32 mmr_coefficient_v[22][2];
	u32 mmr_order_v;
	u32 el_nlq_off_v;
	u32 el_coefficient_v[3];
};

struct dm_reg_ipcore1 {
	u32 s_range;
	u32 s_range_inverse;
	u32 frame_fmt1;
	u32 frame_fmt2;
	u32 frame_pixel_def;
	u32 y2rgb_coeff1;
	u32 y2rgb_coeff2;
	u32 y2rgb_coeff3;
	u32 y2rgb_coeff4;
	u32 y2rgb_coeff5;
	u32 y2rgb_off1;
	u32 y2rgb_off2;
	u32 y2rgb_off3;
	u32 eotf;
	u32 a2b_coeff1;
	u32 a2b_coeff2;
	u32 a2b_coeff3;
	u32 a2b_coeff4;
	u32 a2b_coeff5;
	u32 c2d_coeff1;
	u32 c2d_coeff2;
	u32 c2d_coeff3;
	u32 c2d_coeff4;
	u32 c2d_coeff5;
	u32 c2d_off;
	u32 active_left_top;
	u32 active_bottom_right;
};

struct dm_reg_ipcore2 {
	u32 s_range;
	u32 s_range_inverse;
	u32 y2rgb_coeff1;
	u32 y2rgb_coeff2;
	u32 y2rgb_coeff3;
	u32 y2rgb_coeff4;
	u32 y2rgb_coeff5;
	u32 y2rgb_off1;
	u32 y2rgb_off2;
	u32 y2rgb_off3;
	u32 frame_fmt;
	u32 eotf;
	u32 a2b_coeff1;
	u32 a2b_coeff2;
	u32 a2b_coeff3;
	u32 a2b_coeff4;
	u32 a2b_coeff5;
	u32 c2d_coeff1;
	u32 c2d_coeff2;
	u32 c2d_coeff3;
	u32 c2d_coeff4;
	u32 c2d_coeff5;
	u32 c2d_off;
	u32 vdr_res;
};

struct dm_reg_ipcore3 {
	u32 d2c_coeff1;
	u32 d2c_coeff2;
	u32 d2c_coeff3;
	u32 d2c_coeff4;
	u32 d2c_coeff5;
	u32 b2a_coeff1;
	u32 b2a_coeff2;
	u32 b2a_coeff3;
	u32 b2a_coeff4;
	u32 b2a_coeff5;
	u32 eotf_param1;
	u32 eotf_param2;
	u32 ipt_scale;
	u32 ipt_off1;
	u32 ipt_off2;
	u32 ipt_off3;
	u32 output_range1;
	u32 output_range2;
	u32 rgb2yuv_coeff_reg1;
	u32 rgb2yuv_coeff_reg2;
	u32 rgb2yuv_coeff_reg3;
	u32 rgb2yuv_coeff_reg4;
	u32 rgb2yuv_coeff_reg5;
	u32 rgb2yuv_off0;
	u32 rgb2yuv_off1;
	u32 rgb2yuv_off2;
};

/*dm luts for core1 and core2 */
struct dm_lut_ipcore {
	u32 tm_lut_i[64 * 4];
	u32 tm_lut_s[64 * 4];
	u32 sm_lut_i[64 * 4];
	u32 sm_lut_s[64 * 4];
	u32 g_2_l[256];
};

/*hdmi metadata for core3 */
struct md_reg_ipcore3 {
	u32 raw_metadata[512];
	u32 size;
};

struct hdr10_infoframe {
	u8 type_code;
	u8 version_number;
	u8 len_of_info_frame;
	u8 data_byte1;
	u8 data_byte2;
	u8 primaries_x_0_lsb;
	u8 primaries_x_0_msb;
	u8 primaries_y_0_lsb;
	u8 primaries_y_0_msb;
	u8 primaries_x_1_lsb;
	u8 primaries_x_1_msb;
	u8 primaries_y_1_lsb;
	u8 primaries_y_1_msb;
	u8 primaries_x_2_lsb;
	u8 primaries_x_2_msb;
	u8 primaries_y_2_lsb;
	u8 primaries_y_2_msb;
	u8 white_point_x_lsb;
	u8 white_point_x_msb;
	u8 white_point_y_lsb;
	u8 white_point_y_msb;
	u8 max_display_mastering_lum_lsb;
	u8 max_display_mastering_lum_msb;
	u8 min_display_mastering_lum_lsb;
	u8 min_display_mastering_lum_msb;
	u8 max_content_light_level_lsb;
	u8 max_content_light_level_msb;
	u8 max_frame_avg_light_level_lsb;
	u8 max_frame_avg_light_level_msb;
};

struct hdr10_parameter {
	u32 min_display_mastering_lum;
	u32 max_display_mastering_lum;
	u16 r_x;
	u16 r_y;
	u16 g_x;
	u16 g_y;
	u16 b_x;
	u16 b_y;
	u16 w_x;
	u16 w_y;
	u16 max_content_light_level;
	u16 max_frame_avg_light_level;
};

struct ext_level_1 {
	u8 min_pq_h;
	u8 min_pq_l;
	u8 max_pq_h;
	u8 max_pq_l;
	u8 avg_pq_h;
	u8 avg_pq_l;
};

struct ext_level_2 {
	u8 target_max_pq_h;
	u8 target_max_pq_l;
	u8 trim_slope_h;
	u8 trim_slope_l;
	u8 trim_off_h;
	u8 trim_off_l;
	u8 trim_power_h;
	u8 trim_power_l;
	u8 trim_chroma_weight_h;
	u8 trim_chroma_weight_l;
	u8 trim_sat_gain_h;
	u8 trim_sat_gain_l;
	u8 ms_weight_h;
	u8 ms_weight_l;
};

struct ext_level_4 {
	u8 anchor_pq_h;
	u8 anchor_pq_l;
	u8 anchor_power_h;
	u8 anchor_power_l;
};

struct ext_level_5 {
	u8 active_area_left_off_h;
	u8 active_area_left_off_l;
	u8 active_area_right_off_h;
	u8 active_area_right_off_l;
	u8 active_area_top_off_h;
	u8 active_area_top_off_l;
	u8 active_area_bot_off_h;
	u8 active_area_bot_off_l;
};

struct ext_level_6 {
	u8 max_display_mastering_lum_h;
	u8 max_display_mastering_lum_l;
	u8 min_display_mastering_lum_h;
	u8 min_display_mastering_lum_l;
	u8 max_content_light_level_h;
	u8 max_content_light_level_l;
	u8 max_frame_avg_light_level_h;
	u8 max_frame_avg_light_level_l;
};

struct ext_level_255 {
	u8 run_mode;
	u8 run_version;
	u8 dm_debug_0;
	u8 dm_debug_1;
	u8 dm_debug_2;
	u8 dm_debug_3;
};

struct ext_md_s {
	u32 avail_level_mask;
	struct ext_level_1 level_1;
	struct ext_level_2 level_2;
	struct ext_level_4 level_4;
	struct ext_level_5 level_5;
	struct ext_level_6 level_6;
	struct ext_level_255 level_255;
};

enum ctrl_data_type_enum {
	TYPE_INVALID = -1,
	TYPE_VSIF = 0,
	TYPE_VSEM = 1
};

enum cp_dv_type_eunm {
	SRC_TYPE_NONDOVI = 0x0, /*output is not amdv*/
	SRC_TYPE_DV    = 0x1, /*input is dv content, and output is amdv. */
	SRC_TYPE_HDR10   = 0x3, /*input is HDR10, and output is a amdv. */
	SRC_TYPE_SDR     = 0x5, /*input is SDR, and output is a amdv*/
	SRC_TYPE_HLG     = 0x7  /*input is HLG, and output is a amdv */
};

struct vsif_parameter_s {
	int lowlatency;
	int backlight_ctrl_md_present;
	int src_dm_version;
	int eff_tmax_pq;
	enum cp_dv_type_eunm dobly_vision_signal;
	int auxi_md_present;
	int l11_md_present;
	u8 auxi_runmode;
	u8 auxi_runversion;
	u8 auxi_debug0;
	u8 content_type;
	u8 intended_white_point;
	u8 l11_byte2;
	u8 l11_byte3;
	int bt2020_container;
};

struct content_info_s {
	u8 content_type_info;
	u8 white_point;
	u8 l11_byte2;
	u8 l11_byte3;
};

struct dovi_setting_video_s {
	struct composer_reg_ipcore comp_reg;
	struct dm_reg_ipcore1 dm_reg;
	struct dm_lut_ipcore dm_lut;
};

struct dovi_setting_s {
	struct composer_reg_ipcore comp_reg;
	struct dm_reg_ipcore1 dm_reg1;
	struct dm_reg_ipcore2 dm_reg2;
	struct dm_reg_ipcore3 dm_reg3;
	struct dm_lut_ipcore dm_lut1;
	struct dm_lut_ipcore dm_lut2;
	/* for dovi output */
	struct md_reg_ipcore3 md_reg3;
	/* for hdr10 output */
	struct hdr10_infoframe hdr_info;
	/* current process */
	enum signal_format_enum src_format;
	enum signal_format_enum dst_format;
	/* enhanced layer */
	bool el_flag;
	bool el_halfsize_flag;
	/* frame width & height */
	u32 video_width;
	u32 video_height;
	/* use for stb 2.4 */
	enum graphics_format_enum g_format;
	u32 g_bitdepth;
	u32 dovi2hdr10_nomapping;
	u32 use_ll_flag;
	u32 ll_rgb_desired;
	u32 diagnostic_enable;
	u32 diagnostic_mux_select;
	u32 dovi_ll_enable;
	u32 vout_width;
	u32 vout_height;
	u8 vsvdb_tbl[32];
	struct ext_md_s ext_md;
};

struct cp_struct {
	int src_format;
	int dst_format;
	void *in_comp;
	int in_comp_size;
	void *in_md;
	int in_md_size;
	int set_priority;
	int set_bit_depth;
	int set_chroma_format;
	int set_yuv_range;
	int set_graphic_min_lum;
	int set_graphic_max_lum;
	int set_target_min_lum;
	int set_target_max_lum;
	int set_no_el;
	void *p_hdr10_param;
	void *dovi_setting;
};

//multi mode releated
struct private_info_s {
	int valid; /*1: input is valid; 0: input is invalid*/
	enum signal_format_enum src_format;

	/* enhanced layer */
	bool el_flag;
	bool el_halfsize_flag;

	/* frame width & height */
	u32 video_width;
	u32 video_height;

	/* Dovi LL or non Dovi */
	int set_bit_depth;
	enum cp_chroma_format_enum set_chroma_format;
	enum cp_signal_range_enum set_yuv_range;
	enum cp_clr_enum color_format;

	char *in_comp;
	int in_comp_size;
	char *in_md;
	int in_md_size;
	char *vsem_if;
	int vsem_if_size;
	enum input_mode_enum input_mode;
	int use_primaries_for_dv;
	struct hdr10_parameter *p_hdr10_param;
};

struct m_dovi_setting_s {
	/*********input to dv lib*******/
	/*common info*/
	int num_input;
	int num_video;
	int pri_input;
	int enable_multi_core1;
	int enable_debug;
	enum priority_mode_enum set_priority;
	uint32_t dovi2hdr10_nomapping;
	uint32_t use_ll_flag;
	uint32_t ll_rgb_desired;
	uint32_t vout_width;
	uint32_t vout_height;
	u8 vsvdb_tbl[32];
	u32 vsvdb_len;
	u32 vsvdb_changed;
	u32 mode_changed;
	enum signal_format_enum dst_format;
	/*private info for each instance*/
	struct private_info_s input[NUM_IPCORE1 + NUM_IPCORE2];
	/*only used for graphic*/
	int set_graphic_min_lum;
	int set_graphic_max_lum;
	int set_target_min_lum;
	int set_target_max_lum;

	/********get from dv lib********/
	struct dovi_setting_video_s core1[NUM_IPCORE1];
	struct dm_reg_ipcore2 dm_reg2;
	struct dm_reg_ipcore3 dm_reg3;
	struct dm_lut_ipcore dm_lut2;
	/* for dovi output */
	struct md_reg_ipcore3 md_reg3;
	/* for hdr10 output */
	struct hdr10_infoframe hdr_info;
	uint32_t diagnostic_enable;
	uint32_t diagnostic_mux_select;
	uint32_t dovi_ll_enable;
	struct ext_md_s ext_md;
	struct vsif_parameter_s output_vsif;
	u8 *output_ctrl_data; /* VSEM or VSIF */
	u32 output_ctrl_data_len;
	enum ctrl_data_type_enum ctrl_data_type;
	struct content_info_s content_info;/*copy src content infos*/
	u32 reserved[128];
};

int control_path
	(enum signal_format_enum in_format,
	 enum signal_format_enum out_format,
	 char *in_comp, int in_comp_size,
	 char *in_md, int in_md_size,
	 enum priority_mode_enum set_priority,
	 int set_bit_depth, int set_chroma_format, int set_yuv_range,
	 int set_graphic_min_lum, int set_graphic_max_lum,
	 int set_target_min_lum, int set_target_max_lum,
	 int set_no_el,
	 struct hdr10_parameter *hdr10_param,
	 struct dovi_setting_s *output);

int multi_control_path(struct m_dovi_setting_s *cp_para);
#endif
