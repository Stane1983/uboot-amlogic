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
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/board.h>
#include <asm-generic/u-boot.h>
#include <command.h>

#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#ifdef CONFIG_AML_HDMITX21
#include <amlogic/media/vout/hdmitx21/hdmitx_module.h>
#endif
#ifdef CONFIG_AML_CVBS
#include <amlogic/media/vout/aml_cvbs.h>
#endif
#ifdef CONFIG_AMLOGIC_AMFC
#include <amlogic/amfc.h>
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
	gd->ram_size = (readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4;
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

#ifdef CONFIG_AML_HDMITX20
static void hdmitx_set_hdmi_5v(void)
{
	/*Power on VCC_5V for HDMI_5V */
}
#endif
void board_init_mem(void)
{
	/* config bootm low size, make sure whole dram/psram space can be used */
	phys_size_t ram_size;
	char *env_tmp;

	env_tmp = env_get("bootm_size");
	if (!env_tmp) {
		ram_size =
		    ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4) >
		    0xe0000000 ? 0xe0000000 : ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4);
		env_set_hex("bootm_low", 0);
		env_set_hex("bootm_size", ram_size);
	}
}

int board_init(void)
{
	printf("board init\n");

#ifdef CONFIG_AML_HDMITX21
	hdmitx21_chip_type_init(MESON_CPU_ID_S6);
	hdmitx21_init();
#endif

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
	return 0;
}

int board_late_init(void)
{
	printf("board late init\n");
	env_set("defenv_para", "-c");
	aml_board_late_init_front(NULL);
#ifdef CONFIG_PXP_EMULATOR
	return 0;
#endif

#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
#ifdef CONFIG_AML_VPP
	vpp_init();
#endif
#ifdef CONFIG_AML_CVBS
	cvbs_init();
#endif

	aml_board_late_init_tail(NULL);
	return 0;
}

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4) > 0xe0000000 ? 0xe0000000 :
	    (((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4) - CONFIG_SYS_MEM_TOP_HIDE);
#else
	return ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4) > 0xe0000000 ? 0xe0000000 :
	    ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xffffUL) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
}

int mach_cpu_init(void)
{
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

	strcpy(dtb_name, "s6_s905x5_skt\0");

	strcpy(name, dtb_name);
	env_set("aml_dt", dtb_name);
	return 0;
}
#endif

const char *const _env_args_reserve_[] = {
	"lock",
	"upgrade_step",
	"bootloader_version",

	NULL			//Keep NULL be last to tell END
};

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
