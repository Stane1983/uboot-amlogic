#ifndef EXTRA_REGISTER_H
#define EXTRA_REGISTER_H

//otp
#define OTP_LIC					(OTP_LIC_A)
#define OTP_LIC00              (OTP_LIC + 0x00)
#define OTP_LIC0               (OTP_LIC00)

//dmc
#define DMC_SEC_STATUS                             ((0x051a  << 2) + 0xfe036000)
#define DMC_VIO_ADDR0                              ((0x051b  << 2) + 0xfe036000)
#define DMC_VIO_ADDR1                              ((0x051c  << 2) + 0xfe036000)
#define DMC_VIO_ADDR2                              ((0x051d  << 2) + 0xfe036000)
#define DMC_VIO_ADDR3                              ((0x051e  << 2) + 0xfe036000)


//ao
#define SEC_AO_SEC_GP_CFG2		SYSCTRL_SEC_STATUS_REG6

//gpio
#define PADCTRL_GPIOD_PULL_UP                      ((0x0064  << 2) + 0xfe004000)

#endif

