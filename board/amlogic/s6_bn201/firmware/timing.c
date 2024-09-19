// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>
#include <ddr_timing.c>

/* board clk defines */
#define CPU_CLK                                 2004

/* board vmin_value defines */
#define VMIN_FF_VALUE                           670
#define VMIN_TT_VALUE                           690
#define VMIN_SS_VALUE                           710
/* board vddee_value defines */
/* FF/TT/SS = 740/750/780 mv */
#define VDDEE_FF_VALUE                          0x80005
#define VDDEE_TT_VALUE                          0x70006
#define VDDEE_SS_VALUE                          0x40009

board_clk_set_t __board_clk_setting
__attribute__ ((section(".clk_param"))) = {
	/* clock settings for bl2 */
	.cpu_clk	= CPU_CLK / 12 * 12,
#ifdef CONFIG_PXP_DDR
	.pxp = 1,
#else
	.pxp = 0,
#endif
	.low_console_baud = CONFIG_LOW_CONSOLE_BAUD,
};

#define VCCK_VAL_1                              AML_VCCK_INIT_VOLTAGE_1
#define VCCK_VAL_2                              AML_VCCK_INIT_VOLTAGE_2
#define VDDEE_VAL                               AML_VDDEE_INIT_VOLTAGE
/* BOARD_1 VCCK PWM table */
#if   (VCCK_VAL_1 == 979)
#define VCCK_VAL_REG_1    0x00003e8
#elif (VCCK_VAL_1 == 969)
#define VCCK_VAL_REG_1    0x02003c6
#elif (VCCK_VAL_1 == 959)
#define VCCK_VAL_REG_1    0x04103a5
#elif (VCCK_VAL_1 == 949)
#define VCCK_VAL_REG_1    0x0620384
#elif (VCCK_VAL_1 == 939)
#define VCCK_VAL_REG_1    0x0830363
#elif (VCCK_VAL_1 == 929)
#define VCCK_VAL_REG_1    0x0a40342
#elif (VCCK_VAL_1 == 919)
#define VCCK_VAL_REG_1    0x0c50321
#elif (VCCK_VAL_1 == 909)
#define VCCK_VAL_REG_1    0x0e60300
#elif (VCCK_VAL_1 == 899)
#define VCCK_VAL_REG_1    0x10702df
#elif (VCCK_VAL_1 == 889)
#define VCCK_VAL_REG_1    0x12802be
#elif (VCCK_VAL_1 == 879)
#define VCCK_VAL_REG_1    0x149029d
#elif (VCCK_VAL_1 == 869)
#define VCCK_VAL_REG_1    0x16a027c
#elif (VCCK_VAL_1 == 859)
#define VCCK_VAL_REG_1    0x18b025b
#elif (VCCK_VAL_1 == 849)
#define VCCK_VAL_REG_1    0x1ac023a
#elif (VCCK_VAL_1 == 839)
#define VCCK_VAL_REG_1    0x1cd0219
#elif (VCCK_VAL_1 == 829)
#define VCCK_VAL_REG_1    0x1ee01f8
#elif (VCCK_VAL_1 == 819)
#define VCCK_VAL_REG_1    0x21901cd
#elif (VCCK_VAL_1 == 809)
#define VCCK_VAL_REG_1    0x23a01ac
#elif (VCCK_VAL_1 == 799)
#define VCCK_VAL_REG_1    0x25b018b
#elif (VCCK_VAL_1 == 789)
#define VCCK_VAL_REG_1    0x27c016a
#elif (VCCK_VAL_1 == 779)
#define VCCK_VAL_REG_1    0x29d0149
#elif (VCCK_VAL_1 == 769)
#define VCCK_VAL_REG_1    0x2be0128
#elif (VCCK_VAL_1 == 759)
#define VCCK_VAL_REG_1    0x2df0107
#elif (VCCK_VAL_1 == 749)
#define VCCK_VAL_REG_1    0x30000e6
#elif (VCCK_VAL_1 == 739)
#define VCCK_VAL_REG_1    0x32100c5
#elif (VCCK_VAL_1 == 729)
#define VCCK_VAL_REG_1    0x34200a4
#elif (VCCK_VAL_1 == 719)
#define VCCK_VAL_REG_1    0x3630083
#elif (VCCK_VAL_1 == 709)
#define VCCK_VAL_REG_1    0x3840062
#elif (VCCK_VAL_1 == 699)
#define VCCK_VAL_REG_1    0x3a50041
#elif (VCCK_VAL_1 == 689)
#define VCCK_VAL_REG_1    0x3c60020
#elif (VCCK_VAL_1 == 679)
#define VCCK_VAL_REG_1    0x3e80000
#else
#error "VCCK val out of range\n"
#endif

/* BOARD_2 VCCK PWM table */
#if   (VCCK_VAL_2 == 1029)
#define VCCK_VAL_REG_2    0x00003e8
#elif (VCCK_VAL_2 == 1019)
#define VCCK_VAL_REG_2    0x01D03C9
#elif (VCCK_VAL_2 == 1009)
#define VCCK_VAL_REG_2    0x03B03AB
#elif (VCCK_VAL_2 == 999)
#define VCCK_VAL_REG_2    0x059038D
#elif (VCCK_VAL_2 == 989)
#define VCCK_VAL_REG_2    0x077036F
#elif (VCCK_VAL_2 == 979)
#define VCCK_VAL_REG_2    0x08B035B
#elif (VCCK_VAL_2 == 969)
#define VCCK_VAL_REG_2    0x0A9033D
#elif (VCCK_VAL_2 == 959)
#define VCCK_VAL_REG_2    0x0C7031F
#elif (VCCK_VAL_2 == 949)
#define VCCK_VAL_REG_2    0x0E50301
#elif (VCCK_VAL_2 == 939)
#define VCCK_VAL_REG_2    0x10302E3
#elif (VCCK_VAL_2 == 929)
#define VCCK_VAL_REG_2    0x12102C5
#elif (VCCK_VAL_2 == 919)
#define VCCK_VAL_REG_2    0x13502B1
#elif (VCCK_VAL_2 == 909)
#define VCCK_VAL_REG_2    0x1530293
#elif (VCCK_VAL_2 == 899)
#define VCCK_VAL_REG_2    0x1710275
#elif (VCCK_VAL_2 == 889)
#define VCCK_VAL_REG_2    0x18F0257
#elif (VCCK_VAL_2 == 879)
#define VCCK_VAL_REG_2    0x1AD0239
#elif (VCCK_VAL_2 == 869)
#define VCCK_VAL_REG_2    0x1C10225
#elif (VCCK_VAL_2 == 859)
#define VCCK_VAL_REG_2    0x1DF0207
#elif (VCCK_VAL_2 == 849)
#define VCCK_VAL_REG_2    0x1FD01E9
#elif (VCCK_VAL_2 == 839)
#define VCCK_VAL_REG_2    0x21B01CB
#elif (VCCK_VAL_2 == 829)
#define VCCK_VAL_REG_2    0x22F01B7
#elif (VCCK_VAL_2 == 819)
#define VCCK_VAL_REG_2    0x24D0199
#elif (VCCK_VAL_2 == 809)
#define VCCK_VAL_REG_2    0x26B017B
#elif (VCCK_VAL_2 == 799)
#define VCCK_VAL_REG_2    0x289015D
#elif (VCCK_VAL_2 == 789)
#define VCCK_VAL_REG_2    0x2A7013F
#elif (VCCK_VAL_2 == 779)
#define VCCK_VAL_REG_2    0x2BB012B
#elif (VCCK_VAL_2 == 769)
#define VCCK_VAL_REG_2    0x2D9010D
#elif (VCCK_VAL_2 == 759)
#define VCCK_VAL_REG_2    0x2F700EF
#elif (VCCK_VAL_2 == 749)
#define VCCK_VAL_REG_2    0x31500D1
#elif (VCCK_VAL_2 == 739)
#define VCCK_VAL_REG_2    0x32900BD
#elif (VCCK_VAL_2 == 729)
#define VCCK_VAL_REG_2    0x347009F
#elif (VCCK_VAL_2 == 719)
#define VCCK_VAL_REG_2    0x3650081
#elif (VCCK_VAL_2 == 709)
#define VCCK_VAL_REG_2    0x379006D
#elif (VCCK_VAL_2 == 699)
#define VCCK_VAL_REG_2    0x397004F
#elif (VCCK_VAL_2 == 689)
#define VCCK_VAL_REG_2    0x3B50031
#elif (VCCK_VAL_2 == 679)
#define VCCK_VAL_REG_2    0x3D30013
#elif (VCCK_VAL_2 == 669)
#define VCCK_VAL_REG_2    0x3E80000
#else
#error "VCCK val out of range\n"
#endif

/* VDDEE_VAL_REG */
#if   (VDDEE_VAL == 679)
#define VDDEE_VAL_REG   0xf0000
#elif (VDDEE_VAL == 689)
#define VDDEE_VAL_REG   0xd0000
#elif (VDDEE_VAL == 699)
#define VDDEE_VAL_REG   0xc0001
#elif (VDDEE_VAL == 709)
#define VDDEE_VAL_REG   0xb0002
#elif (VDDEE_VAL == 719)
#define VDDEE_VAL_REG   0xa0003
#elif (VDDEE_VAL == 729)
#define VDDEE_VAL_REG   0x90004
#elif (VDDEE_VAL == 739)
#define VDDEE_VAL_REG   0x80005
#elif (VDDEE_VAL == 749)
#define VDDEE_VAL_REG   0x70006
#elif (VDDEE_VAL == 759)
#define VDDEE_VAL_REG   0x60007
#elif (VDDEE_VAL == 769)
#define VDDEE_VAL_REG   0x50008
#elif (VDDEE_VAL == 779)
#define VDDEE_VAL_REG   0x40009
#elif (VDDEE_VAL == 789)
#define VDDEE_VAL_REG   0x3000a
#elif (VDDEE_VAL == 799)
#define VDDEE_VAL_REG   0x2000b
#elif (VDDEE_VAL == 809)
#define VDDEE_VAL_REG   0x1000c
#elif (VDDEE_VAL == 819)
#define VDDEE_VAL_REG   0x0000d
#elif (VDDEE_VAL == 829)
#define VDDEE_VAL_REG   0x0000f
#else
#error "VDDEE val out of range\n"
#endif

bl2_reg_t __bl2_reg[] __attribute__ ((section(".generic_param"))) = {
	//need fine tune
	{ 0, 0, 0xffffffff, 0, 0, 0 },
};

/* gpio/pinmux/pwm init */
register_ops_t __bl2_ops_reg[MAX_REG_OPS_ENTRIES]
__attribute__ ((section(".misc_param"))) = {
	/* config vmin_ft value */
	{ 0, VMIN_SS_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VMIN_FLAG_1, 0 },
	{ 0, VMIN_TT_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VMIN_FLAG_2, 0 },
	{ 0, VMIN_FF_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VMIN_FLAG_3, 0 },
#ifdef CONFIG_PDVFS_ENABLE
	{ PWM_PWM_D, VDDEE_SS_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_1, 0 },
	{ PWM_PWM_D, VDDEE_TT_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_2, 0 },
	{ PWM_PWM_D, VDDEE_FF_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_3, 0 },
#else
	{ PWM_PWM_D,		   VDDEE_VAL_REG, 0xffffffff, 0, 0, 0},
#endif
	/* DDR reg add multiplexing cpu macros by bl2_ops_reg size issue*/
	{ PWM_PWM_F, VCCK_VAL_REG_1, 0xffffffff, 0, BL2_INIT_CPU_VDDCORE_CONFIG_1, 0 },
	{ PWM_PWM_A, VCCK_VAL_REG_2, 0xffffffff, 0, BL2_INIT_CPU_VDDCORE_CONFIG_2, 0 },
	{ PWM_MISC_REG_D,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_F,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_A,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	/* set pwm a, pwm d and pwm f clock rate to 24M, 24M,666M, enable them */
	{ CLKCTRL_PWM_CLK_AB_CTRL, (0x1 << 8), (0x1 << 8), 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_CD_CTRL, (0x1 << 24), (0x1 << 24), 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_EF_CTRL, (0x1 << 24) | (0x3 << 25), (0x3 << 25), 0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 drive strength to 3 */
	{ PADCTRL_GPIOE_DS,	   0xf,		  0xf,	      0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 GPIOE_2 mux to pwmd pwmf pwma */
	{ PADCTRL_PIN_MUX_REGI,	   (0x3 << 0) | (0x3 << 4) | (0x2 << 8) ,	  (0xf << 0) | (0xf << 4) | (0xf << 4), 0, 0, 0 },
};

#define __section(x)    __attribute__((__section__(x)))
/* for all the storage parameter */
#ifdef CONFIG_MTD_SPI_NAND
/* for spinand storage parameter */
storage_parameter_t __store_para __section(".store_param") = {
	.common				= {
		.version = 0x01,
		.device_fip_container_size = CONFIG_TPL_SIZE_PER_COPY,
		.device_fip_container_copies = ((CONFIG_NAND_TPL_COPY_NUM) |
						   (CONFIG_BL2_COPY_NUM << 16) |
						   (BOARD_CONFIG_BL2_LAYOUT_TYPE << 24)),
		.ddr_fip_container_size = BOOTLOADER_DDR_FIP_SIZE,
	},
	.nand				= {
		.version = 0x01,
		.bbt_pages = 1, // TODO: BL2E BBT
		.bbt_start_block = 20,
		.discrete_mode = 1,
		.setup_data.spi_nand_page_size = 2048,
		.reserved.spi_nand_planes_per_lun = 1,
		.reserved_area_blk_cnt = MTD_RSV_BLOCK_CNT,
		.page_per_block = 64,
		.use_param_page_list = 0,
	},
};
#else
storage_parameter_t __store_para __attribute__ ((section(".store_param"))) = {
	.common					= {
		.version			= 0x01,
		.device_fip_container_size  = CONFIG_TPL_SIZE_PER_COPY,
		.device_fip_container_copies    = ((CONFIG_NAND_TPL_COPY_NUM) |
						   (CONFIG_BL2_COPY_NUM << 16) |
						   (BOARD_CONFIG_BL2_LAYOUT_TYPE << 24)),
		.ddr_fip_container_size		= BOOTLOADER_DDR_FIP_SIZE,
	},
	.nand					= {
		.version			= 0x01,
		.bbt_pages			= 0x1,
		.bbt_start_block		= 20,
		.discrete_mode			= 1,
		.setup_data.nand_setup_data = (2 << 20) |		    \
						  (0 << 19) |			  \
						  (1 << 17) |			  \
						  (1 << 14) |			  \
						  (0 << 13) |			  \
						  (64 << 6) |			  \
						  (8 << 0),
		.reserved_area_blk_cnt		= MTD_RSV_BLOCK_CNT,
		.page_per_block			= 64,
		.use_param_page_list		= 0,
	},
};
#endif
