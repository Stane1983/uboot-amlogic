// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>
#include <ddr_timing.c>

/* board clk defines */
#define CPU_CLK                                 1500

/* board vmin_value defines */
#define VMIN_FF_VALUE                           740
#define VMIN_TT_VALUE                           780
#define VMIN_SS_VALUE                           820
/* board vddee_value defines */
/* FF/TT/SS=0.78/0.82/0.86 */
#define VDDEE_FF_VALUE                          0xa0008
#define VDDEE_TT_VALUE                          0x6000c
#define VDDEE_SS_VALUE                          0x20010


board_clk_set_t __board_clk_setting
__attribute__ ((section(".clk_param"))) = {
	/* clock settings for bl2 */
	.cpu_clk	= CPU_CLK / 24 * 24,
#ifdef CONFIG_PXP_DDR
	.pxp = 1,
#else
	.pxp = 0,
#endif
	.low_console_baud = CONFIG_LOW_CONSOLE_BAUD,
};

#define VCCK_VAL                                AML_VCCK_INIT_VOLTAGE
#define VDDEE_VAL                               AML_VDDEE_INIT_VOLTAGE
/* VCCK PWM table */
#if   (VCCK_VAL == 1049)
#define VCCK_VAL_REG    0x3e8
#elif (VCCK_VAL == 1039)
#define VCCK_VAL_REG    0x1D03C9
#elif (VCCK_VAL == 1029)
#define VCCK_VAL_REG    0x3B03AB
#elif (VCCK_VAL == 1019)
#define VCCK_VAL_REG    0x59038D
#elif (VCCK_VAL == 1009)
#define VCCK_VAL_REG    0x77036F
#elif (VCCK_VAL == 999)
#define VCCK_VAL_REG    0x8B035B
#elif (VCCK_VAL == 989)
#define VCCK_VAL_REG    0xA9033D
#elif (VCCK_VAL == 979)
#define VCCK_VAL_REG    0xC7031F
#elif (VCCK_VAL == 969)
#define VCCK_VAL_REG    0xE50301
#elif (VCCK_VAL == 959)
#define VCCK_VAL_REG    0xF902ED
#elif (VCCK_VAL == 949)
#define VCCK_VAL_REG    0x11702CF
#elif (VCCK_VAL == 939)
#define VCCK_VAL_REG    0x13502B1
#elif (VCCK_VAL == 929)
#define VCCK_VAL_REG    0x1530293
#elif (VCCK_VAL == 919)
#define VCCK_VAL_REG    0x1710275
#elif (VCCK_VAL == 909)
#define VCCK_VAL_REG    0x1850261
#elif (VCCK_VAL == 899)
#define VCCK_VAL_REG    0x1A30243
#elif (VCCK_VAL == 889)
#define VCCK_VAL_REG    0x1C10225
#elif (VCCK_VAL == 879)
#define VCCK_VAL_REG    0x1DF0207
#elif (VCCK_VAL == 869)
#define VCCK_VAL_REG    0x1F301F3
#elif (VCCK_VAL == 859)
#define VCCK_VAL_REG    0x21101D5
#elif (VCCK_VAL == 849)
#define VCCK_VAL_REG    0x22F01B7
#elif (VCCK_VAL == 839)
#define VCCK_VAL_REG    0x24D0199
#elif (VCCK_VAL == 829)
#define VCCK_VAL_REG    0x26B017B
#elif (VCCK_VAL == 819)
#define VCCK_VAL_REG    0x27F0167
#elif (VCCK_VAL == 809)
#define VCCK_VAL_REG    0x29D0149
#elif (VCCK_VAL == 799)
#define VCCK_VAL_REG    0x2BB012B
#elif (VCCK_VAL == 789)
#define VCCK_VAL_REG    0x2D9010D
#elif (VCCK_VAL == 779)
#define VCCK_VAL_REG    0x2ED00F9
#elif (VCCK_VAL == 769)
#define VCCK_VAL_REG    0x30B00DB
#elif (VCCK_VAL == 759)
#define VCCK_VAL_REG    0x32900BD
#elif (VCCK_VAL == 749)
#define VCCK_VAL_REG    0x347009F
#elif (VCCK_VAL == 739)
#define VCCK_VAL_REG    0x3650081
#elif (VCCK_VAL == 729)
#define VCCK_VAL_REG    0x379006D
#elif (VCCK_VAL == 719)
#define VCCK_VAL_REG    0x397004F
#elif (VCCK_VAL == 709)
#define VCCK_VAL_REG    0x3B50031
#elif (VCCK_VAL == 699)
#define VCCK_VAL_REG    0x3D30013
#elif (VCCK_VAL == 689)
#define VCCK_VAL_REG    0x3E80000
#else
#error "VCCK val out of range\n"
#endif

/* VDDEE_VAL_REG */
#if    (VDDEE_VAL == 700)
#define VDDEE_VAL_REG   0x120000
#elif (VDDEE_VAL == 710)
#define VDDEE_VAL_REG   0x110001
#elif (VDDEE_VAL == 720)
#define VDDEE_VAL_REG   0x100002
#elif (VDDEE_VAL == 730)
#define VDDEE_VAL_REG   0xf0003
#elif (VDDEE_VAL == 740)
#define VDDEE_VAL_REG   0xe0004
#elif (VDDEE_VAL == 750)
#define VDDEE_VAL_REG   0xd0005
#elif (VDDEE_VAL == 760)
#define VDDEE_VAL_REG   0xc0006
#elif (VDDEE_VAL == 770)
#define VDDEE_VAL_REG   0xb0007
#elif (VDDEE_VAL == 780)
#define VDDEE_VAL_REG   0xa0008
#elif (VDDEE_VAL == 790)
#define VDDEE_VAL_REG   0x90009
#elif (VDDEE_VAL == 800)
#define VDDEE_VAL_REG   0x8000a
#elif (VDDEE_VAL == 810)
#define VDDEE_VAL_REG   0x7000b
#elif (VDDEE_VAL == 820)
#define VDDEE_VAL_REG   0x6000c
#elif (VDDEE_VAL == 830)
#define VDDEE_VAL_REG   0x5000d
#elif (VDDEE_VAL == 840)
#define VDDEE_VAL_REG   0x4000e
#elif (VDDEE_VAL == 850)
#define VDDEE_VAL_REG   0x3000f
#elif (VDDEE_VAL == 860)
#define VDDEE_VAL_REG   0x20010
#elif (VDDEE_VAL == 870)
#define VDDEE_VAL_REG   0x10011
#elif (VDDEE_VAL == 880)
#define VDDEE_VAL_REG   0x12
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
	/* config vddee and vcck pwm - pwm_e and pwm_f*/
#ifdef CONFIG_PDVFS_ENABLE
	{ PWM_PWM_H, VDDEE_SS_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_1, 0 },
	{ PWM_PWM_H, VDDEE_TT_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_2, 0 },
	{ PWM_PWM_H, VDDEE_FF_VALUE, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_3, 0 },
#else
	{ PWM_PWM_H,		   VDDEE_VAL_REG, 0xffffffff, 0, 0, 0},
#endif
	/* Push-pull the TEST_N output high to make VDDCPU_EN more stable */
	{ PADCTRL_TESTN_O,         (0x1 << 0),    (0x1 << 0), 0, 0, 0 },
	{ PADCTRL_TESTN_OEN,       (0x0 << 0),    (0x1 << 0), 0, 0, 0 },
	{ PWM_PWM_J,		   VCCK_VAL_REG,  0xffffffff, 0, 0, 0 },
	{ PWM_MISC_REG_H,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_J,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	/* set pwm j and pwm h clock rate to 24M 666M, enable them */
	{ CLKCTRL_PWM_CLK_GH_CTRL, (0x1 << 24), 0xffffffff, 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_IJ_CTRL, (0x1 << 24) | (0x3 << 25), 0xffffffff, 0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 drive strength to 2 ,already set by gpio owner on bl2*/
	// { PADCTRL_GPIOE_DS,	   0xa,		  0xf,	      0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 mux to pwmh pwmj */
	{ PADCTRL_PIN_MUX_REGI,	   (0x3 << 0) | (0x3 << 4),
					(0xf << 0) | (0xf << 4), 0, 0, 0 },
	{ PADCTRL_GPIOD_PULL_UP,   (0x1 << 2),	  (0x1 << 2), 0, 0, 0 },
	{ PWM_TEE_ONLY_J,          (0x1 << 0),	  (0xffffffff << 0), 0, 0, 0 },
#ifdef CONFIG_NOVERBOSE_BUILD
	/* use acs flag to disable uart print in each blx
	 * reg must be UART_B_WFIFO, flags: 1 --> disable uart print, 0: enable
	 */
	{ UART_B_WFIFO, 0, 0xffffffff, 0, 1, 0 },
#endif
	/* GPIOH_3 has an external pull-up, so disable the default internal pull-up */
	{ PADCTRL_GPIOH_PULL_EN,   (0x0 << 3),	  (0x1 << 3), 0, 0, 0 },
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
