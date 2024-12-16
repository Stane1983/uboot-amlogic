// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <linux/kernel.h>
#include <amlogic/aml_efuse.h>
#include <amlogic/cpu_id.h>
#include <amlogic/storage.h>
#include <amlogic/partition_table.h>
#include <fastboot.h>
#include <amlogic/emmc_partitions.h>
#include <asm/amlogic/arch/efuse.h>
#include <android_image.h>
#include <amlogic/android_vab.h>
#include <amlogic/aml_rollback.h>
#include <cli.h>
#include <amlogic/store_wrapper.h>


#if defined(CONFIG_EFUSE_OBJ_API) && defined(CONFIG_CMD_EFUSE)
extern efuse_obj_field_t efuse_field;
#endif//#ifdef CONFIG_EFUSE_OBJ_API

int __attribute__((weak)) store_logic_read(const char *name, loff_t off, size_t size, void *buf)
{ return store_read(name, off, size, buf);}

#define debugP(fmt...) //printf("[DbgBootSta]L%d:", __LINE__),printf(fmt)
#define errorP(fmt...) printf("ErrBootSta(L%d):", __LINE__),printf(fmt)
#define wrnP(fmt...)   printf("wrn:"fmt)
#define MsgP(fmt...)   printf("[BootSta]"fmt)

#define BOOTLOADER_OFFSET		512
#define BOOTLOADER_MAX_SIZE		(4 * 1024 * 1024)
#define MIB_SIZE                (1024 * 1024)


//check SWPL-31296 for details
static int do_get_bootloader_status(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	bool printStat = false;
	bool saveenv   = false;
	debugP("Initial value for argc=%d\n", argc);
	while (argc > 1 && **(argv + 1) == '-') {
		char *arg = *++argv;

		--argc;
		while (*++arg) {
			switch (*arg) {
				case 'p':		/* print */
					printStat = true;
					break;
				case 's':		/* saveenv */
					saveenv = true;
					break;
				default:
					return CMD_RET_USAGE;
			}
		}
	}
	debugP("Final value for argc=%d\n", argc);
	//1,forUpgrade_socType, cpu familyId
	const cpu_id_t cpuid = get_cpu_id();
	const int familyId	 = cpuid.family_id;
	env_set_hex("forUpgrade_socType", familyId);

	//2,forUpgrade_secureBoot
	const bool secureboot = IS_FEAT_BOOT_VERIFY();
	env_set("forUpgrade_secureBoot", secureboot ? "true" : "false");

	//3,forUpgrade_robustOta
	bool supportRobustOta = false;
	switch (familyId) {
		case 0x32://sc2
		case 0x36://t7
		case 0x37://s4
		case 0x38://t3
		case 0x3A://s4d
			supportRobustOta = true;
			break;
		default:
		{
			if (familyId > 0x3A) {
				supportRobustOta = true;
			}
		}
		break;
	}
	env_set("forUpgrade_robustOta", supportRobustOta ? "true" : "false");

	//4,forUpgrade_flashType
	const char* BootDevices[] = {
		"BOOT_EMMC",		"BOOT_SD",
		"BOOT_NAND_NFTL",	"BOOT_NAND_MTD",
		"BOOT_SNAND",		"BOOT_SNOR",
	};
	const char* bootDevice = "BOOT_NONE";
	enum boot_type_e  bootType = store_get_type();
	int i = 0;
	for (; i < ARRAY_SIZE(BootDevices); ++i) {
		if ((1<<i) != bootType) continue;
		bootDevice = BootDevices[i];
		break;
	}
	env_set("forUpgrade_flashType", bootDevice);

	//5,forUpgrade_bootloaderCopies, how many copies supported
	int bootCopies = 1;
	switch (bootType) {
		case BOOT_EMMC: bootCopies = 3; break;
		default:break;
	}
	env_set_ulong("forUpgrade_bootloaderCopies", bootCopies);

	//6,forUpgrade_bootloaderIndex
	//for emmc, 0/1/2 is user/boot0/boot1
	const int bootCpyIndex = store_bootup_bootidx("bootloader");
	env_set_ulong("forUpgrade_bootloaderIndex", bootCpyIndex);

	//7,get first boot index, for defendkey
	const int firstBootCpyIndex = store_boot_copy_start();
	env_set_ulong("forUpgrade_1stBootIndex", firstBootCpyIndex);

	if (printStat) run_command("printenv forUpgrade_socType forUpgrade_secureBoot "
			" forUpgrade_robustOta forUpgrade_flashType forUpgrade_bootloaderCopies "
			" forUpgrade_bootloaderIndex forUpgrade_1stBootIndex", 0);

	//8,production mode for ver4 boot.img bootconfig
	env_set_ulong("production_mode", CONFIG_IS_ENABLED(AML_PRODUCT_MODE));
	if (printStat)
		run_command("printenv production_mode", 0);

	if (saveenv) {
		if (CONFIG_IS_ENABLED(AML_UPDATE_ENV))
			run_command("update_env_part -p forUpgrade_robustOta "\
					"forUpgrade_bootloaderIndex", 0);
		else
			run_command("saveenv", 0);
	}


	return CMD_RET_SUCCESS;
}

static void run_recovery_from_flash(void) {
	env_set("dolby_status","0");
	run_command("run init_display", 0);
	run_command("run storeargs", 0);
	run_command("get_rebootmode", 0);
	run_command("if test ${reboot_mode} = quiescent; then "\
		"setenv bootconfig ${bootconfig} androidboot.quiescent=1; fi;", 0);
	run_command("if test ${reboot_mode} = recovery_quiescent; then "\
		"setenv bootconfig ${bootconfig} androidboot.quiescent=1; fi;", 0);
	run_command("run recovery_from_flash", 0);
}

int write_bootloader_back(const char* bootloaderindex, int dstindex) {
	int iRet = 0;
	int copy = 0;
	int ret = -1;
	unsigned char* buffer = NULL;
	int capacity_boot = 0;

	if (strcmp(bootloaderindex, "1") == 0) {
		copy = 1;
	} else if (strcmp(bootloaderindex, "2") == 0) {
		copy = 2;
	} else if (strcmp(bootloaderindex, "0") == 0) {
		copy = 0;
	}

#ifdef CONFIG_MMC_MESON_GX
	struct mmc *mmc = NULL;

	if (store_get_type() == BOOT_EMMC)
		mmc = find_mmc_device(1);

	if (mmc)
		capacity_boot = mmc->capacity_boot;
#endif

	printf("write_bootloader_back_capacity_boot: %x\n", capacity_boot);

	buffer = (unsigned char *)malloc(capacity_boot);
	if (!buffer)
	{
		printf("ERROR! fail to allocate memory ...\n");
		goto exit;
	}
	memset(buffer, 0, capacity_boot);
	iRet = store_boot_read("bootloader", copy, 0, buffer);
	if (iRet) {
		errorP("Fail read bootloader from rsv with sz\n");
		goto exit;
	}
	iRet = store_boot_write("bootloader", dstindex, 0, buffer);
	if (iRet) {
		printf("Failed to write bootloader\n");
		goto exit;
	} else {
		ret = 0;
	}

exit:
	if (buffer)
	{
		free(buffer);
		//buffer = NULL;
	}
	return ret;
}

//bootloader write protect
static void bootloader_wp(void)
{
#ifdef CONFIG_MMC_MESON_GX
	if (store_get_type() == BOOT_EMMC) {//emmc device
		if (IS_FEAT_BOOT_VERIFY()) { //secure boot enable
			if (BOOTLOADER_MODE_ADVANCE_INIT) { //new arch chip
				env_set("bootloader_wp", "1");
			}
		}
	}
#endif
}

static void aml_recovery(void) {
	char *mode = NULL;
	char command[32];
	char miscbuf[4096] = {0};

	run_command("get_rebootmode", 0);

	//get reboot_mode
	mode = env_get("reboot_mode");
	if (mode == NULL) {
		wrnP("can not get reboot mode, so skip recovery check\n");
	} else {
		if ((!strcmp(mode, "factory_reset")) || (!strcmp(mode, "update"))) {
			env_set("dolby_status","0");
		}
	}

#ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK
	int ret = 0;
	extern int boot_info_open_partition(char *miscbuf);
	ret = boot_info_open_partition(miscbuf);
	if (ret != 0) {
		wrnP("open misc partition failed, so skip recovery check");
		return;
	}
#endif

	//if run recovery, need disable dolby
	memcpy(command, miscbuf, 32);
	if (!memcmp(command, "boot-recovery", strlen("boot-recovery"))) {
		env_set("dolby_status","0");
		return;
	}
}

void update_rollback(void)
{
	char *slot = NULL;
	int ret = -1;
	int gpt_flag = -1;

#ifdef CONFIG_MMC_MESON_GX
	struct mmc *mmc = NULL;

	if (store_get_type() == BOOT_EMMC)
		mmc = find_mmc_device(1);

	if (mmc)
		gpt_flag = aml_gpt_valid(mmc);
#endif
	if (gpt_flag == 0)
		ret = 0;

#if defined(CONFIG_EFUSE_OBJ_API) && defined(CONFIG_CMD_EFUSE)
	run_command("efuse_obj get FEAT_DISABLE_EMMC_USER", 0);

	//dis_user_flag = run_command("efuse_obj get FEAT_DISABLE_EMMC_USER", 0);
	if (*efuse_field.data == 1)
		ret = 0;
#endif//#ifdef CONFIG_EFUSE_OBJ_API

	slot = env_get("slot-suffixes");
	if (!slot) {
		run_command("get_valid_slot", 0);
		slot = env_get("slot-suffixes");
	}
	if (strcmp(slot, "0") == 0) {
		if (ret != 0) {
			wrnP("normal mode\n");
			write_bootloader_back("2", 0);
			env_set("expect_index", "0");
		} else {
			wrnP("gpt or disable user bootloader mode\n");
			write_bootloader_back("2", 1);
			env_set("expect_index", "1");
		}
		wrnP("back to slot b\n");
		run_command("set_roll_flag 1", 0);
		run_command("set_active_slot b", 0);
	} else if (strcmp(slot, "1") == 0) {
		if (ret != 0) {
			wrnP("normal mode\n");
			write_bootloader_back("1", 0);
			env_set("expect_index", "0");
		} else {
			wrnP("gpt or disable user bootloader mode\n");
			write_bootloader_back("2", 1);
			env_set("expect_index", "1");
		}
		wrnP("back to slot a\n");
		run_command("set_roll_flag 1", 0);
		run_command("set_active_slot a", 0);
	}
	env_set("update_env", "1");
	env_set("reboot_status", "reboot_next");
}

static int write_bootloader(int i)
{
	unsigned char *buffer = NULL;
	int capacity_boot = 0x2000 * 512;
	int iret = 0;
	char partname[32] = {0};
	char *slot_name = NULL;

#ifdef CONFIG_MMC_MESON_GX
	struct mmc *mmc = NULL;

	if (store_get_type() == BOOT_EMMC)
		mmc = find_mmc_device(1);

	if (mmc)
		capacity_boot = mmc->capacity_boot;
#endif
	printf("capacity_boot: 0x%x\n", capacity_boot);
	buffer = (unsigned char *)malloc(capacity_boot);
	if (!buffer) {
		printf("ERROR! fail to allocate memory ...\n");
		return -1;
	}
	memset(buffer, 0, capacity_boot);

	slot_name = env_get("active_slot");
	if (slot_name && (strcmp(slot_name, "_a") == 0))
		strcpy((char *)partname, "bootloader_a");
	else if (slot_name && (strcmp(slot_name, "_b") == 0))
		strcpy((char *)partname, "bootloader_b");
	else
		strcpy((char *)partname, "bootloader_up");

	iret = store_logic_read(partname, 0, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET, buffer);
	if (iret) {
		errorP("Fail to read 0x%xB from part[%s] at offset 0\n",
					BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET, partname);
		free(buffer);
		return -1;
	}

#ifdef CONFIG_MESON_S7D
	iret = update_boot_hdr_4_s7d_reva(buffer, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET, 0);
	if (iret) {
		printf("Failed to write s7d reva boot0\n");
		free(buffer);
		return -1;
	}
#endif//#ifdef CONFIG_MESON_S7D

	iret = store_boot_write("bootloader", i, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET, buffer);
	if (iret) {
		printf("Failed to write boot0\n");
		free(buffer);
		return -1;
	}

	free(buffer);
	return 0;
}

/**
 *set merge status
*/
int set_mergestatus_cancel(struct misc_virtual_ab_message *message)
{
        char *partition = "misc";
        char vab_buf[1024] = {0};

        if (store_read((const char *)partition,
                SYSTEM_SPACE_OFFSET_IN_MISC, 1024, (unsigned char *)vab_buf) < 0) {
                printf("failed to store read %s.\n", partition);
                return -1;
        }

        memcpy(message, vab_buf, sizeof(struct misc_virtual_ab_message));
        printf("message.merge_status: %d\n", message->merge_status);
        if (message->merge_status == SNAPSHOTTED || message->merge_status == MERGING) {
                message->merge_status = CANCELLED;
                printf("set message.merge_status CANCELLED\n");
        }
        store_write((const char *)partition, SYSTEM_SPACE_OFFSET_IN_MISC, 1024, (unsigned char *)vab_buf);
        return 0;
}

// according to the value of the board,
// reset the product to set the serial
// to distinguish different products.
void set_product(void)
{
	char *board = env_get("board");
	char *str = NULL;
	char *dup_str = NULL;

	str = strstr(board, "_");
	if (!str) {
		env_set("product", board);
		return;
	}
	dup_str = strdup(str);
	if (!(strstr(dup_str, "_")))
		env_set("product", str + 1);
	else
		env_set("product", strtok(str, "_"));
	free(dup_str);
	return;
}

void fastboot_step_check(char *rebootmode, int gpt_flag)
{
	char *bootloaderindex = NULL;
	struct mmc *mmc = NULL;

	if (store_get_type() == BOOT_EMMC)
		mmc = find_mmc_device(1);

	//get bootloader index
	bootloaderindex = env_get("forUpgrade_bootloaderIndex");
	if (!bootloaderindex) {
		wrnP("can not get bootloader index, so skip fastboot step check\n");
		return;
	}

	char *fastboot_step = env_get("fastboot_step");

	if (mmc && fastboot_step && (strcmp(fastboot_step, "1") == 0)) {
		printf("reboot to new bootloader burned by fastboot\n");
		env_set("update_env", "1");
		env_set("fastboot_step", "2");
		run_command("saveenv", 0);
		if (rebootmode && (strcmp(rebootmode, "fastboot") == 0))
			run_command("reboot next,bootloader", 0);
		else
			run_command("reboot next", 0);
	}
	if (mmc && fastboot_step && (strcmp(fastboot_step, "2") == 0)) {
		struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);

		if (dev_desc && ((gpt_flag == 1 && !strcmp(bootloaderindex, "1")) ||
		   (gpt_flag == 0 && !strcmp(bootloaderindex, "0")))) {
			printf("new bootloader error, please fastboot to another one\n");
			env_set("fastboot_step", "0");
			if (CONFIG_IS_ENABLED(AML_UPDATE_ENV))
				run_command("update_env_part -p fastboot_step;", 0);
			else
				run_command("defenv_reserve;setenv fastboot_step 0;saveenv;", 0);
			cli_init();
			cli_loop();
		}
	}
}

static int update_gpt(int flag)
{
	int ret = 0;
#ifdef CONFIG_MMC_MESON_GX
	unsigned char *buffer = NULL;
	int capacity_boot = 0x2000 * 512;
	int iRet = 0;
	struct mmc *mmc = NULL;
	struct blk_desc *dev_desc;

	if (store_get_type() == BOOT_EMMC)
		mmc = find_mmc_device(1);

	if (mmc)
		capacity_boot = mmc->capacity_boot;

	printf("capacity_boot: 0x%x\n", capacity_boot);
	buffer = (unsigned char *)malloc(capacity_boot);
	if (!buffer) {
		printf("ERROR! fail to allocate memory ...\n");
		return -1;
	}
	memset(buffer, 0, capacity_boot);

	if (flag == 0) {
		iRet = store_boot_read("bootloader", 1, 0, buffer);
		if (iRet) {
			printf("Failed to read boot0\n");
			ret = -1;
			goto exit;
		}
	} else if (flag == 1 || flag == 2) {
		iRet = store_boot_read("bootloader", 0, 0, buffer);
		if (iRet) {
			printf("Failed to read bootloader\n");
			ret = -1;
			goto exit;
		}
	} else if (flag == 3) {
		printf("null ab mode\n");
		iRet = store_logic_read("bootloader_up", 0,
			BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET, buffer);
		if (iRet) {
			printf("Fail to read 0x%xB from bootloader_up\n",
				BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET);
			ret = -1;
			goto exit;
		}
	}

	if (mmc) {
		printf("try to read gpt data from bootloader.img\n");
		int erase_flag = 0;

		dev_desc = blk_get_dev("mmc", 1);
		if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
			printf("invalid mmc device\n");
			ret = -1;
			goto exit;
		}

		if (is_valid_gpt_buf(dev_desc, buffer + 0x3DFE00)) {
			printf("printf normal bootloader.img, no gpt partition table\n");
			ret = -1;
			goto exit;
		} else {
			erase_flag = check_gpt_change(dev_desc, buffer + 0x3DFE00);

			if (erase_flag == 4 && has_boot_slot == 0) {
				printf("null ab critical partition change, refused to upgrade\n");
				ret = -1;
				goto exit;
			} else if (has_boot_slot == 1 && (erase_flag == 3 || erase_flag == 4)) {
				printf("Important partition changes, refused to upgrade\n");
				ret = 1;
				goto exit;
			} else if (erase_flag == 0) {
				printf("partition doesn't change, needn't update\n");
				ret = -1;
				goto exit;
			}

			if (flag == 1 || flag == 2) {
				printf("update from dts to gpt, erase first\n");
				erase_gpt_part_table(dev_desc);
			}

			if (flag == 2) {
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
				env_set("dts_to_gpt", "1");
				run_command("update_env_part -p dts_to_gpt;", 0);
#else
				run_command("defenv_reserve;setenv dts_to_gpt 1;saveenv;", 0);
#endif
			}

			if (write_mbr_and_gpt_partitions(dev_desc, buffer + 0x3DFE00)) {
				printf("%s: writing GPT partitions failed\n", __func__);
				ret = 1;
				goto exit;
			}

			if (mmc_device_init(mmc) != 0) {
				printf(" update gpt partition table fail\n");
				ret = 2;
				goto exit;
			}
			printf("%s: writing GPT partitions ok\n", __func__);
		}
	}

	if (has_boot_slot == 1 && (flag == 1 || flag == 2)) {
		printf("update from dts to gpt, backup old bootloader\n");
		char *slot = NULL;

		slot = env_get("slot-suffixes");
		if (!slot) {
			run_command("get_valid_slot", 0);
			slot = env_get("slot-suffixes");
		}
		if (strcmp(slot, "0") == 0) {
			printf("active is a, b is old, don't need backup\n");
		} else if (strcmp(slot, "1") == 0) {
			printf("active is b, a is old, backup boot0 to boot1\n");
			iRet = write_bootloader_back("1", 2);
			if (iRet != 0) {
				printf("Failed to write boot1\n");
				ret = 3;
				goto exit;
			}
		}
		iRet = store_boot_write("bootloader", 1, 0, buffer);
		if (iRet) {
			printf("Failed to write boot0\n");
			ret = 4;
			goto exit;
		}
	}

exit:
	if (buffer)
		free(buffer);

	if (mmc)
		run_command("mmc dev 1 0;", 0);

	if (mmc && ret > 0) {
		if (ret == 1 || ret == 2 || ret == 3) {
			printf("rollback\n");
			update_rollback();
		} else if (ret == 4) {
			printf("write back boot0, rollback\n");
			write_bootloader_back("2", 1);
			update_rollback();
		}
	}
#endif
	return ret;
}

int recovery_update(void)
{
	if (IS_ENABLED(CONFIG_MMC_MESON_GX)) {
		char bufcmd[64];
		unsigned long long size = 0;
		char *recovery_need_update = NULL;
		struct partitions *partition = NULL;
		//"ANDROID!"
		const unsigned char imghead[] = { 0x41, 0x4e, 0x44, 0x52, 0x4f, 0x49, 0x44, 0x21 };

		if (store_get_type() != BOOT_EMMC)
			return 0;

		//check recoverybak partition exist
		partition = find_mmc_partition_by_name("recoverybak");
		if (!partition) {
			printf("can not find recoverybak, skip\n");
			return 0;
		}

		//check if need update recovery from recoverybak
		recovery_need_update = env_get("recovery_need_update");
		if (!recovery_need_update || strcmp(recovery_need_update, "1"))
			return 0;

		//read recoverybak header data for check
		sprintf(bufcmd, "store read $loadaddr recoverybak 0 0x%x", 16);
		run_command(bufcmd, 0);

		unsigned char *loadaddr = NULL;

		loadaddr = (unsigned char *)simple_strtoul(env_get("loadaddr"), NULL, 16);

		//check recoverybak head is start "ANDROID!"
		if (memcmp((void *)imghead, loadaddr, 8)) {
			printf("not valid recovery image, skip update recovery\n");
			return 0;
		}

		//write recoverybak data to recovery
		while (size < partition->size) {
			unsigned long long write_size = 0;
			unsigned long long left_size = partition->size - size;

			if (left_size > 10 * MIB_SIZE)
				write_size = 10 * MIB_SIZE;
			else
				write_size = partition->size - size;

			sprintf(bufcmd, "store read $loadaddr recoverybak 0x%llx 0x%llx",
				size, write_size);
			run_command(bufcmd, 0);
			sprintf(bufcmd, "store write $loadaddr recovery 0x%llx 0x%llx",
				size, write_size);
			run_command(bufcmd, 0);
			size += write_size;
		}

		//clean recovery_need_update to 0
		env_set("recovery_need_update", "0");
		if (CONFIG_IS_ENABLED(AML_UPDATE_ENV))
			run_command("update_env_part -p recovery_need_update;", 0);
		else
			run_command("saveenv", 0);
	}
	return 0;
}

//add non ab recovery update flow here
int non_ab_update(char *rebootmode)
{
	int match_flag = 0;
	char *rebootstatus = NULL;
	char *checkresult = NULL;
	char *bootloaderindex = NULL;
	char *expect_index = NULL;
	char *robustota = NULL;

	char *write_boot = env_get("write_boot");

	if (write_boot && (!strcmp(write_boot, "1"))) {
		printf("non ab for kernel 5.15 update bootloader\n");
		write_bootloader(1);
		write_bootloader(2);
		env_set("write_boot", "0");
		env_set("upgrade_step", "1");
		run_command("saveenv", 0);
		if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
			run_command("reboot quiescent", 0);
		else
			run_command("reboot", 0);
	}

	recovery_update();

	//check_result init
	checkresult = env_get("check_result");
	if (!checkresult)
		env_set("check_result", "succ");

	//reboot_status init
	rebootstatus = env_get("reboot_status");
	if (!rebootstatus) {
		env_set("reboot_status", "reboot_init");
		rebootstatus = env_get("reboot_status");
	}

	if (!rebootstatus) {
		printf("rebootstatus is NULL, skip check\n");
		return -1;
	}

	//check reboot_end
	if (!strcmp(rebootstatus, "reboot_end")) {
		env_set("reboot_status", "reboot_init");
		run_command("saveenv", 0);
	}

	//get forUpgrade_robustOta and check if support robustota
	robustota = env_get("forUpgrade_robustOta");
	if (!robustota || !strcmp(robustota, "false"))
		return -1;

	//no secure check need
	if (!strcmp(rebootstatus, "reboot_init")) {
		printf("rebootstatus is reboot_init, skip check\n");
		return -1;
	}

	//get bootloader index
	bootloaderindex = env_get("forUpgrade_bootloaderIndex");
	if (!bootloaderindex) {
		wrnP("can not get bootloader index, so skip secure check\n");
		return -1;
	}

	//get expect index
	expect_index = env_get("expect_index");
	if (!expect_index) {
		wrnP("can not get expect index, so skip secure check\n");
		return -1;
	}

	wrnP("expect_index is : %s\n", expect_index);

	match_flag = strcmp(bootloaderindex, expect_index);

	//ignore reboot next check if power off during reboot next to finish
	if (rebootmode && (strcmp(rebootmode, "cold_boot") == 0)) {
		if (!strcmp(rebootstatus, "reboot_finish"))
			match_flag = 0;
	}

	if (!strcmp(rebootstatus, "reboot_next")) {
		env_set("reboot_status", "reboot_finish");
		run_command("saveenv", 0);
		run_command("get_rebootmode", 0);
		if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
			run_command("reboot next,quiescent", 0);
		else
			run_command("reboot next", 0);
	} else if (!strcmp(rebootstatus, "reboot_finish")) {
		wrnP("--secure check reboot_finish---\n");
		env_set("reboot_status", "reboot_end");
		run_command("saveenv", 0);
		if (match_flag == 0) {
			wrnP("reboot next succ, bootloader secure check pass......\n");
		} else {
			//bootloader check failed, run recovery show error
			wrnP("reboot next fail, bootloader secure check fail(curr:%s, expect:%s)\n",
			     bootloaderindex, expect_index);
			env_set("check_result", "bootloader_fail");
			run_command("saveenv", 0);
		}
		run_recovery_from_flash();
		return 0;
	} else {
		env_set("check_result", "succ");
	}
	return 0;
}

//add ab update and ab rollback flow here
int ab_update_rollback(char *rebootmode, int gpt_flag)
{
	int match_flag = 0;
	char *rebootstatus = NULL;
	char *bootloaderindex = NULL;
	char *expect_index = NULL;

	//update bootloader_a/b/up to boot0
	char *write_boot = env_get("write_boot");

	if (write_boot && !strcmp(write_boot, "1")) {
		printf("need to write boot0\n");
		if (write_bootloader(1)) {
			printf("write boot0 fail, need to rollback!\n");
			update_rollback();
		} else {
			update_gpt(0);
			printf("write boot0 success, need to reset!\n");
		}

		env_set("write_boot", "0");
		env_set("reboot_status", "reboot_next");
		env_set("expect_index", "1");
		env_set("update_env", "1");
		run_command("saveenv", 0);
		if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
			run_command("reboot quiescent", 0);
		else
			run_command("reboot", 0);
	}

	//check rescueparty and rollback
	if (rebootmode && (strcmp(rebootmode, "rescueparty") == 0)) {
		printf("rebootmode is rescueparty, need rollback\n");
		char *slot;

#ifdef CONFIG_FASTBOOT
		struct misc_virtual_ab_message message;
		set_mergestatus_cancel(&message);
#endif

		slot = env_get("slot-suffixes");
		if (!slot) {
			run_command("get_valid_slot", 0);
			slot = env_get("slot-suffixes");
		}
		if (strcmp(slot, "0") == 0) {
			if (gpt_flag == 0) {
				wrnP("normal mode\n");
				write_bootloader_back("2", 0);
				env_set("expect_index", "0");
			} else {
				wrnP("gpt or disable user bootloader mode\n");
				write_bootloader_back("2", 1);
				env_set("expect_index", "1");
			}
			wrnP("back to slot b\n");
			run_command("set_roll_flag 1", 0);
			run_command("set_active_slot b", 0);
		} else if (strcmp(slot, "1") == 0) {
			if (gpt_flag == 0) {
				wrnP("normal mode\n");
				write_bootloader_back("1", 0);
				env_set("expect_index", "0");
			} else {
				wrnP("gpt or disable user bootloader mode\n");
				write_bootloader_back("2", 1);
				env_set("expect_index", "1");
			}
			wrnP("back to slot a\n");
			run_command("set_roll_flag 1", 0);
			run_command("set_active_slot a", 0);
		}

		env_set("update_env", "1");
		env_set("reboot_status", "reboot_next");
		run_command("saveenv", 0);
		if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
			run_command("reboot quiescent", 0);
		else
			run_command("reboot", 0);
	}

	//get bootloader index
	bootloaderindex = env_get("forUpgrade_bootloaderIndex");
	if (bootloaderindex == NULL) {
		wrnP("can not get bootloader index, so skip secure check\n");
		return -1;
	}

	//get expect index
	expect_index = env_get("expect_index");
	if (expect_index == NULL) {
		wrnP("can not get expect index, so skip secure check\n");
		return -1;
	}

	wrnP("expect_index is : %s\n", expect_index);

	match_flag = strcmp(bootloaderindex, expect_index);

	rebootstatus = env_get("reboot_status");

	//if reboot next need check bootloader rollback
	if (rebootstatus && !strcmp(rebootstatus, "reboot_next")) {
		wrnP("--secure check reboot_next---\n");
		//bootloader index, expect == current, no need reboot next
		if (match_flag != 0) {
			wrnP("bootloader wrong, start rollback\n");
			if (gpt_flag == 1) {
				wrnP("gpt or disable user bootloader mode, write boot1 to boot0\n");
				write_bootloader_back("2", 1);
#ifdef CONFIG_FASTBOOT
				struct misc_virtual_ab_message message;

				set_mergestatus_cancel(&message);
#endif
				char *slot;

				slot = env_get("slot-suffixes");
				if (!slot) {
					run_command("get_valid_slot", 0);
					slot = env_get("slot-suffixes");
				}
				if (strcmp(slot, "0") == 0) {
					wrnP("back to slot b\n");
					run_command("set_roll_flag 1", 0);
					run_command("set_active_slot b", 0);
				} else if (strcmp(slot, "1") == 0) {
					wrnP("back to slot a\n");
					run_command("set_roll_flag 1", 0);
					run_command("set_active_slot a", 0);
				}
				env_set("expect_index", "1");
			} else {
				write_bootloader_back(bootloaderindex, 0);
#ifdef CONFIG_FASTBOOT
				struct misc_virtual_ab_message message;
				set_mergestatus_cancel(&message);
#endif
				if (strcmp(bootloaderindex, "1") == 0) {
					wrnP("back to slot a\n");
					run_command("set_roll_flag 1", 0);
					run_command("set_active_slot a", 0);
				} else if (strcmp(bootloaderindex, "2") == 0) {
					wrnP("back to slot b\n");
					run_command("set_roll_flag 1", 0);
					run_command("set_active_slot b", 0);
				}
				env_set("expect_index", "0");
			}

			env_set("update_env", "1");
			env_set("reboot_status", "reboot_next");
			run_command("saveenv", 0);
			if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
				run_command("reboot quiescent", 0);
			else
				run_command("reboot", 0);
			return 0;
		}else {
			wrnP("current index is expect, no need reboot next, set reboot_status reboot_init\n");
			env_set("reboot_status", "reboot_init");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p reboot_status;", 0);
#else
			run_command("saveenv", 0);
#endif
		}
	}

	return 0;
}

static int do_secureboot_check(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *rebootmode = NULL;
	int gpt_flag = 0;
	int ret = -1;

	// set product first to set serial.
	set_product();

#ifdef CONFIG_MMC_MESON_GX
	struct mmc *mmc = NULL;
	//int capacity_boot = 0;

	if (store_get_type() == BOOT_EMMC) {
		mmc = find_mmc_device(1);
		if (mmc)
			ret = aml_gpt_valid(mmc);
	}
#endif

	//check is gpt mode
	if (ret == 0)
		gpt_flag = 1;

	//unsupport update dt in boothal, update dt in uboot
	run_command("update_dt;", 0);

	bootloader_wp();

	//set default env before check
	env_set("recovery_mode", "false");

	//if recovery mode, need disable dv, if factoryreset, need default uboot env
	aml_recovery();

#if defined(CONFIG_EFUSE_OBJ_API) && defined(CONFIG_CMD_EFUSE)
	run_command("efuse_obj get FEAT_DISABLE_EMMC_USER", 0);

	if (*efuse_field.data == 1) {
		wrnP("efuse_field.data == 1\n");
		env_set("nocs_mode", "true");
		gpt_flag = 1;
	} else {
		wrnP("efuse_field.data != 1\n");
		env_set("nocs_mode", "false");
	}
#endif//#ifdef CONFIG_EFUSE_OBJ_API


	run_command("get_rebootmode", 0);
	rebootmode = env_get("reboot_mode");
	if (!rebootmode) {
		wrnP("can not get reboot mode, so skip secure check\n");
		return -1;
	}
	printf("rebootmode is %s\n", rebootmode);

	//get boot status
	run_command("amlbootsta -p -s", 0);

	if (IS_ENABLED(CONFIG_MMC_MESON_GX))
		fastboot_step_check(rebootmode, gpt_flag);

	if (has_boot_slot == 1)
		ab_update_rollback(rebootmode, gpt_flag);
	else
		non_ab_update(rebootmode);

	return 0;
}


static int do_update_uboot_env(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const int write_boot = env_get_ulong("write_boot", 10, 0);
	const int update_dts_gpt = env_get_ulong("update_dts_gpt", 10, 0);

	//skip update env if need update bootloader
	if (write_boot || update_dts_gpt) {
		run_command("echo write_boot ${write_boot}, update_dts_gpt ${update_dts_gpt}", 0);
		return 0;
	}

	//default uboot env need before anyone use it
	//after factory reset
	if (env_get("default_env")) {
		printf("factory reset, need default all uboot env.\n");
		run_command("defenv_reserv; setenv upgrade_step 2; saveenv;", 0);
	}

	//after ab update
	char *update_env = env_get("update_env");

	if (update_env && (strcmp(update_env, "1") == 0)) {
		printf("ab mode, default all uboot env\n");
		run_command("defenv_reserv; setenv upgrade_step 2; saveenv;", 0);
	}

	//after recovery update
	char *upgrade_step = env_get("upgrade_step");

	if (upgrade_step && (strcmp(upgrade_step, "1") == 0)) {
		printf("after recovery update, need update uboot env\n");
		run_command("defenv_reserv; setenv upgrade_step 2; saveenv;", 0);
	}

	return 0;
}

U_BOOT_CMD_COMPLETE(
	amlbootsta, 3, 0,	do_get_bootloader_status,
	"get bootloader status in env",
	"[-p] print bootloader status\n"
	"[-s] saveenv after generate bootloader status\n",
	var_complete
);

U_BOOT_CMD_COMPLETE(
	amlsecurecheck, 1, 0,	do_secureboot_check,
	"try bootloader/dtb/recovery secure check",
	""
	"",
	var_complete
);

U_BOOT_CMD(aml_update_env,	1, 0, do_update_uboot_env,
	   "aml_update_env",
	"\nThis command will update uboot env\n"
	"So you can execute command: aml_update_env"
);


