#ifndef _ZAPPER_BOOT_COMMON_H_
#define _ZAPPER_BOOT_COMMON_H_

/* Start---uboot command parameters---*/

#define NO_DETAIL	(0)
#define NEED_DETAIL 	(1)
/* End---uboot command parameters---*/

/* Start---ZAPPER USB parameters---*/
#define MAX_USB_FILES (50)

#define MAX_NAMES (50)		//zapper file max name number

#define NAME_LENGTH (256)	//zapper file max name length

#define CMD_USB_START "usb start "

#define CMD_USB_LS "fatls usb 0"	//The usb flash port corresponds to usb 0


/* End---ZAPPER USB parameters---*/


/* Start---ZAPPER return value---*/
#define ZAPPER_FOUND_SDL	(1)
#define ZAPPER_NON_SDL		(0)

#define ZAPPER_ERROR   		(1)
#define ZAPPER_SUCCESS 		(0)

/* End---ZAPPER return value---*/

/* Start---ZAPPER Info in nand---*/
#define BBCB_HEADER_LENGTH	(12)
#define UK_HEADER_LENGTH	(538)
#define KERNEL_HEADER_LENGTH	(538)
#define WORK100_HEADER_LENGTH	(538)
#define WORK200_HEADER_LENGTH	(538)
#define WORK300_HEADER_LENGTH	(538)





#define BBCB_LENGTH	(24)
#define UK_LENGTH	(528)
//#define KERNEL_LENGTH	(7690240)
//#define KERNEL_LENGTH	(12582912)	//MAX LENGTH
#define KERNEL_LENGTH	(12)	//MAX LENGTH
#define WORK100_LENGTH	(4096) //MAX LENGTH
#define WORK200_LENGTH	(4096) //MAX LENGTH
#define WORK300_LENGTH	(4096) //MAX LENGTH

/******  LoaderPartition  ******/
#define LD_HEADER_LENGTH	(12)

/* Size of Download info */
#define LD_AREA_DOWNLOAD_INFO_SIZE (14)  /* size of download_info, same with downloader loadercore */
/* Size of OTA parameters. */
#define OTA_PARAMETER_SIZE (60)	/* included in shared-mem, same with downloader loadercore */

/* size of shared-mem, same with downloader loadercore */
#define LD_SHARED_MEM_SIZE		(2 * OTA_PARAMETER_SIZE + 1 + 1 + 4 + 6)

/* = LDR_SHARED_MEM_SIZE + LDR_AREA_DOWNLOAD_INFO_SIZE */
#define LD_LENGTH	(LD_SHARED_MEM_SIZE + LD_AREA_DOWNLOAD_INFO_SIZE)

#define EC_LENGTH	(4)
#define LD_MODIFY_FLAG_LENGTH	(1)
#define LD_REBOOT_FLAG_LENGTH	(1)
#define LD_DOWNLOAD_MODE_LENGTH	(1)
#define LD_STANDBY_FLAG_LENGTH	(1)

/* LD_HEADER_LENGTH(12) + LD_LENGTH(variable) + EC_LENGTH(4) + Modifyflag(1) + RebootFlag(1) + DownloadMode(1) */
#define LDFLAG_LENGTH	(LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + LD_MODIFY_FLAG_LENGTH + \
	LD_REBOOT_FLAG_LENGTH + LD_DOWNLOAD_MODE_LENGTH + LD_STANDBY_FLAG_LENGTH)

/******  LoaderPartition  ******/

#define HWCONFIG_LENGTH	(36)	//BBCB_HEADER + BBCB_PAYLOAD
#define LDSEC_LENGTH	(1066)	//UK_HEADER + UK_PAYLOAD


#define ZAPPER_FLASH_MAX_ADDRESS	(0x8000000) //nand flash max address
#define ZAPPER_FLASH_DEV	(0x0) //nand 0

/* End---ZAPPER Info in nand---*/

/* Start---ZAPPER key config ---*/
#define KEY_DETECT_PERIOD	(500) //ms
/* End---ZAPPER key config---*/

#endif
