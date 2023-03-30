# SPDX-License-Identifier: GPL-2.0+

dtb-$(CONFIG_TARGET_S4_AP222) += \
	meson-s4-ap222.dtb \

include $(srctree)/scripts/Makefile.dts

targets += $(dtb-y)

# Add any required device tree compiler flags here
DTC_FLAGS += -a 0x8

PHONY += dtbs
dtbs: $(addprefix $(obj)/, $(dtb-y))
	@:

clean-files := *.dtb *.dtbo *_HS
