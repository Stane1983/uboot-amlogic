/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef INC_AML_LCD_H
#define INC_AML_LCD_H

#include <common.h>
#include <linux/list.h>
#include <amlogic/media/vout/lcd/lcd_vout.h>
#include <amlogic/media/vout/lcd/aml_bl.h>
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/media/vout/lcd/lcd_extern.h>
#endif
#ifdef CONFIG_AML_BL_EXTERN
#include <amlogic/media/vout/lcd/bl_extern.h>
#endif

#define LCD_EXT_I2C_BUS_0     0  //A
#define LCD_EXT_I2C_BUS_1     1  //B
#define LCD_EXT_I2C_BUS_2     2  //C
#define LCD_EXT_I2C_BUS_3     3  //D
#define LCD_EXT_I2C_BUS_4     4  //AO
#define LCD_EXT_I2C_BUS_5     5
#define LCD_EXT_I2C_BUS_MAX   6

#define LCD_EXT_I2C_BUS_INVALID        0xff
#define LCD_EXT_I2C_ADDR_INVALID       0xff
#define LCD_EXT_GPIO_INVALID           0xff

#define LCD_EXT_SPI_CLK_FREQ_DFT       10 /* unit: KHz */

/*******************************************/
/*        LCD EXT CMD                      */
/*******************************************/
#define LCD_EXT_CMD_TYPE_CMD_DELAY              0x00
#define LCD_EXT_CMD_TYPE_CMD2_DELAY             0x01  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_CMD3_DELAY             0x02  /* for i2c device 3rd addr */
#define LCD_EXT_CMD_TYPE_CMD4_DELAY             0x03  /* for i2c device 4th addr */
#define LCD_EXT_CMD_TYPE_NONE                   0x10
#define LCD_EXT_CMD_TYPE_MULTI_LIST_FR          0x21 /* dlg fr multi list, 1byte frame rate*/
#define LCD_EXT_CMD_TYPE_MULTI_LIST_UFR         0x2f /* ufr fr multi list, 2byte frame rate*/
#define LCD_EXT_CMD_TYPE_CMD_BIN2               0xa0  /* replace data with offset by reg_addr */
#define LCD_EXT_CMD_TYPE_CMD2_BIN2              0xa1  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_CMD3_BIN2              0xa2  /* for i2c device 3rd addr */
#define LCD_EXT_CMD_TYPE_CMD4_BIN2              0xa3  /* for i2c device 4th addr */
#define LCD_EXT_CMD_TYPE_CMD_BIN                0xb0  /* auto fill reg addr 0x0, and data */
#define LCD_EXT_CMD_TYPE_CMD2_BIN               0xb1  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_CMD3_BIN               0xb2  /* for i2c device 3rd addr */
#define LCD_EXT_CMD_TYPE_CMD4_BIN               0xb3  /* for i2c device 4th addr */
#define LCD_EXT_CMD_TYPE_CMD                    0xc0
#define LCD_EXT_CMD_TYPE_CMD2                   0xc1  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_CMD3                   0xc2  /* for i2c device 3rd addr */
#define LCD_EXT_CMD_TYPE_CMD4                   0xc3  /* for i2c device 4th addr */
#define LCD_EXT_CMD_TYPE_CMD_BIN_DATA           0xd0 /* nonexistent reg_addr, all data replace */
#define LCD_EXT_CMD_TYPE_CMD2_BIN_DATA          0xd1 /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_CMD3_BIN_DATA          0xd2 /* for i2c device 3rd addr */
#define LCD_EXT_CMD_TYPE_CMD4_BIN_DATA          0xd3 /* for i2c device 4th addr */
#define LCD_EXT_CMD_TYPE_CMD_MULTI              0xe0
#define LCD_EXT_CMD_TYPE_CMD2_MULTI             0xe1
#define LCD_EXT_CMD_TYPE_CMD3_MULTI             0xe2
#define LCD_EXT_CMD_TYPE_CMD4_MULTI             0xe3
#define LCD_EXT_CMD_TYPE_MULTI_CMD              0xec /* cmd for multi list matching*/
#define LCD_EXT_CMD_TYPE_MULTI_DFT_CMD          0xed /* cmd for multi list matching,
						      * as default setting bypass when power on
						      */
#define LCD_EXT_CMD_TYPE_GPIO                   0xf0
#define LCD_EXT_CMD_TYPE_WAIT_GPIO              0xf4
#define LCD_EXT_CMD_TYPE_SWITCH_PORT            0xf5
#define LCD_EXT_CMD_TYPE_CHECK_RETRY            0xfb
#define LCD_EXT_CMD_TYPE_CHECK                  0xfc
#define LCD_EXT_CMD_TYPE_DELAY                  0xfd
#define LCD_EXT_CMD_TYPE_EXIT                   0xfe
#define LCD_EXT_CMD_TYPE_END                    0xff

#define LCD_EXT_CMD_SIZE_DYNAMIC       0xff
#define LCD_EXT_DYNAMIC_SIZE_INDEX     1

#define Rsv_val 0xffffffff

#define LCD_NUM_MAX         20
#define LCD_PRBS_MODE_LVDS    BIT(0)
#define LCD_PRBS_MODE_VX1     BIT(1)
#define LCD_PRBS_MODE_FREQ    BIT(2)
#define LCD_PRBS_MODE_MAX     3

#endif /* INC_AML_LCD_H */
