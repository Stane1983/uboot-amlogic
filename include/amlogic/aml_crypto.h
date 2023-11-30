/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_CRYPTO_H
#define _AML_CRYPTO_H

#include <u-boot/sha256.h>

#define CRYPTO_ERROR_NO_ERROR       (0)
#define CRYPTO_ERROR_BAD_PARAMETERS (-1)
#define CRYPTO_ERROR_BAD_PROCESS    (-2)

#define AES_KEY_SIZE_128         16
#define AES_KEY_SIZE_192         24
#define AES_KEY_SIZE_256         32

#define DES_KEY_SIZE            8
#define TDES_2K_KEY_SIZE        16
#define TDES_3K_KEY_SIZE        24

#define SHA224_DIGEST_SIZE  28
#define SHA256_DIGEST_SIZE  32
#define SHA384_DIGEST_SIZE  38
#define SHA512_DIGEST_SIZE  64
#define SHA256_BLOCK_SIZE   64
#define SHA224_BLOCK_SIZE   64

#define SHA3_224_DIGEST_SIZE  28
#define SHA3_256_DIGEST_SIZE  32
#define SHA3_384_DIGEST_SIZE  48
#define SHA3_512_DIGEST_SIZE  64

#define SHA3_MAX_BLOCK_SIZE (168)
#define SHA3_STATE_SIZE (201)

#define SHA3_224_RATE (144)
#define SHA3_256_RATE (136)
#define SHA3_384_RATE (104)
#define SHA3_512_RATE (72)
#define SHAKE_128_RATE (168)
#define SHAKE_256_RATE (136)

enum SHA3_type {
	SHA3_224 = 0,
	SHA3_256 = 1,
	SHA3_384 = 2,
	SHA3_512 = 3,
	SHA3_SHAKE_128 = 4,
	SHA3_SHAKE_256 = 5,

	SHA3_MAX
};

/* SHA2 context */
typedef struct {
	uint32_t tot_len;
	uint32_t digest_len;
	uint32_t buf_len;
	uint8_t buf[SHA256_BLOCK_SIZE];
	uint8_t state[SHA256_DIGEST_SIZE + 16];
} sha2_ctx;

/* SHA3 context */
typedef struct {
	uint32_t mode;
	uint32_t tot_len;
	uint32_t digest_len;
	uint32_t block_size;
	uint32_t buf_len;
	uint8_t buf[SHA3_MAX_BLOCK_SIZE];
	uint8_t state[SHA3_STATE_SIZE];
	uint8_t rsv[7];
} sha3_ctx;

/*
 * aes_ecb_enc_keytbl - AES ECB encrypt with keytable
 *
 * @input - input pointer
 * @output - output pointer
 * @nbytes - data length
 * @key_slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_ecb_enc_keytbl(uint32_t keylen, const void *input, void *output,
		uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * aes_ecb_dec_keytbl - AES ECB decrypt with keytable
 *
 * @input - input pointer
 * @output - output pointer
 * @nbytes - data length
 * @key_slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_ecb_dec_keytbl(uint32_t keylen, const void *input, void *output,
		uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * aes_cbc_enc_keytbl - AES CBC encrypt with keytable
 *
 * @input - input pointer
 * @output - output pointer
 * @iv - AES IV
 * @nbytes - data length
 * @key_slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_cbc_enc_keytbl(uint32_t keylen, const void *input, void *output,
		uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * aes256cbc_dec_keytbl - AES CBC decrypt with keytable
 *
 * @input - input pointer
 * @output - output pointer
 * @iv - AES IV
 * @nbytes - data length
 * @key_slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes256cbc_dec_keytbl(const void *input, void *output, uint8_t iv[16],
		size_t nbytes, uint32_t key_slot);

/*
 * aes_cbc_dec_keytbl - AES CBC decrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - AES IV
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_cbc_dec_keytbl(uint32_t keylen, const void *input, void *output, uint8_t iv[16],
		size_t nbytes, uint32_t slot);
/*
 * aes_ctr_encrypt_keytbl - AES CTR encrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - AES initial counter
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_ctr_encrypt_keytbl(uint32_t keylen, const void *input, void *output,
		uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * aes_ctr_decrypt_keytbl - AES CTR decrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - AES initial counter
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes_ctr_decrypt_keytbl(uint32_t keylen, const void *input, void *output,
		uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * aes256cbc_iv0_dec_keytbl - AES CBC decrypt with keytable and iv0
 *
 * @input - input pointer
 * @output - output pointer
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aes256cbc_iv0_dec_keytbl(const void *input, void *output, size_t nbytes,
		uint32_t slot);

/*
 * des_tdes_ecb_enc_keytbl - DES/TDES ECB encrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - TDES IV
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t des_tdes_ecb_enc_keytbl(uint32_t keylen, const void *input,
		void *output, size_t nbytes, uint32_t slot);
/*
 * des_tdes_ecb_dec_keytbl - DES/TDES ECB decrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - TDES IV
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t des_tdes_ecb_dec_keytbl(uint32_t keylen, const void *input,
		void *output, uint8_t iv[16], size_t nbytes, uint32_t slot);
/*
 * des_tdes_cbc_enc_keytbl - DES/TDES CBC encrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - TDES IV
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t des_tdes_cbc_enc_keytbl(uint32_t keylen, const void *input,
		void *output, uint8_t iv[8], size_t nbytes, uint32_t slot);
/*
 * des_tdes_cbc_dec_keytbl - DES/TDES CBC decrypt with keytable
 *
 * @keylen - key length
 * @input - input pointer
 * @output - output pointer
 * @iv - TDES IV
 * @nbytes - data length
 * @slot - key table slot to be used
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t des_tdes_cbc_dec_keytbl(uint32_t keylen, const void *input,
		void *output, uint8_t iv[8], size_t nbytes, uint32_t slot);

/*
 * aml_hw_sha2_init - HW SHA2 Init
 *
 * @ctx - SHA2 context
 * @is224 - SHA224 or SHA256
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha2_init(sha2_ctx *ctx, uint32_t is224);
/*
 * aml_hw_sha2_update - HW SHA Update
 *
 * @ctx - SHA context
 * @input - input pointer
 * @ilen - input size
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha2_update(sha2_ctx *ctx, const uint8_t *input, uint32_t ilen);
/*
 * aml_hw_sha2_update - HW SHA Update
 *
 * @ctx - SHA context
 * @input - input pointer
 * @ilen - input size
 * @hash - output hash
 * @last_update - to finalize SHA context
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha2_final(sha2_ctx *ctx, uint8_t *hash);

/*
 * aml_hw_sha3_init - HW SHA3 Init
 *
 * @ctx - SHA context
 * @mode - SHA3-224/256/384/512/SHAKE-128/SHAKE-256
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha3_init(sha3_ctx *ctx, uint32_t mode);

/*
 * aml_hw_sha3_update - HW SHA3 Update
 *
 * @ctx - SHA context
 * @input - input pointer
 * @ilen - input size
 * @hash - output hash.  There is no hash out when using SHAKE,
 *                       use aml_hw_sha3_shake_squeeze instead.
 * @last_update - to finalize SHA context
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha3_update(sha3_ctx *ctx, const uint8_t *input, uint32_t ilen);

/*
 * aml_hw_sha3_final - HW SHA3 finalize
 *
 * @ctx - SHA context
 * @hash - output hash
 * @digest_len - length of hash to copy, for SHAKE, it is squeeze size
 * @return - on successful, 0 and negative value, otherwise.
 */
int32_t aml_hw_sha3_final(sha3_ctx *ctx, uint8_t *hash, uint32_t digest_size);
#endif
