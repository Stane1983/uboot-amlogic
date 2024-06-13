#
# (C) Copyright 2020
# hdmitx_mode.o hdmitx_hw_common.o hdmitx_format_para.o

#EXTRA_CFLAGS = -Idrivers/video/include/

#obj-y += hdmitx_common.o
obj-$(CONFIG_AML_HDMITX20) += hdmitx20_common.o
hdmitx20_common-y += mode_policy.o mode_policy_parser.o hdmitx_policy_setting.o

obj-$(CONFIG_AML_HDMITX21) += hdmitx21_common.o
hdmitx21_common-y := hdmitx_edid_parse.o hdmitx_check_valid.o hdmitx_compliance.o
hdmitx21_common-y += mode_policy.o mode_policy_parser.o hdmitx_policy_setting.o

