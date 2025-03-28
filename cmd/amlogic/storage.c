// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/storage.h>
#include <div64.h>
#include <linux/math64.h>
#include <amlogic/cpu_id.h>
#include <amlogic/store_wrapper.h>
#include <asm/amlogic/arch/register.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_efuse.h>
#include <asm/amlogic/arch/cpu_config.h>
#include <asm/amlogic/arch/romboot.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <amlogic/blxx2bl33_param.h>
#include <amlogic/aml_mtd.h>
#include <mmc.h>

#include <display_options.h>

#define ALIGN_SIZE	(4096)

#ifdef CONFIG_SPI_FLASH_MTD
extern int spi_nor_pre(void);
extern int spi_nor_probe(u32 init_flag);
#endif

#ifdef CONFIG_MTD_SPI_NAND
extern int spi_nand_pre(void);
extern int spi_nand_probe(u32 init_flag);
#endif

#ifdef CONFIG_MESON_NFC
extern int nand_pre(void);
extern int nand_probe(uint32_t init_flag);
#endif

#ifdef CONFIG_MMC_MESON_GX
extern int emmc_pre(void);
extern int emmc_probe(u32 init_flag);
#endif

#ifdef CONFIG_MMC_MESON_GX
extern int sdcard_pre(void);
extern int sdcard_probe(u32 init_flag);
#endif
/* for info protect, fixme later */
int info_disprotect = 0;

static struct storage_t *current;
static struct device_node_t device_list[] = {
#ifdef CONFIG_MESON_NFC
	{BOOT_NAND_MTD, "mtd", nand_pre, nand_probe},
#endif
#ifdef CONFIG_MTD_SPI_NAND
	{BOOT_SNAND, "spi-nand", spi_nand_pre, spi_nand_probe},
#endif
#ifdef CONFIG_SPI_FLASH_MTD
	{BOOT_SNOR, "spi-nor", spi_nor_pre, spi_nor_probe},
#endif
#if 0
	{BOOT_SD, "sd", sdcard_pre, sdcard_probe},
#endif

#ifdef CONFIG_MMC_MESON_GX
	{BOOT_EMMC, "emmc", emmc_pre, emmc_probe},
#endif

};

int store_register(struct storage_t *store_dev)
{
	if (!store_dev)
		return 1;
	if (!current) {
		INIT_LIST_HEAD(&store_dev->list);
		current = store_dev;
		return 0;
	}
	/**
	 * the head node will not be a valid node
	 * usually when we use the list, but in storage
	 * interface module, we init the device node as
	 * a head instead a global list_head pointer,
	 * it should be traver scaled.
	 */
	if (store_dev == current)
		return 0;
	struct storage_t *dev;

	if (store_dev->type == current->type)
		return 1;
	list_for_each_entry(dev, &current->list, list) {
		if (dev == store_dev)
			return 0;
		else if (dev->type == store_dev->type)
			return 1;
	}
	list_add_tail(&store_dev->list, &current->list);
	current = store_dev;
	return 0;
}

void store_unregister(struct storage_t *store_dev)
{
	if (store_dev == current) {
		if (list_empty_careful(&store_dev->list)) {
			current = NULL;
		} else {
			current = list_entry((current->list).next,
					     struct storage_t, list);
			list_del_init(&store_dev->list);
		}
	} else {
		list_del_init(&store_dev->list);
	}
}

int store_init(u32 init_flag)
{
	int i, ret = 0;
	u8 record = 0;

	/*1. pre scan*/
	for (i = 0; i < ARRAY_SIZE(device_list); i++) {
		if ((record & BOOT_NAND_MTD) &&
		     ((device_list[i].index & BOOT_SNAND) ||
		       (device_list[i].index & BOOT_SNOR)))
			continue;
		if (!device_list[i].pre()) {
			record |= device_list[i].index;
		}
	}

	pr_info("record = 0x%x\n", record);

	if (!record) {
		pr_info("No Valid storage device\n");
		return record;
	}

	/*2. Enter the probe of the valid device*/
	for (i = 0; i < ARRAY_SIZE(device_list); i++) {
		if (record & device_list[i].index) {
			ret = device_list[i].probe(init_flag);
			if (ret)
				pr_info("the 0x%x storage device probe failed\n",
			device_list[i].index);
		}
	}

	return record;
}

static struct storage_t *store_get_current(void)
{
	return current;
}

int store_set_device(enum boot_type_e type)
{
	struct list_head *entry;
	struct storage_t *dev, *store_dev = store_get_current();

	if (!store_dev) {
		pr_info("%s %d no current device\n", __func__, __LINE__);
		return 1;
	}
	if (store_dev->type == type)
		return 0;
	list_for_each(entry, &store_dev->list) {
		dev = list_entry(entry, struct storage_t, list);
		if (dev->type == type) {
			current = dev;
			return 0;
		}
	}
	pr_info("%s %d please confirm the %d device is valid\n",
		__func__, __LINE__, type);
	return 1;
}

enum boot_type_e store_get_type(void)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return BOOT_NONE;
	}

	return store->type;
}

int store_get_device_info(struct storage_info_t *info)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}

	memcpy((char *)info, (char *)&store->info,
	       sizeof(struct storage_info_t));
	return 0;
}

int store_get_device_bootloader_mode(void)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return -1;
	}
	return store->info.mode;
}

int store_read(const char *name, loff_t off, size_t size, void *buf)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}
	return store->read(name, off, size, buf);
}

int store_write(const char *name, loff_t off, size_t size, void *buf)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}
	return store->write(name, off, size, buf);
}

int store_erase(const char *name, loff_t off, size_t size, int scrub)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}
	return store->erase(name, off, size, scrub);
}

u64 store_part_size(const char *name)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}
	return store->get_part_size(name);
}

u8 store_boot_copy_num(const char *name)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}
	return store->get_copies(name);
}

u8 emmc_boot_seqs_tbl[8][2] = {
		{0, 3}, {0, 2}, {0, 3}, {0, 1},
		{1, 2}, {1, 1}, {2, 1}, {0, 0}
	};

static int _get_emmc_boot_seqs(void)
{
	u8 ebcfg = 0;

	if (IS_FEAT_DIS_EMMC_USER())
		ebcfg |= (1 << 2);
	if (IS_FEAT_DIS_EMMC_BOOT_0())
		ebcfg |= (1 << 1);
	if (IS_FEAT_DIS_EMMC_BOOT_1())
		ebcfg |= (1 << 0);

	return ebcfg;
}

static int storage_get_emmc_boot_start(void)
{
	return emmc_boot_seqs_tbl[_get_emmc_boot_seqs()][0];
}

#ifndef  SYSCTRL_SEC_STATUS_REG2
static u32 fake_reg = 0;
#define SYSCTRL_SEC_STATUS_REG2		(&fake_reg)
#endif
u8 store_boot_copy_start(void)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 0;
	}
	if (store->type != BOOT_EMMC)
		return 0;
	/* new arch since sc2 */
	if (BOOTLOADER_MODE_ADVANCE_INIT)
		return storage_get_emmc_boot_start();
	return 0;
}

u8 store_bootup_bootidx(const char *name)
{
	u8 bl2_idx = 0, fip_idx = 0;
	u32 val = 0;

	/* new arch since sc2 */
	if (BOOTLOADER_MODE_ADVANCE_INIT) {
			bl2_idx = readl(SYSCTRL_SEC_STATUS_REG2) & 0xF;
			//TODO: fixme after robust devfip is finished.
			fip_idx = bl2_idx;
	} else {
		/* according to the:
			commit 975b4acbcfa686601999d56843471d98e9c0a2cd
			storage: robust boot: record bootlog in SEC_AO_SEC_GP_CFG2 [1/2]
			PD#SWPL-4850
			...
			record the bootup bl2/fip into SEC_AO_SEC_GP_CFG2
			bit[27-25] bl2
			bit[24-22] fip
		*/
		val = readl(SEC_AO_SEC_GP_CFG2);
		bl2_idx = (val >> 25) & 0x7;
		fip_idx = (val >> 22) & 0x7;
	}
	if (!strncmp(name, "bl2", sizeof("bl2")) ||
			!strncmp(name, "spl", sizeof("spl")))
		return bl2_idx;
	else
		return fip_idx;
}

void store_restore_bootidx(void)
{
	/* new arch since sc2 */
	if (BOOTLOADER_MODE_ADVANCE_INIT) {
		extern void aml_set_bootsequence(uint32_t val);
		aml_set_bootsequence(0x55);
	}
	return;
}

u64 store_boot_copy_size(const char *name)
{
	struct storage_t *store = store_get_current();

	return store->get_copy_size(name);
}

int store_send_param(void)
{
	struct storage_t *store = store_get_current();

	return store->param_ops ? store->param_ops() : 0;
}

static int _store_boot_read(const char *part_name, u8 cpy, size_t size, void *addr)
{
	struct storage_t *store = store_get_current();
	size_t bl2_align_size = ALIGN(BL2_SIZE, ALIGN_SIZE);
	u_char *buffer = (u_char *)addr;
	int ret;

	if (store_get_device_bootloader_mode() == COMPACT_BOOTLOADER)
		return store->boot_read(part_name, cpy, size, buffer);

	ret = store->boot_read(BOOT_BL2, cpy, BL2_SIZE, buffer);
	if (ret)
		return ret;

	return store->boot_read(BOOT_TPL, cpy,
				  size - bl2_align_size,
				  buffer + bl2_align_size);
}

int store_boot_read(const char *name, u8 copy, size_t size, void *buf)
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init storage device first\n",
			__func__, __LINE__);
		return 1;
	}

	if (!strcmp(name, "bootloader") &&
	    (store->type == BOOT_SNAND ||
	    store->type == BOOT_NAND_MTD))
		return _store_boot_read(name, copy, size, buf);

	return store->boot_read(name, copy, size, buf);
}

static int _store_boot_write(const char *part_name, u8 cpy, size_t size, void *addr)
{
	struct storage_t *store = store_get_current();
	int ret = 0;
	u_char *buffer = (u_char *)addr;
	size_t bl2_align_size = ALIGN(BL2_SIZE, ALIGN_SIZE);

	if (store_get_device_bootloader_mode() == COMPACT_BOOTLOADER)
		return store->boot_write(part_name, cpy, size, buffer);

	ret =  store->boot_write("bl2", cpy, BL2_SIZE, buffer);
	if (ret)
		return ret;

	return store->boot_write("tpl", cpy,
				 size - bl2_align_size,
				 buffer + bl2_align_size);
}

int store_boot_write(const char *name, u8 copy, size_t size, void *buf)
{
	return _store_boot_write(name, copy, size, buf);
}

int store_boot_erase(const char *name, u8 copy)
{
	struct storage_t *store = store_get_current();

	return store->boot_erase(name, copy);
}

int store_gpt_read(void *buf)
{
	struct storage_t *store = store_get_current();

	if (!store->gpt_read)
		return 1;

	return store->gpt_read(buf);
}

int store_gpt_write(void *buf)
{
	struct storage_t *store = store_get_current();

	if (!store->gpt_write)
		return 1;

	return store->gpt_write(buf);
}

int store_gpt_erase(void)
{
	struct storage_t *store = store_get_current();

	if (!store->gpt_erase)
		return 1;

	return store->gpt_erase();
}

int store_boot_copy_enable(int index)
{
	struct storage_t *store = store_get_current();

	if (!store->boot_copy_enable)
		return -1;

	return store->boot_copy_enable(index);
}

u32 store_rsv_size(const char *name)
{
	struct storage_t *store = store_get_current();

	return store->get_rsv_size(name);
}

int store_rsv_read(const char *name, size_t size, void *buf)
{
	struct storage_t *store = store_get_current();

	return store->read_rsv(name, size, buf);
}

int store_rsv_write(const char *name, size_t size, void *buf)
{
	struct storage_t *store = store_get_current();

	return store->write_rsv(name, size, buf);
}

int store_rsv_erase(const char *name)
{
	struct storage_t *store = store_get_current();

	return store->erase_rsv(name);
}

int store_rsv_protect(const char *name, bool ops)
{
	struct storage_t *store = store_get_current();

	return store->protect_rsv(name, ops);
}

static int do_store_init(cmd_tbl_t *cmdtp,
			 int flag, int argc, char * const argv[])
{
	u32 init_flag = 1;
	u8 ret = 0;

	if (unlikely(argc != 2 && argc != 3))
		return CMD_RET_USAGE;

	if (argc == 3)
		init_flag = simple_strtoul(argv[2], NULL, 10);

	/*Returns a nonzero value: device index*/
	if (store_init(init_flag))
		ret = 0;
	else ret = 1;
	return ret;
}

void store_print_device(struct storage_t *store_dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(device_list); i++)
		if (store_dev->type & device_list[i].index)
			pr_info("device type: [%s]\n", device_list[i].type);
	pr_info("name %s\n", store_dev->info.name);
	pr_info("id :");
	for (i = 0; i < ARRAY_SIZE(store_dev->info.id); i++)
		pr_info(" 0x%x", store_dev->info.id[i]);
	pr_info("\n");
	pr_info("read unit %d\n", store_dev->info.read_unit);
	pr_info("write unit %d\n", store_dev->info.write_unit);
	pr_info("erase unit %d\n", store_dev->info.erase_unit);
	pr_info("total size %lld\n", store_dev->info.caps);
	if (store_dev->info.mode)
		pr_info("bootloader in discrete mode : %d\n",
			store_dev->info.mode);
	else
		pr_info("bootloader in compact mode : %d\n",
			store_dev->info.mode);
}

static int do_store_device(cmd_tbl_t *cmdtp,
			int flag, int argc, char * const argv[])
{
	if (argc == 2) {
		struct storage_t *store_dev, *dev;
		struct list_head *entry;

		store_dev = store_get_current();
		pr_info("current device:\n");
		pr_info("----------------------------------\n");
		store_print_device(store_dev);
		pr_info("----------------------------------\n");
		list_for_each(entry, &store_dev->list) {
			dev = list_entry(entry, struct storage_t, list);
			pr_info("valid device:\n");
			pr_info("----------------------------------\n");
			store_print_device(dev);
			pr_info("----------------------------------\n");
		}
		return 0;
	} else if (argc == 3) {
		char *name = NULL;
		int i = 0, ret = 0;
		name = argv[2];
		for (i = 0; i < ARRAY_SIZE(device_list); i++)
			if (!strcmp(name, device_list[i].type)) {

				ret = store_set_device(device_list[i].index);
				if (!ret) {
					pr_info("now current device is: %s\n",
						name);
					return 0;
				}
			}
		pr_info("%s %d no such device: %s\n",
			__func__, __LINE__, name);
		return ret;
	}
	return CMD_RET_USAGE;
}

static int do_store_partition(cmd_tbl_t *cmdtp,
			int flag, int argc, char * const argv[])
{
	struct storage_t *store_dev;
	int i = 0, partitions = 0;
	int ret = 0;
	char name[16];

	if (argc > 2)
		return CMD_RET_USAGE;
	else {
		store_dev = store_get_current();
		if (store_dev->get_part_count)
			partitions = store_dev->get_part_count();
		pr_err("%d partitions of device %s:\n",
		       partitions, store_dev->info.name);

		if (store_dev->list_part_name)
			ret = store_dev->list_part_name(i, name);

		return ret;
	}
}

#ifdef CONFIG_AML_MTD
extern int is_mtd_store_boot_area(const char *part_name);
#endif
static int do_store_erase(cmd_tbl_t *cmdtp,
			  int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long offset;
	size_t size = 0;
	char *name = NULL;
	char *s;
	int erase_flag = 0, ret;
	unsigned long time;

	const char *scrub =
		"Warning: scrub_flag is 1!!!!"
		"scrub operation!!!\n"
		"will erase oob area\n"
		"There is no reliable way to recover them.\n"
		"		  "
		"are sure of what you are doing!\n"
		"\nReally erase this NAND flash? <y/N>\n";

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (!strncmp(argv[1], "scrub", 5)) {
		erase_flag |= STORE_SCRUB;
		puts(scrub);
		if (!confirm_yesno()) {
			printf("erase aborted\n");
			return 1;
		}
	}

	/*store erase.chip*/
	s = strchr(argv[1], '.');
	if (s != NULL && strcmp(s, ".chip") == 0) {
		if (argc == 3 && !simple_strtoul(argv[argc - 1], NULL, 16))
			erase_flag |= STORE_ERASE_DATA;
		else if ((argc == 3) && (simple_strtoul(argv[argc - 1], NULL, 16) == 1))
			erase_flag |= STORE_ERASE_RSV;
		else if (argc == 3)
			return CMD_RET_USAGE;

		offset = 0;
	} else {
		/*store erase normal, partition name can't NULL*/
		if (unlikely(argc != 5))
			return CMD_RET_USAGE;

		size = (size_t)simple_strtoul(argv[argc - 1], NULL, 16);
		offset = simple_strtoul(argv[argc - 2], NULL, 16);
		name = argv[2];
#ifdef CONFIG_AML_MTD
		if (is_mtd_store_boot_area(name)) {
			pr_info("%s %d please enter normal partition name except tpl area!\n",
				__func__, __LINE__);
			return CMD_RET_FAILURE;
		}
#endif
	}

	time = get_timer(0);
	ret = store->erase(name, offset, size, erase_flag);
	time = get_timer(time);

	if (size != 0)
		printf("%lu bytes ", size);

	printf("erased in %lu ms", time);
	if ((time > 0) && (size != 0)) {
		puts(" (");
		print_size(div_u64(size, time) * 1000, "/s");
		puts(")");
	}
	puts("\n");

	return ret;
}

static int do_store_read(cmd_tbl_t *cmdtp,
			 int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long offset, addr, time;
	size_t size;
	char *name = NULL;
	int ret;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 5 && argc != 6))
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[2], NULL, 16);
	size = (size_t)simple_strtoul(argv[argc - 1], NULL, 16);
	offset = simple_strtoul(argv[argc - 2], NULL, 16);
	if (argc == 6)
		name = argv[3];
#ifdef CONFIG_AML_MTD
	if (is_mtd_store_boot_area(name)) {
			pr_info("%s %d please enter normal partition name except tpl area!\n",
				__func__, __LINE__);
			return CMD_RET_FAILURE;
		}
#endif
	time = get_timer(0);
	ret = store->read(name, offset, size, (u_char *)addr);
	time = get_timer(time);

	if (size != 0)
		printf("%lu %s ", size, (argc == 6) ? "bytes" : "blocks");

	printf("read in %lu ms", time);
	if ((time > 0) && (size != 0)) {
		puts(" (");
		print_size(div_u64(size, time) * 1000, "/s");
		puts(")");
	}
	puts("\n");

	return ret;
}

static int do_store_write(cmd_tbl_t *cmdtp,
			  int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long offset, addr, time;
	size_t size;
	char *name = NULL;
	int ret;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 5 && argc != 6))
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[2], NULL, 16);
	offset = simple_strtoul(argv[argc - 2], NULL, 16);
	size = (size_t)simple_strtoul(argv[argc - 1], NULL, 16);
	if (argc == 6)
		name = argv[3];
#ifdef CONFIG_AML_MTD
	if (is_mtd_store_boot_area(name)) {
			pr_info("%s %d please enter normal partition name except tpl area!\n",
				__func__, __LINE__);
			return CMD_RET_FAILURE;
		}
#endif
	time = get_timer(0);
	ret = store->write(name, offset, size, (u_char *)addr);
	time = get_timer(time);

	if (size != 0)
		printf("%lu %s ", size, (argc == 6) ? "bytes" : "blocks");

	printf("write in %lu ms", time);
	if ((time > 0) && (size != 0)) {
		puts(" (");
		print_size(div_u64(size, time) * 1000, "/s");
		puts(")");
	}
	puts("\n");

	return ret;
}

static int do_store_boot_read(cmd_tbl_t *cmdtp,
			      int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long addr;
	size_t size;
	u8 cpy;
	char *name;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 6))
		return CMD_RET_USAGE;

	name = argv[2];
	addr = (unsigned long)simple_strtoul(argv[3], NULL, 16);
	cpy = (u8)simple_strtoul(argv[4], NULL, 16);
	size = (size_t)simple_strtoul(argv[5], NULL, 16);

	return store->boot_read(name, cpy, size, (u_char *)addr);
}

static int do_store_boot_write(cmd_tbl_t *cmdtp,
			       int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long addr;
	size_t size;
	u8 cpy = BOOT_OPS_ALL;
	char *name;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 5 && argc != 6))
		return CMD_RET_USAGE;

	name = argv[2];
	addr = (unsigned long)simple_strtoul(argv[3], NULL, 16);
	size = (size_t)simple_strtoul(argv[argc - 1], NULL, 16);
	if (argc == 6)
		cpy = (u8)simple_strtoul(argv[4], NULL, 16);

	if (strcmp(name, "bootloader") == 0) {
		return _store_boot_write(name, cpy, size, (u_char *)addr);
	}

	return store->boot_write(name, cpy, size, (u_char *)addr);
}

static int do_store_boot_erase(cmd_tbl_t *cmdtp,
			       int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	u8 cpy = BOOT_OPS_ALL;
	char *name;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 3 && argc != 4))
		return CMD_RET_USAGE;

	name = argv[2];
	if (argc == 4)
		cpy = (u8)simple_strtoul(argv[3], NULL, 16);

	return store->boot_erase(name, cpy);
}

static int do_store_gpt_read(cmd_tbl_t *cmdtp,
			 int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long addr;
	int ret;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 3))
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[2], NULL, 16);

	if (store->gpt_read) {
		ret = store->gpt_read((u_char *)addr);
		return ret;
	}

	printf("read gpt is not prepared\n");
	return CMD_RET_USAGE;
}

static int do_store_gpt_write(cmd_tbl_t *cmdtp,
			 int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	unsigned long addr;
	int ret;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 3))
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[2], NULL, 16);

	if (store->gpt_write) {
		ret = store->gpt_write((u_char *)addr);
		return ret;
	}

	printf("write gpt is not prepared\n");
	return CMD_RET_USAGE;
}

static int do_store_gpt_erase(cmd_tbl_t *cmdtp,
			 int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	int ret;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (unlikely(argc != 2))
		return CMD_RET_USAGE;

	if (store->gpt_erase) {
		ret = store->gpt_erase();
		return ret;
	}

	printf("erase gpt is not prepared\n");
	return CMD_RET_USAGE;
}

/*
 * Check whether the current boot can be written
 * ret: 0 disable; 1: enable
 */
static int do_store_boot_copy_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum boot_type_e medium_type = store_get_type();
	struct storage_t *store = store_get_current();
	int ret = CMD_RET_USAGE, index;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return ret;
	}

	if (unlikely(argc != 3))
		return ret;

	if (medium_type != BOOT_EMMC) {
		printf("%s %d not eMMC boot\n", __func__, __LINE__);
		return ret;
	}

	index = simple_strtoul(argv[2], NULL, 16);
	if (store->boot_copy_enable) {
		ret = store->boot_copy_enable(index);
		printf("%s\n", ret ? "enable" : "disable");
	} else
		printf("boot copy enable is not prepared\n");

	return ret;
}

static int do_store_rsv_ops(cmd_tbl_t *cmdtp,
			    int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();
	char *name = NULL;

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (!strcmp(argv[2], "erase")) {
		if (argc == 3)
			;
		else if (argc == 4)
			name = argv[3];
		else
			return CMD_RET_USAGE;
		return store->erase_rsv(name);
	} else if (!strcmp(argv[2], "read") ||
			   !strcmp(argv[2], "write")) {
		u8 cmd = strcmp(argv[2], "read") ? 0 : 1;
		unsigned long addr = simple_strtoul(argv[4], NULL, 16);
		size_t size = (size_t)simple_strtoul(argv[5], NULL, 16);

		name = argv[3];
		if (unlikely(argc != 6))
			return CMD_RET_USAGE;
		if (cmd)
			return store->read_rsv(name, size, (u_char *)addr);
		else
			return store->write_rsv(name, size, (u_char *)addr);
	} else if (!strcmp(argv[2], "protect")) {
		char *ops;
		flag = false;

		if (unlikely(argc != 4 && argc != 5))
			return CMD_RET_USAGE;

		name = (argc == 4) ? NULL : argv[3];
		ops = argv[argc - 1];
		if (!strcmp(ops, "on"))
			flag = true;
		else if (!strcmp(ops, "off"))
			flag = false;
		return store->protect_rsv(name, flag);
	}
	return CMD_RET_USAGE;
}

static int do_store_param_ops(cmd_tbl_t *cmdtp,
			    int flag, int argc, char * const argv[])
{
	struct storage_t *store = store_get_current();

	if (!store) {
		pr_info("%s %d please init your storage device first!\n",
			__func__, __LINE__);
		return CMD_RET_FAILURE;
	}

	if (store->param_ops)
		return store->param_ops();

	return 0;
}

static int do_store_ffu_op(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct storage_t *store = store_get_current();
	u64 ver, cnt;
	int ret = CMD_RET_FAILURE;
	void *addr;

	if (argc != 4)
		return CMD_RET_USAGE;

	ver = simple_strtoul(argv[1], NULL, 16);
	addr = (void *)simple_strtoul(argv[2], NULL, 16);
	cnt = simple_strtoul(argv[3], NULL, 16);

	if (store->ffu_op)
		ret = store->ffu_op(ver, addr, cnt);

	return (ret == 0) ? CMD_RET_SUCCESS : CMD_RET_FAILURE;
}

static cmd_tbl_t cmd_store_sub[] = {
	U_BOOT_CMD_MKENT(init, 4, 0, do_store_init, "", ""),
	U_BOOT_CMD_MKENT(device, 4, 0, do_store_device, "", ""),
	U_BOOT_CMD_MKENT(partition, 3, 0, do_store_partition, "", ""),
	U_BOOT_CMD_MKENT(scrub, 5, 0, do_store_erase, "", ""),
	U_BOOT_CMD_MKENT(erase, 5, 0, do_store_erase, "", ""),
	U_BOOT_CMD_MKENT(read, 6, 0, do_store_read, "", ""),
	U_BOOT_CMD_MKENT(write, 7, 0, do_store_write, "", ""),
	U_BOOT_CMD_MKENT(write_gpt, 3, 0, do_store_gpt_write, "", ""),
	U_BOOT_CMD_MKENT(read_gpt, 3, 0, do_store_gpt_read, "", ""),
	U_BOOT_CMD_MKENT(erase_gpt, 2, 0, do_store_gpt_erase, "", ""),
	U_BOOT_CMD_MKENT(boot_read,	6, 0, do_store_boot_read, "", ""),
	U_BOOT_CMD_MKENT(boot_write, 6, 0, do_store_boot_write, "", ""),
	U_BOOT_CMD_MKENT(boot_erase, 4, 0, do_store_boot_erase, "", ""),
	U_BOOT_CMD_MKENT(rsv, 6, 0, do_store_rsv_ops, "", ""),
	U_BOOT_CMD_MKENT(param, 2, 0, do_store_param_ops, "", ""),
	U_BOOT_CMD_MKENT(boot_copy_enable, 3, 0, do_store_boot_copy_enable, "", ""),
	U_BOOT_CMD_MKENT(ffu_op, 4, 0, do_store_ffu_op, "", ""),
};

static int do_store(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return CMD_RET_USAGE;

	c = find_cmd_tbl(argv[1], cmd_store_sub, ARRAY_SIZE(cmd_store_sub));
	if (c)
		return c->cmd(cmdtp, flag, argc, argv);

	return CMD_RET_USAGE;
}

U_BOOT_CMD(store, CONFIG_SYS_MAXARGS, 1, do_store,
	"STORE sub-system:",
	"store init [flag]\n"
	"	init storage device\n"
	"store device [name]\n"
	"	show or set storage device\n"
	"	'store device' command will list\n"
	"	all valid storage device and print.\n"
	"	'store device [name]' will set the\n"
	"	[name] device to the current device\n"
	"store partition\n"
	"	show partitions of current device\n"
	"store read addr [partition name] off size\n"
	"	read 'size' bytes from offset 'off'\n"
	"	of device/partition 'partition name' to.\n"
	"	address 'addr' of memory.\n"
	"	if partition name not value. read start with\n"
	"	offset in normal logic area,if tpl area exist\n"
	"	read offset at end of tpl area\n"
	"store write addr [partition name] off size\n"
	"	write 'size' bytes to offset 'off' of\n"
	"	device/partition [partition name] from\n"
	"	address 'addr' of memory.\n"
	"	if partition name not value. write start with\n"
	"	offset in normal logic area,if tpl area exist\n"
	"	write offset at end of tpl area\n"
	"store write_gpt addr\n"
	"   write gpt from address 'addr'\n"
	"store read_gpt addr\n"
	"   read gpt to address 'addr'\n"
	"store erase_gpt\n"
	"   erase primary and secondary gpt\n"
	"store erase partition name off size.\n"
	"	erase 'size' bytes from offset 'off'\n"
	"	of device/partition [partition name]\n"
	"	partition name must't NULL\n"
	"store scrub partition name off size.\n"
	"	erase 'size' bytes from offset 'off'\n"
	"	of device/partition [partition name]\n"
	"	includes oob area if the device has.\n"
	"	partition name must't NULL\n"
	"store erase.chip [flag]\n"
	"	erase all nand chip,except bad block\n"
	"	flag 0 erase all nand chip,except bootloader&rsv\n"
	"	flag 1 erase rsv\n"
	"store scrub.chip\n"
	"	erase all nand chip,include bad block\n"
	"store boot_read name addr copy size\n"
	"	read 'size' bytes from 'copy'th backup\n"
	"	in name partition, 'copy' can't be null.\n"
	"	name:\n"
	"	in discrete mode: 'bl2'/'tpl'(fip)\n"
	"	in compact mode: 'bootloader'\n"
	"store boot_write name addr [copy] size\n"
	"	write 'size' bytes to 'copy'th backup\n"
	"	in [name] partition from address\n"
	"	'addr' of memory. when the optional 'copy'\n"
	"	is null, it will writes to all copies\n"
	"	name:\n"
	"	in discrete mode:\n"
	"	'bl2/bl2e/bl2x/ddrfip/tpl(fip), only update part\n"
	"	'bootloader', update whole uboot.bin, in this case\n"
	"	@copy:if used, must < min(tplCpyNum, Bl2CpyNum), update only the specified copy\n"
	"	if not used, update all the copies of bl2 bl2e bl2x ddrfip tpl!\n"
	"	in compact mode: 'bootloader'\n"
	"store boot_erase name [copy]\n"
	"	erase the name info from 'copy'th backup\n"
	"	when the optional 'copy' not value, it\n"
	"	will erase all copies.\n"
	"	name:\n"
	"	in discrete mode: \n"
	"	'bl2'/'tpl'(fip): erase bl2/tpl partition\n"
	"	'bootloader':erase bl2 + tpl partition\n"
	"	in compact mode: 'bootloader'\n"
	"store rsv read name addr size\n"
	"	read 'size' bytes 'name' rsv info\n"
	"	to address 'addr' of memory\n"
	"	'name' could be key/dtb/env etc...\n"
	"store rsv write name addr size\n"
	"	write 'size' bytes 'name' rsv info\n"
	"	from address 'addr' of memory\n"
	"store rsv erase name\n"
	"	erase 'name' rsv info\n"
	"	name must't null\n"
	"store rsv protect name on/off\n"
	"	turn on/off the rsv info protection\n"
	"	name must't null\n"
	"store param\n"
	"	transfer bl2e/x ddrfip devfip size to kernel in such case like sc2\n"
	"store boot_copy_enable [boot_index]\n"
	"	check bootloader_x whether enable\n"
	"store ffu [version] [addr] [cnt]\n"
	"	update ffu fw\n"
);
