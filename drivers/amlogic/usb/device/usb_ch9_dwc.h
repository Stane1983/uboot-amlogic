/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __LINUX_USB_CH9_DWC_H
#define __LINUX_USB_CH9__DWC_H

/** USB_DT_ENDPOINT: Endpoint descriptor */
struct usb_endpoint_descriptor_udc {
        __u8  bLength;
        __u8  bDescriptorType;

        __u8  bEndpointAddress;
        __u8  bmAttributes;
        __u16 wMaxPacketSize;
        __u8  bInterval;

        /// NOTE:  these two are _only_ in audio endpoints.
        /// use USB_DT_ENDPOINT*_SIZE in bLength, not sizeof.
        __u8  bRefresh;
        __u8  bSynchAddress;

        /// Extra descriptors

        unsigned char *extra;
        int extralen;
} __attribute__ ((packed));
typedef struct usb_endpoint_descriptor_udc usb_endpoint_descriptor_t;


#endif  /* __LINUX_USB_CH9__UDC_H */

