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

void cvbs_dac_cfg(void)
{
	int64_t cvbs_dac_val;

	/* get trim value from efuse, return -1 if not programed */
	extern int64_t meson_trustzone_efuse_caliItem(const char *str);
	cvbs_dac_val = meson_trustzone_efuse_caliItem("cvbsdac");
	/* cvbs dac cfg*/
	if (cvbs_dac_val >= 0) {
		writel_bits(MSK(7, 0), cvbs_dac_val & 0xff, ANACTRL_VDAC_CTRL1);
		printf("cvbsdac val=0x%llx\n", cvbs_dac_val);
	} else {
		printf("no cvbsdac val!\n");
	}
}
