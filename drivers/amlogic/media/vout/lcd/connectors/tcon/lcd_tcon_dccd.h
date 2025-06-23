/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _LCD_TCON_DCCD_H
#define _LCD_TCON_DCCD_H
#include <amlogic/media/vout/lcd/aml_lcd.h>

int lcd_tcon_load_dccd(struct aml_lcd_drv_s *pdrv);
unsigned int lcd_tcon_dccd_get_crc(void);
unsigned int lcd_tcon_is_support_dccd(void);
unsigned int lcd_tcon_dccd_has_tcon_file(void);

#endif //_LCD_TCON_DCCD_H

