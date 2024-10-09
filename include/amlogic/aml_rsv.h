/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_RSV_H_
#define __MESON_RSV_H_

#define BBT_NAND_MAGIC	"nbbt"
#define ENV_NAND_MAGIC	"nenv"
#define KEY_NAND_MAGIC	"nkey"
#define SEC_NAND_MAGIC	"nsec"
#define DTB_NAND_MAGIC	"ndtb"
#define DDR_NAND_MAGIC	"nddr"

/*define abnormal state for reserved area*/
#define POWER_ABNORMAL_FLAG	0x01
#define ECC_ABNORMAL_FLAG	0x02
#define DDR_PARA_SIZE 2048

#define BBT_INFO_INDEX		0
#define ENV_INFO_INDEX		1
#define KEY_INFO_INDEX		2
#define DTB_INFO_INDEX		3
#define DDR_INFO_INDEX		4

#define INFO_DATA(n, b, s)	{ .name = (n), .blocks = (b), .size = (s) }

#define BBT_START_BLOCK		20
#define BBT_TOTAL_BLOCKS	4

enum BL2_LAYOUT_SELECT {
	BL2_LAYOUT_DEFAULT = 0,	//BL2 align with block size
	BL2_LAYOUT_512	   = 1,	//BL2 size fixed 512 pages
	BL2_LAYOUT_1024	   = 2,	//BL2 size fixed 1024 pages
};

struct rsv_part {
	char name[8];
	unsigned int start_block;
	unsigned int blocks;
	unsigned int size;
};

struct meson_rsv_info_t {
	struct mtd_info *mtd;
	struct valid_node_t *nvalid;
	struct free_node_t *nfree;
	unsigned int start;
	unsigned int end;
	unsigned int size;
	char name[8];
	u_char valid;
	u_char init;
	void *handler;
};

struct valid_node_t {
	s16 ec;
	s16	blk_addr;
	s16	page_addr;
	int timestamp;
	s16 status;
};

struct free_node_t {
	unsigned int index;
	s16 ec;
	s16	blk_addr;
	int dirty_flag;
	struct free_node_t *next;
};

struct oobinfo_t {
	char name[4];
	s16 ec;
	unsigned timestamp:15;
	unsigned status_page:1;
};

#define MAX_MESON_RSV_INFO_NUM	8

struct meson_rsv_handler_t {
	struct mtd_info *mtd;
	unsigned long long fn_bitmask;
	struct free_node_t **free_node;
	struct rsv_part *rsv_part;
	int entries;
	struct meson_rsv_info_t rsv_info[MAX_MESON_RSV_INFO_NUM];
	void *priv;
};

int rsvname2index(const char *rsv_name);
u32 meson_rsv_part_get_start_block(struct mtd_info *mtd);
u32 meson_rsv_part_get_bl2_part_size(struct mtd_info *mtd);
u32 meson_rsv_part_get_bl2_copy_number(struct mtd_info *mtd);
u32 meson_rsv_part_get_bl2_copy_size(struct mtd_info *mtd);
u64 meson_rsv_part_get_tpl_start(struct mtd_info *mtd);
u64 meson_rsv_part_get_tpl_size(struct mtd_info *mtd);
int meson_ext_rsv_info_read(u_char *dest, size_t size, int index);
int meson_ext_rsv_info_write(u_char *source, size_t size, int index);
u32 meson_ext_rsv_info_size(int index);
int meson_ext_rsv_info_erase(int index);

void meson_rsv_check_all_except_bbt(void);
int meson_rsv_check_bbt(void);
int meson_rsv_save_bbt(u8 *bbt);
int meson_rsv_read_bbt(u8 *bbt);

int meson_rsv_check(struct meson_rsv_info_t *rsv_info);
int meson_rsv_init(struct mtd_info *mtd, struct meson_rsv_handler_t *handler);
int meson_rsv_scan(struct meson_rsv_info_t *rsv_info);
int meson_rsv_read(struct meson_rsv_info_t *rsv_info, u_char *buf);
int meson_rsv_save(struct meson_rsv_info_t *rsv_info, u_char *buf);
int meson_rsv_write(struct meson_rsv_info_t *rsv_info, u_char *buf);
int meson_rsv_erase_protect(struct meson_rsv_handler_t *handler,
uint32_t block_addr);
int meson_rsv_add_dtb(void *blob, int parent_offset);
struct mtd_info *mtd_store_get(int dev);

struct rsv_part *get_mtd_rsv_partition(void);
int get_mtd_rsv_partition_count(void);

#endif/* __MESON_RSV_H_ */
