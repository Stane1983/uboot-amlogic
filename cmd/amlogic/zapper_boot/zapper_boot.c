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

//#define DEBUG_ENABLE_USB_DOWNLOAD
#define DOWNLOAD_MASK (0b00010000)
/**
 * Download MODE, Consistent with the definition in the downloader.
 */
enum DOWNLOAD_MODE_TYPE{
    DOWNLOAD_MODE_NORMAL = 0,
    DOWNLOAD_MODE_RESCUE,
    DOWNLOAD_MODE_MANUAL_FORCE,
    DOWNLOAD_MODE_USB,
    DOWNLOAD_MODE_TUNING_CODE,
    DOWNLOAD_MODE_ADVANCE_SETUP,
    DOWNLOAD_MODE_MAX
};

static int g_download_flag_isenable = 0;

static lc_result LoaderPartition_SetLoaderPartition(lc_loader_pt_st *loaderPt)
{
	lc_result result = LC_SUCCESS;

	result = LC_WriteSharedMemory(LC_TRUE, &(loaderPt->sharedMemory) );
	printf("in LoaderPartition_SetLoaderPartition setting pLoaderPt->sharedMemory.downloadIndicator = 0x%x\n",loaderPt->sharedMemory.downloadIndicator);

	if (LC_SUCCESS == result)
	{
		result = LC_StoreDownloadInformation( &(loaderPt->downloadInfo) );
	}

	return result;
}

static lc_result LoaderPartition_usb_init(lc_loader_pt_st *pLoaderPt, unsigned char download_mode)
{
	lc_result result = LC_SUCCESS;

	/* read LoaderPartition */
	result = LC_ReadLoaderPartition(pLoaderPt);
	printf("pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);

	/* check download flag */
	/* if download flag is ture do nothing.
	 * 0xff is means there don't have ld data.
	 */
	if ((pLoaderPt->sharedMemory.downloadIndicator != 0xff) &&
		((pLoaderPt->sharedMemory.downloadIndicator & DOWNLOAD_MASK) == DOWNLOAD_MASK))
	{
		g_download_flag_isenable = 1;
		printf("download flag is enabled go to recovery now in usb judgement\n");
		run_command("reboot recovery", NO_DETAIL);
		return result;
	}

	/* download flag and type */
	/* [7:4], 0x1 for download available */
	/* [3:0], 0x03 for usb */
	/* enble download for USB */
	if (/*Zapper_get_usb_download_request() || */(download_mode == DOWNLOAD_MODE_USB)) {
		pLoaderPt->sharedMemory.downloadIndicator = 0b00010011;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		result = Zapper_set_jump_recovery_status(USB_DETECT_JUMP);
	}

	return result;
}


static lc_result LoaderPartition_enable_usb(unsigned char download_mode)
{
	lc_result result = LC_SUCCESS;
	lc_loader_pt_st pt;

	/* write a normal LoaderPartition */
	result = LoaderPartition_usb_init(&pt, download_mode);

	if (g_download_flag_isenable == 0)
	{
		printf("Not set download flag!\n");
		if (result == LC_SUCCESS)
		{
			result = LoaderPartition_SetLoaderPartition(&pt);
		}
	}
	if (result != LC_SUCCESS) {
		printf("[ZAPPER] %s:%d set error code\n", __FUNCTION__, __LINE__);
		ERR_REPORT_SetErrorCode(ERROR_CODE_INVALID_LOADERPT);
	}
	return result;
}

static lc_result LoaderPartition_ota_init(lc_loader_pt_st *pLoaderPt, unsigned char download_mode)
{
	lc_result result = LC_SUCCESS;
	unsigned char key = 0;

	/* read LoaderPartition */
	result = LC_ReadLoaderPartition(pLoaderPt);
	printf("pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);

	/* check download flag */
	/* if download flag is ture do nothing.
	 * 0xff is means there don't have ld data.
	 */
	if ((pLoaderPt->sharedMemory.downloadIndicator != 0xff) &&
		((pLoaderPt->sharedMemory.downloadIndicator & DOWNLOAD_MASK) == DOWNLOAD_MASK))
	{
		g_download_flag_isenable = 1;
		printf("download flag is enabled from ota judge jump recovery directly\n");
		run_command("reboot recovery", NO_DETAIL);
		return result;
	}

	/* download flag and type */
	/* [7:4], 0x1 for download available */
	/* [3:0], 0x03 for usb */
	/* enble download for USB */
	result = Zapper_get_key_info(&key);
	if ((result == ZAPPER_SUCCESS && key == ADC_KEY_A_PRESS) ||
		(download_mode == DOWNLOAD_MODE_TUNING_CODE) ||
		(download_mode == DOWNLOAD_MODE_ADVANCE_SETUP) ||
		(download_mode == DOWNLOAD_MODE_MANUAL_FORCE)){
		pLoaderPt->sharedMemory.downloadIndicator = 0b00010010;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		result = Zapper_set_jump_recovery_status(OTA_DETECT_JUMP);
	}

	return result;
}

static lc_result LoaderPartition_enable_ota(unsigned char download_mode)
{
	lc_result result = LC_SUCCESS;
	lc_loader_pt_st pt;

	/* write a normal LoaderPartition */
	result = LoaderPartition_ota_init(&pt, download_mode);

	if (g_download_flag_isenable == 0)
	{
		printf("Not set download flag from ota judge!\n");
		if (result == LC_SUCCESS)
		{
			result = LoaderPartition_SetLoaderPartition(&pt);
		}
	}

	if (result != LC_SUCCESS) {
		printf("[ZAPPER] %s:%d set error code\n", __FUNCTION__, __LINE__);
		ERR_REPORT_SetErrorCode(ERROR_CODE_INVALID_LOADERPT);
	}

	return result;
}

static lc_result BSTRAP_BootCheck(lc_bool *pCodeModuleError)
{
	lc_result result = LC_SUCCESS;
	lc_uint32 moduleCount = 3;
	unsigned short pModuleList[3] = {0};
	lc_uchar *pUk = LC_NULL;
	lc_uint32 cnt = 0;
	pModuleList[0] = 0x20;	/* boot module id, 0x20 */
	pModuleList[1] = 0x21;	/* system module id, 0x21 */
	//pModuleList[2] = 0x24;	/* casecure module id, 0x24 */
	pModuleList[2] = 0x23;	/* ccaconfig module id, 0x23 */
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

static int do_zapper_boot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned char boot_status = NO_NEED_JUMP;
	int ret = ZAPPER_SUCCESS;
	unsigned char rcu_combination_type;
	struct Zapper_boot_info boot_info={0};

	CRC_CreateTables();
	ret = Zapper_get_nand_ldflag_partition_info(&boot_info);
	if (ret) {
		return ZAPPER_ERROR;
	}

	ret = Zapper_get_rcu_combination_type(&rcu_combination_type);
	if (ret == ZAPPER_SUCCESS && rcu_combination_type != RCU_COMBINATION_MAX) {
		printf("[ZAPPER] get download mode success \n");
		if (rcu_combination_type != RCU_COMBINATION_FACTORY_RESET) {
			boot_info.download_mode = rcu_combination_type_convert_to_download_mode(rcu_combination_type);
			ret = Zapper_set_nand_ldflag_partition_info(&boot_info);
			if (ret) {
				return ZAPPER_ERROR;
			}
		} else {
			/* FACTORY RESET */
			/* TODO */
		}
	}

	LoaderPartition_enable_ota(boot_info.download_mode);

	printf("[ZAPPER] boot_info.reboot_flag = %x \n",boot_info.reboot_flag);

	if (boot_info.reboot_flag != 0xff && boot_info.reboot_flag != 0x55) {//0x55 & 0xff is used to jump dvtapp
		ret = Zapper_get_jump_recovery_status(&boot_status);

		if (ret == ZAPPER_SUCCESS && boot_status == NO_NEED_JUMP) {
			printf("[ZAPPER] run usb download flag judgement \n");
			LoaderPartition_enable_usb(boot_info.download_mode);
		}
	}
	else { //restore reboot_flag
		//printf("[ZAPPER] need set boot_info.reboot_flag \n");
		ret = Zapper_get_nand_ldflag_partition_info(&boot_info);
		boot_info.reboot_flag = 0xAA; //0xAA is used to judge usb Downloadflag
		if (ret) {
			return ZAPPER_ERROR;
		}
		ret = Zapper_set_nand_ldflag_partition_info(&boot_info);
		if (ret) {
			return ZAPPER_ERROR;
		}
	}

	return ZAPPER_SUCCESS;
}

static int do_zapper_verify(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	lc_bool codeModuleError = LC_FALSE;
	int ret = ZAPPER_ERROR;
	printf("[ZAPPER] run do_zapper_verify \n");
	ret = BSTRAP_BootCheck(&codeModuleError);

	return ret;
}



U_BOOT_CMD(
	zapper_boot, 1, 0, do_zapper_boot,"do_zapper_test_write" ,"It will use our yaffs2 driver"
);

U_BOOT_CMD(
	zapper_verify, 1, 0, do_zapper_verify,"do_zapper_test_write" ,"It will use our yaffs2 driver"
);






