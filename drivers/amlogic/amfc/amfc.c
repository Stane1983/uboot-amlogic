// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/types.h>
#include <asm/io.h>
#include <stdio.h>
#include <common.h>
#include <malloc.h>
#include <amlogic/amfc_regs.h>
#include <amlogic/amfc.h>
#include <command.h>
#include <time.h>
#include <amlogic/cpu_id.h>
#include <asm/amlogic/arch/register.h>

#define CONFIG_AMFC_TEST	1

#ifdef CONFIG_AMFC_TEST
#include <linux/zstd.h>
#include <abuf.h>

#define TABLE_SRC		0
#define TABLE_DST		1

static unsigned long total_sw_compressed_size;
static unsigned long total_hw_compressed_size;
static unsigned long total_hw_compress_time;
static unsigned long total_hw_decompress_time;
static unsigned long total_hw_self_decompress_time;
bool self;
int table_test_mask;
#endif

int amfc_init(void)
{
	unsigned int value;
	cpu_id_t cpu_id = get_cpu_id();

	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_S7D && cpu_id.chip_rev == 0x0A)
		writel(0 | (1 << 6) | (5 << 7), CLKCTRL_AMFC_CLK_CTRL);	// 500MHz
	else
		writel(0 | (1 << 6) | (4 << 7), CLKCTRL_AMFC_CLK_CTRL);	// 666MHz

	printf("AMFC VLSI version:%x, feature:%x\n",
		readl(AMFC_GL_VERSION), readl(AMFC_GL_CMD1_FEATURE));

	writel(0, AMFC_GL_CMD0_CONTROL);
	writel(0, AMFC_GL_CMD1_CONTROL);
	writel(3, AMFC_GL_CMD0_IRQCLR);
	writel(3, AMFC_GL_CMD1_IRQCLR);
	writel(0, AMFC_GL_MISC);		// cmd0/1 mixed mode
	writel((2 << 6) | (2 << 4) | (1 << 1) | (1), AMFC_CODEC_CTRL);
	writel(1, AMFC_ZSTD_MODE_MISC);

	value = readl(AMFC_WR_MIF_CTRL);
	value &= ~(0x7 << 8);
	value |=  (2 << 8);
	writel(value, AMFC_WR_MIF_CTRL);

	value = readl(AMFC_RD_MIF_CTRL);
	value &= ~(0x7 << 8);
	value |=  (2 << 8);
	writel(value, AMFC_RD_MIF_CTRL);

	return 0;
}

static struct amfc_cmd_list acl  __attribute__((aligned(64)));
static int page_table_mode = 0;
static int log_en = 0;

static void dump_addr(void *buf, unsigned int size)
{
        int i;
        unsigned int *p = (unsigned int *)buf;

        printf("%s addr:%px, size:%d\n", __func__, buf, size);
        for (i = 0; i < size / 4; i += 4) {
                printf("%px: %08x %08x %08x %08x\n",
                        p + i, p[i], p[i + 1], p[i + 2], p[i + 3]);
        }
}

static int build_tables(unsigned int *table, unsigned long base, ssize_t size, int type)
{
	int i, count, need_reverse = 0;
	unsigned long tmp;

	if (!table || size <= PAGE_SIZE)
		return -1;

	if (base & ~PAGE_MASK) {
		printf("%s, base:%lx not align to %d\n",
			__func__, base, PAGE_SIZE);
		return -1;
	}
	/* physical contiguous in uboot */
	count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;
	/*
	 * type and page_table_mode for test
	 * page_table_mode = 1 --> src/dst all sequence table
	 * page_table_mode = 2 --> src sequence/dst reverse
	 * page_table_mode = 3 --> src reverse/dst sequence
	 * page_table_mode = 4 --> src/dst all reverse table
	 * page_table_mode = 5 --> dst sequence table/src not use page_table,
	 *                          for EROFS test
	 */
	if ((page_table_mode == 2 && type == TABLE_DST) ||
	    (page_table_mode == 3 && type == TABLE_SRC) ||
	    (page_table_mode == 4))
		need_reverse = 1;

	if (need_reverse)
		base += (count - 1) * PAGE_SIZE;

	for (i = 0; i < count; i++) {
		tmp = ((base >> PAGE_SHIFT) << 6) | 1;
		table[i] = tmp;
		if (need_reverse)
			base -= PAGE_SIZE;
		else
			base += PAGE_SIZE;
	}
	flush_dcache_range((unsigned long)table,
			   (unsigned long)table + count * 4);
	return 0;
}

#define ADDR_SRC		0
#define ADDR_DST		1

static void set_up_addr(struct amfc_cmd_list *acl, unsigned long addr, int type)
{
	unsigned int low, high = 0;

	low  = addr & 0xffffffff;
	high = (addr >> 32) & 0xf;

	switch (type) {
	case ADDR_SRC:
		acl->src_addr = low;
		acl->control |= (high << 20);
		break;

	case ADDR_DST:
		acl->dst_addr = low;
		acl->control |= (high << 16);
		break;

	default:
		break;
	}
}

/*
 * src: source buffer that need decompress
 * dst: destination buffer that need store
 * src_size: size of source buffer
 * dst_size: size for destination buffer, caller should make sure
 *           dst size is enough
 * return value: decompressed size if success, negative value if failed
 */
int amfc_decompress(void *src, void *dst, ssize_t src_size, ssize_t dst_size)
{
	unsigned int *src_table = NULL, *dst_table = NULL, *tmp;
	int table_size;
	unsigned int status;
	int cmd_time;
	int timeout = ((src_size + PAGE_SIZE) / PAGE_SIZE) * 100 + 5000;
	unsigned long inv_end = (unsigned long)dst + dst_size;
	unsigned long secmon_start = readl(SYSCTRL_SEC_STATUS_REG17) + (1 << 20);

	if (log_en)
		printf("%s, acl:%p, src:%p, dst:%p, src size:%ld, dst size:%ld\n",
			__func__, &acl, src, dst, src_size, dst_size);
	/* setup command list */
	memset(&acl, 0, sizeof(acl));
	acl.src_size = src_size;
	acl.dst_size = dst_size;
	if (page_table_mode && src_size > PAGE_SIZE) {
		table_size = sizeof(int) * (src_size / PAGE_SIZE) +
			     PAGE_SIZE * 2;
		src_table = malloc(table_size);
		if (!src_table)
			return -ENOMEM;

		tmp = (unsigned int *)(((unsigned long)src_table + PAGE_SIZE) & PAGE_MASK);
		build_tables(tmp, (unsigned long)src, src_size, TABLE_SRC);
		set_up_addr(&acl, virt_to_phys(tmp), ADDR_SRC);
		acl.src_scatter = 1;
		if (log_en)
			printf("%s, table_size:%d, src_table:%p, tmp:%p\n",
				__func__, table_size, src_table, tmp);
	} else {
		set_up_addr(&acl, virt_to_phys(src), ADDR_SRC);
	}

	if (page_table_mode && dst_size > PAGE_SIZE) {
		table_size = sizeof(int) * (dst_size / PAGE_SIZE) +
			     PAGE_SIZE * 2;
		dst_table = malloc(table_size);
		if (!dst_table)
			return -ENOMEM;

		tmp = (unsigned int *)(((unsigned long)dst_table + PAGE_SIZE) & PAGE_MASK);
		build_tables(tmp, (unsigned long)dst, dst_size, TABLE_DST);
		set_up_addr(&acl, virt_to_phys(tmp), ADDR_DST);
		acl.dst_scatter = 1;
		if (log_en)
			printf("%s, table_size:%d, dst_table:%p, tmp:%p\n",
				__func__, table_size, dst_table, tmp);
	} else {
		set_up_addr(&acl, virt_to_phys(dst), ADDR_DST);
	}
	acl.algorithm = ALGORITHM_ZSTD;
	acl.end       = 1;
	acl.compress  = CMD_DECOMPRESS;
	acl.owner     = 1;
	acl.status    = 0xffffffff;

	flush_dcache_range((unsigned long)src, (unsigned long)src + src_size);
	flush_dcache_range((unsigned long)&acl, (unsigned long)&acl + sizeof(acl));

	if (inv_end > secmon_start && (unsigned long)dst < secmon_start)
		inv_end = secmon_start;

	invalidate_dcache_range((unsigned long)dst, inv_end);

	/* sw reset */
	writel(0x80000000, AMFC_GL_CMD1_CONTROL);
	writel(0x03, AMFC_GL_CMD1_IRQCLR);
	writel((unsigned long)&acl >> ADDR_SHIFT, AMFC_GL_CMD1_DESC_BASE0_ADDR);
	/* trigger */
	writel(1, AMFC_GL_CMD1_CONTROL);

	cmd_time = get_time();
	/* wait for done or error */
	while (1) {
		status = readl(AMFC_GL_CMD1_STATUS);
		if ((status & IRQ_MASK))
			break;
		if (get_time() - cmd_time > timeout) {
			printf("time out\n");
			break;
		}
	}
	cmd_time = get_time() - cmd_time;

	if (src_table)
		free(src_table);
	if (dst_table)
		free(dst_table);

	status = readl(AMFC_GL_CMD1_STATUS);
	if (!(status & AMFC_ERROR_MASK)) {
	#ifdef CONFIG_AMFC_TEST
		if (self)
			total_hw_self_decompress_time += readl(AMFC_CMD1_TIME_MEASURE);
		else
			total_hw_decompress_time += readl(AMFC_CMD1_TIME_MEASURE);
	#endif
		writel(0x01, AMFC_GL_CMD1_IRQCLR);
		if (log_en)
			printf("decompress done:%x %x, size:%d, cycles:%d, time:%d us\n",
				acl.status, status, acl.result_size,
				readl(AMFC_CMD1_TIME_MEASURE), cmd_time);
		return acl.result_size;
	} else {
		printf("decompress failed:%x %x, time:%d us, acl:%p, src:%p, dst:%p, src_size:%ld, dst_size:%ld, timeout:%d\n",
			acl.status, readl(AMFC_GL_CMD1_STATUS),
			readl(AMFC_CMD1_TIME_MEASURE),
			&acl, src, dst, src_size, dst_size, timeout);
		dump_addr((void *)0xfe024000, 300);
		dump_addr((void *)&acl, 32);
		dump_addr((void *)src, 32);
		dump_addr((void *)dst, 32);
		writel(0x03, AMFC_GL_CMD1_IRQCLR);
		return -EINVAL;
	}
}

int amfc_compress(void *src, void *dst, ssize_t src_size, ssize_t dst_size)
{
	unsigned int *src_table = NULL, *dst_table = NULL, *tmp;
	int table_size;
	unsigned int status;
	int cmd_time;
	int timeout = ((src_size + PAGE_SIZE) / PAGE_SIZE) * 20 + 5000;

	if (log_en)
		printf("%s, acl:%p, src:%p, dst:%p, src sizs:%ld, dst size:%ld\n",
			__func__, &acl, src, dst, src_size, dst_size);
	/* setup command list */
	memset(&acl, 0, sizeof(acl));
	acl.src_size = src_size;
	acl.dst_size = dst_size;
	if (page_table_mode && src_size > PAGE_SIZE) {
		table_size = sizeof(int) * (src_size / PAGE_SIZE) +
			     PAGE_SIZE * 2;
		src_table = malloc(table_size);
		if (!src_table)
			return -ENOMEM;

		tmp = (unsigned int *)(((unsigned long)src_table + PAGE_SIZE) & PAGE_MASK);
		build_tables(tmp, (unsigned long)src, src_size, TABLE_SRC);
		set_up_addr(&acl, virt_to_phys(tmp), ADDR_SRC);
		acl.src_scatter = 1;
		if (log_en)
			printf("%s, table_size:%d, src_table:%p, tmp:%p\n",
				__func__, table_size, src_table, tmp);
	} else {
		set_up_addr(&acl, virt_to_phys(src), ADDR_SRC);
	}

	if (page_table_mode && dst_size > PAGE_SIZE) {
		table_size = sizeof(int) * (dst_size / PAGE_SIZE) +
			     PAGE_SIZE * 2;
		dst_table = malloc(table_size);
		if (!dst_table)
			return -ENOMEM;

		tmp = (unsigned int *)(((unsigned long)dst_table + PAGE_SIZE) & PAGE_MASK);
		build_tables(tmp, (unsigned long)dst, dst_size, TABLE_DST);
		set_up_addr(&acl, virt_to_phys(tmp), ADDR_DST);
		acl.dst_scatter = 1;
	#ifdef CONFIG_AMFC_TEST
		if (table_test_mask)
			tmp[2] = 0;
	#endif
		if (log_en)
			printf("%s, table_size:%d, dst_table:%p, tmp:%p\n",
				__func__, table_size, dst_table, tmp);
	} else {
		set_up_addr(&acl, virt_to_phys(dst), ADDR_DST);
	}
	acl.algorithm = ALGORITHM_ZSTD;
	acl.end       = 1;
	acl.compress  = CMD_COMPRESS;
	acl.owner     = 1;
	acl.status    = 0xffffffff;

	flush_dcache_range((unsigned long)src, (unsigned long)src + src_size);
	flush_dcache_range((unsigned long)&acl, (unsigned long)&acl + sizeof(acl));
	invalidate_dcache_range((unsigned long)dst, (unsigned long)dst + dst_size);

	/* SW reset */
	writel(0x80000000, AMFC_GL_CMD0_CONTROL);
	writel(0x03, AMFC_GL_CMD0_IRQCLR);
	writel((unsigned long)&acl >> ADDR_SHIFT, AMFC_GL_CMD0_DESC_BASE0_ADDR);
	/* trigger */
	writel(1, AMFC_GL_CMD0_CONTROL);

	cmd_time = get_time();
	// wait for done or error
	while (1) {
		status = readl(AMFC_GL_CMD0_STATUS);
		if ((status & IRQ_MASK))
			break;
		if (get_time() - cmd_time > timeout) {
			printf("time out\n");
			break;
		}
	}
	cmd_time = get_time() - cmd_time;

	if (src_table)
		free(src_table);
	if (dst_table)
		free(dst_table);
	status = readl(AMFC_GL_CMD0_STATUS);
	if (!(status & AMFC_ERROR_MASK)) {
	#ifdef CONFIG_AMFC_TEST
		total_hw_compress_time += readl(AMFC_CMD0_TIME_MEASURE);
	#endif
		writel(0x01, AMFC_GL_CMD0_IRQCLR);
		if (log_en)
			printf("compress done:%x %x, size:%d, cycles:%d, time:%d us\n",
				acl.status, status, acl.result_size,
				readl(AMFC_CMD0_TIME_MEASURE), cmd_time);
		return acl.result_size;
	} else {
		printf("compress failed:%x %x, time:%d us, acl:%p, src:%p, dst:%p, src_size:%ld, dst_size:%ld, timeout:%d\n",
			acl.status, readl(AMFC_GL_CMD0_STATUS),
			readl(AMFC_CMD0_TIME_MEASURE),
			&acl, src, dst, src_size, dst_size, timeout);
		writel(0x03, AMFC_GL_CMD0_IRQCLR);
		return -EINVAL;
	}
}

#ifdef CONFIG_AMFC_TEST
#define EROFS_MAX_SIZE		(PAGE_SIZE * 64)

struct pack_file {
	char *name;
	int type;
	unsigned int size;
	unsigned int offset;
};

/*
 * one raw file and different compressed args files
 */
struct pack_file test_files[] = {
{"compress/521-new-log-2023-08-18_10_53_51.log", 0, 10858121, 0},
{"amfc_4m4k_level1/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 1076412, 10858496},
{"amfc_4m4k_level3/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 1212912, 11935744},
{"amfc_4m4k_level22/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 704301, 13152256},
{"amfc_4m128k_level1/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 926383, 13856768},
{"amfc_4m128k_level3/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 1057632, 14786560},
{"amfc_4m128k_level22/compress/521-new-log-2023-08-18_10_53_51.log.zsd", 1, 610781, 15847424},
{"compress/android_r_anon.bin", 0, 16777216, 16461824},
{"amfc_4m4k_level1/compress/android_r_anon.bin.zsd", 1, 4557912, 33243136},
{"amfc_4m4k_level3/compress/android_r_anon.bin.zsd", 1, 4353752, 37801984},
{"amfc_4m4k_level22/compress/android_r_anon.bin.zsd", 1, 3796876, 42156032},
{"amfc_4m128k_level1/compress/android_r_anon.bin.zsd", 1, 4800941, 45953024},
{"amfc_4m128k_level3/compress/android_r_anon.bin.zsd", 1, 4510906, 50757632},
{"amfc_4m128k_level22/compress/android_r_anon.bin.zsd", 1, 3784060, 55271424},
{"compress/DroidLogicTvInput.apk", 0, 10823186, 59056128},
{"amfc_4m4k_level1/compress/DroidLogicTvInput.apk.zsd", 1, 5880370, 69881856},
{"amfc_4m4k_level3/compress/DroidLogicTvInput.apk.zsd", 1, 5659473, 75763712},
{"amfc_4m4k_level22/compress/DroidLogicTvInput.apk.zsd", 1, 5260621, 81424384},
{"amfc_4m128k_level1/compress/DroidLogicTvInput.apk.zsd", 1, 5932553, 86687744},
{"amfc_4m128k_level3/compress/DroidLogicTvInput.apk.zsd", 1, 5698327, 92622848},
{"amfc_4m128k_level22/compress/DroidLogicTvInput.apk.zsd", 1, 5145713, 98324480},
{"compress/droidlogic-tv.jar", 0, 477934, 103473152},
{"amfc_4m4k_level1/compress/droidlogic-tv.jar.zsd", 1, 194111, 103952384},
{"amfc_4m4k_level3/compress/droidlogic-tv.jar.zsd", 1, 184103, 104148992},
{"amfc_4m4k_level22/compress/droidlogic-tv.jar.zsd", 1, 158674, 104333312},
{"amfc_4m128k_level1/compress/droidlogic-tv.jar.zsd", 1, 202254, 104493056},
{"amfc_4m128k_level3/compress/droidlogic-tv.jar.zsd", 1, 189089, 104697856},
{"amfc_4m128k_level22/compress/droidlogic-tv.jar.zsd", 1, 155530, 104890368},
{"compress/framework-res.apk", 0, 49495950, 105046016},
{"amfc_4m4k_level1/compress/framework-res.apk.zsd", 1, 16369915, 154542080},
{"amfc_4m4k_level3/compress/framework-res.apk.zsd", 1, 16087772, 170913792},
{"amfc_4m4k_level22/compress/framework-res.apk.zsd", 1, 15306909, 187002880},
{"amfc_4m128k_level1/compress/framework-res.apk.zsd", 1, 16283236, 202313728},
{"amfc_4m128k_level3/compress/framework-res.apk.zsd", 1, 15961873, 218599424},
{"amfc_4m128k_level22/compress/framework-res.apk.zsd", 1, 15155446, 234561536},
{"compress/Image_1", 0, 22347776, 249720832},
{"amfc_4m4k_level1/compress/Image_1.zsd", 1, 10684238, 272072704},
{"amfc_4m4k_level3/compress/Image_1.zsd", 1, 9889053, 282759168},
{"amfc_4m4k_level22/compress/Image_1.zsd", 1, 8067883, 292651008},
{"amfc_4m128k_level1/compress/Image_1.zsd", 1, 10509407, 300720128},
{"amfc_4m128k_level3/compress/Image_1.zsd", 1, 9685857, 311230464},
{"amfc_4m128k_level22/compress/Image_1.zsd", 1, 7817946, 320917504},
{"compress/NotoColorEmoji.ttf", 0, 2961984, 328736768},
{"amfc_4m4k_level1/compress/NotoColorEmoji.ttf.zsd", 1, 1794329, 331702272},
{"amfc_4m4k_level3/compress/NotoColorEmoji.ttf.zsd", 1, 1721225, 333500416},
{"amfc_4m4k_level22/compress/NotoColorEmoji.ttf.zsd", 1, 1542996, 335224832},
{"amfc_4m128k_level1/compress/NotoColorEmoji.ttf.zsd", 1, 1811509, 336769024},
{"amfc_4m128k_level3/compress/NotoColorEmoji.ttf.zsd", 1, 1723016, 338583552},
{"amfc_4m128k_level22/compress/NotoColorEmoji.ttf.zsd", 1, 1498439, 340307968},
{"compress/uImage", 0, 11194624, 341807104},
{"amfc_4m4k_level1/compress/uImage.zsd", 1, 11166211, 353005568},
{"amfc_4m4k_level3/compress/uImage.zsd", 1, 11163633, 364175360},
{"amfc_4m4k_level22/compress/uImage.zsd", 1, 11156644, 375341056},
{"amfc_4m128k_level1/compress/uImage.zsd", 1, 11170371, 386498560},
{"amfc_4m128k_level3/compress/uImage.zsd", 1, 11167452, 397672448},
{"amfc_4m128k_level22/compress/uImage.zsd", 1, 11149718, 408842240},
{"compress/zero.bin", 0, 2097152, 419995648},
{"amfc_4m4k_level1/compress/zero.bin.zsd", 1, 2070, 422096896},
{"amfc_4m4k_level3/compress/zero.bin.zsd", 1, 2069, 422100992},
{"amfc_4m4k_level22/compress/zero.bin.zsd", 1, 2068, 422105088},
{"amfc_4m128k_level1/compress/zero.bin.zsd", 1, 87, 422109184},
{"amfc_4m128k_level3/compress/zero.bin.zsd", 1, 86, 422113280},
{"amfc_4m128k_level22/compress/zero.bin.zsd", 1, 85, 422117376},
{}
};

/*
 * pack file test for performance and data consistant.
 * test file and above array are generated by scripts
 * You need preload packed test file to a ddr address and then
 * use command like:
 *
 *    s7_pxp# amfc pack_test 12000000 30000000 34000000
 *
 * dst address must not overlap with base + file_size
 * dst2 address must not overlap with dst 1
 */
static void amfc_pack_test(unsigned long base, void *dst, void *dst2)
{
	int i, ret;
	struct pack_file *pack;
	void *src, *cur_src_addr = NULL;
	int cur_src_size = 0;

	for (i = 0; i < ARRAY_SIZE(test_files); i++) {
		pack = &test_files[i];
		if (!pack->name)
			break;
		src = (void *)(base + pack->offset);
		if (!pack->type) {
			/*
			 * check hw compress and hw decompress
			 */
			printf("-----------------------------\n");
			printf("compress %s\n", pack->name);
			ret = amfc_compress(src, dst, pack->size, pack->size);
			cur_src_size = pack->size;
			cur_src_addr = src;
			if (ret > 0) {
				printf("decompress %s\n", pack->name);
				ret = amfc_decompress(dst, dst2, ret, pack->size);
				ret = memcmp(dst2, src, pack->size);
				printf("compare %p -> %p with %d %s\n\n",
					dst2, cur_src_addr, cur_src_size,
					ret ? "FAIL!!" : "OK!!");
			} else {
				printf("compress failed\n");
			}
		} else {
			/*
			 * check sw compressed and hw decompress
			 */
			printf("decompress %s\n", pack->name);
			ret = amfc_decompress(src, dst, pack->size, cur_src_size);
			ret = memcmp(dst, cur_src_addr, cur_src_size);
			printf("compare %p -> %p with %d %s\n\n",
				dst, cur_src_addr, cur_src_size,
				ret ? "FAIL!!" : "OK!!");
		}
		if (tstc()) {
			printf("key abort\n");
			(void)getchar();
			break;
		}
	}
}

static unsigned char test_page[EROFS_MAX_SIZE * 2] __attribute__((aligned(4096)));

static int get_sw_compressed_size(void *src)
{
	int i = 0;
	unsigned char *p;

	p = (unsigned char *)src + PAGE_SIZE - 1;
	while (*p == 0) {
		i++;
		p--;
	}
	return i > PAGE_SIZE ? 0 : PAGE_SIZE - i;
}

static void *contain_zstd_page(void *src, int cluster)
{
	unsigned char *p = (unsigned char *)src;
	int i = 0;

	/* skip leading zero */
	while (p[i] == 0 && i < cluster) {
		i++;
	}
	if (i >= cluster)
		return NULL;

	p += i;
	if (p[0] == 0x28 && p[1] == 0xb5 && p[2] == 0x2f && p[3] == 0xfd) {
		return p;
	} else
		return NULL;
}

static int memcmp1(void *p1, void *p2, int size)
{
	unsigned int *i1, *i2;
	int i;
	int ret = 0;

	i1 = (unsigned int *)p1;
	i2 = (unsigned int *)p2;
	for (i = 0; i < size / 4; i++) {
		if (i1[i] != i2[i]) {
			printf("i:%d, addr:%p->%p, value:%x->%x\n",
				i, i1, i2, i1[i], i2[i]);
			ret = 1;
		}
	}
	return ret;
}

/*
 * data is captured from Android system by
 * https://scgit.amlogic.com/#/c/329847/
 * pattern: a raw page + a zstd compressed page interleave
 *
 * before test, you should change a captured data to hex and pre-load it
 * to DDR via load_mem.tcl in PXP
 */
static void amfc_zram_test(void *src, unsigned long size, int cmp)
{
	unsigned long i;
	unsigned char *p, *z;
	int sw_csize, hw_csize;
	int no_compress = 0;
	int sw_decompress = 0;
	int hw_compress = 0;
	int hw_decompress = 0;
	int sd_error = 0;

	total_sw_compressed_size = 0;
	total_hw_compressed_size = 0;
	total_hw_compress_time   = 0;
	total_hw_decompress_time = 0;
	total_hw_self_decompress_time = 0;
	for (i = 0; i < size; i += PAGE_SIZE) {
		p = (unsigned char *)(src + i);
		if (self) { // sw compressed page
			self = 0;
			z = contain_zstd_page(p, PAGE_SIZE);
			if (z) {
				sw_csize = PAGE_SIZE - (z - p);
				total_sw_compressed_size += sw_csize;
				amfc_decompress(z, test_page, sw_csize, PAGE_SIZE);
				sw_decompress++;
				if (cmp && memcmp(test_page, (void *)((unsigned long)p - PAGE_SIZE), PAGE_SIZE)) {
					printf("hw decompress address :%p not match\n", p);
				}
			}
		} else {	// src page
			self = 1;
			memset(test_page, 0, sizeof(test_page));
			hw_csize = amfc_compress(p, test_page, PAGE_SIZE, PAGE_SIZE);
			hw_compress++;
			if (hw_csize > 0) {
				total_hw_compressed_size += hw_csize;
				amfc_decompress(test_page, test_page + PAGE_SIZE, hw_csize, PAGE_SIZE);
				hw_decompress++;
				if (cmp && memcmp1(test_page + PAGE_SIZE, (void *)p, PAGE_SIZE)) {
					char cmd[128] = {0};

					printf("hw self-decompress address :%p, size:%d not match\n", p, hw_csize);
					sprintf(cmd, "md %lx %x\n", (unsigned long)test_page, hw_csize / 4 + 10);
					run_command(cmd, 0);
					sd_error++;
				}
			} else {
				no_compress++;
				printf("hw compress address :%p failed\n", p);
			}
		}
		if (!(i & 0x3f000)) {
			printf("ADDR:%lx, sw_csize:%8ld, hw_csize:%8ld, hw_ctime:%8ld us, hw_dtime:%8ld, hw_sdtime:%8ld, no_comp:%4d, sw_dec_page:%5d, hw_dec_page:%5d, hw_com_page:%5d, sd_error:%d\n",
				(unsigned long)p, total_sw_compressed_size,
				total_hw_compressed_size,
				total_hw_compress_time, total_hw_decompress_time,
				total_hw_self_decompress_time, no_compress,
				sw_decompress, hw_decompress, hw_compress, sd_error);
		}
		if (tstc()) {
			printf("key abort\n");
			(void)getchar();
			break;
		}
	}
	printf("ADDR:%lx, sw_csize:%8ld, hw_csize:%8ld, hw_ctime:%8ld us, hw_dtime:%8ld, hw_sdtime:%8ld, no_comp:%4d, sw_dec_page:%5d, hw_dec_page:%5d, hw_com_page:%5d, sd_error:%d\n",
		(unsigned long)p, total_sw_compressed_size, total_hw_compressed_size,
		total_hw_compress_time, total_hw_decompress_time,
		total_hw_self_decompress_time,
		no_compress, sw_decompress, hw_decompress, hw_compress, sd_error);
}

static void *get_zstd_addr(void *src, int cluster, int *c_size)
{
	unsigned char *p = (unsigned char *)src;
	int i = 0;
	unsigned long addr, remain;

	/* skip leading zero */
	while (p[i] == 0 && i < cluster) {
		i++;
	}
	if (i >= cluster)
		return NULL;

	p += i;
	if (p[0] == 0x28 && p[1] == 0xb5 && p[2] == 0x2f && p[3] == 0xfd) {
		if (cluster > PAGE_SIZE) {
			/* search next zstd page in this cluster to detect real size */
			remain = cluster - i;
			remain &= PAGE_MASK;
			addr = ((unsigned long)p + PAGE_SIZE) & PAGE_MASK;
			while (remain) {
				if (contain_zstd_page((void *)addr, PAGE_SIZE)) {
					//printf("have zstd page addr:%lx, p:%p, i:%d, remain:%ld, cluster:%d\n",
					//	addr, p, i, remain, cluster);
					break;
				}
				addr += PAGE_SIZE;
				remain -= PAGE_SIZE;
			}
			*c_size = addr - (unsigned long)p;
			//printf("addr:%lx, p:%p, i:%d, remain:%ld, cluster:%d, real size:%d\n",
			//	addr, p, i, remain, cluster, *c_size);
		} else
			*c_size = cluster - i;

		return p;
	} else
		return NULL;
}

/*
 * Refer https://confluence.amlogic.com/display/SW/EROFS+with+ZSTD+and+performance+compare to
 * build a ZSTD supported erofs image
 *
 * before test, you should change a erofs image to hex and pre-load it
 * to DDR via load_mem.tcl in PXP
 */
static void amfc_erofs_test(void *src, unsigned long size, int pcluster, int verify)
{
	unsigned long i, total_decompress, total_sw_csize;
	unsigned char *p;
	int hw_decompress = 0, ret, c_size;
	struct abuf in, out;
	int decompress_fail = 0;
	int match_fail = 0;

	total_decompress = 0;
	total_sw_csize = 0;
	total_hw_decompress_time = 0;
	self = 0;
	for (i = 0; i < size; i += PAGE_SIZE) {
		p = get_zstd_addr(src + i, pcluster, &c_size);
		//if (c_size < 64) {
		//	// HW BUG:
		//	//1, source address must align to 64 bytes, if src size is tool small, decode will hung
		//	//2, if size set to 4k but too many leading zero, decode will hung
		//	//3, for erofs test, page table mode must closed.
		//	printf("skip size too small:%lx, c_size:%d\n", (unsigned long)src + i, c_size);
		//	continue;
		//}
		if (p) {  // zstd compressed page
			hw_decompress = amfc_decompress(p, test_page, c_size, EROFS_MAX_SIZE);
			if (hw_decompress) {
				/* sw_decompress and double check */
				total_decompress += hw_decompress;
				total_sw_csize += c_size;
				if (verify) {
					abuf_init_set(&in, p, c_size);
					abuf_init_set(&out, test_page + EROFS_MAX_SIZE,
						      EROFS_MAX_SIZE);
					ret = zstd_decompress(&in, &out);
					if (ret >= 0 && ret == hw_decompress) {
						if (memcmp(test_page, test_page + EROFS_MAX_SIZE, ret)) {
							match_fail++;
							printf("hw decompress %p %d->%d not match\n", p, c_size, ret);
						}
					} else {
						printf("sw decompress %p %d %d <-> %d failed\n",
							p, c_size, hw_decompress, ret);
					}
				}
			} else {
				decompress_fail++;
			}
		}
		if (!(i & 0x3f000)) {
			printf("ADDR:%lx, hw_decompressed:%8ld, time:%8ld, dec failed:%4d, match failed:%4d, sw_csize:%ld\n",
			       (unsigned long)src + i, total_decompress,
			       total_hw_decompress_time,
			       decompress_fail, match_fail, total_sw_csize);
		}
		if (tstc()) {
			printf("key abort\n");
			(void)getchar();
			break;
		}
	}
	printf("ADDR:%lx, hw_decompressed:%8ld, time:%8ld, dec failed:%4d, match failed:%4d, sw_csize:%ld\n",
	       (unsigned long)src + i, total_decompress,
	       total_hw_decompress_time,
	       decompress_fail, match_fail, total_sw_csize);
}

static void show_acl(struct amfc_cmd_list *p_acl, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		printf("%p: %08x %08x %08x %08x  %08x %08x %08x %08x\n",
			p_acl + i, p_acl[i].src_addr, p_acl[i].dst_addr,
			p_acl[i].link_addr, p_acl[i].src_size,
			p_acl[i].dst_size, p_acl[i].control,
			p_acl[i].status, p_acl[i].result_size);
	}
}

static void run_table_test(struct amfc_cmd_list *p_acl, void *src,
			   void *dst, void *dst2, long size)
{
	unsigned int status;
	int count;

	count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	flush_dcache_range((unsigned long)src, (unsigned long)src + size);
	flush_dcache_range((unsigned long)dst, (unsigned long)dst + size);
	flush_dcache_range((unsigned long)dst2, (unsigned long)dst2 + size);
	flush_dcache_range((unsigned long)p_acl, (unsigned long)p_acl + (count + 3) * sizeof(acl));
	/* SW reset */
	writel(0x80000000, AMFC_GL_CMD0_CONTROL);
	writel(0x03, AMFC_GL_CMD0_IRQCLR);
	writel((unsigned long)p_acl >> ADDR_SHIFT, AMFC_GL_CMD0_DESC_BASE0_ADDR);
	/* trigger */
	writel(1, AMFC_GL_CMD0_CONTROL);

	// wait for done or error
	while (1) {
		status = readl(AMFC_GL_CMD0_STATUS);
		if ((status & IRQ_MASK))
			break;
		if (tstc()) {
			printf("key abort\n");
			(void)getchar();
			break;
		}
	}
	status = readl(AMFC_GL_CMD0_STATUS);
	if (!(status & AMFC_ERROR_MASK)) {
		writel(0x01, AMFC_GL_CMD0_IRQCLR);
	} else {
		writel(0x03, AMFC_GL_CMD0_IRQCLR);
		printf("test failed:%x %x\n",
			p_acl[0].status, readl(AMFC_GL_CMD0_STATUS));
	}
}

/*
 * before test, you should change a simple text file(size > 16KB) to hex and pre-load it
 * to DDR via load_mem.tcl in PXP
 */
static void amfc_table_test(void *src, void *dst, void *dst2, unsigned long size)
{
	struct amfc_cmd_list *p_acl, *p_raw;
	int count, i;

	count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	p_acl = malloc((count + 3) * sizeof(acl));
	if (!p_acl)
		return;

	memset(dst, 0, size);
	p_raw = p_acl;
	memset(p_acl, 0, (count + 3) * sizeof(acl));
	if ((unsigned long)p_acl & 0x3f) // not aligned to 64 bytes
		p_acl = (struct amfc_cmd_list *)(((unsigned long)p_acl + 0x40) & ~0x3f);

	printf("acl:%p, src:%p, dst:%p, dst2:%p, size:%ld\n", p_acl, src, dst, dst2, size);
	// case 1: compress src to dst by table mode
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = PAGE_SIZE;
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_COMPRESS;
		p_acl[i].owner       = 1;
	}
	printf("table test case 1: compress with no end bit\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 2: decompress dst to dst2 by table mode
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 2: decompress with end bit\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 3: decompress and compress interleave mode
	for (i = 0; i < count; i++) {
		if (i & 0x01) {
			p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
			p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
			p_acl[i].link_addr   = 0;
			p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
			p_acl[i].dst_size    = PAGE_SIZE;
			p_acl[i].status      = 0xffffffff;
			p_acl[i].result_size = 0;
			p_acl[i].control     = 0;
			p_acl[i].algorithm   = ALGORITHM_ZSTD;
			p_acl[i].compress    = CMD_DECOMPRESS;
			p_acl[i].owner       = 1;
		} else {
			p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
			p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
			p_acl[i].link_addr   = 0;
			p_acl[i].src_size    = PAGE_SIZE;
			p_acl[i].dst_size    = PAGE_SIZE;
			p_acl[i].status      = 0xffffffff;
			p_acl[i].result_size = 0;
			p_acl[i].control     = 0;
			p_acl[i].algorithm   = ALGORITHM_ZSTD;
			p_acl[i].compress    = CMD_COMPRESS;
			p_acl[i].owner       = 1;
		}
	}
	p_acl[i - 1].end = 1;

	printf("table test case 3: enc/dec interleave table mode\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 4: bad table with no owner bit set
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		if (i != 1)
			p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 4: descriptor error with no owner\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 5: bad table with no wrong algorithm
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		if (i == 1)
			p_acl[i].algorithm   = ALGORITHM_LZ4HC;
		else
			p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 5: descriptor error with wrong algorithm\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 6: bad table with zero src size
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		if (i != 1)
			p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		else
			p_acl[i].src_size    = 0;
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 6: descriptor error with zero source size\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 7: bad table with zero dst size
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		if (i != 1)
			p_acl[i].dst_size    = PAGE_SIZE;
		else
			p_acl[i].dst_size    = 0;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 7: descriptor error with zero dst size\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 8: bad table with zero link address
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		if (i == 1)
			p_acl[i].link_mode = 1;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 8: descriptor error with zero link address\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 9: bad table with dst size over flow
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		if (i != 1)
			p_acl[i].dst_size    = PAGE_SIZE;
		else
			p_acl[i].dst_size    = 10;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 9: descriptor error with dst size overflow\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 10: bad table with decompress src size too small
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		if (i == 1)
			p_acl[i].src_size    = 10;
		else
			p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("table test case 10: descriptor error with decompress size too small\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	// case 11: bad table with compress size too small
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
		p_acl[i].link_addr   = 0;
		if (i == 1)
			p_acl[i].src_size    = 2;
		else
			p_acl[i].src_size    = PAGE_SIZE;
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].control     = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_COMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;
	printf("table test case 11: bad table with compress size too small\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count);

	free(p_raw);
}

/*
 * before test, you should change a simple text file(size > 64KB) to hex and pre-load it
 * to DDR via load_mem.tcl in PXP
 */
static void amfc_link_test(void *src, void *dst, void *dst2, unsigned long size)
{
	struct amfc_cmd_list *p_acl, *p_raw;
	int count, i;

	count = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;

	p_acl = malloc((count + 3) * sizeof(acl));
	if (!p_acl)
		return;

	memset(dst, 0, size);
	p_raw = p_acl;
	memset(p_acl, 0, (count + 3) * sizeof(acl));
	if ((unsigned long)p_acl & 0x3f) // not aligned to 64 bytes
		p_acl = (struct amfc_cmd_list *)(((unsigned long)p_acl + 0x40) & ~0x3f);

	printf("acl:%p, src:%p, dst:%p, dst2:%p, size:%ld\n", p_acl, src, dst, dst2, size);
	// case 1: compress src to dst by link mode
	for (i = 0; i < count; i += 2) {
		p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
		p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
		p_acl[i].src_size    = PAGE_SIZE;
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].link_mode   = 1;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_COMPRESS;
		p_acl[i].owner       = 1;
	}
	printf("link test case 1: compress with NULL link end\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	// case 2: decompress dst to dst2 by table mode
	memset(p_acl, 0, (count + 2 ) * sizeof(*p_acl));
	for (i = 0; i < count; i += 2) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].link_mode   = 1;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 2].end = 1;  // end but build a valid link next descriptor
	p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
	p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
	p_acl[i].link_addr   = 0;
	p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
	p_acl[i].dst_size    = PAGE_SIZE;
	p_acl[i].status      = 0xffffffff;
	p_acl[i].result_size = 0;
	p_acl[i].link_mode   = 0;
	p_acl[i].algorithm   = ALGORITHM_ZSTD;
	p_acl[i].compress    = CMD_DECOMPRESS;
	p_acl[i].owner       = 1;
	p_acl[i].end = 1;  // end

	printf("link test case 2: decompress with end bit but have a valid link descriptor\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	// case 3: decompress and compress interleave link mode
	memset(p_acl, 0, (count + 2 ) * sizeof(*p_acl));
	for (i = 0; i < count; i += 2) {
		if (i & 0x02) {
			p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
			p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
			p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
			p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
			p_acl[i].dst_size    = PAGE_SIZE;
			p_acl[i].status      = 0xffffffff;
			p_acl[i].result_size = 0;
			p_acl[i].link_mode   = 1;
			p_acl[i].algorithm   = ALGORITHM_ZSTD;
			p_acl[i].compress    = CMD_DECOMPRESS;
			p_acl[i].owner       = 1;
		} else {
			p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
			p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
			p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
			p_acl[i].src_size    = PAGE_SIZE;
			p_acl[i].dst_size    = PAGE_SIZE;
			p_acl[i].status      = 0xffffffff;
			p_acl[i].result_size = 0;
			p_acl[i].link_mode   = 1;
			p_acl[i].algorithm   = ALGORITHM_ZSTD;
			p_acl[i].compress    = CMD_COMPRESS;
			p_acl[i].owner       = 1;
		}
	}
	p_acl[i - 2].end = 1;
	p_acl[i - 2].link_mode = 0;

	printf("link test case 3: enc/dec interleave link mode\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	// case 4: valid link descriptor but link mode disabled
	memset(p_acl, 0, (count + 2 ) * sizeof(*p_acl));
	for (i = 0; i < count; i++) {
		p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
		p_acl[i].src_size    = PAGE_SIZE;
		p_acl[i].dst_size    = PAGE_SIZE;
		if (!i)	{		// link to 3rd desc
			p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
		}
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_COMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 1].end = 1;

	printf("link test case 4: valid link descriptor but link mode disabled \n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	// case 5: circular link
	memset(p_acl, 0, (count + 2 ) * sizeof(*p_acl));
	for (i = 0; i < count; i += 2) {
		p_acl[i].src_addr    = ((unsigned long)dst  + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst2 + PAGE_SIZE * i);
		p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 2]) >> ADDR_SHIFT;
		p_acl[i].src_size    = get_sw_compressed_size(dst + PAGE_SIZE * i);
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].link_mode   = 1;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_DECOMPRESS;
		p_acl[i].owner       = 1;
	}
	p_acl[i - 2].link_addr   = ((unsigned long)&p_acl[0]) >> ADDR_SHIFT;

	printf("link test case 5: circular link\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	// case 6: table and link
	memset(p_acl, 0, (count + 2 ) * sizeof(*p_acl));
	for (i = 0; i < count; i ++) {
		p_acl[i].src_addr    = ((unsigned long)src + PAGE_SIZE * i);
		p_acl[i].dst_addr    = ((unsigned long)dst + PAGE_SIZE * i);
		p_acl[i].src_size    = PAGE_SIZE;
		p_acl[i].dst_size    = PAGE_SIZE;
		p_acl[i].status      = 0xffffffff;
		p_acl[i].result_size = 0;
		p_acl[i].algorithm   = ALGORITHM_ZSTD;
		p_acl[i].compress    = CMD_COMPRESS;
		p_acl[i].owner       = 1;
		if (i == 2) {
			p_acl[i].link_addr   = ((unsigned long)&p_acl[i + 4]) >> ADDR_SHIFT;
			p_acl[i].link_mode   = 1;
		}
	}
	p_acl[i - 1].end = 1;

	printf("link test case 6: table and link\n");
	run_table_test(p_acl, src, dst, dst2, size);
	show_acl(p_acl, count + 3);

	free(p_raw);

}

static int check_zero(void *start, int range)
{
	unsigned char *p = (unsigned char *)start;
	int i;

	for (i = 0; i < range; i++)
		if (p[i])
			return 1;
	return 0;
}

static void amfc_address_test(void *src, void *dst,
			      void *dst2, int src_size)
{
	int i, ret;
	void *p, *p1, *p2;
	char cmd_buf[128] = {};

	// 1, compress dst not align test
	printf("==== compress dst and decompress src not aligned test\n");
	for (i = 0; i < 64; i++) {
		p = (void *)(dst + i);
		memset(dst,  0, src_size + 128);
		memset(dst2, 0, src_size + 128);
		run_command("dcache flush", 0);

		ret = amfc_compress(src, p, src_size, src_size);
		if (ret > 0) {
			amfc_decompress(p, dst2, ret, src_size);
			if (memcmp(src, dst2, src_size)) {
				printf("%s, src:%p, dst:%p, dst2:%p, ret:%d, src_size:%d, i:%d not match\n",
					__func__, src, p, dst2, ret, src_size, i);
			}
			p1 = p + ret;
			p2 = (void *)(((unsigned long)p1 + 64) & ~63);
			if (check_zero(dst, i)) {
				printf("left over write\n");
				sprintf(cmd_buf, "md %lx 10\n", (long)dst);
				run_command(cmd_buf, 0);
			}
			if (check_zero(p1, p2 - p1)) {
				printf("right over write\n");
				sprintf(cmd_buf, "md %lx 10\n", (long)p1);
				run_command(cmd_buf, 0);
			}
		}
	}

	// 2, compress src not aligned
	memset(dst,  0, src_size + 128);
	amfc_compress(src, dst, src_size, src_size);
	printf("==== compress src not aligned test\n");
	for (i = 0; i < 64; i++) {
		p = (void *)(dst + src_size + i);
		memset(p,  0, src_size + 128);
		memset(dst2, 0, src_size + 128);
		memcpy(p, src, src_size);
		run_command("dcache flush", 0);

		ret = amfc_compress(p, dst2, src_size, src_size);
		if (ret > 0) {
			if (memcmp(dst, dst2, ret)) {
				printf("%s, dst:%p, dst2:%p, ret:%d, src_size:%d i:%d, not match\n",
					__func__, dst, dst2, ret, src_size, i);
			}
		}
	}

	// 3, decompress dest not aligned
	memset(dst,  0, src_size + 128);
	ret = amfc_compress(src, dst, src_size, src_size);
	printf("==== decompress dst not aligned test\n");
	for (i = 0; i < 64; i++) {
		p = (void *)(dst2 + i);
		memset(dst2, 0, src_size + 128);
		run_command("dcache flush", 0);

		ret = amfc_decompress(dst, p, ret, src_size);
		if (ret > 0) {
			if (memcmp(p, src, src_size)) {
				printf("%s, p:%p, src:%p, ret:%d, src_size:%d, i:%d not match\n",
					__func__, p, src, ret, src_size, i);
			}
			if (check_zero(dst2, i)) {
				printf("left over write\n");
				sprintf(cmd_buf, "md %lx 10\n", (long)dst2);
				run_command(cmd_buf, 0);
			}
			p1 = p + src_size;
			p2 = (void *)(((unsigned long)p1 + 64) & ~63);
			if (check_zero(p1, p2 - p1)) {
				printf("right over write\n");
				sprintf(cmd_buf, "md %lx 10\n", (long)p1);
				run_command(cmd_buf, 0);
			}
		}
	}
}
#endif


static int do_amfc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long src;
	unsigned long dst;
	unsigned long src_size;
	unsigned long dst_size;
	int ret;

	if (!strcmp(argv[1], "set")) {
		if (argc != 4)
			return -1;

		if (!strcmp(argv[2], "page_table")) {
			page_table_mode = simple_strtoul(argv[3], NULL, 16);
			printf("set page_table_mode to:%d\n", page_table_mode);
		}
		if (!strcmp(argv[2], "log")) {
			log_en = simple_strtoul(argv[3], NULL, 16);
			printf("set log to:%d\n", log_en);
		}
		return 0;
	}

#ifdef CONFIG_AMFC_TEST
	if (!strcmp(argv[1], "pack_test")) {
		if (argc != 5)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		dst      = simple_strtoul(argv[3], NULL, 16);
		src_size = simple_strtoul(argv[4], NULL, 16);
		amfc_pack_test(src, (void *)dst, (void *)src_size);
		return 0;
	}

	if (!strcmp(argv[1], "zram_test")) {
		if (argc != 5)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		src_size = simple_strtoul(argv[3], NULL, 16);
		dst_size = simple_strtoul(argv[4], NULL, 16);
		amfc_zram_test((void *)src, src_size, dst_size);
		return 0;
	}
	if (!strcmp(argv[1], "erofs_test")) {
		if (argc != 6)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		dst      = simple_strtoul(argv[3], NULL, 16);
		src_size = simple_strtoul(argv[4], NULL, 16);
		dst_size = simple_strtoul(argv[5], NULL, 16);
		amfc_erofs_test((void *)src, dst, src_size, dst_size);
		return 0;
	}

	if (!strcmp(argv[1], "table_test")) {
		if (argc != 6)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		dst      = simple_strtoul(argv[3], NULL, 16);
		src_size = simple_strtoul(argv[4], NULL, 16);
		dst_size = simple_strtoul(argv[5], NULL, 16);
		amfc_table_test((void *)src, (void *)dst, (void *)src_size, dst_size);
		return 0;
	}

	if (!strcmp(argv[1], "link_test")) {
		if (argc != 6)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		dst      = simple_strtoul(argv[3], NULL, 16);
		src_size = simple_strtoul(argv[4], NULL, 16);
		dst_size = simple_strtoul(argv[5], NULL, 16);
		amfc_link_test((void *)src, (void *)dst, (void *)src_size, dst_size);
		return 0;
	}
	if (!strcmp(argv[1], "table_mask")) {
		if (argc != 3)
			return -1;
		table_test_mask = simple_strtoul(argv[2], NULL, 16);
		return 0;
	}
	if (!strcmp(argv[1], "addr_test")) {
		if (argc != 6)
			return -1;
		src      = simple_strtoul(argv[2], NULL, 16);
		dst      = simple_strtoul(argv[3], NULL, 16);
		src_size = simple_strtoul(argv[4], NULL, 16);
		dst_size = simple_strtoul(argv[5], NULL, 16);
		amfc_address_test((void *)src, (void *)dst, (void *)src_size, dst_size);
		return 0;
	}
#endif

	if (argc != 6)
		return -1;
	src      = simple_strtoul(argv[2], NULL, 16);
	dst      = simple_strtoul(argv[3], NULL, 16);
	src_size = simple_strtoul(argv[4], NULL, 16);
	dst_size = simple_strtoul(argv[5], NULL, 16);
	if (!strcmp(argv[1], "compress"))
		ret  = amfc_compress((void *)src, (void *)dst, src_size, dst_size);
	else if (!strcmp(argv[1], "decompress"))
		ret  = amfc_decompress((void *)src, (void *)dst, src_size, dst_size);

	return 0;
}

U_BOOT_CMD(
	amfc,	7,	0,	do_amfc,
	"amfc subsystem",
	"compress/decompress [src_addr] [dst_addr] [src_size] [dst_size]\n"
	"set:\n"
	"    page_table [0/1]      ---- enable/disable page table mode\n"
	"memcmp [src] [dst] [size] ----compare two memory region\n"
	"All parameters in hex value"
);
