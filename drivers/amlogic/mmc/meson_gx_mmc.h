/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MESON_GX_MMC_H__
#define __MESON_GX_MMC_H__

#include <mmc.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>

enum meson_gx_mmc_compatible {
	MMC_COMPATIBLE_GX,
	MMC_COMPATIBLE_SM1,
};

#define SDIO_PORT_A			0
#define SDIO_PORT_B			1
#define SDIO_PORT_C			2

#define SD_EMMC_CLKSRC_24M		24000000	/* 24 MHz */
#define SD_EMMC_CLKSRC_DIV2		1000000000	/* 1 GHz */

struct meson_host {
	struct mmc *mmc;
	uint is_in;
	uint is_sduart;
	uint is_tuning;
	uint card_type;
	uint src_clk;
	uint ignore_desc_busy;
	uint nwr_cnt;
	struct clk core;
	struct clk xtal;
	struct clk div2;
	struct clk mux;
	struct clk div;
	struct clk gate;
	struct gpio_desc gpio_cd;
	struct gpio_desc gpio_reset;
	char *blk_test;
	char *desc_buf;
	unsigned char timing;
	dma_addr_t dma_addr_data;
	dma_addr_t dma_addr_info;
};

struct sd_emmc_desc_info {
	u32 cmd_info;
	u32 cmd_arg;
	u32 data_addr;
	u32 resp_addr;
};

/* unknown */
#define CARD_TYPE_UNKNOWN       0
/* MMC card */
#define CARD_TYPE_MMC           1
/* SD card */
#define CARD_TYPE_SD            2
/* SDIO card */
#define CARD_TYPE_SDIO          3
/* SD combo (IO+mem) card */
#define CARD_TYPE_SD_COMBO      4
/* NON sdio device (means SD/MMC card) */
#define CARD_TYPE_NON_SDIO      5

#define aml_card_type_unknown(c)    ((c)->card_type == CARD_TYPE_UNKNOWN)
#define aml_card_type_mmc(c)        ((c)->card_type == CARD_TYPE_MMC)
#define aml_card_type_sd(c)      ((c)->card_type == CARD_TYPE_SD)
#define aml_card_type_sdio(c)      ((c)->card_type == CARD_TYPE_SDIO)
#define aml_card_type_non_sdio(c)   ((c)->card_type == CARD_TYPE_NON_SDIO)

struct meson_mmc_plat {
	struct mmc_config cfg;
	struct mmc mmc;
	void *regbase;
};

#endif
