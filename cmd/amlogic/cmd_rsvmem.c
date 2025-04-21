// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/secure_apb.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <asm/global_data.h>

#ifdef CONFIG_CMD_RSVMEM

#if defined(P_AO_SEC_GP_CFG3)
#define REG_RSVMEM_SIZE        P_AO_SEC_GP_CFG3
#define REG_RSVMEM_BL32_START  P_AO_SEC_GP_CFG4
#define REG_RSVMEM_BL31_START  P_AO_SEC_GP_CFG5
#elif defined(SYSCTRL_SEC_STATUS_REG15)
#define REG_RSVMEM_SIZE        SYSCTRL_SEC_STATUS_REG15
#define REG_RSVMEM_BL32_START  SYSCTRL_SEC_STATUS_REG16
#define REG_RSVMEM_BL31_START  SYSCTRL_SEC_STATUS_REG17
#endif

//#define RSVMEM_DEBUG_ENABLE
#ifdef RSVMEM_DEBUG_ENABLE
#define rsvmem_dbg(fmt...)	printf("[rsvmem] "fmt)
#else
#define rsvmem_dbg(fmt...)
#endif
#define rsvmem_info(fmt...)	printf("[rsvmem] "fmt)
#define rsvmem_err(fmt...)	printf("[rsvmem] "fmt)

#ifndef DTB_BIND_KERNEL
#define RSVMEM_NONE -1
#define RSVMEM_RESERVED	0
#define RSVMEM_CMA	1
#define BL31_SHARE_MEM_SIZE  0x100000
#ifndef BL32_SHARE_MEM_SIZE
#define BL32_SHARE_MEM_SIZE  0x400000
#endif

static int do_rsvmem_kernel_dts_check(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = 0;
	char *fdt_str = NULL;
	void *fdt = NULL;
	int node, subnode;
	int addr_cells, size_cells;
	u64 r_addr, r_size, alloc_size;
	const fdt32_t *prop;
	int len, i, conflict_count = 0;
	u64 addr, size;
	const char *status;
	char cmdbuf[128];
	const char *name;
	int count, conflict_flag = 0;
	ulong fdt_addr;

	if (argc < 3)
		return CMD_RET_USAGE;

	addr = simple_strtoull(argv[1], NULL, 16);
	size = simple_strtoull(argv[2], NULL, 16);

	printf("\n[rsvmem] Check reserved-memory conflict for [0x%llx ~ 0x%llx]\n",
			addr, addr + size);

	fdt_str = env_get("fdtaddr");
	if (!fdt_str) {
		printf("[rsvmem] Error! get fdtaddr NULL!\n");
		return -1;
	}

	fdt_addr = simple_strtoul(fdt_str, NULL, 16);
	fdt = (void *)fdt_addr;

	sprintf(cmdbuf, "fdt addr %s;", fdt_str);
	rsvmem_dbg("CMD: %s\n", cmdbuf);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("[rsvmem] Error! fdt addr error.\n");
		return -2;
	}

	node = fdt_path_offset(fdt, "/reserved-memory");
	if (node < 0) {
		printf("[rsvmem] No /reserved-memory node found.\n");
		return CMD_RET_FAILURE;
	}

	addr_cells = fdt_address_cells(fdt, node);
	if (addr_cells < 0)
		addr_cells = fdt_address_cells(fdt, 0);

	size_cells = fdt_size_cells(fdt, node);
	if (size_cells < 0)
		size_cells = fdt_size_cells(fdt, 0);

	printf("-----------------------------------------------");
	printf("----------------------------------------------\n");
	printf("| %-28s | %-8s | %-25s | %-8s | %-8s |\n",
			"Node", "Type", "Mem Range", "Size(MB)", "Conflict");
	printf("-----------------------------------------------");
	printf("----------------------------------------------\n");

	fdt_for_each_subnode(subnode, fdt, node) {
		name = fdt_get_name(fdt, subnode, NULL);
		conflict_flag = 0;

		// disabled
		status = fdt_getprop(fdt, subnode, "status", NULL);
		if (status && !strcmp(status, "disabled"))
			continue;

		// reg
		prop = fdt_getprop(fdt, subnode, "reg", &len);
		if (prop && len >= (addr_cells + size_cells) * sizeof(fdt32_t)) {
			count = len / ((addr_cells + size_cells) * sizeof(fdt32_t));
			for (i = 0; i < count; i++) {
				r_addr = fdt_read_number(prop, addr_cells);
				prop += addr_cells;
				r_size = fdt_read_number(prop, size_cells);
				prop += size_cells;
				if (!r_size)
					break;

				if ((addr + size > r_addr) && (addr < r_addr + r_size)) {
					conflict_count++;
					conflict_flag = 1;
				}

				printf("| %-28s | %-8s | [0x%08llx ~ 0x%08llx] | %-8lld | %-8s |\n",
						name, "reg", r_addr, r_addr + r_size,
						r_size / 1024 / 1024,
						conflict_flag ? "Y" : "N");
			}
			continue;
		}

		// alloc-ranges
		prop = fdt_getprop(fdt, subnode, "alloc-ranges", &len);
		if (prop && len >= (addr_cells + size_cells) * sizeof(fdt32_t)) {
			count = len / ((addr_cells + size_cells) * sizeof(fdt32_t));
			for (i = 0; i < count; i++) {
				r_addr = fdt_read_number(prop, addr_cells);
				prop += addr_cells;
				r_size = fdt_read_number(prop, size_cells);
				prop += size_cells;
				if (!r_size)
					break;

				prop = fdt_getprop(fdt, subnode, "size", &len);
				if (!prop)
					break;

				alloc_size = fdt_read_number(prop, size_cells);
				if (!alloc_size)
					break;

				if ((addr + size > (r_addr + r_size - alloc_size)) &&
					(addr < r_addr + r_size)) {
					conflict_count++;
					conflict_flag = 1;
				}

				printf("| %-28s | %-8s | [0x%08llx ~ 0x%08llx] | %-8lld | %-8s |\n",
						name, "alloc", r_addr, r_addr + r_size,
						alloc_size / 1024 / 1024,
						conflict_flag ? "Y" : "N");
			}
			continue;
		}

		// cma-size
		prop = fdt_getprop(fdt, subnode, "size", &len);
		if (prop && len >= size_cells * sizeof(fdt32_t)) {
			count = len / (size_cells * sizeof(fdt32_t));
			for (i = 0; i < count; i++) {
				r_size = fdt_read_number(prop, size_cells);
				if (!r_size)
					break;

				prop += size_cells;
				printf("| %-28s | %-8s | [  %-8s - 0x%08llx] | %-8lld | %-8s |\n",
						name, "CMApool", "ddr_end", r_size,
						r_size / 1024 / 1024, "-");
			}
			continue;
		}
	}

	printf("-----------------------------------------------");
	printf("----------------------------------------------\n");

	if (conflict_count > 0) {
		printf("[rsvmem] Error! Found %d conflict(s) dts node.\n\n", conflict_count);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_rsvmem_check(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	unsigned int data = 0;
	unsigned int bl31_rsvmem_size = 0;
	unsigned int bl32_rsvmem_size = 0;
	unsigned int bl31_rsvmem_start = 0;
	unsigned int bl32_rsvmem_start = 0;
	unsigned int alignment = 0;
#ifndef CONFIG_RSVMEM_ALIGNMENT_1M
	unsigned int alignment_temp = 0;
#endif
	unsigned int secure_monitor_size = 0;
	unsigned int secure_monitor_size_final = 0;
	unsigned int ramoops_start = 0;
	int reg_flag = 0;
	char cmdbuf[128];
	char *fdtaddr = NULL;
	int ret = 0;
	char *temp_env = NULL;
	int rsvmemtype = RSVMEM_NONE;
	unsigned int aarch32 = 0;

	rsvmem_dbg("reserved memory check!\n");
	data = readl(REG_RSVMEM_SIZE);
	/* workaround for bl3x size */
	if ((data >> 16) & 0xff) {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 16;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 16;
	} else {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 10;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 10;
	}
	bl31_rsvmem_start = readl(REG_RSVMEM_BL31_START);
	bl32_rsvmem_start = readl(REG_RSVMEM_BL32_START);

	fdtaddr = env_get("fdtaddr");
	if (fdtaddr == NULL) {
		rsvmem_err("get fdtaddr NULL!\n");
		return -1;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt addr %s;", fdtaddr);
	rsvmem_dbg("CMD: %s\n", cmdbuf);
	ret = run_command(cmdbuf, 0);
	if (ret != 0 ) {
		rsvmem_err("fdt addr error.\n");
		return -2;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_env / \\#address-cells;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get size #address-cells failed.\n");
		return -2;
	}
	temp_env = env_get("temp_env");
	//if (temp_env && !strcmp(temp_env, "0x01000000"))
	if (temp_env && !strcmp(temp_env, "0x00000001"))
		aarch32 = 1;
#ifdef CONFIG_RSVMEM_ALIGNMENT_1M
	alignment = 0x100000;
	memset(cmdbuf, 0, sizeof(cmdbuf));
	if (aarch32)
		sprintf(cmdbuf,
			"fdt set /reserved-memory/linux,secmon alignment <0x%x>;",
			alignment);
	else
		sprintf(cmdbuf,
			"fdt set /reserved-memory/linux,secmon alignment <0x0 0x%x>;",
			alignment);
	run_command(cmdbuf, 0);

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_reusable /reserved-memory/linux,secmon reusable;");
	ret = run_command(cmdbuf, 0);
	if (!ret) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		sprintf(cmdbuf, "fdt rm /reserved-memory/linux,secmon reusable");
		run_command(cmdbuf, 0);
	}
#else
	/* Get alignment size
	 * If arm64, alignment has 2 parameters
	 * The second parameter need convert big-endian to little-endian
	 */
	alignment = 0x400000;
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_alignment /reserved-memory/linux,secmon alignment;");
	ret = run_command(cmdbuf, 0);
	if (!ret) {
		temp_env = env_get("temp_alignment");
		alignment_temp = simple_strtoul(temp_env, NULL, 16);
		if (aarch32) {
			alignment = alignment_temp;
		} else {
			alignment = (alignment_temp & 0xff) << 24;
			alignment |= (alignment_temp & 0xff00) << 8;
			alignment |= (alignment_temp & 0xff0000) >> 8;
			alignment |= (alignment_temp & 0xff000000) >> 24;
		}
	}
#endif

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value env_compatible /reserved-memory/linux,secmon compatible;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get prop fail.\n");
		return -2;
	}
	temp_env = env_get("env_compatible");
	if (strcmp(temp_env, "shared-dma-pool") == 0)
		rsvmemtype = RSVMEM_CMA;
	else if (strcmp(temp_env, "amlogic, aml_secmon_memory") == 0)
		rsvmemtype = RSVMEM_RESERVED;
	else
		rsvmemtype = RSVMEM_NONE;
	if (rsvmemtype == RSVMEM_NONE) {
		rsvmem_err("env set fail.\n");
		return -2;
	}
	run_command("setenv env_compatible;", 0);

	secure_monitor_size = ((bl31_rsvmem_size + alignment - 1) / alignment) * alignment;
	secure_monitor_size_final = bl31_rsvmem_size + bl32_rsvmem_size + alignment - 1;
	secure_monitor_size_final = (secure_monitor_size_final / alignment) * alignment;
	ramoops_start = bl31_rsvmem_start + bl31_rsvmem_size + bl32_rsvmem_size + alignment - 1;
	ramoops_start = (ramoops_start / alignment) * alignment;

	if ((bl31_rsvmem_size > 0) && (bl31_rsvmem_start > 0)) {
		if (rsvmemtype == RSVMEM_RESERVED) {
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
					bl31_rsvmem_start, bl31_rsvmem_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
					bl31_rsvmem_start, bl31_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set addr error.\n");
				return -3;
			}
		}
		if (rsvmemtype == RSVMEM_CMA) {
			/* Check parameter reg, add for linux 5.15 and before */
			reg_flag = 0;
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf,
					"fdt get value temp_rsv_reg /reserved-memory/linux,secmon reg;");
			ret = run_command(cmdbuf, 0);
			if (!ret) {
				reg_flag = 1;
				if (aarch32)
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
				else
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret) {
					rsvmem_err("bl31 reserved memory set reg error.\n");
					return -3;
				}
			}

			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x%x>;",
						secure_monitor_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x0 0x%x>;",
						secure_monitor_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set size error.\n");
				/*
				 * If reg exist, to modify bl32,
				 * need not return if modify size failed
				 */
				if (!reg_flag)
					return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set alloc-ranges error.\n");
				/*
				 * If reg exist, to modify bl32,
				 * need not return if modify size failed
				 */
				if (!reg_flag)
					return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x%x>;",
						bl31_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set reserve_mem_size error.\n");
				return -3;
			}
		}
	}

	if ((bl32_rsvmem_size > 0) && (bl32_rsvmem_start > 0)) {
		if ((rsvmemtype == RSVMEM_RESERVED)
				|| ((bl31_rsvmem_start + bl31_rsvmem_size != bl32_rsvmem_start)
					&& (rsvmemtype == RSVMEM_CMA))) {
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos status okay;");
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl32 reserved memory set status error.\n");
				return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos reg <0x%x 0x%x>;",
					bl32_rsvmem_start, bl32_rsvmem_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos reg <0x0 0x%x 0x0 0x%x>;",
					bl32_rsvmem_start, bl32_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl32 reserved memory set addr error.\n");
				return -3;
			}
		}
		if ((bl31_rsvmem_start + bl31_rsvmem_size == bl32_rsvmem_start)
				&& (rsvmemtype == RSVMEM_CMA)) {
			/* Modify parameter reg, add for linux 5.15 and before */
			if (reg_flag) {
				if (aarch32)
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				else
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret) {
					rsvmem_err("bl32 reserved memory set reg error.\n");
					return -3;
				}
			}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				if (aarch32)
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x%x>;",
						secure_monitor_size_final);
				else
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x0 0x%x>;",
						secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set size error.\n");
					/*
					 * If reg exist, to modify reserve_mem_size,
					 * need not return if modify size failed
					 */
					if (!reg_flag)
						return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				if (aarch32)
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				else
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set alloc-ranges error.\n");
					/*
					 * If reg exist, to modify reserve_mem_size,
					 * need not return if modify size failed
					 */
					if (!reg_flag)
						return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x%x>;",
						bl31_rsvmem_size + bl32_rsvmem_size);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set reserve_mem_size error.\n");
					return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt get value ramoops_reg /reserved-memory/ramoops reg;");
				if (run_command(cmdbuf, 0) == 0) {
					memset(cmdbuf, 0, sizeof(cmdbuf));
					if (aarch32)
						sprintf(cmdbuf, "fdt set /reserved-memory/ramoops reg <0x%x 0x%x>;",
								ramoops_start, 0x100000);
					else
						sprintf(cmdbuf, "fdt set /reserved-memory/ramoops reg <0x0 0x%x 0x0 0x%x>;",
								ramoops_start, 0x100000);

					rsvmem_dbg("CMD: %s\n", cmdbuf);
					ret = run_command(cmdbuf, 0);
					if (ret != 0 ) {
						rsvmem_err("fdt set /reserved-memory/ramoops reg  error.\n");
						return -3;
					}
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt get value secmon_clear_range /secmon clear_range;");
				if (run_command(cmdbuf, 0) == 0) {
					memset(cmdbuf, 0, sizeof(cmdbuf));
					sprintf(cmdbuf, "fdt set /secmon clear_range <0x%x 0x%x>;",
							bl31_rsvmem_start + BL31_SHARE_MEM_SIZE , bl31_rsvmem_size + bl32_rsvmem_size
							- BL31_SHARE_MEM_SIZE - BL32_SHARE_MEM_SIZE);
					rsvmem_dbg("CMD: %s\n", cmdbuf);
					ret = run_command(cmdbuf, 0);
					if (ret != 0 ) {
						rsvmem_err("bl32 reserved memory set clear_range error.\n");
						return -3;
					}
				}
		}
	}

	return ret;
}

static int do_rsvmem_dump(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	unsigned int data = 0;
	unsigned int bl31_rsvmem_size = 0;
	unsigned int bl32_rsvmem_size = 0;
	unsigned int bl31_rsvmem_start = 0;
	unsigned int bl32_rsvmem_start = 0;

	rsvmem_info("reserved memory:\n");
	data = readl(REG_RSVMEM_SIZE);
	/* workaround for bl3x size */
	if ((data >> 16) & 0xff) {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 16;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 16;
	} else {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 10;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 10;
	}
	bl31_rsvmem_start = readl(REG_RSVMEM_BL31_START);
	bl32_rsvmem_start = readl(REG_RSVMEM_BL32_START);

	rsvmem_info("bl31 reserved memory start: 0x%08x\n", bl31_rsvmem_start);
	rsvmem_info("bl31 reserved memory size:  0x%08x\n", bl31_rsvmem_size);
	rsvmem_info("bl32 reserved memory start: 0x%08x\n", bl32_rsvmem_start);
	rsvmem_info("bl32 reserved memory size:  0x%08x\n", bl32_rsvmem_size);

	return 0;
}

static cmd_tbl_t cmd_rsvmem_sub[] = {
	U_BOOT_CMD_MKENT(check, 2, 0, do_rsvmem_check, "", ""),
	U_BOOT_CMD_MKENT(dump, 2, 0, do_rsvmem_dump, "", ""),
	U_BOOT_CMD_MKENT(kernel, 4, 0, do_rsvmem_kernel_dts_check, "", ""),
};
#endif

static int do_rsvmem(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#ifdef DTB_BIND_KERNEL
	rsvmem_err("no check for rsvmem, should check int kernel\n");
	return 0;
#else
	cmd_tbl_t *c;

	/* Strip off leading 'rsvmem' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_rsvmem_sub[0], ARRAY_SIZE(cmd_rsvmem_sub));

	if (c) {
		int ret = c->cmd(cmdtp, flag, argc, argv);

		if (ret < 0) {
			/*only negative -1 is identifid as fail */
			ret = -1;
		}
		return ret;
	} else {
		return CMD_RET_USAGE;
	}
#endif
}

U_BOOT_CMD(
	rsvmem, 4, 0, do_rsvmem,
	"reserve memory",
	"rsvmem check                   - check reserved memory\n"
	"rsvmem dump                    - dump reserved memory\n"
	"rsvmem kernel <addr> <size>    - check if region conflicts (kernel dts)\n"
);
#endif
