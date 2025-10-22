/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * include/rtk_fw.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __AMLBT_FW_H__
#define __AMLBT_FW_H__

#include <usb.h>

#define AML_XFER_TO_DEVICE		0
#define AML_XFER_TO_HOST		0x80
#define AML_USB_CONTROL_MSG_TIMEOUT	3000
#define WRITE_SRAM_DATA_LEN		477
#define USB_DOWNLOAD_LEN		(20 * 1024)

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

#define W2_HCI_EVT_QUEUE_SIZE		2048

#define W2_HCI_EVT_QUEUE_ADDR		0x0051413c
#define W2_HCI_EVT_QUEUE_R		(W2_HCI_TYPE_QUEUE_R + 0x04)  //event queue read point address
#define W2_HCI_EVT_QUEUE_W		(W2_HCI_TYPE_QUEUE_R + 0x24)  //event queue write point address

#define W2_HCI_CMD_QUEUE_SIZE		4096
#define W2_HCI_CMD_QUEUE_ADDR		0x00518000  //length:4096 bytes
#define W2_HCI_CMD_QUEUE_R		0x00510000  //hci cmd queue read point address
#define W2_HCI_CMD_QUEUE_W		0x00510004  //hci cmd queue write point address

#define W2_USB_STATUS_REG		0x005101fc
#define WF_SRAM_FD_INIT_FLAG		(1 << 1)

#define USB_POLL_TOTAL_LEN		(2364)  // event addr offset:0x13c+event size:2364

#define MANFDATA_LEN			256
#define FW_MANFDATA_ADDR		(W2_HCI_TYPE_QUEUE_R + USB_POLL_TOTAL_LEN + 48 + 4) //0x514970
#define FIFO_FW_RC_LIST_ADDR		(W2_HCI_TYPE_QUEUE_R + USB_POLL_TOTAL_LEN + 4) //0x514940


#ifdef CONFIG_YOCTO
	#define AML_FW_PATH "/vendor/firmware/wireless/"
#else
	#define AML_FW_PATH "/odm_ext/aml_bt_fw/"
#endif

#define W2U_FW AML_FW_PATH "w2_bt_fw_usb.bin"
#define W2L_U_FW  AML_FW_PATH "w2l_bt_15p4_fw_usb.bin"
#define AMLBT_CONF AML_FW_PATH "aml_bt.conf"

#define W2U_ICCM_BASE 256
#define W2U_ICCM_SIZE   0x40000

#define W2L_U_ICCM_BASE 384
#define W2L_U_ICCM_SIZE   0x38000

void amlbt_start(void);
int amlbt_should_start(void);

#endif
