// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/io.h>
#include <asm/amlogic/arch/register.h>
#include <amlogic/aml_tvafe.h>

/**
 * Bit field mask
 * @param m	width
 * @param n shift
 */
#define MSK(m, n)		(((1 << (m)) - 1) << (n))

#define writel_bits(mask, val, addr) \
	writel((readl(addr) & ~(mask)) | (val), addr)

void cec_get_efuse_val(void)
{
	s64 cec_trim_val, gpiow_cfg_val;

	/* get trim value from efuse, return -1 if not programed */
	extern int64_t meson_trustzone_efuse_caliItem(const char *str);
	cec_trim_val = meson_trustzone_efuse_caliItem("cec_io");
	/* cec gpiow 17 enable */
	if (cec_trim_val == 1) {
		gpiow_cfg_val = 0x10;
		printf("[cec] gpiow_17 enabled\n");
	} else if (cec_trim_val == 1) {
		gpiow_cfg_val = 0x1;
		printf("[cec] gpiow_16 enabled\n");
	}
	writel_bits(MSK(8, 0), gpiow_cfg_val, PADCTRL_PIN_MUX_REGK);
}
