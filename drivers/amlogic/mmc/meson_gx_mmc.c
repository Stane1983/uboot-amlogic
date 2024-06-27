// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

//#define DEBUG
#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <pwrseq.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/log2.h>
#include <dm/pinctrl.h>
#include <asm/amlogic/arch/sd_emmc.h>
#include <amlogic/emmc_partitions.h>
#include <amlogic/aml_mmc.h>
#include "meson_gx_mmc.h"

bool meson_gx_mmc_is_compatible(struct udevice *dev,
				enum meson_gx_mmc_compatible family)
{
	enum meson_gx_mmc_compatible compat = dev_get_driver_data(dev);

	return compat == family;
}

static inline void *get_regbase(const struct mmc *mmc)
{
	struct meson_mmc_plat *pdata = mmc->priv;

	return pdata->regbase;
}

static inline u32 meson_read(struct mmc *mmc, int offset)
{
	return readl(get_regbase(mmc) + offset);
}

static inline void meson_write(struct mmc *mmc, u32 val, int offset)
{
	writel(val, get_regbase(mmc) + offset);
}

void mmc_print_reg(struct udevice *dev)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);

	pr_info("%s: clk = %x, dly1 = %x, dly2 = %x, adj = %x, cfg = %x\n",
		mmc->cfg->name,
		meson_read(mmc, MESON_SD_EMMC_CLOCK),
		meson_read(mmc, MESON_SD_EMMC_DELAY1),
		meson_read(mmc, MESON_SD_EMMC_DELAY2),
		meson_read(mmc, MESON_SD_EMMC_ADJUST),
		meson_read(mmc, MESON_SD_EMMC_CFG));
}

static void meson_mmc_config_clock(struct meson_host *host)
{
	struct mmc *mmc = host->mmc;
	u32 meson_mmc_clk = 0, cfg = 0;
	u32 co_phase = 0, tx_phase = 0, tx_delay = 0;
	unsigned int clk = 0, clk_src = 0, clk_div = 0;

	if (!mmc->clock)
		return;

	/* TOFIX This should use the proper clock taken from DT */

	/* 1GHz / CLK_MAX_DIV = 15,9 MHz */
	if (mmc->clock > 16000000) {
		clk = SD_EMMC_CLKSRC_DIV2;
		clk_src = 1;
		if (host->src_clk != 0) {
			clk = host->src_clk;
			clk_src = 0;
		}
		clk_disable(&host->xtal);
		clk_set_parent(&host->mux, &host->div2);
		clk_set_rate(&host->div, clk);
		cfg = meson_read(mmc, MESON_SD_EMMC_CFG);
		cfg |= CFG_AUTO_CLK;
		meson_write(mmc, cfg, MESON_SD_EMMC_CFG);
	} else {
		clk = SD_EMMC_CLKSRC_24M;
		clk_src = 0;
		clk_set_parent(&host->mux, &host->xtal);
		clk_enable(&host->xtal);
		clk_set_rate(&host->div, clk);
	}
	clk_div = DIV_ROUND_UP(clk, mmc->clock);
	mmc->clock = clk / clk_div;
	if (mmc->ddr_mode) {
		clk_div /= 2;
	}

	switch (mmc->selected_mode) {
	case MMC_LEGACY:
			co_phase = dev_read_u32_default(mmc->dev, "init_co_phase", 2);
			tx_phase = dev_read_u32_default(mmc->dev, "init_tx_phase", 0);
			break;
	case MMC_HS:
	case MMC_HS_52:
			dev_read_u32(mmc->dev, "hs_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs_tx_phase", &tx_phase);
			/* enable adjust, set adj = 1 */
			meson_write(mmc, 0x2000, MESON_SD_EMMC_ADJUST);
			break;
	case SD_HS:
			dev_read_u32(mmc->dev, "sd_hs_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "sd_hs_tx_phase", &tx_phase);
			/* enable adjust, set adj = 1 */
			meson_write(mmc, 0x2000, MESON_SD_EMMC_ADJUST);
			break;
	case MMC_HS_200:
			dev_read_u32(mmc->dev, "hs2_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs2_tx_phase", &tx_phase);
			break;
	case UHS_SDR104:
			dev_read_u32(mmc->dev, "sdr104_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "sdr104_tx_phase", &tx_phase);
			break;
	case MMC_DDR_52:
			dev_read_u32(mmc->dev, "ddr_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "ddr_tx_phase", &tx_phase);
			break;
	case MMC_HS_400:
			dev_read_u32(mmc->dev, "hs4_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs4_tx_delay", &tx_delay);
			break;
	case MMC_HS_400_ES:
			dev_read_u32(mmc->dev, "hs4es_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs4es_tx_delay", &tx_delay);
			break;
	default:
			co_phase = dev_read_u32_default(mmc->dev, "init_co_phase", 2);
			tx_phase = dev_read_u32_default(mmc->dev, "init_tx_phase", 0);
			break;
	}

	meson_mmc_clk = ((0 << CFG_IRQ_SDIO_SLEEP_DS) |
					(0 << CFG_IRQ_SDIO_SLEEP) |
					(1 << CFG_ALWAYS_ON) |
					(0 << CFG_RX_DELAY) |
					(tx_delay << CFG_TX_DELAY) |
					(0 << CFG_SRAM_PD) |
					(0 << CFG_RX_PHASE) |
					(tx_phase << CFG_TX_PHASE) |
					(co_phase << CFG_CO_PHASE) |
					(clk_src << CFG_SRC) |
					(clk_div << CFG_DIV));

	meson_write(mmc, meson_mmc_clk, MESON_SD_EMMC_CLOCK);
	debug("clk_config:0x%x\n", meson_read(mmc, MESON_SD_EMMC_CLOCK));
	debug("sd_src:0x%x, emmc_src:0x%x\n", readl(0xfe00016c), readl(0xfe000168));
	/*
	 * SM1 SoCs doesn't work fine over 50MHz with CLK_CO_PHASE_180
	 * If CLK_CO_PHASE_270 is used, it's more stable than other.
	 * Other SoCs use CLK_CO_PHASE_180 by default.
	 * It needs to find what is a proper value about each SoCs.
	 */
	//if (meson_gx_mmc_is_compatible(mmc->dev, MMC_COMPATIBLE_SM1))
	//	meson_mmc_clk |= CLK_CO_PHASE_270;
	//else
	//	meson_mmc_clk |= CLK_CO_PHASE_180;

	/* 180 phase tx clock */
	//meson_mmc_clk |= CLK_TX_PHASE_000;

	/* clock settings */
	//meson_mmc_clk |= clk_src;
	//meson_mmc_clk |= clk_div;

	//meson_write(mmc, meson_mmc_clk, MESON_SD_EMMC_CLOCK);
}

static void meson_mmc_check_resampling(struct udevice *dev)
{
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	unsigned int val;

	if (host->timing == mmc->selected_mode) {
		debug("[%s]bail-out, timing\n", __func__);
		return;
	}

	meson_write(mmc, 0, MESON_SD_EMMC_INTF3);
	switch (mmc->selected_mode) {
	case MMC_HS_400_ES:
		val = meson_read(mmc, MESON_SD_EMMC_ADJUST);
		val |= (1 << CFG_DS_EN);
		meson_write(mmc, val, MESON_SD_EMMC_ADJUST);
		val = meson_read(mmc, MESON_SD_EMMC_INTF3);
		val |= SD_INTF3;
		val |= RESP_DS;
		meson_write(mmc, val, MESON_SD_EMMC_INTF3);
		break;
	case MMC_LEGACY:
	case MMC_HS:
		val = meson_read(mmc, MESON_SD_EMMC_ADJUST);
		val &= ~(1 << CFG_DS_EN);
		val |= CFG_ADJ_EN;
		val &= ~CFG_ADJ_DLY;
		val |= 0 << __ffs(CFG_ADJ_DLY);
		meson_write(mmc, val, MESON_SD_EMMC_ADJUST);
		val = meson_read(mmc, MESON_SD_EMMC_INTF3);
		val &= ~SD_INTF3;
		val &= ~RESP_DS;
		meson_write(mmc, val, MESON_SD_EMMC_INTF3);
		break;
	default:
		break;
	}

	host->timing = mmc->selected_mode;
}

static int meson_dm_mmc_set_ios(struct udevice *dev)
{
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	u32 meson_mmc_cfg;

	if (!mmc->clock) {
		meson_write(mmc, 0, MESON_SD_EMMC_DELAY1);
		meson_write(mmc, 0, MESON_SD_EMMC_DELAY2);
		meson_write(mmc, 0, MESON_SD_EMMC_ADJUST);
	}
	meson_mmc_config_clock(host);

	meson_mmc_cfg = meson_read(mmc, MESON_SD_EMMC_CFG);

	meson_mmc_cfg &= ~CFG_BUS_WIDTH_MASK;
	if (mmc->bus_width == 1)
		meson_mmc_cfg |= CFG_BUS_WIDTH_1;
	else if (mmc->bus_width == 4)
		meson_mmc_cfg |= CFG_BUS_WIDTH_4;
	else if (mmc->bus_width == 8)
		meson_mmc_cfg |= CFG_BUS_WIDTH_8;
	else
		return -EINVAL;

	/* 512 bytes block length */
	meson_mmc_cfg &= ~CFG_BL_LEN_MASK;
	meson_mmc_cfg |= CFG_BL_LEN_512;

	/* Response timeout 256 clk */
	meson_mmc_cfg &= ~CFG_RESP_TIMEOUT_MASK;
	meson_mmc_cfg |= CFG_RESP_TIMEOUT_256;

	/* Command-command gap 16 clk */
	meson_mmc_cfg &= ~CFG_RC_CC_MASK;
	meson_mmc_cfg |= CFG_RC_CC_16;

	meson_mmc_cfg &= ~CFG_DDR;
	if (mmc->ddr_mode)
		meson_mmc_cfg |= CFG_DDR;

	meson_write(mmc, meson_mmc_cfg, MESON_SD_EMMC_CFG);

	meson_mmc_check_resampling(dev);

	return 0;
}

/*
 *static void meson_mmc_setup_cmd(struct mmc *mmc, struct mmc_data *data,
 *				struct mmc_cmd *cmd)
 *{
 *	u32 meson_mmc_cmd = 0, cfg;
 *
 *	meson_mmc_cmd |= cmd->cmdidx << CMD_CFG_CMD_INDEX_SHIFT;
 *
 *	if (cmd->resp_type & MMC_RSP_PRESENT) {
 *		if (cmd->resp_type & MMC_RSP_136)
 *			meson_mmc_cmd |= CMD_CFG_RESP_128;
 *
 *		if (cmd->resp_type & MMC_RSP_BUSY)
 *			meson_mmc_cmd |= CMD_CFG_R1B;
 *
 *		if (!(cmd->resp_type & MMC_RSP_CRC))
 *			meson_mmc_cmd |= CMD_CFG_RESP_NOCRC;
 *	} else {
 *		meson_mmc_cmd |= CMD_CFG_NO_RESP;
 *	}
 *
 *	if (data) {
 *		cfg = meson_read(mmc, MESON_SD_EMMC_CFG);
 *		cfg &= ~CFG_BL_LEN_MASK;
 *		cfg |= ilog2(data->blocksize) << CFG_BL_LEN_SHIFT;
 *		meson_write(mmc, cfg, MESON_SD_EMMC_CFG);
 *
 *		if (data->flags == MMC_DATA_WRITE)
 *			meson_mmc_cmd |= CMD_CFG_DATA_WR;
 *
 *		meson_mmc_cmd |= CMD_CFG_DATA_IO | CMD_CFG_BLOCK_MODE |
 *				 data->blocks;
 *	}
 *
 *	meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S | CMD_CFG_OWNER |
 *			 CMD_CFG_END_OF_CHAIN;
 *
 *	meson_write(mmc, meson_mmc_cmd, MESON_SD_EMMC_CMD_CFG);
 *}
 *
 *static void meson_mmc_setup_addr(struct mmc *mmc, struct mmc_data *data)
 *{
 *	struct meson_mmc_plat *pdata = mmc->priv;
 *	unsigned int data_size;
 *	u32 data_addr = 0;
 *
 *	if (data) {
 *		data_size = data->blocks * data->blocksize;
 *
 *		if (data->flags == MMC_DATA_READ) {
 *			data_addr = (ulong) data->dest;
 *			invalidate_dcache_range(data_addr,
 *						data_addr + data_size);
 *		} else {
 *			pdata->w_buf = calloc(data_size, sizeof(char));
 *			data_addr = (ulong) pdata->w_buf;
 *			memcpy(pdata->w_buf, data->src, data_size);
 *			flush_dcache_range(data_addr, data_addr + data_size);
 *		}
 *	}
 *
 *	meson_write(mmc, data_addr, MESON_SD_EMMC_CMD_DAT);
 *}
 */

void meson_hw_reset(struct udevice *dev)
{
	struct meson_host *host = dev_get_priv(dev);
	u32 cfg = 0;

	/* send the initialization stream: 74 clock cycles */
	cfg = meson_read(host->mmc, MESON_SD_EMMC_CFG);
	cfg &= ~CFG_AUTO_CLK;
	meson_write(host->mmc, cfg, MESON_SD_EMMC_CFG);

	if (aml_card_type_mmc(host)) {
		dm_gpio_set_value(&host->gpio_reset, 0);
		mdelay(2);
		dm_gpio_set_value(&host->gpio_reset, 1);
		mdelay(2);
	}
}

int meson_get_cd(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	int ret = 0, sduart_f = 0;
	u32 status = 0;

	if (aml_card_type_non_sdio(host)) {
		sduart_f = pinctrl_select_state(mmc->dev, "sd_all_pins");
		ret = dm_gpio_get_value(&host->gpio_cd);
		if (ret < 0) {
			pr_err("card detect get failed!\n");
		} else if (!ret) {
			host->is_in = 1;
			status = meson_read(mmc, MESON_SD_EMMC_STATUS);
			if (!(status & (1 << 19)) && !sduart_f) {
				pinctrl_select_state(mmc->dev, "sd_uart");
				host->is_sduart = 1;
				host->is_in = 0;
				printf("[%s]uart in\n", __func__);
			} else {
				host->is_sduart = 0;
				printf("[%s]card in\n", __func__);
			}
		} else {
			host->is_in = 0;
		}
	}

	return host->is_in;
}

static int meson_wait_dat0(struct udevice *dev, int state, int timeout_us)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	unsigned long timeout = timer_get_us() + timeout_us;
	u32 status = 0;

	do {
		status = meson_read(mmc, MESON_SD_EMMC_STATUS);
		if (!!(status & STATUS_DATA_0) == !!state)
			return 0;
	} while (!timeout_us || !time_after(timer_get_us(), timeout));

	return -ETIMEDOUT;
}

static int mmc_controller_debug(struct udevice *dev,
				struct mmc_cmd *cmd, u32 status)
{
	int ret = 0;
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct meson_host *host = dev_get_priv(dev);

	if (status & STATUS_RXD_ERR_MASK) {
		ret |= SD_EMMC_RXD_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: read crc err, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_TXD_ERR) {
		ret |= SD_EMMC_TXD_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: write tx err, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_DESC_ERR) {
		ret |= SD_EMMC_DESC_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: desc error, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_RESP_ERR) {
		ret |= SD_EMMC_RESP_CRC_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: resp crc error, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_RESP_TIMEOUT) {
		ret |= SD_EMMC_RESP_TIMEOUT_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: resp timeout, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_DESC_TIMEOUT) {
		ret |= SD_EMMC_DESC_TIMEOUT_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: desc timeout, cmd%d, status=0x%x\n",
			       mmc->cfg->name, cmd->cmdidx, status);
	}

	debug("cmd->response[0]=0x%x;\n", cmd->response[0]);
	debug("cmd->response[1]=0x%x;\n", cmd->response[1]);
	debug("cmd->response[2]=0x%x;\n", cmd->response[2]);
	debug("cmd->response[3]=0x%x;\n", cmd->response[3]);

	return ret;
}

static void meson_mmc_read_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP3);
		cmd->response[1] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP2);
		cmd->response[2] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP1);
		cmd->response[3] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP);
	} else {
		cmd->response[0] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP);
	}
}

static void meson_mmc_clear_response(unsigned int *res_buf)
{
	int i;

	if (!res_buf)
		return;

	for (i = 0; i < MAX_RESPONSE_BYTES; i++)
		res_buf[i] = 0;
}

static int mmc_pre_dma(struct udevice *dev, struct mmc_data *data,
		       struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 *meson_mmc_cmd = NULL;
	unsigned int blks = 0, desc_cnt = 0, bl_len = 0;
	u32 data_addr = 0;

	meson_mmc_cmd = &desc_cur->cmd_info;
	if (data->flags == MMC_DATA_WRITE)
		data_addr = (ulong)pdata->w_buf;
	else
		data_addr = (ulong)data->dest;

	if (data->blocks > 1 ||
	    data->blocksize >= MMC_MAX_BLOCK_LEN) {
		blks = data->blocks;
		while (blks) {
			meson_mmc_cmd = &desc_cur->cmd_info;
			*meson_mmc_cmd |= CMD_CFG_BLOCK_MODE;
			bl_len = (blks > mmc->cfg->b_max) ?
				mmc->cfg->b_max : blks;
			*meson_mmc_cmd &= ~CMD_CFG_LENGTH_MASK;
			*meson_mmc_cmd |= bl_len;
			blks -= bl_len;

			if (desc_cnt != 0) {
				*meson_mmc_cmd |= CMD_CFG_NO_RESP;
				*meson_mmc_cmd |= CMD_CFG_NO_CMD;
			}
			*meson_mmc_cmd &= ~CMD_CFG_RESP_NUM;
			*meson_mmc_cmd |= CMD_CFG_DATA_IO;
			*meson_mmc_cmd |= CMD_CFG_OWNER;
			*meson_mmc_cmd &= ~CMD_CFG_DATA_WR;
			if (data->flags == MMC_DATA_WRITE)
				*meson_mmc_cmd |= CMD_CFG_DATA_WR;
			*meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S;
			desc_cur->data_addr = data_addr
				+ (desc_cnt * mmc->cfg->b_max * mmc->read_bl_len);
			desc_cur->data_addr &= ~(1 << 0);
			if (blks) {
				desc_cur++;
				desc_cnt++;
				memset(desc_cur, 0, sizeof(struct sd_emmc_desc_info));
			}
		}
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_BLOCK_MODE;
		*meson_mmc_cmd |= data->blocksize;
		desc_cur->data_addr = data_addr;
		desc_cur->data_addr &= ~(1 << 0);
	}

	return desc_cnt;
}

static int mmc_setup_data(struct udevice *dev, struct mmc_data *data,
			  struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	u32 *meson_mmc_cmd = NULL;
	unsigned int data_size, desc_cnt = 0;
	u32 data_addr = 0;

	meson_mmc_cmd = &desc_cur->cmd_info;
	*meson_mmc_cmd |= CMD_CFG_DATA_IO;
	*meson_mmc_cmd &= ~CMD_CFG_DATA_NUM;
	*meson_mmc_cmd &= ~CMD_CFG_LENGTH_MASK;

	data_size = data->blocks * data->blocksize;
	if (data->flags == MMC_DATA_WRITE) {
		*meson_mmc_cmd |= CMD_CFG_DATA_WR;
		pdata->w_buf = (u32 *)malloc(data_size);
		memset(pdata->w_buf, 0, data_size);
		memcpy(pdata->w_buf, (u32 *)data->src, data_size);
		flush_dcache_range((ulong)pdata->w_buf,
				   (ulong)(pdata->w_buf + data_size));
		data_addr = (ulong)pdata->w_buf;
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_DATA_WR;
		data_addr = (ulong)data->dest;
		invalidate_dcache_range(data_addr,
					data_addr + data_size);
	}

	desc_cnt = mmc_pre_dma(dev, data, desc_cur);

	return desc_cnt;
}

static void mmc_setup_desc(struct udevice *dev, struct mmc_cmd *cmd,
			   struct mmc_data *data,
			   struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 *meson_mmc_cmd = NULL;
	u32 cfg = 0, bl_len = 0, resp_addr;
	unsigned int desc_cnt = 0;

	meson_mmc_cmd = &desc_cur->cmd_info;
	*meson_mmc_cmd |= cmd->cmdidx << CMD_CFG_CMD_INDEX_SHIFT;
	desc_cur->cmd_arg |= cmd->cmdarg;

	*meson_mmc_cmd |= CMD_CFG_OWNER;
	*meson_mmc_cmd &= ~CMD_CFG_ERR;
	*meson_mmc_cmd &= ~CMD_CFG_END_OF_CHAIN;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		resp_addr = (unsigned long)cmd->response;
		*meson_mmc_cmd &= ~CMD_CFG_NO_RESP;
		if (cmd->resp_type & MMC_RSP_136)
			*meson_mmc_cmd |= CMD_CFG_RESP_128;

		if (cmd->resp_type & MMC_RSP_BUSY)
			*meson_mmc_cmd |= CMD_CFG_R1B;

		if (!(cmd->resp_type & MMC_RSP_CRC))
			*meson_mmc_cmd |= CMD_CFG_RESP_NOCRC;

		*meson_mmc_cmd &= ~CMD_CFG_RESP_NUM;
		desc_cur->resp_addr = resp_addr;
	} else {
		*meson_mmc_cmd |= CMD_CFG_NO_RESP;
	}

	if (data) {
		cfg = meson_read(mmc, MESON_SD_EMMC_CFG);
		bl_len = (cfg & CFG_BL_LEN_MASK) >> CFG_BL_LEN_SHIFT;
		if (bl_len != ilog2(data->blocksize)) {
			cfg &= ~CFG_BL_LEN_MASK;
			cfg |= ilog2(data->blocksize) << CFG_BL_LEN_SHIFT;
			meson_write(mmc, cfg, MESON_SD_EMMC_CFG);
		}

		desc_cnt = mmc_setup_data(dev, data, desc_cur);
		if (desc_cnt)
			desc_cur += desc_cnt;
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_DATA_IO;
	}

	meson_mmc_cmd = &desc_cur->cmd_info;
	/* It takes longer to erase large amounts of data */
	if (cmd->cmdidx != MMC_CMD_ERASE)
		*meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S;
	*meson_mmc_cmd |= CMD_CFG_END_OF_CHAIN;
}

static int meson_mmc_desc_transfer(struct udevice *dev, struct mmc_cmd *cmd,
				   struct mmc_data *data)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	struct sd_emmc_desc_info *desc_cur = NULL;
	u32 start = 0;

	if (!host->desc_buf)
		return -EINVAL;

	start &= ~CFG_DESC_BUSY;
	meson_write(mmc, start, MESON_SD_EMMC_START);

	desc_cur = (struct sd_emmc_desc_info *)host->desc_buf;
	memset(desc_cur, 0, sizeof(struct sd_emmc_desc_info));
	meson_mmc_clear_response(cmd->response);

	mmc_setup_desc(dev, cmd, data, desc_cur);

	meson_write(mmc, STATUS_MASK, MESON_SD_EMMC_STATUS);
	invalidate_dcache_range((unsigned long)host->desc_buf,
				(unsigned long)(host->desc_buf
				+ MMC_MAX_DESC_NUM
				* (sizeof(struct sd_emmc_desc_info))));

	start = 0;
	start &= ~CFG_DESC_INIT;
	start |= CFG_DESC_BUSY;
	start |= ((unsigned long)host->desc_buf >> 2) << CFG_DESC_ADDR;
	meson_write(mmc, start, MESON_SD_EMMC_START);

	return 0;
}

static int meson_dm_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd,
				 struct mmc_data *data)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct meson_mmc_plat *pdata = mmc->priv;
	u32 status;
	ulong start;
	int ret = 0;

	/* max block size supported by chip is 512 byte */
	if (data && data->blocksize > 512)
		return -EINVAL;

	ret = meson_mmc_desc_transfer(dev, cmd, data);

	/* use 30s timeout */
	start = get_timer(0);
	do {
		status = meson_read(mmc, MESON_SD_EMMC_STATUS);
	} while (!(status & STATUS_END_OF_CHAIN) && get_timer(start) < 30000);

	ret = mmc_controller_debug(dev, cmd, status);

	meson_mmc_read_response(mmc, cmd);

	if (data && data->flags == MMC_DATA_WRITE)
		free(pdata->w_buf);

	if (ret) {
		if (status & STATUS_RESP_TIMEOUT)
			return -ETIMEDOUT;
		else
			return ret;
	}

	if (data && data->dest && data->flags == MMC_DATA_READ)
		invalidate_dcache_range((ulong)data->dest,
			(ulong)data->dest + data->blocksize * data->blocks);

	return ret;
}

static int meson_send_cali_blks(struct udevice *dev, u32 opcode, char *buffer,
				uint cnt, u8 *pattern)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;
	int err = 0, ret = 0;
	struct mmc_cmd cmd = {0};
	struct mmc_cmd stop = {0};
	struct mmc_data data = {{0}, 0};

	cmd.cmdidx = opcode;
	if (!strcmp(pattern, MMC_PATTERN_NAME))
		cmd.cmdarg = CALI_PATTERN_ADDR;
	else if (!strcmp(pattern, MMC_MAGIC_NAME))
		cmd.cmdarg = MAGIC_ADDR;
	else if (!strcmp(pattern, MMC_RANDOM_NAME))
		cmd.cmdarg = RANDOM_ADDR;
	else if (!strcmp(pattern, MMC_DTB_NAME))
		cmd.cmdarg = DTB_ADDR;
	if (mmc->high_capacity)
		cmd.cmdarg /= mmc->read_bl_len;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = buffer;
	data.blocks = cnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;
	memset(buffer, 0, data.blocks * data.blocksize);

	err = meson_dm_mmc_send_cmd(dev, &cmd, &data);
	if (err) {
		pr_debug("%s: send calibration read blocks error %d cnt = %d\n",
			 mmc->cfg->name, err, cnt);
	}

	if (cnt > 1 || err) {
		stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		stop.cmdarg = 0;
		stop.resp_type = MMC_RSP_R1b;

		ret = meson_dm_mmc_send_cmd(dev, &stop, NULL);
		if (ret)
			pr_debug("%s: send calibration stop blocks error %d\n",
				 mmc->cfg->name, ret);
	}
	return (ret || err) ? -1 : 0;
}

u32 meson_tuning_transfer(struct udevice *dev, u32 opcode)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	int cmd_err = 0, n, nmatch, tuning_err = 0;

	for (n = 0, nmatch = 0; n < TUNING_NUM_PER_POINT; n++) {
		if (opcode == MMC_CMD_SEND_TUNING_BLOCK_HS200 ||
		    opcode == MMC_CMD_SEND_TUNING_BLOCK) {
			tuning_err = mmc_send_tuning(mmc, opcode, &cmd_err);
		} else {
			tuning_err = meson_send_cali_blks(dev, opcode, host->blk_test,
							  REFIX_BLK_CNT, MMC_PATTERN_NAME);
		}
		if (!tuning_err) {
			nmatch++;
		} else {
			pr_debug("Tuning transfer error: nmatch=%d tuning_err:%d\n",
				 nmatch, tuning_err);
			/* After the cmd21 command fails,
			 * it takes a certain time for the emmc status to
			 * switch from data back to transfer. Currently,
			 * only this model has this problem. THGBMJG6C1LBAIL
			 * by adding ndelay(20000) to resolve
			 */
			ndelay(20000);
			break;
		}
	}
	return nmatch;
}

int meson_execute_tuning(struct udevice *dev, uint opcode)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 clk_src, clock;
	u32 vclk = 0, clk_div = 0, adj = 0, dly = 0, d1_dly, old_dly;
	int ret = 0, adj_delay = 0;
	int tuning_num = 0;
#ifdef MMC_HS200_MODE
	int pre_status = 0;
	int start = 0;
#endif
	int nmatch;
	int wrap_win_start = -1, wrap_win_size = 0;
	int best_win_start = -1, best_win_size = -1;
	int curr_win_start = -1, curr_win_size = 0;
	u8 rx_tuning_result[25] = { 0 };

	if (!host->blk_test)
		return -EINVAL;

	printf("%s: tuning start:\n", mmc->cfg->name);
	meson_write(mmc, 0, MESON_SD_EMMC_ADJUST);
	old_dly = meson_read(mmc, MESON_SD_EMMC_DELAY1);
	d1_dly = (old_dly & DLY_D1_MASK) >> DLY_D1;
	pr_debug("Data 1 aligned delay is %d\n", d1_dly);

tuning:
	wrap_win_start = -1;
	wrap_win_size = 0;
	best_win_start = -1;
	best_win_size = 0;
	curr_win_start = -1;
	curr_win_size = 0;

	vclk = meson_read(mmc, MESON_SD_EMMC_CLOCK);
	clk_div = vclk & CLK_MAX_DIV;
	if (!(vclk & CLK_MAX_SRC))
		clk_src = 24000000;
	else
		clk_src = 1000000000;
	clock = (clk_src / clk_div);
	pr_debug("%s: clk %d tuning start:\n", mmc->cfg->name, clock);

	host->is_tuning = 1;
	for (adj_delay = 0; adj_delay < clk_div; adj_delay++) {
		// Perform tuning ntries times per clk_div increment
		adj = 0;
		adj |= (adj_delay << CFG_ADJ_DLY);
		adj |= (1 << CFG_ADJ_EN);
		meson_write(mmc, adj, MESON_SD_EMMC_ADJUST);
		nmatch = meson_tuning_transfer(dev, opcode);
		if (adj_delay
				< ARRAY_SIZE(rx_tuning_result))
			rx_tuning_result[adj_delay] = nmatch;

		if (nmatch == TUNING_NUM_PER_POINT) {
			if (adj_delay == 0)
				wrap_win_start = adj_delay;

			if (wrap_win_start >= 0)
				wrap_win_size++;

			if (curr_win_start < 0)
				curr_win_start = adj_delay;

			curr_win_size++;
			pr_debug("%s: rx_tuning_result[%d] = %d\n",
				 mmc->cfg->name, adj_delay, nmatch);
		} else {
			if (curr_win_start >= 0) {
				if (best_win_start < 0) {
					best_win_start = curr_win_start;
					best_win_size = curr_win_size;
				} else {
					if (best_win_size < curr_win_size) {
						best_win_start = curr_win_start;
						best_win_size = curr_win_size;
					}
				}

				wrap_win_start = -1;
				curr_win_start = -1;
				curr_win_size = 0;
			}
		}
	}

	if (curr_win_start >= 0) {
		if (best_win_start < 0) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		} else if (wrap_win_size > 0) {
			/* Wrap around case */
			if (curr_win_size + wrap_win_size > best_win_size) {
				best_win_start = curr_win_start;
				best_win_size = curr_win_size + wrap_win_size;
			}
		} else if (best_win_size < curr_win_size) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		}

		curr_win_start = -1;
		curr_win_size = 0;
	}

	if (best_win_start < 0) {
		if ((tuning_num++ > MAX_TUNING_RETRY) ||
		    clk_div >= 10) {
			pr_err("%s: final result of tuning failed\n",
			       mmc->cfg->name);
			host->is_tuning = 0;
			return -1;
		}
		clk_div++;
		vclk &= ~CLK_MAX_DIV;
		vclk |= clk_div;
		meson_write(mmc, vclk, MESON_SD_EMMC_CLOCK);
		pr_err("%s: tuning failed, reduce freq and retuning\n",
		       mmc->cfg->name);
		goto tuning;
	} else if (best_win_size == clk_div) {
		dly = meson_read(mmc, MESON_SD_EMMC_DELAY1);
		d1_dly = (dly & DLY_D1_MASK) >> DLY_D1;
		pr_warn("%s: d1_dly %d, window start %d, size %d\n",
			mmc->cfg->name, d1_dly, best_win_start, best_win_size);
		if (++d1_dly > 0x3F) {
			pr_err("%s: tuning failed\n", mmc->cfg->name);
			host->is_tuning = 0;
			return -1;
		}
		dly &= ~DLY_D1_MASK;
		dly |= d1_dly << DLY_D1;
		meson_write(mmc, dly, MESON_SD_EMMC_DELAY1);
		goto tuning;
	} else {
		printf("%s: best_win_start =%d, best_win_size =%d\n",
		       mmc->cfg->name, best_win_start, best_win_size);
	}

	adj_delay = best_win_start + (best_win_size - 1) / 2
		+ (best_win_size - 1) % 2;
	adj_delay = adj_delay % clk_div;

	adj = 0;
	adj |= (adj_delay << CFG_ADJ_DLY);
	adj |= (1 << CFG_ADJ_EN);
	meson_write(mmc, adj, MESON_SD_EMMC_ADJUST);
	meson_write(mmc, old_dly, MESON_SD_EMMC_DELAY1);
	mmc_print_reg(dev);
#ifdef MMC_HS200_MODE
	for (n = 0; n < clk_div; n++) {
		if (n == clk_div - 1) {
			if (rx_tuning_result[n] == TUNING_NUM_PER_POINT &&
			    pre_status == 1)
				pr_debug("meson-mmc:emmc:[%d--%d] is ok\n", start, n);
			else if ((rx_tuning_result[n] != TUNING_NUM_PER_POINT) &&
				 (pre_status == -1))
				pr_debug("meson-mmc:emmc: [%d--%d] is nok\n", start, n);
			else if ((rx_tuning_result[n] == TUNING_NUM_PER_POINT) &&
				 (pre_status != 1))
				pr_debug("meson-mmc:emmc: [%d] is ok\n", n);
			else if ((rx_tuning_result[n] != TUNING_NUM_PER_POINT) &&
				 (pre_status == 1))
				pr_debug("meson-mmc:emmc: [%d] is nok\n", n);
		}
		if (rx_tuning_result[n] == TUNING_NUM_PER_POINT) {
			if (pre_status == -1) {
				if (start == n - 1)
					pr_debug("meson-mmc:emmc:[%d] is nok\n", start);
				else
					pr_debug("meson-mmc:emmc:[%d -- %d] is nok\n",
						 start, n - 1);
			} else if (pre_status == 1) {
				continue;
			}
			start = n;
			pre_status = 1;
		} else if (rx_tuning_result[n] != TUNING_NUM_PER_POINT) {
			if (pre_status == 1) {
				if (start == n - 1)
					pr_debug("meson-mmc:emmc:[%d] is ok\n", start);
				else
					pr_debug("meson-mmc:emmc:[%d--%d] is ok\n",
						 start, n - 1);
			} else if (pre_status == -1) {
				continue;
			}
			start = n;
			pre_status = -1;
		}
	}

	mmc_print_reg(dev);
#endif
	host->is_tuning = 0;

	return ret;
}

#ifdef CONFIG_MMC_HS400_ES_SUPPORT
static int emmc_test_bus(struct udevice *dev)
{
	struct meson_host *host = dev_get_priv(dev);
	int err = 0;
	u32 opcode = MMC_CMD_READ_MULTIPLE_BLOCK;

	err = meson_send_cali_blks(dev, opcode, host->blk_test, 40, MMC_PATTERN_NAME);
	if (err)
		return err;

	err = meson_send_cali_blks(dev, opcode, host->blk_test, 40, MMC_RANDOM_NAME);
	if (err)
		return err;

	err = meson_send_cali_blks(dev, opcode, host->blk_test, 40, MMC_MAGIC_NAME);
	if (err)
		return err;

	return err;
}

static int emmc_ds_manual_sht(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 val, intf3 = meson_read(mmc, MESON_SD_EMMC_INTF3);
	int i, err = 0;
	int match[64], size = 0;
	int best_start = -1, best_size = -1;
	int cur_start = -1, cur_size = 0;

	memset(match, -1, sizeof(match));
	for (i = 0; i < 64; i++) {
		host->is_tuning = 1;
		err = emmc_test_bus(dev);
		host->is_tuning = 0;
		pr_debug("intf3: 0x%x, err[%d]: %d\n",
			 meson_read(mmc, MESON_SD_EMMC_INTF3), i, err);
		if (!err) {
			match[i] = 0;
			++size;
		} else {
			match[i] = -1;
			if (size > DELAY_CELL_COUNTS)
				break;
		}
		val = intf3 & DS_SHT_M_MASK;
		val += 1 << __ffs(DS_SHT_M_MASK);
		intf3 &= ~DS_SHT_M_MASK;
		intf3 |= val;
		meson_write(mmc, intf3, MESON_SD_EMMC_INTF3);
	}
	for (i = 0; i < 64; i++) {
		if (match[i] == 0) {
			if (cur_start < 0)
				cur_start = i;
			cur_size++;
		} else {
			if (cur_start >= 0) {
				if (best_start < 0) {
					best_start = cur_start;
					best_size = cur_size;
				} else {
					if (best_size < cur_size) {
						best_start = cur_start;
						best_size = cur_size;
					}
				}
				cur_start = -1;
				cur_size = 0;
			}
		}
	}
	if (cur_start >= 0) {
		if (best_start < 0) {
			best_start = cur_start;
			best_size = cur_size;
		} else if (best_size < cur_size) {
			best_start = cur_start;
			best_size = cur_size;
		}
		cur_start = -1;
		cur_size = -1;
	}
	intf3 &= ~DS_SHT_M_MASK;
	intf3 &= ~DS_SHT_EXP_MASK;
	intf3 |= (best_start + best_size / 2) << __ffs(DS_SHT_M_MASK);
	meson_write(mmc, intf3, MESON_SD_EMMC_INTF3);
	pr_info("ds_sht:%lu, window:%d, intf3:0x%x, clock:0x%x, adjust:0x%x\n",
		(intf3 & DS_SHT_M_MASK) >> __ffs(DS_SHT_M_MASK), best_size,
		meson_read(mmc, MESON_SD_EMMC_INTF3),
		meson_read(mmc, MESON_SD_EMMC_CLOCK),
		meson_read(mmc, MESON_SD_EMMC_ADJUST));
	return 0;
}

static int aml_mmc_clktest(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 intf3 = meson_read(mmc, MESON_SD_EMMC_INTF3);
	u32 clktest = 0, delay_cell = 0, clktest_log = 0, count = 0;
	u32 vcfg = meson_read(mmc, MESON_SD_EMMC_CFG);
	int i = 0, ret = 0;
	u32 cycle = 0;

	meson_write(mmc, 0, MESON_SD_EMMC_ADJUST);
	cycle = (1000000000 / mmc->clock) * 1000;
	vcfg &= ~CFG_AUTO_CLK;
	meson_write(mmc, vcfg, MESON_SD_EMMC_CFG);
	meson_write(mmc, 0, MESON_SD_EMMC_DELAY1);
	meson_write(mmc, 0, MESON_SD_EMMC_DELAY2);
	intf3 &= ~CLKTEST_EXP_MASK;
	intf3 |= 8 << __ffs(CLKTEST_EXP_MASK);
	intf3 |= CLKTEST_ON_M;
	meson_write(mmc, intf3, MESON_SD_EMMC_INTF3);
	clktest_log = meson_read(mmc, MESON_SD_EMMC_CLKTEST_LOG);
	clktest = meson_read(mmc, MESON_SD_EMMC_CLKTEST_OUT);
	while (!(clktest_log & CLKTEST_DONE)) {
		mdelay(1);
		i++;
		if (i > 4) {
			pr_warn("emmc clktest error\n");
			ret = -EOPNOTSUPP;
			break;
		}
		clktest_log = meson_read(mmc, MESON_SD_EMMC_CLKTEST_LOG);
		clktest = meson_read(mmc, MESON_SD_EMMC_CLKTEST_OUT);
	}
	if (clktest_log & CLKTEST_DONE) {
		clktest = meson_read(mmc, MESON_SD_EMMC_CLKTEST_OUT);
		count = clktest / (1 << 8);
		if (vcfg & CFG_DDR)
			delay_cell = ((cycle / 2) / count);
		else
			delay_cell = (cycle / count);
	}
	pr_info("clktest: %u, delay_cell: %d, count: %u\n", clktest, delay_cell, count);
	intf3 = meson_read(mmc, MESON_SD_EMMC_INTF3);
	intf3 &= ~CLKTEST_ON_M;
	meson_write(mmc, intf3, MESON_SD_EMMC_INTF3);
	vcfg = meson_read(mmc, MESON_SD_EMMC_CFG);
	vcfg |= CFG_AUTO_CLK;
	meson_write(mmc, vcfg, MESON_SD_EMMC_CFG);

	return ret;
}

/* disable enhanced_strobe mode when initialization
 * enable enhanced_strobe mode and tuning intf3 when mmc_select_hs400es
 */
static int meson_set_enhanced_strobe(struct udevice *dev)
{
	int ret = 0;

	ret = aml_mmc_clktest(dev);
	if (ret)
		return 0;

	ret = emmc_ds_manual_sht(dev);
	return ret;
}
#endif

static const struct dm_mmc_ops meson_dm_mmc_ops = {
	.send_cmd = meson_dm_mmc_send_cmd,
	.set_ios = meson_dm_mmc_set_ios,
	.get_cd = meson_get_cd,
	.wait_dat0 = meson_wait_dat0,
#ifdef MMC_SUPPORTS_TUNING
	.execute_tuning = meson_execute_tuning,
#endif
#ifdef CONFIG_MMC_HS400_ES_SUPPORT
	.set_enhanced_strobe = meson_set_enhanced_strobe,
#endif
};

static int meson_mmc_of_to_plat(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc_config *cfg = &pdata->cfg;
	fdt_addr_t addr;
	int ret = 0;

	addr = dev_read_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->regbase = (void *)addr;

	ret = mmc_of_parse(dev, cfg);
	if (ret)
		return ret;

	host->src_clk = dev_read_u32_default(dev, "source-clock", 0);

	dev->name = dev_read_string(dev, "pinname");
	if (dev_read_bool(dev, "non-removable"))
		host->is_in = 1;

	if (dev_read_bool(dev, "ignore_desc_busy"))
		host->ignore_desc_busy = 1;

	if (dev_read_u32(dev, "nwr_cnt", &host->nwr_cnt) < 0)
		host->nwr_cnt = 0;

	dev_read_u32(dev, "card_type", &host->card_type);
	if (aml_card_type_non_sdio(host)) {
		ret = gpio_request_by_name(dev,
					   "cd-gpios", 0, &host->gpio_cd, GPIOD_IS_IN);
		if (ret)
			return ret;
	}
	if (aml_card_type_mmc(host)) {
		ret = gpio_request_by_name(dev, "hw_reset", 0, &host->gpio_reset,
					   GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);
		if (ret)
			return ret;
	}

	clk_get_by_name(dev, "clkin", &host->div2);
	clk_get_by_name(dev, "xtal", &host->xtal);
	clk_get_by_name(dev, "mux", &host->mux);
	clk_get_by_name(dev, "div", &host->div);
	clk_get_by_name(dev, "gate", &host->gate);

	//clk_enable(&host->core);
	clk_enable(&host->gate);

	return 0;
}

static int meson_mmc_probe(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	struct mmc_config *cfg = &pdata->cfg;
	struct clk_bulk clocks;
	u32 val;
	int ret;

	/* Enable the clocks feeding the MMC controller */
	ret = clk_get_bulk(dev, &clocks);
	if (ret)
		return ret;

	ret = clk_enable_bulk(&clocks);
	if (ret)
		return ret;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 |
			MMC_VDD_31_32 | MMC_VDD_165_195;
	cfg->f_min = 400000; /* 400 Khz */
	cfg->b_max = 511; /* max 512 - 1 blocks */
	cfg->name = dev->name;
	host->mmc = &pdata->mmc;
	if (!host->blk_test)
		host->blk_test = malloc(MMC_MAX_BLOCK_LEN * CALI_BLK_CNT);
	if (!host->blk_test) {
		ret = -ENOMEM;
		goto err;
	}
	if (!host->desc_buf)
		host->desc_buf =
			malloc(MMC_MAX_DESC_NUM * (sizeof(struct sd_emmc_desc_info)));
	if (!host->desc_buf) {
		ret = -ENOMEM;
		goto err;
	}
	host->timing = -1;
	mmc->priv = pdata;
	upriv->mmc = mmc;

	mmc_set_clock(mmc, cfg->f_min, MMC_CLK_ENABLE);

#ifdef CONFIG_MMC_PWRSEQ
		/* Enable power if needed */
		ret = mmc_pwrseq_get_power(dev, cfg);
		if (!ret) {
			ret = pwrseq_set_power(cfg->pwr_dev, true);
			if (ret)
				return ret;
		}
#endif

	/* reset all status bits */
	meson_write(mmc, STATUS_MASK, MESON_SD_EMMC_STATUS);

	/* disable interrupts */
	meson_write(mmc, 0, MESON_SD_EMMC_IRQ_EN);

	/* enable auto clock mode */
	val = meson_read(mmc, MESON_SD_EMMC_CFG);
	val &= ~CFG_SDCLK_ALWAYS_ON;
	val |= CFG_AUTO_CLK;
	meson_write(mmc, val, MESON_SD_EMMC_CFG);

	printf("[%s]%s: Controller probe success!\n",
	       __func__, mmc->cfg->name);

	return 0;
err:
	pr_err("[%s]%s: Controller probe fail, ret = %d!\n",
	       __func__, mmc->cfg->name, ret);
	if (host->blk_test)
		free(host->blk_test);
	if (host->desc_buf)
		free(host->desc_buf);
	return ret;
}

int meson_mmc_bind(struct udevice *dev)
{
	struct meson_mmc_plat *pdata = dev_get_plat(dev);

	return mmc_bind(dev, &pdata->mmc, &pdata->cfg);
}

static const struct udevice_id meson_mmc_match[] = {
	{ .compatible = "amlogic,meson-gx-mmc", .data = MMC_COMPATIBLE_GX },
	{ .compatible = "amlogic,meson-axg-mmc", .data = MMC_COMPATIBLE_GX },
	{ .compatible = "amlogic,meson-sm1-mmc", .data = MMC_COMPATIBLE_SM1 },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(meson_mmc) = {
	.name = "meson_gx_mmc",
	.id = UCLASS_MMC,
	.of_match = meson_mmc_match,
	.ops = &meson_dm_mmc_ops,
	.probe = meson_mmc_probe,
	.bind = meson_mmc_bind,
	.of_to_plat = meson_mmc_of_to_plat,
	.plat_auto	= sizeof(struct meson_mmc_plat),
	.priv_auto = sizeof(struct meson_host),
};
