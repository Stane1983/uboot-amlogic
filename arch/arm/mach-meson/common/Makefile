# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (c) 2016 Beniamino Galvani <b.galvani@gmail.com>

obj-y += cpu_id.o

ifdef CONFIG_AML_AVB2_ANTIROLLBACK
obj-y += avb2-anti-rollback.o
else
obj-$(CONFIG_AML_ANTIROLLBACK) += anti-rollback.o
endif

