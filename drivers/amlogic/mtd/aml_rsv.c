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

extern int info_disprotect;
static struct meson_rsv_handler_t *rsv_handler;

struct rsv_info rsv_board_info[] = {
	INFO_DATA(BBT_NAND_MAGIC, MTD_RSV_BBT_BLOCK_CNT, 0),
	INFO_DATA(ENV_NAND_MAGIC, MTD_RSV_ENV_BLOCK_CNT, CONFIG_ENV_SIZE),
	INFO_DATA(KEY_NAND_MAGIC, MTD_RSV_KEY_BLOCK_CNT, MTD_RSV_KEY_SIZE),
	INFO_DATA(DTB_NAND_MAGIC, MTD_RSV_DTB_BLOCK_CNT, MTD_RSV_DTB_SIZE),
	INFO_DATA(DDR_NAND_MAGIC, MTD_RSV_DDR_BLOCK_CNT, MTD_RSV_DDR_SIZE),
};

static struct free_node_t *get_free_node(struct meson_rsv_info_t *rsv_info)
{
	struct meson_rsv_handler_t *handler = rsv_info->handler;
	u32 index;

	index =
		find_first_zero_bit((void *)&handler->fn_bitmask,
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

int meson_rsv_erase_protect(struct meson_rsv_handler_t *handler,
			    u32 block_addr)
{
	if (handler->key && handler->key->valid) {
		if (!(info_disprotect & DISPROTECT_KEY) &&
		    block_addr >= handler->key->start &&
		    block_addr < handler->key->end)
			return -1;
	}
	if (handler->bbt && handler->bbt->valid) {
		if ((!(info_disprotect & DISPROTECT_FBBT)) &&
			(block_addr >= handler->bbt->start) &&
			(block_addr < handler->bbt->end))
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
	struct mtd_info *mtd;
	struct free_node_t *free_node, *temp_node;
	struct erase_info erase_info;
	int ret = 0, i = 1, pages_per_blk;
	loff_t offset = 0;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
	pages_per_blk = 1 << (mtd->erasesize_shift - mtd->writesize_shift);
	if ((rsv_info->nvalid->status & POWER_ABNORMAL_FLAG) ||
	    (rsv_info->nvalid->status & ECC_ABNORMAL_FLAG))
		rsv_info->nvalid->page_addr = pages_per_blk;
	if (mtd->writesize < rsv_info->size)
		i = (rsv_info->size + mtd->writesize - 1) / mtd->writesize;
	pr_info("%s %d: %s, valid = %d, pages = %d\n", __func__, __LINE__,
		rsv_info->name, rsv_info->valid, i);
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
			pr_info("%s %d: %s bad block here 0x%llx\n",
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
			pr_info("%s %d %s erase failed at 0x%llx ,mark it bad\n",
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
		memset(&erase_info,	0, sizeof(struct erase_info));
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

int meson_rsv_scan(struct meson_rsv_info_t *rsv_info)
{
	struct mtd_info *mtd;
	struct mtd_oob_ops oob_ops;
	struct oobinfo_t oobinfo;
	struct free_node_t *free_node, *temp_node;
	loff_t offset;
	u32 start, end;
	int ret = 0, error, rsv_status, i, k;

	u8 scan_status;
	u8 good_addr[256] = {0};
	u32 page_num, pages_per_blk;

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	mtd = rsv_info->mtd;
RE_RSV_INFO_EXT:
	start = rsv_info->start;
	end = rsv_info->end;
	pr_info("%s:info size = 0x%x, start blk = %d, end blk = %d\n",
		rsv_info->name, rsv_info->size, start, end);
	do {
		offset = start;
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
		if (!memcmp(oobinfo.name, rsv_info->name,
			    4)) {
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
					rsv_info->nvalid->blk_addr = start;
					rsv_info->nvalid->page_addr = 0;
					rsv_info->nvalid->ec = oobinfo.ec;
					rsv_info->nvalid->timestamp =
						oobinfo.timestamp;
				} else {
					free_node->blk_addr = start;
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
				rsv_info->nvalid->blk_addr = start;
				rsv_info->nvalid->page_addr = 0;
				rsv_info->nvalid->ec = oobinfo.ec;
				rsv_info->nvalid->timestamp = oobinfo.timestamp;
			}
		} else {
			free_node = get_free_node(rsv_info);
			if (!free_node)
				return -ENOMEM;
			free_node->blk_addr = start;
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

	pr_info("%s blk = %d, ec = %d, page = %d, timestamp = %d\n",
			rsv_info->name, rsv_info->nvalid->blk_addr, rsv_info->nvalid->ec,
			rsv_info->nvalid->page_addr, rsv_info->nvalid->timestamp);
	pr_info("%s free list:\n", rsv_info->name);
	temp_node = rsv_info->nfree;
	while (temp_node) {
		pr_info("block num = %d, ec = %d, dirty_flag = %d\n",
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

	if (!rsv_info) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

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

static void aml_nand_rsv_info_ptr_fill(struct mtd_info *mtd,
				       struct meson_rsv_handler_t *handler)
{
	if (rsv_board_info[BBT_INFO_INDEX].rsv_info)
		handler->bbt = rsv_board_info[BBT_INFO_INDEX].rsv_info;
	if (rsv_board_info[ENV_INFO_INDEX].rsv_info)
		handler->env = rsv_board_info[ENV_INFO_INDEX].rsv_info;
	if (rsv_board_info[KEY_INFO_INDEX].rsv_info)
		handler->key = rsv_board_info[KEY_INFO_INDEX].rsv_info;
	if (rsv_board_info[DTB_INFO_INDEX].rsv_info)
		handler->dtb = rsv_board_info[DTB_INFO_INDEX].rsv_info;
	if (rsv_board_info[DDR_INFO_INDEX].rsv_info)
		handler->ddr_para = rsv_board_info[DDR_INFO_INDEX].rsv_info;
}

static int aml_nand_rsv_info_alloc_init(struct mtd_info *mtd,
					u32 vernier,
					char *name,
					struct meson_rsv_info_t **rsv_info,
					struct meson_rsv_handler_t *handler,
					unsigned int blocks, unsigned int size)
{
	if (!blocks)
		return 1;

	*rsv_info = kzalloc(sizeof(struct meson_rsv_info_t), GFP_KERNEL);
	if (!(*rsv_info))
		return -ENOMEM;

	(*rsv_info)->nvalid =
		kzalloc(sizeof(struct valid_node_t), GFP_KERNEL);
	if (!(*rsv_info)->nvalid)
		return -ENOMEM;

	(*rsv_info)->mtd = mtd;
	(*rsv_info)->start = vernier;
	(*rsv_info)->end = vernier + blocks;
	(*rsv_info)->nvalid->blk_addr = -1;
	(*rsv_info)->handler = handler;
	if (!memcmp(name, BBT_NAND_MAGIC, 4))
		(*rsv_info)->size = mtd->size >> mtd->erasesize_shift;
	else
		(*rsv_info)->size = size;

	memcpy((*rsv_info)->name, name, 4);
	return 0;
}

int meson_rsv_init(struct mtd_info *mtd,
		   struct meson_rsv_handler_t *handler)
{
	int i, ret = 0;
	u32 start, vernier;

	start = MTD_RSV_START_BLOCK + MTD_RSV_GAP_BLOCK_CNT;
	vernier = start;
	handler->fn_bitmask = 0;
	for (i = 0; i < MTD_RSV_BLOCK_CNT; i++) {
		handler->free_node[i] =
			kzalloc(sizeof(struct free_node_t), GFP_KERNEL);
		if (!handler->free_node[i]) {
			ret = -ENOMEM;
			goto error0;
		}
		memset(handler->free_node[i], 0, sizeof(struct free_node_t));
		handler->free_node[i]->index = i;
	}

	for (i = 0; i < ARRAY_SIZE(rsv_board_info); i++) {
		ret = aml_nand_rsv_info_alloc_init(mtd, vernier, rsv_board_info[i].name,
						   &rsv_board_info[i].rsv_info,
						   handler,
						   rsv_board_info[i].blocks,
						   rsv_board_info[i].size);
		if (ret < 0) {
			pr_err("%s info alloc init failed\n", rsv_board_info[i].name);
			ret = -ENOMEM;
			goto error1;
		} else if (ret == 1) {
			pr_err("rsv no need to init %s\n", rsv_board_info[i].name);
		} else {
			pr_err("%s start 0x%x end 0x%x size 0x%x\n", rsv_board_info[i].name,
			       rsv_board_info[i].rsv_info->start,
			       rsv_board_info[i].rsv_info->end,
			       rsv_board_info[i].rsv_info->size);
		}
		vernier += rsv_board_info[i].blocks;
	}

	aml_nand_rsv_info_ptr_fill(mtd, handler);
	if ((vernier - start) > MTD_RSV_BLOCK_CNT) {
		pr_err("ERROR: total blk number is over the limit\n");
		ret = -ENOMEM;
		goto error2;
	}
	rsv_handler = handler;
	return 0;

error2:
	handler->ddr_para = NULL;
	handler->key = NULL;
	handler->env = NULL;
	handler->bbt = NULL;
	handler->bbt = NULL;
error1:
	for (i = 0; i < ARRAY_SIZE(rsv_board_info); i++) {
		kfree(rsv_board_info[i].rsv_info->nvalid);
		rsv_board_info[i].rsv_info->nvalid = NULL;
		kfree(rsv_board_info[i].rsv_info);
		rsv_board_info[i].rsv_info = NULL;
	}
error0:
	for (i = 0; i < MTD_RSV_BLOCK_CNT; i++) {
		kfree(handler->free_node[i]);
		handler->free_node[i] = NULL;
	}

	return ret;
}

int meson_rsv_bbt_read(u_char *dest, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->bbt) {
		pr_info("%s %d: not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	if (!rsv_handler->bbt->valid) {
		pr_info("%s, %d, %s invalid!, read exit!\n",
			__func__, __LINE__,
			rsv_handler->bbt->name);
		return RSV_INVALID;
	}
	if (!dest || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, dest, size);
		return 1;
	}
	len = rsv_handler->bbt->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = %ld\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	ret = meson_rsv_read(rsv_handler->bbt, temp);
	memcpy(dest, temp, len > size ? size : len);
	pr_info("%s %d read 0x%lx bytes from bbt, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_key_read(u_char *dest, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->key) {
		pr_info("%s %d: not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	if (!rsv_handler->key->valid) {
		pr_info("%s, %d, %s invalid!, read exit!\n",
			__func__, __LINE__,
			rsv_handler->key->name);
		return RSV_INVALID;
	}
	if (!dest || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, dest, size);
		return 1;
	}
	len = rsv_handler->key->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	ret = meson_rsv_read(rsv_handler->key, temp);
	memcpy(dest, temp, len > size ? size : len);
	pr_info("%s %d read 0x%lx bytes from key, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_ddr_para_read(u_char *dest, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->ddr_para) {
		pr_info("%s %d: not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	if (!rsv_handler->ddr_para->valid) {
		pr_info("%s, %d, %s invalid!, read exit!\n",
			__func__, __LINE__,
			rsv_handler->ddr_para->name);
		return RSV_INVALID;
	}
	if (!dest || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, dest, size);
		return 1;
	}
	len = rsv_handler->ddr_para->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	ret = meson_rsv_read(rsv_handler->ddr_para, temp);
	memcpy(dest, temp, len > size ? size : len);
	pr_info("%s %d read 0x%lx bytes from ddr_para, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_env_read(u_char *dest, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->env) {
		pr_info("%s %d: not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!rsv_handler->env->valid) {
		pr_info("%s, %d, %s invalid!, read exit!\n",
			__func__, __LINE__,
			rsv_handler->env->name);
		return RSV_INVALID;
	}
	if (!dest || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, dest, size);
		return 1;
	}
	len = rsv_handler->env->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	ret = meson_rsv_read(rsv_handler->env, temp);
	memcpy(dest, temp, len > size ? size : len);
	pr_info("%s %d read 0x%lx bytes from env, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_dtb_read(u_char *dest, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->dtb) {
		pr_info("%s %d: rsv info not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!rsv_handler->dtb->valid) {
		pr_info("%s, %d, %s invalid!, read exit!\n",
			__func__, __LINE__,
			rsv_handler->dtb->name);
		return RSV_INVALID;
	}
	if (!dest || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, dest, size);
		return 1;
	}
	len = rsv_handler->dtb->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	ret = meson_rsv_read(rsv_handler->dtb, temp);
	memcpy(dest, temp, len > size ? size : len);
	pr_info("%s %d read 0x%lx bytes from dtb, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

/*update bbt*/
int meson_rsv_bbt_write(u_char *source, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->bbt) {
		pr_info("%s %d rsv info not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!source || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, source, size);
		return 1;
	}
	len = rsv_handler->bbt->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	memcpy(temp, source, len > size ? size : len);
	ret = meson_rsv_save(rsv_handler->bbt, temp);
	pr_info("%s %d write 0x%lx bytes to bbt, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_key_write(u_char *source, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->key) {
		pr_info("%s %d rsv info not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!source || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, source, size);
		return 1;
	}
	len = rsv_handler->key->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	memcpy(temp, source, len > size ? size : len);
	ret = meson_rsv_save(rsv_handler->key, temp);
	pr_info("%s %d write 0x%lx bytes to key, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

#ifdef CONFIG_DDR_PARAMETER_SUPPORT
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
#endif

int meson_rsv_ddr_para_write(u_char *source, size_t size)
{
#if defined(CONFIG_DDR_PARAMETER_SUPPORT) && defined(CONFIG_MTD_SPI_NAND)
	struct mtd_info *mtd = rsv_handler->ddr_para->mtd;
#endif
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->ddr_para) {
		pr_info("%s %d rsv info not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!source || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, source, size);
		return 1;
	}
	len = rsv_handler->ddr_para->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	memcpy(temp, source, len > size ? size : len);
	ret = meson_rsv_save(rsv_handler->ddr_para, temp);
	pr_info("%s %d write 0x%lx bytes to key, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);

#if defined(CONFIG_DDR_PARAMETER_SUPPORT) && defined(CONFIG_MTD_SPI_NAND)
	ret = meson_modify_page_info_and_save(mtd);
#endif
	kfree(temp);
	return ret;
}

int meson_rsv_env_write(u_char *source, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->env) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!source || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, source, size);
		return 1;
	}
	len = rsv_handler->env->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	memcpy(temp, source, len > size ? size : len);
	ret = meson_rsv_save(rsv_handler->env, temp);
	pr_info("%s %d write 0x%lx bytes to env, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

int meson_rsv_dtb_write(u_char *source, size_t size)
{
	u_char *temp;
	size_t len;
	int ret;

	if (!rsv_handler ||
	    !rsv_handler->dtb) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (!source || size == 0) {
		pr_info("%s %d parameter error %p %ld\n",
			__func__, __LINE__, source, size);
		return 1;
	}
	len = rsv_handler->dtb->size;
	temp = kzalloc(len, GFP_KERNEL);
	if (!temp) {
		pr_err("%s %d kzalloc fail size = 0x%lx\n",
			__func__, __LINE__, len);
		return -ENOMEM;
	}
	memset(temp, 0, len);
	memcpy(temp, source, len > size ? size : len);
	ret = meson_rsv_save(rsv_handler->dtb, temp);
	pr_info("%s %d write 0x%lx bytes to dtb, ret %d\n",
		__func__, __LINE__, len > size ? size : len, ret);
	kfree(temp);
	return ret;
}

u32 meson_rsv_bbt_size(void)
{
	if (!rsv_handler ||
	    !rsv_handler->bbt) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 0;
	}
	return rsv_handler->bbt->size;
}

u32 meson_rsv_key_size(void)
{
	if (!rsv_handler ||
	    !rsv_handler->key) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 0;
	}
	return rsv_handler->key->size;
}

u32 meson_rsv_ddr_para_size(void)
{
	if (!rsv_handler ||
	    !rsv_handler->ddr_para) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 0;
	}
	return rsv_handler->ddr_para->size;
}


u32 meson_rsv_env_size(void)
{
	if (!rsv_handler ||
	    !rsv_handler->env) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 0;
	}
	return rsv_handler->env->size;
}

u32 meson_rsv_dtb_size(void)
{
	if (!rsv_handler ||
	    !rsv_handler->dtb) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 0;
	}
	return rsv_handler->dtb->size;
}

int meson_rsv_bbt_erase(void)
{
	if (!rsv_handler ||
	    !rsv_handler->bbt) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}

	if (rsv_handler->bbt->valid) {
		return meson_rsv_erase(rsv_handler->bbt);
	}
	return 0;
}

int meson_rsv_key_erase(void)
{
	if (!rsv_handler ||
	    !rsv_handler->key) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (rsv_handler->key->valid) {
		return meson_rsv_erase(rsv_handler->key);
	}
	return 0;
}

int meson_rsv_ddr_para_erase(void)
{
	if (!rsv_handler ||
	    !rsv_handler->ddr_para) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (rsv_handler->ddr_para->valid) {
		return meson_rsv_erase(rsv_handler->ddr_para);
	}
	return 0;
}


int meson_rsv_env_erase(void)
{
	if (!rsv_handler ||
	    !rsv_handler->env) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (rsv_handler->env->valid) {
		return meson_rsv_erase(rsv_handler->env);
	}
	return 0;
}

int meson_rsv_dtb_erase(void)
{
	if (!rsv_handler ||
	    !rsv_handler->dtb) {
		pr_info("%s %d rsv info has not inited yet!\n",
			__func__, __LINE__);
		return 1;
	}
	if (rsv_handler->dtb->valid) {
		return meson_rsv_erase(rsv_handler->dtb);
	}
	return 0;

}

struct rsv_info *meson_rsv_get_info(int *size)
{
	*size = ARRAY_SIZE(rsv_board_info);
	return rsv_board_info;
}
