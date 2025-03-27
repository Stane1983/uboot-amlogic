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

#define LPDDR4_SKT 0
#define DDR4_SKT 1
#define DDR3_SKT 1
#define CONFIG_BOARD_TIMMING

ddr_set_ps0_only_t __ddr_setting[] __attribute__ ((section(".ddr_param"))) = {
//{{0,}},
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default[] = {
#if LPDDR4_SKT
#ifdef CONFIG_DDR_PXP_SUPPORT
#define  CACLU_CLK_LP4   1584 //for pxp since force clk ==1600, need take care pxp pll od is fix
#else
#define  CACLU_CLK_LP4   1584 //1792//600 //1200 //(1900)// (1440)//(1008)
#endif
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default_lp4 = {
	{
		.cfg_board_common_setting.timming_magic = 0,
		.cfg_board_common_setting.timming_max_valid_configs = 1,
			//sizeof(ddr_set_t_default) / sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_version = 0,
		.cfg_board_common_setting.timming_struct_org_size =
			sizeof(ddr_set_ps0_only_t),
		.cfg_board_common_setting.timming_struct_real_size = 0,
		.cfg_board_common_setting.fast_boot = {
			0x0, 0, 0, 0
		},
		.cfg_board_common_setting.ddr_func = DDR_FUNC_CONFIG_DFE_FUNCTION |
		DDR_FUNC_CONFIG_ENABLE_PZQ_DET_DRAM_TYPE_RETURN,
		.cfg_board_common_setting.board_id = CONFIG_BOARD_ID_MASK,
		.cfg_board_common_setting.DramType = CONFIG_DDR_TYPE_LPDDR4,
		.cfg_board_common_setting.enable_lpddr4x_mode = 0,
		//0  force lp4x   1 force lp4
		//2 auto 4x use nn 4 use pn drivere
		//3 auto + force 4 4x both use nn driver
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_32BIT_RANK0_CH0,
		//.cfg_board_common_setting.dram_cs0_base_add = 0,
		//.cfg_board_common_setting.dram_cs1_base_add = 0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.dram_ch0_size_MB = 0xff,
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0xf0,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		//.cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.dbi_enable = 0,

		//CNTL3 bit20 = 1 set this bit to 1 when use spread spectrum,
		//CNTL0 bit11=0 disable SSC,cfg_board_common_setting.pll_ssc_mode = 0,
		//center_ssc_1000ppm,(1 << 16) | (2 << 4) | (0 << 2)
		//CNTL0 bit11=1 enable SSC,

		//CNTL2
		//bit[19:16],1-12, 1-x500ppm, 2-x1000ppm, 3-x1500ppm
		//bit[7:4], 1-10,ssc value = bit[7:4] * bit[19:16];
		//bit[3:2], 0-center_ssc, 1-up_ssc, 2-down_ssc
		//center_ssc_1000ppm,
		.cfg_board_common_setting.pll_ssc_mode =
			(1 << 16) | (2 << 4) | (0 << 2),
		//.cfg_board_common_setting.pll_ssc_mode = 0,

		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
		//.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_LPDDR4_16BIT_T3X,
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_LPDDR4_16BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
		// .cfg_board_common_setting.ac_pinmux = {
		//2, 3, 1, 0, 5, 4, 0, 0, 0, 0, 1, 3, 5, 2, 4, 0 },
		.cfg_board_common_setting.ddr_dqs_swap = 0,
		//.cfg_board_common_setting.ddr_dq_remap = { 27,
		//31,
		//29,
		//35,
		//24,
		//28,
		//26,
		//30,
		//25,
		//22,
		//23,
		//21,
		//34,
		//20,
		//17,
		//16,
		//18,
		//19,
		//13,
		//9,
		//11,
		//33,
		//10,
		//8,
		//14,
		//12,
		//15,
		//5,
		//4,
		//1,
		//32,
		//0,
		//3,
		//7,
		//2,
		//6, },
		.cfg_board_common_setting.ddr_vddee_setting = {
			0
		},
		.cfg_board_SI_setting_ps.DRAMFreq = CACLU_CLK_LP4,
		// .cfg_ddr_training_delay_ps.DRAMFreq = 600,// 2112,//
		//.cfg_ddr_training_delay_ps.PllBypassEn = 0,
		.cfg_board_SI_setting_ps.training_SequenceCtrl = 0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_W_CS0_ODT0,
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n = 40,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p = 0,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 40, //60,
		#ifdef CONFIG_PXP_TIMMING
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 0, //60,
		#endif
		.cfg_board_SI_setting_ps.dram_data_drv_ohm = 40,
		.cfg_board_SI_setting_ps.dram_data_odt_ohm = 60,
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm = 0,
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm = 120,//240,//120,
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm =
			DDR_DRAM_LPDDR4_ODT_40_OHM,
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range =
			1,//DDR_DRAM_LPDDR4_OUTPUT_1_3_VDDQ,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,
		.cfg_board_SI_setting_ps.vref_ac_permil = 375,//420,
		.cfg_board_SI_setting_ps.vref_soc_data_permil = 0,
		.cfg_board_SI_setting_ps.vref_dram_data_permil = 0,
		.cfg_board_SI_setting_ps.max_core_timmming_frequency = 0,

// 1 //real chip stk lp4
		.cfg_board_common_setting.ddr_rfc_type = DDR_RFC_TYPE_LPDDR4_8Gbx1, // 13,
//.cfg_board_common_setting.max_core_timmming_frequency=0x00000e10,// 3600,0x0000006a

//.cfg_board_common_setting.lpddr4_x8_mode=0x00000000,// 0,0x00000087
//.cfg_board_common_setting.tdqs2dq=0x00000000,// 0,0x0000008a
//.cfg_board_common_setting.dfe_offset_value=0x00000000,// 0,0x0000008e
//.cfg_board_common_setting.training_offset=0x00000000,// 0,0x0000008f

#ifdef CONFIG_PXP_TIMMING
//pxp
#define TDQS2DQ   -128  //((410 * 128 * CACLU_CLK_LP4 * 2) / 1000000)       //
//#define TDQSCK   64
#define CLK_DELAY 0     // (64)
//#define BOARD_DQS_DELAY   64
#define TDQSCK_ADD_DQS   64
//128   //clk should use 64 steps
#define PHY_TDQS2DQ  0
#define DQ_PXP_OFFSET   -1
#endif
#ifdef CONFIG_RTL_TIMMING
//rtl
#define TDQS2DQ  ((450 * 128 * CACLU_CLK_LP4) / 1000000)  //
//#define TDQSCK   128 //
//#define BOARD_DQS_DELAY   64+32
#define TDQSCK_ADD_DQS   ((3000 * 128 * CACLU_CLK_LP4) / 1000000)   //clk should use 64 steps
#define CLK_DELAY  (0)
#define PHY_TDQS2DQ  ((200 * 128 * CACLU_CLK_LP4) / 1000000)
#define DQ_PXP_OFFSET   0
#endif
#ifdef CONFIG_BOARD_TIMMING                                     //skt lp4 board
#define TDQS2DQ   ((410 * 128 * CACLU_CLK_LP4 * 2) / 1000000)       //
//#define TDQSCK   64
//#define TDQSCK_ADD_DQS   ((3080 * 128 * CACLU_CLK_LP4) / 1000000) //clk should use 64 steps
#define CLK_DELAY 0                                             // (64)
//#define BOARD_DQS_DELAY   64
#define TDQSCK_ADD_DQS   64
//128   //clk should use 64 steps
#define PHY_TDQS2DQ  0
#define DQ_PXP_OFFSET   0
#endif

#ifdef CONFIG_PXP_TIMMING
		.cfg_board_common_setting.ac_pinmux = {
			0,	1,	2,	3,	4,	5,	6,	7,	0,
			0,	0,	8,	9,	28,	29,
			10,	11,	12,	13,	14,	15,	16,	17,	0,
			0,	0,	18,	19,	26,	27,
		},

		.cfg_board_common_setting.ddr_dq_remap = {
			0,
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			32,
			8,
			9,
			10,
			11,
			12,
			13,
			14,
			15,
			33,
			16,
			17,
			18,
			19,
			20,
			21,
			22,
			23,
			34,
			24,
			25,
			26,
			27,
			28,
			29,
			30,
			31,
			35,
		},
#if PXP_USE_REAL_PINMUX
		.cfg_board_common_setting.ac_pinmux = {
			6,	2,	0,	3,	4,	5,	0,	1,	0,
			0,	7,	8,	9,	28,	29,
			15,	0,	13,	0,	17,	16,	11,	0,	10,
			12,	14,	18,	19,	26,	27,
		},
		.cfg_board_common_setting.ddr_dq_remap = {
			14,
			13,
			12,
			9,
			10,
			15,
			8,
			33,
			11,
			4,
			5,
			2,
			3,
			32,
			6,
			7,
			1,
			0,
			18,
			19,
			22,
			16,
			34,
			17,
			21,
			20,
			23,
			27,
			35,
			25,
			31,
			29,
			28,
			26,
			30,
			24,

		},
#endif
#else
		.cfg_board_common_setting.ac_pinmux = {
			1,  0,	2,  3,	4,  5,	8,  9,	29,
			28, 0,	6,  0,	7,  0,
			14, 15, 13, 12, 11, 10, 18, 19, 26,
			27, 17, 16, 0,	0,  0,
		},
		.cfg_board_common_setting.ac_pinmux = {
			6,	2,	0,	3,	4,	5,	0,	1,	0,
			0,	7,	8,	9,	28,	29,
			15,	0,	13,	0,	17,	16,	11,	0,	10,
			12,	14,	18,	19,	26,	27,
		},
		.cfg_board_common_setting.ddr_dq_remap = {
			14,
			13,
			12,
			9,
			10,
			15,
			8,
			33,
			11,
			4,
			5,
			2,
			3,
			32,
			6,
			7,
			1,
			0,
			18,
			19,
			22,
			16,
			34,
			17,
			21,
			20,
			23,
			27,
			35,
			25,
			31,
			29,
			28,
			26,
			30,
			24,

		},
		.cfg_board_common_setting.ddr_dq_remap = {
			7,	6,	4,	3,	5,	0,	1,	2,	32,
			9,	14,	15,	11,	33,	12,	13,	8,	10,
			29,	25,	30,	27,	35,	26,	24,	31,	28,
			22,	34,	16,	18,	21,	20,	23,	19,	17,
		},

#endif
		.cfg_ddr_training_delay_ps.rx_offset[0] = (1 << 7) | 0x10,
		.cfg_ddr_training_delay_ps.tx_offset[0] = (1 << 7) | 0x8,
		//.cfg_ddr_training_delay_ps.dac_offset[0] = 0,//(1 << 7) | 0x10,
		//.cfg_ddr_training_delay_ps.dac_offset[1] = 0,//(0 << 7) | 0x10,
		.cfg_ddr_training_delay_ps.dac_offset[0] = (1 << 7) | 0x3,
		.cfg_ddr_training_delay_ps.dac_offset[1] = (0 << 7) | 0x3,
#ifdef CONFIG_PXP_TIMMING
#else
		.cfg_ddr_training_delay_ps.reserve_para[0] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[1] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[2] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[3] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[4] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[5] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[6] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[7] = (0 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 0] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 1] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 2] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 3] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 4] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 5] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 6] = (1 << 7) | 0x20,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 7] = (1 << 7) | 0x20,//read dqs
#endif
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 256,     //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 256,     //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 256,    //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256,    //clk
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 256,
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 256,
		#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256 + 0, //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 256 + 0, //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + 128, //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + 128, //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256,//cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256,//cke
		#else
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256 + 40, // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 256 + 20, // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 256 + 30,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 256 + 50,

		#endif
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[64] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[1] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[2] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[3] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[4] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[5] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[6] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[7] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[8] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[9] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[10] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[11] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[12] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[13] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[14] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[15] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[16] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[17] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[18] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[19] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[20] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[21] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[22] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[23] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[24] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[25] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[26] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[27] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[28] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[29] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[30] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[31] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[32] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[33] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[34] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[35] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[36] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[37] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[38] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[39] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[40] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[41] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[42] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[43] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[44] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[45] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[46] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[47] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[48] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[49] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[50] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[51] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[52] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[53] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[54] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[55] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[56] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[57] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[58] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[59] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[60] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[61] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[62] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[63] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[64] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[65] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[66] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[67] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[68] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[69] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[70] = 64 + DQ_PXP_OFFSET,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[71] = 64 + DQ_PXP_OFFSET,
#ifdef CONFIG_PXP_TIMMING
		//clk_delay+128+wl_offset*128
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x00000200,
		.cfg_ddr_training_delay_ps.wdq_delay[0] = 384 - 64 + 64 - 128 + TDQS2DQ,
		//write dqs+64+tdqs2dq
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 384 - 64 + 64 - 128 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 384 - 64 + 64 - 128 + TDQS2DQ,
#else
		//pcb write dqs length > clk length 0.5UI, ideal write dqs =
		//clk_delay(==0)+128+wl_offset*128
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 128,//384 - 128 - 64,
		//clk_delay+128+wl_offset*128
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 128,//384 - 128 - 64,
		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x00000200,
		.cfg_ddr_training_delay_ps.wdq_delay[0] = 192 - 64 + 64 + TDQS2DQ,
		//write dqs+64+tdqs2dq
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 192 - 64 + 64 + TDQS2DQ,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 192 - 64 + 64 + TDQS2DQ,
#endif
#ifdef CONFIG_PXP_TIMMING
		//rl-3
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 512,
#else
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 512,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 512,
#endif

//1584 real board parameter
#if 0
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0x43,//67,0x118
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 0x5f,//95,0x11a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 0x4c,//76,0x11c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 0x70,//112,0x11e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 0x42,//66,0x120
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 0x41,//65,0x122
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 0x62,//98,0x124
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 0x46,//70,0x126
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 0x51,//81,0x128
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 0x51,//81,0x12a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 0x63,//99,0x12c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 0x4b,//75,0x12e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 0x62,//98,0x130
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 0x52,//82,0x132
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 0x3a,//58,0x134
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 0x50,//80,0x136
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 0x4a,//74,0x138
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 0x61,//97,0x13a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 0x4e,//78,0x13c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 0x64,//100,0x13e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 0x46,//70,0x140
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 0x60,//96,0x142
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 0x53,//83,0x144
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 0x44,//68,0x146
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 0x58,//88,0x148
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 0x48,//72,0x14a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 0x62,//98,0x14c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 0x4d,//77,0x14e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 0x58,//88,0x150
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 0x57,//87,0x152
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 0x71,//113,0x154
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 0x49,//73,0x156
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 0x45,//69,0x158
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 0x63,//99,0x15a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 0x56,//86,0x15c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 0x68,//104,0x15e
#endif
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 128 - 32,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 128 - 32,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x0000000,
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
		.cfg_ddr_training_delay_ps.dram_vref[0] = 0x00000000,
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

#if DDR4_SKT
//s7 signoff with 3200MBPS
#define  CACLU_CLK_D4   1584// 600 //1200 //1792//600 //1200 //(1900)// (1440)//(1008)
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default_ddr4 = {
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
		.cfg_board_common_setting.ddr_func =
		DDR_FUNC_CONFIG_AUTO_DET_DQ_PINMUX_FUNCTION,
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
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0xfc,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		//.cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.dbi_enable = DDR_WRITE_READ_DBI_DISABLE,

		//CNTL3 bit20 = 1 set this bit to 1 when use spread spectrum,
		//CNTL0 bit11=0 disable SSC,cfg_board_common_setting.pll_ssc_mode = 0,
		//center_ssc_1000ppm,(1 << 16) | (2 << 4) | (0 << 2)
		//CNTL0 bit11=1 enable SSC,

		//CNTL2
		//bit[19:16],1-12, 1-x500ppm, 2-x1000ppm, 3-x1500ppm
		//bit[7:4], 1-10,ssc value = bit[7:4] * bit[19:16];
		//bit[3:2], 0-center_ssc, 1-up_ssc, 2-down_ssc
		//center_ssc_1000ppm,
		//.cfg_board_common_setting.pll_ssc_mode =
		//	(1 << 16) | (2 << 4) | (0 << 2),
		.cfg_board_common_setting.pll_ssc_mode = 0,

		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
		.cfg_board_common_setting.ddr_dmc_remap =
		//DDR_DMC_REMAP_DDR4_16BIT_S1A,
		//DDR_DMC_REMAP_DDR4_16BIT_2,
		DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
#ifdef ENABLE_DDR4_16BIT
		//ddr4 32bit
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_16BIT_CH0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.DisabledDbyte[0] = 0xfc,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_DDR4_16BIT_S1A,
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
		.cfg_board_common_setting.ddr_rfc_type = DDR_RFC_TYPE_DDR4_2Gbx4,//DDR_RFC_TYPE_DDR4_2Gbx8,
//.cfg_board_common_setting.max_core_timmming_frequency=0x00000e10,// 3600,0x0000006a

//.cfg_board_common_setting.lpddr4_x8_mode=0x00000000,// 0,0x00000087
//.cfg_board_common_setting.tdqs2dq=0x00000000,// 0,0x0000008a
//.cfg_board_common_setting.dfe_offset_value=0x00000000,// 0,0x0000008e
//.cfg_board_common_setting.training_offset=0x00000000,// 0,0x0000008f

#ifdef CONFIG_PXP_TIMMING
//pxp
#define TDQS2DQ_D4  (0 + ((0 * 128 * CACLU_CLK_D4 * 2) / 1000000))    //
//#define TDQSCK   64
#define CLK_DELAY_D4 0                                             // (64)
//#define BOARD_DQS_DELAY   64
#define TDQSCK_ADD_DQS_D4   64
//128   //clk should use 64 steps
#define PHY_TDQS2DQ  0
#define WL0  0
//((480 * 128 * CACLU_CLK_D4 * 2) / 1000000)
#endif

#ifdef CONFIG_RTL_TIMMING
//rtl
#define TDQS2DQ_D4  ((0 * 128 * CACLU_CLK_D4) / 1000000)  //
//#define TDQSCK   128 //
//#define BOARD_DQS_DELAY   64+32
#define TDQSCK_ADD_DQS_D4   ((0 * 128 * CACLU_CLK_D4) / 1000000)   //clk should use 64 steps
#define CLK_DELAY_D4  (0)
#define PHY_TDQS2DQ_D4  ((200 * 128 * CACLU_CLK_D4) / 1000000)
#define WL0  0 //((480 * 128 * CACLU_CLK_D4 * 2) / 1000000)
#endif

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
//#ifdef CONFIG_PXP_TIMMING
//		.cfg_board_common_setting.ac_pinmux = {
//			0,	1,	2,	3,	4,
//5,	24,	25,	28,	29,
//		6,	7,	8,	9,	10,	11,
//12,	13,	14,	15,	16,
//		17,	18,	19,	20,	21,	22,	23,	26,	27,
//		},
//#else
#ifdef CONFIG_PXP_TIMMING
//	.cfg_board_common_setting.ac_pinmux = {
//	27,	11,	26,	2,	23,	0,	9,	14,	17,	7,
//5,	19,	22,	15,	12,	16,	8,	1,	10,	13,	4,
//20,	20,	3,	21,	6,	24,	25,	29,	28,
//},
//S7 pxp cs pinmux change
		.cfg_board_common_setting.ac_pinmux = {
		27,	11,	26,	2,	23,	0,	9,	14,	17,	7,
		5,	19,	22,	15,	12,	16,	8,	1,	10,	13,	4,
		17,	20,	3,	21,	6,	24,	25,	28,	29,
		},
		.cfg_board_common_setting.ddr_dq_remap = {
			0,
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			32,
			8,
			9,
			10,
			11,
			12,
			13,
			14,
			15,
			33,
			16,
			17,
			18,
			19,
			20,
			21,
			22,
			23,
			34,
			24,
			25,
			26,
			27,
			28,
			29,
			30,
			31,
			35,
		},
#if PXP_USE_REAL_PINMUX
		//S7 realpinmux
		//T6D pinmux
		.cfg_board_common_setting.ac_pinmux = {
		4,	6,	5,	0,	1,	8,	9,	7,	13,	2,
		11,	10,	3,	15,	14,	26,	27,	22,	23,	20,	21,
		19,	16,	18,	12,	17,	24,	25,	28,	29,
		},
		.cfg_board_common_setting.ddr_dq_remap = {
			34,
			16,
			17,
			19,
			18,
			21,
			20,
			22,
			23,
			32,
			0,
			1,
			3,
			2,
			5,
			4,
			7,
			6,
			9,
			8,
			11,
			10,
			13,
			12,
			15,
			14,
			33,
			24,
			25,
			26,
			27,
			28,
			30,
			29,
			31,
			35,
		},
#endif
#else

		//.cfg_board_common_setting.ac_pinmux = {
		//	3,  6,	5,  0,	1,  8,	9,  7,	13, 2,
		//	11, 14, 4,  15, 10, 26, 27, 20, 23, 22, 21,
		//	19, 16, 18, 12, 17, 24, 25, 28, 29,
		//},
		.cfg_board_common_setting.ac_pinmux = {
			1,  16, 7,  26, 8,  13, 11, 3,	22, 9,
			2,  23, 5,  27, 21, 20, 19, 4,	10, 12,0,
			15, 17, 18, 6,	14, 24, 25, 28, 29,
		},

//#ifdef CONFIG_BOARD_TIMMING
		.cfg_board_common_setting.ddr_dq_remap = {
			12,
			33,
			10,
			8,
			14,
			9,
			11,
			13,
			15,
			0,
			2,
			32,
			4,
			1,
			5,
			6,
			3,
			7,
			21,
			22,
			23,
			18,
			17,
			16,
			34,
			19,
			20,
			24,
			25,
			35,
			27,
			28,
			26,
			31,
			30,
			29,
		},
		.cfg_board_common_setting.ac_pinmux = {//txhd2 non-sip
		0,	11,	6,	2,	14,	8,	4,	17,	21,	16,
		23,	22,	26,	12,	9,	18,	20,	5,	7,	27,
		1,	15,	19,	13,	10,	3,	24,	25,	28,	29,
		},

		//.cfg_board_common_setting.ac_pinmux = {
		//	26, 11, 27, 2,	22, 0,	9,  14, 17, 7,
		//	5,  19, 23, 15, 12, 16, 8,  1,	10, 13,4,
		//	18, 20, 3,  21, 6,  25, 24, 29, 28,
		//},
		.cfg_board_common_setting.ddr_dq_remap = {
			6,
			32,
			1,
			2,
			0,
			4,
			5,
			7,
			3,
			33,
			8,
			13,
			10,
			15,
			9,
			12,
			11,
			14,
			16,
			17,
			18,
			19,
			20,
			21,
			22,
			23,
			34,
			24,
			25,
			26,
			27,
			28,
			29,
			30,
			31,
			35,
	},

		.cfg_board_common_setting.ac_pinmux = {
		4,	6,	5,	0,	1,	8,	9,	7,	13,	2,
		11,	10,	3,	15,	14,	26,	27,	22,	23,	20,	21,
		19,	16,	18,	12,	17,	24,	25,	28,	29,
		},

		.cfg_board_common_setting.ddr_dq_remap = {
			0 , 0 , 0,
		//	12,	10,	15,	13,	9,	8,	11,	14,	33,
		//	6,	4,	0,	2,	32,	1,	3,	5,	7,
		//	25,	29,	26,	27,	35,	24,	28,	30,	31,
		//	22,	34,	18,	19,	17,	21,	20,	23,	16,
		},

#endif
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
#endif
		//#define  AC_OFFSET  (-128) //> 1650 M use -128  or use 0
		#define  AC_OFFSET  (128)
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 256 + AC_OFFSET,
		//cke 128 only 1UI margin
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 256 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 256 + AC_OFFSET,
#ifdef CONFIG_PXP_TIMMING                                                       //t3x pxp
#ifndef PXP_USE_REAL_PINMUX
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 128 + AC_OFFSET,    //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 128 + AC_OFFSET,     //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFFSET,  //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFFSET,  //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFFSET,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFFSET,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 128 + AC_OFFSET,     //odt0
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 128 + AC_OFFSET,         //odt1
#else
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 128 + AC_OFFSET,       // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 128 + AC_OFFSET,      // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFFSET,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFFSET,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 128 + AC_OFFSET,        //odt0
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 128 + AC_OFFSET,       //odt1
#endif
#else
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 128 + AC_OFFSET,       // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 128 + AC_OFFSET,      // cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFFSET,    //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFFSET,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFFSET,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 128 + AC_OFFSET,        //odt0
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 128 + AC_OFFSET,       //odt1
       //odt1
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


#ifdef CONFIG_PXP_TIMMING  //clk p n swap
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = AC_OFFSET + WL0 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = AC_OFFSET + WL0 + 0,
#else
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 0 + AC_OFFSET + WL0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 0 + AC_OFFSET + WL0,

		//.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 225,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 225,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 208,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 208,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 128 + WL0 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 128 + WL0 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 128 + WL0 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 128 + WL0 + 0,
#endif
		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x00000200,
#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.wdq_delay[0] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[1] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = AC_OFFSET + TDQS2DQ_D4 + WL0,
#else
		.cfg_ddr_training_delay_ps.wdq_delay[0] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 192 + AC_OFFSET + TDQS2DQ_D4 + WL0,
#endif
#ifdef CONFIG_PXP_TIMMING
		//AC_OFFSET - 256 ,old_dmc_readen_latency_offset = 2
		//AC_OFFSET + 0 ,old_dmc_readen_latency_offset = 3
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = AC_OFFSET,
#else
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 128 * 3 + AC_OFFSET,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 128 * 3 + AC_OFFSET,
#endif
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 128,
#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 128,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x0000001a,
		//0 for auto training
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[1] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[2] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[3] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[4] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[5] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[6] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[7] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[8] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[9] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[10] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[11] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[12] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[13] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[14] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[15] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[16] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[17] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[18] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[19] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[20] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[21] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[22] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[23] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[24] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[25] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[26] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[27] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[28] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[29] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[30] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[31] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[32] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[33] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[34] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[35] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[36] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[37] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[38] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[39] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[40] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[41] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[42] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[43] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[44] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[45] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[46] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[47] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[48] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[49] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[50] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[51] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[52] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[53] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[54] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[55] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[56] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[57] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[58] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[59] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[60] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[61] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[62] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[63] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[65] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[66] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[67] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[68] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[69] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[70] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[71] = 0,
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 65,
#else
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x000000,
		//0 for auto training

#endif
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

#if DDR3_SKT
#ifdef ENABLE_CP_DDR3_TIMMING
#define  CACLU_CLK_D3	912
#else
#define  CACLU_CLK_D3	1056 //636 //1792//600 //1200 //(1900)// (1440)//(1008)
#endif
//ddr_set_ps0_only_t __attribute__ ((aligned(8))) ddr_set_t_default_ddr3 = {
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
		//DDR_FUNC_CONFIG_ENABLE_PZQ_DET_DRAM_TYPE_RETURN,
		.cfg_board_common_setting.board_id = CONFIG_BOARD_ID_MASK,
		.cfg_board_common_setting.DramType = CONFIG_DDR_TYPE_DDR3,
		.cfg_board_common_setting.enable_lpddr4x_mode = 0,
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_32BIT_RANK0_CH0,
		//.cfg_board_common_setting.dram_cs0_base_add = 0,
		//.cfg_board_common_setting.dram_cs1_base_add = 0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX1 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.DisabledDbyte[1] = 0xf0,
		//bit 0 -3 ch1 cs0 ,bit 4-7 ch1
		.cfg_board_common_setting.dram_x4x8x16_mode = CONFIG_DRAM_MODE_X16,
		.cfg_board_common_setting.Is2Ttiming = CONFIG_USE_DDR_2T_MODE,
		.cfg_board_common_setting.log_level = 0xff,
		//.cfg_board_common_setting.log_level = 4,
		//4,//LOG_LEVEL_BASIC,
		.cfg_board_common_setting.dbi_enable = DDR_WRITE_READ_DBI_DISABLE,

		//CNTL3 bit20 = 1 set this bit to 1 when use spread spectrum,
		//CNTL0 bit11=0 disable SSC,cfg_board_common_setting.pll_ssc_mode = 0,
		//center_ssc_1000ppm,(1 << 16) | (2 << 4) | (0 << 2)
		//CNTL0 bit11=1 enable SSC,

		//CNTL2
		//bit[19:16],1-12, 1-x500ppm, 2-x1000ppm, 3-x1500ppm
		//bit[7:4], 1-10,ssc value = bit[7:4] * bit[19:16];
		//bit[3:2], 0-center_ssc, 1-up_ssc, 2-down_ssc
		//center_ssc_1000ppm,
		//.cfg_board_common_setting.pll_ssc_mode =
		//	(1 << 16) | (2 << 4) | (0 << 2),
		.cfg_board_common_setting.pll_ssc_mode = 0,

		.cfg_board_common_setting.org_tdqs2dq = 0,
		.cfg_board_common_setting.reserve1_test = {
			0
		},
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_DDR3_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
		//DDR_DMC_REMAP_DDR4_32BIT,
#ifdef ENABLE_DDR3_16BIT
		//ddr3 32bit
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_16BIT_CH0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_DDR3_32BIT,
#endif
#if 0 //def ENABLE_DDR3_32BIT
		//ddr3 32bit
		.cfg_board_common_setting.dram_rank_config = CONFIG_DDR0_32BIT_RANK0_CH0,
		.cfg_board_common_setting.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,
		//bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.cfg_board_common_setting.ddr_dmc_remap = DDR_DMC_REMAP_DDR3_32BIT,
#endif
		.cfg_board_common_setting.ddr_dqs_swap = 0,
		// .cfg_board_common_setting.ddr_dq_remap = {
		//3, 2, 6, 5, 7, 4, 0, 1, 10, 8, 14, 13, 15,
		//12, 9, 11, 23, 21, 17, 18, 16, 19, 20,
		//22, 31, 28, 24, 25, 27, 26, 30, 29 },
		.cfg_board_common_setting.ddr_vddee_setting = {
			0
		},
		.cfg_board_SI_setting_ps.DRAMFreq = CACLU_CLK_D3,
		// .cfg_ddr_training_delay_ps.DRAMFreq = 600,
		// 2112,//1176,cfg_ddr_training_delay_ps
		//.cfg_ddr_training_delay_ps.PllBypassEn = 0,
		.cfg_board_SI_setting_ps.training_SequenceCtrl = 0,
		.cfg_board_SI_setting_ps.dfi_odt_config = DDR_DRAM_ODT_W_CS0_ODT0,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,
		.cfg_board_SI_setting_ps.vref_ac_permil = 0,
		.cfg_board_SI_setting_ps.vref_soc_data_permil = 0,
		.cfg_board_SI_setting_ps.vref_dram_data_permil = 0,
		.cfg_board_SI_setting_ps.max_core_timmming_frequency = 0,//1320,//0,

// 1 //real chip stk lp4
		.cfg_board_common_setting.ddr_rfc_type = DDR_RFC_TYPE_DDR4_2Gbx8,
//.cfg_board_common_setting.max_core_timmming_frequency=0x00000e10,// 3600,0x0000006a

//.cfg_board_common_setting.lpddr4_x8_mode=0x00000000,// 0,0x00000087
//.cfg_board_common_setting.tdqs2dq=0x00000000,// 0,0x0000008a
//.cfg_board_common_setting.dfe_offset_value=0x00000000,// 0,0x0000008e
//.cfg_board_common_setting.training_offset=0x00000000,// 0,0x0000008f

#ifdef CONFIG_PXP_TIMMING
//pxp
#define TDQS2DQ_D3  (0 + ((0 * 128 * CACLU_CLK_D3 * 2) / 1000000))	  //
//#define TDQSCK   64
#define CLK_DELAY_D3 0			// (64)
//#define BOARD_DQS_DELAY   64
#define TDQSCK_ADD_DQS_D3   64			 //128   //clk should use 64 steps
#define PHY_TDQS2DQ_D3  0
#define WL0_D3  0 //((480 * 128 * CACLU_CLK_D3 * 2) / 1000000)
#endif
#ifdef CONFIG_RTL_TIMMING
//rtl
#define TDQS2DQ_D3  ((0 * 128 * CACLU_CLK_D3) / 1000000)  //
//#define TDQSCK   128 //
//#define BOARD_DQS_DELAY   64+32
#define TDQSCK_ADD_DQS_D3   ((0 * 128 * CACLU_CLK_D3) / 1000000)   //clk should use 64 steps
#define CLK_DELAY_D3  (0)
#define PHY_TDQS2DQ_D3  ((200 * 128 * CACLU_CLK_D3) / 1000000)
#define WL0_D3  0 //((480 * 128 * CACLU_CLK_D3 * 2) / 1000000)
#endif
#ifdef CONFIG_BOARD_TIMMING //skt lp4 board
#define TDQS2DQ_D3  ((0 * 128 * CACLU_CLK_D3) / 1000000)
//#define BOARD_DQS_DELAY   64+32
#define TDQSCK_ADD_DQS_D3   ((0 * 128 * CACLU_CLK_D3) / 1000000) //clk should use 64 steps
#define CLK_DELAY_D3  (0)
#define PHY_TDQS2DQ_D3  ((0 * 128 * CACLU_CLK_D3) / 1000000)
#define WL0_D3  ((0 * 128 * CACLU_CLK_D3 * 2) / 1000000)
#define WL1  ((0 * 128 * CACLU_CLK_D3 * 2) / 1000000)
//for 1584M 100 STEP 250ps //((480 * 128 * CACLU_CLK_D3 * 2) / 1000000)
#endif
		.cfg_board_SI_setting_ps.clk_drv_ohm = 40,
		.cfg_board_SI_setting_ps.cs_drv_ohm = 40,
		.cfg_board_SI_setting_ps.ac_drv_ohm = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p = 40,
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n = 40,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p = 120,
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n = 120,
		.cfg_board_SI_setting_ps.dram_data_drv_ohm = 40,
		.cfg_board_SI_setting_ps.dram_data_odt_ohm = 120,
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm = 0,
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm = 120,
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm =
			DDR_DRAM_LPDDR4_ODT_40_OHM,
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range =
			DDR_DRAM_LPDDR4_OUTPUT_1_3_VDDQ,
		//.cfg_ddr_training_delay_ps.dfe_offset = 0,

#ifdef CONFIG_PXP_TIMMING
		.cfg_board_common_setting.ac_pinmux = {
			27,	11,	26,	2,	23,	0,	9,	16,	18,	7,
			5,	19,	22,	17,	12,	11,	8,	1,	10,	13,	4,
			19,	20,	3,	21,	6,	24,	25,	28,	29,
			},
		.cfg_board_common_setting.ddr_dq_remap = {
			0,
			1,
			2,
			3,
			4,
			5,
			6,
			7,
			32,
			8,
			9,
			10,
			11,
			12,
			13,
			14,
			15,
			33,
			16,
			17,
			18,
			19,
			20,
			21,
			22,
			23,
			34,
			24,
			25,
			26,
			27,
			28,
			29,
			30,
			31,
			35,
		},

#else
		.cfg_board_common_setting.ac_pinmux = {
			6,	10,	19,	3,	5,	7,	0,	22,	15,	26,
			23,	27,	21,	16,	18,	17,	20,	12,	14,	8,	11,
			4,	1,	2,	13,	9,	24,	25,	28,	29,
			},

		.cfg_board_common_setting.ddr_dq_remap = {
			12,	10,	15,	13,	9,	8,	11,	14,	33,
			6,	4,	0,	2,	32,	1,	3,	5,	7,
			25,	29,	26,	27,	35,	24,	28,	30,	31,
			22,	34,	18,	19,	17,	21,	20,	23,	16,
		},
#ifdef ENABLE_CP_DDR3_TIMMING
		.cfg_board_common_setting.ac_pinmux = {
		11,	3,	20,	19,	0,	2,	9,	23,	26,	26,
		22,	1,	16,	4,	21,	13,	6,	17,	10,	8,	7,
		12,	5,	18,	14,	15,	24,	25,	28,	29,
		},
		.cfg_board_common_setting.ddr_dq_remap = {
		7,
		0,
		4,
		6,
		3,
		5,
		2,
		1,
		32,
		8,
		12,
		14,
		10,
		33,
		13,
		15,
		9,
		11,
		22,
		20,
		34,
		16,
		21,
		19,
		18,
		23,
		17,
		28,
		35,
		27,
		26,
		29,
		25,
		30,
		31,
		24,

		},
#endif
#if ENABLE_RTL_DDR3_PINMUX
//rtl debug
		.cfg_board_common_setting.ac_pinmux = {
		0x1b,	0xb,	0x1a,	2,	0x17,	0,	9,	0x10,	0x12,	0x7,
		5,	14,	22,	17,	12,	0x13,	8,	1,	10,	13,
		4,	21,	20,	3,	15,	6,	24,	25,	29,	28,

		},
		.cfg_board_common_setting.ddr_dq_remap = {
		0,
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		32,
		8,
		9,
		10,
		11,
		12,
		13,
		14,
		15,
		33,
		16,
		17,
		18,
		19,
		20,
		21,
		22,
		23,
		34,
		24,
		25,
		26,
		27,
		28,
		29,
		30,
		31,
		35,
		},
#endif
#endif

#ifdef CONFIG_BOARD_TIMMING
		.cfg_ddr_training_delay_ps.tx_offset[0] = (0 << 7) | 0x0,
		.cfg_ddr_training_delay_ps.rx_offset[0] = (0 << 7) | 0x0,
		.cfg_ddr_training_delay_ps.reserve_para[0] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[1] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[2] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[3] = (1 << 7) | 0x8,     //write dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 0] = (1 << 7) | 0x6,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 1] = (1 << 7) | 0x6,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 2] = (1 << 7) | 0x6,//read dqs
		.cfg_ddr_training_delay_ps.reserve_para[8 + 3] = (1 << 7) | 0x6,//read dqs
		//.cfg_ddr_training_delay_ps.dac_offset[0] = 0,//(1 << 7) | 0x10,
		//.cfg_ddr_training_delay_ps.dac_offset[1] = 0,//(0 << 7) | 0x10,
		//.cfg_ddr_training_delay_ps.dac_offset[0] = (1 << 7) | 0x5,
		//.cfg_ddr_training_delay_ps.dac_offset[1] = (0 << 7) | 0x5,
#endif
		#define  AC_OFF_D3  (128) //for sip should use AC_OFFSET 128,
		//if use ac_offset 0, some chip use coarse 0 bad ,some use coars 1 bad
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 256 + AC_OFF_D3,
		//cke 128 only 1UI margin
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 256 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 256 + AC_OFF_D3,
#ifdef CONFIG_PXP_TIMMING                                           //t3x pxp
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 128 + AC_OFF_D3,//pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 128 + AC_OFF_D3,  //pxp cs
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFF_D3,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFF_D3,     //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 128 + AC_OFF_D3,      //odt0
		//.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 128 + AC_OFF_D3,      //odt1
#else
#ifdef ENABLE_CP_DDR3_TIMMING
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 128 + AC_OFF_D3,     // cs0
		//.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 90 + AC_OFF_D3,    // cs0 + 40
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFF_D3,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFF_D3,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 + AC_OFF_D3,      //odt0
		//.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 128 + AC_OFF_D3,      //odt1
#else
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 128 + AC_OFF_D3,     // cs1
		//.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 90 + AC_OFF_D3,    // cs0 + 40
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 128 + AC_OFF_D3,      //ck
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 128 + AC_OFF_D3,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 128 + AC_OFF_D3,      //cke
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 128 + AC_OFF_D3,      //odt0
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 128 + AC_OFF_D3,      //odt1
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 128 + AC_OFF_D3,      //odt1
#endif
#endif
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[64] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 64,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 64,
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
		//for non-sip skt board

#ifdef CONFIG_PXP_TIMMING  //clk p n swap
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = AC_OFF_D3 + WL0_D3 + 0,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = AC_OFF_D3 + WL0_D3 + 0,
#else
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 128 + AC_OFF_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 128 + AC_OFF_D3 + WL0_D3,

		//.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 225,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 225,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 208,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 208,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 128 + WL0_D3 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 128 + WL0_D3 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 128 + WL0_D3 + 0,
		//.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 128 + WL0_D3 + 0,
#endif
		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x00000200,
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x00000200,
#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.wdq_delay[0] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[1] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
#else
		.cfg_ddr_training_delay_ps.wdq_delay[0] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 192 + AC_OFF_D3 + TDQS2DQ_D3 + WL0_D3,
#endif
#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = AC_OFF_D3 + 128,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = AC_OFF_D3 + 128,
#else
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 128 * 5 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 128 * 5 + AC_OFF_D3,
#ifdef ENABLE_CP_DDR3_TIMMING
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 128 * 6 + 64 + AC_OFF_D3,
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 128 * 6 + 64 + AC_OFF_D3,
#endif
#endif
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 128,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 128,
#ifdef CONFIG_PXP_TIMMING
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x0000001a,
		//0 for auto training
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[1] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[2] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[3] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[4] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[5] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[6] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[7] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[8] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[9] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[10] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[11] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[12] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[13] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[14] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[15] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[16] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[17] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[18] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[19] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[20] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[21] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[22] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[23] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[24] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[25] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[26] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[27] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[28] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[29] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[30] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[31] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[32] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[33] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[34] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[35] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[36] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[37] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[38] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[39] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[40] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[41] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[42] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[43] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[44] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[45] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[46] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[47] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[48] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[49] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[50] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[51] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[52] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[53] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[54] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[55] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[56] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[57] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[58] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[59] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[60] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[61] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[62] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[63] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[65] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[66] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[67] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[68] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[69] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[70] = 0,
		.cfg_ddr_training_delay_ps.read_dq_delay_c[71] = 0,
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 65,
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 65,
#else
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x000000,
		//0 for auto training

#endif
		.cfg_ddr_training_delay_ps.soc_bit_vref0[1] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[2] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[3] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[4] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[5] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[6] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[7] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[8] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[9] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[10] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[11] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[12] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[13] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[14] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[15] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[16] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[17] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[18] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[19] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[20] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[21] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[22] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[23] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[24] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[25] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[26] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[27] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[28] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[29] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[30] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[31] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[32] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[33] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[34] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref0[35] = 0x0000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[0] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[1] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[2] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[3] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[4] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[5] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[6] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[7] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[8] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[9] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[10] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[11] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[12] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[13] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[14] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[15] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[16] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[17] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[18] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[19] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[20] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[21] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[22] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[23] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[24] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[25] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[26] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[27] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[28] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[29] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[30] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[31] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[32] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[33] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[34] = 0x00000021,
		.cfg_ddr_training_delay_ps.soc_bit_vref1[35] = 0x00000021,
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
		.cfg_ddr_training_delay_ps.dram_vref[0] = 0x00000000,
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

		//DDR3 SKIP_TRAINING
#if DDR3_SKIP_TRAINING
		//old fast_boot[0]=0x       0,// 0
		.cfg_board_common_setting.timming_magic=0x0,//0,0x0
		.cfg_board_common_setting.timming_max_valid_configs=0x1,//1,0x4
		.cfg_board_common_setting.timming_struct_version=0x0,//0,0x6
		.cfg_board_common_setting.timming_struct_org_size=0x490,//1168,0x8
		.cfg_board_common_setting.timming_struct_real_size=0x0,//0,0xa
		.cfg_board_common_setting.fast_boot[0] = 0xfd,//253,0xc
		.cfg_board_common_setting.fast_boot[1] = 0x0,//0,0xd
		.cfg_board_common_setting.fast_boot[2] = 0x0,//0,0xe
		.cfg_board_common_setting.fast_boot[3] = 0x0,//0,0xf
		.cfg_board_common_setting.ddr_func=0x0,//0,0x10
		.cfg_board_common_setting.board_id=0xff,//255,0x14
		.cfg_board_common_setting.DramType=0x0,//0,0x15
		.cfg_board_common_setting.dram_rank_config=0x2,//2,0x16
		.cfg_board_common_setting.rsv_char0=0x0,//0,0x17
		.cfg_board_common_setting.DisabledDbyte[0] = 0xf0,//252,0x18
		.cfg_board_common_setting.DisabledDbyte[1] = 0xf0,//252,0x19
		.cfg_board_common_setting.dram_ch0_size_MB=0x1,//1,0x1a
		.cfg_board_common_setting.dram_ch1_size_MB=0x0,//0,0x1c
		.cfg_board_common_setting.dram_x4x8x16_mode=0x0,//0,0x1e
		.cfg_board_common_setting.Is2Ttiming=0x1,//1,0x1f
		.cfg_board_common_setting.log_level=0x4,//4,0x20
		.cfg_board_common_setting.dbi_enable=0x0,//0,0x21
		.cfg_board_common_setting.ddr_rfc_type=0x8,//8,0x22
		.cfg_board_common_setting.enable_lpddr4x_mode=0x0,//0,0x23
		.cfg_board_common_setting.pll_ssc_mode=0x0,//0,0x24
		.cfg_board_common_setting.org_tdqs2dq=0x0,//0,0x28
		.cfg_board_common_setting.reserve1_test[0] = 0x0,//0,0x2a
		.cfg_board_common_setting.reserve1_test[1] = 0x0,//0,0x2b
		.cfg_board_common_setting.ddr_dmc_remap[0] = 0x14941cc5,//345251013,0x2c
		.cfg_board_common_setting.ddr_dmc_remap[1] = 0x2306000b,//587595787,0x30
		.cfg_board_common_setting.ddr_dmc_remap[2] = 0x2f6ad272,//795529842,0x34
		.cfg_board_common_setting.ddr_dmc_remap[3] = 0x3bcdeb38,//1003350840,0x38
		.cfg_board_common_setting.ddr_dmc_remap[4] = 0x7b9be,//506302,0x3c
		.cfg_board_common_setting.ddr_dmc_remap[5] = 0x0,//0,0x40
		.cfg_board_common_setting.lpddr34_ca_remap[0] = 0x0,//0,0x44
		.cfg_board_common_setting.lpddr34_ca_remap[1] = 0x0,//0,0x45
		.cfg_board_common_setting.lpddr34_ca_remap[2] = 0x0,//0,0x46
		.cfg_board_common_setting.lpddr34_ca_remap[3] = 0x0,//0,0x47
		.cfg_board_common_setting.ddr_dq_remap[0] = 0xc,//12,0x48
		.cfg_board_common_setting.ddr_dq_remap[1] = 0xa,//10,0x49
		.cfg_board_common_setting.ddr_dq_remap[2] = 0xf,//15,0x4a
		.cfg_board_common_setting.ddr_dq_remap[3] = 0xd,//13,0x4b
		.cfg_board_common_setting.ddr_dq_remap[4] = 0x9,//9,0x4c
		.cfg_board_common_setting.ddr_dq_remap[5] = 0x8,//8,0x4d
		.cfg_board_common_setting.ddr_dq_remap[6] = 0xb,//11,0x4e
		.cfg_board_common_setting.ddr_dq_remap[7] = 0xe,//14,0x4f
		.cfg_board_common_setting.ddr_dq_remap[8] = 0x21,//33,0x50
		.cfg_board_common_setting.ddr_dq_remap[9] = 0x6,//6,0x51
		.cfg_board_common_setting.ddr_dq_remap[10] = 0x4,//4,0x52
		.cfg_board_common_setting.ddr_dq_remap[11] = 0x0,//0,0x53
		.cfg_board_common_setting.ddr_dq_remap[12] = 0x2,//2,0x54
		.cfg_board_common_setting.ddr_dq_remap[13] = 0x20,//32,0x55
		.cfg_board_common_setting.ddr_dq_remap[14] = 0x1,//1,0x56
		.cfg_board_common_setting.ddr_dq_remap[15] = 0x3,//3,0x57
		.cfg_board_common_setting.ddr_dq_remap[16] = 0x5,//5,0x58
		.cfg_board_common_setting.ddr_dq_remap[17] = 0x7,//7,0x59
		.cfg_board_common_setting.ddr_dq_remap[18] = 0x19,//25,0x5a
		.cfg_board_common_setting.ddr_dq_remap[19] = 0x1d,//29,0x5b
		.cfg_board_common_setting.ddr_dq_remap[20] = 0x1a,//26,0x5c
		.cfg_board_common_setting.ddr_dq_remap[21] = 0x1b,//27,0x5d
		.cfg_board_common_setting.ddr_dq_remap[22] = 0x23,//35,0x5e
		.cfg_board_common_setting.ddr_dq_remap[23] = 0x18,//24,0x5f
		.cfg_board_common_setting.ddr_dq_remap[24] = 0x1c,//28,0x60
		.cfg_board_common_setting.ddr_dq_remap[25] = 0x1e,//30,0x61
		.cfg_board_common_setting.ddr_dq_remap[26] = 0x1f,//31,0x62
		.cfg_board_common_setting.ddr_dq_remap[27] = 0x16,//22,0x63
		.cfg_board_common_setting.ddr_dq_remap[28] = 0x22,//34,0x64
		.cfg_board_common_setting.ddr_dq_remap[29] = 0x12,//18,0x65
		.cfg_board_common_setting.ddr_dq_remap[30] = 0x13,//19,0x66
		.cfg_board_common_setting.ddr_dq_remap[31] = 0x11,//17,0x67
		.cfg_board_common_setting.ddr_dq_remap[32] = 0x15,//21,0x68
		.cfg_board_common_setting.ddr_dq_remap[33] = 0x14,//20,0x69
		.cfg_board_common_setting.ddr_dq_remap[34] = 0x17,//23,0x6a
		.cfg_board_common_setting.ddr_dq_remap[35] = 0x10,//16,0x6b
		.cfg_board_common_setting.ac_pinmux[0] = 0x6,//6,0x6c
		.cfg_board_common_setting.ac_pinmux[1] = 0xa,//10,0x6d
		.cfg_board_common_setting.ac_pinmux[2] = 0x13,//19,0x6e
		.cfg_board_common_setting.ac_pinmux[3] = 0x3,//3,0x6f
		.cfg_board_common_setting.ac_pinmux[4] = 0x5,//5,0x70
		.cfg_board_common_setting.ac_pinmux[5] = 0x7,//7,0x71
		.cfg_board_common_setting.ac_pinmux[6] = 0x0,//0,0x72
		.cfg_board_common_setting.ac_pinmux[7] = 0x16,//22,0x73
		.cfg_board_common_setting.ac_pinmux[8] = 0xf,//15,0x74
		.cfg_board_common_setting.ac_pinmux[9] = 0x1a,//26,0x75
		.cfg_board_common_setting.ac_pinmux[10] = 0x17,//23,0x76
		.cfg_board_common_setting.ac_pinmux[11] = 0x1b,//27,0x77
		.cfg_board_common_setting.ac_pinmux[12] = 0x15,//21,0x78
		.cfg_board_common_setting.ac_pinmux[13] = 0x10,//16,0x79
		.cfg_board_common_setting.ac_pinmux[14] = 0x12,//18,0x7a
		.cfg_board_common_setting.ac_pinmux[15] = 0x11,//17,0x7b
		.cfg_board_common_setting.ac_pinmux[16] = 0x14,//20,0x7c
		.cfg_board_common_setting.ac_pinmux[17] = 0xc,//12,0x7d
		.cfg_board_common_setting.ac_pinmux[18] = 0xe,//14,0x7e
		.cfg_board_common_setting.ac_pinmux[19] = 0x8,//8,0x7f
		.cfg_board_common_setting.ac_pinmux[20] = 0xb,//11,0x80
		.cfg_board_common_setting.ac_pinmux[21] = 0x4,//4,0x81
		.cfg_board_common_setting.ac_pinmux[22] = 0x1,//1,0x82
		.cfg_board_common_setting.ac_pinmux[23] = 0x2,//2,0x83
		.cfg_board_common_setting.ac_pinmux[24] = 0xd,//13,0x84
		.cfg_board_common_setting.ac_pinmux[25] = 0x9,//9,0x85
		.cfg_board_common_setting.ac_pinmux[26] = 0x18,//24,0x86
		.cfg_board_common_setting.ac_pinmux[27] = 0x19,//25,0x87
		.cfg_board_common_setting.ac_pinmux[28] = 0x1c,//28,0x88
		.cfg_board_common_setting.ac_pinmux[29] = 0x1d,//29,0x89
		.cfg_board_common_setting.dfi_pinmux[0] = 0x0,//0,0x8a
		.cfg_board_common_setting.dfi_pinmux[1] = 0x0,//0,0x8b
		.cfg_board_common_setting.dfi_pinmux[2] = 0x0,//0,0x8c
		.cfg_board_common_setting.dfi_pinmux[3] = 0x0,//0,0x8d
		.cfg_board_common_setting.dfi_pinmux[4] = 0x0,//0,0x8e
		.cfg_board_common_setting.dfi_pinmux[5] = 0x0,//0,0x8f
		.cfg_board_common_setting.dfi_pinmux[6] = 0x0,//0,0x90
		.cfg_board_common_setting.dfi_pinmux[7] = 0x0,//0,0x91
		.cfg_board_common_setting.dfi_pinmux[8] = 0x0,//0,0x92
		.cfg_board_common_setting.dfi_pinmux[9] = 0x0,//0,0x93
		.cfg_board_common_setting.ddr_dqs_swap=0x0,//0,0x94
		.cfg_board_common_setting.rsv_char1=0x0,//0,0x95
		.cfg_board_common_setting.rsv_char2=0x0,//0,0x96
		.cfg_board_common_setting.rsv_char3=0x0,//0,0x97
		.cfg_board_common_setting.ddr_vddee_setting[0] = 0x0,//0,0x98
		.cfg_board_common_setting.ddr_vddee_setting[1] = 0x0,//0,0x9c
		.cfg_board_common_setting.ddr_vddee_setting[2] = 0x0,//0,0xa0
		.cfg_board_common_setting.ddr_vddee_setting[3] = 0x0,//0,0xa4
		.cfg_board_SI_setting_ps.DRAMFreq=0x420,//1056,0xa8
		.cfg_board_SI_setting_ps.PllBypassEn=0x0,//0,0xaa
		.cfg_board_SI_setting_ps.training_SequenceCtrl=0x0,//0,0xab
		.cfg_board_SI_setting_ps.dfi_odt_config=0x1,//1,0xac
		.cfg_board_SI_setting_ps.phy_odt_config_rank[0] = 0x0,//0,0xb0
		.cfg_board_SI_setting_ps.phy_odt_config_rank[1] = 0x0,//0,0xb1
		.cfg_board_SI_setting_ps.clk_drv_ohm=0x28,//40,0xb2
		.cfg_board_SI_setting_ps.cs_drv_ohm=0x28,//40,0xb4
		.cfg_board_SI_setting_ps.ac_drv_ohm=0x28,//40,0xb6
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_p=0x28,//40,0xb8
		.cfg_board_SI_setting_ps.soc_data_drv_ohm_n=0x28,//40,0xba
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_p=0x78,//120,0xbc
		.cfg_board_SI_setting_ps.soc_data_odt_ohm_n=0x78,//120,0xbe
		.cfg_board_SI_setting_ps.dram_data_drv_ohm=0x28,//40,0xc0
		.cfg_board_SI_setting_ps.dram_data_odt_ohm=0x78,//120,0xc2
		.cfg_board_SI_setting_ps.dram_data_wr_odt_ohm=0x0,//0,0xc4
		.cfg_board_SI_setting_ps.dram_ac_odt_ohm=0x78,//120,0xc6
		.cfg_board_SI_setting_ps.soc_clk_slew_rate=0x0,//0,0xc8
		.cfg_board_SI_setting_ps.soc_cs_slew_rate=0x0,//0,0xca
		.cfg_board_SI_setting_ps.soc_ac_slew_rate=0x0,//0,0xcc
		.cfg_board_SI_setting_ps.soc_data_slew_rate=0x0,//0,0xce
		.cfg_board_SI_setting_ps.dram_drv_pull_up_cal_ohm=0x28,//40,0xd0
		.cfg_board_SI_setting_ps.lpddr4_dram_vout_range=0x1,//1,0xd1
		.cfg_board_SI_setting_ps.char_rev0=0x0,//0,0xd2
		.cfg_board_SI_setting_ps.char_rev1=0x0,//0,0xd3
		.cfg_board_SI_setting_ps.vref_ac_permil=0x1f4,//500,0xd4
		.cfg_board_SI_setting_ps.vref_soc_data_permil=0x1f4,//500,0xd6
		.cfg_board_SI_setting_ps.vref_dram_data_permil=0x1f4,//500,0xd8
		.cfg_board_SI_setting_ps.max_core_timmming_frequency=0x855,//2133,0xda
		.cfg_ddr_training_delay_ps.ac_trace_delay[0] = 0x180,//384,0xdc
		.cfg_ddr_training_delay_ps.ac_trace_delay[1] = 0x180,//384,0xde
		.cfg_ddr_training_delay_ps.ac_trace_delay[2] = 0x180,//384,0xe0
		.cfg_ddr_training_delay_ps.ac_trace_delay[3] = 0x180,//384,0xe2
		.cfg_ddr_training_delay_ps.ac_trace_delay[4] = 0x180,//384,0xe4
		.cfg_ddr_training_delay_ps.ac_trace_delay[5] = 0x180,//384,0xe6
		.cfg_ddr_training_delay_ps.ac_trace_delay[6] = 0x180,//384,0xe8
		.cfg_ddr_training_delay_ps.ac_trace_delay[7] = 0x100,//256,0xea
		.cfg_ddr_training_delay_ps.ac_trace_delay[8] = 0x180,//384,0xec
		.cfg_ddr_training_delay_ps.ac_trace_delay[9] = 0x100,//256,0xee
		.cfg_ddr_training_delay_ps.ac_trace_delay[10] = 0x100,//256,0xf0
		.cfg_ddr_training_delay_ps.ac_trace_delay[11] = 0x100,//256,0xf2
		.cfg_ddr_training_delay_ps.ac_trace_delay[12] = 0x180,//384,0xf4
		.cfg_ddr_training_delay_ps.ac_trace_delay[13] = 0x180,//384,0xf6
		.cfg_ddr_training_delay_ps.ac_trace_delay[14] = 0x180,//384,0xf8
		.cfg_ddr_training_delay_ps.ac_trace_delay[15] = 0x180,//384,0xfa
		.cfg_ddr_training_delay_ps.ac_trace_delay[16] = 0x180,//384,0xfc
		.cfg_ddr_training_delay_ps.ac_trace_delay[17] = 0x180,//384,0xfe
		.cfg_ddr_training_delay_ps.ac_trace_delay[18] = 0x180,//384,0x100
		.cfg_ddr_training_delay_ps.ac_trace_delay[19] = 0x180,//384,0x102
		.cfg_ddr_training_delay_ps.ac_trace_delay[20] = 0x180,//384,0x104
		.cfg_ddr_training_delay_ps.ac_trace_delay[21] = 0x180,//384,0x106
		.cfg_ddr_training_delay_ps.ac_trace_delay[22] = 0x180,//384,0x108
		.cfg_ddr_training_delay_ps.ac_trace_delay[23] = 0x180,//384,0x10a
		.cfg_ddr_training_delay_ps.ac_trace_delay[24] = 0x180,//384,0x10c
		.cfg_ddr_training_delay_ps.ac_trace_delay[25] = 0x180,//384,0x10e
		.cfg_ddr_training_delay_ps.ac_trace_delay[26] = 0x100,//256,0x110
		.cfg_ddr_training_delay_ps.ac_trace_delay[27] = 0x100,//256,0x112
		.cfg_ddr_training_delay_ps.ac_trace_delay[28] = 0x100,//256,0x114
		.cfg_ddr_training_delay_ps.ac_trace_delay[29] = 0x100,//256,0x116
		.cfg_ddr_training_delay_ps.read_dq_delay_t[0] = 0x2c,//44,0x118
		.cfg_ddr_training_delay_ps.read_dq_delay_t[1] = 0x33,//51,0x11a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[2] = 0x30,//48,0x11c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[3] = 0x3f,//63,0x11e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[4] = 0x2a,//42,0x120
		.cfg_ddr_training_delay_ps.read_dq_delay_t[5] = 0x2a,//42,0x122
		.cfg_ddr_training_delay_ps.read_dq_delay_t[6] = 0x32,//50,0x124
		.cfg_ddr_training_delay_ps.read_dq_delay_t[7] = 0x26,//38,0x126
		.cfg_ddr_training_delay_ps.read_dq_delay_t[8] = 0x3f,//63,0x128
		.cfg_ddr_training_delay_ps.read_dq_delay_t[9] = 0x2e,//46,0x12a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[10] = 0x39,//57,0x12c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[11] = 0x31,//49,0x12e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[12] = 0x3a,//58,0x130
		.cfg_ddr_training_delay_ps.read_dq_delay_t[13] = 0x3f,//63,0x132
		.cfg_ddr_training_delay_ps.read_dq_delay_t[14] = 0x28,//40,0x134
		.cfg_ddr_training_delay_ps.read_dq_delay_t[15] = 0x32,//50,0x136
		.cfg_ddr_training_delay_ps.read_dq_delay_t[16] = 0x2b,//43,0x138
		.cfg_ddr_training_delay_ps.read_dq_delay_t[17] = 0x38,//56,0x13a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[18] = 0x32,//50,0x13c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[19] = 0x3c,//60,0x13e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[20] = 0x2f,//47,0x140
		.cfg_ddr_training_delay_ps.read_dq_delay_t[21] = 0x33,//51,0x142
		.cfg_ddr_training_delay_ps.read_dq_delay_t[22] = 0x3f,//63,0x144
		.cfg_ddr_training_delay_ps.read_dq_delay_t[23] = 0x27,//39,0x146
		.cfg_ddr_training_delay_ps.read_dq_delay_t[24] = 0x30,//48,0x148
		.cfg_ddr_training_delay_ps.read_dq_delay_t[25] = 0x26,//38,0x14a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[26] = 0x31,//49,0x14c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[27] = 0x29,//41,0x14e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[28] = 0x3f,//63,0x150
		.cfg_ddr_training_delay_ps.read_dq_delay_t[29] = 0x2c,//44,0x152
		.cfg_ddr_training_delay_ps.read_dq_delay_t[30] = 0x39,//57,0x154
		.cfg_ddr_training_delay_ps.read_dq_delay_t[31] = 0x2b,//43,0x156
		.cfg_ddr_training_delay_ps.read_dq_delay_t[32] = 0x2b,//43,0x158
		.cfg_ddr_training_delay_ps.read_dq_delay_t[33] = 0x34,//52,0x15a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[34] = 0x2e,//46,0x15c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[35] = 0x36,//54,0x15e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[36] = 0x0,//0,0x160
		.cfg_ddr_training_delay_ps.read_dq_delay_t[37] = 0x0,//0,0x162
		.cfg_ddr_training_delay_ps.read_dq_delay_t[38] = 0x0,//0,0x164
		.cfg_ddr_training_delay_ps.read_dq_delay_t[39] = 0x0,//0,0x166
		.cfg_ddr_training_delay_ps.read_dq_delay_t[40] = 0x0,//0,0x168
		.cfg_ddr_training_delay_ps.read_dq_delay_t[41] = 0x0,//0,0x16a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[42] = 0x0,//0,0x16c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[43] = 0x0,//0,0x16e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[44] = 0x0,//0,0x170
		.cfg_ddr_training_delay_ps.read_dq_delay_t[45] = 0x0,//0,0x172
		.cfg_ddr_training_delay_ps.read_dq_delay_t[46] = 0x0,//0,0x174
		.cfg_ddr_training_delay_ps.read_dq_delay_t[47] = 0x0,//0,0x176
		.cfg_ddr_training_delay_ps.read_dq_delay_t[48] = 0x0,//0,0x178
		.cfg_ddr_training_delay_ps.read_dq_delay_t[49] = 0x0,//0,0x17a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[50] = 0x0,//0,0x17c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[51] = 0x0,//0,0x17e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[52] = 0x0,//0,0x180
		.cfg_ddr_training_delay_ps.read_dq_delay_t[53] = 0x0,//0,0x182
		.cfg_ddr_training_delay_ps.read_dq_delay_t[54] = 0x0,//0,0x184
		.cfg_ddr_training_delay_ps.read_dq_delay_t[55] = 0x0,//0,0x186
		.cfg_ddr_training_delay_ps.read_dq_delay_t[56] = 0x0,//0,0x188
		.cfg_ddr_training_delay_ps.read_dq_delay_t[57] = 0x0,//0,0x18a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[58] = 0x0,//0,0x18c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[59] = 0x0,//0,0x18e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[60] = 0x0,//0,0x190
		.cfg_ddr_training_delay_ps.read_dq_delay_t[61] = 0x0,//0,0x192
		.cfg_ddr_training_delay_ps.read_dq_delay_t[62] = 0x0,//0,0x194
		.cfg_ddr_training_delay_ps.read_dq_delay_t[63] = 0x0,//0,0x196
		.cfg_ddr_training_delay_ps.read_dq_delay_t[64] = 0x0,//0,0x198
		.cfg_ddr_training_delay_ps.read_dq_delay_t[65] = 0x0,//0,0x19a
		.cfg_ddr_training_delay_ps.read_dq_delay_t[66] = 0x0,//0,0x19c
		.cfg_ddr_training_delay_ps.read_dq_delay_t[67] = 0x0,//0,0x19e
		.cfg_ddr_training_delay_ps.read_dq_delay_t[68] = 0x0,//0,0x1a0
		.cfg_ddr_training_delay_ps.read_dq_delay_t[69] = 0x0,//0,0x1a2
		.cfg_ddr_training_delay_ps.read_dq_delay_t[70] = 0x0,//0,0x1a4
		.cfg_ddr_training_delay_ps.read_dq_delay_t[71] = 0x0,//0,0x1a6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[0] = 0x0,//0,0x1a8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[1] = 0x0,//0,0x1aa
		.cfg_ddr_training_delay_ps.read_dq_delay_c[2] = 0x0,//0,0x1ac
		.cfg_ddr_training_delay_ps.read_dq_delay_c[3] = 0x0,//0,0x1ae
		.cfg_ddr_training_delay_ps.read_dq_delay_c[4] = 0x0,//0,0x1b0
		.cfg_ddr_training_delay_ps.read_dq_delay_c[5] = 0x0,//0,0x1b2
		.cfg_ddr_training_delay_ps.read_dq_delay_c[6] = 0x0,//0,0x1b4
		.cfg_ddr_training_delay_ps.read_dq_delay_c[7] = 0x0,//0,0x1b6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[8] = 0x0,//0,0x1b8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[9] = 0x0,//0,0x1ba
		.cfg_ddr_training_delay_ps.read_dq_delay_c[10] = 0x0,//0,0x1bc
		.cfg_ddr_training_delay_ps.read_dq_delay_c[11] = 0x0,//0,0x1be
		.cfg_ddr_training_delay_ps.read_dq_delay_c[12] = 0x0,//0,0x1c0
		.cfg_ddr_training_delay_ps.read_dq_delay_c[13] = 0x0,//0,0x1c2
		.cfg_ddr_training_delay_ps.read_dq_delay_c[14] = 0x0,//0,0x1c4
		.cfg_ddr_training_delay_ps.read_dq_delay_c[15] = 0x0,//0,0x1c6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[16] = 0x0,//0,0x1c8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[17] = 0x0,//0,0x1ca
		.cfg_ddr_training_delay_ps.read_dq_delay_c[18] = 0x0,//0,0x1cc
		.cfg_ddr_training_delay_ps.read_dq_delay_c[19] = 0x0,//0,0x1ce
		.cfg_ddr_training_delay_ps.read_dq_delay_c[20] = 0x0,//0,0x1d0
		.cfg_ddr_training_delay_ps.read_dq_delay_c[21] = 0x0,//0,0x1d2
		.cfg_ddr_training_delay_ps.read_dq_delay_c[22] = 0x0,//0,0x1d4
		.cfg_ddr_training_delay_ps.read_dq_delay_c[23] = 0x0,//0,0x1d6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[24] = 0x0,//0,0x1d8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[25] = 0x0,//0,0x1da
		.cfg_ddr_training_delay_ps.read_dq_delay_c[26] = 0x0,//0,0x1dc
		.cfg_ddr_training_delay_ps.read_dq_delay_c[27] = 0x0,//0,0x1de
		.cfg_ddr_training_delay_ps.read_dq_delay_c[28] = 0x0,//0,0x1e0
		.cfg_ddr_training_delay_ps.read_dq_delay_c[29] = 0x0,//0,0x1e2
		.cfg_ddr_training_delay_ps.read_dq_delay_c[30] = 0x0,//0,0x1e4
		.cfg_ddr_training_delay_ps.read_dq_delay_c[31] = 0x0,//0,0x1e6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[32] = 0x0,//0,0x1e8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[33] = 0x0,//0,0x1ea
		.cfg_ddr_training_delay_ps.read_dq_delay_c[34] = 0x0,//0,0x1ec
		.cfg_ddr_training_delay_ps.read_dq_delay_c[35] = 0x0,//0,0x1ee
		.cfg_ddr_training_delay_ps.read_dq_delay_c[36] = 0x0,//0,0x1f0
		.cfg_ddr_training_delay_ps.read_dq_delay_c[37] = 0x0,//0,0x1f2
		.cfg_ddr_training_delay_ps.read_dq_delay_c[38] = 0x0,//0,0x1f4
		.cfg_ddr_training_delay_ps.read_dq_delay_c[39] = 0x0,//0,0x1f6
		.cfg_ddr_training_delay_ps.read_dq_delay_c[40] = 0x0,//0,0x1f8
		.cfg_ddr_training_delay_ps.read_dq_delay_c[41] = 0x0,//0,0x1fa
		.cfg_ddr_training_delay_ps.read_dq_delay_c[42] = 0x0,//0,0x1fc
		.cfg_ddr_training_delay_ps.read_dq_delay_c[43] = 0x0,//0,0x1fe
		.cfg_ddr_training_delay_ps.read_dq_delay_c[44] = 0x0,//0,0x200
		.cfg_ddr_training_delay_ps.read_dq_delay_c[45] = 0x0,//0,0x202
		.cfg_ddr_training_delay_ps.read_dq_delay_c[46] = 0x0,//0,0x204
		.cfg_ddr_training_delay_ps.read_dq_delay_c[47] = 0x0,//0,0x206
		.cfg_ddr_training_delay_ps.read_dq_delay_c[48] = 0x0,//0,0x208
		.cfg_ddr_training_delay_ps.read_dq_delay_c[49] = 0x0,//0,0x20a
		.cfg_ddr_training_delay_ps.read_dq_delay_c[50] = 0x0,//0,0x20c
		.cfg_ddr_training_delay_ps.read_dq_delay_c[51] = 0x0,//0,0x20e
		.cfg_ddr_training_delay_ps.read_dq_delay_c[52] = 0x0,//0,0x210
		.cfg_ddr_training_delay_ps.read_dq_delay_c[53] = 0x0,//0,0x212
		.cfg_ddr_training_delay_ps.read_dq_delay_c[54] = 0x0,//0,0x214
		.cfg_ddr_training_delay_ps.read_dq_delay_c[55] = 0x0,//0,0x216
		.cfg_ddr_training_delay_ps.read_dq_delay_c[56] = 0x0,//0,0x218
		.cfg_ddr_training_delay_ps.read_dq_delay_c[57] = 0x0,//0,0x21a
		.cfg_ddr_training_delay_ps.read_dq_delay_c[58] = 0x0,//0,0x21c
		.cfg_ddr_training_delay_ps.read_dq_delay_c[59] = 0x0,//0,0x21e
		.cfg_ddr_training_delay_ps.read_dq_delay_c[60] = 0x0,//0,0x220
		.cfg_ddr_training_delay_ps.read_dq_delay_c[61] = 0x0,//0,0x222
		.cfg_ddr_training_delay_ps.read_dq_delay_c[62] = 0x0,//0,0x224
		.cfg_ddr_training_delay_ps.read_dq_delay_c[63] = 0x0,//0,0x226
		.cfg_ddr_training_delay_ps.read_dq_delay_c[64] = 0x0,//0,0x228
		.cfg_ddr_training_delay_ps.read_dq_delay_c[65] = 0x0,//0,0x22a
		.cfg_ddr_training_delay_ps.read_dq_delay_c[66] = 0x0,//0,0x22c
		.cfg_ddr_training_delay_ps.read_dq_delay_c[67] = 0x0,//0,0x22e
		.cfg_ddr_training_delay_ps.read_dq_delay_c[68] = 0x0,//0,0x230
		.cfg_ddr_training_delay_ps.read_dq_delay_c[69] = 0x0,//0,0x232
		.cfg_ddr_training_delay_ps.read_dq_delay_c[70] = 0x0,//0,0x234
		.cfg_ddr_training_delay_ps.read_dq_delay_c[71] = 0x0,//0,0x236
		.cfg_ddr_training_delay_ps.read_dqs_delay[0] = 0x5b,//91,0x238
		.cfg_ddr_training_delay_ps.read_dqs_delay[1] = 0x5b,//91,0x23a
		.cfg_ddr_training_delay_ps.read_dqs_delay[2] = 0x5b,//91,0x23c
		.cfg_ddr_training_delay_ps.read_dqs_delay[3] = 0x5b,//91,0x23e
		.cfg_ddr_training_delay_ps.read_dqs_delay[4] = 0x61,//97,0x240
		.cfg_ddr_training_delay_ps.read_dqs_delay[5] = 0x61,//97,0x242
		.cfg_ddr_training_delay_ps.read_dqs_delay[6] = 0x61,//97,0x244
		.cfg_ddr_training_delay_ps.read_dqs_delay[7] = 0x61,//97,0x246
		.cfg_ddr_training_delay_ps.write_dqs_delay[0] = 0xe7,//231,0x248
		.cfg_ddr_training_delay_ps.write_dqs_delay[1] = 0xe5,//229,0x24a
		.cfg_ddr_training_delay_ps.write_dqs_delay[2] = 0xe6,//230,0x24c
		.cfg_ddr_training_delay_ps.write_dqs_delay[3] = 0xdf,//223,0x24e
		.cfg_ddr_training_delay_ps.write_dqs_delay[4] = 0x1bf,//447,0x250
		.cfg_ddr_training_delay_ps.write_dqs_delay[5] = 0x1bf,//447,0x252
		.cfg_ddr_training_delay_ps.write_dqs_delay[6] = 0x1bf,//447,0x254
		.cfg_ddr_training_delay_ps.write_dqs_delay[7] = 0x1bf,//447,0x256
		.cfg_ddr_training_delay_ps.write_wck_delay[0] = 0x0,//0,0x258
		.cfg_ddr_training_delay_ps.write_wck_delay[1] = 0x0,//0,0x25a
		.cfg_ddr_training_delay_ps.write_wck_delay[2] = 0x0,//0,0x25c
		.cfg_ddr_training_delay_ps.write_wck_delay[3] = 0x0,//0,0x25e
		.cfg_ddr_training_delay_ps.write_wck_delay[4] = 0x0,//0,0x260
		.cfg_ddr_training_delay_ps.write_wck_delay[5] = 0x0,//0,0x262
		.cfg_ddr_training_delay_ps.write_wck_delay[6] = 0x0,//0,0x264
		.cfg_ddr_training_delay_ps.write_wck_delay[7] = 0x0,//0,0x266
		.cfg_ddr_training_delay_ps.wdq_delay[0] = 0x10b,//267,0x268
		.cfg_ddr_training_delay_ps.wdq_delay[1] = 0x110,//272,0x26a
		.cfg_ddr_training_delay_ps.wdq_delay[2] = 0x119,//281,0x26c
		.cfg_ddr_training_delay_ps.wdq_delay[3] = 0x125,//293,0x26e
		.cfg_ddr_training_delay_ps.wdq_delay[4] = 0x10c,//268,0x270
		.cfg_ddr_training_delay_ps.wdq_delay[5] = 0x10d,//269,0x272
		.cfg_ddr_training_delay_ps.wdq_delay[6] = 0x113,//275,0x274
		.cfg_ddr_training_delay_ps.wdq_delay[7] = 0x104,//260,0x276
		.cfg_ddr_training_delay_ps.wdq_delay[8] = 0x111,//273,0x278
		.cfg_ddr_training_delay_ps.wdq_delay[9] = 0x10f,//271,0x27a
		.cfg_ddr_training_delay_ps.wdq_delay[10] = 0x11b,//283,0x27c
		.cfg_ddr_training_delay_ps.wdq_delay[11] = 0x119,//281,0x27e
		.cfg_ddr_training_delay_ps.wdq_delay[12] = 0x123,//291,0x280
		.cfg_ddr_training_delay_ps.wdq_delay[13] = 0x115,//277,0x282
		.cfg_ddr_training_delay_ps.wdq_delay[14] = 0x10c,//268,0x284
		.cfg_ddr_training_delay_ps.wdq_delay[15] = 0x114,//276,0x286
		.cfg_ddr_training_delay_ps.wdq_delay[16] = 0x10e,//270,0x288
		.cfg_ddr_training_delay_ps.wdq_delay[17] = 0x11a,//282,0x28a
		.cfg_ddr_training_delay_ps.wdq_delay[18] = 0x11a,//282,0x28c
		.cfg_ddr_training_delay_ps.wdq_delay[19] = 0x125,//293,0x28e
		.cfg_ddr_training_delay_ps.wdq_delay[20] = 0x114,//276,0x290
		.cfg_ddr_training_delay_ps.wdq_delay[21] = 0x109,//265,0x292
		.cfg_ddr_training_delay_ps.wdq_delay[22] = 0x10f,//271,0x294
		.cfg_ddr_training_delay_ps.wdq_delay[23] = 0x105,//261,0x296
		.cfg_ddr_training_delay_ps.wdq_delay[24] = 0x108,//264,0x298
		.cfg_ddr_training_delay_ps.wdq_delay[25] = 0x108,//264,0x29a
		.cfg_ddr_training_delay_ps.wdq_delay[26] = 0x109,//265,0x29c
		.cfg_ddr_training_delay_ps.wdq_delay[27] = 0x107,//263,0x29e
		.cfg_ddr_training_delay_ps.wdq_delay[28] = 0x10d,//269,0x2a0
		.cfg_ddr_training_delay_ps.wdq_delay[29] = 0x10f,//271,0x2a2
		.cfg_ddr_training_delay_ps.wdq_delay[30] = 0x120,//288,0x2a4
		.cfg_ddr_training_delay_ps.wdq_delay[31] = 0x109,//265,0x2a6
		.cfg_ddr_training_delay_ps.wdq_delay[32] = 0x102,//258,0x2a8
		.cfg_ddr_training_delay_ps.wdq_delay[33] = 0x10f,//271,0x2aa
		.cfg_ddr_training_delay_ps.wdq_delay[34] = 0x10c,//268,0x2ac
		.cfg_ddr_training_delay_ps.wdq_delay[35] = 0x112,//274,0x2ae
		.cfg_ddr_training_delay_ps.wdq_delay[36] = 0x0,//0,0x2b0
		.cfg_ddr_training_delay_ps.wdq_delay[37] = 0x0,//0,0x2b2
		.cfg_ddr_training_delay_ps.wdq_delay[38] = 0x0,//0,0x2b4
		.cfg_ddr_training_delay_ps.wdq_delay[39] = 0x0,//0,0x2b6
		.cfg_ddr_training_delay_ps.wdq_delay[40] = 0x0,//0,0x2b8
		.cfg_ddr_training_delay_ps.wdq_delay[41] = 0x0,//0,0x2ba
		.cfg_ddr_training_delay_ps.wdq_delay[42] = 0x0,//0,0x2bc
		.cfg_ddr_training_delay_ps.wdq_delay[43] = 0x0,//0,0x2be
		.cfg_ddr_training_delay_ps.wdq_delay[44] = 0x0,//0,0x2c0
		.cfg_ddr_training_delay_ps.wdq_delay[45] = 0x0,//0,0x2c2
		.cfg_ddr_training_delay_ps.wdq_delay[46] = 0x0,//0,0x2c4
		.cfg_ddr_training_delay_ps.wdq_delay[47] = 0x0,//0,0x2c6
		.cfg_ddr_training_delay_ps.wdq_delay[48] = 0x0,//0,0x2c8
		.cfg_ddr_training_delay_ps.wdq_delay[49] = 0x0,//0,0x2ca
		.cfg_ddr_training_delay_ps.wdq_delay[50] = 0x0,//0,0x2cc
		.cfg_ddr_training_delay_ps.wdq_delay[51] = 0x0,//0,0x2ce
		.cfg_ddr_training_delay_ps.wdq_delay[52] = 0x0,//0,0x2d0
		.cfg_ddr_training_delay_ps.wdq_delay[53] = 0x0,//0,0x2d2
		.cfg_ddr_training_delay_ps.wdq_delay[54] = 0x0,//0,0x2d4
		.cfg_ddr_training_delay_ps.wdq_delay[55] = 0x0,//0,0x2d6
		.cfg_ddr_training_delay_ps.wdq_delay[56] = 0x0,//0,0x2d8
		.cfg_ddr_training_delay_ps.wdq_delay[57] = 0x0,//0,0x2da
		.cfg_ddr_training_delay_ps.wdq_delay[58] = 0x0,//0,0x2dc
		.cfg_ddr_training_delay_ps.wdq_delay[59] = 0x0,//0,0x2de
		.cfg_ddr_training_delay_ps.wdq_delay[60] = 0x0,//0,0x2e0
		.cfg_ddr_training_delay_ps.wdq_delay[61] = 0x0,//0,0x2e2
		.cfg_ddr_training_delay_ps.wdq_delay[62] = 0x0,//0,0x2e4
		.cfg_ddr_training_delay_ps.wdq_delay[63] = 0x0,//0,0x2e6
		.cfg_ddr_training_delay_ps.wdq_delay[64] = 0x0,//0,0x2e8
		.cfg_ddr_training_delay_ps.wdq_delay[65] = 0x0,//0,0x2ea
		.cfg_ddr_training_delay_ps.wdq_delay[66] = 0x0,//0,0x2ec
		.cfg_ddr_training_delay_ps.wdq_delay[67] = 0x0,//0,0x2ee
		.cfg_ddr_training_delay_ps.wdq_delay[68] = 0x0,//0,0x2f0
		.cfg_ddr_training_delay_ps.wdq_delay[69] = 0x0,//0,0x2f2
		.cfg_ddr_training_delay_ps.wdq_delay[70] = 0x0,//0,0x2f4
		.cfg_ddr_training_delay_ps.wdq_delay[71] = 0x0,//0,0x2f6
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[0] = 0x2e2,//738,0x2f8
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[1] = 0x2e3,//739,0x2fa
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[2] = 0x2dd,//733,0x2fc
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[3] = 0x2e1,//737,0x2fe
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[4] = 0x0,//0,0x300
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[5] = 0x0,//0,0x302
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[6] = 0x0,//0,0x304
		.cfg_ddr_training_delay_ps.read_dqs_gate_delay[7] = 0x0,//0,0x306
		.cfg_ddr_training_delay_ps.soc_bit_vref0[0] = 0x20,//32,0x308
		.cfg_ddr_training_delay_ps.soc_bit_vref0[1] = 0x20,//32,0x309
		.cfg_ddr_training_delay_ps.soc_bit_vref0[2] = 0x22,//34,0x30a
		.cfg_ddr_training_delay_ps.soc_bit_vref0[3] = 0x21,//33,0x30b
		.cfg_ddr_training_delay_ps.soc_bit_vref0[4] = 0x20,//32,0x30c
		.cfg_ddr_training_delay_ps.soc_bit_vref0[5] = 0x20,//32,0x30d
		.cfg_ddr_training_delay_ps.soc_bit_vref0[6] = 0x20,//32,0x30e
		.cfg_ddr_training_delay_ps.soc_bit_vref0[7] = 0x20,//32,0x30f
		.cfg_ddr_training_delay_ps.soc_bit_vref0[8] = 0x15,//21,0x310
		.cfg_ddr_training_delay_ps.soc_bit_vref0[9] = 0x21,//33,0x311
		.cfg_ddr_training_delay_ps.soc_bit_vref0[10] = 0x20,//32,0x312
		.cfg_ddr_training_delay_ps.soc_bit_vref0[11] = 0x21,//33,0x313
		.cfg_ddr_training_delay_ps.soc_bit_vref0[12] = 0x21,//33,0x314
		.cfg_ddr_training_delay_ps.soc_bit_vref0[13] = 0x15,//21,0x315
		.cfg_ddr_training_delay_ps.soc_bit_vref0[14] = 0x20,//32,0x316
		.cfg_ddr_training_delay_ps.soc_bit_vref0[15] = 0x1f,//31,0x317
		.cfg_ddr_training_delay_ps.soc_bit_vref0[16] = 0x21,//33,0x318
		.cfg_ddr_training_delay_ps.soc_bit_vref0[17] = 0x21,//33,0x319
		.cfg_ddr_training_delay_ps.soc_bit_vref0[18] = 0x23,//35,0x31a
		.cfg_ddr_training_delay_ps.soc_bit_vref0[19] = 0x22,//34,0x31b
		.cfg_ddr_training_delay_ps.soc_bit_vref0[20] = 0x21,//33,0x31c
		.cfg_ddr_training_delay_ps.soc_bit_vref0[21] = 0x20,//32,0x31d
		.cfg_ddr_training_delay_ps.soc_bit_vref0[22] = 0x15,//21,0x31e
		.cfg_ddr_training_delay_ps.soc_bit_vref0[23] = 0x20,//32,0x31f
		.cfg_ddr_training_delay_ps.soc_bit_vref0[24] = 0x21,//33,0x320
		.cfg_ddr_training_delay_ps.soc_bit_vref0[25] = 0x20,//32,0x321
		.cfg_ddr_training_delay_ps.soc_bit_vref0[26] = 0x21,//33,0x322
		.cfg_ddr_training_delay_ps.soc_bit_vref0[27] = 0x20,//32,0x323
		.cfg_ddr_training_delay_ps.soc_bit_vref0[28] = 0x15,//21,0x324
		.cfg_ddr_training_delay_ps.soc_bit_vref0[29] = 0x21,//33,0x325
		.cfg_ddr_training_delay_ps.soc_bit_vref0[30] = 0x21,//33,0x326
		.cfg_ddr_training_delay_ps.soc_bit_vref0[31] = 0x21,//33,0x327
		.cfg_ddr_training_delay_ps.soc_bit_vref0[32] = 0x20,//32,0x328
		.cfg_ddr_training_delay_ps.soc_bit_vref0[33] = 0x21,//33,0x329
		.cfg_ddr_training_delay_ps.soc_bit_vref0[34] = 0x21,//33,0x32a
		.cfg_ddr_training_delay_ps.soc_bit_vref0[35] = 0x21,//33,0x32b
		.cfg_ddr_training_delay_ps.soc_bit_vref1[0] = 0x20,//32,0x32c
		.cfg_ddr_training_delay_ps.soc_bit_vref1[1] = 0x20,//32,0x32d
		.cfg_ddr_training_delay_ps.soc_bit_vref1[2] = 0x22,//34,0x32e
		.cfg_ddr_training_delay_ps.soc_bit_vref1[3] = 0x21,//33,0x32f
		.cfg_ddr_training_delay_ps.soc_bit_vref1[4] = 0x20,//32,0x330
		.cfg_ddr_training_delay_ps.soc_bit_vref1[5] = 0x20,//32,0x331
		.cfg_ddr_training_delay_ps.soc_bit_vref1[6] = 0x20,//32,0x332
		.cfg_ddr_training_delay_ps.soc_bit_vref1[7] = 0x20,//32,0x333
		.cfg_ddr_training_delay_ps.soc_bit_vref1[8] = 0x15,//21,0x334
		.cfg_ddr_training_delay_ps.soc_bit_vref1[9] = 0x21,//33,0x335
		.cfg_ddr_training_delay_ps.soc_bit_vref1[10] = 0x20,//32,0x336
		.cfg_ddr_training_delay_ps.soc_bit_vref1[11] = 0x21,//33,0x337
		.cfg_ddr_training_delay_ps.soc_bit_vref1[12] = 0x21,//33,0x338
		.cfg_ddr_training_delay_ps.soc_bit_vref1[13] = 0x15,//21,0x339
		.cfg_ddr_training_delay_ps.soc_bit_vref1[14] = 0x20,//32,0x33a
		.cfg_ddr_training_delay_ps.soc_bit_vref1[15] = 0x1f,//31,0x33b
		.cfg_ddr_training_delay_ps.soc_bit_vref1[16] = 0x21,//33,0x33c
		.cfg_ddr_training_delay_ps.soc_bit_vref1[17] = 0x21,//33,0x33d
		.cfg_ddr_training_delay_ps.soc_bit_vref1[18] = 0x23,//35,0x33e
		.cfg_ddr_training_delay_ps.soc_bit_vref1[19] = 0x22,//34,0x33f
		.cfg_ddr_training_delay_ps.soc_bit_vref1[20] = 0x21,//33,0x340
		.cfg_ddr_training_delay_ps.soc_bit_vref1[21] = 0x20,//32,0x341
		.cfg_ddr_training_delay_ps.soc_bit_vref1[22] = 0x15,//21,0x342
		.cfg_ddr_training_delay_ps.soc_bit_vref1[23] = 0x20,//32,0x343
		.cfg_ddr_training_delay_ps.soc_bit_vref1[24] = 0x21,//33,0x344
		.cfg_ddr_training_delay_ps.soc_bit_vref1[25] = 0x20,//32,0x345
		.cfg_ddr_training_delay_ps.soc_bit_vref1[26] = 0x21,//33,0x346
		.cfg_ddr_training_delay_ps.soc_bit_vref1[27] = 0x20,//32,0x347
		.cfg_ddr_training_delay_ps.soc_bit_vref1[28] = 0x15,//21,0x348
		.cfg_ddr_training_delay_ps.soc_bit_vref1[29] = 0x21,//33,0x349
		.cfg_ddr_training_delay_ps.soc_bit_vref1[30] = 0x21,//33,0x34a
		.cfg_ddr_training_delay_ps.soc_bit_vref1[31] = 0x21,//33,0x34b
		.cfg_ddr_training_delay_ps.soc_bit_vref1[32] = 0x20,//32,0x34c
		.cfg_ddr_training_delay_ps.soc_bit_vref1[33] = 0x21,//33,0x34d
		.cfg_ddr_training_delay_ps.soc_bit_vref1[34] = 0x21,//33,0x34e
		.cfg_ddr_training_delay_ps.soc_bit_vref1[35] = 0x21,//33,0x34f
		.cfg_ddr_training_delay_ps.soc_bit_vref2[0] = 0x0,//0,0x350
		.cfg_ddr_training_delay_ps.soc_bit_vref2[1] = 0x0,//0,0x351
		.cfg_ddr_training_delay_ps.soc_bit_vref2[2] = 0x0,//0,0x352
		.cfg_ddr_training_delay_ps.soc_bit_vref2[3] = 0x0,//0,0x353
		.cfg_ddr_training_delay_ps.soc_bit_vref2[4] = 0x0,//0,0x354
		.cfg_ddr_training_delay_ps.soc_bit_vref2[5] = 0x0,//0,0x355
		.cfg_ddr_training_delay_ps.soc_bit_vref2[6] = 0x0,//0,0x356
		.cfg_ddr_training_delay_ps.soc_bit_vref2[7] = 0x0,//0,0x357
		.cfg_ddr_training_delay_ps.soc_bit_vref2[8] = 0x0,//0,0x358
		.cfg_ddr_training_delay_ps.soc_bit_vref2[9] = 0x0,//0,0x359
		.cfg_ddr_training_delay_ps.soc_bit_vref2[10] = 0x0,//0,0x35a
		.cfg_ddr_training_delay_ps.soc_bit_vref2[11] = 0x0,//0,0x35b
		.cfg_ddr_training_delay_ps.soc_bit_vref2[12] = 0x0,//0,0x35c
		.cfg_ddr_training_delay_ps.soc_bit_vref2[13] = 0x0,//0,0x35d
		.cfg_ddr_training_delay_ps.soc_bit_vref2[14] = 0x0,//0,0x35e
		.cfg_ddr_training_delay_ps.soc_bit_vref2[15] = 0x0,//0,0x35f
		.cfg_ddr_training_delay_ps.soc_bit_vref2[16] = 0x0,//0,0x360
		.cfg_ddr_training_delay_ps.soc_bit_vref2[17] = 0x0,//0,0x361
		.cfg_ddr_training_delay_ps.soc_bit_vref2[18] = 0x0,//0,0x362
		.cfg_ddr_training_delay_ps.soc_bit_vref2[19] = 0x0,//0,0x363
		.cfg_ddr_training_delay_ps.soc_bit_vref2[20] = 0x0,//0,0x364
		.cfg_ddr_training_delay_ps.soc_bit_vref2[21] = 0x0,//0,0x365
		.cfg_ddr_training_delay_ps.soc_bit_vref2[22] = 0x0,//0,0x366
		.cfg_ddr_training_delay_ps.soc_bit_vref2[23] = 0x0,//0,0x367
		.cfg_ddr_training_delay_ps.soc_bit_vref2[24] = 0x0,//0,0x368
		.cfg_ddr_training_delay_ps.soc_bit_vref2[25] = 0x0,//0,0x369
		.cfg_ddr_training_delay_ps.soc_bit_vref2[26] = 0x0,//0,0x36a
		.cfg_ddr_training_delay_ps.soc_bit_vref2[27] = 0x0,//0,0x36b
		.cfg_ddr_training_delay_ps.soc_bit_vref2[28] = 0x0,//0,0x36c
		.cfg_ddr_training_delay_ps.soc_bit_vref2[29] = 0x0,//0,0x36d
		.cfg_ddr_training_delay_ps.soc_bit_vref2[30] = 0x0,//0,0x36e
		.cfg_ddr_training_delay_ps.soc_bit_vref2[31] = 0x0,//0,0x36f
		.cfg_ddr_training_delay_ps.soc_bit_vref2[32] = 0x0,//0,0x370
		.cfg_ddr_training_delay_ps.soc_bit_vref2[33] = 0x0,//0,0x371
		.cfg_ddr_training_delay_ps.soc_bit_vref2[34] = 0x0,//0,0x372
		.cfg_ddr_training_delay_ps.soc_bit_vref2[35] = 0x0,//0,0x373
		.cfg_ddr_training_delay_ps.soc_bit_vref3[0] = 0x0,//0,0x374
		.cfg_ddr_training_delay_ps.soc_bit_vref3[1] = 0x0,//0,0x375
		.cfg_ddr_training_delay_ps.soc_bit_vref3[2] = 0x0,//0,0x376
		.cfg_ddr_training_delay_ps.soc_bit_vref3[3] = 0x0,//0,0x377
		.cfg_ddr_training_delay_ps.soc_bit_vref3[4] = 0x0,//0,0x378
		.cfg_ddr_training_delay_ps.soc_bit_vref3[5] = 0x0,//0,0x379
		.cfg_ddr_training_delay_ps.soc_bit_vref3[6] = 0x0,//0,0x37a
		.cfg_ddr_training_delay_ps.soc_bit_vref3[7] = 0x0,//0,0x37b
		.cfg_ddr_training_delay_ps.soc_bit_vref3[8] = 0x0,//0,0x37c
		.cfg_ddr_training_delay_ps.soc_bit_vref3[9] = 0x0,//0,0x37d
		.cfg_ddr_training_delay_ps.soc_bit_vref3[10] = 0x0,//0,0x37e
		.cfg_ddr_training_delay_ps.soc_bit_vref3[11] = 0x0,//0,0x37f
		.cfg_ddr_training_delay_ps.soc_bit_vref3[12] = 0x0,//0,0x380
		.cfg_ddr_training_delay_ps.soc_bit_vref3[13] = 0x0,//0,0x381
		.cfg_ddr_training_delay_ps.soc_bit_vref3[14] = 0x0,//0,0x382
		.cfg_ddr_training_delay_ps.soc_bit_vref3[15] = 0x0,//0,0x383
		.cfg_ddr_training_delay_ps.soc_bit_vref3[16] = 0x0,//0,0x384
		.cfg_ddr_training_delay_ps.soc_bit_vref3[17] = 0x0,//0,0x385
		.cfg_ddr_training_delay_ps.soc_bit_vref3[18] = 0x0,//0,0x386
		.cfg_ddr_training_delay_ps.soc_bit_vref3[19] = 0x0,//0,0x387
		.cfg_ddr_training_delay_ps.soc_bit_vref3[20] = 0x0,//0,0x388
		.cfg_ddr_training_delay_ps.soc_bit_vref3[21] = 0x0,//0,0x389
		.cfg_ddr_training_delay_ps.soc_bit_vref3[22] = 0x0,//0,0x38a
		.cfg_ddr_training_delay_ps.soc_bit_vref3[23] = 0x0,//0,0x38b
		.cfg_ddr_training_delay_ps.soc_bit_vref3[24] = 0x0,//0,0x38c
		.cfg_ddr_training_delay_ps.soc_bit_vref3[25] = 0x0,//0,0x38d
		.cfg_ddr_training_delay_ps.soc_bit_vref3[26] = 0x0,//0,0x38e
		.cfg_ddr_training_delay_ps.soc_bit_vref3[27] = 0x0,//0,0x38f
		.cfg_ddr_training_delay_ps.soc_bit_vref3[28] = 0x0,//0,0x390
		.cfg_ddr_training_delay_ps.soc_bit_vref3[29] = 0x0,//0,0x391
		.cfg_ddr_training_delay_ps.soc_bit_vref3[30] = 0x0,//0,0x392
		.cfg_ddr_training_delay_ps.soc_bit_vref3[31] = 0x0,//0,0x393
		.cfg_ddr_training_delay_ps.soc_bit_vref3[32] = 0x0,//0,0x394
		.cfg_ddr_training_delay_ps.soc_bit_vref3[33] = 0x0,//0,0x395
		.cfg_ddr_training_delay_ps.soc_bit_vref3[34] = 0x0,//0,0x396
		.cfg_ddr_training_delay_ps.soc_bit_vref3[35] = 0x0,//0,0x397
		.cfg_ddr_training_delay_ps.dram_vref[0] = 0x0,//0,0x398
		.cfg_ddr_training_delay_ps.dram_vref[1] = 0x0,//0,0x399
		.cfg_ddr_training_delay_ps.dram_vref[2] = 0x0,//0,0x39a
		.cfg_ddr_training_delay_ps.dram_vref[3] = 0x0,//0,0x39b
		.cfg_ddr_training_delay_ps.dram_vref[4] = 0x0,//0,0x39c
		.cfg_ddr_training_delay_ps.dram_vref[5] = 0x0,//0,0x39d
		.cfg_ddr_training_delay_ps.dram_vref[6] = 0x0,//0,0x39e
		.cfg_ddr_training_delay_ps.dram_vref[7] = 0x0,//0,0x39f
		.cfg_ddr_training_delay_ps.dram_vref[8] = 0x0,//0,0x3a0
		.cfg_ddr_training_delay_ps.dram_vref[9] = 0x0,//0,0x3a1
		.cfg_ddr_training_delay_ps.dram_vref[10] = 0x0,//0,0x3a2
		.cfg_ddr_training_delay_ps.dram_vref[11] = 0x0,//0,0x3a3
		.cfg_ddr_training_delay_ps.dram_vref[12] = 0x0,//0,0x3a4
		.cfg_ddr_training_delay_ps.dram_vref[13] = 0x0,//0,0x3a5
		.cfg_ddr_training_delay_ps.dram_vref[14] = 0x0,//0,0x3a6
		.cfg_ddr_training_delay_ps.dram_vref[15] = 0x0,//0,0x3a7
		.cfg_ddr_training_delay_ps.dram_vref[16] = 0x0,//0,0x3a8
		.cfg_ddr_training_delay_ps.dram_vref[17] = 0x0,//0,0x3a9
		.cfg_ddr_training_delay_ps.dram_vref[18] = 0x0,//0,0x3aa
		.cfg_ddr_training_delay_ps.dram_vref[19] = 0x0,//0,0x3ab
		.cfg_ddr_training_delay_ps.dram_vref[20] = 0x0,//0,0x3ac
		.cfg_ddr_training_delay_ps.dram_vref[21] = 0x0,//0,0x3ad
		.cfg_ddr_training_delay_ps.dram_vref[22] = 0x0,//0,0x3ae
		.cfg_ddr_training_delay_ps.dram_vref[23] = 0x0,//0,0x3af
		.cfg_ddr_training_delay_ps.dram_vref[24] = 0x0,//0,0x3b0
		.cfg_ddr_training_delay_ps.dram_vref[25] = 0x0,//0,0x3b1
		.cfg_ddr_training_delay_ps.dram_vref[26] = 0x0,//0,0x3b2
		.cfg_ddr_training_delay_ps.dram_vref[27] = 0x0,//0,0x3b3
		.cfg_ddr_training_delay_ps.dram_vref[28] = 0x0,//0,0x3b4
		.cfg_ddr_training_delay_ps.dram_vref[29] = 0x0,//0,0x3b5
		.cfg_ddr_training_delay_ps.dram_vref[30] = 0x0,//0,0x3b6
		.cfg_ddr_training_delay_ps.dram_vref[31] = 0x0,//0,0x3b7
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[0] = 0x0,//0,0x3b8
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[1] = 0x0,//0,0x3ba
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[2] = 0x0,//0,0x3bc
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[3] = 0x0,//0,0x3be
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[4] = 0x0,//0,0x3c0
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[5] = 0x0,//0,0x3c2
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[6] = 0x0,//0,0x3c4
		.cfg_ddr_training_delay_ps.dca_wck_tx_t[7] = 0x0,//0,0x3c6
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[0] = 0x0,//0,0x3c8
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[1] = 0x0,//0,0x3ca
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[2] = 0x0,//0,0x3cc
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[3] = 0x0,//0,0x3ce
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[4] = 0x0,//0,0x3d0
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[5] = 0x0,//0,0x3d2
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[6] = 0x0,//0,0x3d4
		.cfg_ddr_training_delay_ps.dca_wck_rx_t[7] = 0x0,//0,0x3d6
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[0] = 0x0,//0,0x3d8
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[1] = 0x0,//0,0x3da
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[2] = 0x0,//0,0x3dc
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[3] = 0x0,//0,0x3de
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[4] = 0x0,//0,0x3e0
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[5] = 0x0,//0,0x3e2
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[6] = 0x0,//0,0x3e4
		.cfg_ddr_training_delay_ps.dca_dqs_tx_t[7] = 0x0,//0,0x3e6
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[0] = 0x0,//0,0x3e8
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[1] = 0x0,//0,0x3ea
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[2] = 0x0,//0,0x3ec
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[3] = 0x0,//0,0x3ee
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[4] = 0x0,//0,0x3f0
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[5] = 0x0,//0,0x3f2
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[6] = 0x0,//0,0x3f4
		.cfg_ddr_training_delay_ps.dca_wck_tx_c[7] = 0x0,//0,0x3f6
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[0] = 0x0,//0,0x3f8
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[1] = 0x0,//0,0x3fa
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[2] = 0x0,//0,0x3fc
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[3] = 0x0,//0,0x3fe
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[4] = 0x0,//0,0x400
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[5] = 0x0,//0,0x402
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[6] = 0x0,//0,0x404
		.cfg_ddr_training_delay_ps.dca_wck_rx_c[7] = 0x0,//0,0x406
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[0] = 0x0,//0,0x408
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[1] = 0x0,//0,0x40a
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[2] = 0x0,//0,0x40c
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[3] = 0x0,//0,0x40e
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[4] = 0x0,//0,0x410
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[5] = 0x0,//0,0x412
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[6] = 0x0,//0,0x414
		.cfg_ddr_training_delay_ps.dca_dqs_tx_c[7] = 0x0,//0,0x416
		.cfg_ddr_training_delay_ps.dca_dq_tx[0] = 0x0,//0,0x418
		.cfg_ddr_training_delay_ps.dca_dq_tx[1] = 0x0,//0,0x41a
		.cfg_ddr_training_delay_ps.dca_dq_tx[2] = 0x0,//0,0x41c
		.cfg_ddr_training_delay_ps.dca_dq_tx[3] = 0x0,//0,0x41e
		.cfg_ddr_training_delay_ps.dca_dq_tx[4] = 0x0,//0,0x420
		.cfg_ddr_training_delay_ps.dca_dq_tx[5] = 0x0,//0,0x422
		.cfg_ddr_training_delay_ps.dca_dq_tx[6] = 0x0,//0,0x424
		.cfg_ddr_training_delay_ps.dca_dq_tx[7] = 0x0,//0,0x426
		.cfg_ddr_training_delay_ps.dfi_mrl[0] = 0x0,//0,0x428
		.cfg_ddr_training_delay_ps.dfi_mrl[1] = 0x0,//0,0x429
		.cfg_ddr_training_delay_ps.dfi_mrl[2] = 0x0,//0,0x42a
		.cfg_ddr_training_delay_ps.dfi_mrl[3] = 0x0,//0,0x42b
		.cfg_ddr_training_delay_ps.dfi_hwtmrl=0x0,//0,0x42c
		.cfg_ddr_training_delay_ps.csr_hwtctrl=0x0,//0,0x42d
		.cfg_ddr_training_delay_ps.rever1=0x0,//0,0x42e
		.cfg_ddr_training_delay_ps.dram_vref_offset=0x0,//0,0x42f
		.cfg_ddr_training_delay_ps.pptdqscnttg0[0] = 0x0,//0,0x430
		.cfg_ddr_training_delay_ps.pptdqscnttg0[1] = 0x0,//0,0x432
		.cfg_ddr_training_delay_ps.pptdqscnttg0[2] = 0x0,//0,0x434
		.cfg_ddr_training_delay_ps.pptdqscnttg0[3] = 0x0,//0,0x436
		.cfg_ddr_training_delay_ps.pptdqscnttg1[0] = 0x0,//0,0x438
		.cfg_ddr_training_delay_ps.pptdqscnttg1[1] = 0x0,//0,0x43a
		.cfg_ddr_training_delay_ps.pptdqscnttg1[2] = 0x0,//0,0x43c
		.cfg_ddr_training_delay_ps.pptdqscnttg1[3] = 0x0,//0,0x43e
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[0] = 0x0,//0,0x440
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[1] = 0x0,//0,0x442
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[2] = 0x0,//0,0x444
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg0[3] = 0x0,//0,0x446
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[0] = 0x0,//0,0x448
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[1] = 0x0,//0,0x44a
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[2] = 0x0,//0,0x44c
		.cfg_ddr_training_delay_ps.PptWck2DqoCntTg1[3] = 0x0,//0,0x44e
		.cfg_ddr_training_delay_ps.dac_offset[0] = 0x0,//0,0x478
		.cfg_ddr_training_delay_ps.dac_offset[1] = 0x0,//0,0x479
		.cfg_ddr_training_delay_ps.dac_offset[2] = 0x0,//0,0x47a
		.cfg_ddr_training_delay_ps.dac_offset[3] = 0x0,//0,0x47b
		.cfg_ddr_training_delay_ps.rx_offset[0] = 0x0,//0,0x47c
		.cfg_ddr_training_delay_ps.rx_offset[1] = 0x0,//0,0x47d
		.cfg_ddr_training_delay_ps.tx_offset[0] = 0x0,//0,0x47e
		.cfg_ddr_training_delay_ps.tx_offset[1] = 0x0,//0,0x47f
		.cfg_ddr_training_delay_ps.reserve_para[0] = 0x0,//0,0x480
		.cfg_ddr_training_delay_ps.reserve_para[1] = 0x0,//0,0x481
		.cfg_ddr_training_delay_ps.reserve_para[2] = 0x0,//0,0x482
		.cfg_ddr_training_delay_ps.reserve_para[3] = 0x0,//0,0x483
		.cfg_ddr_training_delay_ps.reserve_para[4] = 0x0,//0,0x484
		.cfg_ddr_training_delay_ps.reserve_para[5] = 0x0,//0,0x485
		.cfg_ddr_training_delay_ps.reserve_para[6] = 0x0,//0,0x486
		.cfg_ddr_training_delay_ps.reserve_para[7] = 0x0,//0,0x487
		.cfg_ddr_training_delay_ps.reserve_para[8] = 0x0,//0,0x488
		.cfg_ddr_training_delay_ps.reserve_para[9] = 0x0,//0,0x489
		.cfg_ddr_training_delay_ps.reserve_para[10] = 0x0,//0,0x48a
		.cfg_ddr_training_delay_ps.reserve_para[11] = 0x0,//0,0x48b
		.cfg_ddr_training_delay_ps.reserve_para[12] = 0x0,//0,0x48c
		.cfg_ddr_training_delay_ps.reserve_para[13] = 0x0,//0,0x48d
		.cfg_ddr_training_delay_ps.reserve_para[14] = 0x0,//0,0x48e
		.cfg_ddr_training_delay_ps.reserve_para[15] = 0x0,//0,0x48f
#endif
	},
//};
#endif
};
