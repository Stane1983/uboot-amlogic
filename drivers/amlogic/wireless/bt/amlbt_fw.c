// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <env.h>
#include <malloc.h>
#include <usb.h>

#include <asm/processor.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>

#include <linux/compiler.h>
#include <linux/compat.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#include <dm/device.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

#include <amlogic/aml_model.h>
#include <amlogic/wireless/wifi_bt_comm.h>
#include <amlogic/wireless/bt/amlbt_fw.h>

#ifndef getenv
#define getenv env_get
#endif

#define USB_BEGIN_LOCK()
#define USB_END_LOCK()

#define CHIP_INTF_REG_BASE      (0xf00000)
#define RG_AON_A52     (CHIP_INTF_REG_BASE + 0xd0)

#define MAC_ADDR_LEN 6
#define MAX_MAC_LIST 8
#define MAX_USER_BUF_LEN    (MAX_MAC_LIST * (17 + 1))

typedef struct {
	char mac[MAC_ADDR_LEN];
	char used;
} rc_list_t;

typedef struct {
	unsigned char *iccm;
	unsigned int iccm_size;
	unsigned int iccm_base_addr;
	unsigned char *dccm;
	unsigned int dccm_size;
	unsigned int dccm_base_addr;
} fw_info;

static struct crg_msc_cbw g_cmd_buf;
static gdsl_fifo_t *g_cmd_fifo;
static gdsl_fifo_t *g_event_fifo;
static gdsl_fifo_t *g_hci_type_fifo;
static unsigned char usb_buf[4096];
static unsigned char evt_buf[1024];

static rc_list_t rc_list[MAX_MAC_LIST] = {0};

static int auc_write_reg_by_ep(struct usb_device *udev, unsigned int addr, unsigned int value,
	unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;

	USB_BEGIN_LOCK();
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
	/* cmd stage */
	ret = usb_bulk_msg(udev, (unsigned int)usb_sndbulkpipe(udev, ep),
		(void *)&g_cmd_buf, sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u, value:%#x\n",
			ret, ep, addr, len, value);
		USB_END_LOCK();
		return ret;
	}
	USB_END_LOCK();

	return actual_length; //bt write maybe use the value
}

static void auc_write_word_by_ep_for_bt(struct usb_device *udev, unsigned int addr, unsigned int data,
	unsigned int ep)
{
	int len = 4;

	switch (ep) {
	case USB_EP2:
		auc_write_reg_by_ep(udev, addr, data, len, ep);
		break;
	default:
		printf("EP-%u unsupported!\n", ep);
		break;
	}
}

static unsigned int auc_read_reg_by_ep(struct usb_device *udev, unsigned int addr, unsigned int len,
	unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned int reg_data;
	unsigned char *data = NULL;

	USB_BEGIN_LOCK();

	data = (unsigned char *)kzalloc(len, GFP_KERNEL|GFP_ATOMIC);

	if (!data) {
		printf("auc_read_reg_by_ep kzalloc fail, ep:%u, addr:%#x, len:%u\n", ep, addr, len);
		USB_END_LOCK();
		return -ENOMEM;
	}
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

	/* cmd stage */
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)&g_cmd_buf,
		sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_reg_by_ep cmd usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n",
			ret, ep, addr, len);
		kfree(data);
		USB_END_LOCK();
		return ret;
	}

	/* data stage */
	ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, ep), (void *)data, len,
		&actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_reg_by_ep data usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n",
			ret, ep, addr, len);
		kfree(data);
		USB_END_LOCK();
		return ret;
	}

	memcpy(&reg_data, data, actual_length);
	kfree(data);
	USB_END_LOCK();

	return reg_data;
}

static unsigned int auc_read_word_by_ep_for_bt(struct usb_device *udev, unsigned int addr,
	unsigned int ep)
{
	int len = 4;
	unsigned int value = 0;

	switch (ep) {
	case USB_EP2:
		value = auc_read_reg_by_ep(udev, addr, len, ep);
		break;
	default:
		printf("EP-%u unsupported!\n", ep);
		break;
	}
	return value;
}

static void auc_write_sram_by_ep(struct usb_device *udev, const unsigned char *pdata, unsigned int addr,
	unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned char *kmalloc_buf = NULL;

	USB_BEGIN_LOCK();
	{
		memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
		auc_build_cbw(&g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len);
		/* cmd stage */
		ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)&g_cmd_buf,
			sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
		if (ret) {
			printf("auc_write_sram_by_ep usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n",
				ret, ep, addr, len);
			USB_END_LOCK();
			return;
		}

		kmalloc_buf = (unsigned char *)kzalloc(len,  GFP_ATOMIC);
		if (!kmalloc_buf) {
			printf("kzalloc buf fail, ep:%u, addr:%#x, len:%u\n", ep, addr, len);
			USB_END_LOCK();
			return;
		}

		memcpy(kmalloc_buf, pdata, len);
		/* data stage */
		ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)kmalloc_buf, len,
			&actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
		if (ret) {
			printf("auc_write_sram_by_ep data usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n",
				ret, ep, addr, len);
			kfree(kmalloc_buf);
			USB_END_LOCK();
			return;
		}
		kfree(kmalloc_buf);
	}
	USB_END_LOCK();
}

static void auc_write_sram_by_ep_for_bt(struct usb_device *udev, const unsigned char *buf,
	unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
	if (len == 0) {
		printf("EP-%u write len err!\n", ep);
		return;
	}

	switch (ep) {
	case USB_EP2:
		auc_write_sram_by_ep(udev, buf, (unsigned int)(unsigned long)sram_addr, len, ep);
		break;
	default:
		printf("EP-%u unsupported!\n", ep);
		break;
	}
}

static void auc_read_sram_by_ep(struct usb_device *udev, unsigned char *pdata, unsigned int addr,
	unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned char *kmalloc_buf = NULL;

	USB_BEGIN_LOCK();
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
	/* cmd stage */
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, ep), (void *)&g_cmd_buf,
		sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("%s: cmd usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n", __func__,
			ret, ep, addr, len);
		USB_END_LOCK();
		return;
	}

	kmalloc_buf = (unsigned char *)kzalloc(len, GFP_ATOMIC);
	if (!kmalloc_buf) {
		printf("kmalloc buf fail, ep: %d, len: %d\n", ep, len);
		USB_END_LOCK();
		return;
	}

	/* data stage */
	ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, ep), (void *)kmalloc_buf, len,
		&actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("%s: cmd usb_bulk_msg fail, ret:%d, ep:%u, addr:%#x, len:%u\n", __func__,
			ret, ep, addr, len);
		kfree(kmalloc_buf);
		USB_END_LOCK();
		return;
	}

	memcpy(pdata, kmalloc_buf, actual_length);
	kfree(kmalloc_buf);

	USB_END_LOCK();
}

static void auc_read_sram_by_ep_for_bt(struct usb_device *udev, unsigned char *buf,
	unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
	if (len == 0) {
		printf("EP-%u read len err!\n", ep);
		return;
	}

	switch (ep) {
	case USB_EP2:
		auc_read_sram_by_ep(udev,   buf, (unsigned int)(unsigned long)sram_addr, len, ep);
		break;
	default:
		printf("EP-%u unsupported!\n", ep);
		break;
	}
}

static int amlbt_read_fw_data(const aml_chip_type *chip_type, fw_info *info)
{
	unsigned int iccm_base = 0;
	unsigned int temp_size = 0;
	const char *file_path = NULL;
	unsigned char *temp = NULL;
	int raw_size = 0;

	if (chip_type->family_id == AML_W2L) {
		file_path = W2L_U_FW;
		iccm_base = W2L_U_ICCM_BASE;
		info->iccm_size = W2L_U_ICCM_SIZE;

	} else {
		file_path = W2U_FW;
		iccm_base = W2U_ICCM_BASE;
		info->iccm_size = W2U_ICCM_SIZE;
	}

	raw_size = model_get_file_size(file_path);
	if (raw_size <= 0) {
		printf("%s: model_get_file_size fail\n", file_path);
		return -1;
	}

	temp = (unsigned char *)malloc(raw_size + 1);
	if (!temp) {
		printf("%s: malloc fail\n", __func__);
		return -1;
	}

	memset(temp, 0, raw_size + 1);

	if (model_read_file(file_path, temp, raw_size) <= 0) {
		printf("Failed to read file: %s\n", file_path);
		free(temp);
		return -1;
	}

	//printf("%s: iccm_size:%#x,%#x,%#x,%#x\n", __func__, temp[0], temp[1], temp[2], temp[3]);
	//printf("%s: dccm_size:%#x,%#x,%#x,%#x\n", __func__, temp[4], temp[5], temp[6], temp[7]);

	temp_size =  UINT32_FROM_BYTES_LE(temp[0], temp[1], temp[2], temp[3]);

	info->iccm = malloc(info->iccm_size);
	memcpy(info->iccm, &temp[8 + (iccm_base * 1024)], (info->iccm_size));
	info->iccm_base_addr = BT_ICCM_AHB_BASE + iccm_base * 1024;

	info->dccm_size =  UINT32_FROM_BYTES_LE(temp[4], temp[5], temp[6], temp[7]);
	info->dccm = malloc(info->dccm_size);
	memcpy(info->dccm, &temp[8 + temp_size], info->dccm_size);
	info->dccm_base_addr = BT_DCCM_AHB_BASE;

	printf("%s: raw_size:%#x iccm_size:%#x dccm_size:%#x\n", __func__, raw_size,
		info->iccm_size, info->dccm_size);

	free(temp);

	return 0;
}

static int amlbt_load_firmware(const aml_chip_type *chip_type, struct usb_device *udev)
{
	unsigned int offset = 0;
	unsigned int remain_len = 0;
	unsigned int temp_addr = 0;
	fw_info aml_fw_info = {0};
	int ret = -1;

	printf("%s: start:%u family_id:%d\n", __func__, get_time(), chip_type->family_id);

	ret = amlbt_read_fw_data(chip_type, &aml_fw_info);
	if (ret != 0 || !aml_fw_info.iccm || !aml_fw_info.dccm) {
		printf("get fw data fail, exit\n");
		return -1;
	}

	temp_addr = aml_fw_info.iccm_base_addr;
	remain_len = (aml_fw_info.iccm_size - offset);
	while (offset < aml_fw_info.iccm_size) {
		if (remain_len < USB_DOWNLOAD_LEN) {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.iccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, remain_len, USB_EP2);
			offset += remain_len;
			temp_addr += remain_len;
			//printf("bt_usb_download_firmware iccm1 offset %#x, write_len %#x\n", offset, remain_len);
		} else {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.iccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, USB_DOWNLOAD_LEN, USB_EP2);
			offset += USB_DOWNLOAD_LEN;
			remain_len -= USB_DOWNLOAD_LEN;
			temp_addr += USB_DOWNLOAD_LEN;
			//printf("bt_usb_download_firmware iccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
		}
	}
/******************************iccm check*****************************************/
	offset = 0;
	temp_addr = aml_fw_info.iccm_base_addr;
	while (offset < aml_fw_info.iccm_size) {
		memset(usb_buf, 0, sizeof(usb_buf));
		auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)temp_addr,
			sizeof(usb_buf), USB_EP2);
		if (memcmp(usb_buf, &(aml_fw_info.iccm[offset]), sizeof(usb_buf))) {
			printf("iccm check err!! offset:%#x size:%lu\n", offset, sizeof(usb_buf));
			printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", usb_buf[0], usb_buf[1],
				usb_buf[2], usb_buf[3], usb_buf[4], usb_buf[5], usb_buf[6], usb_buf[7]);
			printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", aml_fw_info.iccm[offset],
				aml_fw_info.iccm[offset + 1], aml_fw_info.iccm[offset + 2],
				aml_fw_info.iccm[offset + 3], aml_fw_info.iccm[offset + 4],
				aml_fw_info.iccm[offset + 5], aml_fw_info.iccm[offset + 6],
				aml_fw_info.iccm[offset + 7]);

			return -1;
		}
		offset += sizeof(usb_buf);
		temp_addr += sizeof(usb_buf);
	}
/*********************************************************************************/

	offset = 0;
	temp_addr = aml_fw_info.dccm_base_addr;
	remain_len = aml_fw_info.dccm_size;
	while (offset < aml_fw_info.dccm_size) {
		if (remain_len < USB_DOWNLOAD_LEN) {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.dccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, remain_len, USB_EP2);
			offset += remain_len;
			temp_addr += remain_len;
			//printf("bt_usb_download_firmware dccm1 offset %#x, write_len %#x\n", offset, remain_len);
		} else {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.dccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, USB_DOWNLOAD_LEN, USB_EP2);
			offset += USB_DOWNLOAD_LEN;
			remain_len -= USB_DOWNLOAD_LEN;
			temp_addr += USB_DOWNLOAD_LEN;
			//printf("bt_usb_download_firmware dccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
		}
	}
/******************************dccm check*****************************************/
	offset = 0;
	temp_addr = aml_fw_info.dccm_base_addr;
	while (offset < aml_fw_info.dccm_size) {
		memset(usb_buf, 0, sizeof(usb_buf));
		auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)temp_addr,
			sizeof(usb_buf), USB_EP2);
		if (memcmp(usb_buf, &(aml_fw_info.dccm[offset]), sizeof(usb_buf))) {
			printf("dccm check err!! offset:%#x size:%lu\n", offset, sizeof(usb_buf));
			printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", usb_buf[0], usb_buf[1],
				usb_buf[2], usb_buf[3], usb_buf[4], usb_buf[5], usb_buf[6], usb_buf[7]);
			printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", aml_fw_info.dccm[offset],
				aml_fw_info.dccm[offset + 1], aml_fw_info.dccm[offset + 2],
				aml_fw_info.dccm[offset + 3], aml_fw_info.dccm[offset + 4],
				aml_fw_info.dccm[offset + 5], aml_fw_info.dccm[offset + 6],
				aml_fw_info.dccm[offset + 7]);

			return -1;
		}
		offset += sizeof(usb_buf);
		temp_addr += sizeof(usb_buf);
	}
/*********************************************************************************/

	//printf("%s: end:%u\n", __func__, get_time());
	return 0;
}

static gdsl_fifo_t *gdsl_fifo_init(unsigned int len, unsigned char *base_addr)
{
	gdsl_fifo_t *p_fifo = (gdsl_fifo_t *)kzalloc(sizeof(gdsl_fifo_t), GFP_ATOMIC);

	//printf("%s \n", __func__);

	if (p_fifo) {
		memset(p_fifo, 0, sizeof(gdsl_fifo_t));
		p_fifo->w = 0;
		p_fifo->r = 0;
		p_fifo->base_addr = base_addr;
		p_fifo->size = len;
	}

	return p_fifo;
}

static unsigned int gdsl_fifo_used(gdsl_fifo_t *p_fifo)
{
	if (p_fifo->r <= p_fifo->w)
		return (p_fifo->w - p_fifo->r);

	return (p_fifo->size + p_fifo->w - p_fifo->r);
}

static unsigned int gdsl_fifo_remain(gdsl_fifo_t *p_fifo)
{
	unsigned int used = gdsl_fifo_used(p_fifo);

	return p_fifo->size - used - 4;
}

unsigned int gdsl_write_data_by_ep(struct usb_device *udev, gdsl_fifo_t *p_fifo, unsigned char *data,
	unsigned int len, unsigned int ep)
{
	unsigned int index = 0;
	unsigned char *w = p_fifo->w;
	unsigned int i = 0;

	len = ((len + 3) & 0xFFFFFFFC);
	if (gdsl_fifo_remain(p_fifo) < len) {
		printf("write data no space!!\n");
		return 0;
	}

	if (w == 0) {
		debug("w s1 %#lx\n", (unsigned long)p_fifo->w);
		auc_write_sram_by_ep_for_bt(udev, data, p_fifo->base_addr, len, ep);
		p_fifo->w = (unsigned char *)(unsigned long)(len % p_fifo->size);
		return len;
	}

	while (i < len) {
		w = (unsigned char *)(((unsigned long)w + 1) % p_fifo->size);
		i++;
		if (w == 0) {
			debug("w s2 %#lx\n", (unsigned long)p_fifo->w);
			auc_write_sram_by_ep_for_bt(udev, data,
				(unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr),
				i, ep);
			p_fifo->w = 0;
			index = i;
		}
	}
	if (index < len) {
		debug("w s3 %#lx\n", (unsigned long)p_fifo->w);
		auc_write_sram_by_ep_for_bt(udev, &data[index],
			(unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr),
			len - index, ep);
	}
	p_fifo->w = w;
	return len;
}

static unsigned int gdsl_fifo_get_data(gdsl_fifo_t *p_fifo, unsigned char *buff,
	unsigned int len)
{
	unsigned int used = gdsl_fifo_used(p_fifo);
	unsigned int i = 0;
	unsigned int get_len = (len >= used ? used : len);

	debug("gdsl get data, len:%u, base_addr:%#lx, w:%#lx, r:%#lx\n", get_len,
		(unsigned long)p_fifo->base_addr, (unsigned long)p_fifo->w, (unsigned long)p_fifo->r);
	if (used == 0) {
		printf("gdsl fifo used == 0\n");
		return 0;
	}

	while (i < get_len) {
		buff[i] = *(unsigned char *)((unsigned long)p_fifo->r + (unsigned long)p_fifo->base_addr);
		p_fifo->r = (unsigned char *)(((unsigned long)p_fifo->r + 1) % p_fifo->size);
		i++;
	}

	debug("gdsl get data, actual len %d \n", i);

	return i;
}

static void amlbt_usb_hci_cmd_fifo_init(struct usb_device *udev)
{
	debug("cmd fifo init\n");
	if (g_cmd_fifo == 0) {
		g_cmd_fifo = gdsl_fifo_init(W2_HCI_CMD_QUEUE_SIZE, (unsigned char *)(W2_HCI_CMD_QUEUE_ADDR));
		auc_write_word_by_ep_for_bt(udev, W2_HCI_CMD_QUEUE_R, (unsigned int)(unsigned long)g_cmd_fifo->r,
			USB_EP2);
		auc_write_word_by_ep_for_bt(udev, W2_HCI_CMD_QUEUE_W, (unsigned int)(unsigned long)g_cmd_fifo->w,
			USB_EP2);
	}
}

static void amlbt_usb_hci_evt_fifo_init(struct usb_device *udev)
{
	debug("evt fifo init\n");
	if (g_event_fifo == 0) {
		g_event_fifo = gdsl_fifo_init(W2_HCI_EVT_QUEUE_SIZE, (unsigned char *)(W2_HCI_EVT_QUEUE_ADDR));
		auc_write_word_by_ep_for_bt(udev, W2_HCI_EVT_QUEUE_R, (unsigned int)(unsigned long)g_event_fifo->r,
			USB_EP2);
		//printf("event fifo init r: %#lx\n", (unsigned long)g_event_fifo->r);
		auc_write_word_by_ep_for_bt(udev, W2_HCI_EVT_QUEUE_W, (unsigned int)(unsigned long)g_event_fifo->w,
			USB_EP2);
		//printf("event fifo init w : %#lx\n", (unsigned long)g_event_fifo->w);
	}
}

static void amlbt_usb_hci_type_fifo_init(struct usb_device *udev)
{
	debug("hci type fifo init\n");
	if (g_hci_type_fifo == 0) {
		g_hci_type_fifo = gdsl_fifo_init(W2_HCI_TYPE_QUEUE_SIZE, (unsigned char *)W2_HCI_TYPE_QUEUE_ADDR);
		auc_write_word_by_ep_for_bt(udev, W2_HCI_TYPE_QUEUE_R, (unsigned int)(unsigned long)g_hci_type_fifo->r,
			USB_EP2);
		//printf("hci type fifo init r: %#lx\n", (unsigned long)g_hci_type_fifo->r);
		auc_write_word_by_ep_for_bt(udev, W2_HCI_TYPE_QUEUE_W, (unsigned int)(unsigned long)g_hci_type_fifo->w,
			USB_EP2);
		//printf("hci type fifo init w: %#lx\n", (unsigned long)g_hci_type_fifo->w);
	}
}

static void amlbt_usb_send_hci_cmd(struct usb_device *udev, unsigned char *data, unsigned int len)
{
	if (g_cmd_fifo == NULL) {
		printf("bt_usb_hci_cmd_fifo NULL!!!!\n");
		return ;
	}

	debug("hci cmd, len %d, payload:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", len,
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	len = ((len + 3) & 0xFFFFFFFC);//Keep 4 bytes aligned
	//printf("Actual length %d \n", len);

	//step 1: Update the command FIFO read pointer
	g_cmd_fifo->r = (unsigned char *)(unsigned long)auc_read_word_by_ep_for_bt(udev, W2_HCI_CMD_QUEUE_R, USB_EP2);
	//printf("cmd r %#lx\n", (unsigned long)g_cmd_fifo->r);

	//step 2: Write HCI commands to WiFi SRAM
	gdsl_write_data_by_ep(udev, g_cmd_fifo, data, len, USB_EP2);

	//step 3: Update the write pointer and write to WiFi SRAM
	//printf("before write:r:%#lx, w:%#lx\n", (unsigned long)g_cmd_fifo->r, (unsigned long)g_cmd_fifo->w);
	auc_write_word_by_ep_for_bt(udev, W2_HCI_CMD_QUEUE_W, (unsigned long)g_cmd_fifo->w & 0xfff, USB_EP2);

	debug("hci cmd finished, len %#x:w %#lx, r %#lx\n", len, (unsigned long)g_cmd_fifo->w,
		(unsigned long)g_cmd_fifo->r);
}

static int amlbt_usb_recv_event(const aml_chip_type *chip_type, struct usb_device *udev,
	unsigned char *evt, unsigned int *len, unsigned int *cnt)
{
	gdsl_fifo_t read_fifo = {0};
	unsigned int type_size = 0;
	unsigned int evt_size = 0;
	unsigned int i = 0;
	unsigned int reg = 0;
	unsigned int retry_cnt = 0;
	unsigned int read_len = 0;
	unsigned int evt_len = 0;
	unsigned char type_buff[W2_HCI_TYPE_QUEUE_SIZE] = {0};
	unsigned char read_reg[16] = {0};

	debug("hci evt recv\n");

	if (chip_type->family_id == AML_W2L) {
		read_len = USB_POLL_TOTAL_LEN_W2L;
		evt_len = W2l_HCI_EVT_QUEUE_SIZE;
	} else {
		read_len = USB_POLL_TOTAL_LEN;
		evt_len = W2_HCI_EVT_QUEUE_SIZE;
	}

	auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)W2_HCI_TYPE_QUEUE_R,
		read_len, USB_EP2);

	g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|
		(usb_buf[33]<<8)|usb_buf[32]);
	g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|
		(usb_buf[37]<<8)|usb_buf[36]);

	while ((g_event_fifo->w == g_event_fifo->r) || (g_hci_type_fifo->w == g_hci_type_fifo->r)) {
		udelay(50000);
		auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)W2_HCI_TYPE_QUEUE_R,
			read_len, USB_EP2);
		g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|
			(usb_buf[33]<<8)|usb_buf[32]);
		g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|
			(usb_buf[37]<<8)|usb_buf[36]);
		debug("recv retry %d: w:%#lx, r:%#lx\n", retry_cnt, (unsigned long)g_hci_type_fifo->w,
			(unsigned long)g_hci_type_fifo->r);
		retry_cnt++;

		if (retry_cnt > 10) {
			printf("read event or data timeout!\n");
			return -1;
		}
	}

	debug("evt: w:%#lx, r:%#lx\n", (unsigned long)g_event_fifo->w, (unsigned long)g_event_fifo->r);
	debug("type: w:%#lx, r:%#lx\n", (unsigned long)g_hci_type_fifo->w, (unsigned long)g_hci_type_fifo->r);

	//get hci type
	read_fifo.base_addr = &usb_buf[W2_HCI_TYPE_QUEUE_ADDR - W2_HCI_TYPE_QUEUE_R];
	read_fifo.r = g_hci_type_fifo->r;
	read_fifo.w = g_hci_type_fifo->w;
	read_fifo.size = W2_HCI_TYPE_QUEUE_SIZE;

	type_size = gdsl_fifo_get_data(&read_fifo, type_buff, sizeof(type_buff));
	if (!type_size) {
		printf("type queue fatal err\n");
		*len = 0;
		return -1;
	}
	debug("hci type size %d\n", type_size);

	//check hci type
	while (i < type_size) {
		if (type_buff[i] != 0x4) {
			printf("hci type err, i:%u, type:%#x\n", i, type_buff[i]);
			*len = 0;
			return -1;
		}
		i += 4;
	}
	*cnt = type_size/4;
	g_hci_type_fifo->r = read_fifo.r;
	reg = (((unsigned int)(unsigned long)read_fifo.r) & 0xff);
	read_reg[0] = (reg & 0xff);
	read_reg[1] = ((reg >> 8) & 0xff);
	read_reg[2] = ((reg >> 16) & 0xff);
	read_reg[3] = ((reg >> 24) & 0xff);

	//get event
	read_fifo.base_addr = &usb_buf[W2_HCI_EVT_QUEUE_ADDR - W2_HCI_TYPE_QUEUE_R];
	read_fifo.r = g_event_fifo->r;
	read_fifo.w = g_event_fifo->w;
	read_fifo.size = evt_len;
	evt_size = gdsl_fifo_get_data(&read_fifo, evt, evt_len);
	if (!evt_size) {
		printf("event queue fatal err!!!!\n");
		*len = 0;
		*cnt = 0;
		return -1;
	}
	debug("event cnt %d, size %d\n", *cnt, evt_size);
	g_event_fifo->r = read_fifo.r;
	reg = (((unsigned int)(unsigned long)read_fifo.r) & 0x1fff);
	read_reg[4] = (reg & 0xff);
	read_reg[5] = ((reg >> 8) & 0xff);
	read_reg[6] = ((reg >> 16) & 0xff);
	read_reg[7] = ((reg >> 24) & 0xff);
	auc_write_sram_by_ep_for_bt(udev, &read_reg[0],
		(unsigned char *)(unsigned long)(W2_HCI_TYPE_QUEUE_R), 16, USB_EP2);
	*len = evt_size;

	return 0;
}

void amlbt_firmware_init(struct usb_device *udev)
{
	unsigned int st_reg = 0;

	auc_write_word_by_ep_for_bt(udev, REG_PMU_POWER_CFG, 0x20000000, USB_EP2);

	st_reg = auc_read_word_by_ep_for_bt(udev, W2_USB_STATUS_REG, USB_EP2);
	st_reg |= WF_SRAM_FD_INIT_FLAG;
	auc_write_word_by_ep_for_bt(udev, W2_USB_STATUS_REG, st_reg, USB_EP2);

	amlbt_usb_hci_cmd_fifo_init(udev);
	amlbt_usb_hci_type_fifo_init(udev);
	amlbt_usb_hci_evt_fifo_init(udev);

	st_reg &= ~(WF_SRAM_FD_INIT_FLAG);
	auc_write_word_by_ep_for_bt(udev, W2_USB_STATUS_REG, st_reg, USB_EP2);

	//start BT CPU
	auc_write_word_by_ep_for_bt(udev, REG_DEV_RESET,
		(unsigned int)((BIT_CPU | BIT_MAC | BIT_PHY) << DEV_RESET_SW), USB_EP2);
}

static void aml_hci_cmd_process(const aml_chip_type *chip_type, struct usb_device *udev,
	unsigned char *cmd, unsigned int len)
{
	unsigned int evt_len = 0;
	unsigned int evt_cnt = 0;
	int ret;

	memset(evt_buf, 0, sizeof(evt_buf));

	amlbt_usb_send_hci_cmd(udev, cmd, len);
	ret = amlbt_usb_recv_event(chip_type, udev, evt_buf, &evt_len, &evt_cnt);
	if (ret != 0) {
		printf("amlbt_usb_recv_event error!");
		return ;
	}

	debug("amlbt_usb_recv_event cnt:%d, payload:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", evt_cnt,
		evt_buf[0], evt_buf[1], evt_buf[2], evt_buf[3],
		evt_buf[4], evt_buf[5], evt_buf[6], evt_buf[7]);
}

/*
hci set bluetooth address
|opcode|length|bluetooth address|
|0xfc1a|0x06  |address		  |
*/
/*
static void aml_hci_set_bluetooth_address(void)
{
	unsigned char cmd[] = {0x1a, 0xfc, 0x06, 0x58, 0xeb, 0xa4, 0x63, 0x87, 0x44};

	debug("--------------------------------------------------->\n");
	printf("hci send cmd:[set bluetooth address]\n");
	aml_hci_cmd_process(chip_type, udev, cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}
*/
static void aml_hci_reset(const aml_chip_type *chip_type, struct usb_device *udev)
{
	unsigned char cmd[] = {0x03, 0x0c, 0x00};

	debug("--------------------------------------------------->\n");
	printf("hci send cmd:[reset]\n");
	aml_hci_cmd_process(chip_type, udev, cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}

static void aml_hci_read_bt_addr(const aml_chip_type *chip_type, struct usb_device *udev)
{
	unsigned char cmd[] = {0x09, 0x10, 0x00};

	debug("--------------------------------------------------->\n");
	printf("hci send cmd:[read bt addr]\n");
	aml_hci_cmd_process(chip_type, udev, cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}

static void aml_hci_set_vsc_wake_write_data(const aml_chip_type *chip_type, struct usb_device *udev)
{
	// b12 g20 RCU
	unsigned char cmd[] = {0x22, 0xfc, 0x05, 0x19, 0xff, 0x01, 0x0a, 0x0b};
	debug("--------------------------------------------------->\n");
	printf("hci send cmd:[set vsc wake write data]\n");
	aml_hci_cmd_process(chip_type, udev, cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");

	//google RCU
	unsigned char cmd2[] = {0x22, 0xfc, 0x05, 0x13, 0x16, 0x01, 0x04, 0x06};
	debug("--------------------------------------------------->\n");
	printf("hci send cmd2:[set vsc wake write data]2\n");
	aml_hci_cmd_process(chip_type, udev, cmd2, sizeof(cmd2));
}

static int amlbt_add_rc_list(char *mac)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_MAC_LIST; i++) {
		if (!rc_list[i].used) {
			printf("amlbt_add_rc_list %d\n", i);
			rc_list[i].used = 1;
			memcpy(rc_list[i].mac, mac, MAC_ADDR_LEN);
			printf("mac:[%#x,%#x,%#x,%#x,%#x,%#x]\n",
				rc_list[i].mac[0], rc_list[i].mac[1], rc_list[i].mac[2],
				rc_list[i].mac[3], rc_list[i].mac[4], rc_list[i].mac[5]);
			return 1;
		}
	}
	printf("%s list full!!\n", __func__);
	return 0;
}

static ssize_t rc_list_analyze(const char *buf)
{
	char *mac_str = NULL;
	char *mac_token = NULL;
	char *mac_byte_str = NULL;
	char mac[MAC_ADDR_LEN] = {0};
	int i = 0, j = 0;
	int count = strlen(buf);

	printf("rc_list write count:%d\n", count);
	if (count > MAX_USER_BUF_LEN) {
		printf("rc_list too long!:%d, %d\n", count, MAX_USER_BUF_LEN);
		return -ENOMEM;
	}

	memset(rc_list, 0, sizeof(rc_list));
	mac_str = (char *)buf;
	while ((mac_token = strsep(&mac_str, ";")) != NULL) {
		printf("mac_token length: %d\n", (int)strlen(mac_token));

		if (strlen(mac_token) == 0) {
			printf("NULL str\n");
			continue;
		}

		if (strlen(mac_token) != MAC_ADDR_LEN * 2 + 5) {
			printf("Invalid MAC address length: %s\n", mac_token);
			break;
		}

		for (i = MAC_ADDR_LEN - 1, j = 0; i >= 0; i--, j += 3) {
			mac_byte_str = &mac_token[j];
			if (mac_token[j + 2] != ':' && j != 15) {
				printf("Invalid MAC address format: %s i=%d j=%d\n", mac_token, i, j);
				return -EINVAL;
			}
			mac[i] = simple_strtoul(mac_byte_str, NULL, 16);
		}
		printf("parsed mac: [%#x,%#x,%#x,%#x,%#x,%#x]\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		amlbt_add_rc_list(mac);
	}

	return count;
}

static void aml_hci_set_remote_address(const aml_chip_type *chip_type, struct usb_device *udev)
{
	char *temp_env = NULL;
	int i, j;
	temp_env = getenv("remotebtmac");
	printf("hci remotebtmac env:%s \n", temp_env);

	if (temp_env == NULL) {
		return ;
	}
	rc_list_analyze(temp_env);
	unsigned char cmd[9] = {0x55, 0xfc, 0x06};
	for (i = 0; i < MAX_MAC_LIST; i++) {
		if (rc_list[i].used) {
			for (j = 0; j < MAC_ADDR_LEN; j++) {
				cmd[j + 3] = rc_list[i].mac[j];
			}
			printf("rc_list mac: [%#x,%#x,%#x,%#x,%#x,%#x]\n",
				rc_list[i].mac[0], rc_list[i].mac[1], rc_list[i].mac[2], rc_list[i].mac[3],
				rc_list[i].mac[4], rc_list[i].mac[5]);
			debug("--------------------------------------------------->\n");
			printf("hci send cmd:[set bluetooth address]\n");
			aml_hci_cmd_process(chip_type, udev, cmd, sizeof(cmd));
			debug("<---------------------------------------------------\n");
		}
	}
}

static void aml_hci_set_suspend_reg(const aml_chip_type *chip_type, struct usb_device *udev)
{
	unsigned int reg_value = 0;
		reg_value = auc_read_word_by_ep_for_bt(udev, RG_AON_A52, USB_EP2);
		//printf("%s RG_AON_A52:%#x\n", __func__, reg_value);
		reg_value |= (1 << 26);
		auc_write_word_by_ep_for_bt(udev, RG_AON_A52, reg_value, USB_EP2);
		debug("suspend bit26:%#x\n", auc_read_word_by_ep_for_bt(udev, RG_AON_A52, USB_EP2));

		reg_value = auc_read_word_by_ep_for_bt(udev, RG_AON_A52, USB_EP2);
		//printf("%s RG_AON_A52:%#x\n", __func__, reg_value);
		reg_value &= ~(1 << 25);
		auc_write_word_by_ep_for_bt(udev, RG_AON_A52, reg_value, USB_EP2);
		debug("low power bit25:%#x\n", auc_read_word_by_ep_for_bt(udev, RG_AON_A52, USB_EP2));
}

void amlbt_wakeup_set(const aml_chip_type *chip_type, struct usb_device *udev)
{
	//printf("%s: start %u\n", __func__, get_time());

	aml_hci_reset(chip_type, udev);
	aml_hci_read_bt_addr(chip_type, udev);
	//aml_hci_set_bluetooth_address();
	aml_hci_set_vsc_wake_write_data(chip_type, udev);
	aml_hci_set_remote_address(chip_type, udev);
	aml_hci_set_suspend_reg(chip_type, udev);

	printf("%s: end %u\n", __func__, get_time());
}

void amlbt_start(void)
{
	struct usb_device *aml_bt = NULL;
	aml_chip_type *type = NULL;

	aml_bt = aml_mod_get_udev();
	if (!aml_bt) {
		printf("%s:null aml_bt, exit\n", __func__);
		return;
	}

	type = aml_mod_get_type();
	if (!type) {
		printf("%s:null type, exit\n", __func__);
		return;
	}

	if (amlbt_load_firmware(type, aml_bt) == 0) {
		amlbt_firmware_init(aml_bt);
		udelay(50000);
		amlbt_wakeup_set(type, aml_bt);
	}
}

/*
 * Function: amlbt_should_start
 *
 * Conditions to start USB BT:
 * 1. powermode = "standby"
 * 2. powermode = "last" && suspend != "off"
 *
 * Purpose:
 * USB BT needs to download firmware and support wakeup.
 */
int amlbt_should_start(void)
{
	const char *pm_val = env_get("powermode");
	const char *suspend_val = env_get("suspend");

	if (!pm_val)
		return 0;

	if (strcmp(pm_val, "standby") == 0)
		return 1;

	if (strcmp(pm_val, "last") == 0 && suspend_val && strcmp(suspend_val, "off") != 0)
		return 1;

	return 0;
}
