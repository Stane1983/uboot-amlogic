// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <dt-bindings/amlogic/gpio/meson-s6-gpio.h>

#include <../../pinctrl/meson/pinctrl-meson-axg.h>

/* GPIOD func1 */
static const unsigned int uart_b_tx_d_pins[]			= { GPIOD_0 };
static const unsigned int uart_b_rx_d_pins[]			= { GPIOD_1 };
static const unsigned int jtag_clk_d_pins[]			= { GPIOD_2 };
static const unsigned int jtag_tms_d_pins[]			= { GPIOD_3 };
static const unsigned int jtag_tdi_d_pins[]			= { GPIOD_4 };
static const unsigned int jtag_tdo_d_pins[]			= { GPIOD_5 };
static const unsigned int ao_cec_d_pins[]			= { GPIOD_6 };

/* GPIOD func2 */
static const unsigned int i2c0_scl_d_pins[]			= { GPIOD_0 };
static const unsigned int i2c0_sda_d_pins[]			= { GPIOD_1 };
static const unsigned int i2c1_scl_d_pins[]			= { GPIOD_2 };
static const unsigned int i2c1_sda_d_pins[]			= { GPIOD_3 };
static const unsigned int i2c2_scl_d_pins[]			= { GPIOD_4 };
static const unsigned int i2c2_sda_d_pins[]			= { GPIOD_5 };
static const unsigned int ir_remote_in_d_pins[]			= { GPIOD_6 };

/* GPIOD func3 */
static const unsigned int pwm_c_d2_pins[]			= { GPIOD_2 };
static const unsigned int pwm_b_d_pins[]			= { GPIOD_3 };
static const unsigned int uart_c_tx_d_pins[]			= { GPIOD_4 };
static const unsigned int uart_c_rx_d_pins[]			= { GPIOD_5 };
static const unsigned int pwm_c_d6_pins[]			= { GPIOD_6 };

/* GPIOD func4 */
static const unsigned int tsin_a_sop_d_pins[]			= { GPIOD_2 };
static const unsigned int tsin_a_din0_d_pins[]			= { GPIOD_3 };
static const unsigned int tsin_a_clk_d_pins[]			= { GPIOD_4 };
static const unsigned int tsin_a_valid_d_pins[]			= { GPIOD_5 };
static const unsigned int spdif_out_d_pins[]			= { GPIOD_6 };

/* GPIOD func5 */
static const unsigned int tdm_d2_d_pins[]			= { GPIOD_2 };
static const unsigned int tdm_fs1_d_pins[]			= { GPIOD_3 };
static const unsigned int tdm_sclk1_d_pins[]			= { GPIOD_4 };
static const unsigned int mclk_1_d_pins[]			= { GPIOD_5 };
static const unsigned int tdm_d3_d_pins[]			= { GPIOD_6 };

/* GPIOD func6 */
static const unsigned int pdm_din0_d_pins[]			= { GPIOD_2 };
static const unsigned int pdm_din1_d_pins[]			= { GPIOD_3 };
static const unsigned int pdm_din2_d_pins[]			= { GPIOD_4 };
static const unsigned int pdm_dclk_d_pins[]			= { GPIOD_5 };
static const unsigned int clk12m_24m_d_pins[]			= { GPIOD_6 };

/* GPIOF func1 */
static const unsigned int i2c0_scl_f_pins[]			= { GPIOF_0 };
static const unsigned int i2c0_sda_f_pins[]			= { GPIOF_1 };
static const unsigned int ir_remote_out_f_pins[]		= { GPIOF_2 };
static const unsigned int ir_remote_in_f3_pins[]		= { GPIOF_3 };
static const unsigned int pwm_c_f_pins[]			= { GPIOF_4 };

/* GPIOF func2 */
static const unsigned int uart_c_tx_f_pins[]			= { GPIOF_0 };
static const unsigned int uart_c_rx_f_pins[]			= { GPIOF_1 };
static const unsigned int clk_32k_in_pins[]			= { GPIOF_2 };
static const unsigned int pwm_c_hiz_pins[]			= { GPIOF_4 };

/* GPIOF func3 */
static const unsigned int i2c_slave_scl_pins[]			= { GPIOF_0 };
static const unsigned int i2c_slave_sda_pins[]			= { GPIOF_1 };
static const unsigned int pwm_a_f_pins[]			= { GPIOF_2 };
static const unsigned int pwm_b_f_pins[]			= { GPIOF_3 };
static const unsigned int gen_clk_f_pins[]			= { GPIOF_4 };

/* GPIOF func4 */
static const unsigned int spdif_in_f_pins[]			= { GPIOF_0 };
static const unsigned int ir_remote_in_f1_pins[]		= { GPIOF_1 };
static const unsigned int pwm_a_hiz_pins[]			= { GPIOF_2 };
static const unsigned int pwm_b_hiz_pins[]			= { GPIOF_3 };

/* GPIOF func5 */
static const unsigned int tdm_d4_f_pins[]			= { GPIOF_0 };
static const unsigned int tdm_d5_f_pins[]			= { GPIOF_1 };
static const unsigned int i2c3_scl_f_pins[]			= { GPIOF_2 };
static const unsigned int i2c3_sda_f_pins[]			= { GPIOF_3 };

/* GPIOF func6 */
static const unsigned int spi_a_mosi_f_pins[]			= { GPIOF_0 };
static const unsigned int spi_a_miso_f_pins[]			= { GPIOF_1 };
static const unsigned int spi_a_ss0_f_pins[]			= { GPIOF_2 };
static const unsigned int spi_a_clk_f_pins[]			= { GPIOF_3 };

/* GPIOF func7 */
static const unsigned int mic_mute_en_pins[]			= { GPIOF_0 };
static const unsigned int mic_mute_key_pins[]			= { GPIOF_1 };

/* GPIOE func1 */
static const unsigned int i2c0_scl_e_pins[]			= { GPIOE_0 };
static const unsigned int i2c0_sda_e_pins[]			= { GPIOE_1 };
static const unsigned int clk12m_24m_e_pins[]			= { GPIOE_2 };

/* GPIOE func2 */
static const unsigned int uart_b_tx_e_pins[]			= { GPIOE_0 };
static const unsigned int uart_b_rx_e_pins[]			= { GPIOE_1 };
static const unsigned int pwm_a_e_pins[]			= { GPIOE_2 };

/* GPIOE func3 */
static const unsigned int pwm_d_pins[]				= { GPIOE_0 };
static const unsigned int pwm_f_e_pins[]			= { GPIOE_1 };
static const unsigned int gen_clk_e_pins[]			= { GPIOE_2 };

/* GPIOB func1 */
static const unsigned int emmc_nand_d0_pins[]			= { GPIOB_0 };
static const unsigned int emmc_nand_d1_pins[]			= { GPIOB_1 };
static const unsigned int emmc_nand_d2_pins[]			= { GPIOB_2 };
static const unsigned int emmc_nand_d3_pins[]			= { GPIOB_3 };
static const unsigned int emmc_nand_d4_pins[]			= { GPIOB_4 };
static const unsigned int emmc_nand_d5_pins[]			= { GPIOB_5 };
static const unsigned int emmc_nand_d6_pins[]			= { GPIOB_6 };
static const unsigned int emmc_nand_d7_pins[]			= { GPIOB_7 };
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
static const unsigned int spinf_clk_pins[]			= { GPIOB_6 };
static const unsigned int spinf_wp_d2_pins[]			= { GPIOB_7 };
static const unsigned int spinf_d4_pins[]			= { GPIOB_8 };
static const unsigned int spinf_d5_pins[]			= { GPIOB_9 };
static const unsigned int spinf_d6_pins[]			= { GPIOB_10 };
static const unsigned int spinf_d7_pins[]			= { GPIOB_11 };
static const unsigned int spinf_cs1_pins[]			= { GPIOB_12 };
static const unsigned int spinf_cs0_pins[]			= { GPIOB_13 };

/* GPIOB func4 */
static const unsigned int i2c4_sda_b_pins[]			= { GPIOB_12 };
static const unsigned int i2c4_scl_b_pins[]			= { GPIOB_13 };

/* GPIOB func6 */
static const unsigned int pdm_din0_b_pins[]			= { GPIOB_12 };
static const unsigned int pdm_dclk_b_pins[]			= { GPIOB_13 };

/* GPIOC func1 */
static const unsigned int sdcard_d0_pins[]			= { GPIOC_0 };
static const unsigned int sdcard_d1_pins[]			= { GPIOC_1 };
static const unsigned int sdcard_d2_pins[]			= { GPIOC_2 };
static const unsigned int sdcard_d3_pins[]			= { GPIOC_3 };
static const unsigned int sdcard_clk_pins[]			= { GPIOC_4 };
static const unsigned int sdcard_cmd_pins[]			= { GPIOC_5 };
static const unsigned int card_det_pins[]			= { GPIOC_6 };
static const unsigned int pcieck_reqn_pins[]			= { GPIOC_7 };

/* GPIOC func2 */
static const unsigned int jtag2_tdo_pins[]			= { GPIOC_0 };
static const unsigned int jtag2_tdi_pins[]			= { GPIOC_1 };
static const unsigned int uart_b_rx_c_pins[]			= { GPIOC_2 };
static const unsigned int uart_b_tx_c_pins[]			= { GPIOC_3 };
static const unsigned int jtag2_clk_pins[]			= { GPIOC_4 };
static const unsigned int jtag2_tms_pins[]			= { GPIOC_5 };
static const unsigned int ir_remote_in_c_pins[]			= { GPIOC_7 };

/* GPIOC func3 */
static const unsigned int tsin_a_clk_c_pins[]			= { GPIOC_0 };
static const unsigned int tsin_a_sop_c_pins[]			= { GPIOC_1 };
static const unsigned int tsin_a_valid_c_pins[]			= { GPIOC_2 };
static const unsigned int tsin_a_din0_c_pins[]			= { GPIOC_3 };
static const unsigned int gen_clk_c_pins[]			= { GPIOC_4 };
static const unsigned int i2c1_sda_c_pins[]			= { GPIOC_5 };
static const unsigned int i2c1_scl_c_pins[]			= { GPIOC_6 };

/* GPIOC func4 */
static const unsigned int pdm_din0_c_pins[]			= { GPIOC_0 };
static const unsigned int pdm_din1_c_pins[]			= { GPIOC_1 };
static const unsigned int pdm_din2_c_pins[]			= { GPIOC_2 };
static const unsigned int pdm_din3_c_pins[]			= { GPIOC_3 };
static const unsigned int pdm_dclk_c_pins[]			= { GPIOC_4 };
static const unsigned int iso7816_clk_c5_pins[]			= { GPIOC_5 };
static const unsigned int iso7816_data_c6_pins[]		= { GPIOC_6 };

/* GPIOC func5 */
static const unsigned int tdm_d6_c_pins[]			= { GPIOC_0 };
static const unsigned int tdm_d7_c_pins[]			= { GPIOC_1 };
static const unsigned int tdm_fs1_c_pins[]			= { GPIOC_2 };
static const unsigned int tdm_sclk1_c_pins[]			= { GPIOC_3 };
static const unsigned int mclk_1_c_pins[]			= { GPIOC_4 };
static const unsigned int tdm_d8_c_pins[]			= { GPIOC_5 };
static const unsigned int tdm_d9_c_pins[]			= { GPIOC_6 };

/* GPIOC func6 */
static const unsigned int tsin_b_clk_c_pins[]			= { GPIOC_0 };
static const unsigned int tsin_b_sop_c_pins[]			= { GPIOC_1 };
static const unsigned int tsin_b_valid_c_pins[]			= { GPIOC_2 };
static const unsigned int tsin_b_din0_c_pins[]			= { GPIOC_3 };
static const unsigned int tdm_d3_c_pins[]			= { GPIOC_4 };

/* GPIOC func7 */
static const unsigned int iso7816_clk_c0_pins[]			= { GPIOC_0 };
static const unsigned int iso7816_data_c1_pins[]		= { GPIOC_1 };

/* GPIOX func1 */
static const unsigned int sdio_d0_pins[]			= { GPIOX_0 };
static const unsigned int sdio_d1_pins[]			= { GPIOX_1 };
static const unsigned int sdio_d2_pins[]			= { GPIOX_2 };
static const unsigned int sdio_d3_pins[]			= { GPIOX_3 };
static const unsigned int sdio_clk_pins[]			= { GPIOX_4 };
static const unsigned int sdio_cmd_pins[]			= { GPIOX_5 };
static const unsigned int uart_a_tx_x6_pins[]			= { GPIOX_6 };
static const unsigned int uart_a_rx_x7_pins[]			= { GPIOX_7 };
static const unsigned int tdm_d1_x_pins[]			= { GPIOX_8 };
static const unsigned int tdm_d0_x_pins[]			= { GPIOX_9 };
static const unsigned int tdm_fs0_x_pins[]			= { GPIOX_10 };
static const unsigned int tdm_sclk0_x_pins[]			= { GPIOX_11 };
static const unsigned int uart_a_tx_x12_pins[]			= { GPIOX_12 };
static const unsigned int uart_a_rx_x13_pins[]			= { GPIOX_13 };
static const unsigned int uart_a_cts_pins[]			= { GPIOX_14 };
static const unsigned int uart_a_rts_pins[]			= { GPIOX_15 };
static const unsigned int pwm_e_pins[]				= { GPIOX_16 };
static const unsigned int i2c1_sda_x_pins[]			= { GPIOX_17 };
static const unsigned int i2c1_scl_x_pins[]			= { GPIOX_18 };
static const unsigned int pwm_b_x_pins[]			= { GPIOX_19 };

/* GPIOX func2 */
static const unsigned int pdm_din0_x0_pins[]			= { GPIOX_0 };
static const unsigned int pdm_din1_x1_pins[]			= { GPIOX_1 };
static const unsigned int pdm_din2_x2_pins[]			= { GPIOX_2 };
static const unsigned int pdm_din3_x3_pins[]			= { GPIOX_3 };
static const unsigned int pdm_dclk_x4_pins[]			= { GPIOX_4 };
static const unsigned int pwm_c_x5_pins[]			= { GPIOX_5 };
static const unsigned int pwm_a_x6_pins[]			= { GPIOX_6 };
static const unsigned int pwm_f_x_pins[]			= { GPIOX_7 };
static const unsigned int pdm_dclk_x8_pins[]			= { GPIOX_8 };
static const unsigned int pdm_din0_x9_pins[]			= { GPIOX_9 };
static const unsigned int pdm_din1_x10_pins[]			= { GPIOX_10 };
static const unsigned int pdm_din2_x11_pins[]			= { GPIOX_11 };
static const unsigned int pdm_din0_x12_pins[]			= { GPIOX_12 };
static const unsigned int pdm_din1_x13_pins[]			= { GPIOX_13 };
static const unsigned int pdm_din2_x14_pins[]			= { GPIOX_14 };
static const unsigned int pdm_dclk_x15_pins[]			= { GPIOX_15 };

/* GPIOX func3 */
static const unsigned int tsin_b_din0_x0_pins[]			= { GPIOX_0 };
static const unsigned int tsin_b_sop_x1_pins[]			= { GPIOX_1 };
static const unsigned int tsin_b_valid_x2_pins[]		= { GPIOX_2 };
static const unsigned int tsin_b_clk_x3_pins[]			= { GPIOX_3 };
static const unsigned int tsin_b_sop_x8_pins[]			= { GPIOX_8 };
static const unsigned int tsin_b_valid_x9_pins[]		= { GPIOX_9 };
static const unsigned int tsin_b_din0_x10_pins[]		= { GPIOX_10 };
static const unsigned int tsin_b_clk_x11_pins[]			= { GPIOX_11 };
static const unsigned int iso7816_clk_x12_pins[]		= { GPIOX_12 };
static const unsigned int iso7816_data_x13_pins[]		= { GPIOX_13 };
static const unsigned int i2c3_sda_x14_pins[]			= { GPIOX_14 };
static const unsigned int i2c3_scl_x15_pins[]			= { GPIOX_15 };

/* GPIOX func4 */
static const unsigned int i2c3_sda_x0_pins[]			= { GPIOX_0 };
static const unsigned int i2c3_scl_x1_pins[]			= { GPIOX_1 };
static const unsigned int i2c2_sda_x_pins[]			= { GPIOX_2 };
static const unsigned int i2c2_scl_x_pins[]			= { GPIOX_3 };
static const unsigned int spi_a_mosi_x_pins[]			= { GPIOX_8 };
static const unsigned int spi_a_miso_x_pins[]			= { GPIOX_9 };
static const unsigned int spi_a_ss0_x_pins[]			= { GPIOX_10 };
static const unsigned int spi_a_clk_x_pins[]			= { GPIOX_11 };

/* GPIOX func5 */
static const unsigned int tdm_d2_x0_pins[]			= { GPIOX_0 };
static const unsigned int tdm_d3_x1_pins[]			= { GPIOX_1 };
static const unsigned int tdm_fs1_x_pins[]			= { GPIOX_2 };
static const unsigned int tdm_sclk1_x_pins[]			= { GPIOX_3 };
static const unsigned int mclk_1_x_pins[]			= { GPIOX_4 };
static const unsigned int tdm_d4_x5_pins[]			= { GPIOX_5 };
static const unsigned int tdm_d5_x6_pins[]			= { GPIOX_6 };
static const unsigned int tdm_d2_x7_pins[]			= { GPIOX_7 };
static const unsigned int iso7816_clk_x8_pins[]			= { GPIOX_8 };
static const unsigned int iso7816_data_x9_pins[]		= { GPIOX_9 };
static const unsigned int i2c3_sda_x10_pins[]			= { GPIOX_10 };
static const unsigned int i2c3_scl_x11_pins[]			= { GPIOX_11 };
static const unsigned int tdm_d3_x17_pins[]			= { GPIOX_17 };

/* GPIOX func6 */
static const unsigned int iso7816_clk_x0_pins[]			= { GPIOX_0 };
static const unsigned int iso7816_data_x1_pins[]		= { GPIOX_1 };
static const unsigned int tdm_d4_x2_pins[]			= { GPIOX_2 };
static const unsigned int tdm_d5_x3_pins[]			= { GPIOX_3 };
static const unsigned int pwm_c_x8_pins[]			= { GPIOX_8 };
static const unsigned int gen_clk_x_pins[]			= { GPIOX_19 };

/* GPIOH func1 */
static const unsigned int hdmitx_sda_pins[]			= { GPIOH_0 };
static const unsigned int hdmitx_scl_pins[]			= { GPIOH_1 };
static const unsigned int hdmitx_hpd_in_pins[]			= { GPIOH_2 };
static const unsigned int ao_cec_h_pins[]			= { GPIOH_3 };
static const unsigned int spdif_out_h_pins[]			= { GPIOH_4 };
static const unsigned int spdif_in_h_pins[]			= { GPIOH_5 };
static const unsigned int iso7816_clk_h_pins[]			= { GPIOH_6 };
static const unsigned int iso7816_data_h_pins[]			= { GPIOH_7 };

/* GPIOH func2 */
static const unsigned int i2c2_sda_h_pins[]			= { GPIOH_0 };
static const unsigned int i2c2_scl_h_pins[]			= { GPIOH_1 };
static const unsigned int i2c3_sda_h_pins[]			= { GPIOH_2 };
static const unsigned int i2c3_scl_h_pins[]			= { GPIOH_3 };
static const unsigned int uart_c_rts_pins[]			= { GPIOH_4 };
static const unsigned int uart_c_cts_pins[]			= { GPIOH_5 };
static const unsigned int uart_c_rx_h_pins[]			= { GPIOH_6 };
static const unsigned int uart_c_tx_h_pins[]			= { GPIOH_7 };

/* GPIOH func3 */
static const unsigned int jtag_clk_h_pins[]			= { GPIOH_0 };
static const unsigned int jtag_tms_h_pins[]			= { GPIOH_1 };
static const unsigned int jtag_tdi_h_pins[]			= { GPIOH_2 };
static const unsigned int jtag_tdo_h_pins[]			= { GPIOH_3 };
static const unsigned int spi_a_mosi_h_pins[]			= { GPIOH_4 };
static const unsigned int spi_a_miso_h_pins[]			= { GPIOH_5 };
static const unsigned int spi_a_ss0_h_pins[]			= { GPIOH_6 };
static const unsigned int spi_a_clk_h_pins[]			= { GPIOH_7 };

/* GPIOH func4 */
static const unsigned int uart_b_tx_h_pins[]			= { GPIOH_0 };
static const unsigned int uart_b_rx_h_pins[]			= { GPIOH_1 };
static const unsigned int pwm_b_h5_pins[]			= { GPIOH_5 };
static const unsigned int i2c4_sda_h_pins[]			= { GPIOH_6 };
static const unsigned int i2c4_scl_h_pins[]			= { GPIOH_7 };

/* GPIOH func5 */
static const unsigned int ir_remote_out_h_pins[]		= { GPIOH_6 };
static const unsigned int pwm_b_h7_pins[]			= { GPIOH_7 };

/* GPIOZ func1 */
static const unsigned int eth_mdio_pins[]			= { GPIOZ_0 };
static const unsigned int eth_mdc_pins[]			= { GPIOZ_1 };
static const unsigned int eth_rgmii_rx_clk_pins[]		= { GPIOZ_2 };
static const unsigned int eth_rx_dv_pins[]			= { GPIOZ_3 };
static const unsigned int eth_rxd0_pins[]			= { GPIOZ_4 };
static const unsigned int eth_rxd1_pins[]			= { GPIOZ_5 };
static const unsigned int eth_rxd2_rgmii_pins[]			= { GPIOZ_6 };
static const unsigned int eth_rxd3_rgmii_pins[]			= { GPIOZ_7 };
static const unsigned int eth_rgmii_tx_clk_pins[]		= { GPIOZ_8 };
static const unsigned int eth_txen_pins[]			= { GPIOZ_9 };
static const unsigned int eth_txd0_pins[]			= { GPIOZ_10 };
static const unsigned int eth_txd1_pins[]			= { GPIOZ_11 };
static const unsigned int eth_txd2_rgmii_pins[]			= { GPIOZ_12 };
static const unsigned int eth_txd3_rgmii_pins[]			= { GPIOZ_13 };
static const unsigned int eth_link_led_z_pins[]			= { GPIOZ_14 };
static const unsigned int eth_act_led_z_pins[]			= { GPIOZ_15 };

/* GPIOZ func2 */
static const unsigned int tsin_a_din0_z_pins[]			= { GPIOZ_0 };
static const unsigned int tsin_a_sop_z_pins[]			= { GPIOZ_1 };
static const unsigned int tsin_a_valid_z_pins[]			= { GPIOZ_2 };
static const unsigned int tsin_a_clk_z_pins[]			= { GPIOZ_3 };
static const unsigned int tsin_b_sop_z4_pins[]			= { GPIOZ_4 };
static const unsigned int tsin_b_valid_z5_pins[]		= { GPIOZ_5 };
static const unsigned int tsin_b_din0_z6_pins[]			= { GPIOZ_6 };
static const unsigned int tsin_b_clk_z7_pins[]			= { GPIOZ_7 };
static const unsigned int tsin_c_valid_pins[]			= { GPIOZ_8 };
static const unsigned int tsin_c_sop_pins[]			= { GPIOZ_9 };
static const unsigned int tsin_c_din0_pins[]			= { GPIOZ_10 };
static const unsigned int tsin_c_clk_pins[]			= { GPIOZ_11 };
static const unsigned int tsin_d_valid_pins[]			= { GPIOZ_12 };
static const unsigned int tsin_d_sop_pins[]			= { GPIOZ_13 };
static const unsigned int tsin_d_din0_pins[]			= { GPIOZ_14 };
static const unsigned int tsin_d_clk_pins[]			= { GPIOZ_15 };

/* GPIOZ func3 */
static const unsigned int iso7816_clk_z_pins[]			= { GPIOZ_0 };
static const unsigned int iso7816_data_z_pins[]			= { GPIOZ_1 };
static const unsigned int tsin_b_valid_z2_pins[]		= { GPIOZ_2 };
static const unsigned int tsin_b_sop_z3_pins[]			= { GPIOZ_3 };
static const unsigned int tsin_b_din0_z4_pins[]			= { GPIOZ_4 };
static const unsigned int tsin_b_clk_z5_pins[]			= { GPIOZ_5 };
static const unsigned int tsin_b_fail_pins[]			= { GPIOZ_6 };
static const unsigned int tsin_b_din1_pins[]			= { GPIOZ_7 };
static const unsigned int tsin_b_din2_pins[]			= { GPIOZ_8 };
static const unsigned int tsin_b_din3_pins[]			= { GPIOZ_9 };
static const unsigned int tsin_b_din4_pins[]			= { GPIOZ_10 };
static const unsigned int tsin_b_din5_pins[]			= { GPIOZ_11 };
static const unsigned int tsin_b_din6_pins[]			= { GPIOZ_12 };
static const unsigned int tsin_b_din7_pins[]			= { GPIOZ_13 };
static const unsigned int i2c4_sda_z_pins[]			= { GPIOZ_14 };
static const unsigned int i2c4_scl_z_pins[]			= { GPIOZ_15 };

/* GPIOZ func4 */
static const unsigned int i2c0_sda_z_pins[]			= { GPIOZ_0 };
static const unsigned int i2c0_scl_z_pins[]			= { GPIOZ_1 };
static const unsigned int tdm_d4_z_pins[]			= { GPIOZ_2 };
static const unsigned int tdm_d5_z_pins[]			= { GPIOZ_3 };
static const unsigned int tdm_d6_z_pins[]			= { GPIOZ_4 };
static const unsigned int tdm_d7_z_pins[]			= { GPIOZ_5 };
static const unsigned int tdm_fs2_z_pins[]			= { GPIOZ_6 };
static const unsigned int tdm_sclk2_z_pins[]			= { GPIOZ_7 };
static const unsigned int mclk_2_z_pins[]			= { GPIOZ_8 };
static const unsigned int mclk_1_z_pins[]			= { GPIOZ_9 };
static const unsigned int tdm_sclk1_z_pins[]			= { GPIOZ_10 };
static const unsigned int tdm_fs1_z_pins[]			= { GPIOZ_11 };
static const unsigned int tdm_d8_z_pins[]			= { GPIOZ_12 };
static const unsigned int tdm_d9_z_pins[]			= { GPIOZ_13 };
static const unsigned int tdm_d10_z_pins[]			= { GPIOZ_14 };
static const unsigned int tdm_d11_z_pins[]			= { GPIOZ_15 };

/* GPIOZ func5 */
static const unsigned int pwm_b_z0_pins[]			= { GPIOZ_0 };
static const unsigned int pwm_c_z_pins[]			= { GPIOZ_1 };
static const unsigned int pdm_din0_z_pins[]			= { GPIOZ_2 };
static const unsigned int pdm_din1_z_pins[]			= { GPIOZ_3 };
static const unsigned int pdm_din2_z4_pins[]			= { GPIOZ_4 };
static const unsigned int pdm_din3_z5_pins[]			= { GPIOZ_5 };
static const unsigned int pdm_dclk_z_pins[]			= { GPIOZ_6 };
static const unsigned int i2c5_sda_z_pins[]			= { GPIOZ_7 };
static const unsigned int i2c5_scl_z_pins[]			= { GPIOZ_8 };
static const unsigned int i2c2_sda_z10_pins[]			= { GPIOZ_10 };
static const unsigned int i2c2_scl_z11_pins[]			= { GPIOZ_11 };
static const unsigned int pwm_f_z_pins[]			= { GPIOZ_12 };
static const unsigned int pwm_b_z13_pins[]			= { GPIOZ_13 };
static const unsigned int pdm_din2_z14_pins[]			= { GPIOZ_14 };
static const unsigned int pdm_din3_z15_pins[]			= { GPIOZ_15 };

/* GPIOZ func6 */
static const unsigned int i2c2_sda_z0_pins[]			= { GPIOZ_0 };
static const unsigned int i2c2_scl_z1_pins[]			= { GPIOZ_1 };
static const unsigned int i2c1_sda_z3_pins[]			= { GPIOZ_3 };
static const unsigned int pdm_din2_z7_pins[]			= { GPIOZ_7 };
static const unsigned int pdm_din3_z8_pins[]			= { GPIOZ_8 };
static const unsigned int ir_remote_out_z_pins[]		= { GPIOZ_10 };
static const unsigned int i2c1_scl_z11_pins[]			= { GPIOZ_11 };
static const unsigned int clk12m_24m_z_pins[]			= { GPIOZ_13 };

/* GPIOZ func7 */
static const unsigned int gen_clk_z_pins[]			= { GPIOZ_13 };

/* GPIOA func1 */
static const unsigned int tdm_d2_a_pins[]			= { GPIOA_0 };
static const unsigned int tdm_d3_a_pins[]			= { GPIOA_1 };
static const unsigned int tdm_d4_a_pins[]			= { GPIOA_2 };
static const unsigned int tdm_d5_a_pins[]			= { GPIOA_3 };
static const unsigned int tdm_d6_a_pins[]			= { GPIOA_4 };
static const unsigned int tdm_d7_a_pins[]			= { GPIOA_5 };
static const unsigned int tdm_fs2_a_pins[]			= { GPIOA_6 };
static const unsigned int tdm_sclk2_a_pins[]			= { GPIOA_7 };
static const unsigned int mclk_2_a_pins[]			= { GPIOA_8 };
static const unsigned int mclk_1_a_pins[]			= { GPIOA_9 };
static const unsigned int tdm_sclk1_a_pins[]			= { GPIOA_10 };
static const unsigned int tdm_fs1_a_pins[]			= { GPIOA_11 };
static const unsigned int tdm_d8_a_pins[]			= { GPIOA_12 };
static const unsigned int tdm_d9_a_pins[]			= { GPIOA_13 };
static const unsigned int tdm_d10_a14_pins[]			= { GPIOA_14 };
static const unsigned int tdm_d11_a15_pins[]			= { GPIOA_15 };

/* GPIOA func2 */
static const unsigned int pdm_din0_a0_pins[]			= { GPIOA_0 };
static const unsigned int pdm_din1_a1_pins[]			= { GPIOA_1 };
static const unsigned int pdm_dclk_a2_pins[]			= { GPIOA_2 };
static const unsigned int spi_a_mosi_a_pins[]			= { GPIOA_3 };
static const unsigned int spi_a_miso_a_pins[]			= { GPIOA_4 };
static const unsigned int spi_a_ss0_a_pins[]			= { GPIOA_5 };
static const unsigned int spi_a_clk_a_pins[]			= { GPIOA_6 };
static const unsigned int tdm_d10_a7_pins[]			= { GPIOA_7 };
static const unsigned int tdm_d11_a8_pins[]			= { GPIOA_8 };
static const unsigned int pdm_dclk_a9_pins[]			= { GPIOA_9 };
static const unsigned int pdm_din3_a_pins[]			= { GPIOA_10 };
static const unsigned int pdm_din2_a_pins[]			= { GPIOA_11 };
static const unsigned int spdif_in_a12_pins[]			= { GPIOA_12 };
static const unsigned int spdif_out_a_pins[]			= { GPIOA_13 };
static const unsigned int i2c3_sda_a14_pins[]			= { GPIOA_14 };
static const unsigned int i2c3_scl_a15_pins[]			= { GPIOA_15 };

/* GPIOA func3 */
static const unsigned int i2c0_sda_a_pins[]			= { GPIOA_0 };
static const unsigned int i2c0_scl_a_pins[]			= { GPIOA_1 };
static const unsigned int i2c1_scl_a_pins[]			= { GPIOA_2 };
static const unsigned int i2c1_sda_a_pins[]			= { GPIOA_3 };
static const unsigned int i2c2_scl_a_pins[]			= { GPIOA_4 };
static const unsigned int i2c2_sda_a_pins[]			= { GPIOA_5 };
static const unsigned int i2c3_scl_a6_pins[]			= { GPIOA_6 };
static const unsigned int i2c3_sda_a7_pins[]			= { GPIOA_7 };
static const unsigned int i2c4_scl_a_pins[]			= { GPIOA_8 };
static const unsigned int i2c4_sda_a_pins[]			= { GPIOA_9 };
static const unsigned int i2c5_sda_a10_pins[]			= { GPIOA_10 };
static const unsigned int i2c5_scl_a11_pins[]			= { GPIOA_11 };
static const unsigned int pdm_din0_a12_pins[]			= { GPIOA_12 };
static const unsigned int pdm_din1_a13_pins[]			= { GPIOA_13 };
static const unsigned int spdif_in_a14_pins[]			= { GPIOA_14 };
static const unsigned int ir_remote_out_a_pins[]		= { GPIOA_15 };

/* GPIOA func4 */
static const unsigned int iso7816_clk_a_pins[]			= { GPIOA_0 };
static const unsigned int iso7816_data_a_pins[]			= { GPIOA_1 };
static const unsigned int i2c5_sda_a14_pins[]			= { GPIOA_14 };
static const unsigned int i2c5_scl_a15_pins[]			= { GPIOA_15 };

/* GPIOA func5 */
static const unsigned int eth_link_led_a_pins[]			= { GPIOA_14 };
static const unsigned int eth_act_led_a_pins[]			= { GPIOA_15 };

/* GPIOA func6 */
static const unsigned int gen_clk_a_pins[]			= { GPIOA_11 };
static const unsigned int mic_mute_en_a_pins[]			= { GPIOA_14 };
static const unsigned int mic_mute_key_a_pins[]			= { GPIOA_15 };

static struct meson_pmx_group meson_s6_periphs_groups[] = {
	GPIO_GROUP(GPIOD_0,		0),
	GPIO_GROUP(GPIOD_1,		0),
	GPIO_GROUP(GPIOD_2,		0),
	GPIO_GROUP(GPIOD_3,		0),
	GPIO_GROUP(GPIOD_4,		0),
	GPIO_GROUP(GPIOD_5,		0),
	GPIO_GROUP(GPIOD_6,		0),
	GPIO_GROUP(GPIOF_0,		0),
	GPIO_GROUP(GPIOF_1,		0),
	GPIO_GROUP(GPIOF_2,		0),
	GPIO_GROUP(GPIOF_3,		0),
	GPIO_GROUP(GPIOF_4,		0),
	GPIO_GROUP(GPIOE_0,		0),
	GPIO_GROUP(GPIOE_1,		0),
	GPIO_GROUP(GPIOE_2,		0),
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
	GPIO_GROUP(GPIOH_0,		0),
	GPIO_GROUP(GPIOH_1,		0),
	GPIO_GROUP(GPIOH_2,		0),
	GPIO_GROUP(GPIOH_3,		0),
	GPIO_GROUP(GPIOH_4,		0),
	GPIO_GROUP(GPIOH_5,		0),
	GPIO_GROUP(GPIOH_6,		0),
	GPIO_GROUP(GPIOH_7,		0),
	GPIO_GROUP(GPIOH_8,		0),
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
	GPIO_GROUP(GPIOZ_13,		0),
	GPIO_GROUP(GPIOZ_14,		0),
	GPIO_GROUP(GPIOZ_15,		0),
	GPIO_GROUP(GPIOA_0,		0),
	GPIO_GROUP(GPIOA_1,		0),
	GPIO_GROUP(GPIOA_2,		0),
	GPIO_GROUP(GPIOA_3,		0),
	GPIO_GROUP(GPIOA_4,		0),
	GPIO_GROUP(GPIOA_5,		0),
	GPIO_GROUP(GPIOA_6,		0),
	GPIO_GROUP(GPIOA_7,		0),
	GPIO_GROUP(GPIOA_8,		0),
	GPIO_GROUP(GPIOA_9,		0),
	GPIO_GROUP(GPIOA_10,		0),
	GPIO_GROUP(GPIOA_11,		0),
	GPIO_GROUP(GPIOA_12,		0),
	GPIO_GROUP(GPIOA_13,		0),
	GPIO_GROUP(GPIOA_14,		0),
	GPIO_GROUP(GPIOA_15,		0),
	GPIO_GROUP(GPIO_TEST_N,		0),
	GPIO_GROUP(GPIO_CC1,		0),
	GPIO_GROUP(GPIO_CC2,		0),

	/* GPIOD func1 */
	GROUP(uart_b_tx_d,		1),
	GROUP(uart_b_rx_d,		1),
	GROUP(jtag_clk_d,		1),
	GROUP(jtag_tms_d,		1),
	GROUP(jtag_tdi_d,		1),
	GROUP(jtag_tdo_d,		1),
	GROUP(ao_cec_d,			1),

	/* GPIOD func2 */
	GROUP(i2c0_scl_d,		2),
	GROUP(i2c0_sda_d,		2),
	GROUP(i2c1_scl_d,		2),
	GROUP(i2c1_sda_d,		2),
	GROUP(i2c2_scl_d,		2),
	GROUP(i2c2_sda_d,		2),
	GROUP(ir_remote_in_d,		2),

	/* GPIOD func3 */
	GROUP(pwm_c_d2,			3),
	GROUP(pwm_b_d,			3),
	GROUP(uart_c_tx_d,		3),
	GROUP(uart_c_rx_d,		3),
	GROUP(pwm_c_d6,			3),

	/* GPIOD func4 */
	GROUP(tsin_a_sop_d,		4),
	GROUP(tsin_a_din0_d,		4),
	GROUP(tsin_a_clk_d,		4),
	GROUP(tsin_a_valid_d,		4),
	GROUP(spdif_out_d,		4),

	/* GPIOD func5 */
	GROUP(tdm_d2_d,			5),
	GROUP(tdm_fs1_d,		5),
	GROUP(tdm_sclk1_d,		5),
	GROUP(mclk_1_d,			5),
	GROUP(tdm_d3_d,			5),

	/* GPIOD func6 */
	GROUP(pdm_din0_d,		6),
	GROUP(pdm_din1_d,		6),
	GROUP(pdm_din2_d,		6),
	GROUP(pdm_dclk_d,		6),
	GROUP(clk12m_24m_d,		6),

	/* GPIOF func1 */
	GROUP(i2c0_scl_f,		1),
	GROUP(i2c0_sda_f,		1),
	GROUP(ir_remote_out_f,		1),
	GROUP(ir_remote_in_f3,		1),
	GROUP(pwm_c_f,			1),

	/* GPIOF func2 */
	GROUP(uart_c_tx_f,		2),
	GROUP(uart_c_rx_f,		2),
	GROUP(clk_32k_in,		2),
	GROUP(pwm_c_hiz,		2),

	/* GPIOF func3 */
	GROUP(i2c_slave_scl,		3),
	GROUP(i2c_slave_sda,		3),
	GROUP(pwm_a_f,			3),
	GROUP(pwm_b_f,			3),
	GROUP(gen_clk_f,		3),

	/* GPIOF func4 */
	GROUP(spdif_in_f,		4),
	GROUP(ir_remote_in_f1,		4),
	GROUP(pwm_a_hiz,		4),
	GROUP(pwm_b_hiz,		4),

	/* GPIOF func5 */
	GROUP(tdm_d4_f,			5),
	GROUP(tdm_d5_f,			5),
	GROUP(i2c3_scl_f,		5),
	GROUP(i2c3_sda_f,		5),

	/* GPIOF func6 */
	GROUP(spi_a_mosi_f,		6),
	GROUP(spi_a_miso_f,		6),
	GROUP(spi_a_ss0_f,		6),
	GROUP(spi_a_clk_f,		6),

	/* GPIOF func7 */
	GROUP(mic_mute_en,		7),
	GROUP(mic_mute_key,		7),

	/* GPIOE func1 */
	GROUP(i2c0_scl_e,		1),
	GROUP(i2c0_sda_e,		1),
	GROUP(clk12m_24m_e,		1),

	/* GPIOE func2 */
	GROUP(uart_b_tx_e,		2),
	GROUP(uart_b_rx_e,		2),
	GROUP(pwm_a_e,			2),

	/* GPIOE func3 */
	GROUP(pwm_d,			3),
	GROUP(pwm_f_e,			3),
	GROUP(gen_clk_e,		3),

	/* GPIOB func1 */
	GROUP(emmc_nand_d0,		1),
	GROUP(emmc_nand_d1,		1),
	GROUP(emmc_nand_d2,		1),
	GROUP(emmc_nand_d3,		1),
	GROUP(emmc_nand_d4,		1),
	GROUP(emmc_nand_d5,		1),
	GROUP(emmc_nand_d6,		1),
	GROUP(emmc_nand_d7,		1),
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
	GROUP(spinf_clk,		3),
	GROUP(spinf_wp_d2,		3),
	GROUP(spinf_d4,			3),
	GROUP(spinf_d5,			3),
	GROUP(spinf_d6,			3),
	GROUP(spinf_d7,			3),
	GROUP(spinf_cs1,		3),
	GROUP(spinf_cs0,		3),

	/* GPIOB func4 */
	GROUP(i2c4_sda_b,		4),
	GROUP(i2c4_scl_b,		4),

	/* GPIOB func6 */
	GROUP(pdm_din0_b,		6),
	GROUP(pdm_dclk_b,		6),

	/* GPIOC func1 */
	GROUP(sdcard_d0,		1),
	GROUP(sdcard_d1,		1),
	GROUP(sdcard_d2,		1),
	GROUP(sdcard_d3,		1),
	GROUP(sdcard_clk,		1),
	GROUP(sdcard_cmd,		1),
	GROUP(card_det,			1),
	GROUP(pcieck_reqn,		1),

	/* GPIOC func2 */
	GROUP(jtag2_tdo,		2),
	GROUP(jtag2_tdi,		2),
	GROUP(uart_b_rx_c,		2),
	GROUP(uart_b_tx_c,		2),
	GROUP(jtag2_clk,		2),
	GROUP(jtag2_tms,		2),
	GROUP(ir_remote_in_c,		2),

	/* GPIOC func3 */
	GROUP(tsin_a_clk_c,		3),
	GROUP(tsin_a_sop_c,		3),
	GROUP(tsin_a_valid_c,		3),
	GROUP(tsin_a_din0_c,		3),
	GROUP(gen_clk_c,		3),
	GROUP(i2c1_sda_c,		3),
	GROUP(i2c1_scl_c,		3),

	/* GPIOC func4 */
	GROUP(pdm_din0_c,		4),
	GROUP(pdm_din1_c,		4),
	GROUP(pdm_din2_c,		4),
	GROUP(pdm_din3_c,		4),
	GROUP(pdm_dclk_c,		4),
	GROUP(iso7816_clk_c5,		4),
	GROUP(iso7816_data_c6,		4),

	/* GPIOC func5 */
	GROUP(tdm_d6_c,			5),
	GROUP(tdm_d7_c,			5),
	GROUP(tdm_fs1_c,		5),
	GROUP(tdm_sclk1_c,		5),
	GROUP(mclk_1_c,			5),
	GROUP(tdm_d8_c,			5),
	GROUP(tdm_d9_c,			5),

	/* GPIOC func6 */
	GROUP(tsin_b_clk_c,		6),
	GROUP(tsin_b_sop_c,		6),
	GROUP(tsin_b_valid_c,		6),
	GROUP(tsin_b_din0_c,		6),
	GROUP(tdm_d3_c,			6),

	/* GPIOC func7 */
	GROUP(iso7816_clk_c0,		7),
	GROUP(iso7816_data_c1,		7),

	/* GPIOX func1 */
	GROUP(sdio_d0,			1),
	GROUP(sdio_d1,			1),
	GROUP(sdio_d2,			1),
	GROUP(sdio_d3,			1),
	GROUP(sdio_clk,			1),
	GROUP(sdio_cmd,			1),
	GROUP(uart_a_tx_x6,		1),
	GROUP(uart_a_rx_x7,		1),
	GROUP(tdm_d1_x,			1),
	GROUP(tdm_d0_x,			1),
	GROUP(tdm_fs0_x,		1),
	GROUP(tdm_sclk0_x,		1),
	GROUP(uart_a_tx_x12,		1),
	GROUP(uart_a_rx_x13,		1),
	GROUP(uart_a_cts,		1),
	GROUP(uart_a_rts,		1),
	GROUP(pwm_e,			1),
	GROUP(i2c1_sda_x,		1),
	GROUP(i2c1_scl_x,		1),
	GROUP(pwm_b_x,			1),

	/* GPIOX func2 */
	GROUP(pdm_din0_x0,		2),
	GROUP(pdm_din1_x1,		2),
	GROUP(pdm_din2_x2,		2),
	GROUP(pdm_din3_x3,		2),
	GROUP(pdm_dclk_x4,		2),
	GROUP(pwm_c_x5,			2),
	GROUP(pwm_a_x6,			2),
	GROUP(pwm_f_x,			2),
	GROUP(pdm_dclk_x8,		2),
	GROUP(pdm_din0_x9,		2),
	GROUP(pdm_din1_x10,		2),
	GROUP(pdm_din2_x11,		2),
	GROUP(pdm_din0_x12,		2),
	GROUP(pdm_din1_x13,		2),
	GROUP(pdm_din2_x14,		2),
	GROUP(pdm_dclk_x15,		2),

	/* GPIOX func3 */
	GROUP(tsin_b_din0_x0,		3),
	GROUP(tsin_b_sop_x1,		3),
	GROUP(tsin_b_valid_x2,		3),
	GROUP(tsin_b_clk_x3,		3),
	GROUP(tsin_b_sop_x8,		3),
	GROUP(tsin_b_valid_x9,		3),
	GROUP(tsin_b_din0_x10,		3),
	GROUP(tsin_b_clk_x11,		3),
	GROUP(iso7816_clk_x12,		3),
	GROUP(iso7816_data_x13,		3),
	GROUP(i2c3_sda_x14,		3),
	GROUP(i2c3_scl_x15,		3),

	/* GPIOX func4 */
	GROUP(i2c3_sda_x0,		4),
	GROUP(i2c3_scl_x1,		4),
	GROUP(i2c2_sda_x,		4),
	GROUP(i2c2_scl_x,		4),
	GROUP(spi_a_mosi_x,		4),
	GROUP(spi_a_miso_x,		4),
	GROUP(spi_a_ss0_x,		4),
	GROUP(spi_a_clk_x,		4),

	/* GPIOX func5 */
	GROUP(tdm_d2_x0,		5),
	GROUP(tdm_d3_x1,		5),
	GROUP(tdm_fs1_x,		5),
	GROUP(tdm_sclk1_x,		5),
	GROUP(mclk_1_x,			5),
	GROUP(tdm_d4_x5,		5),
	GROUP(tdm_d5_x6,		5),
	GROUP(tdm_d2_x7,		5),
	GROUP(iso7816_clk_x8,		5),
	GROUP(iso7816_data_x9,		5),
	GROUP(i2c3_sda_x10,		5),
	GROUP(i2c3_scl_x11,		5),
	GROUP(tdm_d3_x17,		5),

	/* GPIOX func6 */
	GROUP(iso7816_clk_x0,		6),
	GROUP(iso7816_data_x1,		6),
	GROUP(tdm_d4_x2,		6),
	GROUP(tdm_d5_x3,		6),
	GROUP(pwm_c_x8,			6),
	GROUP(gen_clk_x,		6),

	/* GPIOH func1 */
	GROUP(hdmitx_sda,		1),
	GROUP(hdmitx_scl,		1),
	GROUP(hdmitx_hpd_in,		1),
	GROUP(ao_cec_h,			1),
	GROUP(spdif_in_h,		1),
	GROUP(spdif_out_h,		1),
	GROUP(iso7816_clk_h,		1),
	GROUP(iso7816_data_h,		1),

	/* GPIOH func2 */
	GROUP(i2c2_sda_h,		2),
	GROUP(i2c2_scl_h,		2),
	GROUP(i2c3_sda_h,		2),
	GROUP(i2c3_scl_h,		2),
	GROUP(uart_c_rts,		2),
	GROUP(uart_c_cts,		2),
	GROUP(uart_c_rx_h,		2),
	GROUP(uart_c_tx_h,		2),

	/* GPIOH func3 */
	GROUP(jtag_clk_h,		3),
	GROUP(jtag_tms_h,		3),
	GROUP(jtag_tdi_h,		3),
	GROUP(jtag_tdo_h,		3),
	GROUP(spi_a_mosi_h,		3),
	GROUP(spi_a_miso_h,		3),
	GROUP(spi_a_ss0_h,		3),
	GROUP(spi_a_clk_h,		3),

	/* GPIOH func4 */
	GROUP(uart_b_tx_h,		4),
	GROUP(uart_b_rx_h,		4),
	GROUP(pwm_b_h5,			4),
	GROUP(i2c4_sda_h,		4),
	GROUP(i2c4_scl_h,		4),

	/* GPIOH func5 */
	GROUP(ir_remote_out_h,		5),
	GROUP(pwm_b_h7,			5),

	/* GPIOZ func1 */
	GROUP(eth_mdio,			1),
	GROUP(eth_mdc,			1),
	GROUP(eth_rgmii_rx_clk,		1),
	GROUP(eth_rx_dv,		1),
	GROUP(eth_rxd0,			1),
	GROUP(eth_rxd1,			1),
	GROUP(eth_rxd2_rgmii,		1),
	GROUP(eth_rxd3_rgmii,		1),
	GROUP(eth_rgmii_tx_clk,		1),
	GROUP(eth_txen,			1),
	GROUP(eth_txd0,			1),
	GROUP(eth_txd1,			1),
	GROUP(eth_txd2_rgmii,		1),
	GROUP(eth_txd3_rgmii,		1),
	GROUP(eth_link_led_z,		1),
	GROUP(eth_act_led_z,		1),

	/* GPIOZ func2 */
	GROUP(tsin_a_din0_z,		2),
	GROUP(tsin_a_sop_z,		2),
	GROUP(tsin_a_valid_z,		2),
	GROUP(tsin_a_clk_z,		2),
	GROUP(tsin_b_sop_z4,		2),
	GROUP(tsin_b_valid_z5,		2),
	GROUP(tsin_b_din0_z6,		2),
	GROUP(tsin_b_clk_z7,		2),
	GROUP(tsin_c_valid,		2),
	GROUP(tsin_c_sop,		2),
	GROUP(tsin_c_din0,		2),
	GROUP(tsin_c_clk,		2),
	GROUP(tsin_d_valid,		2),
	GROUP(tsin_d_sop,		2),
	GROUP(tsin_d_din0,		2),
	GROUP(tsin_d_clk,		2),

	/* GPIOZ func3 */
	GROUP(iso7816_clk_z,		3),
	GROUP(iso7816_data_z,		3),
	GROUP(tsin_b_valid_z2,		3),
	GROUP(tsin_b_sop_z3,		3),
	GROUP(tsin_b_din0_z4,		3),
	GROUP(tsin_b_clk_z5,		3),
	GROUP(tsin_b_fail,		3),
	GROUP(tsin_b_din1,		3),
	GROUP(tsin_b_din2,		3),
	GROUP(tsin_b_din3,		3),
	GROUP(tsin_b_din4,		3),
	GROUP(tsin_b_din5,		3),
	GROUP(tsin_b_din6,		3),
	GROUP(tsin_b_din7,		3),
	GROUP(i2c4_sda_z,		3),
	GROUP(i2c4_scl_z,		3),

	/* GPIOZ func4 */
	GROUP(i2c0_sda_z,		4),
	GROUP(i2c0_scl_z,		4),
	GROUP(tdm_d4_z,			4),
	GROUP(tdm_d5_z,			4),
	GROUP(tdm_d6_z,			4),
	GROUP(tdm_d7_z,			4),
	GROUP(tdm_fs2_z,		4),
	GROUP(tdm_sclk2_z,		4),
	GROUP(mclk_2_z,			4),
	GROUP(mclk_1_z,			4),
	GROUP(tdm_sclk1_z,		4),
	GROUP(tdm_fs1_z,		4),
	GROUP(tdm_d8_z,			4),
	GROUP(tdm_d9_z,			4),
	GROUP(tdm_d10_z,		4),
	GROUP(tdm_d11_z,		4),

	/* GPIOZ func5 */
	GROUP(pwm_b_z0,			5),
	GROUP(pwm_c_z,			5),
	GROUP(pdm_din0_z,		5),
	GROUP(pdm_din1_z,		5),
	GROUP(pdm_din2_z4,		5),
	GROUP(pdm_din3_z5,		5),
	GROUP(pdm_dclk_z,		5),
	GROUP(i2c5_sda_z,		5),
	GROUP(i2c5_scl_z,		5),
	GROUP(i2c2_sda_z10,		5),
	GROUP(i2c2_scl_z11,		5),
	GROUP(pwm_f_z,			5),
	GROUP(pwm_b_z13,		5),
	GROUP(pdm_din2_z14,		5),
	GROUP(pdm_din3_z15,		5),

	/* GPIOZ func6 */
	GROUP(i2c2_sda_z0,		6),
	GROUP(i2c2_scl_z1,		6),
	GROUP(i2c1_sda_z3,		6),
	GROUP(pdm_din2_z7,		6),
	GROUP(pdm_din3_z8,		6),
	GROUP(ir_remote_out_z,		6),
	GROUP(i2c1_scl_z11,		6),
	GROUP(clk12m_24m_z,		6),

	/* GPIOZ func7 */
	GROUP(gen_clk_z,		7),

	/* GPIOA func1 */
	GROUP(tdm_d2_a,			1),
	GROUP(tdm_d3_a,			1),
	GROUP(tdm_d4_a,			1),
	GROUP(tdm_d5_a,			1),
	GROUP(tdm_d6_a,			1),
	GROUP(tdm_d7_a,			1),
	GROUP(tdm_fs2_a,		1),
	GROUP(tdm_sclk2_a,		1),
	GROUP(mclk_2_a,			1),
	GROUP(mclk_1_a,			1),
	GROUP(tdm_sclk1_a,		1),
	GROUP(tdm_fs1_a,		1),
	GROUP(tdm_d8_a,			1),
	GROUP(tdm_d9_a,			1),
	GROUP(tdm_d10_a14,		1),
	GROUP(tdm_d11_a15,		1),

	/* GPIOA func2 */
	GROUP(pdm_din0_a0,		2),
	GROUP(pdm_din1_a1,		2),
	GROUP(pdm_dclk_a2,		2),
	GROUP(spi_a_mosi_a,		2),
	GROUP(spi_a_miso_a,		2),
	GROUP(spi_a_ss0_a,		2),
	GROUP(spi_a_clk_a,		2),
	GROUP(tdm_d10_a7,		2),
	GROUP(tdm_d11_a8,		2),
	GROUP(pdm_dclk_a9,		2),
	GROUP(pdm_din2_a,		2),
	GROUP(pdm_din3_a,		2),
	GROUP(spdif_in_a12,		2),
	GROUP(spdif_out_a,		2),
	GROUP(i2c3_sda_a14,		2),
	GROUP(i2c3_scl_a15,		2),

	/* GPIOA func3 */
	GROUP(i2c0_sda_a,		3),
	GROUP(i2c0_scl_a,		3),
	GROUP(i2c1_scl_a,		3),
	GROUP(i2c1_sda_a,		3),
	GROUP(i2c2_scl_a,		3),
	GROUP(i2c2_sda_a,		3),
	GROUP(i2c3_scl_a6,		3),
	GROUP(i2c3_sda_a7,		3),
	GROUP(i2c4_scl_a,		3),
	GROUP(i2c4_sda_a,		3),
	GROUP(i2c5_sda_a10,		3),
	GROUP(i2c5_scl_a11,		3),
	GROUP(pdm_din0_a12,		3),
	GROUP(pdm_din1_a13,		3),
	GROUP(spdif_in_a14,		3),
	GROUP(ir_remote_out_a,		3),

	/* GPIOA func4 */
	GROUP(iso7816_clk_a,		4),
	GROUP(iso7816_data_a,		4),
	GROUP(i2c5_sda_a14,		4),
	GROUP(i2c5_scl_a15,		4),

	/* GPIOA func5 */
	GROUP(eth_link_led_a,		5),
	GROUP(eth_act_led_a,		5),

	/* GPIOA func6 */
	GROUP(gen_clk_a,		6),
	GROUP(mic_mute_en_a,		6),
	GROUP(mic_mute_key_a,		6),
};

static const char * const gpio_periphs_groups[] = {
	"GPIOD_0", "GPIOD_1", "GPIOD_2", "GPIOD_3", "GPIOD_4", "GPIOD_5",
	"GPIOD_6",
	"GPIOF_0", "GPIOF_1", "GPIOF_2", "GPIOF_3", "GPIOF_4",
	"GPIOE_0", "GPIOE_1", "GPIOE_2", "GPIOB_0", "GPIOB_1", "GPIOB_2",
	"GPIOB_3", "GPIOB_4", "GPIOB_5", "GPIOB_6", "GPIOB_7", "GPIOB_8",
	"GPIOB_9", "GPIOB_10", "GPIOB_11", "GPIOB_12", "GPIOB_13", "GPIOC_0",
	"GPIOC_1", "GPIOC_2", "GPIOC_3", "GPIOC_4", "GPIOC_5", "GPIOC_6",
	"GPIOC_7", "GPIOX_0", "GPIOX_1", "GPIOX_2", "GPIOX_3", "GPIOX_4",
	"GPIOX_5", "GPIOX_6", "GPIOX_7", "GPIOX_8", "GPIOX_9", "GPIOX_10",
	"GPIOX_11", "GPIOX_12", "GPIOX_13", "GPIOX_14", "GPIOX_15", "GPIOX_16",
	"GPIOX_17", "GPIOX_18", "GPIOX_19", "GPIOH_0", "GPIOH_1", "GPIOH_2",
	"GPIOH_3", "GPIOH_4", "GPIOH_5", "GPIOH_6", "GPIOH_7", "GPIOH_8",
	"GPIOZ_0", "GPIOZ_1", "GPIOZ_2", "GPIOZ_3", "GPIOZ_4", "GPIOZ_5",
	"GPIOZ_6", "GPIOZ_7", "GPIOZ_8", "GPIOZ_9", "GPIOZ_10", "GPIOZ_11",
	"GPIOZ_12", "GPIOZ_13", "GPIOZ_14", "GPIOZ_15", "GPIOA_0", "GPIOA_1",
	"GPIOA_2", "GPIOA_3", "GPIOA_4", "GPIOA_5", "GPIOA_6", "GPIOA_7",
	"GPIOA_8", "GPIOA_9", "GPIOA_10", "GPIOA_11", "GPIOA_12", "GPIOA_13",
	"GPIOA_14", "GPIOA_15", "GPIO_TEST_N", "GPIO_CC1", "GPIO_CC2"
};

static const char * const uart_a_groups[] = {
	"uart_a_cts", "uart_a_rts", "uart_a_tx_x6", "uart_a_rx_x7",
	"uart_a_tx_x12", "uart_a_rx_x13"
};

static const char * const uart_b_groups[] = {
	"uart_b_tx_d", "uart_b_rx_d", "uart_b_tx_e", "uart_b_rx_e",
	"uart_b_tx_c", "uart_b_rx_c", "uart_b_tx_h", "uart_b_rx_h"
};

static const char * const uart_c_groups[] = {
	"uart_c_rts", "uart_c_cts", "uart_c_rx_h", "uart_c_tx_h",
	"uart_c_tx_d", "uart_c_rx_d", "uart_c_tx_f", "uart_c_rx_f"
};

static const char * const i2c0_groups[] = {
	"i2c0_sda_e", "i2c0_scl_e", "i2c0_sda_z", "i2c0_scl_z", "i2c0_sda_a",
	"i2c0_scl_a", "i2c0_scl_f", "i2c0_sda_f", "i2c0_scl_d",
	"i2c0_sda_d"
};

static const char * const i2c1_groups[] = {
	"i2c1_scl_d", "i2c1_sda_d", "i2c1_scl_c", "i2c1_sda_c",
	"i2c1_scl_x", "i2c1_sda_x", "i2c1_scl_a", "i2c1_sda_a",
	"i2c1_sda_z3", "i2c1_scl_z11"
};

static const char * const i2c2_groups[] = {
	"i2c2_scl_d", "i2c2_sda_d", "i2c2_scl_x", "i2c2_sda_x",
	"i2c2_sda_h", "i2c2_scl_h", "i2c2_scl_a", "i2c2_sda_a",
	"i2c2_sda_z0", "i2c2_scl_z1", "i2c2_sda_z10", "i2c2_scl_z11"
};

static const char * const i2c3_groups[] = {
	"i2c3_scl_f", "i2c3_sda_f", "i2c3_sda_h", "i2c3_scl_h",
	"i2c3_sda_x0", "i2c3_scl_x1", "i2c3_scl_a6", "i2c3_sda_a7",
	"i2c3_sda_x14", "i2c3_scl_x15", "i2c3_sda_x10", "i2c3_scl_x11",
	"i2c3_sda_a14", "i2c3_scl_a15"
};

static const char * const i2c4_groups[] = {
	"i2c4_sda_b", "i2c4_scl_b", "i2c4_sda_h", "i2c4_scl_h",
	"i2c4_sda_z", "i2c4_scl_z", "i2c4_scl_a", "i2c4_sda_a",
};

static const char * const i2c5_groups[] = {
	"i2c5_scl_z", "i2c5_sda_z", "i2c5_sda_a10", "i2c5_scl_a11",
	"i2c5_sda_a14", "i2c5_scl_a15"
};

static const char * const ir_remote_groups[] = {
	"ir_remote_in_c", "ir_remote_out_f", "ir_remote_in_f3",
	"ir_remote_in_d", "ir_remote_out_h", "ir_remote_out_z",
	"ir_remote_out_a", "ir_remote_in_d10"
};

static const char * const jtag_groups[] = {
	"jtag_clk_d", "jtag_tms_d", "jtag_tdi_d", "jtag_tdo_d",
	"jtag_clk_h", "jtag_tms_h", "jtag_tdi_h", "jtag_tdo_h"
};

static const char * const jtag2_groups[] = {
	"jtag2_tdo", "jtag2_tdi", "jtag2_clk", "jtag2_tms"
};

static const char * const ao_cec_groups[] = {
	"ao_cec_d", "ao_cec_h"
};

static const char * const pwm_a_groups[] = {
	"pwm_a_f", "pwm_a_e", "pwm_a_x6"
};

static const char * const pwm_a_hiz_groups[] = {
	"pwm_a_hiz"
};

static const char * const pwm_b_groups[] = {
	"pwm_b_x", "pwm_b_d", "pwm_b_f",
	"pwm_b_h5", "pwm_b_h7", "pwm_b_z0",
	"pwm_b_z13"
};

static const char * const pwm_b_hiz_groups[] = {
	"pwm_b_hiz"
};

static const char * const pwm_c_groups[] = {
	"pwm_c_z", "pwm_c_f", "pwm_c_d6",
	"pwm_c_d2", "pwm_c_x5", "pwm_c_x8"
};

static const char * const pwm_c_hiz_groups[] = {
	"pwm_c_hiz"
};

static const char * const pwm_d_groups[] = {
	"pwm_d"
};

static const char * const pwm_e_groups[] = {
	"pwm_e"
};

static const char * const pwm_f_groups[] = {
	"pwm_f_e", "pwm_f_x", "pwm_f_z"
};

static const char * const clk_32k_in_groups[] = {
	"clk_32k_in"
};

static const char * const gen_clk_groups[] = {
	"gen_clk_f", "gen_clk_e", "gen_clk_c",
	"gen_clk_x", "gen_clk_z", "gen_clk_a"
};

static const char * const spidf_in_groups[] = {
	"spdif_in_f", "spdif_in_h", "spdif_in_a12", "spdif_in_a14"
};

static const char * const spidf_out_groups[] = {
	"spdif_out_d", "spdif_out_h", "spdif_out_a"
};

static const char * const tsin_a_groups[] = {
	"tsin_a_sop_d", "tsin_a_din0_d", "tsin_a_clk_d", "tsin_a_valid_d",
	"tsin_a_clk_c", "tsin_a_sop_c", "tsin_a_valid_c", "tsin_a_din0_c",
	"tsin_a_din0_z", "tsin_a_sop_z", "tsin_a_valid_z",
	"tsin_a_clk_z",
};

static const char * const tsin_b_groups[] = {
	"tsin_b_clk_c", "tsin_b_sop_c", "tsin_b_valid_c", "tsin_b_din0_c",
	"tsin_b_din0_x0", "tsin_b_sop_x1", "tsin_b_valid_x2",
	"tsin_b_clk_x3", "tsin_b_sop_x8", "tsin_b_valid_x9", "tsin_b_din0_x10",
	"tsin_b_clk_x11",
	"tsin_b_sop_z4", "tsin_b_valid_z5", "tsin_b_din0_z6", "tsin_b_clk_z7",
	"tsin_b_valid_z2", "tsin_b_sop_z3", "tsin_b_din0_z4", "tsin_b_fail",
	"tsin_b_din1", "tsin_b_din2", "tsin_b_din3",
	"tsin_b_din4", "tsin_b_din5", "tsin_b_din6",
	"tsin_b_din7"
};

static const char * const tsin_c_groups[] = {
	"tsin_c_valid", "tsin_c_sop", "tsin_c_din0", "tsin_c_clk",
};

static const char * const tsin_d_groups[] = {
	"tsin_d_valid", "tsin_d_sop", "tsin_d_din0", "tsin_d_clk",
};

static const char * const tdm_groups[] = {
	"tdm_d2_d", "tdm_fs1_d", "tdm_sclk1_d", "tdm_d3_d",
	"tdm_d4_f", "tdm_d5_f", "tdm_d6_c", "tdm_d7_c",
	"tdm_fs1_c", "tdm_sclk1_c", "tdm_d8_c", "tdm_d9_c",
	"tdm_d1_x", "tdm_d0_x", "tdm_fs0_x", "tdm_sclk0_x", "tdm_d2_x0",
	"tdm_d3_x1", "tdm_fs1_x", "tdm_sclk1_x", "tdm_d4_x5",
	"tdm_d5_x6", "tdm_d2_x7", "tdm_d3_x17", "tdm_d4_x2",
	"tdm_d5_x3", "tdm_d4_z", "tdm_d5_z", "tdm_d6_z", "tdm_d7_z",
	"tdm_fs2_z", "tdm_sclk2_z", "tdm_sclk1_z", "tdm_fs1_z", "tdm_d8_z",
	"tdm_d9_z", "tdm_d10_z", "tdm_d11_z", "tdm_d2_a",
	"tdm_d3_a", "tdm_d5_a", "tdm_d6_a", "tdm_d7_a", "tdm_fs2_a",
	"tdm_sclk2_a", "tdm_sclk1_a", "tdm_fs1_a", "tdm_d8_a", "tdm_d9_a",
	"tdm_d10_a14", "tdm_d11_a15", "tdm_d10_a7", "tdm_d11_a8", "tdm_d3_c"
};

static const char * const mclk_groups[] = {
	"mclk_1_d", "mclk_1_c", "mclk_1_x", "mclk_2_z",
	"mclk_1_z", "mclk_2_a", "mclk_1_a"
};

static const char * const clk12m_24m_groups[] = {
	"clk12m_24m_d", "clk12m_24m_e", "clk12m_24m_z"
};

static const char * const spi_a_groups[] = {
	"spi_a_ss0_f", "spi_a_clk_f", "spi_a_ss0_x", "spi_a_clk_x",
	"spi_a_ss0_h", "spi_a_clk_h", "spi_a_ss0_a", "spi_a_clk_a",
	"spi_a_mosi_f", "spi_a_miso_f", "spi_a_mosi_x", "spi_a_miso_x",
	"spi_a_mosi_h", "spi_a_miso_h", "spi_a_mosi_a", "spi_a_miso_a"
};

static const char * const pdm_groups[] = {
	"pdm_din0_d", "pdm_din1_d", "pdm_din2_d", "pdm_dclk_d",
	"pdm_din0_b", "pdm_dclk_b", "pdm_din0_c", "pdm_din1_c",
	"pdm_din2_c", "pdm_din3_c", "pdm_dclk_c", "pdm_din0_z", "pdm_din1_z",
	"pdm_dclk_z", "pdm_dclk_z", "pdm_din2_a", "pdm_din3_a",
	"pdm_din0_x0", "pdm_din1_x1", "pdm_din2_x2", "pdm_din3_x3",
	"pdm_dclk_x4", "pdm_dclk_x8", "pdm_din0_x9", "pdm_din2_z4", "pdm_din3_z5",
	"pdm_din2_z7", "pdm_din3_z8", "pdm_din0_a0", "pdm_din1_a1",
	"pdm_dclk_a2", "pdm_dclk_a9", "pdm_din1_x10", "pdm_din2_x11", "pdm_din0_x12",
	"pdm_din1_x13", "pdm_din2_x14", "pdm_dclk_x15", "pdm_din2_z14",
	"pdm_din3_z15", "pdm_din0_a12", "pdm_din1_a13"
};

static const char * const mic_mute_groups[] = {
	"mic_mute_en_d", "mic_mute_en_a", "mic_mute_key_d", "mic_mute_key_a"
};

static const char * const emmc_groups[] = {
	"emmc_nand_d0", "emmc_nand_d1", "emmc_nand_d2", "emmc_nand_d3",
	"emmc_nand_d4", "emmc_nand_d5", "emmc_nand_d6", "emmc_nand_d7", "emmc_ds",
	"emmc_clk", "emmc_cmd", "emmc_rstgpio",
};

static const char * const nand_groups[] = {
	"emmc_nand_d0", "emmc_nand_d1", "emmc_nand_d2", "emmc_nand_d3",
	"emmc_nand_d4", "emmc_nand_d5", "emmc_nand_d6", "emmc_nand_d7",
	"nand_ale", "nand_cle", "nand_ce0", "nand_ren_wr",
	"nand_wen_clk"
};

static const char * const spinf_groups[] = {
	"spinf_clk", "spinf_d4", "spinf_d5", "spinf_d6",
	"spinf_d7", "spinf_cs1", "spinf_cs0", "spinf_mo_d0", "spinf_mi_d1",
	"spinf_wp_d2", "spinf_rstgpio", "spinf_hold_d3"
};

static const char * const sdcard_groups[] = {
	"sdcard_d0", "sdcard_d1", "sdcard_d2", "sdcard_d3",
	"sdcard_clk", "sdcard_cmd", "card_det"
};

static const char * const sdio_groups[] = {
	"sdio_d0", "sdio_d1", "sdio_d2", "sdio_d3",
	"sdio_clk", "sdio_cmd"
};

static const char * const pcieck_groups[] = {
	"pcieck_reqn"
};

static const char * const iso7816_groups[] = {
	"iso7816_clk_h", "iso7816_clk_z", "iso7816_clk_a", "iso7816_clk_c5",
	"iso7816_clk_c0", "iso7816_clk_x8", "iso7816_clk_x0", "iso7816_data_h",
	"iso7816_data_z", "iso7816_data_a", "iso7816_data_c6", "iso7816_data_c1",
	"iso7816_clk_x12", "iso7816_data_x9", "iso7816_data_x1",
	"iso7816_data_x13"
};

static const char * const eth_groups[] = {
	"eth_mdio", "eth_mdc", "eth_rgmii_rx_clk", "eth_rx_dv",
	"eth_rxd0", "eth_rxd1", "eth_rxd2_rgmii", "eth_rxd3_rgmii",
	"eth_rgmii_tx_clk", "eth_txen", "eth_txd0", "eth_txd1",
	"eth_txd2_rgmii", "eth_txd3_rgmii", "eth_link_led_z", "eth_act_led_z",
	"eth_link_led_a", "eth_act_led_a"
};

static const char * const hdmitx_groups[] = {
	"hdmitx_sda", "hdmitx_scl", "hdmitx_hpd_in"
};

static struct meson_pmx_func meson_s6_periphs_functions[] = {
	FUNCTION(gpio_periphs),
	FUNCTION(uart_a),
	FUNCTION(uart_b),
	FUNCTION(uart_c),
	FUNCTION(i2c0),
	FUNCTION(i2c1),
	FUNCTION(i2c2),
	FUNCTION(i2c3),
	FUNCTION(i2c4),
	FUNCTION(i2c5),
	FUNCTION(ir_remote),
	FUNCTION(jtag),
	FUNCTION(jtag2),
	FUNCTION(pwm_a),
	FUNCTION(pwm_b),
	FUNCTION(pwm_c),
	FUNCTION(pwm_d),
	FUNCTION(pwm_e),
	FUNCTION(pwm_f),
	FUNCTION(pwm_a_hiz),
	FUNCTION(pwm_b_hiz),
	FUNCTION(pwm_c_hiz),
	FUNCTION(gen_clk),
	FUNCTION(spidf_in),
	FUNCTION(spidf_out),
	FUNCTION(tsin_a),
	FUNCTION(tsin_b),
	FUNCTION(tsin_c),
	FUNCTION(tsin_d),
	FUNCTION(tdm),
	FUNCTION(mclk),
	FUNCTION(clk12m_24m),
	FUNCTION(spi_a),
	FUNCTION(pdm),
	FUNCTION(emmc),
	FUNCTION(nand),
	FUNCTION(spinf),
	FUNCTION(sdcard),
	FUNCTION(sdio),
	FUNCTION(pcieck),
	FUNCTION(iso7816),
	FUNCTION(eth),
	FUNCTION(mic_mute),
	FUNCTION(clk_32k_in),
	FUNCTION(ao_cec),
	FUNCTION(hdmitx)
};

static struct meson_bank meson_s6_periphs_banks[] = {
	/*    name   first   last   irq   pullen   pull   dir   out   in   ds */
	BANK_DS("D",  GPIOD_0,  GPIOD_6,
		0x033,  0, 0x034,  0, 0x032,  0, 0x031,  0, 0x030,  0, 0x037,  0),
	BANK_DS("F",  GPIOF_0,  GPIOF_4,
		0x03b,  0, 0x03c,  0, 0x03a,  0, 0x039,  0, 0x038,  0, 0x03f,  0),
	BANK_DS("E",  GPIOE_0,  GPIOE_2,
		0x043,  0, 0x044,  0, 0x042,  0, 0x041,  0, 0x040,  0, 0x047,  0),
	BANK_DS("B",  GPIOB_0, GPIOB_13,
		0x063,  0, 0x064,  0, 0x062,  0, 0x061,  0, 0x060,  0, 0x067,  0),
	BANK_DS("C",  GPIOC_0,  GPIOC_7,
		0x053,  0, 0x054,  0, 0x052,  0, 0x051,  0, 0x050,  0, 0x057,  0),
	BANK_DS("X",  GPIOX_0, GPIOX_19,
		0x013,  0, 0x014,  0, 0x012,  0, 0x011,  0, 0x010,  0, 0x017,  0),
	BANK_DS("H",  GPIOH_0, GPIOH_8,
		0x023,  0, 0x024,  0, 0x022,  0, 0x021,  0, 0x020,  0, 0x027,  0),
	BANK_DS("Z",  GPIOZ_0, GPIOZ_15,
		0x003,  0, 0x004,  0, 0x002,  0, 0x001,  0, 0x000,  0, 0x007,  0),
	BANK_DS("A", GPIOA_0,  GPIOA_15,
		0x073,  0, 0x074,  0, 0x072,  0, 0x071,  0, 0x070,  0, 0x077,  0),
	BANK_DS("_TEST_N", GPIO_TEST_N, GPIO_TEST_N,
		0x083,  0, 0x084,  0, 0x082,  0, 0x081,  0, 0x080,  0, 0x087,  0),
	BANK_DS("CC", GPIO_CC1, GPIO_CC2,
		0x091,  2, 0x091,  4, 0x092,  0, 0x091,  0, 0x090,  0, 0x091,  6),
};

static struct meson_pmx_bank meson_s6_periphs_pmx_banks[] = {
	/*       name      first        last         reg   offset */
	BANK_PMX("D",      GPIOD_0,     GPIOD_5,     0x002,   0),
	BANK_PMX("D1",     GPIOD_6,     GPIOD_6,     0x008,   0),
	BANK_PMX("F",      GPIOF_0,     GPIOF_4,     0x008,   4),
	BANK_PMX("E",      GPIOE_0,     GPIOE_2,     0x012,   0),
	BANK_PMX("B",      GPIOB_0,     GPIOB_13,    0x000,   0),
	BANK_PMX("C",      GPIOC_0,     GPIOC_7,     0x009,   0),
	BANK_PMX("X",      GPIOX_0,     GPIOX_19,    0x003,   0),
	BANK_PMX("H",      GPIOH_0,     GPIOH_8,     0x00b,   0),
	BANK_PMX("Z",      GPIOZ_0,     GPIOZ_15,    0x006,   0),
	BANK_PMX("A",      GPIOA_0,     GPIOA_15,    0x010,   0),
	BANK_PMX("TEST_N", GPIO_TEST_N, GPIO_TEST_N, 0x00f,   0),
	BANK_PMX("CC",     GPIO_CC1,    GPIO_CC2,    0x005,  24),
};

static struct meson_axg_pmx_data meson_s6_periphs_pmx_banks_data = {
	.pmx_banks	= meson_s6_periphs_pmx_banks,
	.num_pmx_banks	= ARRAY_SIZE(meson_s6_periphs_pmx_banks),
};

static int meson_s6_parse_dt_extra(struct meson_pinctrl *pc)
{
	pc->reg_ds = pc->reg_gpio;

	return 0;
}

static struct meson_pinctrl_data meson_s6_periphs_pinctrl_data = {
	.name		= "periphs-banks",
	.groups		= meson_s6_periphs_groups,
	.funcs		= meson_s6_periphs_functions,
	.banks		= meson_s6_periphs_banks,
	.num_pins	= 101,
	.num_groups	= ARRAY_SIZE(meson_s6_periphs_groups),
	.num_funcs	= ARRAY_SIZE(meson_s6_periphs_functions),
	.num_banks	= ARRAY_SIZE(meson_s6_periphs_banks),
	.gpio_driver	= &meson_axg_gpio_driver,
	.pmx_data	= &meson_s6_periphs_pmx_banks_data,
	.parse_dt	= &meson_s6_parse_dt_extra,
};

static const struct udevice_id meson_s6_pinctrl_match[] = {
	{
		.compatible = "amlogic,meson-s6-periphs-pinctrl",
		.data = (ulong)&meson_s6_periphs_pinctrl_data,
	},
	{ }
};

U_BOOT_DRIVER(meson_s6_pinctrl) = {
	.name	= "meson-s6-pinctrl",
	.id	= UCLASS_PINCTRL,
	.of_match = of_match_ptr(meson_s6_pinctrl_match),
	.probe = meson_pinctrl_probe,
	.priv_auto = sizeof(struct meson_pinctrl),
	.ops = &meson_axg_pinctrl_ops,
};
