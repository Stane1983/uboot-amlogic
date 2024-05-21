// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <command.h>
#include <linux/arm-smccc.h>
#include <vsprintf.h>
#include <linux/errno.h>
#include <stdio.h>
#include <asm/arch/register.h>
#include <amlogic/cpu_id.h>

#define DMC_MON_RW			0x8200004A

unsigned long dmc_rw(uint64_t addr, uint64_t value, uint64_t rw)
{
	struct arm_smccc_res res;

	arm_smccc_smc(DMC_MON_RW, addr, value, rw, 0, 0, 0, 0, &res);

	return res.a0;
}

int do_dmc_rw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 4 || argc > 5)
		return cmd_usage(cmdtp);

	uint64_t addr = simple_strtoul(argv[1], NULL, 16);
	uint64_t value = simple_strtoul(argv[2], NULL, 16);
	uint64_t rw = simple_strtoul(argv[3], NULL, 16);
	uint64_t dmc_back;

	dmc_back = dmc_rw(addr, value, rw);

	printf("return value is %llx\n", dmc_back);

	return dmc_back;
}

U_BOOT_CMD(dmc_rw, 4, 0, do_dmc_rw,
	"dmc read write function",
	"\n"
	"dmc_rw fe037470 ffffffff 0 - show dmc read information\n"
	"dmc_rw fe037470 ffffffff 1 - show dmc write information\n"
);

static int dmc_sec_check(unsigned int dmc_number)
{
#define DMC_READ	0
#define DMC_WRITE	1
	cpu_id_t cpu_id = get_cpu_id();
	unsigned int offset = 0;

	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_T3X)
		offset = 0x4000 * dmc_number;
	else
		offset = 0x2000 * dmc_number;

	unsigned int dmc_vio_status = dmc_rw(DMC_SEC_STATUS + offset, 0, DMC_READ);
	unsigned int dmc_vio_reg0 = dmc_rw(DMC_VIO_ADDR0 + offset, 0, DMC_READ);
	unsigned int dmc_vio_reg1 = dmc_rw(DMC_VIO_ADDR1 + offset, 0, DMC_READ);
	unsigned int dmc_vio_reg2 = dmc_rw(DMC_VIO_ADDR2 + offset, 0, DMC_READ);
	unsigned int dmc_vio_reg3 = dmc_rw(DMC_VIO_ADDR3 + offset, 0, DMC_READ);
	unsigned int dmc_vio_reg4 = 0;
	unsigned int dmc_vio_reg5 = 0;
	unsigned int dmc_vio_reg6 = 0;
	unsigned int dmc_vio_reg7 = 0;

	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_GXBB ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_GXTVBB ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_GXL ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_GXM ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_TXL ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_TXLX ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_AXG ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_GXLX ||
	cpu_id.family_id == MESON_CPU_MAJOR_ID_TXHD) {
		dmc_vio_reg4 = dmc_rw(DMC_VIO_ADDR3 + 4 + offset, 0, DMC_READ);
		dmc_vio_reg5 = dmc_rw(DMC_VIO_ADDR3 + 8 + offset, 0, DMC_READ);
		dmc_vio_reg6 = dmc_rw(DMC_VIO_ADDR3 + 12 + offset, 0, DMC_READ);
		dmc_vio_reg7 = dmc_rw(DMC_VIO_ADDR3 + 16 + offset, 0, DMC_READ);
	}

	if (dmc_vio_status || dmc_vio_reg0 || dmc_vio_reg1 || dmc_vio_reg2 || dmc_vio_reg3 ||
	    dmc_vio_reg4 || dmc_vio_reg5 || dmc_vio_reg6 || dmc_vio_reg7) {
		printf("WARNING: DMC%d SEC VIOLATION CHECK FAILED!!!\n", dmc_number);
		printf("DMC%d_SEC_STATUS:%d\n", dmc_number, dmc_vio_status);
		printf("DMC%d_VIO_ADDR0:%x\n", dmc_number, dmc_vio_reg0);
		printf("DMC%d_VIO_ADDR1:%x\n", dmc_number, dmc_vio_reg1);
		printf("DMC%d_VIO_ADDR2:%x\n", dmc_number, dmc_vio_reg2);
		printf("DMC%d_VIO_ADDR3:%x\n", dmc_number, dmc_vio_reg3);

		if (cpu_id.family_id == MESON_CPU_MAJOR_ID_GXBB ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_GXTVBB ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_GXL ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_GXM ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_TXL ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_TXLX ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_AXG ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_GXLX ||
		cpu_id.family_id == MESON_CPU_MAJOR_ID_TXHD) {
			printf("DMC%d_VIO_ADDR4:%x\n", dmc_number, dmc_vio_reg4);
			printf("DMC%d_VIO_ADDR5:%x\n", dmc_number, dmc_vio_reg5);
			printf("DMC%d_VIO_ADDR6:%x\n", dmc_number, dmc_vio_reg6);
			printf("DMC%d_VIO_ADDR7:%x\n", dmc_number, dmc_vio_reg7);
		}
	} else {
		printf("DMC%d SEC VIOLATION CHECK PASS.\n", dmc_number);
	}
	dmc_rw(DMC_SEC_STATUS + offset, 0x3, DMC_WRITE);

	return 0;
}

int do_dmc_sec_vio_check(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cpu_id_t cpu_id = get_cpu_id();
	unsigned int dmc_number = 1;
	unsigned int i;

	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_T3 ||
	    cpu_id.family_id == MESON_CPU_MAJOR_ID_T7 ||
	    cpu_id.family_id == MESON_CPU_MAJOR_ID_T5M ||
	    cpu_id.family_id == MESON_CPU_MAJOR_ID_T3X) {
		dmc_number = 2;
	} else if (cpu_id.family_id == MESON_CPU_MAJOR_ID_P1 ||
		   cpu_id.family_id == MESON_CPU_MAJOR_ID_S5) {
		dmc_number = 4;
	}

	for (i = 0; i < dmc_number; i++)
		dmc_sec_check(i);

	return 0;
}

U_BOOT_CMD(dmc_vio_check, 1, 0, do_dmc_sec_vio_check,
	"dmc sec violation reg check",
	"\n"
	"dmc_vio_check - show dmc sec vio reg check information\n"
);
