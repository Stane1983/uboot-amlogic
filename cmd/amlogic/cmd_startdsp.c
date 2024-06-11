// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/amlogic/arch/register.h>
#include <asm/amlogic/arch/bl31_apis.h>
#include <asm/amlogic/arch/pwr_ctrl.h>
#include <serial.h>

// --------------------------------------------------
//                    set_dsp_clk
// --------------------------------------------------

/*
 * set_dsp_clk
 * freq_sel:
 *      [ for S6 ]
 *           0:   1000MHz  gp2_pll
 *           1:    800MHz  fclk_div2p5
 *           2: 666.67MHz  fclk_div3
 *           3:     24MHz  oscin
 *           4:    500MHz  fclk_div4
 *           5:    400MHz  fclk_div2p5/2
 *           6:    333MHz  fclk_div3/2
 *           7:    250MHz  fclk_div4/2
 *           8:    200MHz  fclk_div5/2
 *           9:    100MHz  fclk_div5/4
 *          10: 245.76MHz  hifi_pll/2
 *     default:    800MHz  fclk_div2p5
 */

int set_dsp_clk(uint32_t id, uint32_t freq_sel)
{
	uint32_t control;
	uint32_t clk_sel;
	uint32_t clk_div;
	uint32_t addr = 0;
	char *dsp_clk_tag = NULL;
	int ret = 0;

	switch (id) {
	case 0:
		addr = CLKCTRL_DSPA_CLK_CTRL0;
		break;
	default:
		ret = -1;
		break;
	}

	if (ret) {
		printf("dsp clk set failed: invalid id %d\n", id);
		return ret;
	}

	// Make sure not busy from last setting and we currently match the last setting

	control = readl(addr);
	printf("CLKCTRL_DSP_CLK_CTRL0 = %x\n", readl(addr));

	switch (freq_sel) {
	case 0:
		clk_sel = 4;
		clk_div = 0;
		dsp_clk_tag = "gp2_pll:1000MHz";
		break;
	case 1:
		clk_sel = 1;
		clk_div = 0;
		dsp_clk_tag = "fclk_div2p5:800MHz";
		break;
	case 2:
		clk_sel = 2;
		clk_div = 0;
		dsp_clk_tag = "fclk_div3:666.67MHz";
		break;
	case 3:
		clk_sel = 0;
		clk_div = 0;
		dsp_clk_tag = "oscin:24MHz";
		break;
	case 4:
		clk_sel = 5;
		clk_div = 0;
		dsp_clk_tag = "fclk_div4:500MHz";
		break;
	case 5:
		clk_sel = 3;
		clk_div = 0;
		dsp_clk_tag = "fclk_div5:400MHz";
		break;
	case 6:
		clk_sel = 2;
		clk_div = 1;
		dsp_clk_tag = "fclk_div3/2:333MHz";
		break;
	case 7:
		clk_sel = 5;
		clk_div = 1;
		dsp_clk_tag = "fclk_div4/2:250MHz";
		break;
	case 8:
		clk_sel = 3;
		clk_div = 1;
		dsp_clk_tag = "fclk_div2p5/2:200MHz";
		break;
	case 9:
		clk_sel = 3;
		clk_div = 3;
		dsp_clk_tag = "fclk_div5/4:100MHz";
		break;
	case 10:
		clk_sel = 6;
		clk_div = 1;
		dsp_clk_tag = "hifi_pll/2:245.76MHz";
		break;
	default:
		clk_sel = 1;
		clk_div = 0;
		dsp_clk_tag = "fclk_div2p5:800MHz (default)";
		break;
	}

	printf("CLK_UTIL:dsp[%d]:%s\n", id, dsp_clk_tag);

	if (control & (1 << 15)) {   //if sync_mux ==1, sel mux 0
		control = (control & ~((1 << 15) | (0x3ff << 0) | (0x7 << 10))) |
			(1 << 13) | (1 << 29) | (clk_div << 0) | (clk_sel << 10);
	} else {
		control = (control & ~((1 << 15) | (0x3ff << 16) | (0x7 << 26))) |
			(1 << 13) | (1 << 29) | (clk_div << 16) | (clk_sel << 26) | (1 << 15);
	}

	writel(control, addr);

	if (id == 0)
		printf("CLKCTRL_DSPA_CLK_CTRL0 = %x\n", readl(addr));

	return ret;
}

static int do_startdsp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t dspid;
	uint32_t id;
	uint32_t addr;
	uint32_t freq_sel;
	uint32_t cfg0;
	uint32_t StatVectorSel;
	uint32_t strobe = 1;

	if (argc <= 1) {
		printf("please input dsp boot args: id, address, clk!\n");
		return CMD_RET_USAGE;
	}

	dspid = simple_strtoul(argv[1], NULL, 16);
	if (dspid > 1) {
		printf("start dsp failed: invalid id\n");
		return -1;
	}

	addr = simple_strtoul(argv[2], NULL, 16);
	freq_sel = simple_strtoul(argv[3], NULL, 16);

	printf("dsp%d boot\n", dspid);
	printf("dspboot start address: 0x%x\n", addr);
	printf("dsp clk num:%d\n", freq_sel);

	StatVectorSel = (addr != 0xfffa0000);
	cfg0 = 0x1 | StatVectorSel << 1 | strobe << 2;

	id = PACK_SMC_SUBID_ID(HIFI_DSP_PWR_SET, PDID_DSPA);
	power_set_dsp(id, PWR_ON);

	udelay(100);

	if (set_dsp_clk(dspid, freq_sel))
		return -1;

	udelay(100);

	id = PACK_SMC_SUBID_ID(HIFI_DSP_BOOT, dspid);
	init_dsp(id, addr, cfg0);
	printf("dsp init over!\n");

	return 0;
}

U_BOOT_CMD(
	startdsp, 4, 1, do_startdsp,
	"load dspboot.bin from address",
	"\narg[0]: cmd\n"
	"arg[1]: dspid\n"
	"arg[2]: dspboot.bin load address!\n"
	"arg[3]: dsp clk set\n 0:1G 1:800M 2:667M 3:24M 4:500M 5:400M 6:333M 7:250M 8:200M 9:100M 10:245.76M\n"
);

