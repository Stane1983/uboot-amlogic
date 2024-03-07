/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014-2015 Freescale Semiconductor
 *
 * Extracted from armv8/start.S
 */

#include <config.h>
#include <linux/linkage.h>
#include <asm/gic.h>
#include <asm/macro.h>
#include <asm/u-boot.h>

	.align 2
ENTRY(secondary_boot_func)
	/*
	 * MPIDR_EL1 Fields:
	 * MPIDR[1:0] = AFF0_CPUID <- Core ID (0,1)
	 * MPIDR[7:2] = AFF0_RES
	 * MPIDR[15:8] = AFF1_CLUSTERID <- Cluster ID (0,1,2,3)
	 * MPIDR[23:16] = AFF2_CLUSTERID
	 * MPIDR[24] = MT
	 * MPIDR[29:25] = RES0
	 * MPIDR[30] = U
	 * MPIDR[31] = ME
	 * MPIDR[39:32] = AFF3
	 *
	 * Linear Processor ID (LPID) calculation from MPIDR_EL1:
	 * (We only use AFF0_CPUID and AFF1_CLUSTERID for now
	 * until AFF2_CLUSTERID and AFF3 have non-zero values)
	 *
	 * LPID = MPIDR[15:8] | MPIDR[1:0]
	 */
	mrs x0, mpidr_el1
	and x1, x0, #1<<24
	cmp x1, #0
	beq 1f
	lsr x0, x0, #8
1:
	ubfx x1, x0, #8, #8
	ubfx x0, x0, #0, #8
	lsl x1, x1, 2             /* cluster0 has 4 cores*/
	add x0, x0, x1
	mov x2, x0
 	bl secondary_bootup_f
	mov x0, x2
 	bl get_smp_sp
	bic	sp, x0, #0xf	/* 16-byte alignment for ABI compliance */

	mrs	x0, CurrentEL
	cmp	x0, #CurrentEL_EL2
	b.ne	2f
	ldr	x0, =(HCR_EL2_RW_AARCH64 | HCR_TGE)
	mrs x1, hcr_el2
	orr x1, x1, x0
	msr hcr_el2, x1
2:
	bl secondary_start
ENDPROC(secondary_boot_func)

.align 3
.globl	CPU_RELEASE_ADDR
CPU_RELEASE_ADDR:
	.quad secondary_boot_func

