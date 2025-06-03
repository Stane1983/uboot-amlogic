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
#define _BIT(n)			MSK(1, (n))

#define writel_bits(mask, val, addr) \
	writel((readl(addr) & ~(mask)) | (val), addr)

void cec_get_efuse_val(void)
{
	s64 cec_trim_val;

	/* get trim value from efuse, return -1 if not programed */
	extern int64_t meson_trustzone_efuse_caliItem(const char *str);
	cec_trim_val = meson_trustzone_efuse_caliItem("odio33");
	/* cec gpiow 12 enable */
	writel_bits(_BIT(12), 1 << 12, PADCTRL_GPIOW_PULL_EN);
	if (cec_trim_val >= 0) {
		writel_bits(MSK(2, 24), cec_trim_val << 24, PADCTRL_GPIOW_DS);
		printf("cec trim:0x%llx\n", cec_trim_val);
	} else {
		printf("no cec trim val!\n");
	}
}
