/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AUGE_SOUND_H__
#define __AUGE_SOUND_H__

/* auge audio register is defined in asm/arch/secure_apb.h */

/* For earc rx, from sc2 has 4 bits trim value */
#define EARC_RX_ANA_V1 1
/* For earc rx, from t7 has 5 bits trim value */
#define EARC_RX_ANA_V2 2

void update_bits(unsigned int reg, unsigned int mask, unsigned int val);

int aml_audio_init(void);

/* earcrx */
void earcrx_init(int version);

#endif
