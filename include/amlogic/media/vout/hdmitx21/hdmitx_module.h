/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_MODULE_H__
#define __HDMITX_MODULE_H__
#ifndef __HDMITX_MODULE21_H__
#define __HDMITX_MODULE21_H__

#include "hdmi_common.h"
#include "hdmitx_ext.h"
#include <amlogic/media/vout/dsc.h>
#include <amlogic/media/vout/hdmi_tx_repeater.h>
#include <amlogic/media/vout/hdmitx_common/hdmitx_edid.h>
//#include <amlogic/media/vout/hdmitx_common/hdmitx_common.h>

#define HZ 100000000 // TODO

struct tx_cap {
	/* configure in dts file */
	u8 tx_max_frl_rate;
	/* default 600Mhz, if res_1080p, then 225Mhz */
	u32 tx_max_tmds_clk;
	bool dsc_capable;
	u8 dsc_policy;
};

struct hdmitx_hw_common {
	/* soc/hdmitx driver capability */
	struct tx_cap hdmi_tx_cap;
};

struct hdmitx_common {
	struct hdmitx_hw_common *tx_hw;
	/*soc limitation config*/
	u32 res_1080p;
	/* efuse ctrl state
	 * 1 disable the function
	 * 0 dont disable the function
	 */
	bool efuse_dis_hdmi_4k60;	/* 4k50,60hz */
	bool efuse_dis_output_4k;	/* all 4k resolution*/
	bool efuse_dis_hdcp_tx22;	/* hdcptx22 */
	bool efuse_dis_hdmi_tx3d;	/* 3d */
	bool efuse_dis_hdcp_tx14;	/* s1a hdcptx14 */
	u32 max_refreshrate;

	/*edid related*/
	/* edid hdr/dv cap lock, hdr/dv handle in irq, need spinlock*/
	//spinlock_t edid_spinlock;
	//u32 forced_edid; /* for external loading EDID */
	//unsigned char EDID_buf[EDID_MAX_BLOCK * 128];
	struct rx_cap rxcap;
};

struct hdmitx21_hw {
	struct hdmitx_hw_common base;
};

struct hdmitx_dev {
	struct tx_cap txcap;
	struct hdmitx_common tx_common;
	struct hdmitx21_hw tx_hw;
	struct {
		int (*get_hpd_state)(void);
		int (*read_edid)(unsigned char *buf);
		void (*turn_off)(void);
		void (*list_support_modes)(void);
		void (*dump_regs)(void);
		void (*test_bist)(unsigned int mode);
		void (*test_prbs)(void);
		void (*set_div40)(bool div40);
		void (*output_blank)(unsigned int blank);
	} hwop;
	struct {
		u32 enable;
		union hdmi_infoframe vend;
		union hdmi_infoframe avi;
		union hdmi_infoframe spd;
		union hdmi_infoframe aud;
		union hdmi_infoframe drm;
	} infoframes;
	u32 colormetry;
	unsigned char rawedid[EDID_BLK_SIZE * EDID_MAX_BLOCK];
	struct rx_cap RXCap;
	struct hdmi_format_para *para;
	enum hdmi_vic vic; /* qms: tfr_vic  normal: vic */
	/* for s7,s7d,s6 default 1
	 * 1: new clk config, encp/pixel clk is directly configured by the pll simulation part.
	 * through [ 49]hdmi_vx1_pix_clk to encp/pixel clk
	 * CLKCTRL_VID_CLK0_CTRL clk source should select vid_pix_clk.
	 */
	u8 clk_analog_path;
	enum frl_rate_enum manual_frl_rate; /* for manual setting */
	u8 tx_max_frl_rate; /* configure in dts file */
	bool flt_train_st; /* 0 means FLT train failed */
	bool frl_train_fail_flag;
	u32 dfm_type;
	/* pps data and clk info from dsc module */
	struct dsc_offer_tx_data dsc_data;
	unsigned int frac_rate_policy;
	unsigned int mode420;
	unsigned int dc30;
	enum eotf_type hdmi_current_eotf_type;
	enum mode_type hdmi_current_tunnel_mode;
	/* Add dongle_mode, clock, phy may be different from mbox */
	unsigned int dongle_mode;
	unsigned char enc_idx;
	int dv_en;
	int qms_en; /* qms function enable */
	enum hdmi_vic brr_vic; /* qms BRR vic */
	unsigned char pxp_mode; /* for running at pxp only */
	enum amhdmitx_chip_e chip_type;
	bool hpd_state;
};

struct hdmitx_dev *get_hdmitx21_device(void);
void hdmitx21_mux_ddc(void);
u32 hdmitx_calc_tmds_clk(u32 pixel_freq, enum hdmi_colorspace cs, enum hdmi_color_depth cd);
u32 hdmitx_get_frl_bandwidth(const enum frl_rate_enum rate);
u32 hdmitx_calc_frl_bandwidth(u32 pixel_freq, enum hdmi_colorspace cs, enum hdmi_color_depth cd);
enum frl_rate_enum hdmitx_select_frl_rate(u8 *dsc_en, u8 dsc_policy, enum hdmi_vic vic,
					  enum hdmi_colorspace cs, enum hdmi_color_depth cd);
#ifdef CONFIG_AMLOGIC_DSC
enum frl_rate_enum get_dsc_frl_rate(enum dsc_encode_mode dsc_mode);
#endif

bool hdmitx_frl_training_main(enum frl_rate_enum frl_rate);
int hdmitx21_read_edid(u8 *_rx_edid);
void scdc21_rd_sink(u8 adr, u8 *val);
void scdc21_wr_sink(u8 adr, u8 val);
struct hdmi_format_para *hdmitx21_get_fmt_paras(enum hdmi_vic vic);
const struct hdmi_timing *hdmitx21_get_timing_para0(void);
int hdmitx21_timing_size(void);
void hdmitx21_set_clk(struct hdmitx_dev *hdev);
u32 hdmitx_check_frac_rate(struct hdmitx_dev *hdev);
const struct hdmi_timing *hdmitx_mode_vic_to_hdmi_timing(enum hdmi_vic vic);
const struct hdmi_timing *hdmitx21_gettiming_from_vic(enum hdmi_vic vic);
const struct hdmi_timing *hdmitx_mode_match_vesa_timing(struct vesa_standard_timing *t);
const struct hdmi_timing *hdmitx_mode_match_dtd_timing(struct dtd *t);

struct hdmi_format_para *hdmitx21_get_fmtpara(const char *mode,
	const char *attr);
struct hdmi_format_para *hdmitx21_get_fmt_name(char const *name, char const *attr);
struct hdmi_format_para *hdmitx21_tst_fmt_name(char const *name, char const *attr);
struct hdmi_format_para *hdmitx21_match_dtd_paras(struct dtd *t);
bool pre_process_str(char *name);

void hdmitx21_set(struct hdmitx_dev *hdev);
void hdmitx_module_disable(void);
void hdmitx21_dump_regs(void);
void hdmitx21_infoframe_send(u16 info_type, u8 *body);
int hdmitx21_infoframe_rawget(u8 info_type, u8 *body);

/* there are 2 ways to send out infoframes
 * xxx_infoframe_set() will take use of struct xxx_infoframe_set
 * xxx_infoframe_rawset() will directly send with rawdata
 * if info, hb, or pb == NULL, disable send infoframe
 */
void hdmi_vend_infoframe_set(struct hdmi_vendor_infoframe *info);
void hdmi_vend_infoframe_rawset(u8 *hb, u8 *pb);
void hdmi_vend_infoframe2_rawset(u8 *hb, u8 *pb);
void hdmi_avi_infoframe_set(struct hdmi_avi_infoframe *info);
void hdmi_avi_infoframe_rawset(u8 *hb, u8 *pb);
void hdmi_spd_infoframe_set(struct hdmi_spd_infoframe *info);
void hdmi_audio_infoframe_set(struct hdmi_audio_infoframe *info);
void hdmi_audio_infoframe_rawset(u8 *hb, u8 *pb);
void hdmi_drm_infoframe_set(struct hdmi_drm_infoframe *info);
void hdmi_drm_infoframe_rawset(u8 *hb, u8 *pb);
void hdmi_avi_infoframe_config(enum avi_component_conf conf, u8 val);
void hdmi_sbtm_infoframe_rawset(u8 *hb, u8 *pb);

/* Parsing RAW EDID data from edid to prxcap */
unsigned int hdmi_edid_parsing(unsigned char *edid, struct rx_cap *prxcap);
void dsc_cap_show(struct rx_cap *prxcap);
bool is_dolby_enabled(void);
bool is_amdolby_enabled(void);
bool is_tv_support_dv(struct hdmitx_dev *hdev);
bool is_dv_preference(struct hdmitx_dev *hdev);
bool is_hdr_preference(struct hdmitx_dev *hdev);

bool hdmitx_edid_only_support_sd(struct rx_cap *prxcap);
bool hdmitx21_validate_mode(struct hdmitx_dev *hdev, struct hdmi_format_para *para);

struct hdmi_format_para *hdmi_tst_fmt_name(char const *name, char const *attr);
bool is_support_4k(void);
bool hdmitx_chk_mode_attr_sup(struct hdmitx_dev *hdev, const char *mode, char *attr);
int get_ubootenv_dv_type(void);
int get_ubootenv_dv_status(void);
int get_hdr_policy(void);
void hdmitx_phy_pre_init(struct hdmitx_dev *hdev);
void hdmitx_set_phypara(enum hdmi_phy_para mode);
int hdmitx_get_hpd_state(void);
void hdmitx_turnoff(void);
void hdmitx_test_prbs(void);
struct hdr_info *hdmitx_get_rx_hdr_info(void);
const char *hdmitx_edid_vic_to_string(enum hdmi_vic vic);
enum hdmi_vic hdmitx_edid_vic_tab_map_vic(const char *disp_mode);
void hdmitx_set_drm_pkt(struct master_display_info_s *data);
void hdmitx_set_vsif_pkt(enum eotf_type type, enum mode_type tunnel_mode,
	struct dv_vsif_para *data);
bool is_hdmi_mode(char *mode);
void hdmitx21_pbist_config(struct hdmitx_dev *hdev, enum hdmi_vic vic, int reg_pbist_en);
void pkt_send_position_change(u32 enable_all, enum pkt_op pkt, u8 mov_val);
void hdmitx21_write_dhdr_sram(void);
void hdmitx21_read_dhdr_sram(void);
void hdmitx21_send_sbtm_pkt(void);
void vrr_init_qms_para(struct hdmitx_dev *hdev);
enum hdmi_vic hdmitx_find_brr_vic(enum hdmi_vic vic);
void hdmitx_qms_map_vic(struct hdmitx_dev *hdev);

/* the hdmitx output limits to 1080p */
bool is_hdmitx_limited_1080p(void);
const struct hdmi_timing *hdmitx21_match_dtd_timing(struct dtd *t);
void hdmitx_dsc_cvtem_pkt_send(struct dsc_pps_data_s *pps,
			       struct hdmi_timing *timing);
void hdmitx_dsc_cvtem_pkt_disable(void);
enum hdmi_vic hdmitx_get_prefer_vic(struct hdmitx_dev *hdev, enum hdmi_vic vic);

int hdmitx_format_para_reset(struct hdmi_format_para *para);
int hdmitx_common_build_format_para(struct hdmitx_common *tx_comm, struct hdmi_format_para *para,
				    enum hdmi_vic vic, u32 frac_rate_policy,
				    enum hdmi_colorspace cs, enum hdmi_color_depth cd,
				    enum hdmi_quantization_range cr);
int hdmitx_hw_validate_mode(struct hdmitx_hw_common *tx_hw, u32 vic, u32 max_refreshrate);

/* DDC */
bool hdmitx_ddcm_write(u8 seg_index, u8 slave_addr, u8 reg_addr, u8 *data, u16 len);
void hdmitx21_send_ake_init(void);

#ifdef CONFIG_AML_DSC_ENC
bool edid_check_dsc_support(struct tx_cap *hdmi_tx_cap,
		struct rx_cap *rxcap, struct hdmi_format_para *para, u8 dsc_policy);
#endif
#undef printk
#define printk printf
#undef pr_info
#define pr_info printf

#define module_param_array(...);

// TODO
#define hdmitx_debug() printf("hdmitx21: %s[%d]\n", __func__, __LINE__)
#endif
#endif
