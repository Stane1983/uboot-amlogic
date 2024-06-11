/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __PARTITION_ENCRYPTION_H__
#define __PARTITION_ENCRYPTION_H__

#if 1
//typedef u32 mkl_ek128_t[4];
typedef struct {
	s32 kte;
#if 0
	s32 mrk;
	s32 func_id;
	u32 flag;
	s32 userid;
	u32 usage;
	u32 mode;
	u32 stage;
	u32 level;
	u32 tee_priv;
	u32 tee_sep;
	mkl_ek128_t ek3[4];
	mkl_ek128_t ek2[4];
	mkl_ek128_t ek1[4];
	s32 depth;
#endif
} partition_enc_kl_derive_params;
#endif

typedef enum {
    ENCRYPT = 0,
    DECRYPT = 1
} OP_MODE_T;

int part_dec(const char *name, u8 *in, u64 in_sz,
        u8 *out, u64 out_sz,
        u64 off);

s32 find_enc_parts(const char *part_name);

#endif//#ifndef __PARTITION_ENCRYPTION_H__

