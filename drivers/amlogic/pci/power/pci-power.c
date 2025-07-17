// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/pm.h>

__weak void set_pci_power_off(void)
{
}

__weak void set_pci_power_on(void)
{
}

int aml_pci_suspend(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("pci suspend: %s\n", pm->name);
	set_pci_power_off();
	return 0;
}

int aml_pci_resume(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("pci resume: %s\n", pm->name);
	set_pci_power_on();
	return 0;
}

int aml_pci_poweroff(void *pm_ops)
{
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	printf("pci poweroff: %s\n", pm->name);
	aml_pci_suspend(pm_ops);
	return 0;
}

void pci_power_init(void)
{
	struct dev_pm_ops *pm_ops = NULL;

	pm_ops = dev_register_pm("pci_ops", &aml_pci_suspend, &aml_pci_resume, &aml_pci_poweroff, 0);
}
