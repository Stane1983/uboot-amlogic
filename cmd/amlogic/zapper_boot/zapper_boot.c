#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <command.h>
#include <usb.h>
#include <amlogic/zapper_boot.h>
#include "LoaderCore/Src/Shared/Bbcb.h"
#include "LoaderCore/Src/Shared/Bit.h"
#include "LoaderCore/Src/Shared/Crc.h"
#include "LoaderCore/Src/Shared/LoaderPartition.h"
#include "LoaderCore/Src/Shared/KeyUpdate.h"
#include "LoaderCore/Src/Shared/ModuleManager.h"
#include "LoaderCore/Include/ImportSPI/LoaderCoreSPI_Module.h"

//#define DEBUG_ENABLE_USB_DOWNLOAD
static lc_uchar g_reboot_type = REBOOT_FLAG_COLD;
static lc_uchar g_skip_verify = 0;

static lc_result LoaderPartition_SetLoaderPartition(lc_loader_pt_st *loaderPt)
{
	lc_result result = LC_SUCCESS;

	result = LC_WriteSharedMemory(LC_TRUE, &(loaderPt->sharedMemory) );
	printf("in LoaderPartition_SetLoaderPartition setting pLoaderPt->sharedMemory.downloadIndicator = 0x%x\n",loaderPt->sharedMemory.downloadIndicator);

	printf("save sharedMemory ret = 0x%x!\n", result);
	if (LC_SUCCESS == result)
	{
		result = LC_StoreDownloadInformation( &(loaderPt->downloadInfo) );
		printf("save downloadInfo ret = 0x%x!\n", result);
	}

	result = LC_StoreShareDataInformation( &(loaderPt->shareDataInfo));
	printf("save shareDataInfo ret = 0x%x!\n", result);

	return result;
}

static lc_result BSTRAP_BootCheck(lc_bool *pCodeModuleError)
{
	lc_result result = LC_SUCCESS;
	lc_uint32 moduleCount = 4;
	unsigned short pModuleList[3] = {0};
	lc_uchar *pUk = LC_NULL;
	lc_uint32 cnt = 0;
	pModuleList[0] = MODULE_DRV_ID_KERNEL;	/* boot module id, 0x20 */
	pModuleList[1] = MODULE_DRV_ID_ROOTFS;	/* system module id, 0x21 */
	pModuleList[2] = MODULE_DRV_ID_CASECURE;	/* casecure module id, 0x24 */
	pModuleList[3] = MODULE_DRV_ID_CCA;	/* ccaconfig module id, 0x23 */
	//pModuleList[4] = 0x22;	/* rescue list module id, 0x22 */
	//pModuleList[2] = 35;	/* backup ccaconfig & rescue list  */

	/* Retrieve UK (optional, only necessary when boot check algorithm is LC_CHECKSUM_RSASSA_PKCS1_V1_5). */
	if (LC_SUCCESS == result)
	{
		printf("[ZAPPER] Run RetrieveUK\n");

		result = LC_RetrieveUK(&pUk, LC_NULL);
		if (LC_SUCCESS != result)
		{
			pUk = LC_NULL;
			*pCodeModuleError = LC_FALSE;
		}
	}

	/* Go through and verify all modules*/
	if (LC_SUCCESS == result)
	{
		for (cnt = 0; cnt < moduleCount; cnt++)
		{
	/* US 257412
	only check High level application module validation
	*/
	/* Verify all the existing module. */
			result= LC_ReadAndVerifyExistingModule(
				pModuleList[cnt],
				LC_TRUE,
				0x02,
				pUk,
				LC_NULL,
				LC_NULL,
				LC_NULL);
			printf("[ZAPPER] after LC_ReadAndVerifyExistingModule\n");
			if ( LC_SUCCESS != result)
			{
				break;
			}

		}
	}

	if (LC_SUCCESS == result)
	{
		printf("[ZAPPER] boot_check done\n");
	}

	/* clear memory blocks */
	LC_Free_Memory((void **)&pUk);

	return result;

}	/* BSTRAP_VerifyFlash */

lc_bool codeModuleError = LC_FALSE;

static unsigned char rcu_combination_type_convert_to_download_mode(unsigned char rcu_combination_type)
{
	if (rcu_combination_type == RCU_COMBINATION_ADVANCED_TUNING_CODE_SCREEN) {
		return DOWNLOAD_MODE_TUNING_CODE;
	}
	if (rcu_combination_type == RCU_COMBINATION_ADVANCED_SETUP_SCREEN) {
		return DOWNLOAD_MODE_ADVANCE_SETUP;
	}
	if (rcu_combination_type == RCU_COMBINATION_USB_UPGRADE) {
		return DOWNLOAD_MODE_USB;
	}
	if (rcu_combination_type == RCU_COMBINATION_MANUAL_FORCED_DOWNLOAD) {
		return DOWNLOAD_MODE_MANUAL_FORCE;
	}

	return DOWNLOAD_MODE_MAX;
}

static void LoaderPartition_check_modify_flag_and_rcu(lc_loader_pt_st *pLoaderPt, unsigned char downloadModeFlag)
{
	int ret = ZAPPER_SUCCESS;
	unsigned char key = 0;
	unsigned char last_downloadModeFlag = DOWNLOAD_MODE_MAX;

	printf("[ZAPPER] pLoaderPt->shareDataInfo.modifyFlag = 0x%x, downloadIndicator=0x%x\n",
		pLoaderPt->shareDataInfo.modifyFlag, pLoaderPt->sharedMemory.downloadIndicator);

	if (pLoaderPt->shareDataInfo.modifyFlag == PS_MODIFIED) {
		pLoaderPt->sharedMemory.downloadIndicator &= 0x0f;
		pLoaderPt->sharedMemory.downloadIndicator |= (lc_uchar)(1 << 4);
	}

	if (pLoaderPt->sharedMemory.downloadIndicator == OTA_DOWNLOAD_INDICATOR) {
		last_downloadModeFlag = DOWNLOAD_MODE_NORMAL;
	}

	if (pLoaderPt->sharedMemory.downloadIndicator == USB_DOWNLOAD_INDICATOR) {
		last_downloadModeFlag = DOWNLOAD_MODE_USB;
	}

	ret = Zapper_get_key_info(&key);
	if (key == ADC_KEY_A_PRESS ||
		downloadModeFlag == DOWNLOAD_MODE_MANUAL_FORCE ||
		downloadModeFlag == DOWNLOAD_MODE_TUNING_CODE ||
		downloadModeFlag == DOWNLOAD_MODE_ADVANCE_SETUP ||
		last_downloadModeFlag == DOWNLOAD_MODE_NORMAL) {
		pLoaderPt->sharedMemory.downloadIndicator = OTA_DOWNLOAD_INDICATOR;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		ret = Zapper_set_jump_recovery_status(OTA_DETECT_JUMP);
	}

	if (/*Zapper_get_usb_download_request() || */downloadModeFlag == DOWNLOAD_MODE_USB ||
		last_downloadModeFlag == DOWNLOAD_MODE_USB) {
		pLoaderPt->sharedMemory.downloadIndicator = USB_DOWNLOAD_INDICATOR;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		ret = Zapper_set_jump_recovery_status(USB_DETECT_JUMP);
	}
	pLoaderPt->shareDataInfo.downloadModeFlag = (downloadModeFlag == DOWNLOAD_MODE_MAX) ? last_downloadModeFlag : downloadModeFlag;

	printf("[ZAPPER] pLoaderPt->shareDataInfo.modifyFlag = 0x%x, downloadIndicator=0x%x\n",
		pLoaderPt->shareDataInfo.modifyFlag, pLoaderPt->sharedMemory.downloadIndicator);
	return;
}

static int do_zapper_boot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = ZAPPER_SUCCESS;
	unsigned char rcu_combination_type;
	lc_loader_pt_st LoaderPt;
	unsigned char downloadModeFlag = DOWNLOAD_MODE_MAX;

	CRC_CreateTables();

	memset(&LoaderPt, 0x0, sizeof(LoaderPt));
	ret = LC_ReadLoaderPartition(&LoaderPt);
	printf("[ZAPPER] LoaderPt.sharedMemory.downloadIndicator = 0x%x, ret=0x%x\n", LoaderPt.sharedMemory.downloadIndicator, ret);
	if (ret != ZAPPER_SUCCESS) {
		printf("[ZAPPER] get loaderpartition failed \n");
		ERR_REPORT_SetErrorCode(ERROR_CODE_INVALID_LOADERPT);
		return ZAPPER_ERROR;
	}

	printf("[ZAPPER] LoaderPt.shareDataInfo.resetType = 0x%x \n", LoaderPt.shareDataInfo.resetType);
	if (LoaderPt.shareDataInfo.resetType == REBOOT_FLAG_WARM) {
		printf("[ZAPPER] warm boot, directly go to MRS! \n");
		g_reboot_type = REBOOT_FLAG_WARM;
		goto warm_reboot_skip;
	}

	ret = Zapper_get_rcu_combination_type(&rcu_combination_type);
	if (ret == ZAPPER_SUCCESS && rcu_combination_type != RCU_COMBINATION_MAX) {
		printf("[ZAPPER] get download mode success \n");
		if (rcu_combination_type != RCU_COMBINATION_FACTORY_RESET) {
			downloadModeFlag = rcu_combination_type_convert_to_download_mode(rcu_combination_type);
		} else {
			/* FACTORY RESET */
			printf("[ZAPPER] prepare to factory reset \n");
			Zapper_nand_factory_reset();
		}
	}

	LoaderPartition_check_modify_flag_and_rcu(&LoaderPt, downloadModeFlag);

	LoaderPt.shareDataInfo.resetType = REBOOT_FLAG_COLD;
	g_reboot_type = REBOOT_FLAG_COLD;

	ret = LoaderPartition_SetLoaderPartition(&LoaderPt);
	printf("%s:%d call LoaderPartition_SetLoaderPartition ret = 0x%x!\n", __FUNCTION__, __LINE__, ret);

	/* check download flag */
	/* if download flag is ture do nothing.
	 * 0xff is means there don't have ld data.
	 */
	if ((LoaderPt.sharedMemory.downloadIndicator != 0xff) &&
		((LoaderPt.sharedMemory.downloadIndicator & DOWNLOAD_MASK) == DOWNLOAD_MASK))
	{
		printf("download flag is enabled, don't need to bootcheck!\n");
		g_skip_verify = 1;
	}

warm_reboot_skip:

	return ret;
}

static int do_zapper_verify(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	lc_bool codeModuleError = LC_FALSE;
	int ret = ZAPPER_SUCCESS;

	printf("[ZAPPER] run do_zapper_verify \n");
	/* Determine if the current state is a warm boot. If it is, directly launch MRS and skip boot verification. */
	if ((g_reboot_type == REBOOT_FLAG_WARM) || (g_skip_verify == 1)) {
		printf("[ZAPPER] reset type is warm reboot, jump bootcheck directly\n");
		return ZAPPER_SUCCESS;
	}

	ret = Zapper_read_verify_module_partition();
	if (ret != ZAPPER_SUCCESS) {
		printf("[ZAPPER] read verify module partition failed!\n");
		return ret;
	}

	ret = BSTRAP_BootCheck(&codeModuleError);

	Zapper_free_verify_module_partition();
	printf("[ZAPPER] do_zapper_verify done, ret=0x%x\n", ret);
	return ret;
}

static int do_zapper_boot_error_process(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = ZAPPER_ERROR;
	lc_loader_pt_st LoaderPt;
	unsigned int bootstrap_ec = ERROR_CODE_SUCCESS;

	CRC_CreateTables();

	memset(&LoaderPt, 0x0, sizeof(LoaderPt));
	ret = LC_ReadLoaderPartition(&LoaderPt);
	printf("[ZAPPER] LoaderPt.sharedMemory.downloadIndicator = 0x%x, ret=0x%x\n", LoaderPt.sharedMemory.downloadIndicator, ret);

	/* save bootstrap's error code to loader partition */
	ERR_REPORT_GetErrorCode(&bootstrap_ec, NULL);
	printf("[ZAPPER] %s:%d, error code: 0x%x\n", __FUNCTION__, __LINE__, bootstrap_ec);
	LoaderPt.shareDataInfo.errorCode = bootstrap_ec;

	printf("[ZAPPER] LoaderPt.shareDataInfo.resetType = 0x%x, g_reboot_type=0x%x\n", LoaderPt.shareDataInfo.resetType, g_reboot_type);
	if (g_reboot_type == REBOOT_FLAG_WARM) {
		/**
		 * If everything is okey and the high-level application is going to be executed,
		 * Boot Strap will reset the PS modify flag in case of any misoperations.
		 */
		LoaderPt.shareDataInfo.modifyFlag = PS_UNMODIFIED;
		LoaderPt.shareDataInfo.resetType = REBOOT_FLAG_COLD;
	} else {
		if (bootstrap_ec != ERROR_CODE_SUCCESS) {
			if (bootstrap_ec == ERROR_CODE_INVALID_BBCB) {
				/* If there is an error with BBCB, as it cannot be upgraded, the device will keep rebooting. */
				printf("[ZAPPER] ERROR!!! INVALID BBCB\n");
#ifdef SECUREBOOT_ENABLE
				run_command("reboot", 0);
#endif
			} else {
				/* If any other errors occur, then it will enter rescue download upgrade mode. */
				LoaderPt.shareDataInfo.downloadModeFlag = DOWNLOAD_MODE_RESCUE;
				LoaderPt.sharedMemory.downloadIndicator = 0b00010010;
				printf("After setting LoaderPt.sharedMemory.downloadIndicator = %x\n",LoaderPt.sharedMemory.downloadIndicator);
				ret = Zapper_set_jump_recovery_status(OTA_DETECT_JUMP);
				printf("[ZAPPER] Zapper_set_jump_recovery_status ret=0x%x\n", ret);
			}
		}

		/* A special case: dl is done, but fFlashCorrupted=TRUE wasn't corrected in time. */
		if (((LoaderPt.sharedMemory.downloadIndicator & DOWNLOAD_MASK) != DOWNLOAD_MASK) && (bootstrap_ec == ERROR_CODE_SUCCESS)) {
			if (LoaderPt.shareDataInfo.modifyFlag == PS_MODIFIED) {
				LoaderPt.shareDataInfo.modifyFlag = PS_UNMODIFIED;
			}
		}
	}
	ret = LoaderPartition_SetLoaderPartition(&LoaderPt);
	printf("call LoaderPartition_SetLoaderPartition ret = 0x%x!\n", ret);

	return ret;
}


U_BOOT_CMD(
	zapper_boot, 1, 0, do_zapper_boot, "do_zapper_boot", "do zapper boot"
);

U_BOOT_CMD(
	zapper_verify, 1, 0, do_zapper_verify, "do_zapper_verify", "do zapper verify"
);

U_BOOT_CMD(
	zapper_boot_error_process, 1, 0, do_zapper_boot_error_process, "do_zapper_boot_error_process", "process the boot error"
);


