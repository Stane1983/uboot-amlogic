// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/amlogic/gpio/meson-s1a-gpio.h>

#include <../../pinctrl/meson/pinctrl-meson-axg.h>

/* GPIOB func1 */
static const unsigned int spinf_mo_d0_pins[]			= { GPIOB_0 };
static const unsigned int spinf_mi_d1_pins[]			= { GPIOB_1 };
static const unsigned int spinf_wp_d2_pins[]			= { GPIOB_2 };
static const unsigned int spinf_hold_d3_pins[]			= { GPIOB_3 };
static const unsigned int spinf_clk_pins[]			= { GPIOB_4 };
static const unsigned int spinf_cs_pins[]			= { GPIOB_5 };

/* GPIOD func1 */
static const unsigned int uart_b_tx_d0_pins[]			= { GPIOD_0 };
static const unsigned int uart_b_rx_d1_pins[]			= { GPIOD_1 };
static const unsigned int ir_remote_input_d2_pins[]		= { GPIOD_2 };
static const unsigned int jtag_clk_pins[]			= { GPIOD_3 };
static const unsigned int jtag_tms_pins[]			= { GPIOD_4 };
static const unsigned int jtag_tdi_pins[]			= { GPIOD_5 };
static const unsigned int jtag_tdo_pins[]			= { GPIOD_6 };
static const unsigned int i2c2_sda_d7_pins[]			= { GPIOD_7 };
static const unsigned int i2c2_scl_d8_pins[]			= { GPIOD_8 };
static const unsigned int eth_link_led_pins[]			= { GPIOD_9 };
static const unsigned int eth_act_led_pins[]			= { GPIOD_10 };

/* GPIOD func2 */
static const unsigned int i2c0_scl_d0_pins[]			= { GPIOD_0 };
static const unsigned int i2c0_sda_d1_pins[]			= { GPIOD_1 };
static const unsigned int gen_clk_d2_pins[]			= { GPIOD_2 };
static const unsigned int pwm_c_pins[]				= { GPIOD_3 };
static const unsigned int pwm_d_pins[]				= { GPIOD_4 };
static const unsigned int pwm_b_pins[]				= { GPIOD_5 };
static const unsigned int pwm_a_pins[]				= { GPIOD_6 };
static const unsigned int iso7816_clk_d7_pins[]			= { GPIOD_7 };
static const unsigned int iso7816_data_d8_pins[]		= { GPIOD_8 };
static const unsigned int uart_a_rx_d9_pins[]			= { GPIOD_9 };
static const unsigned int uart_a_tx_d10_pins[]			= { GPIOD_10 };

/* GPIOD func3 */
static const unsigned int iso7816_data_d0_pins[]		= { GPIOD_0 };
static const unsigned int iso7816_clk_d1_pins[]			= { GPIOD_1 };
static const unsigned int clk12m_24m_d2_pins[]			= { GPIOD_2 };
static const unsigned int pwm_c_hiz_pins[]			= { GPIOD_3 };
static const unsigned int pwm_d_hiz_pins[]			= { GPIOD_4 };
static const unsigned int s2_demod_gpio7_pins[]			= { GPIOD_7 };
static const unsigned int s2_demod_gpio6_pins[]			= { GPIOD_8 };
static const unsigned int i2c0_sda_d9_pins[]			= { GPIOD_9 };
static const unsigned int i2c0_scl_d10_pins[]			= { GPIOD_10 };

/* GPIOD func4 */
static const unsigned int clk_32k_in_d2_pins[]			= { GPIOD_2 };
static const unsigned int i2c2_sda_d3_pins[]			= { GPIOD_3 };
static const unsigned int i2c2_scl_d4_pins[]			= { GPIOD_4 };
static const unsigned int i2c0_scl_d5_pins[]			= { GPIOD_5 };
static const unsigned int i2c0_sda_d6_pins[]			= { GPIOD_6 };

/* GPIOD func5 */
static const unsigned int spdif_out_d1_pins[]			= { GPIOD_1 };
static const unsigned int uart_b_tx_d3_pins[]			= { GPIOD_3 };
static const unsigned int uart_b_rx_d4_pins[]			= { GPIOD_4 };

/* GPIOD func6 */
static const unsigned int tsin_a_clk_d7_pins[]			= { GPIOD_7 };
static const unsigned int tsin_a_sop_d8_pins[]			= { GPIOD_8 };
static const unsigned int tsin_a_valid_d9_pins[]		= { GPIOD_9 };
static const unsigned int tsin_a_d0_d10_pins[]			= { GPIOD_10 };

/* GPIOH func1 */
static const unsigned int hdmitx_sda_pins[]			= { GPIOH_0 };
static const unsigned int hdmitx_scl_pins[]			= { GPIOH_1 };
static const unsigned int hdmitx_hpd_in_pins[]			= { GPIOH_2 };
static const unsigned int ao_cec_b_pins[]			= { GPIOH_3 };
static const unsigned int spdif_out_h4_pins[]			= { GPIOH_4 };
static const unsigned int dtv_a_if_agc_h6_pins[]		= { GPIOH_6 };
static const unsigned int diseqc_out_h7_pins[]			= { GPIOH_7 };
static const unsigned int s2_demod_gpio0_h8_pins[]		= { GPIOH_8 };
static const unsigned int iso7816_clk_h9_pins[]			= { GPIOH_9 };
static const unsigned int iso7816_data_h10_pins[]		= { GPIOH_10 };

/* GPIOH func2 */
static const unsigned int tsin_a_clk_h4_pins[]			= { GPIOH_4 };
static const unsigned int tsin_a_sop_h5_pins[]			= { GPIOH_5 };
static const unsigned int dtv_a_rf_agc_h6_pins[]		= { GPIOH_6 };
static const unsigned int tsin_a_valid_h9_pins[]		= { GPIOH_9 };
static const unsigned int tsin_a_d0_h10_pins[]			= { GPIOH_10 };

/* GPIOH func3 */
static const unsigned int s2_demod_gpio5_pins[]			= { GPIOH_4 };

/* GPIOZ func1 */
static const unsigned int ir_remote_input_z0_pins[]		= { GPIOZ_0 };
static const unsigned int dtv_a_if_agc_z1_pins[]		= { GPIOZ_1 };
static const unsigned int diseqc_out_z2_pins[]			= { GPIOZ_2 };
static const unsigned int i2c1_sda_pins[]			= { GPIOZ_3 };
static const unsigned int i2c1_scl_pins[]			= { GPIOZ_4 };
static const unsigned int dtv_a_if_agc_z5_pins[]		= { GPIOZ_5 };
static const unsigned int uart_a_tx_z6_pins[]			= { GPIOZ_6 };
static const unsigned int uart_a_rx_z7_pins[]			= { GPIOZ_7 };
static const unsigned int dtv_a_if_agc_z8_pins[]		= { GPIOZ_8 };

/* GPIOZ func2 */
static const unsigned int i2c0_scl_z0_pins[]			= { GPIOZ_0 };
static const unsigned int i2c0_sda_z1_pins[]			= { GPIOZ_1 };
static const unsigned int clk12m_24m_z2_pins[]			= { GPIOZ_2 };
static const unsigned int clk_32k_in_z3_pins[]			= { GPIOZ_3 };
static const unsigned int spdif_out_z4_pins[]			= { GPIOZ_4 };
static const unsigned int dtv_a_rf_agc_z5_pins[]		= { GPIOZ_5 };
static const unsigned int i2c2_sda_z6_pins[]			= { GPIOZ_6 };
static const unsigned int i2c2_scl_z7_pins[]			= { GPIOZ_7 };

/* GPIOZ func3 */
static const unsigned int i2c2_sda_z2_pins[]			= { GPIOZ_2 };
static const unsigned int i2c2_scl_z5_pins[]			= { GPIOZ_5 };
static const unsigned int iso7816_clk_z6_pins[]			= { GPIOZ_6 };
static const unsigned int iso7816_data_z7_pins[]		= { GPIOZ_7 };

/* GPIOZ func4 */
static const unsigned int tsin_a_clk_z0_pins[]			= { GPIOZ_0 };
static const unsigned int tsin_a_sop_z1_pins[]			= { GPIOZ_1 };
static const unsigned int s2_demod_gpio4_pins[]			= { GPIOZ_2 };
static const unsigned int s2_demod_gpio3_pins[]			= { GPIOZ_3 };
static const unsigned int s2_demod_gpio2_pins[]			= { GPIOZ_4 };
static const unsigned int s2_demod_gpio1_pins[]			= { GPIOZ_5 };
static const unsigned int tsin_a_valid_z6_pins[]		= { GPIOZ_6 };
static const unsigned int tsin_a_d0_z7_pins[]			= { GPIOZ_7 };

/* GPIOZ func5 */
static const unsigned int hdmitx_test_o_z3_pins[]		= { GPIOZ_3 };
static const unsigned int gen_clk_z4_pins[]			= { GPIOZ_4 };
static const unsigned int hdmitx_test_o_z5_pins[]		= { GPIOZ_5 };
static const unsigned int spdif_out_z6_pins[]			= { GPIOZ_6 };

/* GPIOZ func6 */
static const unsigned int s2_demod_gpio0_z7_pins[]		= { GPIOZ_7 };
static const unsigned int s2_demod_gpio0_z8_pins[]		= { GPIOZ_8 };

/* GPIOZ func7 */
static const unsigned int gen_clk_z5_pins[]			= { GPIOZ_5 };

static struct meson_pmx_group meson_s1a_periphs_groups[] = {
	/* func0 as GPIO */
	GPIO_GROUP(GPIOB_0, 0),
	GPIO_GROUP(GPIOB_1, 0),
	GPIO_GROUP(GPIOB_2, 0),
	GPIO_GROUP(GPIOB_3, 0),
	GPIO_GROUP(GPIOB_4, 0),
	GPIO_GROUP(GPIOB_5, 0),
	GPIO_GROUP(GPIOD_0, 0),
	GPIO_GROUP(GPIOD_1, 0),
	GPIO_GROUP(GPIOD_2, 0),
	GPIO_GROUP(GPIOD_3, 0),
	GPIO_GROUP(GPIOD_4, 0),
	GPIO_GROUP(GPIOD_5, 0),
	GPIO_GROUP(GPIOD_6, 0),
	GPIO_GROUP(GPIOD_7, 0),
	GPIO_GROUP(GPIOD_8, 0),
	GPIO_GROUP(GPIOD_9, 0),
	GPIO_GROUP(GPIOD_10, 0),
	GPIO_GROUP(GPIOD_11, 0),
	GPIO_GROUP(GPIOH_0, 0),
	GPIO_GROUP(GPIOH_1, 0),
	GPIO_GROUP(GPIOH_2, 0),
	GPIO_GROUP(GPIOH_3, 0),
	GPIO_GROUP(GPIOH_4, 0),
	GPIO_GROUP(GPIOH_5, 0),
	GPIO_GROUP(GPIOH_6, 0),
	GPIO_GROUP(GPIOH_7, 0),
	GPIO_GROUP(GPIOH_8, 0),
	GPIO_GROUP(GPIOH_9, 0),
	GPIO_GROUP(GPIOH_10, 0),
	GPIO_GROUP(GPIOZ_0, 0),
	GPIO_GROUP(GPIOZ_1, 0),
	GPIO_GROUP(GPIOZ_2, 0),
	GPIO_GROUP(GPIOZ_3, 0),
	GPIO_GROUP(GPIOZ_4, 0),
	GPIO_GROUP(GPIOZ_5, 0),
	GPIO_GROUP(GPIOZ_6, 0),
	GPIO_GROUP(GPIOZ_7, 0),
	GPIO_GROUP(GPIOZ_8, 0),
	GPIO_GROUP(GPIOZ_9, 0),
	GPIO_GROUP(GPIO_TEST_N, 0),

	/* GPIOB func1 */
	GROUP(spinf_mo_d0,				1),
	GROUP(spinf_mi_d1,				1),
	GROUP(spinf_wp_d2,				1),
	GROUP(spinf_hold_d3,				1),
	GROUP(spinf_clk,				1),
	GROUP(spinf_cs,					1),

	/* GPIOD func1 */
	GROUP(uart_b_tx_d0,				1),
	GROUP(uart_b_rx_d1,				1),
	GROUP(ir_remote_input_d2,			1),
	GROUP(jtag_clk,					1),
	GROUP(jtag_tms,					1),
	GROUP(jtag_tdi,					1),
	GROUP(jtag_tdo,					1),
	GROUP(i2c2_sda_d7,				1),
	GROUP(i2c2_scl_d8,				1),
	GROUP(eth_link_led,				1),
	GROUP(eth_act_led,				1),

	/* GPIOD func2 */
	GROUP(i2c0_scl_d0,				2),
	GROUP(i2c0_sda_d1,				2),
	GROUP(gen_clk_d2,				2),
	GROUP(pwm_c,					2),
	GROUP(pwm_d,					2),
	GROUP(pwm_b,					2),
	GROUP(pwm_a,					2),
	GROUP(iso7816_clk_d7,				2),
	GROUP(iso7816_data_d8,				2),
	GROUP(uart_a_rx_d9,				2),
	GROUP(uart_a_tx_d10,				2),

	/* GPIOD func3 */
	GROUP(iso7816_data_d0,				3),
	GROUP(iso7816_clk_d1,				3),
	GROUP(clk12m_24m_d2,				3),
	GROUP(pwm_c_hiz,				3),
	GROUP(pwm_d_hiz,				3),
	GROUP(s2_demod_gpio7,				3),
	GROUP(s2_demod_gpio6,				3),
	GROUP(i2c0_sda_d9,				3),
	GROUP(i2c0_scl_d10,				3),

	/* GPIOD func4 */
	GROUP(clk_32k_in_d2,				4),
	GROUP(i2c2_sda_d3,				4),
	GROUP(i2c2_scl_d4,				4),
	GROUP(i2c0_scl_d5,				4),
	GROUP(i2c0_sda_d6,				4),

	/* GPIOD func5 */
	GROUP(spdif_out_d1,				5),
	GROUP(uart_b_tx_d3,				5),
	GROUP(uart_b_rx_d4,				5),

	/* GPIOD func6 */
	GROUP(tsin_a_clk_d7,				6),
	GROUP(tsin_a_sop_d8,				6),
	GROUP(tsin_a_valid_d9,				6),
	GROUP(tsin_a_d0_d10,				6),

	/* GPIOH func1 */
	GROUP(hdmitx_sda,				1),
	GROUP(hdmitx_scl,				1),
	GROUP(hdmitx_hpd_in,				1),
	GROUP(ao_cec_b,					1),
	GROUP(spdif_out_h4,				1),
	GROUP(dtv_a_if_agc_h6,				1),
	GROUP(diseqc_out_h7,				1),
	GROUP(s2_demod_gpio0_h8,			1),
	GROUP(iso7816_clk_h9,				1),
	GROUP(iso7816_data_h10,				1),

	/* GPIOH func2 */
	GROUP(tsin_a_clk_h4,				2),
	GROUP(tsin_a_sop_h5,				2),
	GROUP(dtv_a_rf_agc_h6,				2),
	GROUP(tsin_a_valid_h9,				2),
	GROUP(tsin_a_d0_h10,				2),

	/* GPIOH func3 */
	GROUP(s2_demod_gpio5,				3),

	/* GPIOZ func1 */
	GROUP(ir_remote_input_z0,			1),
	GROUP(dtv_a_if_agc_z1,				1),
	GROUP(diseqc_out_z2,				1),
	GROUP(i2c1_sda,				1),
	GROUP(i2c1_scl,				1),
	GROUP(dtv_a_if_agc_z5,				1),
	GROUP(uart_a_tx_z6,				1),
	GROUP(uart_a_rx_z7,				1),
	GROUP(dtv_a_if_agc_z8,				1),

	/* GPIOZ func2 */
	GROUP(i2c0_scl_z0,				2),
	GROUP(i2c0_sda_z1,				2),
	GROUP(clk12m_24m_z2,				2),
	GROUP(clk_32k_in_z3,				2),
	GROUP(spdif_out_z4,				2),
	GROUP(dtv_a_rf_agc_z5,				2),
	GROUP(i2c2_sda_z6,				2),
	GROUP(i2c2_scl_z7,				2),

	/* GPIOZ func3 */
	GROUP(i2c2_sda_z2,				3),
	GROUP(i2c2_scl_z5,				3),
	GROUP(iso7816_clk_z6,				3),
	GROUP(iso7816_data_z7,				3),

	/* GPIOZ func4 */
	GROUP(tsin_a_clk_z0,				4),
	GROUP(tsin_a_sop_z1,				4),
	GROUP(s2_demod_gpio4,				4),
	GROUP(s2_demod_gpio3,				4),
	GROUP(s2_demod_gpio2,				4),
	GROUP(s2_demod_gpio1,				4),
	GROUP(tsin_a_valid_z6,				4),
	GROUP(tsin_a_d0_z7,				4),

	/* GPIOZ func5 */
	GROUP(hdmitx_test_o_z3,				5),
	GROUP(gen_clk_z4,				5),
	GROUP(hdmitx_test_o_z5,				5),
	GROUP(spdif_out_z6,				5),

	/* GPIOZ func6 */
	GROUP(s2_demod_gpio0_z7,			6),
	GROUP(s2_demod_gpio0_z8,			6),

	/* GPIOZ func7 */
	GROUP(gen_clk_z5,				7)
};

static const char * const gpio_periphs_groups[] = {
	"GPIOB_0", "GPIOB_1", "GPIOB_2", "GPIOB_3",
	"GPIOB_4", "GPIOB_5", "GPIOD_0", "GPIOD_1",
	"GPIOD_2", "GPIOD_3", "GPIOD_4", "GPIOD_5",
	"GPIOD_6", "GPIOD_7", "GPIOD_8", "GPIOD_9",
	"GPIOD_10", "GPIOD_11", "GPIOH_0", "GPIOH_1",
	"GPIOH_2", "GPIOH_3", "GPIOH_4", "GPIOH_5",
	"GPIOH_6", "GPIOH_7", "GPIOH_8", "GPIOH_9",
	"GPIOH_10", "GPIOZ_0", "GPIOZ_1", "GPIOZ_2",
	"GPIOZ_3", "GPIOZ_4", "GPIOZ_5", "GPIOZ_6",
	"GPIOZ_7", "GPIOZ_8", "GPIOZ_9",
	"GPIO_TEST_N"
};

static const char * const ao_cec_b_groups[] = {
	"ao_cec_b"
};

static const char * const clk12m_24m_groups[] = {
	"clk12m_24m_d2", "clk12m_24m_z2"
};

static const char * const clk_32k_in_groups[] = {
	"clk_32k_in_d2", "clk_32k_in_z3"
};

static const char * const diseqc_out_groups[] = {
	"diseqc_out_h7", "diseqc_out_z2"
};

static const char * const dtv_groups[] = {
	"dtv_a_if_agc_h6", "dtv_a_rf_agc_h6", "dtv_a_if_agc_z1",
	"dtv_a_if_agc_z5", "dtv_a_rf_agc_z5", "dtv_a_if_agc_z8"
};

static const char * const eth_groups[] = {
	"eth_link_led", "eth_act_led"
};

static const char * const gen_clk_groups[] = {
	"gen_clk_d2", "gen_clk_z4", "gen_clk_z5"
};

static const char * const hdmitx_groups[] = {
	"hdmitx_sda", "hdmitx_scl", "hdmitx_hpd_in", "hdmitx_test_o_z3",
	"hdmitx_test_o_z5"
};

static const char * const i2c0_groups[] = {
	"i2c0_scl_d0", "i2c0_sda_d1", "i2c0_scl_d5", "i2c0_sda_d6",
	"i2c0_sda_d9", "i2c0_scl_d10", "i2c0_scl_z0", "i2c0_sda_z1"
};

static const char * const i2c1_groups[] = {
	"i2c1_sda", "i2c1_scl"
};

static const char * const i2c2_groups[] = {
	"i2c2_sda_d3", "i2c2_scl_d4", "i2c2_sda_d7", "i2c2_scl_d8",
	"i2c2_sda_z2", "i2c2_scl_z5", "i2c2_sda_z6", "i2c2_scl_z7"
};

static const char * const ir_remote_input_groups[] = {
	"ir_remote_input_d2", "ir_remote_input_z0"
};

static const char * const iso7816_groups[] = {
	"iso7816_data_d0", "iso7816_clk_d1", "iso7816_clk_d7",
	"iso7816_data_d8", "iso7816_clk_h9", "iso7816_data_h10",
	"iso7816_clk_z6", "iso7816_data_z7"
};

static const char * const jtag_groups[] = {
	"jtag_clk", "jtag_tms", "jtag_tdi", "jtag_tdo"
};

static const char * const pwm_c_hiz_groups[] = {
	"pwm_c_hiz"
};

static const char * const pwm_d_hiz_groups[] = {
	"pwm_d_hiz"
};

static const char * const pwm_a_groups[] = {
	"pwm_a"
};

static const char * const pwm_b_groups[] = {
	"pwm_b"
};

static const char * const pwm_c_groups[] = {
	"pwm_c"
};

static const char * const pwm_d_groups[] = {
	"pwm_d"
};

static const char * const s2_demod_gpio_groups[] = {
	"s2_demod_gpio7", "s2_demod_gpio6", "s2_demod_gpio5",
	"s2_demod_gpio0_h8", "s2_demod_gpio4", "s2_demod_gpio3",
	"s2_demod_gpio2", "s2_demod_gpio1", "s2_demod_gpio0_z7",
	"s2_demod_gpio0_z8"
};

static const char * const spdif_out_groups[] = {
	"spdif_out_d1", "spdif_out_h4", "spdif_out_z4", "spdif_out_z6"
};

static const char * const spinf_groups[] = {
	"spinf_mo_d0", "spinf_mi_d1", "spinf_wp_d2", "spinf_hold_d3",
	"spinf_clk", "spinf_cs"
};

static const char * const tsin_a_groups[] = {
	"tsin_a_clk_d7", "tsin_a_sop_d8", "tsin_a_valid_d9", "tsin_a_d0_d10",
	"tsin_a_clk_h4", "tsin_a_sop_h5", "tsin_a_valid_h9", "tsin_a_d0_h10",
	"tsin_a_clk_z0", "tsin_a_sop_z1", "tsin_a_valid_z6", "tsin_a_d0_z7"
};

static const char * const uart_a_groups[] = {
	"uart_a_rx_d9", "uart_a_tx_d10", "uart_a_tx_z6", "uart_a_rx_z7"
};

static const char * const uart_b_groups[] = {
	"uart_b_tx_d0", "uart_b_rx_d1", "uart_b_tx_d3", "uart_b_rx_d4"
};

static struct meson_pmx_func meson_s1a_periphs_functions[] = {
	FUNCTION(gpio_periphs),
	FUNCTION(ao_cec_b),
	FUNCTION(clk12m_24m),
	FUNCTION(clk_32k_in),
	FUNCTION(diseqc_out),
	FUNCTION(dtv),
	FUNCTION(eth),
	FUNCTION(gen_clk),
	FUNCTION(hdmitx),
	FUNCTION(i2c0),
	FUNCTION(i2c1),
	FUNCTION(i2c2),
	FUNCTION(ir_remote_input),
	FUNCTION(iso7816),
	FUNCTION(jtag),
	FUNCTION(pwm_c_hiz),
	FUNCTION(pwm_d_hiz),
	FUNCTION(pwm_a),
	FUNCTION(pwm_b),
	FUNCTION(pwm_c),
	FUNCTION(pwm_d),
	FUNCTION(s2_demod_gpio),
	FUNCTION(spdif_out),
	FUNCTION(spinf),
	FUNCTION(tsin_a),
	FUNCTION(uart_a),
	FUNCTION(uart_b)
};

static struct meson_bank meson_s1a_periphs_banks[] = {
	/* name  first  last  irq  pullen  pull  dir  out  in */
	BANK_DS("B",  GPIOB_0,  GPIOB_5,
		0x63,  0, 0x64,  0, 0x62,  0, 0x61,  0, 0x60,  0, 0x07,  0),
	BANK_DS("D",  GPIOD_0, GPIOD_11,
		0x33,  0, 0x34,  0, 0x32,  0, 0x31,  0, 0x30,  0, 0x07,  0),
	BANK_DS("H",  GPIOH_0, GPIOH_10,
		0x23,  0, 0x24,  0, 0x22,  0, 0x21,  0, 0x20,  0, 0x07,  0),
	BANK_DS("Z",  GPIOZ_0,  GPIOZ_9,
		0x03,  0, 0x04,  0, 0x02,  0, 0x01,  0, 0x00,  0, 0x07,  0),
	BANK_DS("_TEST_N", GPIO_TEST_N,    GPIO_TEST_N,
		0x83,  0, 0x84,  0, 0x82, 0,  0x81,  0, 0x80, 0, 0x07, 0),
};

static struct meson_pmx_bank meson_s1a_periphs_pmx_banks[] = {
	/* name  first  lask  reg  offset */
	BANK_PMX("B",  GPIOB_0,  GPIOB_5, 0x00,  0),
	BANK_PMX("D",  GPIOD_0, GPIOD_11, 0x10,  0),
	BANK_PMX("H",  GPIOH_0, GPIOH_10, 0x0b,  0),
	BANK_PMX("Z",  GPIOZ_0,  GPIOZ_9, 0x06,  0),
	BANK_PMX("TEST_N", GPIO_TEST_N, GPIO_TEST_N, 0xf,  0),
};

static struct meson_axg_pmx_data meson_s1a_periphs_pmx_banks_data = {
	.pmx_banks	= meson_s1a_periphs_pmx_banks,
	.num_pmx_banks	= ARRAY_SIZE(meson_s1a_periphs_pmx_banks),
};

static int meson_s1a_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_ds = pc->reg_gpio;

	return 0;
}

static struct meson_pinctrl_data meson_s1a_periphs_pinctrl_data = {
	.name		= "periphs-banks",
	.pin_base	= 0,
	.groups		= meson_s1a_periphs_groups,
	.funcs		= meson_s1a_periphs_functions,
	.banks		= meson_s1a_periphs_banks,
	.num_pins	= 40,
	.num_groups	= ARRAY_SIZE(meson_s1a_periphs_groups),
	.num_funcs	= ARRAY_SIZE(meson_s1a_periphs_functions),
	.num_banks	= ARRAY_SIZE(meson_s1a_periphs_banks),
	.gpio_driver	= &meson_axg_gpio_driver,
	.pmx_data	= &meson_s1a_periphs_pmx_banks_data,
	.parse_dt	= meson_s1a_parse_dt_extra,
};

static const struct udevice_id meson_s1a_pinctrl_match[] = {
	{
		.compatible = "amlogic,meson-s1a-periphs-pinctrl",
		.data = (ulong)&meson_s1a_periphs_pinctrl_data,
	},
	{ }
};

U_BOOT_DRIVER(meson_s1a_pinctrl) = {
	.name	= "meson-s1a-pinctrl",
	.id	= UCLASS_PINCTRL,
	.of_match = of_match_ptr(meson_s1a_pinctrl_match),
	.probe = meson_pinctrl_probe,
	.priv_auto = sizeof(struct meson_pinctrl),
	.ops = &meson_axg_pinctrl_ops,
};
