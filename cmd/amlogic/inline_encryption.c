// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
//#include <command.h>
#include <amlogic/aml_crypto.h>
#include <amlogic/aml_mkl.h>

#if (defined(CONFIG_MESON_T6D) || defined(CONFIG_MESON_T6W))
#define KEY_TABLE_ENTRY_EMMC_ENC	(32)
#elif (defined(CONFIG_MESON_S7) || defined(CONFIG_MESON_S6) || defined(CONFIG_MESON_S7D))
#define KEY_TABLE_ENTRY_EMMC_ENC	(128)
#else
#error "Inline encryption is not supported by this chip.\n"
#endif

#define DEBUG		(0)

#if DEBUG
static void dump_bufs(const char *name, const u8 *buf, s32 len)
{
	char line_buf[128] = {0x0};
	char *line_buf_ptr = line_buf;
	u32 i = 0;
	const u8 *ptr = buf;
	const s32 sz = len;

	printf("\n========= Start dump %s(%d)\n", name, sz);
	while (len - 16 >= 0) {
		printf("%02x %02x %02x %02x %02x %02x %02x %02x ",
		       ptr[0], ptr[i + 1], ptr[i + 2], ptr[i + 3], ptr[i + 4], ptr[i + 5],
		       ptr[i + 6], ptr[i + 7]);
		printf("%02x %02x %02x %02x %02x %02x %02x %02x\n",
		       ptr[i + 8], ptr[i + 9], ptr[i + 10], ptr[i + 11], ptr[i + 12],
		       ptr[i + 13], ptr[i + 14], ptr[i + 15]);
		ptr += 16;
		len -= 16;
	}

	if (len * 3 <= sizeof(line_buf)) {
		while (len) {
			snprintf(line_buf_ptr, sizeof(line_buf), "%02x ", *ptr);
			line_buf_ptr += 3; /*"xx "*/
			len--;
		}
		if (strlen(line_buf))
			printf("%s\n", line_buf);
		printf("========= End dump %s(%d)\n", name, sz);
	} else {
		printf("remaining length exceeds linebuf length\n");
	}
}
#endif

static int derive_key_by_kl(u8 *ek1, u8 *ek2, u8 *ek3)
{
	int ret = 0;
	struct amlkl_params kl_param;

	if (!ek1 || !ek2 || !ek3) {
		printf("%s:%d: invalid input\n", __func__, __LINE__);
		return -1;
	}

	memset(&kl_param, 0, sizeof(kl_param));
	kl_param.kt_handle = KEY_TABLE_ENTRY_EMMC_ENC;
	kl_param.levels = AML_KL_LEVEL_3;
	kl_param.usage.crypto = AML_KT_FLAG_ENC_DEC;
	kl_param.usage.algo = AML_KT_ALGO_AES;
	kl_param.usage.uid = AML_KT_USER_M2M_0;
	kl_param.kl_algo = AML_KL_ALGO_AES;
	kl_param.kl_mode = AML_KL_MODE_AML;
	/* use mrk DGPK1 */
	kl_param.mrk_cfg_index = AML_KL_MRK_DGPK1;
	/* function id 2 */
	kl_param.func_id = AML_KL_FUNC_AES_2;
	memcpy(kl_param.eks[0], ek1, 16);
	memcpy(kl_param.eks[1], ek2, 16);
	memcpy(kl_param.eks[2], ek3, 16);
	ret = aml_mkl_run(&kl_param);
	if (ret != 0) {
		printf("derive key error[ret:%d]\n", ret);
		return -1;
	}
	return 0;
}

int derive_inline_enc_key(u8 *key, u32 key_sz)
{
	int ret = -1;

	u8 ek1[16] = { 0x67, 0x44, 0x2b, 0xb4, 0xd1, 0x3f, 0x9a, 0x59,
		       0xd5, 0x1a, 0x0d, 0x55, 0xe7, 0xd8, 0x9d, 0xad };
	u8 ek2[16] = { 0xdb, 0xf0, 0xa8, 0x00, 0x04, 0xda, 0x58, 0x6d,
		       0x6a, 0x62, 0xdb, 0x3b, 0x88, 0xad, 0xae, 0xad };
	u8 ek3[16] = { 0x63, 0x80, 0xf2, 0xfa, 0xcf, 0x1e, 0x26, 0x08,
		       0x47, 0x9f, 0xe9, 0xdd, 0x16, 0x4d, 0xd6, 0x4d };

	u8 ek1_xts[32] = {
		0x3a, 0xc2, 0x22, 0xeb, 0x28, 0x0b, 0x1d, 0x6e,
		0xe3, 0x57, 0x4c, 0x1c, 0x1c, 0x49, 0x12, 0x29,
		0x5a, 0x3a, 0xfb, 0x1a, 0x17, 0xad, 0x11, 0x37,
		0x9e, 0x30, 0xc3, 0xd6, 0x76, 0x50, 0x99, 0x8f
	};

	u8 ek2_xts[32] = {
		0x70, 0x13, 0x94, 0xf7, 0x0b, 0x38, 0x44, 0x8b,
		0x8a, 0x0a, 0x7b, 0xc8, 0x35, 0x3c, 0xf8, 0xe3,
		0xfb, 0x58, 0xf7, 0xe3, 0x84, 0x95, 0x63, 0x65,
		0x0f, 0x95, 0x0b, 0x12, 0xd8, 0x3b, 0x81, 0x0c
	};

	u8 dst[AES_KEY_SIZE_256 * 2] = {0x0};

	if (!key || key_sz != AES_KEY_SIZE_256 * 2) {
		printf("%s:%d: invalid input(%p, %u)\n",
		       __func__, __LINE__, key, key_sz);
		return -1;
	}

	ret = derive_key_by_kl(ek1, ek2, ek3);
	if (ret < 0) {
		printf("%s:%d: derive_key_by_kl failed(%d)\n",
		       __func__, __LINE__, ret);
		return -1;
	}

	/* ek1_xts */
	ret = aes_ecb_dec_keytbl(AES_KEY_SIZE_128, ek1_xts, dst,
				 NULL, AES_KEY_SIZE_256, KEY_TABLE_ENTRY_EMMC_ENC);
	if (ret) {
		printf("%s:%d: aes_ecb_dec_keytbl failed(%d)\n",
		       __func__, __LINE__, ret);
		return -1;
	}

	/* ek2_xts */
	ret = aes_ecb_dec_keytbl(AES_KEY_SIZE_128, ek2_xts, &dst[AES_KEY_SIZE_256],
				 NULL, AES_KEY_SIZE_256, KEY_TABLE_ENTRY_EMMC_ENC);
	if (ret) {
		printf("%s:%d: aes_ecb_dec_keytbl failed(%d)\n",
		       __func__, __LINE__, ret);
		return -1;
	}
	memcpy(key, dst, sizeof(dst));
#if DEBUG
	dump_bufs("inline key", key, key_sz);
#endif
	return 0;
}

int derive_inline_enc_usb_key(u8 *key, u32 key_sz)
{
	int ret = 0;

	u8 ek1[16] = { 0xf9, 0x6f, 0x94, 0xf7, 0x5f, 0x37, 0x45, 0xe8,
		       0xb7, 0x80, 0x1a, 0xdb, 0x7d, 0x6e, 0x1f, 0x4f };
	u8 ek2[16] = { 0xa6, 0x18, 0xdf, 0x0d, 0x61, 0xd7, 0x7f, 0x09,
		       0x54, 0xa1, 0xfc, 0xc7, 0xc6, 0x53, 0x71, 0xe6 };
	u8 ek3[16] = { 0x11, 0x37, 0xb4, 0x5b, 0xf2, 0xad, 0x21, 0xbe,
		       0xfb, 0xdb, 0x16, 0x6e, 0x5f, 0x3c, 0x60, 0xd5 };
	u8 ek_usb[16] = { 0x62, 0x53, 0x05, 0xc3, 0x10, 0x19, 0x37, 0xc8,
			  0x08, 0x38, 0x72, 0x18, 0xfe, 0x2a, 0x63, 0x2f };
	u8 dst[AES_KEY_SIZE_128] = {0x0};

	if (!key || key_sz != AES_KEY_SIZE_128) {
		printf("%s:%d: invalid input(%p, %u)\n",
		       __func__, __LINE__, key, key_sz);
		return -1;
	}

	ret = derive_key_by_kl(ek1, ek2, ek3);
	if (ret < 0) {
		printf("%s:%d: derive_key_by_kl failed\n",
		       __func__, __LINE__);
		return -1;
	}

	ret = aes_ecb_dec_keytbl(AES_KEY_SIZE_128, ek_usb, dst,
				 NULL, AES_KEY_SIZE_128, KEY_TABLE_ENTRY_EMMC_ENC);
	if (ret) {
		printf("%s:%d: aes_ecb_dec_keytbl failed\n",
		       __func__, __LINE__);
		return -1;
	}

	memcpy(key, dst, sizeof(dst));
#if DEBUG
	dump_bufs("usb key", key, key_sz);
#endif
	return 0;
}

