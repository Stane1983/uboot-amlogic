#include <common.h>
#include <command.h>
#include <asm/processor.h>
#include <linux/compiler.h>
#include <linux/compat.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>

#include <malloc.h>
#include <usb.h>
#include <aml_fw.h>
#include <env.h>
#ifndef getenv
#define getenv env_get
#endif


#define USB_BEGIN_LOCK()

#define USB_END_LOCK()

#define CHIP_INTF_REG_BASE	(0xf00000)
#define RG_AON_A52		(CHIP_INTF_REG_BASE + 0xd0)

static aml_chip_type aml_type = {0};
static char *chip_list[] = {"NULL\n", "w1\n", "w1u\n", "w2\n"};
static char *rev_list[] = {"RevA\n", "RevB\n", "RevC\n", "RevD\n", "RevE\n", "RevF\n"};
static char *itf_list[] = {"sdio\n", "usb\n", "pcie\n"};
static struct usb_device *aml_dev;
static struct crg_msc_cbw g_cmd_buf;
static gdsl_fifo_t *g_cmd_fifo;
static gdsl_fifo_t *g_event_fifo;
static gdsl_fifo_t *g_hci_type_fifo;
static unsigned char usb_buf[4096];
static unsigned char evt_buf[1024];

#define MAC_ADDR_LEN 6
#define MAX_MAC_LIST 8
#define MAX_USER_BUF_LEN MAX_MAC_LIST*(17+1)
typedef struct _rc_list_t
{
    char mac[MAC_ADDR_LEN];
    char used;
} rc_list_t;
static rc_list_t rc_list[MAX_MAC_LIST] = {0};

static const unsigned char w2u_iccm[] = {
#include CHIP_TYPE_W2U_ICCM
};

static const unsigned char w2u_dccm[] = {
#include CHIP_TYPE_W2U_DCCM
};

static void aml_type_info(aml_chip_type *type)
{
	printf("+++++++++++++++++++++++++++++++\n");
	printf("|version:2024-0810-1354	   |\n");
	printf("+++++++++++++++++++++++++++++++\n");
	if (type->family_id < 4 && type->family_rev < 6 && type->interface < 3) {
		printf("aml chip:%s", chip_list[type->family_id]);
		printf("aml rev :%s", rev_list[type->family_rev]);
		printf("aml interface:%s", itf_list[type->interface]);
	} else {
		printf("invalid id!\n");
	}

}

static struct usb_device *qiu_usb_find_device(int devnum)
{
	struct usb_device *udev;
	struct udevice *hub;
	struct uclass *uc;
	int ret;

	/* Device addresses start at 1 */
	devnum++;
	ret = uclass_get(UCLASS_USB_HUB, &uc);
	if (ret)
		return NULL;

	uclass_foreach_dev(hub, uc) {
		struct udevice *dev;

		if (!device_active(hub))
			continue;
		udev = dev_get_parent_priv(hub);
		if (udev->devnum == devnum)
			return udev;

		for (device_find_first_child(hub, &dev);
			dev;
			device_find_next_child(&dev)) {
			if (!device_active(hub))
				continue;

			udev = dev_get_parent_priv(dev);
			if (udev->devnum == devnum)
				return udev;
		}
	}
	return NULL;
}


aml_chip_type *get_aml_dev(void)
{
	struct usb_device *udev = NULL;

	int d;
	for (d = 0; d < USB_MAX_DEVICE; d++) {
		udev = qiu_usb_find_device(d);
		if (udev == NULL)
			break;
		if (udev->descriptor.bDescriptorType == USB_DT_DEVICE) {
			if (udev->descriptor.idVendor == AML_W2_VENDOR) {
				aml_type.interface = udev->descriptor.idProduct & 0x07;
				aml_type.wireless = 0;
				aml_type.family_rev = (udev->descriptor.idProduct >> 6) & 0x03;
				aml_type.family_id = (udev->descriptor.idProduct >> 9) & 0x1f;
				aml_type.reserved = 0;
				aml_type_info(&aml_type);
				aml_dev = udev;
				return &aml_type;
			}
		}
	}
	return NULL;
}

static void auc_build_cbw(struct crg_msc_cbw *cbw_buf,
			  unsigned char dir,
			  unsigned int len,
			  unsigned char cdb1,
			  unsigned int cdb2,
			  unsigned long cdb3,
			  unsigned long cdb4)
{
	cbw_buf->sig = AML_SIG_CBW;
	cbw_buf->tag = 0x5da729a0;
	cbw_buf->data_len = len;
	cbw_buf->flag = dir; //direction
	cbw_buf->len = 16; //command length
	cbw_buf->lun = 0;

	cbw_buf->cdb[0] = cdb1;
	cbw_buf->cdb[1] = cdb2; // read or write addr
	cbw_buf->cdb[2] = (unsigned int)(unsigned long)cdb3;
	cbw_buf->cdb[3] = cdb4; //read or write data length
}

static int auc_write_reg_by_ep(unsigned int addr, unsigned int value, unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;

	USB_BEGIN_LOCK();
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
	/* cmd stage */
	ret = usb_bulk_msg(aml_dev, (unsigned int)usb_sndbulkpipe(aml_dev, ep), (void *)&g_cmd_buf, sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_write_reg_by_ep Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, value: 0x%x\n", ret, ep, addr, len, value);
		USB_END_LOCK();
		return ret;
	}
	USB_END_LOCK();

	return actual_length; //bt write maybe use the value
}

static void auc_write_word_by_ep_for_bt(unsigned int addr, unsigned int data, unsigned int ep)
{
	int len = 4;

	switch (ep) {
	case USB_EP2:
		auc_write_reg_by_ep(addr, data, len, ep);
		break;
	default:
		printf("EP-%d unsupported!\n", ep);
		break;
	}
}

static unsigned int auc_read_reg_by_ep(unsigned int addr, unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned int reg_data;
	unsigned char *data = NULL;

	USB_BEGIN_LOCK();

	data = (unsigned char *)kzalloc(len, GFP_KERNEL|GFP_ATOMIC);

	if (!data) {
		printf("auc_read_reg_by_ep data malloc fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
		USB_END_LOCK();
		return -ENOMEM;
	}
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

	/* cmd stage */
	ret = usb_bulk_msg(aml_dev, usb_sndbulkpipe(aml_dev, ep), (void *)&g_cmd_buf, sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_reg_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
		kfree(data);
		USB_END_LOCK();
		return ret;
	}

	/* data stage */
	ret = usb_bulk_msg(aml_dev, usb_rcvbulkpipe(aml_dev, ep), (void *)data, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_reg_by_ep data Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
		kfree(data);
		USB_END_LOCK();
		return ret;
	}

	memcpy(&reg_data, data, actual_length);
	kfree(data);
	USB_END_LOCK();

	return reg_data;
}

static unsigned int auc_read_word_by_ep_for_bt(unsigned int addr, unsigned int ep)
{
	int len = 4;
	unsigned int value = 0;

	switch (ep) {
	case USB_EP2:
		value = auc_read_reg_by_ep(addr, len, ep);
		break;
	default:
		printf("EP-%d unsupported!\n", ep);
		break;
	}
	return value;
}

static void auc_write_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned char *kmalloc_buf = NULL;

	USB_BEGIN_LOCK();
	{
		memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
		auc_build_cbw(&g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len);
		/* cmd stage */
		ret = usb_bulk_msg(aml_dev, usb_sndbulkpipe(aml_dev, ep), (void *)&g_cmd_buf, sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
		if (ret) {
			printf("auc_write_sram_by_ep 2 Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
			printf("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
			USB_END_LOCK();
			return;
		}

		kmalloc_buf = (unsigned char *)kzalloc(len,  GFP_ATOMIC);
		if (kmalloc_buf == NULL) {
			printf("kmalloc buf fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
			USB_END_LOCK();
			return;
		}

		memcpy(kmalloc_buf, pdata, len);
		/* data stage */
		ret = usb_bulk_msg(aml_dev, usb_sndbulkpipe(aml_dev, ep), (void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
		if (ret) {
			printf("auc_write_sram_by_ep data Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d\n", ret, ep, addr, len);
			kfree(kmalloc_buf);
			USB_END_LOCK();
			return;
		}
		kfree(kmalloc_buf);
	}
	USB_END_LOCK();
}

static void auc_write_sram_by_ep_for_bt(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
	if (len == 0) {
		printf("EP-%d write len err!\n", ep);
		return;
	}

	switch (ep) {
	case USB_EP2:
		auc_write_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
		break;
	default:
		printf("EP-%d unsupported!\n", ep);
		break;
	}
}

static void auc_read_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
	int ret = 0;
	int actual_length = 0;
	unsigned char *kmalloc_buf = NULL;

	USB_BEGIN_LOCK();
	memset(&g_cmd_buf, 0, sizeof(g_cmd_buf));
	auc_build_cbw(&g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
	/* cmd stage */
	ret = usb_bulk_msg(aml_dev, usb_sndbulkpipe(aml_dev, ep), (void *)&g_cmd_buf, sizeof(g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_sram_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
		printf("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
		USB_END_LOCK();
		return;
	}

	kmalloc_buf = (unsigned char *)kzalloc(len, GFP_ATOMIC);
	if (kmalloc_buf == NULL) {
		printf("kmalloc buf fail, ep: %d, len: %d\n", ep, len);
		USB_END_LOCK();
		return;
	}

	/* data stage */
	ret = usb_bulk_msg(aml_dev, usb_rcvbulkpipe(aml_dev, ep), (void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("auc_read_sram_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
		kfree(kmalloc_buf);
		USB_END_LOCK();
		return;
	}

	memcpy(pdata, kmalloc_buf, actual_length);
	kfree(kmalloc_buf);

	USB_END_LOCK();
}


static void auc_read_sram_by_ep_for_bt(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
	if (len == 0) {
		printf("EP-%d read len err!\n", ep);
		return;
	}
	switch (ep) {
	case USB_EP2:
		auc_read_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
		break;
	default:
		printf("EP-%d unsupported!\n", ep);
		break;
	}
}

void load_aml_firmware_dev(aml_chip_type *type)
{
	unsigned int offset = 0;
	unsigned int remain_len = 0;
	unsigned int iccm_base_addr = BT_ICCM_AHB_BASE + 256*1024;
	unsigned int dccm_base_addr = BT_DCCM_AHB_BASE;
	unsigned int download_size = sizeof(w2u_iccm);

	if (type->family_id == AML_W2 && type->interface == AML_INTF_USB) {
		remain_len = (download_size - offset);
		while (offset < download_size) {
			if (remain_len < USB_DOWNLOAD_LEN) {
				auc_write_sram_by_ep_for_bt((unsigned char *)&w2u_iccm[offset], (unsigned char *)(unsigned long)iccm_base_addr, remain_len, USB_EP2);
				offset += remain_len;
				iccm_base_addr += remain_len;
				//printf("bt_usb_download_firmware iccm1 offset %#x, write_len %#x\n", offset, remain_len);
			} else {
				auc_write_sram_by_ep_for_bt((unsigned char *)&w2u_iccm[offset], (unsigned char *)(unsigned long)iccm_base_addr, USB_DOWNLOAD_LEN, USB_EP2);
				offset += USB_DOWNLOAD_LEN;
				remain_len -= USB_DOWNLOAD_LEN;
				iccm_base_addr += USB_DOWNLOAD_LEN;
				//printf("bt_usb_download_firmware iccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
			}
		}
/******************************iccm check*****************************************/
		offset = 0;
		iccm_base_addr = BT_ICCM_AHB_BASE + 256*1024;
		while (offset < sizeof(w2u_iccm)) {
			memset(usb_buf, 0, sizeof(usb_buf));
			auc_read_sram_by_ep_for_bt(usb_buf, (unsigned char *)(unsigned long)iccm_base_addr, sizeof(usb_buf), USB_EP2);
			if (memcmp(usb_buf, &w2u_iccm[offset], sizeof(usb_buf))) {
				printf("iccm check err!! offset %#x\n", offset);
				printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", usb_buf[0], usb_buf[1], usb_buf[2], usb_buf[3],
					usb_buf[4], usb_buf[5], usb_buf[6], usb_buf[7]);
				printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", w2u_iccm[offset], w2u_iccm[offset+1], w2u_iccm[offset+2],
					w2u_iccm[offset+3], w2u_iccm[offset+4], w2u_iccm[offset+5], w2u_iccm[offset+6], w2u_iccm[offset+7]);
				return ;
			}
			offset += sizeof(usb_buf);
			iccm_base_addr += sizeof(usb_buf);
		}
/*********************************************************************************/

		download_size = sizeof(w2u_dccm);

		offset = 0;
		remain_len = download_size;
		while (offset < download_size) {
			if (remain_len < USB_DOWNLOAD_LEN) {
				auc_write_sram_by_ep_for_bt((unsigned char *)&w2u_dccm[offset], (unsigned char *)(unsigned long)dccm_base_addr, remain_len, USB_EP2);
				offset += remain_len;
				dccm_base_addr += remain_len;
				//printf("bt_usb_download_firmware dccm1 offset %#x, write_len %#x\n", offset, remain_len);
			} else {
				auc_write_sram_by_ep_for_bt((unsigned char *)&w2u_dccm[offset], (unsigned char *)(unsigned long)dccm_base_addr, USB_DOWNLOAD_LEN, USB_EP2);
				offset += USB_DOWNLOAD_LEN;
				remain_len -= USB_DOWNLOAD_LEN;
				dccm_base_addr += USB_DOWNLOAD_LEN;
				//printf("bt_usb_download_firmware dccm2 offset %#x, write_len %#x\n", offset, USB_DOWNLOAD_LEN);
			}
		}
/******************************dccm check*****************************************/
		offset = 0;
		dccm_base_addr = BT_DCCM_AHB_BASE;
		while (offset < sizeof(w2u_dccm)) {
			memset(usb_buf, 0, sizeof(usb_buf));
			auc_read_sram_by_ep_for_bt(usb_buf, (unsigned char *)(unsigned long)dccm_base_addr, sizeof(usb_buf), USB_EP2);
			if (memcmp(usb_buf, &w2u_dccm[offset], sizeof(usb_buf))) {
				printf("dccm check err!! offset %#x\n", offset);
				printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", usb_buf[0], usb_buf[1], usb_buf[2], usb_buf[3],
					usb_buf[4], usb_buf[5], usb_buf[6], usb_buf[7]);
				printf("%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", w2u_dccm[offset], w2u_dccm[offset+1], w2u_dccm[offset+2],
					w2u_dccm[offset+3], w2u_dccm[offset+4], w2u_dccm[offset+5], w2u_dccm[offset+6], w2u_dccm[offset+7]);
				return ;
			}
			offset += sizeof(usb_buf);
			dccm_base_addr += sizeof(usb_buf);
		}
/*********************************************************************************/
	}
	printf("load_aml_firmware_dev success!!\n");
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

unsigned int gdsl_write_data_by_ep(gdsl_fifo_t *p_fifo, unsigned char *data, unsigned int len, unsigned int ep)
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
		auc_write_sram_by_ep_for_bt(data, p_fifo->base_addr, len, ep);
		p_fifo->w = (unsigned char *)(unsigned long)(len % p_fifo->size);
		return len;
	}

	while (i < len) {
		w = (unsigned char *)(((unsigned long)w + 1) % p_fifo->size);
		i++;
		if (w == 0) {
			debug("w s2 %#lx\n", (unsigned long)p_fifo->w);
			auc_write_sram_by_ep_for_bt(data,
				(unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr), i, ep);
			p_fifo->w = 0;
			index = i;
		}
	}
	if (index < len) {
		debug("w s3 %#lx\n", (unsigned long)p_fifo->w);
		auc_write_sram_by_ep_for_bt(&data[index],
			(unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr), len - index, ep);
	}
	p_fifo->w = w;
	return len;
}

static unsigned int gdsl_fifo_get_data(gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len)
{
	unsigned int used = gdsl_fifo_used(p_fifo);
	unsigned int i = 0;
	unsigned int get_len = (len >= used ? used : len);

	debug("gdsl get data, len:%d, base_addr:%#lx, w:%#lx, r:%#lx\n", get_len,
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

static void amlbt_usb_hci_cmd_fifo_init(void)
{
	debug("cmd fifo init\n");
	if (g_cmd_fifo == 0) {
		g_cmd_fifo = gdsl_fifo_init(W2_HCI_CMD_QUEUE_SIZE, (unsigned char *)(W2_HCI_CMD_QUEUE_ADDR));
		auc_write_word_by_ep_for_bt(W2_HCI_CMD_QUEUE_R, (unsigned int)(unsigned long)g_cmd_fifo->r, USB_EP2);
		auc_write_word_by_ep_for_bt(W2_HCI_CMD_QUEUE_W, (unsigned int)(unsigned long)g_cmd_fifo->w, USB_EP2);
	}
}

static void amlbt_usb_hci_evt_fifo_init(void)
{
	debug("evt fifo init\n");
	if (g_event_fifo == 0) {
		g_event_fifo = gdsl_fifo_init(W2_HCI_EVT_QUEUE_SIZE, (unsigned char *)(W2_HCI_EVT_QUEUE_ADDR));
		auc_write_word_by_ep_for_bt(W2_HCI_EVT_QUEUE_R, (unsigned int)(unsigned long)g_event_fifo->r, USB_EP2);
		//printf("event fifo init r: %#lx\n", (unsigned long)g_event_fifo->r);
		auc_write_word_by_ep_for_bt(W2_HCI_EVT_QUEUE_W, (unsigned int)(unsigned long)g_event_fifo->w, USB_EP2);
		//printf("event fifo init w : %#lx\n", (unsigned long)g_event_fifo->w);
	}
}

static void amlbt_usb_hci_type_fifo_init(void)
{
	debug("hci type fifo init\n");
	if (g_hci_type_fifo == 0) {
		g_hci_type_fifo = gdsl_fifo_init(W2_HCI_TYPE_QUEUE_SIZE, (unsigned char *)W2_HCI_TYPE_QUEUE_ADDR);
		auc_write_word_by_ep_for_bt(W2_HCI_TYPE_QUEUE_R, (unsigned int)(unsigned long)g_hci_type_fifo->r, USB_EP2);
		//printf("hci type fifo init r: %#lx\n", (unsigned long)g_hci_type_fifo->r);
		auc_write_word_by_ep_for_bt(W2_HCI_TYPE_QUEUE_W, (unsigned int)(unsigned long)g_hci_type_fifo->w, USB_EP2);
		//printf("hci type fifo init w: %#lx\n", (unsigned long)g_hci_type_fifo->w);
	}
}

static void amlbt_usb_send_hci_cmd(unsigned char *data, unsigned int len)
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
	g_cmd_fifo->r = (unsigned char *)(unsigned long)auc_read_word_by_ep_for_bt(W2_HCI_CMD_QUEUE_R, USB_EP2);
	//printf("cmd r %#lx\n", (unsigned long)g_cmd_fifo->r);

	//step 2: Write HCI commands to WiFi SRAM
	gdsl_write_data_by_ep(g_cmd_fifo, data, len, USB_EP2);

	//step 3: Update the write pointer and write to WiFi SRAM
	//printf("before write:r:%#lx, w:%#lx\n", (unsigned long)g_cmd_fifo->r, (unsigned long)g_cmd_fifo->w);
	auc_write_word_by_ep_for_bt(W2_HCI_CMD_QUEUE_W, (unsigned long)g_cmd_fifo->w & 0xfff, USB_EP2);

	debug("hci cmd finished, len %#x:w %#lx, r %#lx\n", len, (unsigned long)g_cmd_fifo->w, (unsigned long)g_cmd_fifo->r);
}

static void amlbt_usb_recv_event(unsigned char *evt, unsigned int *len, unsigned int *cnt)
{
	//unsigned long w_point = 0;
	gdsl_fifo_t read_fifo = {0};
	unsigned char type_buff[W2_HCI_TYPE_QUEUE_SIZE] = {0};
	unsigned int type_size = 0;
	unsigned int evt_size = 0;
	unsigned int i = 0;
	unsigned char read_reg[16] = {0};
	unsigned int reg = 0;

	debug("hci evt recv\n");
	if (evt == NULL || len == NULL || cnt == NULL) {
		printf("param NULL!!!!\n");
		return ;
	}
	auc_read_sram_by_ep_for_bt(usb_buf, (unsigned char *)(unsigned long)W2_HCI_TYPE_QUEUE_R, USB_POLL_TOTAL_LEN, USB_EP2);

	g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|(usb_buf[33]<<8)|usb_buf[32]);
	g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|(usb_buf[37]<<8)|usb_buf[36]);

	while ((g_event_fifo->w == g_event_fifo->r) || (g_hci_type_fifo->w == g_hci_type_fifo->r)) {
		udelay(50000);
		auc_read_sram_by_ep_for_bt(usb_buf, (unsigned char *)(unsigned long)W2_HCI_TYPE_QUEUE_R, USB_POLL_TOTAL_LEN, USB_EP2);
		g_hci_type_fifo->w = (unsigned char *)(unsigned long)((usb_buf[35]<<24)|(usb_buf[34]<<16)|(usb_buf[33]<<8)|usb_buf[32]);
		g_event_fifo->w = (unsigned char *)(unsigned long)((usb_buf[39]<<24)|(usb_buf[38]<<16)|(usb_buf[37]<<8)|usb_buf[36]);
		debug("recv retry: w:%#lx, r:%#lx\n", (unsigned long)g_hci_type_fifo->w, (unsigned long)g_hci_type_fifo->r);
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
		printf("type queue fatal err!!!!\n");
		*len = 0;
		return ;
	}
	debug("hci type size %d\n", type_size);

	//check hci type
	while (i < type_size) {
		if (type_buff[i] != 0x4) {
			printf("hci type err, i:%d, type:%#x!!!!\n", i, type_buff[i]);
			*len = 0;
			return ;
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
	read_fifo.size = W2_HCI_EVT_QUEUE_SIZE;
	evt_size = gdsl_fifo_get_data(&read_fifo, evt, W2_HCI_EVT_QUEUE_SIZE);
	if (!evt_size) {
		printf("event queue fatal err!!!!\n");
		*len = 0;
		*cnt = 0;
		return ;
	}
	debug("event cnt %d, size %d\n", *cnt, evt_size);
	g_event_fifo->r = read_fifo.r;
	reg = (((unsigned int)(unsigned long)read_fifo.r) & 0x1fff);
	read_reg[4] = (reg & 0xff);
	read_reg[5] = ((reg >> 8) & 0xff);
	read_reg[6] = ((reg >> 16) & 0xff);
	read_reg[7] = ((reg >> 24) & 0xff);
	auc_write_sram_by_ep_for_bt(&read_reg[0],
	(unsigned char *)(unsigned long)(W2_HCI_TYPE_QUEUE_R), 16,
	USB_EP2);
	*len = evt_size;
}

void amlbt_firmware_init(void)
{
	unsigned int st_reg = 0;

	auc_write_word_by_ep_for_bt(REG_PMU_POWER_CFG, 0x20000000, USB_EP2);

	st_reg = auc_read_word_by_ep_for_bt(W2_USB_STATUS_REG, USB_EP2);
	st_reg |= WF_SRAM_FD_INIT_FLAG;
	auc_write_word_by_ep_for_bt(W2_USB_STATUS_REG, st_reg, USB_EP2);

	amlbt_usb_hci_cmd_fifo_init();
	amlbt_usb_hci_type_fifo_init();
	amlbt_usb_hci_evt_fifo_init();

	st_reg &= ~(WF_SRAM_FD_INIT_FLAG);
	auc_write_word_by_ep_for_bt(W2_USB_STATUS_REG, st_reg, USB_EP2);

	//start BT CPU
	auc_write_word_by_ep_for_bt(REG_DEV_RESET, (unsigned int)((BIT_CPU | BIT_MAC | BIT_PHY) << DEV_RESET_SW), USB_EP2);
}

static void aml_hci_cmd_process(unsigned char *cmd, unsigned int len)
{
	unsigned int evt_len = 0;
	unsigned int evt_cnt = 0;

	memset(evt_buf, 0, sizeof(evt_buf));

	amlbt_usb_send_hci_cmd(cmd, len);
	while (!evt_cnt || !evt_len) {
		amlbt_usb_recv_event(evt_buf, &evt_len, &evt_cnt);
	}
	debug("aml_wakeup_set recv event cnt:%d, payload:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", evt_cnt,
		evt_buf[0], evt_buf[1], evt_buf[2], evt_buf[3], evt_buf[4], evt_buf[5], evt_buf[6], evt_buf[7]);
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
	printk("hci send cmd:[set bluetooth address]\n");
	aml_hci_cmd_process(cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}
*/
static void aml_hci_reset(void)
{
	unsigned char cmd[] = {0x03, 0x0c, 0x00};

	debug("--------------------------------------------------->\n");
	printk("hci send cmd:[reset]\n");
	aml_hci_cmd_process(cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}

static void aml_hci_read_bt_addr(void)
{
	unsigned char cmd[] = {0x09, 0x10, 0x00};

	debug("--------------------------------------------------->\n");
	printk("hci send cmd:[read bt addr]\n");
	aml_hci_cmd_process(cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");
}

static void aml_hci_set_vsc_wake_write_data(void)
{
	// b12 g20 RCU
	unsigned char cmd[] = {0x22, 0xfc, 0x05, 0x19, 0xff, 0x01, 0x0a, 0x0b};
	debug("--------------------------------------------------->\n");
	printk("hci send cmd:[set vsc wake write data]\n");
	aml_hci_cmd_process(cmd, sizeof(cmd));
	debug("<---------------------------------------------------\n");

	//google RCU
	unsigned char cmd2[] = {0x22, 0xfc, 0x05, 0x13, 0x16, 0x01, 0x04, 0x06};
	debug("--------------------------------------------------->\n");
	printk("hci send cmd2:[set vsc wake write data]2\n");
	aml_hci_cmd_process(cmd2, sizeof(cmd2));
}

static int amlbt_add_rc_list(char *mac)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_MAC_LIST; i++)
	{
		if (!rc_list[i].used)
		{
			printk("amlbt_add_rc_list %d\n", i);
			rc_list[i].used = 1;
			memcpy(rc_list[i].mac, mac, MAC_ADDR_LEN);
			printk("mac:[%#x,%#x,%#x,%#x,%#x,%#x]\n", rc_list[i].mac[0], rc_list[i].mac[1], rc_list[i].mac[2],
			rc_list[i].mac[3],rc_list[i].mac[4],rc_list[i].mac[5]);
			return 1;
		}
	}
	printk("%s list full!!\n", __func__);
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

	printk("rc_list write count:%d\n", count);
	if (count > MAX_USER_BUF_LEN)
	{
		printk("rc_list too long!:%d, %d\n", count, MAX_USER_BUF_LEN);
		return -ENOMEM;
	}

	memset(rc_list, 0, sizeof(rc_list));
	mac_str = (char *)buf;
	while ((mac_token = strsep(&mac_str, ";")) != NULL)
	{
		printk("mac_token length: %d\n", (int)strlen(mac_token));

		if (strlen(mac_token) == 0)
		{
			printk("NULL str\n");
			continue;
		}

		if (strlen(mac_token) != MAC_ADDR_LEN * 2 + 5)
		{
			printk("Invalid MAC address length: %s\n", mac_token);
			break;
		}

		for (i = MAC_ADDR_LEN - 1, j = 0; i >= 0; i--, j += 3)
		{
			mac_byte_str = &mac_token[j];
			if (mac_token[j + 2] != ':' && j != 15)
			{
				printk("Invalid MAC address format: %s i=%d j=%d\n", mac_token, i, j);
				return -EINVAL;
			}
			mac[i] = simple_strtoul(mac_byte_str, NULL, 16);
		}
		printk("parsed mac: [%#x,%#x,%#x,%#x,%#x,%#x]\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		amlbt_add_rc_list(mac);
	}

	return count;
}


static void aml_hci_set_remote_address(void)
{
	char *temp_env = NULL;
	int i,j;
	temp_env = getenv("remotebtmac");
	printk("hci remotebtmac env:%s \n",temp_env);

	if (temp_env  == NULL) {
		return ;
	}
	rc_list_analyze(temp_env);
	unsigned char cmd[9] = {0x55, 0xfc, 0x06};
	for (i = 0; i < MAX_MAC_LIST; i++) {
		if (rc_list[i].used) {
			for (j = 0; j < MAC_ADDR_LEN; j++) {
				cmd[j+3] = rc_list[i].mac[j];
			}
			printk("rc_list mac: [%#x,%#x,%#x,%#x,%#x,%#x]\n",rc_list[i].mac[0], rc_list[i].mac[1], rc_list[i].mac[2], rc_list[i].mac[3], rc_list[i].mac[4], rc_list[i].mac[5]);
			debug("--------------------------------------------------->\n");
			printk("hci send cmd:[set bluetooth address]\n");
			aml_hci_cmd_process(cmd, sizeof(cmd));
			debug("<---------------------------------------------------\n");
		}
	}
}

static void aml_hci_set_suspend_reg(void)
{
	unsigned int reg_value = 0;
		reg_value = auc_read_word_by_ep_for_bt(RG_AON_A52, USB_EP2);
		//printf("%s RG_AON_A52:%#x\n", __func__, reg_value);
		reg_value |= (1 << 26);
		auc_write_word_by_ep_for_bt(RG_AON_A52, reg_value, USB_EP2);
		debug("suspend bit26:%#x\n", auc_read_word_by_ep_for_bt(RG_AON_A52, USB_EP2));

		reg_value = auc_read_word_by_ep_for_bt(RG_AON_A52, USB_EP2);
		//printf("%s RG_AON_A52:%#x\n", __func__, reg_value);
		reg_value &= ~(1 << 25);
		auc_write_word_by_ep_for_bt(RG_AON_A52, reg_value, USB_EP2);
		debug("lowpower bit25:%#x\n", auc_read_word_by_ep_for_bt(RG_AON_A52, USB_EP2));
}

void aml_wakeup_set(void)
{
	aml_hci_reset();
	aml_hci_read_bt_addr();
	//aml_hci_set_bluetooth_address();
	aml_hci_set_vsc_wake_write_data();
	aml_hci_set_remote_address();
	aml_hci_set_suspend_reg();
	printk("aml_wakeup_set end\n");
}
