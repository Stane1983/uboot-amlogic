// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/string.h>
#include <amlogic/media/vout/hdmitx_common.h>
#include <common.h>

/* Recommended N and Expected CTS for 32kHz */
static const struct hdmi_audio_fs_ncts aud_32k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 4576,
		.cts = 28125,
		.n_30bit = 9152,
		.cts_30bit = 70312,
		.n_36bit = 9152,
		.cts_36bit = 84375,
		.n_48bit = 4576,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 4096,
		.cts = 25200,
		.n_30bit = 4096,
		.cts_30bit = 31500,
		.n_36bit = 4096,
		.cts_36bit = 37800,
		.n_48bit = 4096,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 4096,
		.cts = 27000,
		.n_30bit = 4096,
		.cts_30bit = 33750,
		.n_36bit = 4096,
		.cts_36bit = 40500,
		.n_48bit = 4096,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 4096,
		.cts = 27027,
		.n_30bit = 8192,
		.cts_30bit = 67567,
		.n_36bit = 8192,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 4096,
		.cts = 54000,
		.n_30bit = 4096,
		.cts_30bit = 67500,
		.n_36bit = 4096,
		.cts_36bit = 81000,
		.n_48bit = 4096,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 4096,
		.cts = 54054,
		.n_30bit = 8192,
		.cts_30bit = 135135,
		.n_36bit = 4096,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 210937,
		.n_30bit = 11648,
		.cts_30bit = 263671,
		.n_36bit = 11648,
		.cts_36bit = 316406,
		.n_48bit = 11648,
		.cts_48bit = 421875,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 4096,
		.cts = 74250,
		.n_30bit = 8192,
		.cts_30bit = 185625,
		.n_36bit = 4096,
		.cts_36bit = 111375,
		.n_48bit = 4096,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 11648,
		.cts = 421875,
		.n_30bit = 11648,
		.cts_30bit = 527343,
		.n_36bit = 11648,
		.cts_36bit = 632812,
		.n_48bit = 11648,
		.cts_48bit = 843750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 4096,
		.cts = 148500,
		.n_30bit = 4096,
		.cts_30bit = 185625,
		.n_36bit = 4096,
		.cts_36bit = 222750,
		.n_48bit = 4096,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 421875,
		.n_30bit = 5824,
		.cts_30bit = 527343,
		.n_36bit = 5824,
		.cts_36bit = 632812,
		.n_48bit = 5824,
		.cts_48bit = 843750,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 3072,
		.cts = 222750,
		.n_30bit = 6144,
		.cts_30bit = 556875,
		.n_36bit = 4096,
		.cts_36bit = 445500,
		.n_48bit = 3072,
		.cts_48bit = 445500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 843750,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 3072,
		.cts = 445500,
	},
	.def_n = 4096,
};

/* Recommended N and Expected CTS for 44.1kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_44k1_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 7007,
		.cts = 31250,
		.n_30bit = 14014,
		.cts_30bit = 78125,
		.n_36bit = 7007,
		.cts_36bit = 46875,
		.n_48bit = 7007,
		.cts_48bit = 62500,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6272,
		.cts = 28000,
		.n_30bit = 6272,
		.cts_30bit = 35000,
		.n_36bit = 6272,
		.cts_36bit = 42000,
		.n_48bit = 6272,
		.cts_48bit = 56000,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6272,
		.cts = 30000,
		.n_30bit = 6272,
		.cts_30bit = 37500,
		.n_36bit = 6272,
		.cts_36bit = 45000,
		.n_48bit = 6272,
		.cts_48bit = 60000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6272,
		.cts = 30030,
		.n_30bit = 12544,
		.cts_30bit = 75075,
		.n_36bit = 6272,
		.cts_36bit = 45045,
		.n_48bit = 6272,
		.cts_48bit = 60060,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6272,
		.cts = 60000,
		.n_30bit = 6272,
		.cts_30bit = 75000,
		.n_36bit = 6272,
		.cts_36bit = 90000,
		.n_48bit = 6272,
		.cts_48bit = 120000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6272,
		.cts = 60060,
		.n_30bit = 6272,
		.cts_30bit = 75075,
		.n_36bit = 6272,
		.cts_36bit = 90090,
		.n_48bit = 6272,
		.cts_48bit = 120120,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 17836,
		.cts = 234375,
		.n_30bit = 17836,
		.cts_30bit = 292968,
		.n_36bit = 17836,
		.cts_36bit = 351562,
		.n_48bit = 17836,
		.cts_48bit = 468750,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6272,
		.cts = 82500,
		.n_30bit = 6272,
		.cts_30bit = 103125,
		.n_36bit = 6272,
		.cts_36bit = 123750,
		.n_48bit = 6272,
		.cts_48bit = 165000,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 8918,
		.cts = 234375,
		.n_30bit = 17836,
		.cts_30bit = 585937,
		.n_36bit = 17836,
		.cts_36bit = 703125,
		.n_48bit = 8918,
		.cts_48bit = 468750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6272,
		.cts = 165000,
		.n_30bit = 6272,
		.cts_30bit = 206250,
		.n_36bit = 6272,
		.cts_36bit = 247500,
		.n_48bit = 6272,
		.cts_48bit = 330000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 4459,
		.cts = 234375,
		.n_30bit = 8918,
		.cts_30bit = 585937,
		.n_36bit = 8918,
		.cts_36bit = 703125,
		.n_48bit = 4459,
		.cts_48bit = 468750,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 4707,
		.cts = 247500,
		.n_30bit = 4704,
		.cts_30bit = 309375,
		.n_36bit = 4704,
		.cts_36bit = 371250,
		.n_48bit = 4704,
		.cts_48bit = 495000,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 8918,
		.cts = 937500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 9408,
		.cts = 990000,
	},
	.def_n = 6272,
};

/* Recommended N and Expected CTS for 48kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_48k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 6864,
		.cts = 28125,
		.n_30bit = 9152,
		.cts_30bit = 46875,
		.n_36bit = 9152,
		.cts_36bit = 58250,
		.n_48bit = 6864,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6144,
		.cts = 25200,
		.n_30bit = 6144,
		.cts_30bit = 31500,
		.n_36bit = 6144,
		.cts_36bit = 37800,
		.n_48bit = 6144,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6144,
		.cts = 27000,
		.n_30bit = 6144,
		.cts_30bit = 33750,
		.n_36bit = 6144,
		.cts_36bit = 40500,
		.n_48bit = 6144,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6144,
		.cts = 27027,
		.n_30bit = 8192,
		.cts_30bit = 45045,
		.n_36bit = 8192,
		.cts_36bit = 54054,
		.n_48bit = 6144,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6144,
		.cts = 54000,
		.n_30bit = 6144,
		.cts_30bit = 67500,
		.n_36bit = 6144,
		.cts_36bit = 81000,
		.n_48bit = 6144,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6144,
		.cts = 54054,
		.n_30bit = 8192,
		.cts_30bit = 90090,
		.n_36bit = 6144,
		.cts_36bit = 81081,
		.n_48bit = 6144,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 140625,
		.n_30bit = 11648,
		.cts_30bit = 175781,
		.n_36bit = 11648,
		.cts_36bit = 210937,
		.n_48bit = 11648,
		.cts_48bit = 281250,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6144,
		.cts = 74250,
		.n_30bit = 12288,
		.cts_30bit = 185625,
		.n_36bit = 6144,
		.cts_36bit = 111375,
		.n_48bit = 6144,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 5824,
		.cts = 140625,
		.n_30bit = 11648,
		.cts_30bit = 351562,
		.n_36bit = 11648,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 281250,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6144,
		.cts = 148500,
		.n_30bit = 6144,
		.cts_30bit = 185625,
		.n_36bit = 6144,
		.cts_36bit = 222750,
		.n_48bit = 6144,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 281250,
		.n_30bit = 11648,
		.cts_30bit = 703125,
		.n_36bit = 5824,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 562500,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 5120,
		.cts = 247500,
		.n_30bit = 5120,
		.cts_30bit = 309375,
		.n_36bit = 5120,
		.cts_36bit = 371250,
		.n_48bit = 5120,
		.cts_48bit = 495000,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 562500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 6144,
		.cts = 594000,
	},
	.def_n = 6144,
};

static const struct hdmi_audio_fs_ncts *all_aud_paras[] = {
	&aud_32k_para,
	&aud_44k1_para,
	&aud_48k_para,
	NULL,
};

/*
 * note: param tmds_clk is actually pixel_clk
 * for 8bit mode, use param cd for ACR_N of
 * deep color mode
 */
u32 hdmitx_hw_get_audio_n_paras(enum hdmi_audio_fs fs,
				int cd,
				u32 tmds_clk)
{
	const struct hdmi_audio_fs_ncts *p = NULL;
	u32 i, n;
	u32 N_multiples = 1;

	printf("audio: fs = %d, cd = %d, tmds_clk = %d\n", fs, cd, tmds_clk);
	switch (fs) {
	case FS_32K:
		p = all_aud_paras[0];
		N_multiples = 1;
		break;
	case FS_44K1:
		p = all_aud_paras[1];
		N_multiples = 1;
		break;
	case FS_88K2:
		p = all_aud_paras[1];
		N_multiples = 2;
		break;
	case FS_176K4:
		p = all_aud_paras[1];
		N_multiples = 4;
		break;
	case FS_48K:
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	case FS_96K:
		p = all_aud_paras[2];
		N_multiples = 2;
		break;
	case FS_192K:
		p = all_aud_paras[2];
		N_multiples = 4;
		break;
	default: /* Default as FS_48K */
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	}
	for (i = 0; i < AUDIO_PARA_MAX_NUM; i++) {
		if (tmds_clk == p->array[i].tmds_clk ||
		    (tmds_clk + 1) == p->array[i].tmds_clk ||
		    (tmds_clk - 1) == p->array[i].tmds_clk)
			break;
	}

	if (i < AUDIO_PARA_MAX_NUM)
		if (cd == 4)
			n = p->array[i].n ? p->array[i].n : p->def_n;
		else if (cd == 5)
			n = p->array[i].n_30bit ?
				p->array[i].n_30bit : p->def_n;
		else if (cd == 6)
			n = p->array[i].n_36bit ?
				p->array[i].n_36bit : p->def_n;
		else if (cd == 7)
			n = p->array[i].n_48bit ?
				p->array[i].n_48bit : p->def_n;
		else
			n = p->array[i].n ? p->array[i].n : p->def_n;
	else
		n = p->def_n;
	return n * N_multiples;
}

u32 hdmi21_get_frl_aud_n_paras(enum hdmi_audio_fs fs,
			       u32 frl_rate)
{
	u32 base32k_n = 6048;
	u32 base44k1_n = 5292;
	u32 base48k_n = 5760;

	switch (frl_rate) {
	case 1:
		base32k_n = 6048;
		base44k1_n = 5292;
		base48k_n = 5760;
		break;
	case 2:
	case 3:
		base32k_n = 4032;
		base44k1_n = 5292;
		base48k_n = 6048;
		break;
	case 4:
		base32k_n = 4032;
		base44k1_n = 3969;
		base48k_n = 6048;
		break;
	case 5:
		base32k_n = 3456;
		base44k1_n = 3969;
		base48k_n = 5184;
		break;
	case 6:
		base32k_n = 3072;
		base44k1_n = 3969;
		base48k_n = 4752;
		break;
	default:
		break;
	}
	switch (fs) {
	case FS_32K:
		return base32k_n * 1;
	case FS_44K1:
		return base44k1_n * 1;
	case FS_88K2:
		return base44k1_n * 2;
	case FS_176K4:
		return base44k1_n * 4;
	case FS_48K:
		return base48k_n * 1;
	case FS_96K:
		return base48k_n * 2;
	case FS_192K:
		return base48k_n * 4;
	default:
		return base48k_n;
	}
}
