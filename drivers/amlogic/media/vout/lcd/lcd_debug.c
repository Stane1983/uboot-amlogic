// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "lcd_debug.h"

int lcd_debug_info_len(int num)
{
	int ret = 0;

	if (num >= (PR_BUF_MAX - 1)) {
		printf("%s: string length %d is out of support\n",
			__func__, num);
		return 0;
	}

	ret = PR_BUF_MAX - 1 - num;
	return ret;
}

static void lcd_timing_info_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	int ret, herr, verr;

	ret = lcd_config_timing_check(pdrv, &pconf->timing.act_timing);
	herr = ret & 0xf;
	verr = (ret >> 4) & 0xf;

	printf("h_period          %d\n"
		"v_period          %d\n"
		"hs_width          %d\n"
		"hs_backporch      %d%s\n"
		"hs_frontporch     %d%s\n"
		"hs_pol            %d\n"
		"vs_width          %d\n"
		"vs_backporch      %d%s\n"
		"vs_frontporch     %d%s\n"
		"vs_pol            %d\n"
		"pre_de_h          %d\n"
		"pre_de_v          %d\n"
		"video_hstart      %d\n"
		"video_vstart      %d\n\n",
		pconf->timing.act_timing.h_period,
		pconf->timing.act_timing.v_period,
		pconf->timing.act_timing.hsync_width,
		pconf->timing.act_timing.hsync_bp,
		((herr & 0x4) ? "(X)" : ((herr & 0x8) ? "(!)" : "")),
		pconf->timing.act_timing.hsync_fp,
		((herr & 0x1) ? "(X)" : ((herr & 0x2) ? "(!)" : "")),
		pconf->timing.act_timing.hsync_pol,
		pconf->timing.act_timing.vsync_width,
		pconf->timing.act_timing.vsync_bp,
		((verr & 0x4) ? "(X)" : ((verr & 0x8) ? "(!)" : "")),
		pconf->timing.act_timing.vsync_fp,
		((verr & 0x1) ? "(X)" : ((verr & 0x2) ? "(!)" : "")),
		pconf->timing.act_timing.vsync_pol,
		pconf->timing.pre_de_h,
		pconf->timing.pre_de_v,
		pconf->timing.hstart, pconf->timing.vstart);

	printf("Timing range:\n"
		"  h_period  : %4d ~ %4d\n"
		"  v_period  : %4d ~ %4d\n"
		"  frame_rate: %4d ~ %4d\n"
		"  pixel_clk : %4d ~ %4d\n"
		"  vrr_range : %4d ~ %4d\n\n",
		pconf->timing.act_timing.h_period_min,
		pconf->timing.act_timing.h_period_max,
		pconf->timing.act_timing.v_period_min,
		pconf->timing.act_timing.v_period_max,
		pconf->timing.act_timing.frame_rate_min,
		pconf->timing.act_timing.frame_rate_max,
		pconf->timing.act_timing.pclk_min,
		pconf->timing.act_timing.pclk_max,
		pconf->timing.act_timing.vfreq_vrr_min,
		pconf->timing.act_timing.vfreq_vrr_max);

	printf("base_pixel_clk  %d\n"
		"base_h_period   %d\n"
		"base_v_period   %d\n"
		"base_frame_rate %d\n\n",
		pconf->timing.base_timing.pixel_clk,
		pconf->timing.base_timing.h_period,
		pconf->timing.base_timing.v_period,
		pconf->timing.base_timing.frame_rate);

	printf("pll_ctrl       0x%08x\n"
		"div_ctrl       0x%08x\n"
		"clk_ctrl       0x%08x\n",
		pconf->timing.pll_ctrl, pconf->timing.div_ctrl,
		pconf->timing.clk_ctrl);

	if (pconf->timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
		printf("pll_ctrl2      0x%08x\n"
			"div_ctrl2      0x%08x\n"
			"clk_ctrl2      0x%08x\n",
			pconf->timing.pll_ctrl2, pconf->timing.div_ctrl2,
			pconf->timing.clk_ctrl2);
	}
	printf("\n");
}

static void lcd_gpio_info_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_power_ctrl_s *lcd_power;
	int i = 0;

	lcd_power = &pdrv->config.power;
	printf("\ncpu_gpio:\n");
	while (i < LCD_CPU_GPIO_NUM_MAX) {
		if (strcmp(lcd_power->cpu_gpio[i], "invalid") == 0)
			break;
		printf("%d: gpio name=%s\n", i, lcd_power->cpu_gpio[i]);
		i++;
	}
}

static void lcd_power_info_print(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_power_step_s *power_step;
	int i;

	if (status) {
		/* check if factory test */
		if (pdrv->factory_lcd_power_on_step) {
			printf("factory test power on step:\n");
			power_step = pdrv->factory_lcd_power_on_step;
		} else {
			printf("power on step:\n");
			power_step = &pdrv->config.power.power_on_step[0];
		}
	} else {
		printf("power off step:\n");
		power_step = &pdrv->config.power.power_off_step[0];
	}

	i = 0;
	while (i < LCD_PWR_STEP_MAX) {
		if (power_step->type >= LCD_POWER_TYPE_MAX)
			break;
		switch (power_step->type) {
		case LCD_POWER_TYPE_CPU:
		case LCD_POWER_TYPE_PMU:
		case LCD_POWER_TYPE_WAIT_GPIO:
		case LCD_POWER_TYPE_CLK_SS:
			printf("  %d: type=%d, index=%d, value=%d, delay=%d\n",
				i, power_step->type, power_step->index,
				power_step->value, power_step->delay);
			break;
		case LCD_POWER_TYPE_EXTERN:
			printf("  %d: type=%d, index=%d, delay=%d\n",
				i, power_step->type, power_step->index,
				power_step->delay);
			break;
		case LCD_POWER_TYPE_SIGNAL:
			printf("  %d: type=%d, delay=%d\n",
				i, power_step->type, power_step->delay);
			break;
		default:
			break;
		}
		i++;
		power_step++;
	}
}

static void lcd_pinmux_info_print(struct lcd_config_s *pconf)
{
	int i;

	printf("pinmux:\n");

	i = 0;
	while (i < LCD_PINMUX_NUM) {
		if (pconf->pinmux_set[i][0] == LCD_PINMUX_END)
			break;
		printf("pinmux_set: %d, 0x%08x\n",
			pconf->pinmux_set[i][0], pconf->pinmux_set[i][1]);
		i++;
	}
	i = 0;
	while (i < LCD_PINMUX_NUM) {
		if (pconf->pinmux_clr[i][0] == LCD_PINMUX_END)
			break;
		printf("pinmux_clr: %d, 0x%08x\n",
			pconf->pinmux_clr[i][0], pconf->pinmux_clr[i][1]);
		i++;
	}

	printf("\n");
}

static void lcd_info_print_lvds(struct aml_lcd_drv_s *pdrv)
{
	printf("lvds_repack       %u\n"
		"dual_port         %u\n"
		"pn_swap           %u\n"
		"port_swap         %u\n"
		"lane_reverse      %u\n"
		"phy_vswing        0x%x\n"
		"phy_preem         0x%x\n\n",
		pdrv->config.control.lvds_cfg.lvds_repack,
		pdrv->config.control.lvds_cfg.dual_port,
		pdrv->config.control.lvds_cfg.pn_swap,
		pdrv->config.control.lvds_cfg.port_swap,
		pdrv->config.control.lvds_cfg.lane_reverse,
		pdrv->config.control.lvds_cfg.phy_vswing,
		pdrv->config.control.lvds_cfg.phy_preem);
}

static void lcd_info_print_vbyone(struct aml_lcd_drv_s *pdrv)
{
	printf("lane_count                 %u\n"
		"region_num                 %u\n"
		"byte_mode                  %u\n"
		"bit_rate                   %lluHz\n"
		"phy_vswing                 0x%x\n"
		"phy_preemphasis            0x%x\n"
		"hw_filter_time             0x%x\n"
		"hw_filter_cnt              0x%x\n"
		"ctrl_flag                  0x%x\n\n",
		pdrv->config.control.vbyone_cfg.lane_count,
		pdrv->config.control.vbyone_cfg.region_num,
		pdrv->config.control.vbyone_cfg.byte_mode,
		pdrv->config.timing.bit_rate,
		pdrv->config.control.vbyone_cfg.phy_vswing,
		pdrv->config.control.vbyone_cfg.phy_preem,
		pdrv->config.control.vbyone_cfg.hw_filter_time,
		pdrv->config.control.vbyone_cfg.hw_filter_cnt,
		pdrv->config.control.vbyone_cfg.ctrl_flag);
	if (pdrv->config.control.vbyone_cfg.ctrl_flag & 0x1) {
		printf("power_on_reset_en          %u\n"
			"power_on_reset_delay       %ums\n\n",
			(pdrv->config.control.vbyone_cfg.ctrl_flag & 0x1),
			pdrv->config.control.vbyone_cfg.power_on_reset_delay);
	}
	if (pdrv->config.control.vbyone_cfg.ctrl_flag & 0x2) {
		printf("hpd_data_delay_en          %u\n"
			"hpd_data_delay             %ums\n\n",
			((pdrv->config.control.vbyone_cfg.ctrl_flag >> 1) & 0x1),
			pdrv->config.control.vbyone_cfg.hpd_data_delay);
	}
	if (pdrv->config.control.vbyone_cfg.ctrl_flag & 0x4) {
		printf("cdr_training_hold_en       %u\n"
			"cdr_training_hold          %ums\n\n",
			((pdrv->config.control.vbyone_cfg.ctrl_flag >> 2) & 0x1),
			pdrv->config.control.vbyone_cfg.cdr_training_hold);
	}
	lcd_pinmux_info_print(&pdrv->config);
}

static void lcd_info_print_rgb(struct aml_lcd_drv_s *pdrv)
{
	printf("type              %u\n"
		"clk_pol           %u\n"
		"DE_valid          %u\n"
		"sync_valid        %u\n"
		"rb_swap           %u\n"
		"bit_swap          %u\n\n",
		pdrv->config.control.rgb_cfg.type,
		pdrv->config.control.rgb_cfg.clk_pol,
		pdrv->config.control.rgb_cfg.de_valid,
		pdrv->config.control.rgb_cfg.sync_valid,
		pdrv->config.control.rgb_cfg.rb_swap,
		pdrv->config.control.rgb_cfg.bit_swap);
	lcd_pinmux_info_print(&pdrv->config);
}

static void lcd_info_print_bt(struct aml_lcd_drv_s *pdrv)
{
	printf("clk_phase       %u\n"
		"field_type      %u\n"
		"mode_422        %u\n"
		"yc_swap         %u\n"
		"cbcr_swap       %u\n\n",
		pdrv->config.control.bt_cfg.clk_phase,
		pdrv->config.control.bt_cfg.field_type,
		pdrv->config.control.bt_cfg.mode_422,
		pdrv->config.control.bt_cfg.yc_swap,
		pdrv->config.control.bt_cfg.cbcr_swap);
	lcd_pinmux_info_print(&pdrv->config);
}

static void lcd_info_print_mipi(struct aml_lcd_drv_s *pdrv)
{
#ifdef CONFIG_AML_LCD_TABLET
	lcd_dsi_info_print(&pdrv->config);
#endif
}

static void lcd_info_print_edp(struct aml_lcd_drv_s *pdrv)
{
	printf("max_lane_count        %u\n"
		"max_link_rate         %u\n"
		"training_mode         %u\n"
		"edid_en               %u\n"
		"sync_clk_mode         %u\n"
		"lane_count            %u\n"
		"link_rate             %u\n"
		"bit_rate              %llu\n"
		"phy_vswing            0x%x\n"
		"phy_preem             0x%x\n\n",
		pdrv->config.control.edp_cfg.max_lane_count,
		pdrv->config.control.edp_cfg.max_link_rate,
		pdrv->config.control.edp_cfg.training_mode,
		pdrv->config.control.edp_cfg.edid_en,
		pdrv->config.control.edp_cfg.sync_clk_mode,
		pdrv->config.control.edp_cfg.lane_count,
		pdrv->config.control.edp_cfg.link_rate,
		pdrv->config.timing.bit_rate,
		pdrv->config.control.edp_cfg.phy_vswing_preset,
		pdrv->config.control.edp_cfg.phy_preem_preset);
	lcd_pinmux_info_print(&pdrv->config);
}

#ifdef CONFIG_AML_LCD_TCON
static void lcd_info_print_mlvds(struct aml_lcd_drv_s *pdrv)
{
	printf("channel_num       %d\n"
		"channel_sel0      0x%08x\n"
		"channel_sel1      0x%08x\n"
		"clk_phase         0x%04x\n"
		"pn_swap           %u\n"
		"bit_swap          %u\n"
		"phy_vswing        0x%x\n"
		"phy_preem         0x%x\n"
		"bit_rate          %lluHz\n"
		"pi_clk_sel        0x%03x\n\n",
		pdrv->config.control.mlvds_cfg.channel_num,
		pdrv->config.control.mlvds_cfg.channel_sel0,
		pdrv->config.control.mlvds_cfg.channel_sel1,
		pdrv->config.control.mlvds_cfg.clk_phase,
		pdrv->config.control.mlvds_cfg.pn_swap,
		pdrv->config.control.mlvds_cfg.bit_swap,
		pdrv->config.control.mlvds_cfg.phy_vswing,
		pdrv->config.control.mlvds_cfg.phy_preem,
		pdrv->config.timing.bit_rate,
		pdrv->config.control.mlvds_cfg.pi_clk_sel);
	lcd_tcon_info_print(pdrv);
	lcd_pinmux_info_print(&pdrv->config);
}

static void lcd_info_print_p2p(struct aml_lcd_drv_s *pdrv)
{
	printf("p2p_type          0x%x\n"
		"lane_num          %d\n"
		"channel_sel0      0x%08x\n"
		"channel_sel1      0x%08x\n"
		"pn_swap           %u\n"
		"bit_swap          %u\n"
		"bit_rate          %lluHz\n"
		"phy_vswing        0x%x\n"
		"phy_preem         0x%x\n\n",
		pdrv->config.control.p2p_cfg.p2p_type,
		pdrv->config.control.p2p_cfg.lane_num,
		pdrv->config.control.p2p_cfg.channel_sel0,
		pdrv->config.control.p2p_cfg.channel_sel1,
		pdrv->config.control.p2p_cfg.pn_swap,
		pdrv->config.control.p2p_cfg.bit_swap,
		pdrv->config.timing.bit_rate,
		pdrv->config.control.p2p_cfg.phy_vswing,
		pdrv->config.control.p2p_cfg.phy_preem);
	lcd_tcon_info_print(pdrv);
	lcd_pinmux_info_print(&pdrv->config);
}
#endif

static void lcd_phy_print(struct lcd_config_s *pconf)
{
	struct phy_config_s *phy = &pconf->phy_cfg;
	int i;

	switch (pconf->basic.lcd_type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
	case LCD_P2P:
	case LCD_EDP:
		printf("ctrl_flag:         0x%x\n"
		"vswing_level:      %u\n"
		"ext_pullup:        %u\n"
		"preem_level:       %u\n"
		"vcm:               0x%x\n"
		"ref_bias:          0x%x\n"
		"odt:               0x%x\n",
		phy->flag,
		phy->vswing_level,
		phy->ext_pullup,
		phy->preem_level,
		phy->vcm,
		phy->ref_bias,
		phy->odt);
		for (i = 0; i < phy->lane_num; i++) {
			printf("lane%d_amp:        0x%x\n"
			"lane%d_preem:      0x%x\n",
			i, phy->lane[i].amp,
			i, phy->lane[i].preem);
		}
		printf("\n");
		break;
	default:
		break;
	}
}

static void lcd_reg_print_rgb(struct aml_lcd_drv_s *pdrv)
{
	printf("\nrgb regs: todo\n");
}

static void lcd_reg_print_bt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nbt656/1120 regs:\n");
	reg = VPU_VOUT_BT_CTRL;
	printf("VPU_VOUT_BT_CTRL     [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_PLD_LINE;
	printf("VPU_VOUT_BT_PLD_LINE  [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_PLDIDT0;
	printf("VPU_VOUT_BT_PLDIDT0  [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_PLDIDT1;
	printf("VPU_VOUT_BT_PLDIDT1  [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_BLK_DATA;
	printf("VPU_VOUT_BT_BLK_DATA  [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_DAT_CLPY;
	printf("VPU_VOUT_BT_DAT_CLPY  [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
	reg = VPU_VOUT_BT_DAT_CLPC;
	printf("VPU_VOUT_BT_DAT_CLPC [0x%04x] = 0x%08x\n",
		reg, lcd_vcbus_read(reg));
}

static void lcd_reg_print_lvds(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg, offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];

	printf("\nlvds registers:\n");
	reg = LVDS_PACK_CNTL_ADDR + offset;
	printf("LVDS_PACK_CNTL      [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = LVDS_GEN_CNTL + offset;
	printf("LVDS_GEN_CNTL       [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));

	reg = P2P_CH_SWAP0 + offset;
	printf("P2P_CH_SWAP0        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = P2P_CH_SWAP1 + offset;
	printf("P2P_CH_SWAP1        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
}

static void lcd_reg_print_vbyone(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg, offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];

	printf("\nvbyone registers:\n");
	reg = VBO_STATUS_L + offset;
	printf("VX1_STATUS          [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_FSM_HOLDER_L + offset;
	printf("VX1_FSM_HOLDER_L    [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_FSM_HOLDER_H + offset;
	printf("VX1_FSM_HOLDER_H    [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_STATE_CTRL + offset;
	printf("VX1_INTR_STATE_CTRL [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_UNMASK + offset;
	printf("VX1_INTR_UNMASK     [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_INTR_STATE + offset;
	printf("VX1_INTR_STATE      [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_INSGN_CTRL + offset;
	printf("VBO_INSGN_CTRL      [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = LCD_PORT_SWAP + offset;
	printf("LCD_PORT_SWAP        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = P2P_CH_SWAP0 + offset;
	printf("P2P_CH_SWAP0        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = P2P_CH_SWAP1 + offset;
	printf("P2P_CH_SWAP1        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
}

#ifdef CONFIG_AML_LCD_TCON
static void lcd_reg_print_tcon_tl1(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\ntcon registers:\n");
	reg = HHI_TCON_CLK_CNTL;
	printf("HHI_TCON_CLK_CNTL   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = TCON_TOP_CTRL;
	printf("TCON_TOP_CTRL       [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_RGB_IN_MUX;
	printf("TCON_RGB_IN_MUX     [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL0;
	printf("TCON_OUT_CH_SEL0    [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL1;
	printf("TCON_OUT_CH_SEL1    [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_STATUS0;
	printf("TCON_STATUS0        [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_PLLLOCK_CNTL;
	printf("TCON_PLLLOCK_CNTL   [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_RST_CTRL;
	printf("TCON_RST_CTRL       [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST0;
	printf("TCON_AXI_OFST0      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST1;
	printf("TCON_AXI_OFST1      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST2;
	printf("TCON_AXI_OFST2      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_CLK_CTRL;
	printf("TCON_CLK_CTRL       [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_STATUS1;
	printf("TCON_STATUS1        [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_DDRIF_CTRL1;
	printf("TCON_DDRIF_CTRL1    [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_DDRIF_CTRL2;
	printf("TCON_DDRIF_CTRL2    [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));

	reg = P2P_CH_SWAP0;
	printf("P2P_CH_SWAP0        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = P2P_CH_SWAP1;
	printf("P2P_CH_SWAP1        [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
}

static void lcd_reg_print_tcon_t3(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\ntcon registers:\n");
	reg = CLKCTRL_TCON_CLK_CNTL;
	printf("CLKCTRL_TCON_CLK_CNTL [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = TCON_TOP_CTRL;
	printf("TCON_TOP_CTRL         [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_RGB_IN_MUX;
	printf("TCON_RGB_IN_MUX       [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL0;
	printf("TCON_OUT_CH_SEL0      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_OUT_CH_SEL1;
	printf("TCON_OUT_CH_SEL1      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_STATUS0;
	printf("TCON_STATUS0          [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_PLLLOCK_CNTL;
	printf("TCON_PLLLOCK_CNTL     [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_RST_CTRL;
	printf("TCON_RST_CTRL         [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST0;
	printf("TCON_AXI_OFST0        [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST1;
	printf("TCON_AXI_OFST1        [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_AXI_OFST2;
	printf("TCON_AXI_OFST2        [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_CLK_CTRL;
	printf("TCON_CLK_CTRL         [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_STATUS1;
	printf("TCON_STATUS1          [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_DDRIF_CTRL1;
	printf("TCON_DDRIF_CTRL1      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));
	reg = TCON_DDRIF_CTRL2;
	printf("TCON_DDRIF_CTRL2      [0x%04x] = 0x%08x\n",
	       reg, lcd_tcon_read(reg));

	reg = P2P_CH_SWAP0;
	printf("P2P_CH_SWAP0          [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = P2P_CH_SWAP1;
	printf("P2P_CH_SWAP1          [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
}
#endif

static void lcd_reg_print_mipi(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nmipi_dsi registers:\n");
	reg = MIPI_DSI_TOP_CNTL;
	printf("MIPI_DSI_TOP_CNTL            [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_TOP_CLK_CNTL;
	printf("MIPI_DSI_TOP_CLK_CNTL        [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_PWR_UP_OS;
	printf("MIPI_DSI_DWC_PWR_UP_OS       [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_PCKHDL_CFG_OS;
	printf("MIPI_DSI_DWC_PCKHDL_CFG_OS   [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_LPCLK_CTRL_OS;
	printf("MIPI_DSI_DWC_LPCLK_CTRL_OS   [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_CMD_MODE_CFG_OS;
	printf("MIPI_DSI_DWC_CMD_MODE_CFG_OS [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_VID_MODE_CFG_OS;
	printf("MIPI_DSI_DWC_VID_MODE_CFG_OS [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_MODE_CFG_OS;
	printf("MIPI_DSI_DWC_MODE_CFG_OS     [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_PHY_STATUS_OS;
	printf("MIPI_DSI_DWC_PHY_STATUS_OS   [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_INT_ST0_OS;
	printf("MIPI_DSI_DWC_INT_ST0_OS      [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_DWC_INT_ST1_OS;
	printf("MIPI_DSI_DWC_INT_ST1_OS      [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_TOP_STAT;
	printf("MIPI_DSI_TOP_STAT            [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_TOP_INTR_CNTL_STAT;
	printf("MIPI_DSI_TOP_INTR_CNTL_STAT  [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
	reg = MIPI_DSI_TOP_MEM_PD;
	printf("MIPI_DSI_TOP_MEM_PD          [0x%04x] = 0x%08x\n", reg, dsi_host_read(pdrv, reg));
}

static void lcd_reg_print_edp(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;
	int index = pdrv->index;

	if (index > 1) {
		LCDERR("%s: invalid drv_index %d\n", __func__, index);
		return;
	}

	printf("\nedp registers:\n");
	reg = EDP_TX_LINK_BW_SET;
	printf("EDP_TX_LINK_BW_SET               [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_LINK_COUNT_SET;
	printf("EDP_TX_LINK_COUNT_SET            [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_TRAINING_PATTERN_SET;
	printf("EDP_TX_TRAINING_PATTERN_SET      [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_SCRAMBLING_DISABLE;
	printf("EDP_TX_SCRAMBLING_DISABLE        [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_SCRAMBLING_DISABLE;
	printf("EDP_TX_SCRAMBLING_DISABLE        [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_TRANSMITTER_OUTPUT_ENABLE;
	printf("EDP_TX_TRANSMITTER_OUTPUT_ENABLE [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_MAIN_STREAM_ENABLE;
	printf("EDP_TX_MAIN_STREAM_ENABLE        [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_PHY_RESET;
	printf("EDP_TX_PHY_RESET                 [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_PHY_STATUS;
	printf("EDP_TX_PHY_STATUS                [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_COMMAND;
	printf("EDP_TX_AUX_COMMAND               [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_ADDRESS;
	printf("EDP_TX_AUX_ADDRESS               [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_STATE;
	printf("EDP_TX_AUX_STATE                 [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_REPLY_CODE;
	printf("EDP_TX_AUX_REPLY_CODE            [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_REPLY_COUNT;
	printf("EDP_TX_AUX_REPLY_COUNT           [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_REPLY_DATA_COUNT;
	printf("EDP_TX_AUX_REPLY_DATA_COUNT      [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));
	reg = EDP_TX_AUX_TRANSFER_STATUS;
	printf("EDP_TX_AUX_TRANSFER_STATUS       [0x%04x] = 0x%08x\n",
	       reg, dptx_reg_read(index, reg));

	dptx_DPCD_dump(pdrv);
}

static void lcd_reg_print_serializer(void)
{
	unsigned int reg;

	reg = HHI_LVDS_TX_PHY_CNTL0;
	printf("HHI_LVDS_TX_PHY_CNTL0     [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_LVDS_TX_PHY_CNTL1;
	printf("HHI_LVDS_TX_PHY_CNTL1     [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

static void lcd_reg_print_combo_dphy_serializer(void)
{
	unsigned int reg;

	reg = COMBO_DPHY_CNTL0;
	printf("COMBO_DPHY_CNTL0          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
	printf("COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0     [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
	printf("COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1     [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));

}

static void lcd_reg_print_phy_analog(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nphy analog registers:\n");
	lcd_reg_print_serializer();

	reg = HHI_DIF_CSI_PHY_CNTL1;
	printf("PHY_CNTL1           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL2;
	printf("PHY_CNTL2           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL3;
	printf("PHY_CNTL3           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

static void lcd_reg_print_phy_analog_txhd2(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nphy analog registers:\n");
	lcd_reg_print_combo_dphy_serializer();

	reg = HHI_DIF_CSI_PHY_CNTL1;
	printf("PHY_CNTL1           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL2;
	printf("PHY_CNTL2           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL3;
	printf("PHY_CNTL3           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL4;
	printf("PHY_CNTL4           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL6;
	printf("PHY_CNTL6           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL8;
	printf("PHY_CNTL8           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL9;
	printf("PHY_CNTL9           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL10;
	printf("PHY_CNTL10          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL11;
	printf("PHY_CNTL11          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL12;
	printf("PHY_CNTL12          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL13;
	printf("PHY_CNTL13          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL14;
	printf("PHY_CNTL14          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL15;
	printf("PHY_CNTL15          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

static void lcd_reg_print_phy_analog_tl1(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nphy analog registers:\n");
	lcd_reg_print_serializer();

	reg = HHI_DIF_CSI_PHY_CNTL1;
	printf("PHY_CNTL1           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL2;
	printf("PHY_CNTL2           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL3;
	printf("PHY_CNTL3           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL4;
	printf("PHY_CNTL4           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL6;
	printf("PHY_CNTL6           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL7;
	printf("PHY_CNTL7           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL8;
	printf("PHY_CNTL8           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL9;
	printf("PHY_CNTL9           [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL10;
	printf("PHY_CNTL10          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL11;
	printf("PHY_CNTL11          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL12;
	printf("PHY_CNTL12          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL13;
	printf("PHY_CNTL13          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL14;
	printf("PHY_CNTL14          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL15;
	printf("PHY_CNTL15          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_DIF_CSI_PHY_CNTL16;
	printf("PHY_CNTL16          [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

static void lcd_reg_print_dphy_t7(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg0, reg1;

	switch (pdrv->index) {
	case 1:
		reg0 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL0;
		reg1 = COMBO_DPHY_EDP_LVDS_TX_PHY1_CNTL1;
		break;
	case 2:
		reg0 = COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL0;
		reg1 = COMBO_DPHY_EDP_LVDS_TX_PHY2_CNTL1;
		break;
	case 0:
	default:
		reg0 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL0;
		reg1 = COMBO_DPHY_EDP_LVDS_TX_PHY0_CNTL1;
		break;
	}

	printf("COMBO_DPHY_CNTL0    [0x%08x] = 0x%08x\n",
	       COMBO_DPHY_CNTL0, lcd_combo_dphy_read(COMBO_DPHY_CNTL0));
	printf("COMBO_DPHY_CNTL1    [0x%08x] = 0x%08x\n",
	       COMBO_DPHY_CNTL1, lcd_combo_dphy_read(COMBO_DPHY_CNTL1));
	printf("COMBO_DPHY_EDP_LVDS_TX_PHY%d_CNTL0    [0x%08x] = 0x%08x\n",
	       pdrv->index, reg0, lcd_combo_dphy_read(reg0));
	printf("COMBO_DPHY_EDP_LVDS_TX_PHY%d_CNTL1    [0x%08x] = 0x%08x\n",
	       pdrv->index, reg1, lcd_combo_dphy_read(reg1));
}

static void lcd_reg_print_phy_analog_t7(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nphy analog registers:\n");
	lcd_reg_print_dphy_t7(pdrv);

	reg = ANACTRL_DIF_PHY_CNTL1;
	printf("PHY_CNTL1           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL2;
	printf("PHY_CNTL2           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL3;
	printf("PHY_CNTL3           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL4;
	printf("PHY_CNTL4           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL5;
	printf("PHY_CNTL5           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL6;
	printf("PHY_CNTL6           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL7;
	printf("PHY_CNTL7           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL8;
	printf("PHY_CNTL8           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL9;
	printf("PHY_CNTL9           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL10;
	printf("PHY_CNTL10          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL11;
	printf("PHY_CNTL11          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL12;
	printf("PHY_CNTL12          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL13;
	printf("PHY_CNTL13          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL14;
	printf("PHY_CNTL14          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL15;
	printf("PHY_CNTL15          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL16;
	printf("PHY_CNTL16          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL17;
	printf("PHY_CNTL17          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL18;
	printf("PHY_CNTL18          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL19;
	printf("PHY_CNTL19          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL20;
	printf("PHY_CNTL20          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL21;
	printf("PHY_CNTL21          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
}

static void lcd_reg_print_dphy_t3(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg0, reg1;

	switch (pdrv->index) {
	case 1:
		reg0 = ANACTRL_LVDS_TX_PHY_CNTL2;
		reg1 = ANACTRL_LVDS_TX_PHY_CNTL3;
		break;
	case 0:
	default:
		reg0 = ANACTRL_LVDS_TX_PHY_CNTL0;
		reg1 = ANACTRL_LVDS_TX_PHY_CNTL1;
		break;
	}

	printf("ANACTRL_LVDS_TX_PHY_CNTL0    [0x%08x] = 0x%08x\n",
	       reg0, lcd_ana_read(reg0));
	printf("ANACTRL_LVDS_TX_PHY_CNTL1    [0x%08x] = 0x%08x\n",
	       reg1, lcd_ana_read(reg1));
}

static void lcd_reg_print_phy_analog_t3(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\nphy analog registers:\n");
	lcd_reg_print_dphy_t3(pdrv);

	reg = ANACTRL_DIF_PHY_CNTL1;
	printf("PHY_CNTL1           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL2;
	printf("PHY_CNTL2           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL3;
	printf("PHY_CNTL3           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL4;
	printf("PHY_CNTL4           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL5;
	printf("PHY_CNTL5           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL6;
	printf("PHY_CNTL6           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL7;
	printf("PHY_CNTL7           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL8;
	printf("PHY_CNTL8           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL9;
	printf("PHY_CNTL9           [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL10;
	printf("PHY_CNTL10          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL11;
	printf("PHY_CNTL11          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL12;
	printf("PHY_CNTL12          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL13;
	printf("PHY_CNTL13          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL14;
	printf("PHY_CNTL14          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL15;
	printf("PHY_CNTL15          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
	reg = ANACTRL_DIF_PHY_CNTL16;
	printf("PHY_CNTL16          [0x%08x] = 0x%08x\n",
	       reg, lcd_ana_read(reg));
}

static void lcd_reg_print_mipi_phy_analog(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;
#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	printf("\nphy analog registers:\n");
	reg = HHI_MIPI_CNTL0;
	printf("PHY_CNTL0   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_MIPI_CNTL1;
	printf("PHY_CNTL1   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = HHI_MIPI_CNTL2;
	printf("PHY_CNTL2   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

static void lcd_reg_print_mipi_phy_analog_c3(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;
#ifdef CONFIG_AML_LCD_PXP
	return;
#endif
	printf("\nphy analog registers:\n");
	reg = ANACTRL_MIPIDSI_CTRL0;
	printf("PHY_CNTL0   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = ANACTRL_MIPIDSI_CTRL1;
	printf("PHY_CNTL1   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
	reg = ANACTRL_MIPIDSI_CTRL2;
	printf("PHY_CNTL2   [0x%08x] = 0x%08x\n",
	       reg, lcd_clk_read(reg));
}

/* **********************************
 * lcd prbs function
 * **********************************
 */
unsigned int lcd_prbs_flag = 0, lcd_prbs_performed = 0, lcd_prbs_err = 0;

int lcd_prbs_test(struct aml_lcd_drv_s *pdrv, unsigned int ms,
		  unsigned int mode_flag)
{
	struct lcd_debug_info_reg_s *info_reg;
	int ret = -1;

	info_reg = (struct lcd_debug_info_reg_s *)pdrv->debug_info_reg;
	if (info_reg && info_reg->prbs_test)
		ret = info_reg->prbs_test(pdrv, ms, mode_flag);
	else
		LCDERR("[%d]: %s: don't support prbs test\n", pdrv->index, __func__);

	return ret;
}

void lcd_info_print(struct aml_lcd_drv_s *pdrv)
{
	unsigned int sync_duration;
	struct lcd_config_s *pconf;
	struct lcd_debug_info_if_s *info_if;

	pconf = &pdrv->config;
	LCDPR("[%d]: lcd driver version: %s\n", pdrv->index, LCD_DRV_VERSION);
	LCDPR("config_check_glb: %d, config_check_para: 0x%x, config_check_en: %d\n",
		pdrv->config_check_glb, pconf->basic.config_check, pdrv->config_check_en);
	LCDPR("key_valid: %d\n", pdrv->key_valid);
	LCDPR("custom_pinmux: %d\n", pconf->custom_pinmux);
	LCDPR("mode: %s, status: %d\n",
	      lcd_mode_mode_to_str(pdrv->mode), pdrv->status);

	sync_duration = pconf->timing.act_timing.sync_duration_num;
	sync_duration = (sync_duration * 100 / pconf->timing.act_timing.sync_duration_den);
	LCDPR("%s, %s %ubit, %dppc, %ux%u@%d.%02dHz\n"
		"lcd_clk           %uHz\n"
		"enc_clk           %uHz\n"
		"clk_mode          %s(%d)\n"
		"ss_level          %d\n"
		"ss_freq           %d\n"
		"ss_mode           %d\n"
		"fr_adj_type       %d\n\n",
		pconf->basic.model_name,
		lcd_type_type_to_str(pconf->basic.lcd_type),
		pconf->basic.lcd_bits, pconf->timing.ppc,
		pconf->timing.act_timing.h_active, pconf->timing.act_timing.v_active,
		(sync_duration / 100), (sync_duration % 100),
		pconf->timing.act_timing.pixel_clk, pconf->timing.enc_clk,
		(pconf->timing.clk_mode ? "independence" : "dependence"),
		pconf->timing.clk_mode, pconf->timing.ss_level,
		pconf->timing.ss_freq, pconf->timing.ss_mode,
		pconf->timing.act_timing.fr_adjust_type);

	lcd_timing_info_print(pdrv);

	info_if = (struct lcd_debug_info_if_s *)pdrv->debug_info_if;
	if (info_if) {
		if (info_if->interface_print)
			info_if->interface_print(pdrv);
		else
			LCDERR("%s: interface_print is null\n", __func__);
	} else {
		LCDERR("%s: lcd_debug_info_if is null\n", __func__);
	}

	lcd_phy_print(pconf);

	lcd_cus_ctrl_dump_info(pdrv);

	lcd_power_info_print(pdrv, 1);
	lcd_power_info_print(pdrv, 0);

	lcd_gpio_info_print(pdrv);

	printf("\n");
	lcd_clk_config_print(pdrv);
}

void lcd_reg_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_debug_info_reg_s *info_reg;
	struct lcd_debug_info_if_s *info_if;
	unsigned int *table;
	int i = 0;

	info_reg = (struct lcd_debug_info_reg_s *)pdrv->debug_info_reg;
	info_if = (struct lcd_debug_info_if_s *)pdrv->debug_info_if;

	if (!info_reg) {
		LCDERR("%s: lcd_debug_info_reg is null\n", __func__);
		goto lcd_reg_print_next;
	}
	LCDPR("[%d]: lcd regs:\n", pdrv->index);
	if (info_reg->reg_pll_table) {
		table = info_reg->reg_pll_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("pll     [0x%08x] = 0x%08x\n",
				table[i], lcd_ana_read(table[i]));
			i++;
		}
	}
	if (info_reg->reg_clk_table) {
		table = info_reg->reg_clk_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("clk     [0x%08x] = 0x%08x\n",
				table[i], lcd_clk_read(table[i]));
			i++;
		}
	}
	if (info_reg->reg_clk_combo_dphy_table) {
		table = info_reg->reg_clk_combo_dphy_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("combo_dphy [0x%08x] = 0x%08x\n",
				table[i], lcd_combo_dphy_read(table[i]));
			i++;
		}
	}

	if (info_reg->reg_encl_table) {
		printf("\nencl regs:\n");
		table = info_reg->reg_encl_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("vcbus   [0x%04x] = 0x%08x\n",
				table[i], lcd_vcbus_read(table[i]));
			i++;
		}
	}

	if (info_reg->reg_pinmux_table) {
		printf("\npinmux regs:\n");
		table = info_reg->reg_pinmux_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("PERIPHS_PIN_MUX  [0x%08x] = 0x%08x\n",
				table[i], lcd_periphs_read(table[i]));
			i++;
		}
	}

lcd_reg_print_next:
	if (!info_if) {
		LCDERR("%s: lcd_debug_info_if is null\n", __func__);
		return;
	}
	if (info_if->reg_dump_interface)
		info_if->reg_dump_interface(pdrv);

	if (info_if->reg_dump_phy)
		info_if->reg_dump_phy(pdrv);
}

/* **********************************
 * lcd debug match data
 * **********************************
 */
/* chip_type data */
static struct lcd_debug_info_reg_s lcd_debug_info_reg_g12a_clk_path0 = {
	.reg_pll_table = NULL,
	.reg_clk_table = lcd_reg_dump_clk_hpll_g12a,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_dft,
	.reg_pinmux_table = NULL,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_g12a_clk_path1 = {
	.reg_pll_table = NULL,
	.reg_clk_table = lcd_reg_dump_clk_gp0_g12a,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_dft,
	.reg_pinmux_table = NULL,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_tl1 = {
	.reg_pll_table = NULL,
	.reg_clk_table = lcd_reg_dump_clk_tl1,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_tl1,
	.reg_pinmux_table = lcd_reg_dump_pinmux_tl1,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t5w = {
	.reg_pll_table = NULL,
	.reg_clk_table = lcd_reg_dump_clk_tl1,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_t5w,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t5w,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_txhd2 = {
	.reg_pll_table = NULL,
	.reg_clk_table = lcd_reg_dump_clk_txhd2,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_txhd2,
	.reg_encl_table = lcd_reg_dump_encl_tl1,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t5w,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t7_0 = {
	.reg_pll_table = lcd_reg_dump_pll_t7_0,
	.reg_clk_table = lcd_reg_dump_clk_t7_0,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_t7_0,
	.reg_encl_table = lcd_reg_dump_encl_t7_0,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t7,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t7_1 = {
	.reg_pll_table = lcd_reg_dump_pll_t7_1,
	.reg_clk_table = lcd_reg_dump_clk_t7_1,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_t7_1,
	.reg_encl_table = lcd_reg_dump_encl_t7_1,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t7,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t7_2 = {
	.reg_pll_table = lcd_reg_dump_pll_t7_2,
	.reg_clk_table = lcd_reg_dump_clk_t7_2,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_t7_2,
	.reg_encl_table = lcd_reg_dump_encl_t7_2,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t7,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t3_0 = {
	.reg_pll_table = lcd_reg_dump_pll_t3,
	.reg_clk_table = lcd_reg_dump_clk_t7_0,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_t7_0,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t3_1 = {
	.reg_pll_table = lcd_reg_dump_pll_t3,
	.reg_clk_table = lcd_reg_dump_clk_t7_1,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_t7_1,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t3x_0 = {
	.reg_pll_table = lcd_reg_dump_pll_t7_0,
	.reg_clk_table = lcd_reg_dump_clk_t7_0,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_t7_0,
	.reg_encl_table = lcd_reg_dump_encl_t3x_0,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_t3x_1 = {
	.reg_pll_table = lcd_reg_dump_pll_t7_1,
	.reg_clk_table = lcd_reg_dump_clk_t7_1,
	.reg_clk_combo_dphy_table = lcd_reg_dump_clk_combo_dphy_t7_1,
	.reg_encl_table = lcd_reg_dump_encl_t3x_1,
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,
};

static struct lcd_debug_info_reg_s lcd_debug_info_reg_c3 = {
	.reg_pll_table = lcd_reg_dump_pll_c3,
	.reg_clk_table = lcd_reg_dump_clk_c3,
	.reg_clk_combo_dphy_table = NULL,
	.reg_encl_table = lcd_reg_dump_encl_c3,
	.reg_pinmux_table = lcd_reg_dump_pinmux_c3,
};

/* interface data */
static struct lcd_debug_info_if_s lcd_debug_info_if_rgb = {
	.interface_print = lcd_info_print_rgb,
	.reg_dump_interface = lcd_reg_print_rgb,
	.reg_dump_phy = NULL,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_bt = {
	.interface_print = lcd_info_print_bt,
	.reg_dump_interface = lcd_reg_print_bt,
	.reg_dump_phy = NULL,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_lvds = {
	.interface_print = lcd_info_print_lvds,
	.reg_dump_interface = lcd_reg_print_lvds,
	.reg_dump_phy = lcd_reg_print_phy_analog,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_vbyone = {
	.interface_print = lcd_info_print_vbyone,
	.reg_dump_interface = lcd_reg_print_vbyone,
	.reg_dump_phy = lcd_reg_print_phy_analog,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_mipi = {
	.interface_print = lcd_info_print_mipi,
	.reg_dump_interface = lcd_reg_print_mipi,
	.reg_dump_phy = lcd_reg_print_mipi_phy_analog,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_edp = {
	.interface_print = lcd_info_print_edp,
	.reg_dump_interface = lcd_reg_print_edp,
	.reg_dump_phy = lcd_reg_print_phy_analog_t7,
};

#ifdef CONFIG_AML_LCD_TCON
static struct lcd_debug_info_if_s lcd_debug_info_if_mlvds = {
	.interface_print = lcd_info_print_mlvds,
	.reg_dump_interface = lcd_reg_print_tcon_tl1,
	.reg_dump_phy = lcd_reg_print_phy_analog,
};

static struct lcd_debug_info_if_s lcd_debug_info_if_p2p = {
	.interface_print = lcd_info_print_p2p,
	.reg_dump_interface = lcd_reg_print_tcon_tl1,
	.reg_dump_phy = lcd_reg_print_phy_analog,
};
#endif

void lcd_debug_probe(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_debug_info_reg_s *lcd_debug_info_reg = NULL;
	struct lcd_debug_info_if_s *lcd_debug_info_if = NULL;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
		switch (pdrv->index) {
		case 1:
			lcd_debug_info_reg = &lcd_debug_info_reg_t7_1;
			break;
		case 2:
			lcd_debug_info_reg = &lcd_debug_info_reg_t7_2;
			break;
		case 0:
		default:
			lcd_debug_info_reg = &lcd_debug_info_reg_t7_0;
			break;
		}
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
		lcd_debug_info_if_vbyone.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
		lcd_debug_info_if_mipi.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
		break;
	case LCD_CHIP_T5M:
	case LCD_CHIP_T3:
		switch (pdrv->index) {
		case 1:
			lcd_debug_info_reg = &lcd_debug_info_reg_t3_1;
			break;
		default:
			lcd_debug_info_reg = &lcd_debug_info_reg_t3_0;
			break;
		}
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_t3;
		lcd_debug_info_if_vbyone.reg_dump_phy =
			lcd_reg_print_phy_analog_t3;
#ifdef CONFIG_AML_LCD_TCON
		lcd_debug_info_if_mlvds.reg_dump_interface =
			lcd_reg_print_tcon_t3;
		lcd_debug_info_if_mlvds.reg_dump_phy =
			lcd_reg_print_phy_analog_t3;
		lcd_debug_info_if_p2p.reg_dump_interface =
			lcd_reg_print_tcon_t3;
		lcd_debug_info_if_p2p.reg_dump_phy =
			lcd_reg_print_phy_analog_t3;
#endif
		break;
	case LCD_CHIP_T3X:
		switch (pdrv->index) {
		case 1:
			lcd_debug_info_reg = &lcd_debug_info_reg_t3x_1;
			break;
		default:
			lcd_debug_info_reg = &lcd_debug_info_reg_t3x_0;
			if (pdrv->config.timing.clk_mode == LCD_CLK_MODE_INDEPENDENCE) {
				lcd_debug_info_reg->reg_pll_table =
					lcd_reg_dump_pll_t3x_independence;
				lcd_debug_info_reg->reg_clk_combo_dphy_table =
					lcd_reg_dump_clk_combo_dphy_t3x_independence;
			}
			break;
		}
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
		lcd_debug_info_if_vbyone.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
#ifdef CONFIG_AML_LCD_TCON
		lcd_debug_info_if_mlvds.reg_dump_interface =
			lcd_reg_print_tcon_t3;
		lcd_debug_info_if_mlvds.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
		lcd_debug_info_if_p2p.reg_dump_interface =
			lcd_reg_print_tcon_t3;
		lcd_debug_info_if_p2p.reg_dump_phy =
			lcd_reg_print_phy_analog_t7;
#endif
		break;

	case LCD_CHIP_T5W:
		lcd_debug_info_reg = &lcd_debug_info_reg_t5w;
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
		lcd_debug_info_if_mipi.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
#ifdef CONFIG_AML_LCD_TCON
		lcd_debug_info_if_mlvds.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
		lcd_debug_info_if_p2p.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
#endif
		break;
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
	case LCD_CHIP_T5:
	case LCD_CHIP_T5D:
		lcd_debug_info_reg = &lcd_debug_info_reg_tl1;
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
		lcd_debug_info_if_mipi.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
#ifdef CONFIG_AML_LCD_TCON
		lcd_debug_info_if_mlvds.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
		lcd_debug_info_if_p2p.reg_dump_phy =
			lcd_reg_print_phy_analog_tl1;
#endif
		break;
	case LCD_CHIP_TXHD2:
		lcd_debug_info_reg = &lcd_debug_info_reg_txhd2;
		lcd_debug_info_if_lvds.reg_dump_phy =
			lcd_reg_print_phy_analog_txhd2;
		lcd_debug_info_if_mipi.reg_dump_phy =
			lcd_reg_print_phy_analog_txhd2;
#ifdef CONFIG_AML_LCD_TCON
		lcd_debug_info_if_mlvds.reg_dump_phy =
			lcd_reg_print_phy_analog_txhd2;
		lcd_debug_info_if_p2p.reg_dump_phy =
			lcd_reg_print_phy_analog_txhd2;
#endif
		break;
	case LCD_CHIP_G12A:
	case LCD_CHIP_G12B:
	case LCD_CHIP_SM1:
		if (pdrv->clk_path)
			lcd_debug_info_reg = &lcd_debug_info_reg_g12a_clk_path1;
		else
			lcd_debug_info_reg = &lcd_debug_info_reg_g12a_clk_path0;
		break;
	case LCD_CHIP_C3:
		lcd_debug_info_reg = &lcd_debug_info_reg_c3;
		lcd_debug_info_if_mipi.reg_dump_phy =
			lcd_reg_print_mipi_phy_analog_c3;
		break;
	default:
		lcd_debug_info_reg = NULL;
		break;
	}

	switch (pdrv->config.basic.lcd_type) {
	case LCD_RGB:
		lcd_debug_info_if = &lcd_debug_info_if_rgb;
		break;
	case LCD_BT656:
	case LCD_BT1120:
		lcd_debug_info_if = &lcd_debug_info_if_bt;
		break;
	case LCD_LVDS:
		lcd_debug_info_if = &lcd_debug_info_if_lvds;
		break;
	case LCD_VBYONE:
		lcd_debug_info_if = &lcd_debug_info_if_vbyone;
		break;
	case LCD_MIPI:
		lcd_debug_info_if = &lcd_debug_info_if_mipi;
		break;
	case LCD_EDP:
		lcd_debug_info_if = &lcd_debug_info_if_edp;
		break;
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_debug_info_if = &lcd_debug_info_if_mlvds;
		break;
	case LCD_P2P:
		lcd_debug_info_if = &lcd_debug_info_if_p2p;
		break;
#endif
	default:
		lcd_debug_info_if = NULL;
		break;
	}

	pdrv->debug_info_reg = (void *)lcd_debug_info_reg;
	pdrv->debug_info_if = (void *)lcd_debug_info_if;
}
