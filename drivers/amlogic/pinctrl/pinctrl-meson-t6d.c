// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/amlogic/gpio/meson-t6d-gpio.h>

#include <../../pinctrl/meson/pinctrl-meson-axg.h>

static struct meson_pmx_group meson_t6d_analog_groups[] = {
	GPIO_GROUP(CDAC_IOUT,		0),
	GPIO_GROUP(CVBS0,		0),
};

static const char * const gpio_analog_groups[] = {
	"CDAC_IOUT", "CVBS0",
};

static struct meson_pmx_func meson_t6d_analog_functions[] = {
	FUNCTION(gpio_analog),
};

static struct meson_bank meson_t6d_analog_banks[] = {
	BANK("ANALOG", CDAC_IOUT, CVBS0, 0, 30, 0, 28, 1, 0, 0, 26, 0, 0),
};

static struct meson_pmx_bank meson_t6d_analog_pmx_banks[] = {
	BANK_PMX("ANALOG", CDAC_IOUT, CVBS0, 0x000, 24),
};

static struct meson_axg_pmx_data meson_t6d_analog_pmx_banks_data = {
	.pmx_banks	= meson_t6d_analog_pmx_banks,
	.num_pmx_banks	= ARRAY_SIZE(meson_t6d_analog_pmx_banks),
};

static struct meson_pinctrl_data meson_t6d_analog_pinctrl_data = {
	.name		= "analog-banks",
	.groups		= meson_t6d_analog_groups,
	.funcs		= meson_t6d_analog_functions,
	.banks		= meson_t6d_analog_banks,
	.num_pins	= 2,
	.num_groups	= ARRAY_SIZE(meson_t6d_analog_groups),
	.num_funcs	= ARRAY_SIZE(meson_t6d_analog_functions),
	.num_banks	= ARRAY_SIZE(meson_t6d_analog_banks),
	.gpio_driver	= &meson_axg_gpio_driver,
	.pmx_data	= &meson_t6d_analog_pmx_banks_data,
};

/* GPIOW func1 */
static const unsigned int hdmirx_hpd_a_pins[]			= { GPIOW_0 };
static const unsigned int hdmirx_5vdet_a_pins[]			= { GPIOW_1 };
static const unsigned int hdmirx_sda_a_pins[]			= { GPIOW_2 };
static const unsigned int hdmirx_scl_a_pins[]			= { GPIOW_3 };
static const unsigned int hdmirx_hpd_b_pins[]			= { GPIOW_4 };
static const unsigned int hdmirx_5vdet_b_pins[]			= { GPIOW_5 };
static const unsigned int hdmirx_sda_b_pins[]			= { GPIOW_6 };
static const unsigned int hdmirx_scl_b_pins[]			= { GPIOW_7 };
static const unsigned int hdmirx_hpd_c_pins[]			= { GPIOW_8 };
static const unsigned int hdmirx_5vdet_c_pins[]			= { GPIOW_9 };
static const unsigned int hdmirx_sda_c_pins[]			= { GPIOW_10 };
static const unsigned int hdmirx_scl_c_pins[]			= { GPIOW_11 };
static const unsigned int cec_pins[]				= { GPIOW_12 };

/* GPIOW func2 */
static const unsigned int uart_b_tx_w2_pins[]			= { GPIOW_2 };
static const unsigned int uart_b_rx_w3_pins[]			= { GPIOW_3 };
static const unsigned int uart_b_tx_w6_pins[]			= { GPIOW_6 };
static const unsigned int uart_b_rx_w7_pins[]			= { GPIOW_7 };
static const unsigned int uart_b_tx_w10_pins[]			= { GPIOW_10 };
static const unsigned int uart_b_rx_w11_pins[]			= { GPIOW_11 };

/* GPIOW func3 */
static const unsigned int jtag_a_clk_w2_pins[]			= { GPIOW_2 };
static const unsigned int jtag_a_tms_w3_pins[]			= { GPIOW_3 };
static const unsigned int jtag_a_tdi_w6_pins[]			= { GPIOW_6 };
static const unsigned int jtag_a_tdo_w7_pins[]			= { GPIOW_7 };

/* GPIOW func4 */
static const unsigned int gpiow3_loop_pins[]			= { GPIOW_2 };
static const unsigned int gpiow2_loop_pins[]			= { GPIOW_3 };
static const unsigned int jtag_a_clk_w6_pins[]			= { GPIOW_6 };
static const unsigned int jtag_a_tms_w7_pins[]			= { GPIOW_7 };
static const unsigned int jtag_a_tdi_w10_pins[]			= { GPIOW_10 };
static const unsigned int jtag_a_tdo_w11_pins[]			= { GPIOW_11 };

/* GPIOD func1 */
static const unsigned int uart_b_tx_d0_pins[]			= { GPIOD_0 };
static const unsigned int uart_b_rx_d1_pins[]			= { GPIOD_1 };
static const unsigned int i2c1_scl_d2_pins[]			= { GPIOD_2 };
static const unsigned int i2c1_sda_d3_pins[]			= { GPIOD_3 };
static const unsigned int clk_32k_in_pins[]			= { GPIOD_4 };
static const unsigned int ir_in_a_pins[]			= { GPIOD_5 };
static const unsigned int jtag_a_clk_d6_pins[]			= { GPIOD_6 };
static const unsigned int jtag_a_tms_d7_pins[]			= { GPIOD_7 };
static const unsigned int jtag_a_tdi_d8_pins[]			= { GPIOD_8 };
static const unsigned int jtag_a_tdo_d9_pins[]			= { GPIOD_9 };
static const unsigned int clk12_24m_d10_pins[]			= { GPIOD_10 };
static const unsigned int pwm_a_d11_pins[]			= { GPIOD_11 };
static const unsigned int pwm_f_d12_pins[]			= { GPIOD_12 };
static const unsigned int i2c1_sda_d13_pins[]			= { GPIOD_13 };
static const unsigned int i2c1_scl_d14_pins[]			= { GPIOD_14 };

/* GPIOD func2 */
static const unsigned int ir_out_d1_pins[]			= { GPIOD_1 };
static const unsigned int i2cs_a_scl_pins[]			= { GPIOD_2 };
static const unsigned int i2cs_a_sda_pins[]			= { GPIOD_3 };
static const unsigned int atv_if_agc_d4_pins[]			= { GPIOD_4 };
static const unsigned int pwm_c_d5_pins[]			= { GPIOD_5 };
static const unsigned int pwm_f_d6_pins[]			= { GPIOD_6 };
static const unsigned int pwm_c_d7_pins[]			= { GPIOD_7 };
static const unsigned int spdif_out_a_d8_pins[]			= { GPIOD_8 };
static const unsigned int pwm_d_d9_pins[]			= { GPIOD_9 };
static const unsigned int pwm_e_d10_pins[]			= { GPIOD_10 };
static const unsigned int pwm_c_d11_pins[]			= { GPIOD_11 };

/* GPIOD func3 */
static const unsigned int cicam_waitn_d0_pins[]			= { GPIOD_0 };
static const unsigned int demod_uart_tx_d2_pins[]		= { GPIOD_2 };
static const unsigned int demod_uart_rx_d3_pins[]		= { GPIOD_3 };
static const unsigned int dtv_if_agc_d4_pins[]			= { GPIOD_4 };
static const unsigned int clk12_24m_d5_pins[]			= { GPIOD_5 };
static const unsigned int pwm_d_hiz_pins[]			= { GPIOD_6 };
static const unsigned int pwm_c_hiz_pins[]			= { GPIOD_7 };
static const unsigned int ir_out_d9_pins[]			= { GPIOD_9 };
static const unsigned int gen_clk_out_d10_pins[]		= { GPIOD_10 };
static const unsigned int ir_in_b_d14_pins[]			= { GPIOD_14 };

/* GPIOD func4 */
static const unsigned int gen_clk_out_d5_pins[]			= { GPIOD_5 };
static const unsigned int uart_c_tx_d6_pins[]			= { GPIOD_6 };
static const unsigned int uart_c_rx_d7_pins[]			= { GPIOD_7 };
static const unsigned int tdm_d2_d8_pins[]			= { GPIOD_8 };
static const unsigned int tdm_d3_d9_pins[]			= { GPIOD_9 };

/* GPIOD func5 */
static const unsigned int dcon_led_d7_pins[]			= { GPIOD_7 };
static const unsigned int dcon_led_d8_pins[]			= { GPIOD_8 };
static const unsigned int dcon_led_d9_pins[]			= { GPIOD_9 };

/* GPIOD func6 */
static const unsigned int pwm_h_d6_pins[]			= { GPIOD_6 };
static const unsigned int mclk_1_d7_pins[]			= { GPIOD_7 };
static const unsigned int spdif_out_b_d8_pins[]			= { GPIOD_8 };
static const unsigned int pwm_vs_d9_pins[]			= { GPIOD_9 };

/* GPIOD func7 */
static const unsigned int i2c3_scl_d6_pins[]			= { GPIOD_6 };
static const unsigned int i2c3_sda_d7_pins[]			= { GPIOD_7 };

/* GPIOE func1 */
static const unsigned int pwm_a_e0_pins[]			= { GPIOE_0 };
static const unsigned int pwm_b_e1_pins[]			= { GPIOE_1 };
static const unsigned int pwm_e_e2_pins[]			= { GPIOE_2 };

/* GPIOE func2 */
static const unsigned int i2c2_scl_e0_pins[]			= { GPIOE_0 };
static const unsigned int i2c2_sda_e1_pins[]			= { GPIOE_1 };
static const unsigned int clk12_24m_e2_pins[]			= { GPIOE_2 };

/* GPIOE func3 */
static const unsigned int spdif_out_a_e2_pins[]			= { GPIOE_2 };

/* GPIOE func4 */
static const unsigned int gen_clk_out_e2_pins[]			= { GPIOE_2 };

/* GPIOB func1 */
static const unsigned int emmc_d0_pins[]			= { GPIOB_0 };
static const unsigned int emmc_d1_pins[]			= { GPIOB_1 };
static const unsigned int emmc_d2_pins[]			= { GPIOB_2 };
static const unsigned int emmc_d3_pins[]			= { GPIOB_3 };
static const unsigned int emmc_d4_pins[]			= { GPIOB_4 };
static const unsigned int emmc_d5_pins[]			= { GPIOB_5 };
static const unsigned int emmc_d6_pins[]			= { GPIOB_6 };
static const unsigned int emmc_d7_pins[]			= { GPIOB_7 };
static const unsigned int emmc_clk_pins[]			= { GPIOB_8 };
static const unsigned int emmc_cmd_pins[]			= { GPIOB_10 };
static const unsigned int emmc_ds_pins[]			= { GPIOB_11 };
static const unsigned int gpiob12_loop_pins[]			= { GPIOB_13 };

/* GPIOB func2 */
static const unsigned int nand_wen_clk_pins[]			= { GPIOB_8 };
static const unsigned int nand_ale_pins[]			= { GPIOB_9 };
static const unsigned int nand_ren_wr_pins[]			= { GPIOB_10 };
static const unsigned int nand_cle_pins[]			= { GPIOB_11 };
static const unsigned int nand_ce0_pins[]			= { GPIOB_12 };
static const unsigned int gpioh21_loop_pins[]			= { GPIOB_13 };

/* GPIOB func3 */
static const unsigned int spinf_hold_d3_pins[]			= { GPIOB_3 };
static const unsigned int spinf_mo_d0_pins[]			= { GPIOB_4 };
static const unsigned int spinf_mi_d1_pins[]			= { GPIOB_5 };
static const unsigned int spinf_clk_pins[]			= { GPIOB_6 };
static const unsigned int spinf_wp_d2_pins[]			= { GPIOB_7 };
static const unsigned int spinf_d4_pins[]			= { GPIOB_8 };
static const unsigned int spinf_d5_pins[]			= { GPIOB_9 };
static const unsigned int spinf_d6_pins[]			= { GPIOB_10 };
static const unsigned int spinf_d7_pins[]			= { GPIOB_11 };
static const unsigned int spinf_cs1_pins[]			= { GPIOB_12 };
static const unsigned int spinf_cs0_pins[]			= { GPIOB_13 };

/* GPIOB func4 */
static const unsigned int ir_out_b12_pins[]			= { GPIOB_12 };

/* GPIOB func7 */
static const unsigned int gen_clk_out_b6_pins[]			= { GPIOB_6 };

/* GPIOC func1 */
static const unsigned int i2c3_scl_c0_pins[]			= { GPIOC_0 };
static const unsigned int i2c3_sda_c1_pins[]			= { GPIOC_1 };
static const unsigned int uart_a_tx_c6_pins[]			= { GPIOC_6 };
static const unsigned int uart_a_rx_c7_pins[]			= { GPIOC_7 };
static const unsigned int uart_a_cts_c8_pins[]			= { GPIOC_8 };
static const unsigned int uart_a_rts_c9_pins[]			= { GPIOC_9 };
static const unsigned int pwm_f_c10_pins[]			= { GPIOC_10 };

/* GPIOC func2 */
static const unsigned int spi_miso_b_c0_pins[]			= { GPIOC_0 };
static const unsigned int spi_mosi_b_c1_pins[]			= { GPIOC_1 };
static const unsigned int spi_clk_b_c2_pins[]			= { GPIOC_2 };
static const unsigned int spi_ss0_b_c3_pins[]			= { GPIOC_3 };
static const unsigned int spi_ss1_b_pins[]			= { GPIOC_4 };
static const unsigned int spi_ss2_b_pins[]			= { GPIOC_5 };
static const unsigned int spi_clk_b_c6_pins[]			= { GPIOC_6 };
static const unsigned int spi_ss0_b_c7_pins[]			= { GPIOC_7 };
static const unsigned int mclk_1_c8_pins[]			= { GPIOC_8 };
static const unsigned int mclk_2_c9_pins[]			= { GPIOC_9 };

/* GPIOC func3 */
static const unsigned int cicam_a2_c0_pins[]			= { GPIOC_0 };
static const unsigned int cicam_a3_c1_pins[]			= { GPIOC_1 };
static const unsigned int cicam_a4_c2_pins[]			= { GPIOC_2 };
static const unsigned int cicam_a5_c3_pins[]			= { GPIOC_3 };
static const unsigned int cicam_a6_c4_pins[]			= { GPIOC_4 };
static const unsigned int cicam_a7_c5_pins[]			= { GPIOC_5 };
static const unsigned int cicam_a8_c6_pins[]			= { GPIOC_6 };
static const unsigned int cicam_a9_c7_pins[]			= { GPIOC_7 };
static const unsigned int cicam_a10_c8_pins[]			= { GPIOC_8 };
static const unsigned int cicam_a11_c9_pins[]			= { GPIOC_9 };
static const unsigned int cicam_a12_c10_pins[]			= { GPIOC_10 };

/* GPIOC func4 */
static const unsigned int demod_uart_tx_c6_pins[]		= { GPIOC_6 };
static const unsigned int demod_uart_rx_c7_pins[]		= { GPIOC_7 };
static const unsigned int pwm_vs_c10_pins[]			= { GPIOC_10 };

/* GPIOC func5 */
static const unsigned int tdm_fs2_c2_pins[]			= { GPIOC_2 };
static const unsigned int tdm_sclk2_c3_pins[]			= { GPIOC_3 };
static const unsigned int tdm_d6_c4_pins[]			= { GPIOC_4 };
static const unsigned int tdm_d7_c5_pins[]			= { GPIOC_5 };
static const unsigned int tdm_fs1_c6_pins[]			= { GPIOC_6 };
static const unsigned int tdm_sclk1_c7_pins[]			= { GPIOC_7 };
static const unsigned int tdm_d4_c8_pins[]			= { GPIOC_8 };
static const unsigned int tdm_d5_c9_pins[]			= { GPIOC_9 };

/* GPIOC func6 */
static const unsigned int gen_clk_out_c2_pins[]			= { GPIOC_2 };

/* GPIOC func7 */
static const unsigned int dcon_led_c10_pins[]			= { GPIOC_10 };

/* GPIOZ func1 */
static const unsigned int tdm_fs2_z0_pins[]			= { GPIOZ_0 };
static const unsigned int tdm_sclk2_z1_pins[]			= { GPIOZ_1 };
static const unsigned int tdm_d4_z2_pins[]			= { GPIOZ_2 };
static const unsigned int tdm_d5_z3_pins[]			= { GPIOZ_3 };
static const unsigned int tdm_d6_z4_pins[]			= { GPIOZ_4 };
static const unsigned int tdm_d7_z5_pins[]			= { GPIOZ_5 };
static const unsigned int mclk_2_z6_pins[]			= { GPIOZ_6 };
static const unsigned int tsout_clk_pins[]			= { GPIOZ_7 };
static const unsigned int tsout_sop_pins[]			= { GPIOZ_8 };
static const unsigned int tsout_valid_pins[]			= { GPIOZ_9 };
static const unsigned int tsout_d0_pins[]			= { GPIOZ_10 };
static const unsigned int tsout_d1_pins[]			= { GPIOZ_11 };
static const unsigned int tsout_d2_pins[]			= { GPIOZ_12 };
static const unsigned int tsout_d3_pins[]			= { GPIOZ_13 };
static const unsigned int tsout_d4_pins[]			= { GPIOZ_14 };
static const unsigned int tsout_d5_pins[]			= { GPIOZ_15 };
static const unsigned int tsout_d6_pins[]			= { GPIOZ_16 };
static const unsigned int tsout_d7_pins[]			= { GPIOZ_17 };
static const unsigned int dcon_led_z18_pins[]			= { GPIOZ_18 };
static const unsigned int spdif_out_a_z19_pins[]		= { GPIOZ_19 };

/* GPIOZ func2 */
static const unsigned int tsin_a_clk_z0_pins[]			= { GPIOZ_0 };
static const unsigned int tsin_a_sop_z1_pins[]			= { GPIOZ_1 };
static const unsigned int tsin_a_valid_z2_pins[]		= { GPIOZ_2 };
static const unsigned int tsin_a_d0_z3_pins[]			= { GPIOZ_3 };
static const unsigned int dtv_rf_agc_z6_pins[]			= { GPIOZ_6 };
static const unsigned int cicam_a2_z8_pins[]			= { GPIOZ_8 };
static const unsigned int cicam_a3_z9_pins[]			= { GPIOZ_9 };
static const unsigned int cicam_a4_z10_pins[]			= { GPIOZ_10 };
static const unsigned int cicam_a5_z11_pins[]			= { GPIOZ_11 };
static const unsigned int cicam_a6_z12_pins[]			= { GPIOZ_12 };
static const unsigned int cicam_a7_z13_pins[]			= { GPIOZ_13 };
static const unsigned int cicam_a8_z14_pins[]			= { GPIOZ_14 };
static const unsigned int cicam_a9_z15_pins[]			= { GPIOZ_15 };
static const unsigned int cicam_a10_z16_pins[]			= { GPIOZ_16 };
static const unsigned int cicam_a11_z17_pins[]			= { GPIOZ_17 };
static const unsigned int dtv_rf_agc_z18_pins[]			= { GPIOZ_18 };

/* GPIOZ func3 */
static const unsigned int iso7816_clk_z0_pins[]			= { GPIOZ_0 };
static const unsigned int iso7816_data_z1_pins[]		= { GPIOZ_1 };
static const unsigned int cicam_a13_pins[]			= { GPIOZ_2 };
static const unsigned int cicam_a14_pins[]			= { GPIOZ_3 };
static const unsigned int i2c0_scl_pins[]			= { GPIOZ_4 };
static const unsigned int i2c0_sda_pins[]			= { GPIOZ_5 };
static const unsigned int atv_if_agc_z6_pins[]			= { GPIOZ_6 };
static const unsigned int atv_if_agc_z18_pins[]			= { GPIOZ_18 };
static const unsigned int spdif_out_b_z19_pins[]		= { GPIOZ_19 };

/* GPIOZ func4 */
static const unsigned int spi_miso_b_z0_pins[]			= { GPIOZ_0 };
static const unsigned int spi_mosi_b_z1_pins[]			= { GPIOZ_1 };
static const unsigned int spi_clk_b_z2_pins[]			= { GPIOZ_2 };
static const unsigned int spi_ss0_b_z3_pins[]			= { GPIOZ_3 };
static const unsigned int dtv_if_agc_z6_pins[]			= { GPIOZ_6 };
static const unsigned int dtv_if_agc_z18_pins[]			= { GPIOZ_18 };
static const unsigned int pwm_g_z19_pins[]			= { GPIOZ_19 };

/* GPIOZ func5 */
static const unsigned int uart_a_tx_z0_pins[]			= { GPIOZ_0 };
static const unsigned int uart_a_rx_z1_pins[]			= { GPIOZ_1 };
static const unsigned int uart_a_cts_z2_pins[]			= { GPIOZ_2 };
static const unsigned int uart_a_rts_z3_pins[]			= { GPIOZ_3 };
static const unsigned int pwm_b_z4_pins[]			= { GPIOZ_4 };
static const unsigned int pwm_d_z5_pins[]			= { GPIOZ_5 };
static const unsigned int pwm_e_z6_pins[]			= { GPIOZ_6 };
static const unsigned int pwm_g_z12_pins[]			= { GPIOZ_12 };
static const unsigned int pwm_h_z13_pins[]			= { GPIOZ_13 };
static const unsigned int gen_clk_out_z16_pins[]		= { GPIOZ_16 };
static const unsigned int ir_in_b_z18_pins[]			= { GPIOZ_18 };
static const unsigned int ir_in_b_z19_pins[]			= { GPIOZ_19 };

/* GPIOZ func6 */
static const unsigned int cicam_a12_z0_pins[]			= { GPIOZ_0 };
static const unsigned int pdm_din2_z1_pins[]			= { GPIOZ_1 };
static const unsigned int pdm_dclk_z2_pins[]			= { GPIOZ_2 };
static const unsigned int pdm_din0_z3_pins[]			= { GPIOZ_3 };
static const unsigned int pdm_din1_z6_pins[]			= { GPIOZ_6 };
static const unsigned int tdm_d1_z10_pins[]			= { GPIOZ_10 };
static const unsigned int tdm_d0_z11_pins[]			= { GPIOZ_11 };
static const unsigned int tdm_sclk1_z16_pins[]			= { GPIOZ_16 };
static const unsigned int tdm_fs1_z17_pins[]			= { GPIOZ_17 };
static const unsigned int pwm_vs_z18_pins[]			= { GPIOZ_18 };
static const unsigned int diseqc_out_z19_pins[]			= { GPIOZ_19 };

/* GPIOZ func7 */
static const unsigned int diseqc_out_z0_pins[]			= { GPIOZ_0 };
static const unsigned int s2_demod_gpio0_z1_pins[]		= { GPIOZ_1 };
static const unsigned int spdif_out_a_z2_pins[]			= { GPIOZ_2 };
static const unsigned int eth_link_led_z6_pins[]		= { GPIOZ_6 };
static const unsigned int eth_act_led_z19_pins[]		= { GPIOZ_19 };

/* GPIOH func1 */
static const unsigned int tcon_0_pins[]				= { GPIOH_0 };
static const unsigned int tcon_1_pins[]				= { GPIOH_1 };
static const unsigned int tcon_2_pins[]				= { GPIOH_2 };
static const unsigned int tcon_3_pins[]				= { GPIOH_3 };
static const unsigned int tcon_4_pins[]				= { GPIOH_4 };
static const unsigned int tcon_5_pins[]				= { GPIOH_5 };
static const unsigned int tcon_6_pins[]				= { GPIOH_6 };
static const unsigned int tcon_7_pins[]				= { GPIOH_7 };
static const unsigned int tcon_8_pins[]				= { GPIOH_8 };
static const unsigned int tcon_9_pins[]				= { GPIOH_9 };
static const unsigned int tcon_10_pins[]			= { GPIOH_10 };
static const unsigned int tcon_11_pins[]			= { GPIOH_11 };
static const unsigned int tcon_12_pins[]			= { GPIOH_12 };
static const unsigned int tcon_13_pins[]			= { GPIOH_13 };
static const unsigned int tcon_14_pins[]			= { GPIOH_14 };
static const unsigned int tcon_15_pins[]			= { GPIOH_15 };
static const unsigned int hsync_pins[]				= { GPIOH_18 };
static const unsigned int vsync_pins[]				= { GPIOH_19 };
static const unsigned int i2c2_scl_h20_pins[]			= { GPIOH_20 };
static const unsigned int i2c2_sda_h21_pins[]			= { GPIOH_21 };

/* GPIOH func2 */
static const unsigned int i2c2_scl_h2_pins[]			= { GPIOH_2 };
static const unsigned int i2c2_sda_h3_pins[]			= { GPIOH_3 };
static const unsigned int sync_3d_out_pins[]			= { GPIOH_5 };
static const unsigned int tdm_d3_h6_pins[]			= { GPIOH_6 };
static const unsigned int spi_ss1_a_h7_pins[]			= { GPIOH_7 };
static const unsigned int spi_ss0_a_h8_pins[]			= { GPIOH_8 };
static const unsigned int spi_miso_a_h9_pins[]			= { GPIOH_9 };
static const unsigned int spi_mosi_a_h10_pins[]			= { GPIOH_10 };
static const unsigned int spi_clk_a_h11_pins[]			= { GPIOH_11 };
static const unsigned int spi_ss2_a_h12_pins[]			= { GPIOH_12 };
static const unsigned int tdm_d3_h13_pins[]			= { GPIOH_13 };
static const unsigned int mclk_1_h14_pins[]			= { GPIOH_14 };
static const unsigned int tdm_fs1_h15_pins[]			= { GPIOH_15 };
static const unsigned int tdm_sclk1_h16_pins[]			= { GPIOH_16 };
static const unsigned int tdm_d0_h17_pins[]			= { GPIOH_17 };
static const unsigned int tdm_d1_h18_pins[]			= { GPIOH_18 };
static const unsigned int tdm_d2_h19_pins[]			= { GPIOH_19 };
static const unsigned int tdm_d3_h20_pins[]			= { GPIOH_20 };
static const unsigned int tdm_d4_h21_pins[]			= { GPIOH_21 };

/* GPIOH func3 */
static const unsigned int uart_c_tx_h2_pins[]			= { GPIOH_2 };
static const unsigned int uart_c_rx_h3_pins[]			= { GPIOH_3 };
static const unsigned int uart_c_cts_h4_pins[]			= { GPIOH_4 };
static const unsigned int uart_c_rts_h5_pins[]			= { GPIOH_5 };
static const unsigned int i2c2_scl_h10_pins[]			= { GPIOH_10 };
static const unsigned int i2c2_sda_h11_pins[]			= { GPIOH_11 };
static const unsigned int pwm_vs_h12_pins[]			= { GPIOH_12 };
static const unsigned int pwm_vs_h13_pins[]			= { GPIOH_13 };
static const unsigned int pdm_din1_h14_pins[]			= { GPIOH_14 };
static const unsigned int pdm_din2_h16_pins[]			= { GPIOH_16 };
static const unsigned int pdm_din3_h17_pins[]			= { GPIOH_17 };
static const unsigned int pdm_din0_h18_pins[]			= { GPIOH_18 };
static const unsigned int pdm_dclk_h19_pins[]			= { GPIOH_19 };
static const unsigned int gpiob13_loop_pins[]			= { GPIOH_21 };

/* GPIOH func4 */
static const unsigned int pwm_d_h5_pins[]			= { GPIOH_5 };
static const unsigned int pwm_d_h12_pins[]			= { GPIOH_12 };
static const unsigned int pwm_e_h13_pins[]			= { GPIOH_13 };
static const unsigned int tdm_d3_h14_pins[]			= { GPIOH_14 };
static const unsigned int eth_act_led_h18_pins[]		= { GPIOH_18 };
static const unsigned int eth_link_led_h19_pins[]		= { GPIOH_19 };

/* GPIOH func5 */
static const unsigned int clk12_24m_h13_pins[]			= { GPIOH_13 };
static const unsigned int spi_clk_a_h14_pins[]			= { GPIOH_14 };
static const unsigned int tdm_fs2_h18_pins[]			= { GPIOH_18 };
static const unsigned int tdm_sclk2_h19_pins[]			= { GPIOH_19 };
static const unsigned int demod_uart_tx_h20_pins[]		= { GPIOH_20 };
static const unsigned int demod_uart_rx_h21_pins[]		= { GPIOH_21 };

/* GPIOH func6 */
static const unsigned int gen_clk_out_h13_pins[]		= { GPIOH_13 };
static const unsigned int s2_demod_gpio0_h14_pins[]		= { GPIOH_14 };
static const unsigned int s2_demod_gpio1_pins[]			= { GPIOH_15 };
static const unsigned int s2_demod_gpio2_pins[]			= { GPIOH_16 };
static const unsigned int s2_demod_gpio3_pins[]			= { GPIOH_17 };
static const unsigned int s2_demod_gpio4_pins[]			= { GPIOH_18 };
static const unsigned int s2_demod_gpio5_pins[]			= { GPIOH_19 };
static const unsigned int s2_demod_gpio6_pins[]			= { GPIOH_20 };
static const unsigned int s2_demod_gpio7_pins[]			= { GPIOH_21 };

/* GPIOH func7 */
static const unsigned int gen_clk_out_h0_pins[]			= { GPIOH_0 };

/* GPIOM func1 */
static const unsigned int tsin_b_clk_pins[]			= { GPIOM_0 };
static const unsigned int tsin_b_sop_pins[]			= { GPIOM_1 };
static const unsigned int tsin_b_valid_pins[]			= { GPIOM_2 };
static const unsigned int tsin_b_d0_pins[]			= { GPIOM_3 };
static const unsigned int tsin_b_d1_pins[]			= { GPIOM_4 };
static const unsigned int tsin_b_d2_pins[]			= { GPIOM_5 };
static const unsigned int tsin_b_d3_pins[]			= { GPIOM_6 };
static const unsigned int tsin_b_d4_pins[]			= { GPIOM_7 };
static const unsigned int tsin_b_d5_pins[]			= { GPIOM_8 };
static const unsigned int tsin_b_d6_pins[]			= { GPIOM_9 };
static const unsigned int tsin_b_d7_pins[]			= { GPIOM_10 };
static const unsigned int cicam_data0_pins[]			= { GPIOM_11 };
static const unsigned int cicam_data1_pins[]			= { GPIOM_12 };
static const unsigned int cicam_data2_pins[]			= { GPIOM_13 };
static const unsigned int cicam_data3_pins[]			= { GPIOM_14 };
static const unsigned int cicam_data4_pins[]			= { GPIOM_15 };
static const unsigned int cicam_data5_pins[]			= { GPIOM_16 };
static const unsigned int cicam_data6_pins[]			= { GPIOM_17 };
static const unsigned int cicam_data7_pins[]			= { GPIOM_18 };
static const unsigned int cicam_a0_pins[]			= { GPIOM_19 };
static const unsigned int cicam_a1_pins[]			= { GPIOM_20 };
static const unsigned int cicam_cen_pins[]			= { GPIOM_21 };
static const unsigned int cicam_oen_pins[]			= { GPIOM_22 };
static const unsigned int cicam_wen_pins[]			= { GPIOM_23 };
static const unsigned int cicam_iordn_pins[]			= { GPIOM_24 };
static const unsigned int cicam_iowrn_pins[]			= { GPIOM_25 };
static const unsigned int cicam_reset_pins[]			= { GPIOM_26 };
static const unsigned int cicam_waitn_m29_pins[]		= { GPIOM_29 };

/* GPIOM func2 */
static const unsigned int pdm_din2_m8_pins[]			= { GPIOM_8 };
static const unsigned int pdm_din3_m9_pins[]			= { GPIOM_9 };
static const unsigned int pdm_din0_m10_pins[]			= { GPIOM_10 };
static const unsigned int pdm_dclk_m11_pins[]			= { GPIOM_11 };
static const unsigned int mclk_2_m12_pins[]			= { GPIOM_12 };
static const unsigned int tdm_fs2_m13_pins[]			= { GPIOM_13 };
static const unsigned int tdm_sclk2_m14_pins[]			= { GPIOM_14 };
static const unsigned int tdm_d4_m15_pins[]			= { GPIOM_15 };
static const unsigned int tdm_d5_m16_pins[]			= { GPIOM_16 };
static const unsigned int tdm_d6_m17_pins[]			= { GPIOM_17 };
static const unsigned int tdm_d7_m18_pins[]			= { GPIOM_18 };
static const unsigned int iso7816_clk_m19_pins[]		= { GPIOM_19 };
static const unsigned int iso7816_data_m20_pins[]		= { GPIOM_20 };
static const unsigned int mclk_1_m21_pins[]			= { GPIOM_21 };
static const unsigned int tdm_fs1_m22_pins[]			= { GPIOM_22 };
static const unsigned int tdm_sclk1_m23_pins[]			= { GPIOM_23 };
static const unsigned int tdm_d0_m24_pins[]			= { GPIOM_24 };
static const unsigned int tdm_d1_m25_pins[]			= { GPIOM_25 };
static const unsigned int tdm_d2_m26_pins[]			= { GPIOM_26 };
static const unsigned int tdm_d3_m29_pins[]			= { GPIOM_29 };

/* GPIOM func3 */
static const unsigned int uart_a_tx_m0_pins[]			= { GPIOM_0 };
static const unsigned int uart_a_rx_m1_pins[]			= { GPIOM_1 };
static const unsigned int uart_a_cts_m2_pins[]			= { GPIOM_2 };
static const unsigned int uart_a_rts_m3_pins[]			= { GPIOM_3 };
static const unsigned int pwm_f_m10_pins[]			= { GPIOM_10 };
static const unsigned int spi_miso_a_m19_pins[]			= { GPIOM_19 };
static const unsigned int spi_mosi_a_m20_pins[]			= { GPIOM_20 };
static const unsigned int spi_clk_a_m21_pins[]			= { GPIOM_21 };
static const unsigned int spi_ss0_a_m22_pins[]			= { GPIOM_22 };
static const unsigned int spi_ss1_a_m23_pins[]			= { GPIOM_23 };
static const unsigned int spi_ss2_a_m24_pins[]			= { GPIOM_24 };
static const unsigned int i2c2_scl_m25_pins[]			= { GPIOM_25 };
static const unsigned int i2c2_sda_m26_pins[]			= { GPIOM_26 };
static const unsigned int i2c3_scl_m27_pins[]			= { GPIOM_27 };
static const unsigned int i2c3_sda_m28_pins[]			= { GPIOM_28 };

/* GPIOM func4 */
static const unsigned int pwm_d_m1_pins[]			= { GPIOM_1 };
static const unsigned int pwm_g_m8_pins[]			= { GPIOM_8 };
static const unsigned int pwm_h_m9_pins[]			= { GPIOM_9 };
static const unsigned int uart_c_tx_m19_pins[]			= { GPIOM_19 };
static const unsigned int uart_c_rx_m20_pins[]			= { GPIOM_20 };
static const unsigned int uart_c_cts_m21_pins[]			= { GPIOM_21 };
static const unsigned int uart_c_rts_m22_pins[]			= { GPIOM_22 };
static const unsigned int pwm_d_m23_pins[]			= { GPIOM_23 };
static const unsigned int pwm_e_m24_pins[]			= { GPIOM_24 };
static const unsigned int pwm_f_m26_pins[]			= { GPIOM_26 };
static const unsigned int gen_clk_out_m29_pins[]		= { GPIOM_29 };

/* GPIOM func5 */
static const unsigned int tsin_a_clk_m19_pins[]			= { GPIOM_19 };
static const unsigned int tsin_a_sop_m20_pins[]			= { GPIOM_20 };
static const unsigned int tsin_a_valid_m21_pins[]		= { GPIOM_21 };
static const unsigned int tsin_a_d0_m22_pins[]			= { GPIOM_22 };
static const unsigned int demod_uart_tx_m23_pins[]		= { GPIOM_23 };
static const unsigned int demod_uart_rx_m24_pins[]		= { GPIOM_24 };
static const unsigned int demod_uart_tx_m25_pins[]		= { GPIOM_25 };
static const unsigned int demod_uart_rx_m26_pins[]		= { GPIOM_26 };

static struct meson_pmx_group meson_t6d_periphs_groups[] = {
	/* func0 as GPIO */
	GPIO_GROUP(GPIOW_0,				0),
	GPIO_GROUP(GPIOW_1,				0),
	GPIO_GROUP(GPIOW_2,				0),
	GPIO_GROUP(GPIOW_3,				0),
	GPIO_GROUP(GPIOW_4,				0),
	GPIO_GROUP(GPIOW_5,				0),
	GPIO_GROUP(GPIOW_6,				0),
	GPIO_GROUP(GPIOW_7,				0),
	GPIO_GROUP(GPIOW_8,				0),
	GPIO_GROUP(GPIOW_9,				0),
	GPIO_GROUP(GPIOW_10,				0),
	GPIO_GROUP(GPIOW_11,				0),
	GPIO_GROUP(GPIOW_12,				0),
	GPIO_GROUP(GPIOD_0,				0),
	GPIO_GROUP(GPIOD_1,				0),
	GPIO_GROUP(GPIOD_2,				0),
	GPIO_GROUP(GPIOD_3,				0),
	GPIO_GROUP(GPIOD_4,				0),
	GPIO_GROUP(GPIOD_5,				0),
	GPIO_GROUP(GPIOD_6,				0),
	GPIO_GROUP(GPIOD_7,				0),
	GPIO_GROUP(GPIOD_8,				0),
	GPIO_GROUP(GPIOD_9,				0),
	GPIO_GROUP(GPIOD_10,				0),
	GPIO_GROUP(GPIOD_11,				0),
	GPIO_GROUP(GPIOD_12,				0),
	GPIO_GROUP(GPIOD_13,				0),
	GPIO_GROUP(GPIOD_14,				0),
	GPIO_GROUP(GPIOE_0,				0),
	GPIO_GROUP(GPIOE_1,				0),
	GPIO_GROUP(GPIOE_2,				0),
	GPIO_GROUP(GPIOB_0,				0),
	GPIO_GROUP(GPIOB_1,				0),
	GPIO_GROUP(GPIOB_2,				0),
	GPIO_GROUP(GPIOB_3,				0),
	GPIO_GROUP(GPIOB_4,				0),
	GPIO_GROUP(GPIOB_5,				0),
	GPIO_GROUP(GPIOB_6,				0),
	GPIO_GROUP(GPIOB_7,				0),
	GPIO_GROUP(GPIOB_8,				0),
	GPIO_GROUP(GPIOB_9,				0),
	GPIO_GROUP(GPIOB_10,				0),
	GPIO_GROUP(GPIOB_11,				0),
	GPIO_GROUP(GPIOB_12,				0),
	GPIO_GROUP(GPIOB_13,				0),
	GPIO_GROUP(GPIOC_0,				0),
	GPIO_GROUP(GPIOC_1,				0),
	GPIO_GROUP(GPIOC_2,				0),
	GPIO_GROUP(GPIOC_3,				0),
	GPIO_GROUP(GPIOC_4,				0),
	GPIO_GROUP(GPIOC_5,				0),
	GPIO_GROUP(GPIOC_6,				0),
	GPIO_GROUP(GPIOC_7,				0),
	GPIO_GROUP(GPIOC_8,				0),
	GPIO_GROUP(GPIOC_9,				0),
	GPIO_GROUP(GPIOC_10,				0),
	GPIO_GROUP(GPIOZ_0,				0),
	GPIO_GROUP(GPIOZ_1,				0),
	GPIO_GROUP(GPIOZ_2,				0),
	GPIO_GROUP(GPIOZ_3,				0),
	GPIO_GROUP(GPIOZ_4,				0),
	GPIO_GROUP(GPIOZ_5,				0),
	GPIO_GROUP(GPIOZ_6,				0),
	GPIO_GROUP(GPIOZ_7,				0),
	GPIO_GROUP(GPIOZ_8,				0),
	GPIO_GROUP(GPIOZ_9,				0),
	GPIO_GROUP(GPIOZ_10,				0),
	GPIO_GROUP(GPIOZ_11,				0),
	GPIO_GROUP(GPIOZ_12,				0),
	GPIO_GROUP(GPIOZ_13,				0),
	GPIO_GROUP(GPIOZ_14,				0),
	GPIO_GROUP(GPIOZ_15,				0),
	GPIO_GROUP(GPIOZ_16,				0),
	GPIO_GROUP(GPIOZ_17,				0),
	GPIO_GROUP(GPIOZ_18,				0),
	GPIO_GROUP(GPIOZ_19,				0),
	GPIO_GROUP(GPIOH_0,				0),
	GPIO_GROUP(GPIOH_1,				0),
	GPIO_GROUP(GPIOH_2,				0),
	GPIO_GROUP(GPIOH_3,				0),
	GPIO_GROUP(GPIOH_4,				0),
	GPIO_GROUP(GPIOH_5,				0),
	GPIO_GROUP(GPIOH_6,				0),
	GPIO_GROUP(GPIOH_7,				0),
	GPIO_GROUP(GPIOH_8,				0),
	GPIO_GROUP(GPIOH_9,				0),
	GPIO_GROUP(GPIOH_10,				0),
	GPIO_GROUP(GPIOH_11,				0),
	GPIO_GROUP(GPIOH_12,				0),
	GPIO_GROUP(GPIOH_13,				0),
	GPIO_GROUP(GPIOH_14,				0),
	GPIO_GROUP(GPIOH_15,				0),
	GPIO_GROUP(GPIOH_16,				0),
	GPIO_GROUP(GPIOH_17,				0),
	GPIO_GROUP(GPIOH_18,				0),
	GPIO_GROUP(GPIOH_19,				0),
	GPIO_GROUP(GPIOH_20,				0),
	GPIO_GROUP(GPIOH_21,				0),
	GPIO_GROUP(GPIOM_0,				0),
	GPIO_GROUP(GPIOM_1,				0),
	GPIO_GROUP(GPIOM_2,				0),
	GPIO_GROUP(GPIOM_3,				0),
	GPIO_GROUP(GPIOM_4,				0),
	GPIO_GROUP(GPIOM_5,				0),
	GPIO_GROUP(GPIOM_6,				0),
	GPIO_GROUP(GPIOM_7,				0),
	GPIO_GROUP(GPIOM_8,				0),
	GPIO_GROUP(GPIOM_9,				0),
	GPIO_GROUP(GPIOM_10,				0),
	GPIO_GROUP(GPIOM_11,				0),
	GPIO_GROUP(GPIOM_12,				0),
	GPIO_GROUP(GPIOM_13,				0),
	GPIO_GROUP(GPIOM_14,				0),
	GPIO_GROUP(GPIOM_15,				0),
	GPIO_GROUP(GPIOM_16,				0),
	GPIO_GROUP(GPIOM_17,				0),
	GPIO_GROUP(GPIOM_18,				0),
	GPIO_GROUP(GPIOM_19,				0),
	GPIO_GROUP(GPIOM_20,				0),
	GPIO_GROUP(GPIOM_21,				0),
	GPIO_GROUP(GPIOM_22,				0),
	GPIO_GROUP(GPIOM_23,				0),
	GPIO_GROUP(GPIOM_24,				0),
	GPIO_GROUP(GPIOM_25,				0),
	GPIO_GROUP(GPIOM_26,				0),
	GPIO_GROUP(GPIOM_27,				0),
	GPIO_GROUP(GPIOM_28,				0),
	GPIO_GROUP(GPIOM_29,				0),
	GPIO_GROUP(GPIO_TEST_N,				0),

	/* GPIOW func1 */
	GROUP(hdmirx_hpd_a,				1),
	GROUP(hdmirx_5vdet_a,				1),
	GROUP(hdmirx_sda_a,				1),
	GROUP(hdmirx_scl_a,				1),
	GROUP(hdmirx_hpd_b,				1),
	GROUP(hdmirx_5vdet_b,				1),
	GROUP(hdmirx_sda_b,				1),
	GROUP(hdmirx_scl_b,				1),
	GROUP(hdmirx_hpd_c,				1),
	GROUP(hdmirx_5vdet_c,				1),
	GROUP(hdmirx_sda_c,				1),
	GROUP(hdmirx_scl_c,				1),
	GROUP(cec,					1),

	/* GPIOW func2 */
	GROUP(uart_b_tx_w2,				2),
	GROUP(uart_b_rx_w3,				2),
	GROUP(uart_b_tx_w6,				2),
	GROUP(uart_b_rx_w7,				2),
	GROUP(uart_b_tx_w10,				2),
	GROUP(uart_b_rx_w11,				2),

	/* GPIOW func3 */
	GROUP(jtag_a_clk_w2,				3),
	GROUP(jtag_a_tms_w3,				3),
	GROUP(jtag_a_tdi_w6,				3),
	GROUP(jtag_a_tdo_w7,				3),

	/* GPIOW func4 */
	GROUP(gpiow3_loop,				4),
	GROUP(gpiow2_loop,				4),
	GROUP(jtag_a_clk_w6,				4),
	GROUP(jtag_a_tms_w7,				4),
	GROUP(jtag_a_tdi_w10,				4),
	GROUP(jtag_a_tdo_w11,				4),

	/* GPIOD func1 */
	GROUP(uart_b_tx_d0,				1),
	GROUP(uart_b_rx_d1,				1),
	GROUP(i2c1_scl_d2,				1),
	GROUP(i2c1_sda_d3,				1),
	GROUP(clk_32k_in,				1),
	GROUP(ir_in_a,					1),
	GROUP(jtag_a_clk_d6,				1),
	GROUP(jtag_a_tms_d7,				1),
	GROUP(jtag_a_tdi_d8,				1),
	GROUP(jtag_a_tdo_d9,				1),
	GROUP(clk12_24m_d10,				1),
	GROUP(pwm_a_d11,				1),
	GROUP(pwm_f_d12,				1),
	GROUP(i2c1_sda_d13,				1),
	GROUP(i2c1_scl_d14,				1),

	/* GPIOD func2 */
	GROUP(ir_out_d1,				2),
	GROUP(i2cs_a_scl,				2),
	GROUP(i2cs_a_sda,				2),
	GROUP(atv_if_agc_d4,				2),
	GROUP(pwm_c_d5,					2),
	GROUP(pwm_f_d6,					2),
	GROUP(pwm_c_d7,					2),
	GROUP(spdif_out_a_d8,				2),
	GROUP(pwm_d_d9,					2),
	GROUP(pwm_e_d10,				2),
	GROUP(pwm_c_d11,				2),

	/* GPIOD func3 */
	GROUP(cicam_waitn_d0,				3),
	GROUP(demod_uart_tx_d2,				3),
	GROUP(demod_uart_rx_d3,				3),
	GROUP(dtv_if_agc_d4,				3),
	GROUP(clk12_24m_d5,				3),
	GROUP(pwm_d_hiz,				3),
	GROUP(pwm_c_hiz,				3),
	GROUP(ir_out_d9,				3),
	GROUP(gen_clk_out_d10,				3),
	GROUP(ir_in_b_d14,				3),

	/* GPIOD func4 */
	GROUP(gen_clk_out_d5,				4),
	GROUP(uart_c_tx_d6,				4),
	GROUP(uart_c_rx_d7,				4),
	GROUP(tdm_d2_d8,				4),
	GROUP(tdm_d3_d9,				4),

	/* GPIOD func5 */
	GROUP(dcon_led_d7,				5),
	GROUP(dcon_led_d8,				5),
	GROUP(dcon_led_d9,				5),

	/* GPIOD func6 */
	GROUP(pwm_h_d6,					6),
	GROUP(mclk_1_d7,				6),
	GROUP(spdif_out_b_d8,				6),
	GROUP(pwm_vs_d9,				6),

	/* GPIOD func7 */
	GROUP(i2c3_scl_d6,				7),
	GROUP(i2c3_sda_d7,				7),

	/* GPIOE func1 */
	GROUP(pwm_a_e0,					1),
	GROUP(pwm_b_e1,					1),
	GROUP(pwm_e_e2,					1),

	/* GPIOE func2 */
	GROUP(i2c2_scl_e0,				2),
	GROUP(i2c2_sda_e1,				2),
	GROUP(clk12_24m_e2,				2),

	/* GPIOE func3 */
	GROUP(spdif_out_a_e2,				3),

	/* GPIOE func4 */
	GROUP(gen_clk_out_e2,				4),

	/* GPIOB func1 */
	GROUP(emmc_d0,					1),
	GROUP(emmc_d1,					1),
	GROUP(emmc_d2,					1),
	GROUP(emmc_d3,					1),
	GROUP(emmc_d4,					1),
	GROUP(emmc_d5,					1),
	GROUP(emmc_d6,					1),
	GROUP(emmc_d7,					1),
	GROUP(emmc_clk,					1),
	GROUP(emmc_cmd,					1),
	GROUP(emmc_ds,					1),
	GROUP(gpiob12_loop,				1),

	/* GPIOB func2 */
	GROUP(nand_wen_clk,				2),
	GROUP(nand_ale,					2),
	GROUP(nand_ren_wr,				2),
	GROUP(nand_cle,					2),
	GROUP(nand_ce0,					2),
	GROUP(gpioh21_loop,				2),

	/* GPIOB func3 */
	GROUP(spinf_hold_d3,				3),
	GROUP(spinf_mo_d0,				3),
	GROUP(spinf_mi_d1,				3),
	GROUP(spinf_clk,				3),
	GROUP(spinf_wp_d2,				3),
	GROUP(spinf_d4,					3),
	GROUP(spinf_d5,					3),
	GROUP(spinf_d6,					3),
	GROUP(spinf_d7,					3),
	GROUP(spinf_cs1,				3),
	GROUP(spinf_cs0,				3),

	/* GPIOB func4 */
	GROUP(ir_out_b12,				4),

	/* GPIOB func7 */
	GROUP(gen_clk_out_b6,				7),

	/* GPIOC func1 */
	GROUP(i2c3_scl_c0,				1),
	GROUP(i2c3_sda_c1,				1),
	GROUP(uart_a_tx_c6,				1),
	GROUP(uart_a_rx_c7,				1),
	GROUP(uart_a_cts_c8,				1),
	GROUP(uart_a_rts_c9,				1),
	GROUP(pwm_f_c10,				1),

	/* GPIOC func2 */
	GROUP(spi_miso_b_c0,				2),
	GROUP(spi_mosi_b_c1,				2),
	GROUP(spi_clk_b_c2,				2),
	GROUP(spi_ss0_b_c3,				2),
	GROUP(spi_ss1_b,				2),
	GROUP(spi_ss2_b,				2),
	GROUP(spi_clk_b_c6,				2),
	GROUP(spi_ss0_b_c7,				2),
	GROUP(mclk_1_c8,				2),
	GROUP(mclk_2_c9,				2),

	/* GPIOC func3 */
	GROUP(cicam_a2_c0,				3),
	GROUP(cicam_a3_c1,				3),
	GROUP(cicam_a4_c2,				3),
	GROUP(cicam_a5_c3,				3),
	GROUP(cicam_a6_c4,				3),
	GROUP(cicam_a7_c5,				3),
	GROUP(cicam_a8_c6,				3),
	GROUP(cicam_a9_c7,				3),
	GROUP(cicam_a10_c8,				3),
	GROUP(cicam_a11_c9,				3),
	GROUP(cicam_a12_c10,				3),

	/* GPIOC func4 */
	GROUP(demod_uart_tx_c6,				4),
	GROUP(demod_uart_rx_c7,				4),
	GROUP(pwm_vs_c10,				4),

	/* GPIOC func5 */
	GROUP(tdm_fs2_c2,				5),
	GROUP(tdm_sclk2_c3,				5),
	GROUP(tdm_d6_c4,				5),
	GROUP(tdm_d7_c5,				5),
	GROUP(tdm_fs1_c6,				5),
	GROUP(tdm_sclk1_c7,				5),
	GROUP(tdm_d4_c8,				5),
	GROUP(tdm_d5_c9,				5),

	/* GPIOC func6 */
	GROUP(gen_clk_out_c2,				6),

	/* GPIOC func7 */
	GROUP(dcon_led_c10,				7),

	/* GPIOZ func1 */
	GROUP(tdm_fs2_z0,				1),
	GROUP(tdm_sclk2_z1,				1),
	GROUP(tdm_d4_z2,				1),
	GROUP(tdm_d5_z3,				1),
	GROUP(tdm_d6_z4,				1),
	GROUP(tdm_d7_z5,				1),
	GROUP(mclk_2_z6,				1),
	GROUP(tsout_clk,				1),
	GROUP(tsout_sop,				1),
	GROUP(tsout_valid,				1),
	GROUP(tsout_d0,					1),
	GROUP(tsout_d1,					1),
	GROUP(tsout_d2,					1),
	GROUP(tsout_d3,					1),
	GROUP(tsout_d4,					1),
	GROUP(tsout_d5,					1),
	GROUP(tsout_d6,					1),
	GROUP(tsout_d7,					1),
	GROUP(dcon_led_z18,				1),
	GROUP(spdif_out_a_z19,				1),

	/* GPIOZ func2 */
	GROUP(tsin_a_clk_z0,				2),
	GROUP(tsin_a_sop_z1,				2),
	GROUP(tsin_a_valid_z2,				2),
	GROUP(tsin_a_d0_z3,				2),
	GROUP(dtv_rf_agc_z6,				2),
	GROUP(cicam_a2_z8,				2),
	GROUP(cicam_a3_z9,				2),
	GROUP(cicam_a4_z10,				2),
	GROUP(cicam_a5_z11,				2),
	GROUP(cicam_a6_z12,				2),
	GROUP(cicam_a7_z13,				2),
	GROUP(cicam_a8_z14,				2),
	GROUP(cicam_a9_z15,				2),
	GROUP(cicam_a10_z16,				2),
	GROUP(cicam_a11_z17,				2),
	GROUP(dtv_rf_agc_z18,				2),

	/* GPIOZ func3 */
	GROUP(iso7816_clk_z0,				3),
	GROUP(iso7816_data_z1,				3),
	GROUP(cicam_a13,				3),
	GROUP(cicam_a14,				3),
	GROUP(i2c0_scl,				3),
	GROUP(i2c0_sda,				3),
	GROUP(atv_if_agc_z6,				3),
	GROUP(atv_if_agc_z18,				3),
	GROUP(spdif_out_b_z19,				3),

	/* GPIOZ func4 */
	GROUP(spi_miso_b_z0,				4),
	GROUP(spi_mosi_b_z1,				4),
	GROUP(spi_clk_b_z2,				4),
	GROUP(spi_ss0_b_z3,				4),
	GROUP(dtv_if_agc_z6,				4),
	GROUP(dtv_if_agc_z18,				4),
	GROUP(pwm_g_z19,				4),

	/* GPIOZ func5 */
	GROUP(uart_a_tx_z0,				5),
	GROUP(uart_a_rx_z1,				5),
	GROUP(uart_a_cts_z2,				5),
	GROUP(uart_a_rts_z3,				5),
	GROUP(pwm_b_z4,					5),
	GROUP(pwm_d_z5,					5),
	GROUP(pwm_e_z6,					5),
	GROUP(pwm_g_z12,				5),
	GROUP(pwm_h_z13,				5),
	GROUP(gen_clk_out_z16,				5),
	GROUP(ir_in_b_z18,				5),
	GROUP(ir_in_b_z19,				5),

	/* GPIOZ func6 */
	GROUP(cicam_a12_z0,				6),
	GROUP(pdm_din2_z1,				6),
	GROUP(pdm_dclk_z2,				6),
	GROUP(pdm_din0_z3,				6),
	GROUP(pdm_din1_z6,				6),
	GROUP(tdm_d1_z10,				6),
	GROUP(tdm_d0_z11,				6),
	GROUP(tdm_sclk1_z16,				6),
	GROUP(tdm_fs1_z17,				6),
	GROUP(pwm_vs_z18,				6),
	GROUP(diseqc_out_z19,				6),

	/* GPIOZ func7 */
	GROUP(diseqc_out_z0,				7),
	GROUP(s2_demod_gpio0_z1,			7),
	GROUP(spdif_out_a_z2,				7),
	GROUP(eth_link_led_z6,				7),
	GROUP(eth_act_led_z19,				7),

	/* GPIOH func1 */
	GROUP(tcon_0,					1),
	GROUP(tcon_1,					1),
	GROUP(tcon_2,					1),
	GROUP(tcon_3,					1),
	GROUP(tcon_4,					1),
	GROUP(tcon_5,					1),
	GROUP(tcon_6,					1),
	GROUP(tcon_7,					1),
	GROUP(tcon_8,					1),
	GROUP(tcon_9,					1),
	GROUP(tcon_10,					1),
	GROUP(tcon_11,					1),
	GROUP(tcon_12,					1),
	GROUP(tcon_13,					1),
	GROUP(tcon_14,					1),
	GROUP(tcon_15,					1),
	GROUP(hsync,					1),
	GROUP(vsync,					1),
	GROUP(i2c2_scl_h20,				1),
	GROUP(i2c2_sda_h21,				1),

	/* GPIOH func2 */
	GROUP(i2c2_scl_h2,				2),
	GROUP(i2c2_sda_h3,				2),
	GROUP(sync_3d_out,				2),
	GROUP(tdm_d3_h6,				2),
	GROUP(spi_ss1_a_h7,				2),
	GROUP(spi_ss0_a_h8,				2),
	GROUP(spi_miso_a_h9,				2),
	GROUP(spi_mosi_a_h10,				2),
	GROUP(spi_clk_a_h11,				2),
	GROUP(spi_ss2_a_h12,				2),
	GROUP(tdm_d3_h13,				2),
	GROUP(mclk_1_h14,				2),
	GROUP(tdm_fs1_h15,				2),
	GROUP(tdm_sclk1_h16,				2),
	GROUP(tdm_d0_h17,				2),
	GROUP(tdm_d1_h18,				2),
	GROUP(tdm_d2_h19,				2),
	GROUP(tdm_d3_h20,				2),
	GROUP(tdm_d4_h21,				2),

	/* GPIOH func3 */
	GROUP(uart_c_tx_h2,				3),
	GROUP(uart_c_rx_h3,				3),
	GROUP(uart_c_cts_h4,				3),
	GROUP(uart_c_rts_h5,				3),
	GROUP(i2c2_scl_h10,				3),
	GROUP(i2c2_sda_h11,				3),
	GROUP(pwm_vs_h12,				3),
	GROUP(pwm_vs_h13,				3),
	GROUP(pdm_din1_h14,				3),
	GROUP(pdm_din2_h16,				3),
	GROUP(pdm_din3_h17,				3),
	GROUP(pdm_din0_h18,				3),
	GROUP(pdm_dclk_h19,				3),
	GROUP(gpiob13_loop,				3),

	/* GPIOH func4 */
	GROUP(pwm_d_h5,					4),
	GROUP(pwm_d_h12,				4),
	GROUP(pwm_e_h13,				4),
	GROUP(tdm_d3_h14,				4),
	GROUP(eth_act_led_h18,				4),
	GROUP(eth_link_led_h19,				4),

	/* GPIOH func5 */
	GROUP(clk12_24m_h13,				5),
	GROUP(spi_clk_a_h14,				5),
	GROUP(tdm_fs2_h18,				5),
	GROUP(tdm_sclk2_h19,				5),
	GROUP(demod_uart_tx_h20,			5),
	GROUP(demod_uart_rx_h21,			5),

	/* GPIOH func6 */
	GROUP(gen_clk_out_h13,				6),
	GROUP(s2_demod_gpio0_h14,			6),
	GROUP(s2_demod_gpio1,				6),
	GROUP(s2_demod_gpio2,				6),
	GROUP(s2_demod_gpio3,				6),
	GROUP(s2_demod_gpio4,				6),
	GROUP(s2_demod_gpio5,				6),
	GROUP(s2_demod_gpio6,				6),
	GROUP(s2_demod_gpio7,				6),

	/* GPIOH func7 */
	GROUP(gen_clk_out_h0,				7),

	/* GPIOM func1 */
	GROUP(tsin_b_clk,				1),
	GROUP(tsin_b_sop,				1),
	GROUP(tsin_b_valid,				1),
	GROUP(tsin_b_d0,				1),
	GROUP(tsin_b_d1,				1),
	GROUP(tsin_b_d2,				1),
	GROUP(tsin_b_d3,				1),
	GROUP(tsin_b_d4,				1),
	GROUP(tsin_b_d5,				1),
	GROUP(tsin_b_d6,				1),
	GROUP(tsin_b_d7,				1),
	GROUP(cicam_data0,				1),
	GROUP(cicam_data1,				1),
	GROUP(cicam_data2,				1),
	GROUP(cicam_data3,				1),
	GROUP(cicam_data4,				1),
	GROUP(cicam_data5,				1),
	GROUP(cicam_data6,				1),
	GROUP(cicam_data7,				1),
	GROUP(cicam_a0,					1),
	GROUP(cicam_a1,					1),
	GROUP(cicam_cen,				1),
	GROUP(cicam_oen,				1),
	GROUP(cicam_wen,				1),
	GROUP(cicam_iordn,				1),
	GROUP(cicam_iowrn,				1),
	GROUP(cicam_reset,				1),
	GROUP(cicam_waitn_m29,				1),

	/* GPIOM func2 */
	GROUP(pdm_din2_m8,				2),
	GROUP(pdm_din3_m9,				2),
	GROUP(pdm_din0_m10,				2),
	GROUP(pdm_dclk_m11,				2),
	GROUP(mclk_2_m12,				2),
	GROUP(tdm_fs2_m13,				2),
	GROUP(tdm_sclk2_m14,				2),
	GROUP(tdm_d4_m15,				2),
	GROUP(tdm_d5_m16,				2),
	GROUP(tdm_d6_m17,				2),
	GROUP(tdm_d7_m18,				2),
	GROUP(iso7816_clk_m19,				2),
	GROUP(iso7816_data_m20,				2),
	GROUP(mclk_1_m21,				2),
	GROUP(tdm_fs1_m22,				2),
	GROUP(tdm_sclk1_m23,				2),
	GROUP(tdm_d0_m24,				2),
	GROUP(tdm_d1_m25,				2),
	GROUP(tdm_d2_m26,				2),
	GROUP(tdm_d3_m29,				2),

	/* GPIOM func3 */
	GROUP(uart_a_tx_m0,				3),
	GROUP(uart_a_rx_m1,				3),
	GROUP(uart_a_cts_m2,				3),
	GROUP(uart_a_rts_m3,				3),
	GROUP(pwm_f_m10,				3),
	GROUP(spi_miso_a_m19,				3),
	GROUP(spi_mosi_a_m20,				3),
	GROUP(spi_clk_a_m21,				3),
	GROUP(spi_ss0_a_m22,				3),
	GROUP(spi_ss1_a_m23,				3),
	GROUP(spi_ss2_a_m24,				3),
	GROUP(i2c2_scl_m25,				3),
	GROUP(i2c2_sda_m26,				3),
	GROUP(i2c3_scl_m27,				3),
	GROUP(i2c3_sda_m28,				3),

	/* GPIOM func4 */
	GROUP(pwm_d_m1,					4),
	GROUP(pwm_g_m8,					4),
	GROUP(pwm_h_m9,					4),
	GROUP(uart_c_tx_m19,				4),
	GROUP(uart_c_rx_m20,				4),
	GROUP(uart_c_cts_m21,				4),
	GROUP(uart_c_rts_m22,				4),
	GROUP(pwm_d_m23,				4),
	GROUP(pwm_e_m24,				4),
	GROUP(pwm_f_m26,				4),
	GROUP(gen_clk_out_m29,				4),

	/* GPIOM func5 */
	GROUP(tsin_a_clk_m19,				5),
	GROUP(tsin_a_sop_m20,				5),
	GROUP(tsin_a_valid_m21,				5),
	GROUP(tsin_a_d0_m22,				5),
	GROUP(demod_uart_tx_m23,			5),
	GROUP(demod_uart_rx_m24,			5),
	GROUP(demod_uart_tx_m25,			5),
	GROUP(demod_uart_rx_m26,			5),
};

static const char * const gpio_periphs_groups[] = {
	"GPIOW_0", "GPIOW_1", "GPIOW_2", "GPIOW_3",
	"GPIOW_4", "GPIOW_5", "GPIOW_6", "GPIOW_7",
	"GPIOW_8", "GPIOW_9", "GPIOW_10", "GPIOW_11",
	"GPIOW_12", "GPIOD_0", "GPIOD_1", "GPIOD_2",
	"GPIOD_3", "GPIOD_4", "GPIOD_5", "GPIOD_6",
	"GPIOD_7", "GPIOD_8", "GPIOD_9", "GPIOD_10",
	"GPIOD_11", "GPIOD_12", "GPIOD_13", "GPIOD_14",
	"GPIOE_0", "GPIOE_1", "GPIOE_2", "GPIOB_0",
	"GPIOB_1", "GPIOB_2", "GPIOB_3", "GPIOB_4",
	"GPIOB_5", "GPIOB_6", "GPIOB_7", "GPIOB_8",
	"GPIOB_9", "GPIOB_10", "GPIOB_11", "GPIOB_12",
	"GPIOB_13", "GPIOC_0", "GPIOC_1", "GPIOC_2",
	"GPIOC_3", "GPIOC_4", "GPIOC_5", "GPIOC_6",
	"GPIOC_7", "GPIOC_8", "GPIOC_9", "GPIOC_10",
	"GPIOZ_0", "GPIOZ_1", "GPIOZ_2", "GPIOZ_3",
	"GPIOZ_4", "GPIOZ_5", "GPIOZ_6", "GPIOZ_7",
	"GPIOZ_8", "GPIOZ_9", "GPIOZ_10", "GPIOZ_11",
	"GPIOZ_12", "GPIOZ_13", "GPIOZ_14", "GPIOZ_15",
	"GPIOZ_16", "GPIOZ_17", "GPIOZ_18", "GPIOZ_19",
	"GPIOH_0", "GPIOH_1", "GPIOH_2", "GPIOH_3",
	"GPIOH_4", "GPIOH_5", "GPIOH_6", "GPIOH_7",
	"GPIOH_8", "GPIOH_9", "GPIOH_10", "GPIOH_11",
	"GPIOH_12", "GPIOH_13", "GPIOH_14", "GPIOH_15",
	"GPIOH_16", "GPIOH_17", "GPIOH_18", "GPIOH_19",
	"GPIOH_20", "GPIOH_21", "GPIOM_0", "GPIOM_1",
	"GPIOM_2", "GPIOM_3", "GPIOM_4", "GPIOM_5",
	"GPIOM_6", "GPIOM_7", "GPIOM_8", "GPIOM_9",
	"GPIOM_10", "GPIOM_11", "GPIOM_12", "GPIOM_13",
	"GPIOM_14", "GPIOM_15", "GPIOM_16", "GPIOM_17",
	"GPIOM_18", "GPIOM_19", "GPIOM_20", "GPIOM_21",
	"GPIOM_22", "GPIOM_23", "GPIOM_24", "GPIOM_25",
	"GPIOM_26", "GPIOM_27", "GPIOM_28", "GPIOM_29",
	"GPIO_TEST_N",
};

static const char * const sync_3d_out_groups[] = {
	"sync_3d_out",
};

static const char * const atv_if_agc_groups[] = {
	"atv_if_agc_d4", "atv_if_agc_z6", "atv_if_agc_z18",
};

static const char * const cec_groups[] = {
	"cec",
};

static const char * const cicam_groups[] = {
	"cicam_waitn_d0", "cicam_a2_c0", "cicam_a3_c1", "cicam_a4_c2",
	"cicam_a5_c3", "cicam_a6_c4", "cicam_a7_c5", "cicam_a8_c6",
	"cicam_a9_c7", "cicam_a10_c8", "cicam_a11_c9", "cicam_a12_c10",
	"cicam_a12_z0", "cicam_a13", "cicam_a14", "cicam_a2_z8",
	"cicam_a3_z9", "cicam_a4_z10", "cicam_a5_z11", "cicam_a6_z12",
	"cicam_a7_z13", "cicam_a8_z14", "cicam_a9_z15", "cicam_a10_z16",
	"cicam_a11_z17", "cicam_data0", "cicam_data1", "cicam_data2",
	"cicam_data3", "cicam_data4", "cicam_data5", "cicam_data6",
	"cicam_data7", "cicam_a0", "cicam_a1", "cicam_cen",
	"cicam_oen", "cicam_wen", "cicam_iordn", "cicam_iowrn",
	"cicam_reset", "cicam_waitn_m29",
};

static const char * const clk12_24m_groups[] = {
	"clk12_24m_d5", "clk12_24m_d10", "clk12_24m_e2", "clk12_24m_h13",
};

static const char * const clk_32k_in_groups[] = {
	"clk_32k_in",
};

static const char * const dcon_led_groups[] = {
	"dcon_led_d7", "dcon_led_d8", "dcon_led_d9", "dcon_led_c10",
	"dcon_led_z18",
};

static const char * const demod_uart_groups[] = {
	"demod_uart_tx_d2", "demod_uart_rx_d3", "demod_uart_tx_c6",
	"demod_uart_rx_c7", "demod_uart_tx_h20", "demod_uart_rx_h21",
	"demod_uart_tx_m23", "demod_uart_rx_m24", "demod_uart_tx_m25",
	"demod_uart_rx_m26",
};

static const char * const diseqc_out_groups[] = {
	"diseqc_out_z0", "diseqc_out_z19",
};

static const char * const dtv_groups[] = {
	"dtv_if_agc_d4", "dtv_rf_agc_z6", "dtv_if_agc_z6", "dtv_rf_agc_z18",
	"dtv_if_agc_z18",
};

static const char * const emmc_groups[] = {
	"emmc_d0", "emmc_d1", "emmc_d2", "emmc_d3",
	"emmc_d4", "emmc_d5", "emmc_d6", "emmc_d7",
	"emmc_clk", "emmc_cmd", "emmc_ds",
};

static const char * const eth_groups[] = {
	"eth_link_led_z6", "eth_act_led_z19", "eth_act_led_h18", "eth_link_led_h19",
};

static const char * const gen_clk_out_groups[] = {
	"gen_clk_out_d5", "gen_clk_out_d10", "gen_clk_out_e2", "gen_clk_out_b6",
	"gen_clk_out_c2", "gen_clk_out_z16", "gen_clk_out_h0", "gen_clk_out_h13",
	"gen_clk_out_m29",
};

static const char * const gpiob12_loop_groups[] = {
	"gpiob12_loop",
};

static const char * const gpiob13_loop_groups[] = {
	"gpiob13_loop",
};

static const char * const gpioh21_loop_groups[] = {
	"gpioh21_loop",
};

static const char * const gpiow2_loop_groups[] = {
	"gpiow2_loop",
};

static const char * const gpiow3_loop_groups[] = {
	"gpiow3_loop",
};

static const char * const hdmirx_groups[] = {
	"hdmirx_hpd_a", "hdmirx_5vdet_a", "hdmirx_sda_a", "hdmirx_scl_a",
	"hdmirx_hpd_b", "hdmirx_5vdet_b", "hdmirx_sda_b", "hdmirx_scl_b",
	"hdmirx_hpd_c", "hdmirx_5vdet_c", "hdmirx_sda_c", "hdmirx_scl_c",
};

static const char * const hsync_groups[] = {
	"hsync",
};

static const char * const i2c0_groups[] = {
	"i2c0_scl", "i2c0_sda",
};

static const char * const i2c1_groups[] = {
	"i2c1_scl_d2", "i2c1_sda_d3", "i2c1_sda_d13", "i2c1_scl_d14",
};

static const char * const i2c2_groups[] = {
	"i2c2_scl_e0", "i2c2_sda_e1", "i2c2_scl_h2", "i2c2_sda_h3",
	"i2c2_scl_h10", "i2c2_sda_h11", "i2c2_scl_h20", "i2c2_sda_h21",
	"i2c2_scl_m25", "i2c2_sda_m26",
};

static const char * const i2c3_groups[] = {
	"i2c3_scl_d6", "i2c3_sda_d7", "i2c3_scl_c0", "i2c3_sda_c1",
	"i2c3_scl_m27", "i2c3_sda_m28",
};

static const char * const i2cs_a_groups[] = {
	"i2cs_a_scl", "i2cs_a_sda",
};

static const char * const ir_groups[] = {
	"ir_out_d1", "ir_in_a", "ir_out_d9", "ir_in_b_d14",
	"ir_out_b12", "ir_in_b_z18", "ir_in_b_z19",
};

static const char * const iso7816_groups[] = {
	"iso7816_clk_z0", "iso7816_data_z1", "iso7816_clk_m19", "iso7816_data_m20",
};

static const char * const jtag_a_groups[] = {
	"jtag_a_clk_w2", "jtag_a_tms_w3", "jtag_a_tdi_w6", "jtag_a_clk_w6",
	"jtag_a_tdo_w7", "jtag_a_tms_w7", "jtag_a_tdi_w10", "jtag_a_tdo_w11",
	"jtag_a_clk_d6", "jtag_a_tms_d7", "jtag_a_tdi_d8", "jtag_a_tdo_d9",
};

static const char * const mclk_groups[] = {
	"mclk_1_d7", "mclk_1_c8", "mclk_2_c9", "mclk_2_z6",
	"mclk_1_h14", "mclk_2_m12", "mclk_1_m21",
};

static const char * const nand_groups[] = {
	"emmc_d0", "emmc_d1", "emmc_d2", "emmc_d3",
	"emmc_d4", "emmc_d5", "emmc_d6", "emmc_d7",
	"nand_wen_clk", "nand_ale", "nand_ren_wr", "nand_cle",
	"nand_ce0",
};

static const char * const pdm_groups[] = {
	"pdm_din2_z1", "pdm_dclk_z2", "pdm_din0_z3", "pdm_din1_z6",
	"pdm_din1_h14", "pdm_din2_h16", "pdm_din3_h17", "pdm_din0_h18",
	"pdm_dclk_h19", "pdm_din2_m8", "pdm_din3_m9", "pdm_din0_m10",
	"pdm_dclk_m11",
};

static const char * const pwm_a_groups[] = {
	"pwm_a_d11", "pwm_a_e0",
};

static const char * const pwm_b_groups[] = {
	"pwm_b_e1", "pwm_b_z4",
};

static const char * const pwm_c_groups[] = {
	"pwm_c_d5", "pwm_c_d7", "pwm_c_hiz", "pwm_c_d11",
};

static const char * const pwm_d_groups[] = {
	"pwm_d_hiz", "pwm_d_d9", "pwm_d_z5", "pwm_d_h5",
	"pwm_d_h12", "pwm_d_m1", "pwm_d_m23",
};

static const char * const pwm_e_groups[] = {
	"pwm_e_d10", "pwm_e_e2", "pwm_e_z6", "pwm_e_h13",
	"pwm_e_m24",
};

static const char * const pwm_f_groups[] = {
	"pwm_f_d6", "pwm_f_d12", "pwm_f_c10", "pwm_f_m10",
	"pwm_f_m26",
};

static const char * const pwm_g_groups[] = {
	"pwm_g_z12", "pwm_g_z19", "pwm_g_m8",
};

static const char * const pwm_h_groups[] = {
	"pwm_h_d6", "pwm_h_z13", "pwm_h_m9",
};

static const char * const pwm_vs_groups[] = {
	"pwm_vs_d9", "pwm_vs_c10", "pwm_vs_z18", "pwm_vs_h12",
	"pwm_vs_h13",
};

static const char * const s2_demod_groups[] = {
	"s2_demod_gpio0_z1", "s2_demod_gpio0_h14", "s2_demod_gpio1",
	"s2_demod_gpio2", "s2_demod_gpio3", "s2_demod_gpio4",
	"s2_demod_gpio5", "s2_demod_gpio6", "s2_demod_gpio7",
};

static const char * const spdif_groups[] = {
	"spdif_out_a_d8", "spdif_out_b_d8", "spdif_out_a_e2", "spdif_out_a_z2",
	"spdif_out_a_z19", "spdif_out_b_z19",
};

static const char * const spinf_groups[] = {
	"spinf_hold_d3", "spinf_mo_d0", "spinf_mi_d1", "spinf_clk",
	"spinf_wp_d2", "spinf_d4", "spinf_d5", "spinf_d6",
	"spinf_d7", "spinf_cs1", "spinf_cs0",
};

static const char * const spi_groups[] = {
	"spi_miso_b_c0", "spi_mosi_b_c1", "spi_clk_b_c2", "spi_ss0_b_c3",
	"spi_ss1_b", "spi_ss2_b", "spi_clk_b_c6", "spi_ss0_b_c7",
	"spi_miso_b_z0", "spi_mosi_b_z1", "spi_clk_b_z2", "spi_ss0_b_z3",
	"spi_ss1_a_h7", "spi_ss0_a_h8", "spi_miso_a_h9", "spi_mosi_a_h10",
	"spi_clk_a_h11", "spi_ss2_a_h12", "spi_clk_a_h14", "spi_miso_a_m19",
	"spi_mosi_a_m20", "spi_clk_a_m21", "spi_ss0_a_m22", "spi_ss1_a_m23",
	"spi_ss2_a_m24",
};

static const char * const tcon_groups[] = {
	"tcon_0", "tcon_1", "tcon_2", "tcon_3",
	"tcon_4", "tcon_5", "tcon_6", "tcon_7",
	"tcon_8", "tcon_9", "tcon_10", "tcon_11",
	"tcon_12", "tcon_13", "tcon_14", "tcon_15",
};

static const char * const tdm_groups[] = {
	"tdm_d2_d8", "tdm_d3_d9", "tdm_fs2_c2", "tdm_sclk2_c3",
	"tdm_d6_c4", "tdm_d7_c5", "tdm_fs1_c6", "tdm_sclk1_c7",
	"tdm_d4_c8", "tdm_d5_c9", "tdm_fs2_z0", "tdm_sclk2_z1",
	"tdm_d4_z2", "tdm_d5_z3", "tdm_d6_z4", "tdm_d7_z5",
	"tdm_d1_z10", "tdm_d0_z11", "tdm_sclk1_z16", "tdm_fs1_z17",
	"tdm_d3_h6", "tdm_d3_h13", "tdm_d3_h14", "tdm_fs1_h15",
	"tdm_sclk1_h16", "tdm_d0_h17", "tdm_d1_h18", "tdm_fs2_h18",
	"tdm_d2_h19", "tdm_sclk2_h19", "tdm_d3_h20", "tdm_d4_h21",
	"tdm_fs2_m13", "tdm_sclk2_m14", "tdm_d4_m15", "tdm_d5_m16",
	"tdm_d6_m17", "tdm_d7_m18", "tdm_fs1_m22", "tdm_sclk1_m23",
	"tdm_d0_m24", "tdm_d1_m25", "tdm_d2_m26", "tdm_d3_m29",
};

static const char * const tsin_groups[] = {
	"tsin_a_clk_z0", "tsin_a_sop_z1", "tsin_a_valid_z2", "tsin_a_d0_z3",
	"tsin_b_clk", "tsin_b_sop", "tsin_b_valid", "tsin_b_d0",
	"tsin_b_d1", "tsin_b_d2", "tsin_b_d3", "tsin_b_d4",
	"tsin_b_d5", "tsin_b_d6", "tsin_b_d7", "tsin_a_clk_m19",
	"tsin_a_sop_m20", "tsin_a_valid_m21", "tsin_a_d0_m22",
};

static const char * const tsout_groups[] = {
	"tsout_clk", "tsout_sop", "tsout_valid", "tsout_d0",
	"tsout_d1", "tsout_d2", "tsout_d3", "tsout_d4",
	"tsout_d5", "tsout_d6", "tsout_d7",
};

static const char * const uart_a_groups[] = {
	"uart_a_tx_c6", "uart_a_rx_c7", "uart_a_cts_c8", "uart_a_rts_c9",
	"uart_a_tx_z0", "uart_a_rx_z1", "uart_a_cts_z2", "uart_a_rts_z3",
	"uart_a_tx_m0", "uart_a_rx_m1", "uart_a_cts_m2", "uart_a_rts_m3",
};

static const char * const uart_b_groups[] = {
	"uart_b_tx_w2", "uart_b_rx_w3", "uart_b_tx_w6", "uart_b_rx_w7",
	"uart_b_tx_w10", "uart_b_rx_w11", "uart_b_tx_d0", "uart_b_rx_d1",
};

static const char * const uart_c_groups[] = {
	"uart_c_tx_d6", "uart_c_rx_d7", "uart_c_tx_h2", "uart_c_rx_h3",
	"uart_c_cts_h4", "uart_c_rts_h5", "uart_c_tx_m19", "uart_c_rx_m20",
	"uart_c_cts_m21", "uart_c_rts_m22",
};

static const char * const vsync_groups[] = {
	"vsync",
};

static struct meson_pmx_func meson_t6d_periphs_functions[] = {
	FUNCTION(gpio_periphs),
	FUNCTION(sync_3d_out),
	FUNCTION(atv_if_agc),
	FUNCTION(cec),
	FUNCTION(cicam),
	FUNCTION(clk12_24m),
	FUNCTION(clk_32k_in),
	FUNCTION(dcon_led),
	FUNCTION(demod_uart),
	FUNCTION(diseqc_out),
	FUNCTION(dtv),
	FUNCTION(emmc),
	FUNCTION(eth),
	FUNCTION(gen_clk_out),
	FUNCTION(gpiob12_loop),
	FUNCTION(gpiob13_loop),
	FUNCTION(gpioh21_loop),
	FUNCTION(gpiow2_loop),
	FUNCTION(gpiow3_loop),
	FUNCTION(hdmirx),
	FUNCTION(hsync),
	FUNCTION(i2c0),
	FUNCTION(i2c1),
	FUNCTION(i2c2),
	FUNCTION(i2c3),
	FUNCTION(i2cs_a),
	FUNCTION(ir),
	FUNCTION(iso7816),
	FUNCTION(jtag_a),
	FUNCTION(mclk),
	FUNCTION(nand),
	FUNCTION(pdm),
	FUNCTION(pwm_a),
	FUNCTION(pwm_b),
	FUNCTION(pwm_c),
	FUNCTION(pwm_d),
	FUNCTION(pwm_e),
	FUNCTION(pwm_f),
	FUNCTION(pwm_g),
	FUNCTION(pwm_h),
	FUNCTION(pwm_vs),
	FUNCTION(s2_demod),
	FUNCTION(spdif),
	FUNCTION(spinf),
	FUNCTION(spi),
	FUNCTION(tcon),
	FUNCTION(tdm),
	FUNCTION(tsin),
	FUNCTION(tsout),
	FUNCTION(uart_a),
	FUNCTION(uart_b),
	FUNCTION(uart_c),
	FUNCTION(vsync),
};

static struct meson_bank meson_t6d_periphs_banks[] = {
	/*  name  first  last  pullen  pull  dir  out  in  ds  (ds_ex  ds_div) */
	BANK_DS("W",  GPIOW_0, GPIOW_12,
		0x063,  0, 0x064,  0, 0x062,  0, 0x061,  0, 0x060,  0, 0x067,  0),
	BANK_DS("D",  GPIOD_0, GPIOD_14,
		0x003,  0, 0x004,  0, 0x002,  0, 0x001,  0, 0x000,  0, 0x007,  0),
	BANK_DS("E",  GPIOE_0,  GPIOE_2,
		0x00b,  0, 0x00c,  0, 0x00a,  0, 0x009,  0, 0x008,  0, 0x00f,  0),
	BANK_DS("B",  GPIOB_0, GPIOB_13,
		0x02b,  0, 0x02c,  0, 0x02a,  0, 0x029,  0, 0x028,  0, 0x02f,  0),
	BANK_DS("C",  GPIOC_0, GPIOC_10,
		0x033,  0, 0x034,  0, 0x032,  0, 0x031,  0, 0x030,  0, 0x037,  0),
	BANK_DS_EX("Z",  GPIOZ_0,  GPIOZ_19,     0x013,  0, 0x014,  0,
		0x012,  0, 0x011,  0, 0x010,  0, 0x017,  0, 0x0c0,  0, GPIOZ_16),
	BANK_DS_EX("H",   GPIOH_0, GPIOH_21,     0x01b,  0, 0x01c,  0,
		0x01a,  0, 0x019,  0, 0x018,  0, 0x01f,  0, 0x0c1,  0, GPIOH_16),
	BANK_DS_EX("M",   GPIOM_0, GPIOM_29,     0x073,  0, 0x074,  0,
		0x072,  0, 0x071,  0, 0x070,  0, 0x077,  0, 0x0c2,  0, GPIOM_16),
	BANK_DS("TEST_N", GPIO_TEST_N, GPIO_TEST_N,
		0x083,  0, 0x084,  0, 0x082,  0, 0x081,  0, 0x080,  0, 0x087,  0),
};

static struct meson_pmx_bank meson_t6d_periphs_pmx_banks[] = {
	/*       name      first        last         reg   offset */
	BANK_PMX("W",      GPIOW_0,     GPIOW_12,    0x012,  0),
	BANK_PMX("D",      GPIOD_0,     GPIOD_14,    0x00b,  0),
	BANK_PMX("E",      GPIOE_0,      GPIOE_2,    0x00d,  0),
	BANK_PMX("B",      GPIOB_0,     GPIOB_13,    0x000,  0),
	BANK_PMX("C",      GPIOC_0,     GPIOC_10,    0x002,  0),
	BANK_PMX("Z",      GPIOZ_0,     GPIOZ_19,    0x004,  0),
	BANK_PMX("H",      GPIOH_0,     GPIOH_21,    0x007,  0),
	BANK_PMX("M",      GPIOM_0,     GPIOM_29,    0x00e,  0),
	BANK_PMX("TEST_N", GPIO_TEST_N, GPIO_TEST_N, 0x014,  0),
};

static struct meson_axg_pmx_data meson_t6d_periphs_pmx_banks_data = {
	.pmx_banks	= meson_t6d_periphs_pmx_banks,
	.num_pmx_banks	= ARRAY_SIZE(meson_t6d_periphs_pmx_banks),
};

static int meson_t6d_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_ds = pc->reg_gpio;

	return 0;
}

static struct meson_pinctrl_data meson_t6d_periphs_pinctrl_data = {
	.name		= "periphs-banks",
	.groups		= meson_t6d_periphs_groups,
	.funcs		= meson_t6d_periphs_functions,
	.banks		= meson_t6d_periphs_banks,
	.num_pins	= 129,
	.num_groups	= ARRAY_SIZE(meson_t6d_periphs_groups),
	.num_funcs	= ARRAY_SIZE(meson_t6d_periphs_functions),
	.num_banks	= ARRAY_SIZE(meson_t6d_periphs_banks),
	.gpio_driver	= &meson_axg_gpio_driver,
	.pmx_data	= &meson_t6d_periphs_pmx_banks_data,
	.parse_dt	= meson_t6d_parse_dt_extra,
};

static const struct udevice_id meson_t6d_pinctrl_match[] = {
	{
		.compatible = "amlogic,meson-t6d-periphs-pinctrl",
		.data = (ulong)&meson_t6d_periphs_pinctrl_data,
	},
	{
		.compatible = "amlogic,meson-t6d-analog-pinctrl",
		.data = (ulong)&meson_t6d_analog_pinctrl_data,
	},
	{ }
};

U_BOOT_DRIVER(meson_t6d_pinctrl) = {
	.name	= "meson-t6d-pinctrl",
	.id	= UCLASS_PINCTRL,
	.of_match = of_match_ptr(meson_t6d_pinctrl_match),
	.probe = meson_pinctrl_probe,
	.priv_auto = sizeof(struct meson_pinctrl),
	.ops = &meson_axg_pinctrl_ops,
};
