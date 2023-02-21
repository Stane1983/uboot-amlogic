#// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#/*
# * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
# */


EXTRA_CFLAGS = -I./v2_common -I./v2_usb_tool -I./v2_sdc_burn

obj-$(CONFIG_AML_V2_FACTORY_BURN) += aml_v2_burn.o
aml_v2_burn-y = aml_v2_burning.o
aml_v2_burn-y += v2_common/optimus_download.o v2_common/optimus_buffer_manager.o v2_common/optimus_simg2img.o
aml_v2_burn-y += v2_common/optimus_progress.o
aml_v2_burn-$(CONFIG_UNIFY_KEY_MANAGE) += v2_common/optimus_download_key.o

ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE
aml_v2_burn-y += v2_common/optimus_img_decoder.o v2_common/optimus_fat.o
aml_v2_burn-y += v2_sdc_burn/optimus_sdc_update.o v2_sdc_burn/optimus_sdc_burn.o v2_sdc_burn/optimus_ini_parser.o
aml_v2_burn-y += v2_sdc_burn/optimus_ini__aml_sdc_burn.o
aml_v2_burn-y += v2_usb_burn/optimus_usb_burn.o v2_usb_burn/optimus_usb_update.o
aml_v2_burn-$(CONFIG_SD_BURNING_SUPPORT_UI) += v2_common/optimus_progress_ui.o
aml_v2_burn-$(CONFIG_SD_BURNING_SUPPORT_LED) += v2_sdc_burn/optimus_led.o
aml_v2_burn-y += aml_sysrecovery/aml_sysrecovery.o
aml_v2_burn-$(CONFIG_DETECT_SYS_RECOVERY_KEY) += aml_sysrecovery/cmd_detect_sys_recovery_key.o
aml_v2_burn-$(CONFIG_SUPPORT_SDC_KEYBURN) += v2_sdc_burn/sdc_burnkeys/optimus_key_burn.o
aml_v2_burn-$(CONFIG_SUPPORT_SDC_KEYBURN) += v2_sdc_burn/sdc_burnkeys/sdc_keysprovider.o v2_sdc_burn/sdc_burnkeys/sdc_bootPart_license.o
endif#ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE

