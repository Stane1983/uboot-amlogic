// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <errno.h>
#include <cli.h>
#include <exports.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <abuf.h>
#include <version.h>
#include <amlogic/aml_v3_burning.h>
#include <amlogic/aml_v2_burning.h>
#include <command.h>
#include <amlogic/board.h>
#include <amlogic/cpu_id.h>
#include <asm-generic/u-boot.h>
#include <amlogic/aml_profile.h>
#ifdef CONFIG_AML_CVBS
#include <amlogic/media/vout/aml_cvbs.h>
#endif
#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/lcd_vout.h>
#endif
#ifdef CONFIG_RX_RTERM
#include <amlogic/aml_hdmirx.h>
#endif
#ifdef CONFIG_AML_VOUT
#include <amlogic/media/vout/aml_vout.h>
#endif
#ifdef CONFIG_ARMV8_MULTIENTRY
#include <asm/arch-meson/smp.h>
#endif
#ifdef CONFIG_AMLOGIC_AMFC
#include <amlogic/amfc.h>
#endif

#ifdef CONFIG_DDR_FASTBOOT
#include <amlogic/aml_mmc.h>
#endif

#include <amlogic/aml_efuse.h>
#include <amlogic/storage.h>
#include <asm/amlogic/arch/efuse.h>
#include <amlogic/emmc_partitions.h>

#ifdef CONFIG_FACTORY_BURN_MODE
extern efuse_obj_field_t efuse_field;
#include <hexdump.h>
#define MAX_LINE_LENGTH 256
#define MAX_COMMAND_LENGTH 256
#define HASH_LENGTH 64
#define MAX_KEY_LENGTH 64
char command[MAX_COMMAND_LENGTH];
#endif

DECLARE_GLOBAL_DATA_PTR;
#define UNUSED(x) (void)(x)

#ifdef CONFIG_MISC_INIT_R
#define _AML_MISC_INTERRUPT_KEY 0x09
static int _aml_interrupt_key_pressed;
static int ctrli(void)
{
	if (1/*gd->have_console*/) {
		if (tstc()) {
			switch (getchar()) {
			case _AML_MISC_INTERRUPT_KEY:/* ^I - Control I */
				_aml_interrupt_key_pressed = 1;
				printf("Detect Ctrl I...\t\nInput yes to force stopped anyway:\t");
				return 1;
			default:
				break;
			}
		}
	}

	return 0;
}

static int aml_misc_confirm_yesno(const int tm/*timeout in ms*/)
{
	int i;
	unsigned long ts;
	char str_input[5];

	_aml_interrupt_key_pressed = 0;
	if (!ctrli()) {//not detect ctrl-I when bootup
		return 0;
	}
	ts = get_timer(0);

	for (i = 0; i < sizeof(str_input);) {
		int c = 0;

		while (!tstc()) {
			if (get_timer(ts) >= tm) {
				printf("Input timeout\n");
				return 0;
			}
		}
		c = getchar();
		if (i == 0 && c == _AML_MISC_INTERRUPT_KEY) {//drop first duplicated ctrlI
			printf("Wait YES/y input\n");
			continue;
		}
		putc(c);
		str_input[i++] = c;
		if (c == '\r')
			break;
	}
	putc('\n');
	if (strncmp(str_input, "y\r", 2) == 0 ||
	    strncmp(str_input, "Y\r", 2) == 0 ||
	    strncmp(str_input, "yes\r", 4) == 0 ||
	    strncmp(str_input, "YES\r", 4) == 0)
		return 1;
	return 0;
}

int misc_init_r(void)
{
	printf("board common misc_init\n");
#if defined(CONFIG_MDUMP_COMPRESS) || \
	((defined CONFIG_SUPPORT_BL33Z) && \
	(defined CONFIG_FULL_RAMDUMP))
	extern void ramdump_init(void);
	ramdump_init();
#endif
	if (!aml_misc_confirm_yesno(5000))
		return 0;

	cli_init();
	cli_loop();
	panic("No CLI available");
	return 0;
}
#endif // #ifdef CONFIG_MISC_INIT_R

#ifdef CONFIG_DDR_FASTBOOT
typedef struct __ddr_savedparams {
	uint8_t d1[16];
	uint32_t data[2048 / 4 - 16 / 4 - 32 / 4];
	uint8_t d2[32];
} ddr_savedparams_t;

static void ddr_save_fastboot_para(void)
{
	unsigned long addr = 0x2000000UL;

	if (!memcmp((void *)addr, "DDRPARAMS", 8)) { // match tag
		mmc_write_rsv(MMC_DDR_PARAMETER_NAME, sizeof(ddr_savedparams_t),
				(void *)(addr + 8));
		printf("Saved ddr fastboot\n");
	}
}
#endif //CONFIG_DDR_FASTBOOT

#ifdef CONFIG_BOARD_LATE_INIT
int aml_board_late_init_front(void *arg)
{
	UNUSED(arg);
	printf("init front\n");
	run_command("aml_update_env", 0);
	/* ****************************************************
	 * 1.setup bootup resource
	 * ****************************************************
	 */

#ifdef CONFIG_DDR_FASTBOOT
	ddr_save_fastboot_para();
#endif

	board_init_mem();
	run_command("run bcb_cmd", 0);

#ifndef CONFIG_SYSTEM_RTOS //prue rtos not need dtb
	PUSH_TIME_TE("load dtb", BL33_LOAD_DTB_s);
	if (run_command("run common_dtb_load", 0)) {
		printf("Fail in load dtb with cmd[%s], try _aml_dtb\n", env_get("common_dtb_load"));
		//load dtb here then users can directly use 'fdt' command
		run_command("echo reboot_mode ${reboot_mode}; "
				"imgread dtb _aml_dtb ${dtb_mem_addr}; fi;", 0);
	}
	PUSH_TIME_TE("load dtb", BL33_LOAD_DTB_e);
	run_command("if fdt addr ${dtb_mem_addr}; then else echo no dtb at ${dtb_mem_addr};fi;", 0);
	/* load unifykey */
	run_command("keyman init 0x1234", 0);
#endif//#ifndef CONFIG_SYSTEM_RTOS //prue rtos not need dtb

	return 0;
}

#if !defined(SYSCTRL_SEC_STICKY_REG2) && defined(P_PREG_STICKY_REG2)
#define SYSCTRL_SEC_STICKY_REG2 P_PREG_STICKY_REG2
#endif//#if !defined(SYSCTRL_SEC_STICKY_REG2) && defined(P_PREG_STICKY_REG2)

#ifdef CONFIG_FACTORY_BURN_MODE
int extract_hash_from_memory(const char *loadaddr, const char *hash_type, char *hash_value)
{
	const char *current = loadaddr;
	const char *line_start = loadaddr;
	char search_pattern[64];

	snprintf(search_pattern, sizeof(search_pattern), "HASH_%s_DEVICE_ROOTCERT", hash_type);

	while (*current != '\0') {
		if (*current == '\n' || *current == '\r') {
			size_t line_length = current - line_start;

			if (line_length > strlen(search_pattern) &&
				strstr(line_start, search_pattern)) {
				const char *hash_start = strstr(line_start, search_pattern);

				if (hash_start) {
					hash_start += strlen(search_pattern);

					while (*hash_start != '\0' && isspace(*hash_start))
						hash_start++;

					if (strlen(hash_start) >= HASH_LENGTH) {
						memcpy(hash_value, hash_start, HASH_LENGTH);
						hash_value[HASH_LENGTH] = '\0';
						return 1;
					}
				}
			}

			line_start = current + 1;

			while (*line_start == '\n' || *line_start == '\r')
				line_start++;
			current = line_start;
		} else {
			current++;
		}
	}

	return 0;
}

int extract_key_value(const char *loadaddr, char *value)
{
	const char *current = loadaddr;
	int id_length = 0;

	while (*current != '\0' && isspace(*current))
		current++;

	while (*current != '\0' && !isspace(*current) && id_length < MAX_KEY_LENGTH)
		value[id_length++] = *current++;

	value[id_length] = '\0';

	return (id_length == MAX_KEY_LENGTH) ? 1 : 0;
}

int aml_factory_burn(void)
{
	int reboot_flag = 0;
	char *rpmb_flag = env_get("rpmb_state");
	char *factorytest = env_get("factorytest");
	const int bootCpyIndex = store_bootup_bootidx("bootloader");
	char *rebootmode = env_get("reboot_mode");
	int ret = -1;
	unsigned char *loadaddr = 0;
	char hash_value[HASH_LENGTH + 1];

	loadaddr = (unsigned char *)simple_strtoul(env_get("loadaddr"), NULL, 16);
#ifdef CONFIG_READ_KEY_FROM_PARAM
	const int partIndex = get_partition_num_by_name("param");

	env_set_hex("paramPart", partIndex);
	env_set_hex("paramLoadAddr", (ulong)loadaddr);
#else
#ifdef CONFIG_READ_KEY_FROM_UDISK
	env_set_hex("udiskLoadAddr", (ulong)loadaddr);
#endif
#endif

	printf("bootCpyIndex is %d\n", bootCpyIndex);
	if (!rebootmode) {
		run_command("get_rebootmode", 0);
		rebootmode = env_get("reboot_mode");
	}

	if (bootCpyIndex == 2 &&
		factorytest && (strcmp(factorytest, "1") == 0)) {
		printf("bootup from boot1, copy boot1 to boot0\n");
		run_command("copy_slot_bootable 2 1", 0);
		env_set("factorytest", "0");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p factorytest;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif
		if (rebootmode && (strcmp(rebootmode, "quiescent") == 0))
			run_command("reboot quiescent", 0);
		else
			run_command("reboot", 0);
	}

	if ((!factorytest || (factorytest && strcmp(factorytest, "1"))) &&
		!IS_FEAT_BOOT_VERIFY()) {
		printf("normal mode, set factorytest 1\n");
		factorytest = "1";
		env_set("factorytest", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p factorytest;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif
	}

	if (factorytest)
		printf("factorytest is %s\n", factorytest);

	if (rebootmode && strcmp(rebootmode, "fastboot") &&
		strcmp(rebootmode, "factory_reset")) {
		if (factorytest && (strcmp(factorytest, "1") == 0) &&
			!IS_FEAT_BOOT_VERIFY() &&
			bootCpyIndex == 1) {
			printf("need to enter factory burn mode\n");
			if (rpmb_flag) {
				if (!strcmp(rpmb_flag, "0")) {
					printf("Do rpmb_init\n");
					run_command("rpmb_init", 0);
					reboot_flag = 1;
				} else {
					printf("RPMB already init\n");
				}
			} else {
				printf("Get RPMB state failed\n");
			}

			run_command("efuse_obj get_lock HASH_NORMAL_DEVICE_ROOTCERT", 0);
			if (*efuse_field.data == 0) {
				printf("Need burn HASH_NORMAL_DEVICE_ROOTCERT\n");
#ifdef CONFIG_READ_KEY_FROM_PARAM
				env_set("ext4paramLoadCmd",
						"ext4load mmc 1:${paramPart} ${paramLoadAddr} /pattern.efuse");
				ret = run_command("run ext4paramLoadCmd", 0);
#else
#ifdef CONFIG_READ_KEY_FROM_UDISK
				env_set("udiskparamLoadCmd",
						"fatload usb 0 ${udiskLoadAddr} pattern.efuse");
				ret = run_command("run udiskparamLoadCmd", 0);
#endif
#endif

				if (extract_hash_from_memory(loadaddr, "NORMAL", hash_value)) {
					snprintf(command, MAX_COMMAND_LENGTH,
						"efuse_obj set HASH_NORMAL_DEVICE_ROOTCERT %s",
						hash_value);
					printf("command is %s\n", command);
					ret = run_command(command, 0);
					if (ret == 0)
						ret = run_command("efuse_obj lock HASH_NORMAL_DEVICE_ROOTCERT", 0);
					else
						return -1;
				} else {
					printf("read HASH_NORMAL_DEVICE_ROOTCERT error\n");
					return -1;
				}
			}

			run_command("efuse_obj get_lock HASH_DFU_DEVICE_ROOTCERT", 0);
			if (*efuse_field.data == 0) {
				printf("Need burn HASH_DFU_DEVICE_ROOTCERT\n");
#ifdef CONFIG_READ_KEY_FROM_PARAM
				env_set("ext4paramLoadCmd",
						"ext4load mmc 1:${paramPart} ${paramLoadAddr} /pattern.efuse");
				ret = run_command("run ext4paramLoadCmd", 0);
#else
#ifdef CONFIG_READ_KEY_FROM_UDISK
				env_set("udiskparamLoadCmd",
						"fatload usb 0 ${udiskLoadAddr} pattern.efuse");
				ret = run_command("run udiskparamLoadCmd", 0);
#endif
#endif

				if (extract_hash_from_memory(loadaddr, "DFU", hash_value)) {
					snprintf(command, MAX_COMMAND_LENGTH,
						"efuse_obj set HASH_NORMAL_DEVICE_ROOTCERT %s",
						hash_value);
					printf("command is %s\n", command);
					ret = run_command(command, 0);
					if (ret == 0)
						ret = run_command("efuse_obj lock HASH_DFU_DEVICE_ROOTCERT", 0);
					else
						return -1;
				} else {
					printf("read HASH_NORMAL_DEVICE_ROOTCERT error\n");
					return -1;
				}
			}

			ret = run_command("ampk regen_enc_key", 0);
			printf("ampk regen_enc_key ret = %d\n", ret);

			run_command("efuse_obj get_lock AUDIO_VENDOR_ID", 0);
			if (*efuse_field.data == 0) {
				printf("Need burn AUDIO_VENDOR_ID\n");
				char audio_id[MAX_KEY_LENGTH + 1];
#ifdef CONFIG_READ_KEY_FROM_PARAM
				env_set("ext4paramLoadCmd",
						"ext4load mmc 1:${paramPart} ${paramLoadAddr} /eaudio_id.txt");
				ret = run_command("run ext4paramLoadCmd", 0);
#else
#ifdef CONFIG_READ_KEY_FROM_UDISK
				env_set("udiskparamLoadCmd",
						"fatload usb 0 ${udiskLoadAddr} eaudio_id.txt");
				ret = run_command("run udiskparamLoadCmd", 0);
#endif
#endif

				if (extract_key_value(loadaddr, audio_id)) {
					snprintf(command, sizeof(command),
						"efuse_obj set_enc AUDIO_VENDOR_ID %s", audio_id);
					printf("command is %s\n", command);
					ret = run_command(command, 0);
					if (ret == 0) {
						printf("Lock AUDIO_VENDOR_ID\n");
						ret = run_command("efuse_obj lock AUDIO_VENDOR_ID", 0);
						if (ret == 0) {
							reboot_flag = 1;
						} else {
							printf("Lock AUDIO_VENDOR_ID error\n");
							return -1;
						}
					} else {
						printf("burn AUDIO_VENDOR_ID error\n");
						return -1;
					}
				} else {
						printf("read AUDIO_VENDOR_ID error\n");
						return -1;
				}
			} else {
				printf("AUDIO_VENDOR_ID already set\n");
			}

			run_command("efuse_obj get_lock DGPK1", 0);
			if (*efuse_field.data == 0) {
				printf("Need burn DGPK1\n");
				char dgpk_value[MAX_KEY_LENGTH + 1];
#ifdef CONFIG_READ_KEY_FROM_PARAM
				env_set("ext4paramLoadCmd",
						"ext4load mmc 1:${paramPart} ${paramLoadAddr} /edgpk1.txt");
				ret = run_command("run ext4paramLoadCmd", 0);
#else
#ifdef CONFIG_READ_KEY_FROM_UDISK
				env_set("udiskparamLoadCmd",
						"fatload usb 0 ${udiskLoadAddr} edgpk1.txt");
				ret = run_command("run udiskparamLoadCmd", 0);
#endif
#endif

				if (extract_key_value(loadaddr, dgpk_value)) {
					snprintf(command, sizeof(command),
							"efuse_obj set_enc DGPK1 %s", dgpk_value);
					printf("command is %s\n", command);
					ret = run_command(command, 0);
					if (ret == 0) {
						printf("Lock DGPK1\n");
						ret = run_command("efuse_obj lock DGPK1", 0);
						if (ret == 0) {
							reboot_flag = 1;
						} else {
							printf("Lock DGPK1 error\n");
							return -1;
						}
					} else {
						printf("burn DGPK1 error\n");
						return -1;
					}
				} else {
					printf("read DGPK1 error\n");
					return -1;
				}
			} else {
				printf("DGPK1 already set\n");
			}

			if (reboot_flag) {
				printf("Need reboot\n");
#ifdef CONFIG_FACTORY_BURN_MODE
				run_command("bcb factory", 0);
#else
#ifdef CONFIG_DIAG_MODE
				run_command("reboot", 0);
#endif
#endif
			}
		}
		return 0;
	} else {
		return -1;
	}
}

int hexstr_to_bin(const char *hexstr, u_char *bin, int bin_len)
{
	int i, len = strlen(hexstr);
	int bytes = len / 2;

	if (bytes > bin_len)
		return -1;

	for (i = 0; i < bytes; i++) {
		char high = hexstr[2 * i];
		char low = hexstr[2 * i + 1];

		bin[i] = (hex_to_bin(high) << 4) | hex_to_bin(low);
	}

	return bytes;
}

int ascii_hex_to_bin(u_char *ascii_hex, u_char *bin, int max_len, int bin_len)
{
	if (bin_len > max_len) {
		printf("Error: Binary buffer too small for data length\n");
		return -1;
	}

	for (int i = 0; i < bin_len; i++) {
		char high = ascii_hex[i * 2];
		char low = ascii_hex[i * 2 + 1];

		int high_val = hex_to_bin(high);
		int low_val = hex_to_bin(low);

		if (high_val < 0 || low_val < 0) {
			printf("Error: Invalid hex character at position %d\n", i * 2);
			return -1;
		}

		bin[i] = (high_val << 4) | low_val;
	}

	return bin_len;
}

int verify_loaded_data(ulong loadaddr, const char *expected_str)
{
	u_char expected_bin[256];
	u_char loaded_bin[256];
	u_char *loaded_ascii = (u_char *)loadaddr;
	int expected_len, loaded_len;
	int i;

	expected_len = hexstr_to_bin(expected_str, expected_bin, sizeof(expected_bin));
	if (expected_len < 0) {
		printf("Invalid expected hex string\n");
		return -1;
	}

	loaded_len = ascii_hex_to_bin(loaded_ascii, loaded_bin, sizeof(loaded_bin), expected_len);
	if (loaded_len < 0) {
		printf("Failed to convert loaded ASCII hex to binary\n");
		return -1;
	}

	if (loaded_len != expected_len) {
		printf("Data length mismatch: expected %d, loaded %d\n",
			   expected_len, loaded_len);
		return -1;
	}

	if (memcmp(loaded_bin, expected_bin, expected_len) == 0) {
		printf("Data verification succeeded!\n");
		return 0;
	}

	printf("Data verification failed!\n");

	for (i = 0; i < loaded_len; i++) {
		if (loaded_bin[i] != expected_bin[i]) {
			printf("Mismatch at offset %d: loaded=0x%02x, expected=0x%02x\n",
				i, loaded_bin[i], expected_bin[i]);
			break;
		}
	}

	return -1;
}
#endif

int aml_board_late_init_tail(void *arg)
{
	unsigned char chipid[16];
	char con_str[128], env_str[128];
	int con_len = 0;

	PUSH_TIME_TE("tail init", BL33_TAIL_INIT_s);
	UNUSED(arg);
	printf("init tail\n");
	run_command("amlsecurecheck", 0);

#ifdef CONFIG_FACTORY_BURN_MODE
	char *rebootmode = env_get("reboot_mode");

	if (rebootmode && strcmp(rebootmode, "fastboot") &&
		strcmp(rebootmode, "factory_reset")) {
		int rc = 0;
#ifdef CONFIG_READ_KEY_FROM_UDISK
		ulong loadaddr;
		char cmd[128];

		loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);

		run_command("usb start 0", 0);
		sprintf(cmd, "fatload usb 0 %lx edgpk1.txt", loadaddr);
		rc = run_command(cmd, 0);

		if (rc) {
			printf("Fail in read efuse file from udisk\n");
		} else {
			printf("read efuse file from udisk ok\n");
			rc = aml_factory_burn();
			/*rc = verify_loaded_data(loadaddr, AML_DGPK1);
			if (rc == 0) {
				printf("verify data ok\n");
				rc = aml_factory_burn();
			}*/
		}
#else
#ifdef CONFIG_READ_KEY_FROM_PARAM
		if (!IS_FEAT_BOOT_VERIFY())
			rc = aml_factory_burn();
#endif
#endif
		if (rc == 0) {
			printf("enter factory_boot mode\n");
			env_set("factory_mode", "1");
			env_set("EnableSelinux", "permissive");
			run_command("run init_display", 0);
			run_command("run storeargs", 0);
			run_command("run recovery_from_flash", 0);
		}
	}
#endif

	run_command("update_tries", 0);

#ifdef CONFIG_REPLACE_TV_MODE
	char *keymatch = env_get("keymatch");
	char partnam[32] = {0};
	char *slot_name;
	int ret = 0;

	slot_name = env_get("slot-suffixes");
	if (slot_name && (strcmp(slot_name, "0") == 0))
		strcpy((char *)partnam, "odm_ext_a");
	else if (slot_name && (strcmp(slot_name, "1") == 0))
		strcpy((char *)partnam, "odm_ext_b");
	else
		strcpy((char *)partnam, "odm_ext");

	if (IS_FEAT_BOOT_VERIFY()) {
		printf("secureboot, check key match\n");
		run_command("check_avb_key", 0);
	}

	keymatch = env_get("keymatch");
	printf("keymatch is %s\n", keymatch);

	if (!IS_FEAT_BOOT_VERIFY() ||
		(keymatch && (strcmp(keymatch, "true") == 0))) {
		printf("non secureboot or key matched\n");
		ret = run_command("usb start 0", 0);
		printf("ret = %d\n", ret);

		if (ret == 0) {
			env_set("odmextPart", partnam);
			env_set("udiskodmextBurn",
					"usb_update ${odmextPart} odm_ext.img");
			ret = run_command("run udiskodmextBurn", 0);

			env_set("udiskproductionextBurn",
					"usb_update production_ext production_ext.img");
			ret = run_command("run udiskproductionextBurn", 0);
		}
	}
#endif

	//Need save outputmode/connector_type to flash if changed after display drv init
	if (env_get("outputmode"))
		con_len = sprintf(con_str, "outputmode");
	if (env_get("connector0_type"))
		con_len += sprintf(con_str + con_len, " connector0_type");
	if (env_get("connector1_type"))
		con_len += sprintf(con_str + con_len, " connector1_type");
	if (env_get("connector2_type"))
		con_len += sprintf(con_str + con_len, " connector2_type");
	if (con_len) {
		sprintf(env_str, "printenv %s", con_str);
		run_command(env_str, 0);
		sprintf(env_str, "update_env_part -p -f %s", con_str);
		run_command(env_str, 0);
	}

	memset(chipid, 0, 16);
	env_set("cpu_id", "1234567890");
	if (get_chip_id(chipid, 16) != -1) {
		char chipid_str[32];
		char chipid_env[32];
		int i, j;
		char buf_tmp[4];

		memset(chipid_str, 0, 32);
		memset(chipid_env, 0, 32);

		char *buff = &chipid_str[0];
		char *buff2 = &chipid_env[0];

		for (i = 0, j = 0; i < 12; ++i) {
			sprintf(&buf_tmp[0], "%02x", chipid[15 - i]);
			if (strcmp(buf_tmp, "00") != 0) {
				sprintf(buff + j, "%02x", chipid[15 - i]);
				j = j + 2;
			}
		}

		for (i = 0, j = 0; i < 12; ++i) {
			sprintf(buff2 + j, "%02x", chipid[15 - i]);
			j = j + 2;
		}

		env_set("cpuid_env", chipid_env);
		env_set("cpu_id", chipid_str);
		printf("buff: %s\n", buff);
	}

//auto enter usb mode after board_late_init if 'adnl.exe setvar burnsteps 0x1b8ec003'
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if (readl(SYSCTRL_SEC_STICKY_REG2) == 0x1b8ec003)
		aml_v3_factory_usb_burning(0, gd->bd);
#endif//#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
#ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE //try auto upgrade from ext-sdcard
	aml_try_factory_sdcard_burning(0, gd->bd);
#endif//#ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE
	PUSH_TIME_TE("tail init", BL33_TAIL_INIT_e);

	return 0;
}
#endif//#ifdef CONFIG_BOARD_LATE_INIT

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
#ifdef CONFIG_SAVE_UBOOT_LOG
	blx_init_uboot_log();
#endif
}
#endif

#ifdef CONFIG_SAVE_UBOOT_LOG
int uboot_log_replace_addr_with_fdt(void)
{
	int nodeoffset, err;
	char *pfdtaddr = env_get("fdtaddr");
	long fdt_addr;
	struct fdt_resource res;
	long addr, size;
	int subnode;

	char *blob;

	fdt_addr = 0x1000000;
	if (pfdtaddr)
		fdt_addr = simple_strtoul(pfdtaddr, NULL, 16);

	blob = (char *)fdt_addr;
	nodeoffset = fdt_path_offset(blob, "/reserved-memory/linux,log");
	if (nodeoffset > 0) {
		subnode = nodeoffset;
		err = fdt_get_resource(blob, subnode, "reg", 0, &res);
		if (err >= 0) {
			addr = res.start;
			size = res.end - res.start + 1;
			uboot_log_use_new_addr(addr, size);
		} else {
			printf("%s:%d, %s nodeoffset=%x\n", __func__, __LINE__,
			       "/reserved-memory/linux,log", nodeoffset);
		}
	}
	return 0;
}
#endif

void aml_board_display_env_handler(void)
{
	char *bootup_display;
#ifdef CONFIG_AML_LCD
	char lcd_init_str[8];
#endif

	run_command("get_rebootmode", 0);
	printf("reboot_mode: %s\n", env_get("reboot_mode"));
	run_command("run check_display", 0);
	bootup_display = env_get("bootup_display");
	if (!bootup_display)
		return;

	printf("bootup_display: %s\n", bootup_display);
#ifdef CONFIG_AML_LCD
	if (strcmp(bootup_display, "on") == 0)
		snprintf(lcd_init_str, 8, "%d", LCD_INIT_LEVEL_NORMAL);
	else if (strcmp(bootup_display, "preboot") == 0)
		snprintf(lcd_init_str, 8, "%d", LCD_INIT_LEVEL_PREBOOT);
	else
		snprintf(lcd_init_str, 8, "%d", LCD_INIT_LEVEL_PWR_OFF);

	env_set("lcd_init_level", lcd_init_str);
	//printf("lcd_init_level: %s\n", env_get("lcd_init_level"));
#endif
}

unsigned int random(void)
{
	volatile unsigned int val;

#ifdef CONFIG_HW_RNG_OLD
	val = readl(RNG_USR_DATA);
#else
	do {} while (readl(RNG_REE_READY) & 0x1);
	do {} while (readl(RNG_REE_CFG) & 0x1);
	writel(readl(RNG_REE_CFG) | (1 << 31), RNG_REE_CFG);
	do {} while (readl(RNG_REE_CFG) >> 31);

	val = readl(RNG_REE_OUT0);
	writel(0x1, RNG_REE_READY);
#endif
	return val;
}

void get_rng_hw(unsigned char *buf, unsigned int size)
{
	unsigned int *p = (unsigned int *)buf;
	unsigned int cnt, ncnt;
	unsigned int i;
	unsigned int rng;

	cnt = (size >> 2);
	ncnt = size & 0x3;

	for (i = 0; i < cnt; i++)
		p[i] = random();

	if (ncnt) {
		rng = random();
		for (i = 0; i < ncnt; i++)
			buf[cnt * 4 + i] = ((rng >> (i * 8)) & 0xff);
	}
}

int board_rng_seed(struct abuf *buf)
{
	abuf_init(buf);
	abuf_realloc(buf, 128);
	if (buf->data)
		get_rng_hw(buf->data, buf->size);

	return 0;
}

#ifdef CONFIG_AML_DEFENV
const char * const _aml_env_reserv_array[] = {
	"lock",
	"upgrade_step",
	"bootloader_version",
	"hdmimode",
	"is.bestmode",
	"dts_to_gpt",
	"fastboot_step",
	"reboot_status",
	"expect_index",
	"recovery_check_part",
	"factorytest",
	"defenv_para",	//set in board_late_init
	"rollback_reason",
#ifndef CONFIG_CMD_CAR_PARAMS
	"outputmode",
	"connector0_type",
#endif
	"sw_version",	//linux swupdate version
	NULL//Keep NULL be last to tell END
};

const char * const _aml_env_reserv_array1[] = {
	"lock",
	"upgrade_step",
	"bootloader_version",
	"dts_to_gpt",
	"fastboot_step",
	"reboot_status",
	"expect_index",
	"recovery_check_part",
	"defenv_para",	//set in board_late_init
	NULL//Keep NULL be last to tell END
};
#endif//#ifdef CONFIG_AML_DEFENV

