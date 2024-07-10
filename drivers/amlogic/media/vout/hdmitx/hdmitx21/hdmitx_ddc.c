// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <amlogic/media/vout/aml_vout.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include <linux/compat.h>
#include "hdmitx_drv.h"
#include "../hdmitx_common/hdmitx_log.h"

#define TPI_DDC_CMD_ENHANCED_DDC_READ  0x04
#define TPI_DDC_CMD_SEQUENTIAL_READ    0x02
#define LEN_TPI_DDC_FIFO_SIZE          16
#define usleep_range(a, b) udelay(a)
/* DDC func */
DEFINE_MUTEX(ddc_mutex);

static u32 ddc_write_1byte(u8 slave, u8 offset_addr, u8 data)
{
	u32 st = 0;

	/* Programe I2C operation */
	/* SCDC slave addr */
	hdmitx21_wr_reg(DDC_ADDR_IVCTX, 0xa8);
	/* SCDC slave offset */
	hdmitx21_wr_reg(DDC_OFFSET_IVCTX, offset_addr & 0xff);
	/* SCDC slave offset data to ddc fifo */
	hdmitx21_wr_reg(DDC_DATA_AON_IVCTX, data & 0xff);
	/* data length lo */
	hdmitx21_wr_reg(DDC_DIN_CNT1_IVCTX, 0x01);
	/* data length hi */
	hdmitx21_wr_reg(DDC_DIN_CNT2_IVCTX, 0x00);
	/* DDC Write CMD */
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_SEQ_RW_IGNORE_ACK);
	/* Wait until I2C done */
	/* i2c process */
	hdmitx21_poll_reg(DDC_STATUS_IVCTX, 1 << 4, ~(1 << 4), 0xffffffff);
	/* i2c done */
	hdmitx21_poll_reg(DDC_STATUS_IVCTX, 0 << 4, ~(1 << 4), 0xffffffff);

	return st;
}

static u32 ddc_read_1byte(u8 slave, u8 offset_addr, u8 *rd_data)
{
	u32 st = 0;

	/* Programe I2C operation */
	/* clear fifo */
	hdmitx21_wr_reg(DDC_CMD_CLR_FIFO, DDC_CMD_CLR_FIFO);
	/* SCDC slave addr */
	hdmitx21_wr_reg(DDC_ADDR_IVCTX, 0xa8);
	/* SCDC slave offset */
	hdmitx21_wr_reg(DDC_OFFSET_IVCTX, offset_addr & 0xff);
	/* data length lo */
	hdmitx21_wr_reg(DDC_DIN_CNT1_IVCTX, 0x01);
	/* data length hi */
	hdmitx21_wr_reg(DDC_DIN_CNT2_IVCTX, 0x00);
	/* DDC Write CMD */
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_SEQ_RD_NO_ACK);
	/* Wait until I2C done */
	/* i2c process */
	hdmitx21_poll_reg(DDC_STATUS_IVCTX, 1 << 4, ~(1 << 4), 0xffffffff);
	/* i2c done */
	hdmitx21_poll_reg(DDC_STATUS_IVCTX, 0 << 4, ~(1 << 4), 0xffffffff);
	/* Read back 1 byte */
	*rd_data = hdmitx21_rd_reg(DDC_DATA_AON_IVCTX);

	return st;
}

static void ddc_tx_en(u8 seg_index, u8 slave_addr, u8 reg_addr)
{
	hdmitx21_set_bit(TPI_DDC_MASTER_EN_IVCTX, BIT_TPI_DDC_MASTER_EN_HW_EN, true);
	hdmitx21_wr_reg(DDC_ADDR_IVCTX, slave_addr & BIT_DDC_ADDR_REG);
	hdmitx21_wr_reg(DDC_SEGM_IVCTX, seg_index);
	hdmitx21_wr_reg(DDC_OFFSET_IVCTX, reg_addr);
}

/* After fifo clear, need some delay */
static void ddc_tx_read(u8 seg_index, u16 length, u8 read_cmd)
{
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_CLR_FIFO);
	hdmitx21_wr_reg(DDC_DIN_CNT2_IVCTX, (u8)(length >> 8));
	hdmitx21_wr_reg(DDC_DIN_CNT1_IVCTX, (u8)length);
	hdmitx21_wr_reg(DDC_CMD_IVCTX, read_cmd);
}

void scdc21_rd_sink(u8 adr, u8 *val)
{
	ddc_read_1byte(DDC_SCDC_ADDR, adr, val);
}

void scdc21_wr_sink(u8 adr, u8 val)
{
	ddc_write_1byte(DDC_SCDC_ADDR, adr, val);
}

void hdmitx21_fifo_read(u16 offset, u8 *buf, u16 cnt)
{
	int i = 0;

	if (!buf)
		return;
	for (i = 0; i < cnt; i++)
		buf[i] = hdmitx21_rd_reg(offset);
}

static void ddc_tx_fifo_read(u8 *p_buf, u16 fifo_size)
{
	hdmitx21_fifo_read(DDC_DATA_AON_IVCTX, p_buf, fifo_size);
}

static u8 ddc_tx_fifo_size_read(void)
{
	return hdmitx21_rd_reg(DDC_DOUT_CNT_IVCTX) & BIT_DDC_DOUT_CNT_DATA_OUT_CNT;
}

static bool ddc_tx_err_check(void)
{
	bool check_failed = false;
	u8 ddc_status;

	ddc_status = hdmitx21_rd_reg(DDC_STATUS_IVCTX);
	if (ddc_status & (BIT_DDC_STATUS_BUSLOW | BIT_DDC_STATUS_NACK)) {
		hdmitx21_wr_reg(DDC_STATUS_IVCTX, 0x00);
		check_failed = true;
	}

	return check_failed;
}

static void ddc_tx_disable(void)
{
	hdmitx21_set_bit(TPI_DDC_MASTER_EN_IVCTX, BIT_TPI_DDC_MASTER_EN_HW_EN, false);
}

static u8 ddc_tx_hdcp2x_check(void)
{
	u8 val;

	val = hdmitx21_rd_reg(HDCP2X_CTL_0_IVCTX) & BIT_HDCP2X_CTL_0_EN;
	if (val)
		hdmitx21_set_bit(SCDC_CTL_IVCTX, BIT_SCDC_CTL_REG_DDC_STALL_REQ, true);
	return val;
}

static void ddc_tx_scdc_clr(u8 val)
{
	if (val)
		hdmitx21_set_bit(SCDC_CTL_IVCTX, BIT_SCDC_CTL_REG_DDC_STALL_REQ, false);
}

static void ddc_tx_error_check(enum ddc_err_t ds_ddc_error)
{
	if (ds_ddc_error) {
		hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_ABORT_TRANSACTION);
		hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_CLK_RESET);
	}
}

static void ddc_tx_ddc_error_reset(void)
{
	hdmitx21_set_bit(TPI_DDC_MASTER_EN_IVCTX, BIT_TPI_DDC_MASTER_EN_HW_EN, true);
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_ABORT_TRANSACTION);
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_CLK_RESET);
	hdmitx21_set_bit(TPI_DDC_MASTER_EN_IVCTX, BIT_TPI_DDC_MASTER_EN_HW_EN, false);
}

static u8 ddc_tx_busy_check(void)
{
	return hdmitx21_rd_reg(DDC_STATUS_IVCTX) & BIT_DDC_STATUS_INPROG;
}

static bool ddc_wait_free(void)
{
	u8 val;
	/* unit: ms */
	u8 tmo1 = 5;
	u8 tmo2 = 2;

	while (tmo2--) {
		tmo1 = 5;
		while (tmo1--) {
			val = ddc_tx_busy_check();
			if (!val)
				return true;
			usleep_range(2000, 2500);
		}
		HDMITX_INFO("ddc bus busy\n");
		ddc_tx_ddc_error_reset();
		usleep_range(2000, 2500);
	}
	return false;
}

static enum ddc_err_t _hdmitx_ddcm_read_(u8 seg_index, u8 slave_addr, u8 reg_addr,
					 u8 *p_buf, u16 length, u8 read_cmd)
{
	enum ddc_err_t ds_ddc_error = DDC_ERR_NONE;
	u16 fifo_size;
	u16 timeout_ms;
	u8 val = ddc_tx_hdcp2x_check();

	mutex_lock(&ddc_mutex);
	do {
		if (length == 0 || !p_buf)
			break;

		if (!ddc_wait_free()) {
			/*
			 * need to clr DDC_STALL_REQ, otherwise
			 * DDC will always be occupied by SCDC
			 */
			ddc_tx_scdc_clr(val);
			mutex_unlock(&ddc_mutex);
			return DDC_ERR_BUSY;
		}

		ddc_tx_en(seg_index, slave_addr, reg_addr);
		ddc_tx_read(seg_index, length, read_cmd);

		timeout_ms = (u16)(length * 12 / 10);
		usleep_range(2000, 3000);
		do {
			fifo_size = ddc_tx_fifo_size_read();

			if (fifo_size) {
				if (fifo_size > length) {
					ds_ddc_error = DDC_ERR_HW;
					break;
				} else if (fifo_size > LEN_TPI_DDC_FIFO_SIZE) {
					ds_ddc_error = DDC_ERR_LIM_EXCEED;
					break;
				}
				/* read fifo_size bytes */
				ddc_tx_fifo_read(p_buf, fifo_size);

				length -= fifo_size;
				p_buf += fifo_size;
			} else {
				usleep_range(1000, 1500);
				timeout_ms--;
				if (ddc_tx_err_check()) {
					ds_ddc_error = DDC_ERR_NACK;
					break;
				}
			}
		} while (length && timeout_ms);

		if (ds_ddc_error)
			break;
	} while (false);

	ddc_tx_scdc_clr(val);
	ddc_tx_error_check(ds_ddc_error);

	/* disable the DDC master */
	ddc_tx_disable();
	mutex_unlock(&ddc_mutex);
	return ds_ddc_error;
}

bool hdmitx_ddcm_read(u8 seg_index, u8 slave_addr, u8 reg_addr, u8 *p_buf, u16 len, u8 read_cmd)
{
	enum ddc_err_t ddc_err;

	ddc_err = _hdmitx_ddcm_read_(seg_index, slave_addr, reg_addr, p_buf, len, read_cmd);
	return (ddc_err == DDC_ERR_NONE) ? false : true;
}

/*
 * Note: read 128 Bytes of EDID data every time
 */
int hdmitx21_read_edid(u8 *_rx_edid)
{
	u32 blk_idx = 0;
	u8 ext_block_num = 0;
	u8 *rx_edid = _rx_edid;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	/* skip edid reading in pxp */
	if (hdev->pxp_mode)
		return 0;

	/* Read complete EDID data sequentially */
	while (blk_idx < (1 + ext_block_num)) {
		hdmitx_ddcm_read(blk_idx >> 1, DDC_EDID_ADDR, (blk_idx * EDID_BLK_SIZE) & 0xff,
				 &rx_edid[blk_idx * EDID_BLK_SIZE], EDID_BLK_SIZE,
				 TPI_DDC_CMD_ENHANCED_DDC_READ);
		if (blk_idx == 0)
			ext_block_num = rx_edid[126];
		if (blk_idx == 1)
			if (rx_edid[128 + 4] == EXTENSION_EEODB_EXT_TAG &&
				rx_edid[128 + 5] == EXTENSION_EEODB_EXT_CODE)
				ext_block_num = rx_edid[128 + 6];
		if (ext_block_num > 7) {
			HDMITX_INFO("edid extension block number:");
			HDMITX_INFO(" %d, reset to MAX 7\n", ext_block_num);
			/* Max extended block */
			ext_block_num = 7;
		}
		blk_idx++;
	}
	return 1;
}

static void ddc_tx_sequential_write(u8 *data, u16 len)
{
	int i;

	if (!data || !len)
		return;
	hdmitx21_wr_reg(DDC_DIN_CNT2_IVCTX, (u8)(len >> 8));
	hdmitx21_wr_reg(DDC_DIN_CNT1_IVCTX, (u8)len);
	hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_CLR_FIFO | 0x30);
	for (i = 0; i < len; i++)
		hdmitx21_wr_reg(DDC_DATA_AON_IVCTX, data[i]);
	if (len == 1)
		hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_SEQ_RW_IGNORE_ACK | 0x30);
	else
		hdmitx21_wr_reg(DDC_CMD_IVCTX, DDC_CMD_SEQ_RW_REQUIRE_ACK | 0x30);
}

static enum ddc_err_t _hdmitx_ddcm_write_(u8 seg_index,
		u8 slave_addr, u8 reg_addr, u8 *data, u16 length)
{
	enum ddc_err_t ds_ddc_error = DDC_ERR_NONE;
	u8 val = ddc_tx_hdcp2x_check();

	mutex_lock(&ddc_mutex);
	if (!ddc_wait_free()) {
		ddc_tx_scdc_clr(val);
		mutex_unlock(&ddc_mutex);
		return DDC_ERR_BUSY;
	}

	ddc_tx_en(seg_index, slave_addr, reg_addr);
	ddc_tx_sequential_write(data, length);

	usleep_range(2000, 3000);
	if (ddc_tx_err_check())
		ds_ddc_error = DDC_ERR_NACK;

	ddc_tx_scdc_clr(val);
	ddc_tx_error_check(ds_ddc_error);

	/* disable the DDC master */
	ddc_tx_disable();
	mutex_unlock(&ddc_mutex);
	return ds_ddc_error;
}

bool hdmitx_ddcm_write(u8 seg_index, u8 slave_addr, u8 reg_addr, u8 *data, u16 len)
{
	enum ddc_err_t ddc_err;

	ddc_err = _hdmitx_ddcm_write_(seg_index, slave_addr, reg_addr, data, len);
	return (ddc_err == DDC_ERR_NONE) ? false : true;
}
