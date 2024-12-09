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

void str_add_reg_sets(struct aml_lcd_drv_s *pdrv,
		      unsigned char reg_bus, unsigned int reg_offset,
		      struct reg_name_set_s *reg_table, unsigned char reg_cnt)
{
	unsigned char idx, str_pos = 0;
#ifdef CONFIG_AML_LCD_TABLET
	unsigned char reg_temp;
#endif
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
#ifdef CONFIG_AML_LCD_TABLET
		case LCD_REG_DBG_MIPIHOST_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			if (pdrv->index)
				reg_addr = REG_ADDR_DSI_B_HOST(reg_addr);
			else
				reg_addr = REG_ADDR_DSI_HOST(reg_addr);
			reg_val = dsi_host_read(pdrv, reg_addr);
			break;
		case LCD_REG_DBG_MIPIPHY_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			if (pdrv->index)
				reg_addr = REG_ADDR_DSI_B_PHY(reg_addr);
			else
				reg_addr = REG_ADDR_DSI_PHY(reg_addr);
			reg_val = dsi_phy_read(pdrv, reg_addr);
			break;
		case LCD_REG_DBG_EDPHOST_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = dptx_reg_read(pdrv->index, reg_addr);
			break;
		case LCD_REG_DBG_EDPDPCD_BUS:
			reg_addr = reg_table[idx].reg;
			if (dptx_aux_read(pdrv, reg_addr, 1, &reg_temp))
				continue;
			reg_val = reg_temp;
			break;
#endif
#ifdef CONFIG_AMLOGIC_LCD_TV
		case LCD_REG_DBG_TCON_BUS:
			reg_addr = reg_table[idx].reg + reg_offset;
			reg_val = lcd_tcon_reg_read(pdrv, reg_addr);
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
	int i = 0, base_id = 0;
	struct lcd_detail_timing_s *dt, *base, *dft;

	base = pconf->timing.base_timing;
	dft = pconf->timing.dft_timing;
	if (!base || !dft)
		return;

	printf("support timings:\n");
	for (i = 0; i < pconf->timing.num_timings; i++) {
		dt = pconf->timing.timings[i];
		if (base == dt)
			base_id = i;
		printf("dt[%d]:%s%s\n", i, base == dt ? "(base)" : "", dft == dt ? "(dft)" : "");
		lcd_detail_timing_print(pdrv, dt);
	}

	dt = &pconf->timing.act_timing;
	printf("active timing: based dt[%d]:\n", base_id);
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

		printf("  %d: type=%d, index=%d, value=%d, delay=%d\n",
		       i, power_step->type, power_step->index,
		       power_step->value, power_step->delay);
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

#ifdef CONFIG_AML_LCD_TABLET
static void lcd_info_print_mipi(struct aml_lcd_drv_s *pdrv)
{
	lcd_dsi_info_print(&pdrv->config);
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

static void lcd_phy_print(struct aml_lcd_drv_s *pdrv)
{
	struct phy_attr_s *phy;
	struct phy_config_s *phy_cfg = &pdrv->config.phy_cfg;
	int base_id = 0, i;

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
	case LCD_VBYONE:
	case LCD_MLVDS:
	case LCD_P2P:
	case LCD_EDP:
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

static void lcd_reg_print_vbyone(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg, offset;

	offset = pdrv->data->offset_venc_if[pdrv->index];

	printf("\nvbyone registers:\n");
	reg = VBO_STATUS_L + offset;
	printf("VX1_STATUS          [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_CTRL_L + offset;
	printf("VBO_CTRL            [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
	reg = VBO_GCLK_MAIN + offset;
	printf("VBO_GCLK_MAIN       [0x%04x] = 0x%08x\n",
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
	printf("LCD_PORT_SWAP       [0x%04x] = 0x%08x\n",
	       reg, lcd_vcbus_read(reg));
}

#ifdef CONFIG_AML_LCD_TCON
static void lcd_reg_print_tcon(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg;

	printf("\ntcon registers:\n");
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
}
#endif

#ifdef CONFIG_AML_LCD_TABLET
static void lcd_reg_print_mipi(struct aml_lcd_drv_s *pdrv)
{
	struct reg_name_set_s reg_table[] = {
		{MIPI_DSI_TOP_CNTL,            "TOP_CNTL"},
		{MIPI_DSI_TOP_CLK_CNTL,        "TOP_CLK_CNTL"},
		{MIPI_DSI_DWC_PWR_UP_OS,       "DWC_PWR_UP_OS"},
		{MIPI_DSI_DWC_PCKHDL_CFG_OS,   "DWC_PCKHDL_CFG_OS"},
		{MIPI_DSI_DWC_LPCLK_CTRL_OS,   "DWC_LPCLK_CTRL_OS"},
		{MIPI_DSI_DWC_CMD_MODE_CFG_OS, "DWC_CMD_MODE_CFG_OS"},
		{MIPI_DSI_DWC_VID_MODE_CFG_OS, "DWC_VID_MODE_CFG_OS"},
		{MIPI_DSI_DWC_VID_PKT_SIZE_OS, "DWC_VID_PKT_SIZE_OS"},
		{MIPI_DSI_DWC_VID_NUM_CHUNKS_OS, "DWC_VID_NUM_CHUNKS_OS"},
		{MIPI_DSI_DWC_VID_NULL_SIZE_OS, "DWC_VID_NULL_SIZE_OS"},
		{MIPI_DSI_DWC_MODE_CFG_OS,     "DWC_MODE_CFG_OS"},
		{MIPI_DSI_DWC_PHY_STATUS_OS,   "DWC_PHY_STATUS_OS"},
		{MIPI_DSI_DWC_INT_ST0_OS,      "DWC_INT_ST0_OS"},
		{MIPI_DSI_DWC_INT_ST1_OS,      "DWC_INT_ST1_OS"},
		{MIPI_DSI_TOP_STAT,            "TOP_STAT"},
		{MIPI_DSI_TOP_INTR_CNTL_STAT,  "TOP_INTR_CNTL_STAT"},
		{MIPI_DSI_TOP_MEM_PD,          "TOP_MEM_PD"},
	};

	printf("\nmipi_dsi registers:\n");
	str_add_reg_sets(pdrv, LCD_REG_DBG_MIPIHOST_BUS, 0, reg_table, ARRAY_SIZE(reg_table));
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
		pdrv->config.timing.act_timing.lcd_bits, pconf->timing.ppc,
		pconf->timing.act_timing.h_active, pconf->timing.act_timing.v_active,
		(sync_duration / 100), (sync_duration % 100),
		pconf->timing.act_timing.pixel_clk, pconf->timing.enc_clk,
		(pconf->timing.clk_mode ? "independence" : "dependence"),
		pconf->timing.clk_mode, pconf->timing.ss_level,
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
static struct lcd_debug_info_s lcd_debug_info_t5m = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

	.reg_dump_lvds   = lcd_reg_print_lvds,
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#ifdef CONFIG_AML_LCD_TABLET
	.reg_dump_mipi   = NULL,
	.reg_dump_edp    = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = lcd_reg_print_tcon,
#endif
};

static struct lcd_debug_info_s lcd_debug_info_txhd2 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_txdh2,

	.reg_dump_lvds   = lcd_reg_print_lvds,
	.reg_dump_vbyone = NULL,
#ifdef CONFIG_AML_LCD_TABLET
	.reg_dump_mipi   = NULL,
	.reg_dump_edp    = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = NULL,
#endif
};

static struct lcd_debug_info_s lcd_debug_info_t3x_0 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

	.reg_dump_lvds   = lcd_reg_print_lvds,
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#ifdef CONFIG_AML_LCD_TABLET
	.reg_dump_mipi   = NULL,
	.reg_dump_edp    = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = lcd_reg_print_tcon,
	.reg_dump_p2p    = lcd_reg_print_tcon,
#endif
};

static struct lcd_debug_info_s lcd_debug_info_t3x_1 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_t3,

	.reg_dump_lvds   = NULL,
	.reg_dump_vbyone = lcd_reg_print_vbyone,
#ifdef CONFIG_AML_LCD_TABLET
	.reg_dump_mipi   = NULL,
	.reg_dump_edp    = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = NULL,
	.reg_dump_p2p    = NULL,
#endif
};

static struct lcd_debug_info_s lcd_debug_info_c3 = {
	.reg_pinmux_table = lcd_reg_dump_pinmux_c3,

	.reg_dump_lvds   = NULL,
	.reg_dump_vbyone = NULL,
#ifdef CONFIG_AML_LCD_TABLET
	.reg_dump_mipi   = lcd_reg_print_mipi,
	.reg_dump_edp    = NULL,
#endif
#ifdef CONFIG_AML_LCD_TCON
	.reg_dump_mlvds  = NULL,
	.reg_dump_p2p    = NULL,
#endif
};

void lcd_debug_probe(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_debug_info_s *lcd_debug_info = NULL;

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T5M:
	case LCD_CHIP_T6D:
		lcd_debug_info = &lcd_debug_info_t5m;
		break;
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

	case LCD_CHIP_TXHD2:
		lcd_debug_info = &lcd_debug_info_txhd2;
		break;
	case LCD_CHIP_C3:
		lcd_debug_info = &lcd_debug_info_c3;
		break;
	default:
		lcd_debug_info = NULL;
		return;
	}

	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_debug_info->interface_print = lcd_info_print_lvds;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_lvds;
		break;
	case LCD_VBYONE:
		lcd_debug_info->interface_print = lcd_info_print_vbyone;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_vbyone;
		break;
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_MIPI:
		lcd_debug_info->interface_print = lcd_info_print_mipi;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_mipi;
		break;
	case LCD_EDP:
		lcd_debug_info->interface_print = lcd_info_print_edp;
		lcd_debug_info->reg_dump_interface = lcd_debug_info->reg_dump_edp;
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
