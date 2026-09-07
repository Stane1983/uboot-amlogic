// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/amlogic/arch/efuse.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <amlogic/aml_efuse.h>

#define CMD_EFUSE_WRITE            0
#define CMD_EFUSE_READ             1
#define CMD_EFUSE_READ_CALI        2
#define CMD_EFUSE_READ_CALI_ITEM   3
#define CMD_EFUSE_CHECK_PATTERN_ITEM   10

int cmd_efuse(int argc, char * const argv[], char *buf)
{
	int i, action = -1;
	u64 offset;
	u32 size = 0, max_size;
	char *end;
	char *s;
	int ret;

	if (strncmp(argv[1], "read", 4) == 0) {
		action = CMD_EFUSE_READ;
	} else if (strncmp(argv[1], "cali_read", 9) == 0) {
		action = CMD_EFUSE_READ_CALI;
	} else if (strncmp(argv[1], "item_read", 9) == 0) {
		action = CMD_EFUSE_READ_CALI_ITEM;
		goto efuse_action;
	} else if (strncmp(argv[1], "check", 9) == 0) {
		action = CMD_EFUSE_CHECK_PATTERN_ITEM;
		goto efuse_action;
	} else if (strncmp(argv[1], "write", 5) == 0) {
		action = CMD_EFUSE_WRITE;
	} else{
		printf("%s arg error\n", argv[1]);
		return CMD_RET_USAGE;
	}

	if (argc < 4)
		return CMD_RET_USAGE;
	/*check efuse user data max size*/
	offset = simple_strtoul(argv[2], &end, 16);
	size = simple_strtoul(argv[3], &end, 16);
	printf("%s: offset is %lld  size is  %d\n", __func__, offset, size);
	max_size = efuse_get_max();
	if (!size) {
		printf("\n error: size is zero!!!\n");
		return -1;
	}
	if (offset > max_size) {
		printf("\n error: offset is too large!!!\n");
		printf("\n offset should be less than %d!\n", max_size);
		return -1;
	}
	if (offset + size > max_size) {
		printf("\n error: offset + size is too large!!!\n");
		printf("\n offset + size should be less than %d!\n", max_size);
		return -1;
	}

efuse_action:

	/* efuse read */
	if (action == CMD_EFUSE_READ) {
		memset(buf, 0, size);
		ret = efuse_read_usr(buf, size, (loff_t *)&offset);
		if (ret == -1) {
			printf("ERROR: efuse read user data fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);
		printf("efuse read data");
		for (i = 0; i < size; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf(":%02x", buf[i]);
		}
		printf("\n");
	}
	else if (action == CMD_EFUSE_READ_CALI) {
		memset(buf, 0, size);
		ret = efuse_read_cali(buf, size, offset);
		if (ret == -1) {
			printf("ERROR: efuse read cali data fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);
		printf("efuse read cali data");
		for (i = 0; i < size; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf(":%02x", buf[i]);
		}
		printf("\n");
	}
	else if (action == CMD_EFUSE_READ_CALI_ITEM) {
		s = argv[2];
		ret = efuse_get_cali_item(s);
		if (ret < 0) {
			printf("ERROR: efuse read cali item data fail!\n");
			return -1;
		}
		printf("efuse %s cali data=0x%x\n",s,ret);
	} else if (action == CMD_EFUSE_CHECK_PATTERN_ITEM) {
		s = argv[2];
		ret = efuse_check_pattern_item(s);
		if (ret < 0) {
			printf("ERROR: efuse check pattern fail!\n");
			return -1;
		}
		printf("efuse %s %s\n", s, ret > 0 ? "has been written" : "is not write");
		return ret == 0 ? 1 : 0; //cmd return 0: written, 1: not write
	}

	/* efuse write */
	else if (action == CMD_EFUSE_WRITE) {
		if (argc < 5) {
			printf("arg count error\n");
			return CMD_RET_USAGE;
		}
		memset(buf, 0, size);

		s = argv[4];
		memcpy(buf, s, strlen(s));
		if (efuse_write_usr(buf, size, (loff_t *)&offset) < 0) {
			printf("error: efuse write fail.\n");
			return -1;
		} else {
			printf("%s written done.\n", __func__);
		}
	} else {
		/*
		 * This part of the code is necessary to prevent unknown errors
		 */
		/* coverity[dead_error_begin] */
		printf("arg error\n");
		return CMD_RET_USAGE;
	}

	return 0;
}

int do_efuse(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char buf[EFUSE_BYTES];

	memset(buf, 0, sizeof(buf));

	if (argc < 2)
		return CMD_RET_USAGE;

	return cmd_efuse(argc, argv, buf);
}

static char efuse_help_text[] =
	"[read/write offset size [data]]\n"
	"  [cali_read]  -read from cali\n"
	"                   example: [efuse cali_read 0 7]; offset is 0,size is 7. \n"
	"  [item_read]  -read cali item\n"
	"                   [item_read sensor/saradc/mipicsi/hdmirx/eth/cvbs/earcrx/earctx]\n"
	"  [check]      -check if pattern is write\n"
	"                  [check dgpk1|dgpk2|aud_id]\n"
	"  [read/write]  - read or write 'size' data from\n"
	"                  'offset' from efuse user data ;\n"
	"  [offset]      - the offset byte from the beginning\n"
	"                  of efuse user data zone;\n"
	"  [size]        - data size\n"
	"  [data]        - the optional argument for 'write',\n"
	"                  data is treated as characters\n"
	"  examples: efuse write 0xc 0xd abcdABCD1234\n";

U_BOOT_CMD(
	efuse,  5,  1,  do_efuse,
	"efuse commands", efuse_help_text
);

#ifdef CONFIG_EFUSE_OBJ_API
static char *efuse_obj_err_parse(uint32_t  efuse_obj_err_status)
{
	char *err_char = NULL;

	switch (efuse_obj_err_status) {
	case EFUSE_OBJ_ERR_INVALID_DATA:
		err_char = "invalid data";
		break;
	case EFUSE_OBJ_ERR_NOT_FOUND:
		err_char = "field not found";
		break;
	case EFUSE_OBJ_ERR_SIZE:
		err_char = "size not match";
		break;
	case EFUSE_OBJ_ERR_NOT_SUPPORT:
		err_char = "not support";
		break;
	case EFUSE_OBJ_ERR_ACCESS:
		err_char = "access denied";
		break;
	case EFUSE_OBJ_ERR_WRITE_PROTECTED:
		err_char = "write protected";
		break;
#if (IS_ENABLED(CONFIG_AMPK))
	case EFUSE_OBJ_ERR_TAG:
		err_char = "invalid encrypted data tag. check device pub key and re-encrypt";
		break;
#endif
	case EFUSE_OBJ_ERR_INTERNAL:
	case EFUSE_OBJ_ERR_OTHER_INTERNAL:
		err_char = "internal error";
		break;
	default:
		err_char = "unknown error";
		break;
		}

	return err_char;
}

int do_efuse_obj(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	char *name = NULL;

	if (argc < 3 || argc > 4) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	memset(&efuse_field, 0, sizeof(efuse_field));

	if (strcmp(argv[1], "get") == 0) {
		// $0 get field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_get_data(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				int i;

				for (i = 0; i < efuse_field.size; i++)
					printf("%02x%s", efuse_field.data[i],
						((i && i % 16 == 15) || (i == efuse_field.size - 1)
						? "\n" : " "));
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error getting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else if (strcmp(argv[1], "set") == 0) {
		// $0 set field data
		if (argc == 4) {
			name = argv[2];
			char *data = argv[3];

			rc = efuse_obj_set_data(name, data);
			if (rc == EFUSE_OBJ_SUCCESS) {
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error setting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too few arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
#if (IS_ENABLED(CONFIG_AMPK))
	} else if (strcmp(argv[1], "set_enc") == 0) {
		// $0 set field data
		if (argc == 4) {
			name = argv[2];
			char *data = argv[3];

			rc = efuse_obj_set_enc_data(name, data);
			if (rc == EFUSE_OBJ_SUCCESS) {
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error setting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too few arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
#endif /* CONFIG_AMPK */
	} else if (strcmp(argv[1], "lock") == 0) {
		// $0 lock field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_lock(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error setting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else if (strcmp(argv[1], "get_lock") == 0) {
		// $0 get_lock field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_get_lock(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				int i;

				for (i = 0; i < efuse_field.size; i++)
					printf("%02x%s", efuse_field.data[i],
						((i && i % 16 == 15) || (i == efuse_field.size - 1)
						? "\n" : " "));
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error getting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else {
		printf("Error: cmd %s not supported\n", argv[1]);
		rc = CMD_RET_USAGE;
	}

	return rc;
}

static char efuse_obj_help_text[] =
	"[set | get | lock | get_lock] <FIELD> {<field-value-hexdump-string>}\n"
	"\n"
	"get FIELD         Get field value. FIELD is the field name\n"
	"                          expected_data is an optional expected data\n"
	"set FIELD DATA    Set field to data.  DATA is in continuous\n"
	"                          hexdump format, e.g. aabb1122\n"
#if defined(CONFIG_AMPK)
	"set_enc FIELD ENC_DATA    Set field to encrypted data.  ENC_DATA is in continuous\n"
	"                          hexdump format, e.g. aabb1122\n"
#endif /* CONFIG_AMPK */
	"lock FIELD        Lock field. Program exits with status 0 if success\n"
	"get_lock FIELD    Check if field is locked.  Program exits with\n"
	"                          status 1 if locked, or 0 if unlocked\n"
	"                          expected_result is 00 or 01, an optional expected lock result\n"
	"\n";

U_BOOT_CMD(efuse_obj,	4,	0,	do_efuse_obj,
	"eFUSE object program commands", efuse_obj_help_text
);
#endif /* CONFIG_EFUSE_OBJ_API */

#ifdef CONFIG_EFUSE_MRK_GET_CHECKNUM
int do_efuse_mrk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	char name[16];
	uint32_t checknum = 0;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	memset(name, 0, sizeof(name));
	strncpy(name, argv[1], sizeof(name) - 1);
	rc = efuse_mrk_get_checknum(name, 0, &checknum);
	if (!rc) {
		printf("%s: 0x%08x\n", argv[1], checknum);
		rc = CMD_RET_SUCCESS;
	} else if (rc == EFUSE_MRK_CHECKNUM_NOT_SUPPORTED) {
		printf("MRK field %s not supported\n", argv[1]);
	} else {
		printf("get mrk checknum for %s failed, MRK field may not be written\n", argv[1]);
	}

	return rc;
}

int do_efuse_mrk_long(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	char name[16];
	uint32_t checknum[4];

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	memset(name, 0, sizeof(name));
	strncpy(name, argv[1], sizeof(name) - 1);
	rc = efuse_mrk_get_checknum(name, 1, &checknum[0]);
	if (!rc) {
		printf("%s: 0x%08x 0x%08x 0x%08x 0x%08x\n",
			argv[1], checknum[0], checknum[1], checknum[2], checknum[3]);
		rc = CMD_RET_SUCCESS;
	} else if (rc == EFUSE_MRK_CHECKNUM_NOT_SUPPORTED) {
		printf("MRK field %s not supported\n", argv[1]);
	} else {
		printf("get mrk checknum for %s failed, MRK field may not be written\n", argv[1]);
	}

	return rc;
}

static char efuse_mrk_help_text[] =
	"<MRK>\n"
	"Supported MRK includes: DVGK, DVUK,\n"
	"DGPK1, DGPK2,\n"
	"ETSI_SCK_0, ETSI_SCK_1, ETSI_SCK_2,\n"
	"SCPU_ETSI_SCK_0, SCPU_ETSI_SCK_1, SEGK\n";

U_BOOT_CMD(efuse_mrk,	2,	0,	do_efuse_mrk,
	"eFUSE mrk checknum", efuse_mrk_help_text
);

U_BOOT_CMD(efuse_mrk_long,	2,	0,	do_efuse_mrk_long,
	"eFUSE mrk long checknum", efuse_mrk_help_text
);
#endif /* CONFIG_EFUSE_MRK_GET_CHECKNUM */

#ifdef CONFIG_EFUSE_OBJ_API
/*
 * Command to burn efuse_obj set/set_enc list from a file or memory
 * 1> load to mem if dev not mem
 */
#define debugP(fmt...) //printf("[DbgObj]L%d:", __LINE__),printf(fmt)
#define errorP(fmt...) do {pr_err("ERR[obj]L%d:", __LINE__); pr_err(fmt); } while (0)
#define wrnP(fmt...)   pr_warn("WRN[obj]" fmt)
#define MsgP(fmt...)   pr_info("INF[obj]" fmt)
extern efuse_obj_field_t efuse_field;

/*
 * detect if pattern contains more than one space, empty line or win new line
 */
static bool has_format_issues(const char *str, const int nsz, int *needLock, int *locked)
{
	bool last_was_space = false;
	bool last_was_newline = false;
	bool only_spaces_in_line = true;
	const char *p = str;
	const char *line_head = "efuse_obj ";
	const char *line_lock = "efuse_obj lock ";
	const int head_sz = 10;
	const int lock_sz = 15;
	int i = 0;
	int fields_need_lock = 0;
	int fields_is_locked = 0;

	if (str[nsz - 1] != '\n') {
		errorP("need ends with newline\n");
		return true;
	}
	if (str[nsz - 2] == '\r') {
		errorP("windows new line!!\n");
		return true;
	}
	if (memcmp(str, line_head, head_sz)) {
		errorP("Err pattern header\n");
		return true;
	}

	for (; *p && i < nsz; ++p, ++i) {
		// Detect Windows newline \r\n
		if (*p == '\r' && *(p + 1) == '\n') {
			errorP("windows new line!!\n");
			return true;
		}

		// Detect multiple spaces
		if (*p == ' ') {
			if (last_was_space) {
				errorP("has dup space\n");
				return true;
			}
			last_was_space = true;
		} else {
			last_was_space = false;
		}

		// Detect multiple newlines
		if (*p == '\n') {
			if (last_was_newline) {
				errorP("has empty line\n");
				return true;
			}
			last_was_newline = true;
			if (i + head_sz < nsz) {
				if (memcmp(p + 1, line_head, head_sz)) {
					errorP("Err line head\n");
					return true;
				}
				if (!memcmp(p + 1, line_lock, lock_sz)) {
					char tmpWord[32];
					const char *ps = p + 1 + lock_sz;
					char *pt = tmpWord;

					for (; *ps != '\n'; ++pt, ++ps)
						*pt = *ps;
					*pt = '\0';
					env_set("_objx_n", tmpWord);
					if (run_command("efuse_obj get_lock ${_objx_n}", 0)) {
						errorP("Fail in get state of %s\n", tmpWord);
						return CMD_RET_FAILURE;
					}
					fields_is_locked += efuse_field.data[0];
					MsgP("%s get_lock %02x\n", tmpWord, efuse_field.data[0]);
					fields_need_lock += 1;
				}
			}

			if (only_spaces_in_line) {
				errorP("space only line\n");
				return true; // Line contains only spaces
			}

			only_spaces_in_line = true; // Reset for next line
		} else {
			last_was_newline = false;
			if (*p != ' ')
				only_spaces_in_line = false;
		}
	}

	MsgP("Lock need %d, locked %d\n", fields_need_lock, fields_is_locked);
	//TODO, when partial maybe need allow only 2 HASH locked
	if ((fields_need_lock && fields_is_locked) && fields_need_lock != fields_is_locked) {
		errorP("Some fields locked but not all, not allowed\n");
		return true;
	}

	*needLock = fields_need_lock;
	*locked   = fields_is_locked;
	return false;
}

/*
 * 1> load to mem if interface not 'mem'
 * 2> detect pattern valid
 * 3> assert all 'efuse_obj lock ' are all 01 or all 00
 * 4> burn 2 HASH first if has set_enc
 * 5> run each line except HASH
 */
static int do_efuse_obj_file_burn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *inf = argv[1];
	unsigned long mem_addr = env_get_hex("loadaddr", 0x1080000);
	unsigned int memsz = 0;
	int inf_len = 0;
	int has_enc = 0;
	const char *str_pattern = NULL;
	const char *ampkHash2[] = {"HASH_NORMAL_DEVICE_ROOTCERT", "HASH_DFU_DEVICE_ROOTCERT"};
	int ampk2hashlocks = 0;
	int i = 0, ret = 0;
	int _need_lock = 0, _is_locked = 0;

	if (argc < 3) {
		errorP("too few args\n");
		return CMD_RET_USAGE;
	}
	inf_len = strnlen(inf, 8);
	if (!memcmp(inf, "mem", inf_len + 1)) {
		mem_addr = hextoul(argv[2], NULL);
		if (argc < 4) {
			errorP("Need specify mem size\n");
			return CMD_RET_USAGE;
		}
		memsz = simple_strtoul(argv[3], NULL, 0);

	} else if (!memcmp(inf, "usb", inf_len + 1)) {
		run_command("usb start", 0);
		env_set("_objinf", "usb");
	} else if (!memcmp(inf, "sdc", inf_len + 1)) {
		run_command("mmcinfo", 0);
		env_set("_objinf", "mmc");
	} else {
		errorP("Error interface %s\n", inf);
		return CMD_RET_USAGE;
	}

	if (memcmp(inf, "mem", inf_len + 1)) {
		env_set_hex("_objLoadAddr", mem_addr);
		env_set("_objPath", argv[2]);
		run_command("echo fatload ${_objinf} 0 ${_objLoadAddr} ${_objPath}", 0);
		ret = run_command("fatload ${_objinf} 0 ${_objLoadAddr} ${_objPath}", 0);
		if (ret) {
			errorP("Fail load object file from fat32 disk\n");
			return CMD_RET_FAILURE;
		}
		memsz = env_get_hex("filesize", 0);
	}

	str_pattern = (const char *)mem_addr;
	if (has_format_issues(str_pattern, memsz, &_need_lock, &_is_locked)) {
		errorP("pattern has format issue\n");
		return CMD_RET_FAILURE;
	}
	if (_need_lock && (_need_lock == _is_locked)) {
		MsgP("All fields locked,just skip\n");
		return CMD_RET_SUCCESS;
	}

	has_enc = (strstr(str_pattern, " set_enc ") != NULL);
	if (has_enc) {//check 'HASH_NORMAL_DEVICE_ROOTCERT', 'HASH_DFU_DEVICE_ROOTCERT'
		MsgP("check hash same states\n");//2 01 or 2 00
		for (i = 0; i < 2; ++i) {
			env_set("_objx_n", ampkHash2[i]);
			if (run_command("efuse_obj get_lock ${_objx_n}", 0)) {
				errorP("Fail in get state of %s\n", ampkHash2[i]);
				return CMD_RET_FAILURE;
			}
			ampk2hashlocks += efuse_field.data[0];
			MsgP("%s get_lock %02x\n", ampkHash2[i], efuse_field.data[0]);
		}
		if (ampk2hashlocks && ampk2hashlocks != 2) {
			errorP("hash partital lock!\n");
			return CMD_RET_FAILURE;
		}

		if (!ampk2hashlocks) {
			for (i = 0; i < 2; ++i) {
				char *curHash = strstr(str_pattern, ampkHash2[i]);
				int hashLen = 0;
				char *pNewLine = NULL;
				const int max_l = 96;
				char tmpWord[max_l];

				if (!curHash) {
					errorP("Has set_enc but no %s\n", ampkHash2[i]);
					return CMD_RET_FAILURE;
				}
				pNewLine = strstr(curHash, "\n");
				if (!pNewLine) {
					errorP("fail find new line after %s\n", ampkHash2[i]);
					return CMD_RET_FAILURE;
				}
				hashLen = pNewLine - curHash;
				if (hashLen + 1 > max_l) {
					errorP("err hash val\n");
					return CMD_RET_FAILURE;
				}
				memcpy(tmpWord, curHash, hashLen);
				tmpWord[hashLen] = '\0';
				env_set("_objx_n", tmpWord);
				run_command("echo efuse_obj set ${_objx_n}", 0);
				run_command("efuse_obj set ${_objx_n}", 0);
			}
			ret = run_command("ampk regen_enc_key", 0);
			if (ret != 1) {
				errorP("Fail in regen ampk enc key\n");
				return CMD_RET_FAILURE;
			}
		}
	}

	//5
	if (1) {
		const char *line = NULL;
		char *reset = (char *)str_pattern;
		const char *hashLine = "HASH_";

		while ((line = strsep(&reset, "\n")) != NULL) {
			env_set("_setcmd", line);
			run_command("echo ${_setcmd}", 0);
			if (has_enc) {
				if (strstr(line, hashLine)) {
					MsgP("skip set HASH\n");
					continue;
				}
			}
			ret = run_command("${_setcmd}", 0);
			if (ret != CMD_RET_SUCCESS) {
				errorP("Fail in obj cmd\n");
				return CMD_RET_FAILURE;
			}
		}
		printf("efuse obj file burn OKAY\n");
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD_COMPLETE(obj_file_burn,       //command name
	4,  //maxargs
	0,                   //repeatable
	do_efuse_obj_file_burn,    //command function
	"cmd for burn efuse object file",           //usage
	"    argv: obj_file_burn <interface> <file/addr> [<size>]\n"
	"    usb/sdc is FAT32 usb/sdc disk\n"
	"	<interface> mem/usb/sdc\n"
	"	<file/addr> must addr if interface mem, other file path\n"
	"	<size> only need sz when mem\n"
	"    - e.g.\n"
	"        obj_file_burn sdc pattern.obj.t950d5\n",
	var_complete
);

#endif//#ifdef CONFIG_EFUSE_OBJ_API

#ifdef CONFIG_EFUSE_AUDIO_ID_CHECKNUM
int do_efuse_audio_id_check(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int rc, i;
	char buff[512] = {0};

	rc = efuse_audio_id_get_checknum(buff, sizeof(buff));
	if (rc < 0) {
		printf("get audio id checknum failed:%d\n", rc);
		return rc;
	}
	for (i = 0; i < sizeof(buff); i++) {
		printf("%02x", buff[i]);
	}
	printf("\n");

	return rc;
}

static char efuse_audio_id_check_help_text[] =
	"<efuse_audio_id_check>\n"
	"  printf checksum for efuse audio id, which can help to check if audio id is burned correctly\n";

U_BOOT_CMD(efuse_audio_id_check, 2, 0, do_efuse_audio_id_check,
	"eFUSE audio id checknum", efuse_audio_id_check_help_text
);
#endif /* CONFIG_EFUSE_MRK_GET_CHECKNUM */
