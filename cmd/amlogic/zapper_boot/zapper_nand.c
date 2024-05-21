#include <nand.h>
#include <mtd.h>
#include <command.h>
#include <amlogic/zapper_boot.h>
#include <amlogic/storage.h>
#include <amlogic/partition_encryption.h>
#include <../legacy-mtd-utils.h>
#include "LoaderCore/Include/ImportSPI/LoaderCoreSPI.h"
#include "LoaderCore/Src/Shared/ErrorReport.h"
#include "LoaderCore/Src/Shared/Bit.h"
#include "LoaderCore/Src/Shared/LoaderPartition.h"
#include "LoaderCore/Src/Shared/Crc.h"

static unsigned char flash_map_index = 0;
static unsigned long long hwconfig_start;	//hwconfig partition start address
static unsigned long long hwconfig_end;		//hwconfig partition end address

static unsigned long long ldflag_start;		//ldflag partition start address
static unsigned long long ldflag_end;		//ldflag partition end address

static unsigned long long ldsec_start;		//ldsec partition start address
static unsigned long long ldsec_end;		//ldsec partition end address

static unsigned long long kernel_start = 0x2240000;		//kernel partition start address
static unsigned long long kernel_end = 0x2e40000;		//kernel partition end address

//static unsigned long long kernel_header_start = 0x2240000;		//kernel partition start address
//static unsigned long long kernel_header_end = 0x2e40000;		//kernel partition end address



static unsigned char zapper_ldflag_partition[LDFLAG_LENGTH];
static unsigned char zapper_hwconfig_partition[HWCONFIG_LENGTH];
static unsigned char zapper_ldsec_partition[LDSEC_LENGTH];
static unsigned char zapper_kernel_partition[KERNEL_LENGTH];

static unsigned char zapper_kernel_header_partition[KERNEL_HEADER_LENGTH];

static struct Zapper_boot_info s_boot_info = {0};

static unsigned char *g_boot_partition = NULL;
static unsigned char *g_system_partition = NULL;
static unsigned char *g_casecure_partition = NULL;
static unsigned char *g_ccaconfig_partition = NULL;
static unsigned char *g_caverify_partition = NULL;

int Zapper_get_nand_hwconfig_partition_address(unsigned long long hwconfig_s, unsigned long long hwconfig_e)
{
	if (hwconfig_s <= 0 || hwconfig_e <= 0 || hwconfig_e < hwconfig_s) {
		return ZAPPER_ERROR;
	}

	hwconfig_start = hwconfig_s;
	hwconfig_end = hwconfig_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}


int Zapper_get_nand_ldflag_partition_address(unsigned long long ldflag_s, unsigned long long ldflag_e)
{
	if (ldflag_s <= 0 || ldflag_e <= 0 || ldflag_e < ldflag_s) {
		return ZAPPER_ERROR;
	}

	ldflag_start = ldflag_s;
	ldflag_end = ldflag_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldsec_partition_address(unsigned long long ldsec_s, unsigned long long ldsec_e)
{
	if (ldsec_s <= 0 || ldsec_e <= 0 || ldsec_e < ldsec_s) {
		return ZAPPER_ERROR;
	}

	ldsec_start = ldsec_s;
	ldsec_end = ldsec_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_kernel_partition_address(unsigned long long kernel_s, unsigned long long kernel_e)
{
	if (kernel_s <= 0 || kernel_e <= 0 || kernel_e < kernel_s) {
                return ZAPPER_ERROR;
	}

	kernel_start = kernel_s;
	kernel_end = kernel_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}
	memcpy((void *)p_s_e_boot_info->loader_partition_header, (void *)s_boot_info.loader_partition_header , LD_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->loader_partition, (void *)s_boot_info.loader_partition , LD_LENGTH);
	memcpy((void *)p_s_e_boot_info->error_code, (void *)s_boot_info.error_code , EC_LENGTH);
	p_s_e_boot_info->modify_flag = s_boot_info.modify_flag;
	p_s_e_boot_info->reboot_flag = s_boot_info.reboot_flag;
	p_s_e_boot_info->download_mode = s_boot_info.download_mode;
	p_s_e_boot_info->standby_flag = s_boot_info.standby_flag;
	p_s_e_boot_info->backupODU = s_boot_info.backupODU;
	memcpy((void *)p_s_e_boot_info->share_crc, (void *)s_boot_info.share_crc , LD_SHARE_DATA_CRC_LENGTH);

	for (int i = 0;i < LD_HEADER_LENGTH ; i++) {
		printf ("p_s_e_boot_info->loader_partition_header[%d] is %x\n", i, *(p_s_e_boot_info->loader_partition_header + i));
	}

	for (int i = 0;i < LD_LENGTH ; i++) {
		printf ("p_s_e_boot_info->loader_partition[%d] is %x\n", i, *(p_s_e_boot_info->loader_partition + i));
	}

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_hwconfig_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->bbcb_header, (void *)s_boot_info.bbcb_header , BBCB_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->bbcb, (void *)s_boot_info.bbcb , BBCB_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldsec_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->uk_header, (void *)s_boot_info.uk_header , UK_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->uk, (void *)s_boot_info.uk , UK_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_kernel_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->kernel_header, (void *)s_boot_info.kernel_header , KERNEL_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->kernel, (void *)s_boot_info.kernel , KERNEL_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_set_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}
	memcpy((void *)zapper_ldflag_partition, (void *)p_s_e_boot_info->loader_partition_header , LD_HEADER_LENGTH);
	memcpy((void *)zapper_ldflag_partition + LD_HEADER_LENGTH, (void *)p_s_e_boot_info->loader_partition , LD_LENGTH);
	memcpy((void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH , (void *)p_s_e_boot_info->error_code , EC_LENGTH);
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH] = p_s_e_boot_info->modify_flag;
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1] = p_s_e_boot_info->reboot_flag;
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1] = p_s_e_boot_info->download_mode;
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1] = p_s_e_boot_info->standby_flag;
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1 + 1] = p_s_e_boot_info->backupODU;
	memcpy((void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1 + 1 + 1 ,
		(void *)p_s_e_boot_info->share_crc, LD_SHARE_DATA_CRC_LENGTH);

	memcpy((void *)s_boot_info.loader_partition_header, (void *)zapper_ldflag_partition, LD_HEADER_LENGTH);
	memcpy((void *)s_boot_info.loader_partition, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH , LD_LENGTH);
	memcpy((void *)s_boot_info.error_code, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH, EC_LENGTH);
	s_boot_info.modify_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH];
	s_boot_info.reboot_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1];
	s_boot_info.download_mode = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1];
	s_boot_info.standby_flag= zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1];
	memcpy((void *)s_boot_info.share_crc, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1 + 1 + 1 ,
		LD_SHARE_DATA_CRC_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_read_verify_module_partition(void)
{
	int ret = ZAPPER_SUCCESS;
	unsigned long local_dec = 0;
	char *tmp_str = NULL;

	tmp_str = env_get("local_dec");
	if (tmp_str == NULL) {
		return -1;
	}

	strict_strtoul(tmp_str, 16, &local_dec);

	if (g_boot_partition == NULL) {
		g_boot_partition = malloc(BOOT_PARTITION_SIZE);
		if (g_boot_partition == NULL) {
			printf("[ZAPPER] malloc boot partition failed. \n");
			ret = ZAPPER_ERROR;
			goto err;
		}
	}
	ret = store_read(BOOT_PARTITION_NAME, 0, BOOT_PARTITION_SIZE, g_boot_partition);
	if (ret != 0) {
		printf("[ZAPPER] read boot partition failed. ret=0x%x\n", ret);
		goto err;
	}
	if (local_dec == 1) {
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		ret = part_dec(BOOT_PARTITION_NAME, g_boot_partition, BOOT_PARTITION_SIZE, g_boot_partition, BOOT_PARTITION_SIZE, 0);
		if (ret != 0) {
			printf("[ZAPPER] decrypt boot partition failed. ret=0x%x\n", ret);
			goto err;
		}
#endif
	}

	if (g_system_partition == NULL) {
		g_system_partition = malloc(SYSTEM_PARTITION_SIZE);
		if (g_system_partition == NULL) {
			printf("[ZAPPER] malloc system partition failed. \n");
			ret = ZAPPER_ERROR;
			goto err;
		}
	}
	ret = store_read(SYSTEM_PARTITION_NAME, 0, SYSTEM_PARTITION_SIZE, g_system_partition);
	if (ret != 0) {
		printf("[ZAPPER] read system partition failed. ret=0x%x\n", ret);
		goto err;
	}
	if (local_dec == 1) {
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		ret = part_dec(SYSTEM_PARTITION_NAME, g_system_partition, SYSTEM_PARTITION_SIZE, g_system_partition, SYSTEM_PARTITION_SIZE, 0);
		if (ret != 0) {
			printf("[ZAPPER] decrypt system partition failed. ret=0x%x\n", ret);
			goto err;
		}
#endif
	}

	if (g_casecure_partition == NULL) {
		g_casecure_partition = malloc(CASECURE_PARTITION_SIZE);
		if (g_casecure_partition == NULL) {
			printf("[ZAPPER] malloc casecure partition failed. \n");
			ret = ZAPPER_ERROR;
			goto err;
		}
	}
	ret = store_read(CASECURE_PARTITION_NAME, 0, CASECURE_PARTITION_SIZE, g_casecure_partition);
	if (ret != 0) {
		printf("[ZAPPER] read casecure partition failed. ret=0x%x\n", ret);
		goto err;
	}
	if (local_dec == 1) {
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		ret = part_dec(CASECURE_PARTITION_NAME, g_casecure_partition, CASECURE_PARTITION_SIZE, g_casecure_partition, CASECURE_PARTITION_SIZE, 0);
		if (ret != 0) {
			printf("[ZAPPER] decrypt casecure partition failed. ret=0x%x\n", ret);
			goto err;
		}
#endif
	}

	if (g_ccaconfig_partition == NULL) {
		g_ccaconfig_partition = malloc(CCACONFIG_PARTITION_SIZE);
		if (g_ccaconfig_partition == NULL) {
			printf("[ZAPPER] malloc ccaconfig partition failed. \n");
			ret = ZAPPER_ERROR;
			goto err;
		}
	}
	ret = store_read(CCACONFIG_PARTITION_NAME, 0, CCACONFIG_PARTITION_SIZE, g_ccaconfig_partition);
	if (ret != 0) {
		printf("[ZAPPER] read ccaconfig partition failed. ret=0x%x\n", ret);
		goto err;
	}

	if (g_caverify_partition == NULL) {
		g_caverify_partition = malloc(CAVERIFY_PARTITION_SIZE);
		if (g_caverify_partition == NULL) {
			printf("[ZAPPER] malloc caverify partition failed. \n");
			ret = ZAPPER_ERROR;
			goto err;
		}
	}
	ret = store_read(CAVERIFY_PARTITION_NAME, 0, CAVERIFY_PARTITION_SIZE, g_caverify_partition);
	if (ret != 0) {
		printf("[ZAPPER] read caverify partition failed. ret=0x%x\n", ret);
		goto err;
	}

err:
	if (ret != 0) {
		ERR_REPORT_SetErrorCode(ERROR_CODE_BOOT_CHECK_FAILED);
		if (g_caverify_partition != NULL) {
			free(g_caverify_partition);
			g_caverify_partition = NULL;
		}
		if (g_ccaconfig_partition != NULL) {
			free(g_ccaconfig_partition);
			g_ccaconfig_partition = NULL;
		}
		if (g_casecure_partition != NULL) {
			free(g_casecure_partition);
			g_casecure_partition = NULL;
		}
		if (g_system_partition != NULL) {
			free(g_system_partition);
			g_system_partition = NULL;
		}
		if (g_boot_partition != NULL) {
			free(g_boot_partition);
			g_boot_partition = NULL;
		}
	}

	return ret;
}

void Zapper_free_verify_module_partition(void)
{
	if (g_caverify_partition != NULL) {
		free(g_caverify_partition);
		g_caverify_partition = NULL;
	}
	if (g_ccaconfig_partition != NULL) {
		free(g_ccaconfig_partition);
		g_ccaconfig_partition = NULL;
	}
	if (g_casecure_partition != NULL) {
		free(g_casecure_partition);
		g_casecure_partition = NULL;
	}
	if (g_system_partition != NULL) {
		free(g_system_partition);
		g_system_partition = NULL;
	}
	if (g_boot_partition != NULL) {
		free(g_boot_partition);
		g_boot_partition = NULL;
	}
}

int Zapper_get_verify_module_header(VERIFY_MODULE_TYPE module, unsigned char **buf, unsigned int *len)
{
	if (buf == NULL || len == NULL) {
		printf("[ZAPPER] Zapper_get_verify_module_payload, invalid para, module=0x%x\n", module);
		return ZAPPER_ERROR;
	}

	if (module == VERIFY_MODULE_BOOT) {
		*buf = g_caverify_partition + BOOT_HEAD_OFFSET;
	} else if (module == VERIFY_MODULE_SYSTEM) {
		*buf = g_caverify_partition + SYSTEM_HEAD_OFFSET;
	} else if (module == VERIFY_MODULE_CCACONFIG) {
		*buf = g_caverify_partition + CCA_HEAD_OFFSET;
	} else if (module == VERIFY_MODULE_CASECURE) {
		*buf = g_caverify_partition + CASECURE_HEAD_OFFSET;
	}

	*len = AML_NORMAL_HEAD_SIZE;

	return ZAPPER_SUCCESS;
}

int Zapper_get_verify_module_payload(VERIFY_MODULE_TYPE module, unsigned char **buf, unsigned int *len)
{
	if (buf == NULL || len == NULL) {
		printf("[ZAPPER] Zapper_get_verify_module_payload, invalid para, module=0x%x\n", module);
		return ZAPPER_ERROR;
	}

	if (module == VERIFY_MODULE_BOOT) {
		*buf = g_boot_partition;
		*len = BOOT_PARTITION_SIZE;
	} else if (module == VERIFY_MODULE_SYSTEM) {
		*buf = g_boot_partition;
		*len = SYSTEM_PARTITION_SIZE;
	} else if (module == VERIFY_MODULE_CCACONFIG) {
		*buf = g_boot_partition;
		*len = CASECURE_PARTITION_SIZE;
	} else if (module == VERIFY_MODULE_CASECURE) {
		*buf = g_boot_partition;
		*len = CCACONFIG_PARTITION_SIZE;
	}

	return ZAPPER_SUCCESS;
}

int Zapper_get_rescuelist_module_payload(unsigned char *buf, unsigned int len)
{
	int ret = ZAPPER_SUCCESS;

	if (buf == NULL || len != RESCUELIST_PARTITION_SIZE) {
		printf("[ZAPPER] read rescuelist partition, invalid para, size=0x%x\n", len);
		return ZAPPER_ERROR;
	}

	printf("[ZAPPER] read rescuelist partition, size=0x%x\n", len);
	ret = store_read(RESCUELIST_PARTITION_NAME, 0, len, buf);
	if (ret != 0) {
		printf("[ZAPPER] read rescuelist partition failed. ret=0x%x\n", ret);
	}

	return ret;
}

static int Zapper_read_all_info(struct Zapper_boot_info *p_s_boot_info)
{
	if (p_s_boot_info == NULL || flash_map_index < 2) {
		printf("[ZAPPER] flash_map_index < 2,The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);
		return ZAPPER_ERROR;
	}

	int ret = ZAPPER_ERROR;
	struct mtd_info *mtd = NULL;
	unsigned long rwsize;

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	if (!mtd) {
		puts("\n[ZAPPER]no devices available\n");
		return ZAPPER_ERROR;
	}

    printf("[ZAPPER] The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	rwsize = LDFLAG_LENGTH;
	ret = nand_read_skip_bad(mtd, ldflag_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldflag_partition);

	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_ldflag_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}
	for (int i = 0;i < LDFLAG_LENGTH ; i++) {
		printf ("zapper_ldflag_partition[%d] is %x\n", i, *(zapper_ldflag_partition + i));

	}

	rwsize = HWCONFIG_LENGTH;
	ret = nand_read_skip_bad(mtd, hwconfig_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_hwconfig_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_hwconfig_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}

	for (int i = 0;i < HWCONFIG_LENGTH ; i++) {
		printf ("zapper_hwconfig_partition[%d] is %x\n", i, *(zapper_hwconfig_partition + i));
	}

	rwsize = LDSEC_LENGTH;
	ret = nand_read_skip_bad(mtd, ldsec_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldsec_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_ldsec_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}

	for (int i = 0;i < LDSEC_LENGTH ; i++) {
		printf ("zapper_ldsec_partition[%d] is %x\n", i, *(zapper_ldsec_partition + i));
	}

	rwsize = KERNEL_LENGTH;
	ret = nand_read_skip_bad(mtd, kernel_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_kernel_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_kernel_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}


	memcpy((void *)p_s_boot_info->loader_partition_header, (void *)zapper_ldflag_partition, LD_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->loader_partition, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH , LD_LENGTH);
	memcpy((void *)p_s_boot_info->error_code, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH, EC_LENGTH);
	p_s_boot_info->modify_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH];
	p_s_boot_info->reboot_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1];
	p_s_boot_info->download_mode = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1];
	p_s_boot_info->standby_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1];
	p_s_boot_info->backupODU = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1 + 1];
	memcpy((void *)p_s_boot_info->share_crc,
		(void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1 + 1 + 1 + 1 + 1, LD_SHARE_DATA_CRC_LENGTH);

	memcpy((void *)p_s_boot_info->bbcb_header, (void *)zapper_hwconfig_partition, BBCB_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->bbcb, (void *)zapper_hwconfig_partition + BBCB_HEADER_LENGTH, BBCB_LENGTH);

	memcpy((void *)p_s_boot_info->uk_header, (void *)zapper_ldsec_partition, UK_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->uk, (void *)zapper_ldsec_partition + UK_HEADER_LENGTH, UK_LENGTH);

	memcpy((void *)p_s_boot_info->kernel, (void *)zapper_kernel_partition, KERNEL_LENGTH);
	memcpy((void *)p_s_boot_info->kernel_header, (void *)zapper_kernel_header_partition, KERNEL_LENGTH);

	return ZAPPER_SUCCESS;

}

static int Zapper_write_ldflag(void)
{
	int ret = ZAPPER_ERROR;
	struct mtd_info *mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);
	unsigned long rwsize;

	if (!mtd) {
		puts("\n[ZAPPER]no devices available\n");
		return ZAPPER_ERROR;
	}

	printf("[ZAPPER] The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	rwsize = LDFLAG_LENGTH;
	ret = nand_write_skip_bad(mtd, ldflag_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldflag_partition,
							 0x2);

	if (ret) {
		printf("[ZAPPER] nand_write_skip_bad zapper_ldflag_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}
	printf("[ZAPPER] after writing We will printf zapper_ldflag_partition\n");
	for (int i = 0;i < LDFLAG_LENGTH ; i++) {
		printf ("zapper_ldflag_partition[%d] is %x\n", i, *(zapper_ldflag_partition+i));

	}

	return ZAPPER_SUCCESS;
}

unsigned char Zapper_get_nand_standby_flag(void)
{
	return s_boot_info.standby_flag;
}

static void Zapper_factory_reset_led_display(int ret)
{
	unsigned int i = 0;
	led_display_type standby_led;
	printf("%s:%d\n", __FUNCTION__, __LINE__);
	/* On successful clearing of all the data as per the above steps,
	 * the Standby LED will blink in green colour for 5 times
	 * whereas in case of failure in any of the above mentioned three steps it will blink in RED colour for 5 times
	 * as an indication of failure in performing the intended operation
	 */
	standby_led = (ret == ZAPPER_SUCCESS) ? LED_POWER_GREEN : LED_POWER_RED;

	for (i = 0; i < 5; i++) {
		Zapper_led_set(standby_led);
		Zapper_led_set(LED_REMOTE_OFF);
		Zapper_led_set(LED_ALERT_OFF);
		Zapper_led_show();
		Zapper_led_set(LED_POWER_OFF);
		Zapper_led_set(LED_REMOTE_OFF);
		Zapper_led_set(LED_ALERT_OFF);
		Zapper_led_show();
	}
}

#ifdef CONFIG_YAFFS2
extern int meson_yaffs2_mount(char *mtpoint, char *part_name);
#endif
static int zapper_erase_cadata_specific_data(void)
{
	int ret = ZAPPER_SUCCESS;

#ifdef CONFIG_YAFFS2
	char yaffs_cmd[50] = {0};
	ret = meson_yaffs2_mount("tmp", FACTORY_RESET_CADATA_PARTITION_NAME);
	if (ret != 0) {
		printf("[ZAPPER] failed to mount cadata partition for factory reset! ret=0x%x\n", ret);
		return ZAPPER_ERROR;
	}

	/* rm PS INDEX 0 file */
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yls tmp/%s", FACTORY_RESET_PS_INDEX_0);
	run_command(yaffs_cmd, 0);
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yrm tmp/%s", FACTORY_RESET_PS_INDEX_0);
	run_command(yaffs_cmd, 0);
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yls tmp/%s", FACTORY_RESET_PS_INDEX_0);
	run_command(yaffs_cmd, 0);

	/* rm PS INDEX 15 file */
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yls tmp/%s", FACTORY_RESET_PS_INDEX_15);
	run_command(yaffs_cmd, 0);
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yrm tmp/%s", FACTORY_RESET_PS_INDEX_15);
	run_command(yaffs_cmd, 0);
	memset(yaffs_cmd, 0, sizeof(yaffs_cmd));
	snprintf(yaffs_cmd, sizeof(yaffs_cmd), "yls tmp/%s", FACTORY_RESET_PS_INDEX_15);
	run_command(yaffs_cmd, 0);
#endif

	return ret;
}

static int zapper_erase_data_partition(void)
{
	run_command("store erase data 0 0", 0);

	return ZAPPER_SUCCESS;
}

int Zapper_nand_factory_reset(void)
{
	int ret = ZAPPER_SUCCESS;
	/* erase the cadata_a partition's Index-0 and Index-5 */
	ret |= zapper_erase_cadata_specific_data();

	/* erase the data partition */
	ret |= zapper_erase_data_partition();

	/* show the reset status by led */
	Zapper_factory_reset_led_display(ret);

	/* reboot */
	printf("[ZAPPER] factory reset done, will reboot! ret=0x%x\n", ret);
	run_command("reboot", 0);

	return ZAPPER_SUCCESS;
}

static int do_zapper_read_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do zapper flash read\n");
	int ret = ZAPPER_ERROR;

	ERR_REPORT_Initialize();
	ret = Zapper_read_all_info(&s_boot_info);
	if (ret != ZAPPER_SUCCESS) {
		printf("[ZAPPER] %s:%d set error code\n", __FUNCTION__, __LINE__);
		ERR_REPORT_SetErrorCode(ERROR_CODE_BOOT_CHECK_FAILED);
	}
	return ret;
}

static int do_zapper_write_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do zapper flash write\n");
	int ret = ZAPPER_ERROR;
	run_command("nand erase.part ldflag", 0);
	ret = Zapper_write_ldflag();
	return ret;
}


U_BOOT_CMD(
	zapper_flash_read, 1, 0, do_zapper_read_flash,"zapper read" ,"zapper read for irdeto loader"
);

U_BOOT_CMD(
	zapper_flash_write, 1, 0, do_zapper_write_flash,"zapper write" ,"zapper write for irdeto loader"
);
