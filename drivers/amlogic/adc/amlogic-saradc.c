// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2024 Amlogic, Inc. All rights reserved.
 * Author: Huqiang Qin <huqiang.qin@amlogic.com>
 *
 * Amlogic SARADC driver for U-Boot
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <log.h>
#include <adc.h>
#include <clk.h>
#include <reset.h>

#define SARADC_REG0				0x00
#define SARADC_REG0_SAMPLING_STOP		BIT(14)
#define SARADC_REG0_ADC_EN			BIT(9)
#define SARADC_REG0_FIFO_CNT_IRQ_MASK		GENMASK(8, 4)
#define SARADC_REG0_FIFO_IRQ_EN			BIT(3)
#define SARADC_REG0_SAMPLE_START		BIT(2)
#define SARADC_REG0_CONTINUOUS_EN		BIT(1)
#define SARADC_REG0_SAMPLING_ENABLE		BIT(0)

#define SARADC_REG1				0x04
#define SARADC_REG1_MAX_INDEX_MASK		GENMASK(26, 24)
#define SARADC_REG1_ENTRY_SHIFT(_chan)		((_chan) * 3)
#define SARADC_REG1_ENTRY_MASK(_chan)		(GENMASK(2, 0) << ((_chan) * 3))

#define SARADC_REG2				0x08
#define SARADC_REG2_AVG_MODE_SHIFT(_chan)	(16 + ((_chan) * 2))
#define SARADC_REG2_AVG_MODE_MASK(_chan)	(GENMASK(17, 16) << ((_chan) * 2))
#define SARADC_REG2_NUM_SAMPLES_SHIFT(_chan)	(0 + ((_chan) * 2))
#define SARADC_REG2_NUM_SAMPLES_MASK(_chan)	(GENMASK(1, 0) << ((_chan) * 2))

#define SARADC_REG3				0x0c
#define SARADC_REG4				0x10
#define SARADC_REG5				0x14
#define SARADC_REG6				0x18
#define SARADC_REG7				0x1c
#define SARADC_REG8				0x20
#define SARADC_REG9				0x24
#define SARADC_REG10				0x28

#define SARADC_REG11				0x2c
#define SARADC_REG11_CHANNEL7_AUX_CTRL_MASK	GENMASK(27, 25)

#define SARADC_REG12				0x30
#define SARADC_REG13				0x34
#define SARADC_REG14				0x38

#define SARADC_STATUS0				0x3c
#define SARADC_STATUS0_BUSY_MASK		GENMASK(14, 12)
#define SARADC_STATUS0_FIFO_COUNT_MASK		GENMASK(7, 3)

#define SARADC_STATUS1				0x40
#define SARADC_STATUS2				0x44
#define SARADC_STATUS3				0x48
#define SARADC_STATUS3_CHANNEL_ID_MASK		GENMASK(22, 20)

#define SARADC_STATUS4				0x4c
#define SARADC_STATUS5				0x50
#define SARADC_STATUS6				0x54
#define SARADC_STATUS7				0x58
#define SARADC_STATUS8				0x5c
#define SARADC_STATUS9				0x60
#define SARADC_STATUS10				0x64
#define SARADC_STATUS11				0x68
#define SARADC_STATUS12				0x6c
#define SARADC_STATUS13				0x70
#define SARADC_STATUS14				0x74
#define SARADC_STATUS15				0x78
#define SARADC_RDY				0x80

#define SARADC_CHANNEL_MAX			8
#define SARADC_MAX_FIFO_SIZE			16
#define SARADC_SOFT_AVG_NUM			8

#define SARADC_DEFAULT_CLOCK_FREQUENCY		1200000
#define SARADC_DEFAULT_TEST_CHANNEL		7
#define SARADC_DEFAULT_FIFO_DATA_WIDTH		16
#define SARADC_DEFAULT_OUT_DATA_WIDTH		10
#define SARADC_DEFAULT_VREF_VOLTAGE		1800000
#define SARADC_DEFAULT_TIMEOUT			(120 * 1000)

struct amlogic_saradc {
	phys_addr_t base;
	struct clk clk_src;
	struct clk clk_mux;
	struct clk clk_div;
	struct clk clk_gate;
	u32 clock_frequency;
	u32 test_channel;
	u32 fifo_data_width;
	u32 out_data_width;
	bool apply_workaround;
	struct reset_ctl_bulk resets;
	int info_offset;
	int info_scale;
};

enum amlogic_saradc_avg_mode {
	NO_AVERAGING = 0x0,
	MEAN_AVERAGING = 0x1,
	MEDIAN_AVERAGING = 0x2,
};

enum amlogic_saradc_num_samples {
	ONE_SAMPLE = 0x0,
	TWO_SAMPLES = 0x1,
	FOUR_SAMPLES = 0x2,
	EIGHT_SAMPLES = 0x3,
};

static int amlogic_saradc_parse_dt(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	ofnode node = dev_ofnode(dev);
	int ret;

	ret = ofnode_read_u32(node, "amlogic,clock-frequency",
			      &priv->clock_frequency);
	if (ret)
		priv->clock_frequency = SARADC_DEFAULT_CLOCK_FREQUENCY;
	debug("saradc clock-frequency: %u\n", priv->clock_frequency);

	ret = ofnode_read_u32(node, "amlogic,test-channel",
			      &priv->test_channel);
	if (ret)
		priv->test_channel = SARADC_DEFAULT_TEST_CHANNEL;
	debug("saradc test-channel: %u\n", priv->test_channel);

	ret = ofnode_read_u32(node, "amlogic,fifo-data-width",
			      &priv->fifo_data_width);
	if (ret)
		priv->fifo_data_width = SARADC_DEFAULT_FIFO_DATA_WIDTH;
	debug("saradc fifo-data-width: %u\n", priv->fifo_data_width);

	ret = ofnode_read_u32(node, "amlogic,out-data-width",
			      &priv->out_data_width);
	if (ret)
		priv->out_data_width = SARADC_DEFAULT_OUT_DATA_WIDTH;
	debug("saradc out-data-width: %u\n", priv->out_data_width);

	priv->apply_workaround = device_is_compatible(dev, "amlogic,s7d-saradc");

	return 0;
}

static int amlogic_saradc_clk_init(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int ret;

	ret = clk_get_by_name(dev, "src", &priv->clk_src);
	if (ret) {
		dev_err(dev, "failed to get src clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "mux", &priv->clk_mux);
	if (ret) {
		dev_err(dev, "failed to get mux clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "div", &priv->clk_div);
	if (ret) {
		dev_err(dev, "failed to get div clk\n");
		return ret;
	}

	ret = clk_get_by_name(dev, "gate", &priv->clk_gate);
	if (ret) {
		dev_err(dev, "failed to get gate\n");
		return ret;
	}

	ret = clk_set_parent(&priv->clk_mux, &priv->clk_src);
	if (ret) {
		dev_err(dev, "failed to reparent adc clk\n");
		return ret;
	}

	ret = clk_set_rate(&priv->clk_div, priv->clock_frequency);
	if (ret) {
		dev_err(dev, "failed to set rate\n");
		return ret;
	}

	return 0;
}

static void amlogic_saradc_set_averaging(struct amlogic_saradc *priv, int address,
					 enum amlogic_saradc_avg_mode mode,
					 enum amlogic_saradc_num_samples samples)
{
	unsigned int  val;

	val = samples << SARADC_REG2_NUM_SAMPLES_SHIFT(address);
	clrsetbits_le32(priv->base + SARADC_REG2,
			SARADC_REG2_NUM_SAMPLES_MASK(address), val);

	val = mode << SARADC_REG2_AVG_MODE_SHIFT(address);
	clrsetbits_le32(priv->base + SARADC_REG2,
			SARADC_REG2_AVG_MODE_MASK(address), val);
}

static void amlogic_saradc_hw_init(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	ofnode node = dev_ofnode(dev);
	char buf[32];
	u32 reg_value;
	int ret;
	int index;

	/* Load register configuration from DT [REG0 - REG14] */
	for (index = 0; index <= 14; index++) {
		/* Generate attribute names */
		snprintf(buf, sizeof(buf), "amlogic,reg%d-init", index);
		/* If it exists, configure it to the register */
		ret = ofnode_read_u32(node, buf, &reg_value);
		if (!ret)
			writel(reg_value, priv->base + (index << 2));
	}

	/* ADC is disabled by default */
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_STOP,
			SARADC_REG0_SAMPLING_STOP);
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_ENABLE, 0);
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_ADC_EN, 0);

	/* Configure the averaging mode for all channels */
	if (priv->apply_workaround) {
		for (index = 0; index < SARADC_CHANNEL_MAX; index++)
			amlogic_saradc_set_averaging(priv, index, NO_AVERAGING, ONE_SAMPLE);
	} else {
		for (index = 0; index < SARADC_CHANNEL_MAX; index++)
			amlogic_saradc_set_averaging(priv, index, MEDIAN_AVERAGING, EIGHT_SAMPLES);
	}
}

static void amlogic_saradc_clear_fifo(struct amlogic_saradc *priv)
{
	int i;

	for (i = 0; i < SARADC_MAX_FIFO_SIZE; i++) {
		if (!(readl(priv->base + SARADC_STATUS0) &
		      SARADC_STATUS0_FIFO_COUNT_MASK))
			break;
		readl(priv->base + SARADC_STATUS3);
	}
}

static void amlogic_saradc_hw_enable(struct amlogic_saradc *priv)
{
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_ENABLE,
			SARADC_REG0_SAMPLING_ENABLE);

	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_STOP, 0);
}

static void amlogic_saradc_hw_disable(struct amlogic_saradc *priv)
{
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_STOP,
			SARADC_REG0_SAMPLING_STOP);

	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLING_ENABLE, 0);
}

static int amlogic_saradc_set_mode(struct udevice *dev, int ch, unsigned int mode)
{
	/* Only average mode is supported */
	if (mode != ADC_CAPACITY_AVERAGE)
		return -ENOTSUPP;

	return 0;
}

static void amlogic_saradc_enable_channel(struct amlogic_saradc *priv,
					  int address, int index)
{
	u32 regval;

	regval = SARADC_REG1_ENTRY_MASK(index) & (address << (index * 3));
	clrsetbits_le32(priv->base + SARADC_REG1,
			SARADC_REG1_ENTRY_MASK(index), regval);
}

static int amlogic_saradc_start_sample(struct amlogic_saradc *priv)
{
	int ret;

	if (priv->apply_workaround)
		udelay(50);

	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_ADC_EN,
			SARADC_REG0_ADC_EN);

	if (priv->apply_workaround) {
		udelay(200);
		ret = reset_assert_bulk(&priv->resets);
		if (ret)
			goto failed;
		ret = reset_deassert_bulk(&priv->resets);
		if (ret)
			goto failed;
	}

	udelay(20);

	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_SAMPLE_START,
			SARADC_REG0_SAMPLE_START);

	return 0;
failed:
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_ADC_EN, 0);
	return ret;
}

static void amlogic_saradc_stop_sample(struct amlogic_saradc *priv)
{
	clrsetbits_le32(priv->base + SARADC_REG0,
			SARADC_REG0_ADC_EN, 0);
}

static int amlogic_saradc_start_channel(struct udevice *dev, int channel)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);

	amlogic_saradc_enable_channel(priv, channel, 0);

	return 0;
}

static int amlogic_saradc_stop(struct udevice *dev)
{
	/* Do nothing */
	return 0;
}

static int amlogic_saradc_select_input_voltage(struct udevice *dev, int channel,
					       int mux)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	unsigned int regval;

	if (channel != priv->test_channel) {
		dev_err(dev, "channel%d does not support self-test\n", channel);
		return -EINVAL;
	}

	regval = FIELD_PREP(SARADC_REG11_CHANNEL7_AUX_CTRL_MASK, mux);
	clrsetbits_le32(priv->base + SARADC_REG11,
			SARADC_REG11_CHANNEL7_AUX_CTRL_MASK, regval);

	return 0;
}

static int amlogic_saradc_get_test_channel(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);

	return (int)priv->test_channel;
}

static inline int amlogic_saradc_channel_raw_data(struct udevice *dev, int *from_ch,
						  unsigned int *data)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	unsigned int count;
	unsigned int tmp;
	unsigned int mask;

	tmp = readl(priv->base + SARADC_STATUS0);
	count = FIELD_GET(SARADC_STATUS0_FIFO_COUNT_MASK, tmp);

	/* Data ready? */
	if ((tmp & SARADC_STATUS0_BUSY_MASK) || count < 1)
		return -EBUSY;
	if (count != 1)
		dev_warn(dev, "the current fifo data is %u, not 1\n", count);

	tmp = readl(priv->base + SARADC_STATUS3);

	/* Channel data */
	mask = BIT(priv->fifo_data_width) - 1;
	*data = tmp & mask;

	/* Channel id */
	mask = BIT(SARADC_CHANNEL_MAX) - 1;
	*from_ch = FIELD_GET(SARADC_STATUS3_CHANNEL_ID_MASK, tmp) & mask;

	return 0;
}

static int amlogic_saradc_read_raw_data(struct udevice *dev, int channel,
					unsigned int *data)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int ret;
	unsigned int timeout_us = 30000;
	int from_ch;
	unsigned int ch_data;

	amlogic_saradc_clear_fifo(priv);
	ret = amlogic_saradc_start_sample(priv);
	if (ret)
		return ret;

	do {
		ret = amlogic_saradc_channel_raw_data(dev, &from_ch, &ch_data);
		if (!ret || ret != -EBUSY)
			break;
		udelay(1);
	} while (timeout_us--);

	amlogic_saradc_stop_sample(priv);

	if (ret)
		return ret;

	if (channel != from_ch) {
		dev_err(dev, "channel mismatch: required channel is %d, actual channel is %d\n",
			channel, from_ch);
		return -EINVAL;
	}

	*data = ch_data;

	return 0;
}

static int amlogic_saradc_channel_data_once(struct udevice *dev, int channel,
					    unsigned int *data)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int ret;
	unsigned int ch_data;
	short signed_data;
	unsigned int mask;

	ret = amlogic_saradc_read_raw_data(dev, channel, &ch_data);
	if (ret)
		return -EINVAL;

	/* Calibration data */
	signed_data = (short)ch_data;
	signed_data += priv->info_offset;
	signed_data = signed_data < 0 ? 0 : signed_data;
	signed_data = signed_data * priv->info_scale / 1000000;
	mask = BIT(priv->out_data_width) - 1;
	signed_data = signed_data > mask ? mask : signed_data;
	*data = signed_data;

	return 0;
}

static int amlogic_saradc_channel_data(struct udevice *dev, int channel,
				       unsigned int *data)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int sum = 0;
	int ret;
	int count;
	unsigned int tmp;

	if (!priv->apply_workaround)
		return amlogic_saradc_channel_data_once(dev, channel, data);

	for (count = 0; count < SARADC_SOFT_AVG_NUM; count++) {
		ret = amlogic_saradc_channel_data_once(dev, channel, &tmp);
		if (ret)
			return ret;
		sum += (int)(short)tmp;
	}

	*data = (short)(sum / SARADC_SOFT_AVG_NUM);

	return 0;
}

static int amlogic_saradc_self_calib(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int raw_gnd;
	int raw_vref;
	int channel = priv->test_channel;
	int ret;
	unsigned int raw;

	ret = amlogic_saradc_set_mode(dev, channel, ADC_CAPACITY_AVERAGE);
	if (ret)
		return ret;

	/* Set to GND */
	ret = amlogic_saradc_select_input_voltage(dev, channel, 0);
	if (ret)
		return ret;
	udelay(10);
	ret = amlogic_saradc_start_channel(dev, channel);
	if (ret)
		return ret;
	ret = amlogic_saradc_read_raw_data(dev, channel, &raw);
	if (ret < 0) {
		dev_err(dev, "failed: self-calibration sample error\n");
		return ret;
	}
	raw_gnd = (short)raw;

	/* Set to VREF */
	ret = amlogic_saradc_select_input_voltage(dev, channel, 4);
	if (ret)
		return ret;
	udelay(10);
	ret = amlogic_saradc_start_channel(dev, channel);
	if (ret)
		return ret;
	ret = amlogic_saradc_read_raw_data(dev, channel, &raw);
	if (ret < 0) {
		dev_err(dev, "failed: self-calibration sample error\n");
		return ret;
	}
	raw_vref = (short)raw;

	debug("self-calibration: raw_gnd=%d, raw_vref=%d\n", raw_gnd, raw_vref);

	priv->info_offset = -raw_gnd;
	priv->info_scale = (BIT(priv->out_data_width) - 1) * 1000000 /
			   (raw_vref - raw_gnd);

	if (priv->info_scale <= 0) {
		dev_err(dev, "self-calibration failed: gnd=%d, vref=%d\n",
			raw_gnd, raw_vref);
		return -EINVAL;
	}

	debug("self-calibration: offset=%d, scale=%d\n",
	      priv->info_offset, priv->info_scale);

	return 0;
}

int amlogic_saradc_probe(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int ret;

	priv->base = devfdt_get_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE) {
		dev_err(dev, "cannot find saradc base address\n");
		return -EINVAL;
	}

	ret = amlogic_saradc_clk_init(dev);
	if (ret)
		return ret;

	if (priv->apply_workaround) {
		ret = reset_get_bulk(dev, &priv->resets);
		if (ret)
			return ret;
	}

	clk_enable(&priv->clk_gate);
	udelay(5);

	amlogic_saradc_hw_init(dev);

	amlogic_saradc_hw_enable(priv);

	amlogic_saradc_self_calib(dev);

	return 0;
}

int amlogic_saradc_remove(struct udevice *dev)
{
	struct amlogic_saradc *priv = dev_get_priv(dev);

	amlogic_saradc_hw_disable(priv);
	clk_disable(&priv->clk_gate);

	return 0;
}

int amlogic_saradc_of_to_plat(struct udevice *dev)
{
	struct adc_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);
	struct amlogic_saradc *priv = dev_get_priv(dev);
	int ret;

	ret = amlogic_saradc_parse_dt(dev);
	if (ret)
		return ret;

	uc_pdata->data_mask = BIT(priv->out_data_width) - 1;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_timeout_us = SARADC_DEFAULT_TIMEOUT;
	uc_pdata->channel_mask = BIT(SARADC_CHANNEL_MAX) - 1;
	uc_pdata->vdd_microvolts = SARADC_DEFAULT_VREF_VOLTAGE;

	return 0;
}

const struct adc_ops amlogic_saradc_ops = {
	.set_mode		= amlogic_saradc_set_mode,
	.start_channel		= amlogic_saradc_start_channel,
	.channel_data		= amlogic_saradc_channel_data,
	.stop			= amlogic_saradc_stop,
	.select_input_voltage	= amlogic_saradc_select_input_voltage,
	.get_test_channel	= amlogic_saradc_get_test_channel,
};

static const struct udevice_id amlogic_saradc_ids[] = {
	{ .compatible = "amlogic,saradc" },
	{ }
};

U_BOOT_DRIVER(amlogic_saradc) = {
	.name			= "amlogic_saradc",
	.id			= UCLASS_ADC,
	.of_match		= amlogic_saradc_ids,
	.ops			= &amlogic_saradc_ops,
	.probe			= amlogic_saradc_probe,
	.remove			= amlogic_saradc_remove,
	.of_to_plat		= amlogic_saradc_of_to_plat,
	.priv_auto		= sizeof(struct amlogic_saradc),
};
