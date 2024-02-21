/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_TX_REPEATER_H__
#define __HDMI_TX_REPEATER_H__

/*
 * HDMI Repeater TX I/F
 * RX downstream Information from rptx to rprx
 */

void rx_edid_physical_addr(int a, int b, int c, int d);
__weak void rx_edid_physical_addr(int a, int b, int c, int d)
{
}

int rx_set_hdr_lumi(unsigned char *data, int len);
__weak int rx_set_hdr_lumi(unsigned char *data, int len)
{
	return 0;
}

void rx_set_repeater_support(bool enable);
__weak void rx_set_repeater_support(bool enable)
{
}

void rx_set_receiver_edid(unsigned char *data, int len);
__weak void rx_set_receiver_edid(unsigned char *data, int len)
{
}

void rx_set_receive_hdcp(unsigned char *data, int len, int depth,
			 bool max_cascade, bool max_devs);
__weak void rx_set_receive_hdcp(unsigned char *data, int len,
					      int depth, bool max_cascade,
					      bool max_devs)
{
}

#endif
