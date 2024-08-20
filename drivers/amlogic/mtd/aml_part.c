// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#define pr_fmt(fmt)	"aml_part: " fmt

#include <common.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/compat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spinand.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/storage.h>
#include <amlogic/aml_rsv.h>

const char *part_name[] = {
	BOOT_LOADER, BOOT_BL2E, BOOT_BL2X, BOOT_DDRFIP, BOOT_DEVFIP
};

#ifndef CONFIG_NOT_SKIP_BAD_BLOCK
static void mtd_get_logic_part_info(struct mtd_info *mtd,
						struct mtd_partition *part)
{
	loff_t offset = part->offset, end = part->offset + part->size;
	loff_t append_size = 0;

	do {
		if (mtd->_block_isbad(mtd, offset) == NAND_FACTORY_BAD) {
			pr_err("%s %d found bad block in 0x%llx\n",
					__func__, __LINE__, offset);
			end += mtd->erasesize;
			append_size += mtd->erasesize;
		}
		offset += mtd->erasesize;
	} while (offset < end && offset < mtd->size);
	part->size += append_size;
}
#else
void mtd_get_logic_part_info(struct mtd_info *mtd,
	struct mtd_partition *part)
{ }
#endif

static inline void set_part_info(struct mtd_partition *part,
			const char *name, uint64_t offset, uint64_t size)
{
	part->name = name;
	part->offset = offset;
	part->size = size;
}

static uint64_t mtd_get_tpl_start(struct mtd_info *mtd)
{
	uint64_t tpl_start = (uint64_t)(MTD_RSV_START_BLOCK + MTD_RSV_BLOCK_CNT) * mtd->erasesize;

	if ((store_boot_layout_is_discrete_all() ||
	     store_boot_layout_is_discrete_default()) &&
	     !store_boot_layout_is_discrete_bl2())
		/* compatible with the case uboot set boot layout to discrete all mode,
		 * but bl2 don't sync to pass boot_layout to bl33 in boot workmode or
		 * uboot set boot layout to discrete bl2, but bl2 pass boot layout by
		 * boot_layout_compat
		 */
		tpl_start = g_ssp.boot_entry[BOOT_AREA_DEVFIP].offset;

	return tpl_start;
}

static uint64_t mtd_get_tpl_size(struct mtd_info *mtd)
{
	if (!store_boot_layout_is_discrete_bl2() &&
	    !store_boot_layout_is_discrete_all() &&
	    !store_boot_layout_is_discrete_default())
		return (CONFIG_TPL_SIZE_PER_COPY * CONFIG_NAND_TPL_COPY_NUM);
	else if (store_boot_layout_is_discrete_bl2())
		/* if BOOT_DISCRETE_BL2, the devfip size includes all (BL2E,BL2X,....) */
		return ((uint64_t)g_ssp.boot_entry[BOOT_AREA_DEVFIP].size *
				g_ssp.boot_backups);
	else
		return ((uint64_t)g_ssp.boot_entry[BOOT_AREA_DEVFIP].size *
				CONFIG_NAND_TPL_COPY_NUM);
}

uint64_t mtd_get_normal_part_offset(struct mtd_info *mtd)
{
	return mtd_get_tpl_start(mtd) + mtd_get_tpl_size(mtd);
}

static uint32_t mtd_get_invalid_boot_parts(void)
{
	int i, cnt = 0;

	for (i = 0; i <= BOOT_AREA_DEVFIP; i++)
		if (!g_ssp.boot_entry[i].size)
			cnt++;

	return cnt;
}

int mtd_get_boot_parts_num(void)
{
	if ((store_boot_layout_is_discrete_all() ||
	     store_boot_layout_is_discrete_default()) &&
	     !store_boot_layout_is_discrete_bl2())
		/* compatible with the case uboot set boot layout to discrete all mode,
		 * but bl2 don't sync to pass boot_layout to bl33 in boot workmode or
		 * uboot set boot layout to discrete bl2, but bl2 pass boot layout by
		 * boot_layout_compat
		 */
		return 5 - mtd_get_invalid_boot_parts();
	else
		return 2;
}

int mtd_get_boot_partition(struct mtd_info *mtd, struct mtd_partition *parts,
			   u8 index, u8 cnt)
{
	struct mtd_partition boot_parts[MAX_BOOT_AREA_ENTRIES];
	u32 page_size = g_ssp.sip.nsp.page_size;
	u64 tpl_start, tpl_size;
	int i, j;

	if (index + cnt > MAX_BOOT_AREA_ENTRIES)
		return -1;

	memset(boot_parts, 0, sizeof(boot_parts));

	tpl_start = mtd_get_tpl_start(mtd);
	tpl_size = mtd_get_tpl_size(mtd);

	set_part_info(&boot_parts[0], BOOT_BL2, 0,
		      BOOT_TOTAL_PAGES * (uint64_t)page_size);
	set_part_info(&boot_parts[1], BOOT_TPL, tpl_start, tpl_size);

	if (store_boot_layout_is_discrete_all() ||
	    store_boot_layout_is_discrete_default()) {
		for (i = BOOT_AREA_BL2E, j = BOOT_AREA_BL2E;
				i <= BOOT_AREA_DEVFIP; i++) {
			u64 part_size = g_ssp.boot_entry[i].size;

			if (i == BOOT_AREA_DEVFIP)
				part_size *= CONFIG_NAND_TPL_COPY_NUM;
			else
				part_size *= g_ssp.boot_backups;

			if (part_size) {
				set_part_info(&boot_parts[j], part_name[i],
					      g_ssp.boot_entry[i].offset, part_size);
				j++;
			}
		}
	}

	memcpy(parts, &boot_parts[index], cnt * sizeof(struct mtd_partition));

	return 0;
}

int mtd_add_boot_partitions(struct mtd_info *mtd,
			    struct mtd_partition *parts,
			    int nparts)
{
	struct mtd_partition boot_parts[MAX_BOOT_AREA_ENTRIES];
	u8 index = 0, boot_parts_cnt = mtd_get_boot_parts_num();

	if (parts)
		return add_mtd_partitions(mtd, parts, nparts);

	memset(boot_parts, 0, sizeof(boot_parts));

	mtd_get_boot_partition(mtd, boot_parts, index, boot_parts_cnt);

	return add_mtd_partitions(mtd, boot_parts, boot_parts_cnt);
}

int mtd_add_normal_partitions(struct mtd_info *mtd,
			      const struct mtd_partition *parts,
			      int nbparts, int normal_offset)
{
	struct mtd_partition *new_part;
	int normal_part_num = 0, i, ret;

	new_part = kcalloc(nbparts, sizeof(*new_part), GFP_KERNEL);
	if (IS_ERR_OR_NULL(new_part))
		return -ENOMEM;

	memcpy(new_part, parts, nbparts * sizeof(struct mtd_partition));

	for (i = 0; i < nbparts; i++) {
		if (!new_part[i].size && !new_part[i].offset) {
			normal_part_num++;
			continue;
		}

		if ((normal_offset + new_part[i].size) > mtd->size) {
			pr_err("%s %d over nand size!\n", __func__, __LINE__);
			ret = -1;
			goto _out;
		}

		new_part[i].offset = normal_offset;
		mtd_get_logic_part_info(mtd, &new_part[i]);
		if (i == (nbparts - 1))
			new_part[i].size = mtd->size - normal_offset;
		normal_offset += new_part[i].size;
	}

	ret = add_mtd_partitions(mtd,
				 &new_part[normal_part_num],
				 nbparts - normal_part_num);
_out:
	kfree(new_part);
	return ret;
}

/* The size of the partition must be block aligned */
int mtd_add_partitions(struct mtd_info *mtd,
		       const struct mtd_partition *parts,
		       int nbparts)
{
	u64 normal_part_offset =
					mtd_get_tpl_start(mtd) + mtd_get_tpl_size(mtd);
	int ret;

	ret = mtd_add_boot_partitions(mtd, NULL, -1);
	if (ret)
		return ret;

	return mtd_add_normal_partitions(mtd, parts, nbparts,
					     normal_part_offset);
}
