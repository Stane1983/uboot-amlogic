// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <hexdump.h>
#include <mmc.h>
#include <asm/amlogic/arch/efuse.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_efuse.h>
#include <u-boot/crc.h>

#ifdef CONFIG_AML_PROVISION_UBOOT
#define AML_MAX_KEY_NAME 32
#define AML_MAX_EFUSE_KEYS 8
#define AML_MAX_EFUSE_KEY_BLOCKS (1)
#define MMC_BLOCK_SIZE (512)

#if CONFIG_AML_PROVISION_UBOOT_NUM == 1 ||CONFIG_AML_PROVISION_UBOOT_NUM == 2
#define AML_BOOT_BOOT_NUM CONFIG_AML_PROVISION_UBOOT_NUM
#else
#define AML_BOOT_BOOT_NUM 2
#error "CONFIG_AML_PROVISION_UBOOT_NUM is not correct, use 2 as default"
#endif

#if CONFIG_AML_PROVISION_UBOOT_OFFSET < 5 || CONFIG_AML_PROVISION_UBOOT_OFFSET > 504
    #error "CONFIG_AML_PROVISION_UBOOT_OFFSET is out of range, just default:300"
	#define AML_CUSTOM_OFFSET 300
#else
	#define AML_CUSTOM_OFFSET CONFIG_AML_PROVISION_UBOOT_OFFSET
#endif

#define AML_EFUSE_KEY_BLOCK_START (AML_MAX_EFUSE_KEYS*AML_MAX_EFUSE_KEY_BLOCKS + AML_CUSTOM_OFFSET)

#define AML_KEY_HASH_NORMAL_MAGIC (0x484e4452)  /*HNDR*/
#define AML_KEY_HASH_DFU_MAGIC (0x48444452)  /*HDDR*/
#define AML_KEY_DGPK1_MAGIC (0x44475031)  /*DGP1*/
#define AML_KEY_DVGK_MAGIC (0x4456474b)  /*DVGK*/
#define AML_KEY_DVUK_MAGIC (0x4456554b)  /*DVUK*/
#define AML_KEY_AUDIO_VENDOR_ID_MAGIC (0x41564944)  /*AVID*/

static const struct {
	const char *name;
	u64 offset;
	unsigned int magic;
} aml_key_map[AML_MAX_EFUSE_KEYS] = {
	[0] = {
		.name = "HASH_NORMAL_DEVICE_ROOTCERT",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 0*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_HASH_NORMAL_MAGIC
	},
	[1] = {
		.name = "HASH_DFU_DEVICE_ROOTCERT",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 1*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_HASH_DFU_MAGIC
	},
	[2] = {
		.name = "DGPK1",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 2*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_DGPK1_MAGIC
	},
	[3] = {
		.name = "DVGK",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 3*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_DVGK_MAGIC
	},
	[4] = {
		.name = "DVUK",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 4*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_DVUK_MAGIC
	},
	[5] = {
		.name = "AUDIO_VENDOR_ID",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 5*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = AML_KEY_AUDIO_VENDOR_ID_MAGIC
	},
	[6] = {
		.name = "reserved",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 6*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = 0
	},
	[7] = {
		.name = "reserved",
		.offset = (AML_EFUSE_KEY_BLOCK_START - 7*AML_MAX_EFUSE_KEY_BLOCKS) * MMC_BLOCK_SIZE,
		.magic = 0
	},
};

#define AML_MAX_FEAT_ENABLE 6
char aml_enable_efuse[AML_MAX_FEAT_ENABLE][64] = {
	"FEAT_ENABLE_DEVICE_ROOT_PUBRSA_PROT",
	"FEAT_ENABLE_DEVICE_LVL1_PUBRSA_PROT",
	"FEAT_ENABLE_DEVICE_LVLX_PUBRSA_PROT",
	"FEAT_ENABLE_DEVICE_VENDOR_SIG",
	"FEAT_ENABLE_DEVICE_PROT",
	"FEAT_ENABLE_DEVICE_SCS_SIG"
};

#define AML_KEY_FLAG_ENCRYPTED 0x1
#define AML_KEY_FLAG_BURNED 0x2
typedef struct __aml_prov_key {
	unsigned int magic;
	unsigned short flags;
	unsigned short len;
	unsigned int crc32;
	unsigned char data[0];
} aml_prov_key_t;

static unsigned char s_key_buf[MMC_BLOCK_SIZE * AML_MAX_EFUSE_KEY_BLOCKS];

static int aml_find_key_map(const char *name)
{
	int i = 0;

	if (NULL == name)
		return -1;

	for (i = 0; i < AML_MAX_EFUSE_KEYS; i++)
	{
		if (0 == strncmp(name, aml_key_map[i].name, AML_MAX_KEY_NAME))
			return i;
	}
	return -1;
}

static int aml_read_key_from_boot(unsigned char *pbuf, u64 offset)
{
	struct mmc *mmc;
	int nread = 0;
	u64 block_offset = 0;

	printf("%s read data from boot%d\n", __func__, (AML_BOOT_BOOT_NUM - 1));

	if (!pbuf) {
		printf("Null pbuf used in %s\n", __func__);
		return -1;
	}

	mmc = find_mmc_device(1);
	if (!mmc) {
		printf("no mmc devices available\n");
		return -1;
	}

	/* switch to boot partition */
	if (mmc_switch_part(mmc, AML_BOOT_BOOT_NUM) != 0)
	{
		printf("ERROR: couldn't switch to boot partition\n");
		return -1;
	}

	block_offset = mmc->capacity - offset;
	printf("%s block_offset=%lx, capacity=%lx\n", __func__, (long)block_offset, (long)mmc->capacity);
#ifndef CONFIG_BLK
	nread = mmc->block_dev.block_read(AML_BOOT_BOOT_NUM,
					block_offset / MMC_BLOCK_SIZE,
					AML_MAX_EFUSE_KEY_BLOCKS, pbuf);
#else
	nread = blk_dread(mmc_get_blk_desc(mmc),
						block_offset / MMC_BLOCK_SIZE,
						AML_MAX_EFUSE_KEY_BLOCKS, pbuf);
#endif

	/* switch to user partition */
	if (mmc_switch_part(mmc, 0) != 0)
	{
		printf("ERROR: couldn't switch to user partition\n");
		return -1;
	}

	if (nread < 0) {
		printf("ERROR: read failure, nread %d\n", nread);
		return -1;
	}

	return 0;
}

static int aml_write_key_to_boot(const unsigned char *pbuf, u64 offset)
{
	struct mmc *mmc;
	int nwrite = 0;
	u64 block_offset = 0;

	printf("%s write data into boot%d\n", __func__, (AML_BOOT_BOOT_NUM - 1));

	if (!pbuf) {
		printf("Null pbuf used in %s\n", __func__);
		return -1;
	}

	mmc = find_mmc_device(1);
	if (!mmc) {
		printf("no mmc devices available\n");
		return -1;
	}

	/* switch to boot partition */
	if (mmc_switch_part(mmc, AML_BOOT_BOOT_NUM) != 0)
	{
		printf("ERROR: couldn't switch to boot partition\n");
		return -1;
	}

	block_offset = mmc->capacity - offset;
	printf("%s block_offset=%lx, capacity=%lx\n", __func__, (long)block_offset, (long)mmc->capacity);
#ifndef CONFIG_BLK
	nwrite = mmc->block_dev.block_write(AML_BOOT_BOOT_NUM,
					block_offset / MMC_BLOCK_SIZE,
					AML_MAX_EFUSE_KEY_BLOCKS, pbuf);
#else
	nwrite = blk_dwrite(mmc_get_blk_desc(mmc),
					block_offset/MMC_BLOCK_SIZE,
					AML_MAX_EFUSE_KEY_BLOCKS, pbuf);
#endif

	/* switch to user partition */
	if (mmc_switch_part(mmc, 0) != 0)
	{
		printf("ERROR: couldn't switch to user partition\n");
		return -1;
	}

	if (nwrite < 0) {
		printf("ERROR: write failure, nwrite %d\n", nwrite);
		return -1;
	}

	return 0;
}

static int aml_verify_key_data(aml_prov_key_t *key, unsigned int magic)
{
	unsigned int data_crc = 0;

	if (!key)
		return -1;

	printf("%s magic=0x%x, crc32=0x%x, len:%d, flags:%d\n", __func__, key->magic, key->crc32, key->len, key->flags);
	if (key->magic != magic) {
		printf("%s wrong magic=0x%x, should be 0x%x\n", __func__, key->magic, magic);
		return -1;
	}

	if (key->len == 0 || key->crc32 == 0) {
		printf("%s len=%d, crc32=0x%x\n", __func__, key->len, key->crc32);
		return -1;
	}

	data_crc = crc32(0, key->data, key->len);
	if (key->crc32 != data_crc) {
		printf("%s crc=0x%x, data crc=0x%x\n", __func__, key->crc32, data_crc);
		return -1;
	}
	printf("%s verify data successfully\n", __func__);
	return 0;
}

static int aml_clear_key_data(int map_id)
{
	int ret = 0;
	aml_prov_key_t *key = NULL;
	char *efuse_debug = env_get("provision_efuse_debug");

	if (map_id < 0 ||  map_id > sizeof(aml_key_map)/sizeof(aml_key_map[0]))
		return -1;

	printf("%s clear data offset:%lld, name:%s\n", __func__, aml_key_map[map_id].offset, aml_key_map[map_id].name);

	if (efuse_debug && strncmp(efuse_debug, "1", 1) == 0 ) {
		printf("%s do not clear data in debug mode\n", __func__);
		return 0;
	}

	memset(s_key_buf, 0, MMC_BLOCK_SIZE * AML_MAX_EFUSE_KEY_BLOCKS);

	key = (aml_prov_key_t *)s_key_buf;
	key->flags = AML_KEY_FLAG_BURNED;
	key->len = 0;
	key->crc32 = 0;
	key->magic = aml_key_map[map_id].magic;
	ret = aml_write_key_to_boot(s_key_buf, aml_key_map[map_id].offset);

	return ret;
}


#define AML_CMD_BUF_LEN 256
char g_aml_cmd_buf[AML_CMD_BUF_LEN] = { 0 };

static void aml_execute_efuse_cmd(int run)
{
	char *efuse_debug = env_get("provision_efuse_debug");

	printf("efuse cmd:%s\n",g_aml_cmd_buf);
	if (!efuse_debug || strncmp(efuse_debug, "1", 1) || run) {
		run_command(g_aml_cmd_buf, 0);
		printf("%s run efuse_obj cmd finished!\n", __func__);
	} else {
		printf("%s run efuse_obj cmd, not execute in debug mode!\n", __func__);
	}
}

/*return 0 -non-existent, return 1 --exist*/
static int aml_get_efuse_obj(const char *name, int lock)
{
	int val = 0;

	memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
	if (!name)
		return 0;

	if (lock)
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "%s %s\n", "efuse_obj get_lock", name);
	else
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "%s %s\n", "efuse_obj get", name);

	aml_execute_efuse_cmd(1);

	val = (int)efuse_field.data[0];
	printf("%s efuse:%s,%d: %s\n", __func__, name, val, val ? "exist" : "non-existent");
	return val;
}

static int aml_set_efuse_obj(const char *name, aml_prov_key_t *key)
{
	char *hex_data_p = NULL;
	char *hex_data = NULL;
	int key_data_len = key->len * 2 + 1;

	hex_data = hex_data_p = (char *)malloc(key_data_len);
	if (!hex_data)
		return -1;

	memset(hex_data_p, 0, key_data_len);
	hex_data_p = bin2hex(hex_data_p, key->data, key->len);
	*hex_data_p = '\0';

	printf("%s,hex data len:%lu\n", __func__, strlen(hex_data));

	if (key->flags & AML_KEY_FLAG_ENCRYPTED) {
		memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "efuse_obj set_enc %s %s", name, hex_data);
		aml_execute_efuse_cmd(0);

		memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "efuse_obj lock %s", name);
		aml_execute_efuse_cmd(0);
	} else {
		memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "efuse_obj set %s %s", name, hex_data);
		aml_execute_efuse_cmd(0);

		memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "efuse_obj lock %s", name);
		aml_execute_efuse_cmd(0);
	}

	free(hex_data);
	return 0;
}

static int aml_enable_efuse_obj(const char *name)
{
	int ret = 0;

	ret = aml_get_efuse_obj(name, 0);
	if (!ret) {
		memset(g_aml_cmd_buf, 0, sizeof(g_aml_cmd_buf));
		snprintf(g_aml_cmd_buf, AML_CMD_BUF_LEN, "efuse_obj set %s 01", name);
		aml_execute_efuse_cmd(0);
		return 0;
	} else {
		printf("%s %s is already enabled\n", __func__, name);
		return 1;
	}
}

/*just provision efuse which name is in support list, do not check if the key exists or not*/
int aml_provision_key(const char *name)
{
	int ret = -1;
	aml_prov_key_t *key = NULL;
	int map_id = -1;

	printf("%s provision start %s\n", __func__, name);

	map_id = aml_find_key_map(name);
	if (map_id < 0)
		return ret;

	memset(s_key_buf, 0, MMC_BLOCK_SIZE * AML_MAX_EFUSE_KEY_BLOCKS);

	ret = aml_read_key_from_boot(s_key_buf, aml_key_map[map_id].offset);
	/*verify data*/
	if (!ret) {
		key = (aml_prov_key_t *)s_key_buf;
		ret = aml_verify_key_data(key, aml_key_map[map_id].magic);
	} else {
		printf("%s aml_read_prov_key ret:%d\n", __func__, ret);
	}

	/*call provision API to burn*/
	if (!ret ) {
		ret = aml_set_efuse_obj(aml_key_map[map_id].name, key);
		if (ret)
			printf("%s write key ret:%d\n", __func__, ret);
		else
			aml_clear_key_data(map_id);
	} else {
		printf("%s provision key %s,ret:%d\n", __func__, name, ret);
		ret = -1;
	}

	return ret;
}

static int aml_provision_rootcert(void)
{
	int need_regen=0;
	int ret = 0;

	/*query if dgpk1 exists or not, 1 -- existed, 0 --non existed*/
	ret = aml_get_efuse_obj("HASH_NORMAL_DEVICE_ROOTCERT", 1);
	if (!ret) {
		aml_provision_key("HASH_NORMAL_DEVICE_ROOTCERT");
		printf("%s provision HASH_NORMAL_DEVICE_ROOTCERT \n", __func__);
		need_regen = 1;
	}
	ret = aml_get_efuse_obj("HASH_DFU_DEVICE_ROOTCERT", 1);
	if (!ret) {
		aml_provision_key("HASH_DFU_DEVICE_ROOTCERT");
		printf("%s provision HASH_DFU_DEVICE_ROOTCERT \n", __func__);
		need_regen = 1;
	}
	if (need_regen)
		run_command("ampk regen_enc_key", 0);

	return 0;
}

void aml_provision_dgpk1(int reboot)
{
	int ret = 0;
	int need_reboot = 0;

	aml_provision_rootcert();

	ret = aml_get_efuse_obj("DGPK1", 1);
	if (!ret) {
		aml_provision_key("DGPK1");
		printf("%s provision DGPK1 \n", __func__);
		need_reboot = 1;
	}
	if (need_reboot && reboot)
		run_command("reboot", 0);
}

void aml_enable_secureboot(int reboot)
{
	int ret = 0;
	int i = 0;
	int need_reboot = 0;

	aml_provision_rootcert();
	ret = aml_get_efuse_obj("DVGK", 1);
	if (!ret) {
		aml_provision_key("DVGK");
		printf("%s provision DVGK \n", __func__);
		need_reboot = 1;
	}
	ret = aml_get_efuse_obj("DVUK", 1);
	if (!ret) {
		aml_provision_key("DVUK");
		printf("%s provision DVUK \n", __func__);
		need_reboot = 1;
	}
	for (i = 0; i < AML_MAX_FEAT_ENABLE; i++) {
		ret = aml_enable_efuse_obj(aml_enable_efuse[i]);
		if (!ret)
			need_reboot = 1;
	}

	if (need_reboot && reboot)
		run_command("reboot", 0);
}

int do_provision_efuse(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	rc = aml_provision_key(argv[1]);
	if (!rc)
		rc = CMD_RET_SUCCESS;
	return rc;
}

static char provision_efuse_help_text[] =
	"<provision_efuse key_name>\n"
	"Support root key name including:HASH_NORMAL_DEVICE_ROOTCERT, \n"
	"HASH_DFU_DEVICE_ROOTCERT, DGPK1, DVGK, DVUK, AUDIO_VENDOR_ID\n";

U_BOOT_CMD(provision_efuse,	2,	0,	do_provision_efuse,
	"provision_efuse root keys stored in boot, do not check if the key exist or not",
	provision_efuse_help_text
);

int do_provision_dgpk1(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	int reboot_flag = 0;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	reboot_flag = simple_strtol(argv[1], NULL, 10);

	aml_provision_dgpk1(reboot_flag);
	rc = CMD_RET_SUCCESS;

	return rc;
}

static char provision_dgpk1_help_text[] =
	"<provision_dgpk1 reboot>\n"
	"will provision HASH_NORMAL_DEVICE_ROOTCERT and HASH_DFU_DEVICE_ROOTCERT first if they do not exist, \n"
	"and then provision DGPK1\n";


U_BOOT_CMD(provision_dgpk1,	2,	0,	do_provision_dgpk1,
	"provision_dgpk1 will provision dgp1 and root certs if it needed ",
	provision_dgpk1_help_text
);

int do_secureboot_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	int reboot_flag = 0;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	reboot_flag = simple_strtol(argv[1], NULL, 10);

	aml_enable_secureboot(reboot_flag);
	rc = CMD_RET_SUCCESS;

	return rc;
}

static char secureboot_enable_help_text[] =
	"<secureboot_enable reboot>\n"
	"will provision HASH_NORMAL_DEVICE_ROOTCERT and HASH_DFU_DEVICE_ROOTCERT first if they do not exist, \n"
	"and then enable secure boot\n";


U_BOOT_CMD(secureboot_enable,	2,	0,	do_secureboot_enable,
	"secureboot_enable will provision and enable efuses related to secure boot and root certs if it needed ",
	secureboot_enable_help_text
);

int do_provision_clear(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	int map_id = -1;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	map_id = aml_find_key_map(argv[1]);
	if (map_id < 0)
		return rc;

	rc = aml_clear_key_data(map_id);
	if (!rc)
		rc = CMD_RET_SUCCESS;
	return rc;
}

static char provision_clear_help_text[] =
	"<provision_clear key_name>\n"
	"Support root key name including:HASH_NORMAL_DEVICE_ROOTCERT, \n"
	"HASH_DFU_DEVICE_ROOTCERT, DGPK1, DVGK, DVUK, AUDIO_VENDOR_ID\n";

U_BOOT_CMD(provision_clear,	2,	0,	do_provision_clear,
	"provision_clear root keys stored in boot, do not check if the key exist or not",
	provision_clear_help_text
);

#endif /* CONFIG_AML_PROVISION_UBOOT */
