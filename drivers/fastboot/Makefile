# SPDX-License-Identifier: GPL-2.0+

obj-y += fb_common.o
ifdef CONFIG_AMLOGIC_MODIFY
obj-y += fb_getvar_aml.o
else
obj-y += fb_getvar.o
endif
obj-y += fb_command.o
obj-$(CONFIG_FASTBOOT_FLASH_MMC) += fb_mmc.o
obj-$(CONFIG_FASTBOOT_FLASH_NAND) += fb_nand.o
