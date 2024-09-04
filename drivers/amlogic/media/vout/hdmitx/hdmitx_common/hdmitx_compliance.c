// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>

/* Base Block, Vendor/Product Information, byte[8]~[17] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t hdr_delay_id[] = {
	/* HUAWEI */
	{ {0x22, 0xf6, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x1d} }
	/* Add new vendor data here */
};

static struct edid_venddat_t ake_init_id[] = {
	/* LG UH6100 */
	{ {0x1E, 0x6D, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1A} }
	/* Add new vendor data here */
};

/* On Huawei TVs, HDR will cause a flickering screen,
 * and HDR PKT needs to be moved behind VSYNC on S7 or later SOCs
 */
bool hdmitx_find_hdr_pkt_delay_to_vsync(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;

	for (i = 0; i < ARRAY_SIZE(hdr_delay_id); i++) {
		if (memcmp(&edid_buf[8], hdr_delay_id[i].data, sizeof(hdr_delay_id[i].data)) == 0)
			return true;
	}
	return false;
}

/*
 * Sending ake_init for the first time after booting will cause the TV to flash black.
 * Add a workaround method and send ake_initi before uboot outputs the picture.
 */
bool hdmitx_find_send_ake_init(unsigned char *edid_buf)
{
	int i;

	if (!edid_buf)
		return false;

	for (i = 0; i < ARRAY_SIZE(ake_init_id); i++) {
		if (memcmp(&edid_buf[8], ake_init_id[i].data, sizeof(ake_init_id[i].data)) == 0)
			return true;
	}
	return false;
}
