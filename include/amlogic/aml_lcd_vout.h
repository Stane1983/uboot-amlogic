/*
 * include/amlogic/aml_lcd_vout.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef INC_AML_LCD_VOUT_H
#define INC_AML_LCD_VOUT_H

#include <common.h>
#include <linux/list.h>
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/aml_lcd_extern.h>
#endif

/* **********************************
 * debug print define
 * ********************************** */
//#define LCD_DEBUG_INFO
extern unsigned int lcd_debug_print_flag;

#define LCDPR(fmt, args...)     printf("lcd: "fmt"", ## args)
#define LCDERR(fmt, args...)    printf("lcd: error: "fmt"", ## args)

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
#define DIV_CTRL_DIV_POST           12 /* [14:12] */
#define DIV_CTRL_DIV_PRE            8 /* [10:8] */
#define DIV_CTRL_DIV_SEL            8 /* [15:8] */
#define DIV_CTRL_XD                 0 /* [7:0] */

/* ******** clk_ctrl ******** */
#define CLK_CTRL_LEVEL              12 /* [14:12] */
#define CLK_CTRL_FRAC               0  /* [11:0] */

/* **********************************
 * other parameter bit define
 * pol_ctrl, gamma_ctrl
 * ********************************** */
/* pol_ctrl */
#define POL_CTRL_CLK                3
#define POL_CTRL_DE                 2
#define POL_CTRL_VS                 1
#define POL_CTRL_HS                 0

/* gamma_ctrl */
#define GAMMA_CTRL_REV              1
#define GAMMA_CTRL_EN               0

#define LCD_PINMUX_END              0xff
#define LCD_PINMUX_NUM              15

/* **********************************
 * VENC to TCON sync delay
 * ********************************** */
#define TTL_DELAY                   13


/* ******** AXG ******** */
/* bit[15:11] */
#define BIT_PHY_LANE_AXG        11
#define PHY_LANE_WIDTH_AXG       5

/* MIPI-DSI */
#define DSI_LANE_0              (1 << 4)
#define DSI_LANE_1              (1 << 3)
#define DSI_LANE_CLK            (1 << 2)
#define DSI_LANE_2              (1 << 1)
#define DSI_LANE_3              (1 << 0)
#define DSI_LANE_COUNT_1        (DSI_LANE_CLK | DSI_LANE_0)
#define DSI_LANE_COUNT_2        (DSI_LANE_CLK | DSI_LANE_0 | DSI_LANE_1)
#define DSI_LANE_COUNT_3        (DSI_LANE_CLK | DSI_LANE_0 |\
					DSI_LANE_1 | DSI_LANE_2)
#define DSI_LANE_COUNT_4        (DSI_LANE_CLK | DSI_LANE_0 |\
					DSI_LANE_1 | DSI_LANE_2 | DSI_LANE_3)


/* **********************************
 * global control define
 * ********************************** */
enum lcd_mode_e {
	LCD_MODE_TV = 0,
	LCD_MODE_TABLET,
	LCD_MODE_MAX,
};

enum lcd_chip_e {
	LCD_CHIP_GXTVBB = 0,
	LCD_CHIP_GXL,     	/* 1 */
	LCD_CHIP_GXM,     	/* 2 */
	LCD_CHIP_TXL,     	/* 3 */
	LCD_CHIP_TXLX,    	/* 4 */
	LCD_CHIP_AXG, 		/* 5 */
	LCD_CHIP_MAX,
};

enum lcd_type_e {
	LCD_TTL = 0,
	LCD_LVDS,
	LCD_VBYONE,
	LCD_MIPI,
	LCD_TYPE_MAX,
};

#define MOD_LEN_MAX        30
struct lcd_basic_s {
	char model_name[MOD_LEN_MAX];
	enum lcd_type_e lcd_type;
	unsigned char lcd_bits;

	unsigned short h_active; /* Horizontal display area */
	unsigned short v_active; /* Vertical display area */
	unsigned short h_period; /* Horizontal total period time */
	unsigned short v_period; /* Vertical total period time */
	unsigned short h_period_min;
	unsigned short h_period_max;
	unsigned short v_period_min;
	unsigned short v_period_max;
	unsigned int lcd_clk_min;
	unsigned int lcd_clk_max;

	unsigned short screen_width;  /* screen physical width in "mm" unit */
	unsigned short screen_height; /* screen physical height in "mm" unit */
};

#define LCD_CLK_FRAC_UPDATE     (1 << 0)
#define LCD_CLK_PLL_CHANGE      (1 << 1)
struct lcd_timing_s {
	unsigned char clk_auto; /* clk parameters auto generation */
	unsigned int lcd_clk;   /* pixel clock(unit: Hz) */
	unsigned int lcd_clk_dft; /* internal used */
	unsigned int h_period_dft; /* internal used */
	unsigned int v_period_dft; /* internal used */
	unsigned char clk_change; /* internal used */
	unsigned int pll_ctrl;  /* pll settings */
	unsigned int div_ctrl;  /* divider settings */
	unsigned int clk_ctrl;  /* clock settings */

	unsigned char fr_adjust_type; /* 0=clock, 1=htotal, 2=vtotal */
	unsigned char ss_level;
	/* unsigned int pol_ctrl; */

	unsigned short sync_duration_num;
	unsigned short sync_duration_den;

	unsigned short video_on_pixel;
	unsigned short video_on_line;

	unsigned short hsync_width;
	unsigned short hsync_bp;
	unsigned short hsync_pol;
	unsigned short vsync_width;
	unsigned short vsync_bp;
	unsigned short vsync_pol;
	/* unsigned int vsync_h_phase; // [31]sign, [15:0]value */
	unsigned int h_offset;
	unsigned int v_offset;

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
};
/*
struct lcd_effect_s {
	unsigned int rgb_base_addr;
	unsigned int rgb_coeff_addr;
	unsigned char dith_user;
	unsigned int dith_ctrl;

	unsigned char gamma_ctrl;
	unsigned short gamma_r_coeff;
	unsigned short gamma_g_coeff;
	unsigned short gamma_b_coeff;
	unsigned short GammaTableR[256];
	unsigned short GammaTableG[256];
	unsigned short GammaTableB[256];
};
*/
struct ttl_config_s {
	unsigned int clk_pol;
	unsigned int sync_valid; /* [1]DE, [0]hvsync */
	unsigned int swap_ctrl; /* [1]rb swap, [0]bit swap */
};

#define LVDS_PHY_VSWING_DFT        3
#define LVDS_PHY_PREEM_DFT         0
#define LVDS_PHY_CLK_VSWING_DFT    0
#define LVDS_PHY_CLK_PREEM_DFT     0
struct lvds_config_s {
	unsigned int lvds_vswing;
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

#define VX1_SW_FILTER_TIME_DFT       10 /* 10*100us=1ms */
#define VX1_SW_FILTER_CNT_DFT        6
#define VX1_SW_FILTER_RETRY_CNT_DFT  2
#define VX1_SW_FILTER_RETRY_DLY_DFT  100 /* 100ms */
#define VX1_SW_CDR_DET_TIME_DFT      20 /* 20us*100=2ms */
#define VX1_SW_CDR_DET_CNT_DFT       100
#define VX1_SW_CDR_TIMEOUT_CNT_DFT   100

struct vbyone_config_s {
	unsigned int lane_count;
	unsigned int region_num;
	unsigned int byte_mode;
	unsigned int color_fmt;
	unsigned int phy_div;
	unsigned int bit_rate;
	unsigned int phy_vswing; /*[4]:ext_pullup, [3:0]vswing*/
	unsigned int phy_preem;
	unsigned int ctrl_flag;
		/* bit[0]:power_on_reset_en
		   bit[1]:hpd_data_delay_en
		   bit[2]:cdr_training_hold_en
		   bit[3]:hw_filter_en
		   bit[5:4]:sw_filter */

	/* ctrl timing */
	unsigned int power_on_reset_delay; /* ms */
	unsigned int hpd_data_delay; /* ms */
	unsigned int cdr_training_hold; /* ms */
	/* hw filter */
	unsigned int hpd_hw_filter_time; /* ms */
	unsigned int hpd_hw_filter_cnt;
	unsigned int lockn_hw_filter_time; /* ms */
	unsigned int lockn_hw_filter_cnt;
	/* sw filter */
	unsigned int vx1_sw_filter_en; /* 0=disable, 1=sw_filter, 2=sw_filter with sw_reset */
	unsigned int vx1_sw_filter_time; /* 100us base */
	unsigned int vx1_sw_filter_cnt;
	unsigned int vx1_sw_filter_retry_cnt;
	unsigned int vx1_sw_filter_retry_delay; /* ms base */
	unsigned int vx1_sw_cdr_detect_time; /* us base * 100 times, must cover tcon lockn pulse */
	unsigned int vx1_sw_cdr_detect_cnt;
	unsigned int vx1_sw_cdr_timeout_cnt;
};

/* mipi-dsi config */
/* Operation mode parameters */
#define OPERATION_VIDEO_MODE     0
#define OPERATION_COMMAND_MODE   1

#define SYNC_PULSE               0x0
#define SYNC_EVENT               0x1
#define BURST_MODE               0x2

/* command config */
#define DSI_CMD_INDEX            1  /* byte[1] */

#define DSI_INIT_ON_MAX          100
#define DSI_INIT_OFF_MAX         30

struct dsi_config_s {
	unsigned char lane_num;
	unsigned int bit_rate_max; /* MHz */
	unsigned int bit_rate_min; /* MHz*/
	unsigned int bit_rate; /* Hz */
	unsigned int clk_factor; /* bit_rate/pclk */
	unsigned int factor_numerator;
	unsigned int factor_denominator; /* 100 */
	unsigned char operation_mode_init; /* 0=video mode, 1=command mode */
	unsigned char operation_mode_display; /* 0=video mode, 1=command mode */
	unsigned char video_mode_type; /* 0=sync_pulse, 1=sync_event, 2=burst */
	unsigned char clk_lp_continuous; /* 0=stop, 1=continue */
	unsigned char phy_stop_wait; /* 0=auto, 1=standard, 2=slow */

	unsigned int venc_data_width;
	unsigned int dpi_data_format;
	unsigned int venc_fmt;

	unsigned char *dsi_init_on;
	unsigned char *dsi_init_off;
	unsigned char extern_init;
};

struct lcd_ctrl_config_s {
	struct ttl_config_s *ttl_config;
	struct lvds_config_s *lvds_config;
	struct vbyone_config_s *vbyone_config;
	struct dsi_config_s *mipi_config;
};

/* **********************************
 * power control define
 * ********************************** */
enum lcd_power_type_e {
	LCD_POWER_TYPE_CPU = 0,
	LCD_POWER_TYPE_PMU,
	LCD_POWER_TYPE_SIGNAL,
	LCD_POWER_TYPE_EXTERN,
	LCD_POWER_TYPE_MAX,
};

enum lcd_pmu_gpio_e {
	LCD_PMU_GPIO0 = 0,
	LCD_PMU_GPIO1,
	LCD_PMU_GPIO2,
	LCD_PMU_GPIO3,
	LCD_PMU_GPIO4,
	LCD_PMU_GPIO_MAX,
};

#define LCD_GPIO_MAX                    0xff
#define LCD_GPIO_OUTPUT_LOW             0
#define LCD_GPIO_OUTPUT_HIGH            1
#define LCD_GPIO_INPUT                  2

/* Power Control */
#define LCD_CPU_GPIO_NUM_MAX         10
#define LCD_CPU_GPIO_NAME_MAX        10
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
};

struct lcd_config_s {
	unsigned char lcd_mode;
	unsigned char lcd_key_valid;
	unsigned int backlight_index;
	struct lcd_basic_s lcd_basic;
	struct lcd_timing_s lcd_timing;
	/*struct lcd_effect_s lcd_effect;*/
	struct lcd_ctrl_config_s lcd_control;
	struct lcd_power_ctrl_s *lcd_power;
	unsigned int pinmux_set[LCD_PINMUX_NUM][2];
	unsigned int pinmux_clr[LCD_PINMUX_NUM][2];
};

extern struct lcd_config_s lcd_config_dft;

/* ==============backlight control config================== */
enum bl_ctrl_method_e {
	BL_CTRL_GPIO = 0,
	BL_CTRL_PWM,
	BL_CTRL_PWM_COMBO,
	BL_CTRL_LOCAL_DIMING,
	BL_CTRL_EXTERN,
	BL_CTRL_MAX,
};

enum bl_pwm_method_e {
	BL_PWM_NEGATIVE = 0,
	BL_PWM_POSITIVE,
	BL_PWM_METHOD_MAX,
};

enum bl_pwm_port_e {
	BL_PWM_A = 0,
	BL_PWM_B,
	BL_PWM_C,
	BL_PWM_D,
	BL_PWM_E,
	BL_PWM_F,
	BL_PWM_VS,
	BL_PWM_MAX,
};

enum bl_off_policy_e {
	BL_OFF_POLICY_NONE = 0,
	BL_OFF_POLICY_ALWAYS,
	BL_OFF_POLICY_ONCE,
	BL_OFF_POLICY_MAX,
};

#define XTAL_FREQ_HZ		(24*1000*1000) /* 24M in HZ */
#define XTAL_HALF_FREQ_HZ	(24*1000*500)  /* 24M/2 in HZ */

#define BL_FREQ_DEFAULT			1000 /* unit: HZ */
#define BL_FREQ_VS_DEFAULT		2    /* multiple 2 of vfreq */

#define BL_LEVEL_MAX		255
#define BL_LEVEL_MIN		10
#define BL_LEVEL_OFF		1

#define BL_LEVEL_MID		128
#define BL_LEVEL_MID_MAPPED	BL_LEVEL_MID //102
#define BL_LEVEL_DEFAULT	BL_LEVEL_MID

#define BL_GPIO_NUM_MAX		5

struct bl_pwm_config_s {
	unsigned int index;
	enum bl_pwm_method_e pwm_method;
	enum bl_pwm_port_e pwm_port;
	unsigned int level_max;
	unsigned int level_min;
	unsigned int pwm_freq; /* pwm_vs: 1~4(vfreq), pwm: freq(unit: Hz) */
	unsigned int pwm_duty; /* unit: % */
	unsigned int pwm_duty_max; /* unit: % */
	unsigned int pwm_duty_min; /* unit: % */
	unsigned int pwm_cnt; /* internal used for pwm control */
	unsigned int pwm_pre_div; /* internal used for pwm control */
	unsigned int pwm_max; /* internal used for pwm control */
	unsigned int pwm_min; /* internal used for pwm control */
	unsigned int pwm_level; /* internal used for pwm control */
	unsigned int pwm_gpio;
	unsigned int pwm_gpio_off;
	unsigned int pinmux_flag;
	unsigned int pinmux_set[10][2];
	unsigned int pinmux_clr[10][2];
};

struct bl_config_s {
	unsigned char bl_key_valid;
	char name[20];
	int level_default;
	int level_min;
	int level_max;
	int level_mid;
	int level_mid_mapping;
	int level;

	enum bl_ctrl_method_e method;
	unsigned int en_gpio;
	unsigned int en_gpio_on;
	unsigned int en_gpio_off;
	unsigned short power_on_delay;
	unsigned short power_off_delay;
	unsigned int dim_max;
	unsigned int dim_min;

	struct bl_pwm_config_s *bl_pwm;
	struct bl_pwm_config_s *bl_pwm_combo0;
	struct bl_pwm_config_s *bl_pwm_combo1;
	unsigned int pwm_on_delay;
	unsigned int pwm_off_delay;
	unsigned int pwm_en_sequence_reverse;

	char gpio_name[BL_GPIO_NUM_MAX][LCD_CPU_GPIO_NAME_MAX];
	//unsigned pinmux_set_num;
	unsigned int pinmux_set[10][2];
	//unsigned pinmux_clr_num;
	unsigned int pinmux_clr[10][2];
	int bl_extern_index;
};

extern struct bl_config_s bl_config_dft;

/* ==============lcd driver================== */
struct aml_lcd_drv_s {
	char version[15];
	enum lcd_chip_e chip_type;
	unsigned char lcd_status;

	struct lcd_config_s *lcd_config;
	struct bl_config_s *bl_config;

	int  (*config_check)(char *mode);
	void  (*driver_init_pre)(void);
	int  (*driver_init)(void);
	void (*driver_disable)(void);
	void (*list_support_mode)(void);
	int  (*lcd_probe)(void);
	void (*lcd_enable)(char *mode);
	void (*lcd_disable)(void);
	void (*lcd_set_ss)(int level);
	char *(*lcd_get_ss)(void);
	void (*lcd_test)(int num);
	void (*lcd_clk)(void);
	void (*lcd_info)(void);
	void (*lcd_reg)(void);
	void (*bl_on)(void);
	void (*bl_off)(void);
	void (*set_bl_level)(int level);
	int  (*get_bl_level)(void);
	void (*bl_config_print)(void);
	int unifykey_test_flag;
	void (*unifykey_test)(void);
	void (*unifykey_dump)(void);
	void (*lcd_extern_info)(void);
};

extern void lcd_config_bsp_init(void);

extern struct aml_lcd_drv_s *aml_lcd_get_driver(void);

extern int lcd_probe(void);

#endif /* INC_AML_LCD_VOUT_H */
