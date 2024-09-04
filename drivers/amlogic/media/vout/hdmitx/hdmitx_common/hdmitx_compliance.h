/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_COMPLIANCE_H
#define __HDMITX_COMPLIANCE_H

bool hdmitx_find_hdr_pkt_delay_to_vsync(unsigned char *edid_buf);
bool hdmitx_find_send_ake_init(unsigned char *edid_buf);

#endif

