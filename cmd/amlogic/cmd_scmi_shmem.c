// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <linux/arm-smccc.h>
#include <linux/libfdt_env.h>

#define SIP_SMC_SCMI_CMD			0x820000C1
#define SCMI_SUBID_GET_SHMEM_ADDR		0x1


static uint32_t get_scmi_shmem_addr(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SIP_SMC_SCMI_CMD, SCMI_SUBID_GET_SHMEM_ADDR,
		      0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int ftd_get_root_address_cells(void)
{
	char *address_cells_str = NULL;
	int ret = 0;

	ret = run_command("fdt get value address_cells / \\#address-cells", 0);
	if (ret != 0) {
		printf("Error: Failed to get address_cells for root\n");
		return -EBADMSG;
	}

	address_cells_str = env_get("address_cells");
	if (!address_cells_str) {
		printf("Error: Failed to retrieve address_cells from environment\n");
		return -EBADMSG;
	}

	return strtoul(address_cells_str, NULL, 16);
}

static int ftd_get_scmi_shmem_addr(void)
{
	char *shmem_addr_str = NULL;
	u32 *shmem_addr = NULL;
	int root_addrs_cells = ftd_get_root_address_cells();
	int ret = 0;

	ret = run_command("fdt get addr shmem_addr /shmem reg", 0);
	if (ret != 0) {
		printf("Error: Failed to get shmem_addr for /shmem\n");
		return -EBADMSG;
	}

	shmem_addr_str = env_get("shmem_addr");
	if (!shmem_addr_str) {
		printf("Error: Failed to retrieve shmem_addr from environment\n");
		return -EBADMSG;
	}
	shmem_addr = (u32 *)strtoul(shmem_addr_str, NULL, 16);
	if (!shmem_addr)
		return -EFAULT;

	if (root_addrs_cells == 2)  /* CONFIG_ARM64 */
		return be32_to_cpu(*(++shmem_addr));
	else  /* CONFIG_ARM */
		return be32_to_cpu(*shmem_addr);
}

static int fdt_set_scmi_shmem_reg(void)
{
	int ret = 0;
	char cmdbuf[256] = {0};
	u32 bl31_shmem_addr = get_scmi_shmem_addr();
	u32 dtb_shmem_addr = ftd_get_scmi_shmem_addr();

	if (dtb_shmem_addr == bl31_shmem_addr)
		return 0;

	printf("Warning: Address of shmem in DTB is inconsistent with BL31!!!\n");
	printf("bl31_shmem_addr = 0x%x, dtb_shmem_addr = 0x%x\n",
	       bl31_shmem_addr, dtb_shmem_addr);
	printf("Update the address of shmem in DTB\n");
	memset(cmdbuf, 0, sizeof(cmdbuf));
	if (ftd_get_root_address_cells() == 1) {  /* CONFIG_ARM */
		sprintf(cmdbuf, "fdt set /shmem reg <0x%x 0x100>",
			bl31_shmem_addr);
	} else if (ftd_get_root_address_cells() == 2) {  /* CONFIG_ARM64 */
		sprintf(cmdbuf, "fdt set /shmem reg <0 0x%x 0 0x100>",
			bl31_shmem_addr);
	} else {
		printf("Error: Failed to adapt address-cells for DTB.\n");
		return -ENXIO;
	}
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("Error: Failed to set reg of scmi_shmem for DTB\n");
		return -EBADMSG;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt set /shmem ranges <0 0 0x%x 0x100>",
		bl31_shmem_addr);
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		printf("Error: Failed to set ranges of scmi_shmem for DTB\n");
		return -EBADMSG;
	}

	return 0;
}

static int do_update_scmi_shmem(cmd_tbl_t *cmdtp, int flag, int argc,
				char * const argv[])
{
	return fdt_set_scmi_shmem_reg();
}

U_BOOT_CMD(update_scmi_shmem, 1, 0,	do_update_scmi_shmem,
	   "Update the shared memory address of the SCMI in the DTB",
	   ""
);
