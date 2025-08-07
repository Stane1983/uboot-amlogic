/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __DPTX_COMMON_H_
#define __DPTX_COMMON_H_

#include <amlogic/media/vout/eDPTX/DPTX.h>
#include <div64.h>

/* 20240704: lcd tcon support user info */
#define DPTX_DRV_VERSION    "20240820"

// extern unsigned long clk_util_clk_msr(unsigned long clk_mux);

void mdelay(unsigned long n);

static inline unsigned long long dptx_div(unsigned long long num, uint32_t den)
{
	unsigned long long ret = num;

	do_div(ret, den);
	return ret;
}

static inline unsigned long long dptx_div_around(unsigned long long num, uint32_t den)
{
	unsigned long long ret = num + den / 2;

	if (den == 1)
		return num;
	do_div(ret, den);
	return ret;
}

#define dptx_diff(a, b) (((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))

#define CAP_COMP(X, Y) ({typeof(X) x_ = (X); typeof(Y) y_ = (Y); (x_ < y_) ? x_ : y_; })

enum DP_link_rate_e {
	DP_LINK_RATE_RBR    = 0x06,
	DP_LINK_RATE_2P16G  = 0x08,
	DP_LINK_RATE_HBR    = 0x0a,
	DP_LINK_RATE_3P24G  = 0x0c,
	DP_LINK_RATE_4P32G  = 0x0f,
	DP_LINK_RATE_HBR2   = 0x14,
	DP_LINK_RATE_HBR3   = 0x1e,

	DP_LINK_RATE_UBR10  = 0x1,
	DP_LINK_RATE_UBR135 = 0x2,
	DP_LINK_RATE_UBR20  = 0x3,
	DP_LINK_RATE_INVALID = 0,
};

#define DPTX_TPS_DISABLE      0
#define DPTX_TPS1             1
#define DPTX_TPS2             2
#define DPTX_TPS3             3
#define DPTX_TPS4             4
#define DPTX_QUAL_PAT_DISABLE 5
#define DPTX_D10P2            6
#define DPTX_SYMBOL_ERROR_MSR 7
#define DPTX_PRBS7            8
#define DPTX_80BIT_CUSTOM     9
#define DPTX_HBR2_EYE         10
#define DPTX_TRP_CR           DPTX_TPS1
#define DPTX_TRP_EQ           0xf0

#define DPTX_AUX_CMD_WRITE            0x8
#define DPTX_AUX_CMD_READ             0x9
#define DPTX_AUX_CMD_I2C_WRITE        0x0
#define DPTX_AUX_CMD_I2C_WRITE_MOT    0x4
#define DPTX_AUX_CMD_I2C_READ         0x1
#define DPTX_AUX_CMD_I2C_READ_MOT     0x5
#define DPTX_AUX_CMD_I2C_WRITE_STATUS 0x2

struct dptx_aux_req_s {
	u32 address; //20bit for DPCD and 8 bit for I2C
	u8 cmd_code;
	u8 byte_cnt; //up to 16 word
	u8 *data;
};

/* dptx GPIO */
void dptx_gpio_set(const char *name, int value);
unsigned int dptx_gpio_input_get(const char *name);
void dptx_HPD_pinmux_set(struct dptx_drv_s *dptx);

/* VENC */
unsigned int dptx_get_encl_line_cnt(struct dptx_drv_s *dptx);
unsigned int dptx_get_max_line_cnt(struct dptx_drv_s *dptx);
void dptx_debug_test(struct dptx_drv_s *dptx, uint8_t num);
void dptx_set_venc_timing(struct dptx_drv_s *dptx);
void dptx_set_venc(struct dptx_drv_s *dptx);
void dptx_venc_enable(struct dptx_drv_s *dptx, uint8_t flag);
void dptx_mute_set(struct dptx_drv_s *dptx, uint8_t flag);
int dptx_venc_probe(struct dptx_chip_data_s *dptx_chip_data);

/* ANALOG PHY */
void dptx_phy_enable(struct dptx_drv_s *dptx, uint8_t port);
void dptx_phy_disable(struct dptx_drv_s *dptx, uint8_t port);
void dptx_phy_set_lane(struct dptx_drv_s *dptx, uint8_t port, uint8_t lane_mask);
void dptx_phy_probe(struct dptx_chip_data_s *dptx_chip_data);

/* CLK */
void dptx_clk_config_print(struct dptx_drv_s *dptx);
void dptx_clk_set_link_clk(struct dptx_drv_s *dptx, uint8_t port, uint8_t dptx_link_rate);
void dptx_clk_set_vid_clk(struct dptx_drv_s *dptx, uint32_t pixel_clk);
void dptx_clk_set_ssc(struct dptx_drv_s *dptx, uint8_t status);
void dptx_clk_config_probe(struct dptx_drv_s *dptx);

/* IP-interface */
uint8_t dptx_if_aux_write(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf);
uint8_t dptx_if_aux_write_single(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, uint8_t val);
uint8_t dptx_if_aux_read(struct dptx_drv_s *dptx, uint8_t port,
				uint32_t addr, int len, uint8_t *buf);
uint8_t dptx_if_aux_i2c_op(struct dptx_drv_s *dptx, uint8_t port,
				uint8_t cmd_type, uint32_t dev_addr, uint8_t len, uint8_t *data);
void dptx_if_transmit_pattern(struct dptx_drv_s *dptx, uint8_t port,
				uint8_t pattern, uint8_t lane);
void dptx_if_set_MSA(struct dptx_drv_s *dptx, uint8_t port);
#define DPTX_RESET_COMBO_DPHY          BIT(0)
#define DPTX_RESET_eDP_PIPE            BIT(1)
#define DPTX_RESET_eDP_CTRL            BIT(2)
#define DPTX_RESET_AUX_CLK_DIVIDER     BIT(3)
#define DPTX_RESET_PHY                 BIT(4)
#define DPTX_RESET_VENC                BIT(5)
#define DPTX_RESET_ALL                 0xff
void dptx_if_path_reset(struct dptx_drv_s *dptx, uint8_t port, uint8_t mask);
void dptx_if_set_lane_cfg(struct dptx_drv_s *dptx, uint8_t port);
void dptx_if_set_phy_cfg(struct dptx_drv_s *dptx, uint8_t port, uint8_t lane_mask);
void dptx_if_transmitter_init(struct dptx_drv_s *dptx, uint8_t port);
void dptx_if_transmitter_output(struct dptx_drv_s *dptx, uint8_t port, uint8_t en);
uint8_t dptx_if_get_hpd_level(struct dptx_drv_s *dptx, uint8_t port);
uint16_t dptx_if_get_hpd_irq(struct dptx_drv_s *dptx, uint8_t port);

#define DPTX_IRQ_REPLY_TIMEOUT_MASK    BIT(3)
#define DPTX_IRQ_REPLY_RECEIVED_MASK   BIT(2)
#define DPTX_IRQ_HPD_EVENT_MASK        BIT(1)
#define DPTX_IRQ_HPD_IRQ_EVENT         BIT(0)
void dptx_if_set_hpd_interrupt_mask(struct dptx_drv_s *dptx, uint8_t port, u8 mask);

#define DPTX_SCRAMBLE_RESET_OFF              0
#define DPTX_SCRAMBLE_RESET_ON               1
#define DPTX_eDP_ALTERNATIVE_SCRAMBLE_RESET  2
void dptx_if_scramble_reset_set(struct dptx_drv_s *dptx, uint8_t port, u8 sr_type);
void dptx_if_PSR1_ctrl(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);
void dptx_if_PSR2_ctrl(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);

void dptx_if_IP_probe(struct dptx_drv_s *dptx);
/* IP-interface END */

/* dptx_link_training.c */
struct DPTX_test_pat_s {
	char *name;
	uint8_t data;
	bool SR_disable, encoding_en;
};

extern struct DPTX_test_pat_s DP_test_pat[];

#define DPTX_LINK_TRAINING_AUTO      0
#define DPTX_FAST_LINK_TRAINING      1
#define DPTX_FULL_LINK_TRAINING      2
int __dptx_link_training(struct dptx_drv_s *dptx);
int __dptx_full_link_training(struct dptx_drv_s *dptx);
int __dptx_fast_link_training(struct dptx_drv_s *dptx);

/* dptx_EDID_DisplayID.c */
int __dptx_EDID_probe(struct dptx_drv_s *dptx);

/* dptx_vmode_mgr.c */
void dptx_print_vmode(struct dptx_drv_s *dptx, uint8_t print_flag);
void dptx_vmode_manage(struct dptx_drv_s *dptx);

/* dptx_utils.c */
extern uint16_t dptx_train_rd_intv[5];
extern uint16_t dptx_PSR_setup_time[8];
// extern char *eDP_ver_str[6];
uint8_t dptx_vswing_ds_to_phy(struct dptx_drv_s *dptx, uint8_t ds_level);
uint8_t dptx_preem_ds_to_phy(struct dptx_drv_s *dptx, uint8_t ds_level);
uint8_t ds_to_DPCD_LANESET(uint8_t ds_level);
uint8_t dptx_ds_to_vswing(uint8_t ds);
uint8_t dptx_ds_to_preem(uint8_t ds);
uint8_t dptx_v_p_to_ds(uint8_t vsw, uint8_t preem);
void dptx_link_cfg_dft(struct dptx_drv_s *dptx, uint8_t port);
uint8_t dptx_DPCD_capability_to_link_cfg(struct dptx_drv_s *dptx, uint8_t port);
void dptx_link_policy_maker(struct dptx_drv_s *dptx, uint8_t port);
uint8_t dptx_vid_band_width_check(struct dptx_drv_s *dptx, uint32_t pclk, uint8_t bpp);

int dptx_connector_check(struct dptx_drv_s *dptx);
int dptx_outputmode_check(struct dptx_drv_s *dptx, char *mode);

void __dptx_set_phy_config(struct dptx_drv_s *dptx, uint8_t port, uint8_t use_preset);
void __dptx_set_lane_config(struct dptx_drv_s *dptx, uint8_t port);
void dptx_eDP_PSR1(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);
void dptx_eDP_PSR2(struct dptx_drv_s *dptx, uint8_t port, uint8_t flag);

/* ************* DPTX VMODE related ************/
struct dptx_vmode_s *dptx_get_vmode(struct dptx_drv_s *dptx, uint8_t th);
struct dptx_vmode_s *dptx_get_optimum_vmode(struct dptx_drv_s *dptx);
void dptx_vmode_apply_to_act_timing(struct dptx_drv_s *dptx, struct dptx_vmode_s *vmd);
void dptx_act_timing_apply(struct dptx_drv_s *dptx);
void dptx_list_support_vmode(struct dptx_drv_s *dptx);
void dptx_user_set_vmode(struct dptx_drv_s *dptx, uint8_t vmd_idx);

extern struct dptx_detail_timing_s DPTX_SafeMode_640x480_timing;
extern struct dptx_vmode_s DPTX_SafeMode_640x480_vmode;

void __dptx_update_ctrl_bootargs(struct dptx_drv_s *dptx);

void dptx_act_timing_to_venc_config(struct dptx_drv_s *dptx);

uint8_t __str_add_vmode(struct dptx_drv_s *dptx, char *buf, uint8_t vmd_idx, uint8_t fr);

#define DPTX_PHY_STA_VSW_0_PREEM_0     0
#define DPTX_PHY_STA_VSW_0_PREEM_1     1
#define DPTX_PHY_STA_VSW_0_PREEM_2     2
#define DPTX_PHY_STA_VSW_0_PREEM_3     3
#define DPTX_PHY_STA_VSW_1_PREEM_0     4
#define DPTX_PHY_STA_VSW_1_PREEM_1     5
#define DPTX_PHY_STA_VSW_1_PREEM_2     6
#define DPTX_PHY_STA_VSW_2_PREEM_0     7
#define DPTX_PHY_STA_VSW_2_PREEM_1     8
#define DPTX_PHY_STA_VSW_3_PREEM_0     9
#define DPTX_PHY_STA_DISABLE           0xff

// //! Content Protect
void dptx_set_content_protection(struct dptx_drv_s *dptx, uint8_t port);

void dptx_drv_check_HPD(struct dptx_drv_s *dptx);
void dptx_driver_ready(struct dptx_drv_s *dptx);
void dptx_drv_start(struct dptx_drv_s *dptx);
void dptx_drv_disp_on(struct dptx_drv_s *dptx);
void dptx_drv_disp_off(struct dptx_drv_s *dptx);
void dptx_driver_close(struct dptx_drv_s *dptx);
void dptx_drv_eDP_PSR1_en(struct dptx_drv_s *dptx, uint8_t port_mask, uint8_t en);
void dptx_drv_eDP_PSR2_en(struct dptx_drv_s *dptx, uint8_t port_mask, uint8_t en);

// ! debug
void dptx_info_print(struct dptx_drv_s *dptx);
void dptx_reg_print(struct dptx_drv_s *dptx);
void dptx_debug_reset(struct dptx_drv_s *dptx, uint8_t port_mask, uint8_t reset_part);

#endif
