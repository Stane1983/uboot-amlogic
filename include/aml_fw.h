/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * include/rtk_fw.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __RTLFW_H__
#define __RTLFW_H__

#include <usb.h>

#define AML_W1U_VENDOR			0x414D
#define AML_W2_VENDOR			0x1B8E

#define AML_SIG_CBW			0x43425355
#define AML_XFER_TO_DEVICE		0
#define AML_XFER_TO_HOST		0x80
#define AML_USB_CONTROL_MSG_TIMEOUT	3000
#define WRITE_SRAM_DATA_LEN		477
#define USB_DOWNLOAD_LEN		(20*1024)

#define BT_ICCM_AHB_BASE		0x00300000
#define BT_DCCM_AHB_BASE		0x00400000

#define REG_DEV_RESET			0xf03058
#define REG_PMU_POWER_CFG		0xf03040

#define BIT_PHY				1
#define BIT_MAC				(1 << 1)
#define BIT_CPU 			(1 << 2)
#define DEV_RESET_SW			16
#define DEV_RESET_HW			0

#define W2_HCI_TYPE_QUEUE_SIZE		256
#define W2_HCI_TYPE_QUEUE_ADDR		0x0051403c
#define W2_HCI_TYPE_QUEUE_R		0x00514000  //type queue read point address
#define W2_HCI_TYPE_QUEUE_W		0x00514020  //type queue write point address

#define W2_HCI_EVT_QUEUE_SIZE		1024
#define W2_HCI_EVT_QUEUE_ADDR		0x0051413c
#define W2_HCI_EVT_QUEUE_R		(W2_HCI_TYPE_QUEUE_R+0x04)  //event queue read point address
#define W2_HCI_EVT_QUEUE_W		(W2_HCI_TYPE_QUEUE_R+0x24)  //event queue write point address

#define W2_HCI_CMD_QUEUE_SIZE		4096
#define W2_HCI_CMD_QUEUE_ADDR		0x00518000  //length:4096 bytes
#define W2_HCI_CMD_QUEUE_R		0x00510000  //hci cmd queue read point address
#define W2_HCI_CMD_QUEUE_W		0x00510004  //hci cmd queue write point address

#define W2_USB_STATUS_REG		0x005101fc
#define WF_SRAM_FD_INIT_FLAG		(1 << 1)

#define USB_POLL_TOTAL_LEN		(1340)	  //event addr offset:0x13c + event size:1024


typedef struct
{
	unsigned short interface : 3;
	unsigned short wireless  : 3;
	unsigned short family_rev: 3;
	unsigned short family_id : 5;
	unsigned short reserved  : 2;
} __attribute__((packed)) aml_chip_type;

enum {
	AML_INTF_SDIO,
	AML_INTF_USB,
	AML_INTF_PCIE,
};

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

enum usb_endpoint_num{
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
	CMD_USB_IRQ
};

struct crg_msc_cbw {
	unsigned int sig;
	unsigned int tag;
	unsigned int data_len;
	unsigned char flag;
	unsigned char lun;
	unsigned char len;
	unsigned int cdb[4];
	unsigned char resv[481];
} __attribute__ ((packed));

struct _gdsl_fifo_t;
typedef struct _gdsl_fifo_t gdsl_fifo_t;

struct _gdsl_fifo_t {
	unsigned char *r;
	unsigned char *w;
	unsigned char *base_addr;
	unsigned int size;
};

#define CHIP_TYPE_W2U_ICCM "fw_w2u_iccm.txt"
#define CHIP_TYPE_W2U_DCCM "fw_w2u_dccm.txt"

aml_chip_type *get_aml_dev(void);
void load_aml_firmware_dev(aml_chip_type *type);
void amlbt_firmware_init(void);
void aml_wakeup_set(void);


#endif
