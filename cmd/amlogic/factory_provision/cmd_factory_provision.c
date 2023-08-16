// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 */

#include <stdlib.h>
#include <common.h>
#include <command.h>
#include <amlogic/storage.h>
#include "factory_provision_utils.h"
#include "emmc_factory_provision.h"
#include "nand_factory_provision.h"

int cmd_func(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = CMD_RET_SUCCESS;
	struct input_param params = { 0 };
	char medium_name[MAX_SIZE_MEDIUM_NAME] = { 0 };
	int medium_type = store_get_type();

	ret = is_storage_medium_supported();
	if (ret != CMD_RET_SUCCESS)
		return ret;

	parse_params(argc, argv, &params);

	ret = check_params(&params);
	if (ret != CMD_RET_SUCCESS)
		return ret;

	get_medium_name(medium_type, medium_name);

	if (medium_type == BOOT_NAND_NFTL || medium_type == BOOT_NAND_MTD || medium_type == BOOT_SNAND) {
#ifdef CONFIG_NAND_FACTORY_PROVISION
		ret = nand_factory_provision(&params);
#endif
	} else {
#ifdef CONFIG_MMC
		ret = emmc_factory_provision(&params);
#endif
	}

	return ret;
}

/* -------------------------------------------------------------------- */
U_BOOT_CMD(
	factory_provision, CONFIG_SYS_MAXARGS, 0, cmd_func,
	"provision keybox\n",
	"write <keybox_name> <keybox_addr> <keybox_size>\n"
	"	- write keybox to key partition\n\n"
	"query <keybox_name> [ret_data_addr]\n"
	"	- query whether the keybox exists by keybox name\n"
	"	- when keybox exists, return data: keybox_size(4bytes)\n\n"
	"list\n"
	"	- list all keyboxs of key partition\n"
	"remove <keybox_name>\n"
	"	- remove the keybox by keybox name\n"
	"clear\n"
	"	- clear all keyboxs of key partition\n"
	"version\n"
	"	- show version of factory provision\n"
);
