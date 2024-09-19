/*
 * @Author: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @Date: 2024-01-03 10:39:59
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-01-03 10:55:31
 * @FilePath: \v2023\include\amlogic\aml_mtd.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AMLMTD_H_
#define __AMLMTD_H_

#include <amlogic/aml_rsv.h>

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
extern struct mtd_partition *get_aml_mtd_partition(void);
extern int get_aml_partition_count(void);
extern struct part_info *get_aml_mtdpart_by_index(struct mtd_info *master,
						   int idx);
uint64_t mtd_get_normal_part_offset(struct mtd_info *mtd);
int mtd_raw_nand_add_boot_partitions(struct mtd_info *mtd);
int mtd_raw_nand_add_normal_partitions(struct mtd_info *mtd,
				       const struct mtd_partition *parts,
				       int nbparts);
int mtd_spi_nand_add_partitions(struct mtd_info *mtd,
				const struct mtd_partition *parts,
				int nbparts);
#endif/* __AMLMTD_H_ */
