// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/types.h>
//#include <amlogic/media/vout/hdmitx/hdmitx.h>

/* Base Block, Vendor/Product Information, byte[8]~[17] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t vendor_shield_hdr[] = {
	/* BLACKLINK */
	{ {0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x21} }
	/* Add new vendor data here */
};

/*
 * special TV to mask the HDR capabilities at 422,12bit.
 */
bool hdmitx21_find_vendor_shield_hdr(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;

	for (i = 0; i < ARRAY_SIZE(vendor_shield_hdr); i++) {
		if (memcmp(&edid_buf[8], vendor_shield_hdr[i].data,
			sizeof(vendor_shield_hdr[i].data)) == 0)
			return true;
	}

	return false;
}
