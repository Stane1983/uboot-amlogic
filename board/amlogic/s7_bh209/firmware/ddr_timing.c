// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>

//bit 6 adc_channel bit 0-5 adc value,chan 3 value 8 is layer 2
#define DDR_ID_ACS_ADC   ((3 << 6) | (8))

#define DDR_RESV_CHECK_ID_ENABLE  0Xfe
#define SAR_ADC_DDR_ID_BASE   0
#define SAR_ADC_DDR_ID_STEP   80
#define CARAMEL_BOARD_1G_1G_ADC_ID   \
	SAR_ADC_DDR_ID_BASE + SAR_ADC_DDR_ID_STEP	//85  0-125 step 0
#define CARAMEL_BOARD_2G_1G_ADC_ID   \
	SAR_ADC_DDR_ID_BASE + SAR_ADC_DDR_ID_STEP + SAR_ADC_DDR_ID_STEP	//167 126-200 step 1
#define DDR_TIMMING_OFFSET(X)  \
	(unsigned int)(unsigned long)(&(((ddr_set_ps0_only_t *)(0))->X))
#define DDR_TIMMING_OFFSET_SIZE(X) sizeof(((ddr_set_ps0_only_t *)(0))->X)
#define DDR_TIMMING_TUNE_TIMMING0(DDR_ID, PARA, VALUE)  \
	{DDR_ID, DDR_TIMMING_OFFSET(PARA), \
	VALUE, DDR_TIMMING_OFFSET_SIZE(PARA), 0, DDR_RESV_CHECK_ID_ENABLE }
#define DDR_TIMMING_TUNE_TIMMING1(DDR_ID, PARA, VALUE) \
	{DDR_ID, sizeof(((ddr_set_ps0_only_t) + DDR_TIMMING_OFFSET(PARA),\
	VALUE, DDR_TIMMING_OFFSET_SIZE(PARA), 0, DDR_RESV_CHECK_ID_ENABLE }

//bit24-31 define ID and size
#define DDR_ID_FROM_EFUSE  (0Xff << 24)
#define DDR_ID_FROM_ADC  (0Xfeu << 24)
#define DDR_ID_FROM_GPIO_CONFIG1  (0Xfd << 24)
#define DDR_ID_START_MASK  (0XFFDDCCBB)

#define DDR_ADC_CH0  (0X0 << 6)
#define DDR_ADC_CH1  (0X1 << 6)
#define DDR_ADC_CH2  (0X2 << 6)
#define DDR_ADC_CH3  (0X3 << 6)

#define DDR_ADC_VALUE0  (0X0 << 0)
#define DDR_ADC_VALUE1  (0X1 << 0)
#define DDR_ADC_VALUE2  (0X2 << 0)
#define DDR_ADC_VALUE3  (0X3 << 0)
#define DDR_ADC_VALUE4  (0X4 << 0)
#define DDR_ADC_VALUE5  (0X5 << 0)
#define DDR_ADC_VALUE6  (0X6 << 0)
#define DDR_ADC_VALUE7  (0X7 << 0)
#define DDR_ADC_VALUE8  (0X8 << 0)
#define DDR_ADC_VALUE9  (0X9 << 0)
#define DDR_ADC_VALUE10  (0Xa << 0)
#define DDR_ADC_VALUE11  (0Xb << 0)
#define DDR_ADC_VALUE12  (0Xc << 0)
#define DDR_ADC_VALUE13 (0Xd << 0)
#define DDR_ADC_VALUE14  (0Xe << 0)
#define DDR_ADC_VALUE15  (0Xf << 0)

typedef struct ddr_para_data {
	/*bit0-23 ddr_id value,bit 24-31 ddr_id source,
	 * 0xfe source from adc, 0xfd source from gpio_default_config
	 */
	//start from    DDR_ID_START_MASK,ddr_id;

    //bit 0-15 parameter offset value,bit16-23 overrid size,bit24-31 mux ddr_id source
	//reg_offset
	//unsigned int  reg_offset;
	//unsigned int  value;

	//bit0-15 only support data size =1byte or 2bytes,no support int value
	uint32_t value:16;  //bit0-15
	uint32_t reg_offset:12;	//bit16-27
	uint32_t data_size:4;	//bit28-31 if data size =15,then  will mean DDR_ID start

} ddr_para_data_t;

typedef struct ddr_para_data_start {
	uint32_t id_value:24;	//bit0-23  efuse id or ddr id
	//uint32_t      id_adc_ch : 2;//bit6-7
	uint32_t id_src_from:8;	//bit24-31 ddr id from adc or gpio
} ddr_para_data_start_t;
/*#define DDR_TIMMING_OFFSET(X)  \
 *	(unsigned int)(unsigned long)(&(((ddr_set_ps0_only_t *)(0))->X))
 *#define DDR_TIMMING_OFFSET_SIZE(X) sizeof(((ddr_set_ps0_only_t *)(0))->X)
 *#define DDR_TIMMING_TUNE_TIMMING0(DDR_ID, PARA, VALUE) \
 *	{ DDR_ID, DDR_TIMMING_OFFSET(PARA), VALUE, DDR_TIMMING_OFFSET_SIZE(PARA), \
 *	0, DDR_RESV_CHECK_ID_ENABLE }
 *#define DDR_TIMMING_TUNE_TIMMING1(DDR_ID, PARA, VALUE)  \
 * { DDR_ID, sizeof(((ddr_set_ps0_only_t)+ DDR_TIMMING_OFFSET(PARA), VALUE, \
 *  DDR_TIMMING_OFFSET_SIZE(PARA), 0, DDR_RESV_CHECK_ID_ENABLE }
 */
#define DDR_TIMMING_TUNE_TIMMING0_F(PARA, VALUE)  \
	(((DDR_TIMMING_OFFSET(PARA)) << 16) | ((DDR_TIMMING_OFFSET_SIZE(PARA)) << 28) | (VALUE))
#define DDR_TIMMING_TUNE_TIMMING1_F(PARA, VALUE) \
	(((sizeof(ddr_set_ps0_only_t) + DDR_TIMMING_OFFSET(PARA)) << 16) | \
	((DDR_TIMMING_OFFSET_SIZE(PARA)) << 28) | (VALUE))

#define DDR_TIMMING_TUNE_START(id_src_from, id_adc_ch, id_value) \
	((id_src_from) | (id_adc_ch) | (id_value))
#define DDR_TIMMING_TUNE_STRUCT_SIZE(a)  sizeof(a)

#if 1
uint32_t __bl2_ddr_reg_data[] __attribute__ ((section(".ddr_2acs_data"))) = {
	DDR_TIMMING_TUNE_START(DDR_ID_FROM_ADC, DDR_ADC_CH1, DDR_ADC_VALUE1),
	//data start
	DDR_TIMMING_TUNE_TIMMING0_F(cfg_board_common_setting.Is2Ttiming, CONFIG_USE_DDR_2T_MODE),
};

////_ddr_para_2nd_setting

uint32_t __ddr_parameter_reg_index[] __attribute__ ((section(".ddr_2acs_index"))) = {
	DDR_ID_START_MASK,
	DDR_TIMMING_TUNE_STRUCT_SIZE(__bl2_ddr_reg_data),
	//0,
};
#endif

ddr_set_ps0_only_t __ddr_setting[] __attribute__ ((section(".ddr_param"))) = {
{{0,}},
//{ 0,},
};
