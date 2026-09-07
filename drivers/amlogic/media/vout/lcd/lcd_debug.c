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
#include "./connectors/lcd_connector.h"

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

void str_add_reg_sets(struct aml_lcd_drv_s *pdrv,
		      unsigned char reg_bus, unsigned int reg_offset,
		      struct reg_name_set_s *reg_table, unsigned char reg_cnt)
{
	unsigned char idx, str_pos = 0;
	unsigned int  reg_addr = 0, reg_val = 0;

	for (idx = 0; idx < reg_cnt; idx++) {
		if (strlen(reg_table[idx].name) > str_pos)
			str_pos = strlen(reg_table[idx].name);
	}
	str_pos++;

	for (idx = 0; idx < reg_cnt; idx++) {
		switch (reg_bus) {
		case LCD_REG_DBG_VC_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_vcbus_read(reg_addr);
			break;
		case LCD_REG_DBG_ANA_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_ana_read(reg_addr);
			break;
		case LCD_REG_DBG_CLK_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_clk_read(reg_addr);
			break;
		case LCD_REG_DBG_PERIPHS_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_periphs_read(reg_addr);
			break;
#ifdef CONFIG_AML_LCD_MIPI_DSI
		case LCD_REG_DBG_MIPIHOST_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			if (pdrv->index)
				reg_addr = REG_ADDR_DSI_1_HOST_A(reg_addr);
			else
				reg_addr = REG_ADDR_DSI_0_HOST_A(reg_addr);
			reg_val = dsi_host_read(pdrv, 0, reg_addr);
			break;
		case LCD_REG_DBG_MIPIPHY_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			if (pdrv->index)
				reg_addr = REG_ADDR_DSI_1_PHY_A(reg_addr);
			else
				reg_addr = REG_ADDR_DSI_0_PHY_A(reg_addr);
			reg_val = dsi_phy_read(pdrv, 0, reg_addr);
			break;
#endif
#ifdef CONFIG_AML_LCD_TCON
		case LCD_REG_DBG_TCON_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_tcon_read(reg_addr);
			break;
#endif
		case LCD_REG_DBG_COMBOPHY_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_combo_dphy_read(reg_addr);
			break;
		case LCD_REG_DBG_RST_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_reset_read(reg_addr);
			break;
		case LCD_REG_DBG_HHI_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_hiu_read(reg_addr);
			break;
		default:
			return;
		}

		printf("%-*s [0x%04x] = 0x%08x\n", str_pos,
		       reg_table[idx].name, reg_addr, reg_val);
	}
}

static void lcd_timing_info_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	struct lcd_clk_config_s *cconf = get_lcd_clk_config(pdrv);
	int i = 0, base_id = 0;
	struct lcd_detail_timing_s *dt, *base, *dft;

	if (!cconf)
		return;

	base = pconf->timing.base_timing;
	dft = pconf->timing.dft_timing;
	if (!base || !dft)
		return;

	printf("support timings:\n");
	for (i = 0; i < pconf->timing.num_timings; i++) {
		dt = pconf->timing.timings[i];
		if (base == dt)
			base_id = i;
		printf("timing[%d]:%s%s\n",
		       i, base == dt ? "(base)" : "", dft == dt ? "(dft)" : "");
		lcd_detail_timing_print(pdrv, dt);
	}

	dt = &pconf->timing.act_timing;
	printf("active timing: based timing[%d]:\n", base_id);
	lcd_detail_timing_print(pdrv, dt);

	printf("pre_de_h:%d, pre_de_v:%d\n"
	       "hstart:%d, vstart:%d\n\n",
	       pconf->timing.pre_de_h, pconf->timing.pre_de_v,
	       pconf->timing.hstart, pconf->timing.vstart);

	printf("pll_ctrl       0x%08x\n"
		"div_ctrl       0x%08x\n"
		"clk_ctrl       0x%08x\n",
		pconf->timing.pll_ctrl, pconf->timing.div_ctrl,
		pconf->timing.clk_ctrl);

	if (cconf->pll_mode & LCD_PLL_MODE_DUAL_PLL) {
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

		printf("  %d: type=%d, index=%d, value=%d, delay=%d\n",
		       i, power_step->type, power_step->index,
		       power_step->value, power_step->delay);
		i++;
		power_step++;
	}
}

#ifdef CONFIG_AML_LCD_LVDS
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
#endif

#ifdef CONFIG_AML_LCD_VBYONE
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
}
#endif

#ifdef CONFIG_AML_LCD_MIPI_DSI
static void lcd_info_print_mipi(struct aml_lcd_drv_s *pdrv)
{
	lcd_dsi_info_print(&pdrv->config);
}
#endif

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
		"bit_rate          %lluHz\n\n",
		pdrv->config.control.mlvds_cfg.channel_num,
		pdrv->config.control.mlvds_cfg.channel_sel0,
		pdrv->config.control.mlvds_cfg.channel_sel1,
		pdrv->config.control.mlvds_cfg.clk_phase,
		pdrv->config.control.mlvds_cfg.pn_swap,
		pdrv->config.control.mlvds_cfg.bit_swap,
		pdrv->config.control.mlvds_cfg.phy_vswing,
		pdrv->config.control.mlvds_cfg.phy_preem,
		pdrv->config.timing.bit_rate);
	lcd_tcon_info_print(pdrv);
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
}
#endif

static void lcd_phy_print(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	int base_id = 0, i;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MIPI:
	case LCD_MLVDS:
	case LCD_P2P:
		printf("support phy group:\n");
		for (i = 0; i < phy_cfg->group_num; i++) {
			phy = pdrv->config.phy_cfg.phys[i];
			if (!phy)
				continue;
			printf("phy_attr[%d]:\n", i);
			lcd_phy_attr_print(phy, phy_cfg->lane_num);
			if (phy == pdrv->config.phy_cfg.act_phy)
				base_id = i;
		}

		printf("active phy_attr(group[%d]):\n", base_id);
		lcd_phy_param_print(pdrv);
		printf("\n");
		break;
	default:
		break;
	}
}

#ifdef CONFIG_AML_LCD_LVDS
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
}
#endif

#ifdef CONFIG_AML_LCD_VBYONE
struct reg_name_set_s vx1_table_t3x_t6x[] = {
	{VBO_CTRL             , "VBO_CTRL           "},
	{VBO_SOFT_RST         , "VBO_SOFT_RST       "},
	{VBO_LANES            , "VBO_LANES          "},
	{VBO_VIN_CTRL         , "VBO_VIN_CTRL       "},
	{VBO_ACT_VSIZE        , "VBO_ACT_VSIZE      "},
	{VBO_LANE_SKEW_L      , "VBO_LANE_SKEW_L    "},
	{VBO_LANE_SKEW_H      , "VBO_LANE_SKEW_H    "},
	{VBO_STATUS_L         , "VBO_STATUS_L       "},
	{VBO_STATUS_H         , "VBO_STATUS_H       "},
	{VBO_LANE_OUTPUT      , "VBO_LANE_OUTPUT    "},
	{LCD_PORT_SWAP        , "LCD_PORT_SWAP      "},
	{VBO_TMCHK_THRD       , "VBO_TMCHK_THRD     "},
	{VBO_FSM_HOLDER       , "VBO_FSM_HOLDER     "},
	{VBO_INTR_STATE_CTRL  , "VBO_INTR_STATE_CTRL"},
	{VBO_INTR_UNMASK      , "VBO_INTR_UNMASK    "},
	{VBO_INTR_STATE       , "VBO_INTR_STATE     "},
	{VBO_INFILTER_CTRL    , "VBO_INFILTER_CTRL  "},
	{VBO_INSGN_CTRL       , "VBO_INSGN_CTRL     "},
	{VBO_INFILTER_CTRL_H  , "VBO_INFILTER_CTRL_H"},
	{VBO_SLICE_CTRL       , "VBO_SLICE_CTRL     "},
	{VBO_RGN_CTRL         , "VBO_RGN_CTRL       "},
	{VBO_RGN_HSIZE        , "VBO_RGN_HSIZE      "}
};

struct reg_name_set_s vx1_table[] = {
	{VBO_STATUS_L       , "BO_STATUS_L       "},
	{VBO_LANE_OUTPUT    , "VBO_LANE_OUTPUT   "},
	{LCD_PORT_SWAP      , "LCD_PORT_SWAP     "},
	{VBO_CTRL_L         , "BO_CTRL_L         "},
	{VBO_LANES          , "BO_LANES          "},
	{VBO_VIN_CTRL       , "BO_VIN_CTRL       "},
	{VBO_ACT_VSIZE      , "BO_ACT_VSIZE      "},
	{VBO_GCLK_MAIN      , "BO_GCLK_MAIN      "},
	{VBO_FSM_HOLDER_L   , "BO_FSM_HOLDER_L   "},
	{VBO_FSM_HOLDER_H   , "BO_FSM_HOLDER_H   "},
	{VBO_INTR_STATE_CTRL, "BO_INTR_STATE_CTRL"},
	{VBO_INTR_UNMASK    , "BO_INTR_UNMASK    "},
	{VBO_INTR_STATE     , "BO_INTR_STATE     "},
	{VBO_INFILTER_CTRL  , "BO_INFILTER_CTRL  "},
	{VBO_INSGN_CTRL     , "BO_INSGN_CTRL     "},
	{VBO_INFILTER_CTRL_H, "BO_INFILTER_CTRL_H"}
};

static void lcd_reg_print_vbyone(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset, reg_cnt, i, base_addr = 0;
	struct reg_name_set_s *regtbl;

	offset = pdrv->data->offset_venc_if[pdrv->index];
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T3X:
	case LCD_CHIP_T6X:
		regtbl = vx1_table_t3x_t6x;
		reg_cnt = ARRAY_SIZE(vx1_table_t3x_t6x);
		base_addr = 0xff800000;
		break;
	default:
		regtbl = vx1_table;
		reg_cnt = ARRAY_SIZE(vx1_table);
		base_addr = 0;
	}

	printf("\nvbyone registers:\n");
	if (base_addr)
		for (i = 0; i < reg_cnt; i++)
			printf("%s-[0x%08x][0x%04x] = 0x%08x\n", regtbl[i].name, regtbl[i].reg,
				(regtbl[i].reg - base_addr) >> 2, lcd_vcbus_read(regtbl[i].reg));
	else
		for (i = 0; i < reg_cnt; i++)
			printf("%s-[0x%08x] = 0x%08x\n", regtbl[i].name, regtbl[i].reg,
				lcd_vcbus_read(regtbl[i].reg));
}

#endif

#ifdef CONFIG_AML_LCD_TCON
static void lcd_reg_print_tcon(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{TCON_TOP_CTRL,     "TCON_TOP_CTRL"},
		{TCON_RGB_IN_MUX,   "TCON_RGB_IN_MUX"},
		{TCON_OUT_CH_SEL0,  "TCON_OUT_CH_SEL0"},
		{TCON_OUT_CH_SEL1,  "TCON_OUT_CH_SEL1"},
		{TCON_STATUS0,      "TCON_STATUS0"},
		{TCON_PLLLOCK_CNTL, "TCON_PLLLOCK_CNTL"},
		{TCON_RST_CTRL,     "TCON_RST_CTRL"},
		{TCON_AXI_OFST0,    "TCON_AXI_OFST0"},
		{TCON_AXI_OFST1,    "TCON_AXI_OFST1"},
		{TCON_AXI_OFST2,    "TCON_AXI_OFST2"},
		{TCON_CLK_CTRL,     "TCON_CLK_CTRL"},
		{TCON_STATUS1,      "TCON_STATUS1"},
		{TCON_DDRIF_CTRL1,  "TCON_DDRIF_CTRL1"},
		{TCON_DDRIF_CTRL2,  "TCON_DDRIF_CTRL2"},
		{TCON_CLK_CTRL,     "TCON_CLK_CTRL"},
	};

	printf("\ntcon registers:\n");
	str_add_reg_sets(pdrv, LCD_REG_DBG_TCON_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
}
#endif

#ifdef CONFIG_AML_LCD_MIPI_DSI
static void lcd_reg_print_mipi(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s host_reg_table[] = {
		{MIPI_DSI_TOP_CNTL,            "TOP_CNTL"},
		{MIPI_DSI_TOP_CLK_CNTL,        "TOP_CLK_CNTL"},
		{MIPI_DSI_TOP_MEAS_CNTL,       "TOP_MEAS_CNTL"},
		{MIPI_DSI_TOP_STAT,            "TOP_STAT"},
		{MIPI_DSI_TOP_MEAS_STAT_TE0,   "TOP_MEAS_STAT_TE0"},
		{MIPI_DSI_TOP_MEAS_STAT_TE1,   "TOP_MEAS_STAT_TE1"},
		{MIPI_DSI_TOP_INTR_CNTL_STAT,  "TOP_INTR_CNTL_STAT"},
		{MIPI_DSI_DWC_CLKMGR_CFG_OS,        "CLKMGR_CFG"},
		{MIPI_DSI_DWC_DPI_COLOR_CODING_OS,  "DPI_COLOR_CODING"},
		{MIPI_DSI_DWC_DPI_LP_CMD_TIM_OS,    "DPI_LP_CMD_TIM"},
		{MIPI_DSI_DWC_PCKHDL_CFG_OS,        "PCKHDL_CFG"},
		{MIPI_DSI_DWC_MODE_CFG_OS,          "MODE_CFG"},
		{MIPI_DSI_DWC_VID_MODE_CFG_OS,      "VID_MODE_CFG"},
		{MIPI_DSI_DWC_VID_PKT_SIZE_OS,      "VID_PKT_SIZE"},
		{MIPI_DSI_DWC_VID_NUM_CHUNKS_OS,    "VID_NUM_CHUNKS"},
		{MIPI_DSI_DWC_VID_NULL_SIZE_OS,     "VID_NULL_SIZE"},
		{MIPI_DSI_DWC_VID_HSA_TIME_OS,      "VID_HSA_TIME"},
		{MIPI_DSI_DWC_VID_HBP_TIME_OS,      "VID_HBP_TIME"},
		{MIPI_DSI_DWC_VID_HLINE_TIME_OS,    "VID_HLINE_TIME"},
		{MIPI_DSI_DWC_VID_VSA_LINES_OS,     "VID_VSA_LINES"},
		{MIPI_DSI_DWC_VID_VBP_LINES_OS,     "VID_VBP_LINES"},
		{MIPI_DSI_DWC_VID_VFP_LINES_OS,     "VID_VFP_LINES"},
		{MIPI_DSI_DWC_VID_VACTIVE_LINES_OS, "VID_VACTIVE_LINES"},
		{MIPI_DSI_DWC_EDPI_CMD_SIZE_OS,     "EDPI_CMD_SIZE"},
		{MIPI_DSI_DWC_CMD_MODE_CFG_OS,      "CMD_MODE_CFG"},
		{MIPI_DSI_DWC_PHY_TMR_LPCLK_CFG_OS, "PHY_TMR_LPCLK_CFG"},
		{MIPI_DSI_DWC_PHY_TMR_CFG_OS,       "PHY_TMR_CFG"},
		{MIPI_DSI_DWC_PHY_STATUS_OS,        "PHY_STATUS"},
		{MIPI_DSI_DWC_INT_ST0_OS,           "INT_ST0"},
		{MIPI_DSI_DWC_INT_ST1_OS,           "INT_ST1"},
	};
	struct reg_name_set_s dsi_dphy_reg_table[] = {
		{MIPI_DSI_PHY_CTRL,     "PHY_CTRL"},
		{MIPI_DSI_CHAN_CTRL,    "CHAN_CTRL"},
		{MIPI_DSI_CHAN_STS,     "CHAN_STS"},
		{MIPI_DSI_CLK_TIM,      "CLK_TIM"},
		{MIPI_DSI_HS_TIM,       "HS_TIM"},
		{MIPI_DSI_LP_TIM,       "LP_TIM"},
		{MIPI_DSI_INIT_TIM,     "INIT_TIM"},
		{MIPI_DSI_WAKEUP_TIM,   "WAKEUP_TIM"},
		{MIPI_DSI_LPOK_TIM,     "LPOK_TIM"},
		{MIPI_DSI_LP_WCHDOG,    "LP_WCHDOG"},
		{MIPI_DSI_ANA_CTRL,     "ANA_CTRL"},
		{MIPI_DSI_CLK_TIM1,     "CLK_TIM1"},
		{MIPI_DSI_TURN_WCHDOG,  "TURN_WCHDOG"},
		{MIPI_DSI_ULPS_CHECK,   "ULPS_CHECK"},
		{MIPI_DSI_TEST_CTRL0,   "TEST_CTRL0"},
		{MIPI_DSI_TEST_CTRL1,   "TEST_CTRL1"},
	};

	printf("\nmipi_dsi registers:\n");
	str_add_reg_sets(pdrv, LCD_REG_DBG_MIPIHOST_BUS, 0,
		host_reg_table, ARRAY_SIZE(host_reg_table));
	str_add_reg_sets(pdrv, LCD_REG_DBG_MIPIHOST_BUS, 0,
		dsi_dphy_reg_table, ARRAY_SIZE(dsi_dphy_reg_table));
}
#endif

void lcd_info_print(struct aml_lcd_drv_s *pdrv)
{
	unsigned int sync_duration;
	struct lcd_config_s *pconf;
	struct lcd_debug_info_s *dbg_info;

	pconf = &pdrv->config;
	LCDPR("[%d]: lcd driver version: %s\n", pdrv->index, LCD_DRV_VERSION);
	LCDPR("config_check_glb: %d, config_check_para: 0x%x, config_check_en: %d\n",
		pdrv->config_check_glb, pconf->basic.config_check, pdrv->config_check_en);
	LCDPR("key_valid: %d\n", pdrv->key_valid);
	LCDPR("custom_pinmux: %s\n", pconf->cus_pinmux_name);
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
		pdrv->config.timing.act_timing.lcd_bits, pconf->timing.ppc,
		pconf->timing.act_timing.h_active, pconf->timing.act_timing.v_active,
		(sync_duration / 100), (sync_duration % 100),
		pconf->timing.act_timing.pixel_clk, pconf->timing.enc_clk,
		(pconf->timing.act_timing.clk_mode ? "adaptive" : "fixed"),
		pconf->timing.act_timing.clk_mode, pconf->timing.ss_level,
		pconf->timing.ss_freq, pconf->timing.ss_mode,
		pconf->timing.act_timing.fr_adjust_type);

	lcd_timing_info_print(pdrv);

	dbg_info = (struct lcd_debug_info_s *)pdrv->debug_info;
	if (dbg_info) {
		if (dbg_info->interface_print)
			dbg_info->interface_print(pdrv);
		else
			LCDERR("%s: interface_print is null\n", __func__);
	} else {
		LCDERR("%s: lcd_debug_info_if is null\n", __func__);
	}

	lcd_phy_print(pdrv);
	lcd_power_info_print(pdrv, 1);
	lcd_power_info_print(pdrv, 0);

	lcd_gpio_info_print(pdrv);

	printf("\n");
	lcd_clk_config_print(pdrv);
}

void lcd_reg_print(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_debug_info_s *dbg_info;
	unsigned int *table;
	int i = 0;

	dbg_info = (struct lcd_debug_info_s *)pdrv->debug_info;
	if (!dbg_info)
		LCDERR("%s: lcd_debug_info is null\n", __func__);

	LCDPR("[%d]: lcd regs:\n", pdrv->index);
	lcd_clk_reg_print(pdrv);

	lcd_venc_reg_print(pdrv);

	if (dbg_info && dbg_info->reg_pinmux_table) {
		printf("\npinmux regs:\n");
		table = dbg_info->reg_pinmux_table;
		i = 0;
		while (i < LCD_DEBUG_REG_CNT_MAX) {
			if (table[i] == LCD_DEBUG_REG_END)
				break;
			printf("PERIPHS_PIN_MUX  [0x%08x] = 0x%08x\n",
				table[i], lcd_periphs_read(table[i]));
			i++;
		}
	}

	if (dbg_info && dbg_info->reg_dump_interface)
		dbg_info->reg_dump_interface(pdrv);

	lcd_dphy_reg_print(pdrv);
	lcd_phy_analog_reg_print(pdrv);
}

/* **********************************
 * lcd debug match data
 * **********************************
 */
#if defined(CONFIG_MESON_T5M) || defined(CONFIG_MESON_T6D) || defined(CONFIG_MESON_T6W)
static struct lcd_debug_info_s lcd_debug_info_t5m = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = lcd_reg_print_lvds,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = lcd_reg_print_tcon,
#endif
};
#endif

#if defined(CONFIG_MESON_TXHD2)
static struct lcd_debug_info_s lcd_debug_info_txhd2 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_txdh2,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = lcd_reg_print_lvds,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = NULL,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = NULL,
#endif
};
#endif

#if defined(CONFIG_MESON_T3X)
static struct lcd_debug_info_s lcd_debug_info_t3x_0 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = lcd_reg_print_lvds,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = lcd_reg_print_tcon,
#endif
};

static struct lcd_debug_info_s lcd_debug_info_t3x_1 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = NULL,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = NULL,
	.reg_dump_p2p    = NULL,
#endif
};
#endif

#if defined(CONFIG_MESON_C3)
static struct lcd_debug_info_s lcd_debug_info_c3 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_c3,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = NULL,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = NULL,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = lcd_reg_print_mipi,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = NULL,
	.reg_dump_p2p    = NULL,
#endif
};
#endif

#if defined(CONFIG_MESON_S6)
static struct lcd_debug_info_s lcd_debug_info_s6 = {
	.reg_pinmux_table = NULL,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = NULL,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = NULL,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = lcd_reg_print_mipi,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = NULL,
	.reg_dump_p2p    = NULL,
#endif
};
#endif

#if defined(CONFIG_MESON_T6X)
static struct lcd_debug_info_s lcd_debug_info_t6x = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

#ifdef CONFIG_AML_LCD_LVDS
	.reg_dump_lvds   = lcd_reg_print_lvds,
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	.reg_dump_mipi   = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = lcd_reg_print_tcon,
#endif
};
#endif

void lcd_debug_probe(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_debug_info_s *lcd_debug_info = NULL;

	switch (pdrv->data->chip_type) {
#if defined(CONFIG_MESON_T5M) || defined(CONFIG_MESON_T6D) || defined(CONFIG_MESON_T6W)
	case LCD_CHIP_T5M:
	case LCD_CHIP_T6D:
	case LCD_CHIP_T6W:
		lcd_debug_info = &lcd_debug_info_t5m;
		break;
#endif
#if defined(CONFIG_MESON_T3X)
	case LCD_CHIP_T3X:
		switch (pdrv->index) {
		case 1:
			lcd_debug_info = &lcd_debug_info_t3x_1;
			break;
		default:
			lcd_debug_info = &lcd_debug_info_t3x_0;
			break;
		}
		break;
#endif
#if defined(CONFIG_MESON_T6X)
	case LCD_CHIP_T6X:
		lcd_debug_info = &lcd_debug_info_t6x;
		break;
#endif
#if defined(CONFIG_MESON_TXHD2)
	case LCD_CHIP_TXHD2:
		lcd_debug_info = &lcd_debug_info_txhd2;
		break;
#endif
#if defined(CONFIG_MESON_C3)
	case LCD_CHIP_C3:
		lcd_debug_info = &lcd_debug_info_c3;
		break;
#endif
#if defined(CONFIG_MESON_S6)
	case LCD_CHIP_S6:
		lcd_debug_info = &lcd_debug_info_s6;
		break;
#endif
	default:
		lcd_debug_info = NULL;
		return;
	}

	switch (pdrv->config.basic.lcd_type) {
#ifdef CONFIG_AML_LCD_LVDS
	case LCD_LVDS:
		lcd_debug_info->interface_print = lcd_info_print_lvds;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_lvds;
		break;
#endif
#ifdef CONFIG_AML_LCD_VBYONE
	case LCD_VBYONE:
		lcd_debug_info->interface_print = lcd_info_print_vbyone;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_vbyone;
		break;
#endif
#ifdef CONFIG_AML_LCD_MIPI_DSI
	case LCD_MIPI:
		lcd_debug_info->interface_print = lcd_info_print_mipi;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_mipi;
		break;
#endif
#ifdef CONFIG_AML_LCD_TCON
	case LCD_MLVDS:
		lcd_debug_info->interface_print = lcd_info_print_mlvds;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_mlvds;
		break;
	case LCD_P2P:
		lcd_debug_info->interface_print = lcd_info_print_p2p;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_p2p;
		break;
#endif
	default:
		lcd_debug_info->interface_print = NULL;
		lcd_debug_info->reg_dump_interface = NULL;
		break;
	}

	pdrv->debug_info = (void *)lcd_debug_info;
}
