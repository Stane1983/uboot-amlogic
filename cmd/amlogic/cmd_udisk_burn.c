// SPDX-License-Identifier: (GPL-2.0-only OR MIT)
/*
 * Copyright (C) 2025 Amlogic, Inc. All rights reserved
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <amlogic/keyunify.h>
#include <amlogic/aml_efuse.h>
#include <amlogic/storage.h>
#include <asm/amlogic/arch/efuse.h>
#include <amlogic/emmc_partitions.h>
#include <amlogic/store_wrapper.h>
#include <../../drivers/usb/gadget/v2_burning/v2_common/optimus_download.h>
#include <../../drivers/usb/gadget/v2_burning/v2_burning_i.h>
#include <u-boot/sha256.h>
#include <fs.h>

#ifdef CONFIG_DDR_FASTBOOT
#include <amlogic/aml_mmc.h>
#endif
#include <part.h>
#if defined(CONFIG_AML_ANTIROLLBACK) || defined(CONFIG_AML_AVB2_ANTIROLLBACK)
#include <amlogic/anti-rollback.h>
#endif

#define PARTNAME_MAX_LEN 128

/* UDISK package format for xxx_udisk.img */
#define UDISK_PKG_MAGIC "UDISKPKG"
#define UDISK_PKG_MAGIC_SIZE 8
#define UDISK_PKG_HEADER_SIZE 64
#define UDISK_PKG_IMG_OFFSET (64 << 10) /* 64KB */
#define MAX_IMAGE_NAMES 16
#define MAX_IMAGE_NAME_LEN 64
#define INI_BUFFER_SIZE 1024

#define BOOTLOADER_OFFSET		512
#define BOOTLOADER_MAX_SIZE		(4 * 1024 * 1024)

/* HMAC-SHA256 security */
#define HMAC_SHA256_SIZE 32
#define HMAC_KEY_SIZE 32
#define HMAC_IMG_DATA_MAX_SIZE (1 << 20) /* 1MB - max img data for HMAC */

/*
 * HMAC key storage (obfuscated)
 * The actual key is computed at runtime by XORing these parts
 * Key = part1 ^ part2 ^ part3
 */
static const u8 hmac_key_part1[HMAC_KEY_SIZE] = {
	0x4A, 0x8B, 0x2C, 0x5D, 0x9E, 0x1F, 0x6A, 0x3B, 0x7C, 0x0D, 0xBE,
	0x4F, 0xE0, 0x81, 0x22, 0x63, 0xD4, 0x95, 0x56, 0x17, 0xA8, 0xC9,
	0x7A, 0x2B, 0x8C, 0x4D, 0xEE, 0x8F, 0x30, 0xD1, 0x62, 0xB3};

static const u8 hmac_key_part2[HMAC_KEY_SIZE] = {
	0x1D, 0xCE, 0x7F, 0x20, 0xE1, 0x52, 0x33, 0x74, 0x25, 0x46, 0xF7,
	0x18, 0xB9, 0xDA, 0x6B, 0x2C, 0x9D, 0xDE, 0x0F, 0x50, 0xF1, 0x82,
	0x23, 0x74, 0xC5, 0x06, 0xB7, 0xC8, 0x79, 0x8A, 0x3B, 0xEC};

static const u8 hmac_key_part3[HMAC_KEY_SIZE] = {
	0x73, 0x24, 0x95, 0xE6, 0x07, 0xB8, 0xC9, 0x1A, 0xCB, 0xAC, 0x1D,
	0xF6, 0x57, 0x30, 0x81, 0xC2, 0x53, 0x34, 0xE5, 0xBE, 0x1F, 0x68,
	0xC9, 0x9A, 0x2B, 0xEC, 0x5D, 0x26, 0x97, 0x60, 0xD1, 0x02};

/* Compute the actual HMAC key at runtime */
static void get_hmac_key(u8 *key)
{
	int i;

	for (i = 0; i < HMAC_KEY_SIZE; i++)
		key[i] = hmac_key_part1[i] ^ hmac_key_part2[i] ^ hmac_key_part3[i];
}

/* Simple HMAC-SHA256 implementation using U-Boot's SHA256 */
static void hmac_sha256(const u8 *key, size_t key_len, const u8 *data,
					size_t data_len, u8 *out)
{
	sha256_context ctx;
	u8 k_ipad[64];
	u8 k_opad[64];
	u8 tk[32];
	u8 inner_hash[32];
	int i;

	/* If key is longer than 64 bytes, hash it first */
	if (key_len > 64) {
		sha256_starts(&ctx);
		sha256_update(&ctx, key, key_len);
		sha256_finish(&ctx, tk);
		key = tk;
		key_len = 32;
	}

	/* Prepare ipad and opad */
	memset(k_ipad, 0x36, 64);
	memset(k_opad, 0x5c, 64);
	for (i = 0; i < (int)key_len; i++) {
		k_ipad[i] ^= key[i];
		k_opad[i] ^= key[i];
	}

	/* Inner hash: SHA256(k_ipad || data) */
	sha256_starts(&ctx);
	sha256_update(&ctx, k_ipad, 64);
	sha256_update(&ctx, data, data_len);
	sha256_finish(&ctx, inner_hash);

	/* Outer hash: SHA256(k_opad || inner_hash) */
	sha256_starts(&ctx);
	sha256_update(&ctx, k_opad, 64);
	sha256_update(&ctx, inner_hash, 32);
	sha256_finish(&ctx, out);
}

typedef struct {
	char magic[UDISK_PKG_MAGIC_SIZE]; /* "UDISKPKG" */
	u32 vendor_info_offset;
	u32 vendor_info_size;
	u32 avb_key_offset;
	u32 avb_key_size;
	u32 img_offset;
	u32 img_size;
	u32 hmac_offset; /* HMAC-SHA256 offset */
	u32 hmac_size;   /* HMAC-SHA256 size (32 bytes) */
	u8 reserved[24];
} __attribute__((packed)) udisk_pkg_header_t;

#ifdef CONFIG_AVB2_KPUB_VENDOR
extern const char avb2_kpub_vendor[];
extern const int avb2_kpub_vendor_len;
#endif /* CONFIG_AVB_KPUB_VENDOR */

extern const char avb2_kpub_default[];
extern const int avb2_kpub_default_len;
extern const char avb2_kpub_production[];
extern const int avb2_kpub_production_len;

const char *const UpgradeLogo_Addr =
	(const char *)(OPTIMUS_DOWNLOAD_DISPLAY_BUF + OPTIMUS_DOWNLOAD_SLOT_SZ);

#ifdef CONFIG_REPLACE_TV_MODE

/* Upgrade file names */
static const char *const big_pkg_files[] = {
	"aml_upgrade_package_all.img",
	"aml_upgrade_package_no_oem_key.img",
	"aml_upgrade_package_no_tvconfig_oem.img",
	"aml_upgrade_package_no_tvconfig_oem_key.img",
	"aml_upgrade_package_no_tvconfig_oem_key_pid.img",
	"aml_upgrade_package_no_tvconfig_oem_skw_key_pid.img",
	"aml_upgrade_package.img",
	NULL
};

static const char *const small_pkg_files[] = {
	"oem_udisk.img",
	"tvconfig_udisk.img",
	"skw_udisk.img",
	"bootloader_udisk.img",
	"vbmeta_udisk.img",
	NULL
};

#define MAX_SMALL_PKGS 3
#define MAX_FILENAME_LEN 128
#define PID_PREFIX "pid_index_"

/* Structure to hold scan results */
typedef struct {
	int big_pkg_count;
	char big_pkg_name[MAX_FILENAME_LEN];
	int big_pkg_index; /* Index in big_pkg_files array */
	int small_pkg_count;
	char small_pkg_names[MAX_SMALL_PKGS][MAX_FILENAME_LEN];
	int pid_count;
	char pid_name[MAX_FILENAME_LEN];
} usb_upgrade_files_t;

/**
 * scan_usb_upgrade_files - Scan USB root directory for upgrade files
 * @files: Output structure to hold scan results
 *
 * Uses fatls to list all files in USB root, then matches against known
 * upgrade file names. This is more efficient than checking each file
 * individually with fatsize.
 *
 * Return: 0 on success, -1 on failure
 */
static int scan_usb_upgrade_files(usb_upgrade_files_t *files)
{
	int ret, i;
	struct fs_dir_stream *dirs;
	struct fs_dirent *dent;

	if (!files)
		return -1;

	memset(files, 0, sizeof(*files));

	/* First, check if USB is ready */
	ret = run_command("usb start 0", 0);
	if (ret != 0) {
		printf("USB start failed\n");
		return -1;
	}

	/* Set block device for fs operations */
	if (fs_set_blk_dev("usb", "0", FS_TYPE_FAT)) {
		printf("Failed to set usb device\n");
		return -1;
	}

	/* Open root directory */
	dirs = fs_opendir("/");
	if (!dirs) {
		printf("Failed to open USB root directory\n");
		return -1;
	}

	/* Iterate through directory entries */
	while ((dent = fs_readdir(dirs)) != NULL) {
		/* Skip directories */
		if (dent->type == FS_DT_DIR)
			continue;

		const char *name = dent->name;

		/* Check big packages */
		for (i = 0; big_pkg_files[i]; i++) {
			if (strcasecmp(name, big_pkg_files[i]) == 0) {
				files->big_pkg_count++;
				if (files->big_pkg_count == 1) {
					strncpy(files->big_pkg_name, name, MAX_FILENAME_LEN - 1);
					files->big_pkg_index = i;
				}
				printf("Found big package: %s\n", name);
				break;
			}
		}

		/* Check small packages */
		for (i = 0; small_pkg_files[i]; i++) {
			if (strcasecmp(name, small_pkg_files[i]) == 0) {
				if (files->small_pkg_count < MAX_SMALL_PKGS) {
					strncpy(files->small_pkg_names[files->small_pkg_count], name,
						MAX_FILENAME_LEN - 1);
					files->small_pkg_count++;
				}
				printf("Found small package: %s\n", name);
				break;
			}
		}

		/* Check PID files (pid_index_*.img pattern) */
		if (strncasecmp(name, PID_PREFIX, strlen(PID_PREFIX)) == 0) {
			char *ext = strstr(name, ".img");

			if (ext && (ext[4] == '\0' || ext[4] == ' ')) {
				if (files->pid_count == 0)
					strncpy(files->pid_name, name, MAX_FILENAME_LEN - 1);
				files->pid_count++;
				printf("Found PID file: %s\n", name);
			}
		}
	}

	fs_closedir(dirs);

	printf("Scan complete: big_pkg=%d, small_pkg=%d, pid=%d\n",
		files->big_pkg_count, files->small_pkg_count, files->pid_count);
	return 0;
}

/**
 * generate_ini_file - Generate aml_sdc_burn.ini on USB disk
 * @pkg_name: Name of the upgrade package file
 *
 * Creates the INI file required for USB burning.
 *
 * Return: 0 on success, -1 on failure
 */
static int generate_ini_file(const char *pkg_name)
{
	char cmd[256];
	ulong loadaddr;
	char *buf;
	int size;
	int ret;

	if (!pkg_name)
		return -1;

	loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);
	buf = (char *)loadaddr;

	/* Generate INI content */
	size = sprintf(
		buf,
		";\n"
		";Amlogic sdcard burning configure script\n"
		";This card burning script support both dos and unix file format\n"
		";Except comment, all must readable ASCII letters\n"
		";\n"
		"\n"
		"[common]\n"
		"erase_bootloader    = 0\n"
		"erase_flash         = 1\n"
		"reboot              = 1\n"
		"\n"
		";package will filled by sdcard burning tool\n"
		"[burn_ex]\n"
		"package     = %s\n"
		";media       =\n",
		pkg_name);

	printf("Generating aml_sdc_burn.ini for package: %s\n", pkg_name);

	/* Write INI file to USB */
	sprintf(cmd, "fatwrite usb 0 %lx aml_sdc_burn.ini %x", loadaddr, size);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to write aml_sdc_burn.ini to USB\n");
		return -1;
	}

	printf("Successfully created aml_sdc_burn.ini\n");
	return 0;
}

int aml_udisk_lock(int state)
{
	char *rpmb_flag = env_get("rpmb_state");

	if (rpmb_flag && strcmp(rpmb_flag, "0")) {
		char *lock_s = env_get("lock");
		LockData_t info = { 0 };
		int erase_flag = 0;
		char lock_d[LOCK_DATA_SIZE];

		memset(&info, 0, sizeof(struct LockData));
		info.version_major = (int)(lock_s[0] - '0');
		info.version_minor = (int)(lock_s[1] - '0');
		info.unlock_ability = (int)(lock_s[2] - '0');
		info.lock_state = (int)(lock_s[4] - '0');
		info.lock_critical_state = (int)(lock_s[5] - '0');
		info.lock_bootloader = (int)(lock_s[6] - '0');

#if defined(CONFIG_AML_ANTIROLLBACK) || defined(CONFIG_AML_AVB2_ANTIROLLBACK)
		uint32_t lock_state = 0;

		printf("avb arb mode\n");
		if (is_avb_arb_available()) {
			if (get_avb_lock_state(&lock_state)) {
				if (state == 0 &&
					(info.lock_state == 1 || lock_state == 1)) {
					printf("device is locked, need unlock\n");
					if (avb_unlock())
						erase_flag = 1;
				} else if (state == 1 &&
					(info.lock_state == 0 || lock_state == 0)) {
					printf("device is unlocked, need lock\n");
					if (avb_lock())
						erase_flag = 1;
				}
			}
		}
#endif

		if (state == 0 && info.lock_state == 1) {
			printf("device is locked, need unlock\n");
			info.lock_state = 0;
			info.lock_critical_state = 0;
			erase_flag = 1;
			env_set("lock_state", "green");
			sprintf(lock_d, "%d%d%d0%d%d%d0", info.version_major, info.version_minor,
				info.unlock_ability, info.lock_state, info.lock_critical_state,
				info.lock_bootloader);
			printf("lock_d state: %s\n", lock_d);
			env_set("lock", lock_d);
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p lock;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
		} else if (state == 1 && info.lock_state == 0) {
			printf("device is unlocked, need lock\n");
			info.lock_state = 1;
			erase_flag = 1;
			env_set("lock_state", "orange");
			sprintf(lock_d, "%d%d%d0%d%d%d0", info.version_major, info.version_minor,
				info.unlock_ability, info.lock_state, info.lock_critical_state,
				info.lock_bootloader);
			printf("lock_d state: %s\n", lock_d);
			env_set("lock", lock_d);
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p lock;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
		}

		if (erase_flag == 1)
			run_command("store erase metadata 0 0", 0);
	}
	return 0;
}

int check_pid_update_udisk(const char *file_name, int show_flag);

int check_udisk_burn_fw(usb_upgrade_files_t *files)
{
	int ret = 0;
	ulong loadaddr, tmp_addr;
	char cmd[128];
	int erase_all_flag = 0;
	int erase_pid_flag = 0;
	int erase_key_flag = 0;
	char pid[50] = {0x0};
	char mac[50] = {0x0};
	char usid[50] = {0x0};
	int ret_key = 0;
	int len = 0;
	const char *target_img = NULL;

	if (!files || files->big_pkg_count != 1) {
		printf("No big package to upgrade\n");
		return -1;
	}

	loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);
	tmp_addr = loadaddr - 0x1000;

	printf("upgrading %s\n", files->big_pkg_name);

	/* Generate INI file for this package */
	ret = generate_ini_file(files->big_pkg_name);
	if (ret != 0) {
		printf("Failed to generate INI file\n");
		return -1;
	}

	target_img = files->big_pkg_name;

	/* Set erase flags based on package index */
	if (files->big_pkg_index == 0)
		erase_all_flag = 1;
	if (files->big_pkg_index >= 1 && files->big_pkg_index <= 3)
		erase_pid_flag = 1;
	if (files->big_pkg_index == 2)
		erase_key_flag = 1;

	/* Verify the image exists and load it */
	sprintf(cmd, "fatload usb 0 %lx %s 4", loadaddr, target_img);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load %s\n", target_img);
		return -1;
	}

	printf("read aml_upgrade_package.img from udisk ok\n");
	printf("need burn\n");

	if (erase_all_flag == 1 || erase_key_flag == 1) {
		env_set("need_erase_key", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p need_erase_key;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif
	}
	if (erase_all_flag == 1) {
		printf("erase all unifykey\n");
		run_command("mmc dev 1", 0);
		run_command("emmc erase key", 0);
		if (files->pid_count > 0 && files->pid_name[0] != '\0') {
			printf("found valid pid file %s, trying to update pid via udisk package\n", files->pid_name);
			int pid_ret = check_pid_update_udisk(files->pid_name, 0);
			if (pid_ret == 0) {
				printf("write pid from %s ok\n", files->pid_name);
			} else {
				printf("write pid from %s failed\n", files->pid_name);
			}
		}
	}
	if (erase_pid_flag == 1) {
		printf("erase pid\n");
		if (key_unify_read("model_name", pid, sizeof(pid)) == 0) {
			debug("key_unify_read pid ok, reset it\n");
			run_command("defenv_reserv; setenv reboot_mode cold_boot; saveenv;", 0);
			char *model = env_get("model_name");

			printf("model is %s\n", model);
			len = strlen(model) + 1;
			ret_key = key_unify_write("model_name", model, len);

			if (ret_key == 0) {
				printf("write default pid ok, erase param\n");
				run_command("store erase param 0 0", 0);
			}
		} else {
			printf("key_unify_read pid error, skip\n");
		}

		if (files->pid_count > 0 && files->pid_name[0] != '\0') {
			printf("found valid pid file %s, trying to update pid via udisk package\n", files->pid_name);
			int pid_ret = check_pid_update_udisk(files->pid_name, 0);
			if (pid_ret == 0) {
				printf("write pid from %s ok\n", files->pid_name);
			} else {
				printf("write pid from %s failed\n", files->pid_name);
			}
		}
	}
	if (erase_key_flag == 1) {
		printf("delete mac\n");
		if (key_unify_read("mac", mac, sizeof(mac)) == 0) {
			debug("key_unify_read mac ok, reset it\n");
			memset(mac, 0, sizeof(mac));
			strcpy(mac, "00:00:00:00:00:00");
			len = strlen("00:00:00:00:00:00") + 1;
			ret_key = key_unify_write("mac", mac, len);
			if (ret_key == 0)
				debug("write default mac ok\n");
		} else {
			printf("key_unify_read mac error, skip\n");
		}

		printf("delete usid\n");
		if (key_unify_read("usid", usid, sizeof(usid)) == 0) {
			debug("key_unify_read usid ok, reset it\n");
			memset(usid, 0, sizeof(usid));
			strcpy(usid, "0000");
			len = strlen("0000") + 1;
			ret_key = key_unify_write("usid", usid, len);
			if (ret_key == 0)
				debug("write default usid ok\n");
		} else {
			printf("key_unify_read usid error, skip\n");
		}
	}

	//erase rollback index
	aml_udisk_lock(0);
	aml_udisk_lock(1);

	env_set("usb_burn_state", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
	run_command("update_env_part -p usb_burn_state;", 0);
#else
	run_command("defenv_reserv; saveenv;", 0);
#endif
	ret = run_command("usb_burn aml_sdc_burn.ini", 0);
	printf("ret = %d\n", ret);
	if (ret != 0) {
		env_set("usb_burn_state", "2");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p usb_burn_state;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif

		run_command("reboot", 0);
	}

	return ret;
}

int read_file_part(const char *filename, unsigned long offset,
	unsigned long len, unsigned long buf_addr)
{
	char cmd[128];
	int ret;

	sprintf(cmd, "fatload usb 0 %lx %s %lx %lx",
		buf_addr, filename, len, offset);
	ret = run_command(cmd, 0);

	unsigned long actual_len = env_get_hex("filesize", 0);

	return (ret == 0 && actual_len == len) ? 0 : -1;
}

/**
 * verify_udisk_pkg_key - Verify AVB public key in udisk package
 * @pkg_addr: Address where the udisk package is loaded
 * @header: Pointer to the parsed header
 *
 * Return: 0 if key matches, -1 otherwise
 */
static int verify_udisk_pkg_key(ulong pkg_addr, udisk_pkg_header_t *header)
{
	const char *public_key_data;
	int public_key_length;
	int ret = -1;

	public_key_data = (const char *)(pkg_addr + header->avb_key_offset);
	public_key_length = header->avb_key_size;

	printf("UDISK package AVB key: size=%d\n", public_key_length);

#ifdef CONFIG_AVB2_KPUB_VENDOR
	if (avb2_kpub_vendor_len == public_key_length &&
		!memcmp(public_key_data, avb2_kpub_vendor, avb2_kpub_vendor_len)) {
		printf("udisk key match vendor kpub\n");
		ret = 0;
	}

#ifdef CONFIG_TESTKEY
	if (ret != AVB_IO_RESULT_OK) {
		printf("re-verify with default kpub\n");
		if (avb2_kpub_default_len == public_key_length &&
			!memcmp(public_key_data, avb2_kpub_default, public_key_length)) {
			printf("udisk key match default kpub\n");
			ret = 0;
		}
	}
#endif
	if (ret != 0)
		printf("udisk key NOT match vendor kpub!\n");
	return ret;
#else
	printf("not use vendor kpub, skip check\n");
	ret = 0;
#endif

	return ret;
}

/**
 * verify_udisk_pkg_hmac - Verify HMAC-SHA256 of udisk package
 * @pkg_addr: Address where the udisk package is loaded
 * @header: Pointer to the parsed header
 * @img_data: Pointer to the original image data
 * @img_size: Size of the original image data
 *
 * HMAC is computed over: vendor_info + avb_key + img_data
 *
 * Return: 0 if HMAC matches, -1 otherwise
 */
static int verify_udisk_pkg_hmac(ulong pkg_addr, udisk_pkg_header_t *header,
			const u8 *img_data, u32 img_size)
{
	u8 hmac_key[HMAC_KEY_SIZE];
	u8 computed_hmac[HMAC_SHA256_SIZE];
	const u8 *stored_hmac;
	u8 *hmac_data = NULL;
	size_t hmac_data_len;
	const u8 *vendor_info;
	const u8 *avb_key;
	int ret = -1;

	/* Check if HMAC is present */
	if (header->hmac_size != HMAC_SHA256_SIZE || header->hmac_offset == 0) {
		printf("HMAC not present in package (size=%u, offset=%u)\n",
			header->hmac_size, header->hmac_offset);
		return -1;
	}

	/* Get the stored HMAC from package */
	stored_hmac = (const u8 *)(pkg_addr + header->hmac_offset);

	/* Get pointers to data for HMAC computation */
	vendor_info = (const u8 *)(pkg_addr + header->vendor_info_offset);
	avb_key = (const u8 *)(pkg_addr + header->avb_key_offset);

	/* Allocate buffer for HMAC data: vendor_info + avb_key + img_data */
	hmac_data_len = header->vendor_info_size + header->avb_key_size + img_size;
	hmac_data = (u8 *)malloc(hmac_data_len);
	if (!hmac_data) {
		printf("Failed to allocate HMAC data buffer\n");
		return -1;
	}

	/* Copy data for HMAC computation */
	memcpy(hmac_data, vendor_info, header->vendor_info_size);
	memcpy(hmac_data + header->vendor_info_size, avb_key, header->avb_key_size);
	memcpy(hmac_data + header->vendor_info_size + header->avb_key_size, img_data,
		img_size);

	/* Get the HMAC key (computed at runtime from obfuscated parts) */
	get_hmac_key(hmac_key);

	/* Compute HMAC */
	hmac_sha256(hmac_key, HMAC_KEY_SIZE, hmac_data, hmac_data_len, computed_hmac);

	/* Compare with stored HMAC */
	if (memcmp(computed_hmac, stored_hmac, HMAC_SHA256_SIZE) == 0) {
		printf("HMAC verification passed\n");
		ret = 0;
	} else {
		printf("HMAC verification FAILED! Package may be tampered.\n");
		ret = -1;
	}

	/* Clear sensitive data */
	memset(hmac_key, 0, HMAC_KEY_SIZE);
	free(hmac_data);

	return ret;
}

/**
 * parse_udisk_vendor_info - Parse and print vendor info from udisk package
 * @pkg_addr: Address where the udisk package is loaded
 * @header: Pointer to the parsed header
 * @vendor_device: Output buffer for vendor_device value (must be at least 64
 * bytes)
 *
 * Return: 0 on success, -1 on failure
 */
static int parse_udisk_vendor_info(ulong pkg_addr, udisk_pkg_header_t *header,
			char *vendor_device,
			size_t device_buf_size)
{
	char *vendor_info;
	char *line, *end;
	int found_device = 0;

	if (!vendor_device || device_buf_size == 0)
		return -1;

	vendor_device[0] = '\0';
	vendor_info = (char *)(pkg_addr + header->vendor_info_offset);

	debug("=== UDISK Package Vendor Info ===\n");

	/* Parse each line of vendor info */
	line = vendor_info;
	end = vendor_info + header->vendor_info_size;
	while (line < end && *line) {
		char *newline = line;
		char *eq_pos;

		while (newline < end && *newline && *newline != '\n')
			newline++;

		if (newline > line) {
			/* Temporarily null-terminate */
			char saved = *newline;
			*newline = '\0';

			debug("  %s\n", line);

			/* Parse key=value */
			eq_pos = strchr(line, '=');
			if (eq_pos) {
				*eq_pos = '\0';
				if (strcmp(line, "vendor_device") == 0) {
					strncpy(vendor_device, eq_pos + 1, device_buf_size - 1);
					vendor_device[device_buf_size - 1] = '\0';
					found_device = 1;
				}
				*eq_pos = '=';
			}

			*newline = saved;
		}

		line = newline + 1;
	}
	debug("=================================\n");

	if (!found_device) {
		printf("Warning: vendor_device not found in vendor info\n");
		return -1;
	}

	printf("Parsed vendor_device: %s\n", vendor_device);
	return 0;
}

/**
 * check_partition_update - Check and update partition from USB disk
 * @partition_name: Name of partition to update (e.g., "oem", "odm_ext")
 *
 * This function loads the partition image (xxx.img or xxx_udisk.img as passed),
 * and if it's a udisk package, verifies the AVB key and board name match
 * before burning. Only burns if all verifications pass.
 *
 * Return: 0 on success, -1 on failure
 */
int check_partition_update(const char *partition_name)
{
	char partname[128] = {0};
	char imgname[128] = {0};
	char cmd[256] = {0};
	char *slot_name;
	int ret = -1;
	ulong loadaddr;
	ulong pkg_addr;
	u64 rc = 0;
	udisk_pkg_header_t *header;
	char vendor_device[64] = {0};
	char *board_name;

	slot_name = env_get("slot-suffixes");
	loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);
	pkg_addr = loadaddr;

	sprintf(imgname, "%s.img", partition_name);

	ret = run_command("usb start 0", 0);
	if (ret != 0) {
		printf("USB start failed\n");
		return -1;
	}

	/* Check if the image file exists */
	env_set_hex("filesize", 0);
	sprintf(cmd, "fatsize usb 0 %s", imgname);
	ret = run_command(cmd, 0);

	if (env_get_hex("filesize", 0) == 0) {
		printf("%s not found on USB disk\n", imgname);
		return -1;
	}

	printf("Found %s (size=0x%x), loading header first...\n", imgname, UDISK_PKG_HEADER_SIZE);

	/* Load the image file */
	sprintf(cmd, "fatload usb 0 %lx %s %x", pkg_addr, imgname, UDISK_PKG_HEADER_SIZE);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load header of %s\n", imgname);
		return -1;
	}

	/* Check if this is a UDISK package (by checking magic) */
	header = (udisk_pkg_header_t *)pkg_addr;
	if (memcmp(header->magic, UDISK_PKG_MAGIC, UDISK_PKG_MAGIC_SIZE) == 0) {
		printf("Detected UDISK package format\n");

		debug("UDISK package header:\n");
		debug("  vendor_info: offset=%u, size=%u\n", header->vendor_info_offset,
			   header->vendor_info_size);
		debug("  avb_key: offset=%u, size=%u\n", header->avb_key_offset,
			   header->avb_key_size);
		debug("  image: offset=%u, size=%u\n", header->img_offset,
			   header->img_size);

		/* Load header + vendor_info + avb_key for verification */
		ulong verify_size = header->img_offset;

		debug("Loading verification data (size=0x%lx)...\n", verify_size);
		sprintf(cmd, "fatload usb 0 %lx %s %lx", pkg_addr, imgname, verify_size);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Failed to load verification data\n");
			return -1;
		}
		/* Parse vendor info and get vendor_device */
		ret = parse_udisk_vendor_info(pkg_addr, header, vendor_device,
				sizeof(vendor_device));
		if (ret != 0) {
			printf("Failed to parse vendor info, abort upgrade\n");
			return -1;
		}

		/* Verify board name matches vendor_device */
		board_name = env_get("board");
		debug("Board name from env: %s\n", board_name ? board_name : "NULL");
		debug("Vendor device from package: %s\n", vendor_device);

		if (!board_name || strcmp(board_name, vendor_device) != 0) {
			printf("ERROR: Board name mismatch! board=%s, vendor_device=%s\n",
				   board_name ? board_name : "NULL", vendor_device);
			printf("Abort upgrade for safety\n");
			return -1;
		}
		printf("Board name matched: %s\n", board_name);

		/* Verify AVB key */
		ret = verify_udisk_pkg_key(pkg_addr, header);
		if (ret != 0) {
			printf("AVB key verification failed, abort upgrade\n");
			return -1;
		}

		/* Verify HMAC-SHA256 - only use first 1MB of img data for large images */
		ulong hmac_verify_addr = loadaddr + UDISK_PKG_IMG_OFFSET;
		ulong img_size_for_hmac = header->img_size;

		/* For large images, only verify first 1MB */
		if (img_size_for_hmac > HMAC_IMG_DATA_MAX_SIZE) {
			img_size_for_hmac = HMAC_IMG_DATA_MAX_SIZE;
			printf("Large image detected, HMAC will verify first 1MB only\n");
		}

		printf("Loading image for HMAC verification (size=0x%lx)...\n",
			img_size_for_hmac);
		sprintf(cmd, "fatload usb 0 %lx %s %lx %x", hmac_verify_addr, imgname,
			img_size_for_hmac, header->img_offset);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Failed to load image for HMAC verification\n");
			return -1;
		}

		ret = verify_udisk_pkg_hmac(
			pkg_addr, header, (const u8 *)hmac_verify_addr, img_size_for_hmac);
		if (ret != 0) {
			printf("HMAC verification failed, abort upgrade\n");
			return -1;
		}
	} else {
		printf("invalid image\n");
		return -1;
	}

	/* Remove "_udisk" suffix from partition_name if present */
	char real_partname[128] = {0};

	/* Handle special image name mappings:
	 * tvconfig_udisk -> odm_ext
	 * skw_udisk -> production_ext
	 */
	if (strncmp(partition_name, "tvconfig_udisk", 14) == 0) {
		strncpy(real_partname, "odm_ext", sizeof(real_partname) - 1);
	} else if (strncmp(partition_name, "skw_udisk", 9) == 0) {
		strncpy(real_partname, "production_ext", sizeof(real_partname) - 1);
	} else {
		/* Remove "_udisk" suffix from partition_name if present */
		strncpy(real_partname, partition_name, sizeof(real_partname) - 1);
		char *udisk_suffix = strstr(real_partname, "_udisk");

		if (udisk_suffix && udisk_suffix[6] == '\0')
			*udisk_suffix = '\0';
	}

	rc = store_part_size(real_partname);
	if (-1 == rc) {
		printf("try ab mode\n");
		if (slot_name && (strcmp(slot_name, "0") == 0))
			sprintf(partname, "%s_a", real_partname);
		else if (slot_name && (strcmp(slot_name, "1") == 0))
			sprintf(partname, "%s_b", real_partname);
	} else {
		sprintf(partname, "%s", real_partname);
	}

	debug("key matched, proceeding with burn\n");

	//ulong img_addr = pkg_addr + header->img_offset;
	ulong img_size = header->img_size;
	ulong img_offset = header->img_offset;
	ulong chunk_size = UDISK_PKG_IMG_OFFSET; /* 64KB per chunk */
	ulong bytes_written = 0;
	ulong bytes_remaining = img_size;
	ulong current_offset = img_offset;
	__hdle hUiProgress = NULL;
	int logo_flag = -1;
	int rcode = 0;

	printf("Burning %s from UDISK package (offset=0x%lx, size=0x%lx)...\n",
		real_partname, img_offset, img_size);
	debug("Using 64KB chunk transfer\n");

	/*show ui*/
	if (video_res_prepare_for_upgrade(NULL)) {
		printf("Fail when prepare bm res or init video for upgrade\n");
		logo_flag = -1;
		goto _update;
	}
	show_logo_to_report_burning();

	hUiProgress = optimus_progress_ui_request_for_sdc_burn();
	if (!hUiProgress) {
		printf("request progress handle failed!\n");
		logo_flag = -1;
		goto _update;
	}

	optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STEPS_AFTER_DISK_INIT_OK);
	if (strncmp(partname, "bootloader", 10) == 0) {
		rcode = optimus_progress_ui_set_smart_mode(hUiProgress, 200,
			UPGRADE_STEPS_AFTER_BURN_BOOTLOADER_OK - UPGRADE_STEPS_AFTER_DISK_INIT_OK);
		run_command("sleep 6", 0);
		if (rcode)
			optimus_progress_ui_update_by_bytes(hUiProgress, 200);
	} else {
		rcode = optimus_progress_ui_set_smart_mode(hUiProgress, img_size,
			UPGRADE_STEPS_AFTER_BURN_BOOTLOADER_OK - UPGRADE_STEPS_AFTER_DISK_INIT_OK);
	}

	if (rcode) {
		printf("Fail to set smart mode\n");
		logo_flag = -1;
		goto _update;
	}
	logo_flag = 0;

_update:
	/* Read and write in 64KB chunks */
	if (strncmp(partname, "bootloader", 10) == 0) {
		printf("write bootloader\n");
		sprintf(cmd, "fatload usb 0 %lx %s %lx %lx", loadaddr, imgname,
				img_size, img_offset);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Failed to load chunk at offset 0x%lx\n", img_offset);
			return -1;
		}

		ret = store_boot_write("bootloader", 1, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET,
			(void *)loadaddr);
		if (ret != 0) {
			printf("Failed to write boot0\n");
			return -1;
		}
		printf("write boot0 ok\n");
		ret = store_boot_write("bootloader", 2, BOOTLOADER_MAX_SIZE - BOOTLOADER_OFFSET,
			(void *)loadaddr);
		if (ret != 0) {
			printf("Failed to write boot0\n");
			return -1;
		}
		printf("write boot1 ok\n");
		run_command("mmc dev 1 0;", 0);

		if (hUiProgress)
			optimus_progress_ui_report_upgrade_stat(hUiProgress, !rcode);
		if (hUiProgress)
			optimus_progress_ui_release(hUiProgress);
		run_command("sleep 2", 0);
	} else {
		printf("write other part: %s\n", partname);
		while (bytes_remaining > 0) {
			ulong read_size = (bytes_remaining >=
				chunk_size) ? chunk_size : bytes_remaining;

			/* Load 64KB (or remaining) chunk from USB */
			sprintf(cmd, "fatload usb 0 %lx %s %lx %lx", loadaddr, imgname,
				read_size, current_offset);
			ret = run_command(cmd, 0);
			if (ret != 0) {
				printf("Failed to load chunk at offset 0x%lx\n", current_offset);
				return -1;
			}

			/* Write chunk to storage using store_logic_write */
			ret = store_logic_write(partname, bytes_written, read_size,
				(void *)loadaddr);
			if (ret != 0) {
				printf("Failed to write chunk at offset 0x%lx\n", bytes_written);
				return -1;
			}

			if (logo_flag == 0)
				optimus_progress_ui_update_by_bytes(hUiProgress, read_size);

			bytes_written += read_size;
			bytes_remaining -= read_size;
			current_offset += read_size;

			/* Print progress every 1MB */
			if ((bytes_written % (1 << 20)) == 0 || bytes_remaining == 0) {
				printf("Progress: %lu / %lu bytes (%lu%%)\n", bytes_written, img_size,
					(bytes_written * 100) / img_size);
			}
		}
		if (hUiProgress)
			optimus_progress_ui_report_upgrade_stat(hUiProgress, !rcode);
		if (hUiProgress)
			optimus_progress_ui_release(hUiProgress);
	}

	printf("Successfully wrote %lu bytes to %s\n", bytes_written, partname);
	ret = 0;

	return ret;
}

/**
 * check_pid_update_udisk - Check and update PID from UDISK package format
 * @file_name: Name of the PID package file (e.g., "pid_2001.img")
 *
 * This function loads the PID package, verifies board name and AVB key,
 * then extracts and writes the PID value.
 *
 * Return: 0 on success, -1 on failure
 */
int check_pid_update_udisk(const char *file_name, int show_flag)
{
	ulong loadaddr;
	ulong pkg_addr;
	int ret = -1;
	char cmd[256];
	char pid[50] = {0x0};
	udisk_pkg_header_t *header;
	char vendor_device[64] = {0};
	char *board_name;

	loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);
	pkg_addr = loadaddr;

	ret = run_command("usb start 0", 0);
	if (ret != 0) {
		printf("USB start failed\n");
		return -1;
	}

	/* Check if the PID package file exists */
	env_set_hex("filesize", 0);
	sprintf(cmd, "fatsize usb 0 %s", file_name);
	ret = run_command(cmd, 0);
	if (env_get_hex("filesize", 0) == 0) {
		printf("%s not found on USB disk\n", file_name);
		return -1;
	}

	ulong file_size = env_get_hex("filesize", 0);

	printf("Found %s (size=0x%lx)\n", file_name, file_size);

	/* Load header first (64 bytes) */
	sprintf(cmd, "fatload usb 0 %lx %s %x", pkg_addr, file_name,
		UDISK_PKG_HEADER_SIZE);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load header of %s\n", file_name);
		return -1;
	}

	/* Check if this is a UDISK package */
	header = (udisk_pkg_header_t *)pkg_addr;
	if (memcmp(header->magic, UDISK_PKG_MAGIC, UDISK_PKG_MAGIC_SIZE) != 0) {
		printf("%s is not a UDISK package format\n", file_name);
		return -1;
	}

	printf("Detected UDISK PID package format\n");

	/* Load header + vendor_info + avb_key for verification */
	ulong verify_size = header->img_offset;

	sprintf(cmd, "fatload usb 0 %lx %s %lx", pkg_addr, file_name, verify_size);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load verification data\n");
		return -1;
	}

	/* Parse vendor info and get vendor_device */
	ret = parse_udisk_vendor_info(pkg_addr, header, vendor_device,
			sizeof(vendor_device));
	if (ret != 0) {
		printf("Failed to parse vendor info, abort PID update\n");
		return -1;
	}

	/* Verify board name matches vendor_device */
	board_name = env_get("board");

	if (!board_name || strcmp(board_name, vendor_device) != 0) {
		printf("ERROR: Board name mismatch! board=%s, vendor_device=%s\n",
			board_name ? board_name : "NULL", vendor_device);
		printf("Abort PID update for safety\n");
		return -1;
	}
	printf("Board name matched: %s\n", board_name);

	/* Verify AVB key */
	ret = verify_udisk_pkg_key(pkg_addr, header);
	if (ret != 0) {
		printf("AVB key verification failed, abort PID update\n");
		return -1;
	}

	/* Load PID data from package */
	ulong pid_offset = header->img_offset;
	ulong pid_size = header->img_size;

	/* Load PID data to correct offset to avoid overwriting header */
	ulong pid_load_addr = pkg_addr + pid_offset;

	sprintf(cmd, "fatload usb 0 %lx %s %lx %lx", pid_load_addr, file_name, pid_size,
		pid_offset);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load PID data\n");
		return -1;
	}

	/* Verify HMAC-SHA256 */
	ret = verify_udisk_pkg_hmac(pkg_addr, header, (const u8 *)pid_load_addr,
							pid_size);
	if (ret != 0) {
		printf("HMAC verification failed, abort PID update\n");
		return -1;
	}

	char *src = (char *)pid_load_addr;
	size_t src_max_len = pid_size;
	char *line_end = src;

	while (line_end < src + src_max_len && *line_end != '\n' && *line_end != '\r')
		line_end++;

	char *start = src;

	while (start < line_end && isspace((unsigned char)*start))
		start++;

	char *end = line_end - 1;

	while (end > start && isspace((unsigned char)*end))
		end--;

	size_t valid_len = (start <= end) ? (end - start + 1) : 0;

	if (valid_len > sizeof(pid) - 1)
		valid_len = sizeof(pid) - 1;

	if (valid_len > 0)
		memcpy(pid, start, valid_len);

	pid[valid_len] = '\0';

	printf("PID value from package: [%s]\n", pid);

	/* Write PID to key storage */
	ret = key_unify_write("model_name", pid, strlen(pid) + 1);
	if (ret == 0) {
		printf("write pid ok, actual pid: [%s]\n", pid);

		env_set("model_name", pid);
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p model_name;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif
	} else {
		printf("write pid failed, actual pid: [%s]\n", pid);
	}

	__hdle hUiProgress = NULL;
	int rcode = 0;

	if (video_res_prepare_for_upgrade(NULL)) {
		printf("Fail when prepare bm res or init video for upgrade\n");
		goto _update;
	}
	show_logo_to_report_burning();

	hUiProgress = optimus_progress_ui_request_for_sdc_burn();
	if (!hUiProgress) {
		printf("request progress handle failed!\n");
		goto _update;
	}
	optimus_progress_ui_direct_update_progress(hUiProgress, UPGRADE_STEPS_AFTER_DISK_INIT_OK);
	rcode = optimus_progress_ui_set_smart_mode(hUiProgress, 200,
			UPGRADE_STEPS_AFTER_BURN_BOOTLOADER_OK - UPGRADE_STEPS_AFTER_DISK_INIT_OK);
	if (rcode) {
		printf("Fail to set smart mode\n");
		goto _update;
	}
	if (show_flag == 1)
		run_command("sleep 6", 0);
	optimus_progress_ui_update_by_bytes(hUiProgress, 200);

_update:
	if (hUiProgress)
		optimus_progress_ui_report_upgrade_stat(hUiProgress, !rcode);
	if (hUiProgress)
		optimus_progress_ui_release(hUiProgress);

	return ret;
}

/**
 * parse_image_names_and_update - Parse image_name from ini file and update
 * partitions
 *
 * This function reads the aml_sdc_burn.ini file from USB disk, parses the
 * "image_name" line (format: image_name = oem_udisk.img:odm_ext_udisk.img:...),
 * extracts partition names (removing .img suffix), and calls
 * check_partition_update for each.
 *
 * @odm_ext_flag: Output parameter, set to 0 if odm_ext was successfully updated
 * @logo_update_flag: Output parameter, set to 0 if oem or production_ext was
 * successfully updated
 *
 * Return: Number of partitions successfully updated, or -1 on error
 */
int parse_image_names_and_update(int *odm_ext_flag, int *logo_update_flag, int *oem_flag)
{
	const char *ini_file = "aml_sdc_burn.ini";
	char cmd[256] = {0};
	char *ini_buffer = NULL;
	char *line_start, *line_end, *value_start;
	char partition_names[MAX_IMAGE_NAMES][MAX_IMAGE_NAME_LEN];
	int partition_count = 0;
	int update_count = 0;
	int ret = -1;
	ulong loadaddr;
	ulong file_size;

	/* Initialize output flags to -1 (not updated) */
	if (odm_ext_flag)
		*odm_ext_flag = -1;
	if (logo_update_flag)
		*logo_update_flag = -1;
	if (oem_flag)
		*oem_flag = -1;

	loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);

	/* Check if ini file exists and get its size */
	ret = run_command("usb start 0", 0);
	if (ret != 0) {
		printf("USB start failed\n");
		return -1;
	}

	env_set_hex("filesize", 0);
	sprintf(cmd, "fatsize usb 0 %s", ini_file);
	ret = run_command(cmd, 0);
	file_size = env_get_hex("filesize", 0);
	if (file_size == 0) {
		printf("%s not found or empty\n", ini_file);
		return -1;
	}

	/* Limit the read size to avoid large allocations */
	if (file_size > INI_BUFFER_SIZE)
		file_size = INI_BUFFER_SIZE;

	/* Load ini file content */
	sprintf(cmd, "fatload usb 0 %lx %s %lx", loadaddr, ini_file, file_size);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("Failed to load %s\n", ini_file);
		return -1;
	}

	ini_buffer = (char *)loadaddr;
	ini_buffer[file_size] = '\0';

	/* Search for image_name line */
	line_start = ini_buffer;
	while (line_start < ini_buffer + file_size) {
		/* Find end of current line */
		line_end = line_start;
		while (line_end < ini_buffer + file_size && *line_end != '\n' &&
			*line_end != '\r')
			line_end++;

		/* Temporarily null terminate the line */
		char saved_char = *line_end;
		*line_end = '\0';

		/* Check if this line starts with "image_name" (skip whitespace) */
		char *check = line_start;
		while (*check && isspace((unsigned char)*check))
			check++;

		if (strncmp(check, "image_name", 10) == 0) {
			/* Find '=' sign */
			value_start = strchr(check, '=');
			if (value_start) {
				value_start++; /* Skip '=' */

				/* Skip whitespace after '=' */
				while (*value_start && isspace((unsigned char)*value_start))
					value_start++;

				/* Parse separated image names */
				char *token_start = value_start;
				char *token_end;

				while (*token_start && partition_count < MAX_IMAGE_NAMES) {
					/* Skip leading whitespace */
					while (*token_start && isspace((unsigned char)*token_start))
						token_start++;

					if (*token_start == '\0')
						break;

					/* Find comma or end of string */
					token_end = token_start;
					while (*token_end && *token_end != ':')
						token_end++;

					/* Copy token (image filename) */
					size_t token_len = token_end - token_start;

					/* Trim trailing whitespace */
					while (token_len > 0 &&
						isspace((unsigned char)token_start[token_len - 1]))
						token_len--;

					if (token_len > 0 && token_len < MAX_IMAGE_NAME_LEN) {
						char img_name[MAX_IMAGE_NAME_LEN];

						memcpy(img_name, token_start, token_len);
						img_name[token_len] = '\0';

						/* Remove .img suffix to get partition name */
						char *suffix = strstr(img_name, ".img");

						if (suffix && suffix[4] == '\0')
							*suffix = '\0';

						if (strncmp(img_name, "pid_", 4) == 0) {
							printf("Skipping PID file in image_name: %s (use pid_name "
								"instead)\n",
								img_name);
						} else {
							/* Store partition name */
							strncpy(partition_names[partition_count], img_name,
								MAX_IMAGE_NAME_LEN - 1);
							partition_names[partition_count][MAX_IMAGE_NAME_LEN - 1] = '\0';

							printf("Found partition to update: %s\n",
								partition_names[partition_count]);
							partition_count++;
						}
					}

					/* Move to next token */
					if (*token_end == ':')
						token_start = token_end + 1;
					else
						break;
				}
			}
			*line_end = saved_char;
			break;
		}

		*line_end = saved_char; /* Restore */

		/* Move to next line */
		line_start = line_end;
		while (line_start < ini_buffer + file_size &&
			(*line_start == '\n' || *line_start == '\r'))
			line_start++;
	}

	if (partition_count == 0) {
		printf("No image_name entry found in %s\n", ini_file);
		return 0;
	}

	/* Call check_partition_update for each partition */
	printf("Updating %d partitions from ini file...\n", partition_count);
	for (int i = 0; i < partition_count; i++) {
		printf("Checking partition update for: %s\n", partition_names[i]);
		ret = check_partition_update(partition_names[i]);
		if (ret == 0) {
			printf("Successfully updated partition: %s\n", partition_names[i]);
			update_count++;

			/* Set appropriate flag based on partition name prefix */
			if (strncmp(partition_names[i], "odm_ext", 7) == 0 ||
				strncmp(partition_names[i], "tvconfig_udisk", 14) == 0) {
				if (odm_ext_flag)
					*odm_ext_flag = 0;
			} else if (strncmp(partition_names[i], "oem", 3) == 0 ||
					strncmp(partition_names[i], "production_ext", 14) == 0 ||
					strncmp(partition_names[i], "bootloader_udisk", 16) == 0 ||
					strncmp(partition_names[i], "skw_udisk", 9) == 0) {
				if (logo_update_flag)
					*logo_update_flag = 0;
			}

			if (strncmp(partition_names[i], "oem", 3) == 0)
				if (oem_flag)
					*oem_flag = 0;
		} else {
			printf("Partition %s update skipped or failed\n", partition_names[i]);
		}
	}

	return update_count;
}

typedef struct {
	const char *bmp_filename;
	const char *env_prefix;
} BmpFileInfo;

static const BmpFileInfo bmp_files_list[] = {
	{"upgrade_error.bmp",     "upgrade_error"},
	{"upgrade_bar.bmp",       "upgrade_bar"},
	{"upgrade_fail.bmp",      "upgrade_fail"},
	{"upgrade_unfocus.bmp",   "upgrade_unfocus"},
	{"upgrade_success.bmp",   "upgrade_success"},
	{"upgrade_upgrading.bmp", "upgrade_upgrading"}
};

#define BMP_FILES_CNT  (sizeof(bmp_files_list) / sizeof(BmpFileInfo))

int prepare_display(void)
{
	char cmd[256] = {0};
	static const char *bmpsLoadAddr = (const char *)UpgradeLogo_Addr;
	char partname[PARTNAME_MAX_LEN] = {0};
	char env_data[PARTNAME_MAX_LEN] = {0};
	char *slot_name;
	const char *suffix = NULL;
	const char *board_logo_part = env_get("board_logo_part");
	int ret = 0;
	int partIndex;
	ulong file_size = 0;
	int i;
	unsigned long current_load_addr = (unsigned long)bmpsLoadAddr;

	if (board_logo_part && *board_logo_part != '\0')
		strcpy((char *)partname, board_logo_part);
	else
		strcpy((char *)partname, "oem");
	partname[PARTNAME_MAX_LEN - 1] = '\0';

	run_command("printenv bootLogoPart;", 0);

	partIndex = get_partition_num_by_name(env_get("bootLogoPart"));
	printf("partIndex = %d\n", partIndex);
	if (partIndex <= 0) {
		printf("fail find part index for name(%s), try ab\n", env_get("bootLogoPart"));
		slot_name = env_get("slot-suffixes");
		if (slot_name && (strcmp(slot_name, "0") == 0))
			suffix = "_a";
		else if (slot_name && (strcmp(slot_name, "1") == 0))
			suffix = "_b";

		if (suffix != NULL) {
			size_t remaining = PARTNAME_MAX_LEN - strlen(partname) - 1;

			if (remaining > 0)
				strncat(partname, suffix, remaining);
			else
				return -1;
		}
		partIndex = get_partition_num_by_name(partname);
		if (partIndex < 0) {
			printf("Error: get partition index failed\n");
			return -1;
		}
	}

	for (i = 0; i < BMP_FILES_CNT; i++) {
		const BmpFileInfo *bmp_info = &bmp_files_list[i];

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "ext4size mmc 1:0x%x /logo_files/%s",
				partIndex, bmp_info->bmp_filename);
		debug("Execute cmd: %s\n", cmd);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Error: run ext4size for %s failed (ret=%d)\n",
				   bmp_info->bmp_filename, ret);
			return -1;
		}

		file_size = env_get_hex("filesize", 0);
		if (file_size == 0) {
			printf("Error: get %s size failed (size=0)\n", bmp_info->bmp_filename);
			return -1;
		}
		printf("%s size: 0x%lx\n", bmp_info->bmp_filename, file_size);

		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "ext4load mmc 1:0x%x 0x%lx /logo_files/%s",
				partIndex, current_load_addr, bmp_info->bmp_filename);
		debug("Execute cmd: %s\n", cmd);
		ret = run_command(cmd, 0);
		if (ret != 0) {
			printf("Error: run ext4load for %s failed (ret=%d)\n",
				   bmp_info->bmp_filename, ret);
			return -1;
		}

		memset(env_data, 0, sizeof(env_data));
		sprintf(env_data, "0x%lx", current_load_addr);
		snprintf(cmd, sizeof(cmd), "%s_offset", bmp_info->env_prefix);
		ret = env_set(cmd, env_data);
		if (ret != 0) {
			printf("Error: set %s failed\n", cmd);
			return -1;
		}
		printf("Set %s = %s\n", cmd, env_data);

		memset(env_data, 0, sizeof(env_data));
		sprintf(env_data, "0x%lx", file_size);
		snprintf(cmd, sizeof(cmd), "%s_size", bmp_info->env_prefix);
		ret = env_set(cmd, env_data);
		if (ret != 0) {
			printf("Error: set %s failed\n", cmd);
			return -1;
		}
		printf("Set %s = %s\n", cmd, env_data);

		current_load_addr += file_size;
		env_set_hex("filesize", 0);

		env_set("read_logo_oem", "true");
	}

	printf("All BMP files load success!\n");
	return 0;
}
#endif

static int is_target_rebootmode(const char *rebootmode)
{
	const char *targets[] = {"cold_boot", "turn_secure"};
	for (size_t i = 0; i < sizeof(targets)/sizeof(targets[0]); i++) {
		if (rebootmode && !strcmp(rebootmode, targets[i])) {
			return 1;
		}
	}
	return 0;
}

static int validate_udisk_files(usb_upgrade_files_t *files,
		int *has_autoupgrade, int *has_update)
{
	int ret = 0;
	char cmd[256];
	ulong loadaddr = (ulong)env_get_hex("loadaddr", 0x4000);
	ulong tmp_addr = loadaddr - 0x1000;

	/* Check aml_autoupgrade */
	sprintf(cmd, "fatload usb 0 %lx aml_autoupgrade 1", tmp_addr);
	if (run_command(cmd, 0) == 0)
		*has_autoupgrade = 1;
	else
		*has_autoupgrade = 0;

	/* Check aml_sdc_burn.ini */
	sprintf(cmd, "fatload usb 0 %lx aml_update 1", tmp_addr);
	if (run_command(cmd, 0) == 0)
		*has_update = 1;
	else
		*has_update = 0;

	printf("aml_autoupgrade=%d, aml_update=%d\n",
		*has_autoupgrade, *has_update);

	if ((!*has_autoupgrade && !*has_update) ||
		(*has_autoupgrade && *has_update)) {
		printf("Invalid, skip USB upgrade\n");
		return -1;
	}

	/* Scan USB for upgrade files */
	ret = scan_usb_upgrade_files(files);
	if (ret != 0) {
		printf("Failed to scan USB files\n");
		return -1;
	}

	if (*has_autoupgrade) {
		if (files->big_pkg_count > 1) {
			printf("ERROR: Multiple big packages found (%d)\n", files->big_pkg_count);
			return -1;
		}
		if (files->pid_count > 1) {
			printf("ERROR: Multiple PID files found (%d)\n", files->pid_count);
			return -1;
		}
		if (files->big_pkg_count == 0 && files->small_pkg_count == 0 &&
			files->pid_count == 0) {
			printf("No upgrade files found\n");
			return -1;
		}
	} else if (*has_update) {
		int total_files = files->big_pkg_count +
			files->small_pkg_count + files->pid_count;
		printf("=== Single-file upgrade mode (aml_update) ===\n");
		printf("Total upgrade files found: %d\n", total_files);

		if (total_files != 1) {
			printf("ERROR: Single-file mode requires exactly 1 file, found %d\n",
				total_files);
			return -1;
		}
	}

	return 0;
}

static int do_udisk_check(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
#ifdef CONFIG_REPLACE_TV_MODE
	usb_upgrade_files_t files;
	int has_autoupgrade = 0;
	int has_update = 0;
	char *rebootmode = env_get("reboot_mode");
	char *usb_burn_state = env_get("usb_burn_state");

	env_set("udisk_check", "0");

	if (argc > 1)
		return CMD_RET_USAGE;

	printf("rebootmode is %s\n", rebootmode ? rebootmode : "NULL");
	printf("usb_burn_state is %s\n", usb_burn_state ? usb_burn_state : "NULL");

	int is_cold_boot = is_target_rebootmode(rebootmode);
	int is_after_big_pkg = (usb_burn_state && !strcmp(usb_burn_state, "2"));

	if (is_cold_boot || is_after_big_pkg) {
		ret = run_command("usb start 0", 0);
		if (ret != 0) {
			printf("USB start failed\n");
			return CMD_RET_FAILURE;
		}

		ret = validate_udisk_files(&files, &has_autoupgrade, &has_update);
		if (ret != 0)
			return CMD_RET_FAILURE;

		printf("U-disk check passed: Valid upgrade files found.\n");
		env_set("jpg_logo", "false");
		env_set("udisk_check", "true");
	}
#endif

	return CMD_RET_SUCCESS;
}

static int do_udisk_burn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	int odm_ext_flag = -1;
	int pid_flag = -1;
	int logo_update_flag = -1;
	int oem_flag = -1;

	if (argc > 1)
		return CMD_RET_USAGE;

#ifdef CONFIG_REPLACE_TV_MODE
	char *rebootmode = env_get("reboot_mode");
	char *usb_burn_state = env_get("usb_burn_state");
	char *udiskcheck = env_get("udisk_check");

	printf("rebootmode is %s\n", rebootmode ? rebootmode : "NULL");
	printf("usb_burn_state is %s\n", usb_burn_state ? usb_burn_state : "NULL");
	printf("udiskcheck is %s\n", udiskcheck ? udiskcheck : "NULL");

	int is_cold_boot = is_target_rebootmode(rebootmode);
	int is_after_big_pkg = (usb_burn_state && !strcmp(usb_burn_state, "2"));

	if ((is_cold_boot || is_after_big_pkg) && udiskcheck &&
		!strcmp(udiskcheck, "true")) {
		int has_autoupgrade = 0;
		int has_update = 0;
		usb_upgrade_files_t files;
		int i;

		env_set("usb_burn_state", "0");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p usb_burn_state;", 0);
#else
		run_command("defenv_reserv; saveenv;", 0);
#endif

		ret = run_command("usb start 0", 0);
		if (ret != 0) {
			printf("USB start failed, skip USB upgrade\n");
			return ret;
		}

		/* Re-validate files before burning */
		ret = validate_udisk_files(&files, &has_autoupgrade, &has_update);
		if (ret != 0) {
			printf("Validation failed during burn phase\n");
			return ret;
		}

		env_set("jpg_logo", "false");
		prepare_display();

		if (has_autoupgrade) {
			/*
			 * Multi-file upgrade mode (aml_autoupgrade)
			 * Case 1: big_pkg > 1 -> abort
			 * Case 2: pid > 1 -> abort
			 * Case 3: big_pkg 0-1, pid 0-1, small_pkg 1-3 -> upgrade
			 */
			printf("=== Multi-file upgrade mode (aml_autoupgrade) ===\n");

			/* Step 1: Upgrade big package (if exists) */
			if (files.big_pkg_count == 1 && !is_after_big_pkg) {
				printf("Upgrading big package: %s\n", files.big_pkg_name);
				check_udisk_burn_fw(&files);
			}

			/* Step 2: Upgrade small packages */
			for (i = 0; i < files.small_pkg_count; i++) {
				char part_name[MAX_FILENAME_LEN];

				strncpy(part_name, files.small_pkg_names[i], MAX_FILENAME_LEN - 1);
				/* Remove .img suffix */
				char *suffix = strstr(part_name, ".img");

				if (suffix)
					*suffix = '\0';

				printf("Upgrading small package: %s\n", part_name);
				ret = check_partition_update(part_name);
				if (ret == 0) {
					printf("Successfully updated: %s\n", files.small_pkg_names[i]);
					if (strncmp(part_name, "tvconfig_udisk", 14) == 0)
						odm_ext_flag = 0;
					else if (strncmp(part_name, "oem_udisk", 9) == 0 ||
						strncmp(part_name, "skw_udisk", 9) == 0 ||
						strncmp(part_name, "vbmeta_udisk", 12) == 0 ||
						strncmp(part_name, "bootloader_udisk", 16) == 0)
						logo_update_flag = 0;

					if (strncmp(part_name, "oem_udisk", 9) == 0 &&
						files.big_pkg_count == 0)
						oem_flag = 0;
				}
				run_command("sleep 3", 0);
			}

			/* Step 3: Upgrade PID (if exists) */
			if (files.pid_count == 1) {
				printf("Upgrading PID: %s\n", files.pid_name);
				pid_flag = check_pid_update_udisk(files.pid_name, 1);
				run_command("sleep 3", 0);
			}

		} else if (has_update) {
			/*
			 * Single-file upgrade mode (aml_update)
			 * Only upgrade if exactly ONE file from the 10 known files exists
			 */

			printf("=== Single-file upgrade mode (aml_update) ===\n");

			if (files.big_pkg_count == 1 && !is_after_big_pkg) {
				printf("Upgrading single big package: %s\n", files.big_pkg_name);
				check_udisk_burn_fw(&files);
			} else if (files.small_pkg_count == 1) {
				char part_name[MAX_FILENAME_LEN];

				strncpy(part_name, files.small_pkg_names[0], MAX_FILENAME_LEN - 1);
				char *suffix = strstr(part_name, ".img");

				if (suffix)
					*suffix = '\0';

				printf("Upgrading single small package: %s\n", part_name);
				ret = check_partition_update(part_name);
				if (ret == 0) {
					if (strncmp(part_name, "tvconfig_udisk", 14) == 0)
						odm_ext_flag = 0;
					else if (strncmp(part_name, "oem_udisk", 9) == 0 ||
						strncmp(part_name, "skw_udisk", 9) == 0 ||
						strncmp(part_name, "vbmeta_udisk", 12) == 0 ||
						strncmp(part_name, "bootloader_udisk", 16) == 0)
						logo_update_flag = 0;

					if (strncmp(part_name, "oem_udisk", 14) == 0)
						oem_flag = 0;
				}
			} else if (files.pid_count == 1) {
				printf("Upgrading single PID: %s\n", files.pid_name);
				pid_flag = check_pid_update_udisk(files.pid_name, 1);
				run_command("sleep 3", 0);
			}
		}

		/* Post-upgrade actions */
		printf(
			"Upgrade results: odm_ext_flag=%d, pid_flag=%d, logo_update_flag=%d\n",
			odm_ext_flag, pid_flag, logo_update_flag);

		if (oem_flag == 0) {
			printf("update oem, set env\n");
			env_set("oem_update_flag", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p oem_update_flag;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
		}

		if (odm_ext_flag == 0 || pid_flag == 0) {
			printf("update odm_ext or pid, erase param\n");
			run_command("store erase param 0 0", 0);

			env_set("ini_update_flag", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p ini_update_flag;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif

			env_set("powermode_ini", "1");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p powermode_ini;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
		} else if (logo_update_flag == 0) {
			printf("update logo\n");
			run_command("sleep 2", 0);
		}
		run_command("reboot", 0);
	}
#endif
	return ret;
}

U_BOOT_CMD
(udisk_check, 2, 0, do_udisk_check,
	"check valid update files on udisk",
	" "
);

U_BOOT_CMD
(udisk_burn, 2, 0, do_udisk_burn,
	"perform udisk upgrade",
	" "
);

