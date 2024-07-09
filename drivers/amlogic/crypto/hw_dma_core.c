// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/compat.h>
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <cpu_func.h>
#include <dma.h>
#include <asm/amlogic/arch/regs.h>
#include <asm/amlogic/arch/secure_apb.h>
#include <linux/arm-smccc.h>
#include <amlogic/aml_crypto.h>
#include <crypto_internal.h>
#include <cpu_func.h>

//#define CRYPTO_DEBUG

/* lock for crypto T0 if needed in the future */
DEFINE_MUTEX(&crypto_lock);

s8 hw_dma_inited;

#define CRYPTO_CMD					0x8200007B
	#define CRYPTO_CMD_PART_ENC_DERIVE_KEY  0x001
	#define CRYPTO_CMD_CRYPTO_DMA_SET_BUS64 0x002

static int32_t hw_dma_init(void)
{
	s32 ret = 0;
#ifdef CONFIG_AML_CRYPTO_64
	struct arm_smccc_res res;
#endif
	if (hw_dma_inited)
		goto out;
#ifdef CONFIG_AML_CRYPTO_64
	arm_smccc_smc(CRYPTO_CMD, CRYPTO_CMD_CRYPTO_DMA_SET_BUS64,
		      0,	/* thread */
		      64,	/* mode */
		      0, 0, 0, 0, &res);
	ret = res.a0;
#endif
	if (!ret)
		hw_dma_inited = 1;
out:
	return ret;
}

#ifdef CONFIG_AML_CRYPTO_64
static inline void write_thread_reg(u64 data)
{
	u32 high = (u32)(0xff & (data >> 32));
	u32 low = (u32)(0xffffffff & data);

	*P_DMA_T0_H = high;
	*P_DMA_T0 = low;
}
#endif

static int32_t cipher_core(void *key, uint32_t keylen, uint8_t *iv, uint32_t ivlen,
		   const void *src, void *dst, uint8_t encrypt, uint8_t mode,
		   uint8_t op_mode, size_t size)
{
	struct dma_dsc dsc[4];
	uint8_t iv0[16] = {0};
	uint32_t blocks = size / DMA_BLOCK_SIZE;
	uint32_t residues = size & (DMA_BLOCK_SIZE - 1);
	int32_t ret = 0;
#ifdef CRYPTO_DEBUG
	uint32_t i = 0;
#endif
	ret = hw_dma_init();
	if (ret)
		return ret;

	if (blocks > MAX_BLOCK_TRANSFER)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	mutex_lock(&crypto_lock);

	if (!iv)
		iv = iv0;

	dsc[0].src_addr = (uintptr_t)key;
	dsc[0].tgt_addr = 0;
	dsc[0].dsc_cfg.d32 = 0;
	/* HW bug, key has to set to multiple of 16 bytes, even for des */
	dsc[0].dsc_cfg.b.length = 32;
	dsc[0].dsc_cfg.b.mode = DMA_MODE_KEY;
	dsc[0].dsc_cfg.b.owner = 1;

	dsc[1].src_addr = (uintptr_t)iv;
	dsc[1].tgt_addr = 32;
	dsc[1].dsc_cfg.d32 = 0;
	/* HW bug, IV has to set to multiple of 16 bytes, even for des */
	dsc[1].dsc_cfg.b.length = 16;
	dsc[1].dsc_cfg.b.mode = DMA_MODE_KEY;
	dsc[1].dsc_cfg.b.owner = 1;

	if (blocks) {
		dsc[2].src_addr = (uintptr_t)src;
		dsc[2].tgt_addr = (uintptr_t)dst;
		dsc[2].dsc_cfg.d32 = 0;
		dsc[2].dsc_cfg.b.length = blocks;
		dsc[2].dsc_cfg.b.eoc = 0;
		dsc[2].dsc_cfg.b.mode = mode;
		dsc[2].dsc_cfg.b.op_mode = op_mode;
		dsc[2].dsc_cfg.b.enc_sha_only = encrypt ?
			CIPHER_ENC_SHA_ONLY_ENCRYPT : CIPHER_ENC_SHA_ONLY_DECRYPT;
		dsc[2].dsc_cfg.b.block = 1;
		dsc[2].dsc_cfg.b.owner = 1;

		dsc[3].src_addr = (uintptr_t)src + blocks * DMA_BLOCK_SIZE;
		dsc[3].tgt_addr = (uintptr_t)dst + blocks * DMA_BLOCK_SIZE;
		dsc[3].dsc_cfg.d32 = 0;
		dsc[3].dsc_cfg.b.length = residues;
		dsc[3].dsc_cfg.b.eoc = 1;
		dsc[3].dsc_cfg.b.mode = mode;
		dsc[3].dsc_cfg.b.op_mode = op_mode;
		dsc[3].dsc_cfg.b.enc_sha_only = encrypt ?
			CIPHER_ENC_SHA_ONLY_ENCRYPT : CIPHER_ENC_SHA_ONLY_DECRYPT;
		dsc[3].dsc_cfg.b.block = 0;
		dsc[3].dsc_cfg.b.owner = 1;
	} else {
		dsc[2].src_addr = (uintptr_t)src;
		dsc[2].tgt_addr = (uintptr_t)dst;
		dsc[2].dsc_cfg.d32 = 0;
		dsc[2].dsc_cfg.b.length = residues;
		dsc[2].dsc_cfg.b.eoc = 1;
		dsc[2].dsc_cfg.b.mode = mode;
		dsc[2].dsc_cfg.b.op_mode = op_mode;
		dsc[2].dsc_cfg.b.enc_sha_only = encrypt ?
			CIPHER_ENC_SHA_ONLY_ENCRYPT : CIPHER_ENC_SHA_ONLY_DECRYPT;
		dsc[2].dsc_cfg.b.block = 0;
		dsc[2].dsc_cfg.b.owner = 1;
	}

	/* key */
	if (dsc[0].src_addr < KEYTABLE_BASE)
		flush_dcache_range((uintptr_t)dsc[0].src_addr,
				(uintptr_t)dsc[0].src_addr + 32);
	/* iv */
	flush_dcache_range((uintptr_t)dsc[1].src_addr, (unsigned long)dsc[1].src_addr + 16);
	/* dsc */
	flush_dcache_range((uintptr_t)&dsc, (uintptr_t)&dsc + sizeof(dsc));
	flush_dcache_range((uintptr_t)src, (uintptr_t)src + size);
	flush_dcache_range((uintptr_t)dst, (uintptr_t)dst + size);

	*P_DMA_STS0 = 0xf;
#ifdef CONFIG_AML_CRYPTO_64
	write_thread_reg((uintptr_t)dsc | 2);
#else
	*P_DMA_T0 = (uintptr_t)dsc | 2;
#endif
	while (*P_DMA_STS0 == 0)
		;

	if (*P_DMA_STS0 & 0x2)
		ret = CRYPTO_ERROR_BAD_PROCESS;
	else
		ret = CRYPTO_ERROR_NO_ERROR;

	invalidate_dcache_range((uintptr_t)dst, (uintptr_t)dst + size);

	mutex_unlock(&crypto_lock);
#ifdef CRYPTO_DEBUG
	printf("*P_DMA_STS0 = %x\n", *P_DMA_STS0);
	for (i = 0; i < sizeof(dsc) / sizeof(struct dma_dsc); i++) {
		printf("desc (%4x) (len) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.length);
		printf("desc (%4x) (irq) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.irq);
		printf("desc (%4x) (eoc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.eoc);
		printf("desc (%4x) (lop) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.loop);
		printf("desc (%4x) (mod) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.mode);
		printf("desc (%4x) (beg) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.begin);
		printf("desc (%4x) (end) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.end);
		printf("desc (%4x) (opm) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.op_mode);
		printf("desc (%4x) (enc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.enc_sha_only);
		printf("desc (%4x) (blk) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.block);
		printf("desc (%4x) (err) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.error);
		printf("desc (%4x) (own) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.owner);
#ifdef CONFIG_AML_CRYPTO_64
		printf("desc (%4x) (src) = 0x%llx\n", i,
		       dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%llx\n", i,
		       dsc[i].tgt_addr);
#else
		printf("desc (%4x) (src) = 0x%8x\n", i,
				dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%8x\n", i,
				dsc[i].tgt_addr);
#endif
	}
#endif
	return ret;
}

int32_t aes_cipher(void *key, uint32_t keylen, uint8_t iv[16],
		const void *src, void *dst, uint8_t encrypt, uint8_t mode,
		uint8_t op_mode, size_t size)
{
	if (keylen != AES_KEY_SIZE_128 && keylen != AES_KEY_SIZE_256)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	return cipher_core(key, keylen, iv, 16,
		   src, dst, encrypt, mode, op_mode, size);
}

int32_t des_tdes_cipher(void *key, uint32_t keylen, uint8_t iv[8],
		const void *src, void *dst, uint8_t encrypt, uint8_t mode,
		uint8_t op_mode, size_t size)
{
	if (keylen != DES_KEY_SIZE &&
			keylen != TDES_2K_KEY_SIZE &&
			keylen != TDES_3K_KEY_SIZE)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	return cipher_core(key, keylen, iv, 8,
		   src, dst, encrypt, mode, op_mode, size);
}

int32_t sha2_update_internal(sha2_ctx *ctx, const uint8_t *input,
		uint32_t ilen, uint8_t *hash, uint8_t last_update)
{
	struct dma_dsc dsc[2] = {0};
	uint32_t blocks = ilen / DMA_BLOCK_SIZE;
	uint32_t residues = ilen & (DMA_BLOCK_SIZE - 1);
	uint8_t hash_tmp[SHA256_BLOCK_SIZE * 2] = {0};
	int32_t ret = 0;
#ifdef CRYPTO_DEBUG
	uint32_t i = 0;
#endif
	ret = hw_dma_init();
	if (ret)
		return ret;

	if (!ctx)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	if (!last_update && (residues & (0x40 - 1))) {
		printf("Err:sha length not block multiple\n");
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}

	if (last_update && !hash)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	if (blocks > MAX_BLOCK_TRANSFER) {
		printf("Err:sha too large\n");
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}

	mutex_lock(&crypto_lock);

	if (blocks) {
		dsc[0].src_addr = (uintptr_t)input;
		dsc[0].tgt_addr = residues ? 0 : (uintptr_t)hash_tmp;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = blocks;
		dsc[0].dsc_cfg.b.eoc = residues ? 0 : 1;
		dsc[0].dsc_cfg.b.mode = ctx->digest_len == 224 ?
			DMA_MODE_SHA224 : DMA_MODE_SHA256;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = residues ? 0 : last_update;
		dsc[0].dsc_cfg.b.enc_sha_only = SHA2_ENC_SHA_ONLY_SHA;
		dsc[0].dsc_cfg.b.block = 1;
		dsc[0].dsc_cfg.b.owner = 1;

		if (residues) {
			dsc[1].src_addr = (uintptr_t)input + blocks * DMA_BLOCK_SIZE;
			dsc[1].tgt_addr = (uintptr_t)hash_tmp;
			dsc[1].dsc_cfg.d32 = 0;
			dsc[1].dsc_cfg.b.length = residues;
			dsc[1].dsc_cfg.b.eoc = 1;
			dsc[1].dsc_cfg.b.mode = ctx->digest_len == 224 ?
				DMA_MODE_SHA224 : DMA_MODE_SHA256;
			dsc[1].dsc_cfg.b.begin = 0;
			dsc[1].dsc_cfg.b.end = last_update;
			dsc[1].dsc_cfg.b.enc_sha_only = SHA2_ENC_SHA_ONLY_SHA;
			dsc[1].dsc_cfg.b.block = 0;
			dsc[1].dsc_cfg.b.owner = 1;
		}
	} else {
		dsc[0].src_addr = (uintptr_t)input;
		dsc[0].tgt_addr = (uintptr_t)hash_tmp;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = residues;
		dsc[0].dsc_cfg.b.eoc = 1;
		dsc[0].dsc_cfg.b.mode = ctx->digest_len == 224 ?
			DMA_MODE_SHA224 : DMA_MODE_SHA256;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = last_update;
		dsc[0].dsc_cfg.b.enc_sha_only = SHA2_ENC_SHA_ONLY_SHA;
		dsc[0].dsc_cfg.b.block = 0;
		dsc[0].dsc_cfg.b.owner = 1;
	}

	flush_dcache_range((uintptr_t)&dsc, (uintptr_t)&dsc + sizeof(dsc));
	flush_dcache_range((uintptr_t)input, (uintptr_t)input + ilen);
	flush_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));

	*P_DMA_STS0 = 0xf;
#ifdef CONFIG_AML_CRYPTO_64
	write_thread_reg((uintptr_t)dsc | 2);
#else
	*P_DMA_T0 = (uintptr_t)dsc | 2;
#endif
	while (*P_DMA_STS0 == 0)
		;

	invalidate_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));
	if (last_update) {
		if (ctx->digest_len == 224)
			memcpy(hash, hash_tmp, SHA224_DIGEST_SIZE);
		else
			memcpy(hash, hash_tmp, SHA256_DIGEST_SIZE);
	} else {
		memcpy(ctx->state, hash_tmp, sizeof(ctx->state));
	}
	ctx->tot_len += ilen;

	if (*P_DMA_STS0 & 0x1)
		ret = CRYPTO_ERROR_BAD_PROCESS;
	else
		ret = CRYPTO_ERROR_NO_ERROR;

#ifdef CRYPTO_DEBUG
	printf("*P_DMA_STS0 = %x\n", *P_DMA_STS0);
	for (i = 0; i < sizeof(dsc) / sizeof(struct dma_dsc); i++) {
		printf("desc (%4x) (len) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.length);
		printf("desc (%4x) (irq) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.irq);
		printf("desc (%4x) (eoc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.eoc);
		printf("desc (%4x) (lop) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.loop);
		printf("desc (%4x) (mod) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.mode);
		printf("desc (%4x) (beg) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.begin);
		printf("desc (%4x) (end) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.end);
		printf("desc (%4x) (opm) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.op_mode);
		printf("desc (%4x) (enc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.enc_sha_only);
		printf("desc (%4x) (blk) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.block);
		printf("desc (%4x) (err) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.error);
		printf("desc (%4x) (own) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.owner);
#ifdef CONFIG_AML_CRYPTO_64
		printf("desc (%4x) (src) = 0x%llx\n", i,
		       dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%llx\n", i,
		       dsc[i].tgt_addr);
#else
		printf("desc (%4x) (src) = 0x%8x\n", i,
				dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%8x\n", i,
				dsc[i].tgt_addr);
#endif
	}
#endif

	mutex_unlock(&crypto_lock);
	return ret;
}

int32_t sha3_update_internal(sha3_ctx *ctx, const uint8_t *input,
		uint32_t ilen, uint8_t *digest, uint32_t digest_size, uint8_t last_update)
{
	struct dma_dsc dsc[2] = {0};
	uint32_t blocks = ilen / DMA_BLOCK_SIZE;
	uint32_t residues = ilen & (DMA_BLOCK_SIZE - 1);
	uint8_t hash_tmp[SHA3_TMP_SIZE] = {0};
	int32_t ret = 0;
#ifdef CRYPTO_DEBUG
	uint32_t i = 0;
#endif
	uint32_t op_mode = 0;
	uint32_t enc_sha_only = 0;

	ret = hw_dma_init();
	if (ret)
		return ret;

	if (blocks > MAX_BLOCK_TRANSFER) {
		printf("Err:sha too large: %d, ilen: %d\n", blocks, ilen);
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}

#ifndef NO_BLOCK_BUFFERING_FOR_SHA3
	if (!last_update && (ilen % ctx->block_size)) {
		printf("Err:sha length not block multiple\n");
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}
#endif

	if (last_update && !digest)
		return CRYPTO_ERROR_BAD_PARAMETERS;

	op_mode = ctx->mode < SHA3_SHAKE_128 ?
		ctx->mode : ctx->mode == SHA3_SHAKE_128 ?
		SHA3_OP_MODE_SHAKE_128_ABSORB : SHA3_OP_MODE_SHAKE_256_ABSORB;
	enc_sha_only = ctx->mode < SHA3_SHAKE_128 ?
		SHA3_ENC_SHA_ONLY_SHA3 : SHA3_ENC_SHA_ONLY_SHAKE;

	mutex_lock(&crypto_lock);

	if (blocks) {
		dsc[0].src_addr = (uintptr_t)input;
		dsc[0].tgt_addr = residues ? 0 : (uintptr_t)hash_tmp;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = blocks;
		dsc[0].dsc_cfg.b.eoc = residues ? 0 : 1;
		dsc[0].dsc_cfg.b.mode = DMA_MODE_SHA3;
		dsc[0].dsc_cfg.b.op_mode = op_mode;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = residues ? 0 : last_update;
		dsc[0].dsc_cfg.b.enc_sha_only = enc_sha_only;
		dsc[0].dsc_cfg.b.block = 1;
		dsc[0].dsc_cfg.b.owner = 1;

		if (residues) {
			dsc[1].src_addr = (uintptr_t)input + blocks * DMA_BLOCK_SIZE;
			dsc[1].tgt_addr = (uintptr_t)hash_tmp;
			dsc[1].dsc_cfg.d32 = 0;
			dsc[1].dsc_cfg.b.length = residues;
			dsc[1].dsc_cfg.b.eoc = 1;
			dsc[1].dsc_cfg.b.mode = DMA_MODE_SHA3;
			dsc[1].dsc_cfg.b.op_mode = op_mode;
			dsc[1].dsc_cfg.b.begin = 0;
			dsc[1].dsc_cfg.b.end = last_update;
			dsc[1].dsc_cfg.b.enc_sha_only = enc_sha_only;
			dsc[1].dsc_cfg.b.block = 0;
			dsc[1].dsc_cfg.b.owner = 1;
		}
	} else {
		dsc[0].src_addr = (uintptr_t)input;
		dsc[0].tgt_addr = (uintptr_t)hash_tmp;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = residues;
		dsc[0].dsc_cfg.b.eoc = 1;
		dsc[0].dsc_cfg.b.mode = DMA_MODE_SHA3;
		dsc[0].dsc_cfg.b.op_mode = op_mode;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = last_update;
		dsc[0].dsc_cfg.b.enc_sha_only = enc_sha_only;
		dsc[0].dsc_cfg.b.block = 0;
		dsc[0].dsc_cfg.b.owner = 1;
	}

	flush_dcache_range((uintptr_t)&dsc, (uintptr_t)&dsc + sizeof(dsc));
	flush_dcache_range((uintptr_t)input, (uintptr_t)input + ilen);
	flush_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));

	*P_DMA_STS0 = 0xf;
#ifdef CONFIG_AML_CRYPTO_64
	write_thread_reg((uintptr_t)dsc | 2);
#else
	*P_DMA_T0 = (uintptr_t)dsc | 2;
#endif
	while (*P_DMA_STS0 == 0)
		;

	invalidate_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));
	if (last_update && ctx->mode < SHA3_SHAKE_128)
		memcpy(digest, hash_tmp, ctx->digest_len > digest_size ? digest_size : ctx->digest_len);
	else
		memcpy(ctx->state, hash_tmp, sizeof(ctx->state));

	ctx->tot_len += ilen;

	if (*P_DMA_STS0 & 0x1)
		ret = CRYPTO_ERROR_BAD_PROCESS;
	else
		ret = CRYPTO_ERROR_NO_ERROR;

#ifdef CRYPTO_DEBUG
	printf("*P_DMA_STS0 = %x\n", *P_DMA_STS0);
	for (i = 0; i < sizeof(dsc) / sizeof(struct dma_dsc); i++) {
		printf("desc (%4x) (len) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.length);
		printf("desc (%4x) (irq) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.irq);
		printf("desc (%4x) (eoc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.eoc);
		printf("desc (%4x) (lop) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.loop);
		printf("desc (%4x) (mod) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.mode);
		printf("desc (%4x) (beg) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.begin);
		printf("desc (%4x) (end) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.end);
		printf("desc (%4x) (opm) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.op_mode);
		printf("desc (%4x) (enc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.enc_sha_only);
		printf("desc (%4x) (blk) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.block);
		printf("desc (%4x) (err) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.error);
		printf("desc (%4x) (own) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.owner);
#ifdef CONFIG_AML_CRYPTO_64
		printf("desc (%4x) (src) = 0x%llx\n", i,
		       dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%llx\n", i,
		       dsc[i].tgt_addr);
#else
		printf("desc (%4x) (src) = 0x%8x\n", i,
				dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%8x\n", i,
				dsc[i].tgt_addr);
#endif
	}
#endif

	mutex_unlock(&crypto_lock);
	return ret;
}

int32_t sha3_shake_squeeze_internal(sha3_ctx *ctx, uint8_t *digest, uint32_t hash_len)
{
	struct dma_dsc dsc[3] = {0};
	uint32_t blocks = hash_len / DMA_BLOCK_SIZE;
	uint32_t residues = hash_len & (DMA_BLOCK_SIZE - 1);
	uint8_t hash_tmp[SHA3_TMP_SIZE] = {0};
	int32_t ret = 0;
#ifdef CRYPTO_DEBUG
	uint32_t i = 0;
#endif
	uint32_t op_mode = 0;

	ret = hw_dma_init();
	if (ret)
		return ret;

	if (blocks > MAX_BLOCK_TRANSFER) {
		printf("Err:sha too large\n");
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}

	if (ctx->mode < SHA3_SHAKE_128) {
		printf("Err:sha3 mode: %d does not support squeeze\n", ctx->mode);
		return CRYPTO_ERROR_BAD_PARAMETERS;
	}
	op_mode = ctx->mode == SHA3_SHAKE_128 ?
		SHA3_OP_MODE_SHAKE_128_SQUEEZE : SHA3_OP_MODE_SHAKE_256_SQUEEZE;

	mutex_lock(&crypto_lock);

	if (blocks) {
		dsc[0].src_addr = 0;
		dsc[0].tgt_addr = (uintptr_t)digest;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = blocks;
		dsc[0].dsc_cfg.b.eoc = residues ? 0 : 1;
		dsc[0].dsc_cfg.b.mode = DMA_MODE_SHA3;
		dsc[0].dsc_cfg.b.op_mode = op_mode;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = SHA3_END_SHAKE_OUTPUT;
		dsc[0].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
		dsc[0].dsc_cfg.b.block = 1;
		dsc[0].dsc_cfg.b.owner = 1;

		if (residues) {
			dsc[1].src_addr = 0;
			dsc[1].tgt_addr = (uintptr_t)digest + blocks * DMA_BLOCK_SIZE;
			dsc[1].dsc_cfg.d32 = 0;
			dsc[1].dsc_cfg.b.length = residues;
			dsc[1].dsc_cfg.b.eoc = 0;
			dsc[1].dsc_cfg.b.mode = DMA_MODE_SHA3;
			dsc[1].dsc_cfg.b.op_mode = op_mode;
			dsc[1].dsc_cfg.b.begin = 0;
			dsc[1].dsc_cfg.b.end = SHA3_END_SHAKE_OUTPUT;
			dsc[1].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
			dsc[1].dsc_cfg.b.block = 0;
			dsc[1].dsc_cfg.b.owner = 1;

			dsc[2].src_addr = 0;
			dsc[2].tgt_addr = (uintptr_t)hash_tmp;
			dsc[2].dsc_cfg.d32 = 0;
			dsc[2].dsc_cfg.b.length = 0;
			dsc[2].dsc_cfg.b.eoc = 1;
			dsc[2].dsc_cfg.b.mode = DMA_MODE_SHA3;
			dsc[2].dsc_cfg.b.op_mode = op_mode;
			dsc[2].dsc_cfg.b.begin = 0;
			dsc[2].dsc_cfg.b.end = SHA3_END_SHAKE_SAVE_CTX;
			dsc[2].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
			dsc[2].dsc_cfg.b.block = 0;
			dsc[2].dsc_cfg.b.owner = 1;
		} else {
			dsc[1].src_addr = 0;
			dsc[1].tgt_addr = (uintptr_t)hash_tmp;
			dsc[1].dsc_cfg.d32 = 0;
			dsc[1].dsc_cfg.b.length = 0;
			dsc[1].dsc_cfg.b.eoc = 1;
			dsc[1].dsc_cfg.b.mode = DMA_MODE_SHA3;
			dsc[1].dsc_cfg.b.op_mode = op_mode;
			dsc[1].dsc_cfg.b.begin = 0;
			dsc[1].dsc_cfg.b.end = SHA3_END_SHAKE_SAVE_CTX;
			dsc[1].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
			dsc[1].dsc_cfg.b.block = 0;
			dsc[1].dsc_cfg.b.owner = 1;
		}
	} else {
		dsc[0].src_addr = 0;
		dsc[0].tgt_addr = (uintptr_t)digest;
		dsc[0].dsc_cfg.d32 = 0;
		dsc[0].dsc_cfg.b.length = hash_len;
		dsc[0].dsc_cfg.b.eoc = 0;
		dsc[0].dsc_cfg.b.mode = DMA_MODE_SHA3;
		dsc[0].dsc_cfg.b.op_mode = op_mode;
		dsc[0].dsc_cfg.b.begin = ctx->tot_len == 0;
		dsc[0].dsc_cfg.b.end = SHA3_END_SHAKE_OUTPUT;
		dsc[0].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
		dsc[0].dsc_cfg.b.block = 0;
		dsc[0].dsc_cfg.b.owner = 1;

		dsc[1].src_addr = 0;
		dsc[1].tgt_addr = (uintptr_t)hash_tmp;
		dsc[1].dsc_cfg.d32 = 0;
		dsc[1].dsc_cfg.b.length = 0;
		dsc[1].dsc_cfg.b.eoc = 1;
		dsc[1].dsc_cfg.b.mode = DMA_MODE_SHA3;
		dsc[1].dsc_cfg.b.op_mode = op_mode;
		dsc[1].dsc_cfg.b.begin = 0;
		dsc[1].dsc_cfg.b.end = SHA3_END_SHAKE_SAVE_CTX;
		dsc[1].dsc_cfg.b.enc_sha_only = SHA3_ENC_SHA_ONLY_SHAKE;
		dsc[1].dsc_cfg.b.block = 0;
		dsc[1].dsc_cfg.b.owner = 1;
	}

	flush_dcache_range((uintptr_t)&dsc, (uintptr_t)&dsc + sizeof(dsc));
	flush_dcache_range((uintptr_t)digest, (uintptr_t)digest + hash_len);
	flush_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));

	*P_DMA_STS0 = 0xf;
#ifdef CONFIG_AML_CRYPTO_64
	write_thread_reg((uintptr_t)dsc | 2);
#else
	*P_DMA_T0 = (uintptr_t)dsc | 2;
#endif
	while (*P_DMA_STS0 == 0)
		;

	invalidate_dcache_range((uintptr_t)digest, (uintptr_t)digest + sizeof(hash_len));
	invalidate_dcache_range((uintptr_t)hash_tmp, (uintptr_t)hash_tmp + sizeof(hash_tmp));
	memcpy(ctx->state, hash_tmp, sizeof(ctx->state));

	if (*P_DMA_STS0 & 0x1)
		ret = CRYPTO_ERROR_BAD_PROCESS;
	else
		ret = CRYPTO_ERROR_NO_ERROR;

#ifdef CRYPTO_DEBUG
	for (i = 0; i < sizeof(dsc) / sizeof(struct dma_dsc); i++) {
		printf("desc (%4x) (len) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.length);
		printf("desc (%4x) (irq) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.irq);
		printf("desc (%4x) (eoc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.eoc);
		printf("desc (%4x) (lop) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.loop);
		printf("desc (%4x) (mod) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.mode);
		printf("desc (%4x) (beg) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.begin);
		printf("desc (%4x) (end) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.end);
		printf("desc (%4x) (opm) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.op_mode);
		printf("desc (%4x) (enc) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.enc_sha_only);
		printf("desc (%4x) (blk) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.block);
		printf("desc (%4x) (err) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.error);
		printf("desc (%4x) (own) = 0x%8x\n", i,
				dsc[i].dsc_cfg.b.owner);
#ifdef CONFIG_AML_CRYPTO_64
		printf("desc (%4x) (src) = 0x%llx\n", i,
		       dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%llx\n", i,
		       dsc[i].tgt_addr);
#else
		printf("desc (%4x) (src) = 0x%8x\n", i,
				dsc[i].src_addr);
		printf("desc (%4x) (tgt) = 0x%8x\n", i,
				dsc[i].tgt_addr);
#endif
	}
#endif

	mutex_unlock(&crypto_lock);
	return ret;
}
