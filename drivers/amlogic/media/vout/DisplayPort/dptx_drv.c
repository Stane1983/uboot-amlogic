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
#include "DPTX_IP/dptx_IP_ops.h"

#define DPTX_HPD_TIMEOUT        200
#define DPTX_DELAY_AFTER_HPD    200

static void dptx_power_init(struct dptx_drv_s *dptx, int flag)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	unsigned char pd_id;

	switch (dptx->data->chip_type) {
	case DPTX_CHIP_T7:
		//#define PM_EDP0 48
		//#define PM_EDP1 49
		pd_id = dptx->idx ? PM_EDP1 : PM_EDP0;
		pwr_ctrl_psci_smc(pd_id, flag);
		break;
	default:
		return;
	}
	DPTXPR(dptx->idx, LOG_I, "power domain[%u] %s", pd_id, flag ? "on" : "off");
#endif
}

void __dptx_update_ctrl_bootargs(struct dptx_drv_s *dptx)
{
	uint32_t val = 0;
	char env_str[12], ctrl_str[13];

	/*
	 *bit[7:0]: link rate
	 *bit[9:8]: lane count: 0=disabled, 1=1lane, 2=2lane, 3=4lane
	 *bit[17:10]: vmode_sel_idx, 0xff for non-edid or invalid
	 *bit[21:18]: vmode_cfmt_sel, 0xf for invalid
	 *bit[22]: dptx-ip enabled
	 *bit[23]: dptx disp enabled
	 *bit[31:24]: 48it edid crc
	 *******
	 *bit[63:62]: DPTX debug_level
	 */
	val |= dptx->link_cfg.link_rate & 0xff;
	val |= ((dptx->link_cfg.lane_count == 4 ? 3 : dptx->link_cfg.lane_count) & 0x3) << 8;
	val |= (dptx->vmode_mgr.vmode_sel_idx & 0xff)  << 10;
	val |= (dptx->vmode_mgr.vmode_cfmt_sel & 0xf) << 18;
	val |= (dptx->status & DPTX_STA_DRV_READY) ? 1 << 22 : 0;
	val |= (dptx->status & DPTX_STA_DISP_ON) ? 1 << 23 : 0;
	val |= (dptx->edid_info.edid_crc & 0xff) << 24;

	sprintf(ctrl_str, "0x%08x", val);

	sprintf(env_str, "dptx%d_ctrl", dptx->idx);
	env_set(env_str, ctrl_str);
}

void dptx_driver_ready(struct dptx_drv_s *dptx)
{
	if (!(dptx->status & (DPTX_STA_PROBE_DONE))) {
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	DPTXPR(dptx->idx, LOG_I, "driver init(ver %s)", DPTX_DRV_VERSION);
	dptx_phy_enable(dptx);
	dptx_power_init(dptx, 1);

	dptx_venc_enable(dptx, 0);

	dptx_transmitter_init(dptx);

	dptx_gpio_set(dptx->PWR_gpio_name, 1); // gpio DP PWR
	dptx_HPD_pinmux_set(dptx);

	dptx->status |= DPTX_STA_DRV_READY;
}

void dptx_drv_check_HPD(struct dptx_drv_s *dptx)
{
	uint16_t i = 0;
	uint8_t data;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}

	if (dptx->hpd_ignore) {
		DPTXPR(dptx->idx, LOG_I, "ignore HPD");
		mdelay(dptx->hpd_ignore);
		dptx->status |= DPTX_STA_HPD_HIGH;
		return;
	}

	while (i < DPTX_HPD_TIMEOUT) {
		data = dptx_get_hpd_level(dptx);
		if (data) {
			dptx->status |= DPTX_STA_HPD_HIGH;
			DPTXPR(dptx->idx, LOG_I, "HPD after %ums", i);
			return;
		}
		mdelay(1);
		i++;
	}

	DPTXPR(dptx->idx, LOG_I, "DPTX HPD is LOW");
	dptx->status &= ~DPTX_STA_HPD_HIGH;
}

void dptx_drv_start(struct dptx_drv_s *dptx)
{
	struct dptx_vmode_s *dp_vmode = NULL;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY | DPTX_STA_HPD_HIGH))) {
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		goto dptx_setup_link_fail;
	}

	mdelay(DPTX_DELAY_AFTER_HPD);

	dptx_transmitter_init(dptx);
	__dptx_set_phy_config(dptx, 1); // ?

	// Power up link
	if (__dptx_aux_write_single(dptx, DPCD_SET_POWER, 0x1))
		DPTXPR(dptx->idx, LOG_E, "DPCD SET POWER ERROR");
	mdelay(20);

	// DP protocol: EDID -> DPCD, Intel: DPCD -> EDID
	if (dptx_DPCD_capability_to_link_cfg(dptx)) {
		DPTXPR(dptx->idx, LOG_E, "DPCD capability detect ERROR");
		goto dptx_setup_link_fail;
	}

	dptx->link_cfg.link_rate  = dptx->link_cfg.max_link_rate;
	dptx->link_cfg.lane_count = dptx->link_cfg.max_lane_count;

	__dptx_set_lane_config(dptx);
	__dptx_set_phy_config(dptx, 1); // ?

	__dptx_link_training(dptx);

	if (1) {
		if (!__dptx_EDID_probe(dptx)) {
			dptx_vmode_manage(dptx);
			dp_vmode = dptx_get_optimum_vmode(dptx);
		}
		if (!dp_vmode) {
			dp_vmode = &DPTX_SafeMode_640x480_vmode;
			DPTXPR(dptx->idx, LOG_E, "no vmode to satisfy BW, to safemode 480P");
		}
		dptx_vmode_apply_to_act_timing(dptx, dp_vmode);
		dptx_act_timing_apply(dptx);
	}
	__dptx_update_ctrl_bootargs(dptx);

	dptx_set_content_protection(dptx);

	dptx_set_MSA(dptx);

	dptx_venc_enable(dptx, 1);

	dptx_main_stream_enable(dptx);

	dptx->status |= DPTX_STA_LINK_ON;
	DPTXPR(dptx->idx, LOG_I, "%s enable main stream video finished", __func__);
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
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}
	if (dptx->idx == 0)
		run_command("gpio set GPIOY_1", 0);
	else
		run_command("gpio set GPIOY_8", 0);
	dptx->status |= DPTX_STA_DISP_ON;
}

void dptx_drv_disp_off(struct dptx_drv_s *dptx)
{
	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}
	dptx->status &= ~DPTX_STA_DISP_ON;
}

void dptx_driver_close(struct dptx_drv_s *dptx)
{
	unsigned char auxdata;
	int ret;

	if (!(dptx->status & (DPTX_STA_PROBE_DONE | DPTX_STA_DRV_READY))) {
		DPTXPR(dptx->idx, LOG_E, "DPTX sta=0x%x, not ready for %s", dptx->status, __func__);
		return;
	}
	// dptx_clear_timing(dptx);
	// Power down link
	auxdata = 0x2;
	ret = __dptx_aux_write(dptx, DPCD_SET_POWER, 1, &auxdata);
	if (ret)
		DPTXPR(dptx->idx, LOG_V, "sink power down link failed");

	DPTXPR(dptx->idx, LOG_I, "disable main stream video");
	dptx_transmitter_shutdown(dptx);

	memset(&dptx->link_cfg, 0, sizeof(struct dptx_link_cfg_s));
	dptx->status &= ~DPTX_STA_LINK_ON;
	DPTXPR(dptx->idx, LOG_I, "%s finished\n", __func__);
}
