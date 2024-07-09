/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _CRYPTO_INTERNAL_H
#define _CRYPTO_INTERNAL_H

#include <common.h>
#include <u-boot/sha256.h>
#include <amlogic/aml_crypto.h>

#define SHA3_TMP_SIZE (256)
#define NO_BLOCK_BUFFERING_FOR_SHA3

/* DMA operation mode */
#define CIPHER_OP_MODE_ECB 0
#define CIPHER_OP_MODE_CBC 1
#define CIPHER_OP_MODE_CTR 2

#define SHA2_OP_MODE_SHA2       0
#define SHA2_OP_MODE_HMAC_IPAD  1
#define SHA2_OP_MODE_HMAC_OPAD  2

#define SHA3_OP_MODE_SHA3_224   0
#define SHA3_OP_MODE_SHA3_256   1
#define SHA3_OP_MODE_SHA3_384   2
#define SHA3_OP_MODE_SHA3_512   3

#define SHA3_OP_MODE_SHAKE_128_ABSORB    0
#define SHA3_OP_MODE_SHAKE_256_ABSORB    1
#define SHA3_OP_MODE_SHAKE_128_SQUEEZE   2
#define SHA3_OP_MODE_SHAKE_256_SQUEEZE   3

#define CIPHER_ENC_SHA_ONLY_DECRYPT     0
#define CIPHER_ENC_SHA_ONLY_ENCRYPT     1

#define SHA2_ENC_SHA_ONLY_COPY_AND_SHA  0
#define SHA2_ENC_SHA_ONLY_SHA           1

#define SHA3_ENC_SHA_ONLY_SHA3  0
#define SHA3_ENC_SHA_ONLY_SHAKE 1

#define SHA3_END_SHAKE_SAVE_CTX 0
#define SHA3_END_SHAKE_OUTPUT   1

/* DMA mode */
#define DMA_MODE_DMA     0x0
#define DMA_MODE_KEY     0x1
#define DMA_MODE_MEMSET  0x2
#define DMA_MODE_SHA3    0x3
/* 0x4  skipped */
#define DMA_MODE_SHA1    0x5
#define DMA_MODE_SHA256  0x6
#define DMA_MODE_SHA224  0x7
#define DMA_MODE_AES128  0x8
#define DMA_MODE_AES192  0x9
#define DMA_MODE_AES256  0xa
#define DMA_MODE_S17     0xb
#define DMA_MODE_DES     0xc
/* 0xd  skipped */
#define DMA_MODE_TDES_2K 0xe
#define DMA_MODE_TDES_3K 0xf

#define DMA_BLOCK_SIZE (0x200)
#define MAX_BLOCK_TRANSFER (0x1ffff)

#define KEYTABLE_BASE (0xffffff00)
#define KEYTABLE_MAX_SLOT (0xff)

struct dma_dsc {
	union {
		uint32_t d32;
		struct {
		    unsigned length:17;
		    unsigned irq:1;
		    unsigned eoc:1;
		    unsigned loop:1;
		    unsigned mode:4;
		    unsigned begin:1;
		    unsigned end:1;
		    unsigned op_mode:2;
		    unsigned enc_sha_only:1;
		    unsigned block:1;
		    unsigned error:1;
		    unsigned owner:1;
		} b;
	} dsc_cfg;
#ifdef CONFIG_AML_CRYPTO_64
	uint64_t src_addr;
	uint64_t tgt_addr;
#else
	uint32_t src_addr;
	uint32_t tgt_addr;
#endif
} __packed;

/*
 * aes_cipher - aes cipher
 *
 * @key - key pointer, can be actually key buffer, or key table addr
 * @keylen - length of key
 * @iv - AES IV
 * @src - src pointer
 * @dst - dst pointer
 * @encrypt - encrypt or decrypt
 * @mode - DMA mode
 * @op_mode - DMA operation mode
 * @size - data length
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_cipher(void *key, uint32_t keylen, uint8_t iv[16],
		   const void *src, void *dst, uint8_t encrypt, uint8_t mode,
		   uint8_t op_mode, size_t size);

/*
 * des_tdes_cipher - des and tdes cipher
 *
 * @key - key pointer, can be actually key buffer, or key table addr
 * @keylen - length of key
 * @iv - DES/TDES IV
 * @src - src pointer
 * @dst - dst pointer
 * @encrypt - encrypt or decrypt
 * @mode - DMA mode
 * @op_mode - DMA operation mode
 * @size - data length
 * @return - on successful, 0 and negative value, otherwise.
 */

int32_t des_tdes_cipher(void *key, uint32_t keylen, uint8_t iv[8],
		   const void *src, void *dst, uint8_t encrypt, uint8_t mode,
		   uint8_t op_mode, size_t size);

/*
 * sha2_update_internal - Internal logic for SHA update
 *
 * @ctx - SHA context
 * @input - input pointer
 * @ilen - input length
 * @hash - hash buffer
 * @last_update - to finalize context
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t sha2_update_internal(sha2_ctx *ctx, const uint8_t *input,
		uint32_t ilen, uint8_t *hash, uint8_t last_update);

/*
 * sha3_update_internal - Internal logic for SHA3 update
 *
 * @ctx - SHA context
 * @input - input pointer
 * @ilen - input length
 * @digest - hash buffer
 * @digest_size - size of hash buffer
 * @last_update - to finalize context
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t sha3_update_internal(sha3_ctx *ctx, const uint8_t *input,
		uint32_t ilen, uint8_t *digest, uint32_t digest_size, uint8_t last_update);

/*
 * sha3_shake_squeeze_internal - Internal logic for SHAKE squeeze
 *
 * @ctx - SHA context
 * @hash - hash buffer
 * @hash_len - size of hash buffer
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t sha3_shake_squeeze_internal(sha3_ctx *ctx, uint8_t *hash, uint32_t hash_len);
#endif
