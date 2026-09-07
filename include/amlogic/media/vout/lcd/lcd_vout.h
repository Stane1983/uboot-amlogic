/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef INC_AML_LCD_VOUT_H
#define INC_AML_LCD_VOUT_H

#include <common.h>
#include <linux/list.h>
// #include <dm.h>
#include <asm/gpio.h>
#include <amlogic/media/vout/lcd/lcd_timing.h>
#include <amlogic/media/vout/lcd/lcd_cus_ctrl.h>
#include <amlogic/aml_model.h>

#ifdef CONFIG_AML_LCD_TCON
#include <amlogic/media/vout/lcd/lcd_tcon_data.h>
#endif

/* **********************************
 * debug print define
 * ********************************** */
//#define LCD_DEBUG_INFO
extern unsigned int lcd_debug_print_flag;
//bit[15:0]
#define LCD_DBG_PR_NORMAL       BIT(0) //basic info & flow
#define LCD_DBG_PR_ADV          BIT(1)
#define LCD_DBG_PR_ADV2         BIT(2)
#define LCD_DBG_PR_ISR          BIT(3)
#define LCD_DBG_PR_BL_NORMAL    BIT(4) //basic info & flow
#define LCD_DBG_PR_BL_ADV       BIT(5) //ext, ldim
#define LCD_DBG_PR_BL_PWM       BIT(6)
#define LCD_DBG_PR_BL_ISR       BIT(7)
#define LCD_DBG_PR_CLK          BIT(8)
#define LCD_DBG_PR_TCON         BIT(9)
#define LCD_DBG_PR_MEM          BIT(13)
#define LCD_DBG_PR_TEST         BIT(14)
#define LCD_DBG_PR_REG          BIT(15)

#define LCDPR(fmt, args...)     printf("lcd: "fmt"", ## args)
#define LCDERR(fmt, args...)    printf("lcd: error: "fmt"", ## args)

#define PR_BUF_MAX              (4 * 1024)

#define LCD_MAX_DRV             3

/* **********************************
 * clk parameter bit define
 * pll_ctrl, div_ctrl, clk_ctrl
 * ********************************** */
/* ******** pll_ctrl ******** */
#define PLL_CTRL_OD3                20 /* [21:20] */
#define PLL_CTRL_OD2                18 /* [19:18] */
#define PLL_CTRL_OD1                16 /* [17:16] */
#define PLL_CTRL_N                  9  /* [13:9] */
#define PLL_CTRL_M                  0  /* [8:0] */

/* ******** div_ctrl ******** */
#define DIV_CTRL_EDP_DIV1           24 /* [26:24] */
#define DIV_CTRL_EDP_DIV0           20 /* [23:20] */
#define DIV_CTRL_DIV_SEL            8 /* [15:8] */
#define DIV_CTRL_XD                 0 /* [7:0] */

/* ******** clk_ctrl ******** */
#define CLK_CTRL_LEVEL              28 /* [30:28] */
#define CLK_CTRL_FRAC_SHIFT         24 /* [24] */
#define CLK_CTRL_FRAC               0  /* [23:0] */

/* **********************************
 * VENC to TCON sync delay
 * ********************************** */
#define RGB_DELAY                   13
#define PRE_DE_DELAY                8

#define LCD_PINMUX_END          0xff
#define LCD_PINMUX_NUM          22

/* **********************************
 * global control define
 * ********************************** */
enum lcd_mode_e {
	LCD_MODE_TV = 0,
	LCD_MODE_TABLET,
	LCD_MODE_MAX,
};

enum lcd_chip_e {
	LCD_CHIP_G12A = 0,
	LCD_CHIP_G12B, 	/* 1 */
	LCD_CHIP_TL1,
	LCD_CHIP_SM1,
	LCD_CHIP_TM2,
	LCD_CHIP_T5,
	LCD_CHIP_T5D,
	LCD_CHIP_T7,
	LCD_CHIP_T3,
	LCD_CHIP_C3,
	LCD_CHIP_T5W,
	LCD_CHIP_T5M,
	LCD_CHIP_T3X,
	LCD_CHIP_A4,
	LCD_CHIP_TXHD2,
	LCD_CHIP_S6,
	LCD_CHIP_T6D,
	LCD_CHIP_T6W,
	LCD_CHIP_T6X,
	LCD_CHIP_MAX,
};

enum lcd_type_e {
	LCD_RGB = 0,
	LCD_LVDS,
	LCD_VBYONE,
	LCD_MIPI,
	LCD_MLVDS,
	LCD_P2P,
	LCD_EDP,
	LCD_BT656,
	LCD_BT1120,
	LCD_TYPE_MAX,
};

#define MOD_LEN_MAX        48
struct lcd_basic_s {
	char model_name[MOD_LEN_MAX];
	enum lcd_type_e lcd_type;
	unsigned char config_check;

	unsigned short screen_width;  /* screen physical width in "mm" unit */
	unsigned short screen_height; /* screen physical height in "mm" unit */
};

#define LCD_CLK_FRAC_UPDATE     BIT(0)
#define LCD_CLK_PLL_CHANGE      BIT(1)
#define LCD_CLK_PLL_RESET       BIT(2)
#define LCD_MAX_NUM_TIMINGS     15

struct lcd_timing_s {
	struct lcd_detail_timing_s *dft_timing; //panel parameter probe stage
	struct lcd_detail_timing_s *base_timing; //panel parameter init stage
	struct lcd_detail_timing_s act_timing; //panel parameter actual stage
	struct lcd_detail_timing_s *timings[LCD_MAX_NUM_TIMINGS];
	unsigned char num_timings;

	unsigned char pll_flag;
	unsigned char ppc;
	unsigned char clk_change; /* internal used */
	unsigned char ss_level;
	unsigned char ss_freq;
	unsigned char ss_mode;

	unsigned int enc_clk;
	unsigned long long bit_rate; /* Hz */

	unsigned int pll_ctrl;  /* pll settings */
	unsigned int div_ctrl;  /* divider settings */
	unsigned int clk_ctrl;  /* clock settings */
	unsigned int pll_ctrl2;  /* pll settings */
	unsigned int div_ctrl2;  /* divider settings */
	unsigned int clk_ctrl2;  /* clock settings */

	unsigned int hstart;
	unsigned int hend;
	unsigned int vstart;
	unsigned int vend;
	unsigned char pre_de_h;
	unsigned char pre_de_v;

	unsigned short de_hs_addr;
	unsigned short de_he_addr;
	unsigned short de_vs_addr;
	unsigned short de_ve_addr;

	unsigned short hs_hs_addr;
	unsigned short hs_he_addr;
	unsigned short hs_vs_addr;
	unsigned short hs_ve_addr;

	unsigned short vs_hs_addr;
	unsigned short vs_he_addr;
	unsigned short vs_vs_addr;
	unsigned short vs_ve_addr;

	unsigned short pre_h_de_start;
	unsigned short pre_h_de_end;
	unsigned short pre_v_de_start;
	unsigned short pre_v_de_end;
	unsigned short pre_hso_start;
	unsigned short pre_hso_end;
	unsigned short pre_vso_hstart;
	unsigned short pre_vso_hend;
	unsigned short pre_vso_start;
	unsigned short pre_vso_end;
};

struct lcd_disp_tmg_req_s {
	unsigned int alert_level;//0:disable, 1:warning, 2:fatal err
	unsigned int hswbp_vid;
	unsigned int hfp_vid;
	unsigned int vswbp_vid;
	unsigned int vfp_vid;
};

struct rgb_config_s {
	unsigned int type;
	unsigned int clk_pol;
	unsigned int de_valid;
	unsigned int sync_valid;
	unsigned int rb_swap;
	unsigned int bit_swap;
};

struct bt_config_s {
	unsigned int clk_phase;
	unsigned int field_type;
	unsigned int mode_422;
	unsigned int yc_swap;
	unsigned int cbcr_swap;
};

#define LVDS_PHY_VSWING_DFT        3
#define LVDS_PHY_PREEM_DFT         0
#define LVDS_PHY_CLK_VSWING_DFT    0
#define LVDS_PHY_CLK_PREEM_DFT     0
struct lvds_config_s {
	unsigned int lvds_repack;
	unsigned int dual_port;
	unsigned int pn_swap;
	unsigned int port_swap;
	unsigned int lane_reverse;
	unsigned int port_sel;
	unsigned int phy_vswing;
	unsigned int phy_preem;
	unsigned int phy_clk_vswing;
	unsigned int phy_clk_preem;
};

#define VX1_PHY_VSWING_DFT           3
#define VX1_PHY_PREEM_DFT            0

#define VX1_PWR_ON_RESET_DLY_DFT     500 /* 500ms */
#define VX1_HPD_DATA_DELAY_DFT       10 /* 10ms */
#define VX1_CDR_TRAINING_HOLD_DFT    200 /* 200ms */

struct vbyone_config_s {
	unsigned int lane_count;
	unsigned int region_num;
	unsigned int byte_mode;
	unsigned int color_fmt;
	unsigned int phy_div;
	unsigned int phy_vswing; /*[5:4]:ext_pullup, [3:0]vswing*/
	unsigned int phy_preem;
	unsigned int ctrl_flag;
		/* bit[0]:power_on_reset_en
		 * bit[1]:hpd_data_delay_en
		 * bit[2]:cdr_training_hold_en
		 * bit[3]:hw_filter_en
		 */

	/* ctrl timing */
	unsigned int power_on_reset_delay; /* ms */
	unsigned int hpd_data_delay; /* ms */
	unsigned int cdr_training_hold; /* ms */
	/* hw filter */
	unsigned int hw_filter_time;
	unsigned int hw_filter_cnt;

	unsigned int slice;
};

/* mipi-dsi config */
/* Operation mode parameters */
#define OPERATION_VIDEO_MODE     0
#define OPERATION_COMMAND_MODE   1

#define SYNC_PULSE               0x0
#define SYNC_EVENT               0x1
#define BURST_MODE               0x2

/* command config */
#define DSI_CMD_SIZE_INDEX       1  /* byte[1] */
#define DSI_GPIO_INDEX           2  /* byte[2] */

#define DSI_INIT_ON_MAX          3600
#define DSI_INIT_OFF_MAX         400
#define DSI_SUSPEND_MAX          400
#define DSI_RESUME_MAX           400
struct dsi_dphy_s {
	unsigned int t_ui;

	uint16_t lp_tesc[2];
	uint16_t lp_lpx[2];
	uint16_t lp_ta_sure[2];
	uint16_t lp_ta_go[2];
	uint16_t lp_ta_get[2];
	uint16_t hs_exit[2];
	uint16_t hs_trail[2];
	uint16_t hs_zero[2];
	uint16_t hs_prepare[2];
	uint16_t clk_trail[2];
	uint16_t clk_post[2];
	uint16_t clk_zero[2];
	uint16_t clk_prepare[2];

	uint16_t clk_pre[2];

	uint16_t phy_hs2lp_time[2];
	uint16_t phy_lp2hs_time[2];
	uint16_t max_rd_time[2];

	unsigned int init;
	unsigned int wakeup;
};

struct dsi_panel_det_attr_s {
	unsigned char *det_init_table[8];
	unsigned char *det_match_seq[8];
	char *det_type[8];
	char *fallback_type;
};

struct dsi_config_s {
	/* user config */
	unsigned char lane_num;
	unsigned char deskew_en;
	unsigned short deskew_initial_time;
	unsigned short deskew_periodic_time;
	unsigned int bit_rate_target; /* MHz */
	unsigned char multi_port_cfg; /* bit[0:3]*/
	unsigned char operation_mode_init; /* 0=video mode, 1=command mode */
	unsigned char operation_mode_display; /* 0=video mode, 1=command mode */
	unsigned char video_mode_type; /* 0=sync_pulse, 1=sync_event, 2=burst */
	unsigned char clk_always_hs; /* 0=disable, 1=enable */

	unsigned int factor_numerator;
	unsigned int factor_denominator;
	unsigned int lane_byte_clk;
	unsigned int tear_en_ctrl; // 0=dis, 1=dcs, 2=gpio

	/* non_burst vid packet */
	unsigned int user_pkt_size;
	unsigned int vid_num_chunks;
	unsigned int vid_null_size;
	unsigned int vid_pkt_size;

	/* vid timing */
	unsigned int hline;
	unsigned int hsa;
	unsigned int hbp;

	unsigned int venc_data_width;
	unsigned int dpi_data_format;

	unsigned char *dsi_init_on;
	unsigned char *dsi_init_off;
	unsigned char *dsi_suspend;
	unsigned char *dsi_resume;
	unsigned char extern_init;

	unsigned char dsi_rd_n;
	struct dsi_dphy_s dphy;

	//dsi_panel_check.c
	unsigned char check_en;
	unsigned char check_reg;
	unsigned char check_cnt;
	unsigned char check_state;
	//dsi_panel_detect.c
	char matched_panel[20];
	char *dt_addr;
	unsigned char panel_det_attr; //[0]:det_en, [1]:store2env, [2]:0=bsp/1=dts [3]:on_matched
};

struct mlvds_config_s {
	unsigned int channel_num;
	unsigned int channel_sel0;
	unsigned int channel_sel1;
	unsigned int clk_phase; /* [14:13]=clk01_sel,
				 * [12]=data bypass buffer
				 * [11:8]=pi2, [7:4]=pi1, [3:0]=pi0
				 */
	unsigned int pn_swap;
	unsigned int bit_swap; /* MSB/LSB reverse */
	unsigned int phy_vswing;
	unsigned int phy_preem;
};

enum p2p_type_e {
	P2P_CEDS = 0,
	P2P_CMPI,
	P2P_ISP,
	P2P_EPI,
	P2P_CHPI = 0x10, /* low common mode */
	P2P_CSPI,
	P2P_USIT,
	P2P_CSPI_NEW,  /* CSPI New Coding */
	P2P_MAX,
};

struct p2p_config_s {
	unsigned int p2p_type;
	unsigned int lane_num;
	unsigned int channel_sel0;
	unsigned int channel_sel1;
	unsigned int pn_swap;
	unsigned int bit_swap; /* MSB/LSB reverse */
	unsigned int phy_vswing;
	unsigned int phy_preem;

	// cspi new coding: adjust phy bitrate using alpha value
	unsigned int cspi_alpha;
};

#define PHY_BIT_VSWING      BIT(0)
#define PHY_BIT_VCM         BIT(1)
#define PHY_BIT_REF_BIAS    BIT(2)
#define PHY_BIT_ODT         BIT(3)
#define PHY_BIT_CV_MODE     BIT(4)
#define PHY_BIT_LANE_PREEM  BIT(12)
#define PHY_BIT_LANE_AMP    BIT(13)
#define PHY_BIT_LANE_RTERM  BIT(14)
#define PHY_BIT_LANE_SEL    BIT(15)

#define PHY_PHASE_0 0
#define PHY_PHASE_A 1
#define PHY_PHASE_B 2

struct phy_lane_s {
	unsigned int preem; //flag bit[12]
	unsigned int amp; //flag bit[13]
	unsigned int rterm;
};

struct ss_config_s {
	unsigned char level;
	unsigned char freq;
	unsigned char mode;
};

#define PHY_CMODE   0
#define PHY_VMODE   1
#define CH_LANE_MAX 32
#define MAX_NUM_PHY_CFGS 6
struct phy_attr_s {
	unsigned int vswing;     //flag bit[0]
	unsigned int vcm;        //flag bit[1]
	unsigned int ref_bias;   //flag bit[2]
	unsigned int odt;        //flag bit[3]
	unsigned int cv_mode;    //flag bit[4] //val:0=cm,1=vm
	unsigned short phy_clk;
	unsigned short phy_clk_min;
	unsigned short phy_clk_max;
	unsigned short clk_phase;
	struct phy_lane_s lane[CH_LANE_MAX];
	struct ss_config_s ss;
};

struct phy_ch_ctrl_s {
	unsigned char phase_sel;
	unsigned char pn_swap;
	unsigned char sel;
	unsigned char sel_dft;
	unsigned char en;
};

struct phy_config_s {
	unsigned short lane_num;
	unsigned short group_num;
	unsigned int vswing_level;
	unsigned int ext_pullup;
	unsigned int preem_level;
	unsigned char weakly_pull_down;
	unsigned char low_common_mode;
	unsigned char ch_phase_en; //for minilvds
	unsigned char ch_clk_inv; //for lvds

	unsigned int bypass_resample;
	unsigned int state;
	unsigned int flag;
	unsigned int ckdi;
	unsigned int ch_swap0; //for reg
	unsigned int ch_swap1; //for reg
	unsigned int lane_valid; //(valid & mask)<<offset
	unsigned int lane_offset;
	unsigned int lane_mask; //local mask for lane range

	struct phy_ch_ctrl_s ch_ctrl[CH_LANE_MAX];
	struct phy_attr_s *act_phy;
	struct phy_attr_s *phys[MAX_NUM_PHY_CFGS];
};

enum lcd_phy_set_status {
	LCD_PHY_OFF = 0,
	LCD_PHY_ON,
	LCD_PHY_PWR_UP,
	LCD_PHY_PWR_DOWN,
	LCD_PHY_UPDATE,
	LCD_PHY_LOCK_LANE,
};

union lcd_ctrl_config_u {
	struct rgb_config_s rgb_cfg;
	struct bt_config_s bt_cfg;
	struct lvds_config_s lvds_cfg;
	struct vbyone_config_s vbyone_cfg;
	struct dsi_config_s mipi_cfg;
	struct mlvds_config_s mlvds_cfg;
	struct p2p_config_s p2p_cfg;
};

/* **********************************
 * power control define
 * ********************************** */
enum lcd_power_type_e {
	LCD_POWER_TYPE_GPIO = 0,
	LCD_POWER_TYPE_PMU,                 /* 1 */
	LCD_POWER_TYPE_SIGNAL,              /* 2 */
	LCD_POWER_TYPE_EXTERN,              /* 3 */
	LCD_POWER_TYPE_WAIT_GPIO,           /* 4 */
	LCD_POWER_TYPE_CLK_SS,              /* 5 */
	LCD_POWER_TYPE_TCON_SPI_DATA_LOAD,  /* 6 */
	LCD_POWER_TYPE_BACKLIGHT,           /* 7 */
	LCD_POWER_TYPE_MUTE,                /* 8 */
	LCD_POWER_TYPE_OFF_DELAY = 0xfd,
	LCD_POWER_TYPE_MAX,
};
#define LCD_POWER_TYPE_CPU           LCD_POWER_TYPE_GPIO
#define LCD_POWER_TYPE_END           0xff

enum lcd_pmu_gpio_e {
	LCD_PMU_GPIO0 = 0,
	LCD_PMU_GPIO1,
	LCD_PMU_GPIO2,
	LCD_PMU_GPIO3,
	LCD_PMU_GPIO4,
	LCD_PMU_GPIO_MAX,
};

#define LCD_CLK_SS_BIT_FREQ             0
#define LCD_CLK_SS_BIT_MODE             4

#define LCD_GPIO_MAX                    0xff
#define LCD_GPIO_OUTPUT_LOW             0
#define LCD_GPIO_OUTPUT_HIGH            1
#define LCD_GPIO_INPUT                  2

/* Power Control */
#define LCD_CPU_GPIO_NUM_MAX         10
#define LCD_CPU_GPIO_NAME_MAX        15
#define LCD_PMU_GPIO_NUM_MAX         3

#define LCD_PWR_STEP_MAX             15
struct lcd_power_step_s {
	unsigned char type;
	int index; /* point to lcd_cpu_gpio_s or lcd_pmu_gpio_s or lcd_extern */
	unsigned short value;
	unsigned short delay;
};

struct lcd_power_ctrl_s {
	char cpu_gpio[LCD_CPU_GPIO_NUM_MAX][LCD_CPU_GPIO_NAME_MAX];
	int *pmu_gpio;
	struct lcd_power_step_s power_on_step[LCD_PWR_STEP_MAX];
	struct lcd_power_step_s power_off_step[LCD_PWR_STEP_MAX];
	int power_on_step_max; /*  internal use for debug */
	int power_off_step_max; /* internal use for debug */
};

#define LCD_PINMX_MAX              10
#define LCD_PINMUX_MAX             LCD_PINMX_MAX
#define BL_PINMUX_MAX              8
#define LCD_PINMUX_NAME_LEN_MAX    30

struct cus_ctrl_config_s {
	unsigned int ctrl_en;
	unsigned int ctrl_cnt;
	unsigned int timing_cnt;
	unsigned int active_timing_type;
	unsigned char timing_switch_flag;
	unsigned char timing_ctrl_valid;

	struct lcd_cus_ctrl_attr_config_s *attr_config;
	struct lcd_cus_ctrl_attr_config_s *cur_timing_attr;
};

#define LCD_ENABLE_RETRY_MAX    3
#define CUS_PINMUX_NAME_MAX	44
#define CUS_PINMUX_MODE_MODEL_NAME   1
#define CUS_PINMUX_MODE_CUSTOM_NAME  2

struct lcd_config_s {
	unsigned char retry_enable_flag;
	unsigned char retry_enable_cnt;
	char cus_pinmux_name[CUS_PINMUX_NAME_MAX];
	unsigned char fr_auto_cus;  //0=follow global setting, 0xff=disable
	unsigned char fr_auto_flag; //final fr_auto policy
	unsigned int backlight_index;

	struct lcd_basic_s basic;
	struct lcd_timing_s timing;
	union lcd_ctrl_config_u control;
	struct phy_config_s phy_cfg;
	struct cus_ctrl_config_s cus_ctrl;
	struct lcd_power_ctrl_s power;
	unsigned int pinmux_flag;
};

#define LCD_DURATION_MAX    8
struct lcd_duration_s {
	unsigned int frame_rate;
	unsigned int duration_num;
	unsigned int duration_den;
	unsigned int frac;
};

struct lcd_vmode_info_s {
	char name[32];
	unsigned int width;
	unsigned int height;
	unsigned int base_fr;
	unsigned int duration_index;
	unsigned int duration_cnt;
	struct lcd_duration_s duration[LCD_DURATION_MAX];
	struct lcd_detail_timing_s *dft_timing;
};

struct lcd_vmode_list_s {
	struct lcd_vmode_info_s *info;
	struct lcd_vmode_list_s *next;
};

struct lcd_vmode_mgr_s {
	unsigned int vmode_cnt;
	struct lcd_vmode_list_s *vmode_list_header;
	struct lcd_vmode_info_s *cur_vmode_info;
	struct lcd_vmode_info_s *next_vmode_info;
};

#define LCD_INIT_LEVEL_NORMAL         0
#define LCD_INIT_LEVEL_PWR_OFF        1
#define LCD_INIT_LEVEL_KERNEL_ON      2
#define LCD_INIT_LEVEL_KERNEL_OFF     3
#define LCD_INIT_LEVEL_PREBOOT        4  // lcd on, bl off

#define LCD_VENC_1PPC                 0
#define LCD_VENC_2PPC                 1
#define LCD_VENC_4PPC                 2

/*
 * bit[31:24] : frame_rate bit[7:0]
 * bit[23:22] : clk_mode
 * bit[21:20] : ppc
 * bit[19:18] : lcd_init_level
 * bit[17] : dccd flag
 * bit[16] : frame_rate bit[8]
 * bit[15:8] : advanced flag(p2p_type when lcd_type=p2p)
 * bit[7:4] : lcd bits
 * bit[3:0] : lcd_type
 */
struct lcd_boot_ctrl_s {
	unsigned char lcd_type;
	unsigned char lcd_bits;
	unsigned char dccd_flag;
	unsigned char mute_flag;
	unsigned char ppc;
	unsigned char clk_mode;
	unsigned char reserved;
	unsigned char init_level;
	unsigned short advanced_flag;
	unsigned short frame_rate;
	unsigned char if_state;
	unsigned char bl_state;
};

/*
 *bit[31:30]: lcd mode(0=normal, 1=tv; 2=tablet, 3=TBD)
 *bit[29:28]: lcd debug para source(0=normal, 1=dts, 2=unifykey,
 *                                  3=bsp for uboot)
 *bit[27:20]: reserved
 *bit[19:16]: lcd test pattern
 *bit[15:0]:  lcd debug print flag
 */
struct lcd_debug_ctrl_s {
	unsigned short debug_print_flag;
	unsigned char debug_test_pattern;
	unsigned char debug_para_source;
	unsigned char debug_lcd_mode;
};

struct lcd_dft_config_s {
	char (*lcd_gpio)[LCD_CPU_GPIO_NAME_MAX];

	unsigned char key_valid;
	unsigned char clk_path; /* 0=default, 1=gp0_pll */
	unsigned int mode;
	struct ext_lcd_config_s *ext_lcd;

#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_common_s *ext_common;
	struct lcd_extern_config_s *ext_conf;
#endif

	char (*bl_gpio)[LCD_CPU_GPIO_NAME_MAX];
#ifdef CONFIG_AML_LCD_BL_LDIM
	char (*ldim_gpio)[LCD_CPU_GPIO_NAME_MAX];
#endif
};

struct aml_lcd_data_s {
	enum lcd_chip_e chip_type;
	const char *chip_name;
	unsigned char rev_type;
	unsigned char drv_max;
	unsigned int offset_venc[LCD_MAX_DRV];
	unsigned int offset_venc_if[LCD_MAX_DRV];
	unsigned int offset_venc_data[LCD_MAX_DRV];
	struct lcd_dft_config_s *dft_conf[LCD_MAX_DRV];
};

/* ==============lcd driver================== */
#define LCD_STATUS_IF_ON      BIT(0)
#define LCD_STATUS_ENCL_ON    BIT(1)
#define LCD_STATUS_PRE_MUTE   BIT(3)

struct aml_lcd_cma_mem {
	signed char exist;
	signed char ready;
	unsigned char *vbase;
	phys_addr_t pbase;
	phys_addr_t size;
	phys_addr_t offset;
	unsigned int page_size;
	unsigned int page_num;
	unsigned int page_pos;
	unsigned char *bitmap;
};

#define LCD_CONFIG_NONE 0
#define LCD_CONFIG_DTS  1
#define LCD_CONFIG_UKEY 2
#define LCD_CONFIG_FILE 3
#define LCD_CONFIG_BSP  4
#define LCD_CONFIG_ERR  0xff

struct aml_lcd_drv_s {
	unsigned int index;
	unsigned int status;
	unsigned char mode;
	unsigned char key_valid;
	unsigned char config_load;
	unsigned char probe_done;
	unsigned char clk_path; /* 0=hpll, 1=gp0_pll */
	char init_mode[64];
	unsigned int power_on_suspend;
	unsigned char config_check_glb;
	unsigned char config_check_en;
	unsigned char viu_sel; // 1=vout; 2=vou2; 3=vout3

	struct lcd_config_s config;
	struct lcd_vmode_mgr_s vmode_mgr;
	struct lcd_disp_tmg_req_s disp_req;
	struct aml_lcd_data_s *data;
	struct lcd_boot_ctrl_s boot_ctrl;
	struct lcd_duration_s *std_duration;
	void *clk_conf;

	int  (*outputmode_check)(struct aml_lcd_drv_s *pdrv, char *mode);
	int  (*config_valid)(struct aml_lcd_drv_s *pdrv, char *mode);
	void (*driver_init_pre)(struct aml_lcd_drv_s *pdrv);
	void  (*driver_init)(struct aml_lcd_drv_s *pdrv);
	void (*driver_disable)(struct aml_lcd_drv_s *pdrv);
	void (*list_support_mode)(struct aml_lcd_drv_s *pdrv);
#ifdef CONFIG_AML_LCD_TCON
	void (*tcon_reg_print)(void);
	void (*tcon_table_print)(void);
	void (*tcon_data_print)(unsigned char index);
	void (*tcon_spi_print)(void);
	int (*tcon_spi_data_load)(struct aml_lcd_drv_s *pdrv);
	unsigned int (*tcon_reg_read)(unsigned int addr, unsigned int flag);
	void (*tcon_reg_write)(unsigned int addr, unsigned int val,
			       unsigned int flag);
	unsigned int (*tcon_table_read)(unsigned int addr);
	unsigned int (*tcon_table_write)(unsigned int addr, unsigned int val);
	int (*tcon_mem_tee_protect)(int protect_en);
	int (*tcon_forbidden_check)(void);
	void (*tcon_global_reset)(struct aml_lcd_drv_s *pdrv);
#endif
	void *debug_info;
	void (*phy_set)(struct aml_lcd_drv_s *pdrv, int status);
	struct dev_pm_ops *dev_pm_ops;
	struct udevice *dev;
	/* for factory test */
	struct lcd_power_step_s *factory_lcd_power_on_step;
};

extern void lcd_config_bsp_init(void);

struct aml_lcd_data_s *aml_lcd_get_data(void);
struct aml_lcd_drv_s *aml_lcd_get_driver(int index);

char *lcd_get_dt_addr(void);
void lcd_set_dt_addr(char *dt_addr);

/* global api for cmd */
int aml_lcd_driver_probe(int index);
void aml_lcd_driver_list_support_mode(void);
unsigned int aml_lcd_driver_outputmode_check(unsigned char lcd_idx, char *mode);
void aml_lcd_driver_prepare(int index, char *mode);
void aml_lcd_driver_enable(int index, char *mode);
void aml_lcd_driver_disable(int index);
void aml_lcd_driver_power_ctrl(int index, int status);
void aml_lcd_driver_set_ss(int index, unsigned int level, unsigned int freq, unsigned int mode);
void aml_lcd_driver_get_ss(int index);
void aml_lcd_driver_clk_info(int index);
void aml_lcd_driver_debug_print(int index, unsigned int val);
void aml_lcd_driver_info(int index);
void aml_lcd_driver_reg_info(int index);
void aml_lcd_vbyone_rst(int index);
int aml_lcd_vbyone_cdr(int index);
int aml_lcd_vbyone_lock(int index);
// int aml_lcd_edp_debug(int index, char *str, int num);
void lcd_update_ctrl_bootargs(struct aml_lcd_drv_s *pdrv);

// switch MIPI DSI mode: 0:display, 1:command
void aml_lcd_mipi_dsi_mode(int index, unsigned char mode);
void aml_lcd_mipi_dsi_dphy_test(int index, unsigned char mode);
// exec DSI command, payload:[0]:data_type, [1]:number of leftover cmd, [2+]: command
void aml_lcd_mipi_dsi_cmd(int index, unsigned char *payload);
// read DSI command
// @payload format as above
// @rd_data is space to store read back data
// @rd_byte_len is max size of rd_data, data read over this size will not be stored
// return actual read out size
int aml_lcd_mipi_dsi_read(int index,
		unsigned char *payload, unsigned char *rd_data, unsigned char rd_byte_len);
void aml_lcd_driver_test(int index, int num);
int aml_lcd_driver_prbs(int index, unsigned int s, unsigned int prbs_freq, unsigned int mode_flag);
void aml_lcd_driver_mute(int index, unsigned char flag);
void aml_lcd_panel_dump(int index, const char *path);
void aml_lcd_param_mem_dump(void);
void aml_lcd_config_check(int index);

void aml_lcd_driver_ext_info(int index);
void aml_lcd_driver_ext_power_on(int index);
void aml_lcd_driver_ext_power_off(int index);

void aml_lcd_driver_bl_on(int index);
void aml_lcd_driver_bl_off(int index);
void aml_lcd_driver_set_bl_level(int index, int level);
unsigned int aml_lcd_driver_get_bl_level(int index);
void aml_lcd_driver_bl_config_print(int index);

void aml_lcd_list(void);
void aml_lcd_set(uint8_t drv_idx, char *lcd_type_name);


#endif /* INC_AML_LCD_VOUT_H */
