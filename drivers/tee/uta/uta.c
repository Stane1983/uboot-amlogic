/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include<common.h>
#include<linux/types.h>
#include<tee.h>

struct tee_optee_uta optee_uta_tb[] = {
	{}
};

int tee_optee_get_uta(struct tee_optee_uta **uta, size_t *uta_num)
{
	*uta = optee_uta_tb;
	*uta_num = ARRAY_SIZE(optee_uta_tb);

	return 0;
}
