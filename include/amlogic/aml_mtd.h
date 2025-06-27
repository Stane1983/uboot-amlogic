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
u64 mtd_spinor_get_bootpart_size(void);
u64 mtd_spinor_get_boot_size(void);
u64 mtd_spinor_get_rsv_part_size(const char *rsv_name);
int mtd_spinor_get_rsv_part_offset(const char *rsv_name,
				   u64 *retoff, u64 off);
u64 mtd_get_normal_part_offset(struct mtd_info *mtd);
int mtd_register_partitions(struct mtd_info *mtd,
			    const struct mtd_partition *parts,
			    int nbparts, bool nand);
#endif/* __AMLMTD_H_ */
