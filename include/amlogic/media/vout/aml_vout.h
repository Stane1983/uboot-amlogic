/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_VOUT_H_
#define __AML_VOUT_H_

#include <amlogic/media/vout/aml_vinfo.h>

#define VOUT_VIU1_SEL    1
#define VOUT_VIU2_SEL    2
#define VOUT_VIU3_SEL    3

enum viu_mux_e {
	VIU_MUX_ENCL = 0,
	VIU_MUX_ENCI,
	VIU_MUX_ENCP,
	VIU_MUX_MAX,
};

#define CONNECTOR_DEV_MASK     0x0f00
#define CONNECTOR_SUB_DEV_MASK 0x00f0
#define CONNECTOR_ENC_IDX_MASK 0x000f

#define CONNECTOR_DEV_INVALID  0x0f00
#define CONNECTOR_DEV_LCD      0x0100
#define CONNECTOR_DEV_RESERVED 0x0200
#define CONNECTOR_DEV_HDMI     0x0300
#define CONNECTOR_DEV_CVBS     0x0400

#define CONNECTOR_SUB_DEV_INVALID    0x0f00
#define CONNECTOR_SUB_DEV_LCD_LVDS   0x0000
#define CONNECTOR_SUB_DEV_LCD_VBYONE 0x0010
#define CONNECTOR_SUB_DEV_LCD_MIPI   0x0020
#define CONNECTOR_SUB_DEV_LCD_EDP    0x0030
#define CONNECTOR_SUB_DEV_HDMI       0x0000
#define CONNECTOR_SUB_DEV_CVBS       0x0000

unsigned short vout_connector_check(unsigned char vout_index);

void vout_init(void);
void vout_vinfo_dump(void);
int vout_get_current_vmode(void);
int vout_get_current_axis(int *axis);
void vout_set_current_vmode(int mode);
struct vinfo_s *vout_get_current_vinfo(void);
void vout_viu_mux(int index, unsigned int mux_sel);
extern unsigned long get_fb_addr(void);

void vout_probe(void);
#endif

