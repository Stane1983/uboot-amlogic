// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include <command.h>
#include "fs.h"
#include "mapmem.h"

#include <usb.h>
#include <usb/xhci.h>

#include <dm.h>
#include <dm/device.h>
#include <dm/uclass.h>
#include <dm/uclass-internal.h>

#include <asm/processor.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <asm-generic/gpio.h>

#include <linux/compiler.h>
#include <linux/ctype.h>

#include <amlogic/wireless/wifi_bt_comm.h>
#include <amlogic/wireless/wifi/aml_wifi.h>
#include <amlogic/wireless/wifi/w2l.h>

#define AML_XFER_TO_DEVICE      0
#define AML_XFER_TO_HOST        0x80
#define USB_MAX_TRANS_SIZE (64 * 1024)
#define AML_USB_CONTROL_MSG_TIMEOUT 3000
#define AML_ADDR_AON 1
#define CHIP_ANA_REG_BASE                         (0xf05000)

#define RG_DPLL_A0                                (CHIP_ANA_REG_BASE + 0x0)
// Bit 8   :0      rg_bbpll_div_n                 U     RW        default = 'h60
// Bit 31  :16     rg_bbpll_reve                  U     RW        default = 'h0
typedef union {
	unsigned int data;
	struct {
		unsigned int rg_bbpll_div_n : 9;
		unsigned int rsvd_0 : 7;
		unsigned int rg_bbpll_reve : 16;
	} b;
} RG_DPLL_A0_FIELD;

#define RG_DPLL_A1                                (CHIP_ANA_REG_BASE + 0x4)
// Bit 0           rg_bbpll_en                    U     RW        default = 'h0
// Bit 1           rg_bbpll_rst                   U     RW        default = 'h1
// Bit 8           rg_bbpll_fr_en                 U     RW        default = 'h0
// Bit 10  :9      rg_bbpll_fr_adj                U     RW        default = 'h2
// Bit 11          rg_bbpll_cp_en                 U     RW        default = 'h1
typedef union {
	unsigned int data;
	struct {
		unsigned int rg_bbpll_en : 1;
		unsigned int rg_bbpll_rst : 1;
		unsigned int rsvd_0 : 6;
		unsigned int rg_bbpll_fr_en : 1;
		unsigned int rg_bbpll_fr_adj : 2;
		unsigned int rg_bbpll_cp_en : 1;
		unsigned int rsvd_1 : 20;
	} b;
} RG_DPLL_A1_FIELD;

#define RG_DPLL_A2                                (CHIP_ANA_REG_BASE + 0x8)
// Bit 5   :0      rg_bbpll_ibn_adj               U     RW        default = 'h10
// Bit 13  :8      rg_bbpll_ibp_adj               U     RW        default = 'h10
// Bit 19  :16     rg_bbpll_r2_cnt                U     RW        default = 'h2
// Bit 24          rg_bbpll_vref_adj              U     RW        default = 'h0
// Bit 26  :25     rg_bbpll_dt_sel                U     RW        default = 'h0
typedef union {
	unsigned int data;
	struct {
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
} RG_DPLL_A2_FIELD;

#define RG_DPLL_A3                                (CHIP_ANA_REG_BASE + 0xc)
// Bit 1   :0      rg_bbpll_lk_w_sel              U     RW        default = 'h0
// Bit 2           rg_bbpll_lk_clk_gate           U     RW        default = 'h0
// Bit 3           rg_bbpll_lk_lock_long          U     RW        default = 'h0
// Bit 4           rg_bbpll_lk_lock_f             U     RW        default = 'h0
// Bit 5           rg_bbpll_lk_rst                U     RW        default = 'h1
// Bit 26  :8      rg_bbpll_sdm_fra               U     RW        default = 'h0
typedef union {
	unsigned int data;
	struct {
		unsigned int rg_bbpll_lk_w_sel : 2;
		unsigned int rg_bbpll_lk_clk_gate : 1;
		unsigned int rg_bbpll_lk_lock_long : 1;
		unsigned int rg_bbpll_lk_lock_f : 1;
		unsigned int rg_bbpll_lk_rst : 1;
		unsigned int rsvd_0 : 2;
		unsigned int rg_bbpll_sdm_fra : 19;
		unsigned int rsvd_1 : 5;
	} b;
} RG_DPLL_A3_FIELD;

#define RG_DPLL_A4                                (CHIP_ANA_REG_BASE + 0x10)
// Bit 0           rg_bbpll_wadc_clk_en           U     RW        default = 'h1
// Bit 1           rg_bbpll_wdac_clk_en           U     RW        default = 'h1
// Bit 2           rg_bbpll_wdac_clk_sel          U     RW        default = 'h0
// Bit 3           rg_bbpll_btadc_clk_en          U     RW        default = 'h1
// Bit 4           rg_bbpll_test_en               U     RW        default = 'h0
typedef union {
	unsigned int data;
	struct {
		unsigned int rg_bbpll_wadc_clk_en : 1;
		unsigned int rg_bbpll_wdac_clk_en : 1;
		unsigned int rg_bbpll_wdac_clk_sel : 1;
		unsigned int rg_bbpll_btadc_clk_en : 1;
		unsigned int rg_bbpll_test_en : 1;
		unsigned int rsvd_0 : 27;
	} b;
} RG_DPLL_A4_FIELD;

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
typedef union {
	unsigned int data;
	struct {
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
} RG_DPLL_A5_FIELD;

#define RG_DPLL_A6                                (CHIP_ANA_REG_BASE + 0x1c)
// Bit 28          ro_bbpll_fb_clk_done           U     RO        default = 'h0
// Bit 29          ro_bbpll_ref_clk_done          U     RO        default = 'h0
// Bit 30          ro_bbpll_vco_clk_done          U     RO        default = 'h0
// Bit 31          ro_bbpll_done                  U     RO        default = 'h0
typedef union {
	unsigned int data;
	struct {
	unsigned int rsvd_0 : 28;
	unsigned int ro_bbpll_fb_clk_done : 1;
	unsigned int ro_bbpll_ref_clk_done : 1;
	unsigned int ro_bbpll_vco_clk_done : 1;
	unsigned int ro_bbpll_done : 1;
	} b;
} RG_DPLL_A6_FIELD;

static const wifi_dongle usb_dongle[] = {
	{ AML_W2L, "aml_w2l", wifi_w2l_fw_usb, sizeof(wifi_w2l_fw_usb) * 4},
	{ AML_UNKNOWN, NULL, NULL, 0 },
};

struct crg_msc_cbw *g_cmd_buf;
unsigned char *fw_data;

static int wifi_write_reg(struct usb_device *udev, unsigned int addr,
		unsigned int value, unsigned int len)
{
	int ret = 0;
	int actual_length = 0;

	auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
	/* cmd stage */
	ret = usb_bulk_msg(udev, (unsigned int)usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf,
		sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret != 0)
		printf("%s:usb_bulk_msg fail, ret:%d\n", __func__, ret);

	return actual_length; //bt write maybe use the value
}

static unsigned int wifi_read_reg(struct usb_device *udev, unsigned int addr, unsigned int len)
{
	int ret = 0;
	int actual_length = 0;
	unsigned int reg_data;
	unsigned char *data = NULL;

	data = (unsigned char *)malloc(len);

	auc_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, 0xc4, addr, 0, len);

	/* cmd stage */
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf,
		sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret != 0)
		printf("%s:usb_bulk_msg fail1, ret:%d\n", __func__, ret);

	/* data stage */
	ret = usb_bulk_msg(udev, usb_rcvbulkpipe(udev, USB_EP4), (void *)data, len,
		&actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret != 0)
		printf("%s:usb_bulk_msg fail2, ret:%d\n", __func__, ret);

	memcpy(&reg_data, data, actual_length);
	free(data);

	return reg_data;
}

static unsigned int bbpll_init(struct usb_device *udev)
{
	RG_DPLL_A0_FIELD rg_dpll_a0;
	RG_DPLL_A1_FIELD rg_dpll_a1;
	RG_DPLL_A2_FIELD rg_dpll_a2;
	RG_DPLL_A3_FIELD rg_dpll_a3;
	RG_DPLL_A4_FIELD rg_dpll_a4;
	RG_DPLL_A5_FIELD rg_dpll_a5;
	RG_DPLL_A6_FIELD rg_dpll_a6;

	rg_dpll_a0.data = 0x00800060;  //close test path
	wifi_write_reg(udev, 0xf05000, rg_dpll_a0.data, 4);

	rg_dpll_a1.data = 0x00000c02;
	wifi_write_reg(udev, 0xf05004, rg_dpll_a1.data, 4);

	rg_dpll_a2.data = 0x00021f1f;
	wifi_write_reg(udev, 0xf05008, rg_dpll_a2.data, 4);

	rg_dpll_a3.data = 0x00000020;
	wifi_write_reg(udev, 0xf0500c, rg_dpll_a3.data, 4);

	rg_dpll_a4.data = 0x0000000a;
	wifi_write_reg(udev, 0xf05010, rg_dpll_a4.data, 4);

	rg_dpll_a5.data = 0x000000c0;
	wifi_write_reg(udev, 0xf05014, rg_dpll_a5.data, 4);

	rg_dpll_a6.data = 0x00000000;
	wifi_write_reg(udev, 0xf0501c, rg_dpll_a6.data, 4);

	return 0;
}

static unsigned int bbpll_start(struct usb_device *udev)
{
	//RG_DPLL_A0_FIELD rg_dpll_a0;
	RG_DPLL_A1_FIELD rg_dpll_a1;
	//RG_DPLL_A2_FIELD rg_dpll_a2;
	RG_DPLL_A3_FIELD rg_dpll_a3;
	//RG_DPLL_A4_FIELD rg_dpll_a4;
	//RG_DPLL_A5_FIELD rg_dpll_a5;
	RG_DPLL_A6_FIELD rg_dpll_a6;

	//1.enable PLL and set PLL configuration
	rg_dpll_a1.data = wifi_read_reg(udev, 0xf05004, 4);

	rg_dpll_a1.b.rg_bbpll_en = 0x1;
	wifi_write_reg(udev, 0xf05004, rg_dpll_a1.data, 4);

	//delay 20us for LDO and Band-gap to establish the working state
	udelay(20);

	//2.disable PLL reset
	rg_dpll_a1.b.rg_bbpll_rst = 0x0;
	wifi_write_reg(udev, 0xf05004, rg_dpll_a1.data, 4);
	//delay 20 us for lock detector
	udelay(20);

	//3.enable PLL lock-detector
	//rg_dpll_a3.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A3);
	rg_dpll_a3.data = wifi_read_reg(udev, 0xf0500c, 4);
	rg_dpll_a3.b.rg_bbpll_lk_rst = 0;
	//AML_REG_WRITE(rg_dpll_a3.data, aml_plat, AML_ADDR_AON, RG_DPLL_A3);
	wifi_write_reg(udev, 0xf0500c, rg_dpll_a3.data, 4);

	udelay(20);

	//4.check PLL status
	//rg_dpll_a6.data = AML_REG_READ(aml_plat, AML_ADDR_AON, RG_DPLL_A6);
	rg_dpll_a6.data = wifi_read_reg(udev, 0xf0501c, 4);

	if (rg_dpll_a6.b.ro_bbpll_done == 1) {
		printf("bbpll done !\n");
		return 1;
	} else {
		printf("bbpll start failed !\n");
		return 0;
	}
}

static int wifi_fw_download(const aml_chip_type *chip_type, struct usb_device *udev)
{
	int idx, ret;
	int actual_length = 0;

	for (idx = 0; usb_dongle[idx].usb_module; idx++) {
		if (chip_type->family_id == usb_dongle[idx].family_id) {
			printf("wifi:%d, mod family_id:%u\n", idx, usb_dongle[idx].family_id);
			break;
		}
	}

	auc_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, usb_dongle[idx].fw_len, CMD_DOWNLOAD_WIFI,
		0x00041784, 0, usb_dongle[idx].fw_len);

	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf,
		sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("%s:usb_bulk_msg fail1, ret:%d\n", __func__, ret);
		return 1;
	}

	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)usb_dongle[idx].fw_data,
		usb_dongle[idx].fw_len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
	if (ret) {
		printf("%s:usb_bulk_msg fai2, ret:%d\n", __func__, ret);
		return 1;
	}

	return 0;
}

static int start_wifi(struct usb_device *udev)
{
	int ret = 0;
	int actual_length = 0;

	auc_build_cbw(g_cmd_buf, 0, 0, 0xc2, 0, 0, 0);
	/* cmd stage */
	ret = usb_bulk_msg(udev, usb_sndbulkpipe(udev, USB_EP1), (void *)g_cmd_buf,
		sizeof(*g_cmd_buf), &actual_length, 3000);
	if (ret != 0)
		printf("%s:usb_bulk_msg fail, ret:%d\n", __func__, ret);

	return ret;
}

void aml_wifi_set_usb2t_mode(void)
{
	struct usb_device *aml_wifi = NULL;
	aml_chip_type *type = NULL;

	aml_wifi = aml_mod_get_udev();
	if (!aml_wifi) {
		printf("%s:null aml_wifi, exit\n", __func__);
		return;
	}

	type = aml_mod_get_type();
	if (!type) {
		printf("%s:null type, exit\n", __func__);
		return;
	}

	g_cmd_buf = malloc(sizeof(*g_cmd_buf));
	if (!g_cmd_buf)  {
		printf("g_cmd_buf malloc fail\n");
		return;
	}

	/*
	 * 1、bbpll init & start
	 * 2、cpu clock set
	 * 3、download fw
	 * 4、write value 0x00141788 to 0x40000 reg addr ---fw enter point
	 * 5、start wifi
	 */

	bbpll_init(aml_wifi);
	bbpll_start(aml_wifi);

	// set cpu clock
	wifi_write_reg(aml_wifi, 0x00a0d090, 0x4f210033, 4);
	// printf("read reg 0x00a0d090: %x\n",wifi_read_reg(0x00a0d090, 4));

	if (!wifi_fw_download(type, aml_wifi)) {
		wifi_write_reg(aml_wifi, 0x40000, 0x00141788, 4);
		//printf("read reg 0x40000:%x\n", wifi_read_reg(0x40000, 4));

		if (!start_wifi(aml_wifi)) {
			// set usb mode env to kernel when download fw success
			env_set_hex("usb2t_mode", 1);
			//run_command("print usb2t_mode", 0);
			printf("%s: end time:%u\n", __func__, get_time());
		}
	}
}
