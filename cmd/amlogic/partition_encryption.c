// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <fdt_support.h>
#include <amlogic/aml_crypto.h>
#include <amlogic/aml_mkl.h>
#include <amlogic/store_wrapper.h>
#include <malloc.h>
#include <u-boot/md5.h>
#include <uboot_aes.h>
#include <amlogic/partition_encryption.h>
#include <linux/libfdt.h>
#include <asm/amlogic/arch/bl31_apis.h>

#define DEBUG           (0)
#define ERROR_REPORT    (1)

//Maximum number of enc partitions which kernel driver supported
#define MAX_ENC_PARTS (4)

typedef struct {
	char part_name[32];
	u8 wrapped[AES_KEY_SIZE_128];
	u8 in_used;
} ENC_PART_ENT_T;

ENC_PART_ENT_T enc_parts[MAX_ENC_PARTS];

s32 find_free_enc_parts_ent(void)
{
	s32 i = 0;

	for (i = 0; i < MAX_ENC_PARTS; i++)
		if (enc_parts[i].in_used == 0)
			return i;
	return -1;
}

#if DEBUG
void dump_bufs(const char *name, const u8 *buf, s32 len)
{
	char line_buf[128] = {0x0};
	char *line_buf_ptr = line_buf;
	u32 i = 0;
	const u8 *ptr = buf;
	const s32 sz = len;

	printf("\n========= Start dump %s(%d)\n", name, sz);
	while (len - 16 >= 0) {
		printf("%02x %02x %02x %02x %02x %02x %02x %02x",
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

void dump_enc_parts(void)
{
	s32 i, j;

	for (i = 0; i < MAX_ENC_PARTS; i++) {
		printf("%d: %s(%s)", i, enc_parts[i].part_name,
		       enc_parts[i].in_used ? "In used" : "Unused");
		printf("\n    wrapped: ");
		for (j = 0; j < sizeof(enc_parts[i].wrapped); j++)
			printf("%02x", enc_parts[i].wrapped[j]);
		printf("\n");
	}
}

static int hex2bin(char *hex, void *bin, size_t binlen)
{
	int i, c, n1, n2, hexlen, k;

	hexlen = strnlen(hex, 64);
	k = 0;
	n1 = -1;
	n2 = -1;
	for (i = 0; i < hexlen; i++) {
		n2 = n1;
		c = hex[i];
		if (c >= '0' && c <= '9') {
			n1 = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			n1 = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			n1 = c - 'A' + 10;
		} else if (c == ' ') {
			n1 = -1;
			continue;
		} else {
			return -1;
		}

		if (n1 >= 0 && n2 >= 0) {
			if (k < binlen) {
				// k is index and should be 0 <= k < binlen
				((u8 *)bin)[k] = (n2 << 4) | n1;
				n1 = -1;
				k++;
			} else {
#if ERROR_REPORT
				printf("Output overflow(%d >= %lu)\n", k, binlen);
#endif
				goto out;
			}
		}
	}
out:
	return k;
}

static int unwrap_key(u8 *in, u32 in_sz, u8 *out, u32 *out_sz)
{
	int ret = 0;
#if !CONFIG_PARTITION_ENCRYPTION_LOCAL
	struct amlkl_params kl_param;
	/* dgpk_func_0_bs_0_tsep_0.test.vector.txt */
	u8 dgpk_ek3[16] = {
		0xa6, 0x20, 0x65, 0x7f, 0xa9, 0x6b, 0x19, 0x66,
		0xa1, 0x03, 0x47, 0xb4, 0x23, 0xde, 0x20, 0x25
	};

	u8 dgpk_ek2[16] = {
		0xa2, 0xe3, 0x7a, 0xe8, 0x6e, 0xce, 0x64, 0x78,
		0x69, 0x18, 0x51, 0x0c, 0x57, 0xec, 0x68, 0xcf
	};

	u8 dgpk_ek1[16] = {
		0xfa, 0x29, 0x3b, 0x43, 0x25, 0x7c, 0x96, 0xbd,
		0x55, 0x2b, 0x60, 0xd5, 0x95, 0x60, 0xcf, 0xcc
	};
#endif
	u32 key_slot = 31; //only test slot
	u8 dst[AES_KEY_SIZE_128] = {0};

	if (!in || !out) {
#if ERROR_REPORT
		printf("Invalid arguments.\n");
#endif
		return -1;
	}
	if (!out_sz || *out_sz < AES_KEY_SIZE_128) {
#if ERROR_REPORT
		printf("output size too small\n");
#endif
		return -1;
	}

#if CONFIG_PARTITION_ENCRYPTION_LOCAL
	/* when mrk is ACRK, derive it in bl31 */
	ret = partition_enc_kl_derive_key(key_slot);
#else
	memset(&kl_param, 0, sizeof(kl_param));
	kl_param.kt_handle = key_slot;
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
	memcpy(kl_param.eks[0], dgpk_ek1, 16);
	memcpy(kl_param.eks[1], dgpk_ek2, 16);
	memcpy(kl_param.eks[2], dgpk_ek3, 16);
	ret = aml_mkl_run(&kl_param);
#endif
	if (ret != 0) {
#if ERROR_REPORT
		printf("derive key error[ret:%d]\n", ret);
#endif
		return -1;
	}
	ret = aes_ecb_dec_keytbl(AES_KEY_SIZE_128, in, dst,
				 NULL, AES_KEY_SIZE_128, key_slot);
#if DEBUG
	printf("in:\n");
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7],
	       in[8], in[9], in[10], in[11], in[12], in[13], in[14], in[15]);

	printf("unwrap:");
	printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
	       dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7],
	       dst[8], dst[9], dst[10], dst[11], dst[12], dst[13], dst[14], dst[15]);
#endif
	memcpy(out, dst, AES_KEY_SIZE_128);
	*out_sz = AES_KEY_SIZE_128;
	return ret;
}

void apply_enc_parts(void)
{
	s32 i = 0, rc;
	u8 unwrap[AES_KEY_SIZE_128] = {0};
	u32 unwrap_sz = sizeof(unwrap);
	u64 fdt_addr;

	fdt_addr = env_get_ulong("dtb_mem_addr", 16, 0);
	for (i = 0; i < MAX_ENC_PARTS; i++) {
		if (enc_parts[i].in_used != 0) {
			rc = unwrap_key(enc_parts[i].wrapped, AES_KEY_SIZE_128,
					unwrap, &unwrap_sz);
			if (rc < 0) {
#if ERROR_REPORT
				printf("unwrap key for %s failed.(%d)\n",
				       enc_parts[i].part_name, rc);
#endif
				continue;
			}
			rc = fdt_find_and_setprop((void *)fdt_addr, "/keys", enc_parts[i].part_name,
						  unwrap, unwrap_sz, 1 /* create */);
			if (rc) {
				if (rc == -FDT_ERR_NOSPACE) {
					/* no space. Try to make extra space for it */
					int prop_sz = 0x10;

					rc = fdt_shrink_to_minimum((void *)fdt_addr, prop_sz);
					if (rc < 0) {
#if ERROR_REPORT
						printf("Unable to add extra size(%d) for %s\n",
						       prop_sz, enc_parts[i].part_name);
#endif
					} else {
						rc = fdt_find_and_setprop((void *)fdt_addr, "/keys",
									  enc_parts[i].part_name,
									  unwrap, unwrap_sz,
									  1 /* create */);
						if (rc) {
#if ERROR_REPORT
							printf("Failed to set partition enc for %s(%d)\n",
							       enc_parts[i].part_name, rc);
#endif
						}
					}
				} else {
#if ERROR_REPORT
					printf("Failed to set partition enc for %s(%d)\n",
					       enc_parts[i].part_name, rc);
#endif
				}
			}
		}
	}
	return;
}

s32 find_enc_parts(const char *part_name)
{
	s32 i = 0;

	if (!part_name)
		return -1;

	for (i = 0; i < MAX_ENC_PARTS; i++) {
		if (enc_parts[i].in_used != 0) {
			if (!strcmp(part_name, enc_parts[i].part_name))
				return i;
		}
	}
	return -1;
}

int set_enc_parts(char *name, char *wrapped_hex, int wrapped_hex_sz)
{
	s32 i = 0, rc;

	i = find_enc_parts(name);
	if (i < 0) {
		i = find_free_enc_parts_ent();
		if (i < 0) {
#if ERROR_REPORT
			printf("Unable to find free slot for %s\n", name);
#endif
			return CMD_RET_FAILURE;
		}
	}
	rc = hex2bin(wrapped_hex, enc_parts[i].wrapped, sizeof(enc_parts[i].wrapped));
	if (rc < 0) {
#if ERROR_REPORT
		printf("Unable to parse data hex2bin for %s\n", wrapped_hex);
#endif
		return CMD_RET_FAILURE;
	}
	strlcpy(enc_parts[i].part_name, name, sizeof(enc_parts[i].part_name) - 1);
	enc_parts[i].in_used = 1;

	return CMD_RET_SUCCESS;
}

int del_enc_parts(char *name)
{
	if (name) {
		int i = find_enc_parts(name);

		if (i >= 0)
			memset(&enc_parts[i], 0x0, sizeof(ENC_PART_ENT_T));
		return CMD_RET_SUCCESS;
	} else {
		return CMD_RET_FAILURE;
	}
}

int clear_enc_parts(void)
{
	memset(enc_parts, 0x0, sizeof(enc_parts));
	return CMD_RET_SUCCESS;
}

int init_enc_parts(void)
{
	s32 rc = CMD_RET_SUCCESS;
#ifdef PARTITION_ENC_ARGS
	char *args = NULL;
	const u32 args_len = strlen(PARTITION_ENC_ARGS);
	char *pair[MAX_ENC_PARTS] = {NULL, NULL, NULL, NULL};
	char *tmp;
	u32 i = 0;
	char *part_name;
	char *part_key_hex;
#if DEBUG
	printf("%s:%d: args_len = %d\n", __func__, __LINE__, args_len);
#endif
	if (args_len) {
		args = calloc(args_len + 1, sizeof(char));// include EOS
		if (!args) {
#if ERROR_REPORT
			printf("failed to alloc buf for args.(%u)\n", args_len);
#endif
			return -1;
		}
		strlcpy(args, PARTITION_ENC_ARGS, args_len);
		tmp = args;

		for (i = 0; i < MAX_ENC_PARTS; i++) {
			pair[i] = strsep(&tmp, ";");

			if (!pair[i]) {
				break;
			} else if (!strlen(pair[i])) {
				// If delimiter appears at the end of string,
				// drop it and continue the parsing
				pair[i] = NULL;
				i--;
				continue;
			}
		}
		if (i == MAX_ENC_PARTS && strsep(&tmp, ";")) {
#if ERROR_REPORT
			printf("Too many enc parts provided by PARTITION_ENC_ARGS.\n");
#endif
			rc = CMD_RET_FAILURE;
			goto out;
		}
		/* Init enc_parts */
		clear_enc_parts();
		//memset(enc_parts, 0x0, sizeof(enc_parts));
		i = 0;
		while (i < MAX_ENC_PARTS && pair[i]) {
#if DEBUG
			printf("parsing pair[%s] %d ...\n", pair[i], i);
#endif
			tmp = pair[i];
			part_name = strsep(&tmp, ":");
			part_key_hex = tmp;
			if (part_name && part_key_hex) {
				rc = set_enc_parts(part_name,
						   part_key_hex,
						   strnlen(part_key_hex, 32));
			} else {
#if ERROR_REPORT
				printf("parsing error.\n");
#endif
				rc = CMD_RET_FAILURE;
				goto out;
			}
			i++;
		}
		rc = CMD_RET_SUCCESS;
	}
out:
	if (args)
		free(args);
#else
	printf("PARTITION_ENC_ARGS is not defied.\n");
	rc = CMD_RET_FAILURE;
#endif
	return rc;
}

static char partition_enc_help_text[] =
"\n"
"[set] <partition> {<value-hexdump-string>}\n"
"\t\t\tSet partition key to data.  DATA is in continuous\n"
"\t\t\t    hexdump format, e.g. aabb1122\n"
"[del] <partition>\tDelete <partition> in table of partition encryption.\n"
"[init]\t\t\tInitialize table of partition encryption.\n"
"[clear]\t\t\tClear table of partition encryption.\n"
"[dump]\t\t\tDump table of partition encryption.\n"
"[apply]\t\t\tApply table of partition encryption to DTS.\n"
"\n";

int do_partition_enc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc = CMD_RET_SUCCESS;

	if (argc < 2)
		return CMD_RET_USAGE;

	if (argc == 2) {
		if (!strcmp(argv[1], "init"))
			rc = init_enc_parts();
		else if (!strcmp(argv[1], "clear"))
			rc = clear_enc_parts();
		else if (!strcmp(argv[1], "apply"))
			apply_enc_parts();
		else if (!strcmp(argv[1], "dump"))
			dump_enc_parts();
		else
			rc = CMD_RET_USAGE;
	} else if (argc == 3) {
		if (!strcmp(argv[1], "del"))
			rc = del_enc_parts(argv[2]);
		else
			rc = CMD_RET_USAGE;
	} else if (argc == 4) {
		if (!strcmp(argv[1], "set"))
			rc = set_enc_parts(argv[2], argv[3], strnlen(argv[3], 32));
		else
			rc = CMD_RET_USAGE;
	} else {
		rc = CMD_RET_USAGE;
	}

#if ERROR_REPORT
	if (rc != CMD_RET_SUCCESS)
		printf("Invalid arguments: %s\n", argv[1]);
#endif
	return rc;
}

U_BOOT_CMD(partition_enc,	4,	0,	do_partition_enc,
        "partition encryption command", partition_enc_help_text);

#if CONFIG_PARTITION_ENCRYPTION_LOCAL

#define LBN_SIZE                (512)
#define AES_BLOCK_SIZE          (16)
#define LBN_SHIFT               (9)
#define IV_FACTOR_BASE_ADDR     (0x800)

/* Get logical block number for encryption.
 * Size of each lbn is 512 bytes. */
static inline u64 get_lbn(u64 offset)
{
	return (offset >> LBN_SHIFT) + IV_FACTOR_BASE_ADDR;
}

s32 calc_aes_128_cbc(u8 *in, u32 in_sz,
		     u8 *key, u32 key_sz,
		     u8 *iv, u32 iv_sz,
		     u8 *out, s32 *out_sz, s32 mode)
{
	s32 ret = 0;
	u8 key_exp[AES256_EXPAND_KEY_LENGTH];

	/* First we expand the key. */
	aes_expand_key(key, key_sz, key_exp);

	if (mode == DECRYPT)
		aes_cbc_decrypt_blocks(AES128_KEY_LENGTH, key_exp, iv,
				       in, out, in_sz / AES_BLOCK_LENGTH);
	else if (mode == ENCRYPT)
		aes_cbc_encrypt_blocks(AES128_KEY_LENGTH, key_exp, iv,
				       in, out, in_sz / AES_BLOCK_LENGTH);
#if ERROR_REPORT
	else
		printf("Unsupported mode\n");
#endif
	return ret;
}

static s32 get_iv(u64 offset, u8 *md5, u32 md5_sz, u8 *out, s32 *out_sz)
{
	u64 lbn = get_lbn(offset);
	u8 iv[AES_BLOCK_SIZE] = {};
	u8 iv_iv[AES_BLOCK_SIZE] = {0xff};
	s32 ret = 0;

	if (!out || !out_sz || !*out_sz) {
#if ERROR_REPORT
		printf("Invalid input.\n");
#endif
		return -1;
	}

	iv[0] = lbn & 0xFF;
	iv[1] = (lbn >> 8) & 0xFF;
	iv[2] = (lbn >> 16) & 0xFF;
	iv[3] = (lbn >> 24) & 0xFF;

	ret = calc_aes_128_cbc(iv, sizeof(iv),
			       md5, md5_sz,          /* key */
			       iv_iv, sizeof(iv_iv), /* iv  */
			       out, out_sz, 0);

	return ret;
}

u32 calc_md5(u8 *in, u32 in_sz, u8 *md5_hash, u32 *md5_hash_len)
{
	md5(in, in_sz, md5_hash);
	return 0;
}

int part_crypt(u8 *in, u64 in_sz,
		u8 *key, u32 key_sz,
		u8 *out, u64 out_sz,
		u64 offset, OP_MODE_T mode)
{
	s32 ret = 0;
	s32 remaining, read_sz, ret_val = 0;
	u8 md5[16];
	u32 md5_sz = sizeof(md5);
	u64 buf_off = 0;
	u8 page_iv[AES_BLOCK_SIZE] = {};
	s32 page_iv_sz = AES_BLOCK_SIZE;
	s32 wr_sz, wr_total = 0;
	u32 padding = 0;
	u8 *temp = NULL;
	u32 temp_sz = 0;
	u32 remain_of_blk = 0;

	if (!in_sz || !key || !key_sz || !out) {
#if ERROR_REPORT
		printf("Invalid input.\n");
#endif
		return -1;
	}
	if (offset & 0x1ff) {
#if ERROR_REPORT
		printf("off should be multiple of 512\n");
#endif
		return -1;
	}
	if (out_sz < in_sz) {
#if ERROR_REPORT
		printf("Output size should >= input size(%#llx)\n", in_sz);
#endif
		return -1;
	}

	if (mode != ENCRYPT && mode != DECRYPT) {
#if ERROR_REPORT
		printf("Invalid mode.\n");
#endif
		return -1;
	}

	remaining = in_sz;
	/* partition key hash */
	calc_md5(key, key_sz, md5, &md5_sz);
	while (remaining > 0) {
		/* Read in a lbn if it is possible */
		read_sz = (remaining >= LBN_SIZE) ? LBN_SIZE : remaining;
		remain_of_blk = read_sz & (AES_BLOCK_LENGTH - 1);
		if (remain_of_blk) {
			/* Since LBN_SIZE is multiple of AES_BLOCK_LENGTH, entering
			 * this case means it is the last read
			 */
			padding = AES_BLOCK_LENGTH - remain_of_blk;
#if DEBUG
			printf("remain_of_blk = %d, padding = %u\n", remain_of_blk, padding);
#endif
			temp_sz = read_sz + padding;
			temp = (u8 *)malloc(temp_sz);
			if (!temp) {
#if ERROR_REPORT
				printf("failed to alloc temp buffer.(%d)\n", temp_sz);
#endif
				ret = -1;
				goto out;
			}
			/* copy input to temp buffer */
			memcpy(temp, &in[buf_off], read_sz);
		}
		/* update iv */
		page_iv_sz = AES_BLOCK_SIZE;
		ret = get_iv(offset, md5, md5_sz, page_iv, &page_iv_sz);
		if (ret < 0) {
#if ERROR_REPORT
			printf("Error to get enc iv\n");
#endif
			ret_val = -1;
			goto out;
		}
		if (remain_of_blk) {
			wr_sz = temp_sz;
			ret = calc_aes_128_cbc(temp, temp_sz,   /* input */
					key, key_sz,                    /* key */
					page_iv, page_iv_sz,            /* iv */
					temp, &wr_sz, mode);            /* output */
		} else {
			wr_sz = read_sz;
			ret = calc_aes_128_cbc(&in[buf_off], read_sz,   /* input */
					key, key_sz,                            /* key */
					page_iv, page_iv_sz,                    /* iv */
					&out[buf_off], &wr_sz, mode);           /* output */
		}
		if (ret < 0) {
#if ERROR_REPORT
			printf("Error to calc aes 128 cbc\n");
#endif
			ret_val = -1;
			goto out;
		}
		if (remain_of_blk) {
			/* copy result from temp to out buffer */
			memcpy(&out[buf_off], temp, read_sz);
		}

		offset += read_sz;
		buf_off += read_sz;
		wr_total += wr_sz;
		remaining -= read_sz;
	}

out:
	if (temp)
		free(temp);

	return ret;
}

int part_dec(const char *name, u8 *in, u64 in_sz, u8 *out, u64 out_sz, u64 off)
{
	s32 rc = env_get_yesno("local_dec");

	if (rc == 1) {
		int i = find_enc_parts(name);
		u8 unwrap[AES_KEY_SIZE_128] = {0x0};
		u32 unwrap_sz = AES_KEY_SIZE_128;

		if (i < 0)
			return -1;

		printf("%s is an enc part\n", name);
		rc = unwrap_key(enc_parts[i].wrapped, sizeof(enc_parts[i].wrapped),
				unwrap, &unwrap_sz);
		if (rc) {
#if ERROR_REPORT
			printf("unwrap key failed.(%d)\n", rc);
#endif
			return rc;
		}

		return part_crypt(in, in_sz, unwrap, unwrap_sz, out, out_sz, off, DECRYPT);
	} else {
		return -1;
	}
}

#endif //CONFIG_PARTITION_ENCRYPTION_LOCAL
