/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __PARTITION_ENCRYPTION_H__
#define __PARTITION_ENCRYPTION_H__

typedef enum {
    ENCRYPT = 0,
    DECRYPT = 1
} OP_MODE_T;

int part_dec(const char *name, u8 *in, u64 in_sz,
        u8 *out, u64 out_sz,
        u64 off);

int32_t find_enc_parts(const char* part_name);

#endif//#ifndef __PARTITION_ENCRYPTION_H__

