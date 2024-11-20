/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMI_COMMON_H__
#define __HDMI_COMMON_H__

#include <hdmi.h>
#include "../hdmitx_common.h"

#define DDC_EDID_ADDR 0xA0
	#define DDC_EDIDSEG_ADDR 0x30
#define DDC_SCDC_ADDR 0xA8

#define SCDC_UPDATE_0 0x10
#define SCDC_CONFIG_1 0x31
#define RSED_UPDATE             BIT(6)
#define FLT_UPDATE              BIT(5)
#define FRL_START               BIT(4)
#define SOURCE_TEST_UPDATE      BIT(3)
#define READ_REQUEST_TEST       BIT(2)
#define CED_UPDATE              BIT(1)
#define STATUS_UPDATE           BIT(0)
#define HDMI21_UPDATE_FLAGS     \
	(RSED_UPDATE | FLT_UPDATE | FRL_START | SOURCE_TEST_UPDATE)
#define HDMI20_UPDATE_FLAGS     \
	(SOURCE_TEST_UPDATE | READ_REQUEST_TEST | CED_UPDATE | STATUS_UPDATE)

#define HDMI_PACKET_TYPE_GCP 0x3
#define HDMITX_VIC420_OFFSET    0x100
#define HDMI_INFOFRAME_TYPE_SBTM 0xA //SBTM-EM PKT use GEN5

#define VESA_MAX_TIMING 64
/* refer to hdmi2.1 table 7-36, 32 VIC support y420 */
#define Y420_VIC_MAX_NUM 32

#define HDMITX_VESA_OFFSET 0x300
/* Little-Endian format */
enum scdc_addr {
	SINK_VER = 0x01,
	SOURCE_VER, /* RW */
	UPDATE_0 = 0x10, /* RW */
	UPDATE_1, /* RW */
	TMDS_CFG = 0x20, /* RW */
	SCRAMBLER_ST,
	CONFIG_0 = 0x30, /* RW */
	STATUS_FLAGS_0 = 0x40,
	STATUS_FLAGS_1,
	ERR_DET_0_L = 0x50,
	ERR_DET_0_H,
	ERR_DET_1_L,
	ERR_DET_1_H,
	ERR_DET_2_L,
	ERR_DET_2_H,
	ERR_DET_CHKSUM,
	TEST_CONFIG_0 = 0xC0, /* RW */
	MANUFACT_IEEE_OUI_2 = 0xD0,
	MANUFACT_IEEE_OUI_1,
	MANUFACT_IEEE_OUI_0,
	DEVICE_ID = 0xD3, /* 0xD3 ~ 0xDD */
	/* RW   0xDE ~ 0xFF */
	MANUFACT_SPECIFIC = 0xDE,
};

/* HDMI VIC definitions */
enum hdmi_vic {
	/* Refer to CEA 861-D */
	HDMI_UNKNOWN = 0,
	HDMI_1_640x480p60_4x3		= 1,
	HDMI_2_720x480p60_4x3		= 2,
	HDMI_3_720x480p60_16x9		= 3,
	HDMI_4_1280x720p60_16x9		= 4,
	HDMI_5_1920x1080i60_16x9	= 5,
	HDMI_6_720x480i60_4x3		= 6,
	HDMI_7_720x480i60_16x9		= 7,
	HDMI_8_720x240p60_4x3		= 8,
	HDMI_9_720x240p60_16x9		= 9,
	HDMI_10_2880x480i60_4x3		= 10,
	HDMI_11_2880x480i60_16x9	= 11,
	HDMI_12_2880x240p60_4x3		= 12,
	HDMI_13_2880x240p60_16x9	= 13,
	HDMI_14_1440x480p60_4x3		= 14,
	HDMI_15_1440x480p60_16x9	= 15,
	HDMI_16_1920x1080p60_16x9	= 16,
	HDMI_17_720x576p50_4x3		= 17,
	HDMI_18_720x576p50_16x9		= 18,
	HDMI_19_1280x720p50_16x9	= 19,
	HDMI_20_1920x1080i50_16x9	= 20,
	HDMI_21_720x576i50_4x3		= 21,
	HDMI_22_720x576i50_16x9		= 22,
	HDMI_23_720x288p_4x3		= 23,
	HDMI_24_720x288p_16x9		= 24,
	HDMI_25_2880x576i50_4x3		= 25,
	HDMI_26_2880x576i50_16x9	= 26,
	HDMI_27_2880x288p50_4x3		= 27,
	HDMI_28_2880x288p50_16x9	= 28,
	HDMI_29_1440x576p_4x3		= 29,
	HDMI_30_1440x576p_16x9		= 30,
	HDMI_31_1920x1080p50_16x9	= 31,
	HDMI_32_1920x1080p24_16x9	= 32,
	HDMI_33_1920x1080p25_16x9	= 33,
	HDMI_34_1920x1080p30_16x9	= 34,
	HDMI_35_2880x480p60_4x3		= 35,
	HDMI_36_2880x480p60_16x9	= 36,
	HDMI_37_2880x576p50_4x3		= 37,
	HDMI_38_2880x576p50_16x9	= 38,
	HDMI_39_1920x1080i_t1250_50_16x9 = 39,
	HDMI_40_1920x1080i100_16x9	= 40,
	HDMI_41_1280x720p100_16x9	= 41,
	HDMI_42_720x576p100_4x3		= 42,
	HDMI_43_720x576p100_16x9	= 43,
	HDMI_44_720x576i100_4x3		= 44,
	HDMI_45_720x576i100_16x9	= 45,
	HDMI_46_1920x1080i120_16x9	= 46,
	HDMI_47_1280x720p120_16x9	= 47,
	HDMI_48_720x480p120_4x3		= 48,
	HDMI_49_720x480p120_16x9	= 49,
	HDMI_50_720x480i120_4x3		= 50,
	HDMI_51_720x480i120_16x9	= 51,
	HDMI_52_720x576p200_4x3		= 52,
	HDMI_53_720x576p200_16x9	= 53,
	HDMI_54_720x576i200_4x3		= 54,
	HDMI_55_720x576i200_16x9	= 55,
	HDMI_56_720x480p240_4x3		= 56,
	HDMI_57_720x480p240_16x9	= 57,
	HDMI_58_720x480i240_4x3		= 58,
	HDMI_59_720x480i240_16x9	= 59,
	HDMI_60_1280x720p24_16x9	= 60,
	HDMI_61_1280x720p25_16x9	= 61,
	HDMI_62_1280x720p30_16x9	= 62,
	HDMI_63_1920x1080p120_16x9	= 63,
	HDMI_64_1920x1080p100_16x9	= 64,
	HDMI_65_1280x720p24_64x27	= 65,
	HDMI_66_1280x720p25_64x27	= 66,
	HDMI_67_1280x720p30_64x27	= 67,
	HDMI_68_1280x720p50_64x27	= 68,
	HDMI_69_1280x720p60_64x27	= 69,
	HDMI_70_1280x720p100_64x27	= 70,
	HDMI_71_1280x720p120_64x27	= 71,
	HDMI_72_1920x1080p24_64x27	= 72,
	HDMI_73_1920x1080p25_64x27	= 73,
	HDMI_74_1920x1080p30_64x27	= 74,
	HDMI_75_1920x1080p50_64x27	= 75,
	HDMI_76_1920x1080p60_64x27	= 76,
	HDMI_77_1920x1080p100_64x27	= 77,
	HDMI_78_1920x1080p120_64x27	= 78,
	HDMI_79_1680x720p24_64x27	= 79,
	HDMI_80_1680x720p25_64x27	= 80,
	HDMI_81_1680x720p30_64x27	= 81,
	HDMI_82_1680x720p50_64x27	= 82,
	HDMI_83_1680x720p60_64x27	= 83,
	HDMI_84_1680x720p100_64x27	= 84,
	HDMI_85_1680x720p120_64x27	= 85,
	HDMI_86_2560x1080p24_64x27	= 86,
	HDMI_87_2560x1080p25_64x27	= 87,
	HDMI_88_2560x1080p30_64x27	= 88,
	HDMI_89_2560x1080p50_64x27	= 89,
	HDMI_90_2560x1080p60_64x27	= 90,
	HDMI_91_2560x1080p100_64x27	= 91,
	HDMI_92_2560x1080p120_64x27	= 92,
	HDMI_93_3840x2160p24_16x9	= 93,
	HDMI_94_3840x2160p25_16x9	= 94,
	HDMI_95_3840x2160p30_16x9	= 95,
	HDMI_96_3840x2160p50_16x9	= 96,
	HDMI_97_3840x2160p60_16x9	= 97,
	HDMI_98_4096x2160p24_256x135	= 98,
	HDMI_99_4096x2160p25_256x135	= 99,
	HDMI_100_4096x2160p30_256x135	= 100,
	HDMI_101_4096x2160p50_256x135	= 101,
	HDMI_102_4096x2160p60_256x135	= 102,
	HDMI_103_3840x2160p24_64x27	= 103,
	HDMI_104_3840x2160p25_64x27	= 104,
	HDMI_105_3840x2160p30_64x27	= 105,
	HDMI_106_3840x2160p50_64x27	= 106,
	HDMI_107_3840x2160p60_64x27	= 107,
	HDMI_108_1280x720p48_16x9	= 108,
	HDMI_109_1280x720p48_64x27	= 109,
	HDMI_110_1680x720p48_64x27	= 110,
	HDMI_111_1920x1080p48_16x9	= 111,
	HDMI_112_1920x1080p48_64x27	= 112,
	HDMI_113_2560x1080p48_64x27	= 113,
	HDMI_114_3840x2160p48_16x9	= 114,
	HDMI_115_4096x2160p48_256x135	= 115,
	HDMI_116_3840x2160p48_64x27	= 116,
	HDMI_117_3840x2160p100_16x9	= 117,
	HDMI_118_3840x2160p120_16x9	= 118,
	HDMI_119_3840x2160p100_64x27	= 119,
	HDMI_120_3840x2160p120_64x27	= 120,
	HDMI_121_5120x2160p24_64x27	= 121,
	HDMI_122_5120x2160p25_64x27	= 122,
	HDMI_123_5120x2160p30_64x27	= 123,
	HDMI_124_5120x2160p48_64x27	= 124,
	HDMI_125_5120x2160p50_64x27	= 125,
	HDMI_126_5120x2160p60_64x27	= 126,
	HDMI_127_5120x2160p100_64x27	= 127,
	/* 127 ~ 192 reserved */
	HDMI_193_5120x2160p120_64x27	= 193,
	HDMI_194_7680x4320p24_16x9	= 194,
	HDMI_195_7680x4320p25_16x9	= 195,
	HDMI_196_7680x4320p30_16x9	= 196,
	HDMI_197_7680x4320p48_16x9	= 197,
	HDMI_198_7680x4320p50_16x9	= 198,
	HDMI_199_7680x4320p60_16x9	= 199,
	HDMI_200_7680x4320p100_16x9	= 200,
	HDMI_201_7680x4320p120_16x9	= 201,
	HDMI_202_7680x4320p24_64x27	= 202,
	HDMI_203_7680x4320p25_64x27	= 203,
	HDMI_204_7680x4320p30_64x27	= 204,
	HDMI_205_7680x4320p48_64x27	= 205,
	HDMI_206_7680x4320p50_64x27	= 206,
	HDMI_207_7680x4320p60_64x27	= 207,
	HDMI_208_7680x4320p100_64x27	= 208,
	HDMI_209_7680x4320p120_64x27	= 209,
	HDMI_210_10240x4320p24_64x27	= 210,
	HDMI_211_10240x4320p25_64x27	= 211,
	HDMI_212_10240x4320p30_64x27	= 212,
	HDMI_213_10240x4320p48_64x27	= 213,
	HDMI_214_10240x4320p50_64x27	= 214,
	HDMI_215_10240x4320p60_64x27	= 215,
	HDMI_216_10240x4320p100_64x27	= 216,
	HDMI_217_10240x4320p120_64x27	= 217,
	HDMI_218_4096x2160p100_256x135	= 218,
	HDMI_219_4096x2160p120_256x135	= 219,
	HDMI_CEA_VIC_END,

	/*Vesa mode which dont have vic, we specify value for them also*/
	HDMIV_0_640x480p60hz = HDMITX_VESA_OFFSET,
	HDMIV_1_800x480p60hz,
	HDMIV_2_800x600p60hz,
	HDMIV_3_852x480p60hz,
	HDMIV_4_854x480p60hz,
	HDMIV_5_1024x600p60hz,
	HDMIV_6_1024x768p60hz,
	HDMIV_7_1152x864p75hz,
	HDMIV_8_1280x768p60hz,
	HDMIV_9_1280x800p60hz,
	HDMIV_10_1280x960p60hz,
	HDMIV_11_1280x1024p60hz,
	HDMIV_12_1360x768p60hz,
	HDMIV_13_1366x768p60hz,
	HDMIV_14_1400x1050p60hz,
	HDMIV_15_1440x900p60hz,
	HDMIV_16_1440x2560p60hz,
	HDMIV_17_1600x900p60hz,
	HDMIV_18_1600x1200p60hz,
	HDMIV_19_1680x1050p60hz,
	HDMIV_20_1920x1200p60hz,
	HDMIV_21_2048x1080p24hz,
	HDMIV_22_2160x1200p90hz,
	HDMIV_23_2560x1600p60hz,
	HDMIV_24_3440x1440p60hz,
	HDMIV_25_2400x1200p90hz,
	HDMIV_26_3840x1080p60hz,
	/*not supported in timing*/
	HDMIV_2560x1080p60hz,
	HDMIV_2560x1440p60hz,
	HDMIV_1280x600p60hz,
	HDMIV_2560x1600p60hz,
	HDMIV_3440x1440p60hz,
	HDMIV_2400x1200p90hz,

	HDMI_VIC_END = 0xFFFF,
};

/* Compliance with old definitions */
#define HDMIV_640x480p60hz		HDMIV_0_640x480p60hz
#define HDMIV_800x480p60hz		HDMIV_1_800x480p60hz
#define HDMIV_800x600p60hz		HDMIV_2_800x600p60hz
#define HDMIV_852x480p60hz		HDMIV_3_852x480p60hz
#define HDMIV_854x480p60hz		HDMIV_4_854x480p60hz
#define HDMIV_1024x600p60hz		HDMIV_5_1024x600p60hz
#define HDMIV_1024x768p60hz		HDMIV_6_1024x768p60hz
#define HDMIV_1152x864p75hz		HDMIV_7_1152x864p75hz
#define HDMIV_1280x768p60hz		HDMIV_8_1280x768p60hz
#define HDMIV_1280x800p60hz		HDMIV_9_1280x800p60hz
#define HDMIV_1280x960p60hz		HDMIV_10_1280x960p60hz
#define HDMIV_1280x1024p60hz	HDMIV_11_1280x1024p60hz
#define HDMIV_1360x768p60hz		HDMIV_12_1360x768p60hz
#define HDMIV_1366x768p60hz		HDMIV_13_1366x768p60hz
#define HDMIV_1400x1050p60hz	HDMIV_14_1400x1050p60hz
#define HDMIV_1440x900p60hz		HDMIV_15_1440x900p60hz
#define HDMIV_1440x2560p60hz	HDMIV_16_1440x2560p60hz
#define HDMIV_1600x900p60hz		HDMIV_17_1600x900p60hz
#define HDMIV_1600x1200p60hz	HDMIV_18_1600x1200p60hz
#define HDMIV_1680x1050p60hz	HDMIV_19_1680x1050p60hz
#define HDMIV_1920x1200p60hz	HDMIV_20_1920x1200p60hz
#define HDMIV_2048x1080p24hz	HDMIV_21_2048x1080p24hz
#define HDMIV_2160x1200p90hz	HDMIV_22_2160x1200p90hz
#define HDMIV_2560x1600p60hz	HDMIV_23_2560x1600p60hz
#define HDMIV_3440x1440p60hz	HDMIV_24_3440x1440p60hz
#define HDMIV_2400x1200p90hz	HDMIV_25_2400x1200p90hz
#define HDMIV_3840x1080p60hz	HDMIV_26_3840x1080p60hz

#define HDMI_0_UNKNOWN HDMI_UNKNOWN

enum hdmi_phy_para {
	HDMI_PHYPARA_6G = 1, /* 2160p60hz 444 8bit */
	HDMI_PHYPARA_4p5G, /* 2160p50hz 420 12bit */
	HDMI_PHYPARA_3p7G, /* 2160p30hz 444 10bit */
	HDMI_PHYPARA_3G, /* 2160p24hz 444 8bit */
	HDMI_PHYPARA_LT3G, /* 1080p60hz 444 12bit */
	HDMI_PHYPARA_DEF = HDMI_PHYPARA_LT3G,
	HDMI_PHYPARA_270M, /* 480p60hz 444 8bit */
};

#define HDMI_INFOFRAME_TYPE_EMP 0x7f
#define HDMI_INFOFRAME_EMP_VRR_GAME ((HDMI_INFOFRAME_TYPE_EMP << 8) | (EMP_TYPE_VRR_GAME))
#define HDMI_INFOFRAME_EMP_VRR_QMS ((HDMI_INFOFRAME_TYPE_EMP << 8) | (EMP_TYPE_VRR_QMS))
#define HDMI_INFOFRAME_EMP_VRR_SBTM ((HDMI_INFOFRAME_TYPE_EMP << 8) | (EMP_TYPE_SBTM))
#define HDMI_INFOFRAME_EMP_VRR_DSC ((HDMI_INFOFRAME_TYPE_EMP << 8) | (EMP_TYPE_DSC))
#define HDMI_INFOFRAME_EMP_VRR_DHDR ((HDMI_INFOFRAME_TYPE_EMP << 8) | (EMP_TYPE_DHDR))

enum vrr_type {
	T_VRR_NONE,
	T_VRR_GAME,
	T_VRR_QMS,
};

enum vrr_types_e {
	QMS_VRR_SUP = 1,
	GAMING_VRR_SUP = 2,
	UI_VRR_SUP = 4,
};

enum emp_type {
	EMP_TYPE_NONE,
	EMP_TYPE_VRR_GAME = T_VRR_GAME,
	EMP_TYPE_VRR_QMS = T_VRR_QMS,
	EMP_TYPE_SBTM,
	EMP_TYPE_DSC,
	EMP_TYPE_DHDR,
};

/* refer to HDMI2.1A P447 */
enum TARGET_FRAME_RATE {
	TFR_QMSVRR_INACTIVE = 0,
	TFR_23P97,
	TFR_24,
	TFR_25,
	TFR_29P97,
	TFR_30,
	TFR_47P95,
	TFR_48,
	TFR_50,
	TFR_59P94,
	TFR_60,
	TFR_100,
	TFR_119P88,
	TFR_120,
	TFR_MAX,
};

struct emp_packet_header {
	u8 header; /* hb0, fixed value 0x7f */
	u8 last:1; /* hb1 */
	u8 first:1;
	u8 seq_idx; /* hb2 */
};

/* Class 0 video timing extended metedata structure for game/fva, 2.1A P445 */
struct vtem_gamevrr_st {
	u8 vrr_en:1; /* MD0 */
	u8 fva_factor_m1:4;
	u8 base_vfront; /* MD1 */
	u16 brr_rate; /* MD2/3 */
};

/* Class 1 video timing extended metedata structure for qms, 2.1A P445 */
struct vtem_qmsvrr_st {
	u8 m_const:1; /* MD0 */
	u8 qms_en:1;
	u8 base_vfront; /* MD1 */
	u16 brr_rate; /* MD2/3 */
	enum TARGET_FRAME_RATE next_tfr:5;
};

struct emp_packet_0_body {
	u8 sync:1; /* pb0 synchronous metadata */
	u8 vfr:1; /* video format related, cs/cd/resolution */
	u8 afr:1; /* audio format related */
	/* 2b00: periodic pseudo-static MD
	 * 2b01: periodic dynamic MD
	 * 2b10: unique MD
	 */
	u8 ds_type:2;
	u8 end:1;
	u8 new:1;
	/* pb2  0: vendor specific MD 1: defined by 2.1
	 * 2: defined by CTA-861-G  3: defined by VESA
	 */
	u8 org_id;
	u16 ds_tag; /* pb3/4 */
	u16 ds_length; /* pb5/6 */
	union {
		struct vtem_gamevrr_st game_md;
		struct vtem_qmsvrr_st qms_md;
		/* struct vtem_sbtm_st sbtm_md; */
		u8 md[21]; /* pb7~pb27, md0~md20 */
	} md;
};

struct emp_packet_n_body {
	u8 md[28]; /* md(x)~md(x+27) */
};

/* extended metadata packet, 2.1A P304, no checksum in the PB0 */
struct emp_packet_st {
	enum emp_type type;
	struct emp_packet_header header;
	union {
		struct emp_packet_0_body emp0;
		struct emp_packet_n_body empn;
	} body;
};

enum emp_component_conf {
	CONF_HEADER_INIT,
	CONF_HEADER_LAST,
	CONF_HEADER_FIRST,
	CONF_HEADER_SEQ_INDEX,
	CONF_SYNC,
	CONF_VFR,
	CONF_AFR,
	CONF_DS_TYPE,
	CONF_END,
	CONF_NEW,
	CONF_ORG_ID,
	CONF_DATA_SET_TAG,
	CONF_DATA_SET_LENGTH,
	CONF_VRR_EN,
	CONF_FACTOR_M1,
	CONF_QMS_EN,
	CONF_M_CONST,
	CONF_BASE_VFRONT,
	CONF_NEXT_TFR,
	CONF_BASE_REFRESH_RATE,
	CONF_SBTM_VER,
	CONF_SBTM_MODE,
	CONF_SBTM_TYPE,
	CONF_SBTM_GRDM_MIN,
	CONF_SBTM_GRDM_LUM,
	CONF_SBTM_FRMPBLIMITINT,
};

enum frl_rate_enum {
	FRL_NONE = 0,
	FRL_3G3L = 1,
	FRL_6G3L = 2,
	FRL_6G4L = 3,
	FRL_8G4L = 4,
	FRL_10G4L = 5,
	FRL_12G4L = 6,
	FRL_RATE_MAX = 7,
};

/* CEA TIMING STRUCT DEFINITION */
struct hdmi_timing {
	unsigned int vic;
	char *name;
	char *sname;
	unsigned short pi_mode; /* 1: progressive  0: interlaced */
	unsigned int h_freq; /* in Hz */
	unsigned int v_freq; /* in 0.001 Hz */
	unsigned int pixel_freq; /* Unit: 1000 */
	unsigned short h_total;
	unsigned short h_blank;
	unsigned short h_front;
	unsigned short h_sync;
	unsigned short h_back;
	unsigned short h_active;
	unsigned short v_total;
	unsigned short v_blank;
	unsigned short v_front;
	unsigned short v_sync;
	unsigned short v_back;
	unsigned short v_active;
	unsigned short v_sync_ln;

	unsigned short h_pol;
	unsigned short v_pol;
	unsigned short h_pict;
	unsigned short v_pict;
	unsigned short h_pixel;
	unsigned short v_pixel;
};

/* Refer CEA861-D Page 116 Table 55 */
struct dtd {
	unsigned short pixel_clock;
	unsigned short h_active;
	unsigned short h_blank;
	unsigned short v_active;
	unsigned short v_blank;
	unsigned short h_sync_offset;
	unsigned short h_sync;
	unsigned short v_sync_offset;
	unsigned short v_sync;
	u8 h_image_size;
	u8 v_image_size;
	u8 h_border;
	u8 v_border;
	u8 flags;
	enum hdmi_vic vic;
};

struct vesa_standard_timing {
	unsigned short hactive;
	unsigned short vactive;
	unsigned short hblank;
	unsigned short vblank;
	unsigned short vsync;
	unsigned short tmds_clk; /* Value = Pixel clock ?? 10,000 */
	enum hdmi_vic vesa_timing;
};

/* Dolby Version support information from EDID*/
/* Refer to DV Spec version2.9 page26 to page39*/
enum block_type {
	ERROR_NULL = 0,
	ERROR_LENGTH,
	ERROR_OUI,
	ERROR_VER,
	CORRECT,
};

#define DV_IEEE_OUI             0x00D046
#define HDR10_PLUS_IEEE_OUI	0x90848B

#define HDMI_PACKET_VEND        1
#define HDMI_PACKET_DRM		0x86

enum hdmi_hdr_transfer {
	T_UNKNOWN = 0,
	T_BT709,
	T_UNDEF,
	T_BT601,
	T_BT470M,
	T_BT470BG,
	T_SMPTE170M,
	T_SMPTE240M,
	T_LINEAR,
	T_LOG100,
	T_LOG316,
	T_IEC61966_2_4,
	T_BT1361E,
	T_IEC61966_2_1,
	T_BT2020_10,
	T_BT2020_12,
	T_SMPTE_ST_2084,
	T_SMPTE_ST_28,
	T_HLG,
};

enum hdmi_hdr_color {
	C_UNKNOWN = 0,
	C_BT709,
	C_UNDEF,
	C_BT601,
	C_BT470M,
	C_BT470BG,
	C_SMPTE170M,
	C_SMPTE240M,
	C_FILM,
	C_BT2020,
};

enum hdmi_3d_type {
	T3D_FRAME_PACKING = 0,
	T3D_FIELD_ALTER = 1,
	T3D_LINE_ALTER = 2,
	T3D_SBS_FULL = 3,
	T3D_L_DEPTH = 4,
	T3D_L_DEPTH_GRAPHICS = 5,
	T3D_TAB = 6, /* Top and Buttom */
	T3D_RSVD = 7,
	T3D_SBS_HALF = 8,
	T3D_DISABLE,
};

/* master_display_info for display device */
struct master_display_info_s {
	u32 present_flag;
	u32 features;		/* feature bits bt2020/2084 */
	u32 primaries[3][2];	/* normalized 50000 in G,B,R order */
	u32 white_point[2];	/* normalized 50000 */
	u32 luminance[2];	/* max/min lumin, normalized 10000 */
	u32 max_content;	/* Maximum Content Light Level */
	u32 max_frame_average;	/* Maximum Frame-average Light Level */
};

struct hdr10plus_para {
	u8 application_version;
	u8 targeted_max_lum;
	u8 average_maxrgb;
	u8 distribution_values[9];
	u8 num_bezier_curve_anchors;
	u32 knee_point_x;
	u32 knee_point_y;
	u8 bezier_curve_anchors[9];
	u8 graphics_overlay_flag;
	u8 no_delay_flag;
};

struct dv_info {
	unsigned char rawdata[27];
	enum block_type block_flag;
	u32 ieeeoui;
	u8 ver; /* 0 or 1 or 2*/
	u8 length;/*ver1: 15 or 12*/
	/* if as 0, then support RGB tunnel mode */
	u8 sup_yuv422_12bit:1;
	/* if as 0, then support 2160p30hz */
	u8 sup_2160p60hz:1;
	/* if equals 0, then don't support 1080p100/120hz */
	u8 sup_1080p120hz:1;
	u8 sup_global_dimming:1;
	u16 Rx;
	u16 Ry;
	u16 Gx;
	u16 Gy;
	u16 Bx;
	u16 By;
	u16 Wx;
	u16 Wy;
	u16 tminPQ;
	u16 tmaxPQ;
	u8 dm_major_ver;
	u8 dm_minor_ver;
	u8 dm_version;
	u8 tmax_lum;
	u8 colorimetry:1;/* ver1*/
	u8 tmin_lum;
	u8 low_latency;/* ver1_12 and 2*/
	u8 sup_backlight_control:1;/*only ver2*/
	u8 backlt_min_luma;/*only ver2*/
	u8 Interface;/*only ver2*/
	u8 parity:1;/*only ver2*/
	u8 sup_10b_12b_444;/*only ver2*/
	u8 support_DV_RGB_444_8BIT;
	u8 support_LL_YCbCr_422_12BIT;
	u8 support_LL_RGB_444_10BIT;
	u8 support_LL_RGB_444_12BIT;
};

enum eotf_type {
	EOTF_T_NULL = 0,
	EOTF_T_DOLBYVISION,
	EOTF_T_HDR10,
	EOTF_T_SDR,
	EOTF_T_LL_MODE,
	EOTF_T_MAX,
};

enum mode_type {
	YUV422_BIT12 = 0,
	RGB_8BIT,
	RGB_10_12BIT,
	YUV444_10_12BIT,
};

/* Dolby Version VSIF  parameter*/
struct dv_vsif_para {
	u8 ver; /* 0 or 1 or 2*/
	u8 length;/*ver1: 15 or 12*/
	union {
		struct {
			u8 low_latency:1;
			u8 dobly_vision_signal:1;
			u8 backlt_ctrl_MD_present:1;
			u8 auxiliary_MD_present:1;
			u8 eff_tmax_PQ_hi;
			u8 eff_tmax_PQ_low;
			u8 auxiliary_runmode;
			u8 auxiliary_runversion;
			u8 auxiliary_debug0;
		} ver2;
	} vers;
};

#define MAX_VRR_MODE_GROUP 12
#define DRM_DISPLAY_MODE_LEN     32
#define MAX_QMS_GROUP_NUM 8

struct hdmitx_vrr_mode_group {
	__u32 brr_vic; /* brr vic for hdmitx */
	__u32 width;
	__u32 height;
	__u32 vrr_min; /* QMS_VRR range, Unit: 0.01, for example, 2397, 2400, 5994, ... etc */
	__u32 vrr_max;

	/* the list will be limited to qms_vrr_min ~ max */
	/* the TFR contains 13 items, but 2397 and 2400 will be merged to 2400's vic */
	__u16 qms_vic_lists[MAX_QMS_GROUP_NUM];

	__u32 game_vrr_min; /* GAME_VRR range */
	__u32 game_vrr_max;

	__u32 brr;
	char modename[DRM_DISPLAY_MODE_LEN];
	__u32 reserv[16];
};

enum color_attr_type {
	COLOR_ATTR_YCBCR444_12BIT = 0,
	COLOR_ATTR_YCBCR422_12BIT,
	COLOR_ATTR_YCBCR420_12BIT,
	COLOR_ATTR_RGB_12BIT,
	COLOR_ATTR_YCBCR444_10BIT,
	COLOR_ATTR_YCBCR422_10BIT,
	COLOR_ATTR_YCBCR420_10BIT,
	COLOR_ATTR_RGB_10BIT,
	COLOR_ATTR_YCBCR444_8BIT,
	COLOR_ATTR_YCBCR422_8BIT,
	COLOR_ATTR_YCBCR420_8BIT,
	COLOR_ATTR_RGB_8BIT,
	COLOR_ATTR_RESERVED,
};

struct color_attr_to_string {
	enum color_attr_type color_attr;
	const char *color_attr_string;
};

enum hdmi_color_depth {
	COLORDEPTH_24B = 4,
	COLORDEPTH_30B = 5,
	COLORDEPTH_36B = 6,
	COLORDEPTH_48B = 7,
	COLORDEPTH_RESERVED,
};

enum hdmi_color_range {
	COLORRANGE_LIM,
	COLORRANGE_FUL,
};

enum hdmi_audio_packet {
	HDMI_AUDIO_PACKET_SMP = 0x02,
	HDMI_AUDIO_PACKET_1BT = 0x07,
	HDMI_AUDIO_PACKET_DST = 0x08,
	HDMI_AUDIO_PACKET_HBR = 0x09,
};

enum avi_component_conf {
	CONF_AVI_CS,
	CONF_AVI_BT2020,
	CONF_AVI_Q01,
	CONF_AVI_YQ01,
	CONF_AVI_VIC,
	CONF_AVI_RGBYCC_INDIC,
};

/* CONF_AVI_BT2020 */
#define CLR_AVI_BT2020	0x0
#define SET_AVI_BT2020	0x1
/* CONF_AVI_Q01 */
#define RGB_RANGE_DEFAULT	0
#define RGB_RANGE_LIM		1
#define RGB_RANGE_FUL		2
#define RGB_RANGE_RSVD		3
/* CONF_AVI_YQ01 */
#define YCC_RANGE_LIM		0
#define YCC_RANGE_FUL		1
#define YCC_RANGE_RSVD		2

struct parse_cd {
	enum hdmi_color_depth cd;
	const char *name;
};

struct parse_cs {
	enum hdmi_colorspace cs;
	const char *name;
};

struct parse_cr {
	enum hdmi_color_range cr;
	const char *name;
};

/* DDC bus error codes */
enum ddc_err_t {
	DDC_ERR_NONE = 0x00,
	DDC_ERR_TIMEOUT = 0x01,
	DDC_ERR_NACK = 0x02,
	DDC_ERR_BUSY = 0x03,
	DDC_ERR_HW = 0x04,
	DDC_ERR_LIM_EXCEED = 0x05,
};

struct hdmi_format_para {
	enum hdmi_vic vic;
	unsigned char *name;
	unsigned char *sname;

	struct hdmi_timing timing;

	enum hdmi_color_depth cd; /* cd8, cd10 or cd12 */
	enum hdmi_colorspace cs; /* 0/1/2/3: rgb/422/444/420 */
	enum hdmi_quantization_range cr; /* limit, full */
	u32 frac_mode;

	/*hw related information, set in calc_format_para() func*/
	u32 scrambler_en:1;
	u32 tmds_clk_div40:1;
	u32 tmds_clk; /* Unit: 1000 */
	u8 dsc_en;
	enum frl_rate_enum frl_rate;
	/*hw related information end*/
};

#define AUDIO_PARA_MAX_NUM       14
struct hdmi_audio_fs_ncts {
	struct {
		u32 tmds_clk;
		u32 n; /* 24 or 30 bit */
		u32 cts; /* 24 or 30 bit */
		u32 n_36bit;
		u32 cts_36bit;
		u32 n_48bit;
		u32 cts_48bit;
	} array[AUDIO_PARA_MAX_NUM];
	u32 def_n;
};

struct hdmi_support_mode {
	enum hdmi_vic vic;
	char *sname;
	char y420;
};

#define DOLBY_VISION_LL_RGB             3
#define DOLBY_VISION_LL_YUV             2
#define DOLBY_VISION_STD_ENABLE         1
#define DOLBY_VISION_DISABLE            0
#define DOLBY_VISION_ENABLE	1
/* used to indicate that no ubootenv of user_prefer_dv_type,
 * which means that user has not selected dv type on menu
 */
#define AMDV_NONE -1

#define HDMI_IEEEOUI 0x000C03
#define MODE_LEN	32

/* below default ENV is not used, just for backup */
#define DEFAULT_OUTPUTMODE_ENV		"1080p60hz"
#define DEFAULT_HDMIMODE_ENV		"1080p60hz"
#define DEFAULT_COLORATTRIBUTE_ENV	"444,8bit"

#define DEFAULT_COLOR_FORMAT_4K         "420,8bit"
#define DEFAULT_COLOR_FORMAT            "rgb,8bit"
#define DEFAULT_HDMI_MODE               "720p60hz"

typedef enum {
	DOLBY_VISION_PRIORITY = 0,
	HDR10_PRIORITY        = 1,
	SDR_PRIORITY          = 2,
} hdr_priority_e;

typedef enum {
	HDR_POLICY_SINK   = 0,
	HDR_POLICY_SOURCE = 1,
	HDR_POLICY_FORCE = 4,
} hdr_policy_e;

#define DV_SINK_LED    0
#define DV_SOURCE_LED  1
#define FORCE_AMDV       2
#define FORCE_HDR10    3
#define FORCE_HLG      5

enum {
	RESOLUTION_PRIORITY = 0,
	FRAMERATE_PRIORITY  = 1,
};

struct dispmode_vic {
	const char *disp_mode;
	enum hdmi_vic VIC;
};

#define HDCPTX_IOOPR             0x820000ab
enum hdcptx_oprcmd {
	HDCP_DEFAULT,
	HDCP14_KEY_READY,
	HDCP14_LOADKEY,
	HDCP14_RESULT,
	HDCP22_KEY_READY,
	HDCP22_LOADKEY,
	HDCP22_RESULT,
	HDCP22_SET_TOPO,
	HDCP22_GET_TOPO,
	CONF_ENC_IDX, /* 0: get idx; 1: set idx */
	HDMITX_GET_RTERM, /* get the rterm value */
};

enum pkt_op {
	AVI_PKT,
	GAMUT_PKT,
	AUDIO_PKT,
	SPD_PKT,
	MPEG_PKT,
	VSIF_PKT,
	GEN_PKT,
	GEN2_PKT,
	GEN3_PKT,
	GEN4_PKT,
	GEN5_PKT,
	VTEM_PKT,
};

/* half for valid vic, half for vic with y420*/
#define VIC_MAX_NUM 512
#define SVD_VIC_MAX_NUM 128
#define VESA_MAX_TIMING 64
#define Y420_VIC_MAX_NUM 32 /* vic numbers for y420 */

#define HDMITX_VESA_OFFSET	0x300

/***********************************************************************
 *                   hdmi debug printk
 **********************************************************************/
#define VID         "video: "
#define AUD         "audio: "
#define CEC         "cec: "
#define EDID        "edid: "
#define HDCP        "hdcp: "
#define SYS         "system: "
#define HPD         "hpd: "
#define HW          "hw: "
#define REG         "reg: "

#endif
