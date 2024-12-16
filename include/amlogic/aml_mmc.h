/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_MMC_H__
#define __AML_MMC_H__

#include <common.h>
#include <mmc.h>
#include "emmc_partitions.h"

/*mmc.h*/
#define NO_CARD_ERR   -16
#define UNUSABLE_ERR  -17
#define COMM_ERR      -18
#define TIMEOUT       -19
#define IN_PROGRESS   -20
#define SWITCH_ERR    -21

#define MESON_SD_EMMC_CALOUT		0x10
#define MESON_SD_EMMC_ADJ_IDX_LOG	0x20
#define MESON_SD_EMMC_CLKTEST_LOG	0x24
#define   CLKTEST_TIMES_MASK		GENMASK(30, 0)
#define   CLKTEST_DONE			BIT(31)
#define MESON_SD_EMMC_CLKTEST_OUT	0x28
#define MESON_SD_EMMC_EYETEST_LOG	0x2c
#define   EYETEST_TIMES_MASK		GENMASK(30, 0)
#define   EYETEST_DONE			BIT(31)
#define MESON_SD_EMMC_EYETEST_OUT0	0x30
#define MESON_SD_EMMC_EYETEST_OUT1	0x34
#define MESON_SD_EMMC_INTF3		0x38
#define   CLKTEST_EXP_MASK		GENMASK(4, 0)
#define   CLKTEST_ON_M			BIT(5)
#define   EYETEST_EXP_MASK		GENMASK(10, 6)
#define   EYETEST_ON			BIT(11)
#define   DS_SHT_M_MASK			GENMASK(17, 12)
#define   DS_SHT_EXP_MASK		GENMASK(21, 18)
#define   SD_INTF3			BIT(22)
#define   EYETEST_SEL			BIT(26)
#define   RESP_SEL			BIT(27)
#define   CFG_RX_SEL			BIT(26)
#define   CFG_RX_PN			BIT(27)
#define   RESP_OLD			BIT(28)
#define   RESP_PN			BIT(29)
#define   RESP_DS			BIT(30)

#define MMC_CMD23

#define MMC_CMD_SET_WRITE_PROTECT       28
#define MMC_CMD_CLR_WRITE_PROT          29
#define MMC_CMD_SEND_WRITE_PROT         30
#define MMC_CMD_SEND_WRITE_PROT_TYPE    31
#define MMC_SD_HS_TUNING		70

#define MMC_KEY_SIZE            (256*1024)
#define EMMC_KEY_DEV            (1)

/* FFU operation */
#define SAMSUNG_MID			0x15
#define KINGSTON_MID			0x70
#define BIWIN_MID			0xf4
#define SAMSUNG_FFU_ADDR		0xc7810000
#define KINGSTON_FFU_ADDR		0x0000ffff
#define BIWIN_FFU_ADDR			0x0

#define EXT_CSD_CLASS_6_CTRL		59  /*R/W/E_P*/
#define EXT_CSD_DRIVER_STRENGTH		197	/* RO */
#define EXT_CSD_DEV_LIFETIME_EST_TYP_A	268	/* RO */
#define EXT_CSD_DEV_LIFETIME_EST_TYP_B	269	/* RO */
#define EXT_CSD_SUPPORTED_MODES		493 /* RO */
#define EXT_CSD_FW_VERSION		254 /* RO, 261:254 */
#define EXT_CSD_FW_CFG			169 /* R/W */
#define EXT_CSD_MODE_CFG		30 /* R/W */
#define EXT_CSD_FFU_STATUS		26 /* RO */

#define US_PWR_WP_DIS_BIT      1<<3
#define US_PERM_WP_DIS_BIT     1<<4
#define WP_CLEAR_TYPE          0
#define WP_POWER_ON_TYPE       (1<<1)
#define WP_TEMPORARY_TYPE      1
#define WP_PERMANENT_TYPE      ((1<<0)|(1<<1))
#define WP_TYPE_MASK           3
#define WP_ENABLE_MASK         7
#define WP_TEMPORARY_EN_BIT    0
#define WP_POWER_ON_EN_BIT     (1<<0)
#define WP_PERM_EN_BIT         (1<<2)
#define WP_GRP_SIZE_MASK       31

#define MAX_TUNING_RETRY	(4)
#define CALI_BLK_CNT		(1024)
#define REFIX_BLK_CNT		(100)
#define TUNING_NUM_PER_POINT	40
#define MMC_MAX_DESC_NUM	512
#define MAX_RESPONSE_BYTES	4
#define SD_EMMC_DESC_MAX_BLKS	511

#ifndef CONFIG_SYS_MMC_MAX_BLK_COUNT
#define CONFIG_SYS_MMC_MAX_BLK_COUNT (MMC_MAX_DESC_NUM * SD_EMMC_DESC_MAX_BLKS)
#endif
/*mmc.h*/

/* bootloader operation */
#define AML_BL_USER		(0x1 << 0)
#define AML_BL_BOOT0	(0x1 << 1)
#define AML_BL_BOOT1	(0x1 << 2)
#define AML_BL_BOOT     (0x6)
#define AML_BL_ALL		(0x7)

/* delay_cell=70ps, 1ns/delay_cell */
#define DELAY_CELL_COUNTS 14

/** For actually partitions with mask 8 store into bootinfo
 * name: partition name.
 * addr: sector addr of the partition
 * size: sector cont of the partition
*/
struct part_property
{
	char name[8];
	uint32_t addr;
	uint32_t size;
};
#define PART_PROPERTY_SIZE sizeof(struct part_property)
#define BOOTINFO_MAX_PARTITIONS (4)
#define BOOTINFO_PARTITIONS_SIZE (PART_PROPERTY_SIZE * BOOTINFO_MAX_PARTITIONS)


#define VPART_PROPERTY_SIZE sizeof(struct vpart_property)
struct vpart_property {
	u32 addr;
	u32 size;
};

/*
 * sizeof(struct storage_emmc_boot_info) is strictly
 * smaller than or equal to one sector. we will bind
 * it in one sector with u-boot.bin together and
 * write into boot loader area.
 * @rsv_base_addr : the sector address of reserved area
 * @dtb  : the sector address and size of dtb property
 * @ddr  : the sector address and size of ddr property
 */
#define EMMC_BOOT_INFO_SIZE	512
struct storage_emmc_boot_info {
	u32 version;
	u32 rsv_base_addr;
	struct vpart_property dtb;
	struct vpart_property ddr;

	struct part_property parts[BOOTINFO_MAX_PARTITIONS];
	uint8_t reserved[512 - 2 * VPART_PROPERTY_SIZE - BOOTINFO_PARTITIONS_SIZE - 12];
	u32 checksum;
};

#define stamp_after(a,b)   ((int)(b) - (int)(a)  < 0)

int amlmmc_write_bootloader(int dev, int map,
		unsigned int size, const void *src);
int amlmmc_erase_bootloader(int dev, int map);

/* interface on reserved area. */
void mmc_write_cali_mattern(void *addr, struct aml_pattern *table);

/* dtb operation */
int dtb_write(void *addr);

/* emmc key operation */
int mmc_key_read(unsigned char *buf,
		unsigned int size, uint32_t *actual_length);
int mmc_key_write(unsigned char *buf,
		unsigned int size, uint32_t *actual_length);
int mmc_key_erase(void);

/* partition operation */
int renew_partition_tbl(unsigned char *buffer);
int find_dev_num_by_partition_name (char const *name);

#ifdef CONFIG_AML_PARTITION
int emmc_update_mbr(unsigned char *buffer);
#endif

/*mmc ext_csd register operation*/
int mmc_get_ext_csd(struct mmc *mmc, u8 *ext_csd);
int mmc_set_ext_csd(struct mmc *mmc, u8 index, u8 value);

/* mmc caps quirks */
int emmc_quirks(void);
#endif /* __AML_MMC_H__ */
