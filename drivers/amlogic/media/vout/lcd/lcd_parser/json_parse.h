/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __JSON__H
#define __JSON__H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef u64
#define u64 unsigned long long
#endif

#ifndef s64
#define s64 signed long long
#endif

#ifndef u32
#define u32 unsigned int
#endif

#ifndef s32
#define s32 signed int
#endif

#ifndef f32
#define f32 float
#endif

#ifndef f64
#define f64 double
#endif

//#define JSON_NODE_32

#ifdef JSON_NODE_32
#define json_int int
#define json_ssize_t int
#define json_uint unsigned int
#define json_size_t unsigned int
#define JSON_STR_MAX (32 * 1024 - 1) //32K js buffer
#define JSON_NODE_MAX (JSON_STR_MAX / sizeof(struct json_s)) //4095 node

#else
#define json_int short
#define json_ssize_t short
#define json_uint unsigned short
#define json_size_t unsigned short
#define JSON_STR_MAX (16 * 1024 - 1) //16K js buffer
#define JSON_NODE_MAX (JSON_STR_MAX / sizeof(struct json_s)) //2047 node
#endif

#ifndef INT_MAX
#define INT_MAX ((s32)0x7fffffff)
#endif

#ifndef INT_MIN
#define INT_MIN ((s32)0x80000000)
#endif

#ifndef UINT_MAX
#define UINT_MAX ((u32)0xffffffff)
#endif

#ifndef UINT_MIN
#define UINT_MIN (0)
#endif

#ifndef INT64_MAX
#define INT64_MAX ((s64)0x7fffffffffffffff)
#endif

#ifndef INT64_MIN
#define INT64_MIN ((s64)0x8000000000000000)
#endif

#ifndef UINT64_MAX
#define UINT64_MAX ((u64)0xffffffffffffffff)
#endif

#ifndef UINT64_MIN
#define UINT64_MIN ((u64)0)
#endif

#define json_err(fmt, args...) printf("\033[01;0;31mjson err\033[00;0;0m:" fmt, ##args)
#define json_log printf
#define pr printf

// can use comment like C stype
#define JSON_SUPPORT_COMMENT           (1)

// can use ',' at end of object like {"key1":[1,2,], "key2":"value",},
#define JSON_SUPPORT_TAIL_COMMA        (1)

// can fast get child object's(or array's) size json_get_obj_size()/json_get_array_size()
#define JSON_SUPPORT_CHILD_SIZE        (0)

#define JSON_ERR_TRACE
#define JSON_TRACE_LINE 4

#ifndef BIT
#define BIT(nr)                 (1 << (nr))
#endif

#define JSON_INVALID (0)
#define JSON_NUMBER  BIT(0)
#define JSON_STRING  BIT(1)
#define JSON_OBJECT  BIT(2)
#define JSON_ARRAY   BIT(3)
//#define JSON_BOOL    (1 << 4) // regard bool as number true=1 false=0

struct json_s {
	json_ssize_t type;
	json_ssize_t key;   //key  index in json string
	json_ssize_t value; //value index in json string
	json_ssize_t next;  //brother node in json array index
#if (JSON_SUPPORT_CHILD_SIZE)
	json_ssize_t child_size;
#endif
};

#define JSON_STATUS_NO_FILE 1
#define JSON_STATUS_ERROR   2
#define JSON_STATUS_OK      3

struct json_parse_s {
	json_ssize_t json_max; //json array max size
	json_ssize_t json_cnt; //real json array used
	json_ssize_t js_max;   //json string max, short for 32k, int for 2g
	json_ssize_t js_len;   //real json string length
	json_ssize_t pos;      //for parse time recording
	unsigned char status;
	char *js;              //json string
	struct json_s *root;   //json array
	struct json_s *(*alloc)(struct json_parse_s *jsp);
};

s32 json_parse_object(struct json_parse_s *jsp, char *input, s32 limit, s32 *child_size);
s32 json_parse_array(struct json_parse_s *jsp, char *input, s32 limit, s32 *child_size);

/*=json parse====================================================================================*/
/* will pre alloc json nodes buffer and json string buffer
 * @js_max: json string buffer len
 * @json_max: max json node number
 * return 0-success, other-fail
 */
s32 json_init(struct json_parse_s *jsp, s32 js_max, s32 json_max);
void json_deinit(struct json_parse_s *jsp);

/* parse json file, translate key to json node, and value to json string
 * @input: json file buffer
 * @limit: json file len
 * return 0-success, other-fail
 */
struct json_s *json_parse(struct json_parse_s *jsp, char *input, s32 limit);

static inline int json_parse_ok(struct json_parse_s *jsp)
{
	return jsp->status == JSON_STATUS_OK;
}

struct json_s *json_get_object_child(struct json_parse_s *jsp, struct json_s *parent,
				     const char *name);
struct json_s *json_get_object_child_by_id(struct json_parse_s *jsp, struct json_s *parent, int id);

struct json_s *json_get_array_child(struct json_parse_s *jsp, struct json_s *parent, s32 id);
struct json_s *json_get_nd_array_child(struct json_parse_s *jsp, struct json_s *json,
	s32 nid, s32 ids[]);

/* get child node count of a JSON_OBJECT type node
 * return >= 0 for success, < 0 for fail
 */
s32 json_get_object_size(struct json_parse_s *jsp, struct json_s *json);

/* get child node count of a JSON_ARRAY type node
 * return >= 0 for success, < 0 for fail
 */
s32 json_get_array_size(struct json_parse_s *jsp, struct json_s *json);

const char *json_get_key(struct json_parse_s *jsp, struct json_s *json);
const char *json_get_str(struct json_parse_s *jsp, struct json_s *json);
s32 json_get_s32(struct json_parse_s *jsp, struct json_s *json, s32 *num);
s32 json_get_u32(struct json_parse_s *jsp, struct json_s *json, u32 *num);

/*=json util=====================================================================================*/
static inline int __json_is_string(struct json_s *json)
{
	return (json && json->type == JSON_STRING);
}

static inline int __json_is_number(struct json_s *json)
{
	return (json && json->type == JSON_NUMBER);
}

static inline int __json_is_string_or_number(struct json_s *json)
{
	return (json && ((json->type == JSON_STRING) || (json->type == JSON_NUMBER)));
}

static inline int __json_is_object(struct json_s *json)
{
	return (json && json->type == JSON_OBJECT);
}

static inline int __json_is_array(struct json_s *json)
{
	return (json && json->type == JSON_ARRAY);
}

void json_obj_dump(char *js, struct json_s *json_array, struct json_s *json, s32 indent);
void json_array_dump(char *js, struct json_s *json_array, struct json_s *json, s32 indent);

void json_dump(struct json_parse_s *jsp, struct json_s *json);
void json_dump_path(struct json_parse_s *jsp, const char *path);

/* convert path to json node
 * @start: start json node
 * @path: target node path, it can be relative path of start or absolute path of root
 * eg:
 * {
 *     "obj1": {"obj1_1":{"str1": "string1", "str2": "string2"}},
 *     "obj2": {"obj2_1":{"num1": 123456789, "num2": 987654321}},
 *     "arr1": ["hello", "world", 1, 2]
 * }
 * struct json_s *obj1 = json_path_to_node(jsp, NULL, "/obj1");
 * struct json_s *str1 = json_path_to_node(jsp, obj1, "obj1_1/str1");
 * struct json_s *str2 = json_path_to_node(jsp, NULL, "/obj1_1/str2");
 * return target node-success, NULL-fail
 */
struct json_s *json_path_to_node(struct json_parse_s *jsp, struct json_s *start, const char *path);

/* get path key value
 * eg: json_get_key_str(jsp, NULL, "/obj1/obj1_child1/key1", NULL);
 * eg: json_get_key_str(jsp, obj1_node, "obj1_child1/key1", NULL);
 **/
const char *json_get_key_str(struct json_parse_s *jsp, struct json_s *start,
			     const char *key, char *dft);

/* get child string node value of object node
 * eg: json_get_obj_str(jsp, obj1_1_node, "str1", NULL);
 */
const char *json_get_obj_str(struct json_parse_s *jsp, struct json_s *json,
			     const char *name, char *dft);

/* get child string value of array node
 * eg: json_get_arr_str(jsp, arr1_node, 0, NULL);
 */
const char *json_get_arr_str(struct json_parse_s *jsp, struct json_s *json, s32 id, char *dft);

s32 json_get_key_s32(struct json_parse_s *jsp, struct json_s *start, const char *key, s32 dft);
s32 json_get_obj_s32(struct json_parse_s *jsp, struct json_s *json, const char *name, s32 dft);
s32 json_get_arr_s32(struct json_parse_s *jsp, struct json_s *json, s32 id, s32 dft);

u32 json_get_key_u32(struct json_parse_s *jsp, struct json_s *start, const char *key, u32 dft);
u32 json_get_obj_u32(struct json_parse_s *jsp, struct json_s *json, const char *name, u32 dft);
u32 json_get_arr_u32(struct json_parse_s *jsp, struct json_s *json, s32 id, u32 dft);

#ifdef __cplusplus
}
#endif

#endif
