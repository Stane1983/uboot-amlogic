// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/timing.h>
#include <asm/amlogic/arch/ddr_define.h>
#include <ddr_timing.c>

/* board clk defines */
#define CPU_CLK                                 2016
#define DSU_CLK                                 1200

/* board vmin_value defines */
#define VMIN_FF_VALUE                           670
#define VMIN_TT_VALUE                           720
#define VMIN_SS_VALUE                           770
/* board vddee_value defines */
/* SS/TT/FF = 0.77V/0.74V/0.71V */
#define VDDEE_FF_VALUE                          0x50008
#define VDDEE_TT_VALUE                          0x80005
#define VDDEE_SS_VALUE                          0xb0002

board_clk_set_t __board_clk_setting
__attribute__ ((section(".clk_param"))) = {
	/* clock settings for bl2 */
	.cpu_clk	= CPU_CLK / 24 * 24,
	.dsu_clk	= DSU_CLK / 24 * 24,
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
#if   (VCCK_VAL == 980)
#define VCCK_VAL_REG    0x00003e8
#elif (VCCK_VAL == 970)
#define VCCK_VAL_REG    0x02003c6
#elif (VCCK_VAL == 960)
#define VCCK_VAL_REG    0x04103a5
#elif (VCCK_VAL == 950)
#define VCCK_VAL_REG    0x0620384
#elif (VCCK_VAL == 940)
#define VCCK_VAL_REG    0x0830363
#elif (VCCK_VAL == 930)
#define VCCK_VAL_REG    0x0a40342
#elif (VCCK_VAL == 920)
#define VCCK_VAL_REG    0x0c50321
#elif (VCCK_VAL == 910)
#define VCCK_VAL_REG    0x0e60300
#elif (VCCK_VAL == 900)
#define VCCK_VAL_REG    0x10702df
#elif (VCCK_VAL == 890)
#define VCCK_VAL_REG    0x12802be
#elif (VCCK_VAL == 880)
#define VCCK_VAL_REG    0x149029d
#elif (VCCK_VAL == 870)
#define VCCK_VAL_REG    0x16a027c
#elif (VCCK_VAL == 860)
#define VCCK_VAL_REG    0x18b025b
#elif (VCCK_VAL == 850)
#define VCCK_VAL_REG    0x1ac023a
#elif (VCCK_VAL == 840)
#define VCCK_VAL_REG    0x1cd0219
#elif (VCCK_VAL == 830)
#define VCCK_VAL_REG    0x1ee01f8
#elif (VCCK_VAL == 820)
#define VCCK_VAL_REG    0x21901cd
#elif (VCCK_VAL == 810)
#define VCCK_VAL_REG    0x23a01ac
#elif (VCCK_VAL == 800)
#define VCCK_VAL_REG    0x25b018b
#elif (VCCK_VAL == 790)
#define VCCK_VAL_REG    0x27c016a
#elif (VCCK_VAL == 780)
#define VCCK_VAL_REG    0x29d0149
#elif (VCCK_VAL == 770)
#define VCCK_VAL_REG    0x2be0128
#elif (VCCK_VAL == 760)
#define VCCK_VAL_REG    0x2df0107
#elif (VCCK_VAL == 750)
#define VCCK_VAL_REG    0x30000e6
#elif (VCCK_VAL == 740)
#define VCCK_VAL_REG    0x32100c5
#elif (VCCK_VAL == 730)
#define VCCK_VAL_REG    0x34200a4
#elif (VCCK_VAL == 720)
#define VCCK_VAL_REG    0x3630083
#elif (VCCK_VAL == 710)
#define VCCK_VAL_REG    0x3840062
#elif (VCCK_VAL == 700)
#define VCCK_VAL_REG    0x3a50041
#elif (VCCK_VAL == 690)
#define VCCK_VAL_REG    0x3c60020
#elif (VCCK_VAL == 680)
#define VCCK_VAL_REG    0x3e80000
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
	{ PWM_PWM_J,		   VCCK_VAL_REG,  0xffffffff, 0, 0, 0 },
	{ PWM_MISC_REG_H,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PWM_MISC_REG_J,	   (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
	/* set pwm h clock rate to 500M, enable them */
	/* set pwm j clock rate to 500M, enable them */
	{ CLKCTRL_PWM_CLK_GH_CTRL, (0x1 << 24), 0xffffffff, 0, 0, 0 },
	{ CLKCTRL_PWM_CLK_IJ_CTRL, (0x1 << 24) | (0x2 << 25), 0xffffffff, 0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 drive strength to 3 */
	{ PADCTRL_GPIOE_DS,	   0xf,		  0xf,	      0, 0, 0 },
	/* set GPIOE_0 GPIOE_1 mux to pwmh pwmj */
	{ PADCTRL_PIN_MUX_REGI,	   (0x3 << 0),	  (0xf << 0), 0, 0, 0 },
	{ PADCTRL_PIN_MUX_REGI,	   (0x3 << 4),	  (0xf << 4), 0, 0, 0 },
	{ PADCTRL_GPIOD_PULL_UP,   (0x1 << 2),	  (0x1 << 2), 0, 0, 0 },
	{ PWM_TEE_ONLY_J,          (0x1 << 0),	  (0xffffffff << 0), 0, 0, 0 },
	/* disable GPIOH_3 Pull */
	{ PADCTRL_GPIOH_PULL_EN,   (0x0 << 3),    (0x1 << 3), 0, 0, 0 },
#ifdef CONFIG_NOVERBOSE_BUILD
	/* use acs flag to disable uart print in each blx
	 * reg must be UART_B_WFIFO, flags: 1 --> disable uart print, 0: enable
	 */
	{ UART_B_WFIFO, 0, 0xffffffff, 0, 1, 0 },
#endif
	/* set VDDCPU_EN to high */
	{ PADCTRL_TESTN_OEN,       (0x0 << 0),	  (0x1 << 0), 0, 0, 0 },
	{ PADCTRL_TESTN_O,         (0x1 << 0),	  (0x1 << 0), 0, 0, 0 },
};

#define DEV_FIP_SIZE 0x300000
#define DDR_FIP_SIZE 0x40000
/* for all the storage parameter */
#ifdef CONFIG_MTD_SPI_NAND
/* for spinand storage parameter */
storage_parameter_t __store_para __section(.store_param) = {
	.common				= {
		.version = 0x01,
		.device_fip_container_size = DEV_FIP_SIZE,
		.device_fip_container_copies = 4,
		.ddr_fip_container_size = DDR_FIP_SIZE,
	},
	.nand				= {
		.version = 0x01,
		.bbt_pages = 1, // TODO: BL2E BBT
		.bbt_start_block = 20,
		.discrete_mode = 1,
		.setup_data.spi_nand_page_size = 2048,
		.reserved.spi_nand_planes_per_lun = 1,
		.reserved_area_blk_cnt = 48,
		.page_per_block = 64,
		.use_param_page_list = 0,
	},
};
#else
storage_parameter_t __store_para __attribute__ ((section(".store_param"))) = {
	.common					= {
		.version			= 0x01,
		.device_fip_container_size	= DEV_FIP_SIZE,
		.device_fip_container_copies	= 4,
		.ddr_fip_container_size		= DDR_FIP_SIZE,
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
		.reserved_area_blk_cnt		= 48,
		.page_per_block			= 64,
		.use_param_page_list		= 0,
	},
};
#endif
