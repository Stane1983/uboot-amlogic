// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>

#define DDR_FUNC_CONFIG_ENABLE_PZQ_DET_DRAM_TYPE_RETURN                   (0 + (1 << 20))
#define DDR_FUNC_CONFIG_AUTO_DET_DQ_PINMUX_FUNCTION                   (0 + (1 << 21))
//bit 6 adc_channel bit 0-5 adc value,chan 3 value 8 is layer 2
#define DDR_ID_ACS_ADC   ((3 << 6) | (8))

#define DDR_RESV_CHECK_ID_ENABLE  0Xfe
#define SAR_ADC_DDR_ID_BASE   0
#define SAR_ADC_DDR_ID_STEP   80

#define DDR_TIMMING_OFFSET(X) (unsigned int)(unsigned long)(&(((ddr_set_ps0_only_t *)(0))->X))
#define DDR_TIMMING_OFFSET_SIZE(X) sizeof(((ddr_set_ps0_only_t *)(0))->X)
#define DDR_TIMMING_TUNE_TIMMING0(DDR_ID, PARA, VALUE) (DDR_ID, \
DDR_TIMMING_OFFSET(PARA), VALUE, DDR_TIMMING_OFFSET_SIZE(PARA), 0, \
DDR_RESV_CHECK_ID_ENABLE)
#define DDR_TIMMING_TUNE_TIMMING1(DDR_ID, PARA, VALUE) (DDR_ID, \
(sizeof(ddr_set_t) + (DDR_TIMMING_OFFSET(PARA))), VALUE, DDR_TIMMING_OFFSET_SIZE(PARA), \
0, DDR_RESV_CHECK_ID_ENABLE)

//bit24-31 define ID and size
#define DDR_ID_FROM_EFUSE  (0Xff000000)
#define DDR_ID_FROM_ADC  (0Xfe000000)
#define DDR_ID_FROM_GPIO_CONFIG1  (0Xfd000000)
#define DDR_ID_FROM_EFUSE_F  (0Xff << 0)
#define DDR_ID_FROM_ADC_F  (0Xfe << 0)
#define DDR_ID_FROM_GPIO_CONFIG1_F  (0Xfd << 0)
#define DDR_ID_FROM_ADC_MULT (0Xfc000000)
#define DDR_ID_FROM_ADC_MULT_F   (0Xfc << 0)
#define DDR_ID_START_MASK  (0XFFDDCCBB)

#define DDR_ADC_CH0 (0X0 << 5)
#define DDR_ADC_CH1 (0X1 << 5)
#define DDR_ADC_CH2 (0X2 << 5)
#define DDR_ADC_CH3 (0X3 << 5)
#define DDR_ADC_CH4 (0X4 << 5)

#define DDR_ADC_VALUE0 (0X0 << 0)
#define DDR_ADC_VALUE1 (0X1 << 0)
#define DDR_ADC_VALUE2 (0X2 << 0)
#define DDR_ADC_VALUE3 (0X3 << 0)
#define DDR_ADC_VALUE4 (0X4 << 0)
#define DDR_ADC_VALUE5 (0X5 << 0)
#define DDR_ADC_VALUE6 (0X6 << 0)
#define DDR_ADC_VALUE7 (0X7 << 0)
#define DDR_ADC_VALUE8 (0X8 << 0)
#define DDR_ADC_VALUE9 (0X9 << 0)
#define DDR_ADC_VALUE10 (0Xa << 0)
#define DDR_ADC_VALUE11 (0Xb << 0)
#define DDR_ADC_VALUE12 (0Xc << 0)
#define DDR_ADC_VALUE13 (0Xd << 0)
#define DDR_ADC_VALUE14 (0Xe << 0)
#define DDR_ADC_VALUE15 (0Xf << 0)
#define V0  (0X0 << 0)
#define V1  (0X1 << 0)
#define V2  (0X2 << 0)
#define V3  (0X3 << 0)
#define V4  (0X4 << 0)
#define V5  (0X5 << 0)
#define V6  (0X6 << 0)
#define V7  (0X7 << 0)
#define V8  (0X8 << 0)
#define V9  (0X9 << 0)
#define V10  (0Xa << 0)
#define V11  (0Xb << 0)
#define V12  (0Xc << 0)

#define VX  (0Xf << 0)

typedef struct ddr_para_data {
	// start from	DDR_ID_START_MASK,ddr_id;//bit0-23
	// ddr_id value,bit 24-31 ddr_id source  ,0xfe source
	// from adc ,0xfd source from gpio_default_config
	// reg_offset
	// //bit 0-15 parameter offset value,bit16-23 overrid
	// size,bit24-31 mux ddr_id source unsigned int
	// reg_offset; unsigned int	value;
	uint32_t	value : 16;             // bit0-15 only support data size =1byte
	// or 2bytes,no support int value
	uint32_t	reg_offset : 12;        // bit16-27
	uint32_t	data_size : 4;          // bit28-31 if data size =15,then
	// will mean DDR_ID start
} ddr_para_data_t;

typedef struct ddr_para_data_start {
	uint32_t	id_value : 24;          // bit0-23  efuse id or ddr id
	// uint32_t	id_adc_ch : 2;//bit6-7
	uint32_t	id_src_from : 8;        // bit24-31 ddr id from adc or gpio
} ddr_para_data_start_t;

#define DDR_TIMMING_TUNE_STRUCT_SIZE(a)  sizeof(a)

#define DDR_TIMMING_TUNE_TIMMING0_F(PARA, VALUE) ((DDR_TIMMING_OFFSET(PARA)) << 16) |\
((DDR_TIMMING_OFFSET_SIZE(PARA)) << 28) | VALUE
#define DDR_TIMMING_TUNE_TIMMING1_F(PARA, VALUE) ((sizeof(ddr_set_ps0_only_t) +\
DDR_TIMMING_OFFSET(PARA)) << 16) | ((DDR_TIMMING_OFFSET_SIZE(PARA)) << 28) | (VALUE)

#define DDR_TIMMING_TUNE_START(id_src_from, id_adc_ch, id_value) (id_src_from) |\
(id_adc_ch) | (id_value)
#define DDR_TIMMING_TUNE_ADC_MULT_START(id_value, ch0, ch1, ch2, ch3, ch4, ch5) (id_value) |\
(ch0) | ((ch1) << 4) | ((ch2) << 8) | ((ch3) << 12) | ((ch4) << 16) | ((ch5) << 20)
#define DDR_TIMMING_TUNE_STRUCT_SIZE(a)  sizeof(a)

#if 1
uint32_t __bl2_ddr_reg_data[] __attribute__ ((section(".ddr_2acs_data"))) = {
	DDR_ID_START_MASK,
	//DDR_TIMMING_TUNE_ADC_MULT_START(DDR_ID_FROM_ADC_MULT, V4, VX, VX, VX, VX, VX),
	//data start
	//DDR_TIMMING_TUNE_TIMMING0_F(cfg_board_common_setting.Is2Ttiming, CONFIG_USE_DDR_2T_MODE),
	//DDR_TIMMING_TUNE_TIMMING0_F(cfg_board_SI_setting_ps.DRAMFreq, 1320),
};

////_ddr_para_2nd_setting

uint32_t __ddr_parameter_reg_index[] __attribute__ ((section(".ddr_2acs_index"))) = {
	0,
};
#endif

#define DDR4_USE_2CHANNEL 1
// #define LPDDR4_SKT 0
#define DDR4_SKT 1
#define NON_SIP_DDR4_REF 1
// #define DDR3_SKT 1
#define CONFIG_BOARD_TIMMING

ddr_set_ps0_only_t __ddr_setting[] __attribute__ ((section(".ddr_param"))) = {

#if DDR4_SKT
//s7 signoff with 3200MBPS
#define  CACLU_CLK_D4   1584// 600 //1200 //1792//600 //1200 //(1900)// (1440)//(1008)
	{
		.cfg_board_common_setting.timming_magic = 0,
		.cfg_board_common_setting.timming_max_valid_configs = 1,
			//sizeof(ddr_set_t_default) / sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_version = 0,
		.cfg_board_common_setting.timming_struct_org_size =
			sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_real_size = 0,
		.cfg_board_common_setting.fast_boot = {
			0, 0, 0, 0
		},
		.cfg_board_common_setting.ddr_func = DDR_FUNC_CONFIG_AUTO_DET_DQ_PINMUX_FUNCTION,
		//DDR_FUNC_CONFIG_DFE_FUNCTION |
		//DDR_FUNC_CONFIG_ENABLE_PZQ_DET_DRAM_TYPE_RETURN,
		.cfg_board_common_setting.board_id = CONFIG_BOARD_ID_MASK,
		.cfg_board_common_setting.DramType = CONFIG_DDR_TYPE_DDR4,
		.cfg_board_common_setting.enable_lpddr4x_mode = 0,
		.cfg_board_common_setting.dram_rank_config =
		//CONFIG_DDR0_16BIT_CH0,
		CONFIG_DDR0_32BIT_RANK0_CH0,
		//.cfg_board_common_setting.dram_cs0_base_add = 0,
		//.cfg_board_common_setting.dram_cs1_base_add = 0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.dram_ch0_size_MB = 0xffff,
		.cfg_board_common_setting.DisabledDbyte[0] = 0x00,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0x00,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		// .cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.dbi_enable = DDR_WRITE_READ_DBI_DISABLE,
		.cfg_board_common_setting.pll_ssc_mode =
			(1 << 20) | (1 << 8) | (2 << 4) | 0,
		//center_ssc_1000ppm,//SSC_DISABLE,(1 << 20) | (0 << 8) | (2 << 4) | 0,
		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
#ifdef DDR4_USE_2CHANNEL
		.cfg_board_common_setting.ddr_dmc_remap =
		//DDR_DMC_REMAP_DDR4_16BIT_S1A,
		//DDR_DMC_REMAP_DDR4_16BIT_2,
		DDR_DMC_REMAP_DDR4_16BIT,
#else
		.cfg_board_common_setting.ddr_dmc_remap =
		DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
#endif
#ifdef ENABLE_DDR3_4_16BIT
		//ddr4 32bit
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_16BIT_CH0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.DisabledDbyte[0] = 0xfc,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_DDR4_16BIT,
#endif
		// .cfg_board_common_setting.ac_pinmux = {
		//2, 3, 1, 0, 5, 4, 0, 0, 0, 0, 1, 3, 5, 2, 4, 0 },
		.cfg_board_common_setting.ddr_dqs_swap = 0,

		.cfg_board_common_setting.ddr_vddee_setting = {
			0
		},
		.cfg_board_SI_setting_ps.DRAMFreq = CACLU_CLK_D4,
		// .cfg_ddr_training_delay_ps.DRAMFreq = 600,
		// 2112,//1176,cfg_ddr_training_delay_ps
		//.cfg_ddr_training_delay_ps.PllBypassEn = 0,
		.cfg_board_SI_setting_ps.training_SequenceCtrl = 0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_W_CS0_ODT0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_DDR4_PARK_ENABLE,
		//DDR_DRAM_ODT_W_CS0_ODT0,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,
		.cfg_board_SI_setting_ps.vref_ac_permil = 0,
		.cfg_board_SI_setting_ps.vref_soc_data_permil = 0,
		.cfg_board_SI_setting_ps.vref_dram_data_permil = 800,
		.cfg_board_SI_setting_ps.max_core_timmming_frequency = 0,
// 1 //real chip stk lp4
		.cfg_board_common_setting.dbi_enable = 0x00000000,
		.cfg_board_common_setting.ddr_rfc_type = DDR_RFC_TYPE_DDR4_2Gbx4,//DDR_RFC_TYPE_DDR4_2Gbx8,
		.cfg_board_common_setting.pll_ssc_mode = 0x00000000,


#ifdef CONFIG_BOARD_TIMMING     //skt lp4 board
#define TDQS2DQ_D4  ((0 * 128 * CACLU_CLK_D4) / 1000000)
//#define BOARD_DQS_DELAY   64+32
#define TDQSCK_ADD_DQS_D4   ((0 * 128 * CACLU_CLK_D4) / 1000000) //clk should use 64 steps
#define CLK_DELAY_D4  (0)
#define PHY_TDQS2DQ_D4  ((0 * 128 * CACLU_CLK_D4) / 1000000)
#define WL0  ((0 * 128 * CACLU_CLK_D4 * 2) / 1000000)
//for 1584M 100 STEP 250ps //((480 * 128 * CACLU_CLK_D4 * 2) / 1000000)
#endif
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n = 40,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p = 60,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 0,
		.cfg_board_SI_setting_ps.dram_data_drv_ohm = 34,
		.cfg_board_SI_setting_ps.dram_data_odt_ohm = 60,
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm = 0,
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm = 120,
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm =
			DDR_DRAM_LPDDR4_ODT_40_OHM,
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range =
			DDR_DRAM_LPDDR4_OUTPUT_1_3_VDDQ,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,

//#ifdef DDR4_USE_2CHANNEL
		.cfg_board_common_setting.ac_pinmux = {
		3,	6,	5,	1,	0,	9,	13,	8,	7,	2,
		11,	10,	4,	14,	15,	30,	32,	22,	24,	21,	18,
		16,	20,	19,	17,	12,	26,	28,	34,	35,
		},
//#else
//		.cfg_board_common_setting.ac_pinmux = {///T6w pinmux
//		3,	6,	5,	1,	0,	9,	13,	8,	7,	2,
//		11,	10,	4,	14,	15,	30,	30,	22,	22,	21,	18,
//		16,	20,	19,	17,	12,	26,	26,	34,	35,
//		},
//#endif

#if 1
		.cfg_board_common_setting.ac_pinmux = {///T6w pinmux non-sip ddr4
			9,	18,	7,	19,	22,	12,	5,	24,	20,	32,
		15,	30,	13,	1,	6,	8,	21,	2,	17,	10,	14,
		11,	0,	4,	16,	3,	26,	28,	35,	34,
		},
#endif
		.cfg_board_common_setting.ddr_dq_remap = {
			32,
			0,
			1,
			3,
			2,
			5,
			4,
			7,
			6,
			33,
			9,
			8,
			10,
			11,
			13,
			12,
			15,
			14,
			16,
			18,
			19,
			17,
			21,
			20,
			23,
			22,
			34,
			26,
			24,
			27,
			25,
			28,
			29,
			30,
			31,
			35,
		},
		.cfg_board_common_setting.ddr_dq_remap = { 0, 0, 0},
		//.cfg_board_common_setting.ddr_dq_remap= {
		// 3, 0, 2, 1, 4, 6, 5, 7,  14, 12, 13, 15,
		//.cfg_ddr_training_delay_ps.rx_offset[0] = (0 << 7) | 0x0,
#ifdef CONFIG_BOARD_TIMMING
		.cfg_ddr_training_delay_ps.tx_offset[0] = (0 << 7) | 0x0,
		.cfg_ddr_training_delay_ps.rx_offset[0] = (0 << 7) | 0x0,
		.cfg_ddr_training_delay_ps.reserve_para[0] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[1] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[2] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[3] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[4] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[5] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[6] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[7] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 0] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 1] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 2] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 3] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 4] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 5] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 6] = (1 << 7) | 0x10,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 7] = (1 << 7) | 0x10,//read dqs
#ifdef DDR4_USE_2CHANNEL
		#define  AC_OFFSET  (128) - 128 - 0  //non-sip
		#define ac_off_1t (-128)
		#define CK_OFFSET (128) - 128 -128
#else
		#define  AC_OFFSET  (128) - 128 - 0  //non-sip
		#define ac_off_1t (0)
		#define CK_OFFSET (128) - 128 -128
#endif
#endif
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 256 + AC_OFFSET + ac_off_1t,
		//cke 128 only 1UI margin
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 256 + AC_OFFSET + ac_off_1t,
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 256 + AC_OFFSET + ac_off_1t,

#if NON_SIP_DDR4_REF
		//ref board
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256 + AC_OFFSET + ac_off_1t - 40,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256 + AC_OFFSET + ac_off_1t - 40,

		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 128 + CK_OFFSET + 120 - 40,       // cs0
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 128 + CK_OFFSET + 140 - 40,      // cs1
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + CK_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + CK_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + CK_OFFSET,     //cke  real chip
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + CK_OFFSET,     //cke1
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 + CK_OFFSET,        //odt1
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 128 + CK_OFFSET,       //odt0

		//.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 100 + 256 + AC_OFFSET + ac_off_1t - 60,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 100 + 256 + AC_OFFSET + ac_off_1t - 40,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 100 + 256 + AC_OFFSET + ac_off_1t- 80,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 100 + 256 + AC_OFFSET + ac_off_1t -80,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 100 + 256 + AC_OFFSET + ac_off_1t - 100,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 100 + 256 + AC_OFFSET + ac_off_1t,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 100 + 256 + AC_OFFSET + ac_off_1t,

		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 256 + AC_OFFSET + ac_off_1t - 70,
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 256 + AC_OFFSET + ac_off_1t - 60,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 256 + AC_OFFSET + ac_off_1t + 40,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 256 + AC_OFFSET + ac_off_1t + 40,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 256 + AC_OFFSET + ac_off_1t + 40,
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256 + AC_OFFSET + ac_off_1t - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256 + AC_OFFSET + ac_off_1t - 20,
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
#endif
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[64] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 64 + 16,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[1] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[2] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[3] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[4] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[5] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[6] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[7] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[8] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[9] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[10] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[11] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[12] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[13] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[14] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[15] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[16] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[17] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[18] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[19] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[20] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[21] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[22] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[23] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[24] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[25] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[26] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[27] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[28] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[29] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[30] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[31] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[32] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[33] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[34] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[35] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[36] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[37] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[38] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[39] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[40] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[41] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[42] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[43] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[44] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[45] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[46] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[47] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[48] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[49] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[50] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[51] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[52] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[53] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[54] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[55] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[56] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[57] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[58] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[59] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[60] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[61] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[62] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[63] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[64] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[65] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[66] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[67] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[68] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[69] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[70] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[71] = 64,

#if 0
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 0 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 0 + CK_OFFSET + WL0,
#else
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 456 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 456 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 556 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 556 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 256 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 256 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 256 + CK_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 256 + CK_OFFSET + WL0,
#endif

		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x00000200,

		.cfg_ddr_training_delay_ps.wdq_delay[0] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 64 + CK_OFFSET + TDQS2DQ_D4 + WL0,

		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 128 * 3 + AC_OFFSET,

		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 128,

		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x000000,
		//0 for auto training
		.cfg_ddr_training_delay_ps.soc_bit_vref0[1] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[2] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[3] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[4] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[5] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[6] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[7] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[8] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[9] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[10] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[11] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[12] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[13] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[14] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[15] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[16] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[17] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[18] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[19] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[20] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[21] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[22] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[23] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[24] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[25] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[26] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[27] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[28] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[29] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[30] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[31] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[32] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[33] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[34] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[35] = 0x000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[0] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[1] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[2] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[3] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[4] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[5] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[6] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[7] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[8] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[9] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[10] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[11] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[12] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[13] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[14] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[15] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[16] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[17] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[18] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[19] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[20] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[21] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[22] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[23] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[24] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[25] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[26] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[27] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[28] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[29] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[30] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[31] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[32] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[33] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[34] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[35] = 0x0000001a,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[0] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[1] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[2] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[3] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[4] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[5] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[6] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[7] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[8] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[9] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[10] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[11] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[12] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[13] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[14] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[15] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[16] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[17] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[18] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[19] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[20] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[21] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[22] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[23] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[24] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[25] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[26] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[27] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[28] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[29] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[30] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[31] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[32] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[33] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[34] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref2[35] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[0] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[1] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[2] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[3] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[4] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[5] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[6] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[7] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[8] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[9] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[10] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[11] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[12] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[13] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[14] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[15] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[16] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[17] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[18] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[19] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[20] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[21] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[22] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[23] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[24] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[25] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[26] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[27] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[28] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[29] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[30] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[31] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[32] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[33] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[34] = 0x00000030,
		.cfg_ddr_training_delay_ps.soc_bit_vref3[35] = 0x00000030,
		.cfg_ddr_training_delay_ps.dram_vref[0] = 0x0000001d,
		.cfg_ddr_training_delay_ps.dram_vref[1] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[2] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[3] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[4] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[5] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[6] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[7] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[8] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[9] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[10] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[11] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[12] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[13] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[14] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[15] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[16] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[17] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[18] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[19] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[20] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[21] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[22] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[23] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[24] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[25] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[26] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[27] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[28] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[29] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[30] = 0x00000000,
		.cfg_ddr_training_delay_ps.dram_vref[31] = 0x00000000,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[0] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[1] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[2] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[3] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[4] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[5] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[6] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[7] = 0x00000266,
		.cfg_ddr_training_delay_ps.dca_dq_tx[0] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[1] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[2] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[3] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[4] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[5] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[6] = 0x00000002,
		.cfg_ddr_training_delay_ps.dca_dq_tx[7] = 0x00000002,
		.cfg_ddr_training_delay_ps.dfi_mrl[0] = 0x00000004,
		.cfg_ddr_training_delay_ps.dfi_mrl[1] = 0x00000004,
		.cfg_ddr_training_delay_ps.dfi_mrl[2] = 0x00000004,
		.cfg_ddr_training_delay_ps.dfi_mrl[3] = 0x00000004,
		.cfg_ddr_training_delay_ps.dfi_hwtmrl = 0x00000004,
		.cfg_ddr_training_delay_ps.csr_hwtctrl = 0x00000004,

		.cfg_ddr_training_delay_ps.pptdqscnttg0[0] = 0x00000026,
		.cfg_ddr_training_delay_ps.pptdqscnttg0[1] = 0x00000026,
		.cfg_ddr_training_delay_ps.pptdqscnttg0[2] = 0x00000026,
		.cfg_ddr_training_delay_ps.pptdqscnttg0[3] = 0x00000026,
		.cfg_ddr_training_delay_ps.pptdqscnttg1[0] = 0x00000000,
		.cfg_ddr_training_delay_ps.pptdqscnttg1[1] = 0x00000000,
		.cfg_ddr_training_delay_ps.pptdqscnttg1[2] = 0x00000000,
		.cfg_ddr_training_delay_ps.pptdqscnttg1[3] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[0] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[1] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[2] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[3] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[0] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[1] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[2] = 0x00000000,
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[3] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[0][0] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[1][0] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[2][0] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[3][0] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[0][1] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[1][1] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[2][1] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[3][1] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[0][2] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[1][2] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[2][2] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[3][2] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[0][3] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[1][3] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[2][3] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[3][3] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[0][4] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[1][4] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[2][4] = 0x00000000,
		.cfg_ddr_training_delay_ps.RxReplicaPhase[3][4] = 0x00000000,

	},
//};
#endif
};
