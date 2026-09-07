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
	if (mtd->type == MTD_NORFLASH)
		return mtd_spinor_get_bootpart_size();
	else
		return meson_rsv_part_get_tpl_start(mtd) + meson_rsv_part_get_tpl_size(mtd);
}

int mtd_register_partitions(struct mtd_info *mtd,
                            const struct mtd_partition *parts,
                            int nbparts, bool nand)
{
	struct mtd_partition *new_part, *p, *normal, bl2_part[1] = { 0 };
	u64 normal_offset;
	int i, n = nbparts, ret;

	if (!nbparts && nand) {
		set_part_info(&bl2_part[0], BOOT_BL2, 0,
			      meson_rsv_part_get_bl2_part_size(mtd));
		return add_mtd_partitions(mtd, &bl2_part[0], 1);
	}

	/* Maybe need extra two more parts <bl2 and tpl> */
	new_part = kcalloc(n + 2, sizeof(*new_part), GFP_KERNEL);
	if (!new_part)
		return -ENOMEM;

	p = new_part;
	if (mtd->type == MTD_NORFLASH) {
		set_part_info(p++, BOOT_LOADER, 0,
			      mtd_spinor_get_bootpart_size());
	} else {
		/* simplify that bootloader only has BL2 + TPL */
		if (!nand)
			set_part_info(p++, BOOT_BL2, 0,
				      meson_rsv_part_get_bl2_part_size(mtd));

		set_part_info(p++, BOOT_TPL,
			      meson_rsv_part_get_tpl_start(mtd),
			      meson_rsv_part_get_tpl_size(mtd));
	}

	normal = p;

	/* filter out these partitions with wrong size ZERO  */
	for (i = 0; i < nbparts; i++) {
		if (!parts[i].size && !parts[i].offset)
			continue;
		*p++ = parts[i];
	}

	normal_offset = mtd_get_normal_part_offset(mtd);
	nbparts = p - new_part;
	n = p - normal;
	for (i = 0, p = normal; i < n; i++, p++) {
		if ((normal_offset + p->size) > mtd->size) {
			pr_err("over flash size!\n");
			kfree(new_part);
			return -1;
		}
		p->offset = normal_offset;
		if (mtd->type != MTD_NORFLASH)
			mtd_get_logic_part_info(mtd, p);
		if (i == (n - 1))
			p->size = mtd->size - normal_offset;
		normal_offset += p->size;
	}

	ret = add_mtd_partitions(mtd, new_part, nbparts);
	kfree(new_part);

	return ret;
}
