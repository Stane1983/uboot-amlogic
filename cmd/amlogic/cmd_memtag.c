// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/libfdt.h>
#include <amlogic/tee_aml.h>

#ifdef CONFIG_CMD_MEMTAG

#ifndef DTB_BIND_KERNEL
static int do_memtag_check(cmd_tbl_t *cmdtp, int flag, int argc,
			   char *const argv[])
{
	int ret = 0;
	char *fdtaddr = NULL;
	int nodeoffset;
	char *propdata;
	unsigned long base, size;
	unsigned handle;

	fdtaddr = env_get("dtb_mem_addr");
	if (fdtaddr == NULL) {
		printf("get fdtaddr NULL!\n");
		return __LINE__;
	}
	fdtaddr = (char*)simple_strtoul(fdtaddr, NULL, 0);

	nodeoffset = fdt_path_offset(fdtaddr, "/reserved-memory/linux,mte");
	if (nodeoffset < 0) {
		printf("no memtag node in dtb\n");
		return 0;
	}
	propdata = (char *)fdt_getprop(fdtaddr, nodeoffset, "reg", NULL);
	if (!propdata) {
		printf("can't find reg property\n");
		return __LINE__;
	}

	/* only support in aarch64 */
	base = be32_to_cpup((((u32 *)propdata) + 1));
	size = be32_to_cpup((((u32 *)propdata) + 3));

	ret = tee_protect_mem(TEE_MEM_TYPE_MTE, 0, base, size, &handle);
	if (ret) {
		printf("memtag protect fail ret:%d\n", ret);
		return __LINE__;
	}

	printf("memtag base:0x%lx size:0x%lx protect done\n", base, size);
	return 0;
}

static cmd_tbl_t cmd_memtag_sub[] = {
	U_BOOT_CMD_MKENT(check, 2, 0, do_memtag_check, "", ""),
};
#endif

static int do_memtag(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#ifdef DTB_BIND_KERNEL
	printf("no memtag checking, should check int kernel\n");
	return 0;
#else
	cmd_tbl_t *c;

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_memtag_sub[0], ARRAY_SIZE(cmd_memtag_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return CMD_RET_USAGE;
#endif
}

U_BOOT_CMD(
	memtag, 2, 0,	do_memtag,
	"memory tag",
	"check                   - check memory tag\n"
);

#endif
