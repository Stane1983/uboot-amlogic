// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "wifi.h"
#include "./w2l.h"
#include "stdio.h"
#include "fs.h"
#include <command.h>
#include "mapmem.h"
#include <malloc.h>
#include <string.h>
#include <usb.h>
#include <usb/xhci.h>
#include <ctype.h>

#include <asm/processor.h>
#include <asm-generic/gpio.h>
#include <linux/compiler.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <dm/uclass-internal.h>
#include <dm.h>


#define AML_SIG_CBW             0x43425355
#define AML_XFER_TO_DEVICE      0
#define AML_XFER_TO_HOST        0x80
#define USB_MAX_TRANS_SIZE (64 * 1024)
#define AML_USB_CONTROL_MSG_TIMEOUT 3000
#define CMD_WRITE_REG 0xc5
#define AML_ADDR_AON 1
#define CHIP_ANA_REG_BASE                         (0xf05000)

#define RG_DPLL_A0                                (CHIP_ANA_REG_BASE + 0x0)
// Bit 8   :0      rg_bbpll_div_n                 U     RW        default = 'h60
// Bit 31  :16     rg_bbpll_reve                  U     RW        default = 'h0
typedef union RG_DPLL_A0_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_div_n : 9;
    unsigned int rsvd_0 : 7;
    unsigned int rg_bbpll_reve : 16;
  } b;
} RG_DPLL_A0_FIELD_T;

#define RG_DPLL_A1                                (CHIP_ANA_REG_BASE + 0x4)
// Bit 0           rg_bbpll_en                    U     RW        default = 'h0
// Bit 1           rg_bbpll_rst                   U     RW        default = 'h1
// Bit 8           rg_bbpll_fr_en                 U     RW        default = 'h0
// Bit 10  :9      rg_bbpll_fr_adj                U     RW        default = 'h2
// Bit 11          rg_bbpll_cp_en                 U     RW        default = 'h1
typedef union RG_DPLL_A1_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_en : 1;
    unsigned int rg_bbpll_rst : 1;
    unsigned int rsvd_0 : 6;
    unsigned int rg_bbpll_fr_en : 1;
    unsigned int rg_bbpll_fr_adj : 2;
    unsigned int rg_bbpll_cp_en : 1;
    unsigned int rsvd_1 : 20;
  } b;
} RG_DPLL_A1_FIELD_T;

#define RG_DPLL_A2                                (CHIP_ANA_REG_BASE + 0x8)
// Bit 5   :0      rg_bbpll_ibn_adj               U     RW        default = 'h10
// Bit 13  :8      rg_bbpll_ibp_adj               U     RW        default = 'h10
// Bit 19  :16     rg_bbpll_r2_cnt                U     RW        default = 'h2
// Bit 24          rg_bbpll_vref_adj              U     RW        default = 'h0
// Bit 26  :25     rg_bbpll_dt_sel                U     RW        default = 'h0
typedef union RG_DPLL_A2_FIELD
{
	unsigned int data;
	struct
	{
		unsigned int rg_bbpll_ibn_adj : 6;
		unsigned int rsvd_0 : 2;
		unsigned int rg_bbpll_ibp_adj : 6;
		unsigned int rsvd_1 : 2;
		unsigned int rg_bbpll_r2_cnt : 4;
		unsigned int rsvd_2 : 4;
		unsigned int rg_bbpll_vref_adj : 1;
		unsigned int rg_bbpll_dt_sel : 2;
		unsigned int rsvd_3 : 5;
	} b;
} RG_DPLL_A2_FIELD_T;

#define RG_DPLL_A3                                (CHIP_ANA_REG_BASE + 0xc)
// Bit 1   :0      rg_bbpll_lk_w_sel              U     RW        default = 'h0
// Bit 2           rg_bbpll_lk_clk_gate           U     RW        default = 'h0
// Bit 3           rg_bbpll_lk_lock_long          U     RW        default = 'h0
// Bit 4           rg_bbpll_lk_lock_f             U     RW        default = 'h0
// Bit 5           rg_bbpll_lk_rst                U     RW        default = 'h1
// Bit 26  :8      rg_bbpll_sdm_fra               U     RW        default = 'h0
typedef union RG_DPLL_A3_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_lk_w_sel : 2;
    unsigned int rg_bbpll_lk_clk_gate : 1;
    unsigned int rg_bbpll_lk_lock_long : 1;
    unsigned int rg_bbpll_lk_lock_f : 1;
    unsigned int rg_bbpll_lk_rst : 1;
    unsigned int rsvd_0 : 2;
    unsigned int rg_bbpll_sdm_fra : 19;
    unsigned int rsvd_1 : 5;
  } b;
} RG_DPLL_A3_FIELD_T;

#define RG_DPLL_A4                                (CHIP_ANA_REG_BASE + 0x10)
// Bit 0           rg_bbpll_wadc_clk_en           U     RW        default = 'h1
// Bit 1           rg_bbpll_wdac_clk_en           U     RW        default = 'h1
// Bit 2           rg_bbpll_wdac_clk_sel          U     RW        default = 'h0
// Bit 3           rg_bbpll_btadc_clk_en          U     RW        default = 'h1
// Bit 4           rg_bbpll_test_en               U     RW        default = 'h0
typedef union RG_DPLL_A4_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_wadc_clk_en : 1;
    unsigned int rg_bbpll_wdac_clk_en : 1;
    unsigned int rg_bbpll_wdac_clk_sel : 1;
    unsigned int rg_bbpll_btadc_clk_en : 1;
    unsigned int rg_bbpll_test_en : 1;
    unsigned int rsvd_0 : 27;
  } b;
} RG_DPLL_A4_FIELD_T;

#define RG_DPLL_A5                                (CHIP_ANA_REG_BASE + 0x14)
// Bit 0           rg_bbpll_ssc_en                U     RW        default = 'h0
// Bit 3   :1      rg_bbpll_ssc_fref              U     RW        default = 'h0
// Bit 5   :4      rg_bbpll_ssc_os                U     RW        default = 'h0
// Bit 6           rg_bbpll_ssc_load_en           U     RW        default = 'h1
// Bit 7           rg_bbpll_ssc_load              U     RW        default = 'h1
// Bit 8           rg_bbpll_ssc_shift_en          U     RW        default = 'h0
// Bit 12  :9      rg_bbpll_ssc_str_m             U     RW        default = 'h0
// Bit 14  :13     rg_bbpll_ssc_mode              U     RW        default = 'h0
// Bit 16  :15     rg_bbpll_ssc_shift_v           U     RW        default = 'h0
// Bit 20  :17     rg_bbpll_ssc_dep               U     RW        default = 'h0
// Bit 21          rg_bbpll_sdm_en                U     RW        default = 'h0
typedef union RG_DPLL_A5_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rg_bbpll_ssc_en : 1;
    unsigned int rg_bbpll_ssc_fref : 3;
    unsigned int rg_bbpll_ssc_os : 2;
    unsigned int rg_bbpll_ssc_load_en : 1;
    unsigned int rg_bbpll_ssc_load : 1;
    unsigned int rg_bbpll_ssc_shift_en : 1;
    unsigned int rg_bbpll_ssc_str_m : 4;
    unsigned int rg_bbpll_ssc_mode : 2;
    unsigned int rg_bbpll_ssc_shift_v : 2;
    unsigned int rg_bbpll_ssc_dep : 4;
    unsigned int rg_bbpll_sdm_en : 1;
    unsigned int rsvd_0 : 10;
  } b;
} RG_DPLL_A5_FIELD_T;

#define RG_DPLL_A6                                (CHIP_ANA_REG_BASE + 0x1c)
// Bit 28          ro_bbpll_fb_clk_done           U     RO        default = 'h0
// Bit 29          ro_bbpll_ref_clk_done          U     RO        default = 'h0
// Bit 30          ro_bbpll_vco_clk_done          U     RO        default = 'h0
// Bit 31          ro_bbpll_done                  U     RO        default = 'h0
typedef union RG_DPLL_A6_FIELD
{
  unsigned int data;
  struct
  {
    unsigned int rsvd_0 : 28;
    unsigned int ro_bbpll_fb_clk_done : 1;
    unsigned int ro_bbpll_ref_clk_done : 1;
    unsigned int ro_bbpll_vco_clk_done : 1;
    unsigned int ro_bbpll_done : 1;
  } b;
} RG_DPLL_A6_FIELD_T;

static const struct  wifi_dongle usb_dongle[] = {
	{0x1b8e, 0x0841, "aml_w2l", wifi_w2l_fw_usb, sizeof(wifi_w2l_fw_usb) * 4},
	{0x1b8e, 0x0801, "aml_w2l", wifi_w2l_fw_usb, sizeof(wifi_w2l_fw_usb) * 4},
};

static int num = -1;
struct usb_device *g_udev = NULL;
struct crg_msc_cbw *g_cmd_buf;
unsigned char *fw_data;

void auc_build_cbw(struct crg_msc_cbw *cbw_buf,
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
	cbw_buf->cdb[2] = (unsigned int)cdb3;
	cbw_buf->cdb[3] = cdb4; //read or write data length
}

int wifi_write_reg(unsigned int addr, unsigned int value, unsigned int len)
{
    int ret = 0;
    int actual_length = 0;
    struct usb_device *udev = g_udev;

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
    /* cmd stage */
    ret = usb_bulk_msg(udev, (unsigned int)usb_sndbulkpipe(udev, USB_EP1),(void *) g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);

    return actual_length; //bt write maybe use the value
}

unsigned int wifi_read_reg(unsigned int addr, unsigned int len)
{
    int ret = 0;
    int actual_length = 0;
    unsigned int reg_data;
    struct usb_device *udev = g_udev;
    unsigned char *data = NULL;

    data = (unsigned char *)malloc(len);

    auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, 0xc4, addr, 0, len);

    /* cmd stage */
    ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),(void *)g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);

    /* data stage */
    ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, USB_EP4), (void *)data, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);

    memcpy(&reg_data, data, actual_length);
    free(data);

    return reg_data;
}

unsigned int bbpll_init(void)
{
	RG_DPLL_A0_FIELD_T rg_dpll_a0;
	RG_DPLL_A1_FIELD_T rg_dpll_a1;
	RG_DPLL_A2_FIELD_T rg_dpll_a2;
	RG_DPLL_A3_FIELD_T rg_dpll_a3;
	RG_DPLL_A4_FIELD_T rg_dpll_a4;
	RG_DPLL_A5_FIELD_T rg_dpll_a5;
	RG_DPLL_A6_FIELD_T rg_dpll_a6;

	rg_dpll_a0.data = 0x00800060;  //close test path
	wifi_write_reg(0xf05000, rg_dpll_a0.data, 4);

	rg_dpll_a1.data = 0x00000c02;
	wifi_write_reg(0xf05004, rg_dpll_a1.data, 4);

	rg_dpll_a2.data = 0x00021f1f;
	wifi_write_reg(0xf05008, rg_dpll_a2.data, 4);

	rg_dpll_a3.data = 0x00000020;
	wifi_write_reg(0xf0500c, rg_dpll_a3.data, 4);

	rg_dpll_a4.data = 0x0000000a;
	wifi_write_reg(0xf05010, rg_dpll_a4.data, 4);

	rg_dpll_a5.data = 0x000000c0;
	wifi_write_reg(0xf05014, rg_dpll_a5.data, 4);

	rg_dpll_a6.data = 0x00000000;
	wifi_write_reg(0xf0501c, rg_dpll_a6.data, 4);

	return 0;
}

unsigned int bbpll_start(void)
{

	//RG_DPLL_A0_FIELD_T rg_dpll_a0;
	RG_DPLL_A1_FIELD_T rg_dpll_a1;
	//RG_DPLL_A2_FIELD_T rg_dpll_a2;
	RG_DPLL_A3_FIELD_T rg_dpll_a3;
	//RG_DPLL_A4_FIELD_T rg_dpll_a4;
	//RG_DPLL_A5_FIELD_T rg_dpll_a5;
	RG_DPLL_A6_FIELD_T rg_dpll_a6;

	//1.enable PLL and set PLL configuration
	rg_dpll_a1.data = wifi_read_reg(0xf05004, 4);

	rg_dpll_a1.b.rg_bbpll_en = 0x1;
	wifi_write_reg(0xf05004, rg_dpll_a1.data, 4);

	//delay 20us for LDO and Band-gap to establish the working state
	udelay(20);

	//2.disable PLL reset
	rg_dpll_a1.b.rg_bbpll_rst = 0x0;
	wifi_write_reg(0xf05004, rg_dpll_a1.data, 4);
	//delay 20 us for lock detector
	udelay(20);

	//3.enable PLL lock-detector
	//rg_dpll_a3.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A3);
	rg_dpll_a3.data = wifi_read_reg(0xf0500c, 4);
	rg_dpll_a3.b.rg_bbpll_lk_rst = 0;
	//AML_REG_WRITE(rg_dpll_a3.data, aml_plat, AML_ADDR_AON, RG_DPLL_A3);
	wifi_write_reg(0xf0500c, rg_dpll_a3.data, 4);

	udelay(20);

	//4.check PLL status
	//rg_dpll_a6.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A6);
	rg_dpll_a6.data = wifi_read_reg(0xf0501c, 4);

	if (rg_dpll_a6.b.ro_bbpll_done == 1)
	{
		printf( "bbpll done !\n");
		return 1;
	}
	else
	{
		printf( "bbpll start failed !\n");
		return 0;
	}
}

static int wifi_fw_download(void)
{
#if 0
	int i;

	unsigned int * src = &wifi_w2l_fw_usb[0];

	int reg = 0x00041784;

	for (i = 0; i < sizeof(wifi_w2l_fw_usb) / sizeof(wifi_w2l_fw_usb[0]); i++)
	{
		wifi_write_reg(reg, *src, 4);
		src += 1;
		reg += 4;
	}

	return 0;
#endif
	int ret = 0;
	int actual_length = 0;
	struct usb_device *udev = g_udev;
	int len = usb_dongle[num].fw_len;
	unsigned int * src = usb_dongle[num].fw_data;

	auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_DOWNLOAD_WIFI, 0x00041784, 0, len);

	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf,
			   sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("Failed to usb_bulk_msg, ret %d\n", ret);
		return 1;
	}
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1),
			(void *)src, len, &actual_length,
			AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("Failed to usb_bulk_msg, ret %d\n", ret);
		return 1;
	}
	return 0;
}

int start_wifi(void)
{
	int ret = 0;
	int actual_length = 0;
	struct usb_device *udev = g_udev;
	auc_build_cbw(g_cmd_buf, 0, 0, 0xc2, 0, 0, 0);
	/* cmd stage */
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *) g_cmd_buf, sizeof(*g_cmd_buf), &actual_length, 3000);
	return ret;
}

static struct usb_device *usb_find_device(int devnum)
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

static struct usb_device *get_aml_dev(void)
{
	int i = 0;
	int size, usb_size;
	struct usb_device *dev = NULL;
	/* scan all USB Devices */

	printf("USB_MAX_DEVICE:%d\n", USB_MAX_DEVICE);
	for (i = 0; i < USB_MAX_DEVICE; i++) {
		dev = usb_find_device(i);
		if (!dev) {
			printf("usb dev %d is null!!!!\n", i);
			return NULL;
		}
		printf("i=%d dev->devnum:%d dev->mf:%s dev->prod:%s\n",
		       i, dev->devnum, dev->mf, dev->prod);
		printf("vid is %x, pid is %x\n",
		       le16_to_cpu(dev->descriptor.idVendor),
		       le16_to_cpu(dev->descriptor.idProduct));
		/* AML change for haier to match 8723bu bt */

		if (dev->devnum != -1) {
			size = sizeof(usb_dongle) / sizeof(struct wifi_dongle);
			for (usb_size = 0; usb_size < size; usb_size++) {
				if (le16_to_cpu(dev->descriptor.idVendor ==
						usb_dongle[usb_size].idVendor) &&
						le16_to_cpu(dev->descriptor.idProduct) ==
						usb_dongle[usb_size].idProduct) {
					num = usb_size;
					return dev;
				}
			}
		}
	}
	printf("dou't find device\n");
	return NULL;
}

void wifi_init(void)
{
	struct gpio_desc desc;
	int ret;

	printf("wifi init\n");

	ret = dm_gpio_lookup_name(WIFI_POWER_PIN, &desc);
	if (ret) {
		printf("%s: not found\n", WIFI_POWER_PIN);
		return ;
	}

	ret = dm_gpio_request(&desc, "wifi_power");
	if (ret) {
		printf("gpio requesting failed\n");
		return ;
	}
	ret = dm_gpio_set_dir_flags(&desc, GPIOD_IS_OUT);
	if (ret) {
		printf("set direction failed\n");
		return ;
	}

	dm_gpio_set_value(&desc, 0); //output low
	udelay(200000);
	dm_gpio_set_value(&desc, 1); //output high

	usb_init();

	g_udev = get_aml_dev();
	if (!g_udev) {
		printf("cat not found aml usb wifi\n");
		return;
	}

	g_cmd_buf = malloc(sizeof(*g_cmd_buf));
	if (!g_cmd_buf)
		return;

	/*
	1、bbpll init & start
	2、cpu clock set
	3、download fw
	4、write value 0x00141832 to 0x40000 reg addr ---fw enter point
	5、start wifi
	*/

	bbpll_init();
	bbpll_start();

	// set cpu clock
	wifi_write_reg(0x00a0d090, 0x4f210033, 4);
	// printf("read reg 0x00a0d090: %x\n",wifi_read_reg(0x00a0d090, 4));

	if (!wifi_fw_download()) {
		wifi_write_reg(0x40000, 0x0014183c, 4);
		//printf("read reg 0x40000:%x\n", wifi_read_reg(0x40000, 4));

		if (!start_wifi()) {
			// set usb mode env to kernel when download fw success
			env_set_hex("usb2t_mode", 1);
			//run_command("print usb2t_mode", 0);
		}
	}
}
