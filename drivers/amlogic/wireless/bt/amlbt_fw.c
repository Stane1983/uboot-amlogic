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

unsigned int g_dbg_level = 0;
#define BT_DEBUG(fmt, arg...) if (g_dbg_level) printf("AMLBT:" fmt, ## arg)

#define USB_BEGIN_LOCK()
#define USB_END_LOCK()

#define CHIP_INTF_REG_BASE      (0xf00000)
#define RG_AON_A16     (CHIP_INTF_REG_BASE + 0x40) //W2L
#define RG_AON_A24     (CHIP_INTF_REG_BASE + 0x60) //W2

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

typedef struct {
	unsigned char rc_manfdata[MANFDATA_LEN];
	unsigned int manfdata_len;
} manf_data;

typedef struct {
	unsigned char rc_manfdata_rclist[MAC_ADDR_LEN*MAX_MAC_LIST];
	unsigned int rc_manfdata_rclist_len;
} manfdata_rclist;

static struct crg_msc_cbw g_cmd_buf;
static gdsl_fifo_t *g_cmd_fifo;
static gdsl_fifo_t *g_event_fifo;
static gdsl_fifo_t *g_hci_type_fifo;
static unsigned char usb_buf[4096];
static rc_list_t rc_list[MAX_MAC_LIST] = {0};
static unsigned char evt_buf[1024];

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

	BT_DEBUG("%s: iccm_size:%#x,%#x,%#x,%#x\n", __func__, temp[0], temp[1], temp[2], temp[3]);
	BT_DEBUG("%s: dccm_size:%#x,%#x,%#x,%#x\n", __func__, temp[4], temp[5], temp[6], temp[7]);

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
			BT_DEBUG("bt_usb_download_firmware iccm1 offset %#x, write_len %#x\n", offset, remain_len);
		} else {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.iccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, USB_DOWNLOAD_LEN, USB_EP2);
			offset += USB_DOWNLOAD_LEN;
			remain_len -= USB_DOWNLOAD_LEN;
			temp_addr += USB_DOWNLOAD_LEN;
			BT_DEBUG("bt_usb_download_firmware iccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
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
			BT_DEBUG("bt_usb_download_firmware dccm1 offset %#x, write_len %#x\n", offset, remain_len);
		} else {
			auc_write_sram_by_ep_for_bt(udev, &(aml_fw_info.dccm[offset]),
				(unsigned char *)(unsigned long)temp_addr, USB_DOWNLOAD_LEN, USB_EP2);
			offset += USB_DOWNLOAD_LEN;
			remain_len -= USB_DOWNLOAD_LEN;
			temp_addr += USB_DOWNLOAD_LEN;
			BT_DEBUG("bt_usb_download_firmware dccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
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

	BT_DEBUG("%s: end:%u\n", __func__, get_time());
	return 0;
}

static gdsl_fifo_t *gdsl_fifo_init(unsigned int len, unsigned char *base_addr)
{
	gdsl_fifo_t *p_fifo = (gdsl_fifo_t *)kzalloc(sizeof(gdsl_fifo_t), GFP_ATOMIC);

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

static unsigned int gdsl_fifo_get_data(gdsl_fifo_t *p_fifo, unsigned char *buff,
	unsigned int len)
{
	unsigned int used = gdsl_fifo_used(p_fifo);
	unsigned int i = 0;
	unsigned int get_len = (len >= used ? used : len);

	if (used == 0) {
		printf("gdsl fifo used == 0\n");
		return 0;
	}

	while (i < get_len) {
		buff[i] = *(unsigned char *)((unsigned long)p_fifo->r + (unsigned long)p_fifo->base_addr);
		p_fifo->r = (unsigned char *)(((unsigned long)p_fifo->r + 1) % p_fifo->size);
		i++;
	}

	return i;
}

static void amlbt_usb_send_hci_cmd(const aml_chip_type *chip_type, struct usb_device *udev, unsigned char *data, unsigned int len)
{
	enum usb_endpoint_num current_endpoint;
	unsigned int hci_cmd_queue_r;
	unsigned int hci_cmd_queue_w;

	if (g_cmd_fifo == NULL) {
		printf("bt_usb_hci_cmd_fifo NULL!!!!\n");
		return ;
	}

	current_endpoint = USB_EP2;
	hci_cmd_queue_r = W2_HCI_CMD_QUEUE_R;
	hci_cmd_queue_w = W2_HCI_CMD_QUEUE_W;

	printf("hci cmd, len %d, payload:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", len,
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	len = ((len + 3) & 0xFFFFFFFC);//Keep 4 bytes aligned
	BT_DEBUG("Actual length %d \n", len);

	//step 1: Update the command FIFO read pointer
	g_cmd_fifo->r = (unsigned char *)(unsigned long)auc_read_word_by_ep_for_bt(udev, hci_cmd_queue_r, current_endpoint);
	BT_DEBUG("cmd r %#lx\n", (unsigned long)g_cmd_fifo->r);

	//step 2: Write HCI commands to WiFi SRAM
	gdsl_write_data_by_ep(udev, g_cmd_fifo, data, len, current_endpoint);

	//step 3: Update the write pointer and write to WiFi SRAM
	BT_DEBUG("before write:r:%#lx, w:%#lx\n", (unsigned long)g_cmd_fifo->r, (unsigned long)g_cmd_fifo->w);
	auc_write_word_by_ep_for_bt(udev, hci_cmd_queue_w, (unsigned long)g_cmd_fifo->w & 0xfff, current_endpoint);

	BT_DEBUG("hci cmd finished, len %#x:w %#lx, r %#lx\n", len, (unsigned long)g_cmd_fifo->w,
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
	enum usb_endpoint_num current_endpoint;
	unsigned int hci_type_queue_r;
	unsigned long hci_type_queue_addr;
	unsigned long hci_evt_queue_addr;
	BT_DEBUG("hci evt recv\n");

	current_endpoint = USB_EP2;
	hci_type_queue_r = W2_HCI_TYPE_QUEUE_R;
	hci_type_queue_addr = W2_HCI_TYPE_QUEUE_ADDR;
	hci_evt_queue_addr = W2_HCI_EVT_QUEUE_ADDR;

	read_len = USB_POLL_TOTAL_LEN;
	evt_len = W2_HCI_EVT_QUEUE_SIZE;

	auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)hci_type_queue_r,
		read_len, current_endpoint);

	g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|
		(usb_buf[33]<<8)|usb_buf[32]);
	g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|
		(usb_buf[37]<<8)|usb_buf[36]);

	while ((g_event_fifo->w == g_event_fifo->r) || (g_hci_type_fifo->w == g_hci_type_fifo->r)) {
		udelay(50000);
		auc_read_sram_by_ep_for_bt(udev, usb_buf, (unsigned char *)(unsigned long)hci_type_queue_r,
			read_len, current_endpoint);
		g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|
			(usb_buf[33]<<8)|usb_buf[32]);
		g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|
			(usb_buf[37]<<8)|usb_buf[36]);
		printf("recv retry %d: w:%#lx, r:%#lx\n", retry_cnt, (unsigned long)g_hci_type_fifo->w,
			(unsigned long)g_hci_type_fifo->r);
		retry_cnt++;

		if (retry_cnt > 10) {
			printf("read event or data timeout!\n");
			return -1;
		}
	}

	//get hci type
	read_fifo.base_addr = &usb_buf[hci_type_queue_addr - hci_type_queue_r];
	read_fifo.r = g_hci_type_fifo->r;
	read_fifo.w = g_hci_type_fifo->w;
	read_fifo.size = W2_HCI_TYPE_QUEUE_SIZE;

	type_size = gdsl_fifo_get_data(&read_fifo, type_buff, sizeof(type_buff));
	if (!type_size) {
		printf("type queue fatal err\n");
		*len = 0;
		return -1;
	}
	BT_DEBUG("hci type size %d\n", type_size);

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
	read_fifo.base_addr = &usb_buf[hci_evt_queue_addr - hci_type_queue_r];
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
	BT_DEBUG("event cnt %d, size %d\n", *cnt, evt_size);
	g_event_fifo->r = read_fifo.r;
	reg = (((unsigned int)(unsigned long)read_fifo.r) & 0x1fff);
	read_reg[4] = (reg & 0xff);
	read_reg[5] = ((reg >> 8) & 0xff);
	read_reg[6] = ((reg >> 16) & 0xff);
	read_reg[7] = ((reg >> 24) & 0xff);
	auc_write_sram_by_ep_for_bt(udev, &read_reg[0],
		(unsigned char *)(unsigned long)(hci_type_queue_r), 16, current_endpoint);
	*len = evt_size;

	return 0;
}


static void aml_hci_cmd_process(const aml_chip_type *chip_type, struct usb_device *udev,
	unsigned char *cmd, unsigned int len)
{
	unsigned int evt_len = 0;
	unsigned int evt_cnt = 0;
	int ret;

	memset(evt_buf, 0, sizeof(evt_buf));

	amlbt_usb_send_hci_cmd(chip_type, udev, cmd, len);

	ret = amlbt_usb_recv_event(chip_type, udev, evt_buf, &evt_len, &evt_cnt);
	if (ret != 0) {
		printf("amlbt_usb_recv_event error!");
		return ;
	}
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
}

static unsigned char hex_char_to_val(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	else if (c >= 'A' && c <= 'F')
		return 10 + (c - 'A');
	printf("%s: invalid hex char '%c' (0x%02x)\n", __func__, c, (unsigned char)c);
	return 0;
}

static int amlbt_read_conf_data(const aml_chip_type *chip_type, manf_data *manf)
{
	unsigned char *file_buf = NULL;
	char *manf_data_start = NULL;
	char *line_end = NULL;
	char *hex_clean = NULL;
	char *file_path = AMLBT_CONF;
	int file_size = 0;
	int manf_line_len = 0;
	int clean_idx = 0;
	int i = 0;

	if (!manf) {
		printf("%s: manf pointer is NULL!\n", __func__);
		return -1;
	}

	memset(manf, 0, sizeof(manf_data));

	file_size = model_get_file_size(file_path);
	if (file_size <= 0) {
		printf("%s: get file size fail (path: %s, size: %d)\n", __func__,
			file_path, file_size);
		return -1;
	}
	file_buf = (unsigned char *)malloc(file_size + 1);
	if (file_buf == NULL) {
		printf("%s: malloc file_buf fail (need %d bytes)\n", __func__, file_size + 1);
		return -1;
	}

	memset(file_buf, 0, file_size + 1);
	if (model_read_file(file_path, file_buf, file_size) <= 0) {
		printf("%s: read file fail (path: %s)\n", __func__, file_path);
		free(file_buf);
		return -1;
	}

	/* AML_BT_USB wakeup config */
	manf_data_start = strstr((char *)file_buf, "W1UManfData=");
	if (manf_data_start == NULL) {
		printf("%s: 'W2UManfData=' not found in %s\n", __func__, file_path);
		free(file_buf);
		return -1;
	}
	manf_data_start += strlen("W1UManfData=");

	line_end = manf_data_start;
	while (*line_end != '\0' && *line_end != '\n' && *line_end != '\r')
		line_end++;

	manf_line_len = line_end - manf_data_start;
	if (manf_line_len <= 0) {
		printf("%s: ManfData is empty (line len: %d)\n", __func__, manf_line_len);
		free(file_buf);
		return -1;
	}

	hex_clean = (char *)malloc(manf_line_len + 1);
	if (hex_clean == NULL) {
		printf("%s: malloc hex_clean fail (need %d bytes)\n", __func__, manf_line_len + 1);
		free(file_buf);
		return -1;
	}
	memset(hex_clean, 0, manf_line_len + 1);

	for (i = 0; i < manf_line_len; i++) {
		if (manf_data_start[i] != ' ')
			hex_clean[clean_idx++] = manf_data_start[i];
	}

	if (clean_idx == 0) {
		printf("%s: no valid hex data after filtering\n", __func__);
		free(hex_clean);
		free(file_buf);
		return -1;
	}

	manf->manfdata_len = clean_idx / 2;
	if (manf->manfdata_len > sizeof(manf->rc_manfdata)) {
		manf->manfdata_len = sizeof(manf->rc_manfdata);
		printf("%s: ManfData too long (max %ld bytes), truncated\n",
				__func__, sizeof(manf->rc_manfdata));
	}

	for (i = 0; i < manf->manfdata_len; i++) {
		unsigned char high_nibble = hex_char_to_val(hex_clean[2*i]);
		unsigned char low_nibble = hex_char_to_val(hex_clean[2*i + 1]);

		manf->rc_manfdata[i] = (high_nibble << 4) | low_nibble;
	}

	free(hex_clean);
	free(file_buf);
	return 0;
}

static void amlbt_write_manfdata_to_firmware_for_sram(const aml_chip_type *chip_type, struct usb_device *udev)
{
	manf_data local_manf_data;
	unsigned char manf_buf[256];
	int i = 0;

	if (amlbt_read_conf_data(chip_type, &local_manf_data) != 0) {
		printf("Failed to read manf data, aborting write\n");
		return;
	}

	BT_DEBUG("FW_MANFDATA_ADDR = %#x\n", FW_MANFDATA_ADDR);
	memset(manf_buf, 0, sizeof(manf_buf));
	auc_write_sram_by_ep_for_bt(udev, manf_buf, (unsigned char *)FW_MANFDATA_ADDR,
		sizeof(manf_buf), USB_EP2);

	printf("ManfData len: %d\n", local_manf_data.manfdata_len);
	printf("ManfData:[ ");
	for (i = 0; i < local_manf_data.manfdata_len; i++)
		printf("0x%02x ", local_manf_data.rc_manfdata[i]);
	printf("]\n");
	auc_write_sram_by_ep_for_bt(udev, local_manf_data.rc_manfdata,
		(unsigned char *)FW_MANFDATA_ADDR, MANFDATA_LEN, USB_EP2);
}

static void amlbt_write_manfdata_to_firmware_for_cmd(const aml_chip_type *chip_type, struct usb_device *udev)
{
	manf_data local_manf_data;
	int i = 0;
	unsigned int manf_buf_len = 3;  // (2 bytes opcode)+(1 bytes len)
	uint8_t manf_buf[256] = {HCI_VSC_WAKE_WRITE_DATA & 0xff, (HCI_VSC_WAKE_WRITE_DATA >> 8) & 0xff};

	if (amlbt_read_conf_data(chip_type, &local_manf_data) != 0) {
		printf("Failed to read manf data, aborting write\n");
		return;
	}

	printf("ManfData len: %d\n", local_manf_data.manfdata_len);
	printf("ManfData:[ ");
	for (i = 0; i < 40; i++) {
		printf("0x%02x ", local_manf_data.rc_manfdata[i]);
	}
	printf("]\n");

	manf_buf[2] = 0x29;    // (4*10 bytes W1UManfData)+(1 bytes cnt)
	manf_buf[3] = 0x04;    // support B12 and G20 cnt=4
	memcpy(manf_buf + 4, local_manf_data.rc_manfdata, 40);
	manf_buf_len += 0x29;

	aml_hci_cmd_process(chip_type, udev, manf_buf, manf_buf_len);
}

static int amlbt_add_rc_list(char *mac)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_MAC_LIST; i++) {
		if (!rc_list[i].used) {
			BT_DEBUG("amlbt_add_rc_list %d\n", i);
			rc_list[i].used = 1;
			memcpy(rc_list[i].mac, mac, MAC_ADDR_LEN);
			BT_DEBUG("mac:[%#x,%#x,%#x,%#x,%#x,%#x]\n",
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

	BT_DEBUG("rc_list write count:%d\n", count);
	if (count > MAX_USER_BUF_LEN) {
		printf("rc_list too long!:%d, %d\n", count, MAX_USER_BUF_LEN);
		return -ENOMEM;
	}

	memset(rc_list, 0, sizeof(rc_list));
	mac_str = (char *)buf;
	while ((mac_token = strsep(&mac_str, ";")) != NULL) {
		BT_DEBUG("mac_token length: %d\n", (int)strlen(mac_token));

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
		BT_DEBUG("parsed mac: [%#x,%#x,%#x,%#x,%#x,%#x]\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		amlbt_add_rc_list(mac);
	}

	return count;
}

static int amlbt_get_remote_address(manfdata_rclist *rclist, struct usb_device *udev)
{
	char *temp_env = NULL;
	int i, j;
	temp_env = getenv("remotebtmac");
	BT_DEBUG("hci remotebtmac env:%s \n", temp_env);

	if (temp_env == NULL) {
		return -1;
	}

	if (!rclist) {
		printf("%s: manf pointer is NULL!\n", __func__);
		return -1;
	}

	rc_list_analyze(temp_env);
	memset(rclist, 0, sizeof(manfdata_rclist));
	for (i = 0; i < MAX_MAC_LIST; i++) {
		if (rc_list[i].used) {
			for (j = 0; j < MAC_ADDR_LEN; j++) {
				rclist->rc_manfdata_rclist[((i + 1)*6) - 1 - j] = rc_list[i].mac[j];
				rclist->rc_manfdata_rclist_len++;
			}
		}
	}

	return 0;
}

static void amlbt_write_rclist_to_firmware(struct usb_device *udev)
{
	manfdata_rclist local_manfdata_rclist;
	unsigned char manf_rclist_buf[6*8];
	int i = 0;

	if (amlbt_get_remote_address(&local_manfdata_rclist, udev) != 0) {
		printf("Failed to read manfdata_rclist, aborting write\n");
		return;
	}

	BT_DEBUG("FIFO_FW_RC_LIST_ADDR = %#x\n", FIFO_FW_RC_LIST_ADDR);
	memset(manf_rclist_buf, 0, sizeof(manf_rclist_buf));
	auc_write_sram_by_ep_for_bt(udev, manf_rclist_buf, (unsigned char *)FIFO_FW_RC_LIST_ADDR,
		sizeof(manf_rclist_buf), USB_EP2);

	printf("rclist len: %d\n", local_manfdata_rclist.rc_manfdata_rclist_len);
	printf("rclist:[ ");
	for (i = 0; i < local_manfdata_rclist.rc_manfdata_rclist_len; i++) {
		printf("%#x ", local_manfdata_rclist.rc_manfdata_rclist[i]);
	}
	printf("]\n");
	auc_write_sram_by_ep_for_bt(udev, local_manfdata_rclist.rc_manfdata_rclist,
		(unsigned char *)FIFO_FW_RC_LIST_ADDR, MANFDATA_LEN, USB_EP2);
}

static void aml_hci_set_shutdown_reg(const aml_chip_type *chip_type, struct usb_device *udev)
{
	unsigned int reg_value = 0;
	if (chip_type->family_id == AML_W2) {
		reg_value = auc_read_word_by_ep_for_bt(udev, RG_AON_A24, USB_EP2);
		BT_DEBUG("%s RG_AON_A24 %#x :%#x\n", __func__, RG_AON_A24, reg_value);
		reg_value |= (1 << 27);
		auc_write_word_by_ep_for_bt(udev, RG_AON_A24, reg_value, USB_EP2);
		BT_DEBUG("suspend bit27:%#x\n", auc_read_word_by_ep_for_bt(udev, RG_AON_A24, USB_EP2));
	} else if (chip_type->family_id == AML_W2L) {
		reg_value = auc_read_word_by_ep_for_bt(udev, RG_AON_A16, USB_EP2);
		BT_DEBUG("%s RG_AON_A16 %#x :%#x\n", __func__, RG_AON_A16, reg_value);
		reg_value |= (1 << 28);
		auc_write_word_by_ep_for_bt(udev, RG_AON_A16, reg_value, USB_EP2);
		BT_DEBUG("suspend bit28:%#x\n", auc_read_word_by_ep_for_bt(udev, RG_AON_A16, USB_EP2));
	}
}

void amlbt_wakeup_set(const aml_chip_type *chip_type, struct usb_device *udev)
{
	enum usb_endpoint_num current_endpoint;

	BT_DEBUG("%s: start %u\n", __func__, get_time());

	if (chip_type->family_id == AML_W2L) {
		current_endpoint = USB_EP2;
		//start BT CPU
		auc_write_word_by_ep_for_bt(udev, REG_DEV_RESET,
			(unsigned int)((BIT_CPU | BIT_MAC | BIT_PHY) << DEV_RESET_SW), current_endpoint);
		// write rclist for manfdata
		amlbt_write_rclist_to_firmware(udev);
		// write wakeup parameters for manfdata
		amlbt_write_manfdata_to_firmware_for_cmd(chip_type, udev);
	} else if (chip_type->family_id == AML_W2) {
		current_endpoint = USB_EP2;
		// write rclist for manfdata
		amlbt_write_rclist_to_firmware(udev);
		// write wakeup parameters for manfdata
		amlbt_write_manfdata_to_firmware_for_sram(chip_type, udev);
		//start BT CPU
		auc_write_word_by_ep_for_bt(udev, REG_DEV_RESET,
			(unsigned int)((BIT_CPU | BIT_MAC | BIT_PHY) << DEV_RESET_SW), current_endpoint);
	}

	//set shutdown bit
	aml_hci_set_shutdown_reg(chip_type, udev);

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
