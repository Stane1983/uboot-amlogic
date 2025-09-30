/* SPDX-License-Identifier: (GPL-2.0+ OR MIT)
 *
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __WIFI_BT_COMM_H__
#define __WIFI_BT_COMM_H__

#include <usb.h>

#define AML_W1U_VENDOR			0x414D
#define AML_W2_VENDOR			0x1B8E

#define AML_SIG_CBW			0x43425355

#define UINT32_FROM_BYTES_LE(b0, b1, b2, b3) \
	((unsigned int)(b0)         | \
	((unsigned int)(b1) << 8)  | \
	((unsigned int)(b2) << 16) | \
	((unsigned int)(b3) << 24))

typedef struct __packed {
	unsigned short interface : 3;
	unsigned short wireless  : 3;
	unsigned short family_rev: 3;
	unsigned short family_id : 5;
	unsigned short reserved  : 2;
} aml_chip_type;

enum {
	AML_REV_A,
	AML_REV_B,
	AML_REV_C,
	AML_REV_D,
	AML_REV_E,
	AML_REV_F,
	AML_REV_G,
	AML_REV_H,
};

enum {
	AML_UNKNOWN,
	AML_W1,
	AML_W1U,
	AML_W2,
	AML_W2L,
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

struct __packed crg_msc_cbw {
	unsigned int sig;
	unsigned int tag;
	unsigned int data_len;
	unsigned char flag;
	unsigned char lun;
	unsigned char len;
	unsigned int cdb[4];
	unsigned char resv[481];
};

typedef struct {
	unsigned char *r;
	unsigned char *w;
	unsigned char *base_addr;
	unsigned int size;
} gdsl_fifo_t;

void auc_build_cbw(struct crg_msc_cbw *cbw_buf, unsigned char dir, unsigned int len,
	unsigned char cdb1, unsigned int cdb2, unsigned long cdb3, unsigned long cdb4);
struct usb_device *aml_mod_get_udev(void);
aml_chip_type *aml_mod_get_type(void);
void aml_mod_init(void);

#endif
