/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020 Linaro Limited
 */

#ifndef __AES_ARMV8_CE_H
#define __AES_ARMV8_CE_H

//#include <types_ext.h>
#include <linux/types.h>

/* Prototypes for assembly functions */
u32 ce_aes_sub(u32 in);
void ce_aes_invert(void *dst, const void *src);
void ce_aes_ecb_encrypt(u8 out[], u8 const in[], u8 const rk[],
			int rounds, int blocks, int first);
void ce_aes_ecb_decrypt(u8 out[], u8 const in[], u8 const rk[],
			int rounds, int blocks, int first);
void ce_aes_cbc_encrypt(u8 out[], u8 const in[], u8 const rk[],
			int rounds, int blocks, u8 iv[]);
void ce_aes_cbc_decrypt(u8 out[], u8 const in[], u8 const rk[],
			int rounds, int blocks, u8 iv[]);
void ce_aes_ctr_encrypt(u8 out[], u8 const in[], u8 const rk[],
			int rounds, int blocks, u8 ctr[], int first);
void ce_aes_xts_encrypt(u8 out[], u8 const in[], u8 const rk1[],
			int rounds, int blocks, u8 const rk2[],
			u8 iv[]);
void ce_aes_xts_decrypt(u8 out[], u8 const in[], u8 const rk1[],
			int rounds, int blocks, u8 const rk2[],
			u8 iv[]);
void ce_aes_xor_block(u8 out[], u8 const op1[], u8 const op2[]);

#endif /*__AES_ARMV8_CE_H*/
