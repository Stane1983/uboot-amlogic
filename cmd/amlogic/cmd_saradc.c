// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <dm/uclass.h>
#include <amlogic/saradc.h>

#define ENV_SARADC_VALUE "saradc_val"

struct meson_saradc_mode {
	unsigned int sample_mode;
	const char *mode_name;
};

static const struct meson_saradc_mode mode_table[] = {
	{ADC_MODE_AVERAGE,		"average"}, /* default mode */
	{ADC_MODE_AVERAGE,		"average"},
	{ADC_MODE_HIGH_PRECISION,	"high precision"},
	{ADC_MODE_HIGH_RESOLUTION,	"high resolution"},
	{ADC_MODE_DECIM_FILTER,		"decim filter"}
};

static const char * const test_voltage[] = {
	"gnd",
	"vdd/4",
	"vdd/2",
	"vdd*3/4",
	"vdd",
};

static int current_channel = -1;
static unsigned int current_mode;

static int do_saradc_open(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct udevice *dev;
	int channel;
	int mode = ADC_MODE_AVERAGE;
	int ret;
	char *endp = NULL;

	if (argc < 2) {
		pr_err("Invalid parameter: channel number must be specified\n");
		return -1;
	}

	ret = uclass_get_device_by_name(UCLASS_ADC, "adc", &dev);
	if (ret)
		return ret;

	channel = simple_strtoul(argv[1], NULL, 10);

	if (argc >= 3)
		mode = simple_strtoul(argv[2], &endp, 10);

	if ((channel < 0) || (channel >= MESON_SARADC_CH_MAX)) {
		pr_err("No such channel(%d) in SARADC! open failed!\n",
				channel);
		return -1;
	}

	if ((mode < 0) || (mode >= sizeof(mode_table)/sizeof(mode_table[0])) ||
			(endp && !mode)) {
		pr_err("No such mode(%d) in SARADC! open failed!\n", mode);
		return -1;
	}

	ret = adc_set_mode(dev, channel, mode_table[mode].sample_mode);
	if (ret) {
		pr_err("current platform does not support [%s] mode\n",
				mode_table[mode].mode_name);
		return ret;
	}

	current_mode = mode_table[mode].sample_mode;
	current_channel = channel;

	printf("SARADC mode is %s\n", mode_table[mode].mode_name);

	return 0;
}

static int do_saradc_close(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	current_channel = -1;

	printf("SARADC closed.\n");

	return 0;
}

#include <asm/io.h>
#include <asm/amlogic/arch/timer.h>

#define SAR_ADC_CLK                                0xfe00017c
#define SAR_ADC_RST                                0xfe002008

#define SAR_ADC_REG0                               0xfe026000
#define SAR_ADC_CHAN_LIST                          0xfe026004
#define SAR_ADC_AVG_CNTL                           0xfe026008
#define SAR_ADC_REG3                               0xfe02600c
#define SAR_ADC_DELAY                              0xfe026010
#define SAR_ADC_LAST_RD                            0xfe026014
#define SAR_ADC_FIFO_RD                            0xfe026018
#define SAR_ADC_AUX_SW                             0xfe02601c
#define SAR_ADC_CHAN_10_SW                         0xfe026020
#define SAR_ADC_DETECT_IDLE_SW                     0xfe026024
#define SAR_ADC_DELTA_10                           0xfe026028
#define SAR_ADC_REG11                              0xfe02602c
#define SAR_ADC_REG12                              0xfe026030
#define SAR_ADC_REG13                              0xfe026034
#define SAR_ADC_CHNL01                             0xfe026038
#define SAR_ADC_CHNL23                             0xfe02603c
#define SAR_ADC_CHNL45                             0xfe026040
#define SAR_ADC_CHNL67                             0xfe026044
#define SAR_ADC_RDY                                0xfe026080

static int do_dump_reg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[%08x] SAR_ADC_CLK            = %08x\n", SAR_ADC_CLK            , readl(SAR_ADC_CLK           ));
	printf("[%08x] SAR_ADC_REG0           = %08x\n", SAR_ADC_REG0           , readl(SAR_ADC_REG0           ));
	printf("[%08x] SAR_ADC_CHAN_LIST      = %08x\n", SAR_ADC_CHAN_LIST      , readl(SAR_ADC_CHAN_LIST      ));
	printf("[%08x] SAR_ADC_AVG_CNTL       = %08x\n", SAR_ADC_AVG_CNTL       , readl(SAR_ADC_AVG_CNTL       ));
	printf("[%08x] SAR_ADC_REG3           = %08x\n", SAR_ADC_REG3           , readl(SAR_ADC_REG3           ));
	printf("[%08x] SAR_ADC_DELAY          = %08x\n", SAR_ADC_DELAY          , readl(SAR_ADC_DELAY          ));
	printf("[%08x] SAR_ADC_LAST_RD        = %08x\n", SAR_ADC_LAST_RD        , readl(SAR_ADC_LAST_RD        ));
	printf("[%08x] SAR_ADC_FIFO_RD        = %08x\n", SAR_ADC_FIFO_RD        , readl(SAR_ADC_FIFO_RD        ));
	printf("[%08x] SAR_ADC_AUX_SW         = %08x\n", SAR_ADC_AUX_SW         , readl(SAR_ADC_AUX_SW         ));
	printf("[%08x] SAR_ADC_CHAN_10_SW     = %08x\n", SAR_ADC_CHAN_10_SW     , readl(SAR_ADC_CHAN_10_SW     ));
	printf("[%08x] SAR_ADC_DETECT_IDLE_SW = %08x\n", SAR_ADC_DETECT_IDLE_SW , readl(SAR_ADC_DETECT_IDLE_SW ));
	printf("[%08x] SAR_ADC_DELTA_10       = %08x\n", SAR_ADC_DELTA_10       , readl(SAR_ADC_DELTA_10       ));
	printf("[%08x] SAR_ADC_REG11          = %08x\n", SAR_ADC_REG11          , readl(SAR_ADC_REG11          ));
	printf("[%08x] SAR_ADC_REG12          = %08x\n", SAR_ADC_REG12          , readl(SAR_ADC_REG12          ));
	printf("[%08x] SAR_ADC_REG13          = %08x\n", SAR_ADC_REG13          , readl(SAR_ADC_REG13          ));
	printf("[%08x] SAR_ADC_CHNL01         = %08x\n", SAR_ADC_CHNL01         , readl(SAR_ADC_CHNL01         ));
	printf("[%08x] SAR_ADC_CHNL23         = %08x\n", SAR_ADC_CHNL23         , readl(SAR_ADC_CHNL23         ));
	printf("[%08x] SAR_ADC_CHNL45         = %08x\n", SAR_ADC_CHNL45         , readl(SAR_ADC_CHNL45         ));
	printf("[%08x] SAR_ADC_CHNL67         = %08x\n", SAR_ADC_CHNL67         , readl(SAR_ADC_CHNL67         ));
	printf("[%08x] SAR_ADC_RDY            = %08x\n", SAR_ADC_RDY            , readl(SAR_ADC_RDY            ));
	printf("\n");

	return 0;
}

struct samples {
	uint32_t cost_time;
	u32 val;
};

static int do_read_fifo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int count = 1, n, i;
	int timeout = 1000;
	int show_time = 0;
	uint32_t last_time;
	uint32_t cur_time;
	uint32_t cost_time;
	struct samples *buffer;

	if (argc >= 2)
		count = simple_strtoul(argv[1], NULL, 10);
	if (argc >= 3)
		show_time = simple_strtoul(argv[2], NULL, 10);
	if (argc >= 4)
		timeout = simple_strtoul(argv[3], NULL, 10);
	printf("count = %d\n", count);
	printf("timeout = %dms\n", timeout);
	printf("show_time = %d(%s)\n", show_time, show_time ? "on" : "off");
	printf("\n");
	// ms to ns
	timeout *= 1000;

	buffer = (struct samples *)malloc(sizeof(struct samples) * count);
	n = 0;
	last_time = get_time(); // ns

	do {
		cur_time = get_time();
		cost_time = cur_time - last_time;
		// read fifo
		if (readl(SAR_ADC_REG0) & GENMASK(25, 21)) {
			buffer[n].val = readl(SAR_ADC_FIFO_RD) & 0xfff;
			buffer[n].cost_time = cost_time;
			last_time = cur_time;
			n++;
		}
	} while (cost_time < timeout && n < count);

	for (i = 0; i < n; i++) {
		if (show_time)
			printf("[+%8u ns] ", buffer[i].cost_time);
		printf("%u\n", buffer[i].val);
	}

	if (cost_time >= timeout)
		printf("\nA timeout occurred during the read process, and the specified amount of data was not read.");
	printf("\nDone [%d/%d]\n", n, count);
	printf("\n");

	free(buffer);

	return 0;
}

static int do_config_def(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	// RST
	// writel(0x00040000, SAR_ADC_RST);

	// CLK
	writel(0x00000119, SAR_ADC_CLK);

	// Analog Reset
	writel(0x00000000, SAR_ADC_REG3);

	writel(0x00000004, SAR_ADC_CHAN_LIST);
	writel(0x00000000, SAR_ADC_AVG_CNTL);
	writel(0xd0284c0a, SAR_ADC_REG3);
	writel(0x010a000a, SAR_ADC_DELAY);
	writel(0x03eb1a0c, SAR_ADC_AUX_SW);
	writel(0x03800380, SAR_ADC_CHAN_10_SW);
	writel(0x03800380, SAR_ADC_DETECT_IDLE_SW);
	writel(0x00000000, SAR_ADC_DELTA_10);
	writel(0x00000000, SAR_ADC_REG11);
	writel(0x00000000, SAR_ADC_REG12);
	writel(0x00000000, SAR_ADC_REG13);

	writel(0x04004040, SAR_ADC_REG0);
	writel(0x04004041, SAR_ADC_REG0);
	writel(0x04004045, SAR_ADC_REG0);

	printf("Done\n");
	printf("\n");

	return 0;
}

static int do_saradc_getval(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	char value_str[20];
	unsigned int val;
	int ret;

	if (current_channel < 0) {
		pr_err("SARADC channel[%d] is invalid\n", current_channel);
		return -EINVAL;
	};
	memset(value_str, 0, sizeof(value_str));

	ret = adc_channel_single_shot_mode("adc", current_mode,
					   current_channel, &val);
	if (ret)
		return ret;

	printf("SARADC channel(%d) is %d.\n", current_channel, val);

	sprintf(value_str, "0x%x", val);

	env_set(ENV_SARADC_VALUE, value_str);

	return 0;
}

static int do_saradc_test(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	struct udevice *dev;
	unsigned int val;
	int ret;
	int i;
	int channel;

	ret = uclass_get_device_by_name(UCLASS_ADC, "adc", &dev);
	if (ret)
		return ret;

	ret = adc_get_test_channel(dev);
	if (ret < 0)
		return ret;
	channel = ret;

	ret = adc_set_mode(dev, channel, ADC_MODE_AVERAGE);
	if (ret)
		return ret;

	printf("saradc self-test by channel %d:\n", channel);

	for (i = 0; i < ARRAY_SIZE(test_voltage); i++) {
		ret = adc_select_input_voltage(dev, channel, i);
		if (ret)
			return ret;

		udelay(10);

		ret = adc_start_channel(dev, channel);
		if (ret)
			return ret;

		ret = adc_channel_data(dev, channel, &val);
		if (ret)
			return ret;

		printf("%-7s : %d\n", test_voltage[i], val);
	}

	return 0;
}

static int do_saradc_get_in_range(cmd_tbl_t *cmdtp, int flag,
		int argc, char * const argv[])
{
	char value_str[20];
	int max, min;
	unsigned int val;
	int ret;

	ret = adc_channel_single_shot_mode("adc", current_mode,
					   current_channel, &val);
	if (ret)
		return ret;

	memset(value_str, 0, sizeof(value_str));
	min = simple_strtoul(argv[1], NULL, 10);
	max = simple_strtoul(argv[2], NULL, 10);
	int donot_setenv = 0;
	if (argc > 3) {
		donot_setenv = simple_strtoul(argv[2], NULL, 10);
	}
	if ((val < min) || (val > max)) {
		debug("SARADC channel(%d) is %d, Out of range(%d~%d)!\n",
			current_channel, val, min, max);
		return -1;
	}
	debug("SARADC channel(%d) is %d (%d~%d).\n",
		current_channel, val, min, max);
	sprintf(value_str, "0x%x", val);
	if (!donot_setenv)
		env_set(ENV_SARADC_VALUE, value_str);

	return 0;
}

static cmd_tbl_t cmd_saradc_sub[] = {
	U_BOOT_CMD_MKENT(open, 3, 0, do_saradc_open, "", ""),
	U_BOOT_CMD_MKENT(close, 1, 0, do_saradc_close, "", ""),
	U_BOOT_CMD_MKENT(getval, 1, 0, do_saradc_getval, "", ""),
	U_BOOT_CMD_MKENT(test, 1, 0, do_saradc_test, "", ""),
	U_BOOT_CMD_MKENT(get_in_range, 3, 0, do_saradc_get_in_range, "", ""),
	U_BOOT_CMD_MKENT(dump_reg, 1, 0, do_dump_reg, "", ""),
	U_BOOT_CMD_MKENT(read_fifo, 4, 0, do_read_fifo, "", ""),
	U_BOOT_CMD_MKENT(config_def, 1, 0, do_config_def, "", ""),
};

static int do_saradc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	/* Strip off leading 'bmp' command argument */
	argc--;
	argv++;
	c = find_cmd_tbl(argv[0], &cmd_saradc_sub[0],
				ARRAY_SIZE(cmd_saradc_sub));
	if (c) {
		return	c->cmd(cmdtp, flag, argc, argv);
	} else {
		cmd_usage(cmdtp);
		return 1;
	}
}

U_BOOT_CMD(
	saradc,	CONFIG_SYS_MAXARGS, 0, do_saradc,
	"saradc sub-system",
	"saradc open <channel> <mode> - open a SARADC channel\n"
	"       mode: 1: average\n"
	"             2: high precision\n"
	"             3: high resolution\n"
	"             4: decim filter\n"
	"saradc close  - close the SARADC\n"
	"saradc getval - get the value in current channel\n"
	"saradc test   - test the SARADC by channel-7\n"
	"saradc get_in_range <min> <max>\n"
	"       - return 0 if current value in the range of current channel\n"
	"saradc dump_reg\n"
	"saradc read_fifo <count> <show_time> <timeout>\n"
	"saradc config_def\n"
);
