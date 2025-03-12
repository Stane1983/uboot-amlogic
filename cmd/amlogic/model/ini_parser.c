// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include "ini_parser.h"

#define LOG_TAG     "ini_parser"
#define LOG_NDEBUG  0

static int ini_mem_alloc_size;

static void ini_memnd_alloc_recode(const char *fun_name, const char *var_name, int size, int type)
{
	if (size == 0)
		return;
	ini_mem_alloc_size += size;
#if INI_PARSER_MEM_TRACE == 1
	ALOGD("[mem]: size: %s%d, total size: %d: %s: %s%s%s\n",
		size > 0 ? "+" : "", size, ini_mem_alloc_size, fun_name,
		(type == INI_MEM_ALL) ? "#" : ((type == INI_MEM_SECTION) ? "[" : ""),
		var_name,
		(type == INI_MEM_ALL) ? "#" : ((type == INI_MEM_SECTION) ? "]" : ""));
#endif
}

static void ini_alloc_parse_mem_print(void)
{
	ALOGD("ini parse mem alloc size: %d\n\n", ini_mem_alloc_size);
}

static char *pos_to_str(struct ini_s *ini_buf, unsigned int pos)
{
	unsigned char * local_ini_mem = (unsigned char *)ini_buf;

	if (pos == 0 || pos > ini_buf->mem_size)
		return NULL;

	return (char *)(local_ini_mem + ini_buf->mem_start_pos + pos);
}

static struct ini_line_s *pos_to_line(struct ini_s *ini_buf, unsigned int pos)
{
	unsigned char * local_ini_mem = (unsigned char *)ini_buf;

	if (pos == 0 || pos > ini_buf->mem_size)
		return NULL;

	return (struct ini_line_s *)(local_ini_mem + ini_buf->mem_start_pos + pos);
}

static int line_to_pos(struct ini_s *ini_buf, struct ini_line_s *pline)
{
	unsigned char *p;
	unsigned char * local_ini_mem = (unsigned char *)ini_buf;

	if (!ini_buf || !pline)
		return 0;

	p = (unsigned char *)pline;
	return (p - local_ini_mem - ini_buf->mem_start_pos);
}

static struct ini_section_s *pos_to_section(struct ini_s *ini_buf, unsigned int pos)
{
	unsigned char * local_ini_mem = (unsigned char *)ini_buf;

	if (pos == 0 || pos > ini_buf->mem_size)
		return NULL;

	return (struct ini_section_s *)(local_ini_mem + ini_buf->mem_start_pos + pos);
}

static int section_to_pos(struct ini_s *ini_buf, struct ini_section_s *psec)
{
	unsigned char *p;
	unsigned char * local_ini_mem = (unsigned char *)ini_buf;

	if (!ini_buf || !psec)
		return 0;

	p = (unsigned char *)psec;
	return (p - local_ini_mem - ini_buf->mem_start_pos);
}

/* Strip whitespace chars off end of given string, in place. Return s */
static char *rstrip(char *s)
{
	char *p = s + strlen(s);

	while (p > s && isspace((unsigned char)(*--p)))
		*p = '\0';
	return s;
}

/* Return pointer to first non-whitespace char in given string */
static char *lskip(const char *s)
{
	while (*s && isspace((unsigned char)(*s)))
		s++;
	return (char *)s;
}

/* Return pointer to first char c or ';' comment in given string, or pointer to
 *null at end of string if neither found. ';' must be prefixed by a whitespace
 *character to register as a comment
 */
static char *find_char_or_comment(const char *s, char c)
{
	int was_whitespace = 0;

	while (*s && *s != c && !(was_whitespace && (*s == ';' || *s == '#'))) {
		was_whitespace = isspace((unsigned char)(*s));
		s++;
	}
	return (char *)s;
}

struct ini_section_s *ini_get_section(struct ini_s *ini_buf, const char *section)
{
	struct ini_section_s *psec;
	char *name;

	if (!ini_buf || !section)
		return NULL;

	psec = pos_to_section(ini_buf, ini_buf->cur_section);
	if (psec) {
		name = pos_to_str(ini_buf, psec->name_pos);
		if (name) {
			if (strcmp(name, section) == 0)
				return psec;
		}
	}

	psec = pos_to_section(ini_buf, ini_buf->first_section);
	while (psec) {
		name = pos_to_str(ini_buf, psec->name_pos);
		if (name) {
			if (strcmp(name, section) == 0)
				return psec;
		}
		psec = pos_to_section(ini_buf, psec->next);
	}
	return NULL;
}

struct ini_line_s *ini_get_key_line_at_sec(struct ini_s *ini_buf,
			struct ini_section_s *psec, const char *key)
{
	struct ini_line_s *pline;
	char *name;

	if (!ini_buf || !psec || !key)
		return NULL;

	pline = pos_to_line(ini_buf, psec->cur_line);
	if (pline) {
		name = pos_to_str(ini_buf, pline->name_pos);
		if (name) {
			if (strcmp(name, key) == 0)
				return pline;
		}
	}

	pline = pos_to_line(ini_buf, psec->first_line);
	while (pline) {
		name = pos_to_str(ini_buf, pline->name_pos);
		if (name) {
			if (strcmp(name, key) == 0)
				return pline;
		}
		pline = pos_to_line(ini_buf, pline->next);
	}
	return NULL;
}

int ini_set_line_exist_key_val(struct ini_s *ini_buf, struct ini_section_s *psec,
			struct ini_line_s *pline, const char *new_value, unsigned int set_mode)
{
	struct ini_s *tmp_inip;
	struct ini_section_s *tmp_psec, *pre_psec;
	struct ini_line_s *tmp_pline, *pre_pline;
	char *tmp_value, *tmp_name;
	unsigned char *pre_mem, *tmp_mem;
	int pre_len, new_len, offset;
	int tmp_size, new_pos;
	int ret = 0, err_no = 0;
	unsigned char *local_ini_mem = (unsigned char *)ini_buf;

	if (!ini_buf || !psec || !pline || !new_value)
		return -1;

	tmp_mem = malloc(ini_buf->total_size);
	if (!tmp_mem) {
		ALOGE("%s: tmp_mem error\n", __func__);
		return -1;
	}
	memset(tmp_mem, 0, ini_buf->total_size);
	tmp_inip = (struct ini_s *)tmp_mem;
	pre_mem = (void *)ini_buf;
	memcpy(tmp_mem, pre_mem, sizeof(struct ini_s));

	tmp_name = pos_to_str(ini_buf, pline->name_pos);
	tmp_value = pos_to_str(ini_buf, pline->value_pos);
	pre_len = strlen(tmp_value) + 1; //include '\0
	new_len = strlen(new_value) + 1; //include '\0

	offset = section_to_pos(ini_buf, psec);
	tmp_psec = pos_to_section(tmp_inip, offset);
	if (!tmp_psec) {
		err_no = __LINE__;
		ret = -1;
		goto ini_set_line_exist_key_val_end;
	}

	//save front sections & lines
	new_pos = line_to_pos(ini_buf, pline);
	if (new_pos == 0) {
		err_no = __LINE__;
		ret = -1;
		goto ini_set_line_exist_key_val_end;
	}
	memcpy(tmp_mem + tmp_inip->mem_start_pos, local_ini_mem + ini_buf->mem_start_pos, new_pos);
	tmp_inip->mem_cur_pos = new_pos;

	if (set_mode == INI_SET_KEY_VAL_MODE_OVERWRITE) { //overwrite new_value
		tmp_size = pline->local_size - pre_len; //line size without value_str
		if ((new_pos + tmp_size + new_len) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		//save line struct & name_str
		tmp_pline = pos_to_line(tmp_inip, new_pos);
		if (!tmp_pline) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		memcpy(tmp_pline, pline, tmp_size);
		new_pos += tmp_size;
		//save new value_str
		tmp_value = pos_to_str(tmp_inip, tmp_pline->value_pos);
		if (!tmp_value) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		strlcpy(tmp_value, new_value, new_len);
		//update line attr
		tmp_pline->local_size = tmp_size + new_len;
		//update cur_pos
		new_pos += new_len;
		tmp_inip->mem_cur_pos = new_pos;

		//global offset after active_line, signed value
		offset = new_len - pre_len;
		ini_memnd_alloc_recode(__func__, tmp_name, offset, INI_MEM_KEY);
		ALOGI("%s: %s, overwrite\n", __func__, tmp_name);
	} else { //append new_value
		tmp_size = pline->local_size;
		if ((new_pos + tmp_size) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		//save line struct & name_str & pre_value_str
		tmp_pline = pos_to_line(tmp_inip, new_pos);
		if (!tmp_pline) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		memcpy(tmp_pline, pline, pline->local_size);
		//save new value_str
		tmp_value = pos_to_str(tmp_inip, tmp_pline->value_pos);
		if (!tmp_value) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		strlcpy((tmp_value + pre_len - 1), new_value, new_len);
		//update line attr
		tmp_pline->local_size = tmp_size - 1 + new_len; //remove pre_value '\0'
		//update cur_pos
		new_pos += tmp_size;
		tmp_inip->mem_cur_pos = new_pos;

		//global offset behind active_line, signed value.
		offset = new_len - 1; //need remove '\0'
		ini_memnd_alloc_recode(__func__, tmp_name, offset, INI_MEM_KEY);
		ALOGI("%s: %s, append\n", __func__, tmp_name);
	}

	//save behind lines in same section
	pre_pline = pos_to_line(ini_buf, pline->next);
	while (pre_pline) {
		tmp_pline->next = new_pos;
		tmp_pline = pos_to_line(tmp_inip, new_pos);
		if (!tmp_pline) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		tmp_size = pre_pline->local_size;
		if ((new_pos + tmp_size) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		memcpy((void *)tmp_pline, pre_pline, tmp_size);
		tmp_pline->name_pos += offset;
		tmp_pline->value_pos += offset;

		new_pos += tmp_size;
		tmp_inip->mem_cur_pos = new_pos;

		pre_pline = pos_to_line(ini_buf, pre_pline->next);
	}

	//save behind sections & lines
	//need trave lines to update each name_pos and value_pos
	pre_psec = pos_to_section(ini_buf, psec->next);
	while (pre_psec) {
		tmp_psec->next = new_pos;
		tmp_psec = pos_to_section(tmp_inip, new_pos);
		if (!tmp_psec) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		tmp_size = pre_psec->local_size;
		if ((new_pos + tmp_size) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_key_val_end;
		}
		memcpy((void *)tmp_psec, pre_psec, tmp_size);
		tmp_psec->name_pos += offset;

		new_pos += tmp_size;
		tmp_inip->mem_cur_pos = new_pos;

		tmp_pline = NULL;
		pre_pline = pos_to_line(ini_buf, pre_psec->first_line);
		while (pre_pline) {
			if (!tmp_pline)
				tmp_psec->first_line = new_pos;
			else
				tmp_pline->next = new_pos;
			tmp_pline = pos_to_line(tmp_inip, new_pos);
			if (!tmp_pline) {
				err_no = __LINE__;
				ret = -1;
				goto ini_set_line_exist_key_val_end;
			}
			tmp_size = pre_pline->local_size;
			if ((new_pos + tmp_size) > tmp_inip->mem_size) {
				err_no = __LINE__;
				ret = -1;
				goto ini_set_line_exist_key_val_end;
			}
			memcpy((void *)tmp_pline, pre_pline, tmp_size);
			tmp_pline->name_pos += offset;
			tmp_pline->value_pos += offset;

			new_pos += tmp_size;
			tmp_inip->mem_cur_pos = new_pos;

			pre_pline = pos_to_line(ini_buf, pre_pline->next);
		}

		pre_psec = pos_to_section(ini_buf, pre_psec->next);
	}

	//restore cur_mem
	memcpy(local_ini_mem + ini_buf->mem_start_pos, tmp_mem + tmp_inip->mem_start_pos, ini_buf->mem_size);
	ini_buf->mem_cur_pos = tmp_inip->mem_cur_pos;

ini_set_line_exist_key_val_end:
	memset(tmp_mem, 0, ini_buf->total_size);
	free(tmp_mem);
	if (ret)
		ALOGE("%s: %d\n", __func__, err_no);

	return ret;
}

int ini_set_line_exist_keys(struct ini_s *ini_buf, struct ini_section_s *psec,
			const char *new_line, const char *new_value, int line_cnt)
{
	struct ini_s *tmp_inip;
	struct ini_section_s *tmp_psec, *pre_psec;
	struct ini_line_s *tmp_pline, *pre_pline;
	char *tmp_value;
	const char *pre_name, *pre_value, *token_line, *token_value;
	unsigned char *tmp_mem;
	int offset, tmp_size, new_pos;
	int i, j, ret = 0, err_no = 0;
	unsigned char *local_ini_mem = (unsigned char *)ini_buf;

	if (!ini_buf || !psec || !new_line || !new_value)
		return -1;

	tmp_mem = malloc(ini_buf->total_size);
	if (!tmp_mem) {
		ALOGE("%s: tmp_mem error\n", __func__);
		return -1;
	}
	memset(tmp_mem, 0, ini_buf->total_size);
	tmp_inip = (struct ini_s *)tmp_mem;
	memcpy(tmp_mem, (void *)ini_buf, sizeof(struct ini_s));

	new_pos = section_to_pos(ini_buf, psec);
	tmp_psec = pos_to_section(tmp_inip, new_pos);
	if (!tmp_psec) {
		err_no = __LINE__;
		ret = -1;
		goto ini_set_line_exist_keys_end;
	}

	//save front sections
	memcpy(tmp_mem + tmp_inip->mem_start_pos, local_ini_mem + ini_buf->mem_start_pos, new_pos);
	tmp_inip->mem_cur_pos = new_pos;

	//overwrite current section with new lines value
	tmp_size = psec->local_size;
	if ((new_pos + tmp_size) > tmp_inip->mem_size) {
		err_no = __LINE__;
		ret = -1;
		goto ini_set_line_exist_keys_end;
	}
	memcpy((void *)tmp_psec, psec, tmp_size);
	new_pos += tmp_size;
	tmp_inip->mem_cur_pos = new_pos;

	offset = 0;
	tmp_pline = NULL;
	pre_pline = pos_to_line(ini_buf, psec->first_line);
	while (pre_pline) {
		if (!tmp_pline)
			tmp_psec->first_line = new_pos;
		else
			tmp_pline->next = new_pos;
		tmp_pline = pos_to_line(tmp_inip, new_pos);
		if (!tmp_pline) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_keys_end;
		}
		pre_name = pos_to_str(ini_buf, pre_pline->name_pos);
		if (!pre_name) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_keys_end;
		}
		token_line = new_line;
		for (i = 0; i < line_cnt; i++) {
			if (strcmp(pre_name, token_line) == 0) { //target line, overwrite value
				pre_value = pos_to_str(ini_buf, pre_pline->value_pos);
				if (!pre_value) {
					err_no = __LINE__;
					ret = -1;
					goto ini_set_line_exist_keys_end;
				}
				tmp_size = (unsigned char *)pre_value - (unsigned char *)pre_pline;
				if ((new_pos + tmp_size) > tmp_inip->mem_size) {
					err_no = __LINE__;
					ret = -1;
					goto ini_set_line_exist_keys_end;
				}
				memcpy(tmp_pline, pre_pline, tmp_size);
				tmp_pline->name_pos += offset;
				tmp_pline->value_pos += offset;
				new_pos += tmp_size;

				tmp_value = pos_to_str(tmp_inip, tmp_pline->value_pos);
				if (!tmp_value) {
					err_no = __LINE__;
					ret = -1;
					goto ini_set_line_exist_keys_end;
				}
				token_value = new_value;
				for (j = 0; j < i; j++)
					token_value += strlen(token_value) + 2; //next value_str
				tmp_size = strlen(token_value) + 1; //include '\0'
				if ((new_pos + tmp_size) > tmp_inip->mem_size) {
					err_no = __LINE__;
					ret = -1;
					goto ini_set_line_exist_keys_end;
				}
				strlcpy(tmp_value, token_value, tmp_size);
				offset += (tmp_size - (strlen(pre_value) + 1));
				new_pos += tmp_size;

				ini_memnd_alloc_recode(__func__, pre_name, offset, INI_MEM_KEY);
				ALOGI("%s: %s, overwrite\n", __func__, token_line);

				goto ini_set_line_exist_keys_cur_psec_next_line;
			}
			token_line += strlen(token_line) + 2; //next value_str
		}

		//not target line, just copy
		tmp_size = pre_pline->local_size;
		if ((new_pos + tmp_size) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_keys_end;
		}
		memcpy(tmp_pline, pre_pline, tmp_size);
		tmp_pline->name_pos += offset;
		tmp_pline->value_pos += offset;
		new_pos += tmp_size;

ini_set_line_exist_keys_cur_psec_next_line:
		pre_pline = pos_to_line(ini_buf, pre_pline->next);
	}

	//save behind sections
	//need trave lines to update each name_pos and value_pos
	pre_psec = pos_to_section(ini_buf, psec->next);
	while (pre_psec) {
		tmp_psec->next = new_pos;
		tmp_psec = pos_to_section(tmp_inip, tmp_psec->next);
		if (!tmp_psec) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_keys_end;
		}

		tmp_size = pre_psec->local_size;
		if ((new_pos + tmp_size) > tmp_inip->mem_size) {
			err_no = __LINE__;
			ret = -1;
			goto ini_set_line_exist_keys_end;
		}
		memcpy((void *)tmp_psec, pre_psec, tmp_size);
		tmp_psec->name_pos += offset;

		new_pos += tmp_size;
		tmp_inip->mem_cur_pos = new_pos;

		tmp_pline = NULL;
		pre_pline = pos_to_line(ini_buf, pre_psec->first_line);
		while (pre_pline) {
			if (!tmp_pline)
				tmp_psec->first_line = new_pos;
			else
				tmp_pline->next = new_pos;
			tmp_pline = pos_to_line(tmp_inip, new_pos);
			if (!tmp_pline) {
				err_no = __LINE__;
				ret = -1;
				goto ini_set_line_exist_keys_end;
			}

			tmp_size = pre_pline->local_size;
			if ((new_pos + tmp_size) > tmp_inip->mem_size) {
				err_no = __LINE__;
				ret = -1;
				goto ini_set_line_exist_keys_end;
			}
			memcpy((void *)tmp_pline, pre_pline, tmp_size);
			tmp_pline->name_pos += offset;
			tmp_pline->value_pos += offset;

			new_pos += tmp_size;
			tmp_inip->mem_cur_pos = new_pos;

			pre_pline = pos_to_line(ini_buf, pre_pline->next);
		}

		pre_psec = pos_to_section(ini_buf, pre_psec->next);
	}

	//restore cur_mem
	memcpy(local_ini_mem + ini_buf->mem_start_pos, tmp_mem + tmp_inip->mem_start_pos, ini_buf->mem_size);
	ini_buf->mem_cur_pos = tmp_inip->mem_cur_pos;

ini_set_line_exist_keys_end:
	memset(tmp_mem, 0, ini_buf->total_size);
	free(tmp_mem);
	if (ret)
		ALOGE("%s: %d\n", __func__, err_no);

	return ret;
}

static struct ini_line_s *ini_new_line(struct ini_s *ini_buf, struct ini_section_s *psec,
			const char *name, const char *value)
{
	struct ini_line_s *pline = NULL;
	unsigned int tmp_pos, start_pos, tmp_size, str_size;
	char *str;

	start_pos = ini_buf->mem_cur_pos;
	tmp_size = sizeof(struct ini_line_s);
	if (start_pos + tmp_size >= ini_buf->mem_size) {
		ALOGE("%s: %s: out of mem_size %d\n",
			__func__, name, ini_buf->mem_size);
		return NULL;
	}
	pline = pos_to_line(ini_buf, start_pos);
	if (!pline) {
		ALOGE("%s: %s: start_pos error\n", __func__, name);
		return NULL;
	}
	pline->next = 0;
	tmp_pos = start_pos + tmp_size;

	str_size = strlen(name) + 1; //include '\0'
	if (tmp_pos + str_size >= ini_buf->mem_size) {
		ALOGE("%s: %s: out of mem_size %d\n",
			__func__, name, ini_buf->mem_size);
		return NULL;
	}
	pline->name_pos = tmp_pos;
	str = pos_to_str(ini_buf, pline->name_pos);
	if (!str) {
		ALOGE("%s: %s: name_pos error\n", __func__, name);
		return NULL;
	}
	strlcpy(str, name, str_size);
	tmp_pos += str_size;

	str_size = strlen(value) + 1; //include '\0'
	if (tmp_pos + str_size >= ini_buf->mem_size) {
		ALOGE("%s: %s: out of mem_size %d\n",
			__func__, name, ini_buf->mem_size);
		return NULL;
	}
	pline->value_pos = tmp_pos;
	str = pos_to_str(ini_buf, pline->value_pos);
	if (!str) {
		ALOGE("%s: %s: value_pos error\n", __func__, name);
		return NULL;
	}
	strlcpy(str, value, str_size);
	tmp_pos += str_size;

	pline->local_size = tmp_pos - start_pos;
	ini_buf->mem_cur_pos = tmp_pos;
	ini_memnd_alloc_recode(__func__, name, pline->local_size, INI_MEM_KEY);

	return pline;
}

static struct ini_section_s *ini_new_section(struct ini_s *ini_buf, const char *section)
{
	struct ini_section_s *psec = NULL;
	unsigned int tmp_pos, start_pos, tmp_size, str_size;
	char *str;

	start_pos = ini_buf->mem_cur_pos;
	tmp_size = sizeof(struct ini_section_s);
	if (start_pos + tmp_size >= ini_buf->mem_size) {
		ALOGE("%s: [%s]: out of mem_size %d\n",
			__func__, section, ini_buf->mem_size);
		return NULL;
	}
	psec = pos_to_section(ini_buf, start_pos);
	if (!psec) {
		ALOGE("%s: [%s]: start_pos error\n", __func__, section);
		return NULL;
	}
	psec->next = 0;
	tmp_pos = start_pos + tmp_size;

	str_size = strlen(section) + 1; //include '\0'
	if (tmp_pos + str_size >= ini_buf->mem_size) {
		ALOGE("%s: [%s]: out of mem_size %d\n",
			__func__, section, ini_buf->mem_size);
		return NULL;
	}
	psec->name_pos = tmp_pos;
	str = pos_to_str(ini_buf, psec->name_pos);
	if (!str) {
		ALOGE("%s: [%s]: name_pos error\n", __func__, section);
		return NULL;
	}
	strlcpy(str, section, str_size);
	tmp_pos += str_size;

	psec->local_size = tmp_pos - start_pos;
	ini_buf->mem_cur_pos = tmp_pos;
	ini_memnd_alloc_recode(__func__, section, psec->local_size, INI_MEM_SECTION);

	return psec;
}

static void trim(char *str, char ch)
{
	char *pstr, *ptmp;

	pstr = str;
	while (*pstr != '\0') {
		if (*pstr == ch) {
			ptmp = pstr;
			while (*ptmp != '\0') {
				*ptmp = *(ptmp + 1);
				ptmp++;
			}
		} else {
			pstr++;
		}
	}
}

static void trim_all(char *str)
{
	char *pstr = NULL;
	int len;

	pstr = strchr(str, '\n');
	if (pstr)
		*pstr = 0;

	len = strlen(str);
	if (len > 0) {
		if (str[len - 1] == '\r')
			str[len - 1] = '\0';
	}

	pstr = strchr(str, '#');
	if (pstr)
		*pstr = 0;

	pstr = strchr(str, ';');
	if (pstr)
		*pstr = 0;

	trim(str, ' ');
	trim(str, '{');
	trim(str, '\\');
	trim(str, '}');
	trim(str, '\"');
}

int ini_set_key_value(struct ini_s *ini_buf,
		const char *section, const char *key, const char *value, unsigned int set_mode)
{
	struct ini_line_s *pline = NULL, *cur_line;
	struct ini_section_s *psec = NULL, *cur_sec;
	unsigned int pos;
	int ret;

	if (!ini_buf || !section || !key || !value)
		return -1;

	trim_all((char *)value);
	if (value[0] == '\0')
		return 0; //no value is considered not an error

	if (!ini_buf->first_section) {
		psec = ini_new_section(ini_buf, section);
		if (!psec)
			goto ini_set_key_value_end;
		pline = ini_new_line(ini_buf, psec, key, value);
		if (!pline)
			goto ini_set_key_value_end;

		pos = line_to_pos(ini_buf, pline);
		psec->first_line = pos;
		psec->cur_line = pos;

		pos = section_to_pos(ini_buf, psec);
		ini_buf->first_section = pos;
		ini_buf->cur_section = pos;
	} else {
		psec = ini_get_section(ini_buf, section);
		if (!psec) {
			psec = ini_new_section(ini_buf, section);
			if (!psec)
				goto ini_set_key_value_end;
			pline = ini_new_line(ini_buf, psec, key, value);
			if (!pline)
				goto ini_set_key_value_end;
			cur_sec = pos_to_section(ini_buf, ini_buf->cur_section);
			if (!cur_sec)
				goto ini_set_key_value_end;

			pos = line_to_pos(ini_buf, pline);
			psec->first_line = pos;
			psec->cur_line = pos;

			pos = section_to_pos(ini_buf, psec);
			cur_sec->next = pos;
			ini_buf->cur_section = pos;
		} else {
			pline = ini_get_key_line_at_sec(ini_buf, psec, key);
			if (!pline) {
				pline = ini_new_line(ini_buf, psec, key, value);
				if (!pline)
					goto ini_set_key_value_end;
				cur_line = pos_to_line(ini_buf, psec->cur_line);
				if (!cur_line)
					goto ini_set_key_value_end;

				pos = line_to_pos(ini_buf, pline);
				cur_line->next = pos;
				psec->cur_line = pos;
			} else {
				ret = ini_set_line_exist_key_val(ini_buf, psec, pline,
						value, set_mode);
				if (ret)
					goto ini_set_key_value_end;
			}
		}
	}

	return 0;

ini_set_key_value_end:
	ALOGE("%s: [%s]: %s: error\n", __func__, section, key);
	return 0;
}

static int ini_multi_line_str_store(char *buf, char *str, int max_len)
{
	int len;

	trim_all(str);
	if (str[0] == '\0')
		return 0; //no value is not considered an error

	if (strlen(str) >= max_len) {
		ALOGE("%s: out of max_len %d: %s\n", __func__, max_len, str);
		return 0;
	}
	len = sprintf(buf, "%s", str);

	return len;
}

static int ini_multi_line_handler(struct ini_s *ini_buf, struct ini_multi_line_s *multi_line)
{
	int ret;

	if (!multi_line || !multi_line->sec_name || !multi_line->key_name || !multi_line->buf)
		return -1;

	ALOGD("%s: [%s]: %s=%s\n",
	      __func__, multi_line->sec_name, multi_line->key_name, multi_line->buf);
	ret = ini_set_key_value(ini_buf, multi_line->sec_name, multi_line->key_name,
				multi_line->buf, INI_SET_KEY_VAL_MODE_APPEND);

	return ret;
}

static int ini_key_multi_line_add(struct ini_s *ini_buf, struct ini_multi_line_s *multi_line,
				  char *str)
{
	char *end, *p;
	int n, ret = 0;

	end = find_char_or_comment(str, '}');
	if (*end == '}') { //multi_line end
		*end = '\0';
		rstrip(str);
		p = multi_line->buf + multi_line->cur_pos;
		n = multi_line->max_len - multi_line->cur_pos;
		multi_line->cur_pos += ini_multi_line_str_store(p, str, n);

		//set multi_line
		ret = ini_multi_line_handler(ini_buf, multi_line);
		multi_line->cur_pos = 0;
		multi_line->valid = 0;
	} else {
		p = multi_line->buf + multi_line->cur_pos;
		n = multi_line->max_len - multi_line->cur_pos;
		multi_line->cur_pos += ini_multi_line_str_store(p, str, n);
	}

	return ret;
}

static int ini_key_handler(struct ini_s *ini_buf,
		const char *section, const char *name, const char *value)
{
	int ret;

	ret = ini_set_key_value(ini_buf, section, name, value, INI_SET_KEY_VAL_MODE_OVERWRITE);
	return ret;
}

int ini_parse_mem(struct ini_s *ini_buf, const char *tmp_buf)
{
	const char *bufptr;
	char *line;
	struct ini_multi_line_s multi_line;
	char section[INI_SECTION_NAME_LEN_MAX] = "";
	char prev_name[INI_KEY_NAME_LEN_MAX] = "";
	char *start, *end;
	char *name, *value;
	int char_cnt, lineno = 0, error = 0;
	int ret;

	if (!ini_buf || !tmp_buf)
		return -1;

	line = (char *)malloc(INI_LINE_LEN_MAX);
	if (!line)
		return -1;
	memset(line, 0, INI_LINE_LEN_MAX);

	multi_line.max_len = INI_MULTI_LINE_LEN_MAX;
	multi_line.buf = (char *)malloc(multi_line.max_len);
	if (!multi_line.buf) {
		free(line);
		return -1;
	}
	memset(multi_line.buf, 0, multi_line.max_len);
	multi_line.valid = 0;
	multi_line.cur_pos = 0;
	multi_line.sec_name = section;
	multi_line.key_name = prev_name;

	bufptr = tmp_buf;
	while (1) {
		char_cnt = 0;
		while (*bufptr != '\0') {
			if (*bufptr == '\r' || *bufptr == '\n')
				break;

			line[char_cnt] = *bufptr++;
			char_cnt++;
		}
		while (*bufptr == '\r' || *bufptr == '\n')
			bufptr++;
		line[char_cnt] = 0;

		if (char_cnt == 0) //end
			break;

		lineno++;
		//ALOGD("%s: lineno=%d\n", __func__, lineno);
		if (char_cnt > INI_LINE_LEN_MAX) {
			ALOGE("%s: line %d char_cnt %d out of support\n",
			      __func__, lineno, char_cnt);
			if (!error)
				error = lineno;
		}

		start = line;
		start = lskip(rstrip(start));

		if (*start == ';' || *start == '#') {
			/* allow '#' comments at start of line */
		} else if (*start == '[') {
			/* A "[section]" line */
			end = find_char_or_comment(start + 1, ']');
			if (*end == ']') {
				*end = '\0';
				strlcpy(section, start + 1, sizeof(section));
				*prev_name = '\0';
				if (multi_line.valid) {
					multi_line.valid = 0;
					multi_line.cur_pos = 0;
					if (!error)
						error = lineno;
				}
			} else if (!error) {
				/* No ']' found on section line */
				error = lineno;
			}
		} else if (*start) {
			/* Not a comment, should be a name[=:]value pair */
			end = find_char_or_comment(start, '=');
			if (*end != '=')
				end = find_char_or_comment(start, ':');
			if (*end == '=' || *end == ':') {
				/* Valid name[=:]value pair found */
				*end = '\0';
				name = rstrip(start);
				strlcpy(prev_name, name, sizeof(prev_name));
				if (multi_line.valid) {
					multi_line.valid = 0;
					multi_line.cur_pos = 0;
					if (!error)
						error = lineno;
				}

				value = lskip(end + 1);
				end = find_char_or_comment(value, '\0');
				if (*end == ';' || *end == '#')
					*end = '\0';
				rstrip(value);

				if (*value == '{') { //multi_line start
					value = lskip(value + 1);
					multi_line.valid = 1;
					ret = ini_key_multi_line_add(ini_buf, &multi_line, value);
				} else {
					/* Valid name[=:]value pair found, call handler */
					ret = ini_key_handler(ini_buf, section, name, value);
				}
				if (ret && !error)
					error = lineno;
			} else {
				/* No '=' or ':' found, not name[=:]value line */
				value = rstrip(start);
				end = find_char_or_comment(value, '\0');
				if (*end == ';' || *end == '#')
					*end = '\0';
				rstrip(value);

				if (*value == '{') { //multi_line start
					if (multi_line.valid == 0 && *prev_name) {
						value = lskip(value + 1);
						multi_line.valid = 1;
						ret = ini_key_multi_line_add(ini_buf, &multi_line,
									     value);
						if (ret && !error)
							error = lineno;
					} else { //'{' can't appear in the middle of multi_line
						if (!error)
							error = lineno;
					}
				} else {
					if (multi_line.valid && *prev_name) {
						ret = ini_key_multi_line_add(ini_buf, &multi_line,
									     value);
						if (ret && !error)
							error = lineno;
					}
				}
			}
		}
	}

	if (error)
		ALOGE("%s: error=%d\n", __func__, error);

	memset(line, 0, INI_LINE_LEN_MAX);
	free(line);

	ini_alloc_parse_mem_print();

	return error;
}

const char *ini_get_string(struct ini_s *ini_buf, struct ini_section_s *psec,
		const char *key, const char *def_value)
{
	struct ini_line_s *pline;
	char *value;

	if (!ini_buf || !psec)
		return def_value;

	pline = ini_get_key_line_at_sec(ini_buf, psec, key);
	if (!pline)
		return def_value;

	value = pos_to_str(ini_buf, pline->value_pos);
	return value;
}

void ini_list_key_value(struct ini_s *ini_buf)
{
	struct ini_section_s *psec;
	struct ini_line_s *pline;
	char *str, *name, *value;
	int value_size, i, j, n, m;

	if (!ini_buf)
		return;

	str = (char *)malloc(512);
	if (!str) {
		printf("%s: malloc print memory error\n", __func__);
		return;
	}

	psec = pos_to_section(ini_buf, ini_buf->first_section);
	while (psec) {
		name = pos_to_str(ini_buf, psec->name_pos);
		printf("[%s]\n", name);
		pline = pos_to_line(ini_buf, psec->first_line);
		while (pline) {
			name = pos_to_str(ini_buf, pline->name_pos);
			value = pos_to_str(ini_buf, pline->value_pos);
			value_size = strlen(value) + 1;
			if (value_size >= 510) {
				printf("  %s = ", name);
				n = value_size / 510;
				m = value_size % 510;
				for (i = 0; i < n; i++) {
					j = i * 510;
					strncpy(str, value + j, 510);
					str[510] = '\0';
					printf("%s\n", str);
				}
				if (m) {
					j = n * 510;
					strncpy(str, value + j, m);
					str[m] = '\0';
					printf("%s\n", str);
				}
			} else {
				printf("  %s = %s\n", name, value);
			}
			pline = pos_to_line(ini_buf, pline->next);
		}
		printf("\n");
		psec = pos_to_section(ini_buf, psec->next);
	}

	memset(str, 0, 512);
	free(str);
}

void ini_list_section(struct ini_s *ini_buf)
{
	struct ini_section_s *psec;
	char *name;

	if (!ini_buf)
		return;
	psec = pos_to_section(ini_buf, ini_buf->first_section);
	while (psec) {
		name = pos_to_str(ini_buf, psec->name_pos);
		printf("  %s\n", name);
		psec = pos_to_section(ini_buf, psec->next);
	}
}

void ini_handler_mem_free(struct ini_s *ini_buf)
{
	int alloc_size;

	if (!ini_buf)
		return;

	alloc_size = 0 - (ini_buf->mem_cur_pos - INI_MEM_DATA_OFFSET);
	ini_memnd_alloc_recode(__func__, "all", alloc_size, INI_MEM_ALL);

	ini_alloc_parse_mem_print();
}
