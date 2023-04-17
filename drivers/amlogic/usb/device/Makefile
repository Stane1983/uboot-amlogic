# SPDX-License-Identifier: GPL-2.0
#

ifndef CONFIG_USB_GADGET_CRG
obj-$(CONFIG_FASTBOOT_DWC_PCD) += dwc_pcd.o dwc_pcd_irq.o
else
obj-$(CONFIG_USB_GADGET_CRG) += crg_udc.o
endif

obj-$(CONFIG_USB_GADGET) +=  platform.o
