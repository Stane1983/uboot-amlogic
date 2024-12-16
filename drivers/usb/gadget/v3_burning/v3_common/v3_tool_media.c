// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
  *Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "../include/v3_tool_def.h"
#include <fdtdec.h>
#include <dm/ofnode.h>
#include <asm/amlogic/arch/cpu_config.h>
#include <u-boot/sha256.h>
#include <amlogic/nocs_seb.h>
#include <asm/global_data.h>
#include <amlogic/emmc_partitions.h>
#include <amlogic/cpu_id.h>

#define DWN_ERR FB_ERR
#define BOOTLOADER_MAX_SZ   (0x2 << 20)
#define DTB_MAX_SZ          (256 << 10)
#define DWN_DBG FB_DBG
#define DWN_ERR FB_ERR
#define DWN_MSG FB_MSG
#define DWN_WRN FB_WRN

DECLARE_GLOBAL_DATA_PTR;

//make sure logic part sz == dtb configured sz
static int _assert_logic_partition_cap(const char *thePartName, const uint64_t nandPartCap)
{
	extern struct partitions  *part_table;
	int num_parts = 0;
	int partIndex                   = 0;
	struct partitions  *thePart     = NULL;

	if (!part_table)
		return 0;
	if (store_get_type() != BOOT_EMMC && store_get_type() != BOOT_NAND_NFTL)
		return 0;
	num_parts = get_partition_count();
	for (thePart = part_table; partIndex < num_parts; ++thePart, ++partIndex) {
		const u64 partSzInBytes = thePart->size;

		if (memcmp(thePartName, thePart->name, strlen(thePartName)))
			continue;

		FB_DBG("cfg partSzInBytes %llx for part(%s)\n", partSzInBytes, thePartName);
		FB_MSG("part(%s) sz: cfg %llx , flash %llx\n", thePartName, partSzInBytes, nandPartCap);
		if (partIndex + 1 == num_parts) {
			FB_MSG("last part not need check sz\n");
			return 0;
		}
		if (partSzInBytes > nandPartCap) {
			FB_EXIT("partSz of logic part(%s): sz dts %llx > Sz flash %llx\n",
					thePartName, partSzInBytes, nandPartCap);
		}

		return 0;
	}

	FB_EXIT("Can't find your download part(%s)\n", thePartName);
}

#if 1//storage wrapper
static int is_bootloader_discrte(bool *discreteMode)
{
	struct storage_info_t storeInfo;
	if (store_get_device_info(&storeInfo)) {
		FB_ERR("Fail get store dev info\n");
		return __LINE__;
	}
	*discreteMode = ((DISCRETE_BOOTLOADER == storeInfo.mode) || (ADVANCE_BOOTLOADER == storeInfo.mode));
	return 0;
}

int bootloader_copy_sz(void)
{
	return store_boot_copy_size("bootloader");
}

/*#define _NOCS_TEST_ON_NOSCS_*/
// <0 exception, 0 -- no nocs scs , 1 -- nocs scs
static int is_nocs_scs_chip(void)
{
#ifdef CONFIG_UPDATE_UBOOT_NOCS
	nocs_chip_type chip_type = NOCS_UNDEFINE;
	nocs_scs_status scs_sta  = NOCS_SCS_DISABLE;
	bool is_nocs_chip = false;

	if (get_nocs_inf(&chip_type, &scs_sta) != SEB_NO_ERROR) {
		FB_ERR("Exception in check nocs info!\n");
		return -__LINE__;
	}
	DWN_MSG("ty %d, en %d\n", chip_type, scs_sta);
	is_nocs_chip = (chip_type > NOCS_UNDEFINE && chip_type < LAST_NOCS_TYPE);
	if (!is_nocs_chip)
		return 0;
#ifdef _NOCS_TEST_ON_NOSCS_
	return chip_type;
#else
	if (scs_sta == NOCS_SCS_ENABLE)
		return chip_type;
	return 0;//no care chip type if not scs enable
#endif //#ifdef _NOCS_TEST_ON_NOSCS_
#else
	return 0;
#endif//#ifdef CONFIG_UPDATE_UBOOT_NOCS
}

#ifdef CONFIG_UPDATE_UBOOT_NOCS
static nocs_boot_replace  *_nocsInfo;
static int _bootloader_write(u8 *dataBuf, unsigned int off, unsigned int binsz, const char *bootName)
{
	int iCopy = 0, ret = 0;
	const int bootCpyNum = store_boot_copy_num(bootName);
	const int bootCpySz  = (int)store_boot_copy_size(bootName);
	const enum boot_type_e medium_type = store_get_type();
	unsigned char *flash_boot = NULL;

	FB_MSG("[%s] CpyNum %d, bootCpySz 0x%x\n", bootName, bootCpyNum, bootCpySz);
	if (binsz + off > bootCpySz)
		FBS_EXIT(_ACK, "bootloader sz(0x%x) + off(0x%x) > bootCpySz 0x%x\n", binsz, off, bootCpySz);

	if (off) {
		FBS_ERR(_ACK, "current only 0 supported!\n");
		return -__LINE__;
	}
	do {
		int chip_type = is_nocs_scs_chip();

		if (chip_type < 0) {
			FBS_ERR(_ACK, "Fail in get nocs info\n");
			return -__LINE__;
		}
		if (!chip_type)//no scs-en nocs chip
			break;

		if (chip_type < LAST_NOCS_TYPE) {
			flash_boot = (unsigned char *)(V3_DOWNLOAD_VERIFY_INFO + 512);
			int boot_cpy = 1;
			const int usb_boot =
				(v3tool_work_mode_get() == V3TOOL_WORK_MODE_USB_PRODUCE);
			nocs_boot_replace nocs_normal_areas;
			int i = 0;
			int ret = 0;

			boot_cpy = usb_boot ? 1 : store_bootup_bootidx("bootloader");
			FB_MSG("NOCS chip %d with SCS, cur cpy %d\n", chip_type, boot_cpy);
			ret = store_boot_read(bootName, boot_cpy, 0, flash_boot);
			if (ret) {
				FBS_ERR(_ACK, "fail in read bootloader cpy%d\n", boot_cpy);
				return -__LINE__;
			}
			if (nocs_secure_boot_modify(chip_type, flash_boot, bootCpySz,
						dataBuf, binsz, &nocs_normal_areas)
					!= SEB_NO_ERROR) {
				FBS_ERR(_ACK, "Fail in get nocs upgrade areas\n");
				return -__LINE__;
			}
			if (!nocs_normal_areas.area_num || nocs_normal_areas.area_num > 4) {
				FBS_ERR(_ACK, "Invalid upgrade area num %d\n",
						nocs_normal_areas.area_num);
				return -__LINE__;
			}
			for (i = 0; i < nocs_normal_areas.area_num; ++i) {
				unsigned int _off = nocs_normal_areas.offset[i];
				unsigned int sz = nocs_normal_areas.size[i];

				DWN_MSG("wr normal[%d] off 0x%x, sz 0x%x\n", i, _off, sz);
				memcpy(flash_boot + _off, dataBuf + _off, sz);
			}
			_nocsInfo = (nocs_boot_replace  *)V3_DOWNLOAD_VERIFY_INFO;
			memcpy(_nocsInfo, &nocs_normal_areas, sizeof(nocs_boot_replace));
		} else {
			FBS_ERR(_ACK, "scs en but chip %d unsupported\n", chip_type);
			return -__LINE__;
		}
	} while (0);

	for (; iCopy < bootCpyNum; ++iCopy) {
		if (medium_type == BOOT_EMMC)
			if (!store_boot_copy_enable(iCopy)) {
				FB_MSG("skip not EN cpy%d\n", iCopy);
				continue;
			}
		ret = store_boot_write(bootName, iCopy, binsz, flash_boot ? flash_boot : dataBuf);
		if (ret)
			FBS_EXIT(_ACK, "FAil in program[%s] at copy[%d]\n", bootName, iCopy);
	}
	if (flash_boot)
		memcpy(flash_boot, dataBuf, binsz);//for verify
	else
		_nocsInfo = NULL;

	return 0;
}
#else
static int _bootloader_write(u8 *dataBuf, unsigned int off,
			     unsigned int binsz, const char *bootName)
{
	int iCopy = 0, ret = 0;
	const int bootCpyNum = store_boot_copy_num(bootName);
	const int bootCpySz  = (int)store_boot_copy_size(bootName);
	const enum boot_type_e medium_type = store_get_type();

	FB_MSG("[%s] CpyNum %d, bootCpySz 0x%x\n", bootName, bootCpyNum, bootCpySz);
	if (binsz + off > bootCpySz)
		FBS_EXIT(_ACK, "bootloader sz(0x%x) + off(0x%x) > bootCpySz 0x%x\n",
				binsz, off, bootCpySz);

	if (off) {
		FBS_ERR(_ACK, "current only 0 supported!\n");
		return -__LINE__;
	}

	for (; iCopy < bootCpyNum; ++iCopy) {
		if (medium_type == BOOT_EMMC)
			if (!store_boot_copy_enable(iCopy)) {
				FB_MSG("skip not EN cpy%d\n", iCopy);
				continue;
			}
		ret = store_boot_write(bootName, iCopy, binsz, dataBuf);
		if (ret)
			FBS_EXIT(_ACK, "FAil in program[%s] at copy[%d]\n", bootName, iCopy);
	}

	return 0;
}
#endif//#ifdef CONFIG_UPDATE_UBOOT_NOCS

static p_payload_info_t _bl2x_mode_detect(u8 *dataBuf)
{
	p_payload_info_t pInfo      = (p_payload_info_t)(dataBuf + BL2_SIZE);

	if (pInfo->hdr.nMagicL == AML_MAGIC_HDR_L  && pInfo->hdr.nMagicR == AML_MAGIC_HDR_R) {
		FB_MSG("aml log : bootloader blxx mode!\n");
		return pInfo;
	}
	return NULL;
}

#ifdef CONFIG_SHA256
static int _bl2x_mode_check_header(void *pInfo)
{
	p_payload_info_hdr_t hdr    = &((p_payload_info_t)pInfo)->hdr;
	p_payload_info_hdr_v2 v2hdr    = (p_payload_info_hdr_v2)hdr;
	uint8_t gensum[SHA256_SUM_LEN];
	const int nItemNum = hdr->byItemNum;
	char build_info[32];

	memset(build_info, 0, ARRAY_SIZE(build_info));
	if (hdr->byVersion == 1)
		memcpy(build_info, hdr->szTimeStamp, sizeof(hdr->szTimeStamp));
	else
		memcpy(build_info, v2hdr->build_info, sizeof(v2hdr->build_info));
	printf("\naml log : info parse...\n");
	printf("\tsztimes : %s\n", build_info);
	printf("\tversion : %d\n", hdr->byVersion);
	printf("\tItemNum : %d\n", nItemNum);
	printf("\tSize    : %d(0x%x)\n",    hdr->nSize, hdr->nSize);
	if (nItemNum > 8 || nItemNum < 3)
		FBS_EXIT(_ACK, "illegal nitem num %d\n", nItemNum);

	const int nsz = sizeof(payload_info_hdr_t) +
			nItemNum * sizeof(payload_info_item_t) - SHA256_SUM_LEN;
	FB_MSG("nsz 0x%x\n", nsz);
	sha256_context ctx;
	sha256_starts(&ctx);
	sha256_update(&ctx, pInfo + 32/*(u8*)&(hdr->nMagicL)*/, nsz);
	sha256_finish(&ctx, gensum);
	int ret = memcmp(gensum, hdr->szSHA2, SHA256_SUM_LEN);
	if (ret)
		FBS_EXIT(_ACK, "hdr info sha256sum not matched\n");
	FB_MSG("hdr info sha256sum DO matched\n");

	return 0;
}
#else
#define _bl2x_mode_check_header(...) 0
#endif// #ifdef CONFIG_SHA256

static const char *_flashPayload[] = {"bl2",  "bl2e", "bl2x", "ddrfip", "devfip"};
static p_payload_info_t _blxPayloadInf;
static int _payloadInfoSz;

static int _discrete_bootloader_write(u8 *dataBuf, unsigned int off, unsigned int binsz)
{
	int bl2CopySz  = BL2_SIZE/*(int)store_boot_copy_size("bl2")*/;
	FB_MSG("bl2CopySz 0x%x, binsz 0x%x\n", bl2CopySz, binsz);
	int ret = 0;

	if (binsz > bl2CopySz) {
		_blxPayloadInf = NULL;
		p_payload_info_t pInfo = _bl2x_mode_detect(dataBuf);

		if (!pInfo) {
			ret = _bootloader_write(dataBuf + bl2CopySz, 0, binsz - bl2CopySz, "tpl");
			if (ret)
				FBS_EXIT(_ACK, "Fail in burn tpl\n");
		} else {
			if (_bl2x_mode_check_header(pInfo))
				FBS_EXIT(_ACK, "Fail in check bl2x info\n");
			char name[8];
			int nIndex = 0;
			int offPayload = 0, szPayload = 0;

			p_payload_info_hdr_t hdr    = &pInfo->hdr;
			p_payload_info_item_t pItem = pInfo->arrItems;
			_blxPayloadInf = (p_payload_info_t)V3_DOWNLOAD_VERIFY_INFO;
			_payloadInfoSz = sizeof(payload_info_hdr_t) +
					 pInfo->hdr.byItemNum * sizeof(payload_info_item_t);
			memcpy(_blxPayloadInf, pInfo, _payloadInfoSz);
			memset(name, 0, 8);

			if (CONFIG_IS_ENABLED(DISCRETE_BOOTLOADER_NEW)) {
				ret = store_boot_write("bootloader", 0xff, binsz, dataBuf);
				if (ret)
					FBS_EXIT(_ACK, "FAil program bootloader at copy 0xff\n");
				FB_MSG("Okay burn ALL bootloader\n");
				return 0;
			} else {
				for (nIndex = 1, pItem += 1; nIndex < hdr->byItemNum; ++nIndex, ++pItem) {
					memcpy(name, &pItem->nMagic, 8);
					offPayload = pItem->nOffset;
					szPayload  = pItem->nPayLoadSize;
					FB_MSG("Item[%d]%4s offset 0x%08x sz 0x%x\n",
							nIndex, name, offPayload, szPayload);
					if (!szPayload)
						continue;
					ret = _bootloader_write(dataBuf + offPayload, 0, szPayload, _flashPayload[nIndex]);
					if (ret)
						FBS_EXIT(_ACK, "Fail in flash payload %s\n", name);
				}
			}
		}
	}

	ret = _bootloader_write(dataBuf, 0, bl2CopySz, "bl2");
	if (ret)
		FBS_EXIT(_ACK, "Fail in program bl2\n");

	return 0;
}

#ifdef CONFIG_MESON_S7D
int update_boot_hdr_4_s7d_reva(u8 *data_buf, unsigned int binsz, int isread)
{
	const cpu_id_t cpuid = get_cpu_id();
	const int familyid   = cpuid.family_id;
	const int chip_rev   = cpuid.chip_rev;
	const char *magic_reva = "BLOBHDR";
	const char *magic_revb = "@ML";
	const int reva_hdr_off = 0x43e00;
	const int rva_len = 512;
	char  rva_hdr[rva_len];
	int ret = 0;

	if (familyid != MESON_CPU_MAJOR_ID_S7D)
		return 0;
	if (chip_rev != MESON_CPU_CHIP_REVISION_A)
		return 0;
	FB_MSG("x5m reva\n");
	ret = memcmp(magic_reva, data_buf, strnlen(magic_reva, 8));
	if (!ret) {
		FB_MSG("hdr match\n");
		if (!isread)
			return 0;
	}
	ret = memcmp(magic_revb, data_buf + isread * reva_hdr_off, strnlen(magic_revb, 8));
	if (ret) {
		if (isread) {
			FB_MSG("no contain rvb inf\n");
			return 0;
		}
		FB_ERR("boot inf magic err\n");
		return -__LINE__;
	}
	if (isread) {
		memcpy(rva_hdr, data_buf, rva_len);
		ret = memcmp(magic_reva, rva_hdr, strnlen(magic_reva, 8));
		if (ret) {
			FB_ERR("boot inf at reva off err\n");
			return -__LINE__;
		}
		memcpy(data_buf, data_buf + reva_hdr_off, rva_len);//recovery rvb hdr
		memcpy(data_buf + reva_hdr_off, rva_hdr, rva_len);
	} else {
		memcpy(rva_hdr, data_buf + reva_hdr_off, rva_len);
		ret = memcmp(magic_reva, rva_hdr, strnlen(magic_reva, 8));
		if (ret) {
			FB_ERR("boot inf at reva off err\n");
			return -__LINE__;
		}
		memcpy(data_buf + reva_hdr_off, data_buf, rva_len);//save rvb hdr for verify
		memcpy(data_buf, rva_hdr, rva_len);
	}
	FB_MSG("boot hdr changed\n");
	return 0;
}
#endif//#ifdef CONFIG_MESON_S7D

int bootloader_write(u8 *dataBuf, unsigned off, unsigned binsz)
{
	bool discreteMode = false;
	//p_payload_info_t pInfo = _bl2x_mode_detect(dataBuf);

	//_bl2x_mode_check_header(pInfo);
	if (is_bootloader_discrte(&discreteMode))
		return -__LINE__;
#ifdef CONFIG_MESON_S7D
	if (update_boot_hdr_4_s7d_reva(dataBuf, binsz, 0)) {
		FB_ERR("Fail in update x5m inf\n");
		return -__LINE__;
	}
#endif//#ifdef CONFIG_MESON_S7D
	if (!discreteMode) {
		return _bootloader_write(dataBuf, off, binsz, "bootloader");
	} else {
		return _discrete_bootloader_write(dataBuf, off, binsz);
	}
	return -__LINE__;
}

static int _bootloader_read(u8 *pBuf, unsigned off, unsigned binsz, const char *bootName)
{
	int iCopy = 0;
	const int bootCpyNum = store_boot_copy_num(bootName);
	const int bootCpySz  = (int)store_boot_copy_size(bootName);
	const enum boot_type_e medium_type = store_get_type();
	int validCpyNum = bootCpyNum;//at least valid cpy num
	int actVldCpyNum = 0;//actual valid copy num
	int ret = 0;

#if CONFIG_NAND_BL2_VALID_NUM
	if (strcmp("tpl", bootName) && strcmp("devfip", bootName))
		if (CONFIG_NAND_BL2_VALID_NUM != -1)
			validCpyNum = CONFIG_NAND_BL2_VALID_NUM;
#endif // #if CONFIG_NAND_BL2_VALID_NUM
#if CONFIG_NAND_TPL_VALID_NUM
	if (!strcmp("tpl", bootName) || !strcmp("devfip", bootName))
		if (CONFIG_NAND_TPL_VALID_NUM != -1)
			validCpyNum = CONFIG_NAND_TPL_VALID_NUM;
#endif// #if CONFIG_NAND_TPL_VALID_NUM

	if (binsz + off > bootCpySz) {
		FBS_ERR(_ACK, "bootloader sz(0x%x) + off(0x%x) > bootCpySz 0x%x\n", binsz, off, bootCpySz);
		return -__LINE__;
	}
	if (off)
		FBS_EXIT(_ACK, "current only 0 supported!\n");

	for (iCopy = 0; iCopy < bootCpyNum; ++iCopy) {
		void *dataBuf = iCopy ? pBuf + binsz : pBuf;

		if (medium_type == BOOT_EMMC)
			if (!store_boot_copy_enable(iCopy)) {
				FB_MSG("skip not EN cpy%d\n", iCopy);
				--validCpyNum;
				continue;
			}
		ret = store_boot_read(bootName, iCopy, binsz, dataBuf);
		if (ret) {
			FB_ERR("Fail to read boot[%s] at copy[%d]\n", bootName, iCopy);
			continue;
		}
		if (iCopy) {
			if (!actVldCpyNum) {//NO valid cpy yet, so copy0 NOT valid also
				memcpy(pBuf, dataBuf, binsz);
			}
			if (memcmp(pBuf, dataBuf, binsz)) {
				FBS_ERR(_ACK, "[%s] copy[%d] content NOT the same as copy[0]\n",
						bootName, iCopy);//maybe ddr err as nand not err
				memset(pBuf, 0, binsz);
				return -__LINE__;
			}
		}
		++actVldCpyNum;
	}
	if (actVldCpyNum < validCpyNum) {
		FBS_ERR(_ACK, "[%s]actual valid copy num %d < configured num %d\n",
				bootName, actVldCpyNum, validCpyNum);
		memset(pBuf, 0, binsz);
		return -__LINE__;
	}

	return 0;
}

static int _discrete_bootloader_read(u8 *dataBuf, unsigned int off, unsigned int binsz)
{
	int bl2CopySz  = BL2_SIZE/*(int)store_boot_copy_size("bl2")*/;
	int ret = 0;

	FB_MSG("bl2CopySz 0x%x, binsz 0x%x\n", bl2CopySz, binsz);
	ret = _bootloader_read(dataBuf, 0, bl2CopySz, "bl2");
	if (ret)
		FBS_EXIT(_ACK, "Fail in read bl2\n");
	if (binsz <= bl2CopySz)
		return 0;
	memset(dataBuf + bl2CopySz, 0, bl2CopySz);//clear 2k after bl2_size

	if (!_blxPayloadInf) {
		ret = _bootloader_read(dataBuf + bl2CopySz, 0, binsz - bl2CopySz, "tpl");
		if (ret)
			FBS_EXIT(_ACK, "Fail in read tpl\n");
	} else {
		char name[8];
		int nIndex = 0;

		if (!_blxPayloadInf)
			FBS_EXIT(_ACK, "exception, _blxPayloadInf null\n");

		p_payload_info_t pInfo      = _blxPayloadInf;
		p_payload_info_hdr_t hdr    = &pInfo->hdr;
		p_payload_info_item_t pItem = pInfo->arrItems;

		int offPayload = 0, szPayload = 0;

		memset(name, 0, 8);
		if (CONFIG_IS_ENABLED(DISCRETE_BOOTLOADER_NEW)) {
				ret = store_boot_read("bootloader", 0xff, binsz, dataBuf);
				if (ret)
					FBS_EXIT(_ACK, "FAil read bootloader at copy 0xff\n");
				FB_MSG("Okay read ALL bootloader\n");
		} else {
			for (nIndex = 1, pItem += 1; nIndex < hdr->byItemNum; ++nIndex, ++pItem) {
				memcpy(name, &pItem->nMagic, sizeof(unsigned int));
				offPayload = pItem->nOffset;
				szPayload  = pItem->nPayLoadSize;
				FB_MSG("Item[%d]%4s offset 0x%08x sz 0x%x\n",
						nIndex, name, offPayload, szPayload);
				if (!szPayload)
					continue;
				ret = _bootloader_read(dataBuf + offPayload, 0, szPayload,
						_flashPayload[nIndex]);
				if (ret)
					FBS_EXIT(_ACK, "Fail in read payload %s\n", name);
			}
		}
		memcpy(dataBuf + BL2_SIZE, _blxPayloadInf, _payloadInfoSz);
	}

	return 0;
}

int bootloader_read(u8 *pBuf, unsigned int off, unsigned int binsz)
{
	bool discreteMode = false;
	int ret = 0;

	if (is_bootloader_discrte(&discreteMode))
		return -__LINE__;

	if (discreteMode)
		return _discrete_bootloader_read(pBuf, off, binsz);

	ret = _bootloader_read(pBuf, off, binsz, "bootloader");
	if (ret)
		FBS_EXIT(_ACK, "Fail in read bootloader, ret %d\n", ret);
#ifdef CONFIG_UPDATE_UBOOT_NOCS
	if (_nocsInfo) {
		int i = 0;
		u8 *src_data = (u8  *)(V3_DOWNLOAD_VERIFY_INFO + 512);
		const unsigned int normal_num = _nocsInfo->area_num;

		if (normal_num > MAX_SKIP_NUM)
			FBS_EXIT(_ACK, "Exception, err nocs area num %d\n", normal_num);
		for (i = 0; i < normal_num; ++i) {
			unsigned int _off = _nocsInfo->offset[i];
			unsigned int sz  = _nocsInfo->size[i];

			DWN_MSG("rd normal[%d] off 0x%x, sz 0x%x\n", i, _off, sz);
			memcpy(src_data + _off, pBuf + _off, sz);
		}
		memcpy(pBuf, src_data, binsz);
	}
#endif//#ifdef CONFIG_UPDATE_UBOOT_NOCS
#ifdef CONFIG_MESON_S7D
	if (update_boot_hdr_4_s7d_reva(pBuf, binsz, 1)) {
		FB_ERR("Fail in update x5m inf\n");
		return -__LINE__;
	}
#endif//#ifdef CONFIG_MESON_S7D

	return 0;
}

//@rw: 0---read, 1---write, 2---iread
int store_dtb_rw(void *buf, unsigned int dtbsz, int rw)
{
	int ret = 0;
	const unsigned int dtb_cap = store_rsv_size("dtb");

	if (dtb_cap <= dtbsz)
		FBS_EXIT(_ACK, "dtb sz 0x%x > cap 0x%x\t", dtbsz, dtb_cap);

	switch (rw) {
	case 2: {//iread
			ret = store_rsv_read("dtb", dtbsz, buf);
			if (ret)
				FBS_EXIT(_ACK, "err(%d) in read dtb\t", ret);
			break;
	}
	case 0: {//read
		 //TODO: add dtb parser
			FBS_EXIT(_ACK, "dtb parser not implemented yet\t");
	} break;
	case 1: {//write
			ret = store_rsv_erase("dtb");
			if (ret)
				FBS_EXIT(_ACK, "Fail erase dtb, ret %d\n", ret);
			ret = store_rsv_write("dtb", dtbsz, buf);
			if (ret)
				FBS_EXIT(_ACK, "Fail in dtb write, ret %d\t", ret);
	} break;
	default:
		FBS_EXIT(_ACK, "err dtb rw %d\n", rw);
	}

	return 0;
}
#endif// #if 1//storage wrapper


int v3tool_media_check_image_size(int64_t imgsz, const char *_part)
{
	int ret = 0;
	u64 partcap = 0;
	const char *part = _part;

	if (!strncmp("bootloader-", part, strnlen("bootloader-", 11)))
		part = "bootloader";
	if (!strcmp("bootloader", part)) {
		const unsigned int bootSz = bootloader_copy_sz();
		if (imgsz > bootSz)
			FBS_EXIT(_ACK, "imgsz 0x%llx > copy sz 0x%x !\t", imgsz, bootSz);
		return 0;
	}

	if (!strcmp("_aml_dtb", part)) {
		const unsigned int dtb_cap = store_rsv_size("dtb");

		if (imgsz >= dtb_cap)
			FB_EXIT("imgsz 0x%llx >= max sz 0x%x\n", imgsz, dtb_cap);
		return 0;
	}
	if (!strcmp("gpt", part)) {
		if (imgsz >= 0x100000) {
			FB_EXIT("imgsz 0x%llx >= max sz 1M\n", imgsz);
		}
		return 0;
	}

	partcap = store_part_size(part);
	if (!partcap) {
		DWN_ERR("Fail to get size for part %s\n", part);
		return __LINE__;
	}
	DWN_MSG("flash LOGIC partcap 0x%llxB\n", partcap);
	if (imgsz > partcap) {
		DWN_ERR("imgsz 0x%llx out of cap 0x%llx\n", imgsz, partcap);
		return __LINE__;
	}
	ret = _assert_logic_partition_cap(part, partcap);
	if (ret) {
		DWN_ERR("Fail in _assert_logic_partition_cap\n");
		return __LINE__;
	}

	return 0;
}

static int _optimusWorkMode = V3TOOL_WORK_MODE_NONE;

int v3tool_work_mode_get(void)
{
	return _optimusWorkMode;
}

int v3tool_work_mode_set(int workmode)
{
	_optimusWorkMode = workmode;
	return 0;
}

static int _disk_intialed_ok;

int v3tool_is_flash_erased(void)
{
	return _disk_intialed_ok >> 16;
}

static int should_load_env(void)
{
	if (IS_ENABLED(CONFIG_OF_CONTROL))
		return ofnode_conf_read_int("load-environment", 1);

	if (IS_ENABLED(CONFIG_DELAY_ENVIRONMENT))
		return 0;

	return 1;
}

static int initr_env(void)
{
	/*initialize environment */
	if (should_load_env()) {
		DWN_MSG("usb producing env_relocate\n");
		env_relocate();
	}
	return 0;
}

struct mtd_partition *__attribute__((weak)) get_partition_table(int *partitions)
{FB_WRN("get_partition_table undefined\n"); return NULL; }

int __attribute__((weak)) sheader_need(void) {FB_WRN("sheader_need undefined\n"); return 0; }
void __attribute__((weak)) sheader_load(void *addr) {FB_WRN("sheader_load undefined\n"); }

int v3tool_storage_init(const int eraseFlash, unsigned int dtbImgSz, unsigned int gptImgSz)
{
	int ret = 0;
	unsigned char *dtbLoadedAddr = (unsigned char *)V3_DTB_LOAD_ADDR;
	unsigned char *gptLoadedAddr = (unsigned char *)V3_GPT_LOAD_ADDR;

	if (v3tool_work_mode_get() != V3TOOL_WORK_MODE_USB_PRODUCE) {//inited in other work mode
		/*DWN_MSG("Exit before re-init\n");*/
		/*store_exit();*/
	}

	if (sheader_need())
		sheader_load((void *)V3_PAYLOAD_LOAD_ADDR);

	if (dtbImgSz > 0) {
		FB_MSG("to check dtb\n");
		ret = check_valid_dts(dtbLoadedAddr);
		if (ret < 0)
			FBS_EXIT(_ACK, "Fail at check dtb\n");
	}

	ret = store_init(1);
	if (ret <= 0)
		FBS_EXIT(_ACK, "Fail in store init %d, ret %d\n", 1, ret);

#ifdef CONFIG_MMC
	mmc_partition_init();
#endif

	int initFlag = 0;
	int erase_key = 0, force_erase_all = 0;

	switch (eraseFlash) {
	case 0://NO erase
	case 5://NO erase
		initFlag = 1;
		break;
	case 3://erase all(with key)
	case 1://normal erase, store init 3
		initFlag = 3;
		erase_key = (eraseFlash == 3);
		break;
	case 4: {//force erase all
		force_erase_all = 1;
		initFlag = 4;
		break;
	}
	case 2:
		initFlag = 4;
		break;
	default:
		FBS_EXIT(_ACK, "Unsupported erase flag %d\n", eraseFlash);
	}

	FB_MSG("eraseFlash %d, initFlag %d\n", eraseFlash, initFlag);
	if (eraseFlash == 5) {//erase key only
		ret = store_rsv_erase("key");
		if (ret)
			FBS_EXIT(_ACK, "disk_initial 5, Fail in erase key\n");
	} else if (initFlag > 1) {
		FB_MSG("dtbImgSz 0x%x, gptImgSz 0x%x\n", dtbImgSz, gptImgSz);
		if (store_get_type() == BOOT_EMMC) {
			if (gptImgSz) {//gpt first if both exist gpt/dtb
				FB_MSG("To erase dtb && update gpt for gpt from dtb\n");
				ret = store_rsv_erase("dtb");
				if (ret)
					FB_WRN("Fail in erase dtb\n");
				ret = store_gpt_ops(gptImgSz, gptLoadedAddr, 1);
				if (ret)
					FB_WRN("Fail in update gpt\n");
			} else if (dtbImgSz) {
				FB_MSG("To erase gpt && update dtb for gpt to dtb\n");
				store_gpt_erase();
				ret = store_rsv_write("dtb", dtbImgSz, dtbLoadedAddr);
				if (ret)
					FB_WRN("Fail in update dtb\n");
			}
		}

		if (erase_key) {
			if (store_rsv_protect("key", false))
				FBS_EXIT(_ACK, "Fail in disprotect key\n");
		}
		if (force_erase_all) {
			if (store_rsv_protect(NULL, false))
				FBS_EXIT(_ACK, "Fail in disprotect all rsv\n");
		}
		if (is_nocs_scs_chip() > 0) {
			if (store_get_type() != BOOT_EMMC) {
				FBS_ERR(_ACK, "nocs scs chip but storage not emmc\n");
				return -__LINE__;
			}
			FB_MSG("remain bootloader as nocs scs chip\n");
			ret = usb_burn_erase_data(1);
		} else {
			if (v3tool_work_mode_get() == V3TOOL_WORK_MODE_USB_PRODUCE ||
				store_get_type() != BOOT_EMMC) {
				ret = store_erase(NULL, 0, 0, 0);
			} else {
				FB_MSG("remain bootloader as not usb boot\n");
				ret = usb_burn_erase_data(1);
			}
		}
		if (eraseFlash == 3) {
			FB_MSG("Erase unifykey\n");
			ret = store_rsv_erase("key");
			if (ret)
				FBS_EXIT(_ACK, "disk_initial 3, Fail in erase key\n");
		}
		if (ret)
			FBS_EXIT(_ACK, "Fail in erase flash, ret[%d]\n", ret);
#ifdef CONFIG_BACKUP_PART_NORMAL_ERASE
		if (backupPartSz) {
			FB_MSG("restore BackupPart %s from mem\n", BackupPart);
			ret = store_write(BackupPart, 0, backupPartSz, BackupPartAddr);
			if (ret)
				FB_MSG("FAil in restore part %s\n", BackupPart);
		}
#endif//#ifdef CONFIG_BACKUP_PART_NORMAL_ERASE
	}

	if (v3tool_work_mode_get() == V3TOOL_WORK_MODE_USB_PRODUCE) {
		if (!_disk_intialed_ok) {//last disk_initial also okay
			initr_env();//can only be called once
		}
	}
	_disk_intialed_ok  = 1;
	if (eraseFlash && eraseFlash < 5)
		_disk_intialed_ok += (1 << 16);

	if (dtbImgSz) {//for key init, or fail when get /unifykey
		unsigned long fdtAddr = (unsigned long)dtbLoadedAddr;
		unsigned int fdtsz    = 0;

#ifdef CONFIG_MULTI_DTB
		fdtAddr = get_multi_dt_entry(fdtAddr);
#endif// #ifdef CONFIG_MULTI_DTB
		ret = fdt_check_header((char *)fdtAddr);
		if (ret)
			FBS_EXIT(_ACK, "Fail in fdt check header\n");

		fdtsz    = fdt_totalsize((char *)fdtAddr);
		if (fdtAddr != (unsigned long)dtbLoadedAddr)
			memmove((char *)dtbLoadedAddr, (char *)fdtAddr, fdtsz);
		env_set_ulong("dtb_mem_addr", (unsigned long)dtbLoadedAddr);
	}

	return 0;
}

static int _v3tool_is_busy;
static char *_v3tool_media_busy_info = "";

void v3tool_media_set_busy(const char *busyInfo)
{
	_v3tool_is_busy = 1;
	_v3tool_media_busy_info = fb_response_str - 4;
	strncpy(_v3tool_media_busy_info, "INFO", 4 + 1);//add terminated 0
	if (busyInfo)
		strncpy(_v3tool_media_busy_info + 4, busyInfo, 64);
}

void v3tool_media_set_free(const char *info)
{
	_v3tool_is_busy = 0;
	_v3tool_media_busy_info = fb_response_str - 4;
	strncpy(_v3tool_media_busy_info, "OKAY", 4 + 1);
	if (info)
		strncpy(_v3tool_media_busy_info + 4, info, 64);
}

int v3tool_media_is_busy(void)
{
	return _v3tool_is_busy;
}

