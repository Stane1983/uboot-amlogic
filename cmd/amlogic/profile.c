// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <amlogic/aml_profile.h>
#include <asm/amlogic/arch/cpu_config.h>

#define BL2_CORE "BL2"
#define BL2_REE "BL2E"
#define BL2_TEE "BL2X"
#define BL31	"BL31"
#define BL33	"BL33"

int dump_boot_time_flag;

struct measurement_item  bl2_core_measurement_item[] = {
	{.start_index = BL2_CORE_ROM, .end_index = BL2_CORE_ENTRY,
		.name = "ROM time:"},
	{.start_index = BL2_CORE_DDR_s, .end_index = BL2_CORE_DDR_e,
		.name = "ddr time:"},
	{.start_index = BL2_CORE_LOAD_BL2E_s, .end_index = BL2_CORE_LOAD_BL2E_e,
		.name = "load bl2e time:"},
	{.start_index = BL2_CORE_LOAD_BL2X_s, .end_index = BL2_CORE_LOAD_BL2X_e,
		.name = "load bl2x time:"},
	{.start_index = BL2_CORE_VERIFY_BL2EX_s, .end_index = BL2_CORE_VERIFY_BL2EX_e,
		.name = "verify bl2e,bl2x time:"},
	{.start_index = BL2_CORE_ENTRY, .end_index = BL2_CORE_BOOT_BL2E,
		.name = "total bl2 time:"},
};

#define BL2_CORE_MEASUREMENT_ITEM_COUNT	(ARRAY_SIZE(bl2_core_measurement_item))

struct measurement_item  bl2e_measurement_item[] = {
	{.start_index = BL2_REE_LOAD_HEADER_s, .end_index = BL2_REE_LOAD_HEADER_e,
		.name = "load fip head time:"},
	{.start_index = BL2_REE_PROCESS_HEADER_s, .end_index = BL2_REE_PROCESS_HEADER_e,
		.name = "process fip head time:"},
	{.start_index = BL2_REE_LOAD_DEVFIP_s, .end_index = BL2_REE_LOAD_DEVFIP_e,
		.name = "load dev-fip time:"},
	{.start_index = BL2_REE_PROCESS_DEVFIP_s, .end_index = BL2_REE_PROCESS_DEVFIP_e,
		.name = "process dev-fip time:"},
	{.start_index = BL2_REE_ENTRY, .end_index = BL2_REE_PROCESS_DEVFIP_e,
		.name = "total bl2e time:"},
};

#define BL2E_MEASUREMENT_ITEM_COUNT	(ARRAY_SIZE(bl2e_measurement_item))

struct measurement_item  bl2x_measurement_item[] = {
	{.start_index = BL2_TEE_PARSE_BL3X, .end_index = BL2_TEE_DECRYPT_BL3X,
		.name = "parse bl3x time:"},
	{.start_index = BL2_TEE_DECRYPT_BL3X, .end_index = BL2_TEE_BOOT_BL31,
		.name = "decrypt,auth bl3x time:"},
	{.start_index = BL2_TEE_ENTRY, .end_index = BL2_TEE_BOOT_BL31,
		.name = "total bl2x time:"},
};

#define BL2X_MEASUREMENT_ITEM_COUNT	(ARRAY_SIZE(bl2x_measurement_item))

struct measurement_item  bl31_measurement_item[] = {
	{.start_index = BL31_SOC_INIT_s, .end_index = BL31_SOC_INIT_e,
		.name = "soc init time:"},
	{.start_index = BL31_BL32_INIT_s, .end_index = BL31_BL32_INIT_e,
		.name = "bl32 init time:"},
	{.start_index = BL31_ENTRY, .end_index = BL31_EXIT_MAIN,
		.name = "total bl31,bl32 time:"},
};

#define BL31_MEASUREMENT_ITEM_COUNT	(ARRAY_SIZE(bl31_measurement_item))

struct measurement_item bl33_measurement_item[] = {
	{.start_index = BL33_ENTRY, .end_index = BL33_RELOCATE_finish,
		.name = "finish relocation time:"},
	{.start_index = BL33_STORAGE_s, .end_index = BL33_STORAGE_e,
		.name = "storage init time:"},
	{.start_index = BL33_ENV_s, .end_index = BL33_ENV_e,
		.name = "env init time:"},
	{.start_index = BL33_LOAD_DTB_s, .end_index = BL33_LOAD_DTB_e,
		.name = "load dtb time:"},
	{.start_index = BL33_VPUVPP_INIT_s, .end_index = BL33_VPUVPP_INIT_e,
		.name = "vpu vpp init:"},
	{.start_index = BL33_PREBOOT_s, .end_index = BL33_PREBOOT_e,
		.name = "preboot time:"},
	{.start_index = BL33_LOAD_UIMAGE_s, .end_index = BL33_LOAD_UIMAGE_e,
		.name = "load uimage time:"},
	{.start_index = BL33_LOAD_UIMAGE_e, .end_index = BL33_BOOT_KERNEL_s,
		.name = "boot uimage time:"},
	{.start_index = BL33_ENTRY, .end_index = BL33_BOOT_KERNEL_s,
		.name = "total bl33 time:"},
};

#define BL33_MEASUREMENT_ITEM_COUNT	(ARRAY_SIZE(bl33_measurement_item))

struct time_point_t time_point_bl33[BL33_MAX_NUM] = {{BL33, 1}, {"_start", 0},};

void push_time_te(const char *func, unsigned int stage)
{
	if (stage >= BL33_MAX_NUM) {
		printf("%s, func full\n", __func__);
		return;
	}

	if (stage > BL33_ENTRY) {
		unsigned int len = (strlen(func) > 11) ? 11 : strlen(func);

		memcpy(time_point_bl33[stage].func, func, len);
	}
	time_point_bl33[stage].time = get_time();
}

void push_time_te_bl33(void)
{
	time_point_bl33[BL33_ENTRY].time = get_time();
}

static void print_item(unsigned long addr, void *item, unsigned int num)
{
	struct time_point_t *point = (struct time_point_t *)addr;
	struct measurement_item *pitem = (struct measurement_item *)item;
	int i, time_num = 0;
	char *profile = NULL;

	if (point[0].time == 1) {
		printf("%s stage time(us):-------------\n", point[0].func);
		for (i = 0; i < num; i++) {
			printf("%-24s %8d\n", pitem[i].name, point[pitem[i].end_index].time
					- point[pitem[i].start_index].time);
		}

		if (env_get("profile")) {
			profile = env_get("profile");
			if (!(strcmp(profile, "0") && strcmp(profile, "off")))
				return;

			if (strcmp(point[0].func, BL2_CORE) == 0)
				time_num = BL2_CORE_MAX_NUM;
			else if (strcmp(point[0].func, BL2_REE) == 0)
				time_num = BL2_REE_MAX_NUM;
			else if (strcmp(point[0].func, BL2_TEE) == 0)
				time_num = BL2_TEE_MAX_NUM;
			else if (strcmp(point[0].func, BL31) == 0)
				time_num = BL31_MAX_NUM;
			else if (strcmp(point[0].func, BL33) == 0)
				time_num = BL33_MAX_NUM;

			for (i = 1; i < time_num; i++)
				printf("%3d, %16s %7d\n", i, point[i].func, point[i].time);
		}
	}
}

void dump_te(void)
{
	if (dump_boot_time_flag == 0)
		return;

	print_item(BL2E_PARAM_CORE, &bl2_core_measurement_item[0],
		   BL2_CORE_MEASUREMENT_ITEM_COUNT);
	print_item(BL2E_PARAM_BL2E, &bl2e_measurement_item[0],
		   BL2E_MEASUREMENT_ITEM_COUNT);
	print_item(BL2E_PARAM_BL2X, &bl2x_measurement_item[0],
		   BL2X_MEASUREMENT_ITEM_COUNT);
	print_item(BL2E_PARAM_BL31, &bl31_measurement_item[0],
		   BL31_MEASUREMENT_ITEM_COUNT);
	print_item((unsigned long)&time_point_bl33[0],
		   &bl33_measurement_item[0], BL33_MEASUREMENT_ITEM_COUNT);
}

static int do_pushte(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 1)
		return CMD_RET_USAGE;

	dump_boot_time_flag = 1;
	dump_te();
	dump_boot_time_flag = 0;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(pushte,           //command name
	1,                   //maxargs
	0,                   //repeatable
	do_pushte,   //command function
	"print profile time",           //description
	"(no argv)\n"   //usage
	"Set env \"profile\" to print detailed time nodes"
);
