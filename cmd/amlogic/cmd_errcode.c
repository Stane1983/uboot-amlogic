// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <config.h>
#include <asm/amlogic/arch/reboot.h>
#include <asm/amlogic/arch/io.h>
#include <amlogic/storage.h>
#include <stdlib.h>
#include <amlogic/store_wrapper.h>
#include <linux/libfdt_env.h>
#include <amlogic/libavb/libavb.h>

#define MISCBUF_SIZE  1088
#define ERRCODE_OFFSET 896

struct avb_list {
	char part_name[32];
	int errno;
};

struct error_list {
	char err_string[32];
	char err_code[8];
};

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	// The 'recovery' field used to be 1024 bytes.  It has only ever
	// been used to store the recovery command line, so 768 bytes
	// should be plenty.  We carve off the last 256 bytes to store the
	// stage string (for multistage packages) and possible future
	// expansion.
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
};

#define AVB_MAX_PART  (8)

struct avb_list avb_partition[AVB_MAX_PART] = {
	{"vbmeta", 1},
	{"oem", 2},
	{"boot", 3},
	{"vbmeta_system", 4},
	{"dtb", 5},
	{"dtbo", 6},
	{"init_boot", 7},
	{"vendor_boot", 8}
};

#define MODE_MAX_PART  (8)
struct error_list error_mode[MODE_MAX_PART] = {
	{"apexd-failed", "E2001"},
	{"boringssl-self-check-failed", "E2002"},
	{"bpfloader-failed", "E2003"},
	{"vold-failed", "E2004"},
	{"dm-verity device corrupted", "E2005"},
	{"Checkpoint commit failed", "E2006"},
	{"kernel_panic", "E2007"},
	{"rescueparty", "E2008"}
};

#define MISC_MAX_PART  (5)
struct error_list error_misc[MISC_MAX_PART] = {
	{"fs_mgr_mount_all", "E3001"},
	{"enablefilecrypto", "E3002"},
	{"init_user0_failed", "E3003"},
	{"set_policy_failed", "E3004"},
	{"RescueParty", "E3005"}
};

#define AVB_MAX_ERROR  (17)
struct error_list error_avb[AVB_MAX_ERROR] = {
	{"avb2 verify failed", "E1---"},
	{"OOM failed", "E11--"},
	{"IO failed", "E12--"},
	{"VERIFICATION failed", "E13--"},
	{"ROLLBACK_INDEX failed", "E14--"},
	{"PUBLIC_KEY_REJECTED failed", "E15--"},
	{"INVALID_METADATA failed", "E16--"},
	{"UNSUPPORTED_VERSION failed", "E17--"},
	{"INVALID_ARGUMENT failed", "E18--"},
	{"vbmeta verify failed", "E1--1"},
	{"oem verify failed", "E1--2"},
	{"boot verify failed", "E1--3"},
	{"vbmeta_system verify failed", "E1--4"},
	{"dtb verify failed", "E1--5"},
	{"dtbo verify failed", "E1--6"},
	{"init_boot verify failed", "E1--7"},
	{"vendor_boot verify failed", "E1--8"}
};

char *remove_ab_suffix(char *str)
{
	if (!str)
		return NULL;

	size_t len = strlen(str);

	if (len >= 2) {
		if (str[len - 2] == '_' && (str[len - 1] == 'a' || str[len - 1] == 'b'))
			str[len - 2] = '\0';
	}

	return str;
}

int get_errno_by_name(char *part_name)
{
	int i = 0;

	for (i = 0; i < AVB_MAX_PART; i++) {
		if (!strcmp(part_name, avb_partition[i].part_name))
			return avb_partition[i].errno;
	}

	return 0;
}

char *get_errno_by_mode(char *mode)
{
	int i = 0;

	for (i = 0; i < MODE_MAX_PART; i++) {
		if (strstr(mode, error_mode[i].err_string))
			return error_mode[i].err_code;

		if (!strcmp(mode, error_mode[i].err_string))
			return error_mode[i].err_code;
	}

	return NULL;
}

char *get_errno_by_misc(char *mode)
{
	int i = 0;

	for (i = 0; i < MISC_MAX_PART; i++) {
		if (strstr(mode, error_misc[i].err_string))
			return error_misc[i].err_code;

		if (!strcmp(mode, error_misc[i].err_string))
			return error_misc[i].err_code;
	}

	return NULL;
}

int show_errcode(const char *err_str)
{
	int i = 0;
	int ret = 0;
	int offset_x = 100;
	int offset_y = 50;
	int offset_n = 0;
	static int show_init;
	char buf[128] = {0};
	bool unlocked;

	unlocked = is_device_unlocked();
	if (unlocked)
		return 0;

	if (show_init == 1)
		return 0;

	ret = run_command("imgread pic logo key_1 $loadaddr", 0);
	if (ret) {
		printf("bmp source not exist of logo.img, skip errcode show.\n");
		show_init = 1;
		return 0;
	}

	if (!err_str || strlen(err_str) == 0)
		return -1;

	int len = strlen(err_str);

	for (i = 0; i < len; i++) {
		sprintf(buf, "imgread pic logo key_%c $loadaddr", err_str[i]);
		run_command(buf, 0);
		offset_n = offset_x + 40 * i;
		sprintf(buf, "bmp display $key_%c_offset %d %d", err_str[i], offset_n, offset_y);
		run_command(buf, 0);
	}
	show_init = 1;
	return 0;
}

static int do_errcode_avb2(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;
	int errno = 0;
	char strerr[8] = {0};
	char partition_name[32] = {0};

	if (argc != 3)
		return cmd_usage(cmdtp);

	printf("Command: ");
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);

	printf("\n");

	strlcpy(partition_name, argv[1], strlen(argv[1]));
	remove_ab_suffix(partition_name);
	printf("partition_name:%s\n", partition_name);

	errno = get_errno_by_name(partition_name);

	sprintf(strerr, "E1%s%02d", argv[2], errno);
	printf("strerr=%s\n", strerr);
	show_errcode(strerr);
	return 0;
}

static int do_errcode_mode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;

	if (argc != 2)
		return cmd_usage(cmdtp);

	printf("Command: ");
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);

	printf("\n");

	char *mode = get_errno_by_mode(argv[1]);

	if (mode)
		show_errcode(mode);
	return 0;
}

static int do_errcode_misc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;

	if (argc != 2)
		return cmd_usage(cmdtp);

	printf("Command: ");
	for (i = 0; i < argc; i++)
		printf("%s ", argv[i]);

	printf("\n");

	char *mode = get_errno_by_misc(argv[1]);

	if (mode)
		show_errcode(mode);

	return 0;
}

static int do_errcode_show(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;

	printf("AVB2 Error List:\n");
	for (i = 0; i < AVB_MAX_ERROR; i++)
		printf("  %s : %s\n", error_avb[i].err_code, error_avb[i].err_string);

	printf("\nReboot Mode Error List:\n");
	for (i = 0; i < MODE_MAX_PART; i++)
		printf("  %s : %s\n", error_mode[i].err_code, error_mode[i].err_string);

	printf("\nMisc Reason Error List:\n");
	for (i = 0; i < MISC_MAX_PART; i++)
		printf("  %s : %s\n", error_misc[i].err_code, error_misc[i].err_string);
	printf("\n");

	return 0;
}

static cmd_tbl_t cmd_errcode_sub[] = {
	U_BOOT_CMD_MKENT(avb2, 2, 0, do_errcode_avb2, "", ""),
	U_BOOT_CMD_MKENT(mode,    1, 0, do_errcode_mode, "", ""),
	U_BOOT_CMD_MKENT(misc,    1, 0, do_errcode_misc, "", ""),
	U_BOOT_CMD_MKENT(show,    0, 0, do_errcode_show, "", ""),
};

static int do_RunErrCommand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;
	int ret = 0;

	/* Strip off leading 'errcode' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_errcode_sub[0], ARRAY_SIZE(cmd_errcode_sub));

	if (c)
		ret = c->cmd(cmdtp, flag, argc, argv);
	else
		cmd_usage(cmdtp);

	return ret;
}

U_BOOT_CMD
(errcode, 4, 0, do_RunErrCommand,
	"errcode",
	"\nThis command will show and save errcode to misc partition\n"
	"Command format:\n"
	"errcode avb2 <part_name> <avb2_errcode>  --- show avb2 errcode.\n"
	"errcode mode <misc_bcb_reserved_reason>  --- show reboot mode errcode.\n"
	"errcode misc <misc_bcb_recovery_reason>  --- show misc bcb errcode.\n"
	"errcode show                             --- show all errcode.\n"
	"will show errcode E1402;\n"   // command
	"So you can execute command: errcode avb2 boot 4"
);

