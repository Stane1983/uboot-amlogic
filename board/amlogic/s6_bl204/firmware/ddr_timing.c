// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>

#define DDR_FUNC_CONFIG_RX_REPLICA_VT_ENABLE                   (0 + (1 << 17))
#define DDR_FUNC_CONFIG_ENABLE_PZQ_DET_DRAM_TYPE_RETURN                   (0 + (1 << 20))
#define DDR_FUNC_CONFIG_AUTO_DET_DQ_PINMUX_FUNCTION                   (0 + (1 << 21))
#define DDR_FUNC_CONFIG_RD_ECC_FUNCTION                                        (0 + (1 << 22))
#define DDR_FUNC_CONFIG_WR_ECC_FUNCTION                                        (0 + (1 << 17))
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

#define DDR_DMC_REMAP_LPDDR5_16BIT                         \
	{                                                        \
		[0] =  (0 | 0 << 6 | 6 << 12 | 7 << 18 | 8 << 24), \
		[1] =  (9 | 10 << 6 | 11 << 12 |  0 << 18 | 15 << 24), \
		[2] =  (16| 17 << 6 | 18 << 12 | 19 << 18 | 20 << 24), \
		[3] =  (21| 22 << 6 | 23 << 12 | 24 << 18 | 25 << 24), \
		[4] =  (26| 27 << 6 | 28 << 12 | 29 << 18 | 30 << 24), \
		[5] =  (12| 13 << 6 | 14 << 12 | 31 << 18 | 32 << 24), \
	}

#define LPDDR4_SKT 1
#define LPDDR5_SKT 1
//#define DDR4_SKT 1
//#define DDR3_SKT 1

//#define ENABLE_DDR_WINDOW_FAST_BOOT 1
//#define BOARD_USE_S905D5

ddr_set_ps0_only_t __ddr_setting[] __attribute__ ((section(".ddr_param"))) = {
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default[] = {
#if LPDDR5_SKT
#define  CACLU_CLK_LP5   687 //687--5500//600 //1200 //(1900)// (1440)//(1008)
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default_lp5 = {
	{
		.cfg_board_common_setting.timming_magic = 0,
		.cfg_board_common_setting.timming_max_valid_configs = 1,
		.cfg_board_common_setting.timming_struct_version = 9304,
		.cfg_board_common_setting.timming_struct_org_size =
			sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_real_size = 0,
#if ENABLE_DDR_WINDOW_FAST_BOOT
		.cfg_board_common_setting.fast_boot = {
			0x1, 0, 0, 0xc6
		},
#else
		.cfg_board_common_setting.fast_boot = {
			0x0, 0, 0, 0
		},
#endif
		.cfg_board_common_setting.ddr_func = DDR_FUNC_CONFIG_DFE_FUNCTION |
		DDR_FUNC_CONFIG_RD_ECC_FUNCTION,
		.cfg_board_common_setting.board_id = CONFIG_BOARD_ID_MASK,
		.cfg_board_common_setting.DramType = CONFIG_DDR_TYPE_LPDDR5,
		.cfg_board_common_setting.enable_lpddr4x_mode = 0,
		//0  force lp4x   1 force lp4
		//2 auto 4x use nn 4 use pn drivere
		//3 auto + force 4 4x both use nn driver
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_32BIT_RANK0_CH0,
		//.cfg_board_common_setting.dram_ch0_size_MB =
		//	(DRAM_SIZE_ID_512MBX4 << CONFIG_CS0_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX4 << CONFIG_CS0_BYTE_23_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_23_SIZE_512_ID_OFFSET),
		//.cfg_board_common_setting.dram_ch1_size_MB =
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS0_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS0_BYTE_23_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_23_SIZE_512_ID_OFFSET),
		.cfg_board_common_setting.dram_ch0_size_MB = 0xffff,
		.cfg_board_common_setting.DisabledDbyte[0] = 0x00,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0x00,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		//.cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.pll_ssc_mode =
			(1 << 20) | (1 << 8) | (2 << 4) | 0,
		//center_ssc_1000ppm,//SSC_DISABLE,(1 << 20) | (0 << 8) | (2 << 4) | 0,
		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_LPDDR5_16BIT,
		.cfg_board_common_setting.ddr_dqs_swap = 0,
		.cfg_board_common_setting.ddr_vddee_setting = {
			0
		},
		.cfg_board_SI_setting_ps.DRAMFreq = CACLU_CLK_LP5,
		.cfg_board_SI_setting_ps.training_SequenceCtrl = 0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_W_CS0_ODT0,
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n = 40,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p = 0,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 40, //60,
		.cfg_board_SI_setting_ps.dram_data_drv_ohm = 40,
		.cfg_board_SI_setting_ps.dram_data_odt_ohm = 60,
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm = 0,
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm = 240,//must 240 ohm for 8die 16GB LP5
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm =
			DDR_DRAM_LPDDR4_ODT_40_OHM,
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range =
			1,//DDR_DRAM_LPDDR4_OUTPUT_1_3_VDDQ,
		.cfg_board_SI_setting_ps.vref_ac_permil = 400,//375,//375 * 750 / 500,//500,//375,//420,
		.cfg_board_SI_setting_ps.vref_soc_data_permil = 0,
		.cfg_board_SI_setting_ps.vref_dram_data_permil = 0,
		.cfg_board_SI_setting_ps.max_core_timmming_frequency = 0,
		.cfg_board_common_setting.dbi_enable = 3,      // 0,0x00000041
		.cfg_board_common_setting.ddr_rfc_type = DDR_RFC_TYPE_LPDDR5_32Gbx1, // 13,
		.cfg_board_common_setting.pll_ssc_mode = 0x00000000,    // 0,0x00000044
		.cfg_board_common_setting.ac_pinmux = { //0-11 cha,12-23 chb
			3,	0,	2,	4,	6,	5,	0,	1,	0,
			0,	0,	0,
			4,	3,	0,	1,	0,	5,	6,	2,	0,
			0,	0,	0,
		},
#ifdef BOARD_USE_S905D5
		.cfg_board_common_setting.ac_pinmux = { //0-11 cha,12-23 chb
			1,	0,	2,	6,	4,	3,	5,	0,	0,
			0,	0,	0,
			3,	1,	2,	0,	6,	5,	0,	4,	0,
			0,	0,	0,
		},
#endif
		//.cfg_board_common_setting.ddr_dq_remap = {
		//	15,	13,	11,	10,	14,	12,	9,	8,	33,
		//	7,	2,	3,	0,	1,	4,	5,	6,	32,
		//	16 + 2,	16 + 3,	16 + 7,	16 + 0,	16 + 1,	16 + 5,	16 + 6,	16 + 4,	34,
		//	16 + 8,	16 + 10, 16 + 9, 16 + 11, 16 + 13, 16 + 14, 16 + 12, 16 + 15, 35,
		//},
		.cfg_board_common_setting.ddr_dq_remap = {
			0, 0 , 0,
		},
		//.cfg_ddr_training_delay_ps.rx_offset[0] = (1 << 7) | 0x10,
		//.cfg_ddr_training_delay_ps.tx_offset[0] = (1 << 7) | 0x8,
		.cfg_ddr_training_delay_ps.dac_offset[0] = (1 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[1] = (0 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[2] = (1 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[3] = (0 << 7) | 0x1,//1step 2mv
		//.cfg_ddr_training_delay_ps.dac_offset[0] = (1 << 7) | 0x3,
		//.cfg_ddr_training_delay_ps.dac_offset[1] = (0 << 7) | 0x3,
		.cfg_ddr_training_delay_ps.reserve_para[0] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[1] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[2] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[3] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[4] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[5] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[6] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[7] = (0 << 7) | 0x5,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 0] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 1] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 2] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 3] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 4] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 5] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 6] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 7] = (1 << 7) | 0x2,//read dqs
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 128 - 20, //0-11 cha,12-23 chb
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 128 - 20,//max511 1trip = 2wckUI =1Twck
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 128,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 128,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 128 - 0,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 128 + 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 128 - 30,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 128 - 20,
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 128,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 128,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 256,
		//.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 64 + 0, //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 64 + 0, //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 64 + 0, //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 64 + 0, //pxp cs
	},
#endif
#if LPDDR4_SKT
#define  CACLU_CLK_LP4   2112 //1584 //1792//600 //1200 //(1900)// (1440)//(1008)
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default_lp4 = {
	{
		.cfg_board_common_setting.timming_magic = 0,
		.cfg_board_common_setting.timming_max_valid_configs = 1,
		.cfg_board_common_setting.timming_struct_version = 9196,
		.cfg_board_common_setting.timming_struct_org_size =
			sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_real_size = 0,
#if ENABLE_DDR_WINDOW_FAST_BOOT
		.cfg_board_common_setting.fast_boot = {
			0x1, 0, 0, 0xc6
		},
#else
		.cfg_board_common_setting.fast_boot = {
			0x0, 0, 0, 0
		},
#endif
		.cfg_board_common_setting.ddr_func = DDR_FUNC_CONFIG_DFE_FUNCTION,
		.cfg_board_common_setting.board_id = CONFIG_BOARD_ID_MASK,
		.cfg_board_common_setting.DramType = CONFIG_DDR_TYPE_LPDDR4,
		.cfg_board_common_setting.enable_lpddr4x_mode = 1,
		//0  force lp4x   1 force lp4
		//2 auto 4x use nn 4 use pn drivere
		//3 auto + force 4 4x both use nn driver
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_32BIT_RANK0_CH0,
		//.cfg_board_common_setting.dram_cs0_base_add = 0,
		//.cfg_board_common_setting.dram_cs1_base_add = 0,
		//.cfg_board_common_setting.dram_ch0_size_MB =
		//	(DRAM_SIZE_ID_512MBX4 << CONFIG_CS0_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX4 << CONFIG_CS0_BYTE_23_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_23_SIZE_512_ID_OFFSET),
		//.cfg_board_common_setting.dram_ch1_size_MB =
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS0_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS0_BYTE_23_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_01_SIZE_512_ID_OFFSET) +
		//	(DRAM_SIZE_ID_512MBX0 << CONFIG_CS1_BYTE_23_SIZE_512_ID_OFFSET),
		.cfg_board_common_setting.dram_ch0_size_MB = 0xffff,

		.cfg_board_common_setting.DisabledDbyte[0] = 0x00,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0x00,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		//.cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.pll_ssc_mode =
			(1 << 20) | (1 << 8) | (2 << 4) | 0,
		//center_ssc_1000ppm,//SSC_DISABLE,(1 << 20) | (0 << 8) | (2 << 4) | 0,
		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_LPDDR4_16BIT_T3X,
		.cfg_board_SI_setting_ps.DRAMFreq = CACLU_CLK_LP4,
		.cfg_board_SI_setting_ps.training_SequenceCtrl = 0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_W_CS0_ODT0,
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n = 40,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p = 0,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 40, //60,
		.cfg_board_SI_setting_ps.dram_data_drv_ohm = 40,
		.cfg_board_SI_setting_ps.dram_data_odt_ohm = 60,
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm = 0,
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm = 240,//240,//120,
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm =
			DDR_DRAM_LPDDR4_ODT_40_OHM,
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range =
			1,//DDR_DRAM_LPDDR4_OUTPUT_1_3_VDDQ,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,
		.cfg_board_SI_setting_ps.vref_ac_permil = 420,//420,
		.cfg_board_SI_setting_ps.vref_soc_data_permil = 0,
		.cfg_board_SI_setting_ps.vref_dram_data_permil = 0,
		.cfg_board_SI_setting_ps.max_core_timmming_frequency = 0,
		.cfg_board_common_setting.dbi_enable = 0,      // 0,0x00000041
		.cfg_board_common_setting.ddr_rfc_type = 0,//DDR_RFC_TYPE_LPDDR4_8Gbx1, // 13,
		.cfg_board_common_setting.pll_ssc_mode = 0x00000000,    // 0,0x00000044
		.cfg_board_common_setting.ac_pinmux = {
			0,
			7,
			4,
			6,
			3,
			5,
			1,
			2,
			0,
			0,
			0,
			0,
			1,
			0,
			2,
			6,
			5,
			4,
			3,
			7,
			0,
			0,
			0,
			0,
		},
#ifdef BOARD_USE_S905D5
		.cfg_board_common_setting.ac_pinmux = {
			1,
			3,
			0,
			6,
			7,
			2,
			4,
			5,
			0,
			0,
			0,
			0,
			1,
			0,
			7,
			6,
			3,
			4,
			5,
			2,
			0,
			0,
			0,
			0,
		},
#endif
		.cfg_board_common_setting.ddr_dq_remap = {
			0, 0 , 0,
		},
		//.cfg_ddr_training_delay_ps.rx_offset[0] = (1 << 7) | 0x10,
		//.cfg_ddr_training_delay_ps.tx_offset[0] = (1 << 7) | 0x8,
		.cfg_ddr_training_delay_ps.dac_offset[0] = (1 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[1] = (0 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[2] = (1 << 7) | 0x1,//1step 2mv
		.cfg_ddr_training_delay_ps.dac_offset[3] = (0 << 7) | 0x1,//1step 2mv
		//.cfg_ddr_training_delay_ps.reserve_para[0] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[1] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[2] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[3] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[4] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[5] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[6] = (0 << 7) | 0x8,     //write dqs
		//.cfg_ddr_training_delay_ps.reserve_para[7] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 0] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 1] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 2] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 3] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 4] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 5] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 6] = (1 << 7) | 0x4,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 7] = (1 << 7) | 0x4,//read dqs
		#define ac_offset 0
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 128 + ac_offset,     //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 + ac_offset,     //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 128 + ac_offset,    //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 128 + ac_offset,    //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 128 + ac_offset, // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 128 + ac_offset, // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 128 + ac_offset,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 + ac_offset,

	},
#endif
};
