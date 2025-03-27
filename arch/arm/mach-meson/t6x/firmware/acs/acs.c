// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <version.h>
#include <asm/amlogic/arch/acs.h>
#include <asm/amlogic/arch/timing.h>
#include "timing.c"

dev_param_hdr_t __param_hdr __attribute__ ((section(".dev_header"))) = {
	.magic = DEV_PARAM_MAGIC,
	.version = DEV_PARAM_VERSION,
	.head_crc = ACS_HEAD_CRC,

	.bl2_regs_magic = "bl2r_",
	.bl2_regs_length = sizeof(__bl2_reg),

	.board_clk_magic = "bclk_",
	.board_clk_length = sizeof(__board_clk_setting),

	.opt_reg_magic = "ops__",
	.opt_reg_length = sizeof(__bl2_ops_reg),

	.sto_set_magic = "store",
	.sto_set_length = sizeof(__store_para),

	.ddr_set_magic = "ddrs_",
	.ddr_set_length = sizeof(__ddr_setting),
	.ddr_2acs_length = sizeof(__bl2_ddr_reg_data),
	.ddr_2acs_data_p = (unsigned int *)__bl2_ddr_reg_data,
};

__attribute__ ((__weak__)) vendor_key_t __vendor_key __attribute__ ((section(".vendor_key"))) = {
	.magic = 0x6b706d61,
	.flags = 0x0,
	.pubkey = {0}, //VENDOR_PUB_KEY_DATA,
};
