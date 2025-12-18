/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _MD5_H
#define _MD5_H

#include <string.h>

typedef unsigned int __u32;

#define MD5_SUM_LEN	16

struct MD5Context {
	unsigned int buf[4];
	unsigned int bits[2];
	union {
		unsigned char in[64];
		unsigned int in32[16];
	};
};

void md5init(struct MD5Context *ctx);
void md5update(struct MD5Context *ctx, unsigned char const *buf, unsigned int len);
void md5final(unsigned char digest[16], struct MD5Context *ctx);

void md5(unsigned char *input, int len, unsigned char output[16]);
void md5_wd(const unsigned char *input, unsigned int len,
	    unsigned char output[16], unsigned int chunk_sz);

#endif /* _MD5_H */
