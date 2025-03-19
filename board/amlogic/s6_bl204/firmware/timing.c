// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>
#include <ddr_timing.c>

/* board clk defines */
#define CPU_CLK                                 2400

/* board vmin_value defines */
#define VMIN_FT_GROUP3							680
#define VMIN_FT_GROUP2							710
#define VMIN_FT_GROUP1							730
/* board vddee_pwm_value defines */
#define VDDEE_PWM_VAL_GROUP3					0x60007
#define VDDEE_PWM_VAL_GROUP2					0x40009
#define VDDEE_PWM_VAL_GROUP1					0x40009
/* board vddddr_pwm_value defines */
#define VDDDDR_PWM__VAL_GROUP3					0x60007
#define VDDDDR_PWM__VAL_GROUP2					0x40009
#define VDDDDR_PWM__VAL_GROUP1					0x40009

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
#define VDDDDR_VAL                              AML_VDDDDR_INIT_VOLTAGE

/* BOARD_1 VCCK PWM table */
#if (VCCK_VAL_1 == 969)
#define VCCK_VAL_REG_1  0x0000001c
#elif (VCCK_VAL_1 == 959)
#define VCCK_VAL_REG_1  0x0001001b
#elif (VCCK_VAL_1 == 949)
#define VCCK_VAL_REG_1  0x0002001a
#elif (VCCK_VAL_1 == 939)
#define VCCK_VAL_REG_1  0x00030019
#elif (VCCK_VAL_1 == 929)
#define VCCK_VAL_REG_1  0x00040018
#elif (VCCK_VAL_1 == 919)
#define VCCK_VAL_REG_1  0x00050017
#elif (VCCK_VAL_1 == 909)
#define VCCK_VAL_REG_1  0x00060016
#elif (VCCK_VAL_1 == 899)
#define VCCK_VAL_REG_1  0x00070015
#elif (VCCK_VAL_1 == 889)
#define VCCK_VAL_REG_1  0x00080014
#elif (VCCK_VAL_1 == 879)
#define VCCK_VAL_REG_1  0x00090013
#elif (VCCK_VAL_1 == 869)
#define VCCK_VAL_REG_1  0x000a0012
#elif (VCCK_VAL_1 == 859)
#define VCCK_VAL_REG_1  0x000b0011
#elif (VCCK_VAL_1 == 849)
#define VCCK_VAL_REG_1  0x000c0010
#elif (VCCK_VAL_1 == 839)
#define VCCK_VAL_REG_1  0x000d000f
#elif (VCCK_VAL_1 == 829)
#define VCCK_VAL_REG_1  0x000e000e
#elif (VCCK_VAL_1 == 819)
#define VCCK_VAL_REG_1  0x000f000d
#elif (VCCK_VAL_1 == 809)
#define VCCK_VAL_REG_1  0x0010000c
#elif (VCCK_VAL_1 == 799)
#define VCCK_VAL_REG_1  0x0011000b
#elif (VCCK_VAL_1 == 789)
#define VCCK_VAL_REG_1  0x0012000a
#elif (VCCK_VAL_1 == 779)
#define VCCK_VAL_REG_1  0x00130009
#elif (VCCK_VAL_1 == 769)
#define VCCK_VAL_REG_1  0x00140008
#elif (VCCK_VAL_1 == 759)
#define VCCK_VAL_REG_1  0x00150007
#elif (VCCK_VAL_1 == 749)
#define VCCK_VAL_REG_1  0x00160006
#elif (VCCK_VAL_1 == 739)
#define VCCK_VAL_REG_1  0x00170005
#elif (VCCK_VAL_1 == 729)
#define VCCK_VAL_REG_1  0x00180004
#elif (VCCK_VAL_1 == 719)
#define VCCK_VAL_REG_1  0x00190003
#elif (VCCK_VAL_1 == 709)
#define VCCK_VAL_REG_1  0x001a0002
#elif (VCCK_VAL_1 == 699)
#define VCCK_VAL_REG_1  0x001b0001
#elif (VCCK_VAL_1 == 689)
#define VCCK_VAL_REG_1  0x001c0000
#else
#error "VCCK val out of range\n"
#endif

/* BOARD_2 VCCK PWM table */
#if (VCCK_VAL_2 == 1019)
#define VCCK_VAL_REG_2  0x00000022
#elif (VCCK_VAL_2 == 1009)
#define VCCK_VAL_REG_2  0x00010021
#elif (VCCK_VAL_2 == 999)
#define VCCK_VAL_REG_2  0x00020020
#elif (VCCK_VAL_2 == 989)
#define VCCK_VAL_REG_2  0x0003001f
#elif (VCCK_VAL_2 == 979)
#define VCCK_VAL_REG_2  0x0004001e
#elif (VCCK_VAL_2 == 969)
#define VCCK_VAL_REG_2  0x0005001d
#elif (VCCK_VAL_2 == 959)
#define VCCK_VAL_REG_2  0x0006001c
#elif (VCCK_VAL_2 == 949)
#define VCCK_VAL_REG_2  0x0007001b
#elif (VCCK_VAL_2 == 939)
#define VCCK_VAL_REG_2  0x0008001a
#elif (VCCK_VAL_2 == 929)
#define VCCK_VAL_REG_2  0x00090019
#elif (VCCK_VAL_2 == 919)
#define VCCK_VAL_REG_2  0x000a0018
#elif (VCCK_VAL_2 == 909)
#define VCCK_VAL_REG_2  0x000b0017
#elif (VCCK_VAL_2 == 899)
#define VCCK_VAL_REG_2  0x000c0016
#elif (VCCK_VAL_2 == 889)
#define VCCK_VAL_REG_2  0x000d0015
#elif (VCCK_VAL_2 == 879)
#define VCCK_VAL_REG_2  0x000e0014
#elif (VCCK_VAL_2 == 869)
#define VCCK_VAL_REG_2  0x000f0013
#elif (VCCK_VAL_2 == 859)
#define VCCK_VAL_REG_2  0x00100012
#elif (VCCK_VAL_2 == 849)
#define VCCK_VAL_REG_2  0x00110011
#elif (VCCK_VAL_2 == 839)
#define VCCK_VAL_REG_2  0x00120010
#elif (VCCK_VAL_2 == 829)
#define VCCK_VAL_REG_2  0x0013000f
#elif (VCCK_VAL_2 == 819)
#define VCCK_VAL_REG_2  0x0014000e
#elif (VCCK_VAL_2 == 809)
#define VCCK_VAL_REG_2  0x0015000d
#elif (VCCK_VAL_2 == 799)
#define VCCK_VAL_REG_2  0x0016000c
#elif (VCCK_VAL_2 == 789)
#define VCCK_VAL_REG_2  0x0017000b
#elif (VCCK_VAL_2 == 779)
#define VCCK_VAL_REG_2  0x0018000a
#elif (VCCK_VAL_2 == 769)
#define VCCK_VAL_REG_2  0x00190009
#elif (VCCK_VAL_2 == 759)
#define VCCK_VAL_REG_2  0x001a0008
#elif (VCCK_VAL_2 == 749)
#define VCCK_VAL_REG_2  0x001b0007
#elif (VCCK_VAL_2 == 739)
#define VCCK_VAL_REG_2  0x001c0006
#elif (VCCK_VAL_2 == 729)
#define VCCK_VAL_REG_2  0x001d0005
#elif (VCCK_VAL_2 == 719)
#define VCCK_VAL_REG_2  0x001e0004
#elif (VCCK_VAL_2 == 709)
#define VCCK_VAL_REG_2  0x001f0003
#elif (VCCK_VAL_2 == 699)
#define VCCK_VAL_REG_2  0x00200002
#elif (VCCK_VAL_2 == 689)
#define VCCK_VAL_REG_2  0x00210001
#elif (VCCK_VAL_2 == 679)
#define VCCK_VAL_REG_2  0x00220000
#else
#error "VCCK val out of range\n"
#endif

/* VDDEE_VAL_REG */
#if   (VDDEE_VAL == 680)
#define VDDEE_VAL_REG   0xf0000
#elif (VDDEE_VAL == 690)
#define VDDEE_VAL_REG   0xd0000
#elif (VDDEE_VAL == 700)
#define VDDEE_VAL_REG   0xc0001
#elif (VDDEE_VAL == 710)
#define VDDEE_VAL_REG   0xb0002
#elif (VDDEE_VAL == 720)
#define VDDEE_VAL_REG   0xa0003
#elif (VDDEE_VAL == 730)
#define VDDEE_VAL_REG   0x90004
#elif (VDDEE_VAL == 740)
#define VDDEE_VAL_REG   0x80005
#elif (VDDEE_VAL == 750)
#define VDDEE_VAL_REG   0x70006
#elif (VDDEE_VAL == 760)
#define VDDEE_VAL_REG   0x60007
#elif (VDDEE_VAL == 770)
#define VDDEE_VAL_REG   0x50008
#elif (VDDEE_VAL == 780)
#define VDDEE_VAL_REG   0x40009
#elif (VDDEE_VAL == 790)
#define VDDEE_VAL_REG   0x3000a
#elif (VDDEE_VAL == 800)
#define VDDEE_VAL_REG   0x2000b
#elif (VDDEE_VAL == 810)
#define VDDEE_VAL_REG   0x1000c
#elif (VDDEE_VAL == 820)
#define VDDEE_VAL_REG   0x0000d
#elif (VDDEE_VAL == 830)
#define VDDEE_VAL_REG   0x0000f
#else
#error "VDDEE val out of range\n"
#endif

/* VDDDDR_VAL_REG */
#if   (VDDDDR_VAL == 680)
#define VDDDDR_VAL_REG   0xf0000
#elif (VDDDDR_VAL == 690)
#define VDDDDR_VAL_REG   0xd0000
#elif (VDDDDR_VAL == 700)
#define VDDDDR_VAL_REG   0xc0001
#elif (VDDDDR_VAL == 710)
#define VDDDDR_VAL_REG   0xb0002
#elif (VDDDDR_VAL == 720)
#define VDDDDR_VAL_REG   0xa0003
#elif (VDDDDR_VAL == 730)
#define VDDDDR_VAL_REG   0x90004
#elif (VDDDDR_VAL == 740)
#define VDDDDR_VAL_REG   0x80005
#elif (VDDDDR_VAL == 750)
#define VDDDDR_VAL_REG   0x70006
#elif (VDDDDR_VAL == 760)
#define VDDDDR_VAL_REG   0x60007
#elif (VDDDDR_VAL == 770)
#define VDDDDR_VAL_REG   0x50008
#elif (VDDDDR_VAL == 780)
#define VDDDDR_VAL_REG   0x40009
#elif (VDDDDR_VAL == 790)
#define VDDDDR_VAL_REG   0x3000a
#elif (VDDDDR_VAL == 800)
#define VDDDDR_VAL_REG   0x2000b
#elif (VDDDDR_VAL == 810)
#define VDDDDR_VAL_REG   0x1000c
#elif (VDDDDR_VAL == 820)
#define VDDDDR_VAL_REG   0x0000d
#elif (VDDDDR_VAL == 830)
#define VDDDDR_VAL_REG   0x0000f
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
	{ 0, VMIN_FT_GROUP1, 0xffffffff, 0, BL2_VMIN_FT_FLAG_1, 0 },
	{ 0, VMIN_FT_GROUP2, 0xffffffff, 0, BL2_VMIN_FT_FLAG_2, 0 },
	{ 0, VMIN_FT_GROUP3, 0xffffffff, 0, BL2_VMIN_FT_FLAG_3, 0 },
#ifdef CONFIG_PDVFS_ENABLE
	{ PWM_PWM_D, VDDEE_PWM_VAL_GROUP1, 0xffffffff, 0, BL2_VDDEE_PWM_VAL_FLAG_1, 0 },
	{ PWM_PWM_A, VDDDDR_PWM__VAL_GROUP1, 0xffffffff, 0, BL2_VDDDDR_PWM_VAL_FLAG_1, 0 },
	{ PWM_PWM_D, VDDEE_PWM_VAL_GROUP2, 0xffffffff, 0, BL2_VDDEE_PWM_VAL_FLAG_2, 0 },
	{ PWM_PWM_A, VDDDDR_PWM__VAL_GROUP2, 0xffffffff, 0, BL2_VDDDDR_PWM_VAL_FLAG_2, 0 },
	{ PWM_PWM_D, VDDEE_PWM_VAL_GROUP3, 0xffffffff, 0, BL2_VDDEE_PWM_VAL_FLAG_3, 0 },
	{ PWM_PWM_A, VDDDDR_PWM__VAL_GROUP3, 0xffffffff, 0, BL2_VDDDDR_PWM_VAL_FLAG_3, 0 },
#else
	{ PWM_PWM_D,		   VDDEE_VAL_REG, 0xffffffff, 0, 0, 0},
	{ PWM_PWM_A,		   VDDDDR_VAL_REG, 0xffffffff, 0, 0, 0},
#endif
	/* DDR reg add multiplexing cpu macros by bl2_ops_reg size issue*/
	{ PWM_PWM_F, VCCK_VAL_REG_1, 0xffffffff, 0, BL2_VDDCPU_PWM_VAL_FLAG_1, 0 },
	{ PWM_PWM_F, VCCK_VAL_REG_2, 0xffffffff, 0, BL2_VDDCPU_PWM_VAL_FLAG_2, 0 },
	{ PWM_MISC_REG_D,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_F,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_A,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	/* set pwm a, pwm d and pwm f clock rate to 24M, 24M,24M, enable them */
	{ CLKCTRL_PWM_CLK_AB_CTRL, (0x1 << 8), (0x1 << 8), 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_CD_CTRL, (0x1 << 24), (0x1 << 24), 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_EF_CTRL, (0x1 << 24), (0x1 << 24), 0, 0, 0 },
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
