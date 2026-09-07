// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 */

#include <config.h>
#include <common.h>
#include <blk.h>
#include <dm.h>
#include <part.h>
#include <div64.h>
#include <linux/math64.h>
#include "mmc_private.h"
#ifdef CONFIG_AMLOGIC_MODIFY
#include <linux/log2.h>
#include <memalign.h>
#endif

static ulong mmc_erase_t(struct mmc *mmc, ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	ulong end;
	int err, start_cmd, end_cmd;

	if (mmc->high_capacity) {
		end = start + blkcnt - 1;
	} else {
		end = (start + blkcnt - 1) * mmc->write_bl_len;
		start *= mmc->write_bl_len;
	}

	if (IS_SD(mmc)) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}

	cmd.cmdidx = start_cmd;
	cmd.cmdarg = start;
	cmd.resp_type = MMC_RSP_R1;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = end_cmd;
	cmd.cmdarg = end;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = MMC_ERASE_ARG;
	cmd.resp_type = MMC_RSP_R1b;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	puts("mmc erase failed\n");
	return err;
}

/*align to bottom*/
#ifdef CONFIG_AMLOGIC_MODIFY
static ulong mmc_align_erase_size(struct mmc *mmc, lbaint_t *from, lbaint_t *to, u32 nr)
{
	lbaint_t from_new = *from;
	u32 nr_new = nr;
	lbaint_t rem, temp;

	if (!is_power_of_2(mmc->erase_grp_size))
		return 0;

	temp = from_new;
	from_new = round_up(temp, mmc->erase_grp_size);
	rem = from_new - temp;

	if (nr_new <= rem) {
		*to = *from + nr;
		return 0;
	}

	nr_new -= rem;
	nr_new = round_down(nr_new, mmc->erase_grp_size);

	*to = from_new + nr_new;
	*from = from_new;

	return nr_new;
}

static int aml_write_rem(struct udevice *dev, lbaint_t *start, lbaint_t *blkcnt)
{
	struct blk_desc *block_dev = dev_get_uclass_plat(dev);
	struct mmc *mmc = find_mmc_device(block_dev->devnum);
	lbaint_t start_align = *start;
	lbaint_t blk_origin = *blkcnt;
	lbaint_t start_rem, blkcnt_rem, to, erase_blkcnt = 0;
	lbaint_t write_total = 0;
	lbaint_t remaining, write_chunk;
	void *buffer = NULL;
	u32 max_blocks, buf_blocks;
	int ret = 0;

	if (!mmc)
		return -ENODEV;

	max_blocks = mmc->cfg->b_max;

	/* get largest continuous block */
	if (blk_origin < mmc->erase_grp_size) {
		buf_blocks = min(blk_origin, (lbaint_t)max_blocks);
	} else {
		start_rem = round_up(*start, mmc->erase_grp_size) - *start;
		blkcnt_rem = *start + blk_origin -
					round_down(*start + blk_origin, mmc->erase_grp_size);
		buf_blocks = min(max_blocks, (u32)max(start_rem, blkcnt_rem));
	}

	/* atlease 1 bock */
	buf_blocks = max(buf_blocks, 1U);

	buffer = malloc_cache_aligned(buf_blocks * 512);
	if (!buffer) {
		printf("aml_write_rem: Failed to allocate %u blocks buffer\n", buf_blocks);
		return -ENOMEM;
	}

	memset(buffer, 0, buf_blocks * 512);

	printf("Caution! Erase group 0x%x not aligned, using write-zero buf=%u blks, max=%u blks\n",
	       mmc->erase_grp_size, buf_blocks, max_blocks);

	/* erase blk < erase_group */
	if (blk_origin < mmc->erase_grp_size) {
		remaining = blk_origin;
		lbaint_t current_start = *start;

		while (remaining > 0) {
			write_chunk = min(remaining, (lbaint_t)buf_blocks);
			ulong cnt = mmc_bwrite(dev, current_start, write_chunk, buffer);

			if (cnt != write_chunk) {
				printf("Write failed at " LBAF " (req=%lu, ret=%lu)\n",
				       current_start, (ulong)write_chunk, cnt);
				ret = -EIO;
				goto out;
			}

			write_total += cnt;
			current_start += cnt;
			remaining -= cnt;
		}
		ret = 0;
	} else {
		/* align erase range */
		erase_blkcnt = mmc_align_erase_size(mmc, &start_align, &to, blk_origin);

		/* header not align */
		start_rem = start_align - *start;
		if (start_rem > 0) {
			remaining = start_rem;
			lbaint_t current_start = *start;

			while (remaining > 0) {
				write_chunk = min(remaining, (lbaint_t)buf_blocks);
				ulong cnt = mmc_bwrite(dev, current_start, write_chunk, buffer);

				if (cnt != write_chunk) {
					printf("Head write failed at " LBAF "\n", current_start);
					ret = -EIO;
					goto out;
				}

				write_total += cnt;
				current_start += cnt;
				remaining -= cnt;
			}
		}

		/* tail not align */
		blkcnt_rem = *start + blk_origin - to;
		if (blkcnt_rem > 0) {
			remaining = blkcnt_rem;
			lbaint_t current_start = to;

			while (remaining > 0) {
				write_chunk = min(remaining, (lbaint_t)buf_blocks);
				ulong cnt = mmc_bwrite(dev, current_start, write_chunk, buffer);

				if (cnt != write_chunk) {
					printf("Tail write failed at " LBAF "\n", current_start);
					ret = -EIO;
					goto out;
				}

				write_total += cnt;
				current_start += cnt;
				remaining -= cnt;
			}
		}
	}

out:
	if (buffer)
		free(buffer);

	if (ret == 0) {
		*start = start_align;
		*blkcnt = erase_blkcnt;
	}

	return ret;
}
#endif

#if CONFIG_IS_ENABLED(BLK)
ulong mmc_berase(struct udevice *dev, lbaint_t start, lbaint_t blkcnt)
#else
ulong mmc_berase(struct blk_desc *block_dev, lbaint_t start, lbaint_t blkcnt)
#endif
{
#if CONFIG_IS_ENABLED(BLK)
	struct blk_desc *block_dev = dev_get_uclass_plat(dev);
#endif
	int dev_num = block_dev->devnum;
	int err = 0;
	u32 start_rem, blkcnt_rem;
	struct mmc *mmc = find_mmc_device(dev_num);
	lbaint_t blk = 0, blk_r = 0;
	int timeout_ms = 1000;
#ifdef CONFIG_AMLOGIC_MODIFY
	lbaint_t blkcnt_origin = blkcnt;
#endif

	if (!mmc)
		return -1;

	err = blk_select_hwpart_devnum(UCLASS_MMC, dev_num,
				       block_dev->hwpart);
	if (err < 0)
		return -1;

	/*
	 * We want to see if the requested start or total block count are
	 * unaligned.  We discard the whole numbers and only care about the
	 * remainder.
	 */
	err = div_u64_rem(start, mmc->erase_grp_size, &start_rem);
	err = div_u64_rem(blkcnt, mmc->erase_grp_size, &blkcnt_rem);
	if (start_rem || blkcnt_rem) {
		if (IS_ENABLED(CONFIG_AMLOGIC_MODIFY)) {
			err = aml_write_rem(dev, &start, &blkcnt);
			if (err)
				return err;
		} else {
			 printf("\n\nCaution! Your devices Erase group is 0x%x\n"
		       "The erase range would be change to "
		       "0x" LBAF "~0x" LBAF "\n\n",
		       mmc->erase_grp_size, start & ~(mmc->erase_grp_size - 1),
		       ((start + blkcnt + mmc->erase_grp_size - 1)
		       & ~(mmc->erase_grp_size - 1)) - 1);
		 }
	}

	while (blk < blkcnt) {
		if (IS_SD(mmc) && mmc->ssr.au) {
			blk_r = ((blkcnt - blk) > mmc->ssr.au) ?
				mmc->ssr.au : (blkcnt - blk);
		} else {
			blk_r = ((blkcnt - blk) < mmc->erase_grp_size) ?
				mmc->erase_grp_size : (blkcnt - blk);
		}
		err = mmc_erase_t(mmc, start + blk, blk_r);
		if (err)
			break;

		blk += blk_r;

		/* Waiting for the ready status */
		if (mmc_poll_for_busy(mmc, timeout_ms))
			return 0;
	}

	/*when erase success return ok*/
	if (IS_ENABLED(CONFIG_AMLOGIC_MODIFY)) {
		if (!err)
			return blkcnt_origin;
	}

	return blk;
}

static ulong mmc_write_blocks(struct mmc *mmc, lbaint_t start,
		lbaint_t blkcnt, const void *src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout_ms = 1000;

	if ((start + blkcnt) > mmc_get_blk_desc(mmc)->lba) {
		printf("MMC: block number 0x" LBAF " exceeds max(0x" LBAF ")\n",
		       start + blkcnt, mmc_get_blk_desc(mmc)->lba);
		return 0;
	}

	if (blkcnt == 0)
		return 0;
	else if (blkcnt == 1)
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize = mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	if (mmc_send_cmd(mmc, &cmd, &data)) {
		printf("mmc write failed\n");
		return 0;
	}

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
			printf("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	/* Waiting for the ready status */
	if (mmc_poll_for_busy(mmc, timeout_ms))
		return 0;

	return blkcnt;
}

#if CONFIG_IS_ENABLED(BLK)
ulong mmc_bwrite(struct udevice *dev, lbaint_t start, lbaint_t blkcnt,
		 const void *src)
#else
ulong mmc_bwrite(struct blk_desc *block_dev, lbaint_t start, lbaint_t blkcnt,
		 const void *src)
#endif
{
#if CONFIG_IS_ENABLED(BLK)
	struct blk_desc *block_dev = dev_get_uclass_plat(dev);
#endif
	int dev_num = block_dev->devnum;
	lbaint_t cur, blocks_todo = blkcnt;
	int err;

	struct mmc *mmc = find_mmc_device(dev_num);
	if (!mmc)
		return 0;

	err = blk_select_hwpart_devnum(UCLASS_MMC, dev_num, block_dev->hwpart);
	if (err < 0)
		return 0;

	if (mmc_set_blocklen(mmc, mmc->write_bl_len))
		return 0;

	do {
		cur = (blocks_todo > mmc->cfg->b_max) ?
			mmc->cfg->b_max : blocks_todo;
		if (mmc_write_blocks(mmc, start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur * mmc->write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}
