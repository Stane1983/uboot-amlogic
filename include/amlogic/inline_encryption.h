/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __INLINE_ENCRYPTION_H__
#define __INLINE_ENCRYPTION_H__

int derive_inline_enc_key(u8 *key, u32 key_sz);
int derive_inline_enc_usb_key(u8 *key, u32 key_sz);

#endif//#ifndef __INLINE_ENCRYPTION_H__

