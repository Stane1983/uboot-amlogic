// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <bootstage.h>
#include <command.h>
#include <env.h>
#include <malloc.h>
#include <usb.h>

#include <asm/processor.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <asm-generic/gpio.h>
#include <asm/amlogic/arch/mailbox.h>

#include <linux/compiler.h>
#include <linux/compat.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#include <dm/device.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

#include <amlogic/wireless/wifi_bt_comm.h>

#define CMD_SET_MID     0xFC

static const char * const chip_list[] = {"NULL", "w1", "w1u", "w2", "w2l"};
static const char * const rev_list[] = {"RevA", "RevB", "RevC", "RevD", "RevE", "RevF"};
static const char * const itf_list[] = {"sdio", "usb", "pcie"};

static struct usb_device *aml_dev;
static aml_chip_type aml_type = {0};

void auc_build_cbw(struct crg_msc_cbw *cbw_buf, unsigned char dir, unsigned int len,
	unsigned char cdb1, unsigned int cdb2, unsigned long cdb3, unsigned long cdb4)
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

static void aml_type_info(aml_chip_type *type)
{
	size_t chip_cnt = sizeof(chip_list) / sizeof(chip_list[0]);
	size_t rev_cnt  = sizeof(rev_list)  / sizeof(rev_list[0]);
	size_t itf_cnt  = sizeof(itf_list)  / sizeof(itf_list[0]);

	size_t chip_id = type->family_id;
	size_t rev_id  = type->family_rev;
	size_t itf_id  = type->interface;

	if ((chip_id < chip_cnt) && (rev_id < rev_cnt) && (itf_id < itf_cnt)) {
		printf("aml mod chip:%s\n", chip_list[chip_id]);
		printf("interface:%s rev:%s\n", itf_list[itf_id], rev_list[rev_id]);
	} else {
		printf("out of range, family_id:%u family_rev:%u interface:%u\n",
			type->family_id, type->family_rev, type->interface);
	}
}

static void aml_mod_power_on(void)
{
	struct gpio_desc desc;
	int ret;

	printf("%s:start:%u\n", __func__, get_time());

	ret = dm_gpio_lookup_name(WIFI_BT_EN, &desc);
	if (ret) {
		printf("%s: not found\n", WIFI_BT_EN);
		return;
	}

	ret = dm_gpio_request(&desc, "wifi_bt_en");
	if (ret) {
		printf("wifi_bt_en gpio request fail\n");
		return;
	}

	ret = dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	if (ret) {
		printf("wifi_bt_en set dir fail\n");
		return;
	}

	dm_gpio_set_value(&desc, 0);  // set output low
	udelay(50000);
	dm_gpio_set_value(&desc, 1);  // set output high
	udelay(10000);

	printf("%s:wifi_bt_en:%s\n", __func__, WIFI_BT_EN);
	printf("%s:end:%u\n", __func__, get_time());
}

static int aml_mod_usb_start(void)
{
	bootstage_mark_name(BOOTSTAGE_ID_USB_START, "usb_wifi_start");

	int ret = usb_wifi_init();

	return ret;
}

static struct usb_device *aml_mod_find_usb_dev(int devnum)
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

static int aml_mod_get_dev(aml_chip_type *chip_type, struct usb_device **udev)
{
	struct usb_device *udev_tmp = NULL;
	int num;

	for (num = 0; num < USB_MAX_DEVICE; num++) {
		udev_tmp = aml_mod_find_usb_dev(num);
		if (!udev_tmp)
			break;

		if (udev_tmp->descriptor.bDescriptorType == USB_DT_DEVICE) {
			if (udev_tmp->descriptor.idVendor == AML_W2_VENDOR) {
				chip_type->interface = udev_tmp->descriptor.idProduct & 0x07;
				chip_type->wireless = 0;
				chip_type->family_rev = (udev_tmp->descriptor.idProduct >> 6) & 0x03;
				chip_type->family_id = (udev_tmp->descriptor.idProduct >> 9) & 0x1f;
				chip_type->reserved = 0;
				aml_type_info(chip_type);
				*udev = udev_tmp;
				return 0;
			}
		}
	}

	return -1;
}

static void aml_mod_mbox_name(aml_chip_type type)
{
	u32 ret;
	char name[32] = {0};

	switch (type.family_id) {
	case 2:   // W1U
		snprintf(name, sizeof(name), "aml_w1u");
		break;
	case 3:   // W2
		snprintf(name, sizeof(name), "aml_w2_u");
		break;
	case 4:   // W2L
		snprintf(name, sizeof(name), "aml_w2l_u");
		break;
	default:
		snprintf(name, sizeof(name), "aml");
		break;
	}

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SET_MID, name, strlen(name), NULL, 0);
	if (ret != 0)
		printf("%s:%s fail\n", __func__, name);
	else
		printf("%s:%s success\n", __func__, name);
}

struct usb_device *aml_mod_get_udev(void)
{
	return aml_dev;
}

aml_chip_type *aml_mod_get_type(void)
{
	return &aml_type;
}

void aml_mod_init(void)
{
	aml_mod_power_on();

	if (aml_mod_usb_start() != 0) {
		printf("aml_mod_usb_start fail\n");
		return;
	}

	if (aml_mod_get_dev(&aml_type, &aml_dev) != 0) {
		printf("aml_mod_get_dev fail\n");
		return;
	}

	if (!aml_dev) {
		printf("aml_dev is NULL\n");
		return;
	}

	aml_mod_mbox_name(aml_type);
}

