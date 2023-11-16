// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/amlogic/arch/secure_apb.h>

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

#define RAMOOP_MEM_SIZE      0x100000
#define RSV_MEM_ALIGNMENT    0x400000

static int fdt_config_rsv_mem(unsigned int aarch32, unsigned int alignment,
			      unsigned int bl31_start, unsigned int rsv_sz)
{
	int ret = 0;
	char cmdbuf[128] = { 0 };
	unsigned int ramoops_start = 0;
	unsigned int rsv_sz_align = 0;

	if (bl31_start == 0 || rsv_sz == 0) {
		rsvmem_err("Bad args secure monitor start: %u, sz: %u.\n", bl31_start, rsv_sz);
		return -EINVAL;
	}

	if (alignment == 0)
		alignment = RSV_MEM_ALIGNMENT;

	rsv_sz_align = ((rsv_sz + alignment - 1) / alignment) * alignment;
	ramoops_start = bl31_start + rsv_sz_align;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon no-map;");
	if (run_command(cmdbuf, 0) != 0) {
		rsvmem_err("reserved memory set no-map failed.\n");
		return -ENODEV;
	}

	sprintf(cmdbuf, "fdt get value temp_rsv_reg /reserved-memory/linux,secmon reg;");
	ret = run_command(cmdbuf, 0);
	if (!ret) {
		if (aarch32)
			sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
				bl31_start, rsv_sz_align);
		else
			sprintf(cmdbuf,
				"fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
				bl31_start, rsv_sz_align);
		rsvmem_dbg("CMD: %s\n", cmdbuf);
		ret = run_command(cmdbuf, 0);
		if (ret) {
			rsvmem_err("reserved memory set reg error.\n");
			return -ENODEV;
		}
	} else {
		if (aarch32)
			sprintf(cmdbuf,
				"fdt set /reserved-memory/linux,secmon size <0x%x>;", rsv_sz_align);
		else
			sprintf(cmdbuf,
				"fdt set /reserved-memory/linux,secmon size <0x0 0x%x>;",
				rsv_sz_align);
		rsvmem_dbg("CMD: %s\n", cmdbuf);
		ret = run_command(cmdbuf, 0);
		if (ret != 0) {
			rsvmem_err("reserved memory set size error.\n");
			return -ENODEV;
		}

		memset(cmdbuf, 0, sizeof(cmdbuf));
		if (aarch32)
			sprintf(cmdbuf,
				"fdt set /reserved-memory/linux,secmon alloc-ranges <0x%x 0x%x>;",
				bl31_start, rsv_sz_align);
		else
			sprintf(cmdbuf,
				"fdt set /reserved-memory/linux,secmon alloc-ranges <0x0 0x%x 0x0 0x%x>;",
				bl31_start, rsv_sz_align);
		rsvmem_dbg("CMD: %s\n", cmdbuf);
		ret = run_command(cmdbuf, 0);
		if (ret != 0) {
			rsvmem_err("reserved memory set alloc-ranges error.\n");
			return -ENODEV;
		}
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value tmp_rsv_sz /secmon reserve_mem_size;");
	rsvmem_dbg("CMD: %s\n", cmdbuf);
	ret = run_command(cmdbuf, 0);
	if (ret == 0) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		if (aarch32)
			sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x%x>;", rsv_sz);
		else
			sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x0 0x%x>;", rsv_sz);
		rsvmem_dbg("CMD: %s\n", cmdbuf);
		ret = run_command(cmdbuf, 0);
		if (ret != 0) {
			rsvmem_err("reserved memory set reserve_mem_size error.\n");
			return -ENODEV;
		}
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value ramoops_reg /reserved-memory/ramoops reg;");
	if (run_command(cmdbuf, 0) == 0) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		if (aarch32)
			sprintf(cmdbuf, "fdt set /reserved-memory/ramoops reg <0x%x 0x%x>;",
				ramoops_start, RAMOOP_MEM_SIZE);
		else
			sprintf(cmdbuf,
				"fdt set /reserved-memory/ramoops reg <0x0 0x%x 0x0 0x%x>;",
				ramoops_start, RAMOOP_MEM_SIZE);

		rsvmem_dbg("CMD: %s\n", cmdbuf);
		ret = run_command(cmdbuf, 0);
		if (ret != 0) {
			rsvmem_err("reserved memory set /reserved-memory/ramoops reg error.\n");
			return -ENODEV;
		}
	}

	return 0;
}

static int fdt_setup(unsigned int *alignment, unsigned int *aarch32)
{
	int ret = 0;
	char *fdtaddr = NULL;
	char *temp_env = NULL;
	char cmdbuf[128] = { 0 };
	unsigned int alignment_temp = 0;

	fdtaddr = env_get("fdtaddr");
	if (fdtaddr == NULL) {
		rsvmem_err("get fdtaddr NULL!\n");
		return -EBADMSG;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt addr %s;", fdtaddr);
	rsvmem_dbg("CMD: %s\n", cmdbuf);
	ret = run_command(cmdbuf, 0);
	if (ret != 0 ) {
		rsvmem_err("fdt addr error.\n");
		return -EFAULT;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value env_compatible /reserved-memory/linux,secmon compatible;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get linux,secmon compatible failed.\n");
		return -EBADMSG;
	}
	temp_env = env_get("env_compatible");

	if (strcmp(temp_env, "shared-dma-pool")) {
		rsvmem_err("linux,secmon compatible is not as expected: %s.\n", temp_env);
		return -EBADMSG;
	}
	run_command("setenv env_compatible;", 0);

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_env / \\#address-cells;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get #address-cells failed.\n");
		return -EBADMSG;
	}
	temp_env = env_get("temp_env");
	if (temp_env && !strcmp(temp_env, "0x00000001"))
		*aarch32 = 1;

	/* Get alignment size
	 * If arm64, alignment has 2 parameters
	 * The second parameter need convert big-endian to little-endian
	 */
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_alignment /reserved-memory/linux,secmon alignment;");
	ret = run_command(cmdbuf, 0);
	if (!ret) {
		temp_env = env_get("temp_alignment");
		alignment_temp = simple_strtoul(temp_env, NULL, 16);
		if (*aarch32) {
			*alignment = alignment_temp;
		} else {
			*alignment = (alignment_temp & 0xff) << 24;
			*alignment |= (alignment_temp & 0xff00) << 8;
			*alignment |= (alignment_temp & 0xff0000) >> 8;
			*alignment |= (alignment_temp & 0xff000000) >> 24;
		}
	}

	return 0;
}

static void get_blx_info(unsigned int *bl31_start, unsigned int *bl31_sz,
			 unsigned int *bl32_start, unsigned int *bl32_sz)
{
	unsigned int data = 0;

	data = readl(REG_RSVMEM_SIZE);
	/* workaround for bl3x size */
	*bl31_sz = ((data & 0xffff0000) >> 16) << 16;
	*bl32_sz = (data & 0x0000ffff) << 16;

	*bl31_start = readl(REG_RSVMEM_BL31_START);
	*bl32_start = readl(REG_RSVMEM_BL32_START);
}

static int do_rsvmem_check(cmd_tbl_t *cmdtp, int flag, int argc,
			   char *const argv[])
{
	int ret = 0;
	unsigned int aarch32 = 0;
	unsigned int bl31_start = 0;
	unsigned int bl31_sz = 0;
	unsigned int bl32_start = 0;
	unsigned int bl32_sz = 0;
	unsigned int rsv_mem_sz = 0;
	unsigned int alignment = 0;

	rsvmem_dbg("reserved memory check!\n");

	ret = fdt_setup(&alignment, &aarch32);
	if (ret != 0)
		return ret;

	get_blx_info(&bl31_start, &bl31_sz, &bl32_start, &bl32_sz);

	rsv_mem_sz = bl31_sz + bl32_sz;
	if (bl31_start + bl31_sz != bl32_start) {
		rsvmem_info("bl31 and bl32 reserved memory is not continuous");
		return 0;
	}

	return fdt_config_rsv_mem(aarch32, alignment, bl31_start, rsv_mem_sz);
}

static int do_rsvmem_dump(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	unsigned int bl31_start = 0;
	unsigned int bl31_sz = 0;
	unsigned int bl32_start = 0;
	unsigned int bl32_sz = 0;

	get_blx_info(&bl31_start, &bl31_sz, &bl32_start, &bl32_sz);

	rsvmem_info("reserved memory:\n");
	rsvmem_info("bl31 reserved memory start: 0x%08x\n", bl31_start);
	rsvmem_info("bl31 reserved memory size:  0x%08x\n", bl31_sz);
	rsvmem_info("bl32 reserved memory start: 0x%08x\n", bl32_start);
	rsvmem_info("bl32 reserved memory size:  0x%08x\n", bl32_sz);

	return 0;
}

static cmd_tbl_t cmd_rsvmem_sub[] = {
	U_BOOT_CMD_MKENT(check, 2, 0, do_rsvmem_check, "", ""),
	U_BOOT_CMD_MKENT(dump, 2, 0, do_rsvmem_dump, "", ""),
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

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
#endif
}

U_BOOT_CMD(
	rsvmem, 2, 0,	do_rsvmem,
	"reserve memory",
	"check                   - check reserved memory\n"
	"rsvmem dump                    - dump reserved memory\n"
);
#endif
