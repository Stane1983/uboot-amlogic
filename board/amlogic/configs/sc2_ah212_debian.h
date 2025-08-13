/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __DEBIAN_BOARD_CFG_H__
#define __DEBIAN_BOARD_CFG_H__

/*define system of debian private env*/
#ifdef CONFIG_DTB_LOAD
#undef CONFIG_DTB_LOAD
#define CONFIG_DTB_LOAD \
	"if test ${boot_source} = emmc; then "\
	"echo Load dtb/${fdtfile} from eMMC (1:1) ...;" \
	"load mmc 1:4 ${dtb_mem_addr} dtb/${fdtfile};" \
	"else if test ${boot_source} = sd; then "\
	"echo Load dtb/${fdtfile} from SD (0:1) ...;" \
	"load mmc 0:4 ${dtb_mem_addr} dtb/${fdtfile};" \
	"fi;fi;"
#endif

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifndef BOOT_TARGET_DEVICES
#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)
#endif

#include <config_distro_bootcmd.h>

#undef CONFIG_SYS_CBSIZE
#define CONFIG_SYS_CBSIZE 1024

/* debian private env, cmds */
#undef SYSTEM_PRI_ENV_SETTINGS
#define SYSTEM_PRI_ENV_SETTINGS \
	"fdt_addr_r=0x20000000\0"\
	"fdtoverlay_addr_r=0x00a00000\0"\
	"fdtaddr=0x020000000\0"\
	"kernel_addr_r=0x01080000\0"\
	"scriptaddr=0x00010000\0" \
	"pxefile_addr_r=0x00010000\0"\
	"ramdisk_addr_r=0x10000000\0"\
	"kernel_comp_addr_r=0x1080000\0"\
	"kernel_comp_size=0x2000000\0"\
	"pxeuuid=00000000-0000-0000-0000-000000000000\0"\
	"fdtfile=sc2_s905x4_ah212_linux.dtb\0"\
	"display_bpp=24\0" \
	"display_color_index=24\0" \
	"tftp_kernel_path=boot/Image \0" \
	"tftp_dtb_path=boot/dtb/ \0" \
	"tftp_initrd_path=boot/initrd.img \0" \
	"nfsroot_path= \0" \
	"initargs="\
		"init=data=writeback rw rootfstype=ext4 loglevel=4 "\
		"console=ttyS0,921600 console=tty0 no_console_suspend "\
		"earlycon=aml_uart,0xfe07a000 ramoops.pstore_en=1 ramoops.record_size=0x8000 "\
		"ramoops.console_size=0x4000 loop.max_part=4 scsi_mod.scan=async "\
		"xhci_hcd.quirks=0x800000 scramble_reg=0xfe02e030 check_tty_en=1 "\
		"\0"

#undef SYSTEM_PRI_COMMANDS
#define SYSTEM_PRI_COMMANDS \
	"nfs_boot="\
		"dhcp;"\
		"setenv nfs_para root=/dev/nfs rw "\
			"nfsroot=${serverip}:${nfsroot_path} ip=:::::eth0:on;"\
		"printenv nfs_para;"\
		"setenv bootargs ${bootargs} ${nfs_para};"\
		"tftp ${dtb_mem_addr} ${tftp_dtb_path}${fdtfile};"\
		"tftp ${loadaddr_kernel} ${tftp_kernel_path};"\
		"tftp ${ramdisk_addr_r} ${tftp_initrd_path};"\
		"setenv ramdisk_size ${filesize};"\
		"echo ramdisk_size=${ramdisk_size};"\
		"booti ${loadaddr_kernel} ${ramdisk_addr_r}:${ramdisk_size} ${dtb_mem_addr};"\
		"\0"\
	"storeargs="\
		"get_bootloaderversion;" \
		"run storeargs_base;"\
		"setenv bootargs ${bootargs} cma_first_wm_low=on kvm-arm.mode=none "\
			"init_on_alloc=0 boot_source=${boot_source};"\
		"run cmdline_keys;"\
		"\0"\
	"load_bmp_logo=" \
		"if load mmc 1:2 ${loadaddr} /usr/share/amlbian/logo/logo.bmp || "\
			"load mmc 1:4 ${loadaddr} /usr/share/amlbian/logo/logo.bmp; then "\
			"bmp display ${loadaddr};"\
			"bmp scale;"\
		"fi;"\
	"\0" \
	"init_display="\
		"get_rebootmode;"\
		"echo reboot_mode:::: ${reboot_mode};"\
		"hdmitx hpd;hdmitx get_parse_edid;"\
		"dovi process;watermark_init;osd open;osd clear;run load_bmp_logo;"\
		"bmp scale;vout output ${outputmode};dovi set;dovi pkg;vpp hdrpkt;"\
		"\0"\
	BOOTENV\
	"pxe_boot=dhcp; pxe get && pxe boot\0"\
	"bootcmd_storeboot=run storeboot\0"\
	"boot_targets=spi usb0 mmc0 mmc1 storeboot pxe dhcp\0"

#endif // ENDIF __DEBIAN_BOARD_CFG_H__

