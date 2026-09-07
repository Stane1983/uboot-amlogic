// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <image.h>
#include <linux/compat.h>
#include <linux/libfdt.h>
#include <android_image.h>
#if defined(CONFIG_ZIRCON_BOOT_IMAGE)
#include <amlogic/zircon/image.h>
#endif
#include <asm/amlogic/arch/bl31_apis.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <amlogic/store_wrapper.h>
#include <amlogic/aml_efuse.h>
#include <malloc.h>
#include <amlogic/emmc_partitions.h>
#include <version.h>
#include <amlogic/image_check.h>
#include <fs.h>
#include <gzip.h>
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
#include <amlogic/partition_encryption.h>
#endif
#ifdef CONFIG_AVB2
#include <amlogic/libavb/libavb.h>
#endif
#include <amlogic/aml_profile.h>
#include <amlogic/keyunify.h>
#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
#include <amlogic/amfc.h>
#endif

u32 ramdisk_off = 0;

#ifndef IS_FEAT_BOOT_VERIFY
//#define IS_FEAT_BOOT_VERIFY() 0 //always undefined as IS_FEAT_BOOT_VERIFY is function not marco
#endif// #ifndef IS_FEAT_BOOT_VERIFY
int __attribute__((weak)) store_logic_read(const char *name, loff_t off, size_t size, void *buf)
{ return store_read(name, off, size, buf);}

#define debugP(fmt...) //printf("[Dbg imgread]L%d:", __LINE__),printf(fmt)
#define errorP(fmt...) do {pr_err("Err imgread(L%d):", __LINE__); pr_err(fmt); } while (0)
#define wrnP(fmt...)   pr_warn("wrn:" fmt)
#define MsgP(fmt...)   debug("[imgread]" fmt)

#define IMG_PRELOAD_SZ  (1U<<20) //Total read 1M at first to read the image header
#define PIC_PRELOAD_SZ  (8U<<10) //Total read 4k at first to read the image header
#define RES_OLD_FMT_READ_SZ (8U<<20)

#define MAX_RAMDISK_SIZE	SZ_64M
#define MAX_KERNEL_SIZE		SZ_64M
#define MAX_DTB_SIZE		SZ_16M

typedef struct __aml_enc_blk{
	unsigned int  nOffset;
	unsigned int  nRawLength;
	unsigned int  nSigLength;
	unsigned int  nAlignment;
	unsigned int  nTotalLength;
	unsigned char szPad[12];
	unsigned char szSHA2IMG[32];
	unsigned char szSHA2KeyID[32];
}t_aml_enc_blk;

#define AML_SECU_BOOT_IMG_HDR_MAGIC        "AMLSECU!"
#define AML_SECU_BOOT_IMG_HDR_MAGIC_SIZE   (8)
#define AML_SECU_BOOT_IMG_HDR_VESRION      (0x0905)

typedef struct {
	unsigned char magic[AML_SECU_BOOT_IMG_HDR_MAGIC_SIZE];
	//magic to identify whether it is a encrypted boot image

	unsigned int  version; //version for this header struct
	unsigned int  nBlkCnt;

	unsigned char szTimeStamp[16];

	t_aml_enc_blk   amlKernel;
	t_aml_enc_blk   amlRamdisk;
	t_aml_enc_blk   amlDTB;
} amlencryptbootimginfo;

typedef struct _boot_img_hdr_secure_boot
{
	unsigned char           reserve4ImgHdr[1024];

	amlencryptbootimginfo   encrypteImgInfo;

}AmlSecureBootImgHeader;

typedef struct{
	unsigned char           reserve4ImgHdr[2048];

	amlencryptbootimginfo   encrypteImgInfo;

}AmlSecureBootImg9Header;


#define COMPILE_TYPE_ASSERT(expr, t)       typedef char t[(expr) ? 1 : -1]
COMPILE_TYPE_ASSERT(2048 >= sizeof(AmlSecureBootImgHeader), _cc);

static const char * const white_list_boot_part[] = {
	"boot", "boot_a", "boot_b", "recovery", "recovery_a", "recovery_b"
};

static int is_andr_9_image(void* pBuffer)
{
	boot_img_hdr_t *pAHdr = (boot_img_hdr_t *)(unsigned long)pBuffer;
	int nReturn = 0;

	if (!pBuffer)
		goto exit;

	if (pAHdr->header_version)
		nReturn = 1;

exit:

	return nReturn;

}

static int _aml_get_secure_boot_kernel_size(const void *ploadaddr, u32 *ptotalenckernelsz)
{
	const amlencryptbootimginfo *amlencryptebootimginfo = 0;
	int rc = 0;
	u32 securekernelimgsz = 2048;
	u32 nblkcnt = 0;
	const t_aml_enc_blk *pblkinf = NULL;
	int secure_boot_enabled = 0;
	unsigned char *pandhdr = (unsigned char *)ploadaddr;

#ifdef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK
	secure_boot_enabled = 0;//donnot decrypt kernel/dtb if avb2 enabled
#else
	secure_boot_enabled = IS_FEAT_BOOT_VERIFY();
#endif//#ifdef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK

#ifdef CONFIG_IMAGE_CHECK
	rc = __LINE__;

	if (!ploadaddr || !ptotalenckernelsz)
		return rc;

	if (secure_boot_enabled) {
		struct aml_boot_header_t *hdr;
		ulong ncheckoffset = android_image_check_offset();

		hdr = (struct aml_boot_header_t *)(pandhdr + ncheckoffset
			- sizeof(struct aml_boot_header_t));
		if (hdr->magic != AML_BOOT_IMAGE_MAGIC || hdr->version != AML_BOOT_IMAGE_VERSION)
			return rc;
		*ptotalenckernelsz = hdr->img_size + ncheckoffset;
		return 0;
	}

	*ptotalenckernelsz = 0;
#endif
	if (is_andr_9_image(pandhdr))
		securekernelimgsz = 4096;

	amlencryptebootimginfo = (amlencryptbootimginfo *)(pandhdr + (securekernelimgsz >> 1));

	nblkcnt = amlencryptebootimginfo->nBlkCnt;

	*ptotalenckernelsz = 0;

	rc = memcmp(AML_SECU_BOOT_IMG_HDR_MAGIC, amlencryptebootimginfo->magic,
		AML_SECU_BOOT_IMG_HDR_MAGIC_SIZE);
	if (rc) { // img NOT signed
		if (secure_boot_enabled) {
			errorP("img NOT signed but secure boot enabled\n");
			return __LINE__;
		}
		*ptotalenckernelsz = 0;
		return 0;
	}
	//img signed
	if (!secure_boot_enabled) {
		errorP("Img signed but secure boot NOT enabled\n");
		return __LINE__;
	}

	if (amlencryptebootimginfo->version != AML_SECU_BOOT_IMG_HDR_VESRION) {
		errorP("magic ok but version err, err ver=0x%x\n", amlencryptebootimginfo->version);
		return __LINE__;
	}
	MsgP("szTimeStamp[%s]\n", (char *)&amlencryptebootimginfo->szTimeStamp);
	debugP("nblkcnt=%d\n", nblkcnt);

	for (pblkinf = &amlencryptebootimginfo->amlKernel; nblkcnt--; ++pblkinf) {
		const unsigned int thisblklen = pblkinf->nTotalLength;

		debugP("thisblklen=0x%x\n", thisblklen);
		securekernelimgsz += thisblklen;
	}

	*ptotalenckernelsz = securekernelimgsz;
	return 0;
}

static int do_image_read_dtb_from_knl(const char *partname,
	unsigned char *loadaddr, uint64_t lflashreadinitoff)
{
	int ret = __LINE__;
	unsigned int nflashloadlen = 0;
	unsigned int lflashreadoff = 0;
	const int preloadsz = 4096 * 2;
	unsigned int pagesz = 0;
	boot_img_hdr_t *hdr_addr = NULL;
	bool dtb_in_vendor_boot = false;
	bool cache_flag = false;
	char *upgrade_step_s = env_get("upgrade_step");
	char *pbuffpreload = NULL;
#ifdef CONFIG_AVB2
	u64 original_size = 0;
	bool vendor_boot_preload = true;
#endif
	u32 securekernelimgsz = 0;

	pbuffpreload = (char *)malloc(preloadsz);
	if (!pbuffpreload) {
		printf("aml log : Fail to allocate memory for %s!\n", partname);
		return __LINE__;
	}
	memset(pbuffpreload, 0, preloadsz);
	hdr_addr = (boot_img_hdr_t *)pbuffpreload;

	lflashreadoff = lflashreadinitoff;
	nflashloadlen = preloadsz;
	debugP("sizeof preloadsz=%u\n", nflashloadlen);

	if (upgrade_step_s && (strcmp(upgrade_step_s, "3") == 0) &&
		(strcmp(partname, "recovery") == 0)) {
		loff_t len_read;

		MsgP("read recovery.img from cache\n");
		ret = fs_set_blk_dev("mmc", "1:2", FS_TYPE_EXT);
		if (ret) {
			errorP("Fail to set blk dev cache\n");
			cache_flag = false;
		} else {
			fs_read("/recovery/recovery.img", (unsigned long)pbuffpreload,
					lflashreadoff, lflashreadoff, &len_read);
			if (lflashreadoff != len_read) {
				errorP("Fail to read recovery.img from cache\n");
				cache_flag = false;
			} else {
				cache_flag = true;
			}
		}
	}

	if (!cache_flag) {
		MsgP("read from part: %s\n", partname);
		ret = store_logic_read(partname, lflashreadoff, nflashloadlen, pbuffpreload);
		if (ret) {
			errorP("Fail to read 0x%xB from part[%s] at offset 0\n",
					nflashloadlen, partname);
			ret = __LINE__;
			goto exit;
		}
	}

#if CONFIG_PARTITION_ENCRYPTION_LOCAL
	part_dec(partname, (u8 *)pbuffpreload, nflashloadlen,
			(u8 *)pbuffpreload, nflashloadlen, lflashreadoff);
#endif

	if (genimg_get_format(hdr_addr) != IMAGE_FORMAT_ANDROID) {
		errorP("Fmt unsupported! only support 0x%x\n", IMAGE_FORMAT_ANDROID);
		ret = __LINE__;
		goto exit;
	}

	if (is_android_v3_header_image((void *)hdr_addr)) {
		char *slot_name;
		unsigned long ramdisk_size_v3, dtb_size_v3;
		unsigned long ramdisk_table_size;
		unsigned long totalsize;
		u64 vendorboot_part_sz;

		dtb_in_vendor_boot = true;
		slot_name = env_get("slot-suffixes");
		if (strcmp(slot_name, "0") == 0)
			strcpy((char *)partname, "vendor_boot_a");
		else if (strcmp(slot_name, "1") == 0)
			strcpy((char *)partname, "vendor_boot_b");
		else
			strcpy((char *)partname, "vendor_boot");

		vendorboot_part_sz = store_part_size(partname);

#ifdef CONFIG_DIAG_MODE
		char diag_state_buf[200] = {0x0};
		u64 rc_diag = -1;

		rc_diag = store_part_size("diag_vendor_boot");

		if ((key_unify_read("diag_state", diag_state_buf,
			sizeof(diag_state_buf)) == 0) &&
			(strcmp(diag_state_buf, "Test") == 0) &&
			(rc_diag != -1)) {
			printf("read dtb from diag_vendor_boot\n");
			strcpy((char *)partname, "diag_vendor_boot");
		}
#endif
		MsgP("partname = %s\n", partname);
		ret = store_logic_read(partname, lflashreadoff, nflashloadlen, pbuffpreload);
		if (ret) {
			errorP("Fail to read 0x%xB from part[%s] at offset 0\n",
					nflashloadlen, partname);
			ret = __LINE__;
			goto exit;
		}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		part_dec(partname, (u8 *)pbuffpreload, nflashloadlen,
				(u8 *)pbuffpreload, nflashloadlen, lflashreadoff);
#endif
		p_vendor_boot_img_hdr_t pvendorimghdr = (p_vendor_boot_img_hdr_t)pbuffpreload;

		ret = vendor_boot_image_check_header(pvendorimghdr);
		if (!ret) {
			pagesz = pvendorimghdr->page_size;
			ramdisk_size_v3 = ALIGN(pvendorimghdr->vendor_ramdisk_size, pagesz);
			dtb_size_v3 = ALIGN(pvendorimghdr->dtb_size, pagesz);
			nflashloadlen = dtb_size_v3;
			lflashreadoff = ramdisk_size_v3 + VENDOR_BOOT_IMG_HDR_SIZE;

			debugP("ramdisk_size_v3 0x%x, totalSz 0x%lx\n",
				pvendorimghdr->vendor_ramdisk_size, ramdisk_size_v3);
			debugP("dtb_size_v3 0x%x, totalSz 0x%lx\n",
				pvendorimghdr->dtb_size, dtb_size_v3);
			debugP("lflashreadoff=0x%x\n", lflashreadoff);
			debugP("nflashloadlen=0x%x\n", nflashloadlen);
			totalsize = nflashloadlen + lflashreadoff;

			if (pvendorimghdr->header_version > 3) {
				MsgP("vendor_ramdisk_table_size: 0x%x\n",
				pvendorimghdr->vendor_ramdisk_table_size);
				MsgP("vendor_ramdisk_table_entry_num: 0x%x\n",
				pvendorimghdr->vendor_ramdisk_table_entry_num);
				MsgP("vendor_ramdisk_table_entry_size: 0x%x\n",
				pvendorimghdr->vendor_ramdisk_table_entry_size);
				MsgP("vendor_bootconfig_size: 0x%x\n",
				pvendorimghdr->vendor_bootconfig_size);
				ramdisk_table_size =
					ALIGN(pvendorimghdr->vendor_ramdisk_table_size,
					pagesz);
				totalsize += ramdisk_table_size;
				totalsize +=
					ALIGN(pvendorimghdr->vendor_bootconfig_size,
					pagesz);
				MsgP("ramdisk table offset 0x%x, totalsize 0x%lx\n",
				nflashloadlen + lflashreadoff, totalsize);
			}
			//Check if encrypted image
			ret = _aml_get_secure_boot_kernel_size(pbuffpreload, &securekernelimgsz);
			if (ret) {
				errorP("Fail in _aml_get_secure_boot_kernel_size, ret = %d\n", ret);
				ret = __LINE__;
				goto exit;
			}
			if (securekernelimgsz) {
				totalsize = securekernelimgsz;
				MsgP("securekernelimgsz=0x%lx\n", totalsize);
			}
			if (totalsize > vendorboot_part_sz) {
				errorP("totalsize 0x%lx > vendorboot_part_sz 0x%llx\n",
						totalsize, vendorboot_part_sz);
				ret = __LINE__;
				goto exit;
			}
#ifdef CONFIG_AVB2
			if (!(upgrade_step_s && !(strcmp(upgrade_step_s, "3")))) {
				original_size = totalsize;
				totalsize = get_size_avb_footer(partname);
				if (!totalsize || totalsize < original_size) {
					totalsize = original_size;
					vendor_boot_preload = false;
					wrnP("part: %s footer not at correct location\n",
					     partname);
				}
			}
#endif
			free(pbuffpreload);
			pbuffpreload = 0;
			pbuffpreload = malloc(totalsize);
			if (!pbuffpreload) {
				errorP("Fail alloc vendor image buffer\n");
				return __LINE__;
			}
			memset(pbuffpreload, 0, totalsize);
			if (upgrade_step_s && (strcmp(upgrade_step_s, "3") == 0) &&
					(strcmp(partname, "vendor_boot") == 0)) {
				loff_t len_read;

				MsgP("recovery mode, read vendor_boot from cache\n");
				ret = fs_set_blk_dev("mmc", "1:2", FS_TYPE_EXT);
				if (ret) {
					errorP("Fail to set blk dev cache\n");
					cache_flag = false;
				} else {
					fs_read("/recovery/vendor_boot.img",
						(unsigned long)pbuffpreload,
						0, totalsize,
						&len_read);
					if (totalsize != len_read) {
						errorP("Fail to read from cache\n");
						cache_flag = false;
					} else {
						cache_flag = true;
					}
				}
			}
			if (!cache_flag) {
				MsgP("read from part: %s\n", partname);
				ret = store_logic_read(partname, 0, totalsize, pbuffpreload);
				if (ret) {
					errorP("Fail to read 0x%xB from part[%s]\n",
					(unsigned int)totalsize, partname);
					ret = __LINE__;
					goto exit;
				}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
				part_dec(partname, (u8 *)pbuffpreload, totalsize,
						(u8 *)pbuffpreload, totalsize, 0);
#endif
			}
#ifdef CONFIG_AVB2
			if (!ret && !(upgrade_step_s && !(strcmp(upgrade_step_s, "3"))) &&
			    vendor_boot_preload)
				set_avb_parts(partname, (void *)pbuffpreload, totalsize);
			if (avb_verify_single(partname)) {
				ret = __LINE__;
				goto exit;
			}
#endif
			flush_cache((unsigned long)pbuffpreload, totalsize);
#ifndef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK
			if (IS_FEAT_BOOT_VERIFY()) {
#ifndef CONFIG_IMAGE_CHECK
				ret = aml_sec_boot_check(AML_D_P_IMG_DECRYPT,
						(unsigned long)pbuffpreload,
						GXB_IMG_SIZE, GXB_IMG_DEC_DTB);
#else
				ret = secure_image_check((uint8_t *)(unsigned long)
						pbuffpreload, GXB_IMG_SIZE, GXB_IMG_DEC_DTB);
				/*pbuffpreload += android_image_check_offset();*/
				memmove(pbuffpreload, pbuffpreload + android_image_check_offset(),
						totalsize - android_image_check_offset());
#endif
				if (ret) {
					errorP("\n[vendor_boot]aml log : Sig Check is %d\n", ret);
					ret = __LINE__;
					goto exit;
				}
				MsgP("vendor_boot decrypt at 0x%p\n", pbuffpreload);
			}
#endif /* CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK */
			p_vender_boot_img = (p_vendor_boot_img_t)pbuffpreload;
		} else {
			errorP("verndor boot magic error, %s, %d\n", __func__, __LINE__);
			ret = __LINE__;
			goto exit;
		}
	} else {
		pagesz = hdr_addr->page_size;
		lflashreadoff += pagesz;
		lflashreadoff += ALIGN(hdr_addr->kernel_size, pagesz);
		lflashreadoff += ALIGN(hdr_addr->ramdisk_size, pagesz);
		nflashloadlen  = ALIGN(hdr_addr->second_size, pagesz);
		debugP("lflashreadoff=0x%x, nflashloadlen=0x%x\n", lflashreadoff, nflashloadlen);
		debugP("page sz %u\n", hdr_addr->page_size);
	}

	if (pagesz > PAGE_SIZE) {
		errorP("Wrong pagesz:%d\n", pagesz);
		ret = __LINE__;
		goto exit;
	}
	if (!nflashloadlen || nflashloadlen > MAX_DTB_SIZE) {
		errorP("Wrong nflashloadlen:%d\n", nflashloadlen);
		ret = __LINE__;
		goto exit;
	}
	if (lflashreadoff > (MAX_RAMDISK_SIZE + MAX_KERNEL_SIZE)) {
		errorP("Wrong lflashreadoff:%x\n", lflashreadoff);
		ret = __LINE__;
		goto exit;
	}

	unsigned char *secondaddr = (unsigned char *)pbuffpreload + lflashreadoff;

	if (!dtb_in_vendor_boot) {
		secondaddr = (unsigned char *)loadaddr + lflashreadoff;

		loff_t wroff = lflashreadoff;
		size_t wrsz  = nflashloadlen;
		unsigned char *wraddr = secondaddr;

		memcpy((void *)loadaddr, (void *)pbuffpreload, preloadsz);
		free(pbuffpreload);
#if !defined(CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK) && defined(CONFIG_IMAGE_CHECK)
		ret = _aml_get_secure_boot_kernel_size(loadaddr, &securekernelimgsz);
		if (ret) {
			errorP("Fail in _aml_get_secure_boot_kernel_size, rc=%d\n", ret);
			return __LINE__;
		}

		if (securekernelimgsz) {
			debugP("secure kernel sz 0x%x\n", securekernelimgsz);
			wrsz  = securekernelimgsz - preloadsz;
			wroff = lflashreadinitoff + preloadsz;
			wraddr = (unsigned char *)loadaddr + preloadsz;
		}
#endif//#if !defined(CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK) && defined(CONFIG_IMAGE_CHECK)
		if ((BOOT_NAND_MTD == store_get_type() || BOOT_SNAND == store_get_type())) {
			unsigned notAlignSz = wroff & 0xfff;

			wroff -= notAlignSz;
			wrsz  += notAlignSz;
			secondaddr += notAlignSz;
			MsgP("not align dtb off 0x%llx\n", wroff);
		}

		ret = store_logic_read(partname, wroff, wrsz, wraddr);
		if (ret) {
			errorP("Fail to read 0x%xB from part[%s] at offset 0x%x\n",
					(unsigned int)wrsz, partname, (unsigned int)wroff);
			return __LINE__;
		}
#ifdef CONFIG_AVB2
			set_avb_parts(partname, (void *)wraddr, wrsz);
			if (avb_verify_single(partname))
				return __LINE__;
#endif

#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		part_dec(partname, (u8 *)wraddr, wrsz, (u8 *)wraddr, wrsz, wroff);
#endif
#ifndef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK
		if (IS_FEAT_BOOT_VERIFY()) {
#ifndef CONFIG_IMAGE_CHECK
			//because secure boot will use DMA which need disable MMU temp
			//here must update the cache, otherwise nand will fail (eMMC is OK)
			flush_cache((unsigned long)secondaddr, (unsigned long)nflashloadlen);

			ret = aml_sec_boot_check(AML_D_P_IMG_DECRYPT, (unsigned long)loadaddr,
				GXB_IMG_SIZE, GXB_IMG_DEC_DTB);
#else
			//because secure boot will use DMA which need disable MMU temp
			//here must update the cache, otherwise nand will fail (eMMC is OK)
			flush_cache((unsigned long)loadaddr, (unsigned long)securekernelimgsz);

			ret = secure_image_check((uint8_t *)(unsigned long)loadaddr,
				GXB_IMG_SIZE, GXB_IMG_DEC_DTB);
			secondaddr += android_image_check_offset();
#endif
			if (ret) {
				errorP("\n[dtb]aml log : Sig Check is %d\n", ret);
				return __LINE__;
			}
			MsgP("decrypted dtb sz 0x%x\n", nflashloadlen);
		}
#endif
	}
	char *dtdestaddr = (char *)loadaddr;//simple_strtoull(getenv("dtb_mem_addr"), NULL, 0);

	memmove(dtdestaddr, secondaddr, nflashloadlen);

exit:

	if (ret)
		free(pbuffpreload);

	return ret;

}

/*uint32_t store_rsv_size(const char *name);*/
static int do_image_read_dtb_from_rsv(unsigned char* loadaddr)
{
	const int dtbmaxsz = store_rsv_size("dtb");

	if (dtbmaxsz < 0x400) {
		errorP("dtbmaxsz(0x%x) invalid\n", dtbmaxsz);
		return -__LINE__;
	}
	int ret = store_rsv_read("dtb", dtbmaxsz, loadaddr);

	if (ret) {
		errorP("Fail read dtb from rsv with sz 0x%x\n", dtbmaxsz);
		return -__LINE__;
	}
#ifndef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK
	if (IS_FEAT_BOOT_VERIFY()) {
		flush_cache((unsigned long)loadaddr, dtbmaxsz);
#ifndef CONFIG_IMAGE_CHECK
		ret = aml_sec_boot_check(AML_D_P_IMG_DECRYPT, (long)loadaddr, dtbmaxsz, 0);
#else
		ret = secure_image_check((uint8_t *)(unsigned long)loadaddr, dtbmaxsz, 0);
		memmove(loadaddr, (void *)(loadaddr + sizeof(struct aml_boot_header_t)), dtbmaxsz);
#endif
		if (ret) {
			MsgP("decrypt dtb: Sig Check %d\n", ret);
			return -__LINE__;
		}
	}
#endif
	return 0;
}

//imgread dtb boot ${dtb_mem_addr}
//imgread dtb rsv ${dtb_mem_addr}
static int do_image_read_dtb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int iRet = 0;
	const char partName[64] = {0};
	unsigned char *loadaddr = 0;
	u64 lflashReadOff = 0;

	strlcpy((char *)partName, argv[1], 64);

#ifdef CONFIG_DIAG_MODE
	char diag_state_buf[200] = {0x0};
	u64 rc_diag = -1;

	rc_diag = store_part_size("diag_boot");
	if ((key_unify_read("diag_state", diag_state_buf,
			sizeof(diag_state_buf)) == 0) &&
			(strcmp(diag_state_buf, "Test") == 0) &&
			(rc_diag != -1)) {
		printf("%s: %d: read from diag_boot\n", __func__, __LINE__);
		strcpy((char *)partName, "diag_boot");
	}
#endif

	if (argc > 2)
		loadaddr = (unsigned char *)simple_strtoul(argv[2], NULL, 16);
	else
		loadaddr = (unsigned char *)simple_strtoul(env_get("loadaddr"), NULL, 16);

	if (argc > 3)
		lflashReadOff = simple_strtoull(argv[3], NULL, 0);

	const int fromRsv = !strcmp("_aml_dtb", argv[1]);

	if (fromRsv)
		iRet = do_image_read_dtb_from_rsv(loadaddr);
	else
		iRet = do_image_read_dtb_from_knl(partName, loadaddr, lflashReadOff);

	if (iRet) {
		errorP("Fail read dtb from %s, ret %d\n", partName, iRet);
		return CMD_RET_FAILURE;
	}

	unsigned long fdtAddr = (unsigned long)loadaddr;
#ifdef CONFIG_MULTI_DTB
	extern unsigned long get_multi_dt_entry(unsigned long fdt_addr);
	fdtAddr = get_multi_dt_entry((unsigned long)fdtAddr);
	if (!fdtAddr) {
		errorP("Fail in get_multi_dt_entry\n");
		return __LINE__;
	}
#endif// #ifdef CONFIG_MULTI_DTB
	iRet = fdt_check_header((char *)fdtAddr);
	if (iRet) {
		errorP("Fail in fdt check header\n");
		return CMD_RET_FAILURE;
	}
	const unsigned int fdtsz = fdt_totalsize((char *)fdtAddr);

	if (fdtsz >= SZ_1M) {
		errorP("bad fdt size:%d\n", fdtsz);
		return CMD_RET_FAILURE;
	}

	memmove(loadaddr, (char *)fdtAddr, fdtsz);

	return iRet;
}

static int do_image_read_part(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *name = NULL;
	uint64_t addr = 0;
	uint64_t offset = 0;
	uint64_t sz = 0;
	int rc = 0;

	if (argc < 4 || argc > 5) {
		return CMD_RET_USAGE;
	}
	name = argv[1];
	addr = simple_strtoull(argv[2], NULL, 16);
	offset = simple_strtoull(argv[3], NULL, 0);

	if (argc == 5)
		sz = simple_strtoull(argv[4], NULL, 0);
	else
		sz = store_logic_cap(name);

	printf("read %s with %llu bytes at offset %llu to addr %#llx\n",
			name, sz, offset, addr);

	rc = store_logic_read(name, offset, sz, (void*)addr);
	if (rc) {
		printf("Failed to read %s with %llu bytes at offset %llu\n",
				name, sz, offset);
		goto out;
	}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
	part_dec(name, (u8*)addr, sz, (u8*)addr, sz, offset);
#endif
out:
	return rc;
}

static bool is_valid_partition(const char *part_name)
{
	if (!part_name)
		return false;

	size_t part_len = strlen(part_name);

	for (int i = 0; i < ARRAY_SIZE(white_list_boot_part); i++) {
		const char *valid_part = white_list_boot_part[i];
		size_t list_part_len = strlen(valid_part);
		size_t cmp_len = part_len < list_part_len ? part_len : list_part_len;

		if ((strncmp(part_name, valid_part, cmp_len) == 0) &&
			part_len == list_part_len)
			return true;
	}

	return false;
}

static int do_image_read_kernel(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int kernel_size;
	unsigned int ramdisk_size;
	boot_img_hdr_t *hdr_addr = NULL;
	int genFmt = 0;
	u32 actualbootimgsz = 0;
	u32 dtbsz = 0;
	const char *const partname = argv[1];
	unsigned char *loadaddr = 0;
	int rc = 0;
	u64 flashreadoff = 0;
	u32 securekernelimgsz = 0;
	char *upgrade_step_s = NULL;
	bool cache_flag = false;
	ulong kernelEndAddr = 0;
	ulong kernelLoadAddr = 0;
	ulong dtbLoadAddr = 0;
	char strAddr[128] = {0};
#ifdef CONFIG_AVB2
	u64 original_size = 0;
#endif
#ifdef CONFIG_DIAG_MODE
	char diag_state_buf[200] = {0x0};
	u64 rc_diag = -1;
	int diag_flag = 0;
#endif

	if (argc > 2) {
		loadaddr = (unsigned char *)simple_strtoul(argv[2], NULL, 16);
		env_set("loadaddr", (const char *)argv[2]);
	} else {
		loadaddr = (unsigned char *)simple_strtoul(env_get("loadaddr"), NULL, 16);
	}

	hdr_addr = (boot_img_hdr_t *)loadaddr;

	if (argc > 3)
		flashreadoff = simple_strtoull(argv[3], NULL, 0);

	if (!is_valid_partition(partname)) {
		errorP("is not valid partition [%s]\n", partname);
		return __LINE__;
	}

#ifdef CONFIG_DIAG_MODE
	rc_diag = store_part_size("diag_vendor_boot");

	if ((key_unify_read("diag_state", diag_state_buf,
		sizeof(diag_state_buf)) == 0) &&
		(strcmp(diag_state_buf, "Test") == 0) &&
		(rc_diag != -1) &&
		!IS_FEAT_BOOT_VERIFY()) {
		printf("diag mode\n");
		strcpy((char *)partname, "diag_boot");
		diag_flag = 1;
	}
#endif

	upgrade_step_s = env_get("upgrade_step");
	if (upgrade_step_s && (strcmp(upgrade_step_s, "3") == 0) &&
		(strcmp(partname, "recovery") == 0)) {
		loff_t len_read;

		MsgP("read recovery.img from cache\n");
		rc = fs_set_blk_dev("mmc", "1:2", FS_TYPE_EXT);
		if (rc) {
			errorP("Fail to set blk dev cache\n");
			cache_flag = false;
		} else {
			fs_read("/recovery/recovery.img", (unsigned long)loadaddr,
					flashreadoff, IMG_PRELOAD_SZ, &len_read);
			if (IMG_PRELOAD_SZ != len_read) {
				errorP("Fail to read recovery.img from cache\n");
				cache_flag = false;
			} else {
				cache_flag = true;
			}
		}
	}

	if (!cache_flag) {
		MsgP("read from part: %s\n", partname);
		rc = store_logic_read(partname, flashreadoff, IMG_PRELOAD_SZ, loadaddr);
		if (rc) {
			errorP("Fail to read 0x%xB from part[%s] at offset 0\n",
				IMG_PRELOAD_SZ, partname);
			return __LINE__;
		}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		part_dec(partname, (u8*)loadaddr, IMG_PRELOAD_SZ, (u8*)loadaddr, IMG_PRELOAD_SZ, flashreadoff);
#endif
	}
	flashreadoff += IMG_PRELOAD_SZ;

	if (!is_android_v3_header_image((void *)hdr_addr)) {
		genFmt = genimg_get_format(hdr_addr);
#if defined(CONFIG_ZIRCON_BOOT_IMAGE)
	if (IMAGE_FORMAT_ANDROID != genFmt && IMAGE_FORMAT_ZIRCON != genFmt) {
		errorP("Fmt 0x%x unsupported!, supported genFmt 0x%x or 0x%x\n",
			genFmt,
			IMAGE_FORMAT_ANDROID, IMAGE_FORMAT_ZIRCON);
#else
		if (genFmt != IMAGE_FORMAT_ANDROID) {
			errorP("Fmt unsupported!genFmt 0x%x != 0x%x\n", genFmt,
				IMAGE_FORMAT_ANDROID);
#endif
			return __LINE__;
		}

		//Check if encrypted image
		rc = _aml_get_secure_boot_kernel_size(loadaddr, &securekernelimgsz);
		if (rc) {
			errorP("Fail in _aml_get_secure_boot_kernel_size, rc=%d\n", rc);
			return __LINE__;
		}
		if (securekernelimgsz) {
			actualbootimgsz = securekernelimgsz;
			MsgP("securekernelimgsz=0x%x\n", actualbootimgsz);
		} else {
			kernel_size = (hdr_addr->kernel_size + (hdr_addr->page_size - 1) +
				hdr_addr->page_size) & (~(hdr_addr->page_size - 1));
			ramdisk_size = (hdr_addr->ramdisk_size +
				(hdr_addr->page_size - 1)) & (~(hdr_addr->page_size - 1));
			dtbsz = hdr_addr->second_size;
			actualbootimgsz = kernel_size + ramdisk_size + dtbsz;
			debugP("kernel_size 0x%x, page_size 0x%x, totalSz 0x%x\n",
				hdr_addr->kernel_size, hdr_addr->page_size, kernel_size);
			debugP("ramdisk_size 0x%x, totalSz 0x%x\n",
				hdr_addr->ramdisk_size, ramdisk_size);
			debugP("dtbSz 0x%x, Total actualbootimgsz 0x%x\n",
				dtbsz, actualbootimgsz);
			if (kernel_size > MAX_KERNEL_SIZE) {
				errorP("kernel size limit 0x%x,0x%x\n", kernel_size,
				       MAX_KERNEL_SIZE);
				return __LINE__;
			}
			if (ramdisk_size > MAX_RAMDISK_SIZE) {
				errorP("ramdisk size limit 0x%x,0x%x\n", ramdisk_size,
				       MAX_RAMDISK_SIZE);
				return __LINE__;
			}
			if (dtbsz > MAX_DTB_SIZE) {
				errorP("dtb size limit 0x%x,0x%x\n", dtbsz, MAX_DTB_SIZE);
				return __LINE__;
			}
		}

#if defined(CONFIG_ZIRCON_BOOT_IMAGE)
	if (genFmt == IMAGE_FORMAT_ZIRCON) {
		const zbi_header_t *zbi = (zbi_header_t *)hdr_addr;

		actualbootimgsz = zbi->length + sizeof(*zbi);
	}
#endif//#if defined(CONFIG_ZIRCON_BOOT_IMAGE)

		if (actualbootimgsz > IMG_PRELOAD_SZ) {
			const u32 leftsz = actualbootimgsz - IMG_PRELOAD_SZ;

			/* auto adjust kernel image load address avoid
			 * touch iotrace data and secureOS memory space
			 */
			kernelLoadAddr =
				env_get_ulong("loadaddr", 16, KERNEL_DEFAULT_LOAD_ADDR);
			kernelEndAddr = kernelLoadAddr + actualbootimgsz;
			dtbLoadAddr = env_get_ulong("dtb_mem_addr", 16, DTB_LOAD_ADDR);

			if (kernelEndAddr > IOTRACE_LOAD_ADDR) {
				if (ALIGN(actualbootimgsz, LOAD_ADDR_ALIGN_LENGTH) >=
					(IOTRACE_LOAD_ADDR - dtbLoadAddr - ALIGN(dtbsz, LOAD_ADDR_ALIGN_LENGTH)))
					kernelLoadAddr = KERNEL_LOAD_HIGH_ADDR;
				else
					kernelLoadAddr = dtbLoadAddr + ALIGN(dtbsz, LOAD_ADDR_ALIGN_LENGTH);

				sprintf(strAddr, "%lx", kernelLoadAddr);
				memmove((void *)kernelLoadAddr, (void *)loadaddr, IMG_PRELOAD_SZ);
				env_set("loadaddr", strAddr);
				env_set("loadaddr_kernel", strAddr);
				loadaddr = (unsigned char *)
					env_get_ulong("loadaddr", 16, kernelLoadAddr);
				printf("kernel overlap iotrace, reset kernelLoadAddr = 0x%lx\n",
						kernelLoadAddr);
			}

			debugP("Left sz 0x%x\n", leftsz);
			rc = store_logic_read(partname, flashreadoff,
				leftsz, loadaddr + IMG_PRELOAD_SZ);
			if (rc) {
				errorP("Fail to read 0x%xB from part[%s] at offset 0x%x\n",
					leftsz, partname, IMG_PRELOAD_SZ);
				return __LINE__;
			}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
			part_dec(partname, (u8*)(loadaddr + IMG_PRELOAD_SZ), leftsz,
				(u8*)(loadaddr + IMG_PRELOAD_SZ), leftsz, flashreadoff);
#endif
		}
		debugP("totalSz=0x%x\n", actualbootimgsz);

		//because secure boot will use DMA which need disable MMU temp
		//here must update the cache, otherwise nand will fail (eMMC is OK)
		flush_cache((unsigned long)loadaddr, (unsigned long)actualbootimgsz);
	} else {
		char partname_init[32] = {0};
		u64 rc_init;
		char *slot_name;
		p_boot_img_hdr_v3_t hdr_addr_v3 = NULL;
#ifdef CONFIG_AVB2
		bool kernel_preload = true;
		bool init_boot_preload = true;
#endif

		init_boot_ramdisk_size = 0;
		slot_name = env_get("slot-suffixes");
		if (slot_name && (strcmp(slot_name, "0") == 0))
			strcpy((char *)partname_init, "init_boot_a");
		else if (slot_name && (strcmp(slot_name, "1") == 0))
			strcpy((char *)partname_init, "init_boot_b");
		else
			strcpy((char *)partname_init, "init_boot");

		rc_init = store_part_size(partname_init);

		genFmt = genimg_get_format(hdr_addr);
		if (genFmt != IMAGE_FORMAT_ANDROID) {
			errorP("Fmt unsupported!genFmt 0x%x != 0x%x\n",
				genFmt, IMAGE_FORMAT_ANDROID);
			return __LINE__;
		}

		//Check if encrypted image
		rc = _aml_get_secure_boot_kernel_size(loadaddr, &securekernelimgsz);
		if (rc) {
			errorP("Fail in _aml_get_secure_boot_kernel_size, rc=%d\n", rc);
			return __LINE__;
		}

		hdr_addr_v3 = (p_boot_img_hdr_v3_t)hdr_addr;
		kernel_size = ALIGN(hdr_addr_v3->kernel_size, 0x1000);

		ramdisk_size = ALIGN(hdr_addr_v3->ramdisk_size, 0x1000);

		MsgP("kernel_size 0x%x, totalSz 0x%x\n",
			hdr_addr_v3->kernel_size, kernel_size);
		MsgP("ramdisk_size 0x%x, totalSz 0x%x\n",
			hdr_addr_v3->ramdisk_size, ramdisk_size);
		MsgP("boot header_version = %d\n", hdr_addr_v3->header_version);
		if (kernel_size > MAX_KERNEL_SIZE) {
			errorP("kernel size limit 0x%x,0x%x\n", kernel_size, MAX_KERNEL_SIZE);
			return __LINE__;
		}
		if (ramdisk_size > MAX_RAMDISK_SIZE) {
			errorP("ramdisk size limit 0x%x,0x%x\n", ramdisk_size, MAX_RAMDISK_SIZE);
			return __LINE__;
		}
		if (securekernelimgsz) {
			actualbootimgsz = securekernelimgsz;
			MsgP("securekernelimgsz=0x%x\n", actualbootimgsz);
		} else {
			actualbootimgsz = kernel_size + ramdisk_size + 0x1000;
#ifdef CONFIG_AVB2
			original_size = actualbootimgsz;
			actualbootimgsz = get_size_avb_footer(partname);
			if (!actualbootimgsz || actualbootimgsz < original_size) {
				actualbootimgsz = original_size;
				kernel_preload = false;
				wrnP("part: %s footer not at correct location\n", partname);
			}
#endif
		}

		if (actualbootimgsz > IMG_PRELOAD_SZ) {
			const u32 leftsz = actualbootimgsz - IMG_PRELOAD_SZ;

			/* auto adjust kernel image load address avoid
			 * touch iotrace data and secureOS memory space
			 */
			kernelLoadAddr =
				env_get_ulong("loadaddr", 16, KERNEL_DEFAULT_LOAD_ADDR);
			kernelEndAddr = kernelLoadAddr + actualbootimgsz;
			dtbLoadAddr = env_get_ulong("dtb_mem_addr", 16, DTB_LOAD_ADDR);

			if (kernelEndAddr > IOTRACE_LOAD_ADDR) {
				if (ALIGN(actualbootimgsz, LOAD_ADDR_ALIGN_LENGTH) >=
					(IOTRACE_LOAD_ADDR - dtbLoadAddr - LOAD_ADDR_ALIGN_LENGTH))
					kernelLoadAddr = KERNEL_LOAD_HIGH_ADDR;
				else
					kernelLoadAddr = dtbLoadAddr + LOAD_ADDR_ALIGN_LENGTH;

				sprintf(strAddr, "%lx", kernelLoadAddr);
				memmove((void *)kernelLoadAddr, (void *)loadaddr, IMG_PRELOAD_SZ);
				env_set("loadaddr", strAddr);
				env_set("loadaddr_kernel", strAddr);
				loadaddr = (unsigned char *)
					env_get_ulong("loadaddr", 16, kernelLoadAddr);
				printf("kernel overlap iotrace, reset kernelLoadAddr = 0x%lx\n",
						kernelLoadAddr);
			}

			debugP("Left sz 0x%x\n", leftsz);

			if (upgrade_step_s && (strcmp(upgrade_step_s, "3") == 0) &&
				(strcmp(partname, "recovery") == 0)) {
				loff_t len_read;

				MsgP("read recovery.img from cache\n");
				rc = fs_set_blk_dev("mmc", "1:2", FS_TYPE_EXT);
				if (rc) {
					errorP("Fail to set blk dev cache\n");
					cache_flag = false;
				} else {
					fs_read("/recovery/recovery.img",
							(unsigned long)loadaddr,
							0, actualbootimgsz, &len_read);
					if (actualbootimgsz != len_read) {
						errorP("Fail to read recovery.img from cache\n");
						cache_flag = false;
					} else {
						cache_flag = true;
					}
				}
			}
			if (!cache_flag) {
				MsgP("read from part: %s\n", partname);
				rc = store_logic_read(partname, flashreadoff,
					leftsz, loadaddr + IMG_PRELOAD_SZ);
				if (rc) {
					errorP("Fail to read 0x%xB from part[%s] at offset 0x%x\n",
						leftsz, partname, IMG_PRELOAD_SZ);
					return __LINE__;
				}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
				part_dec(partname, (u8*)(loadaddr + IMG_PRELOAD_SZ), leftsz,
					(u8*)(loadaddr + IMG_PRELOAD_SZ), leftsz, flashreadoff);
#endif
#ifdef CONFIG_AVB2
				if (kernel_preload)
					set_avb_parts(partname, loadaddr, leftsz + IMG_PRELOAD_SZ);
#endif
				if (rc_init != -1) {
					MsgP("read header from part: %s\n", partname_init);
					unsigned int nflashloadlen_init = 0;
					const int preloadsz_init = 0x1000 * 2;
					unsigned char *pbuffpreload_init = 0;
#ifdef CONFIG_AVB2
					u8 *init_boot_buf = 0;
#endif

					nflashloadlen_init = preloadsz_init;
					debugP("sizeof preloadSz=%u\n", nflashloadlen_init);

					pbuffpreload_init = malloc(preloadsz_init);

					if (!pbuffpreload_init) {
						printf("Fail to allocate memory for %s!\n",
							partname_init);
						return __LINE__;
					}

					rc = store_logic_read(partname_init, 0,
						nflashloadlen_init, pbuffpreload_init);
					if (rc) {
						errorP("Fail to read 0x%xB from part[%s]\n",
							nflashloadlen_init, partname_init);
						free(pbuffpreload_init);
						pbuffpreload_init = 0;
						return __LINE__;
					}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
					part_dec(partname_init, (u8*)pbuffpreload_init, nflashloadlen_init,
						(u8*)pbuffpreload_init, nflashloadlen_init, 0);
#endif
					p_boot_img_hdr_v3_t pinitbootimghdr;

					pinitbootimghdr = (p_boot_img_hdr_v3_t)pbuffpreload_init;

					ramdisk_size = ALIGN(pinitbootimghdr->ramdisk_size,
							0x1000);

					// Security fix: validate ramdisk_size from untrusted input
					#define RAMDISK_MIN_SIZE 0x1000  // Minimum 4KB

					if (ramdisk_size < RAMDISK_MIN_SIZE ||
						ramdisk_size > MAX_RAMDISK_SIZE ||
						ramdisk_size > rc_init) {
						errorP("Invalid ramdisk_size: 0x%x", ramdisk_size);
						errorP(" (min: 0x%x,", RAMDISK_MIN_SIZE);
						errorP(" max: 0x%x,", MAX_RAMDISK_SIZE);
						errorP(" partition_size: 0x%llx)\n", rc_init);
						free(pbuffpreload_init);
						pbuffpreload_init = 0;
						return __LINE__;
					}

					MsgP("ramdisk_size 0x%x, totalSz 0x%x\n",
						pinitbootimghdr->ramdisk_size, ramdisk_size);
					MsgP("init_boot header_version = %d\n",
						pinitbootimghdr->header_version);
					init_boot_ramdisk_size = pinitbootimghdr->ramdisk_size;

#ifdef CONFIG_AVB2
					original_size = ramdisk_size;
					ramdisk_size = get_size_avb_footer(partname_init);
					if (!ramdisk_size || ramdisk_size < original_size) {
						ramdisk_size = original_size;
						init_boot_preload = false;
						wrnP("part: %s footer not at correct location\n",
						     partname_init);
					}
					if (init_boot_preload) {
						// Security fix: ensure buffer size sufficient
						if (nflashloadlen_init > ramdisk_size) {
							errorP("Buffer overflow risk\n");
							free(pbuffpreload_init);
							pbuffpreload_init = 0;
							return __LINE__;
						}

						init_boot_buf = malloc(ramdisk_size);
					if (!init_boot_buf) {
						printf("Fail to allocate memory for %s!\n",
						       partname_init);
						free(pbuffpreload_init);
						pbuffpreload_init = 0;
						return __LINE__;
					}
						memcpy(init_boot_buf, pbuffpreload_init,
						       nflashloadlen_init);

						// Security fix: prevent integer underflow
						if (ramdisk_size < BOOT_IMG_V3_HDR_SIZE) {
							errorP("Integer underflow risk\n");
							free(init_boot_buf);
							init_boot_buf = 0;
							free(pbuffpreload_init);
							pbuffpreload_init = 0;
							return __LINE__;
						}
						ramdisk_size -= BOOT_IMG_V3_HDR_SIZE;
					}
#endif
					if (init_boot_ramdisk_size != 0) {
						MsgP("read ramdisk from part: %s\n", partname_init);

						// Security fix: additional validation
						if (ramdisk_size > MAX_RAMDISK_SIZE ||
							ramdisk_size > (rc_init - BOOT_IMG_V3_HDR_SIZE)) {
							errorP("Invalid ramdisk_size\n");
							free(pbuffpreload_init);
							pbuffpreload_init = 0;
#ifdef CONFIG_AVB2
							if (init_boot_buf) {
								free(init_boot_buf);
								init_boot_buf = 0;
							}
#endif
							return __LINE__;
						}

						if (securekernelimgsz && init_boot_ramdisk_size)
							ramdisk_off = securekernelimgsz;
						else
							ramdisk_off = kernel_size + BOOT_IMG_V3_HDR_SIZE;

						rc = store_logic_read(partname_init,
							BOOT_IMG_V3_HDR_SIZE,
							ramdisk_size, loadaddr + ramdisk_off);

						if (rc) {
							errorP("Fail to read 0x%xB from part[%s]\n",
								ramdisk_size, partname_init);
							free(pbuffpreload_init);
							pbuffpreload_init = 0;
#ifdef CONFIG_AVB2
							free(init_boot_buf);
							init_boot_buf = 0;
#endif
							return __LINE__;
						}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
						part_dec(partname_init, (u8*)(loadaddr + ramdisk_off),
							ramdisk_size,
							(u8*)(loadaddr + ramdisk_off),
							ramdisk_size,
							BOOT_IMG_V3_HDR_SIZE);
#endif

#ifdef CONFIG_AVB2
					if (init_boot_preload)
						memcpy(init_boot_buf + BOOT_IMG_V3_HDR_SIZE,
						       loadaddr + ramdisk_off,
						       ramdisk_size);
#endif
					}
#ifdef CONFIG_AVB2
					if (init_boot_ramdisk_size != 0 && init_boot_preload)
						set_avb_parts(partname_init, init_boot_buf,
							      ramdisk_size + BOOT_IMG_V3_HDR_SIZE);
					if (init_boot_preload)
						free(init_boot_buf);
#endif
					free(pbuffpreload_init);
					pbuffpreload_init = 0;
				}
			}
		}
		debugP("totalSz=0x%x\n", actualbootimgsz);
		/*
		 *because secure boot will use DMA which need disable MMU temp
		 *here must update the cache, otherwise nand will fail (eMMC is OK)
		 */
		flush_cache((unsigned long)loadaddr, (unsigned long)actualbootimgsz);
	} /*ANDROID R and above version */

	/* image size exceed 24M meanwhile need to decompress would overlap secure zone */
	if ((android_image_get_comp((void *)loadaddr) != IH_COMP_NONE) &&
			(kernel_size > KERNEL_DECOMPRESS_MAX_SIZE)) {
		memset(strAddr, 0, sizeof(strAddr));
		sprintf(strAddr, "%x", KERNEL_HIGH_DEC_ADDR);
		env_set("decaddr_kernel", strAddr);
	}

	return 0;
}

#define AML_RES_IMG_VERSION_V1      (0x01)
#define AML_RES_IMG_VERSION_V2      (0x02)
#define AML_RES_IMG_V1_MAGIC_LEN    8
#define AML_RES_IMG_V1_MAGIC        "AML_RES!"//8 chars
#define AML_RES_IMG_ITEM_ALIGN_SZ   16
#define AML_RES_IMG_HEAD_SZ         (AML_RES_IMG_ITEM_ALIGN_SZ * 4)//64
#define AML_RES_ITEM_HEAD_SZ        (AML_RES_IMG_ITEM_ALIGN_SZ * 4)//64

//typedef for amlogic resource image
#pragma pack(push, 4)
typedef struct {
	__u32   crc;    //crc32 value for the resources image
	__s32   version;//current version is 0x01

	__u8    magic[AML_RES_IMG_V1_MAGIC_LEN];  //resources images magic

	__u32   imgSz;  //total image size in byte
	__u32   imgItemNum;//total item packed in the image

	__u32   alignSz;//AML_RES_IMG_ITEM_ALIGN_SZ
	__u8    reserv[AML_RES_IMG_HEAD_SZ - 8 * 3 - 4];

}AmlResImgHead_t;
#pragma pack(pop)

#define LOGO_OLD_FMT_READ_SZ (8U<<20)//if logo format old, read 8M
#define LOGO_TOTAL_ITEM		(32)

static int img_res_check_log_header(const AmlResImgHead_t* pResImgHead)
{
	int rc = 0;

	rc = memcmp(pResImgHead->magic, AML_RES_IMG_V1_MAGIC, AML_RES_IMG_V1_MAGIC_LEN);
	if (rc) {
		debugP("Magic error for res\n");
		return 1;
	}
	if (pResImgHead->version != AML_RES_IMG_VERSION_V2) {
		errorP("res version 0x%x != 0x%x\n", pResImgHead->version, AML_RES_IMG_VERSION_V2);
		return 2;
	}
	if (pResImgHead->imgItemNum > LOGO_TOTAL_ITEM) {
		errorP("logo size err 0x%x != 0x%x\n", pResImgHead->imgItemNum, LOGO_TOTAL_ITEM);
		return 3;
	}

	return 0;
}

static int do_image_read_res(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char * const partName = argv[1];
	unsigned char *loadaddr = 0;
	int rc = 0;
	AmlResImgHead_t *pResImgHead = NULL;
	unsigned int totalSz    = 0;
	u64 flashReadOff = 0;

	if (argc > 2)
		loadaddr = (unsigned char *)simple_strtoul(argv[2], NULL, 16);
	else
		loadaddr = (unsigned char *)simple_strtoul(env_get("loadaddr"), NULL, 16);

	pResImgHead = (AmlResImgHead_t *)loadaddr;

	rc = store_logic_read(partName, flashReadOff, IMG_PRELOAD_SZ, loadaddr);
	if (rc) {
		errorP("Fail to read 0x%xB from part[%s] at offset 0\n", IMG_PRELOAD_SZ, partName);
		return __LINE__;
	}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
	part_dec(partName, (u8 *)loadaddr, IMG_PRELOAD_SZ,
		(u8 *)loadaddr, IMG_PRELOAD_SZ, flashReadOff);
#endif
	flashReadOff = IMG_PRELOAD_SZ;

	if (img_res_check_log_header(pResImgHead)) {
		errorP("Logo header err.\n");
		return __LINE__;
	}

	//Read the actual size of the new version res image
	totalSz = pResImgHead->imgSz;
	if (totalSz > IMG_PRELOAD_SZ) {
		const unsigned int leftSz = totalSz - flashReadOff;

		rc = store_logic_read(partName, flashReadOff, leftSz,
			loadaddr + (unsigned int)flashReadOff);
		if (rc) {
			errorP("Fail to read 0x%xB from part[%s] at offset 0x%x\n",
				leftSz, partName, IMG_PRELOAD_SZ);
			return __LINE__;
		}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
		part_dec(partName, (u8 *)(loadaddr + (unsigned int)flashReadOff), leftSz,
			(u8 *)(loadaddr + (unsigned int)flashReadOff), leftSz, flashReadOff);
#endif
	}
	debugP("totalSz=0x%x\n", totalSz);

	return 0;
}

#define IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define IH_NMLEN		32	/* Image Name Length		*/

#pragma pack(push, 1)
typedef struct pack_header{
	unsigned int 	magic;	/* Image Header Magic Number	*/
	unsigned int 	hcrc;	/* Image Header CRC Checksum	*/
	unsigned int	size;	/* Image Data Size		*/
	unsigned int	start;	/* item data offset in the image*/
	unsigned int	end;		/* Entry Point Address		*/
	unsigned int	next;	/* Next item head offset in the image*/
	unsigned int	dcrc;	/* Image Data CRC Checksum	*/
	unsigned char	index;		/* Operating System		*/
	unsigned char	nums;	/* CPU architecture		*/
	unsigned char   type;	/* Image Type			*/
	unsigned char 	comp;	/* Compression Type		*/
	char 	name[IH_NMLEN];	/* Image Name		*/
}AmlResItemHead_t;
#pragma pack(pop)

#define CONFIG_MAX_PIC_LEN (32 << 20)
static const unsigned char gzip_magic[] = { 0x1f, 0x8b };
#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
static const unsigned char zstd_magic[] = { 0x28, 0xb5, 0x2f, 0xfd };
#endif

//uncompress known format for 'imgread pic'
static int imgread_uncomp_pic(unsigned char* srcAddr, const unsigned srcSz,
	unsigned char *dstAddr, const unsigned int dstBufSz, unsigned long *dstDatSz)
{
	/*debugP("srcAddr[%x, %x]\n", srcAddr[0], srcAddr[1]);*/
	if (!memcmp(srcAddr, gzip_magic, sizeof(gzip_magic))) {
		*dstDatSz = srcSz;
		return gunzip(dstAddr, dstBufSz, srcAddr, dstDatSz);
	}
#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
	else if (!memcmp(srcAddr, zstd_magic, sizeof(zstd_magic))) {
		int decomp_res = amfc_decompress(srcAddr, dstAddr, srcSz, dstBufSz, 0);

		if (decomp_res < 0) {
			errorP("amfc_decompress failed, res=%d\n", decomp_res);
			return __LINE__;
		}
		*dstDatSz = (unsigned long)decomp_res;
		return 0;
	}
#endif
	return 0;
}

//[imgread pic] logo bootup $loadaddr_misc
static int do_image_read_pic(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char * const partName = argv[1];
	unsigned char *loadaddr = 0;
	int rc = 0;
	const AmlResImgHead_t *pResImgHead = NULL;
	//unsigned totalSz    = 0;
	u64 flashReadOff = 0;
	const unsigned int PreloadSz = PIC_PRELOAD_SZ;
	unsigned int itemIndex = 0;
	const AmlResItemHead_t *pItem = NULL;
	const char *picName = argv[2];

	loadaddr = (unsigned char *)simple_strtoul(argc > 3 ? argv[3] :
		env_get("loadaddr_misc"), NULL, 16);

	pResImgHead = (AmlResImgHead_t *)loadaddr;

	debugP("to read pic (%s)\n", picName);
	rc = store_logic_read(partName, flashReadOff, PreloadSz, loadaddr);
	if (rc) {
		errorP("Fail to read 0x%xB from part[%s] at offset 0\n", PreloadSz, partName);
		return __LINE__;
	}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
	part_dec(partName, (u8 *)loadaddr, PreloadSz,
		(u8 *)loadaddr, PreloadSz, flashReadOff);
#endif
	flashReadOff = PreloadSz;
	debugP("end read pic sz %d\n", PreloadSz);

	if (img_res_check_log_header(pResImgHead)) {
		errorP("Logo header err.\n");
		return __LINE__;
	}

	//correct bootup for mbox
	while (!strcmp("bootup", picName)) {
		char *outputmode = env_get("outputmode");

		if (!outputmode)
			break;//not env outputmode

		rc = !strncmp("720", outputmode, 3) ||
			!strncmp("576", outputmode, 3) ||
			!strncmp("480", outputmode, 3);
		if (rc) {
			picName = "bootup_720";
			break;
		}

		picName = "bootup_1080";
		break;
	}

	pItem = (AmlResItemHead_t *)(pResImgHead + 1);
	for (itemIndex = 0; itemIndex < pResImgHead->imgItemNum; ++itemIndex, ++pItem) {
		if (pItem->magic != IH_MAGIC) {
			errorP("item magic 0x%x != 0x%x\n", pItem->magic, IH_MAGIC);
			return __LINE__;
		}
		if (pItem->start > CONFIG_MAX_PIC_LEN) {
			errorP("item data offset err 0x%x != 0x%x\n", pItem->start,
				CONFIG_MAX_PIC_LEN);
			return __LINE__;
		}
		if (pItem->size > CONFIG_MAX_PIC_LEN) {
			errorP("item data size err 0x%x != 0x%x\n", pItem->size,
				CONFIG_MAX_PIC_LEN);
			return __LINE__;
		}
		if (!strcmp(picName, pItem->name) || !strcmp(argv[2], pItem->name)) {
			char env_name[IH_NMLEN * 2];
			char env_data[IH_NMLEN * 2];
			unsigned long picLoadAddr = (unsigned long)loadaddr +
				(unsigned int)pItem->start;
			unsigned int  itemSz = pItem->size;
			unsigned long uncompSz = 0;

			if (pItem->start + itemSz > flashReadOff) {
				unsigned long rdOff = pItem->start;
				//align 2k page for mtd nand, 512 for emmc
				unsigned long rdOffAlign = (rdOff >> 11) << 11;

				rc = store_logic_read(partName, rdOffAlign,
					itemSz + (rdOff & 0x7ff),
					(char *)((picLoadAddr >> 11) << 11));
				if (rc) {
					errorP("Fail to read pic at offset 0x%x\n", pItem->start);
					return __LINE__;
				}
#if CONFIG_PARTITION_ENCRYPTION_LOCAL
				part_dec(partName, (u8 *)((picLoadAddr >> 11) << 11),
					itemSz + (rdOff & 0x7ff),
					(u8 *)((picLoadAddr >> 11) << 11),
					itemSz + (rdOff & 0x7ff), rdOffAlign);
#endif
				debugP("pic sz 0x%x\n", itemSz);
			}

			//uncompress supported format
			unsigned long uncompLoadaddr = picLoadAddr + itemSz + 7;

			uncompLoadaddr &= ~(0x7U);
			rc = imgread_uncomp_pic((unsigned char *)picLoadAddr, itemSz,
				(unsigned char *)uncompLoadaddr, CONFIG_MAX_PIC_LEN, &uncompSz);
			if (rc) {
				errorP("Fail in uncomp pic,rc[%d]\n", rc);
				return __LINE__;
			}
			if (uncompSz) {
				itemSz = uncompSz;
				picLoadAddr = uncompLoadaddr;
			}

			sprintf(env_name, "%s_offset", argv[2]);
			sprintf(env_data, "0x%lx", picLoadAddr);
			env_set(env_name, env_data);

			sprintf(env_name, "%s_size", argv[2]);
			sprintf(env_data, "0x%x", itemSz);
			env_set(env_name, env_data);

			debugP("end read pic[%s]\n", picName);
			return 0;//success
		}
	}

	return __LINE__;//fail
}

static cmd_tbl_t cmd_imgread_sub[] = {
	U_BOOT_CMD_MKENT(kernel, 4, 0, do_image_read_kernel, "", ""),
	U_BOOT_CMD_MKENT(dtb,    4, 0, do_image_read_dtb, "", ""),
	U_BOOT_CMD_MKENT(res,    3, 0, do_image_read_res, "", ""),
	U_BOOT_CMD_MKENT(pic,    4, 0, do_image_read_pic, "", ""),
	U_BOOT_CMD_MKENT(part,   5, 0, do_image_read_part, "", ""),
};

static int do_image_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_PXP_EMULATOR
	printf("\naml log : PXP image all use preload\n");
	do { (void)cmd_imgread_sub[0]; } while(0);
	return 0;
#else
	cmd_tbl_t *c;
	int ret;

	/* Strip off leading 'imgread' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_imgread_sub[0], ARRAY_SIZE(cmd_imgread_sub));

	if (c) {
		if (!strcmp("kernel", argv[0]))
			PUSH_TIME_TE(__func__, BL33_LOAD_UIMAGE_s);
		ret = c->cmd(cmdtp, flag, argc, argv);
		if (!strcmp("kernel", argv[0]))
			PUSH_TIME_TE(__func__, BL33_LOAD_UIMAGE_e);
		return	ret;
	} else {
		cmd_usage(cmdtp);
		return 1;
	}
#endif //CONFIG_PXP_EMULATOR
}

U_BOOT_CMD(
   imgread,         //command name
   6,               //maxargs
   0,               //repeatable
   do_image_read,   //command function
   "Read the image from internal flash with actual size",           //description
   "    argv: <imageType> <part_name> <loadaddr> \n"   //usage
   "    - <image_type> Current support is kernel/res(ource).\n"
   "imgread kernel  --- Read image in format IMAGE_FORMAT_ANDROID\n"
   "imgread dtb     --- Read dtb in format IMAGE_FORMAT_ANDROID\n"
   "imgread res     --- Read image packed by 'Amlogic resource packer'\n"
   "imgread picture --- Read one picture from Amlogic logo"
   "imgread part    --- Read partition"
   "    - e.g. \n"
   "        to read boot.img     from part boot     from flash: <imgread kernel boot loadaddr> \n"   //usage
   "        to read recovery.img from part recovery from flash: <imgread kernel recovery loadaddr $offset> \n"   //usage
   "        to read logo.img     from part logo     from flash: <imgread res    logo loadaddr> \n"   //usage
   "        to read one picture named 'bootup' from logo.img    from logo: <imgread pic logo bootup loadaddr> \n"   //usage
   "        to read partition    from               from flash: <imgread part <part_name> <load_addr> <offset> <sz>> \n"   //usage
);

//[imgread pic] logo bootup $loadaddr_misc
static int do_unpackimg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char *loadaddr = 0;
	const AmlResImgHead_t *pResImgHead = NULL;
	unsigned int itemIndex = 0;
	const AmlResItemHead_t *pItem = NULL;

	loadaddr = (unsigned char *)simple_strtoul(argc > 1 ? argv[1] :
		env_get("loadaddr_misc"), NULL, 16);

	pResImgHead = (AmlResImgHead_t *)loadaddr;
	const int totalSz = pResImgHead->imgSz;
	unsigned long unCompressBuf = (long)loadaddr + totalSz;

	if (img_res_check_log_header(pResImgHead)) {
		errorP("Logo header err.\n");
		return __LINE__;
	}

	pItem = (AmlResItemHead_t *)(pResImgHead + 1);
	for (itemIndex = 0; itemIndex < pResImgHead->imgItemNum; ++itemIndex, ++pItem) {
		if (pItem->magic != IH_MAGIC) {
			errorP("item magic 0x%x != 0x%x\n", pItem->magic, IH_MAGIC);
			return __LINE__;
		}
		char env_name[IH_NMLEN * 2];
		char env_data[IH_NMLEN * 2];
		unsigned long picLoadAddr = (unsigned long)loadaddr +
			(unsigned int)pItem->start;

		int itemSz = pItem->size;
		unsigned long uncompSz = 0;

		if (unCompressBuf & 0x7U)
			unCompressBuf = ((unCompressBuf + 8) >> 3) << 3;
		imgread_uncomp_pic((unsigned char *)picLoadAddr, pItem->size,
				(unsigned char *)unCompressBuf, CONFIG_MAX_PIC_LEN, &uncompSz);
		if (uncompSz) {
			picLoadAddr = (unsigned long)unCompressBuf;
			itemSz      = uncompSz;
			unCompressBuf += uncompSz;
		}
		sprintf(env_name, "%s_offset", pItem->name);
		sprintf(env_data, "0x%lx", picLoadAddr);
		env_set(env_name, env_data);

		sprintf(env_name, "%s_size", pItem->name);
		sprintf(env_data, "0x%x", itemSz);
		env_set(env_name, env_data);
	}

	return 0;//success
}

U_BOOT_CMD(
   unpackimg,           //command name
   2,                   //maxargs
   0,                   //repeatable
   do_unpackimg,   //command function
   "un pack logo image into pictures",           //description
   "    argv: unpackimg <imgLoadaddr> \n"   //usage
   "    un pack the logo image, which already loaded at <imgLoadaddr>.\n"
);

#if defined(CONFIG_CMD_AUTOSCR)
/*
 * Keep for now for backward compatibility;
 * remove later when support for "autoscr" goes away.
 */
static int
do_autoscr (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf ("\n### WARNING ### "
			"\"autoscr\" is deprecated, use \"source\" instead ###\n\n");
	if (argc < 2) {
		printf("too few argc %d for %s\n", argc, argv[0]);
		return CMD_RET_FAILURE;
	}
	env_set("_src_addr", argv[1]);
	return run_command("echo _src_addr ${_src_addr}; source ${_src_addr}; env delete _src_addr", 0);
}

U_BOOT_CMD_COMPLETE(
	autoscr, 2, 0,	do_autoscr,
	"DEPRECATED - use \"source\" command instead",
	"	argv: autoscr script_mem_addr",
	var_complete
);
#endif//#if defined(CONFIG_CMD_AUTOSCR)

char *get_suffix(const char *filename)
{
	if (!filename)
		return "";
	char *dot = strrchr(filename, '.');

	if (!dot || dot == filename)
		return "";
	return dot + 1;
}

bool is_jpg_file(const char *path)
{
	if (!path || strlen(path) < 4)
		return false;
	char *suffix = get_suffix(path);

	if (!suffix)
		return false;
	else
		return !strcmp(suffix, "jpg");
}

#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
bool is_zstd_file(const char *path)
{
	if (strlen(path) < 5)
		return false;
	char *suffix = get_suffix(path);

	if (!suffix)
		return false;
	else
		return !strcmp(suffix, "zstd");
}

void convert_extension_if_jpg(char *logo_value, size_t max_len)
{
	const char *old_ext = ".jpg";
	const char *new_ext = ".bmp.zstd";

	if (!logo_value)
		return;
	if (!is_jpg_file(logo_value))
		return;

	size_t len = strlen(logo_value);
	size_t old_ext_len = strlen(old_ext);
	size_t new_ext_len = strlen(new_ext);

	if (len < old_ext_len)
		return;

	char *suffix_ptr = logo_value + len - old_ext_len;

	size_t needed_size = len - old_ext_len + new_ext_len + 1;

	if (needed_size <= max_len)
		strcpy(suffix_ptr, new_ext);
	else
		printf("[Error] Buffer size too small to expand extension!\n");
}
#endif

#if defined(CONFIG_CMD_EXT4) && defined(CONFIG_MMC_MESON_GX)
/*"if ext4load mmc 1:x ${dtb_mem_addr} /recovery/dtb.img; then echo cache dtb.img loaded; fi;"\*/
static int do_load_logo_from_ext4(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 3) {
		errorP("argc(%d) < 3 illegal\n", argc);
		return CMD_RET_USAGE;
	}
	int iRet = 0;
	int partIndex;
	const char *ext4Part = argv[1];
	void *loadaddr = (void *)simple_strtoul(argv[2], NULL, 16);
	int autoSelectSlot = 1;//auto detect if need add _a/_b
	if (argc > 3) {
		env_set("ext4LogoPath", argv[3]);
	} else {
		char *tmp = env_get("usb_status");
		char *cc_enable = env_get("cc_enable");

		if (tmp && strcmp("0.5a@5v", tmp) == 0 &&
			cc_enable && strcmp("1", cc_enable) == 0) {
			env_set("ext4LogoPath", "/logo_files/bootup_lowcurrent.bmp");
		} else {
			tmp = env_get("jpg_logo");
			if (tmp && strcmp("true", tmp) == 0)
				env_set("ext4LogoPath", "/logo_files/bootup.jpg");
			else
				env_set("ext4LogoPath", "/logo_files/bootup.bmp");
		}
	}
	if (argc > 4) {
		const char *paraAutoSel = argv[4];

		autoSelectSlot = !memcmp(paraAutoSel, "true", 5);
		if (!autoSelectSlot && strcmp(paraAutoSel, "false")) {
			errorP("illegal para4 %s\n", paraAutoSel);
			return CMD_RET_FAILURE;
		}
	}

	if (!loadaddr) {
		errorP("illegal loadaddr %s\n", argv[2]);
		return CMD_RET_FAILURE;
	}

	if (store_get_type() != BOOT_EMMC) {
		errorP("only support emmc, but store type %d\n", store_get_type());
		return CMD_RET_FAILURE;
	}

	env_set("bootLogoPart", ext4Part);
	run_command("printenv bootLogoPart;", 0);
	partIndex = get_partition_num_by_name(env_get("bootLogoPart"));
	debug("partIndex = %d\n", partIndex);
	if (partIndex <= 0) {
		debug("fail find part index for name(%s), try ab\n", env_get("bootLogoPart"));
		if (autoSelectSlot)
			run_command("if test ${active_slot} != normal; then setenv bootLogoPart ${bootLogoPart}${active_slot}; printenv bootLogoPart; fi", 0);
		partIndex = get_partition_num_by_name(env_get("bootLogoPart"));
		if (partIndex <= 0) {
			errorP("fail find part index for name(%s), exit\n",
				env_get("bootLogoPart"));
			return CMD_RET_FAILURE;
		}
	}
#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
	if (is_jpg_file(env_get("ext4LogoPath"))) {
		char logo_value_buf[128];
		char check_cmd[300];

		strlcpy(logo_value_buf, env_get("ext4LogoPath"), sizeof(logo_value_buf));
		convert_extension_if_jpg(logo_value_buf, 128);

		snprintf(check_cmd, sizeof(check_cmd),
				"ext4size mmc 1:%x %s", partIndex, logo_value_buf);
		if (!run_command(check_cmd, 0)) {
			env_set("logo_file_name", logo_value_buf);
			env_set("ext4LogoPath", logo_value_buf);
			env_set("jpg_logo", "false");
		}
	}
#endif
	env_set_hex("logoPart", partIndex);
	env_set_hex("logoLoadAddr", (ulong)loadaddr);
	env_set("ext4logoLoadCmd", "ext4load mmc 1:${logoPart} ${logoLoadAddr} ${ext4LogoPath}");
	iRet = run_command("printenv ext4logoLoadCmd; run ext4logoLoadCmd", 0);
	if (iRet) {
		errorP("Fail in load logo cmd\n");
		return CMD_RET_FAILURE;
	}
	MsgP("load bmp from ext4 part okay\n");
	run_command("setenv ext4LogoSz ${filesize}", 0);
	const int bmpSz = env_get_hex("filesize", 0);

	if (bmpSz <= 0) {
		errorP("err bmp sz\n");
		return CMD_RET_FAILURE;
	}

#if IS_ENABLED(CONFIG_GZIP) || IS_ENABLED(CONFIG_AMLOGIC_AMFC)
	int is_compressed = 0;

#if IS_ENABLED(CONFIG_GZIP)
	if (!memcmp(loadaddr, gzip_magic, sizeof(gzip_magic)))
		is_compressed = 1;
#endif

#if IS_ENABLED(CONFIG_AMLOGIC_AMFC)
	if (!memcmp(loadaddr, zstd_magic, sizeof(zstd_magic)))
		is_compressed = 1;
#endif

	if (!is_compressed)
		return CMD_RET_SUCCESS;

	MsgP("decompress bmp logo\n");

	void *uncompress = (char *)loadaddr + (((bmpSz + 0xf) >> 4) << 4);
	unsigned long uncompSz = 0;

	iRet = imgread_uncomp_pic((unsigned char *)loadaddr, bmpSz,
			(unsigned char *)uncompress,
			CONFIG_MAX_PIC_LEN, (unsigned long *)&uncompSz);

	if (iRet) {
		errorP("Fail in uncomp pic,rc[%d]\n", iRet);
		return __LINE__;
	}
	if (uncompSz <= 0) {
		errorP("Fail uncompress logo bmp\n");
		return CMD_RET_FAILURE;
	}

	memmove(loadaddr, uncompress, uncompSz);
	env_set_hex("ext4LogoSz", uncompSz);
#endif

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD_COMPLETE(
   rdext4pic,                   //read ext4 picture
	5,                           //maxargs
   0,                           //repeatable
   do_load_logo_from_ext4,      //command function
   "read logo bmp from ext4 part",           //description
   "    argv: rdext4pic <partName> <memAddr> <logoPath>\n"   //usage
   "    load bmp picture from <logoPath> of <partName> to <memAddr>.\n",
   var_complete
);
#endif// #if defined(CONFIG_CMD_EXT4) && defined(CONFIG_MMC_MESON_GX)

