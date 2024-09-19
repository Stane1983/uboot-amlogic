// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <amlogic/aml_rsv.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/partition_table.h>
#include <asm/amlogic/arch/cpu_config.h>
#include <amlogic/storage.h>
#include <fdt_support.h>

extern int info_disprotect;
static struct meson_rsv_handler_t *rsv_handler;

int rsvname2index(const char *rsv_name)
{
	struct meson_rsv_info_t *rsv_info = rsv_handler->rsv_info;
	int i, len = get_mtd_rsv_partition_count();

	for (i = 0; i < len; i++)
		if (!strncmp(rsv_name, rsv_info[i].name, 4) ||
		    !strncmp(rsv_name, &rsv_info[i].name[1], 3))
			return i;
	return -1;
}

u32 meson_rsv_part_get_bl2_part_size(struct mtd_info *mtd)
{
	static u32 bl2_part_size = 0;

	if (bl2_part_size)
		return bl2_part_size;

	if (BOARD_CONFIG_BL2_LAYOUT_TYPE == BL2_LAYOUT_1024)
		bl2_part_size = 1024 * mtd->writesize;
	else if (BOARD_CONFIG_BL2_LAYOUT_TYPE == BL2_LAYOUT_512)
		bl2_part_size = 512 * mtd->writesize;
	else {
		bl2_part_size = round_up(BL2_SIZE, mtd->erasesize);
		bl2_part_size *= CONFIG_BL2_COPY_NUM;
	}

	return bl2_part_size;
}

u32 meson_rsv_part_get_start_block(struct mtd_info *mtd)
{
	u32 bl2_part_size;
	static u32 rev_start_block = 0;

	if (rev_start_block)
		return rev_start_block;

	bl2_part_size = meson_rsv_part_get_bl2_part_size(mtd);
	rev_start_block = bl2_part_size / mtd->erasesize;
	return rev_start_block;
}

static u32 meson_rsv_part_get_total_blocks(struct rsv_part *rsv_part)
{
	int array_size, i;
	static u32 blocks = 0;

	if (blocks)
		return blocks;

	array_size = get_mtd_rsv_partition_count();
	for (i = 0; i < array_size; i++)
		blocks += rsv_part[i].blocks;

	if (blocks > MTD_RSV_BLOCK_CNT)
		printf("mismatch between calculation(%d) and defination(%d)\n",
		        blocks, MTD_RSV_BLOCK_CNT);

	return blocks;
}

u64 meson_rsv_part_get_tpl_start(struct mtd_info *mtd)
{
	u64 reserved_start_block, tpl_start, reserved_total_blocks;

	reserved_start_block = meson_rsv_part_get_start_block(mtd);
	reserved_total_blocks = meson_rsv_part_get_total_blocks(rsv_handler->rsv_part);
	tpl_start = (u64)(reserved_start_block + reserved_total_blocks) * mtd->erasesize;

	return tpl_start;
}

u64 meson_rsv_part_get_tpl_size(struct mtd_info *mtd)
{
	return CONFIG_TPL_SIZE_PER_COPY * CONFIG_NAND_TPL_COPY_NUM;
}

static struct rsv_part *meson_rsv_info_calculate_start_block(struct mtd_info *mtd)
{
	struct rsv_part *rsv_part, *rsv_partitions;
	int i = 0, array_size;
	u32 bbt_start = BBT_START_BLOCK;
	u32 bbt_end = BBT_START_BLOCK + BBT_TOTAL_BLOCKS;
	u32 cur_part_end;

	array_size = get_mtd_rsv_partition_count();
	if (array_size > MAX_MESON_RSV_INFO_NUM)
		return NULL;

	rsv_part = kzalloc(sizeof(*rsv_part) * array_size, GFP_KERNEL);
	if (!rsv_part)
		return NULL;

	/* insert bbt  and always put it 1st */
	rsv_part[0].start_block = BBT_START_BLOCK;
	rsv_part[0].blocks = BBT_TOTAL_BLOCKS;
	memcpy(rsv_part[0].name, BBT_NAND_MAGIC, 4);

	rsv_partitions = get_mtd_rsv_partition();
	rsv_partitions[0].start_block = meson_rsv_part_get_start_block(mtd);
	for (i = 0; i < array_size - 1; i++) {
		cur_part_end = rsv_partitions[i].start_block +
				     rsv_partitions[i].blocks;

		if (cur_part_end >= bbt_start &&
		    bbt_end > rsv_partitions[i].start_block)
			cur_part_end += BBT_TOTAL_BLOCKS;

		rsv_partitions[i + 1].start_block = cur_part_end;
	}
	memcpy(&rsv_part[1], rsv_partitions,
	       (array_size - 1) * sizeof(struct rsv_part));

	return rsv_part;
}

static struct free_node_t *get_free_node(struct meson_rsv_info_t *rsv_info)
{
	struct meson_rsv_handler_t *handler = rsv_info->handler;
	u32 index;

	index = find_first_zero_bit((void *)&handler->fn_bitmask,
				    MTD_RSV_BLOCK_CNT);
	if (index >= MTD_RSV_BLOCK_CNT) {
		pr_info("%s %d index :%d is greater than max rsv block num\n",
			__func__, __LINE__, index);
		return NULL;
	}
	WARN_ON(test_and_set_bit(index, (void *)&handler->fn_bitmask));

	return handler->free_node[index];
}

static void release_free_node(struct meson_rsv_info_t *rsv_info,
			      struct free_node_t *free_node)
{
	struct meson_rsv_handler_t *handler = rsv_info->handler;
	u32 index = free_node->index;

	pr_info("%s %d: bitmask = 0x%llx\n",
		__func__, __LINE__, handler->fn_bitmask);
	if (index >= MTD_RSV_BLOCK_CNT) {
		pr_info("%s %d index :%d is greater than max rsv block num\n",
			__func__, __LINE__, index);
		return;
	}
	WARN_ON(!test_and_clear_bit(index, (void *)&handler->fn_bitmask));
	memset(free_node, 0, sizeof(struct free_node_t));
	free_node->index = index;
	pr_info("%s %d: bitmask = 0x%llx\n",
		__func__, __LINE__, handler->fn_bitmask);
}

static inline void menson_rsv_disprotect(void)
{
	/*disprotect*/
	info_disprotect |= DISPROTECT_KEY;
	info_disprotect |= DISPROTECT_FBBT;
}
static inline void menson_rsv_protect(void)
{
	/*protect*/
	info_disprotect &= ~DISPROTECT_KEY;
	info_disprotect &= ~DISPROTECT_FBBT;

}

static inline u32 is_rsv_block(struct meson_rsv_info_t *rsv_info, u32 block)
{
	struct free_node_t *temp_node = rsv_info->nfree;

	if (block == rsv_info->nvalid->blk_addr)
		return 1;

	while (temp_node) {
		if (block == temp_node->blk_addr)
			return 1;
		temp_node = temp_node->next;
	}
	return 0;
};

int meson_rsv_erase_protect(struct meson_rsv_handler_t *handler,
			    u32 block_addr)
{
	struct meson_rsv_info_t *rsv_info;
	static u32 rsv_end;
	int index = rsvname2index(KEY_NAND_MAGIC);

	if (index < 0)
		return -1;

	if (!rsv_end) {
		rsv_end = meson_rsv_part_get_start_block(handler->mtd) +
		meson_rsv_part_get_total_blocks(handler->rsv_part);
	}

	if (block_addr > rsv_end)
		return 0;

	rsv_info = &handler->rsv_info[index];
	if (rsv_info->valid) {
		if (!(info_disprotect & DISPROTECT_KEY) &&
			 is_rsv_block(rsv_info, block_addr))
			return -1;
	}

	index = rsvname2index(BBT_NAND_MAGIC);
	if (index < 0)
		return -1;

	rsv_info = &handler->rsv_info[index];
	if (rsv_info->valid) {
		if (!(info_disprotect & DISPROTECT_FBBT) &&
		    is_rsv_block(rsv_info, block_addr))
			return -1;
	}
	return 0;
}

int meson_rsv_free(struct meson_rsv_info_t *rsv_info)
{
	struct mtd_info *mtd;
	struct free_node_t *tmp_node, *next_node = NULL;
	int error = 0;
	loff_t addr = 0;
	struct erase_info erase_info;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
	pr_info("free %s\n", rsv_info->name);

	if (rsv_info->valid) {
		addr = rsv_info->nvalid->blk_addr;
		addr *= mtd->erasesize;
		memset(&erase_info, 0, sizeof(struct erase_info));
		erase_info.mtd = mtd;
		erase_info.addr = addr;
		erase_info.len = mtd->erasesize;
		menson_rsv_disprotect();
		error = mtd_erase(mtd, &erase_info);
		menson_rsv_protect();
		pr_info("erasing valid info block: %llx\n", addr);
		rsv_info->nvalid->blk_addr = -1;
		rsv_info->nvalid->ec = -1;
		rsv_info->nvalid->page_addr = 0;
		rsv_info->nvalid->timestamp = 0;
		rsv_info->nvalid->status = 0;
		rsv_info->valid = 0;
	}
	tmp_node = rsv_info->nfree;
	while (tmp_node) {
		next_node = tmp_node->next;
		release_free_node(rsv_info, tmp_node);
		tmp_node = next_node;
	}
	rsv_info->nfree = NULL;

	return error;
}

int meson_rsv_save(struct meson_rsv_info_t *rsv_info, u_char *buf)
{
	struct mtd_info *mtd = rsv_info->mtd;
	struct free_node_t *free_node, *temp_node;
	struct erase_info erase_info;
	int ret = 0, i = 1, pages_per_blk;
	loff_t offset = 0;

	pages_per_blk = 1 << (mtd->erasesize_shift - mtd->writesize_shift);
	if ((rsv_info->nvalid->status & POWER_ABNORMAL_FLAG) ||
	    (rsv_info->nvalid->status & ECC_ABNORMAL_FLAG))
		rsv_info->nvalid->page_addr = pages_per_blk;
	if (mtd->writesize < rsv_info->size)
		i = (rsv_info->size + mtd->writesize - 1) / mtd->writesize;
RE_SEARCH:
	if (rsv_info->valid) {
		rsv_info->nvalid->page_addr += i;
		if ((rsv_info->nvalid->page_addr + i) > pages_per_blk) {
			if ((rsv_info->nvalid->page_addr - i) ==
				pages_per_blk) {
				offset = rsv_info->nvalid->blk_addr;
				offset *= mtd->erasesize;
				erase_info.mtd = mtd;
				erase_info.addr = offset;
				erase_info.len = mtd->erasesize;
				menson_rsv_disprotect();
				mtd_erase(mtd, &erase_info);
				menson_rsv_protect();
				rsv_info->nvalid->ec++;
				pr_info("%s %d: erasing bad info block:0x%llx\n",
					__func__, __LINE__, offset);
			}
			free_node = get_free_node(rsv_info);
			if (!free_node)
				return -ENOMEM;
			/* set current valid node to free list */
			free_node->blk_addr = rsv_info->nvalid->blk_addr;
			free_node->ec = rsv_info->nvalid->ec;
			temp_node = rsv_info->nfree;
			while (temp_node->next)
				temp_node = temp_node->next;
			temp_node->next = free_node;
			/* get one node from free list and set to current */
			temp_node = rsv_info->nfree;
			rsv_info->nvalid->blk_addr = temp_node->blk_addr;
			rsv_info->nvalid->page_addr = 0;
			rsv_info->nvalid->ec = temp_node->ec;
			rsv_info->nvalid->timestamp++;
			rsv_info->nfree = temp_node->next;
			release_free_node(rsv_info, temp_node);
		}
	} else {
		temp_node = rsv_info->nfree;
		rsv_info->nvalid->blk_addr = temp_node->blk_addr;
		rsv_info->nvalid->page_addr = 0;
		rsv_info->nvalid->ec = temp_node->ec;
		rsv_info->nvalid->timestamp++;
		rsv_info->nfree = temp_node->next;
		release_free_node(rsv_info, temp_node);
	}
	offset = rsv_info->nvalid->blk_addr;
	offset *= mtd->erasesize;
	offset += ((u64)rsv_info->nvalid->page_addr) * mtd->writesize;
	if (rsv_info->nvalid->page_addr == 0) {
		ret = mtd_block_isbad(mtd, offset);
		if (ret) {
			/**
			 * cause our rsv list includes bad block,
			 * so we need check it here and for fear
			 * of data lost.
			 */
			printf("%s %d: %s bad block here 0x%llx\n",
				__func__, __LINE__, rsv_info->name, offset);
			rsv_info->nvalid->page_addr = pages_per_blk - i;
			goto RE_SEARCH;
		}
		memset(&erase_info, 0, sizeof(struct erase_info));
		erase_info.mtd = mtd;
		erase_info.addr = offset;
		erase_info.len = mtd->erasesize;
		menson_rsv_disprotect();
		ret = mtd_erase(mtd, &erase_info);
		menson_rsv_protect();
		if (ret) {
			printf("%s %d %s erase failed at 0x%llx ,mark it bad\n",
				__func__, __LINE__, rsv_info->name, offset);
			mtd_block_markbad(mtd, offset);
			//return ret;
			rsv_info->nvalid->page_addr = pages_per_blk;
			goto RE_SEARCH;
		}
		rsv_info->nvalid->ec++;
	}
	ret = meson_rsv_write(rsv_info, buf);
	if (ret) {
		pr_info("%s %d rsv info: %s save failed!\n",
			__func__, __LINE__, rsv_info->name);
		return ret;
	}
	rsv_info->valid = 1;
	rsv_info->nvalid->status = 0;
	return ret;
}

int meson_rsv_write(struct meson_rsv_info_t *rsv_info, u_char *buf)
{
	struct mtd_info *mtd;
	struct oobinfo_t oobinfo;
	struct mtd_oob_ops oob_ops;
	size_t length = 0;
	loff_t offset;
	int ret = 0;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
	offset = rsv_info->nvalid->blk_addr;
	offset *= mtd->erasesize;
	offset += ((u64)rsv_info->nvalid->page_addr) * mtd->writesize;
	pr_info("%s %d write %s to 0x%llx\n",
		__func__, __LINE__, rsv_info->name, offset);
	memcpy(oobinfo.name, rsv_info->name, 4);
	oobinfo.ec = rsv_info->nvalid->ec;
	/* TODO: prevent the unrolling situation here */
	oobinfo.timestamp = rsv_info->nvalid->timestamp;
	while (length < rsv_info->size) {
		oob_ops.mode = MTD_OPS_AUTO_OOB;
		oob_ops.len = min_t(u32, mtd->writesize,
				    (rsv_info->size - length));
		oob_ops.ooblen = sizeof(struct oobinfo_t);
		oob_ops.ooboffs = 0;
		oob_ops.datbuf = buf + length;
		oob_ops.oobbuf = (u8 *)&oobinfo;
		ret = mtd_write_oob(mtd, offset, &oob_ops);
		if (ret) {
			pr_info("fail to write %s to 0x%llx ret:%d\n",
				rsv_info->name, offset, ret);
			return -EIO;
		}
		offset += mtd->writesize;
		length += oob_ops.len;
	}
	return ret;
}

int meson_rsv_read(struct meson_rsv_info_t *rsv_info, u_char *buf)
{
	struct mtd_info *mtd;
	struct oobinfo_t oobinfo;
	struct mtd_oob_ops oob_ops;
	size_t length = 0;
	loff_t offset;
	int ret = 0;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
READ_RSV_AGAIN:
	offset = rsv_info->nvalid->blk_addr;
	offset *= mtd->erasesize;
	offset += ((u64)rsv_info->nvalid->page_addr) * mtd->writesize;
	pr_info("%s %d read %s from 0x%llx\n",
		__func__, __LINE__, rsv_info->name, offset);
	memset(buf, 0, rsv_info->size);
	while (length < rsv_info->size) {
		oob_ops.mode = MTD_OPS_AUTO_OOB;
		oob_ops.len = min_t(u32, mtd->writesize,
				    (rsv_info->size - length));
		oob_ops.ooblen = sizeof(struct oobinfo_t);
		oob_ops.ooboffs = 0;
		oob_ops.datbuf = buf + length;
		oob_ops.oobbuf = (u8 *)&oobinfo;

		memset((u8 *)&oobinfo, 0, oob_ops.ooblen);
		ret = mtd_read_oob(mtd, offset, &oob_ops);
		if (ret && (ret != -EUCLEAN)) {
			pr_info("blk good but read failed: %llx, %d\n",
				(u64)offset, ret);
			ret = meson_rsv_scan(rsv_info);
			if (ret)
				return -EIO;
			goto READ_RSV_AGAIN;
		}
		/* Do not use strlen ,Use ARRAY_SIZE to make the length 4 */
		if (memcmp(oobinfo.name, rsv_info->name,
			   4))
			pr_info("invalid %s info in %llx:%s\n",
				rsv_info->name, offset, oobinfo.name);
		offset += mtd->writesize;
		length += oob_ops.len;
	}
	return ret;
}

int meson_rsv_erase(struct meson_rsv_info_t *rsv_info)
{
	struct mtd_info *mtd;
	struct free_node_t *free_node, *temp_node = NULL;
	int ret = 0;
	struct erase_info erase_info;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
	pr_info("%s %d rsv erasing %s\n",
			__func__, __LINE__, rsv_info->name);

	if (rsv_info->valid) {
		rsv_info->nvalid->ec++;
		rsv_info->nvalid->page_addr = -1;
		rsv_info->nvalid->timestamp = 1;
		rsv_info->valid = 0;

		free_node = get_free_node(rsv_info);
		if (!free_node)
			return -ENOMEM;
		/* set current valid node to free list */
		free_node->blk_addr = rsv_info->nvalid->blk_addr;
		free_node->ec = rsv_info->nvalid->ec;
		temp_node = rsv_info->nfree;
		while (temp_node->next)
			temp_node = temp_node->next;
		temp_node->next = free_node;
	}

	temp_node = rsv_info->nfree;
	while (temp_node) {
		memset(&erase_info, 0, sizeof(struct erase_info));
		erase_info.mtd = mtd;
		erase_info.addr = temp_node->blk_addr* mtd->erasesize;
		erase_info.len = mtd->erasesize;
		menson_rsv_disprotect();
		ret = mtd_erase(mtd, &erase_info);
		menson_rsv_protect();
		printk("erasing valid info block: %llx \n", erase_info.addr);
		rsv_info->nvalid->ec = -1;
		temp_node->dirty_flag = 0;
		temp_node = temp_node->next;
	}
	return ret;
}

static u32 skip_bbt_blocks(struct meson_rsv_info_t *rsv_info, u32 start)
{
	if (memcmp(BBT_NAND_MAGIC, rsv_info->name, 4))
		if (start >= BBT_START_BLOCK &&
		    start < BBT_START_BLOCK + BBT_TOTAL_BLOCKS)
			return start + BBT_TOTAL_BLOCKS;
	return start;
}

int meson_rsv_scan(struct meson_rsv_info_t *rsv_info)
{
	struct mtd_info *mtd = rsv_info->mtd;
	struct mtd_oob_ops oob_ops;
	struct oobinfo_t oobinfo;
	struct free_node_t *free_node, *temp_node;
	loff_t offset;
	u32 start, end;
	int ret = 0, error, rsv_status, i, k;
	u8 scan_status;
	u8 good_addr[256] = {0};
	u32 page_num, pages_per_blk;

RE_RSV_INFO_EXT:
	start = rsv_info->start;
	end = rsv_info->end;
	do {
		offset = skip_bbt_blocks(rsv_info, start);
		offset *= mtd->erasesize;
		scan_status = 0;
RE_RSV_INFO:
		oob_ops.mode = MTD_OPS_AUTO_OOB;
		oob_ops.len = 0;
		oob_ops.ooblen = sizeof(struct oobinfo_t);
		oob_ops.ooboffs = 0;
		oob_ops.datbuf = NULL;
		oob_ops.oobbuf = (u8 *)&oobinfo;
		memset((u8 *)&oobinfo, 0, sizeof(struct oobinfo_t));
		error = mtd_read_oob(mtd, offset, &oob_ops);
		if (error && (error != -EUCLEAN)) {
			pr_info("%s %d blk check good but read failed: %llx, %d\n",
				__func__, __LINE__, (u64)offset, error);
			offset += rsv_info->size;
			if ((scan_status++ > 6) ||
			    (!(offset % mtd->erasesize))) {
				pr_info("ECC error, scan ONE block exit\n");
				scan_status = 0;
				continue;
			}
			goto RE_RSV_INFO;
		}
		rsv_info->init = 1;
		rsv_info->nvalid->status = 0;
		/* Do not use strlen ,Use ARRAY_SIZE to make the length 4 */
		if (!memcmp(oobinfo.name, rsv_info->name, 4)) {
			rsv_info->valid = 1;
			if (rsv_info->nvalid->blk_addr >= 0) {
				free_node = get_free_node(rsv_info);
				if (!free_node)
					return -ENOMEM;
				free_node->dirty_flag = 1;
				if (oobinfo.timestamp >
				    rsv_info->nvalid->timestamp) {
					free_node->blk_addr =
						rsv_info->nvalid->blk_addr;
					free_node->ec = rsv_info->nvalid->ec;
					rsv_info->nvalid->blk_addr =
								skip_bbt_blocks(rsv_info, start);
					rsv_info->nvalid->page_addr = 0;
					rsv_info->nvalid->ec = oobinfo.ec;
					rsv_info->nvalid->timestamp =
						oobinfo.timestamp;
				} else {
					free_node->blk_addr = skip_bbt_blocks(rsv_info, start);
					free_node->ec = oobinfo.ec;
				}
				if (!rsv_info->nfree) {
					rsv_info->nfree = free_node;
				} else {
					temp_node = rsv_info->nfree;
					while (temp_node->next)
						temp_node = temp_node->next;
					temp_node->next = free_node;
				}
			} else {
				rsv_info->nvalid->blk_addr = skip_bbt_blocks(rsv_info, start);
				rsv_info->nvalid->page_addr = 0;
				rsv_info->nvalid->ec = oobinfo.ec;
				rsv_info->nvalid->timestamp = oobinfo.timestamp;
			}
		} else {
			free_node = get_free_node(rsv_info);
			if (!free_node)
				return -ENOMEM;
			free_node->blk_addr = skip_bbt_blocks(rsv_info, start);
			free_node->ec = oobinfo.ec;
			if (!rsv_info->nfree) {
				rsv_info->nfree = free_node;
			} else {
				temp_node = rsv_info->nfree;
				while (temp_node->next)
					temp_node = temp_node->next;
				temp_node->next = free_node;
			}
		}
	} while ((++start) < end);

	printf("%s blk = %d, ec = %d, page = %d, timestamp = %d\n",
			rsv_info->name, rsv_info->nvalid->blk_addr, rsv_info->nvalid->ec,
			rsv_info->nvalid->page_addr, rsv_info->nvalid->timestamp);
	printf("%s free list:\n", rsv_info->name);
	temp_node = rsv_info->nfree;
	while (temp_node) {
		printf("block num = %d, ec = %d, dirty_flag = %d\n",
			temp_node->blk_addr,
			temp_node->ec,
			temp_node->dirty_flag);
		temp_node = temp_node->next;
	}
	/**
	 * step 2, find the newest in the block
	 * watch out here, cause erase size and write size must be
	 * power of 2, and write size must equal page size.
	 */
	pages_per_blk = 1 << (mtd->erasesize_shift - mtd->writesize_shift);
	page_num = rsv_info->size >> mtd->writesize_shift;
	if (!page_num)
		page_num++;
	if (rsv_info->valid == 1) {
		pr_info("%s %d selecting in block: %d\n",
			__func__, __LINE__, rsv_info->nvalid->blk_addr);
		oob_ops.mode = MTD_OPS_AUTO_OOB;
		oob_ops.len = 0;
		oob_ops.ooblen = sizeof(struct oobinfo_t);
		oob_ops.ooboffs = 0;
		oob_ops.datbuf = NULL;
		oob_ops.oobbuf = (u8 *)&oobinfo;
		for (i = 0; i < pages_per_blk; i++) {
			memset((u8 *)&oobinfo, 0, oob_ops.ooblen);
			offset = rsv_info->nvalid->blk_addr;
			offset *= mtd->erasesize;
			offset += ((u64)mtd->writesize) * i;
			error = mtd_read_oob(mtd, offset, &oob_ops);
			if (error && error != -EUCLEAN) {
				pr_info("%s %d blk good but read failed:%llx, %d\n",
					__func__, __LINE__, (u64)offset, error);
				rsv_info->nvalid->status |= ECC_ABNORMAL_FLAG;
				ret = -1;
				continue;
			}
			/* Do not use strlen ,Use ARRAY_SIZE to make the length 4 */
			if (!memcmp(oobinfo.name, rsv_info->name,
				    4)) {
				good_addr[i] = 1;
				rsv_info->nvalid->page_addr = i;
			} else {
				break;
			}
		}
	}
	if (mtd->writesize < rsv_info->size &&
	    rsv_info->valid == 1) {
		i = rsv_info->nvalid->page_addr;
		if (((i + 1) % page_num) != 0) {
			ret = -1;
			rsv_info->nvalid->status |= POWER_ABNORMAL_FLAG;
			pr_info("find %s incomplete\n", rsv_info->name);
		}
		pr_info("%s %d page_num %d\n", __func__, __LINE__, page_num);
		if (ret == -1) {
			for (i = 0; i < (pages_per_blk / page_num); i++) {
				rsv_status = 0;
				for (k = 0; k < page_num; k++) {
					if (!good_addr[k + i * page_num]) {
						rsv_status = 1;
						break;
					}
				}
				if (!rsv_status) {
					pr_info("find %d page ok\n",
						i * page_num);
					rsv_info->nvalid->page_addr =
						k + i * page_num - 1;
					ret = 0;
				}
			}
		}
		if (ret == -1) {
			rsv_info->nvalid->status = 0;
			meson_rsv_free(rsv_info);
			goto RE_RSV_INFO_EXT;
		}
		i = (rsv_info->size + mtd->writesize - 1) / mtd->writesize;
		rsv_info->nvalid->page_addr -= (i - 1);
	}
	if (rsv_info->valid != 1)
		ret = -1;
	offset = rsv_info->nvalid->blk_addr;
	offset *= mtd->erasesize;
	offset += ((u64)rsv_info->nvalid->page_addr) * mtd->writesize;
	pr_info("%s valid address 0x%llx\n", rsv_info->name, offset);
	return ret;
}

int meson_rsv_check(struct meson_rsv_info_t *rsv_info)
{
	int ret = 0;

	ret = meson_rsv_scan(rsv_info);
	if (ret)
		pr_info("%s %d %s info check failed ret %d\n",
			__func__, __LINE__, rsv_info->name, ret);
	if (!rsv_info->valid) {
		pr_info("%s %d no %s info exist\n",
			__func__, __LINE__, rsv_info->name);
		ret = 1;
	}
	return ret;
}

void meson_rsv_check_all_except_bbt(void)
{
	struct meson_rsv_info_t *rsv_info;
	int i;

	for (i = 1; i < rsv_handler->entries; i++) {
		rsv_info = &rsv_handler->rsv_info[i];
		meson_rsv_check(rsv_info);
	}
}

int meson_rsv_check_bbt(void)
{
	struct meson_rsv_info_t *rsv_info;
	int index = rsvname2index(BBT_NAND_MAGIC);

	if (index < 0)
		return -1;

	rsv_info = &rsv_handler->rsv_info[index];
	return meson_rsv_check(rsv_info);
}

int meson_rsv_save_bbt(u8 *bbt)
{
	struct meson_rsv_info_t *rsv_info;
	int index = rsvname2index(BBT_NAND_MAGIC);

	if (index < 0)
		return -1;

	rsv_info = &rsv_handler->rsv_info[index];
	return meson_rsv_save(rsv_info, bbt);
}

int meson_rsv_read_bbt(u8 *bbt)
{
	struct meson_rsv_info_t *rsv_info;
	int index = rsvname2index(BBT_NAND_MAGIC);

	if (index < 0)
		return -1;

	rsv_info = &rsv_handler->rsv_info[index];
	return meson_rsv_read(rsv_info, bbt);
}

static int aml_nand_rsv_info_alloc_init(struct mtd_info *mtd,
					struct meson_rsv_handler_t *handler,
					struct rsv_part *rsv_part)
{
	struct meson_rsv_info_t *rsv_info = &handler->rsv_info[handler->entries++];

	rsv_info->nvalid =
		kzalloc(sizeof(struct valid_node_t), GFP_KERNEL);
	if (!rsv_info->nvalid)
		return -ENOMEM;

	rsv_info->mtd = mtd;
	rsv_info->start = rsv_part->start_block;
	rsv_info->end = rsv_part->start_block + rsv_part->blocks;
	rsv_info->nvalid->blk_addr = -1;
	rsv_info->handler = handler;
	if (!memcmp(rsv_part->name, BBT_NAND_MAGIC, 4))
		rsv_info->size = mtd->size >> mtd->erasesize_shift;
	else
		rsv_info->size = rsv_part->size;

	memcpy(rsv_info->name, rsv_part->name, 4);
	rsv_info->name[4] = '\0';

	printf("%s init : start(end) = %d(%d) %p\n",
		rsv_info->name, rsv_info->start, rsv_info->end, rsv_info);
	return 0;
}

int meson_rsv_init(struct mtd_info *mtd,
		   struct meson_rsv_handler_t *handler)
{
	struct rsv_part *rsv_part;
	struct free_node_t *ptr;
	int i, ret = 0, array_size;
	u32 rsv_blocks;

	rsv_part = meson_rsv_info_calculate_start_block(mtd);
	if (!rsv_part)
		return -ENOMEM;

	rsv_handler = handler;
	rsv_handler->rsv_part = rsv_part;
	handler->entries = 0;

	rsv_blocks = meson_rsv_part_get_total_blocks(rsv_part);
	handler->free_node = kzalloc(rsv_blocks * sizeof(struct free_node_t *), GFP_KERNEL);
	if (!handler->free_node)
		return -ENOMEM;

	ptr = kzalloc(sizeof(struct free_node_t) * rsv_blocks, GFP_KERNEL);
	if (!ptr) {
		kfree(handler->free_node);
		return -ENOMEM;
	}

	handler->fn_bitmask = 0;
	for (i = 0; i < rsv_blocks; i++, ptr++) {
		handler->free_node[i] = ptr;
		handler->free_node[i]->index = i;
	}

	array_size = get_mtd_rsv_partition_count();
	for (i = 0; i < array_size; i++, rsv_part++) {
		if (!rsv_part->blocks)
			continue;
		ret = aml_nand_rsv_info_alloc_init(mtd, handler, rsv_part);
		if (ret)
			break;
	}

	for (; ret && handler->entries > 0; --handler->entries)
		kfree(handler->rsv_info[handler->entries].nvalid);

	if (ret)
		kfree(rsv_handler->rsv_part);

	return ret;
}

int meson_modify_page_info_and_save(struct mtd_info *mtd)
{
	u64 bl2_mem, bl2_size = BL2_SIZE;
	int ret, i;
	u_char *bl2_buf;
	char str[128];

	bl2_buf = malloc(bl2_size);
	if (!bl2_buf)
		return -ENOMEM;

	bl2_mem = (u64)bl2_buf;
	for (i = 0; i < 4; i++) {
		sprintf(str, "store boot_read bl2 0x%llx %d 0x%llx", bl2_mem, i, bl2_size);
		printf("command:    %s\n", str);
		ret = run_command(str, 0);
		if (ret)
			goto _err_modify_page_info;

		sprintf(str, "store boot_erase bl2 %d", i);
		printf("command:    %s\n", str);
		ret = run_command(str, 0);
		if (ret)
			goto _err_modify_page_info;

		sprintf(str, "store boot_write bl2 0x%llx %d 0x%llx", bl2_mem, i, bl2_size);
		printf("command:    %s\n", str);
		ret = run_command(str, 0);
		if (ret)
			goto _err_modify_page_info;
	}

_err_modify_page_info:
	free(bl2_buf);
	return ret;
}

int meson_ext_rsv_info_read(u_char *dest, size_t size, int index)
{
	struct meson_rsv_info_t *rsv_info;
	u_char *temp;
	int ret;

	rsv_info = &rsv_handler->rsv_info[index];
	if (!rsv_info->size || !rsv_info->valid) {
		pr_info("%s %d: not inited yet!\n", __func__, __LINE__);
		return RSV_INVALID;
	}

	temp = kzalloc(rsv_info->size, GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	ret = meson_rsv_read(rsv_info, temp);
	memcpy(dest, temp, rsv_info->size > size ? size : rsv_info->size);

	kfree(temp);
	return ret;
}

int meson_ext_rsv_info_write(u_char *source, size_t size, int index)
{
	struct meson_rsv_info_t *rsv_info;
	u_char *temp;
	int ret;

	rsv_info = &rsv_handler->rsv_info[index];
	if (!rsv_info->size) {
		pr_info("%s %d: not inited yet!\n", __func__, __LINE__);
		return RSV_INVALID;
	}

	temp = kzalloc(rsv_info->size, GFP_KERNEL);
	if (!temp)
		return -ENOMEM;

	memset(temp, 0, rsv_info->size);
	memcpy(temp, source, rsv_info->size > size ? size : rsv_info->size);
	ret = meson_rsv_save(rsv_info, temp);
	kfree(temp);

	if (IS_ENABLED(CONFIG_DDR_PARAMETER_SUPPORT) && IS_ENABLED(CONFIG_MTD_SPI_NAND))
		ret = meson_modify_page_info_and_save(rsv_info->mtd);

	return ret;
}

u32 meson_ext_rsv_info_size(int index)
{
	return rsv_handler->rsv_info[index].size;
}

int meson_ext_rsv_info_erase(int index)
{
	struct meson_rsv_info_t *rsv_info;

	rsv_info = &rsv_handler->rsv_info[index];
	if (rsv_info->valid)
		return meson_rsv_erase(rsv_info);
	return 0;
}

static int meson_rsv_fdt_setprop(void *fdt, int node_offset,
		const char *name, const void *val, bool string)
{
	int err = 0;

add:
	if (string) {
		err = fdt_setprop_string(fdt, node_offset, name, val);
	} else {
		u32 *tmp = (u32 *)val;

		err = fdt_setprop_cell(fdt, node_offset, name, *tmp);
	}
	if (err == -FDT_ERR_NOSPACE) {
		err = fdt_increase_size(fdt, 512);
		if (!err)
			goto add;
		else
			goto err_size;
	}

	return err;

err_size:
	pr_err("Can't increase blob size: %s\n", fdt_strerror(err));
	return err;
}

static int meson_rsv_add_node(void *blob, int parent_offset, int index,
		const char *name, u32 block_start, u32 block_cnt, u32 size)
{
	int err = 0, node_offset = 0;
	char buf[64];

	pr_debug("%s : add node%d %s block start %u block cnt %u size %u\n",
			__func__, index, name, block_start, block_cnt, size);
add_node:
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "node%d", index);
	node_offset = fdt_add_subnode(blob, parent_offset, buf);
	if (node_offset == -FDT_ERR_NOSPACE) {
		err = fdt_increase_size(blob, 512);
		if (!err)
			goto add_node;
		else
			goto err_size;
	} else if (node_offset == -FDT_ERR_EXISTS) {
		return 0;
	} else if (node_offset < 0) {
		printf("Can't add mtdrsvpart node: %s\n",
				fdt_strerror(node_offset));
		return node_offset;
	}

	err = meson_rsv_fdt_setprop(blob, node_offset, "size", &size, 0);
	if (err)
		goto err_prop;

	err = meson_rsv_fdt_setprop(blob, node_offset, "block_cnt",
			&block_cnt, 0);
	if (err)
		goto err_prop;

	err = meson_rsv_fdt_setprop(blob, node_offset, "block_start",
			&block_start, 0);
	if (err)
		goto err_prop;

	err = meson_rsv_fdt_setprop(blob, node_offset, "label", name, 1);
	if (err)
		goto err_prop;

	return 0;

err_size:
	pr_err("Can't increase blob size: %s\n", fdt_strerror(err));

err_prop:
	pr_err("Can't add property: %s\n", fdt_strerror(err));
	return err;
}

int meson_rsv_add_dtb(void *blob, int parent_offset)
{
	struct meson_rsv_info_t *rsv_info;
	int rsvparts_offset, ret = 0, idx = 0, array_size;
	char buf[64];

	sprintf(buf, "rsv_partition");
add_rsvparts:
	rsvparts_offset = fdt_add_subnode(blob, parent_offset, buf);
	if (rsvparts_offset == -FDT_ERR_NOSPACE) {
		ret = fdt_increase_size(blob, 512);
		if (!ret) {
			goto add_rsvparts;
		} else {
			pr_err("Can't increase blob size: %s\n", fdt_strerror(ret));
			return ret;
		}
	} else if (rsvparts_offset == -FDT_ERR_EXISTS) {
		return 0;
	} else if (rsvparts_offset < 0) {
		pr_err("Can't add mtdrsvpart node: %s\n",
				fdt_strerror(rsvparts_offset));
		return rsvparts_offset;
	}

	ret = meson_rsv_add_node(blob, rsvparts_offset, idx, "nrsv",
				 meson_rsv_part_get_start_block(rsv_handler->mtd),
				 meson_rsv_part_get_total_blocks(rsv_handler->rsv_part),
				 0);
	if (ret < 0)
		return ret;

	array_size = get_mtd_rsv_partition_count();
	for (rsv_info = rsv_handler->rsv_info;
	     idx < array_size; rsv_info++, idx++) {
		ret = meson_rsv_add_node(blob,
				rsvparts_offset,
				idx + 1,
				rsv_info->name,
				rsv_info->start,
				rsv_info->end - rsv_info->start,
				rsv_info->size);
		if (ret < 0)
			return ret;
	}

	return 0;
}
