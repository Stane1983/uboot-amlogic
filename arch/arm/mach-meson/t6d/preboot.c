// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <asm/amlogic/arch/io.h>
#include <command.h>
#include <malloc.h>

void check_powerkey(void)
{
	char *preboot_mode = env_get("preboot_mode");
	const char *prefix = "androidboot.preboot_mode=";
	const char *mode = "0";

	if (!preboot_mode || !strcmp(preboot_mode, "0"))
		return;

	run_command("irkey_wakeup 4", 0);
	run_command("watchkey test 0", 0);

	char *key_status = env_get("ir_preboot_key");
	char *adc_status = env_get("wk0_test");

	if ((key_status && strcmp(key_status, "0x0")) ||
		(adc_status && strcmp(adc_status, "no"))) {
		run_command("lcd mute 0", 0);
		run_command("lcd bl on", 0);
		mode  = "-1";
		printf("%s : exec lcd bl power on\n", __func__);
	} else {
		mode = "1";
	}
	char *bootargs = env_get("bootconfig");
	int nlen = strlen(prefix) + strlen(bootargs) + strlen(mode) + 2;
	char *pnewbootargs = malloc(nlen);

	if (pnewbootargs) {
		memset((void *)pnewbootargs, 0, nlen);
		sprintf(pnewbootargs, "%s %s%s", bootargs, prefix, mode);
		env_set("bootconfig", pnewbootargs);
		free(pnewbootargs);
		pnewbootargs = NULL;
	}
}
