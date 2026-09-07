/* SPDX-License-Identifier: GPL-2.0 */
/*
* Copyright (C) 2025 Amlogic, Inc. All rights reserved.
*
* Description:
*/

#ifndef __EXTRA_REGISTER_H__
#define __EXTRA_REGISTER_H__

//sec_ao
#define SEC_AO_SEC_GP_CFG0    SYSCTRL_SEC_STATUS_REG4
#define SEC_AO_SEC_GP_CFG2  SYSCTRL_SEC_STATUS_REG6

//OTP
#define OTP_LIC00		(OTP_LIC + 0x00)

#define OTP_LIC0		(OTP_LIC00)

#define REG_BASE_VCBUS                             (0xff800000L)

#define DMC_SEC_STATUS                             ((0x061a  << 2) + 0xfe300000)

#define DMC_VIO_ADDR0                              ((0x061b  << 2) + 0xfe300000)

#define DMC_VIO_ADDR1                              ((0x061c  << 2) + 0xfe300000)
#define DMC_VIO_ADDR2                              ((0x061d  << 2) + 0xfe300000)
#define DMC_VIO_ADDR3                              ((0x061e  << 2) + 0xfe300000)
#define DMC_AXI2DDR0                               ((0x0620  << 2) + 0xfe300000)

#endif
