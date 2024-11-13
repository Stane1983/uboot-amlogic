// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <asm/arch/mailbox.h>

#define MBSPI_DEFAULT_BUS	1
#define MBSPI_DEFAULT_CS	0

struct SpiBoardInfo {
	uint32_t platform_data;
	uint16_t bus_num;
	uint16_t chip_select;
	uint32_t max_speed_hz;
	int32_t latency;
	uint16_t mode;
	uint8_t bits_per_word;
};

struct SpiTransfer {
	uint32_t tx_buf;
	uint32_t rx_buf;
	uint32_t len;
	uint32_t cs_change:1;
	uint16_t delay_usecs;
	uint32_t time_consump;
};

struct MbSpiTransfer {
	uint8_t bus_num;
	uint8_t chip_select;
	uint8_t use_dma;
	struct SpiTransfer xfer;
};

static inline void mbspi_invalidate_dcache(void *buf, int len)
{
	flush_dcache_range((unsigned long)buf, (unsigned long)buf + len);
}

static inline void mbspi_flush_dcache(void *buf, int len)
{
	invalidate_dcache_range((unsigned long)buf, (unsigned long)buf + len);
}

static int xSpiMemCmp(void *src1, void *src2, int len)
{
	u8 *d1 = (u8 *)src1;
	u8 *d2 = (u8 *)src2;
	int i, diff = 0;

	if (!d1 || !d2) {
		printf("\tnull pointer, total %d\n", len);
		return 0;
	}

	for (i = 0; i < len; i++) {
		if (*d1 != *d2) {
			printf("\t%d: 0x%x, 0x%x\n", i, *d1, *d2);
			diff++;
		}
		d1++;
		d2++;
	}

	return diff;
}

static void vSpiMemSet(void *dest, u8 val, int len, u8 step)
{
	u8 *d = (u8 *)dest;
	int i;

	for (i = 0; i < len; i++) {
		*d++ = val & 0xff;
		val += step;
	}
}

static int __mbspi_dev(struct SpiBoardInfo *info)
{
	int ret;

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SPI_DEV,
			     info, sizeof(*info),
			     NULL, 0);
	return ret;
}

static int __mbspi_xfer(int bus_num, int chip_select, bool use_dma,
			void *tx_buf, void *rx_buf, int len)
{
	void *buf, *send_buf, *recv_buf;
	uint32_t send_size, recv_size;
	struct MbSpiTransfer *mbxfer;
	struct SpiTransfer *t;
	int xfer_ret, ret;

	send_size = sizeof(*mbxfer) + ((!use_dma && tx_buf) ? len : 0);
	recv_size = sizeof(xfer_ret) + ((!use_dma && rx_buf) ? len : 0);
	buf = kcalloc(send_size + recv_size, 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	send_buf = buf;
	recv_buf = (uint8_t *)send_buf + send_size;
	mbxfer = (struct MbSpiTransfer *)send_buf;
	mbxfer->bus_num = bus_num;
	mbxfer->chip_select = chip_select;
	mbxfer->use_dma = use_dma;
	t = &mbxfer->xfer;
	t->tx_buf = (uint32_t)(uint64_t)tx_buf;
	t->rx_buf = (uint32_t)(uint64_t)rx_buf;
	t->len = len;

	if (use_dma) {
		if (tx_buf)
			mbspi_flush_dcache(tx_buf, len);
		if (rx_buf)
			mbspi_flush_dcache(rx_buf, len);
	} else if (tx_buf) {
		memcpy(&mbxfer[1], tx_buf, len);
	}

	ret = scpi_send_data(AOCPU_REE_CHANNEL, CMD_SPI_XFER,
			     send_buf, send_size,
			     recv_buf, recv_size);

	xfer_ret = *(int *)recv_buf;
	if (use_dma) {
		if (tx_buf)
			mbspi_invalidate_dcache(tx_buf, len);
		if (rx_buf)
			mbspi_invalidate_dcache(rx_buf, len);
	} else if (rx_buf) {
		memcpy(rx_buf, (uint8_t *)recv_buf + sizeof(xfer_ret), len);
	}

	kfree(buf);

	return ret ? ret : xfer_ret;
}

int do_mbspi_dev(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct SpiBoardInfo *info;
	char  *cp;
	struct SpiBoardInfo mbspi_info = {
		.platform_data = 0,
		.bus_num = MBSPI_DEFAULT_BUS,
		.chip_select = MBSPI_DEFAULT_CS,
		.max_speed_hz = 10000000,
		.mode = SPI_MODE_3 | SPI_LOOP,
		.bits_per_word = 8,
		.latency = 0,
	};

	info = &mbspi_info;
	if (argc >= 2) {
		info->bus_num = simple_strtoul(argv[1], &cp, 10);
		if (*cp == ':')
			info->chip_select = simple_strtoul(cp + 1, &cp, 10);
		if (*cp == '.')
			info->mode = simple_strtoul(cp + 1, &cp, 16);
		if (*cp == '.')
			info->max_speed_hz = simple_strtoul(cp + 1, &cp, 10);
		if (*cp == '.')
			info->bits_per_word = simple_strtoul(cp + 1, &cp, 10);
	}
	if (argc >= 3)
		info->latency = simple_strtoul(argv[2], NULL, 10);

	__mbspi_dev(info);

	return 0;
}

int do_mbspi_xfer(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint8_t bus_num = MBSPI_DEFAULT_BUS;
	uint8_t cs = MBSPI_DEFAULT_CS;
	uint8_t use_dma = 0;
	int len = 32;
	uchar *tx_buf, *rx_buf;
	uchar val = 1;
	char  *cp;
	int diff;
	int ret;

	if (argc >= 2) {
		bus_num = simple_strtoul(argv[1], &cp, 10);
		if (*cp == ':')
			cs = simple_strtoul(cp + 1, &cp, 10);
		if (*cp == '.')
			use_dma = !!simple_strtoul(cp + 1, &cp, 16);
	}

	if (argc >= 3)
		len = simple_strtoul(argv[2], NULL, 10);

	if (argc >= 4)
		val = simple_strtoul(argv[3], NULL, 16);

	tx_buf = kcalloc(len, 2, GFP_KERNEL);
	if (!tx_buf) {
		printf("%s: alloc failed\n", __func__);
		return -ENOMEM;
	}

	rx_buf = tx_buf + len;
	vSpiMemSet(tx_buf, val, len, 1);
	ret = __mbspi_xfer(bus_num, cs, use_dma, tx_buf, rx_buf, len);
	if (ret) {
		printf("mbspi xfer failed %d\n", ret);
	} else {
		diff = xSpiMemCmp((void *)tx_buf, (void *)rx_buf, len);
		printf("total %d, diff %d\n", len, diff);
	}

	kfree(tx_buf);

	return 0;
}

/***************************************************/
U_BOOT_CMD(mbspi_dev, 3, 1, do_mbspi_dev,
	"Mailbox SPI utility command",
	"[<bus>:]<cs>[.<mode>.<speed>.<bw>] [latency] - New a spi device\n"
);

U_BOOT_CMD(mbspi_xfer, 4, 1, do_mbspi_xfer,
	"Mailbox SPI utility command",
	"[<bus>:]<cs>[.<use_dma>] [<len> <dout>] - Send and receive bits\n"
	"<len>     - bytes send (base 10)\n"
	"<dout>    - Hexadecimal string that gets sent"
);
