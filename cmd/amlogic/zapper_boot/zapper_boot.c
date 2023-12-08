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

static lc_result LoaderPartition_usb_init(lc_loader_pt_st *pLoaderPt)
{
	lc_result result = LC_SUCCESS;

	/* read LoaderPartition */
	result = LC_ReadLoaderPartition(pLoaderPt);
	printf("pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);

	/* check download flag */
	/* if download flag is ture do nothing*/
	if ((pLoaderPt->sharedMemory.downloadIndicator & DOWNLOAD_MASK) == DOWNLOAD_MASK)
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
	if (Zapper_get_usb_download_request()) {
		pLoaderPt->sharedMemory.downloadIndicator = 0b00010011;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		result = Zapper_set_jump_recovery_status(USB_DETECT_JUMP);
	}

	return result;
}


static lc_result LoaderPartition_enable_usb(void)
{
	lc_result result = LC_SUCCESS;
	lc_loader_pt_st pt;

	/* write a normal LoaderPartition */
	result = LoaderPartition_usb_init(&pt);

	if (g_download_flag_isenable == 0)
	{
		printf("Not set download flag!\n");
		if (result == LC_SUCCESS)
		{
			result = LoaderPartition_SetLoaderPartition(&pt);
		}
	}

	return result;
}

static lc_result LoaderPartition_ota_init(lc_loader_pt_st *pLoaderPt)
{
	lc_result result = LC_SUCCESS;
	unsigned char key = 0;

	/* read LoaderPartition */
	result = LC_ReadLoaderPartition(pLoaderPt);
	printf("pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);

	/* check download flag */
	/* if download flag is ture do nothing*/
	if ((pLoaderPt->sharedMemory.downloadIndicator & DOWNLOAD_MASK) == DOWNLOAD_MASK)
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
	if (result == ZAPPER_SUCCESS && key == ADC_KEY_A_PRESS) {
		pLoaderPt->sharedMemory.downloadIndicator = 0b00010010;
		printf("After setting pLoaderPt->sharedMemory.downloadIndicator = %x\n",pLoaderPt->sharedMemory.downloadIndicator);
		result = Zapper_set_jump_recovery_status(OTA_DETECT_JUMP);
	}

	return result;
}

static lc_result LoaderPartition_enable_ota(void)
{
	lc_result result = LC_SUCCESS;
	lc_loader_pt_st pt;

	/* write a normal LoaderPartition */
	result = LoaderPartition_ota_init(&pt);

	if (g_download_flag_isenable == 0)
	{
		printf("Not set download flag from ota judge!\n");
		if (result == LC_SUCCESS)
		{
			result = LoaderPartition_SetLoaderPartition(&pt);
		}
	}

	return result;
}

static lc_result BSTRAP_BootCheck(lc_bool *pCodeModuleError)
{
	lc_result result = LC_SUCCESS;
	lc_uint32 moduleCount = 1;
	//lc_uint16 *pModuleList = LC_NULL;
	unsigned short pModuleList[3] = {0};
	lc_uchar *pUk = LC_NULL;
	lc_uint32 cnt = 0;
	pModuleList[0] = 33;

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

static int do_zapper_boot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned char boot_status = NO_NEED_JUMP;
	unsigned char key_press_status = NO_ADC_KEY_PRESS;
	int ret = ZAPPER_SUCCESS;
	struct Zapper_boot_info boot_info={0};

	CRC_CreateTables();
	ret = Zapper_get_nand_ldflag_partition_info(&boot_info);
	if (ret) {
		return ZAPPER_ERROR;
	}

	ret = Zapper_get_key_info(&key_press_status);

	if (ret == ZAPPER_SUCCESS && key_press_status == ADC_KEY_A_PRESS) {
		printf("[ZAPPER] run OTA download flag judgement \n");
		LoaderPartition_enable_ota();
	}
	printf("[ZAPPER] boot_info.reboot_flag = %x \n",boot_info.reboot_flag);

	if (boot_info.reboot_flag != 0xff && boot_info.reboot_flag != 0x55) {//0x55 & 0xff is used to jump dvtapp
		ret = Zapper_get_jump_recovery_status(&boot_status);

		if (ret == ZAPPER_SUCCESS && boot_status == NO_NEED_JUMP) {
			printf("[ZAPPER] run usb download flag judgement \n");
			LoaderPartition_enable_usb();
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






