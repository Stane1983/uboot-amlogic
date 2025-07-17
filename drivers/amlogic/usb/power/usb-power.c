// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/pm.h>

__weak void set_usb_power_off(void)
{
	printf("[%s]: weak func...\n", __func__);
}

__weak void set_usb_power_on(void)
{
	printf("[%s]: weak func...\n", __func__);
}

int aml_usb_suspend(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("usb suspend: %s\n", pm->name);
	set_usb_power_off();

	return 0;
}

int aml_usb_resume(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("usb resume: %s\n", pm->name);
	set_usb_power_on();

	return 0;
}

int aml_usb_poweroff(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("usb poweroff: %s\n", pm->name);
	aml_usb_suspend(pm_ops);
	return 0;
}

void usb_power_init(void)
{
	struct dev_pm_ops *pm_ops = NULL;

	pm_ops = dev_register_pm("usb_ops", &aml_usb_suspend, &aml_usb_resume, &aml_usb_poweroff, 0);
}
