// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

static struct lcd_phy_ctrl_s *lcd_phy_ctrl;

unsigned int lcd_phy_check_lane_phase_sel(struct aml_lcd_drv_s *pdrv)
{
	if (!lcd_phy_ctrl || pdrv->config.basic.lcd_type != LCD_MLVDS)
		return 0;

	return lcd_phy_ctrl->phy_lane_phase_sel_def ? 1 : 0;
}

int lcd_phy_param_preset(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy = pdrv->config.phy_cfg.act_phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	unsigned int amp = 0, preem = 0;
	int i;

#ifdef CONFIG_AML_LCD_PXP
	return 0;
#endif
	if (!lcd_phy_ctrl)
		return -1;

	phy_cfg->lane_num = lcd_phy_ctrl->lane_num;
	if (lcd_phy_ctrl->phy_glb_param_dft_val)
		lcd_phy_ctrl->phy_glb_param_dft_val(pdrv);

	if (lcd_phy_ctrl->phy_vswing_level_to_val)
		phy->vswing = lcd_phy_ctrl->phy_vswing_level_to_val(pdrv, phy_cfg->vswing_level);

	if (lcd_phy_ctrl->phy_preem_level_to_val)
		preem = lcd_phy_ctrl->phy_preem_level_to_val(pdrv, phy_cfg->preem_level);

	if (lcd_phy_ctrl->phy_amp_dft_val)
		amp = lcd_phy_ctrl->phy_amp_dft_val(pdrv);

	for (i = 0; i < phy_cfg->lane_num; i++) {
		phy->lane[i].amp = amp;
		phy->lane[i].preem = preem;
		phy_cfg->ch_ctrl[i].sel = i;
		if (lcd_phy_ctrl->phy_lane_phase_sel_def) {
			phy_cfg->ch_ctrl[i].phase_sel =
				lcd_phy_ctrl->phy_lane_phase_sel_def(pdrv, i);
		} else {
			phy_cfg->ch_ctrl[i].phase_sel = 0xff;
		}
	}
	for (; i < CH_LANE_MAX; i++)
		phy_cfg->ch_ctrl[i].sel = 0xff;
	if (pdrv->config.basic.lcd_type == LCD_MLVDS) {
		phy->clk_phase = pdrv->config.control.mlvds_cfg.clk_phase & 0xfff;
		phy_cfg->bypass_resample = (pdrv->config.control.mlvds_cfg.clk_phase >> 12) & 1;
	}

	return 0;
}

int lcd_phy_param_get(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy_cfg,
		      struct phy_attr_s *phy)
{
	int ret;

#ifdef CONFIG_AML_LCD_PXP
	return 0;
#endif
	if (!pdrv || !phy_cfg || !phy)
		return -1;
	if (!lcd_phy_ctrl || !lcd_phy_ctrl->phy_param_get)
		return -1;

	memcpy(phy_cfg, &pdrv->config.phy_cfg, sizeof(struct phy_config_s));
	lcd_lane_sel_get(pdrv, phy_cfg);
	ret = lcd_phy_ctrl->phy_param_get(pdrv, phy_cfg, phy);
	return ret;
}

void lcd_phy_param_print(struct aml_lcd_drv_s *pdrv)
{
	struct phy_config_s local_phy_cfg, *phy_cfg;
	struct phy_attr_s local_phy, *phy;
	char str_sel[12], str_phase[12];
	int i, n, ret;

#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!pdrv)
		return;
	ret = lcd_phy_param_get(pdrv, &local_phy_cfg, &local_phy);
	if (ret)
		return;

	phy_cfg = &pdrv->config.phy_cfg;
	phy = pdrv->config.phy_cfg.act_phy;

	printf("vswing  = 0x%x(0x%x)\n"
		"odt     = 0x%x(0x%x)\n"
		"vcm     = 0x%x(0x%x)\n"
		"cv_mode = %d(%d)\n"
		"ref_bias= %d(%d)\n",
		phy->vswing, local_phy.vswing,
		phy->odt, local_phy.odt,
		phy->vcm, local_phy.vcm,
		phy->cv_mode, local_phy.cv_mode,
		phy->ref_bias, local_phy.ref_bias);
	printf("  lane  sel       phase_sel   amp       preem\n");
	for (i = 0; i < phy_cfg->lane_num; i++) {
		if (phy_cfg->ch_ctrl[i].sel == 0xff)
			n = sprintf(str_sel, " - ");
		else
			n = sprintf(str_sel, "0x%x", phy_cfg->ch_ctrl[i].sel);
		if (local_phy_cfg.ch_ctrl[i].sel == 0xff)
			sprintf(str_sel + n, "( - )");
		else
			sprintf(str_sel + n, "(0x%x)", local_phy_cfg.ch_ctrl[i].sel);
		if (phy_cfg->ch_ctrl[i].phase_sel == 0xff)
			n = sprintf(str_phase, " - ");
		else
			n = sprintf(str_phase, "0x%x", phy_cfg->ch_ctrl[i].phase_sel);
		if (local_phy_cfg.ch_ctrl[i].phase_sel == 0xff)
			sprintf(str_phase + n, "( - )");
		else
			sprintf(str_phase + n, "(0x%x)", local_phy_cfg.ch_ctrl[i].phase_sel);
		printf("  [%2d]:  %s, %s, 0x%x(0x%x), 0x%x(0x%x)\n",
		       i, str_sel, str_phase,
		       phy->lane[i].amp, local_phy.lane[i].amp,
		       phy->lane[i].preem, local_phy.lane[i].preem);
	}
	printf("flag=0x%x, lane_num=%d, lane_valid=0x%x, lane_offset=%d, lane_mask=0x%x\n",
		phy_cfg->flag, phy_cfg->lane_num, phy_cfg->lane_valid,
		phy_cfg->lane_offset, phy_cfg->lane_mask);
	printf("ch_swap0=0x%x, ch_swap1=0x%x, clk_phase=0x%x, ckdi=0x%x\n",
		phy_cfg->ch_swap0, phy_cfg->ch_swap1, phy->clk_phase, phy_cfg->ckdi);
}

void lcd_phy_analog_reg_print(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!pdrv)
		return;
	if (!lcd_phy_ctrl || !lcd_phy_ctrl->phy_reg_dump)
		return;

	printf("\nphy analog regs:\n");
	lcd_phy_ctrl->phy_reg_dump(pdrv);
}

void lcd_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	int i;

#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	if (!lcd_phy_ctrl || !pdrv->phy_set) {
		LCDPR("[%d]: %s: phy_set is null\n", pdrv->index, __func__);
		return;
	}

	for (i = 0; i < pdrv->data->drv_max; i++) {
		if (pdrv->index == i)
			continue;
		if (phy->lane_valid & lcd_phy_ctrl->lane_lock[i]) {
			LCDERR("[%d]: %s: lane_valid 0x%x conflict with lane_lock[%d] 0x%x\n",
			       pdrv->index, __func__, phy->lane_valid,
			       i, lcd_phy_ctrl->lane_lock[i]);
			return;
		}
	}

	lcd_phy_ctrl->lane_lock[pdrv->index] = phy->lane_valid;
	if (status)
		lcd_phy_ctrl->lane_lock_total |= phy->lane_valid;
	else
		lcd_phy_ctrl->lane_lock_total &= ~phy->lane_valid;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %d: lane_valid=0x%x, lane_lock_total=0x%x\n",
		      pdrv->index, __func__, status, phy->lane_valid,
		      lcd_phy_ctrl->lane_lock_total);
	}
	pdrv->phy_set(pdrv, status);
}

int lcd_phy_probe(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	pdrv->phy_set = NULL;
	return 0;
#endif
	if (!pdrv->config.phy_cfg.act_phy || !lcd_phy_ctrl) {
		pdrv->phy_set = NULL;
		return 0;
	}

	lcd_phy_ctrl->lane_lock[pdrv->index] = 0;
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_lvds;
		break;
	case LCD_VBYONE:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_vx1;
		break;
	case LCD_MLVDS:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_mlvds;
		break;
	case LCD_P2P:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_p2p;
		break;
	case LCD_MIPI:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_mipi;
		break;
	case LCD_EDP:
		pdrv->phy_set = lcd_phy_ctrl->phy_set_edp;
		break;
	default:
		pdrv->phy_set = NULL;
		break;
	}

	return 0;
}

int lcd_phy_config_init(struct aml_lcd_data_s *pdata)
{
	lcd_phy_ctrl = NULL;
#ifdef CONFIG_AML_LCD_PXP
	return 0;
#endif

	switch (pdata->chip_type) {
#ifdef CONFIG_MESON_T5M
	case LCD_CHIP_T5M:
		lcd_phy_ctrl = lcd_phy_config_init_t5m(pdata);
		break;
#endif
#ifdef CONFIG_MESON_T3X
	case LCD_CHIP_T3X:
		lcd_phy_ctrl = lcd_phy_config_init_t3x(pdata);
		break;
#endif
#ifdef CONFIG_MESON_TXHD2
	case LCD_CHIP_TXHD2:
		lcd_phy_ctrl = lcd_phy_config_init_txhd2(pdata);
		break;
#endif
#ifdef CONFIG_MESON_S6
	case LCD_CHIP_S6:
		lcd_phy_ctrl = lcd_phy_config_init_s6(pdata);
		break;
#endif
#ifdef CONFIG_MESON_T6D
	case LCD_CHIP_T6D:
		lcd_phy_ctrl = lcd_phy_config_init_t6d(pdata);
		break;
#endif
	default:
		break;
	}
	if (lcd_phy_ctrl)
		lcd_phy_ctrl->lane_lock_total = 0;

	return 0;
}
