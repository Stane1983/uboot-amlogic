/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_LOG_H
#define __HDMITX_LOG_H
#include<linux/kernel.h>


#define HDMITX_ERROR(fmt, ...)							\
	printk(fmt, ##__VA_ARGS__)

#define HDMITX_DEBUG(fmt, ...)							\
	printk(fmt, ##__VA_ARGS__)


#define HDMITX_DEBUG_EDID(fmt, ...)						\
	printk(fmt, ##__VA_ARGS__)


#define HDMITX_INFO(fmt, ...)							\
	printk(fmt, ##__VA_ARGS__)



#endif
