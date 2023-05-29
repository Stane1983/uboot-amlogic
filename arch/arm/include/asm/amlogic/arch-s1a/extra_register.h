#ifndef EXTRA_REGISTER_H
#define EXTRA_REGISTER_H

/* rsa */
#define RSA_BASE                                   ((0x0000  << 2) + 0xfe448000)

/* otp */
#define OTP_TEE_RDY                                ((0x0000  << 2) + 0xfe440000)
#define OTP_TEE_DEBUG                              ((0x0001  << 2) + 0xfe440000)
#define OTP_TEE_CFG                                ((0x0002  << 2) + 0xfe440000)
#define OTP_TEE_WR_DAT0                            (0x10 + 0xfe440000)
#define OTP_TEE_WR_DAT1                            (0x14 + 0xfe440000)
#define OTP_TEE_WR_DAT2                            (0x18 + 0xfe440000)
#define OTP_TEE_WR_DAT3                            (0x1c + 0xfe440000)

#define OTP_TEE_RD_DAT0                            (0x20 + 0xfe440000)
#define OTP_TEE_RD_DAT1                            (0x24 + 0xfe440000)
#define OTP_TEE_RD_DAT2                            (0x28 + 0xfe440000)
#define OTP_TEE_RD_DAT3                            (0x2c + 0xfe440000)

#define OTP_LIC0                                   (OTP_LIC00)
#define OTP_LIC                                    ((0x0010  << 2) + 0xfe440000)
#define OTP_LIC00                                  (OTP_LIC + 0x00)
#define OTP_LIC01                                  (OTP_LIC + 0x04)
#define OTP_LIC02                                  (OTP_LIC + 0x08)
#define OTP_LIC03                                  (OTP_LIC + 0x0C)

#define OTP_LIC10                                  (OTP_LIC + 0x10)
#define OTP_LIC11                                  (OTP_LIC + 0x14)
#define OTP_LIC12                                  (OTP_LIC + 0x18)
#define OTP_LIC13                                  (OTP_LIC + 0x1C)

#define OTP_LIC20                                  (OTP_LIC + 0x20)
#define OTP_LIC21                                  (OTP_LIC + 0x24)
#define OTP_LIC22                                  (OTP_LIC + 0x28)
#define OTP_LIC23                                  (OTP_LIC + 0x2C)

#define OTP_LIC30                                  (OTP_LIC + 0x30)
#define OTP_LIC31                                  (OTP_LIC + 0x34)
#define OTP_LIC32                                  (OTP_LIC + 0x38)
#define OTP_LIC33                                  (OTP_LIC + 0x3C)

#define DMC_SEC_RANGE0_RID_CTRL0                   ((0x0020  << 2) + 0xfe037000)
#define DMC_SEC_RANGE0_CTRL1                       ((0x0001  << 2) + 0xfe037000)
#define DMC_SEC_RANGE0_WID_CTRL0                   ((0x0080  << 2) + 0xfe037000)
#define DMC_SEC_RANGE0_CTRL1                       ((0x0001  << 2) + 0xfe037000)
#define DMC_SEC_RANGE0_CTRL                        ((0x0000  << 2) + 0xfe037000)

#define DOLBY_CORE3_OUTPUT_CSC_CRC                 ((0x36fd  << 2) + 0xff000000)
#define DOLBY_CORE3_DIAG_CTRL                      ((0x36f8  << 2) + 0xff000000)
#define DOLBY_CORE3_CTRL                           ((0x3601  << 2) + 0xff000000)
#define DOLBY_CORE3_Metadata_Start                 ((0x3602  << 2) + 0xff000000)
#define DOLBY_CORE3_Metadata_End                   ((0x3603  << 2) + 0xff000000)
#define DOLBY_CORE3_Interrupt_Raw                  ((0x3604  << 2) + 0xff000000)
#define DOLBY_CORE3_Interrupt_Enable               ((0x3605  << 2) + 0xff000000)
#define DC_CAV_LUT_ADDR                            ((0x0014  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAL                           ((0x0012  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAH                           ((0x0013  << 2) + 0xfe036000)
#define DOLBY_CORE2A_Metadata_End                  ((0x3403  << 2) + 0xff000000)
#define DC_CAV_LUT_ADDR                            ((0x0014  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAH                           ((0x0013  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAL                           ((0x0012  << 2) + 0xfe036000)
#define DOLBY_CORE2A_Metadata_Start                ((0x3402  << 2) + 0xff000000)
#define DC_CAV_LUT_ADDR                            ((0x0014  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAH                           ((0x0013  << 2) + 0xfe036000)
#define DC_CAV_LUT_DATAL                           ((0x0012  << 2) + 0xfe036000)
#define DOLBY_CORE2A_CTRL                          ((0x3401  << 2) + 0xff000000)

#define PWMGH_PWM_B                                ((0x0001  << 2) + 0xfe05e000)
#define PWMIJ_PWM_B                                ((0x0001  << 2) + 0xfe060000)
#define PWMIJ_MISC_REG_AB                          ((0x0002  << 2) + 0xfe060000)
#define CLKCTRL_PWM_CLK_GH_CTRL                    ((0x0063  << 2) + 0xfe000000)
#define PADCTRL_GPIOE_DS                           ((0x0077  << 2) + 0xfe004000)
#define PADCTRL_PIN_MUX_REGI                       ((0x0012  << 2) + 0xfe004000)
#define PWMGH_MISC_REG_AB                          ((0x0002  << 2) + 0xfe05e000)
#define CLKCTRL_PWM_CLK_IJ_CTRL                    ((0x0064  << 2) + 0xfe000000)


#endif // EXTRA_REGISTER_H
