/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __MODEL_H__
#define __MODEL_H__
#include "model_log.h"

#define CS_MODEL_NAME_ITEM_NAME                   "model_name"
#define CS_MODEL1_NAME_ITEM_NAME                  "model1_name"
#define CS_MODEL2_NAME_ITEM_NAME                  "model2_name"

int model_check_ukey_size(const char *key_name);
unsigned char *model_read_ukey_data(const char *key_name, int *data_size);
int model_write_ukey_data(const char *key_name, unsigned char *data_buf, int buf_size);
#endif
