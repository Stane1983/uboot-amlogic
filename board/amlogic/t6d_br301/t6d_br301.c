// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <errno.h>
#include <env.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <amlogic/cpu_id.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/pinctrl_init.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#include <amlogic/aml_v3_burning.h>
#include <amlogic/aml_v2_burning.h>
#include <linux/mtd/partitions.h>
#include <mtd/mtd-abi.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/board.h>
#include <asm-generic/u-boot.h>
#include <command.h>
#include <asm/amlogic/arch/stick_mem.h>
#ifdef CONFIG_AMLOGIC_WIFI_USB2T
#include "wifi.h"
#endif
#ifdef CONFIG_AMLOGIC_AMFC
#include <amlogic/amfc.h>
#endif
#include <amlogic/aml_profile.h>
#ifdef CONFIG_AML_CVBS
#include <amlogic/media/vout/aml_cvbs.h>
#endif
#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/lcd_vout.h>
#endif
#ifdef CONFIG_CMD_AML_MODEL
#include <amlogic/aml_model.h>
#endif
#ifdef CONFIG_RX_RTERM
#include <amlogic/aml_hdmirx.h>
#endif
#ifdef CONFIG_AML_VOUT
#include <amlogic/media/vout/aml_vout.h>
#endif
#ifdef CONFIG_ARMV8_MULTIENTRY
#include <asm/arch-meson/smp.h>
#endif
#ifdef CONFIG_AMLOGIC_AMFC
#include <amlogic/amfc.h>
#endif
#ifdef CONFIG_RX_RTERM
#include <amlogic/aml_hdmirx.h>
#endif
#ifdef CONFIG_CEC_TRIM_VAL
#include <amlogic/aml_cec.h>
#endif
#ifdef CONFIG_CVBS_CALI
#include <amlogic/aml_tvafe.h>
#endif
DECLARE_GLOBAL_DATA_PTR;

void sys_led_init(void)
{
}

int serial_set_pin_port(unsigned long port_base)
{
	return 0;
}

int dram_init(void)
{
	gd->ram_size = (readl(SYSCTRL_SEC_STATUS_REG4) & ~0xfffffUL) << 4;
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is only for compiling pass
 */
void secondary_boot_func(void)
{
}

int board_eth_init(bd_t *bis)
{
	return 0;
}

int active_clk(void)
{
	struct udevice *clk = NULL;
	int err;

	err = uclass_get_device_by_name(UCLASS_CLK, "xtal-clk", &clk);
	if (err) {
		pr_err("Can't find xtal-clk clock (%d)\n", err);
		return err;
	}
	err = uclass_get_device_by_name(UCLASS_CLK, "clock-controller@0", &clk);
	if (err) {
		pr_err("Can't find clock-controller@0 clock (%d)\n", err);
		return err;
	}

	return 0;
}

void board_init_mem(void)
{
	/* config bootm low size, make sure whole dram/psram space can be used */
	phys_size_t ram_size;
	char *env_tmp;

	env_tmp = env_get("bootm_size");
	if (!env_tmp) {
		ram_size =
		    ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) >
		    0xe0000000 ? 0xe0000000 : ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4);
		env_set_hex("bootm_low", 0);
		env_set_hex("bootm_size", ram_size);
	}
}

extern void usb_power_init(void);
extern void update_for_dtb_wakeup_key(void);

int board_init(void)
{
	//run_command("irkey_wakeup 3 1", 0);                   //enable bl30 ir
	clrsetbits_le32(PADCTRL_PIN_MUX_REGB, 0xf << 20, 0x1 << 20);//set GPIOD_5 pinmux as IR_IN

	printf("board init\n");

#if 0
	run_command("startdsp 0 0x300a0000 0", 0);
	printf("dsp start!\n");
	while (1)
		;
#endif

#if !defined(CONFIG_PXP_DDR)	//bypass below operations for pxp
	aml_set_bootsequence(0);
	//Please keep try usb boot first in board_init,
	//as other init before usb may cause burning failure
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if ((readl(SYSCTRL_SEC_STICKY_REG2) != 0x1b8ec003) &&
	    (readl(SYSCTRL_SEC_STICKY_REG2) != 0x1b8ec004)) {
		aml_v3_factory_usb_burning(0, gd->bd);
		//
	}
#endif //#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)

#if 0
	active_clk();
#endif
	run_command("gpio set GPIOH_7", 0);

#endif // #if !defined(CONFIG_PXP_DDR) //bypass below operations for pxp
	pinctrl_devices_active(PIN_CONTROLLER_NUM);
#ifdef CONFIG_AMLOGIC_AMFC
	amfc_init();
#endif
	usb_power_init();
	return 0;
}

#ifdef CONFIG_MODEL_DETECT
#ifdef CONFIG_CMD_AML_MODEL
static int board_model_det_gpio_get_val(char *str_gpio)
{
	unsigned int val;
	int gpio, ret;

#if defined(CONFIG_DM_GPIO)
	ret = gpio_lookup_name(str_gpio, NULL, NULL, (unsigned int *)&gpio);
	if (ret) {
		printf("GPIO: '%s' not found\n", str_gpio);
		return -1;
	}
#else
	/* turn the gpio name into a gpio number */
	gpio = name_to_gpio(str_gpio);
	if (gpio < 0) {
		printf("GPIO: '%s' not found\n", str_gpio);
		return -1;
	}
#endif
	/* grab the pin before we tweak it */
	ret = gpio_request(gpio, "board_model_detect");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return -1;
	}
	gpio_direction_input(gpio);
	val = gpio_get_value(gpio);
	printf("gpio: pin %s (gpio %u) value is ", str_gpio, gpio);
	if (IS_ERR_VALUE(val)) {
		printf("unknown (ret=%d)\n", val);
		return -1;
	}
	printf("%d\n", val);
	gpio_free(gpio);

	return val;
}

static int board_model_det_saradc_get_val(int saradc_ch, int level_max, unsigned int *level_table)
{
	char cmd_str[32];
	unsigned int val;
	int i, level = -1;

	snprintf(cmd_str, 32, "saradc open %d", saradc_ch);
	run_command(cmd_str, 0);
	run_command("saradc getval", 0);
	val = env_get_ulong("saradc_val", 16, 0);

	for (i = 0; i < level_max; i++) {
		if (val >= level_table[i * 2] && val <= level_table[i * 2 + 1]) {
			level = i;
			break;
		}
	}
	printf("board_model_det: saradc_val: %d, level: %d\n", val, level);
	return level;
}

#define DEFAULT_MODEL_DET_PATH      "/vendor/etc/tvconfig/model/model_det.ini"
static void board_model_detect(void)
{
	char *model_path, key_str[32], det_key_str[32];
	void *local_ini_mem, *psec;
	const char *str = NULL;
	unsigned int val, adc_ch, level_max, *level_table, table_size;
	int i, ret;

	env_set("model_name_factory", "none");
	model_path = env_get("model_det_path");
	if (!model_path)
		model_path = DEFAULT_MODEL_DET_PATH;

	//printf("%s: %s\n", __func__, model_path);

	//load model_detection file
	local_ini_mem = handle_ini_file_parse(model_path, 0);
	if (!local_ini_mem) {
		printf("error: %s: %s\n", __func__, model_path);
		return;
	}

	psec = handle_ini_get_section(local_ini_mem, "MODEL_DETECT_CONFIG");
	if (!psec) {
		printf("error: %s: section error\n", __func__);
		goto board_model_detect_exit;
	}

	val = handle_ini_get_val(local_ini_mem, psec, "detect_en", 0);
	if (val == 0)
		goto board_model_detect_exit;

	//model detect
	str = handle_ini_get_str(local_ini_mem, psec, "detect_mothod", "null");
	if (strcmp(str, "null") == 0)
		goto board_model_detect_exit;
	if (strcmp(str, "GPIO") == 0) {
		str = handle_ini_get_str(local_ini_mem, psec, "detect_gpio_pin", "null");
		if (strcmp(str, "null") == 0)
			goto board_model_detect_exit;

		ret = board_model_det_gpio_get_val(str);
		if (ret < 0)
			goto board_model_detect_exit;

		snprintf(det_key_str, 32, "detect_model_name_%d", ret);
		str = handle_ini_get_str(local_ini_mem, psec, det_key_str, "none");
		//update env
		env_set("model_name_factory", str);
	} else if (strcmp(str, "SARADC") == 0) {
		adc_ch = handle_ini_get_val(local_ini_mem, psec, "detect_saradc_ch", 0xff);
		if (adc_ch == 0xff)
			goto board_model_detect_exit;

		level_max = handle_ini_get_val(local_ini_mem, psec,
					       "detect_saradc_level_max", 0xffff);
		if (level_max == 0xffff)
			goto board_model_detect_exit;

		table_size = sizeof(unsigned int) * level_max * 2; //min~max
		level_table = (unsigned int *)malloc(table_size);
		if (!level_table)
			goto board_model_detect_exit;

		for (i = 0; i < level_max; i++) {
			snprintf(key_str, 32, "detect_saradc_level_%d_min", i);
			val = handle_ini_get_val(local_ini_mem, psec, key_str, 0xffffffff);
			if (val == 0xffffffff) {
				memset(level_table, 0, table_size);
				free(level_table);
				goto board_model_detect_exit;
			}
			level_table[i * 2] = val;

			snprintf(key_str, 32, "detect_saradc_level_%d_max", i);
			val = handle_ini_get_val(local_ini_mem, psec, key_str, 0xffffffff);
			if (val == 0xffffffff) {
				memset(level_table, 0, table_size);
				free(level_table);
				goto board_model_detect_exit;
			}
			level_table[i * 2 + 1] = val;
		}

		ret = board_model_det_saradc_get_val(adc_ch, level_max, level_table);
		snprintf(det_key_str, 32, "detect_model_name_%d", ret);
		str = handle_ini_get_str(local_ini_mem, psec, det_key_str, "none");
		//update env
		env_set("model_name_factory", str);
	}

board_model_detect_exit:
	handle_ini_parser_uninit(local_ini_mem);
}
#endif
#endif

int board_late_init(void)
{
#ifdef CONFIG_AML_USB_BT
	char *ddr_resume = NULL;
#endif

	printf("board late init\n");
	env_set("defenv_para", "-c");
	aml_board_late_init_front(NULL);

#ifdef CONFIG_PXP_EMULATOR
	return 0;
#endif

	PUSH_TIME_TE("vpu vpp init", BL33_VPUVPP_INIT_s);

#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
#ifdef CONFIG_AML_VPP
	vpp_init();
#endif
#ifdef CONFIG_RX_RTERM
	rx_set_phy_rterm();
#endif
#ifdef CONFIG_AML_CVBS
	cvbs_init();
#endif
#ifdef CONFIG_CVBS_CALI
	cvbs_dac_cfg();
#endif
#ifdef CONFIG_CEC_TRIM_VAL
	cec_get_efuse_val();
#endif

#ifdef CONFIG_MODEL_DETECT
#ifdef CONFIG_CMD_AML_MODEL
	board_model_detect();
#endif
#endif
	run_command("ini_model", 0);
	aml_board_display_env_handler();
#ifdef CONFIG_AML_LCD
	lcd_probe();
#endif
	run_command("logo_display", 0);
	run_command("run init_display", 0);

	PUSH_TIME_TE("vpu vpp init", BL33_VPUVPP_INIT_e);

#ifdef CONFIG_AMLOGIC_WIFI_USB2T
	wifi_init();
#endif

#ifdef CONFIG_AML_USB_BT
	// bt download firmware
	ddr_resume = env_get("ddr_resume");

	if (ddr_resume && ddr_resume[0] == '1')
		run_command("usb bt_start", 0);
#endif
	aml_board_late_init_tail(NULL);

	run_command("multi_remote", 0);
	run_command("multi_keypad", 0);
	update_for_dtb_wakeup_key();  //set bl30 preboot key list
	return 0;
}

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) > 0xe0000000 ? 0xe0000000 :
	    ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4) > 0xe0000000 ? 0xe0000000 :
	    ((readl(SYSCTRL_SEC_STATUS_REG4) & 0xFFF00000) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
}

int mach_cpu_init(void)
{
	//printf("\nmach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}

/* partition table for spinor flash */
#ifdef CONFIG_SPI_FLASH
static const struct mtd_partition spiflash_partitions[] = {
	{
	 .name = "env",
	 .offset = 0,
	 .size = 1 * SZ_256K,
	  },
	{
	 .name = "dtb",
	 .offset = 0,
	 .size = 1 * SZ_256K,
	  },
	{
	 .name = "boot",
	 .offset = 0,
	 .size = 2 * SZ_1M,
	  },
	/* last partition get the rest capacity */
	{
	 .name = "user",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,
	}
};

const struct mtd_partition *get_spiflash_partition_table(int *partitions)
{
	*partitions = ARRAY_SIZE(spiflash_partitions);
	return spiflash_partitions;
}
#endif /* CONFIG_SPI_FLASH */

struct rsv_part rsv_partitions[] = {
#ifndef CONFIG_ENV_IS_IN_NAND
	{ ENV_NAND_MAGIC, 0, MTD_RSV_ENV_BLOCK_CNT, CONFIG_ENV_SIZE },
#endif
	{ KEY_NAND_MAGIC, 0, MTD_RSV_KEY_BLOCK_CNT, MTD_RSV_KEY_SIZE },
#ifndef CONFIG_DTB_BIND_KERNEL
	{ DTB_NAND_MAGIC, 0, MTD_RSV_DTB_BLOCK_CNT, MTD_RSV_DTB_SIZE },
#endif
};

struct rsv_part *get_mtd_rsv_partition(void)
{
	return rsv_partitions;
}

int get_mtd_rsv_partition_count(void)
{
	/* bbt is madatory, so add one  */
	return ARRAY_SIZE(rsv_partitions) + 1;
}
#ifdef CONFIG_MESON_NFC
static struct mtd_partition normal_partition_info[] = {
	{
	 .name = BOOT_BL2E,
	 .offset = 0,
	 .size = 0,
	  },
	{
	 .name = BOOT_BL2X,
	 .offset = 0,
	 .size = 0,
	  },
	{
	 .name = BOOT_DDRFIP,
	 .offset = 0,
	 .size = 0,
	  },
	{
	 .name = BOOT_DEVFIP,
	 .offset = 0,
	 .size = 0,
	  },
	{
	 .name = "logo",
	 .offset = 0,
	 .size = 2 * SZ_1M,
	  },
	{
	 .name = "recovery",
	 .offset = 0,
	 .size = 16 * SZ_1M,
	  },
	{
	 .name = "boot",
	 .offset = 0,
	 .size = 16 * SZ_1M,
	  },
	{
	 .name = "system",
	 .offset = 0,
	 .size = 64 * SZ_1M,
	  },
/* last partition get the rest capacity */
	{
	 .name = "data",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,
	  },
};

struct mtd_partition *get_aml_mtd_partition(void)
{
	return normal_partition_info;
}

int get_aml_partition_count(void)
{
	return ARRAY_SIZE(normal_partition_info);
}

#endif

/* partition table */
/* partition table for spinand flash */
#if (defined(CONFIG_SPI_NAND) || defined(CONFIG_MTD_SPI_NAND))
static const struct mtd_partition spinand_partitions[] = {
	{
	 .name = "logo",
	 .offset = 0,
	 .size = 2 * SZ_1M,
	  },
	{
	 .name = "recovery",
	 .offset = 0,
	 .size = 16 * SZ_1M,
	  },
	{
	 .name = "boot",
	 .offset = 0,
	 .size = 16 * SZ_1M,
	  },
	{
	 .name = "system",
	 .offset = 0,
	 .size = 64 * SZ_1M,
	  },
	/* last partition get the rest capacity */
	{
	 .name = "data",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL,
	}
};

const struct mtd_partition *get_spinand_partition_table(int *partitions)
{
	*partitions = ARRAY_SIZE(spinand_partitions);
	return spinand_partitions;
}
#endif /* CONFIG_SPI_NAND */

#ifdef CONFIG_MULTI_DTB
int checkhw(char *name)
{
	char dtb_name[64] = { 0 };
	cpu_id_t cpu_id = get_cpu_id();

	if (cpu_id.family_id == 0x3A)
		strcpy(dtb_name, "s4d_s905y4_ap222\0");
	else if (cpu_id.family_id == 0x37)
		strcpy(dtb_name, "s4_s905y4_ap222\0");

	strcpy(name, dtb_name);
	env_set("aml_dt", dtb_name);
	return 0;
}
#endif

int __attribute__((weak)) mmc_initialize(bd_t *bis)
{
	return 0;
}

//int __attribute__((weak)) do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
//{
//	return 0;
//}

void __attribute__((weak)) set_working_fdt_addr(ulong addr)
{
}

//int __attribute__((weak)) ofnode_read_u32_default(ofnode node, const char *propname, u32 def)
//{
//	return 0;
//}

void __attribute__((weak)) md5_wd(unsigned char *input, int len, unsigned char output[16],
				  unsigned int chunk_sz)
{
}
