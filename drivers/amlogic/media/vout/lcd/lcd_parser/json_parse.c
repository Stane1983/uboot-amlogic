// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <malloc.h>
#include <common.h>
#include <linux/ctype.h>
#include "json_parse.h"

 #ifndef strtoul
    #define strtoul simple_strtoul
#endif

#ifndef strtol
    #define strtol simple_strtol
#endif

static inline unsigned int __todigit(unsigned char c)
{
	return isdigit(c) ? (c - '0') : isxdigit(c) ? ((tolower(c) - 'a') + 10) : 0;
}

#define todigit __todigit

#ifndef GFP_KERNEL
#define GFP_KERNEL (0)
#endif

#ifdef JSON_ERR_TRACE
void json_err_trace(char *input, s32 pos, char *fmt)
{
	s32 p = pos + 1, i = 0;

	for (i = 0; i < JSON_TRACE_LINE; i++)
		for (p--; p >= 0 && input[p] != '\n'; p--)
			;
	if (p < 0)
		p = 0;
	json_log("\033[01;0;31mjson err:%.*s\033[00;0;0m |%s\n", pos - p + 1, input + p, fmt);
}
#else
static inline void json_err_trace(char *input, s32 pos, char *fmt)
{
}
#endif

struct json_s *json_alloc(struct json_parse_s *parse)
{
	if (parse && parse->root && parse->json_cnt < parse->json_max) {
		parse->root[parse->json_cnt].type = JSON_INVALID;
		parse->root[parse->json_cnt].next = -1;
#if (JSON_SUPPORT_CHILD_SIZE)
		parse->root[parse->json_cnt].child_size = 0;
#endif
		return &parse->root[parse->json_cnt++];
	}

	return NULL;
}

//char *test_json = " {\"key1\":\"value1\"}";
static s32 buffer_skip_space(char *input, s32 pos, s32 limit)
{
	s32 p = pos;

	while (p < limit && input[p] != '\0' && input[p] <= ' ')
		p++;

	return p - pos;
}

#if (JSON_SUPPORT_COMMENT)
//#define js_space_or_comment_entry(c) (((c) <= ' ' && (c) != '\0') || (c) == '/')
static inline int js_space_or_comment_entry(char c)
{
	return  ((c <= ' ' && c != '\0') || c == '/');
}

static s32 buffer_skip_comment(char *input, s32 pos, s32 limit)
{
	s32 p = pos;

	if (p >= limit || input[p] != '/')
		return 0;
	p++;
	if (p >= limit || (input[p] != '/' && input[p] != '*')) {
		json_err_trace(input, p, "comment should start with // or /*");
		return -1;
	}

	if (input[p] == '/') {
		while (p < limit) {
			if (input[p] == '\n' || input[p] == '\0')
				return p + 1 - pos;
			p++;
		}
	} else {
		p++;
		while (p + 1 < limit) {
			if (input[p] == '*' && input[p + 1] == '/')
				return p + 2 - pos;
			p++;
		}
	}
	json_err_trace(input, pos + 2, "comment should end with */");
	return -1;
}
#else
//#define js_space_or_comment_entry(c) ((c) <= ' ' && (c) != '\0')
static inline int js_space_or_comment_entry(char c)
{
	return (c <= ' ' && c != '\0');
}

static inline s32 buffer_skip_comment(char *input, s32 pos, s32 limit)
{
	(void)input;
	(void)pos;
	(void)limit;

	return 0;
}
#endif

static s32 buffer_skip_invalid(char *input, s32 pos, s32 limit)
{
	s32 p = pos, len = 0;

	while (p < limit && js_space_or_comment_entry(input[p])) {
		len = buffer_skip_space(input, p, limit);
		p += len;
		len = buffer_skip_comment(input, p, limit);
		if (len < 0)
			return -1;
		p += len;
	}

	return p - pos;
}

static s32 json_parse_string(struct json_parse_s *jsp, char *input, s32 limit)
{
	s32 p = jsp->pos, p1 = jsp->js_len, rc = -1;
	char *s = input;

	if (p >= limit && s[p] != '\"') {
		json_err_trace(input, jsp->pos, "string should start with \"");
		return -1;
	}
	p++;

	while (p < limit && s[p] != '\0' && jsp->js_len < jsp->js_max) {
		switch (s[p]) {
		case '\"':
			jsp->js[jsp->js_len++] = '\0';
			jsp->pos = p + 1;
			rc = p1;
			return rc;
		case '\r':
		case '\n':
		case '\t':
		case '\b':
			break;
		default:
			jsp->js[jsp->js_len++] = s[p];
			break;
		}
		p++;
	}

	json_err_trace(input, jsp->pos, "string should start with \"");

	return rc;
}

static s32 json_parse_number_as_string(struct json_parse_s *jsp, char *input, s32 limit)
{
	s32 p = jsp->pos, p1 = jsp->js_len, rc = -1;
	char *s = input;

	if (s[p] == 't' && p + 4 < limit && strncmp(s + p, "true", 4) == 0) {
		jsp->js[jsp->js_len++] = 't';
		jsp->js[jsp->js_len++] = 'r';
		jsp->js[jsp->js_len++] = 'u';
		jsp->js[jsp->js_len++] = 'e';
		jsp->js[jsp->js_len++] = '\0';
		jsp->pos = p + 4;
		return p1;
	} else if (s[p] == 'f' && p + 4 < limit && strncmp(s + p, "false", 5) == 0) {
		jsp->js[jsp->js_len++] = 'f';
		jsp->js[jsp->js_len++] = 'a';
		jsp->js[jsp->js_len++] = 'l';
		jsp->js[jsp->js_len++] = 's';
		jsp->js[jsp->js_len++] = 'e';
		jsp->js[jsp->js_len++] = '\0';
		jsp->pos = p + 5;
		return p1;
	} else if (p + 3 < limit && s[p] == '0' && __tolower(s[p + 1]) == 'x' &&
		   (isdigit(s[p + 2]) || isxdigit(s[p + 2]))) {
		jsp->js[jsp->js_len++] = '0';
		jsp->js[jsp->js_len++] = 'x';
		p += 2;

		while (p < limit && s[p] != '\0' && jsp->js_len < jsp->js_max) {
			if (isdigit(s[p]) || isxdigit(s[p])) {
				jsp->js[jsp->js_len++] = s[p];
				p++;
			} else {
				jsp->js[jsp->js_len++] = '\0';
				jsp->pos = p;
				rc = p1;
				return rc;
			}
		}
	} else {
		while (p < limit && s[p] != '\0' && jsp->js_len < jsp->js_max) {
			if (isdigit(s[p])) {
				jsp->js[jsp->js_len++] = s[p];
				p++;
			} else {
				jsp->js[jsp->js_len++] = '\0';
				jsp->pos = p;
				rc = p1;
				return rc;
			}
		}
	}

	json_err_trace(input, jsp->pos, "string should start with \"");

	return rc;
}

s32 json_parse_value(struct json_parse_s *jsp, struct json_s *json, char *input, s32 limit)
{
	s32 child_size = 0;

	switch (input[jsp->pos]) {
	case '\"':
		json->value = json_parse_string(jsp, input, limit);
		if (json->value < 0)
			return -1;
		json->type = JSON_STRING;
		break;
	case '{':
		json->value = json_parse_object(jsp, input, limit, &child_size);
		if (json->value < 0)
			return -1;
		json->type = JSON_OBJECT;

		break;
	case '[':
		json->value = json_parse_array(jsp, input, limit, &child_size);
		if (json->value < 0)
			return -1;
		json->type = JSON_ARRAY;

		break;
	default: // dec & hex number, true/false
		json->value = json_parse_number_as_string(jsp, input, limit);

		if (json->value < 0)
			return -1;
		json->type = JSON_NUMBER;
		break;
	}
#if (JSON_SUPPORT_CHILD_SIZE)
	json->child_size = child_size;
#else
	(void)child_size;
#endif
	return 0;
}

// return -1=meet error, 0=ready to next, 1=meet finished
s32 json_parse_prepare_next(struct json_parse_s *jsp, char *input, s32 limit,
	char next_symb, char end_symb)
{
	s32 offset = 0;

	offset = buffer_skip_invalid(input, jsp->pos, limit);
	if (offset < 0)
		return -1;
	jsp->pos += offset;

	if (jsp->pos < limit && input[jsp->pos] == end_symb) {
		jsp->pos++;
		offset = buffer_skip_invalid(input, jsp->pos, limit);
		if (offset < 0)
			return -1;
		jsp->pos += offset;
		return 1; //parse finish
	}

	if (jsp->pos < limit && input[jsp->pos] == next_symb) {
		jsp->pos++;
		offset = buffer_skip_invalid(input, jsp->pos, limit);
		if (offset < 0)
			return -1;
		jsp->pos += offset;
		if (jsp->pos >= limit) {
			json_err_trace(input, jsp->pos, "parse exceed limit");
			return -1;
		}
		if (input[jsp->pos] == end_symb) {
#if (JSON_SUPPORT_TAIL_COMMA)
			jsp->pos++;
			return 1;
#else
			json_err_trace(input, jsp->pos, "no need a comma at last object");
			return -1;
#endif
		} else {
			return 0;//parse not finish, need parse next
		}
	}
	json_err_trace(input, jsp->pos, "need a comma between 2 object");

	return -1;
}

s32 json_parse_array(struct json_parse_s *jsp, char *input, s32 limit, s32 *child_size)
{
	s32 offset = 0, head = -1, current_cnt = 0, id = -1, ret = 0;
	char *s = input;
	struct json_s *json, *prev;

	if (jsp->pos >= limit || s[jsp->pos] != '[') {
		json_err_trace(input, jsp->pos, "array should start with [");
		return -1;
	}
	jsp->pos++;

	offset = buffer_skip_invalid(s, jsp->pos, limit);
	if (offset < 0)
		return -1;
	jsp->pos += offset;

	while (jsp->pos < limit && s[jsp->pos] != '\0') {
		json = jsp->alloc(jsp);
		if (!json) {
			json_err("%s no json mem\n", __func__);
			return -1;
		}
		json->key = id--; //negative key mean array child id

		if (head < 0) {
			head = jsp->json_cnt - 1;
			prev = json;
		}
		current_cnt = jsp->json_cnt - 1;

		if (json_parse_value(jsp, json, s, limit) < 0)
			return -1;

		if (current_cnt != head)
			prev->next = current_cnt;
		prev = json;
#if (!JSON_SUPPORT_CHILD_SIZE)
		(void)child_size;
#else
		*child_size++;
#endif
		ret = json_parse_prepare_next(jsp, input, limit, ',', ']');
		if (ret == 1)
			return head;
		else if (ret < 0)
			return -1;
	}
	return -1;
}

s32 json_parse_object(struct json_parse_s *jsp, char *input, s32 limit, s32 *child_size)
{
	s32 offset = 0, head = -1, current_cnt = 0, ret = -1;
	char *s = input;
	struct json_s *json, *prev;

	if (jsp->pos >= limit || s[jsp->pos] != '{') {
		json_err_trace(input, jsp->pos, "object should start with {");
		return -1;
	}
	jsp->pos++;

	offset = buffer_skip_invalid(s, jsp->pos, limit);
	if (offset < 0)
		return -1;
	jsp->pos += offset;

	if (jsp->pos >= limit || s[jsp->pos] != '\"') {
		json_err_trace(input, jsp->pos, "log_here \"");
		return -1;
	}

	while (jsp->pos < limit && s[jsp->pos] != '\0') {
		json = jsp->alloc(jsp);
		if (!json) {
			json_err("%s no json mem\n", __func__);
			return -1;
		}

		if (head < 0) {
			head = jsp->json_cnt - 1;
			prev = json;
		}
		current_cnt = jsp->json_cnt - 1;

		offset = json_parse_string(jsp, s, limit);
		if (offset < 0)
			return -1;

		json->key = offset;

		offset = buffer_skip_invalid(s, jsp->pos, limit);
		if (offset < 0)
			return -1;
		jsp->pos += offset;

		if (jsp->pos >= limit || s[jsp->pos] != ':') {
			json_err_trace(input, jsp->pos, "a key should follow ':'");
			return -1;
		}
		jsp->pos++;

		offset = buffer_skip_invalid(s, jsp->pos, limit);
		if (offset < 0)
			return -1;
		jsp->pos += offset;

		if (jsp->pos >= limit) {
			json_err_trace(input, jsp->pos, "parse pos exceed limit");
			return -1;
		}

		if (json_parse_value(jsp, json, s, limit) < 0)
			return -1;

		if (current_cnt != head)
			prev->next = current_cnt;
		prev = json;
#if (!JSON_SUPPORT_CHILD_SIZE)
		(void)child_size;
#else
		*child_size++;
#endif
		ret = json_parse_prepare_next(jsp, input, limit, ',', '}');
		if (ret == 1)
			return head;
		else if (ret < 0)
			return -1;
	}
	return -1;
}

struct json_s *json_parse(struct json_parse_s *jsp, char *input, s32 limit)
{
	struct json_s *root;
	s32 offset = 0, child_size = 0;

	root = jsp->alloc(jsp);
	jsp->js[0] = '/';
	jsp->js[1] = '\0';
	jsp->js_len = 2;

	root->key = 0;
	root->next = -1;
	root->type = JSON_OBJECT;
	root->value = -1;

	offset = buffer_skip_invalid(input, jsp->pos, limit);
	if (offset < 0)
		return NULL;

	jsp->pos += offset;
	root->value = json_parse_object(jsp, input, limit, &child_size);

	if (root->value < 0)
		return NULL;

	return root;
}

s32 json_init(struct json_parse_s *jsp, s32 js_max, s32 json_max)
{
	if (!jsp || js_max <= 0 || json_max <= 0)
		return -1;
	json_max = json_max >= JSON_NODE_MAX ? JSON_NODE_MAX : json_max;
	js_max = js_max >= JSON_STR_MAX ? JSON_STR_MAX : js_max;
	jsp->alloc = json_alloc;
	jsp->js = (char *)malloc(js_max);
	jsp->json_cnt = 0;
	jsp->json_max = json_max;
	jsp->js_len = 0;
	jsp->js_max = js_max;
	jsp->pos = 0;
	jsp->root = (struct json_s *)malloc(json_max * sizeof(*jsp->root));

	memset(jsp->js, 0, js_max);
	memset(jsp->root, 0, json_max * sizeof(*jsp->root));

	if (!jsp->root || !jsp->js)
		return -1;
	return 0;
}

void json_deinit(struct json_parse_s *jsp)
{
	if (!jsp)
		return;

	if (jsp->root)
		free(jsp->root);
	if (jsp->js)
		free(jsp->js);
	memset(jsp, 0, sizeof(*jsp));
}

/*===============================================================================================*/
static inline s32 json_valid(struct json_parse_s *jsp, struct json_s *json, json_ssize_t type)
{
	return !(!jsp || !jsp->root || !jsp->js || !json || json->type != type || json->value < 0);
}

struct json_s *json_get_object_child(struct json_parse_s *jsp, struct json_s *parent,
				     const char *name)
{
	struct json_s *child;

	if (!json_valid(jsp, parent, JSON_OBJECT))
		return NULL;

	child = &jsp->root[parent->value];
	for (; ; child = &jsp->root[child->next]) {
		if (strcmp(name, jsp->js + child->key) == 0)
			return child;
		if (child->next < 0)
			return NULL;
	}

	return NULL;
}

struct json_s *json_get_object_child_by_id(struct json_parse_s *jsp, struct json_s *parent, int id)
{
	s32 i = 0;
	struct json_s *child;

	if (!json_valid(jsp, parent, JSON_OBJECT))
		return NULL;

	child = &jsp->root[parent->value];
	for (; ; child = &jsp->root[child->next]) {
		if (i++ == id)
			return child;
		if (child->next < 0)
			return NULL;
	}

	return NULL;
}

struct json_s *json_get_array_child(struct json_parse_s *jsp, struct json_s *parent, s32 id)
{
	s32 i = 0;
	struct json_s *child;

	if (!json_valid(jsp, parent, JSON_ARRAY))
		return NULL;

	child = &jsp->root[parent->value];
	for (; ; child = &jsp->root[child->next]) {
		if (i++ == id)
			return child;
		if (child->next < 0)
			return NULL;
	}

	return NULL;
}

struct json_s *json_get_nd_array_child(struct json_parse_s *jsp, struct json_s *parent,
	s32 nid, s32 ids[])
{
	struct json_s *child;
	s32 i = 0;

	if (!json_valid(jsp, parent, JSON_ARRAY))
		return NULL;
	child = parent;
	for (i = 0; i < nid; i++) {
		child = json_get_array_child(jsp, child, ids[i]);
		if (!child)
			return NULL;
	}
	return child;
}

static s32 json_get_child_count(struct json_parse_s *jsp, struct json_s *parent, json_size_t type)
{
	s32 i = 0;
	struct json_s *child;

	if (!json_valid(jsp, parent, type))
		return -1;

#if (JSON_SUPPORT_CHILD_SIZE)
	return parent->child_size;
#else
	child = &jsp->root[parent->value];
	for (; ;) {
		i++;
		if (child->next < 0)
			return i;
		child = &jsp->root[child->next];
	}

	return -1;
#endif
}

s32 json_get_object_size(struct json_parse_s *jsp, struct json_s *json)
{
	return json_get_child_count(jsp, json, JSON_OBJECT);
}

s32 json_get_array_size(struct json_parse_s *jsp, struct json_s *json)
{
	return json_get_child_count(jsp, json, JSON_ARRAY);
}

/* not safe, must check jsp and json valid*/
s32 json_get_u32(struct json_parse_s *jsp, struct json_s *json, u32 *num)
{
	char *p;

	if (!__json_is_string_or_number(json))
		return -1;
	p = jsp->js + json->value;
	if (p[0] == 't' && strcmp(p, "true") == 0)
		*num = 1;
	else if (p[0] == 'f' && strcmp(p, "false") == 0)
		*num = 0;
	else
		*num = (u32)strtoul(p, NULL, 0);

	return 0;
}

/* not safe, must check jsp and json valid*/
s32 json_get_s32(struct json_parse_s *jsp, struct json_s *json, s32 *num)
{
	char *p;

	if (!__json_is_string_or_number(json))
		return -1;
	p = jsp->js + json->value;
	if (p[0] == 't' && strcmp(p, "true") == 0)
		*num = 1;
	else if (p[0] == 'f' && strcmp(p, "false") == 0)
		*num = 0;
	else
		*num = (s32)strtol(p, NULL, 0);

	return 0;
}

/* not safe, must check jsp and json valid*/
const char *json_get_str(struct json_parse_s *jsp, struct json_s *json)
{
	if (!__json_is_string(json))
		return NULL;

	return (const char *)(jsp->js + json->value);
}

const char *json_get_key(struct json_parse_s *jsp, struct json_s *json)
{
	if (!jsp || !json)
		return NULL;

	return (const char *)(jsp->js + json->key);
}

