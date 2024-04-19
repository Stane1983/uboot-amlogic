// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "include/v3_tool_def.h"
#include <mmc.h>
#include <wdt.h>
#include <amlogic/cpu_id.h>
#include <asm/global_data.h>

DECLARE_GLOBAL_DATA_PTR;

#ifndef BOOT_DEVICE_USB
#define BOOT_DEVICE_SD                  4
#define BOOT_DEVICE_USB                 5
#endif// #ifndef BOOT_DEVICE_USB
extern void serial_initialize(void);
extern void board_init_mem(void);
extern int aml_v3_usbburning(unsigned timeout, unsigned pcToolWaitTime);

void __board_init_mem(void) {}
void board_init_mem(void) __attribute__((weak, alias("__board_init_mem")));

unsigned int _get_romcode_boot_id(void)
{
	unsigned int boot_id = 0;

	boot_id = readl(SYSCTRL_SEC_STATUS_REG2);
	FB_DBG("boot_id 0x%x\n", boot_id);
	boot_id = (boot_id>>4) & 0xf;
	FB_DBG("boot_id 1x%x\n", boot_id);

	return boot_id;
}

int is_boot_device_usb(void)
{
    return BOOT_DEVICE_USB == _get_romcode_boot_id();
}

static int is_bl1_usb_protocol_DNL(void)
{
	const cpu_id_t cpuid = get_cpu_id();
	const int familyid	 = cpuid.family_id;

	if (familyid < MESON_CPU_MAJOR_ID_G12A)
		return 0;//old cpu, only wcp
	if (familyid > MESON_CPU_MAJOR_ID_C2 || familyid == MESON_CPU_MAJOR_ID_SC2)
		return 1;//new cpu, only dnl

	switch (familyid) {
	case MESON_CPU_MAJOR_ID_A1:
	case MESON_CPU_MAJOR_ID_C1:
	case MESON_CPU_MAJOR_ID_C2: {
#ifdef SYSCTRL_SEC_STATUS_REG1
		const unsigned int cfg9 = readl(SYSCTRL_SEC_STATUS_REG1);

		FB_DBG("SYSCTRL_SEC_STATUS_REG1 0x%x\n", cfg9);
		return !(cfg9 & (1U << 12));
#else
		FB_MSG("cpu 0x%x reg1 dnl\n", familyid);
		return 1;
#endif
	}
	case MESON_CPU_MAJOR_ID_G12A:
	case MESON_CPU_MAJOR_ID_G12B:
	case MESON_CPU_MAJOR_ID_SM1:
	case MESON_CPU_MAJOR_ID_TL1:
	case MESON_CPU_MAJOR_ID_TM2: {
#ifdef P_AO_SEC_SD_CFG9
		const unsigned int cfg9 = readl(P_AO_SEC_SD_CFG9);

		FB_DBG("cfg9 0x%x\n", cfg9);
		return cfg9 & (1U << 15);
#else
		FB_MSG("cpu 0x%x cfg9 dnl\n", familyid);
		return 1;
#endif// #ifdef P_AO_SEC_SD_CFG9
	}
	default:
		FB_MSG("cpu 0x%x dnl\n", familyid);
		return 1;
	}

	FB_MSG("id 0x%x dnl\n", familyid);
	return 1;
}


int aml_v3_factory_usb_burning(int flag, bd_t *bis)
{
	if (IS_ENABLED(CONFIG_DISABLE_AML_SERIAL))
		gd->flags |= GD_FLG_DISABLE_CONSOLE | GD_FLG_SILENT | GD_FLG_SERIAL_READY;

	if (!is_boot_device_usb())
		return 1;
	if (!is_bl1_usb_protocol_DNL())
		return 1;

	bis = bis;//avoid compiling warning
	if (!flag) {
		serial_initialize();//init for write memory
		if (IS_ENABLED(CONFIG_MMC)) {
			//FB_MSG("MMC init for dnl\n");
			//mmc_initialize(bis);
		}
		env_set_default("USB boot", 0);
		board_init_mem();
	}
	if (IS_ENABLED(CONFIG_SILENT_CONSOLE))
		gd->flags &= ~GD_FLG_SILENT;/* enable console output */

	if (IS_ENABLED(CONFIG_WDT))
		initr_watchdog();
	//pull down and sleep in bl2-->tpl,
	//to improve pc compatibility
	/*f_dwc_otg_pullup(0);*/
	udelay(2 * 1000 * 1000);

	v3tool_work_mode_set(V3TOOL_WORK_MODE_USB_PRODUCE);
	optimus_clear_ovd_register();//clear OVD register for normal reboot
	return aml_v3_usbburning(0, 0);
}

