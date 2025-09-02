// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <stdio.h>
#include <command.h>
#include <amlogic/media/vpp/vpp.h>

static char name_soc[] = "t6w";
int vpp_suspend(void *dev_info)
{
	printf("[%s] %s\n", name_soc, __func__);
	return 0;
}

int vpp_resume(void *dev_info)
{
	printf("[%s] %s\n", name_soc, __func__);
	vpp_init();
	run_command("run init_display", 0);
	return 0;
}

int vpp_poweroff(void *dev_info)
{
	printf("[%s] %s\n", name_soc, __func__);
	return 0;
}

