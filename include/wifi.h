/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2018 Amlogic Limited
 */
#ifndef AML_WIFI_H
#define AML_WIFI_H

#include <usb.h>

struct wifi_dongle {
	unsigned short idVendor;
	unsigned short idProduct;
	char *usb_module;
	unsigned int * fw_data;
	int fw_len;
};

enum usb_endpoint_num {
	USB_EP0 = 0x0,
	USB_EP1,
	USB_EP2,
	USB_EP3,
	USB_EP4,
	USB_EP5,
	USB_EP6,
	USB_EP7,
};

enum wifi_cmd {
	CMD_DOWNLOAD_WIFI = 0xC1,
	CMD_START_WIFI,
	CMD_STOP_WIFI,
	CMD_READ_REG,
	CMD_WRITE_REG,
	CMD_READ_PACKET,
	CMD_WRITE_PACKET,
	CMD_WRITE_SRAM,
	CMD_READ_SRAM,
	CMD_DOWNLOAD_BT,
	CMD_GET_TX_CFM,
	CMD_OTHER_CMD,
	CMD_USB_IRQ,
	CMD_BBPLL_INIT,
};

struct crg_msc_cbw {
	u32 sig;
	u32 tag;
	u32 data_len;
	u8 flag;
	u8 lun;
	u8 len;
	u32 cdb[4];
	unsigned char resv[481];
} __attribute__ ((packed));

void wifi_init(void);

#endif
