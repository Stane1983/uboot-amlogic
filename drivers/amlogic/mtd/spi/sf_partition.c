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

/* Hard code, all partitions are aligned in block size, fast erasing */
#define SPINOR_ALIGNED_SIZE		(64 * 1024)

uint64_t spiflash_bootloader_size(void)
{
#if defined(CONFIG_BOOTLOADER_SIZE)
	return (((DIV_ROUND_UP((CONFIG_BOOTLOADER_SIZE + 0x200), 0x1000)) << 12) *
		CONFIG_NOR_TPL_COPY_NUM);
#else
	return SZ_2M;
#endif
}

uint32_t __weak spiflash_rsv_block_num(void)
{
	return 0;
}

extern boot_area_entry_t general_boot_part_entry[MAX_BOOT_AREA_ENTRIES];
/* The size of the partition must be block aligned */
static int _spinor_add_partitions(struct mtd_info *mtd,
				  const struct mtd_partition *parts,
				  int nbparts)
{
	int part_num = 0, i = 0;
	struct mtd_partition *temp, *parts_nm;
	loff_t off;
	int ret = 1;

	part_num = nbparts + 1;

	temp = kzalloc(sizeof(*temp) * part_num, GFP_KERNEL);
	if (store_get_device_bootloader_mode() == COMPACT_BOOTLOADER) {
		temp[0].name = BOOT_LOADER;
		temp[0].offset = 0;
		temp[0].size = spiflash_bootloader_size();
		temp[0].size = ALIGN(temp[0].size, SPINOR_ALIGNED_SIZE);
		off = temp[0].size + temp[0].offset;
		parts_nm = &temp[1];

	} else {
		temp[0].name = BOOT_LOADER;
		temp[0].offset = 0;
		temp[0].size = spiflash_bootloader_size();
		if (temp[0].size % SPINOR_ALIGNED_SIZE)
			WARN_ON(1);
		/* rsv size is aligned with blocksize(64K) */
		off = temp[0].size + spiflash_rsv_block_num() * SPINOR_ALIGNED_SIZE;
		parts_nm = &temp[1];
	}

	for (i = 0; i < nbparts; i++) {
		if (!parts[i].name) {
			pr_err("name can't be null! ");
			pr_err("please check your %d th partition name!\n",
				 i + 1);
			goto _out;
		}
		if ((off + parts[i].size) > mtd->size) {
			pr_err("%s %d over nand size!\n",
				__func__, __LINE__);
			goto _out;
		}
		parts_nm[i].name = parts[i].name;
		parts_nm[i].offset = off;
		if (parts[i].size % SPINOR_ALIGNED_SIZE) {
			pr_err("%s %d \"%s\" size auto align to block size\n",
				__func__, __LINE__, parts[i].name);
			parts_nm[i].size += parts[i].size % SPINOR_ALIGNED_SIZE;
		}
		/* it's ok "+=" here because size has been set to 0 */
		parts_nm[i].size += parts[i].size;
		off += parts_nm[i].size;
		if (i == (nbparts - 1))
			parts_nm[i].size = mtd->size - off;
	}
	ret = add_mtd_partitions(mtd, temp, part_num);
_out:
	kfree(temp);
	return ret;
}

extern struct mtd_partition *get_spiflash_partition_table(int *partitions);
int spinor_add_partitions(struct mtd_info *mtd)
{
	struct mtd_partition *spiflash_partitions;
	int partition_count;

	spiflash_partitions = get_spiflash_partition_table(&partition_count);

	return _spinor_add_partitions(mtd, spiflash_partitions,
			       partition_count);
}

int spinor_del_partitions(struct mtd_info *mtd)
{
	return del_mtd_partitions(mtd);
}
