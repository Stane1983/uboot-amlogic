// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <nand.h>
#include <linux/errno.h>
#include <linux/sizes.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <amlogic/storage.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/aml_nand.h>
#include <amlogic/aml_rsv.h>
#include <amlogic/aml_pageinfo.h>
#include <asm/amlogic/arch/cpu_config.h>
#include <amlogic/partition_table.h>
#include <jffs2/jffs2.h>
#include <time.h>
#include <amlogic/cpu_id.h>
#include <fdt_support.h>
#include <linux/libfdt.h>

struct map_handler_t {
	u16 *map;
	u16 valid_max;
	u8 init_flag;
};
static struct mtd_info *mtd_store_list[MAX_MTD_CNT];
extern int info_disprotect;

struct mtd_info *mtd_store_get(int dev)
{
	enum boot_type_e medium_type = store_get_type();

	if ((medium_type == BOOT_SNAND) || (medium_type == BOOT_SNOR))
		return mtd_store_list[0];
	else if (medium_type == BOOT_NAND_MTD)
		return mtd_store_list[dev];
	else
		return NULL;

}

int get_meson_mtd_partition_table(struct mtd_partition **partitions)
{
#if defined(CONFIG_SPI_NAND) || defined(CONFIG_MTD_SPI_NAND) || defined(CONFIG_MESON_NFC) || defined(CONFIG_SPI_FLASH)
	// enum boot_type_e medium_type = store_get_type();
#endif
	int mtdParts = -1;

#ifdef CONFIG_MESON_NFC
	extern struct mtd_partition *get_aml_mtd_partition(void);
	extern int get_aml_partition_count(void);
	// if (BOOT_NAND_MTD == medium_type) {
		mtdParts = get_aml_partition_count();
		*partitions = get_aml_mtd_partition();
	// }
#endif
#if defined(CONFIG_SPI_NAND) || defined(CONFIG_MTD_SPI_NAND)
	extern const struct mtd_partition *get_spinand_partition_table(int *partitions);
	// if (BOOT_SNAND == medium_type)
		*partitions = (struct mtd_partition *)get_spinand_partition_table(&mtdParts);
#endif
#ifdef CONFIG_SPI_FLASH
	extern const struct mtd_partition *get_spiflash_partition_table(int *partitions);
	// if (medium_type == BOOT_SNOR)
		*partitions = (struct mtd_partition *)get_spiflash_partition_table(&mtdParts);
#endif
	return mtdParts;
}

static struct mtd_info *mtd_store_get_by_name(const char *part_name,
					      int boot)
{
	if ((store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER) ||
	    (store_get_device_bootloader_mode() == ADVANCE_BOOTLOADER)) {
		if (!strcmp(part_name, BOOT_BL2) ||
		    !strcmp(part_name, BOOT_SPL))
			return mtd_store_get(0);
	} else {
		if (!strcmp(part_name, BOOT_LOADER))
			return mtd_store_get(0);
	}

	return mtd_store_get(1);
}

void mtd_store_set(struct mtd_info *mtd, int dev)
{
	enum boot_type_e medium_type = store_get_type();

	if ((medium_type == BOOT_SNAND) || (medium_type == BOOT_SNOR))
		mtd_store_list[0] = mtd;
	else if (medium_type == BOOT_NAND_MTD)
		mtd_store_list[dev] = mtd;
	else
		pr_info("unsupport boot device\n");
}

#ifdef CONFIG_CMD_NAND_TRIMFFS
static size_t drop_ffs(const struct mtd_info *mtd,
		       const u_char *buf, const size_t *len)
{
	size_t l = *len;
	ssize_t i;

	for (i = l - 1; i >= 0; i--)
		if (buf[i] != 0xFF)
			break;

	/* The resulting length must be aligned to the minimum flash I/O size */
	l = i + 1;
	l = (l + mtd->writesize - 1) / mtd->writesize;
	l *=  mtd->writesize;

	/*
	 * since the input length may be unaligned, prevent access past the end
	 * of the buffer
	 */
	return min(l, *len);
}
#endif

#ifdef CONFIG_YAFFS2
int meson_yaffs2_mount(char *mtpoint, char *part_name)
{
	struct mtd_info *mtd;
	struct mtd_device *dev;
	struct part_info *part;
	enum boot_type_e medium_type = store_get_type();
	int mtd_index = 0, nand_index = 0, ret;
	u8 pnum;

	if (!part_name || !mtpoint)
		return -1;

	if (medium_type == BOOT_NAND_MTD) {
		mtd_index = 1;
		nand_index = 1;
	}

	mtd = mtd_store_get(mtd_index);

	ret = find_dev_and_part(part_name,
				&dev,
				&pnum,
				&part);
	if (ret) {
		pr_info("%s %d can not find part:%s\n",
			__func__, __LINE__, part_name);
		return ret;
	}

	cmd_yaffs_devconfig(mtpoint, nand_index, part->offset / mtd->erasesize,
			(part->offset + part->size) / mtd->erasesize - 1);
	cmd_yaffs_mount(mtpoint);

	return 0;
}
#endif

static int check_skip_len(struct mtd_info *mtd,
			  loff_t offset, size_t length, size_t *used)
{
	size_t len_excl_bad = 0;
	int ret = 0;

	while (len_excl_bad < length) {
		size_t block_len, block_off;
		loff_t block_start;

		if (offset >= mtd->size)
			return -1;

		block_start = offset & ~(loff_t)(mtd->erasesize - 1);
		block_off = offset & (mtd->erasesize - 1);
		block_len = mtd->erasesize - block_off;

		if (!mtd_block_isbad(mtd, block_start))
			len_excl_bad += block_len;
		else
			ret = 1;

		offset += block_len;
		*used += block_len;
	}

	/* If the length is not a multiple of block_len, adjust. */
	if (len_excl_bad > length)
		*used -= (len_excl_bad - length);

	return ret;
}

static int mtd_store_read_skip_bad(struct mtd_info *mtd,
				   loff_t offset, size_t *length,
				   size_t *actual, loff_t lim,
				   u_char *buffer)
{
	int rval;
	size_t left_to_read = *length;
	size_t used_for_read = 0;
	u_char *p_buffer = buffer;
	int need_skip;

	if ((offset & (mtd->writesize - 1)) != 0) {
		pr_info("Attempt to read non page-aligned data\n");
		*length = 0;
		if (actual)
			*actual = 0;
		return -EINVAL;
	}

	need_skip = check_skip_len(mtd, offset, *length, &used_for_read);

	if (actual)
		*actual = used_for_read;

	if (need_skip < 0) {
		pr_info("Attempt to read outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}

	if (used_for_read > lim) {
		puts("Size of read exceeds partition or device limit\n");
		*length = 0;
		return -EFBIG;
	}

	if (!need_skip) {
		rval = mtd_read(mtd, offset, *length, length, buffer);
		if (!rval || rval == -EUCLEAN)
			return 0;

		*length = 0;
		pr_info("%s %d read from offset %llx failed %d\n",
			__func__, __LINE__, offset, rval);
		return rval;
	}

	while (left_to_read > 0) {
		size_t block_offset = offset & (mtd->erasesize - 1);
		size_t read_length;

		//INIT_FUNC_WATCHDOG_RESET();

		if (mtd_block_isbad(mtd, offset & ~(mtd->erasesize - 1))) {
			pr_info("Skipping bad block 0x%08llx\n",
				offset & ~(mtd->erasesize - 1));
			offset += mtd->erasesize - block_offset;
			continue;
		}

		if (left_to_read < (mtd->erasesize - block_offset))
			read_length = left_to_read;
		else
			read_length = mtd->erasesize - block_offset;

		rval = mtd_read(mtd, offset,
				read_length, &read_length, p_buffer);
		if (rval && rval != -EUCLEAN) {
			pr_info("%s %d read from offset %llx failed %d\n",
				__func__, __LINE__, offset, rval);
			*length -= left_to_read;
			return rval;
		}

		left_to_read -= read_length;
		offset       += read_length;
		p_buffer     += read_length;
	}

	return 0;
}

int mtd_store_write_skip_bad(struct mtd_info *mtd,
			     loff_t offset, size_t *length,
			     size_t *actual, loff_t lim,
			     u_char *buffer, int flags)
{
	int rval = 0, blocksize;
	size_t left_to_write = *length;
	size_t used_for_write = 0;
	u_char *p_buffer = buffer;
	int need_skip;

	if (actual)
		*actual = 0;

#ifdef CONFIG_CMD_NAND_YAFFS
	if (flags & WITH_YAFFS_OOB) {
		if (flags & ~WITH_YAFFS_OOB)
			return -EINVAL;

		int pages;

		pages = mtd->erasesize / mtd->writesize;
		blocksize = (pages * mtd->oobsize) + mtd->erasesize;
		if (*length % (mtd->writesize + mtd->oobsize)) {
			pr_info("Attempt to write incomplete page");
			pr_info(" in yaffs mode\n");
			return -EINVAL;
		}
	} else
#endif
	{
		blocksize = mtd->erasesize;
	}

	/*
	 * nand_write() handles unaligned, partial page writes.
	 *
	 * We allow length to be unaligned, for convenience in
	 * using the $filesize variable.
	 *
	 * However, starting at an unaligned offset makes the
	 * semantics of bad block skipping ambiguous (really,
	 * you should only start a block skipping access at a
	 * partition boundary).  So don't try to handle that.
	 */
	if ((offset & (mtd->writesize - 1)) != 0) {
		pr_info("Attempt to write non page-aligned data\n");
		*length = 0;
		return -EINVAL;
	}

	need_skip = check_skip_len(mtd, offset, *length, &used_for_write);

	if (actual)
		*actual = used_for_write;

	if (need_skip < 0) {
		pr_info("Attempt to write outside the flash area\n");
		*length = 0;
		return -EINVAL;
	}

	if (used_for_write > lim) {
		puts("Size of write exceeds partition or device limit\n");
		*length = 0;
		return -EFBIG;
	}

	if (!need_skip && !(flags & WITH_DROP_FFS)) {
		rval = mtd_write(mtd, offset, *length, length, buffer);
		if (rval == 0)
			return 0;

		*length = 0;
		pr_info("%s %d write to offset %llx failed %d\n",
			__func__, __LINE__, offset, rval);
		return rval;
	}

	while (left_to_write > 0) {
		size_t block_offset = offset & (mtd->erasesize - 1);
		size_t write_size, truncated_write_size;

		//INIT_FUNC_WATCHDOG_RESET();

		if (mtd_block_isbad(mtd, offset & ~(mtd->erasesize - 1))) {
			pr_info("Skip bad block 0x%08llx\n",
				offset & ~(mtd->erasesize - 1));
			offset += mtd->erasesize - block_offset;
			continue;
		}

		if (left_to_write < (blocksize - block_offset))
			write_size = left_to_write;
		else
			write_size = blocksize - block_offset;

#ifdef CONFIG_CMD_NAND_YAFFS
		if (flags & WITH_YAFFS_OOB) {
			int page, pages;
			size_t pagesize = mtd->writesize;
			size_t pagesize_oob = pagesize + mtd->oobsize;
			struct mtd_oob_ops ops;

			ops.len = pagesize;
			ops.ooblen = mtd->oobsize;
			ops.mode = MTD_OPS_AUTO_OOB;
			ops.ooboffs = 0;

			pages = write_size / pagesize_oob;
			for (page = 0; page < pages; page++) {
				//INIT_FUNC_WATCHDOG_RESET();

				ops.datbuf = p_buffer;
				ops.oobbuf = ops.datbuf + pagesize;

				rval = mtd_write_oob(mtd, offset, &ops);
				if (rval != 0)
					break;

				offset += pagesize;
				p_buffer += pagesize_oob;
			}
		} else
#endif
		{
			truncated_write_size = write_size;
#ifdef CONFIG_CMD_NAND_TRIMFFS
			if (flags & WITH_DROP_FFS)
				truncated_write_size = drop_ffs(mtd, p_buffer,
								&write_size);
#endif

			rval = mtd_write(mtd, offset, truncated_write_size,
					 &truncated_write_size, p_buffer);
			offset += write_size;
			p_buffer += write_size;
		}

		if (rval != 0) {
			pr_info("%s %d write to offset %llx failed %d\n",
				__func__, __LINE__, offset, rval);
			*length -= left_to_write;
			return rval;
		}

		left_to_write -= write_size;
	}

	return 0;
}

static size_t mtd_store_logic_part_size(struct mtd_info *mtd,
					struct part_info *part)
{
	loff_t start, end;
	u32 cnt = 0;

	start = part->offset;
	end = part->offset + part->size;

	while (start < end) {
		if (mtd_block_isbad(mtd, start))
			cnt++;
		start += mtd->erasesize;
	}
	return part->size - cnt * mtd->erasesize;
}

static int mtd_store_get_offset(const char *partname, loff_t *retoff, loff_t off)
{
	int ret = 0;
	loff_t offset;
#if defined(CONFIG_CMD_MTDPARTS)
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
#endif

	*retoff = 0;
	if (!partname)
		offset = off;

#if defined(CONFIG_CMD_MTDPARTS)
	else if (!mtdparts_init()) {
		ret = find_dev_and_part(partname, &dev, &pnum, &part);
		if (ret) {
			pr_info("%s %d can not find part:%s\n",
				__func__, __LINE__, partname);
			return -EINVAL;
		}

		offset = part->offset + off;
	}
#endif
	else {
		pr_info("%s %d no mtd partition, offset to device 0 address\n",
			__func__, __LINE__);
		offset = off;
	}

	*retoff = offset;

	return ret;
}

extern int get_aml_mtdpart_count(void);
static int mtd_store_count(void)
{
	if (mtdparts_init())
		return -1;
	return get_aml_mtdpart_count();
}

extern int get_aml_mtdpart_name(struct mtd_info *master, int idx, char *name);
static int mtd_store_name(int idx, char *partname)
{
	int ret = 0;
	struct mtd_info *mtd = mtd_store_get(1);

	if (idx >= mtd_store_count())
		return -1;
	ret = get_aml_mtdpart_name(mtd, idx, partname);

	return ret;
}

static u64 mtd_store_size(const char *part_name)
{
	struct mtd_info *mtd = mtd_store_get(1);
	u8 pnum;
	struct mtd_device *dev;
	struct part_info *part;
	int ret;

	if (!part_name)
		return mtd->size;

	if (!mtdparts_init()) {
		ret = find_dev_and_part(part_name,
					&dev,
					&pnum,
					&part);
		if (ret) {
			pr_info("%s %d can not find part:%s\n",
				__func__, __LINE__, part_name);
			return 0;
		}
		return mtd_store_logic_part_size(mtd, part);
	}
	return 0;
}

static int mtd_store_read(const char *part_name,
			  loff_t off, size_t size, void *dest)
{
	struct mtd_info *mtd = mtd_store_get(1);
	loff_t offset = 0;
	size_t retlen = 0;
	int ret;
	u8 backup_num = 0;
	enum boot_type_e medium_type = store_get_type();


	ret = mtd_store_get_offset((const char *)part_name, &offset, off);
	if (ret)
		return ret;
	if (!part_name) {/*normal area except tpl*/
		offset = off;
		offset += meson_rsv_part_get_bl2_part_size(mtd);
		offset += MTD_RSV_BLOCK_CNT * ((u64)mtd->erasesize);

		if ((store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER) ||
		    (store_get_device_bootloader_mode() == ADVANCE_BOOTLOADER))  {
				if (BOOT_NAND_MTD == medium_type ||
					BOOT_SNAND == medium_type)
					backup_num = CONFIG_NAND_TPL_COPY_NUM;
				else if (medium_type == BOOT_SNOR)
					backup_num = CONFIG_NOR_TPL_COPY_NUM;

				offset += CONFIG_TPL_SIZE_PER_COPY * backup_num;
		}
	}
	ret = mtd_store_read_skip_bad(mtd, offset, &size,
				      &retlen, mtd->size, (u_char *)dest);
	if (ret) {
		pr_info("%s %d mtd read err, ret %d\n",
			__func__, __LINE__, ret);
		return ret;
	}
	if (retlen < size) {
		ret = -EIO;
		pr_info("%s %d mtd read err, size(%ld), retlen(%ld)\n",
			__func__, __LINE__, size, retlen);
		return ret;
	}
	return ret;
}

static int mtd_store_write(const char *part_name,
			   loff_t off, size_t size, void *source)
{
	struct mtd_info *mtd = mtd_store_get(1);
	loff_t offset = 0;
	size_t retlen = 0;
	int ret;
	enum boot_type_e medium_type = store_get_type();

	ret = mtd_store_get_offset((const char *)part_name, &offset, off);
	if (ret)
		return ret;
	if (!part_name) {/*normal area except tpl*/
		offset = off;
		offset += meson_rsv_part_get_tpl_start(mtd);
		if ((store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER) ||
		    (store_get_device_bootloader_mode() == ADVANCE_BOOTLOADER)) {
			if (BOOT_NAND_MTD == medium_type ||
				BOOT_SNAND == medium_type)
				offset += CONFIG_TPL_SIZE_PER_COPY *
					CONFIG_NAND_TPL_COPY_NUM;
			else if (medium_type == BOOT_SNOR)
				offset += CONFIG_TPL_SIZE_PER_COPY *
					CONFIG_NOR_TPL_COPY_NUM;
		}
	}
	ret = mtd_store_write_skip_bad(mtd, offset, &size,
				       &retlen, mtd->size, (u_char *)source, 0);
	if (ret) {
		pr_info("%s %d mtd write err, ret %d\n",
			__func__, __LINE__, ret);
		return ret;
	}
	if (retlen < size) {
		ret = -EIO;
		pr_info("%s %d mtd write err, size(%ld), retlen(%ld)\n",
			__func__, __LINE__, size, retlen);
		return ret;
	}
	return ret;
}

extern int meson_block_ignore_erase(struct mtd_info *master, loff_t ofs);
static int _mtd_store_erase(struct mtd_info *mtd,
			   loff_t offset, size_t size, int scrub_flag, int bb_flag)
{
	struct erase_info info;
	unsigned long erased_size, erase_len;
	unsigned long chip_size;
	int ret = 0;

	printf("erasing from 0x%llx, length 0x%lx\n",
			   offset, size);

	chip_size = mtd_store_get(1)->size;
	erase_len = lldiv(size + mtd->erasesize - 1,
			 mtd->erasesize);
	if ((MTD_NORFLASH == mtd->type)
		&& (size == mtd->size) && (0 == offset)) {
		/* erase whole spi flash in one cmd */
		info.mtd = mtd;
		info.addr = offset;
		info.len = size;
		info.scrub = scrub_flag;
		//info.callback = NULL;
		ret = mtd_erase(mtd, &info);
		if (ret)
			pr_info("%s %d mtd erase err, ret %d\n",
				__func__, __LINE__, ret);
	} else {
		for (erased_size = 0; erased_size < erase_len;
			 offset += mtd->erasesize) {
			if (bb_flag)/*erase chip,erase_len include bb*/
				erased_size++;
			//INIT_FUNC_WATCHDOG_RESET();
			if (!scrub_flag) {
				ret = mtd_block_isbad(mtd, offset);
				if (ret > 0) {
					pr_info("skip bad block in 0x%08llx\n", offset);
					/* If the last block of chip is a bad block */
					if (offset == (chip_size - mtd->erasesize))
						return 0;
					continue;
				} else if (ret < 0) {
					pr_info("MTD get bad block failed in 0x%08llx\n",
						offset);
					return ret;
				}

				if (bb_flag && meson_block_ignore_erase(mtd, offset)) {
					pr_info("skip protect block in 0x%08llx\n", offset);
					continue;
				}
			}
			info.mtd = mtd;
			info.addr = offset;
			info.len = mtd->erasesize;
			info.scrub = scrub_flag;
			//info.callback = NULL;
			if (!bb_flag) /*erase partition,erase_len except bb*/
				erased_size++;

			loff_t bootloader_max_addr = meson_rsv_part_get_bl2_part_size(mtd);
			if (offset >= bootloader_max_addr) {
				mtd = mtd_store_get(1);
			}

			ret = mtd_erase(mtd, &info);
			if (ret) {
				pr_info("%s %d mtd erase err, ret %d\n",
					__func__, __LINE__, ret);
				/* If the last block of chip is a bad block */
				if (offset == (chip_size - mtd->erasesize))
					return 0;
			}
		}
	}
	return ret;
}

static int mtd_store_erase(const char *part_name,
			   loff_t off, size_t size, int flag)
{
	struct mtd_info *mtd;
	loff_t offset = 0;
	int ret, scrub_flag = flag & STORE_SCRUB;
	int bb_flag = flag & STORE_ERASE_LEN_BB;

	/*part_name=NULL,operation target is whole device*/
	if (!part_name)	{
		bb_flag = 1;
		mtd = mtd_store_get(1);
		if (flag & STORE_ERASE_DATA) {
			printf("!!!warn: erase all data!!!\n");
			if (mtd_store_get_offset(BOOT_TPL, &offset, 0)) {
				if (mtd_store_get_offset(BOOT_DEVFIP, &offset, 0)) {
					offset = mtd_store_size(BOOT_LOADER);
					offset += mtd_store_size(BOOT_LOADER);
				} else {
					offset += mtd_store_size(BOOT_DEVFIP);
				}
			} else {
				offset += mtd_store_size(BOOT_TPL);
			}
			size = mtd->size - offset;
		} else if (flag & STORE_ERASE_RSV) {
			if (store_get_type() == BOOT_SNOR) {
				printf("!!!warn: spinor without rsv!!!\n");
				return 0;
			}
			printf("!!!warn: erase all rsv!!!\n");
			offset = mtd_store_size("bl2");
			size = meson_rsv_part_get_bl2_part_size(mtd);
		} else {
			printf("!!!warn: erase all chip!!!\n");
			offset = 0;
			size = mtd->size;
			mtd = mtd_store_get(0);
		}
	} else {
		mtd = mtd_store_get_by_name(part_name, 0);
		if (IS_ERR(mtd))
			return -ENXIO;

		ret = mtd_store_get_offset((const char *)part_name, &offset, off);
		if (ret)
			return ret;
		if (size == 0)
			size = mtd_store_size(part_name) - off;
	}

	return _mtd_store_erase(mtd, offset, size, scrub_flag, bb_flag);
}

u8 mtd_store_boot_copy_num(const char *part_name)
{
	enum boot_type_e medium_type = store_get_type();

	if (medium_type == BOOT_SNOR)
		return CONFIG_NOR_TPL_COPY_NUM;

	if (!strcmp(part_name, BOOT_BL2) || !strcmp(part_name, BOOT_SPL))
		return meson_rsv_part_get_bl2_copy_number(mtd_store_get(1));

	return CONFIG_NAND_TPL_COPY_NUM;
}

int is_mtd_store_boot_area(const char *part_name)
{
	if (!part_name) {
		pr_info("%s %d invalid name!\n",
			__func__, __LINE__);
		return 0;
	}

	if ((store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER) ||
	    (store_get_device_bootloader_mode() == ADVANCE_BOOTLOADER)) {
		if (!strcmp(part_name, BOOT_BL2) ||
		    !strcmp(part_name, BOOT_SPL) ||
		    !strcmp(part_name, BOOT_TPL) ||
		    !strcmp(part_name, BOOT_FIP) ||
		    !strcmp(part_name, BOOT_BL2E) ||
		    !strcmp(part_name, BOOT_BL2X) ||
		    !strcmp(part_name, BOOT_DDRFIP) ||
		    !strcmp(part_name, BOOT_DEVFIP))
			return 1;
	} else {
		if (!strcmp(part_name, BOOT_LOADER))
			return 1;
	}
	return 0;
}

static u64 mtd_store_boot_copy_size(const char *part_name)
{
	struct mtd_info *mtd = mtd_store_get(0);

	if (!strcmp(part_name, BOOT_BL2) || !strcmp(part_name, BOOT_SPL))
		return meson_rsv_part_get_bl2_copy_size(mtd);

	return CONFIG_TPL_SIZE_PER_COPY;
}

#ifdef CONFIG_MTD_SPI_NAND
static int mtd_store_spinand_bl2_read(loff_t offset, size_t size,
				      u_char *source, struct mtd_info *mtd)
{
	int i, ret, page_cnt;
	size_t retlen, page_size, read_size = 0;
	size_t data_len = (size > BL2_SIZE) ? BL2_SIZE : size;
	loff_t offset_pos = offset;

	if (store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER)
		page_size = 2048;
	else
		page_size = mtd->writesize;

	page_cnt = (size + mtd->writesize - 1) / mtd->writesize;
	for (i = 0; i < page_cnt; i++) {
		offset_pos = offset + i * mtd->writesize;
		if (!page_info_is_page(mtd, offset_pos / mtd->writesize) &&
		    read_size < data_len) {
			ret = mtd_read(mtd, offset_pos, page_size, &retlen, source + read_size);
			if (ret) {
				printf("read 0x%llx bl2 data failed\n", offset_pos);
				return -EIO;
			}
			read_size += retlen;
		}
	}

	return 0;
}
#endif

static int mtd_store_get_page_info_size(enum boot_type_e medium_type)
{
	if (medium_type == BOOT_SNOR) {
		#ifdef SPINOR_HAS_BOOTINFO
		return 512;
		#else
		return 0;
		#endif
	} else {
		return 0;
	}
}

static int mtd_store_boot_read(const char *part_name,
			       u8 cpy, size_t size, void *dest)
{
	enum boot_type_e medium_type = store_get_type();
	struct mtd_info *mtd;
	loff_t offset, limit, page_info_offset = 0, endoff = 0;
	int ret = 1;
	size_t retlen = 0, len = size;
	u8 num = 0, good_num = 0, good_threshold = 0;
	u64 size_per_copy = 0;

	if (!part_name) {
		pr_info("%s %d invalid name!\n",
			__func__, __LINE__);
		return -ENXIO;
	}
	mtd = mtd_store_get_by_name(part_name, 1);
	if (IS_ERR(mtd))
		return -ENXIO;
	ret = mtd_store_get_offset(part_name, &offset, 0);
	if (ret)
		return -ENXIO;
	num = mtd_store_boot_copy_num(part_name);
	if (num == 0)
		return -ENXIO;
	size_per_copy = mtd_store_boot_copy_size(part_name);
	if (size_per_copy == 0)
		return -ENXIO;

	if (cpy != BOOT_OPS_ALL) {
		if (cpy >= num) {
			pr_info("error: read cpy:0x%x >= num: 0x%x\
					please input again\n",
			cpy, num);
			return -ENXIO;
		}
		offset += (cpy * size_per_copy);
		endoff = offset + size_per_copy;
	} else {
		endoff = offset + num * size_per_copy;
	}

	if (!strcmp(part_name, BOOT_BL2) || !strcmp(part_name, BOOT_SPL))
		good_threshold = DIV_ROUND_UP(mtd_store_boot_copy_num(BOOT_BL2) * 3, 4);

	for (; offset < endoff; offset += size_per_copy) {
		pr_info("read %lx bytes from %llx\n",
			size, offset);

		if (medium_type == BOOT_NAND_MTD && good_threshold)
			limit = offset + ROUND(BL2_SIZE, mtd->erasesize);
		else
			limit = offset + size_per_copy;
		#ifdef CONFIG_MTD_SPI_NAND
		if (medium_type == BOOT_SNAND && good_threshold) {
			ret = mtd_store_spinand_bl2_read(offset, size_per_copy, (u_char *)dest, mtd);
			if (ret)
				continue;
			good_num++;

			continue;
		}
		#endif
		if (medium_type == BOOT_SNOR &&
		    !strcmp(part_name, BOOT_LOADER))
			page_info_offset = mtd_store_get_page_info_size(medium_type);

		ret = mtd_store_read_skip_bad(mtd,
				offset + page_info_offset,
				&len,
				&retlen,
				limit,
				(u_char *)dest);
		if (ret && good_threshold)
			continue;
		else if (ret)
			return -EIO;

		if (good_threshold)
			good_num++;
	}

	if (good_threshold && cpy == BOOT_OPS_ALL &&
	    good_num < good_threshold) {
		pr_err("valid bl2 copy num %d is less than threshold %d\n",
		       good_num, good_threshold);
		return -EIO;
	}

	return ret;
}

#ifdef CONFIG_MTD_SPI_NAND
static int mtd_store_spinand_bl2_write(loff_t offset, size_t size,
				       u_char *source, struct mtd_info *mtd)
{
	int i, page_cnt, ret = 0;
	size_t retlen, page_size, write_size = 0;
	size_t data_len = (size > BL2_SIZE) ? BL2_SIZE : size;
	struct udevice *dev = mtd->dev;
	unsigned char *buf, *pageinfo = page_info_post_init(mtd, dev);
	loff_t offset_pos = offset;

	if (store_get_device_bootloader_mode() == DISCRETE_BOOTLOADER)
		page_size = 2048;
	else
		page_size = mtd->writesize;

	buf = kzalloc(mtd->writesize, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0xff, mtd->writesize);
	memcpy(buf, pageinfo, 512);
	page_cnt = (size + mtd->writesize - 1) / mtd->writesize;
	for (i = 0; i < page_cnt; i++) {
		offset_pos = offset + i * mtd->writesize;
		if (page_info_is_page(mtd, offset_pos / mtd->writesize)) {
			printf("write infopage at 0x%llx\n", offset_pos);
			ret = mtd_write(mtd, offset_pos, mtd->writesize, &retlen, buf);
			if (ret) {
				printf("write 0x%llx pageinfo failed\n", offset_pos);
				goto out;
			}
		} else if (write_size < data_len) {
			ret = mtd_write(mtd, offset_pos, page_size, &retlen, source + write_size);
			if (ret) {
				printf("write 0x%llx bl2 data failed\n", offset_pos);
				goto out;
			}
			write_size += retlen;
		}
	}

out:
	kfree(buf);
	return ret;
}
#endif

static int mtd_store_boot_write(const char *part_name,
				u8 cpy, size_t size, void *source)
{
	enum boot_type_e medium_type = store_get_type();
	struct mtd_info *mtd;
	loff_t offset, limit, endoff = 0;
	int ret = 1;
	size_t retlen = 0, len = size, page_info_len;
	u8 num = 0;
	u64 size_per_copy = 0;
	char *page_info;

	if (!part_name) {
		pr_info("%s %d invalid name!\n",
			__func__, __LINE__);
		return -ENXIO;
	}
	mtd = mtd_store_get_by_name(part_name, 1);
	if (IS_ERR(mtd))
		return -ENXIO;
	ret = mtd_store_get_offset(part_name, &offset, 0);
	if (ret)
		return -ENXIO;
	num = mtd_store_boot_copy_num(part_name);
	if (num == 0)
		return -ENXIO;
	size_per_copy = mtd_store_boot_copy_size(part_name);
	if (size_per_copy == 0)
		return -ENXIO;
	printf("%s %d\n", __func__, __LINE__);
	if (size > size_per_copy)
		return -EINVAL;
	printf("%s %d\n", __func__, __LINE__);

	if (cpy != BOOT_OPS_ALL) {
		if (cpy >= num) {
			pr_info("error: write cpy:0x%x >= num: 0x%x\
					please input again\n",
			cpy, num);
			return -ENXIO;
		}
		offset += (cpy * size_per_copy);
		endoff = offset + size_per_copy;
	} else {
		endoff = offset + num * size_per_copy;
		printf("%s %d endoff = %llx\n", __func__, __LINE__, endoff);
	}
	for (; offset < endoff; offset += size_per_copy) {
		limit = offset + size_per_copy;

		#ifdef CONFIG_MTD_SPI_NAND
		if (medium_type == BOOT_SNAND &&
		    (!strcmp(part_name, BOOT_BL2) ||
		     !strcmp(part_name, BOOT_SPL))) {
			ret = mtd_store_spinand_bl2_write(offset, size_per_copy,
							(u_char *)source, mtd);
			if (ret)
				return -EIO;
			continue;
		}
		#endif

		if (medium_type == BOOT_SNOR &&
		    (!strcmp(part_name, BOOT_BL2) ||
		    !strcmp(part_name, BOOT_SPL) ||
		    !strcmp(part_name, BOOT_LOADER))) {
			page_info_len = mtd_store_get_page_info_size(medium_type);
			page_info = page_info_post_init(mtd, mtd->dev);

			pr_info("write %lx bytes to %llx\n",
				page_info_len, offset);
			ret = mtd_store_write_skip_bad(mtd,
						       offset,
						       &page_info_len,
						       NULL,
						       page_info_len,
						       (u_char *)page_info,
						       0);
			if (ret)
				return ret;
		}

		pr_info("write %lx bytes to %llx\n",
			len, offset + mtd_store_get_page_info_size(medium_type));
		ret = mtd_store_write_skip_bad(mtd,
					       offset + mtd_store_get_page_info_size(medium_type),
					       &len,
					       &retlen,
					       limit,
					       (u_char *)source,
					       0);
		if (ret)
			return -EIO;
	}
	return ret;
}

static int _mtd_store_boot_erase(const char *part_name, u8 cpy)
{
	size_t erasesize = 0;
	loff_t offset = 0;
	struct mtd_info *mtd;
	u8 num;

	num = mtd_store_boot_copy_num(part_name);
	if ((cpy > (num - 1)) && cpy != BOOT_OPS_ALL) {
		pr_info("%s %d invalid copy number %d\n",
			__func__, __LINE__, cpy);
		return 1;
	} else if (cpy == BOOT_OPS_ALL) {
		mtd = get_mtd_device_nm(part_name);
		erasesize = mtd->size;
	} else {
		erasesize = mtd_store_boot_copy_size(part_name);
		offset = cpy * erasesize;
	}
	return mtd_store_erase(part_name, offset, erasesize, STORE_ERASE_LEN_BB);
}

static int mtd_store_boot_erase(const char *part_name, u8 cpy)
{
	static const char * const boot_entry[] = {BOOT_BL2, BOOT_TPL};
	enum boot_type_e medium_type = store_get_type();
	int ret, i;

	if (medium_type != BOOT_SNOR &&
	    !strcmp(part_name, BOOT_LOADER)) {
		if (cpy != BOOT_OPS_ALL) {
			pr_info("unsupport single(%d) bootloader erase\n", cpy);
			return 1;
		}
		for (i = 0; i < 2; i++) {
			ret = _mtd_store_boot_erase(boot_entry[i], cpy);
			if (ret)
				return ret;
		}
		return 0;
	}

	return _mtd_store_boot_erase(part_name, cpy);
}

static u32 mtd_store_rsv_size(const char *rsv_name)
{
	int index = rsvname2index(rsv_name);

	if (index < 0)
		return 0;
	return meson_ext_rsv_info_size(index);
}

static int mtd_store_rsv_read(const char *rsv_name, size_t size, void *dest)
{
	int index = rsvname2index(rsv_name);

	if (index < 0)
		return -1;
	return meson_ext_rsv_info_read(dest, size, index);
}

static int mtd_store_rsv_write(const char *rsv_name, size_t size, void *source)
{
	int index = rsvname2index(rsv_name);

	if (index < 0)
		return -1;
	return meson_ext_rsv_info_write(source, size, index);
}

static int mtd_store_rsv_erase(const char *rsv_name)
{
	int index = rsvname2index(rsv_name);

	if (index < 0)
		return -1;
	return meson_ext_rsv_info_erase(index);
}

static int mtd_store_rsv_protect(const char *rsv_name, bool ops)
{
	if (!rsv_name)
		return 1;
	if (!strcmp(rsv_name, RSV_KEY)) {
		if (ops)
			info_disprotect &= ~DISPROTECT_KEY;/*protect*/
		else

			info_disprotect |= DISPROTECT_KEY;/*disprotect*/

		printf("info_disprotect: %d\n", info_disprotect);
	} else if (!strcmp(rsv_name, RSV_BBT)) {
		if (ops)
			info_disprotect &= ~DISPROTECT_FBBT;/*protect*/
		else
			info_disprotect |= DISPROTECT_FBBT;/*disprotect*/
		printf("info_disprotect: %d\n", info_disprotect);

	} else {
		pr_info("no matched info in protection!\n");
	}
	return 0;
}

static u32 nor_rsv_size(const char *name)
{
	if (!name)
		return 0;
	return mtd_store_size(name);
}

static int nor_rsv_read(const char *name, size_t size, void *buf)
{
	struct mtd_info *mtd;
	loff_t offset;
	size_t length, total;
	int ret = 0;

	if (!name)
		return 1;
	length = size;
	mtd = mtd_store_get(0);
	total = mtd_store_size(name);
	if (length > total) {
		pr_info("request 0x%lx over the rsv size 0x%lx\n",
			length, total);
		return 1;
	}
	ret = mtd_store_get_offset(name, &offset, 0);
	if (ret)
		return ret;
	return mtd_read(mtd, offset, length, &length, buf);
}

static int nor_rsv_erase(const char *name)
{
	struct mtd_info *mtd;
	loff_t offset;
	size_t length;
	struct erase_info erase;
	int ret = 0;

	if (!name)
		return 1;
	length = nor_rsv_size(name);
	mtd = mtd_store_get(0);
	ret = mtd_store_get_offset(name, &offset, 0);
	if (ret)
		return ret;
	erase.mtd = mtd;
	erase.addr = offset;
	erase.len = length;
	//erase.callback = NULL;
	return mtd_erase(mtd, &erase);
}

static int nor_rsv_write(const char *name, size_t size, void *buf)
{
	struct mtd_info *mtd;
	loff_t offset;
	size_t length, total;
	int ret = 0;
	struct erase_info erase;

	if (!name)
		return 1;

	length = size;
	mtd = mtd_store_get(0);
	total = mtd_store_size(name);
	if (length > total) {
		pr_info("request 0x%lx over the rsv size 0x%lx\n",
			length, total);
		return 1;
	}

	ret = mtd_store_get_offset(name, &offset, 0);
	if (ret)
		return ret;
	/* special path for erase */
	if (!strcmp(name, RSV_ENV) || !strcmp(name, RSV_DTB) || !strcmp(name, RSV_KEY)) {
		erase.mtd = mtd;
		erase.addr = offset;
		//erase.callback = NULL;
		erase.len = length;
		if (mtd_mod_by_eb(length, mtd))
			erase.len = (mtd_div_by_eb(length, mtd) + 1)
					* mtd->erasesize;
		ret = mtd_erase(mtd, &erase);
		if (!ret)
			printf("%s() %d, erase %s error %d\n",
				__func__, __LINE__, name, ret);
	}

	return mtd_write(mtd, offset, length, &length, buf);
}

static int nor_rsv_protect(const char *name, bool ops)
{
	pr_info("spi nor flash not support this yet!\n");
	return 0;
}

static int mtd_store_get_fdt_node_offset(enum boot_type_e medium_type)
{
	char buf[128];
	char *fdtaddr = NULL;
	u32 mem_dtb;
	int node_offset;

	if (!working_fdt) {
		pr_err("%s: working_fdt not set, fdt add to set working_fdt\n",
		       __FILE__);
		fdtaddr = env_get("dtb_mem_addr");
		if (!fdtaddr) {
			pr_err("get dtb_mem_addr NULL\n");
			return -EBADMSG;
		}
		mem_dtb = simple_strtoul(fdtaddr, NULL, 16);
		sprintf(buf, "fdt addr 0x%x", mem_dtb);
		pr_err("fdt addr 0x%x\n", mem_dtb);
		if (run_command(buf, 0)) {
			pr_err("fdt addr 0x%x error.\n", mem_dtb);
			return -EBADMSG;
		}
	}

	if (working_fdt) {
		if (medium_type == BOOT_SNAND)
			node_offset = fdt_node_offset_by_compatible(working_fdt, -1, "spi-nand");
		else if (medium_type == BOOT_NAND_MTD)
			node_offset = fdt_path_offset(working_fdt, "/soc/nfc");
		else
			return 0;
	} else {
		return -1;
	}

	return node_offset;
}

static int mtd_store_param_rsv_partition(void)
{
	int parent_offset;

	parent_offset = mtd_store_get_fdt_node_offset(store_get_type());
	if (parent_offset > 0)
		return meson_rsv_add_dtb(working_fdt, parent_offset);

	return 0;
}

static int mtd_store_param_bl2_partition(void)
{
	char buf[128];
	char *fdtaddr = NULL;
	u32 mem_dtb;
	enum boot_type_e medium_type = store_get_type();
	int node_offset, err = 0;
	int count = mtd_store_count();
	struct part_info *bl2 = NULL;

	if (!working_fdt) {
		pr_err("%s: working_fdt is set, fdt add to set working_fdt\n",
		       __FILE__);
		fdtaddr = env_get("dtb_mem_addr");
		if (!fdtaddr) {
			pr_err("get dtb_mem_addr NULL\n");
			return -EBADMSG;
		}
		mem_dtb = simple_strtoul(fdtaddr, NULL, 16);
		sprintf(buf, "fdt addr 0x%x", mem_dtb);
		pr_err("fdt addr 0x%x\n", mem_dtb);
		if (run_command(buf, 0)) {
			pr_err("fdt addr 0x%x error.\n", mem_dtb);
			return -EBADMSG;
		}
	}

	if (working_fdt && medium_type == BOOT_NAND_MTD) {
		node_offset = fdt_path_offset(working_fdt,
						 "/soc/nfc/nand@bootloader/partition@0");

		if (count > 0) {
			bl2 = get_aml_mtdpart_by_index(NULL, 0);
			if (!bl2)
				return -EINVAL;

			memset(buf, 0, sizeof(buf));
			*(fdt32_t *)&buf[0] = cpu_to_fdt32(bl2->offset);
			*(fdt32_t *)&buf[4] = cpu_to_fdt32(bl2->size);
			err = fdt_setprop(working_fdt, node_offset, "reg", buf, 8);
			if (err < 0)
				printf("WARNING: could not set bootloader property: %s\n",
				       fdt_strerror(err));
		}

		return err;
	}

	return 0;
}

static int mtd_store_param_boot_layout(void)
{
	struct mtd_info *mtd = mtd_store_get(0);
	int node_offset, err = -1;
	/* bit0:3 - boot layout
	 * bit4:7 - bl2 layout
	 * bit8:11 - bl2 copy number,
	 * bit12:22 - page number per bl2 copy
	 */
	u32 boot_layout;
	u8 bl2_copy_number = meson_rsv_part_get_bl2_copy_number(mtd);
	u16 pages_per_copy = meson_rsv_part_get_bl2_copy_size(mtd) / mtd->writesize;

#ifdef BOARD_BOOT_LAYOUT_DISCRETE_BL2
	boot_layout = BOOT_DISCRETE_BL2;
#else
	boot_layout = BOOT_DISCRETE_ALL;
#endif
	boot_layout |= (BOARD_CONFIG_BL2_LAYOUT_TYPE << 4);
	boot_layout |= ((bl2_copy_number & 0x0F) << 8);
	boot_layout |= ((pages_per_copy & 0x3FF) << 12);

add:
	node_offset = mtd_store_get_fdt_node_offset(store_get_type());
	if (node_offset > 0) {
		err = fdt_setprop_cell(working_fdt, node_offset, "boot_layout",
				       boot_layout);
		if (err == -FDT_ERR_NOSPACE) {
			err = fdt_increase_size(working_fdt, 512);
			if (!err)
				goto add;
		}
	}

	return err;
}

/**
 * Format string describing supplied size. This routine does the opposite job
 * to memsize_parse(). Size in bytes is converted to string and if possible
 * shortened by using k (kilobytes), m (megabytes) or g (gigabytes) suffix.
 *
 * Note, that this routine does not check for buffer overflow, it's the caller
 * who must assure enough space.
 *
 * @param buf : output buffer
 * @param size : size to be converted to string
 */
static void memsize_format(char *buf, u64 size)
{
#define SIZE_GB ((u32)1024 * 1024 * 1024)
#define SIZE_MB ((u32)1024 * 1024)
#define SIZE_KB ((u32)1024)

	if ((size % SZ_1G) == 0)
		sprintf(buf, "%llug", size / SZ_1G);
	else if ((size % SZ_1M) == 0)
		sprintf(buf, "%llum", size / SZ_1M);
	else if (size % SZ_1K == 0)
		sprintf(buf, "%lluk", size / SZ_1K);
	else
		sprintf(buf, "%llu", size);
}

int mtd_store_param_partition(void)
{
	struct part_info *temp;
	int lenvir, i, re, count;
	char buf[512], size[32], offset[32];
	char *p = buf;
	enum boot_type_e medium_type = store_get_type();

	i = 0;
	/* slcnand bl2 pass to kernel still by dtb, other partitions by cmdline */
	if (medium_type == BOOT_NAND_MTD) {
		mtd_store_param_bl2_partition();
		i = 1;
	}

	if (strstr(env_get("bootargs"), "mtdparts")) {
		pr_info("mtdparts already exit in bootargs!\n");
		return 0;
	}

	count = mtd_store_count();
	lenvir = snprintf(buf, sizeof(buf), "%s", "mtdparts=aml-mtd:");
	p += lenvir;
	re = sizeof(buf) - lenvir;

	for (; i < count; i++) {
		temp = get_aml_mtdpart_by_index(NULL, i);
		if (!temp)
			return -EINVAL;

		memset(size, 0, sizeof(size));
		memsize_format(size, temp->size);
		memset(offset, 0, sizeof(offset));
		memsize_format(offset, temp->offset);
		lenvir = snprintf(p, re, "%s@%s(%s),", size, offset, temp->name);
		re -= lenvir;
		p += lenvir;
	}
	p = buf;
	buf[strlen(p) - 1] = 0;	/* delete the last comma */
	env_set("mtdparts", p);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "setenv bootargs ${bootargs} ${mtdparts}");
	printf("command: %s\n", buf);

	return run_command(buf, 0);
}

int mtd_store_param_ops(void)
{
	int ret = 0;

	ret = mtd_store_param_partition();
	if (ret)
		goto error;
	ret = mtd_store_param_rsv_partition();
	if (ret)
		goto error;
	ret = mtd_store_param_boot_layout();

error:
	return ret;
}

void mtd_store_mount_ops(struct storage_t *store)
{
	store->get_part_count = mtd_store_count;
	store->list_part_name = mtd_store_name;
	store->get_part_size = mtd_store_size;
	store->read = mtd_store_read;
	store->write = mtd_store_write;
	store->erase = mtd_store_erase;
	store->boot_read = mtd_store_boot_read;
	store->boot_write = mtd_store_boot_write;
	store->boot_erase = mtd_store_boot_erase;
	store->get_copies = mtd_store_boot_copy_num;
	store->get_copy_size = mtd_store_boot_copy_size;
	store->param_ops = mtd_store_param_ops;
	if (store->type == BOOT_SNOR) {
		store->get_rsv_size = nor_rsv_size;
		store->read_rsv = nor_rsv_read;
		store->write_rsv = nor_rsv_write;
		store->erase_rsv = nor_rsv_erase;
		store->protect_rsv = nor_rsv_protect;
	} else {
		store->get_rsv_size = mtd_store_rsv_size;
		store->read_rsv = mtd_store_rsv_read;
		store->write_rsv = mtd_store_rsv_write;
		store->erase_rsv = mtd_store_rsv_erase;
		store->protect_rsv = mtd_store_rsv_protect;
	}
}
