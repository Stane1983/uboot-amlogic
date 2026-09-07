// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/types.h>
#include <amlogic/media/vout/hdmitx/hdmitx.h>
/* Base Block, Vendor/Product Information, byte[8]~[17] */
struct edid_venddat_t {
	unsigned char data[10];
};

static struct edid_venddat_t vendor_6g[] = {
	/* SAMSUNG UA55KS7300JXXZ */
	{ {0x4c, 0x2d, 0x3b, 0x0d, 0x00, 0x06, 0x00, 0x01, 0x01, 0x1a} },
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_null_pkt[] = {
	/* changhong LT32876 */
	{ {0x0d, 0x04, 0x21, 0x90, 0x01, 0x00, 0x00, 0x00, 0x20, 0x12} }
	/* Add new vendor data here */
};

static struct edid_venddat_t vendor_scan_non_std[] = {
	/* LG OLED55BXPCA */
	{ {0x1e, 0x6d, 0xc0, 0xc0, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1e} }
	/* Add new vendor data here */
};

bool hdmitx_find_vendor(struct hdmitx_dev *hdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vendor_6g); i++) {
		if (memcmp(&hdev->rawedid[8], vendor_6g[i].data,
			sizeof(vendor_6g[i].data)) == 0)
			return true;
	}
	return false;
}

/* need to forcely enable null pkt for such TV */
bool hdmitx_find_vendor_null_pkt(struct hdmitx_dev *hdev)
{
	int i;

	if (!hdev)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_null_pkt); i++) {
		if (memcmp(&hdev->rawedid[8], vendor_null_pkt[i].data,
		    sizeof(vendor_null_pkt[i].data)) == 0)
			return true;
	}
	return false;
}

/* scan non-standard TV, it doesn't display full screen when detect avi scan_info =
 * under_scan, instead it displays with black board around. and need to set
 * avi scan_info = NO_DATA to recover normally.
 */
bool hdmitx_find_vendor_scan_non_std(struct hdmitx_dev *hdev)
{
	int i;

	if (!hdev)
		return false;
	for (i = 0; i < ARRAY_SIZE(vendor_scan_non_std); i++) {
		if (memcmp(&hdev->rawedid[8], vendor_scan_non_std[i].data,
			sizeof(vendor_scan_non_std[i].data)) == 0)
			return true;
	}
	return false;
}
