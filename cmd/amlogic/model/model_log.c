// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include "model_log.h"

static int model_log_level = MODEL_LOG_DEFAULT;

void model_set_log_level(int log_level)
{
	model_log_level = log_level;
}

int model_get_log_level(void)
{
	return model_log_level;
}
