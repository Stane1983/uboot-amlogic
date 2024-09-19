
/*
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __BOARD_CFG_H__
#define __BOARD_CFG_H__

#include <asm/amlogic/arch/cpu.h>

/*
 * platform power init config
 */

#define AML_VCCK_INIT_VOLTAGE	  1009	    //VCCK power up voltage
#define AML_VDDEE_INIT_VOLTAGE    840       // VDDEE power up voltage
/*Distinguish whether to use efuse to adjust vddee*/
#define CONFIG_PDVFS_ENABLE

/* AVB */
#define CONFIG_AML_AVB2_ANTIROLLBACK 1
#define CONFIG_AVB_VERIFY 1
#define CONFIG_SUPPORT_EMMC_RPMB 1
#define CONFIG_AML_DEV_ID 1

#ifdef CONFIG_ARMV8_MULTIENTRY
#define INIT_DISPLAY "if test ${display_on_smp} = 0; then run init_display; fi;"
#else
#define INIT_DISPLAY "run init_display;"
#endif

/* SMP Definitions */
// #define CPU_RELEASE_ADDR		secondary_boot_func

/* Bootloader Control Block function
   That is used for recovery and the bootloader to talk to each other
  */
#if 0
#define CONFIG_BOOTLOADER_CONTROL_BLOCK
#endif

/* Serial config */
#define CONFIG_CONS_INDEX 2
//#define CONFIG_BAUDRATE  115200

/*hign console baudrate*/
#define CONFIG_LOW_CONSOLE_BAUD			1

/* Enable ir remote wake up for bl30 */
#define AML_IR_REMOTE_POWER_UP_KEY_VAL1 0xef10fe01 //amlogic tv ir --- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL2 0XBB44FB04 //amlogic tv ir --- ch+
#define AML_IR_REMOTE_POWER_UP_KEY_VAL3 0xF20DFE01 //amlogic tv ir --- ch-
#define AML_IR_REMOTE_POWER_UP_KEY_VAL4 0XBA45BD02 //amlogic small ir--- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL5 0xe51afb04
#define AML_IR_REMOTE_POWER_UP_KEY_VAL6 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL7 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL8 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL9 0xFFFFFFFF

/*config the default parameters for adc power key*/
#define AML_ADC_POWER_KEY_CHAN   2  /*channel range: 0-7*/
#define AML_ADC_POWER_KEY_VAL    0  /*sample value range: 0-1023*/

//#define CONFIG_BOOTLOADER_CONTROL_BLOCK

//USB_POWEROFF
#define AMLOGIC_USB_POWER

/* args/envs */
#define CONFIG_SYS_MAXARGS  64
#define CONFIG_EXTRA_ENV_SETTINGS \
	"scramble_reg=0xfe02e030\0"\
		"systemsuspend_switch=0\0"\
		"ddr_resume=0\0"\
		"baudrate=115200\0"\
		"panel_type=lvds_1\0" \
		"connector0_type=LVDS-A\0" \
		"lcd_debug=0x00000000\0" \
		"outputmode=1080p60hz\0" \
		"vout_init=disable\0" \
		"model_name=FHD2HDMI\0" \
		"gamma=0\0" \
		"display_width=1920\0" \
		"display_height=1080\0" \
		"display_bpp=16\0" \
		"display_color_index=16\0" \
		"display_layer=osd0\0" \
		"display_color_fg=0xffff\0" \
		"display_color_bg=0\0" \
		"fb_width=1920\0" \
		"fb_height=1080\0" \
		"frac_rate_policy=1\0" \
		"usb_burning=" CONFIG_USB_TOOL_ENTRY "\0" \
		"cvbs_drv=0\0"\
		"upgrade_key_flag=2\0"\
		"suspend=off\0"\
		"powermode=on\0"\
		"ffv_wake=off\0"\
		"ffv_freeze=off\0"\
		"edid_14_dir=/odm/etc/tvconfig/hdmi/port1_14.bin\0" \
		"edid_20_dir=/odm/etc/tvconfig/hdmi/port1_20.bin\0" \
		"edid_select=0\0" \
		"port_map=0x4321\0" \
		"cec_fun=0x2F\0" \
		"logic_addr=0x0\0" \
		"cec_ac_wakeup=1\0" \
		"scramble_reg=0xfe02e030\0" \

#define CONFIG_PREBOOT  \
	"run upgrade_check;"\
	"get_rebootmode;"\
	INIT_DISPLAY \
	"run storeargs;"\
	"run upgrade_key;" \
	"bcb uboot-command;" \
	"run switch_bootmode;" \
	"run reset_suspend;"

#define CONFIG_FIT 1
#define CONFIG_OF_LIBFDT 1
#define CONFIG_ANDROID_BOOT_IMAGE 1

/* ATTENTION */
/* DDR configs move to board/amlogic/[board]/firmware/timing.c */

/* running in sram */
//#define UBOOT_RUN_IN_SRAM
#ifdef UBOOT_RUN_IN_SRAM
#define CONFIG_SYS_INIT_SP_ADDR				(0x00300000)
#else
#define CONFIG_SYS_INIT_SP_ADDR				(0x00300000)
#endif

 /* mtd device board config */
#define BOARD_BOOT_LAYOUT_DISCRETE_BL2         1
#define CONFIG_BL2_COPY_NUM					8
#define CONFIG_NOR_TPL_COPY_NUM				1
#define CONFIG_NAND_TPL_COPY_NUM			2
#define CONFIG_TPL_SIZE_PER_COPY			0x300000

#ifndef CONFIG_ENV_IS_IN_NAND
#define MTD_RSV_ENV_BLOCK_CNT			4
#else
#define MTD_RSV_ENV_BLOCK_CNT			0
#endif

#define MTD_RSV_KEY_BLOCK_CNT			8

#ifndef CONFIG_DTB_BIND_KERNEL
#define MTD_RSV_DTB_BLOCK_CNT			4
#else
#define MTD_RSV_DTB_BLOCK_CNT			0
#endif

#define MTD_RSV_DDR_BLOCK_CNT			0

#define MTD_RSV_KEY_SIZE			0x8000
#define MTD_RSV_DTB_SIZE			0x10000
#define MTD_RSV_DDR_SIZE			0x10000
#define MTD_RSV_BLOCK_CNT	\
	(MTD_RSV_ENV_BLOCK_CNT + \
	MTD_RSV_KEY_BLOCK_CNT +	\
	MTD_RSV_DTB_BLOCK_CNT +	\
	MTD_RSV_DDR_BLOCK_CNT + 4)

/*
 * use BOARD_CONFIG_BL2_LAYOUT_TYPE  to select the BL2 layout:
 * 0 : indicates bl2 is aligned with block size
 * 1 : indicates bl2 is fixed size with 512 pages
 * 2 : indicates bl2  is fixed size with 1024 pages
 */
#define BOARD_CONFIG_BL2_LAYOUT_TYPE		1

//#define CONFIG_NR_DRAM_BANKS			1
/* ddr functions */
#define DDR_FULL_TEST            0 //0:disable, 1:enable. ddr full test
#define DDR_LOW_POWER            0 //0:disable, 1:enable. ddr clk gate for lp
#define DDR_ZQ_PD                0 //0:disable, 1:enable. ddr zq power down
#define DDR_USE_EXT_VREF         0 //0:disable, 1:enable. ddr use external vref
#define DDR4_TIMING_TEST         0 //0:disable, 1:enable. ddr4 timing test function
#define DDR_PLL_BYPASS           0 //0:disable, 1:enable. ddr pll bypass function

/* storage: emmc/nand/sd */
#define CONFIG_ENV_OVERWRITE
/* #define 	CONFIG_CMD_SAVEENV */
/* fixme, need fix*/

#if (defined(CONFIG_ENV_IS_IN_AMLNAND) || defined(CONFIG_ENV_IS_IN_MMC)) && defined(CONFIG_STORE_COMPATIBLE)
#error env in amlnand/mmc already be compatible;
#endif

/*
*				storage
*		|---------|---------|
*		|					|
*		emmc<--Compatible-->nand
*					|-------|-------|
*					|		|
*					MTD<-Exclusive->NFTL
*					|
*			|***************|***************|
*			slc-nand	SPI-nand	SPI-nor
*			(raw nand)
*/
/* axg only support slc nand */
/* swither for mtd nand which is for slc only. */


#if defined(CONFIG_AML_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

#if (defined(CONFIG_AML_NAND) || defined(CONFIG_MESON_NFC)) && defined(CONFIG_MESON_FBOOT)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC CONFIG _MESON_FBOOT can not support at the sametime;
#endif

#if defined(CONFIG_SPI_NAND) && defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_SPI_NAND/CONFIG_MTD_SPI_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

/* #define		CONFIG_AML_SD_EMMC 1 */
#ifdef		CONFIG_AML_SD_EMMC
	#define 	CONFIG_GENERIC_MMC 1
	#define 	CONFIG_CMD_MMC 1
	#define CONFIG_CMD_GPT 1
	#define	CONFIG_SYS_MMC_ENV_DEV 1
	#define CONFIG_EMMC_DDR52_EN 0
	#define CONFIG_EMMC_DDR52_CLK 35000000
#endif
#define		CONFIG_PARTITIONS 1
#if 0
#define 	CONFIG_SYS_NO_FLASH  1
#endif

#if defined CONFIG_MESON_NFC || defined CONFIG_SPI_NAND || defined CONFIG_MTD_SPI_NAND
	#define CONFIG_SYS_MAX_NAND_DEVICE  2
#endif

/* vpu */
#define AML_VPU_CLK_LEVEL_DFT 7

/* LCD */

/* osd */
#define OSD_SCALE_ENABLE
#define AML_OSD_HIGH_VERSION
#define AML_T7_DISPLAY

/* USB
 * Enable CONFIG_MUSB_HCD for Host functionalities MSC, keyboard
 * Enable CONFIG_MUSB_UDD for Device functionalities.
 */
/* #define CONFIG_MUSB_UDC		1 */
/* #define CONFIG_CMD_USB 1 */

#define USB_PHY2_PLL_PARAMETER_1	0x09400414
#define USB_PHY2_PLL_PARAMETER_2	0x927e0000
#define USB_PHY2_PLL_PARAMETER_3	0xAC5F49E5

#define USB_G12x_PHY_PLL_SETTING_1	(0xfe18)
#define USB_G12x_PHY_PLL_SETTING_2	(0xfff)
#define USB_G12x_PHY_PLL_SETTING_3	(0x78000)
#define USB_G12x_PHY_PLL_SETTING_4	(0xe0004)
#define USB_G12x_PHY_PLL_SETTING_5	(0xe000c)

#define AML_TXLX_USB        1
#define AML_USB_M31_PHY		1
#define AML_USB_V2             1
#define USB_GENERAL_BIT         3
#define USB_PHY21_BIT           4

/* UBOOT fastboot config */


/* UBOOT Factory usb/sdcard burning config */

/* net */
/* #define CONFIG_CMD_NET   1 */
#define CONFIG_ETH_DESIGNWARE
#if defined(CONFIG_CMD_NET)
	#define CONFIG_DESIGNWARE_ETH 1
	#define CONFIG_PHYLIB	1
	#define CONFIG_NET_MULTI 1
	#define CONFIG_CMD_PING 1
	#define CONFIG_CMD_DHCP 1
	#define CONFIG_CMD_RARP 1
	#define CONFIG_HOSTNAME        "arm_gxbb"
#if 0
	#define CONFIG_RANDOM_ETHADDR  1				   /* use random eth addr, or default */
#endif
	#define CONFIG_ETHADDR         00:15:18:01:81:31   /* Ethernet address */
	#define CONFIG_IPADDR          10.18.9.97          /* Our ip address */
	#define CONFIG_GATEWAYIP       10.18.9.1           /* Our getway ip address */
	#define CONFIG_SERVERIP        10.18.9.113         /* Tftp server ip address */
	#define CONFIG_NETMASK         255.255.255.0
#endif /* (CONFIG_CMD_NET) */

#define MAC_ADDR_NEW  1

/* other devices */
#define CONFIG_SHA1 1
#define CONFIG_MD5 1

/* commands */
/* #define CONFIG_CMD_PLLTEST 1 */

/*file system*/
#define CONFIG_DOS_PARTITION 1
#define CONFIG_EFI_PARTITION 1
/* #define CONFIG_MMC 1 */
#define CONFIG_FS_FAT 1
#define CONFIG_FS_EXT4 1
#define CONFIG_LZO 1

#define CONFIG_FAT_WRITE 1
#define CONFIG_AML_FACTORY_PROVISION 1

/* Cache Definitions */
/* #define CONFIG_SYS_DCACHE_OFF */
/* #define CONFIG_SYS_ICACHE_OFF */

/* other functions */
#define CONFIG_LIBAVB		1

/* define CONFIG_SYS_MEM_TOP_HIDE 8M space for free buffer */
#define CONFIG_SYS_MEM_TOP_HIDE		0x00800000

#define CONFIG_CPU_ARMV8

//use sha2 command
#define CONFIG_CMD_SHA2

//use cache command
#define CONFIG_CMD_CACHE

//use hardware sha2
//#define CONFIG_AML_HW_SHA2

//Replace avb2 software SHA256 to utilize armce
#define CONFIG_AVB2_UBOOT_SHA256

#define CONFIG_MULTI_DTB    1

#define CONFIG_RX_RTERM		1

#define CONFIG_CMD_HDMIRX   1

#define CONFIG_CMD_CEC		1
/* support secure boot */
#define CONFIG_AML_SECURE_UBOOT   1

#if defined(CONFIG_AML_SECURE_UBOOT)

/* unify build for generate encrypted bootloader "u-boot.bin.encrypt" */
#define CONFIG_AML_CRYPTO_UBOOT   1
//#define CONFIG_AML_SIGNED_UBOOT   1
/* unify build for generate encrypted kernel image
   SRC : "board/amlogic/(board)/boot.img"
   DST : "fip/boot.img.encrypt" */
/* #define CONFIG_AML_CRYPTO_IMG       1 */

#endif /* CONFIG_AML_SECURE_UBOOT */

#define CONFIG_FIP_IMG_SUPPORT  1

/* config ramdump to debug kernel panic */
#define CONFIG_FULL_RAMDUMP

#define BL32_SHARE_MEM_SIZE  0x800000
#define CONFIG_AML_KASLR_SEED

#endif

