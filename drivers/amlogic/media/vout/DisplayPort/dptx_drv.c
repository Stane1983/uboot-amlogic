// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <linux/kernel.h>
#ifdef CONFIG_SECURE_POWER_CONTROL
#include <asm/arch/pwr_ctrl.h>
#endif
#include <amlogic/media/vout/DisplayPort/DPTX.h>
#include <amlogic/media/vout/DisplayPort/DPCD_REG.h>
#include "dptx_regs.h"
#include "dptx_common.h"

#define DPTX_HPD_TIMEOUT        200
#define DPTX_DELAY_AFTER_HPD    200

static void dptx_power_init(struct dptx_drv_s *dptx, uint8_t port, int flag)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	switch (dptx->data->chip_type) {
	case DPTX_CHIP_T7:
		//#define PM_EDP0 48
		//#define PM_EDP1 49
		if (dptx->idx == 0) {
			if (port == 0)
				pwr_ctrl_psci_smc(PM_EDP0, flag);
			else if (port == 1)
				pwr_ctrl_psci_smc(PM_EDP1, flag);
		} else {
			pwr_ctrl_psci_smc(PM_EDP1, flag);
		}
		break;
	default:
		return;
	}
	DPTX_P_DBG(dptx, port, "power domain %s", flag ? "on" : "off");
#endif
}

/* ! FOLLOW KERNEL
 *0:bit[ 7: 0], [ 9: 8]: A = [link rate], [lane count: 0=disabled, 1=1lane, 2=2lane, 3=4lane]
 *0:bit[17:10]: vmode_sel_idx, 0xff for non-edid or invalid
 *0:bit[21:18]: vmode_cfmt_sel, 0xf for invalid
 *0:bit[22]: dptx-ip enabled
 *0:bit[23]: dptx disp enabled
 *0:bit[31:24]: dptx edid_crc
 *1:bit[ 7: 0], [19:18]: B = [link rate], [lane count: 0=disabled, 1=1lane, 2=2lane, 3=4lane]
 *1:bit[17:10], [19:18]: C = [link rate], [lane count: 0=disabled, 1=1lane, 2=2lane, 3=4lane]
 *1:bit[27:20], [29:28]: D = [link rate], [lane count: 0=disabled, 1=1lane, 2=2lane, 3=4lane]
 *1:bit[31:30]: user_port_set: 0=user_default, 1=1port, 2=2port, 3=4port
 *
 * dptx_dbg
 *0:bit[ 2: 0]: print level
 *0:bit[31:28]: user_hpd_ignore: p3, p2, p1, p0
 *0:bit[37:24]: bist test      : p3, p2, p1, p0
 */
void __dptx_update_ctrl_bootargs(struct dptx_drv_s *dptx)
{
	uint32_t val0 = 0, val1 = 0, val2 = 0;
	char env_str[12], ctrl_str[24];
	struct dptx_link_cfg_s *link;

	if (dptx->sink.link[0]) {
		link  = dptx->sink.link[0];
		val0 |= (link->link_rate & 0xff) << 0;
		val0 |= ((link->lane_count == 4 ? 3 : link->lane_count) & 0x3) <<  8;
	}
	val0 |= (dptx->vmode_mgr.vmode_sel_idx & 0xff)  << 10;
	val0 |= (dptx->vmode_mgr.vmode_cfmt_sel & 0xf)  << 18;
	val0 |= (dptx->status & DPTX_STA_DRV_READY) ? 1 << 22 : 0;
	val0 |= (dptx->status & DPTX_STA_DISP_ON) ? 1   << 23 : 0;
	val0 |= (dptx->sink.exp_edid.edid_crc & 0xff)   << 24;

	if (dptx->sink.link[1]) {
		link = dptx->sink.link[1];
		val1 |= (link->link_rate & 0xff) << 0;
		val1 |= ((link->lane_count == 4 ? 3 : link->lane_count) & 0x3) <<  8;
	}
	if (dptx->sink.link[2]) {
		link = dptx->sink.link[1];
		val1 |= (link->link_rate & 0xff) << 10;
		val1 |= ((link->lane_count == 4 ? 3 : link->lane_count) & 0x3) << 18;
	}
	if (dptx->sink.link[3]) {
		link = dptx->sink.link[1];
		val1 |= (link->link_rate & 0xff) << 20;
		val1 |= ((link->lane_count == 4 ? 3 : link->lane_count) & 0x3) << 28;
	}
	val1 |= __builtin_popcount(dptx->setting.user_port_mask & 0xf) << 30;

	sprintf(ctrl_str, "0x%x,0x%x", val0, val1);
	sprintf(env_str, "dptx%d_ctrl", dptx->idx);
	env_set(env_str, ctrl_str);

	val2 = env_get_ulong("dptx_dbg", 16, 0);
	val2 &= ~0x3;
	val2 |= dptx_print_level;
	val2 &= ~BIT(dptx->idx + 28);
	val2 |= dptx->setting.user_hpd_ignore ? BIT(dptx->idx + 28) : 0;
	sprintf(ctrl_str, "0x%x", val2);
	env_set("dptx_dbg", ctrl_str);
}

void dptx_driver_ready(struct dptx_drv_s *dptx)
{
	uint8_t port;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE))) {
		DPTX_ERR(dptx, "sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	DPTX_PR(dptx, "driver init(ver %s)", DPTX_DRV_VERSION);
	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_phy_enable(dptx, port);
		dptx_power_init(dptx, port, 1);
	}

	dptx_venc_enable(dptx, 0);

	dptx_gpio_set(dptx->PWR_gpio_name, 1); // gpio DP PWR
	// run_command("gpio set GPIOY_14", 0);

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_if_transmitter_init(dptx, port);
	}
	dptx_HPD_pinmux_set(dptx);

	dptx->status |= DPTX_STA_DRV_READY;
}

void dptx_drv_check_HPD(struct dptx_drv_s *dptx)
{
	uint16_t i = 0;
	uint8_t data;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	if (dptx->setting.user_hpd_ignore) {
		DPTX_PR(dptx, "ignore HPD");
		mdelay(dptx->setting.user_hpd_ignore);
		dptx->status |= DPTX_STA_HPD_HIGH;
		return;
	}

	while (i < DPTX_HPD_TIMEOUT) {
		data = dptx_if_get_hpd_level(dptx, 0);
		if (data) {
			dptx->status |= DPTX_STA_HPD_HIGH;
			DPTX_PR(dptx, "HPD after %ums", i);
			return;
		}
		mdelay(1);
		i++;
	}

	DPTX_PR(dptx, "DPTX HPD is LOW");
	dptx->status &= ~DPTX_STA_HPD_HIGH;
}

void dptx_drv_start(struct dptx_drv_s *dptx)
{
	struct dptx_vmode_s *dp_vmode = NULL;
	uint8_t port;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY | DPTX_STA_HPD_HIGH))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		goto dptx_setup_link_fail;
	}

	mdelay(DPTX_DELAY_AFTER_HPD);

	dptx_venc_enable(dptx, 0);
	// dptx_if_path_reset(dptx, 0, DPTX_RESET_VENC);
	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_if_transmitter_init(dptx, port);
		__dptx_set_phy_config(dptx, port, 1); // ?

		// Power up link
		if (dptx_if_aux_write_single(dptx, port, DPCD_SET_POWER, 0x1))
			DPTX_P_ERR(dptx, port, "DPCD SET POWER ERROR");
		mdelay(20);

		// dptx_link_cfg_dft(dptx, port);
		// DP protocol: EDID -> DPCD, Intel: DPCD -> EDID
		if (dptx_DPCD_capability_to_link_cfg(dptx, port))
			DPTX_P_ERR(dptx, port, "DPCD capability detect ERROR");

		dptx_link_policy_maker(dptx, port);
	}

	// multi port config check;

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		__dptx_set_lane_config(dptx, port);
		__dptx_set_phy_config(dptx, port, 1); // ?
	}

	__dptx_link_training(dptx);

	if (1) { // use EDID case
		if (!__dptx_EDID_probe(dptx)) {
			dptx_vmode_manage(dptx);
			if (dptx->setting.user_vmode_sel)
				dp_vmode = dptx_get_vmode(dptx, dptx->setting.user_vmode_sel);
			if (!dp_vmode)
				dp_vmode = dptx_get_optimum_vmode(dptx);
		}
		if (!dp_vmode) {
			dp_vmode = &DPTX_SafeMode_640x480_vmode;
			DPTX_ERR(dptx, "no vmode to satisfy BW, to safemode 480P");
		}
		dptx_vmode_apply_to_act_timing(dptx, dp_vmode);
		dptx_act_timing_apply(dptx);
	} else { // use preset panel_config
		//TODO
	}
	__dptx_update_ctrl_bootargs(dptx);

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_if_set_MSA(dptx, port);
		dptx_set_content_protection(dptx, port);
	}

	// dptx_mute_set(dptx, 1);

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_if_transmitter_output(dptx, port, 1);
	}
	dptx_venc_enable(dptx, 1);
	mdelay(1);

	dptx->status |= DPTX_STA_LINK_ON;
	DPTX_PR(dptx, "%s enable main stream[0x%x] video finished", __func__, dptx->sink.port_mask);
	return;

dptx_setup_link_fail:
	dp_vmode = &DPTX_SafeMode_640x480_vmode;
	dptx_vmode_apply_to_act_timing(dptx, dp_vmode);
	dptx_act_timing_apply(dptx);
	__dptx_update_ctrl_bootargs(dptx);
}

void dptx_drv_disp_on(struct dptx_drv_s *dptx)
{
	if (!(dptx->status &
	      (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY | DPTX_STA_HPD_HIGH | DPTX_STA_LINK_ON))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	// dptx_mute_set(dptx, 0);
	// mdelay(10);

	if (dptx->idx == 0)
		run_command("gpio set GPIOY_1", 0);
	else
		run_command("gpio set GPIOY_8", 0);
	dptx->status |= DPTX_STA_DISP_ON;
}

void dptx_drv_disp_off(struct dptx_drv_s *dptx)
{
	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}
	if (dptx->idx == 0)
		run_command("gpio clr GPIOY_1", 0);
	else
		run_command("gpio clr GPIOY_8", 0);

	mdelay(5);
	dptx_mute_set(dptx, 1);
	mdelay(20);

	dptx->status &= ~DPTX_STA_DISP_ON;
}

void dptx_driver_close(struct dptx_drv_s *dptx)
{
	unsigned char auxdata;
	int ret;
	uint8_t port;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	if (!(dptx->status & DPTX_STA_LINK_ON)) {
		DPTX_PR(dptx, "%s finished", __func__);
		return;
	}

	// dptx_clear_timing(dptx);
	// Power down link
	auxdata = 0x5;
	// 010b = Set all link to D3 (power-down mode).
	// 101b = Set Main-Link D3, keep AUX block fully powered
	for (port = 3; port > 0; port--) {
		if (!dptx->sink.link[port])
			continue;
		ret = dptx_if_aux_write(dptx, port, DPCD_SET_POWER, 1, &auxdata);
		if (ret)
			DPTX_DBG(dptx, "sink power down link failed");
	}
	DPTX_PR(dptx, "disable main stream video");
	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_if_transmitter_output(dptx, port, 0);
		dptx_if_path_reset(dptx, port, DPTX_RESET_ALL);
	}
	dptx->status &= ~DPTX_STA_LINK_ON;
	DPTX_PR(dptx, "%s finished\n", __func__);
}

void dptx_drv_eDP_PSR1_en(struct dptx_drv_s *dptx, uint8_t port_mask, uint8_t en)
{
	uint8_t port;

	if (!(dptx->status &
	      (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY | DPTX_STA_HPD_HIGH | DPTX_STA_LINK_ON))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_eDP_PSR1(dptx, port, en);
	}
}

void dptx_drv_eDP_PSR2_en(struct dptx_drv_s *dptx, uint8_t port_mask, uint8_t en)
{
	uint8_t port;

	if (!(dptx->status &
	      (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY | DPTX_STA_HPD_HIGH | DPTX_STA_LINK_ON))) {
		DPTX_ERR(dptx, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	for (port = 0; port < 4; port++) {
		if (!dptx->sink.link[port])
			continue;
		dptx_eDP_PSR2(dptx, port, en);
	}
}
