// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * aml_env_encrypt.c
 *
 * Copyright (C) 2026 Amlogic, Inc. All rights reserved.
 *
 */
#include <common.h>
#include <command.h>
#include <linux/stddef.h>
#include <search.h>
#include <errno.h>
#include <malloc.h>
#include <amlogic/env_encrypt.h>
#include <linux/arm-smccc.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <u-boot/crc.h>

#define PAGE_SIZE	(1 << 12)

#define DEBUG		0

static int env_encrypt(unsigned int size, unsigned int flags)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SECURITY_ENV_ENCRYPT, size, flags, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

unsigned char env_needsave;

/* first imported from old unencrypted env,
 * after it's imported, then we encrypt and save it
 */
void check_envsave(void)
{
	if (env_needsave)
		run_command("saveenv", 1);
}

#if DEBUG
void dump_mem(void *addr, int size)
{
	unsigned int *p = addr;
	int i;

	printf("dump:%p\n", addr);
	for (i = 0; i < size / 4; i += 4)
		printf("%p: %08x %08x %08x %08x\n", p + i * 4,
			p[i + 0], p[i + 1], p[i + 2], p[i + 3]);
}
#endif

int aml_env_aes_crypt(env_t *env, const int enc)
{
	int size, off = 0, ret;
	unsigned long shmem_in, shmem_out;
	unsigned int flags = ENV_DATA_FIRST;
	unsigned int crc;

	if (env->magic != AMLOGIC_ENV_ENCRYPT_MAGIC && !enc) {
		/* if have old unencrypted env, save it will call
		 * env encrypt again, then env on storage is encrypted data
		 */
		printf("ENV magic:%x is not encrypted, may import from raw data\n",
			env->magic);
		env_needsave = 1;
		return 0;
	}

	if (env_needsave && enc) {
		/* if have old unencrypted env, save it will call
		 * env encrypt again, then env on storage is encrypted data
		 */
		printf("ENV: encrypte from raw data first time\n");
		env_needsave = 0;
	}

	shmem_in  = get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);
	shmem_out = get_sharemem_info(GET_SHARE_MEM_OUTPUT_BASE);

	if (!shmem_in || !shmem_out) {
		printf("%s, can't get shmem base\n", __func__);
		return -1;
	}

	if (enc == ENV_ENCRYPT) {
		crc = crc32(0, env->data, sizeof(env->data));
		printf("env text crc:%8x\n", crc);
		memcpy(&env->text_crc, &crc, sizeof(crc));
	}

	/* encrypt/decrypt page by page */
	while (off < ENV_SIZE - 4) {
		if (ENV_SIZE - 4 - off >= PAGE_SIZE)
			size = PAGE_SIZE;
		else
			size = ENV_SIZE - 4 - off;

		if (off + size == (ENV_SIZE - 4))
			flags |= ENV_DATA_LAST;

		memcpy((void *)shmem_in, env->iv + off, size);
	#if DEBUG
		printf("%s, off:%08x, size:%x, enc:%d, flag:%x\n", __func__, off, size, enc, flags);
		dump_mem((void *)shmem_in, 64);
	#endif
		ret = env_encrypt(size, enc | flags);
		if (ret) {
			printf("%s, env encrypt failed, off:%08x, size:%x, enc:%d, flag:%x, ret:%d\n",
			    __func__, off, size, enc, flags, ret);
			return -1;
		}
		memcpy(env->iv + off, (void *)shmem_out, size);
	#if DEBUG
		dump_mem((void *)shmem_out, 64);
	#endif
		off += size;
		flags = 0;
	}

	if (enc == ENV_DECRYPT) {
		crc = crc32(0, env->data, sizeof(env->data));
		printf("env text crc:%8x:%08x, ", crc, env->text_crc);
		if (crc != env->text_crc) {
			printf(" failed, reboot...\n");
			run_command("reset", 0);
		} else {
			printf("ok\n");
		}
	}

	env->magic = AMLOGIC_ENV_ENCRYPT_MAGIC;

	return 0;
}
