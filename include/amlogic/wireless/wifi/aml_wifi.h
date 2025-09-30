/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2018 Amlogic Limited
 */
#ifndef AML_WIFI_H
#define AML_WIFI_H

#include <usb.h>

typedef struct {
	unsigned int family_id;
	char *usb_module;
	unsigned int *fw_data;
	unsigned int fw_len;
} wifi_dongle;

void aml_wifi_set_usb2t_mode(void);

#endif
