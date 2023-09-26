// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <amlogic/rvc_interface.h>
#include <asm/arch/io.h>
#include <asm/byteorder.h>
#include <command.h>
#include <common.h>
#include <config.h>
#include <environment.h>
#include <ext4fs.h>
#include <fs.h>
#include <linux/ctype.h>
#include <malloc.h>
#include <partition_table.h>
#include <stdlib.h>
#include <string.h>
#include <u-boot/sha1.h>

#define CAR_SETTING_MMC_PART_A "1:10"
#define CAR_SETTING_MMC_PART_B "1:18"
#define CAR_SETTING_FILENAME "config.ini"
#define CAR_FREERTOS_SHARED_MEM_ADDR 0x80001100

#ifdef CONFIG_CMD_CAR_PARAMS
#define CALIBRATOR_MAGIC 1234567 /* magic number */

/*
static uint32_t car_crc32(const uint8_t *buf, size_t size)
{
	static uint32_t crc_table[256];
	uint32_t ret = -1;

	// Compute the CRC-32 table only once.
	if (!crc_table[1]) {
		for (uint32_t i = 0; i < 256; ++i) {
			uint32_t crc = i;

			for (uint32_t j = 0; j < 8; ++j) {
				uint32_t mask = -(crc & 1);

				crc = (crc >> 1) ^ (0xEDB88320 & mask);
			}
			crc_table[i] = crc;
		}
	}

	for (size_t i = 0; i < size; ++i)
		ret = (ret >> 8) ^ crc_table[(ret ^ buf[i]) & 0xFF];

	return ~ret;
}
*/

IRtosParam *car_param;
static int do_ReadCarParams(cmd_tbl_t *cmdtp, int flag, int argc,
	char *const argv[])
{
	ini_t config = {NULL, NULL};
	unsigned char *loadaddr = NULL;
	char *active_slot = NULL;
	const char *a_slot = "_a";
	const char *b_slot = "_b";
	char *car_mem_addr = env_get("car_mem_addr");

	car_param = NULL;

	//car_mem_addr is define in t7_an400_mercury.h
	if (car_mem_addr) {
		loadaddr = (unsigned char *)CAR_FREERTOS_SHARED_MEM_ADDR;
		//simple_strtoul is not obsolete, use CAR_FREERTOS_SHARED_MEM_ADDR to do;
		printf("[%s] car_mem_addr: %p, loadaddr: %p\n", __func__, car_mem_addr, loadaddr);
	} else {
		printf("[%s] can's get car_mem_addr, skip to read\n", __func__);
		return -1;
	}

	car_param = (IRtosParam *)loadaddr;
	car_param->magic = CALIBRATOR_MAGIC;

	active_slot = env_get("active_slot");
	if (!active_slot) {
		run_command("get_valid_slot", 0);
		active_slot = env_get("active_slot");
	}

	printf("[%s] read ini file from slot: %s\n", __func__, active_slot);
	/*if need support a/b, change to CAR_SETTING_MMC_PART_B, but now we only has one slot*/
	if (strcmp(active_slot, a_slot) == 0) {
		if (read_ini_file(&config, CAR_SETTING_MMC_PART_A, CAR_SETTING_FILENAME) >= 0) {
			ini_to_param(car_param, &config);
		} else {
			printf("[%s] read ini file: %s from %s failed\n", __func__,
			CAR_SETTING_FILENAME, CAR_SETTING_MMC_PART_A);
		}
	} else if (strcmp(active_slot, b_slot) == 0) {
		if (read_ini_file(&config, CAR_SETTING_MMC_PART_A, CAR_SETTING_FILENAME) >= 0) {
			ini_to_param(car_param, &config);
		} else {
			printf("[%s] read ini file: %s from %s failed\n", __func__,
			CAR_SETTING_FILENAME, CAR_SETTING_MMC_PART_A);
		}
	} else {
		printf("[%s] active_slot is not a or b, skip to read\n", __func__);
		return -1;
	}

	printf("[%s] IRtosParam size: %ld write to 0x%p\n", __func__, sizeof(IRtosParam), loadaddr);

	if (config.data)
		free(config.data);

	print_param(car_param);
	return 0;
}

#endif

U_BOOT_CMD(read_car_params, 3, 0, do_ReadCarParams, "read_car_params",
	"\nThis command will read car params to mem\n"
	"partition by mark to decide whether execute command!\n"
	"So you can execute command: read_car_params");
