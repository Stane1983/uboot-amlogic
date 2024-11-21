// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>
#include "json_parse.h"

/*=dump ===============================================================================*/
#define ARRAY_ID_SHOW 0

void json_string_dump(char *js, struct json_s *json, s32 indent)
{
	s32 k = 0;

	for (k = indent; k > 0; k--)
		json_log("\t");
	if (json->key >= 0)
		json_log("\"%s\":\"%s\"", js + json->key, js + json->value);
	else
#if (ARRAY_ID_SHOW)
		json_log("%d:\"%s\"", -(json->key + 1), js + json->value);
#else
	json_log("\"%s\"", js + json->value);
#endif
	if (json->next == -1)
		json_log("\n");
	else
		json_log(",\n");
}

void json_number_dump(char *js, struct json_s *json, s32 indent)
{
	s32 k = 0;

	for (k = indent; k > 0; k--)
		json_log("\t");
	if (json->key >= 0)
		json_log("\"%s\":%s", js + json->key, js + json->value);
	else
#if (ARRAY_ID_SHOW)
		json_log("%d:%s", -(json->key + 1), js + json->value);
#else
	json_log("%s", js + json->value);
#endif
	if (json->next == -1)
		json_log("\n");
	else
		json_log(",\n");
}

void json_array_dump(char *js, struct json_s *json_array, struct json_s *json, s32 indent)
{
	s32 k = indent;
	struct json_s *child;

	for (k = indent; k > 0; k--)
		json_log("\t");
	if (json->key >= 0)
		json_log("\"%s\":[\n", js + json->key);
	else
#if (ARRAY_ID_SHOW)
		json_log("%d:[\n", -(json->key + 1));
#else
		json_log("[\n");
#endif
	if (json->value == -1) {
		for (k = indent; k > 0; k--)
			json_log("\t");
		json_log("]");
		return;
	}
	child = &json_array[json->value];
	for (;; child = &json_array[child->next]) {
		if (child->type == JSON_NUMBER)
			json_number_dump(js, child, indent + 1);
		else if (child->type == JSON_STRING)
			json_string_dump(js, child, indent + 1);
		else if (child->type == JSON_OBJECT)
			json_obj_dump(js, json_array, child, indent + 1);
		else if (child->type == JSON_ARRAY)
			json_array_dump(js, json_array, child, indent + 1);

		if (child->next == -1)
			break;
	}

	for (k = indent; k > 0; k--)
		json_log("\t");
	json_log("]");

	if (json->next == -1)
		json_log("\n");
	else
		json_log(",\n");
}

void json_obj_dump(char *js, struct json_s *json_array, struct json_s *json, s32 indent)
{
	s32 k = indent;
	struct json_s *child;

	for (k = indent; k > 0; k--)
		json_log("\t");
	if (json->key >= 0) {
		if (json->key == 0)
			json_log("{\n");
		else
			json_log("\"%s\":{\n", js + json->key);
	} else {
#if (ARRAY_ID_SHOW)
		json_log("%d:{\n", -(json->key + 1));
#else
		json_log("{\n");
#endif
	}
	if (json->value == -1) {
		for (k = indent; k > 0; k--)
			json_log("\t");
		json_log("}");
		return;
	}
	child = &json_array[json->value];
	for (;; child = &json_array[child->next]) {
		if (child->type == JSON_NUMBER)
			json_number_dump(js, child, indent + 1);
		else if (child->type == JSON_STRING)
			json_string_dump(js, child, indent + 1);
		else if (child->type == JSON_OBJECT)
			json_obj_dump(js, json_array, child, indent + 1);
		else if (child->type == JSON_ARRAY)
			json_array_dump(js, json_array, child, indent + 1);

		if (child->next == -1)
			break;
	}

	for (k = indent; k > 0; k--)
		json_log("\t");
	json_log("}");

	if (json->next == -1)
		json_log("\n");
	else
		json_log(",\n");
}

void json_dump(struct json_parse_s *jsp, struct json_s *json)
{
	if (!jsp || !jsp->js || !jsp->root || !json)
		return;

	switch (json->type) {
	case JSON_NUMBER:
		json_number_dump(jsp->js, json, 0);
		break;
	case JSON_STRING:
		json_string_dump(jsp->js, json, 0);
		break;
	case JSON_ARRAY:
		json_array_dump(jsp->js, jsp->root, json, 0);
		break;
	case JSON_OBJECT:
		json_obj_dump(jsp->js, jsp->root, json, 0);
		break;
	default:
		json_log("json error type:%d\n", json->type);
		break;
	}
}

void json_dump_path(struct json_parse_s *jsp, const char *path)
{
	struct json_s *json;

	if (!jsp)
		return;
	json = json_path_to_node(jsp, jsp->root, (char *)path);
	if (json)
		json_dump(jsp, json);
	else
		json_dump(jsp, jsp->root);
}

/*=value===============================================================================*/
s32 path_sep(char *path, char **key, s32 max_parm)
{
	char *ps, *token;
	char str[3] = {'/', '\0'};
	s32 n = 0;

	ps = path;
	while (n < max_parm) {
		token = strsep(&ps, str);
		if (!token)
			break;
		if (*token == '\0')
			continue;
		key[n++] = token;
	}
	return n;
}

// trans path to json node, from 'start', if path start with '/', trans from 'root'
struct json_s *json_path_to_node(struct json_parse_s *jsp, struct json_s *start, const char *path)
{
	char *str;
	char *key[32];
	s32 i = 0, cnt = 0;
	struct json_s *json = NULL, *parent;

	if (!jsp || !jsp->js || !jsp->root)
		return NULL;
	str = strdup(path);
	if (!str)
		return NULL;

	if (path[0] == '/') {
		parent = &jsp->root[0];
		if (path[1] == '\0') {
			json = parent;
			goto json_find_node_exit;
		}
		cnt = path_sep(str + 1, key, 32);
	} else	{
		parent = start;
		cnt = path_sep(str, key, 32);
	}
	if (!parent)
		goto json_find_node_exit;

	for (i = 0; i < cnt; i++) {
		json = json_get_object_child(jsp, parent, key[i]);
		if (!json)
			goto json_find_node_exit;
		parent = json;
	}

json_find_node_exit:
	free(str);

	return json;
}

//the follow function and micro I don't check parameter, please check them before call functions

const char *json_get_key_str(struct json_parse_s *jsp, struct json_s *start,
			     const char *key, char *dft)
{
	struct json_s *json = json_path_to_node(jsp, start, key);
	const char *val = (const char *)dft;

	if (json)
		val = json_get_str(jsp, json);

	return val ? val : (const char *)dft;
}

const char *json_get_obj_str(struct json_parse_s *jsp, struct json_s *json,
			     const char *name, char *dft)
{
	struct json_s *child;

	if (!__json_is_object(json))
		return (const char *)dft;
	child = json_get_object_child(jsp, json, name);
	if (child)
		return json_get_str(jsp, child);
	return (const char *)dft;
}

const char *json_get_arr_str(struct json_parse_s *jsp, struct json_s *json, s32 id, char *dft)
{
	struct json_s *child = json_get_array_child(jsp, json, id);
	const char *val = (const char *)dft;

	if (child)
		val = json_get_str(jsp, child);

	return val ? val : (const char *)dft;
}

s32 json_get_key_s32(struct json_parse_s *jsp, struct json_s *start, const char *key, s32 dft)
{
	struct json_s *json = json_path_to_node(jsp, start, key);
	s32 val = dft;

	if (json)
		json_get_s32(jsp, json, &val);

	return val;
}

s32 json_get_obj_s32(struct json_parse_s *jsp, struct json_s *json, const char *name, s32 dft)
{
	struct json_s *child = json_get_object_child(jsp, json, name);
	s32 num = dft;

	if (child)
		json_get_s32(jsp, child, &num);

	return num;
}

s32 json_get_arr_s32(struct json_parse_s *jsp, struct json_s *json, s32 id, s32 dft)
{
	struct json_s *child = json_get_array_child(jsp, json, id);
	s32 num = dft;

	if (child)
		json_get_s32(jsp, child, &num);

	return num;
}

u32 json_get_key_u32(struct json_parse_s *jsp, struct json_s *start, const char *key, u32 dft)
{
	struct json_s *json = json_path_to_node(jsp, start, key);
	u32 val = dft;

	if (json)
		json_get_u32(jsp, json, &val);

	return val;
}

u32 json_get_obj_u32(struct json_parse_s *jsp, struct json_s *json, const char *name, u32 dft)
{
	struct json_s *child = json_get_object_child(jsp, json, name);
	u32 num = dft;

	if (child)
		json_get_u32(jsp, child, &num);

	return num;
}

u32 json_get_arr_u32(struct json_parse_s *jsp, struct json_s *json, s32 id, u32 dft)
{
	struct json_s *child = json_get_array_child(jsp, json, id);
	u32 num = dft;

	if (child)
		json_get_u32(jsp, child, &num);

	return num;
}

