// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <version.h>
#include <linux/compiler.h>
#include <amlogic/cpu_id.h>
#include <asm/amlogic/arch/cpu_config.h>



#ifdef CONFIG_BUILD_MESSAGE
#include <asm/amlogic/arch/bl31_apis.h>
#include <malloc.h>
#include <linux/libfdt.h>
#include <fdt_support.h>

#define U_BOOT_VERSION_STRING U_BOOT_VERSION " (" U_BOOT_DATE " - " \
	U_BOOT_TIME " " U_BOOT_TZ ")" CONFIG_IDENT_STRING

const char __weak version_string[] = U_BOOT_VERSION_STRING;

#ifdef CONFIG_BL30_VERSION_SAVE
#include <asm/amlogic/arch/mailbox.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <asm/amlogic/arch/cpu.h>
#define GET_BL30_VERSION_SIZE 0x30303000
#define GET_BL30_VERSION_INFO 0x30303001
#define GET_BL30_VERSION_TIME 0x30303002
#define GET_BL30_LEN_MAX 400
#define GET_BL30_LEN_ONCE MAILBOX_USER_DATA_SIZE
static int bl30_version_flag, bl30_version_size;
static char bl30_version_str[GET_BL30_LEN_MAX];
static char bl30_version_str_once[GET_BL30_LEN_ONCE];
#endif

#ifdef CONFIG_BL30_VERSION_RTOSSDK
#define BUFF_LENGTH		(512 + 400)
static int bl30_version_total_size, bl30_version_size_once;
static int bl30_version_trans_times;
#else
#define BUFF_LENGTH		512
#endif

const char compile_user[] = COMPILE_USER;
unsigned char *bootloader_build_message;
char version_buffer[BUFF_LENGTH];
int index;

int putin_buf(const char *str)
{
	char *ptr = version_buffer;
	int num = strlen(str);

	if ((index + num) > BUFF_LENGTH)
		return -1;

	ptr += index;
	memcpy(ptr, str, num);
	index += num;

	return 0;
}

void free_build_message(void)
{
	if (bootloader_build_message)
		free(bootloader_build_message);
}
#endif

int get_bootloader_build_message(void)
{
#ifdef CONFIG_BUILD_MESSAGE
	static build_messages_t *build_info;

	bootloader_build_message = (uint8_t *)malloc(sizeof(build_messages_t));
	if (!bootloader_build_message) {
		printf("version message malloc fail\n");
		return -1;
	}

	if (!aml_get_bootloader_version(bootloader_build_message)) {
		build_info = (build_messages_t *)(bootloader_build_message);
		if (build_info->h.type != PARAM_MESSAGE ||
			build_info->h.version != VERSION_2 ||
			build_info->h.size != sizeof(build_messages_t)) {
			printf("invalid build messages info head!\n");
			free_build_message();
			return -1;
		}
		/*
		 * snprintf(build_info->bl33_message.hash, 32, "%s", git_hash);
		 * snprintf(build_info->bl33_message.user, 32, "%s", compile_user);
		 * snprintf(build_info->bl33_message.time, 64, "%s%s", compile_time, compile_data);
		 * build_info->bl33_message.ver = INDEX_BL33_15;
		 * build_info->bl33_message.init_flag = 1;
		 */
		putin_buf("Bootloader:{\n");
		if (build_info->bl2_message.init_flag) {
			putin_buf("\tBL2 :");
			if (build_info->bl2_message.ver == INDEX_SPL)
				putin_buf(" SPL-");
			else if (build_info->bl2_message.ver == INDEX_CORE)
				putin_buf(" CORE-");
			//putin_buf(build_info->bl2_message.ver_str);
			//putin_buf(",");
			putin_buf(build_info->bl2_message.hash);
			putin_buf(",");
			if (build_info->bl2_message.ext_message.ddr.ddr_extern) {
				putin_buf(" DDR-");
				putin_buf(build_info->bl2_message.ext_message.ddr.ddr_hash);
				putin_buf(",");
			}
			putin_buf(build_info->bl2_message.time);
			putin_buf(",");
			putin_buf(build_info->bl2_message.user);
			putin_buf("\n");
		}

		if (build_info->bl2e_message.init_flag) {
			putin_buf("\tBL2E: ");
			//putin_buf(build_info->bl2e_message.ver_str);
			//putin_buf(",");
			putin_buf(build_info->bl2e_message.hash);
			putin_buf(",");
			putin_buf(build_info->bl2e_message.time);
			putin_buf(",");
			putin_buf(build_info->bl2e_message.user);
			putin_buf("\n");
		}

		if (build_info->bl2x_message.init_flag) {
			putin_buf("\tBL2X: ");
			//putin_buf(build_info->bl2x_message.ver_str);
			//putin_buf(",");
			putin_buf(build_info->bl2x_message.hash);
			putin_buf(",");
			putin_buf(build_info->bl2x_message.time);
			putin_buf(",");
			putin_buf(build_info->bl2x_message.user);
			putin_buf("\n");
		}

#ifdef CONFIG_BL30_VERSION_SAVE
		bl30_version_flag = GET_BL30_VERSION_SIZE;
		if (!scpi_send_data(AOCPU_REE_CHANNEL, CMD_GET_BL30_VERSION,
		    &bl30_version_flag, 4, &bl30_version_size, 4)) {
			if (bl30_version_size > 0)
				build_info->bl30_message.init_flag = 1;
		} else {
			printf("bl30 version message get fail\n");
		}
#endif
		if (build_info->bl30_message.init_flag) {
			putin_buf("\tBL30: ");
			//putin_buf(build_info->bl30_message.ver_str);
			//putin_buf(",");
#ifdef CONFIG_BL30_VERSION_SAVE
#ifdef CONFIG_BL30_VERSION_RTOSSDK
			bl30_version_flag = GET_BL30_VERSION_TIME;
			if (!scpi_send_data(AOCPU_REE_CHANNEL, CMD_GET_BL30_VERSION,
			    &bl30_version_flag, 4, &bl30_version_str_once, GET_BL30_LEN_ONCE)) {
				putin_buf(bl30_version_str_once);
				memset(bl30_version_str_once, 0, GET_BL30_LEN_ONCE);
			} else {
				printf("bl30 version message get fail\n");
			}

			bl30_version_flag = GET_BL30_VERSION_INFO;
			bl30_version_total_size = bl30_version_size & 0xffff;
			bl30_version_size_once = bl30_version_size >> 16;
			if (bl30_version_total_size % bl30_version_size_once)
				bl30_version_trans_times =
					bl30_version_total_size / bl30_version_size_once + 1;
			else
				bl30_version_trans_times =
					bl30_version_total_size / bl30_version_size_once;

			for (int i = 0; i < bl30_version_trans_times; i++) {
				if (!scpi_send_data(AOCPU_REE_CHANNEL, CMD_GET_BL30_VERSION,
				    &bl30_version_flag, 4, &bl30_version_str_once,
				    bl30_version_size_once)) {
					memcpy(&bl30_version_str[i * bl30_version_size_once],
					bl30_version_str_once, bl30_version_size_once);
					memset(bl30_version_str_once, 0, bl30_version_size_once);
				} else {
					printf("bl30 version message get fail\n");
					break;
				}
			}
#else
			bl30_version_flag = GET_BL30_VERSION_INFO;
			for (int i = 0; i < (bl30_version_size / GET_BL30_LEN_ONCE + 1); i++) {
				if (!scpi_send_data(AOCPU_REE_CHANNEL, CMD_GET_BL30_VERSION,
				    &bl30_version_flag, 4, &bl30_version_str_once,
				    GET_BL30_LEN_ONCE)) {
					memcpy(&bl30_version_str[i * GET_BL30_LEN_ONCE],
					bl30_version_str_once, GET_BL30_LEN_ONCE);
					memset(bl30_version_str_once, 0, GET_BL30_LEN_ONCE);
				} else {
					printf("bl30 version message get fail\n");
					break;
				}
			}
#endif
			putin_buf(bl30_version_str);
#else
			putin_buf(build_info->bl30_message.hash);
			putin_buf(",");
			putin_buf(build_info->bl30_message.time);
			putin_buf(",");
			putin_buf(build_info->bl30_message.user);
			putin_buf("\n");
#endif
		}

		if (build_info->bl31_message.init_flag) {
			putin_buf("\tBL31: ");
			putin_buf(build_info->bl31_message.ver_str);
			putin_buf(",");
			putin_buf(build_info->bl31_message.hash);
			putin_buf(",");
			putin_buf(build_info->bl31_message.time);
			putin_buf(",");
			putin_buf(build_info->bl31_message.user);
			putin_buf("\n");
		}

		if (build_info->bl32_message.init_flag) {
			putin_buf("\tBL32 :");
			putin_buf(build_info->bl32_message.ver_str);
			putin_buf(",");
			putin_buf(build_info->bl32_message.hash);
			putin_buf(",");
			putin_buf(build_info->bl32_message.time);
			putin_buf(",");
			putin_buf(build_info->bl32_message.user);
			putin_buf("\n");
		}

		putin_buf("\tBL33: ");
		putin_buf(version_string);
		putin_buf(",");
		putin_buf(compile_user);
		putin_buf("\n");
		putin_buf("}\n\0");

		free_build_message();
		return 0;
	}
#endif
	printf("no BUILD_MESSAGE\n");

	return -1;
}

static int do_build_message_print(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_BUILD_MESSAGE
	if (index > 0) {
		printf("%s", version_buffer);
		return 0;
	}
#endif
	printf("no version buffer data!\n");
	return -1;
}

int fdt_bl_version(void *fdt)
{
#ifdef CONFIG_BUILD_MESSAGE
	int   nodeoffset;
	int   err;

	if (index > 0) {
		err = fdt_check_header(fdt);
		if (err < 0) {
			printf("fdt_chosen: %s\n", fdt_strerror(err));
			return err;
		}

		/* find or create "/chosen" node. */
		nodeoffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
		if (nodeoffset < 0)
			return nodeoffset;

		err = fdt_setprop(fdt, nodeoffset, "bootloader_build", version_buffer,
				strlen(version_buffer) + 1);
		if (err < 0) {
			printf("WARNING: could not set bl2_build %s.\n",
			       fdt_strerror(err));
			return err;
		}
	}
#endif
	return 0;
}

U_BOOT_CMD(build_message,	1,		1,	do_build_message_print,
	"print bl2/bl2e/bl2x/bl31/bl32/bl33 version",
	""
);

static int do_bootloader_version(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char s_version[64];
	strcpy(s_version, "01.01.");
	strcat(s_version, U_BOOT_DATE_TIME);
	printf("s_version: %s\n", s_version);
	env_set("bootloader_version", s_version);
	return 0;
}

U_BOOT_CMD(
	get_bootloaderversion,	1,		0,	do_bootloader_version,
	"print bootloader version",
	""
);

static int do_get_cpuid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char chipid[16];
	char chipid_str[32];
	int i, j;
	char buf_tmp[4];
	char *buff = NULL;

	memset(chipid, 0, 16);
	if (get_chip_id(chipid, 16)) {
		env_set("cpu_id", "1122334455667788"); //default
		return 0;
	}

	memset(chipid_str, 0, 32);
	buff = &chipid_str[0];
	for (i = 0, j = 0; i < 12; ++i) {
		sprintf(&buf_tmp[0], "%02x", chipid[15 - i]);
		if (strcmp(buf_tmp, "00") != 0) {
			sprintf(buff + j, "%02x", chipid[15 - i]);
			j = j + 2;
		}
	}
	env_set("cpu_id", chipid_str);
	printf("buff: %s\n", buff);

	return 0;
}

U_BOOT_CMD_COMPLETE(
	get_cpuid,         //get cpu id to env cpu_id
	1,                 //maxargs
	0,                 //repeatable
	do_get_cpuid,      //command function
	"read cpuid to env cpu_id",           //description
	"    argv: get_cpuid\n",   //usage
	var_complete
);
