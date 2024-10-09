// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <nand.h>
#include <asm/io.h>
#include <malloc.h>
#include <linux/err.h>
#include <asm/cache.h>
//#include <asm/arch/secure_apb.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand_ecc.h>
#include <amlogic/meson_nand.h>
#include <amlogic/aml_pageinfo.h>
#include "version.h"

extern struct mtd_info *nand_info[CONFIG_SYS_MAX_NAND_DEVICE];
extern struct hw_controller *controller;
/* provide a policy that calculate the backups of bootloader */
int get_boot_num(struct mtd_info *mtd, size_t rwsize)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	size_t bad_blk_len_low = 0, bad_blk_len_up = 0, skip;
	size_t available_space;
	size_t block_len, block_off;
	loff_t block_start;
	loff_t offset = 0;
	int ret = 1; /*inital for only one copy*/

	if (!rwsize) { /*not need to policy call, only one */
		ret = 1;
		return ret;
	}

	/* align with page size */
	rwsize = ((rwsize + mtd->writesize - 1)/mtd->writesize)*mtd->writesize;

	while (offset < mtd->size) {
		block_start = offset & ~(loff_t)(mtd->erasesize - 1);
		block_off = offset & (mtd->erasesize - 1);
		block_len = mtd->erasesize - block_off;

		if (nand_block_isbad(mtd, block_start)) {
			if (offset < mtd->size / 2)   /*no understand*/
				bad_blk_len_low += block_len;
			else if (offset > mtd->size / 2)
				bad_blk_len_up += block_len;
			else {
				bad_blk_len_up = offset;
			}
		}
		offset += block_len;
	}

	printk("rwsize:0x%zx skip_low:0x%zx skip_up:0x%zx\n",
		rwsize, bad_blk_len_low, bad_blk_len_up);

	skip = bad_blk_len_low + bad_blk_len_up;
	available_space = mtd->size - skip - 2 * mtd->writesize; /*no understand*/

	if (rwsize * 2 <= available_space) {
		ret = 1;
		if (rwsize + mtd->writesize + bad_blk_len_low > mtd->size / 2)
			return 1; /*1st must be write*/
		if (rwsize + mtd->writesize + bad_blk_len_up <= mtd->size / 2)
			ret++;
	} else /*needn't consider bad block length, unlikely so many bad blocks*/
		ret = 1;

	aml_chip->boot_copy_num = ret;
	printk("self-adaption boot count:%d\n", ret);

	return ret;
}

extern void page_info_init_from_mtd_and_dts(struct mtd_info *mtd, struct udevice *udev);
void nand_info_page_prepare(struct aml_nand_chip *aml_chip, u8 *page0_buf)
{
	struct nand_chip *chip = &aml_chip->chip;
	struct mtd_info *mtd = &chip->mtd;
	struct udevice *dev = mtd->dev;
	unsigned char *pageinfo;

	pageinfo = page_info_post_init(mtd, dev);
	memcpy(page0_buf, pageinfo, 512);
}

/* mtd support interface:
 * function:int (*_erase) (struct mtd_info *mtd, struct erase_info *instr);
 */
int m3_nand_boot_erase_cmd(struct mtd_info *mtd, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	struct nand_chip *chip = mtd->priv;
	loff_t ofs;

	ofs = ((loff_t)page << chip->page_shift);

	if (chip->block_bad(mtd, ofs))
		return 0;
	aml_chip->aml_nand_select_chip(aml_chip, 0);
	aml_chip->aml_nand_command(aml_chip,
		NAND_CMD_ERASE1, -1, page, 0);
	aml_chip->aml_nand_command(aml_chip,
		NAND_CMD_ERASE2, -1, -1, 0);
	chip->waitfunc(mtd, chip);

	return 0;
}

/* mtd support interface:
 * chip->ecc.read_page
 * function:int (*read_page)(struct mtd_info *mtd, struct nand_chip *chip,
 *		uint8_t *buf, int oob_required, int page);
 */
int m3_nand_boot_read_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = chip->ecc.steps * chip->ecc.size;
	unsigned pages_per_blk_shift = chip->phys_erase_shift - chip->page_shift;
	int user_byte_num = (chip->ecc.steps * aml_chip->user_byte_mode);
	int bch_mode = aml_chip->bch_mode;
	int error = 0, i = 0, stat = 0;
	int ecc_size, configure_data_w, read_page, each_boot_pages;
	loff_t ofs;


	each_boot_pages =
	meson_rsv_part_get_bl2_copy_size(mtd) >> mtd->writesize_shift;

	if ((page << mtd->writesize_shift) >=
	     meson_rsv_part_get_bl2_part_size(mtd)) {
		memset(buf, 0, (1 << chip->page_shift));
		printk("nand boot read out of uboot failed, page:%d\n", page);
		goto exit;
	}
	/* nand page info */
	if ((page % each_boot_pages) == 0) {
		configure_data_w =
				NFC_CMD_N2M(aml_chip->ran_mode,
		aml_chip->bch_mode, 0, (chip->ecc.size >> 3), chip->ecc.steps);

		ecc_size = chip->ecc.size;  //backup ecc size
		bch_mode = aml_chip->bch_mode;

		chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
		memset(buf, 0xff, (1 << chip->page_shift));
		/* read back page0 and check it */
		if (aml_chip->valid_chip[0]) {
			if (!aml_chip->aml_nand_wait_devready(aml_chip, i)) {
				printk("don't found selected chip:%d ready\n",
					i);
				//error = -EBUSY;
			}
			if (aml_chip->ops_mode & AML_CHIP_NONE_RB)
				chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff,
					NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

			error = aml_chip->aml_nand_dma_read(aml_chip,
				buf, nand_page_size, bch_mode);

			if (error) {
				printk(" page0 aml_nand_dma_read failed\n");
			}

			aml_chip->aml_nand_get_user_byte(aml_chip,
				oob_buf, user_byte_num);
			stat = aml_chip->aml_nand_hwecc_correct(aml_chip,
				buf, nand_page_size, oob_buf);
			if (stat < 0) {
				if (aml_chip->ran_mode
				&& (aml_chip->zero_cnt <  aml_chip->ecc_max)) {
					memset(buf, 0xff, nand_page_size);
					memset(oob_buf, 0xff, user_byte_num);
				} else {
					mtd->ecc_stats.failed++;
					printk("page0 read ecc failed at blk0 chip0\n");
				}
			} else
				mtd->ecc_stats.corrected += stat;

		} else {
			printk("nand boot page 0 no valid chip failed\n");
			error = -ENODEV;
			//goto exit;
		}

		bch_mode = aml_chip->bch_mode;
		chip->ecc.size = ecc_size;
		nand_page_size = chip->ecc.steps * chip->ecc.size;
	}

	read_page = page;
	read_page++;
READ_BAD_BLOCK:
	ofs = ((loff_t)read_page << chip->page_shift);
	if (!(ofs % mtd->erasesize)) {
		if (chip->block_bad(mtd, ofs)) {
			read_page +=
				1 << (chip->phys_erase_shift-chip->page_shift);
			goto READ_BAD_BLOCK;
		}
	}

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, read_page);

	memset(buf, 0xff, (1 << chip->page_shift));
	if (aml_chip->valid_chip[0]) {
		if (!aml_chip->aml_nand_wait_devready(aml_chip, 0)) {
			printk("don't found selected chip0 ready, page: %d \n",
				page);
			error = -EBUSY;
			goto exit;
		}
		if (aml_chip->ops_mode & AML_CHIP_NONE_RB)
			chip->cmd_ctrl(mtd, NAND_CMD_READ0 & 0xff,
				NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

		error = aml_chip->aml_nand_dma_read(aml_chip,
			buf, nand_page_size, bch_mode);
		if (error) {
			error = -ENODEV;
			printk("aml_nand_dma_read failed: page:%d \n", page);
			goto exit;
		}

		aml_chip->aml_nand_get_user_byte(aml_chip,
			oob_buf, user_byte_num);
		stat = aml_chip->aml_nand_hwecc_correct(aml_chip,
			buf, nand_page_size, oob_buf);
		if (stat < 0) {
			error = -ENODEV;
			mtd->ecc_stats.failed++;
			printk("read data ecc failed at page%d blk%d chip%d\n",
				page, (page >> pages_per_blk_shift), i);
		} else
			mtd->ecc_stats.corrected += stat;
		} else
		error = -ENODEV;

exit:
	return error;
}


/* mtd support interface:
 * chip->ecc.write_page
 * function:int (*write_page)(struct mtd_info *mtd, struct nand_chip *chip,
 *		uint8_t *buf, int oob_required, int page);
 */
int m3_nand_boot_write_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required,
	int page)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	uint8_t *oob_buf = chip->oob_poi;
	unsigned nand_page_size = chip->ecc.steps * chip->ecc.size;
	int user_byte_num = (chip->ecc.steps * aml_chip->user_byte_mode);
	int error = 0, i = 0, bch_mode;

	bch_mode = aml_chip->bch_mode;

	/* setting magic for romboot checks. */
	for (i = 0; i < mtd->oobavail; i += 2) {
		oob_buf[i] = 0x55;
		oob_buf[i+1] = 0xaa;
	}

	i = 0;
	if (aml_chip->valid_chip[i]) {
		aml_chip->aml_nand_select_chip(aml_chip, i);
		aml_chip->aml_nand_set_user_byte(aml_chip,
			oob_buf, user_byte_num);
		error = aml_chip->aml_nand_dma_write(aml_chip,
			(unsigned char *)buf, nand_page_size, bch_mode);
		if (error)
			goto exit;
		aml_chip->aml_nand_command(aml_chip,
			NAND_CMD_PAGEPROG, -1, -1, i);
	} else {
		error = -ENODEV;
		goto exit;
	}
exit:
	return error;
}

/* mtd support interface:
 * chip->write_page
 * function:	int (*write_page)(struct mtd_info *mtd, struct nand_chip *chip,
 *			uint32_t offset, int data_len, const uint8_t *buf,
 *			int oob_required, int page, int cached, int raw);
 */
int m3_nand_boot_write_page(struct mtd_info *mtd, struct nand_chip *chip,
	uint32_t offset, int data_len, const uint8_t *buf,
	int oob_required, int page, int raw)
{
	struct aml_nand_chip *aml_chip = mtd_to_nand_chip(mtd);
	int status, write_page;
	int en_slc = 0, each_boot_pages;
	loff_t ofs;

	each_boot_pages =
	meson_rsv_part_get_bl2_copy_size(mtd) >> mtd->writesize_shift;
	/* actual page to be written */
	write_page = page;
	/* zero page of each copy */
	if ((write_page % each_boot_pages) == 0) {
		nand_info_page_prepare(aml_chip, chip->buffers->databuf);
		chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, write_page);
		chip->ecc.write_page(mtd, chip, chip->buffers->databuf, 0, 0);

		status = chip->waitfunc(mtd, chip);

		if (status & NAND_STATUS_FAIL) {
			printk("uboot wr 0 page=0x%x, status=0x%x\n",
				page, status);
			return -EIO;
		}
	}
	/* +1 for skipping nand info page */
	if (en_slc) {
	} else
		write_page++;

WRITE_BAD_BLOCK:
	ofs = ((loff_t)write_page << chip->page_shift);
	if (!(ofs % mtd->erasesize)) {
		if (chip->block_bad(mtd, ofs)) {
			write_page +=
			1 << (chip->phys_erase_shift-chip->page_shift);
			goto WRITE_BAD_BLOCK;
		}
	}
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, write_page);

	if (unlikely(raw))
		chip->ecc.write_page_raw(mtd, chip, buf, 0, 0);
	else
		chip->ecc.write_page(mtd, chip, buf, 0, 0);

	if (!(chip->options & NAND_CACHEPRG)) {
		status = chip->waitfunc(mtd, chip);

		if (status & NAND_STATUS_FAIL) {
			printk("uboot wr page=0x%x, status=0x%x\n",
				page, status);
			return -EIO;
		}
	} else
		status = chip->waitfunc(mtd, chip);
	return 0;
}
