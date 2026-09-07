// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <amlogic/ramdump.h>
#include <amlogic/emmc_partitions.h>
#include <usb.h>
#include <asm/amlogic/arch/regs.h>
#include <asm/global_data.h>
#include <syscon.h>
#include <cli.h>
#include <console.h>
#include <u-boot/md5.h>
#include <image.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <asm/io.h>
#include <linux/types.h>
#include <mapmem.h>
#include <asm/amlogic/arch/timer.h>

extern struct bootm_headers images;

#define DEBUG_RAMDUMP	0
#define PAGE_SIZE                   ((1) << PAGE_SHIFT)
#define AMLOGIC_KERNEL_PANIC		0x0c
#define AMLOGIC_WATCHDOG_REBOOT		0x0d
#ifdef CONFIG_USB_STORAGE
#define _AML_MISC_INTERRUPT_KEY     0x09
static void wait_usb_dev(void);
#endif
unsigned long ramdump_base = 0;
unsigned long ramdump_size = 0;

#define MD5_DIGEST_LENGTH 16
static unsigned char md5_hex[MD5_DIGEST_LENGTH + 1] = {0};
static char md5_str1[MD5_DIGEST_LENGTH * 2 + 1] = {0};
static char md5_str2[MD5_DIGEST_LENGTH * 2 + 1] = {0};
int g_ramdump_skip_osd_lcd;

#undef CONFIG_DUMP_COMPRESS_HEAD
#ifdef CONFIG_DUMP_COMPRESS_HEAD
static void dump_info(unsigned int addr, unsigned int size, const char *info)
{
	int i = 0;

	printf("\nDUMP %s 0X%08x :", info, addr);
	for (i = 0; i < size; i += 4) {
		if (0 == (i % 32))
			printf("\n[0x%08x] ", addr + i);
		printf("%08x ", readl(addr + i));
	}
	printf("\n\n");
}
#endif

static char *md5sum_hex2str(unsigned char *in_md5_hex, char *out_md5_str)
{
	int i = 0;

	for (i = 0; i < MD5_DIGEST_LENGTH; ++i)
		sprintf(out_md5_str + i * 2, "%.2x", in_md5_hex[i]);

	out_md5_str[MD5_DIGEST_LENGTH * 2] = '\0';
	return out_md5_str;
}

static int compare_memory_regions(unsigned int *addr1,
					unsigned int *addr2,
					unsigned int total_size,
					unsigned int block_size)
{
	unsigned int row_index, column_index;
	unsigned int row_max = MD5_PER_ROW_NUM;
	unsigned int column_max, ddr_size, offset;
	unsigned int err_addr[5] = {0};
	unsigned int err_count = 0, ret = 0;
	unsigned int skip1_index_s, skip1_index_e;
	unsigned int skip2_index_s, skip2_index_e;
	unsigned int skip3_index_s;
	struct rammd5_info_t *md5_info1 = (struct rammd5_info_t *)addr1;
	struct rammd5_info_t *md5_info2 = (struct rammd5_info_t *)addr2;

	if ((memcmp((char *)addr1, MD5_MAGIC, strlen(MD5_MAGIC)) != 0) ||
		(memcmp((char *)addr2, MD5_MAGIC, strlen(MD5_MAGIC)) != 0))
		return 0;

	addr1 += sizeof(struct rammd5_info_t) / sizeof(unsigned int);
	addr2 += sizeof(struct rammd5_info_t) / sizeof(unsigned int);

	ddr_size = total_size > 0xe0000000 ? 0xe0000000 : total_size;
	column_max = ddr_size / block_size / row_max;

	/* fix up area1: bl33z reserved */
	skip1_index_s = min(md5_info1->area1_start, md5_info2->area1_start) / block_size;
	skip1_index_e = max(md5_info1->area1_end, md5_info2->area1_end) / block_size;

	/* fix up area2: bl31 and bl32 reserved */
	skip2_index_s = min(md5_info1->area2_start, md5_info2->area2_start) / block_size;
	skip2_index_e = max(md5_info1->area2_end, md5_info2->area2_end) / block_size + 1;

	/* fix up area3: ramdump compress end ~ ddr_end */
	skip3_index_s = (ramdump_base + ramdump_size) / block_size + 1;

	printf("ramdump MD5sum check %-7s vs %-7s, skip: %d~%d, %d~%d, %d~ ...",
			md5_info1->stage, md5_info2->stage, skip1_index_s, skip1_index_e,
			skip2_index_s, skip2_index_e, skip3_index_s);

	for (column_index = 0; column_index < column_max; column_index++) {
		for (row_index = 0; row_index < row_max; row_index++) {
			offset = column_index * row_max + row_index;
			if ((offset >= skip1_index_s && offset < skip1_index_e) ||
				(offset >= skip2_index_s && offset < skip2_index_e) ||
				offset >= skip3_index_s) {
				continue;
			} else {
				if (addr1[offset] != addr2[offset]) {
					if (err_count < 5)
						err_addr[err_count++] = offset * block_size;
				}
			}
		}
	}

	if (err_count == 0) {
		ret = 0;
		printf("  PASS.\n");
	} else {
		ret = -1;
		printf("  ERROR !!\n");
		printf("ramdump Polluted addr: ");
		for (unsigned int i = 0; i < err_count; i++) {
			if (err_addr[i] != 0)
				printf("0x%08x, ", err_addr[i]);
		}
		printf(" ...\n");

		for (column_index = 0; column_index < column_max; column_index++) {
			printf("[0x%8x]:", column_index * row_max * block_size);

			for (row_index = 0; row_index < row_max; row_index++) {
				offset = column_index * row_max + row_index;

				if ((row_index % 8) == 0)
					printf(" ");
				if (addr1[offset] == addr2[offset])
					printf("- ");
				else
					printf("* ");
			}
			printf("\n");
		}
	}

	return ret;
}

static int ramdump_save_ddr_md5_info(unsigned long ddr_size,
						unsigned int block_size,
						unsigned int *store_addr,
						char *stage_info)
{
	unsigned int i, j;
	unsigned int total_size, blk_uint_num;
	unsigned int *src_addr = (unsigned int *)0x0;
	unsigned long sum = 0;
	uintptr_t addr_ptr;
	unsigned int skip1_start, skip2_start;
	unsigned int skip1_end, skip2_end;
	struct rammd5_info_t *md5_info;

	/* Max check size is 3.5GB */
	total_size = ddr_size > 0xe0000000 ? 0xe0000000 : ddr_size;

	/* set ddr md5 skip area */
#ifdef MD5_SKIP_UBOOT_END
	skip1_start	= MD5_SKIP_UBOOT_START;
	skip1_end	= MD5_SKIP_UBOOT_END;
#else
	skip1_start	= 0;
	skip1_end	= 0x01800000;
#endif

#ifdef REG_RSVMEM_SIZE
	unsigned int data = 0;

	/* bl31/32 rsvmem start */
	skip2_start = readl(REG_MDUMP_RSVMEM_BL31_START);
	skip2_end = readl(REG_MDUMP_RSVMEM_BL32_START);

	/* bl32_start + bl32_rsvmem_size = skip2_end */
	data = readl(REG_RSVMEM_SIZE);
	if ((data >> 16) & 0xff)
		skip2_end +=  (data & 0x0000ffff) << 16;
	else
		skip2_end +=  (data & 0x0000ffff) << 10;

	/* + bl32 stack reserved 1MB */
	skip2_end += (1 << 20);
#else
	printf("ramdump md5 use default bl32 size.\n");
	skip2_start	= 0x05000000;
	skip2_end	= 0x08400000;
#endif
	printf("ramdump md5sum skip: 0x%08x~0x%08x, 0x%08x~0x%08x\n",
			skip1_start, skip1_end, skip2_start, skip2_end);

	md5_info = (struct rammd5_info_t *)store_addr;
	memcpy(md5_info->magic, MD5_MAGIC, sizeof(MD5_MAGIC));
	memcpy(md5_info->stage, stage_info, sizeof(md5_info->stage));
	md5_info->block_size = block_size;
	md5_info->ddr_size = ddr_size;
	md5_info->area1_start = skip1_start;
	md5_info->area1_end = skip1_end;
	md5_info->area2_start = skip2_start;
	md5_info->area2_end = skip2_end;

	store_addr += sizeof(struct rammd5_info_t) / sizeof(unsigned int);
	printf("ramdump md5sum total=0x%x, block=%x, store_addr=0x%lx\n",
			total_size, block_size, (unsigned long)store_addr);
	blk_uint_num = block_size / sizeof(unsigned int);
	for (i = 0; i < (total_size / block_size - 1); i++) {
		for (j = 0, sum = 0; j < blk_uint_num; j++) {
			addr_ptr = (uintptr_t)&src_addr[i * blk_uint_num + j];
			if ((addr_ptr >= skip1_start && addr_ptr < skip1_end) ||
				(addr_ptr >= skip2_start && addr_ptr < skip2_end))
				sum += 0;
			else
				sum += src_addr[i * blk_uint_num + j];
		}
		*store_addr = (unsigned int)(sum & 0xFFFFFFFF);
		store_addr++;
	}

	return 0;
}

static int ramdump_check_ddr_md5_info(void)
{
	int ret1 = 0, ret2 = 0, ret3 = 0;
	ulong ddr_size = ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xfffffUL) << 4) >
	0xe0000000 ? 0xe0000000 : ((readl(SYSCTRL_SEC_STATUS_REG4) & ~0xfffffUL) << 4);

	ramdump_save_ddr_md5_info(ddr_size, MD5_BLOCK_SIZE,
					(unsigned int *)MD5_BL33X_1_BASE_ADDR, "BL33X");

	/* compare_memory_regions BL2E1 vs BL2E2 */
	ret1 = compare_memory_regions((unsigned int *)MD5_BL2E_1_BASE_ADDR,
				(unsigned int *)MD5_BL2E_2_BASE_ADDR, ddr_size, MD5_BLOCK_SIZE);

	/* compare_memory_regions BL2E2 vs BL33Z-1 */
	ret2 = compare_memory_regions((unsigned int *)MD5_BL2E_2_BASE_ADDR,
				(unsigned int *)MD5_BL33Z_1_BASE_ADDR, ddr_size, MD5_BLOCK_SIZE);

	/* compare_memory_regions BL33Z-2 vs BL33x */
	ret3 = compare_memory_regions((unsigned int *)MD5_BL33Z_2_BASE_ADDR,
				(unsigned int *)MD5_BL33X_1_BASE_ADDR, ddr_size, MD5_BLOCK_SIZE);

	if (ret1 < 0 || ret2 < 0 || ret3 < 0)
		return -1;
	else
		return 0;
}

unsigned int get_reboot_mode(void)
{
	uint32_t reboot_mode_val = ((readl(REG_MDUMP_REBOOT_MODE) >> 12) & 0xf);
	return reboot_mode_val;
}

void ramdump_clean_ddr(void)
{
	const ulong skip_start = RAMDUMP_SKIP_BL31_START;
	const ulong skip_end = RAMDUMP_SKIP_BL32_END;
	ulong start = RAMDUMP_COMPRESS_START;
	ulong end, uboot_start;
	ulong uboot_stack_size = CONFIG_STACK_SIZE;
	char *env;
	ulong tick;

	/* clean_end_addr = uboot_start - 16MB */
	uboot_start = ((uintptr_t)map_sysmem(gd->start_addr_sp, 0) -
		       uboot_stack_size) & ~EFI_PAGE_MASK;
	end = uboot_start - RAMDUMP_ISOLATION_SIZE;

	/* Retrieve cleaning start address from environment variable if set */
	env = env_get("ramdump_clean_start");
	if (env) {
		start = simple_strtoul(env, NULL, 16);
		printf("%s update start addr: 0x%08lx\n", __func__, start);
	}

	/* Retrieve cleaning end address from environment variable if set */
	env = env_get("ramdump_clean_end");
	if (env) {
		end = simple_strtoul(env, NULL, 16);
		printf("%s update end addr: 0x%08lx\n", __func__, end);
	}

	/* Check if end address exceeds gd->ram_top */
	if (end > gd->ram_top) {
		printf("%s err, end 0x%08lx > ram_top 0x%08llx\n", __func__, end, gd->ram_top);
		return;
	}

	/* Verify that start is less than end */
	if (start >= end) {
		printf("%s err, start (0x%08lx) >= end (0x%08lx)\n", __func__, start, end);
		return;
	}

	printf("ramdump cleaning DDR region: 0x%08lx ~ 0x%08lx\n", start, end);

	tick = get_time();
	/* clean full ddr, skipping the specified region (bl31/bl32) */
	memset((void *)start, 0, skip_start - start);
	memset((void *)skip_end, 0, end - skip_end);
	tick = get_time() - tick;
	printf("ramdump cleaning DDR done. took %lu ms.\n", tick / 1000);
}

void update_ramdump_base_addr(void)
{
	ulong store_addr = 0;
	const char *env;

	/* 1. Runtime configuration has the highest priority */
	env = env_get("ramdump_store_addr");
	if (env) {
		store_addr = simple_strtoul(env, NULL, 16);
		printf("%s: use env store_addr=0x%lx\n",
		       __func__, store_addr);
	}

	/* 2. Fallback to build-time default address */
#if defined(CONFIG_COMPRESSED_ADDR)
	if (!store_addr) {
		store_addr = CONFIG_COMPRESSED_ADDR;
		printf("%s: use CONFIG_COMPRESSED_ADDR=0x%lx\n",
		       __func__, store_addr);
	}
#endif

	/* 3. No relocation requested */
	if (!store_addr)
		return;

	printf("%s: original ramdump_base=0x%lx size=0x%lx\n",
	       __func__, ramdump_base, ramdump_size);

	/* 4. Relocate ramdump data if compression is disabled */
#ifndef CONFIG_MDUMP_COMPRESS
	if (ramdump_base != store_addr && ramdump_size) {
		printf("%s: moving ramdump 0x%lx -> 0x%lx (size=0x%lx)\n",
		       __func__, ramdump_base,
		       store_addr, ramdump_size);

		memmove((void *)store_addr,
			(void *)ramdump_base,
			ramdump_size);
	}
#endif

	/* 5. Update ramdump base address */
	ramdump_base = store_addr;
	printf("%s: ramdump_base updated to 0x%lx\n",
	       __func__, ramdump_base);
}

void ramdump_init(void)
{
	unsigned int data, reboot_mode;
	char *env;

	debug("%s, base reg:0x%08x, size reg:0x%08x\n", __func__,
				REG_MDUMP_COMPRESS_BASE, REG_MDUMP_COMPRESS_SIZE);
	ramdump_base = readl(REG_MDUMP_COMPRESS_BASE);
	ramdump_size = readl(REG_MDUMP_COMPRESS_SIZE);
	if (ramdump_base && ramdump_size && !(ramdump_base & 0x80)) {
		if (ramdump_check_ddr_md5_info() < 0)
			printf("%s, The core-dump is polluted !!!\n\n", __func__);
	}

	update_ramdump_base_addr();

	if (ramdump_base & 0x80) {
		/* 0x80: The flag indicates that the addr exceeds 4G. */
		/* real size = size[31:0] + addr[6:0]<<32 */
		ramdump_size += (ramdump_base & 0x7f) << 32;
		/* real addr = addr[31:8] << 8 */
		ramdump_base = (ramdump_base & 0xffffff00) << 8;
	}

	env = env_get("ramdump_enable");
	if (env && !strcmp(env, "1") && !ramdump_base && !ramdump_size) {
		env = env_get("ramdump_clean_skip");
		if (!env || strcmp(env, "1"))
			ramdump_clean_ddr();
	}

	data = readl(REG_MDUMP_CPUBOOT_STATUS);
	writel(data & ~RAMDUMP_STICKY_DATA_MASK, REG_MDUMP_CPUBOOT_STATUS);
	printf("%s, add:%lx, size:%lx\n", __func__, ramdump_base, ramdump_size);

#ifdef CONFIG_DUMP_COMPRESS_HEAD
	dump_info((unsigned int)ramdump_base, 0x80, "bl33 check COMPRESS DATA 1");
#endif

	reboot_mode = get_reboot_mode();
	if ((reboot_mode == AMLOGIC_WATCHDOG_REBOOT ||
			reboot_mode == AMLOGIC_KERNEL_PANIC)) {
		if (ramdump_base && ramdump_size) {
			ramdump_save_compress_data();
		}
	}
}

#ifdef CONFIG_USB_STORAGE
static int wait_for_interrupt_key(const int tm/*timeout in ms*/)
{
	int i;
	unsigned long ts;
	char str_input[5];

	ts = get_timer(0);
	for (i = 0; i < sizeof(str_input) - 1;) {
		int c = 0;

		while (!tstc()) {
			if (get_timer(ts) >= tm)
				return 0;
		}
		c = getchar();
		if (i == 0 && c == _AML_MISC_INTERRUPT_KEY) {//drop first duplicated ctrlI
			printf("Wait y/yes input\n");
			continue;
		}
		putc(c);
		str_input[i++] = c;
		if (c == '\r')
			break;
	}

	str_input[i] = '\0';
	putc('\n');
	if (strcasecmp(str_input, "y\r") == 0 ||
		strcasecmp(str_input, "yes\r") == 0) {
		cli_init();
		cli_loop();
		panic("No CLI available");
	}

	return 0;
}

static void wait_usb_dev(void)
{
	int print_cnt = 0, ret;
	const char *env = env_get("ramdump_usb_timeout");
	int timeout_minutes = env ? simple_strtoul(env, NULL, 10) : 0;
	int max_attempts = (timeout_minutes > 0) ? (timeout_minutes * 60 / 20) : 0;

	while (1) {
		run_command("usb start", 1);
		mdelay(2000);
		run_command("usb reset", 1);
		ret = usb_stor_scan(1);
		if (ret) {
			print_cnt++;
			if (max_attempts > 0 && print_cnt > max_attempts) {
				printf("ramdump: Udisk not found after %d minutes. Exiting...\n",
					timeout_minutes);
				return;
			}
			printf("ramdump: can't find USB device, attempt %d.", print_cnt);
			printf(" Please insert FAT32 Udisk!\n");
			printf("Or you can input 'Ctrl I' to force stopped anyway ...\n\n\n");
			wait_for_interrupt_key(18000);
		} else {
			run_command("usb dev", 1);
			break;
		}
	}
}
#endif
/*
 * NOTE: this is a default implementation for writing compressed ramdump data
 * to /data/ partition for Android platform. You can read out dumpfile in
 * path /data/crashdump-1.bin when enter Android for crash analyze.
 * by default, /data/ partition for android is EXT4 fs.
 *
 * TODO:
 *    If you are using different fs or OS on your platform, implement compress
 *    data save command for your fs and OS in your board.c with same function
 *    name "ramdump_save_compress_data".
 */
__weak int ramdump_save_compress_data(void)
{
	char *env;
	int ret = 0;
#ifdef CONFIG_USB_STORAGE
	char cmd[128] = {0};
	char path[64] = "/";
	int cnt = 0;
#endif

	env = env_get("ramdump_enable");
	if (!env || strcmp(env, "1") != 0)
		return 0;

	env = env_get("ramdump_location");
	if (!env)
		return 0;

	/* calculate ramdump md5sum */
	if (ramdump_base < (1UL << 32)) {
		md5((unsigned char *)ramdump_base, ramdump_size, md5_hex);
		md5_hex[MD5_DIGEST_LENGTH] = '\0';
		md5sum_hex2str(md5_hex, md5_str1);
		printf("ramdump: get md5sum: %s\n", md5_str1);
	} else {
		memcpy(md5_str1, (void *)MD5_BL33Z_RAMDUMP_STR, MD5_DIGEST_LENGTH * 2 + 1);
		printf("ramdump: addr 0x%lx, use bl33z_2 md5sum: %s\n", ramdump_base, md5_str1);
	}

	printf("ramdump_location:%s\n", env);
	if (!strncmp(env, "data", 4)) {
		env = env_get("ramdump_jump");
		if (!env || strcmp(env, "1") != 0) {
			printf("ramdump: save to Android /data/.\n");
			g_ramdump_skip_osd_lcd = 1;
			printf("ramdump: Set the flag skip_osd_lcd.\n");
			return 0;
		}
		printf("ramdump: jump directly to main_loop.\n");
		run_command("run storeargs;run storeboot", 1);
		printf("ramdump: run storeboot Error! Rebooting in 5 seconds ...\n\n");
		mdelay(5000);
		run_command("reboot", 1);
	} else if (!strncmp(env, "usb", 3)) {
		printf("ramdump: save to Udisk. (cmd: fatwrite usb 0 %lx /crashdump-1.bin %lx\n",
				ramdump_base, ramdump_size);
	} else {
		printf("Error: not supported location: %s\n", env);
		return 0;
	}
#ifdef CONFIG_USB_STORAGE
	wait_usb_dev();

	env = env_get("ramdump_usb_path");
	if (env) {
		printf("ramdump_usb_path: %s\n", env);
		snprintf(cmd, sizeof(cmd), "fatls usb 0 %s/", env);
		ret = run_command(cmd, 0);
		if (ret != 0)
			printf("USB path '%s' does not exist, using default '/'.\n", env);
		else
			snprintf(path, sizeof(path), "%s/", env);
	}
	printf("\nPrepare to save crash file: base=0x%08lx, size=0x%08lx (%ld MB)\n",
		ramdump_base, ramdump_size, ramdump_size / 1024 / 1024);
	snprintf(cmd, sizeof(cmd), "fatwrite usb 0 %lx %scrashdump-1.bin %lx",
		ramdump_base, path, ramdump_size);
	printf("\nCMD: %s\n\n", cmd);

	printf("It may take about 3 minutes, please wait...\n");
	while (cnt < 3) {
		ret = run_command(cmd, 0);
		if (ret == 0) {
			run_command("usb stop", 0);
			printf("run fatwrite usb OK! (%scrashdump-1.bin, md5sum=0x%s)\n",
					path, md5_str1);
			printf("Please check file md5sum on PC with: md5sum ./crashdump-1.bin\n\n");
			break;
		}
		printf("run fatwrite usb ERROR! Try again...\n");
		cnt++;
		run_command("usb stop", 0);
		mdelay(1000);
		run_command("usb start", 1);
		mdelay(2000);
		run_command("usb reset", 1);
		run_command("usb dev", 1);
		mdelay(1000);
	}
	if (cnt >= 3) {
		printf("\n======================= fatwrite ERROR! (try 3 times) =============\n");
		printf("=  1. Please check Udisk! (FAT32 type, and enough space.)\n");
		printf("=  2. Try another Udisk! (Press Ctrl+i, then manually run fatwrite.)\n");
		printf("====================================================================\n\n");
	}

	env = env_get("ramdump_usb_reboot");
	if (env && strcmp(env, "1") == 0) {
		printf("ramdump_usb_reboot: %s\n", env);
		printf("Rebooting in 5 seconds ...\n\n\n");
		mdelay(5000);
		run_command("reboot", 1);
	}

	printf("\nRun fatwrite finished. Stop here and wait for manual reboot!\n");
	printf("\nAnd You can input 'Ctrl I' to enter the U-Boot console...\n\n\n");
	while (1) {
		mdelay(1000);
		wait_for_interrupt_key(100000);
	}
#else
	printf("CONFIG_USB_STORAGE is not defined! Could it be in PRODUCT mode?\n");
	printf("ERROR: The usb drv is not available!\n");
#endif

	return ret;
}

static void ramdump_env_setup(unsigned long addr, unsigned long size)
{
	unsigned int data[10] = {
		0x8E9C929F, 0x9E9C9791,
		0xD28C9191, 0x97949B8D,
		0x888B92,   0xCEBB97,
		0x938E9B90, 0x978D8D97,
		0xC8009B8A, 0xB99CDB
	};
	char *line, *o;
	unsigned char *p;
	int i;

	p = (unsigned char *)data;
	for (i = 0; i < 40; i++)
		p[i] = ~(p[i] - 1);

	/*
	 * TODO: Make sure address for fdt_high and initrd_high
	 * are suitable for all boards
	 *
	 * usually kernel load address is 0x010800000
	 * Make sure:
	 * (kernel image size + ramdisk size) <
	 * (initrd_high - 0x010800000)
	 * dts file size < (fdt_high - initrd_high)
	 */
	//env_set("initrd_high", "0x0BB00000");
	//env_set("fdt_high",    "0x0BE00000");
	line = env_get("bootargs");
	if (!line)
		return;

	i = strlen(line);
	o = malloc(i + 256);
	if (!o)
		return;

	memset(o, 0, i + 256);
	snprintf(o, i + 256, "%s=%s ramdump=%lx,%lx androidboot.ramdumpmd5=%s %s\n",
		(char *)data, (char *)(data + 6), addr, size, md5_str1, line);
	env_set("bootargs", o);
	free(o);
	line = NULL;

#if DEBUG_RAMDUMP
	run_command("printenv bootargs", 1);
	printf("\n");
#endif
}

static int ramdump_check_kernel_dts_rsvmem(u64 addr, u64 size)
{
	int count;
	void *fdt = images.ft_addr;
	int node, subnode;
	int addr_cells, size_cells;
	const fdt32_t *prop;
	int len, i, conflict_count = 0;
	const char *status;
	const char *name;
	u64 r_addr, r_size, alloc_size;
	fdt32_t zero_reg[4] = {0};
	fdt32_t new_size[2];
	u64 max_cma = 48 * 1024 * 1024;

	printf("\n[ramdump rsvmem] check kernel dts conflict for [0x%llx ~ 0x%llx]\n",
		addr, addr + size);

	node = fdt_path_offset(fdt, "/aml_frc");
	if (node >= 0) {
		status = fdt_getprop(fdt, node, "status", NULL);
		if (!status || strcmp(status, "disabled")) {
			printf("[ramdump rsvmem] Disable Node: /aml_frc\n");
			fdt_setprop_string(fdt, node, "status", "disabled");
		}
	}

	node = fdt_path_offset(fdt, "/reserved-memory");
	if (node < 0) {
		printf("[ramdump rsvmem] No /reserved-memory node found.\n");
		return -1;
	}

	addr_cells = fdt_address_cells(fdt, node);
	if (addr_cells < 0)
		addr_cells = fdt_address_cells(fdt, 0);

	size_cells = fdt_size_cells(fdt, node);
	if (size_cells < 0)
		size_cells = fdt_size_cells(fdt, 0);

	if (size_cells == 2) {
		new_size[0] = cpu_to_fdt32((u32)(max_cma >> 32));
		new_size[1] = cpu_to_fdt32((u32)(max_cma & 0xFFFFFFFF));
	} else {
		new_size[0] = cpu_to_fdt32((u32)(max_cma));
	}

	fdt_for_each_subnode(subnode, fdt, node) {
		name = fdt_get_name(fdt, subnode, NULL);
		status = fdt_getprop(fdt, subnode, "status", NULL);
		if (status && !strcmp(status, "disabled"))
			continue;

		// reg
		prop = fdt_getprop(fdt, subnode, "reg", &len);
		if (prop && len >= (addr_cells + size_cells) * sizeof(fdt32_t)) {
			count = len / ((addr_cells + size_cells) * sizeof(fdt32_t));
			const fdt32_t *p = prop;

			for (i = 0; i < count; i++) {
				r_addr = fdt_read_number(p, addr_cells);
				p += addr_cells;
				r_size = fdt_read_number(p, size_cells);
				p += size_cells;
				if (!r_size)
					break;

				if ((addr + size > r_addr) && (addr < r_addr + r_size)) {
					conflict_count++;
					printf("[ramdump rsvmem] Conflict Node:%s", name);
					printf("  Type:reg  [0x%08llx ~ 0x%08llx] -> 0MB\n",
						r_addr, r_addr + r_size);
					fdt_setprop(fdt, subnode, "reg", zero_reg,
						(addr_cells + size_cells) * sizeof(fdt32_t));
				}
			}
			continue;
		}

		// alloc-ranges
		prop = fdt_getprop(fdt, subnode, "alloc-ranges", &len);
		if (prop && len >= (addr_cells + size_cells) * sizeof(fdt32_t)) {
			count = len / ((addr_cells + size_cells) * sizeof(fdt32_t));
			const fdt32_t *p = prop;

			for (i = 0; i < count; i++) {
				r_addr = fdt_read_number(p, addr_cells);
				p += addr_cells;
				r_size = fdt_read_number(p, size_cells);
				p += size_cells;
				if (!r_size)
					break;

				prop = fdt_getprop(fdt, subnode, "size", &len);
				if (!prop)
					break;

				alloc_size = fdt_read_number(prop, size_cells);
				if (!alloc_size)
					break;

				if ((addr + size > (r_addr + r_size - alloc_size)) &&
					(addr < r_addr + r_size)) {
					conflict_count++;
					printf("[ramdump rsvmem] Conflict Node:%s", name);
					printf("  Type:alloc  [0x%08llx ~ 0x%08llx] %lldMB -> 0MB\n",
						r_addr, r_addr + r_size, alloc_size / 1024 / 1024);
					fdt_setprop(fdt, subnode, "size", zero_reg,
						size_cells * sizeof(fdt32_t));
				} else if (alloc_size > max_cma) {
					printf("[ramdump rsvmem] Adjust Node:%s", name);
					printf("  Type:CMApool  size:%lldMB -> 48MB\n",
							alloc_size / 1024 / 1024);
					fdt_setprop(fdt, subnode, "size", new_size,
							size_cells * sizeof(fdt32_t));
				}
			}
			continue;
		}

		// CMA pool
		prop = fdt_getprop(fdt, subnode, "size", &len);
		if (prop && len >= size_cells * sizeof(fdt32_t)) {
			count = len / (size_cells * sizeof(fdt32_t));
			const fdt32_t *p = prop;

			for (i = 0; i < count; i++) {
				r_size = fdt_read_number(p, size_cells);
				p += size_cells;
				if (!r_size)
					break;

				if (r_size > max_cma) {
					printf("[ramdump rsvmem] Adjust Node:%s", name);
					printf("  Type:CMApool  size:%lldMB -> 48MB\n",
						r_size / 1024 / 1024);
					fdt_setprop(fdt, subnode, "size", new_size,
						size_cells * sizeof(fdt32_t));
				}
			}
			continue;
		}
	}

	if (conflict_count > 0) {
		printf("[ramdump rsvmem] Found %d conflict(s) dts node. (Modified)\n\n",
			conflict_count);
		return -1;
	}

	printf("[ramdump rsvmem] Check OK. No conflicts found.\n\n");
	return 0;
}

static int overwrite_bl33z_rsvmem_info(unsigned long addr, unsigned long size)
{
	int address_cells = 0;
	unsigned long align_size = PAGE_ALIGN(size);
	char cmd[0x80];
	u32 addr_hi, addr_lo;
	u32 size_hi, size_lo;

	address_cells = fdt_address_cells(images.ft_addr, 0);
	if (address_cells < 1) {
		printf("%s, bad #address-cells !\n", __func__);
		return -1;
	}
	printf("%s, get fdt #address-cells = %d\n", __func__, address_cells);

	addr_hi = upper_32_bits(addr);
	addr_lo = lower_32_bits(addr);
	size_hi = upper_32_bits(align_size);
	size_lo = lower_32_bits(align_size);

	memset(cmd, 0, sizeof(cmd));
	if (address_cells == 2) {
		printf("fdt set /reserved-memory/ramdump_bl33z status okay\n");
		run_command("fdt set /reserved-memory/ramdump_bl33z status okay", 0);
		snprintf(cmd, sizeof(cmd),
			"fdt set /reserved-memory/ramdump_bl33z reg '<0x%08x 0x%08x 0x%08x 0x%08x>'",
			addr_hi, addr_lo, size_hi, size_lo);
	} else {
		printf("fdt set /reserved-memory/ramdump_data status okay\n");
		run_command("fdt set /reserved-memory/ramdump_data status okay", 0);
		snprintf(cmd, sizeof(cmd),
			"fdt set /reserved-memory/ramdump_data reg '<0x%08lx 0x%08lx>'",
			addr, align_size);
	}

	printf("%s\n\n", cmd);
	run_command(cmd, 0);

	return 0;
}

void check_ramdump(void)
{
	unsigned long addr = 0;
	unsigned long size = 0;
	char *env;
	int reboot_mode;

	env = env_get("ramdump_enable");
	printf("%s, ramdump_enable = %s\n", __func__, env);
	if (!env || strcmp(env, "1"))
		return;

	reboot_mode = get_reboot_mode();
	if (reboot_mode != AMLOGIC_WATCHDOG_REBOOT &&
	    reboot_mode != AMLOGIC_KERNEL_PANIC) {
		ramdump_env_setup(0, 0);
		if (!IS_ENABLED(CONFIG_MDUMP_COMPRESS)) {
			printf("%s, fdt: rsvmem ramdump_bl33z enable.\n", __func__);
			run_command("fdt set /reserved-memory/ramdump_bl33z status okay", 0);
		}
		return;
	}

	addr = ramdump_base;
	size = ramdump_size;
	printf("%s, addr: 0x%lx, size: 0x%lx\n", __func__, addr, size);
	if (!addr || !size) {
		ramdump_env_setup(0, 0);
		return;
	}

	/* calculate ramdump md5sum */
	if (addr < (1UL << 32)) {
		md5((unsigned char *)addr, size, md5_hex);
		md5_hex[MD5_DIGEST_LENGTH] = '\0';
		md5sum_hex2str(md5_hex, md5_str2);
		printf("ramdump: get md5sum: %s\n", md5_str2);
	} else {
		memcpy(md5_str2, (void *)MD5_BL33Z_RAMDUMP_STR,
		       MD5_DIGEST_LENGTH * 2 + 1);
		printf("ramdump: addr 0x%lx exceed 4G, use bl33z_2 md5sum: %s\n",
		       addr, md5_str2);
	}

	if (strcmp(md5_str1, md5_str2)) {
		printf("=====================================\n");
		printf("ramdump md5sum check err! Rebooting ...\n");
		printf("=====================================\n\n");
		mdelay(5000);
		run_command("reboot", 1);
		return;
	}

	printf("ramdump md5sum check OK. init vs bootm.\n");
	ramdump_env_setup(addr, size);

	env = env_get("ramdump_location");
	if (env && !strncmp(env, "data", 4)) {
		printf("Crash file will save to Android /data.\n");
		ramdump_check_kernel_dts_rsvmem(addr, size);
		overwrite_bl33z_rsvmem_info(addr, size);
		env_set("initrd_high", "0x0BB00000");
		env_set("fdt_high",    "0x0BE00000");
	}
}
