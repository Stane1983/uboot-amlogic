/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * aml_env_encrypt.c
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */
#ifndef __AML_ENV_ENCRYPT__
#define __AML_ENV_ENCRYPT__

#include <env_internal.h>

#define AMLOGIC_ENV_ENCRYPT_MAGIC	0xb2d05e13

#define SECURITY_ENV_ENCRYPT		0x820000d0

#define ENV_DECRYPT			0
#define ENV_ENCRYPT			1
#define ENV_DATA_FIRST			0x80000000
#define ENV_DATA_LAST			0x40000000

extern unsigned char env_needsave;
int aml_env_aes_crypt(env_t *env, const int enc);
void check_envsave(void);
#endif
