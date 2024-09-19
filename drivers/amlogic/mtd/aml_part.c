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

static void mtd_get_logic_part_info(struct mtd_info *mtd,
				    struct mtd_partition *part)
{
	loff_t offset = part->offset, end = part->offset + part->size;
	loff_t append_size = 0;

#ifdef CONFIG_NOT_SKIP_BAD_BLOCK
	return;
#endif

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

static inline void set_part_info(struct mtd_partition *part,
			const char *name, uint64_t offset, uint64_t size)
{
	part->name = name;
	part->offset = offset;
	part->size = size;
}

uint64_t mtd_get_normal_part_offset(struct mtd_info *mtd)
{
	return meson_rsv_part_get_tpl_start(mtd) + meson_rsv_part_get_tpl_size(mtd);
}

int mtd_add_partitions(struct mtd_info *mtd, const struct mtd_partition *parts,
		       int nbparts, bool bl2_part_only, bool slc_nand)
{
	struct mtd_partition *new_part, *p, bl2_part[1] = { 0 };
	u64 normal_offset;
	int i, n = nbparts + 2, ret;

	if (bl2_part_only) {
		if (!slc_nand)
			return -1;
		set_part_info(&bl2_part[0], BOOT_BL2, 0,
			      meson_rsv_part_get_bl2_part_size(mtd));
		ret = add_mtd_partitions(mtd, &bl2_part[0], 1);
		return ret;
	}

	new_part = kcalloc(n, sizeof(*new_part), GFP_KERNEL);
	if (IS_ERR_OR_NULL(new_part))
		return -ENOMEM;

	/* simplify that bootloader only has BL2 + TPL */
	set_part_info(&new_part[0], BOOT_BL2, 0,
		      meson_rsv_part_get_bl2_part_size(mtd));
	set_part_info(&new_part[1], BOOT_TPL,
		      meson_rsv_part_get_tpl_start(mtd),
		      meson_rsv_part_get_tpl_size(mtd));

	/* filter out these partitions with wrong size ZERO  */
	for (i = 0, p = new_part + 2; i < nbparts; i++) {
		if (!parts[i].size && !parts[i].offset)
			continue;
		*p++ = parts[i];
	}

	normal_offset = mtd_get_normal_part_offset(mtd);
	n = p - new_part - 2;
	for (i = 0, p = new_part + 2; i < n; i++, p++) {
		if ((normal_offset + p->size) > mtd->size) {
			pr_err("over nand size!\n");
			kfree(new_part);
			return -1;
		}
		p->offset = normal_offset;
		mtd_get_logic_part_info(mtd, p);
		if (i == (n - 1))
			p->size = mtd->size - normal_offset;
		normal_offset += p->size;
	}

	p = (slc_nand) ? new_part + 1 : new_part;
	nbparts = (slc_nand) ? n + 1 : n + 2;
	ret = add_mtd_partitions(mtd, p, nbparts);
	kfree(new_part);

	return ret;
}

int mtd_raw_nand_add_boot_partitions(struct mtd_info *mtd)
{
	return mtd_add_partitions(mtd, NULL, 0, true, true);
}

int mtd_raw_nand_add_normal_partitions(struct mtd_info *mtd,
				       const struct mtd_partition *parts,
				       int nbparts)
{
	return mtd_add_partitions(mtd, parts, nbparts, false, true);
}

int mtd_spi_nand_add_partitions(struct mtd_info *mtd,
				const struct mtd_partition *parts,
				int nbparts)
{
	return mtd_add_partitions(mtd, parts, nbparts, false, false);
}


