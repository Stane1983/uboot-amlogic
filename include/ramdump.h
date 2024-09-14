/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __RAMDUMP_H__
#define __RAMDUMP_H__

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT              12
#define PAGE_SIZE               ((1) << PAGE_SHIFT)

/* align addr on a size boundary - adjust address up/down if needed */
#define _ALIGN_UP(addr, size)	(((addr) + ((size) - 1)) & (~((typeof(addr))(size) - 1)))
#define _ALIGN_DOWN(addr, size)	((addr) & (~((typeof(addr))(size) - 1)))

/* align addr on a size boundary - adjust address up if needed */
#define _ALIGN(addr, size)      _ALIGN_UP(addr, size)

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)        _ALIGN(addr, PAGE_SIZE)

#define RAMDUMP_STICKY_DATA_MASK		(0xFFFF)

extern void check_ramdump(void);
extern void ramdump_init(void);

extern unsigned long ramdump_base;
extern unsigned long ramdump_size;

int ramdump_save_compress_data(void);

/* ramdump ddr md5 check */
#define MD5_BLOCK_SIZE              (8 << 20)
#define MD5_STORE_SIZE              (64 << 10)
#define MD5_PER_ROW_NUM             (32)
#define MD5_BL2E_1_BASE_ADDR        (0x00B00000)
#define MD5_BL2E_2_BASE_ADDR        (0x00B10000)
#define MD5_BL33Z_1_BASE_ADDR       (0x00B20000)
#define MD5_BL33Z_2_BASE_ADDR       (0x00B30000)
#define MD5_BL33X_1_BASE_ADDR       (0x00B40000)
#define MD5_BL33X_2_BASE_ADDR       (0x00B50000)
#define MD5_MAGIC                   "RAMDUMPMD5"

struct rammd5_info_t {
	char magic[16];
	char stage[16];
	unsigned int block_size;
	unsigned int ddr_size;
	unsigned int area1_start;
	unsigned int area1_end;
	unsigned int area2_start;
	unsigned int area2_end;
};

#endif /* __RAMDUMP_H__ */
