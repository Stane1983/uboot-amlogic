# SPDX-License-Identifier: GPL-2.0+
#

obj-$(CONFIG_CMD_MMC) += aml_mmc.o
obj-$(CONFIG_CMD_PLLTEST) += cmd_plltest.o

obj-$(CONFIG_AML_STORE_WRAPPER) += store_wrapper.o
#obj-$(CONFIG_AML_IMG_READ) += imgread.o
obj-$(CONFIG_AML_DEFENV) += defenv_without.o
#obj-$(CONFIG_AML_ROBUST) += bootloader_status.o

obj-$(CONFIG_AML_REBOOT) += cmd_reboot.o
obj-$(CONFIG_CMD_RSVMEM) += cmd_rsvmem.o

obj-y += cmd_version.o

obj-$(CONFIG_CMD_CAR_PARAMS) += cmd_car_param.o

obj-$(CONFIG_AML_RPMB) += cmd_rpmb.o
obj-$(CONFIG_CMD_BOOTCTOL_AVB) += cmd_avb.o
obj-$(CONFIG_CMD_DDR_D2PLL) += cmd_d2pll.o

ifdef CONFIG_NASC_NAGRA_TIER_1
else

ifdef CONFIG_MESON_P1
obj-y += cmd_ddr_test_p1.o
else

ifdef CONFIG_MESON_S5
obj-y += cmd_ddr_test_s5.o
else

ifdef CONFIG_MESON_T5M
obj-y += cmd_ddr_test_t5m.o
else
obj-y += cmd_ddr_test_g12.o
endif
endif

endif

endif

obj-$(CONFIG_AML_STORAGE) += storage.o

obj-$(CONFIG_CMD_OS_IDENT) += os_ident.o
obj-$(CONFIG_AML_FACTORY_PROVISION) += cmd_factory_provision.o
obj-$(CONFIG_AML_WATERMARK) += cmd_watermark.o
obj-$(CONFIG_SECURE_POWER_CONTROL) += cmd_power.o
obj-$(CONFIG_CMD_CEC) += cmd_cec.o
obj-$(CONFIG_CMD_PMU) += cmd_pmic.o
obj-$(CONFIG_CMD_RMA) += cmd_rma.o
obj-$(CONFIG_CMD_CLKMSR)+= cmd_clkmsr.o
obj-$(CONFIG_CMD_IR) += cmd_irkey.o
