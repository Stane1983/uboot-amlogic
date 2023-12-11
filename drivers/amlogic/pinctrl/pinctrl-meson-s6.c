// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/amlogic/gpio/meson-s7d-gpio.h>

#include <../../pinctrl/meson/pinctrl-meson-axg.h>

/* GPIOE func1 */
static const unsigned int i2cm_a_sda_pins[]			= { GPIOE_0 };
static const unsigned int i2cm_a_scl_pins[]			= { GPIOE_1 };

/* GPIOE func2 */
static const unsigned int uart_b_tx_e0_pins[]			= { GPIOE_0 };
static const unsigned int uart_b_rx_e1_pins[]			= { GPIOE_1 };

/* GPIOE func3 */
static const unsigned int pwm_h_pins[]				= { GPIOE_0 };
static const unsigned int pwm_j_pins[]				= { GPIOE_1 };

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
static const unsigned int emmc_rstgpio_pins[]			= { GPIOB_9 };
static const unsigned int emmc_cmd_pins[]			= { GPIOB_10 };
static const unsigned int emmc_ds_pins[]			= { GPIOB_11 };

/* GPIOB func2 */
static const unsigned int nand_wen_clk_pins[]			= { GPIOB_8 };
static const unsigned int nand_ale_pins[]			= { GPIOB_9 };
static const unsigned int nand_ren_wr_pins[]			= { GPIOB_10 };
static const unsigned int nand_cle_pins[]			= { GPIOB_11 };
static const unsigned int nand_ce0_pins[]			= { GPIOB_12 };

/* GPIOB func3 */
static const unsigned int spinf_rstgpio_pins[]			= { GPIOB_2 };
static const unsigned int spinf_hold_d3_pins[]			= { GPIOB_3 };
static const unsigned int spinf_mo_d0_pins[]			= { GPIOB_4 };
static const unsigned int spinf_mi_d1_pins[]			= { GPIOB_5 };
static const unsigned int spinf_clk200m_pins[]			= { GPIOB_6 };
static const unsigned int spinf_wp_d2_pins[]			= { GPIOB_7 };
static const unsigned int spinf_d4_pins[]			= { GPIOB_8 };
static const unsigned int spinf_d5_pins[]			= { GPIOB_9 };
static const unsigned int spinf_d6_pins[]			= { GPIOB_10 };
static const unsigned int spinf_d7_pins[]			= { GPIOB_11 };
static const unsigned int spinf_cs1_pins[]			= { GPIOB_12 };
static const unsigned int spinf_cs0_pins[]			= { GPIOB_13 };

/* GPIOC func1 */
static const unsigned int sdcard_d0_c0_pins[]			= { GPIOC_0 };
static const unsigned int sdcard_d1_c1_pins[]			= { GPIOC_1 };
static const unsigned int sdcard_d2_c2_pins[]			= { GPIOC_2 };
static const unsigned int sdcard_d3_c3_pins[]			= { GPIOC_3 };
static const unsigned int sdcard_clk_c4_pins[]			= { GPIOC_4 };
static const unsigned int sdcard_cmd_c5_pins[]			= { GPIOC_5 };
static const unsigned int card_det_pins[]			= { GPIOC_6 };

/* GPIOC func2 */
static const unsigned int jtag2_tdo_pins[]			= { GPIOC_0 };
static const unsigned int jtag2_tdi_pins[]			= { GPIOC_1 };
static const unsigned int uart_b_rx_c2_pins[]			= { GPIOC_2 };
static const unsigned int uart_b_tx_c3_pins[]			= { GPIOC_3 };
static const unsigned int jtag2_clk_pins[]			= { GPIOC_4 };
static const unsigned int jtag2_tms_pins[]			= { GPIOC_5 };
static const unsigned int i2cm_b_sda_c6_pins[]			= { GPIOC_6 };
static const unsigned int i2cm_b_scl_c7_pins[]			= { GPIOC_7 };

/* GPIOC func3 */
static const unsigned int pdm_din1_c0_pins[]			= { GPIOC_0 };
static const unsigned int pdm_din0_c1_pins[]			= { GPIOC_1 };
static const unsigned int i2cm_e_scl_c2_pins[]			= { GPIOC_2 };
static const unsigned int i2cm_e_sda_c3_pins[]			= { GPIOC_3 };
static const unsigned int pdm_dclk_c4_pins[]			= { GPIOC_4 };
static const unsigned int iso7816_clk_c5_pins[]			= { GPIOC_5 };
static const unsigned int iso7816_data_c6_pins[]		= { GPIOC_6 };

/* GPIOC func4 */
static const unsigned int tdm_d2_c0_pins[]			= { GPIOC_0 };
static const unsigned int tdm_d3_c1_pins[]			= { GPIOC_1 };
static const unsigned int tdm_fs1_c2_pins[]			= { GPIOC_2 };
static const unsigned int tdm_sclk1_c3_pins[]			= { GPIOC_3 };
static const unsigned int mclk_1_c4_pins[]			= { GPIOC_4 };
static const unsigned int tdm_d4_c5_pins[]			= { GPIOC_5 };
static const unsigned int tdm_d5_c6_pins[]			= { GPIOC_6 };

/* GPIOC func6 */
static const unsigned int gen_clk_c4_pins[]			= { GPIOC_4 };

/* GPIOC func7 */
static const unsigned int debug_o14_pins[]			= { GPIOC_2 };
static const unsigned int debug_o15_pins[]			= { GPIOC_3 };

/* GPIOD func1 */
static const unsigned int uart_b_tx_d0_pins[]			= { GPIOD_0 };
static const unsigned int uart_b_rx_d1_pins[]			= { GPIOD_1 };
static const unsigned int uart_b_cts_pins[]			= { GPIOD_2 };
static const unsigned int uart_b_rts_pins[]			= { GPIOD_3 };
static const unsigned int ir_remote_out_pins[]			= { GPIOD_4 };

/* GPIOD func2 */
static const unsigned int i2cm_e_scl_d2_pins[]			= { GPIOD_2 };
static const unsigned int i2cm_e_sda_d3_pins[]			= { GPIOD_3 };
static const unsigned int mclk_1_d4_pins[]			= { GPIOD_4 };

/* GPIOD func3 */
static const unsigned int uart_c_tx_pins[]			= { GPIOD_2 };
static const unsigned int uart_c_rx_pins[]			= { GPIOD_3 };
static const unsigned int pwm_b_d4_pins[]			= { GPIOD_4 };

/* GPIOD func4 */
static const unsigned int clk_32k_in_pins[]			= { GPIOD_2 };
static const unsigned int pwm_b_hiz_pins[]			= { GPIOD_4 };

/* GPIOD func5 */
static const unsigned int mic_mute_en_pins[]			= { GPIOD_2 };
static const unsigned int mic_mute_key_pins[]			= { GPIOD_3 };

/* GPIOD func7 */
static const unsigned int debug_o0_pins[]			= { GPIOD_0 };
static const unsigned int debug_o1_pins[]			= { GPIOD_1 };
static const unsigned int debug_o2_pins[]			= { GPIOD_2 };
static const unsigned int debug_o3_pins[]			= { GPIOD_3 };
static const unsigned int debug_o4_pins[]			= { GPIOD_4 };

/* GPIODV func1 */
static const unsigned int ir_remote_input_pins[]		= { GPIODV_0 };
static const unsigned int jtag_clk_pins[]			= { GPIODV_1 };
static const unsigned int jtag_tms_pins[]			= { GPIODV_2 };
static const unsigned int jtag_tdi_pins[]			= { GPIODV_3 };
static const unsigned int jtag_tdo_pins[]			= { GPIODV_4 };
static const unsigned int clk12m_24m_pins[]			= { GPIODV_5 };
static const unsigned int pwm_g_hiz_pins[]			= { GPIODV_6 };

/* GPIODV func2 */
static const unsigned int tdm_sclk1_dv1_pins[]			= { GPIODV_1 };
static const unsigned int tdm_fs1_dv2_pins[]			= { GPIODV_2 };
static const unsigned int tdm_d4_dv3_pins[]			= { GPIODV_3 };
static const unsigned int tdm_d3_dv4_pins[]			= { GPIODV_4 };
static const unsigned int tdm_d2_dv5_pins[]			= { GPIODV_5 };
static const unsigned int pwm_g_dv6_pins[]			= { GPIODV_6 };

/* GPIODV func3 */
static const unsigned int pwm_a_dv1_pins[]			= { GPIODV_1 };
static const unsigned int pwm_c_dv2_pins[]			= { GPIODV_2 };
static const unsigned int pwm_d_dv3_pins[]			= { GPIODV_3 };
static const unsigned int pwm_i_dv4_pins[]			= { GPIODV_4 };

/* GPIODV func4 */
static const unsigned int pwm_a_hiz_pins[]			= { GPIODV_1 };
static const unsigned int pwm_c_hiz_pins[]			= { GPIODV_2 };
static const unsigned int pdm_dclk_dv3_pins[]			= { GPIODV_3 };
static const unsigned int pdm_din0_dv4_pins[]			= { GPIODV_4 };
static const unsigned int pdm_din1_dv5_pins[]			= { GPIODV_5 };

/* GPIODV func5 */
static const unsigned int i2cm_b_sda_dv1_pins[]			= { GPIODV_1 };
static const unsigned int i2cm_b_scl_dv2_pins[]			= { GPIODV_2 };
static const unsigned int i2cm_c_sda_dv5_pins[]			= { GPIODV_5 };
static const unsigned int i2cm_c_scl_dv6_pins[]			= { GPIODV_6 };

/* GPIODV func6 */
static const unsigned int tsin_b_clk_dv1_pins[]			= { GPIODV_1 };
static const unsigned int tsin_b_sop_dv2_pins[]			= { GPIODV_2 };
static const unsigned int tsin_b_valid_dv3_pins[]		= { GPIODV_3 };
static const unsigned int tsin_b_d0_dv4_pins[]			= { GPIODV_4 };
static const unsigned int gen_clk_dv5_pins[]			= { GPIODV_5 };
static const unsigned int hdmitx_test_o_dv6_pins[]		= { GPIODV_6 };

/* GPIODV func7 */
static const unsigned int debug_o5_pins[]			= { GPIODV_0 };
static const unsigned int debug_o6_pins[]			= { GPIODV_1 };
static const unsigned int debug_o7_pins[]			= { GPIODV_2 };
static const unsigned int debug_o8_pins[]			= { GPIODV_3 };
static const unsigned int debug_o9_pins[]			= { GPIODV_4 };
static const unsigned int debug_o10_pins[]			= { GPIODV_5 };
static const unsigned int debug_o11_pins[]			= { GPIODV_6 };

/* GPIOH func1 */
static const unsigned int hdmitx_sda_pins[]			= { GPIOH_0 };
static const unsigned int hdmitx_scl_pins[]			= { GPIOH_1 };
static const unsigned int hdmitx_hpd_in_pins[]			= { GPIOH_2 };
static const unsigned int spdif_out_h4_pins[]			= { GPIOH_4 };
static const unsigned int spdif_in_h5_pins[]			= { GPIOH_5 };
static const unsigned int i2cm_b_sda_h6_pins[]			= { GPIOH_6 };
static const unsigned int i2cm_b_scl_h7_pins[]			= { GPIOH_7 };
static const unsigned int i2cm_c_sda_h8_pins[]			= { GPIOH_8 };
static const unsigned int i2cm_c_scl_h9_pins[]			= { GPIOH_9 };
static const unsigned int eth_link_led_pins[]			= { GPIOH_10 };
static const unsigned int eth_act_led_pins[]			= { GPIOH_11 };

/* GPIOH func2 */
static const unsigned int i2cm_c_sda_h0_pins[]			= { GPIOH_0 };
static const unsigned int i2cm_c_scl_h1_pins[]			= { GPIOH_1 };
static const unsigned int ao_cec_b_pins[]			= { GPIOH_3 };
static const unsigned int uart_d_tx_h4_pins[]			= { GPIOH_4 };
static const unsigned int uart_d_rx_h5_pins[]			= { GPIOH_5 };
static const unsigned int uart_d_cts_h6_pins[]			= { GPIOH_6 };
static const unsigned int uart_d_rts_h7_pins[]			= { GPIOH_7 };
static const unsigned int iso7816_clk_h8_pins[]			= { GPIOH_8 };
static const unsigned int iso7816_data_h9_pins[]		= { GPIOH_9 };
static const unsigned int uart_e_tx_h10_pins[]			= { GPIOH_10 };
static const unsigned int uart_e_rx_h11_pins[]			= { GPIOH_11 };

/* GPIOH func3 */
static const unsigned int pwm_d_h6_pins[]			= { GPIOH_6 };
static const unsigned int pwm_i_h7_pins[]			= { GPIOH_7 };
static const unsigned int pdm_dclk_h8_pins[]			= { GPIOH_8 };
static const unsigned int pdm_din0_h9_pins[]			= { GPIOH_9 };
static const unsigned int pdm_din1_h10_pins[]			= { GPIOH_10 };

/* GPIOH func4 */
static const unsigned int mclk_2_h4_pins[]			= { GPIOH_4 };
static const unsigned int tdm_sclk2_h5_pins[]			= { GPIOH_5 };
static const unsigned int tdm_fs2_h6_pins[]			= { GPIOH_6 };
static const unsigned int tdm_d5_h7_pins[]			= { GPIOH_7 };
static const unsigned int tdm_d6_h8_pins[]			= { GPIOH_8 };
static const unsigned int tdm_d7_h9_pins[]			= { GPIOH_9 };
static const unsigned int eth_mdc_pins[]			= { GPIOH_11 };

/* GPIOH func5 */
static const unsigned int spi_a_miso_h4_pins[]			= { GPIOH_4 };
static const unsigned int spi_a_mosi_h5_pins[]			= { GPIOH_5 };
static const unsigned int spi_a_clk_h6_pins[]			= { GPIOH_6 };
static const unsigned int spi_a_ss0_h7_pins[]			= { GPIOH_7 };
static const unsigned int spdif_in_h9_pins[]			= { GPIOH_9 };

/* GPIOH func6 */
static const unsigned int tst_loop1_in_pins[]			= { GPIOH_0 };
static const unsigned int tst_loop1_out_pins[]			= { GPIOH_1 };
static const unsigned int tst_clk_1m_h2_pins[]			= { GPIOH_2 };
static const unsigned int tst_clk_1m_h3_pins[]			= { GPIOH_3 };
static const unsigned int tsin_b1_clk_pins[]			= { GPIOH_4 };
static const unsigned int tsin_b1_sop_pins[]			= { GPIOH_5 };
static const unsigned int tsin_b1_valid_pins[]			= { GPIOH_6 };
static const unsigned int tsin_b1_d0_pins[]			= { GPIOH_7 };
static const unsigned int gen_clk_h11_pins[]			= { GPIOH_11 };

/* GPIOH func7 */
static const unsigned int debug_o16_pins[]			= { GPIOH_4 };
static const unsigned int debug_o17_pins[]			= { GPIOH_5 };
static const unsigned int debug_o18_pins[]			= { GPIOH_6 };
static const unsigned int debug_o19_pins[]			= { GPIOH_7 };
static const unsigned int debug_o12_pins[]			= { GPIOH_8 };
static const unsigned int debug_o13_pins[]			= { GPIOH_9 };

/* GPIOX func1 */
static const unsigned int sdio_d0_pins[]			= { GPIOX_0 };
static const unsigned int sdio_d1_pins[]			= { GPIOX_1 };
static const unsigned int sdio_d2_pins[]			= { GPIOX_2 };
static const unsigned int sdio_d3_pins[]			= { GPIOX_3 };
static const unsigned int sdio_clk_pins[]			= { GPIOX_4 };
static const unsigned int sdio_cmd_pins[]			= { GPIOX_5 };
static const unsigned int pwm_a_x6_pins[]			= { GPIOX_6 };
static const unsigned int pwm_f_x7_pins[]			= { GPIOX_7 };
static const unsigned int tdm_d1_pins[]				= { GPIOX_8 };
static const unsigned int tdm_d0_pins[]				= { GPIOX_9 };
static const unsigned int tdm_fs0_pins[]			= { GPIOX_10 };
static const unsigned int tdm_sclk0_pins[]			= { GPIOX_11 };
static const unsigned int uart_a_tx_pins[]			= { GPIOX_12 };
static const unsigned int uart_a_rx_pins[]			= { GPIOX_13 };
static const unsigned int uart_a_cts_pins[]			= { GPIOX_14 };
static const unsigned int uart_a_rts_pins[]			= { GPIOX_15 };
static const unsigned int pwm_e_x16_pins[]			= { GPIOX_16 };
static const unsigned int i2cm_b_sda_x17_pins[]			= { GPIOX_17 };
static const unsigned int i2cm_b_scl_x18_pins[]			= { GPIOX_18 };
static const unsigned int pwm_b_x19_pins[]			= { GPIOX_19 };

/* GPIOX func2 */
static const unsigned int pdm_din0_x8_pins[]			= { GPIOX_8 };
static const unsigned int pdm_din1_x9_pins[]			= { GPIOX_9 };
static const unsigned int pdm_dclk_x11_pins[]			= { GPIOX_11 };

/* GPIOX func3 */
static const unsigned int spi_a_mosi_x8_pins[]			= { GPIOX_8 };
static const unsigned int spi_a_miso_x9_pins[]			= { GPIOX_9 };
static const unsigned int spi_a_ss0_x10_pins[]			= { GPIOX_10 };
static const unsigned int spi_a_clk_x11_pins[]			= { GPIOX_11 };

/* GPIOX func4 */
static const unsigned int pwm_c_x8_pins[]			= { GPIOX_8 };
static const unsigned int i2cs_a_sda_pins[]			= { GPIOX_10 };
static const unsigned int i2cs_a_scl_pins[]			= { GPIOX_11 };
static const unsigned int hdmitx_test_o_x14_pins[]		= { GPIOX_14 };

/* GPIOX func5 */
static const unsigned int i2cm_d_sda_x10_pins[]			= { GPIOX_10 };
static const unsigned int i2cm_d_scl_x11_pins[]			= { GPIOX_11 };

/* GPIOX func6 */
static const unsigned int debug_i0_pins[]			= { GPIOX_0 };
static const unsigned int debug_i1_pins[]			= { GPIOX_1 };
static const unsigned int debug_i2_pins[]			= { GPIOX_2 };
static const unsigned int debug_i3_pins[]			= { GPIOX_3 };
static const unsigned int debug_i4_pins[]			= { GPIOX_4 };
static const unsigned int debug_i5_pins[]			= { GPIOX_5 };
static const unsigned int debug_i6_pins[]			= { GPIOX_6 };
static const unsigned int debug_i7_pins[]			= { GPIOX_7 };
static const unsigned int debug_i8_pins[]			= { GPIOX_8 };
static const unsigned int debug_i9_pins[]			= { GPIOX_9 };
static const unsigned int debug_i10_pins[]			= { GPIOX_10 };
static const unsigned int debug_i11_pins[]			= { GPIOX_11 };
static const unsigned int debug_i12_pins[]			= { GPIOX_12 };
static const unsigned int debug_i13_pins[]			= { GPIOX_13 };
static const unsigned int debug_i14_pins[]			= { GPIOX_14 };
static const unsigned int debug_i15_pins[]			= { GPIOX_15 };
static const unsigned int debug_i16_pins[]			= { GPIOX_16 };
static const unsigned int debug_i17_pins[]			= { GPIOX_17 };
static const unsigned int debug_i18_pins[]			= { GPIOX_18 };
static const unsigned int debug_i19_pins[]			= { GPIOX_19 };

/* GPIOX func7 */
static const unsigned int tst_out0_pins[]			= { GPIOX_0 };
static const unsigned int tst_out1_pins[]			= { GPIOX_1 };
static const unsigned int tst_out2_pins[]			= { GPIOX_2 };
static const unsigned int tst_out3_pins[]			= { GPIOX_3 };
static const unsigned int tst_out4_pins[]			= { GPIOX_4 };
static const unsigned int tst_out5_pins[]			= { GPIOX_5 };
static const unsigned int tst_out6_pins[]			= { GPIOX_6 };
static const unsigned int tst_out7_pins[]			= { GPIOX_7 };
static const unsigned int tst_out8_pins[]			= { GPIOX_8 };
static const unsigned int tst_out9_pins[]			= { GPIOX_9 };
static const unsigned int tst_out10_pins[]			= { GPIOX_10 };
static const unsigned int tst_out11_pins[]			= { GPIOX_11 };
static const unsigned int tst_out12_pins[]			= { GPIOX_12 };
static const unsigned int tst_out13_pins[]			= { GPIOX_13 };
static const unsigned int tst_out14_pins[]			= { GPIOX_14 };
static const unsigned int tst_out15_pins[]			= { GPIOX_15 };
static const unsigned int tst_out16_pins[]			= { GPIOX_16 };
static const unsigned int tst_out17_pins[]			= { GPIOX_17 };
static const unsigned int tst_out18_pins[]			= { GPIOX_18 };
static const unsigned int tst_out19_pins[]			= { GPIOX_19 };

/* GPIOZ func1 */
static const unsigned int tdm_fs2_z0_pins[]			= { GPIOZ_0 };
static const unsigned int tdm_sclk2_z1_pins[]			= { GPIOZ_1 };
static const unsigned int tdm_d4_z2_pins[]			= { GPIOZ_2 };
static const unsigned int tdm_d5_z3_pins[]			= { GPIOZ_3 };
static const unsigned int tdm_d6_z4_pins[]			= { GPIOZ_4 };
static const unsigned int tdm_d7_z5_pins[]			= { GPIOZ_5 };
static const unsigned int mclk_2_z6_pins[]			= { GPIOZ_6 };
static const unsigned int tdm_d8_pins[]				= { GPIOZ_7 };
static const unsigned int spdif_out_z9_pins[]			= { GPIOZ_9 };
static const unsigned int uart_e_tx_z11_pins[]			= { GPIOZ_11 };
static const unsigned int uart_e_rx_z12_pins[]			= { GPIOZ_12 };

/* GPIOZ func2 */
static const unsigned int tsin_a_clk_pins[]			= { GPIOZ_0 };
static const unsigned int tsin_a_sop_pins[]			= { GPIOZ_1 };
static const unsigned int tsin_a_valid_pins[]			= { GPIOZ_2 };
static const unsigned int tsin_a_d0_pins[]			= { GPIOZ_3 };
static const unsigned int i2cm_d_sda_z8_pins[]			= { GPIOZ_8 };
static const unsigned int i2cm_d_scl_z9_pins[]			= { GPIOZ_9 };

/* GPIOZ func3 */
static const unsigned int sdcard_d0_z0_pins[]			= { GPIOZ_0 };
static const unsigned int sdcard_d1_z1_pins[]			= { GPIOZ_1 };
static const unsigned int sdcard_d2_z2_pins[]			= { GPIOZ_2 };
static const unsigned int sdcard_d3_z3_pins[]			= { GPIOZ_3 };
static const unsigned int sdcard_clk_z4_pins[]			= { GPIOZ_4 };
static const unsigned int sdcard_cmd_z5_pins[]			= { GPIOZ_5 };
static const unsigned int uart_e_tx_z8_pins[]			= { GPIOZ_8 };
static const unsigned int uart_e_rx_z9_pins[]			= { GPIOZ_9 };
static const unsigned int pdm_din1_z10_pins[]			= { GPIOZ_10 };
static const unsigned int pdm_din0_z11_pins[]			= { GPIOZ_11 };
static const unsigned int pdm_dclk_z12_pins[]			= { GPIOZ_12 };

/* GPIOZ func4 */
static const unsigned int spi_a_miso_z0_pins[]			= { GPIOZ_0 };
static const unsigned int spi_a_mosi_z1_pins[]			= { GPIOZ_1 };
static const unsigned int spi_a_clk_z2_pins[]			= { GPIOZ_2 };
static const unsigned int spi_a_ss0_z3_pins[]			= { GPIOZ_3 };
static const unsigned int spi_a_ss1_pins[]			= { GPIOZ_4 };
static const unsigned int spi_a_ss2_pins[]			= { GPIOZ_5 };
static const unsigned int hdmitx_test_o_z7_pins[]		= { GPIOZ_7 };
static const unsigned int i2cm_e_scl_z11_pins[]			= { GPIOZ_11 };
static const unsigned int i2cm_e_sda_z12_pins[]			= { GPIOZ_12 };

/* GPIOZ func5 */
static const unsigned int uart_d_tx_z0_pins[]			= { GPIOZ_0 };
static const unsigned int uart_d_rx_z1_pins[]			= { GPIOZ_1 };
static const unsigned int uart_d_cts_z2_pins[]			= { GPIOZ_2 };
static const unsigned int uart_d_rts_z3_pins[]			= { GPIOZ_3 };
static const unsigned int pwm_g_z4_pins[]			= { GPIOZ_4 };
static const unsigned int pwm_f_z5_pins[]			= { GPIOZ_5 };
static const unsigned int pwm_e_z6_pins[]			= { GPIOZ_6 };
static const unsigned int tsin_b_clk_z7_pins[]			= { GPIOZ_7 };
static const unsigned int tsin_b_sop_z10_pins[]			= { GPIOZ_10 };
static const unsigned int tsin_b_valid_z11_pins[]		= { GPIOZ_11 };
static const unsigned int tsin_b_d0_z12_pins[]			= { GPIOZ_12 };

/* GPIOZ func6 */
static const unsigned int eth_rgmii_rx_clk_pins[]		= { GPIOZ_0 };
static const unsigned int eth_rx_dv_pins[]			= { GPIOZ_1 };
static const unsigned int eth_rxd0_pins[]			= { GPIOZ_2 };
static const unsigned int eth_rxd1_pins[]			= { GPIOZ_3 };
static const unsigned int eth_rxd2_rgmii_pins[]			= { GPIOZ_4 };
static const unsigned int eth_rxd3_rgmii_pins[]			= { GPIOZ_5 };
static const unsigned int eth_rgmii_tx_clk_pins[]		= { GPIOZ_6 };
static const unsigned int eth_txen_pins[]			= { GPIOZ_7 };
static const unsigned int eth_txd0_pins[]			= { GPIOZ_8 };
static const unsigned int eth_txd1_pins[]			= { GPIOZ_9 };
static const unsigned int eth_txd2_rgmii_pins[]			= { GPIOZ_10 };
static const unsigned int eth_txd3_rgmii_pins[]			= { GPIOZ_11 };
static const unsigned int eth_mdio_pins[]			= { GPIOZ_12 };

/* GPIOZ func7 */
static const unsigned int gen_clk_z9_pins[]			= { GPIOZ_9 };
static const unsigned int tst_loop2_in_pins[]			= { GPIOZ_10 };
static const unsigned int tst_loop2_out_pins[]			= { GPIOZ_11 };
static const unsigned int gen_clk_z12_pins[]			= { GPIOZ_12 };

/* Bank CC func1 */
static const unsigned int i2cm_a_sda_cc1_pins[]			= { GPIO_CC1 };
static const unsigned int i2cm_a_scl_cc2_pins[]			= { GPIO_CC2 };

static struct meson_pmx_group meson_s7d_periphs_groups[] = {
	/* func0 as GPIO */
	GPIO_GROUP(GPIOE_0,		0),
	GPIO_GROUP(GPIOE_1,		0),
	GPIO_GROUP(GPIOB_0,		0),
	GPIO_GROUP(GPIOB_1,		0),
	GPIO_GROUP(GPIOB_2,		0),
	GPIO_GROUP(GPIOB_3,		0),
	GPIO_GROUP(GPIOB_4,		0),
	GPIO_GROUP(GPIOB_5,		0),
	GPIO_GROUP(GPIOB_6,		0),
	GPIO_GROUP(GPIOB_7,		0),
	GPIO_GROUP(GPIOB_8,		0),
	GPIO_GROUP(GPIOB_9,		0),
	GPIO_GROUP(GPIOB_10,		0),
	GPIO_GROUP(GPIOB_11,		0),
	GPIO_GROUP(GPIOB_12,		0),
	GPIO_GROUP(GPIOB_13,		0),
	GPIO_GROUP(GPIOC_0,		0),
	GPIO_GROUP(GPIOC_1,		0),
	GPIO_GROUP(GPIOC_2,		0),
	GPIO_GROUP(GPIOC_3,		0),
	GPIO_GROUP(GPIOC_4,		0),
	GPIO_GROUP(GPIOC_5,		0),
	GPIO_GROUP(GPIOC_6,		0),
	GPIO_GROUP(GPIOC_7,		0),
	GPIO_GROUP(GPIOD_0,		0),
	GPIO_GROUP(GPIOD_1,		0),
	GPIO_GROUP(GPIOD_2,		0),
	GPIO_GROUP(GPIOD_3,		0),
	GPIO_GROUP(GPIOD_4,		0),
	GPIO_GROUP(GPIODV_0,		0),
	GPIO_GROUP(GPIODV_1,		0),
	GPIO_GROUP(GPIODV_2,		0),
	GPIO_GROUP(GPIODV_3,		0),
	GPIO_GROUP(GPIODV_4,		0),
	GPIO_GROUP(GPIODV_5,		0),
	GPIO_GROUP(GPIODV_6,		0),
	GPIO_GROUP(GPIOH_0,		0),
	GPIO_GROUP(GPIOH_1,		0),
	GPIO_GROUP(GPIOH_2,		0),
	GPIO_GROUP(GPIOH_3,		0),
	GPIO_GROUP(GPIOH_4,		0),
	GPIO_GROUP(GPIOH_5,		0),
	GPIO_GROUP(GPIOH_6,		0),
	GPIO_GROUP(GPIOH_7,		0),
	GPIO_GROUP(GPIOH_8,		0),
	GPIO_GROUP(GPIOH_9,		0),
	GPIO_GROUP(GPIOH_10,		0),
	GPIO_GROUP(GPIOH_11,		0),
	GPIO_GROUP(GPIOX_0,		0),
	GPIO_GROUP(GPIOX_1,		0),
	GPIO_GROUP(GPIOX_2,		0),
	GPIO_GROUP(GPIOX_3,		0),
	GPIO_GROUP(GPIOX_4,		0),
	GPIO_GROUP(GPIOX_5,		0),
	GPIO_GROUP(GPIOX_6,		0),
	GPIO_GROUP(GPIOX_7,		0),
	GPIO_GROUP(GPIOX_8,		0),
	GPIO_GROUP(GPIOX_9,		0),
	GPIO_GROUP(GPIOX_10,		0),
	GPIO_GROUP(GPIOX_11,		0),
	GPIO_GROUP(GPIOX_12,		0),
	GPIO_GROUP(GPIOX_13,		0),
	GPIO_GROUP(GPIOX_14,		0),
	GPIO_GROUP(GPIOX_15,		0),
	GPIO_GROUP(GPIOX_16,		0),
	GPIO_GROUP(GPIOX_17,		0),
	GPIO_GROUP(GPIOX_18,		0),
	GPIO_GROUP(GPIOX_19,		0),
	GPIO_GROUP(GPIOZ_0,		0),
	GPIO_GROUP(GPIOZ_1,		0),
	GPIO_GROUP(GPIOZ_2,		0),
	GPIO_GROUP(GPIOZ_3,		0),
	GPIO_GROUP(GPIOZ_4,		0),
	GPIO_GROUP(GPIOZ_5,		0),
	GPIO_GROUP(GPIOZ_6,		0),
	GPIO_GROUP(GPIOZ_7,		0),
	GPIO_GROUP(GPIOZ_8,		0),
	GPIO_GROUP(GPIOZ_9,		0),
	GPIO_GROUP(GPIOZ_10,		0),
	GPIO_GROUP(GPIOZ_11,		0),
	GPIO_GROUP(GPIOZ_12,		0),
	GPIO_GROUP(GPIO_TEST_N,		0),
	GPIO_GROUP(GPIO_CC1,		0),
	GPIO_GROUP(GPIO_CC2,		0),

	/* GPIOE func1 */
	GROUP(i2cm_a_sda,		1),
	GROUP(i2cm_a_scl,		1),

	/* GPIOE func2 */
	GROUP(uart_b_tx_e0,		2),
	GROUP(uart_b_rx_e1,		2),

	/* GPIOE func3 */
	GROUP(pwm_h,			3),
	GROUP(pwm_j,			3),

	/* GPIOB func1 */
	GROUP(emmc_d0,			1),
	GROUP(emmc_d1,			1),
	GROUP(emmc_d2,			1),
	GROUP(emmc_d3,			1),
	GROUP(emmc_d4,			1),
	GROUP(emmc_d5,			1),
	GROUP(emmc_d6,			1),
	GROUP(emmc_d7,			1),
	GROUP(emmc_clk,			1),
	GROUP(emmc_rstgpio,		1),
	GROUP(emmc_cmd,			1),
	GROUP(emmc_ds,			1),

	/* GPIOB func2 */
	GROUP(nand_wen_clk,		2),
	GROUP(nand_ale,			2),
	GROUP(nand_ren_wr,		2),
	GROUP(nand_cle,			2),
	GROUP(nand_ce0,			2),

	/* GPIOB func3 */
	GROUP(spinf_rstgpio,		3),
	GROUP(spinf_hold_d3,		3),
	GROUP(spinf_mo_d0,		3),
	GROUP(spinf_mi_d1,		3),
	GROUP(spinf_clk200m,		3),
	GROUP(spinf_wp_d2,		3),
	GROUP(spinf_d4,			3),
	GROUP(spinf_d5,			3),
	GROUP(spinf_d6,			3),
	GROUP(spinf_d7,			3),
	GROUP(spinf_cs1,		3),
	GROUP(spinf_cs0,		3),

	/* GPIOC func1 */
	GROUP(sdcard_d0_c0,		1),
	GROUP(sdcard_d1_c1,		1),
	GROUP(sdcard_d2_c2,		1),
	GROUP(sdcard_d3_c3,		1),
	GROUP(sdcard_clk_c4,		1),
	GROUP(sdcard_cmd_c5,		1),
	GROUP(card_det,			1),

	/* GPIOC func2 */
	GROUP(jtag2_tdo,		2),
	GROUP(jtag2_tdi,		2),
	GROUP(uart_b_rx_c2,		2),
	GROUP(uart_b_tx_c3,		2),
	GROUP(jtag2_clk,		2),
	GROUP(jtag2_tms,		2),
	GROUP(i2cm_b_sda_c6,		2),
	GROUP(i2cm_b_scl_c7,		2),

	/* GPIOC func3 */
	GROUP(pdm_din1_c0,		3),
	GROUP(pdm_din0_c1,		3),
	GROUP(i2cm_e_scl_c2,		3),
	GROUP(i2cm_e_sda_c3,		3),
	GROUP(pdm_dclk_c4,		3),
	GROUP(iso7816_clk_c5,		3),
	GROUP(iso7816_data_c6,		3),

	/* GPIOC func4 */
	GROUP(tdm_d2_c0,		4),
	GROUP(tdm_d3_c1,		4),
	GROUP(tdm_fs1_c2,		4),
	GROUP(tdm_sclk1_c3,		4),
	GROUP(mclk_1_c4,		4),
	GROUP(tdm_d4_c5,		4),
	GROUP(tdm_d5_c6,		4),

	/* GPIOC func6 */
	GROUP(gen_clk_c4,		6),

	/* GPIOC func7 */
	GROUP(debug_o14,		7),
	GROUP(debug_o15,		7),

	/* GPIOD func1 */
	GROUP(uart_b_tx_d0,		1),
	GROUP(uart_b_rx_d1,		1),
	GROUP(uart_b_cts,		1),
	GROUP(uart_b_rts,		1),
	GROUP(ir_remote_out,		1),

	/* GPIOD func2 */
	GROUP(i2cm_e_scl_d2,		2),
	GROUP(i2cm_e_sda_d3,		2),
	GROUP(mclk_1_d4,		2),

	/* GPIOD func3 */
	GROUP(uart_c_tx,		3),
	GROUP(uart_c_rx,		3),
	GROUP(pwm_b_d4,			3),

	/* GPIOD func4 */
	GROUP(clk_32k_in,		4),
	GROUP(pwm_b_hiz,		4),

	/* GPIOD func5 */
	GROUP(mic_mute_en,		5),
	GROUP(mic_mute_key,		5),

	/* GPIOD func7 */
	GROUP(debug_o0,			7),
	GROUP(debug_o1,			7),
	GROUP(debug_o2,			7),
	GROUP(debug_o3,			7),
	GROUP(debug_o4,			7),

	/* GPIODV func1 */
	GROUP(ir_remote_input,		1),
	GROUP(jtag_clk,			1),
	GROUP(jtag_tms,			1),
	GROUP(jtag_tdi,			1),
	GROUP(jtag_tdo,			1),
	GROUP(clk12m_24m,		1),
	GROUP(pwm_g_hiz,		1),

	/* GPIODV func2 */
	GROUP(tdm_sclk1_dv1,		2),
	GROUP(tdm_fs1_dv2,		2),
	GROUP(tdm_d4_dv3,		2),
	GROUP(tdm_d3_dv4,		2),
	GROUP(tdm_d2_dv5,		2),
	GROUP(pwm_g_dv6,		2),

	/* GPIODV func3 */
	GROUP(pwm_a_dv1,		3),
	GROUP(pwm_c_dv2,		3),
	GROUP(pwm_d_dv3,		3),
	GROUP(pwm_i_dv4,		3),

	/* GPIODV func4 */
	GROUP(pwm_a_hiz,		4),
	GROUP(pwm_c_hiz,		4),
	GROUP(pdm_dclk_dv3,		4),
	GROUP(pdm_din0_dv4,		4),
	GROUP(pdm_din1_dv5,		4),

	/* GPIODV func5 */
	GROUP(i2cm_b_sda_dv1,		5),
	GROUP(i2cm_b_scl_dv2,		5),
	GROUP(i2cm_c_sda_dv5,		5),
	GROUP(i2cm_c_scl_dv6,		5),

	/* GPIODV func6 */
	GROUP(tsin_b_clk_dv1,		6),
	GROUP(tsin_b_sop_dv2,		6),
	GROUP(tsin_b_valid_dv3,		6),
	GROUP(tsin_b_d0_dv4,		6),
	GROUP(gen_clk_dv5,		6),
	GROUP(hdmitx_test_o_dv6,	6),

	/* GPIODV func7 */
	GROUP(debug_o5,			7),
	GROUP(debug_o6,			7),
	GROUP(debug_o7,			7),
	GROUP(debug_o8,			7),
	GROUP(debug_o9,			7),
	GROUP(debug_o10,		7),
	GROUP(debug_o11,		7),

	/* GPIOH func1 */
	GROUP(hdmitx_sda,		1),
	GROUP(hdmitx_scl,		1),
	GROUP(hdmitx_hpd_in,		1),
	GROUP(spdif_out_h4,		1),
	GROUP(spdif_in_h5,		1),
	GROUP(i2cm_b_sda_h6,		1),
	GROUP(i2cm_b_scl_h7,		1),
	GROUP(i2cm_c_sda_h8,		1),
	GROUP(i2cm_c_scl_h9,		1),
	GROUP(eth_link_led,		1),
	GROUP(eth_act_led,		1),

	/* GPIOH func2 */
	GROUP(i2cm_c_sda_h0,		2),
	GROUP(i2cm_c_scl_h1,		2),
	GROUP(ao_cec_b,			2),
	GROUP(uart_d_tx_h4,		2),
	GROUP(uart_d_rx_h5,		2),
	GROUP(uart_d_cts_h6,		2),
	GROUP(uart_d_rts_h7,		2),
	GROUP(iso7816_clk_h8,		2),
	GROUP(iso7816_data_h9,		2),
	GROUP(uart_e_tx_h10,		2),
	GROUP(uart_e_rx_h11,		2),

	/* GPIOH func3 */
	GROUP(pwm_d_h6,			3),
	GROUP(pwm_i_h7,			3),
	GROUP(pdm_dclk_h8,		3),
	GROUP(pdm_din0_h9,		3),
	GROUP(pdm_din1_h10,		3),

	/* GPIOH func4 */
	GROUP(mclk_2_h4,		4),
	GROUP(tdm_sclk2_h5,		4),
	GROUP(tdm_fs2_h6,		4),
	GROUP(tdm_d5_h7,		4),
	GROUP(tdm_d6_h8,		4),
	GROUP(tdm_d7_h9,		4),
	GROUP(eth_mdc,			4),

	/* GPIOH func5 */
	GROUP(spi_a_miso_h4,		5),
	GROUP(spi_a_mosi_h5,		5),
	GROUP(spi_a_clk_h6,		5),
	GROUP(spi_a_ss0_h7,		5),
	GROUP(spdif_in_h9,		5),

	/* GPIOH func6 */
	GROUP(tst_loop1_in,		6),
	GROUP(tst_loop1_out,		6),
	GROUP(tst_clk_1m_h2,		6),
	GROUP(tst_clk_1m_h3,		6),
	GROUP(tsin_b1_clk,		6),
	GROUP(tsin_b1_sop,		6),
	GROUP(tsin_b1_valid,		6),
	GROUP(tsin_b1_d0,		6),
	GROUP(gen_clk_h11,		6),

	/* GPIOH func7 */
	GROUP(debug_o16,		7),
	GROUP(debug_o17,		7),
	GROUP(debug_o18,		7),
	GROUP(debug_o19,		7),
	GROUP(debug_o12,		7),
	GROUP(debug_o13,		7),

	/* GPIOX func1 */
	GROUP(sdio_d0,			1),
	GROUP(sdio_d1,			1),
	GROUP(sdio_d2,			1),
	GROUP(sdio_d3,			1),
	GROUP(sdio_clk,			1),
	GROUP(sdio_cmd,			1),
	GROUP(pwm_a_x6,			1),
	GROUP(pwm_f_x7,			1),
	GROUP(tdm_d1,			1),
	GROUP(tdm_d0,			1),
	GROUP(tdm_fs0,			1),
	GROUP(tdm_sclk0,		1),
	GROUP(uart_a_tx,		1),
	GROUP(uart_a_rx,		1),
	GROUP(uart_a_cts,		1),
	GROUP(uart_a_rts,		1),
	GROUP(pwm_e_x16,		1),
	GROUP(i2cm_b_sda_x17,		1),
	GROUP(i2cm_b_scl_x18,		1),
	GROUP(pwm_b_x19,		1),

	/* GPIOX func2 */
	GROUP(pdm_din0_x8,		2),
	GROUP(pdm_din1_x9,		2),
	GROUP(pdm_dclk_x11,		2),

	/* GPIOX func3 */
	GROUP(spi_a_mosi_x8,		3),
	GROUP(spi_a_miso_x9,		3),
	GROUP(spi_a_ss0_x10,		3),
	GROUP(spi_a_clk_x11,		3),

	/* GPIOX func4 */
	GROUP(pwm_c_x8,			4),
	GROUP(i2cs_a_sda,		4),
	GROUP(i2cs_a_scl,		4),
	GROUP(hdmitx_test_o_x14,	4),

	/* GPIOX func5 */
	GROUP(i2cm_d_sda_x10,		5),
	GROUP(i2cm_d_scl_x11,		5),

	/* GPIOX func6 */
	GROUP(debug_i0,			6),
	GROUP(debug_i1,			6),
	GROUP(debug_i2,			6),
	GROUP(debug_i3,			6),
	GROUP(debug_i4,			6),
	GROUP(debug_i5,			6),
	GROUP(debug_i6,			6),
	GROUP(debug_i7,			6),
	GROUP(debug_i8,			6),
	GROUP(debug_i9,			6),
	GROUP(debug_i10,		6),
	GROUP(debug_i11,		6),
	GROUP(debug_i12,		6),
	GROUP(debug_i13,		6),
	GROUP(debug_i14,		6),
	GROUP(debug_i15,		6),
	GROUP(debug_i16,		6),
	GROUP(debug_i17,		6),
	GROUP(debug_i18,		6),
	GROUP(debug_i19,		6),

	/* GPIOX func7 */
	GROUP(tst_out0,			7),
	GROUP(tst_out1,			7),
	GROUP(tst_out2,			7),
	GROUP(tst_out3,			7),
	GROUP(tst_out4,			7),
	GROUP(tst_out5,			7),
	GROUP(tst_out6,			7),
	GROUP(tst_out7,			7),
	GROUP(tst_out8,			7),
	GROUP(tst_out9,			7),
	GROUP(tst_out10,		7),
	GROUP(tst_out11,		7),
	GROUP(tst_out12,		7),
	GROUP(tst_out13,		7),
	GROUP(tst_out14,		7),
	GROUP(tst_out15,		7),
	GROUP(tst_out16,		7),
	GROUP(tst_out17,		7),
	GROUP(tst_out18,		7),
	GROUP(tst_out19,		7),

	/* GPIOZ func1 */
	GROUP(tdm_fs2_z0,		1),
	GROUP(tdm_sclk2_z1,		1),
	GROUP(tdm_d4_z2,		1),
	GROUP(tdm_d5_z3,		1),
	GROUP(tdm_d6_z4,		1),
	GROUP(tdm_d7_z5,		1),
	GROUP(mclk_2_z6,		1),
	GROUP(tdm_d8,			1),
	GROUP(spdif_out_z9,		1),
	GROUP(uart_e_tx_z11,		1),
	GROUP(uart_e_rx_z12,		1),

	/* GPIOZ func2 */
	GROUP(tsin_a_clk,		2),
	GROUP(tsin_a_sop,		2),
	GROUP(tsin_a_valid,		2),
	GROUP(tsin_a_d0,		2),
	GROUP(i2cm_d_sda_z8,		2),
	GROUP(i2cm_d_scl_z9,		2),

	/* GPIOZ func3 */
	GROUP(sdcard_d0_z0,		3),
	GROUP(sdcard_d1_z1,		3),
	GROUP(sdcard_d2_z2,		3),
	GROUP(sdcard_d3_z3,		3),
	GROUP(sdcard_clk_z4,		3),
	GROUP(sdcard_cmd_z5,		3),
	GROUP(uart_e_tx_z8,		3),
	GROUP(uart_e_rx_z9,		3),
	GROUP(pdm_din1_z10,		3),
	GROUP(pdm_din0_z11,		3),
	GROUP(pdm_dclk_z12,		3),

	/* GPIOZ func4 */
	GROUP(spi_a_miso_z0,		4),
	GROUP(spi_a_mosi_z1,		4),
	GROUP(spi_a_clk_z2,		4),
	GROUP(spi_a_ss0_z3,		4),
	GROUP(spi_a_ss1,		4),
	GROUP(spi_a_ss2,		4),
	GROUP(hdmitx_test_o_z7,		4),
	GROUP(i2cm_e_scl_z11,		4),
	GROUP(i2cm_e_sda_z12,		4),

	/* GPIOZ func5 */
	GROUP(uart_d_tx_z0,		5),
	GROUP(uart_d_rx_z1,		5),
	GROUP(uart_d_cts_z2,		5),
	GROUP(uart_d_rts_z3,		5),
	GROUP(pwm_g_z4,			5),
	GROUP(pwm_f_z5,			5),
	GROUP(pwm_e_z6,			5),
	GROUP(tsin_b_clk_z7,		5),
	GROUP(tsin_b_sop_z10,		5),
	GROUP(tsin_b_valid_z11,		5),
	GROUP(tsin_b_d0_z12,		5),

	/* GPIOZ func6 */
	GROUP(eth_rgmii_rx_clk,		6),
	GROUP(eth_rx_dv,		6),
	GROUP(eth_rxd0,			6),
	GROUP(eth_rxd1,			6),
	GROUP(eth_rxd2_rgmii,		6),
	GROUP(eth_rxd3_rgmii,		6),
	GROUP(eth_rgmii_tx_clk,		6),
	GROUP(eth_txen,			6),
	GROUP(eth_txd0,			6),
	GROUP(eth_txd1,			6),
	GROUP(eth_txd2_rgmii,		6),
	GROUP(eth_txd3_rgmii,		6),
	GROUP(eth_mdio,			6),

	/* GPIOZ func7 */
	GROUP(gen_clk_z9,		7),
	GROUP(tst_loop2_in,		7),
	GROUP(tst_loop2_out,		7),
	GROUP(gen_clk_z12,		7),

	/* Bank CC func1 */
	GROUP(i2cm_a_sda_cc1,		1),
	GROUP(i2cm_a_scl_cc2,		1)
};

static const char * const gpio_periphs_groups[] = {
	"GPIOE_0", "GPIOE_1", "GPIOB_0", "GPIOB_1",
	"GPIOB_2", "GPIOB_3", "GPIOB_4", "GPIOB_5",
	"GPIOB_6", "GPIOB_7", "GPIOB_8", "GPIOB_9",
	"GPIOB_10", "GPIOB_11", "GPIOB_12", "GPIOB_13",
	"GPIOC_0", "GPIOC_1", "GPIOC_2", "GPIOC_3",
	"GPIOC_4", "GPIOC_5", "GPIOC_6", "GPIOC_7",
	"GPIOD_0", "GPIOD_1", "GPIOD_2", "GPIOD_3",
	"GPIOD_4", "GPIODV_0", "GPIODV_1", "GPIODV_2",
	"GPIODV_3", "GPIODV_4", "GPIODV_5", "GPIODV_6",
	"GPIOH_0", "GPIOH_1", "GPIOH_2", "GPIOH_3",
	"GPIOH_4", "GPIOH_5", "GPIOH_6", "GPIOH_7",
	"GPIOH_8", "GPIOH_9", "GPIOH_10", "GPIOH_11",
	"GPIOX_0", "GPIOX_1", "GPIOX_2", "GPIOX_3",
	"GPIOX_4", "GPIOX_5", "GPIOX_6", "GPIOX_7",
	"GPIOX_8", "GPIOX_9", "GPIOX_10", "GPIOX_11",
	"GPIOX_12", "GPIOX_13", "GPIOX_14", "GPIOX_15",
	"GPIOX_16", "GPIOX_17", "GPIOX_18", "GPIOX_19",
	"GPIOZ_0", "GPIOZ_1", "GPIOZ_2", "GPIOZ_3",
	"GPIOZ_4", "GPIOZ_5", "GPIOZ_6", "GPIOZ_7",
	"GPIOZ_8", "GPIOZ_9", "GPIOZ_10", "GPIOZ_11",
	"GPIOZ_12", "GPIO_TEST_N", "GPIO_CC1", "GPIO_CC2"
};

static const char * const ao_cec_b_groups[] = {
	"ao_cec_b",
};

static const char * const card_det_groups[] = {
	"card_det",
};

static const char * const clk12m_24m_groups[] = {
	"clk12m_24m",
};

static const char * const clk_32k_in_groups[] = {
	"clk_32k_in",
};

static const char * const debug_groups[] = {
	"debug_o14", "debug_o15", "debug_o0", "debug_o1",
	"debug_o2", "debug_o3", "debug_o4", "debug_o5",
	"debug_o6", "debug_o7", "debug_o8", "debug_o9",
	"debug_o10", "debug_o11", "debug_o16", "debug_o17",
	"debug_o18", "debug_o19", "debug_o12", "debug_o13",
	"debug_i0", "debug_i1", "debug_i2", "debug_i3",
	"debug_i4", "debug_i5", "debug_i6", "debug_i7",
	"debug_i8", "debug_i9", "debug_i10", "debug_i11",
	"debug_i12", "debug_i13", "debug_i14", "debug_i15",
	"debug_i16", "debug_i17", "debug_i18", "debug_i19",
};

static const char * const emmc_groups[] = {
	"emmc_d0", "emmc_d1", "emmc_d2", "emmc_d3",
	"emmc_d4", "emmc_d5", "emmc_d6", "emmc_d7",
	"emmc_clk", "emmc_rstgpio", "emmc_cmd", "emmc_ds",
};

static const char * const eth_groups[] = {
	"eth_link_led", "eth_act_led", "eth_mdc", "eth_rgmii_rx_clk",
	"eth_rx_dv", "eth_rxd0", "eth_rxd1", "eth_rxd2_rgmii",
	"eth_rxd3_rgmii", "eth_rgmii_tx_clk", "eth_txen", "eth_txd0",
	"eth_txd1", "eth_txd2_rgmii", "eth_txd3_rgmii", "eth_mdio",
};

static const char * const gen_clk_groups[] = {
	"gen_clk_c4", "gen_clk_dv5", "gen_clk_h11", "gen_clk_z9",
	"gen_clk_z12",
};

static const char * const hdmitx_groups[] = {
	"hdmitx_test_o_dv6", "hdmitx_sda", "hdmitx_scl", "hdmitx_hpd_in",
	"hdmitx_test_o_x14", "hdmitx_test_o_z7",
};

static const char * const i2cm_a_groups[] = {
	"i2cm_a_sda", "i2cm_a_scl", "i2cm_a_sda_cc1", "i2cm_a_scl_cc2",
};

static const char * const i2cm_b_groups[] = {
	"i2cm_b_sda_c6", "i2cm_b_scl_c7", "i2cm_b_sda_dv1", "i2cm_b_scl_dv2",
	"i2cm_b_sda_h6", "i2cm_b_scl_h7", "i2cm_b_sda_x17", "i2cm_b_scl_x18",
};

static const char * const i2cm_c_groups[] = {
	"i2cm_c_sda_dv5", "i2cm_c_scl_dv6", "i2cm_c_sda_h0", "i2cm_c_scl_h1",
	"i2cm_c_sda_h8", "i2cm_c_scl_h9",
};

static const char * const i2cm_d_groups[] = {
	"i2cm_d_sda_x10", "i2cm_d_scl_x11", "i2cm_d_sda_z8", "i2cm_d_scl_z9",
};

static const char * const i2cm_e_groups[] = {
	"i2cm_e_scl_c2", "i2cm_e_sda_c3", "i2cm_e_scl_d2", "i2cm_e_sda_d3",
	"i2cm_e_scl_z11", "i2cm_e_sda_z12",
};

static const char * const i2cs_a_groups[] = {
	"i2cs_a_sda", "i2cs_a_scl",
};

static const char * const ir_remote_groups[] = {
	"ir_remote_out", "ir_remote_input",
};

static const char * const iso7816_groups[] = {
	"iso7816_clk_c5", "iso7816_data_c6", "iso7816_clk_h8", "iso7816_data_h9",
};

static const char * const jtag2_groups[] = {
	"jtag2_tdo", "jtag2_tdi", "jtag2_clk", "jtag2_tms",
};

static const char * const jtag_groups[] = {
	"jtag_clk", "jtag_tms", "jtag_tdi", "jtag_tdo",
};

static const char * const mclk_groups[] = {
	"mclk_1_c4", "mclk_1_d4", "mclk_2_h4", "mclk_2_z6",
};

static const char * const mic_mute_groups[] = {
	"mic_mute_en", "mic_mute_key",
};

static const char * const nand_groups[] = {
	"nand_wen_clk", "nand_ale", "nand_ren_wr", "nand_cle",
	"nand_ce0",
};

static const char * const pdm_groups[] = {
	"pdm_din1_c0", "pdm_din0_c1", "pdm_dclk_c4", "pdm_dclk_dv3",
	"pdm_din0_dv4", "pdm_din1_dv5", "pdm_dclk_h8", "pdm_din0_h9",
	"pdm_din1_h10", "pdm_din0_x8", "pdm_din1_x9", "pdm_dclk_x11",
	"pdm_din1_z10", "pdm_din0_z11", "pdm_dclk_z12",
};

static const char * const pwm_a_groups[] = {
	"pwm_a_dv1", "pwm_a_hiz", "pwm_a_x6",
};

static const char * const pwm_b_groups[] = {
	"pwm_b_d4", "pwm_b_hiz", "pwm_b_x19",
};

static const char * const pwm_c_groups[] = {
	"pwm_c_dv2", "pwm_c_hiz", "pwm_c_x8",
};

static const char * const pwm_d_groups[] = {
	"pwm_d_dv3", "pwm_d_h6",
};

static const char * const pwm_e_groups[] = {
	"pwm_e_x16", "pwm_e_z6",
};

static const char * const pwm_f_groups[] = {
	"pwm_f_x7", "pwm_f_z5",
};

static const char * const pwm_g_groups[] = {
	"pwm_g_hiz", "pwm_g_dv6", "pwm_g_z4",
};

static const char * const pwm_h_groups[] = {
	"pwm_h",
};

static const char * const pwm_i_groups[] = {
	"pwm_i_dv4", "pwm_i_h7",
};

static const char * const pwm_j_groups[] = {
	"pwm_j",
};

static const char * const sdcard_groups[] = {
	"sdcard_d0_c0", "sdcard_d1_c1", "sdcard_d2_c2", "sdcard_d3_c3",
	"sdcard_clk_c4", "sdcard_cmd_c5", "sdcard_d0_z0", "sdcard_d1_z1",
	"sdcard_d2_z2", "sdcard_d3_z3", "sdcard_clk_z4", "sdcard_cmd_z5",
};

static const char * const sdio_groups[] = {
	"sdio_d0", "sdio_d1", "sdio_d2", "sdio_d3",
	"sdio_clk", "sdio_cmd",
};

static const char * const spdif_groups[] = {
	"spdif_out_h4", "spdif_in_h5", "spdif_in_h9", "spdif_out_z9",
};

static const char * const spi_a_groups[] = {
	"spi_a_miso_h4", "spi_a_mosi_h5", "spi_a_clk_h6", "spi_a_ss0_h7",
	"spi_a_mosi_x8", "spi_a_miso_x9", "spi_a_ss0_x10", "spi_a_clk_x11",
	"spi_a_miso_z0", "spi_a_mosi_z1", "spi_a_clk_z2", "spi_a_ss0_z3",
	"spi_a_ss1", "spi_a_ss2",
};

static const char * const spinf_groups[] = {
	"spinf_rstgpio", "spinf_hold_d3", "spinf_mo_d0", "spinf_mi_d1",
	"spinf_clk200m", "spinf_wp_d2", "spinf_d4", "spinf_d5",
	"spinf_d6", "spinf_d7", "spinf_cs1", "spinf_cs0",
};

static const char * const tdm_groups[] = {
	"tdm_d2_c0", "tdm_d3_c1", "tdm_fs1_c2", "tdm_sclk1_c3",
	"tdm_d4_c5", "tdm_d5_c6", "tdm_sclk1_dv1", "tdm_fs1_dv2",
	"tdm_d4_dv3", "tdm_d3_dv4", "tdm_d2_dv5", "tdm_sclk2_h5",
	"tdm_fs2_h6", "tdm_d5_h7", "tdm_d6_h8", "tdm_d7_h9",
	"tdm_d1", "tdm_d0", "tdm_fs0", "tdm_sclk0",
	"tdm_fs2_z0", "tdm_sclk2_z1", "tdm_d4_z2", "tdm_d5_z3",
	"tdm_d6_z4", "tdm_d7_z5", "tdm_d8",
};

static const char * const tsin_groups[] = {
	"tsin_b_clk_dv1", "tsin_b_sop_dv2", "tsin_b_valid_dv3", "tsin_b_d0_dv4",
	"tsin_b1_clk", "tsin_b1_sop", "tsin_b1_valid", "tsin_b1_d0",
	"tsin_a_clk", "tsin_a_sop", "tsin_a_valid", "tsin_a_d0",
	"tsin_b_clk_z7", "tsin_b_sop_z10", "tsin_b_valid_z11", "tsin_b_d0_z12",
};

static const char * const tst_groups[] = {
	"tst_loop1_in", "tst_loop1_out", "tst_clk_1m_h2", "tst_clk_1m_h3",
	"tst_out0", "tst_out1", "tst_out2", "tst_out3",
	"tst_out4", "tst_out5", "tst_out6", "tst_out7",
	"tst_out8", "tst_out9", "tst_out10", "tst_out11",
	"tst_out12", "tst_out13", "tst_out14", "tst_out15",
	"tst_out16", "tst_out17", "tst_out18", "tst_out19",
	"tst_loop2_in", "tst_loop2_out",
};

static const char * const uart_a_groups[] = {
	"uart_a_tx", "uart_a_rx", "uart_a_cts", "uart_a_rts",
};

static const char * const uart_b_groups[] = {
	"uart_b_tx_e0", "uart_b_rx_e1", "uart_b_rx_c2", "uart_b_tx_c3",
	"uart_b_tx_d0", "uart_b_rx_d1", "uart_b_cts", "uart_b_rts",
};

static const char * const uart_c_groups[] = {
	"uart_c_tx", "uart_c_rx",
};

static const char * const uart_d_groups[] = {
	"uart_d_tx_h4", "uart_d_rx_h5", "uart_d_cts_h6", "uart_d_rts_h7",
	"uart_d_tx_z0", "uart_d_rx_z1", "uart_d_cts_z2", "uart_d_rts_z3",
};

static const char * const uart_e_groups[] = {
	"uart_e_tx_h10", "uart_e_rx_h11", "uart_e_tx_z8", "uart_e_rx_z9",
	"uart_e_tx_z11", "uart_e_rx_z12",
};

static struct meson_pmx_func meson_s7d_periphs_functions[] = {
	FUNCTION(gpio_periphs),
	FUNCTION(ao_cec_b),
	FUNCTION(card_det),
	FUNCTION(clk12m_24m),
	FUNCTION(clk_32k_in),
	FUNCTION(debug),
	FUNCTION(emmc),
	FUNCTION(eth),
	FUNCTION(gen_clk),
	FUNCTION(hdmitx),
	FUNCTION(i2cm_a),
	FUNCTION(i2cm_b),
	FUNCTION(i2cm_c),
	FUNCTION(i2cm_d),
	FUNCTION(i2cm_e),
	FUNCTION(i2cs_a),
	FUNCTION(ir_remote),
	FUNCTION(iso7816),
	FUNCTION(jtag2),
	FUNCTION(jtag),
	FUNCTION(mclk),
	FUNCTION(mic_mute),
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
	FUNCTION(pwm_i),
	FUNCTION(pwm_j),
	FUNCTION(sdcard),
	FUNCTION(sdio),
	FUNCTION(spdif),
	FUNCTION(spi_a),
	FUNCTION(spinf),
	FUNCTION(tdm),
	FUNCTION(tsin),
	FUNCTION(tst),
	FUNCTION(uart_a),
	FUNCTION(uart_b),
	FUNCTION(uart_c),
	FUNCTION(uart_d),
	FUNCTION(uart_e),
};

static struct meson_bank meson_s7d_periphs_banks[] = {
	/*      name  first       last    pullen  pull  dir  out   in   ds */
	BANK_DS("E",  GPIOE_0,  GPIOE_1,
		0x043,  0, 0x044,  0, 0x042,  0, 0x041,  0, 0x040,  0, 0x047,  0),
	BANK_DS("B",  GPIOB_0, GPIOB_13,
		0x063,  0, 0x064,  0, 0x062,  0, 0x061,  0, 0x060,  0, 0x067,  0),
	BANK_DS("C",  GPIOC_0,  GPIOC_7,
		0x053,  0, 0x054,  0, 0x052,  0, 0x051,  0, 0x050,  0, 0x057,  0),
	BANK_DS("D",  GPIOD_0,  GPIOD_4,
		0x033,  0, 0x034,  0, 0x032,  0, 0x031,  0, 0x030,  0, 0x037,  0),
	BANK_DS("DV", GPIODV_0, GPIODV_6,
		0x073,  0, 0x074,  0, 0x072,  0, 0x071,  0, 0x070,  0, 0x077,  0),
	BANK_DS("H",  GPIOH_0, GPIOH_11,
		0x023,  0, 0x024,  0, 0x022,  0, 0x021,  0, 0x020,  0, 0x027,  0),
	BANK_DS("X",  GPIOX_0, GPIOX_19,
		0x013,  0, 0x014,  0, 0x012,  0, 0x011,  0, 0x010,  0, 0x017,  0),
	BANK_DS("Z",  GPIOZ_0, GPIOZ_12,
		0x003,  0, 0x004,  0, 0x002,  0, 0x001,  0, 0x000,  0, 0x007,  0),
	BANK_DS("TEST_N", GPIO_TEST_N, GPIO_TEST_N,
		0x083,  0, 0x084,  0, 0x082,  0, 0x081,  0, 0x080,  0, 0x087,  0),
	BANK_DS("CC", GPIO_CC1, GPIO_CC2,
		0x091,  2, 0x091,  4, 0x092,  0, 0x091,  0, 0x090,  0, 0x091,  6),
};

static struct meson_pmx_bank meson_s7d_periphs_pmx_banks[] = {
	/*       name      first        last         reg   offset */
	BANK_PMX("E",      GPIOE_0,     GPIOE_1,     0x012,   0),
	BANK_PMX("B",      GPIOB_0,     GPIOB_13,    0x000,   0),
	BANK_PMX("C",      GPIOC_0,     GPIOC_7,     0x009,   0),
	BANK_PMX("D",      GPIOD_0,     GPIOD_4,     0x010,   0),
	BANK_PMX("DV",     GPIODV_0,    GPIODV_6,    0x002,   0),
	BANK_PMX("H",      GPIOH_0,     GPIOH_11,    0x00b,   0),
	BANK_PMX("X",      GPIOX_0,     GPIOX_19,    0x003,   0),
	BANK_PMX("Z",      GPIOZ_0,     GPIOZ_12,    0x006,   0),
	BANK_PMX("TEST_N", GPIO_TEST_N, GPIO_TEST_N, 0x00f,   0),
	BANK_PMX("CC",     GPIO_CC1,    GPIO_CC2,    0x005,  24),
};

static struct meson_axg_pmx_data meson_s7d_periphs_pmx_banks_data = {
	.pmx_banks	= meson_s7d_periphs_pmx_banks,
	.num_pmx_banks	= ARRAY_SIZE(meson_s7d_periphs_pmx_banks),
};

static int meson_s7d_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_ds = pc->reg_gpio;

	return 0;
}

static struct meson_pinctrl_data meson_s7d_periphs_pinctrl_data = {
	.name		= "periphs-banks",
	.groups		= meson_s7d_periphs_groups,
	.funcs		= meson_s7d_periphs_functions,
	.banks		= meson_s7d_periphs_banks,
	.num_pins	= 84,
	.num_groups	= ARRAY_SIZE(meson_s7d_periphs_groups),
	.num_funcs	= ARRAY_SIZE(meson_s7d_periphs_functions),
	.num_banks	= ARRAY_SIZE(meson_s7d_periphs_banks),
	.gpio_driver	= &meson_axg_gpio_driver,
	.pmx_data	= &meson_s7d_periphs_pmx_banks_data,
	.parse_dt	= meson_s7d_parse_dt_extra,
};

static const struct udevice_id meson_s7d_pinctrl_match[] = {
	{
		.compatible = "amlogic,meson-s7d-periphs-pinctrl",
		.data = (ulong)&meson_s7d_periphs_pinctrl_data,
	},
	{ }
};

U_BOOT_DRIVER(meson_s7d_pinctrl) = {
	.name	= "meson-s7d-pinctrl",
	.id	= UCLASS_PINCTRL,
	.of_match = of_match_ptr(meson_s7d_pinctrl_match),
	.probe = meson_pinctrl_probe,
	.priv_auto = sizeof(struct meson_pinctrl),
	.ops = &meson_axg_pinctrl_ops,
};
