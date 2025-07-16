// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

void board_init_mem(void);

int aml_board_late_init_front(void *arg);

int aml_board_late_init_tail(void *arg);

void aml_board_display_env_handler(void);
// void aml_board_display_init(unsigned char vout_bit);

void board_set_boot_source(void);

int uboot_log_replace_addr_with_fdt(void);
