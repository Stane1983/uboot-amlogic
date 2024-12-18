/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_EDID_HEADER_H_
#define __HDMITX_EDID_HEADER_H_

#include <linux/types.h>
#include <amlogic/media/vout/aml_vinfo.h>
#include <amlogic/media/vout/hdmitx21/hdmitx_ext.h>
#include <amlogic/media/vout/hdmitx21/hdmi_common.h>

/**
 * cec_get_edid_spa_location() - find location of the Source Physical Address
 *
 * @edid: the EDID
 * @size: the size of the EDID
 *
 * This EDID is expected to be a CEA-861 compliant, which means that there are
 * at least two blocks and one or more of the extensions blocks are CEA-861
 * blocks.
 *
 * The returned location is guaranteed to be <= size-2.
 *
 * This is an inline function since it is used by both CEC and V4L2.
 * Ideally this would go in a module shared by both, but it is overkill to do
 * that for just a single function.
 */
static inline unsigned int cec_get_edid_spa_location(const u8 *edid,
						     unsigned int size)
{
	unsigned int blocks = size / 128;
	unsigned int block;
	u8 d;

	/* Sanity check: at least 2 blocks and a multiple of the block size */
	if (blocks < 2 || size % 128)
		return 0;

	/*
	 * If there are fewer extension blocks than the size, then update
	 * 'blocks'. It is allowed to have more extension blocks than the size,
	 * since some hardware can only read e.g. 256 bytes of the EDID, even
	 * though more blocks are present. The first CEA-861 extension block
	 * should normally be in block 1 anyway.
	 */
	if (edid[0x7e] + 1 < blocks)
		blocks = edid[0x7e] + 1;

	for (block = 1; block < blocks; block++) {
		unsigned int offset = block * 128;

		/* Skip any non-CEA-861 extension blocks */
		if (edid[offset] != 0x02 || edid[offset + 1] != 0x03)
			continue;

		/* search Vendor Specific Data Block (tag 3) */
		d = edid[offset + 2] & 0x7f;
		/* Check if there are Data Blocks */
		if (d <= 4)
			continue;
		if (d > 4) {
			unsigned int i = offset + 4;
			unsigned int end = offset + d;

			/* Note: 'end' is always < 'size' */
			do {
				u8 tag = edid[i] >> 5;
				u8 len = edid[i] & 0x1f;

				if (tag == 3 && len >= 5 && i + len <= end &&
				    edid[i + 1] == 0x03 &&
				    edid[i + 2] == 0x0c &&
				    edid[i + 3] == 0x00)
					return i + 4;
				i += len + 1;
			} while (i < end);
		}
	}
	return 0;
}

#endif
