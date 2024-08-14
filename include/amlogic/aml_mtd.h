/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLMTD_H_
#define __AMLMTD_H_

#define BOOT_TOTAL_PAGES			1024
#define NAND_RSV_OFFSET	1024

#define CONFIG_BL2_COPY_NUM			8
#ifdef BOARD_BL2_BACKUPS
#define CONFIG_SPINAND_BL2_COPY_NUM BOARD_BL2_BACKUPS
#endif
#define CONFIG_TPL_SIZE_PER_COPY	0x200000
#define CONFIG_NOR_TPL_COPY_NUM		1 //for spi-nor
#ifdef BOARD_DEVFIP_BACKUPS
#define CONFIG_NAND_TPL_COPY_NUM	BOARD_DEVFIP_BACKUPS
#else
#define CONFIG_NAND_TPL_COPY_NUM        4
#endif

#define BOOT_LOADER			"bootloader"
#define BOOT_BL2			"bl2"
#define BOOT_SPL			"spl"
#define BOOT_BL2E                       "bl2e"
#define BOOT_BL2X                       "bl2x"
#define BOOT_DDRFIP                     "ddrfip"
#define BOOT_DEVFIP                     "devfip"
#define BOOT_TPL			"tpl"
#define BOOT_FIP			"fip"
#define MAX_MTD_CNT			2

struct mtd_info;
struct mtd_partition;
int mtd_store_get_offset(const char *partname, loff_t *retoff, loff_t off);

int mtd_add_normal_partitions(struct mtd_info *mtd,
				  const struct mtd_partition *parts,
				  int nbparts, int normal_offset);
int mtd_add_partitions(struct mtd_info *mtd,
			   const struct mtd_partition *parts,
			   int nbparts);
int mtd_add_boot_partitions(struct mtd_info *mtd,
			struct mtd_partition *parts,
			int nparts);
int mtd_get_boot_parts_num(void);
int mtd_get_boot_partition(struct mtd_info *mtd,
				struct mtd_partition *parts,
				uint8_t index,
				uint8_t cnt);
uint64_t mtd_get_normal_part_offset(struct mtd_info *mtd);
#endif/* __AMLMTD_H_ */
