// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <spi.h>
#include <spi_flash.h>
#include <linux/mtd/partitions.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <mtd.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/storage.h>
#include <amlogic/cpu_id.h>

/* use STORAGE_ROUND_UP, y must be power of 2 */
#define SPINOR_ROUND_UP_IF_UNALIGN(x, y) ((x) = (((x) + (y) - 1) & (~(y - 1))))

extern struct mtd_partition *get_spiflash_partition_table(int *partitions);
int spinor_add_partitions(struct mtd_info *mtd)
{
	struct mtd_partition *spiflash_partitions;
	int i, partition_count;
	uint64_t size;

	spiflash_partitions = get_spiflash_partition_table(&partition_count);

	for (i = 0; i < partition_count; i++) {
		size = spiflash_partitions[i].size;
		if (size % SPINOR_ALIGNED_SIZE)
			spiflash_partitions[i].size =
				SPINOR_ROUND_UP_IF_UNALIGN(size, SPINOR_ALIGNED_SIZE);
	}

	return mtd_register_partitions(mtd, spiflash_partitions,
			partition_count, false);
}

extern void mtdparts_deinit(void);
int spinor_del_partitions(struct mtd_info *mtd)
{
	mtdparts_deinit();

	return del_mtd_partitions(mtd);
}
